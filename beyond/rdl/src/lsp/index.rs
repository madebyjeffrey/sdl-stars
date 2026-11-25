use std::collections::BTreeMap;

use serde_json::{Value, json};

use crate::ast::*;
use crate::engine::{substitute, substitutions};
use crate::lexer::{TokenKind, lex};
use crate::parser;

use super::documents::{Document, range, span_offset};

#[derive(Clone)]
struct Token {
    kind: TokenKind,
    start: usize,
    end: usize,
}

#[derive(Clone)]
struct Source {
    text: String,
    tokens: Vec<Token>,
    statements: Vec<Statement>,
    occurrences: Vec<Occurrence>,
    scopes: Vec<Scope>,
    walk_depth: usize,
}

#[derive(Clone)]
struct Symbol {
    name: String,
    detail: String,
    kind: u8,
    uri: String,
    start: usize,
    end: usize,
    ty: Option<Type>,
}

#[derive(Clone, Default)]
struct Resolution {
    symbol: Option<Symbol>,
    ty: Option<Type>,
}

#[derive(Clone)]
struct Occurrence {
    start: usize,
    end: usize,
    resolution: Resolution,
}
#[derive(Clone)]
struct Scope {
    start: usize,
    end: usize,
    locals: BTreeMap<String, Resolution>,
}
type Locals = BTreeMap<String, Resolution>;

#[derive(Default)]
pub(super) struct Index {
    sources: BTreeMap<String, Source>,
    globals: BTreeMap<String, Symbol>,
    definitions: BTreeMap<String, (Definition, Symbol)>,
    constructors: BTreeMap<String, (Vec<Field>, Symbol)>,
    trait_overloads: BTreeMap<(String, usize), (Vec<Field>, Symbol)>,
}

impl Index {
    pub fn import_links(&mut self, links: &[crate::imports::ResolvedImport]) {
        for link in links {
            if let Some(source) = self.sources.get_mut(&link.import.name_span.source) {
                let mut start = span_offset(&source.text, &link.import.name_span);
                for part in link.import.name.split('.') {
                    source
                        .occurrences
                        .retain(|occurrence| occurrence.start != start);
                    let symbol = Symbol {
                        name: link.import.name.clone(),
                        detail: format!("import {}\n{}", link.import.name, link.target),
                        kind: 2,
                        uri: link.target.clone(),
                        start: 0,
                        end: 0,
                        ty: None,
                    };
                    source.record(
                        start,
                        start + part.len(),
                        Resolution {
                            symbol: Some(symbol),
                            ty: None,
                        },
                    );
                    start += part.len() + 1;
                }
            }
        }
    }

    pub fn semantic_role(&self, uri: &str, byte: usize) -> Option<(&'static str, u32)> {
        let source = self.sources.get(uri)?;
        let i = source.tokens.partition_point(|token| token.start < byte);
        if source.tokens.get(i).is_some_and(|token| token.start == byte && matches!(&token.kind, TokenKind::Word(word) if matches!(word.as_str(), "one" | "optional" | "many" | "some")))
            && i > 0 && matches!(&source.tokens[i - 1].kind, TokenKind::Word(word) if matches!(word.as_str(), "requires" | "allows")) {
            return Some(("keyword", 0));
        }
        let occurrence = source.at(byte)?;
        if occurrence.start != byte {
            return None;
        }
        let symbol = occurrence.resolution.symbol.as_ref()?;
        let kind = match symbol.kind {
            2 => "namespace",
            5 => "class",
            11 => "interface",
            23 if symbol.uri.is_empty() => "type",
            23 => "struct",
            7 | 8 => "property",
            22 => "enumMember",
            14 if symbol.detail.starts_with("trait ") => "enumMember",
            3 if matches!(symbol.name.as_str(), "Set" | "List") => "namespace",
            3 => "function",
            _ => "variable",
        };
        let mut flags = if symbol.uri == uri && symbol.start == byte {
            1
        } else {
            0
        };
        if matches!(kind, "variable" | "property" | "enumMember") {
            flags |= 2;
        }
        if symbol.uri.is_empty() {
            flags |= 4;
        }
        Some((kind, flags))
    }

