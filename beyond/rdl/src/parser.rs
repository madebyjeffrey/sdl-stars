use crate::ast::*;
use crate::diagnostic::{Error, Result};
use crate::lexer::{Token, TokenKind, lex};

pub fn parse(source: &str, text: &str) -> Result<Vec<Statement>> {
    Parser {
        tokens: lex(source, text)?,
        pos: 0,
        depth: 0,
    }
    .program()
}

/// Finds top-level imports even when later editor input is unfinished. Full
/// parsing and validation still run on the original source before loading.
pub fn imports(source: &str, text: &str) -> Result<Vec<Import>> {
    let tokens = match lex(source, text) {
        Ok(tokens) => tokens,
        Err(error) => {
            let mut end = 0;
            for _ in 1..error.span.line {
                if let Some(next) = text[end..].find('\n') {
                    end += next + 1;
                }
            }
            end += text[end..]
                .char_indices()
                .nth(error.span.column.saturating_sub(1))
                .map(|(i, _)| i)
                .unwrap_or(text.len() - end);
            lex(source, &text[..end])?
        }
    };
    let mut parser = Parser {
        tokens,
        pos: 0,
        depth: 0,
    };
    let mut depth = 0;
    let mut imports = Vec::new();
    while parser.current().kind != TokenKind::End {
        if depth == 0 && parser.take("import") {
            let span = parser.tokens[parser.pos - 1].span.clone();
            imports.push(parser.import(span)?);
        } else {
            if matches!(&parser.current().kind, TokenKind::Symbol(symbol) if matches!(symbol.as_str(), "{" | "(" | "["))
            {
                depth += 1;
            }
            if matches!(&parser.current().kind, TokenKind::Symbol(symbol) if matches!(symbol.as_str(), "}" | ")" | "]"))
            {
                depth -= 1;
            }
            parser.pos += 1;
        }
    }
    Ok(imports)
}

struct Parser {
    tokens: Vec<Token>,
    pos: usize,
    depth: usize,
}

impl Parser {
    fn current(&self) -> &Token {
        &self.tokens[self.pos]
    }
    fn at(&self, text: &str) -> bool {
        self.at_offset(0, text)
    }
    fn at_offset(&self, offset: usize, text: &str) -> bool {
        self.tokens
            .get(self.pos + offset)
            .is_some_and(|token| match &token.kind {
                TokenKind::Word(value) | TokenKind::Symbol(value) => value == text,
                _ => false,
            })
    }
    fn take(&mut self, text: &str) -> bool {
        if self.at(text) {
            self.pos += 1;
            true
        } else {
            false
        }
    }
    fn expect(&mut self, text: &str) -> Result<()> {
        if self.take(text) {
            Ok(())
        } else {
            Err(self.error(format!("expected '{text}'")))
        }
    }
    fn error(&self, message: impl Into<String>) -> Error {
        Error::new(&self.current().span, message)
    }
    fn word(&mut self) -> Result<String> {
        match self.current().kind.clone() {
            TokenKind::Word(name) if !reserved(&name) => {
                self.pos += 1;
                Ok(name)
            }
            _ => Err(self.error("expected an identifier")),
        }
    }