    pub fn build(documents: &BTreeMap<String, Document>) -> Self {
        let mut index = Self::default();
        for (uri, document) in documents {
            let mut source = Source::new(uri, &document.text);
            for statement in &source.statements {
                match statement {
                    Statement::Definition(definition) => {
                        let Some(symbol) =
                            source.declaration(uri, &definition.name, &definition.span, true, None)
                        else {
                            continue;
                        };
                        let mut symbol = symbol;
                        symbol.kind = match definition.kind {
                            TypeKind::Trait => 11,
                            TypeKind::Object => 5,
                            TypeKind::Value => 23,
                        };
                        symbol.detail = format!(
                            "{}type {}{}",
                            match definition.kind {
                                TypeKind::Trait => "trait ",
                                TypeKind::Object => "object ",
                                TypeKind::Value => "",
                            },
                            definition
                                .params
                                .iter()
                                .map(|param| format!("'{param} "))
                                .collect::<String>(),
                            definition.name
                        );
                        if let Some(component) = &definition.component {
                            symbol.kind = 5;
                            symbol.detail = format!(
                                "{}component {}{}",
                                if component.abstract_ { "abstract " } else { "" },
                                definition.name,
                                {
                                    let mut bases: Vec<String> =
                                        component.scalar.iter().map(ToString::to_string).collect();
                                    bases.extend(component.parents.clone());
                                    if bases.is_empty() {
                                        String::new()
                                    } else {
                                        format!(" : {}", bases.join(", "))
                                    }
                                }
                            );
                        }
                        symbol.ty = Some(definition_type(definition));
                        index
                            .definitions
                            .insert(definition.name.clone(), (definition.clone(), symbol));
                    }
                    Statement::Binding(binding) => {
                        let Some(mut symbol) = source.declaration(
                            uri,
                            &binding.name,
                            &binding.span,
                            false,
                            binding.ty.clone(),
                        ) else {
                            continue;
                        };
                        symbol.ty = binding.ty.clone().or_else(|| literal_type(&binding.expr));
                        symbol.kind = if binding.kind == TypeKind::Value {
                            13
                        } else {
                            14
                        };
                        symbol.detail = match (&binding.kind, &symbol.ty) {
                            (TypeKind::Object, Some(ty)) => format!("{ty} {}", binding.name),
                            (TypeKind::Trait, Some(ty)) => format!("trait {} : {ty}", binding.name),
                            (_, Some(ty)) => format!("let {} : {ty}", binding.name),
                            _ => format!("let {}", binding.name),
                        };
                        if let Some(parameters) = &binding.parameters {
                            symbol.kind = 12;
                            symbol.detail = format!(
                                "trait {}({}) : {}",
                                binding.name,
                                parameters
                                    .iter()
                                    .map(|p| format!("{}: {}", p.name, p.ty))
                                    .collect::<Vec<_>>()
                                    .join(", "),
                                binding.ty.as_ref().unwrap()
                            );
                            index.trait_overloads.insert(
                                (binding.name.clone(), parameters.len()),
                                (parameters.clone(), symbol.clone()),
                            );
                        }
                        index.globals.insert(binding.name.clone(), symbol);
                    }
                    _ => {}
                }
            }
            source.occurrences.clear();
            index.sources.insert(uri.clone(), source);
        }
        for symbol in index.globals.values_mut() {
            let overloads: Vec<_> = index
                .trait_overloads
                .iter()
                .filter(|((name, _), _)| name == &symbol.name)
                .map(|(_, (_, symbol))| symbol.detail.clone())
                .collect();
            if !overloads.is_empty() {
                symbol.detail = overloads.join("\n");
            }
        }
        for (definition, symbol) in index.definitions.values() {
            if let Body::Union(alternatives) = &definition.body {
                for alternative in alternatives {
                    let constructor = match alternative {
                        Alternative::Constructor(name, fields) => Some((name, fields.clone())),
                        Alternative::Type(Type::Named(name, args))
                            if args.is_empty()
                                && !index.definitions.contains_key(name)
                                && name != "range" =>
                        {
                            Some((name, Vec::new()))
                        }
                        _ => None,
                    };
                    if let Some((name, fields)) = constructor {
                        let source = &index.sources[&symbol.uri];
                        if let Some(token) = source.tokens.iter().find(|token| {
                            token.start > symbol.start
                                && matches!(&token.kind, TokenKind::Word(word) if word == name)
                        }) {
                            let mut constructor = symbol.clone();
                            constructor.name = name.clone();
                            constructor.start = token.start;
                            constructor.end = token.end;
                            constructor.detail = format!("{name} : {}", definition.name);
                            constructor.kind = 22;
                            index
                                .constructors
                                .insert(name.clone(), (fields, constructor));
                        }
                    }
                }
            }
        }
        let uris: Vec<_> = index.sources.keys().cloned().collect();
        for uri in uris {
            let mut source = index.sources[&uri].clone();
            index.walk_source(&uri, &mut source);
            index.sources.insert(uri, source);
        }
        index
    }

    pub fn hover(&self, uri: &str, byte: usize) -> Value {
        let Some(source) = self.sources.get(uri) else {
            return Value::Null;
        };
        let Some(occurrence) = source.at(byte) else {
            return Value::Null;
        };
        let Some(symbol) = &occurrence.resolution.symbol else {
            return Value::Null;
        };
        json!({ "contents": { "kind": "plaintext", "value": symbol.detail }, "range": range(&source.text, occurrence.start, occurrence.end) })
    }

    pub fn definition(&self, uri: &str, byte: usize) -> Value {
        let Some(source) = self.sources.get(uri) else {
            return Value::Null;
        };
        let Some(symbol) = source
            .at(byte)
            .and_then(|occurrence| occurrence.resolution.symbol.as_ref())
        else {
            return Value::Null;
        };
        let Some(target) = self.sources.get(&symbol.uri) else {
            return Value::Null;
        };
        json!({ "uri": symbol.uri, "range": range(&target.text, symbol.start, symbol.end) })
    }

    pub fn completion(&self, uri: &str, byte: usize) -> Value {
        let Some(source) = self.sources.get(uri) else {
            return json!([]);
        };
        let word = source.tokens.iter().position(|token| {
            token.start <= byte && byte <= token.end && matches!(token.kind, TokenKind::Word(_))
        });
        let (start, end, prefix) = word
            .map(|i| {
                let token = &source.tokens[i];
                (token.start, token.end, &source.text[token.start..byte])
            })
            .unwrap_or((byte, byte, ""));
        let previous = source
            .tokens
            .iter()
            .rposition(|token| token.end <= start && !matches!(token.kind, TokenKind::End));
        let mut symbols = BTreeMap::new();
        if let Some(i) = previous.filter(
            |i| matches!(&source.tokens[*i].kind, TokenKind::Symbol(symbol) if symbol == "."),
        ) {
            if let Some(receiver) = i
                .checked_sub(1)
                .and_then(|i| source.at(source.tokens[i].start))
            {
                if let Some(ty) = &receiver.resolution.ty {
                    for symbol in self.fields(ty, 0) {
                        symbols.insert(symbol.name.clone(), symbol);
                    }
                } else if let Some(symbol) = &receiver.resolution.symbol {
                    for (name, detail) in builtins() {
                        if let Some(field) = name.strip_prefix(&format!("{}.", symbol.name)) {
                            symbols.insert(field.into(), builtin_symbol(field, detail, 3));
                        }
                    }
                }
            }
        } else {
            symbols.extend(
                self.definitions
                    .iter()
                    .map(|(name, (_, symbol))| (name.clone(), symbol.clone())),
            );
            symbols.extend(
                self.constructors
                    .iter()
                    .map(|(name, (_, symbol))| (name.clone(), symbol.clone())),
            );
            symbols.extend(self.globals.clone());
            for (name, detail) in builtins() {
                symbols
                    .entry(name.into())
                    .or_insert_with(|| builtin_symbol(name, detail, 3));
            }
            for name in [
                "abstract",
                "component",
                "requires",
                "allows",
                "one",
                "optional",
                "many",
                "some",
                "provides",
                "modifies",
                "multiply",
                "add",
                "compose",
                "Trait",
                "type",
                "trait",
                "object",
                "let",
                "derive",
                "require",
                "with",
                "query",
                "from",
                "import",
                "where",
                "select",
                "order",
                "by",
                "ascending",
                "descending",
                "match",
                "if",
                "then",
                "else",
                "in",
                "none",
                "true",
                "false",
                "int",
                "integer",
                "real",
                "string",
                "bool",
                "option",
                "set",
                "range",
                "self",
            ] {
                symbols
                    .entry(name.into())
                    .or_insert_with(|| builtin_symbol(name, name, 255));
            }
            let mut scopes: Vec<_> = source.scopes.iter().collect();
            scopes.sort_by_key(|scope| std::cmp::Reverse(scope.end.saturating_sub(scope.start)));
            for scope in scopes {
                if scope.start <= byte && byte <= scope.end {
                    for (name, resolution) in &scope.locals {
                        if let Some(symbol) = &resolution.symbol {
                            symbols.insert(name.clone(), symbol.clone());
                        }
                    }
                }
            }
        }
        json!({ "isIncomplete": false, "items": symbols.values().filter(|symbol| symbol.name.starts_with(prefix)).map(|symbol| json!({
            "label": symbol.name, "kind": completion_kind(symbol.kind), "detail": symbol.detail,
            "textEdit": { "range": range(&source.text, start, end), "newText": symbol.name }
        })).collect::<Vec<_>>() })
    }