    fn program(mut self) -> Result<Vec<Statement>> {
        let mut statements = Vec::new();
        while self.current().kind != TokenKind::End {
            if self.take(";") {
                continue;
            }
            let span = self.current().span.clone();
            let kind = if self.at("trait") {
                Some(TypeKind::Trait)
            } else if self.at("object") {
                Some(TypeKind::Object)
            } else {
                None
            };
            if self.at("abstract") || self.at("component") {
                statements.push(Statement::Definition(self.component(span)?));
            } else if self.take("import") {
                statements.push(Statement::Import(self.import(span)?));
            } else if self.at("type") || kind.is_some_and(|_| self.at_offset(1, "type")) {
                if kind.is_some() {
                    self.pos += 1;
                }
                self.expect("type")?;
                statements.push(Statement::Definition(
                    self.definition(kind.unwrap_or(TypeKind::Value), span)?,
                ));
            } else if self.take("trait") {
                let name = self.word()?;
                let parameters = if self.take("(") {
                    let mut parameters = Vec::new();
                    if !self.take(")") {
                        loop {
                            let span = self.current().span.clone();
                            let name = self.word()?;
                            self.expect(":")?;
                            let ty = self.ty()?;
                            parameters.push(Field {
                                name,
                                ty,
                                expr: None,
                                span,
                            });
                            if self.take(")") {
                                break;
                            }
                            self.expect(",")?;
                        }
                    }
                    Some(parameters)
                } else {
                    None
                };
                let ty = if self.take(":") {
                    self.ty()?
                } else {
                    Type::Named("Trait".into(), Vec::new())
                };
                let mut requires = Vec::new();
                let (expr, effects) = if self.take("=") {
                    (self.expression(0)?, Vec::new())
                } else {
                    self.expect("{")?;
                    let mut effects = Vec::new();
                    let mut metadata = Vec::new();
                    while !self.take("}") {
                        if self.take("require") {
                            requires.push(self.expression(0)?);
                        } else if self.take("provides") {
                            let provision_span = self.current().span.clone();
                            let component = self.word()?;
                            let value = if self.take("=") {
                                self.expression(0)?
                            } else if self.at("{") {
                                self.prefix()?
                            } else {
                                Expr {
                                    kind: ExprKind::Record(Vec::new()),
                                    span: provision_span,
                                }
                            };
                            effects.push(Effect::Provides { component, value });
                        } else if self.take("modifies") {
                            let component = self.word()?;
                            let field = if self.take(".") {
                                Some(self.word()?)
                            } else {
                                None
                            };
                            self.expect("{")?;
                            while !self.take("}") {
                                let operation = if self.take("multiply") {
                                    "multiply"
                                } else if self.take("add") {
                                    "add"
                                } else {
                                    return Err(self.error("expected add or multiply"));
                                };
                                let value = self.expression(0)?;
                                effects.push(Effect::Modifies {
                                    component: component.clone(),
                                    field: field.clone(),
                                    operation: operation.into(),
                                    value,
                                });
                                self.take(";");
                            }
                        } else {
                            let field = self.word()?;
                            self.expect("=")?;
                            metadata.push((field, self.expression(0)?));
                        }
                        self.take(";");
                    }
                    (
                        Expr {
                            kind: ExprKind::Record(metadata),
                            span: span.clone(),
                        },
                        effects,
                    )
                };
                statements.push(Statement::Binding(Binding {
                    name,
                    ty: Some(ty),
                    kind: TypeKind::Trait,
                    expr,
                    span,
                    effects,
                    parameters,
                    requires,
                }));
            } else if self.take("let") {
                let name = self.word()?;
                let ty = if self.take(":") {
                    Some(self.ty()?)
                } else {
                    None
                };
                self.expect("=")?;
                let expr = self.expression(0)?;
                statements.push(Statement::Binding(Binding {
                    name,
                    ty,
                    kind: TypeKind::Value,
                    expr,
                    span,
                    effects: Vec::new(),
                    parameters: None,
                    requires: Vec::new(),
                }));
            } else if matches!(self.current().kind, TokenKind::Word(_))
                && matches!(
                    self.tokens.get(self.pos + 1).map(|token| &token.kind),
                    Some(TokenKind::Word(_))
                )
                && self.at_offset(2, "=")
            {
                let ty = Type::Named(self.word()?, Vec::new());
                let name = self.word()?;
                self.expect("=")?;
                let expr = if self.take("{") {
                    let mut fields = Vec::new();
                    let mut traits = Vec::new();
                    while !self.take("}") {
                        let span = self.current().span.clone();
                        let name = self.word()?;
                        if self.take("=") || self.take(":") {
                            fields.push((name, self.expression(0)?));
                            self.take(";");
                        } else {
                            let mut selection = Expr {
                                kind: ExprKind::Name(name),
                                span: span.clone(),
                            };
                            if self.take("(") {
                                selection = Expr {
                                    kind: ExprKind::CallArgs(
                                        Box::new(selection),
                                        self.call_arguments()?,
                                    ),
                                    span,
                                };
                            }
                            traits.push(selection);
                        }
                        self.take(";");
                    }
                    Expr {
                        kind: ExprKind::ObjectRecord(fields, traits),
                        span: span.clone(),
                    }
                } else {
                    self.expression(0)?
                };
                statements.push(Statement::Binding(Binding {
                    name,
                    ty: Some(ty),
                    kind: TypeKind::Object,
                    expr,
                    span,
                    effects: Vec::new(),
                    parameters: None,
                    requires: Vec::new(),
                }));
            } else {
                statements.push(Statement::Expression(self.expression(0)?));
            }
            self.take(";");
        }
        Ok(statements)
    }

    fn import(&mut self, span: crate::diagnostic::Span) -> Result<Import> {
        let name_span = self.current().span.clone();
        let mut parts = Vec::new();
        loop {
            match self.current().kind.clone() {
                TokenKind::Word(name) => {
                    parts.push(name);
                    self.pos += 1;
                }
                _ => {
                    return Err(
                        self.error("expected a module name, such as schema or stars.traits")
                    );
                }
            }
            if !self.take(".") {
                break;
            }
        }
        if self.at("..") || self.at("/") {
            return Err(self.error("imports use dotted module names, not file paths"));
        }
        self.take(";");
        Ok(Import {
            name: parts.join("."),
            span,
            name_span,
        })
    }

    fn definition(&mut self, kind: TypeKind, span: crate::diagnostic::Span) -> Result<Definition> {
        let mut params = Vec::new();
        while let TokenKind::Variable(name) = self.current().kind.clone() {
            params.push(name);
            self.pos += 1;
        }
        if params.len() > 1 {
            return Err(self.error("this version supports one type parameter per definition"));
        }
        let name = self.word()?;
        self.expect("=")?;
        let mut derives = Vec::new();
        let mut requires = Vec::new();
        let body = if self.take("{") {
            Body::Record(self.members(&mut derives, &mut requires)?)
        } else {
            let leading_pipe = self.take("|");
            let first = self.alternative()?;
            if leading_pipe || self.at("|") || matches!(first, Alternative::Constructor(_, _)) {
                let mut alternatives = vec![first];
                while self.take("|") {
                    alternatives.push(self.alternative()?);
                }
                Body::Union(alternatives)
            } else {
                let Alternative::Type(ty) = first else {
                    unreachable!()
                };
                Body::Alias(ty)
            }
        };
        let mut context = None;
        if self.take("with") {
            if self.take("(") {
                let owner = self.word()?;
                self.expect(":")?;
                context = Some((owner, self.ty()?));
                self.expect(")")?;
            }
            self.expect("{")?;
            let fields = self.members(&mut derives, &mut requires)?;
            if !fields.is_empty() {
                return Err(Error::new(
                    &fields[0].span,
                    "with blocks contain derive and require clauses",
                ));
            }
        }
        Ok(Definition {
            name,
            params,
            kind,
            body,
            context,
            derives,
            requires,
            span,
            component: None,
        })
    }

    fn component(&mut self, span: crate::diagnostic::Span) -> Result<Definition> {
        let abstract_ = self.take("abstract");
        self.expect("component")?;
        let name = self.word()?;
        let mut parents = Vec::new();
        let mut scalar = None;
        if self.take(":") {
            loop {
                let ty = self.ty()?;
                match ty {
                    Type::Int | Type::Real | Type::String | Type::Bool if scalar.is_none() => {
                        scalar = Some(ty)
                    }
                    Type::Named(name, args) if args.is_empty() => parents.push(name),
                    _ => {
                        return Err(
                            self.error("expected a scalar primitive type or component base")
                        );
                    }
                }
                if !self.take(",") {
                    break;
                }
            }
        }
        self.expect("{")?;
        let mut fields = Vec::new();
        let mut values = Vec::new();
        let mut derives = Vec::new();
        let mut requires = Vec::new();
        let mut compose = None;
        while !self.take("}") {
            if self.take("require") {
                requires.push(self.expression(0)?);
            } else if self.take("compose") {
                if compose.is_some() {
                    return Err(self.error("duplicate composition rule"));
                }
                self.expect("=")?;
                compose = Some(self.expression(0)?);
            } else {
                let derived = self.take("derive");
                let field_span = self.current().span.clone();
                let field = self.word()?;
                if self.take(":") {
                    let ty = self.ty()?;
                    let expr = if self.take("=") {
                        Some(self.expression(0)?)
                    } else {
                        None
                    };
                    if derived && expr.is_none() {
                        return Err(self.error("derived fields need an expression"));
                    }
                    let field = Field {
                        name: field,
                        ty,
                        expr,
                        span: field_span,
                    };
                    if derived {
                        derives.push(field);
                    } else {
                        fields.push(field);
                    }
                } else {
                    if derived {
                        return Err(self.error("derived fields need a type"));
                    }
                    self.expect("=")?;
                    values.push((field, self.expression(0)?));
                }
            }
            self.take(";");
        }
        let context = if self.take("with") {
            self.expect("(")?;
            let owner = self.word()?;
            self.expect(":")?;
            let ty = self.ty()?;
            self.expect(")")?;
            Some((owner, ty))
        } else {
            None
        };
        Ok(Definition {
            name,
            params: Vec::new(),
            kind: TypeKind::Value,
            body: Body::Record(fields),
            context,
            derives,
            requires,
            span,
            component: Some(Box::new(ComponentDefinition {
                abstract_,
                scalar,
                parents,
                values,
                compose,
                expanded: false,
            })),
        })
    }