    pub fn symbols(&self, uri: &str, hierarchical: bool) -> Value {
        let Some(source) = self.sources.get(uri) else {
            return json!([]);
        };
        let mut symbols = Vec::new();
        for (i, statement) in source.statements.iter().enumerate() {
            let (name, span, definition) = match statement {
                Statement::Definition(definition) => {
                    (&definition.name, &definition.span, Some(definition))
                }
                Statement::Binding(binding) => (&binding.name, &binding.span, None),
                _ => continue,
            };
            let symbol = if definition.is_some() {
                self.definitions.get(name).map(|(_, symbol)| symbol)
            } else if let Statement::Binding(binding) = statement {
                self.binding_symbol(binding)
            } else {
                self.globals.get(name)
            };
            let Some(symbol) = symbol.filter(|symbol| symbol.uri == uri) else {
                continue;
            };
            let begin = span_offset(&source.text, span);
            let end = source
                .statements
                .get(i + 1)
                .map(|statement| span_offset(&source.text, statement_span(statement)))
                .unwrap_or(source.text.len());
            if hierarchical {
                let children: Vec<_> = definition.map(|definition| self.fields(&definition_type(definition), 0)).unwrap_or_default().iter()
                    .filter(|field| field.uri == uri && field.start >= begin && field.end <= end).map(|field| json!({ "name": field.name, "detail": field.detail, "kind": field.kind,
                        "range": range(&source.text, field.start, field.end), "selectionRange": range(&source.text, field.start, field.end) })).collect();
                symbols.push(json!({ "name": symbol.name, "detail": symbol.detail, "kind": symbol.kind,
                    "range": range(&source.text, begin, end), "selectionRange": range(&source.text, symbol.start, symbol.end), "children": children }));
            } else {
                symbols.push(json!({ "name": symbol.name, "kind": symbol.kind, "location": { "uri": uri, "range": range(&source.text, symbol.start, symbol.end) } }));
            }
        }
        json!(symbols)
    }

    fn fields(&self, ty: &Type, depth: usize) -> Vec<Symbol> {
        if depth > 32 {
            return Vec::new();
        }
        let Type::Named(name, args) = ty else {
            return Vec::new();
        };
        let mut fields = Vec::new();
        if let Some((definition, parent)) = self.definitions.get(name) {
            let replacements = substitutions(definition, args);
            if let Some(component) = &definition.component {
                for base in &component.parents {
                    for field in self.fields(&Type::Named(base.clone(), Vec::new()), depth + 1) {
                        if !fields
                            .iter()
                            .any(|existing: &Symbol| existing.name == field.name)
                        {
                            fields.push(field);
                        }
                    }
                }
            }
            let stored = match &definition.body {
                Body::Record(fields) => fields.as_slice(),
                _ => &[],
            };
            if let Body::Alias(ty) = &definition.body {
                fields.extend(self.fields(&substitute(ty, &replacements), depth + 1));
            }
            for field in stored.iter().chain(&definition.derives) {
                let ty = substitute(&field.ty, &replacements);
                let source = &self.sources[&parent.uri];
                let start = span_offset(&source.text, &field.span);
                fields.push(Symbol {
                    name: field.name.clone(),
                    detail: format!(
                        "{}{} : {ty}",
                        if field.expr.is_some() { "derive " } else { "" },
                        field.name
                    ),
                    kind: if field.expr.is_some() { 7 } else { 8 },
                    uri: parent.uri.clone(),
                    start,
                    end: start + field.name.len(),
                    ty: Some(ty),
                });
            }
            if definition.kind == TypeKind::Object && !fields.iter().any(|f| f.name == "traits") {
                let mut symbol =
                    builtin_symbol("traits", "traits : Trait set (inline selections)", 7);
                symbol.ty = Some(Type::Set(
                    Box::new(Type::Named("Trait".into(), Vec::new())),
                    0,
                    None,
                ));
                fields.push(symbol);
            }
        } else if name == "range" && args.len() == 1 {
            for name in ["lower", "upper"] {
                let mut symbol = builtin_symbol(name, &format!("{name} : {}", args[0]), 8);
                symbol.ty = Some(args[0].clone());
                fields.push(symbol);
            }
        } else if let Some((stored, parent)) = self.constructors.get(name) {
            for field in stored {
                let source = &self.sources[&parent.uri];
                let start = span_offset(&source.text, &field.span);
                fields.push(Symbol {
                    name: field.name.clone(),
                    detail: format!("{} : {}", field.name, field.ty),
                    kind: 8,
                    uri: parent.uri.clone(),
                    start,
                    end: start + field.name.len(),
                    ty: Some(field.ty.clone()),
                });
            }
        }
        fields
    }

    fn resolve(&self, name: &str, locals: &Locals) -> Resolution {
        if let Some(resolution) = locals.get(name) {
            return resolution.clone();
        }
        let symbol = self
            .globals
            .get(name)
            .or_else(|| self.definitions.get(name).map(|(_, symbol)| symbol))
            .or_else(|| self.constructors.get(name).map(|(_, symbol)| symbol))
            .cloned()
            .or_else(|| {
                builtins()
                    .into_iter()
                    .find(|(label, _)| *label == name)
                    .map(|(name, detail)| builtin_symbol(name, detail, 3))
            });
        Resolution {
            ty: symbol.as_ref().and_then(|symbol| symbol.ty.clone()),
            symbol,
        }
    }