    fn alternative(&mut self) -> Result<Alternative> {
        let ty = self.ty()?;
        if self.take("of") {
            let Type::Named(name, args) = ty else {
                return Err(self.error("expected a constructor name before 'of'"));
            };
            if !args.is_empty() {
                return Err(self.error("constructors cannot have type arguments"));
            }
            self.expect("{")?;
            let mut derives = Vec::new();
            let mut requires = Vec::new();
            let fields = self.members(&mut derives, &mut requires)?;
            if !derives.is_empty() || !requires.is_empty() {
                return Err(self.error("put constructor rules in the union's with block"));
            }
            Ok(Alternative::Constructor(name, fields))
        } else {
            Ok(Alternative::Type(ty))
        }
    }

    fn members(
        &mut self,
        derives: &mut Vec<Field>,
        requires: &mut Vec<Expr>,
    ) -> Result<Vec<Field>> {
        let mut fields = Vec::new();
        while !self.take("}") {
            let mut separated = false;
            if self.at("requires") || self.at("allows") {
                let required = self.take("requires");
                if !required {
                    self.expect("allows")?;
                }
                let cardinality = self.word()?;
                let span = self.current().span.clone();
                let name = self.word()?;
                let inner = Type::Named(name.clone(), Vec::new());
                let ty = match (required, cardinality.as_str()) {
                    (true, "one") => inner,
                    (false, "one" | "optional") => Type::Option(Box::new(inner)),
                    (false, "many") => Type::Set(Box::new(inner), 0, None),
                    (true, "some") => Type::Set(Box::new(inner), 1, None),
                    _ => {
                        return Err(self.error("use requires one/some or allows one/optional/many"));
                    }
                };
                derives.push(Field {
                    name,
                    ty: ty.clone(),
                    expr: Some(Expr {
                        kind: ExprKind::Compose(ty),
                        span: span.clone(),
                    }),
                    span,
                });
                separated = true;
            } else if self.take("require") {
                requires.push(self.expression(0)?);
            } else {
                let component = self.take("component");
                let derived = component || self.take("derive");
                let span = self.current().span.clone();
                let name = self.word()?;
                self.expect(":")?;
                let ty = self.ty()?;
                let expr = if component {
                    Some(Expr {
                        kind: ExprKind::Compose(ty.clone()),
                        span: span.clone(),
                    })
                } else if derived {
                    self.expect("=")?;
                    Some(self.expression(0)?)
                } else if self.take("=") {
                    Some(self.expression(0)?)
                } else {
                    None
                };
                let field = Field {
                    name,
                    ty,
                    expr,
                    span,
                };
                if derived {
                    derives.push(field);
                } else {
                    fields.push(field);
                }
            }
            if separated {
                self.take(";");
            } else if !self.at("}") {
                self.expect(";")?;
            } else {
                self.take(";");
            }
        }
        Ok(fields)
    }

    fn ty(&mut self) -> Result<Type> {
        self.depth += 1;
        if self.depth > 32 {
            return Err(self.error("type nesting limit exceeded"));
        }
        let result = self.ty_inner();
        self.depth -= 1;
        result
    }

    fn ty_inner(&mut self) -> Result<Type> {
        let mut ty = if self.take("(") {
            let ty = self.ty()?;
            self.expect(")")?;
            ty
        } else {
            match self.current().kind.clone() {
                TokenKind::Variable(name) => {
                    self.pos += 1;
                    Type::Var(name)
                }
                TokenKind::Word(name) if !reserved(&name) => {
                    self.pos += 1;
                    match name.as_str() {
                        "int" | "integer" | "Int" | "Integer" => Type::Int,
                        "real" | "Real" => Type::Real,
                        "string" | "String" => Type::String,
                        "bool" | "Bool" => Type::Bool,
                        _ => Type::Named(name, Vec::new()),
                    }
                }
                _ => return Err(self.error("expected a type")),
            }
        };
        let mut count = 0;
        loop {
            count += 1;
            if count > 32 {
                return Err(self.error("type chain nesting limit exceeded"));
            }
            if self.take("option") {
                if !matches!(ty, Type::Option(_)) {
                    ty = Type::Option(Box::new(ty));
                }
            } else if self.take("set") {
                let (mut min, mut max) = (0, None);
                if self.take("[") {
                    min = self.count()?;
                    self.expect("..")?;
                    max = if self.take("*") {
                        None
                    } else {
                        Some(self.count()?)
                    };
                    self.expect("]")?;
                    if max.is_some_and(|max| min > max) {
                        return Err(self.error("set minimum exceeds maximum"));
                    }
                }
                ty = Type::Set(Box::new(ty), min, max);
            } else if let TokenKind::Word(name) = &self.current().kind {
                if reserved(name) || self.at_offset(1, "=") || self.at_offset(1, ":") {
                    break;
                }
                let name = name.clone();
                self.pos += 1;
                ty = Type::Named(name, vec![ty]);
            } else {
                break;
            }
        }
        Ok(ty)
    }

    fn count(&mut self) -> Result<usize> {
        if let TokenKind::Number(text) = &self.current().kind {
            let count = text
                .parse()
                .map_err(|_| self.error("expected a nonnegative collection size"))?;
            self.pos += 1;
            Ok(count)
        } else {
            Err(self.error("expected a collection size"))
        }
    }

    fn expression(&mut self, min: u8) -> Result<Expr> {
        self.depth += 1;
        if self.depth > 32 {
            return Err(self.error("expression nesting limit exceeded"));
        }
        let result = self.expression_inner(min);
        self.depth -= 1;
        result
    }

    fn expression_inner(&mut self, min: u8) -> Result<Expr> {
        let mut left = self.prefix()?;
        let mut count = 0;
        loop {
            count += 1;
            if count > 32 {
                return Err(self.error("expression chain nesting limit exceeded"));
            }
            let span = left.span.clone();
            if self.at(".") && min <= 90 {
                self.pos += 1;
                let field = self.word()?;
                left = Expr {
                    kind: ExprKind::Field(Box::new(left), field),
                    span,
                };
                continue;
            }
            let operator = match &self.current().kind {
                TokenKind::Word(text) | TokenKind::Symbol(text) => text.clone(),
                _ => String::new(),
            };
            let power = match operator.as_str() {
                "||" | "or" => 10,
                "&&" | "and" => 15,
                "=" | "==" | "!=" | "<>" | "<" | ">" | "<=" | ">=" => 20,
                ".." => 30,
                "+" | "-" => 40,
                "*" | "/" | "%" => 50,
                _ => 0,
            };
            if power > 0 && power >= min {
                self.pos += 1;
                let right = self.expression(power + 1)?;
                left = Expr {
                    kind: ExprKind::Binary(operator, Box::new(left), Box::new(right)),
                    span,
                };
            } else if min <= 80 && self.at_call_arguments() {
                self.expect("(")?;
                left = Expr {
                    kind: ExprKind::CallArgs(Box::new(left), self.call_arguments()?),
                    span,
                };
            } else if min <= 80 && self.argument_start() {
                let right = self.expression(81)?;
                left = Expr {
                    kind: ExprKind::Call(Box::new(left), Box::new(right)),
                    span,
                };
            } else {
                break;
            }
        }
        Ok(left)
    }