    fn walk_source(&self, uri: &str, source: &mut Source) {
        let statements = source.statements.clone();
        for (i, statement) in statements.iter().enumerate() {
            let end = statements
                .get(i + 1)
                .map(|statement| span_offset(&source.text, statement_span(statement)))
                .unwrap_or(source.text.len());
            match statement {
                Statement::Definition(definition) => {
                    let Some((_, symbol)) = self.definitions.get(&definition.name) else {
                        continue;
                    };
                    if symbol.uri != uri {
                        continue;
                    }
                    source.record_symbol(symbol.clone());
                    let ty = definition_type(definition);
                    let mut locals = Locals::new();
                    locals.insert(
                        "self".into(),
                        local_symbol("self", Some(ty.clone()), symbol),
                    );
                    if let Some((name, ty)) = &definition.context {
                        let token = source.tokens.windows(3).find(|tokens| tokens[0].start >= symbol.end && tokens[0].start < end
                            && matches!(&tokens[0].kind, TokenKind::Word(word) if word == "with")
                            && matches!(&tokens[1].kind, TokenKind::Symbol(symbol) if symbol == "(")
                            && matches!(&tokens[2].kind, TokenKind::Word(word) if word == name)).map(|tokens| tokens[2].clone());
                        let resolution = if let Some(token) = token {
                            let symbol = source.local(uri, name, Some(ty.clone()), &token);
                            source.record_symbol(symbol.clone());
                            Resolution {
                                ty: Some(ty.clone()),
                                symbol: Some(symbol),
                            }
                        } else {
                            local_symbol(name, Some(ty.clone()), symbol)
                        };
                        locals.insert(name.clone(), resolution);
                    }
                    source.scopes.push(Scope {
                        start: span_offset(&source.text, &definition.span),
                        end,
                        locals: locals.clone(),
                    });
                    for field in self.fields(&ty, 0) {
                        if field.uri == uri {
                            source.record_symbol(field);
                        }
                    }
                    if let Body::Record(fields) = &definition.body {
                        for field in fields {
                            if let Some(expr) = &field.expr {
                                self.walk(expr, &Locals::new(), Some(&field.ty), source, uri);
                            }
                        }
                    }
                    if let Some(component) = &definition.component {
                        for (name, expr) in &component.values {
                            let field = self
                                .fields(&ty, 0)
                                .into_iter()
                                .find(|field| field.name == *name);
                            let finish = span_offset(&source.text, &expr.span);
                            let label = source.tokens.windows(2).rev().find(|tokens| {
                                tokens[0].start >= symbol.end && tokens[1].end <= finish
                                    && matches!(&tokens[0].kind, TokenKind::Word(word) if word == name)
                                    && matches!(&tokens[1].kind, TokenKind::Symbol(symbol) if symbol == "=")
                            }).map(|tokens| tokens[0].clone());
                            if let (Some(label), Some(field)) = (label, &field) {
                                source.record(
                                    label.start,
                                    label.end,
                                    Resolution {
                                        ty: field.ty.clone(),
                                        symbol: Some(field.clone()),
                                    },
                                );
                            }
                            self.walk(
                                expr,
                                &Locals::new(),
                                field.as_ref().and_then(|f| f.ty.as_ref()),
                                source,
                                uri,
                            );
                        }
                        if let Some(expr) = &component.compose {
                            let mut compose_locals = locals.clone();
                            compose_locals.insert(
                                "base".into(),
                                local_symbol("base", Some(ty.clone()), symbol),
                            );
                            for name in ["additions", "multipliers"] {
                                compose_locals
                                    .insert(name.into(), local_symbol(name, None, symbol));
                            }
                            let (_, compose_end) =
                                self.walk(expr, &compose_locals, Some(&ty), source, uri);
                            source.scopes.push(Scope {
                                start: span_offset(&source.text, &expr.span),
                                end: compose_end,
                                locals: compose_locals,
                            });
                        }
                    }
                    for field in &definition.derives {
                        if let Some(expr) = &field.expr {
                            self.walk(expr, &locals, Some(&field.ty), source, uri);
                        }
                    }
                    for expr in &definition.requires {
                        self.walk(expr, &locals, Some(&Type::Bool), source, uri);
                    }
                }
                Statement::Binding(binding) => {
                    if let Some(symbol) = self.binding_symbol(binding)
                        && symbol.uri == uri
                    {
                        source.record_symbol(symbol.clone());
                    }
                    let mut parameter_locals = Locals::new();
                    for parameter in binding.parameters.iter().flatten() {
                        let start = span_offset(&source.text, &parameter.span);
                        if let Some(token) = source.tokens.iter().find(|t| t.start == start) {
                            let symbol = source.local(
                                uri,
                                &parameter.name,
                                Some(parameter.ty.clone()),
                                token,
                            );
                            source.record_symbol(symbol.clone());
                            parameter_locals.insert(
                                parameter.name.clone(),
                                Resolution {
                                    ty: Some(parameter.ty.clone()),
                                    symbol: Some(symbol),
                                },
                            );
                        }
                    }
                    if !parameter_locals.is_empty() {
                        let start = span_offset(&source.text, &binding.span);
                        let body = source.tokens.iter().find(|t| {
                            t.start >= start && matches!(&t.kind, TokenKind::Symbol(s) if s == "{")
                        });
                        let end = body
                            .and_then(|t| source.delimiter_end(t.start))
                            .unwrap_or(source.text.len());
                        source.scopes.push(Scope {
                            start,
                            end,
                            locals: parameter_locals.clone(),
                        });
                    }
                    self.walk(
                        &binding.expr,
                        &parameter_locals,
                        binding.ty.as_ref(),
                        source,
                        uri,
                    );
                    for requirement in &binding.requires {
                        self.walk(
                            requirement,
                            &parameter_locals,
                            Some(&Type::Bool),
                            source,
                            uri,
                        );
                    }
                    for effect in &binding.effects {
                        match effect {
                            Effect::Provides { component, value } => {
                                self.walk(
                                    value,
                                    &parameter_locals,
                                    Some(&Type::Named(component.clone(), Vec::new())),
                                    source,
                                    uri,
                                );
                            }
                            Effect::Modifies {
                                component,
                                field,
                                value,
                                ..
                            } => {
                                self.walk(value, &parameter_locals, None, source, uri);
                                if let Some(name) = field
                                    && let Some(field_symbol) = self
                                        .fields(&Type::Named(component.clone(), Vec::new()), 0)
                                        .into_iter()
                                        .find(|f| f.name == *name)
                                {
                                    let start = span_offset(&source.text, &binding.span);
                                    let finish = span_offset(&source.text, &value.span);
                                    let tokens: Vec<_> = source.tokens.windows(3).filter(|tokens| tokens[0].start >= start && tokens[2].end <= finish
                                            && matches!(&tokens[0].kind, TokenKind::Word(word) if word == component)
                                            && matches!(&tokens[1].kind, TokenKind::Symbol(symbol) if symbol == ".")
                                            && matches!(&tokens[2].kind, TokenKind::Word(word) if word == name)).map(|tokens| tokens[2].clone()).collect();
                                    for token in tokens {
                                        source.record(
                                            token.start,
                                            token.end,
                                            Resolution {
                                                ty: field_symbol.ty.clone(),
                                                symbol: Some(field_symbol.clone()),
                                            },
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
                Statement::Expression(expr) => {
                    self.walk(expr, &Locals::new(), None, source, uri);
                }
                Statement::Import(_) => {}
            }
        }
        // Type annotations are outside expression scope. Do not override occurrences
        // already resolved as locals, record labels, or expression names.
        for token in source.tokens.clone() {
            if source
                .occurrences
                .iter()
                .any(|occurrence| occurrence.start == token.start)
            {
                continue;
            }
            if let TokenKind::Word(name) = &token.kind {
                let symbol = self
                    .definitions
                    .get(name)
                    .map(|(_, symbol)| symbol.clone())
                    .or_else(|| {
                        self.constructors
                            .get(name)
                            .map(|(_, symbol)| symbol.clone())
                    })
                    .or_else(|| primitive_symbol(name));
                if let Some(symbol) = symbol {
                    source.record(
                        token.start,
                        token.end,
                        Resolution {
                            ty: symbol.ty.clone(),
                            symbol: Some(symbol),
                        },
                    );
                }
            }
        }
    }

    fn binding_symbol(&self, binding: &Binding) -> Option<&Symbol> {
        if let Some(parameters) = &binding.parameters {
            self.trait_overloads
                .get(&(binding.name.clone(), parameters.len()))
                .map(|(_, symbol)| symbol)
        } else {
            self.globals.get(&binding.name)
        }
    }

    fn walk(
        &self,
        expr: &Expr,
        locals: &Locals,
        expected: Option<&Type>,
        source: &mut Source,
        uri: &str,
    ) -> (Option<Type>, usize) {
        if source.walk_depth >= 32 {
            return (None, span_offset(&source.text, &expr.span));
        }
        source.walk_depth += 1;
        let result = self.walk_inner(expr, locals, expected, source, uri);
        source.walk_depth -= 1;
        result
    }

    fn walk_inner(
        &self,
        expr: &Expr,
        locals: &Locals,
        expected: Option<&Type>,
        source: &mut Source,
        uri: &str,
    ) -> (Option<Type>, usize) {
        let start = span_offset(&source.text, &expr.span);
        let token_end = source
            .tokens
            .iter()
            .find(|token| token.start == start)
            .map(|token| token.end)
            .unwrap_or(start);
        match &expr.kind {
            ExprKind::Name(name) => {
                let resolution = self.resolve(name, locals);
                let ty = resolution.ty.clone();
                source.record(start, token_end, resolution);
                (ty, token_end)
            }
            ExprKind::Field(receiver, name) => {
                let (ty, end) = self.walk(receiver, locals, None, source, uri);
                let token = source
                    .tokens
                    .iter()
                    .find(|token| {
                        token.start >= end
                            && matches!(&token.kind, TokenKind::Word(word) if word == name)
                    })
                    .cloned();
                let symbol = ty
                    .as_ref()
                    .and_then(|ty| {
                        self.fields(ty, 0)
                            .into_iter()
                            .find(|field| field.name == *name)
                    })
                    .or_else(|| {
                        if let ExprKind::Name(module) = &receiver.kind {
                            let fullname = format!("{module}.{name}");
                            builtins()
                                .into_iter()
                                .find(|(name, _)| *name == fullname)
                                .map(|(_, detail)| builtin_symbol(name, detail, 3))
                        } else {
                            None
                        }
                    });
                let ty = symbol.as_ref().and_then(|symbol| symbol.ty.clone());
                if let Some(token) = token {
                    source.record(
                        token.start,
                        token.end,
                        Resolution {
                            ty: ty.clone(),
                            symbol,
                        },
                    );
                    (ty, token.end)
                } else {
                    (ty, end)
                }
            }
            ExprKind::ObjectRecord(fields, selections) => {
                let record = Expr {
                    kind: ExprKind::Record(fields.clone()),
                    span: expr.span.clone(),
                };
                let result = self.walk(&record, locals, expected, source, uri);
                for selection in selections {
                    self.walk(
                        selection,
                        locals,
                        Some(&Type::Named("Trait".into(), Vec::new())),
                        source,
                        uri,
                    );
                }
                result
            }
            ExprKind::Record(fields) => {
                let available = expected.map(|ty| self.fields(ty, 0)).unwrap_or_default();
                let mut end = token_end;
                for (name, value) in fields {
                    let field = available.iter().find(|field| field.name == *name);
                    let label = source
                        .tokens
                        .iter()
                        .find(|token| {
                            token.start >= end
                                && matches!(&token.kind, TokenKind::Word(word) if word == name)
                        })
                        .cloned();
                    if let (Some(label), Some(field)) = (label, field) {
                        source.record(
                            label.start,
                            label.end,
                            Resolution {
                                ty: field.ty.clone(),
                                symbol: Some(field.clone()),
                            },
                        );
                    }
                    end = self
                        .walk(
                            value,
                            locals,
                            field.and_then(|field| field.ty.as_ref()),
                            source,
                            uri,
                        )
                        .1;
                }
                (
                    expected.cloned(),
                    source.delimiter_end(start).unwrap_or(end),
                )
            }
            ExprKind::Set(values) => {
                let inner = match expected {
                    Some(Type::Set(inner, _, _)) => Some(inner.as_ref()),
                    _ => None,
                };
                for value in values {
                    self.walk(value, locals, inner, source, uri);
                }
                (
                    expected.cloned(),
                    source.delimiter_end(start).unwrap_or(token_end),
                )
            }
            ExprKind::CallArgs(function, arguments) => {
                let mut call = function.as_ref().clone();
                if let ExprKind::Name(name) = &function.kind
                    && !locals.contains_key(name)
                    && let Some((parameters, symbol)) =
                        self.trait_overloads.get(&(name.clone(), arguments.len()))
                {
                    let call_start = span_offset(&source.text, &function.span);
                    source.record(
                        call_start,
                        call_start + name.len(),
                        Resolution {
                            ty: symbol.ty.clone(),
                            symbol: Some(symbol.clone()),
                        },
                    );
                    let mut end = token_end;
                    for (index, argument) in arguments.iter().enumerate() {
                        end = self
                            .walk(
                                argument,
                                locals,
                                parameters.get(index).map(|p| &p.ty),
                                source,
                                uri,
                            )
                            .1;
                    }
                    return (symbol.ty.clone(), end);
                }
                for argument in arguments {
                    call = Expr {
                        kind: ExprKind::Call(Box::new(call), Box::new(argument.clone())),
                        span: expr.span.clone(),
                    };
                }
                self.walk(&call, locals, expected, source, uri)
            }
            ExprKind::Call(function, argument) => {
                self.walk(function, locals, None, source, uri);
                let name = match &function.kind {
                    ExprKind::Name(name) => Some(name.as_str()),
                    _ => None,
                };
                let argument_type = name
                    .filter(|name| self.definitions.contains_key(*name) || *name == "range")
                    .map(|name| {
                        if name == "range" {
                            Type::Named(name.into(), vec![Type::Int])
                        } else {
                            Type::Named(name.into(), Vec::new())
                        }
                    });
                let (_, end) = self.walk(argument, locals, argument_type.as_ref(), source, uri);
                let ty = match name {
                    Some("round" | "floor" | "ceil" | "int" | "integer") => Some(Type::Int),
                    Some("real" | "sqrt") => Some(Type::Real),
                    Some(name) if self.definitions.contains_key(name) => argument_type,
                    _ => None,
                };
                (ty, end)
            }
            ExprKind::Binary(operator, left, right) => {
                let (left_type, _) = self.walk(left, locals, None, source, uri);
                let (right_type, end) = self.walk(right, locals, None, source, uri);
                let ty = match operator.as_str() {
                    ".." => Some(Type::Named(
                        "range".into(),
                        vec![
                            if left_type == Some(Type::Real) || right_type == Some(Type::Real) {
                                Type::Real
                            } else {
                                Type::Int
                            },
                        ],
                    )),
                    "=" | "==" | "!=" | "<>" | "<" | ">" | "<=" | ">=" | "&&" | "||" | "and"
                    | "or" => Some(Type::Bool),
                    _ => left_type,
                };
                (ty, end)
            }
            ExprKind::Unary(_, value) => self.walk(value, locals, expected, source, uri),
            ExprKind::If(condition, yes, no) => {
                self.walk(condition, locals, Some(&Type::Bool), source, uri);
                let (ty, _) = self.walk(yes, locals, expected, source, uri);
                let (_, end) = self.walk(no, locals, expected, source, uri);
                (ty, end)
            }
            ExprKind::Let(name, value, body) => {
                let (ty, _) = self.walk(value, locals, None, source, uri);
                let mut locals = locals.clone();
                if let Some(token) = source
                    .tokens
                    .iter()
                    .find(|token| {
                        token.start > start
                            && matches!(&token.kind, TokenKind::Word(word) if word == name)
                    })
                    .cloned()
                {
                    let symbol = source.local(uri, name, ty, &token);
                    source.record_symbol(symbol.clone());
                    locals.insert(
                        name.clone(),
                        Resolution {
                            ty: symbol.ty.clone(),
                            symbol: Some(symbol),
                        },
                    );
                }
                let result = self.walk(body, &locals, expected, source, uri);
                source.scopes.push(Scope {
                    start: span_offset(&source.text, &body.span),
                    end: result.1,
                    locals,
                });
                result
            }
            ExprKind::Query(query) => {
                let mut locals = locals.clone();
                let end = source.delimiter_end(start).unwrap_or(source.text.len());
                let mut cursor = start;
                for (name, ty) in &query.bindings {
                    if let Some(token) = source.tokens.windows(3).find(|tokens| tokens[0].start >= cursor
                        && matches!(&tokens[0].kind, TokenKind::Word(word) if word == "from")
                        && matches!(&tokens[1].kind, TokenKind::Word(word) if word == name)
                        && matches!(&tokens[2].kind, TokenKind::Symbol(symbol) if symbol == ":")).map(|tokens| tokens[1].clone())
                    {
                        cursor = token.end;
                        let symbol = source.local(uri, name, Some(ty.clone()), &token);
                        source.record_symbol(symbol.clone());
                        locals.insert(
                            name.clone(),
                            Resolution {
                                ty: Some(ty.clone()),
                                symbol: Some(symbol),
                            },
                        );
                    }
                }
                source.scopes.push(Scope {
                    start,
                    end,
                    locals: locals.clone(),
                });
                for filter in &query.filters {
                    self.walk(filter, &locals, Some(&Type::Bool), source, uri);
                }
                if let Some((order, _)) = &query.order {
                    self.walk(order, &locals, None, source, uri);
                }
                self.walk(&query.select, &locals, None, source, uri);
                (None, end)
            }
            ExprKind::Match(value, arms) => {
                let (ty, mut cursor) = self.walk(value, locals, None, source, uri);
                let mut result = None;
                for (pattern, body) in arms {
                    let mut locals = locals.clone();
                    if let Pattern::Case(name, fields) = pattern {
                        let case = ty.as_ref().and_then(|ty| self.case_type(ty, name));
                        if let ExprKind::Name(value_name) = &value.kind
                            && let Some(local) = locals.get_mut(value_name)
                        {
                            local.ty = case.clone();
                        }
                        if let Some(fields) = fields {
                            for (field, binding) in fields {
                                let field_type = case
                                    .as_ref()
                                    .and_then(|ty| {
                                        self.fields(ty, 0)
                                            .into_iter()
                                            .find(|symbol| symbol.name == *field)
                                    })
                                    .and_then(|symbol| symbol.ty);
                                if let Some(token) = source.tokens.iter().find(|token| token.start >= cursor && matches!(&token.kind, TokenKind::Word(word) if word == binding)).cloned() {
                                    cursor = token.end;
                                    let symbol = source.local(uri, binding, field_type, &token); source.record_symbol(symbol.clone());
                                    locals.insert(binding.clone(), Resolution { ty: symbol.ty.clone(), symbol: Some(symbol) });
                                }
                            }
                        }
                    }
                    let arm_start = span_offset(&source.text, &body.span);
                    let (arm_type, end) = self.walk(body, &locals, expected, source, uri);
                    source.scopes.push(Scope {
                        start: arm_start,
                        end,
                        locals,
                    });
                    cursor = end;
                    result = result.or(arm_type);
                }
                (result, cursor)
            }
            _ => (literal_type(expr), token_end),
        }
    }

    fn case_type(&self, ty: &Type, name: &str) -> Option<Type> {
        let Type::Named(type_name, args) = ty else {
            return None;
        };
        let (definition, _) = self.definitions.get(type_name)?;
        let Body::Union(alternatives) = &definition.body else {
            return Some(ty.clone());
        };
        for alternative in alternatives {
            match alternative {
                Alternative::Type(Type::Named(case, _)) if case == name => {
                    let Alternative::Type(ty) = alternative else {
                        unreachable!()
                    };
                    return Some(substitute(ty, &substitutions(definition, args)));
                }
                Alternative::Constructor(case, _) if case == name => {
                    return Some(Type::Named(name.into(), Vec::new()));
                }
                _ => {}
            }
        }
        None
    }
}

impl Source {
    fn new(uri: &str, text: &str) -> Self {
        let tokens = match lex(uri, text) {
            Ok(tokens) => tokens,
            Err(error) => lex(uri, &text[..span_offset(text, &error.span)]).unwrap_or_default(),
        };
        let tokens: Vec<_> = tokens
            .iter()
            .enumerate()
            .map(|(i, token)| {
                let start = span_offset(text, &token.span);
                let length = match &token.kind {
                    TokenKind::Word(word) | TokenKind::Number(word) | TokenKind::Symbol(word) => {
                        word.len()
                    }
                    TokenKind::Variable(word) => word.len() + 1,
                    TokenKind::End => 0,
                    _ => tokens
                        .get(i + 1)
                        .map(|next| span_offset(text, &next.span).saturating_sub(start))
                        .unwrap_or(0),
                };
                Token {
                    kind: token.kind.clone(),
                    start,
                    end: start + length,
                }
            })
            .collect();
        let statements = editor_parse(uri, text, &tokens);
        Self {
            text: text.into(),
            tokens,
            statements,
            occurrences: Vec::new(),
            scopes: Vec::new(),
            walk_depth: 0,
        }
    }

    fn declaration(
        &self,
        uri: &str,
        name: &str,
        span: &crate::diagnostic::Span,
        definition: bool,
        ty: Option<Type>,
    ) -> Option<Symbol> {
        let start = span_offset(&self.text, span);
        let i = self.tokens.iter().position(|token| token.start == start)?;
        let skip = if definition
            && matches!(&self.tokens[i].kind, TokenKind::Word(word) if word == "object" || word == "trait")
        {
            2
        } else {
            1
        };
        let token = self.tokens[i + skip..]
            .iter()
            .find(|token| matches!(&token.kind, TokenKind::Word(word) if word == name))?;
        Some(Symbol {
            name: name.into(),
            detail: name.into(),
            kind: 13,
            uri: uri.into(),
            start: token.start,
            end: token.end,
            ty,
        })
    }

    fn record(&mut self, start: usize, end: usize, resolution: Resolution) {
        self.occurrences.push(Occurrence {
            start,
            end,
            resolution,
        });
    }

    fn record_symbol(&mut self, symbol: Symbol) {
        self.record(
            symbol.start,
            symbol.end,
            Resolution {
                ty: symbol.ty.clone(),
                symbol: Some(symbol),
            },
        );
    }

    fn local(&self, uri: &str, name: &str, ty: Option<Type>, token: &Token) -> Symbol {
        Symbol {
            name: name.into(),
            detail: ty
                .as_ref()
                .map(|ty| format!("{name} : {ty}"))
                .unwrap_or_else(|| name.into()),
            kind: 13,
            uri: uri.into(),
            start: token.start,
            end: token.end,
            ty,
        }
    }

    fn at(&self, byte: usize) -> Option<&Occurrence> {
        self.occurrences
            .iter()
            .find(|occurrence| occurrence.start <= byte && byte < occurrence.end)
            .or_else(|| {
                self.occurrences
                    .iter()
                    .find(|occurrence| occurrence.end == byte && occurrence.start < byte)
            })
    }

    fn delimiter_end(&self, start: usize) -> Option<usize> {
        let first = self.tokens.iter().position(|token| token.start >= start && matches!(&token.kind, TokenKind::Symbol(symbol) if matches!(symbol.as_str(), "{" | "(" | "[")))?;
        let mut depth = 0;
        for token in &self.tokens[first..] {
            match &token.kind {
                TokenKind::Symbol(symbol) if matches!(symbol.as_str(), "{" | "(" | "[") => {
                    depth += 1
                }
                TokenKind::Symbol(symbol) if matches!(symbol.as_str(), "}" | ")" | "]") => {
                    depth -= 1;
                    if depth == 0 {
                        return Some(token.end);
                    }
                }
                _ => {}
            }
        }
        None
    }
}

fn editor_parse(uri: &str, text: &str, tokens: &[Token]) -> Vec<Statement> {
    if let Ok(statements) = parser::parse(uri, text) {
        return statements;
    }
    let mut repaired = text.as_bytes().to_vec();
    for (i, token) in tokens.iter().enumerate() {
        if matches!(&token.kind, TokenKind::Symbol(symbol) if symbol == ".") && tokens.get(i + 1).is_none_or(|token| matches!(&token.kind, TokenKind::End) || matches!(&token.kind, TokenKind::Symbol(symbol) if symbol == ";" || symbol == "}")) {
            repaired[token.start] = b' ';
        }
    }
    let mut repaired = String::from_utf8(repaired).unwrap();
    let mut stack = Vec::new();
    for token in tokens {
        if let TokenKind::Symbol(symbol) = &token.kind {
            match symbol.as_str() {
                "{" => stack.push('}'),
                "(" => stack.push(')'),
                "[" => stack.push(']'),
                "}" | ")" | "]" => {
                    stack.pop();
                }
                _ => {}
            }
        }
    }
    for closer in stack.into_iter().rev() {
        repaired.push(closer);
    }
    if let Ok(statements) = parser::parse(uri, &repaired) {
        return statements;
    }
    // Recover independent top-level declarations without changing runtime parsing.
    let mut starts = Vec::new();
    let mut depth = 0;
    for (i, token) in tokens.iter().enumerate() {
        if depth == 0
            && let TokenKind::Word(word) = &token.kind
        {
            let previous_kind = i.checked_sub(1).map(|i| &tokens[i].kind);
            let keyword = matches!(
                word.as_str(),
                "type" | "trait" | "object" | "let" | "query" | "import"
            ) && !matches!(previous_kind, Some(TokenKind::Word(previous)) if previous == "trait" || previous == "object");
            let object = matches!(
                tokens.get(i + 1).map(|token| &token.kind),
                Some(TokenKind::Word(_))
            ) && matches!(tokens.get(i + 2).map(|token| &token.kind), Some(TokenKind::Symbol(symbol)) if symbol == "=");
            if keyword || object {
                starts.push(token.start);
            }
        }
        if let TokenKind::Symbol(symbol) = &token.kind {
            if matches!(symbol.as_str(), "{" | "(" | "[") {
                depth += 1;
            }
            if matches!(symbol.as_str(), "}" | ")" | "]") {
                depth -= 1;
            }
        }
    }
    let mut statements = Vec::new();
    for (i, start) in starts.iter().enumerate() {
        let end = starts.get(i + 1).copied().unwrap_or(text.len());
        let prefix: String = text[..*start]
            .chars()
            .map(|ch| if ch == '\n' { '\n' } else { ' ' })
            .collect();
        if let Ok(mut parsed) = parser::parse(uri, &format!("{prefix}{}", &text[*start..end])) {
            statements.append(&mut parsed);
        }
    }
    statements
}

fn statement_span(statement: &Statement) -> &crate::diagnostic::Span {
    match statement {
        Statement::Definition(definition) => &definition.span,
        Statement::Binding(binding) => &binding.span,
        Statement::Expression(expr) => &expr.span,
        Statement::Import(import) => &import.span,
    }
}

fn definition_type(definition: &Definition) -> Type {
    Type::Named(
        definition.name.clone(),
        definition
            .params
            .iter()
            .map(|param| Type::Var(param.clone()))
            .collect(),
    )
}

fn local_symbol(name: &str, ty: Option<Type>, parent: &Symbol) -> Resolution {
    let mut symbol = parent.clone();
    symbol.name = name.into();
    symbol.kind = 13;
    symbol.ty = ty.clone();
    symbol.detail = ty
        .as_ref()
        .map(|ty| format!("{name} : {ty}"))
        .unwrap_or_else(|| name.into());
    Resolution {
        ty,
        symbol: Some(symbol),
    }
}

fn builtin_symbol(name: &str, detail: &str, kind: u8) -> Symbol {
    Symbol {
        name: name.into(),
        detail: detail.into(),
        kind,
        uri: String::new(),
        start: 0,
        end: 0,
        ty: None,
    }
}

fn primitive_symbol(name: &str) -> Option<Symbol> {
    Some(builtin_symbol(
        name,
        match name {
            "int" | "integer" => "int / integer: signed 64-bit integer",
            "real" => "real: finite IEEE 754 binary64 number",
            "string" => "string: UTF-8 text",
            "bool" => "bool: true or false",
            "range" => "'a range: inclusive numeric lower and upper bounds",
            _ => return None,
        },
        23,
    ))
}

fn literal_type(expr: &Expr) -> Option<Type> {
    match &expr.kind {
        ExprKind::Int(_) => Some(Type::Int),
        ExprKind::Real(_) => Some(Type::Real),
        ExprKind::String(_) => Some(Type::String),
        ExprKind::Bool(_) => Some(Type::Bool),
        ExprKind::Unary(_, value) => literal_type(value),
        ExprKind::Binary(operator, lower, upper) if operator == ".." => {
            let lower = literal_type(lower)?;
            let upper = literal_type(upper)?;
            if matches!(lower, Type::Int | Type::Real) && matches!(upper, Type::Int | Type::Real) {
                Some(Type::Named(
                    "range".into(),
                    vec![if lower == Type::Real || upper == Type::Real {
                        Type::Real
                    } else {
                        Type::Int
                    }],
                ))
            } else {
                None
            }
        }
        _ => None,
    }
}

fn completion_kind(kind: u8) -> u8 {
    match kind {
        5 => 7,
        11 => 8,
        23 => 22,
        8 => 5,
        7 => 10,
        22 => 20,
        14 => 21,
        3 => 3,
        255 => 14,
        _ => 6,
    }
}

fn builtins() -> Vec<(&'static str, &'static str)> {
    vec![
        ("round", "round : number -> int (half away from zero)"),
        ("floor", "floor : number -> int"),
        ("ceil", "ceil : number -> int"),
        ("int", "int : number -> int (truncate toward zero)"),
        ("integer", "integer : number -> int"),
        ("real", "real : number -> real"),
        ("abs", "abs : number -> number"),
        ("sqrt", "sqrt : number -> real"),
        ("min", "min : ordered -> ordered -> ordered"),
        ("max", "max : ordered -> ordered -> ordered"),
        ("Set", "Set: set operations"),
        ("Set.contains", "Set.contains : 'a -> 'a set -> bool"),
        ("Set.size", "Set.size : 'a set -> int"),
        ("List", "List: query result operations"),
        ("List.length", "List.length : query result -> int"),
        ("List.sum", "List.sum : numeric query result -> number"),
        (
            "List.product",
            "List.product : numeric query result -> number",
        ),
    ]
}