    fn at_call_arguments(&self) -> bool {
        if !self.at("(") {
            return false;
        }
        if let Some(previous) = self.pos.checked_sub(1).and_then(|i| self.tokens.get(i)) {
            let length = match &previous.kind {
                TokenKind::Word(s) | TokenKind::Symbol(s) => s.len(),
                _ => 0,
            };
            if length > 0
                && previous.span.line == self.current().span.line
                && previous.span.column + length == self.current().span.column
            {
                return true;
            }
        }
        // A comma (or empty list) is unambiguously the new argument-list form.
        // Keep spaced ML calls such as `real (C {...}).field` unchanged.
        if self.at_offset(1, ")") {
            return true;
        }
        let mut depth = 0;
        for token in &self.tokens[self.pos..] {
            if let TokenKind::Symbol(symbol) = &token.kind {
                if matches!(symbol.as_str(), "(" | "{" | "[") {
                    depth += 1;
                }
                if symbol == "," && depth == 1 {
                    return true;
                }
                if matches!(symbol.as_str(), ")" | "}" | "]") {
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
            }
        }
        false
    }

    fn call_arguments(&mut self) -> Result<Vec<Expr>> {
        let mut arguments = Vec::new();
        if !self.take(")") {
            loop {
                arguments.push(self.expression(0)?);
                if self.take(")") {
                    break;
                }
                self.expect(",")?;
            }
        }
        Ok(arguments)
    }

    fn argument_start(&self) -> bool {
        if (self.at_offset(1, "=") || self.at_offset(1, ":"))
            && matches!(self.current().kind, TokenKind::Word(_))
        {
            return false;
        }
        if self.at_offset(2, "=")
            && matches!(self.current().kind, TokenKind::Word(_))
            && matches!(
                self.tokens.get(self.pos + 1).map(|token| &token.kind),
                Some(TokenKind::Word(_))
            )
        {
            return false;
        }
        match &self.current().kind {
            TokenKind::Number(_) | TokenKind::String(_) => true,
            TokenKind::Word(name) => {
                !reserved(name) || matches!(name.as_str(), "none" | "true" | "false")
            }
            TokenKind::Symbol(symbol) => matches!(symbol.as_str(), "(" | "{"),
            _ => false,
        }
    }

    fn prefix(&mut self) -> Result<Expr> {
        let token = self.current().clone();
        self.pos += 1;
        let kind = match token.kind {
            TokenKind::Number(text) => number(&text, &token.span)?,
            TokenKind::String(text) => ExprKind::String(text),
            TokenKind::Word(word) => match word.as_str() {
                "none" => ExprKind::None,
                "true" => ExprKind::Bool(true),
                "false" => ExprKind::Bool(false),
                "if" => {
                    let condition = self.expression(0)?;
                    self.expect("then")?;
                    let yes = self.expression(0)?;
                    self.expect("else")?;
                    ExprKind::If(
                        Box::new(condition),
                        Box::new(yes),
                        Box::new(self.expression(0)?),
                    )
                }
                "let" => {
                    let name = self.word()?;
                    self.expect("=")?;
                    let value = self.expression(0)?;
                    self.expect("in")?;
                    ExprKind::Let(name, Box::new(value), Box::new(self.expression(0)?))
                }
                "match" => {
                    let value = self.expression(0)?;
                    self.expect("with")?;
                    self.take("|");
                    let mut arms = Vec::new();
                    loop {
                        let pattern = self.pattern()?;
                        self.expect("->")?;
                        arms.push((pattern, self.expression(0)?));
                        if !self.take("|") {
                            break;
                        }
                    }
                    ExprKind::Match(Box::new(value), arms)
                }
                "not" => ExprKind::Unary(word, Box::new(self.expression(60)?)),
                "query" => ExprKind::Query(self.query()?),
                _ if !reserved(&word) => ExprKind::Name(word),
                _ => {
                    return Err(Error::new(
                        &token.span,
                        format!("expected an expression, found '{word}'"),
                    ));
                }
            },
            TokenKind::Symbol(symbol) if symbol == "(" => {
                let expr = self.expression(0)?;
                self.expect(")")?;
                return Ok(expr);
            }
            TokenKind::Symbol(symbol) if symbol == "{" => self.collection()?,
            TokenKind::Symbol(symbol) if symbol == "-" || symbol == "+" => {
                if symbol == "-"
                    && matches!(&self.current().kind, TokenKind::Number(text) if text.replace('_', "") == "9223372036854775808")
                {
                    let TokenKind::Number(text) = self.current().kind.clone() else {
                        unreachable!()
                    };
                    self.pos += 1;
                    number(&format!("-{text}"), &token.span)?
                } else {
                    ExprKind::Unary(symbol, Box::new(self.expression(60)?))
                }
            }
            _ => return Err(Error::new(&token.span, "expected an expression")),
        };
        Ok(Expr {
            kind,
            span: token.span,
        })
    }

    fn collection(&mut self) -> Result<ExprKind> {
        if self.take("}") {
            return Ok(ExprKind::Set(Vec::new()));
        }
        if matches!(self.current().kind, TokenKind::Word(_))
            && (self.at_offset(1, "=") || self.at_offset(1, ":"))
        {
            let mut fields = Vec::new();
            loop {
                let name = self.word()?;
                if !self.take(":") {
                    self.expect("=")?;
                }
                fields.push((name, self.expression(0)?));
                if self.take("}") {
                    break;
                }
                self.take(";");
                if self.take("}") {
                    break;
                }
            }
            Ok(ExprKind::Record(fields))
        } else {
            let mut values = Vec::new();
            loop {
                values.push(self.expression(0)?);
                if self.take("}") {
                    break;
                }
                self.expect(";")?;
                if self.take("}") {
                    break;
                }
            }
            Ok(ExprKind::Set(values))
        }
    }

    fn pattern(&mut self) -> Result<Pattern> {
        let name = match self.current().kind.clone() {
            TokenKind::Word(name) => {
                self.pos += 1;
                name
            }
            _ => return Err(self.error("expected a match pattern")),
        };
        if name == "_" {
            return Ok(Pattern::Wildcard);
        }
        let fields = if self.take("{") {
            let mut fields = Vec::new();
            while !self.take("}") {
                let field = self.word()?;
                let binding = if self.take("=") {
                    self.word()?
                } else {
                    field.clone()
                };
                fields.push((field, binding));
                if !self.at("}") {
                    self.expect(";")?;
                }
            }
            Some(fields)
        } else {
            None
        };
        Ok(Pattern::Case(name, fields))
    }

    fn query(&mut self) -> Result<Query> {
        self.expect("{")?;
        let mut bindings = Vec::new();
        let mut filters = Vec::new();
        let mut order = None;
        while !self.at("select") {
            if self.take(";") {
                continue;
            }
            if self.take("from") {
                let name = self.word()?;
                self.expect(":")?;
                bindings.push((name, self.ty()?));
            } else if self.take("where") {
                filters.push(self.expression(0)?);
            } else if self.take("order") {
                self.expect("by")?;
                if order.is_some() {
                    return Err(self.error("only one order by clause is supported"));
                }
                let expr = self.expression(0)?;
                let descending = self.take("descending");
                if !descending {
                    self.take("ascending");
                }
                order = Some((Box::new(expr), descending));
            } else {
                return Err(self.error("expected from, where, order by, or select"));
            }
        }
        if bindings.is_empty() {
            return Err(self.error("a query needs at least one from clause"));
        }
        self.expect("select")?;
        let select = Box::new(self.expression(0)?);
        self.take(";");
        self.expect("}")?;
        Ok(Query {
            bindings,
            filters,
            order,
            select,
        })
    }
}

fn number(text: &str, span: &crate::diagnostic::Span) -> Result<ExprKind> {
    let chars: Vec<_> = text.chars().collect();
    if chars.iter().enumerate().any(|(i, ch)| {
        *ch == '_'
            && (i == 0
                || i + 1 == chars.len()
                || !chars[i - 1].is_ascii_digit()
                || !chars[i + 1].is_ascii_digit())
    }) {
        return Err(Error::new(
            span,
            "numeric separators must occur between digits",
        ));
    }
    let clean = text.replace('_', "");
    let text = clean.as_str();
    if text.contains(['.', 'e', 'E']) {
        let value: f64 = text
            .parse()
            .map_err(|_| Error::new(span, "invalid real literal"))?;
        if !value.is_finite() {
            return Err(Error::new(span, "real literal must be finite"));
        }
        Ok(ExprKind::Real(value))
    } else {
        Ok(ExprKind::Int(text.parse().map_err(|_| {
            Error::new(span, "integer literal exceeds signed 64-bit range")
        })?))
    }
}

pub(crate) fn reserved(name: &str) -> bool {
    matches!(
        name,
        "type"
            | "abstract"
            | "component"
            | "requires"
            | "allows"
            | "provides"
            | "modifies"
            | "multiply"
            | "add"
            | "compose"
            | "trait"
            | "object"
            | "let"
            | "in"
            | "if"
            | "then"
            | "else"
            | "match"
            | "with"
            | "of"
            | "derive"
            | "require"
            | "query"
            | "from"
            | "where"
            | "select"
            | "order"
            | "by"
            | "ascending"
            | "descending"
            | "and"
            | "or"
            | "not"
            | "none"
            | "true"
            | "false"
            | "import"
    )
}
