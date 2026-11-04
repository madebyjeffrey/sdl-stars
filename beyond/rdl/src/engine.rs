use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet};

use crate::ast::*;
use crate::diagnostic::{Error, Result, Span};
use crate::parser::parse;
use crate::value::{Data, Value};

const BUDGET: usize = 200_000;
const MAX_ROWS: usize = 10_000;

#[derive(Clone, Debug, Default)]
pub struct World {
    pub(crate) definitions: BTreeMap<String, Definition>,
    pub(crate) bindings: BTreeMap<String, Binding>,
    pub(crate) constructors: BTreeMap<String, Vec<Field>>,
}

impl World {
    pub fn new() -> Self {
        Self::default()
    }

    /// Loads all sources as one transaction. Forward references work across files.
    pub fn load_sources(&mut self, sources: &[(&str, &str)]) -> Result<Vec<Value>> {
        self.load_source_set(sources, false, None)
    }

    pub(crate) fn load_resolved_sources(
        &mut self,
        sources: &[(&str, &str)],
        entries: &BTreeSet<String>,
    ) -> Result<Vec<Value>> {
        self.load_source_set(sources, true, Some(entries))
    }

    fn load_source_set(
        &mut self,
        sources: &[(&str, &str)],
        resolved: bool,
        entries: Option<&BTreeSet<String>>,
    ) -> Result<Vec<Value>> {
        let mut candidate = self.clone();
        let mut expressions = Vec::new();
        for (name, text) in sources {
            for statement in parse(name, text)? {
                match statement {
                    Statement::Import(import) => {
                        if !resolved {
                            return Err(Error::new(
                                &import.span,
                                "imports need a SourceLoader; in-memory load_sources does not read files",
                            ));
                        }
                    }
                    Statement::Definition(definition) => {
                        if candidate.definitions.contains_key(&definition.name)
                            || primitive(&definition.name).is_some()
                        {
                            return Err(Error::new(
                                &definition.span,
                                format!("duplicate or reserved type '{}'", definition.name),
                            ));
                        }
                        candidate
                            .definitions
                            .insert(definition.name.clone(), definition);
                    }
                    Statement::Binding(binding) => {
                        if candidate.bindings.contains_key(&binding.name)
                            || builtin(&binding.name).is_some()
                        {
                            return Err(Error::new(
                                &binding.span,
                                format!("duplicate or reserved name '{}'", binding.name),
                            ));
                        }
                        candidate.bindings.insert(binding.name.clone(), binding);
                    }
                    Statement::Expression(expr) => expressions
                        .push((expr, entries.is_none_or(|entries| entries.contains(*name)))),
                }
            }
        }
        candidate.prepare()?;
        crate::check::check_world(&candidate)?;
        candidate.validate()?;
        let mut evaluator = Evaluator::new(&candidate);
        let mut outputs = Vec::new();
        for (expr, entry) in &expressions {
            crate::check::check_expression(&candidate, expr)?;
            let value = evaluator.eval(expr, &Environment::default())?;
            if *entry {
                outputs.push(value);
            }
        }
        *self = candidate;
        Ok(outputs)
    }

    pub fn load(&mut self, source: &str, text: &str) -> Result<Vec<Value>> {
        self.load_sources(&[(source, text)])
    }

    /// Evaluates one expression. Declarations are deliberately rejected by the REPL API.
    pub fn query(&self, source: &str, text: &str) -> Result<Value> {
        let statements = parse(source, text)?;
        if statements.len() != 1 {
            return Err(Error::new(
                &Span {
                    source: source.into(),
                    line: 1,
                    column: 1,
                },
                "enter exactly one query or expression",
            ));
        }
        match &statements[0] {
            Statement::Import(import) => Err(Error::new(
                &import.span,
                "the REPL is query-only; edit imports in the source file and reload",
            )),
            Statement::Expression(expr) => {
                crate::check::check_expression(self, expr)?;
                Evaluator::new(self).eval(expr, &Environment::default())
            }
            Statement::Definition(def) => Err(Error::new(
                &def.span,
                "the REPL is query-only; edit the source file and reload",
            )),
            Statement::Binding(binding) => Err(Error::new(
                &binding.span,
                "the REPL is query-only; edit the source file and reload",
            )),
        }
    }

    pub fn object_count(&self) -> usize {
        self.bindings
            .values()
            .filter(|binding| binding.kind == TypeKind::Object)
            .count()
    }
    pub fn trait_count(&self) -> usize {
        self.bindings
            .values()
            .filter(|binding| binding.kind == TypeKind::Trait)
            .count()
    }
    pub fn type_names(&self) -> impl Iterator<Item = &str> {
        self.definitions.keys().map(String::as_str)
    }

    fn prepare(&mut self) -> Result<()> {
        self.constructors.clear();
        for definition in self.definitions.values() {
            if let Body::Union(alternatives) = &definition.body {
                for alternative in alternatives {
                    let constructor = match alternative {
                        Alternative::Type(Type::Named(name, args))
                            if args.is_empty()
                                && !self.definitions.contains_key(name)
                                && name != "range" =>
                        {
                            Some((name, Vec::new()))
                        }
                        Alternative::Constructor(name, fields) => Some((name, fields.clone())),
                        _ => None,
                    };
                    if let Some((name, fields)) = constructor {
                        if self.definitions.contains_key(name)
                            || self.bindings.contains_key(name)
                            || builtin(name).is_some()
                        {
                            return Err(Error::new(
                                &definition.span,
                                format!("constructor '{name}' conflicts with another declaration"),
                            ));
                        }
                        if let Some(previous) =
                            self.constructors.insert(name.clone(), fields.clone())
                            && (!previous.is_empty() || !fields.is_empty())
                        {
                            return Err(Error::new(
                                &definition.span,
                                format!("ambiguous constructor '{name}'"),
                            ));
                        }
                    }
                }
            }
        }
        for definition in self.definitions.values() {
            let mut names = BTreeSet::new();
            let fields = match &definition.body {
                Body::Record(fields) => fields.as_slice(),
                _ => &[],
            };
            for field in fields.iter().chain(&definition.derives) {
                if !names.insert(&field.name) {
                    return Err(Error::new(
                        &field.span,
                        format!("duplicate field '{}'", field.name),
                    ));
                }
                self.check_type(&field.ty, &definition.params, &field.span)?;
            }
            if let Some((name, ty)) = &definition.context {
                if name == "self" {
                    return Err(Error::new(
                        &definition.span,
                        "owner context cannot be named self",
                    ));
                }
                self.check_type(ty, &definition.params, &definition.span)?;
                if !matches!(ty, Type::Named(name, _) if self.definitions.get(name).is_some_and(|def| def.kind == TypeKind::Object))
                {
                    return Err(Error::new(
                        &definition.span,
                        "owner context must have an object type",
                    ));
                }
            }
            match &definition.body {
                Body::Alias(ty) => self.check_type(ty, &definition.params, &definition.span)?,
                Body::Union(alternatives) => {
                    let mut cases = BTreeSet::new();
                    for alternative in alternatives {
                        match alternative {
                            Alternative::Type(Type::Named(name, args))
                                if args.is_empty() && self.constructors.contains_key(name) =>
                            {
                                if !cases.insert(name.clone()) {
                                    return Err(Error::new(
                                        &definition.span,
                                        "duplicate union alternative",
                                    ));
                                }
                            }
                            Alternative::Type(ty) => {
                                self.check_type(ty, &definition.params, &definition.span)?;
                                if !cases.insert(ty.to_string()) {
                                    return Err(Error::new(
                                        &definition.span,
                                        "duplicate union alternative",
                                    ));
                                }
                            }
                            Alternative::Constructor(name, fields) => {
                                if !cases.insert(name.clone()) {
                                    return Err(Error::new(
                                        &definition.span,
                                        "duplicate union alternative",
                                    ));
                                }
                                let mut fields_seen = BTreeSet::new();
                                for field in fields {
                                    if !fields_seen.insert(&field.name) {
                                        return Err(Error::new(
                                            &field.span,
                                            "duplicate constructor field",
                                        ));
                                    }
                                    self.check_type(&field.ty, &definition.params, &field.span)?;
                                }
                            }
                        }
                    }
                }
                Body::Record(_) => {}
            }
            self.check_alias_cycle(&definition.name, &mut Vec::new())?;
        }
        for binding in self.bindings.values() {
            if let Some(ty) = &binding.ty {
                self.check_type(ty, &[], &binding.span)?;
            }
            if binding.kind != TypeKind::Value {
                let expected = match &binding.ty {
                    Some(Type::Named(name, _)) => self.definitions.get(name),
                    _ => None,
                };
                if !expected.is_some_and(|definition| definition.kind == binding.kind) {
                    return Err(Error::new(
                        &binding.span,
                        format!("'{}' must declare a {:?} type", binding.name, binding.kind),
                    ));
                }
            } else if matches!(&binding.ty, Some(Type::Named(name, _)) if self.definitions.get(name).is_some_and(|def| def.kind != TypeKind::Value))
            {
                return Err(Error::new(
                    &binding.span,
                    "use a trait or object declaration to assert a named entity",
                ));
            }
        }
        Ok(())
    }

    fn check_alias_cycle(&self, name: &str, stack: &mut Vec<String>) -> Result<()> {
        let Some(definition) = self.definitions.get(name) else {
            return Ok(());
        };
        if matches!(definition.body, Body::Record(_)) {
            return Ok(());
        }
        if stack.iter().any(|previous| previous == name) {
            return Err(Error::new(
                &definition.span,
                format!("cyclic type alias/union involving '{name}'"),
            ));
        }
        stack.push(name.into());
        let alternatives = match &definition.body {
            Body::Alias(Type::Named(name, _)) => vec![name],
            Body::Union(alternatives) => alternatives
                .iter()
                .filter_map(|alternative| match alternative {
                    Alternative::Type(Type::Named(name, _)) => Some(name),
                    _ => None,
                })
                .collect(),
            _ => Vec::new(),
        };
        for alternative in alternatives {
            self.check_alias_cycle(alternative, stack)?;
        }
        stack.pop();
        Ok(())
    }

    pub(crate) fn check_type(&self, ty: &Type, params: &[String], span: &Span) -> Result<()> {
        match ty {
            Type::Var(name) if !params.contains(name) => {
                Err(Error::new(span, format!("unbound type variable '{name}")))
            }
            Type::Named(name, args) => {
                let count = if let Some(def) = self.definitions.get(name) {
                    def.params.len()
                } else if name == "range" {
                    1
                } else {
                    return Err(Error::new(span, format!("unknown type '{name}'")));
                };
                if args.len() != count {
                    return Err(Error::new(
                        span,
                        format!("type '{name}' expects {count} type argument(s)"),
                    ));
                }
                for arg in args {
                    self.check_type(arg, params, span)?;
                }
                if name == "range"
                    && args.len() == 1
                    && !matches!(args[0], Type::Int | Type::Real | Type::Var(_))
                {
                    return Err(Error::new(span, "range endpoints must be int or real"));
                }
                Ok(())
            }
            Type::Set(ty, _, _) | Type::Option(ty) => self.check_type(ty, params, span),
            _ => Ok(()),
        }
    }

    fn validate(&self) -> Result<()> {
        let mut evaluator = Evaluator::new(self);
        for binding in self.bindings.values() {
            evaluator.resolve(&binding.name, &binding.span)?;
        }
        for binding in self.bindings.values() {
            let value = evaluator.resolve(&binding.name, &binding.span)?;
            evaluator.validate_value(&value, &binding.span)?;
        }
        Ok(())
    }
}

#[derive(Clone, Default)]
struct Environment {
    locals: BTreeMap<String, Value>,
}

struct Evaluator<'a> {
    world: &'a World,
    values: BTreeMap<String, Value>,
    active: Vec<String>,
    budget: usize,
    depth: usize,
}

impl<'a> Evaluator<'a> {
    fn new(world: &'a World) -> Self {
        Self {
            world,
            values: BTreeMap::new(),
            active: Vec::new(),
            budget: BUDGET,
            depth: 0,
        }
    }

    fn tick(&mut self, span: &Span) -> Result<()> {
        if self.budget == 0 {
            return Err(Error::new(span, "evaluation budget exceeded"));
        }
        self.budget -= 1;
        Ok(())
    }

    fn resolve(&mut self, name: &str, span: &Span) -> Result<Value> {
        if let Some(value) = self.values.get(name) {
            return Ok(value.clone());
        }
        let key = format!("binding {name}");
        self.enter(&key, span)?;
        let binding = self
            .world
            .bindings
            .get(name)
            .ok_or_else(|| Error::new(span, format!("unknown name '{name}'")))?
            .clone();
        let mut value = self.eval(&binding.expr, &Environment::default())?;
        if binding.kind != TypeKind::Value {
            if matches!(value.data, Data::Ref(_)) {
                return Err(Error::new(
                    span,
                    "entity declarations require a field collection, not another entity",
                ));
            }
            value.owner = Some(name.into());
            value.path = Some(name.into());
        }
        if let Some(ty) = &binding.ty {
            value = self.normalize(value, ty, &binding.expr.span)?;
        }
        self.active.pop();
        self.values.insert(name.into(), value.clone());
        Ok(value)
    }

    fn enter(&mut self, key: &str, span: &Span) -> Result<()> {
        if self.active.iter().any(|active| active == key) {
            return Err(Error::new(
                span,
                format!("cyclic dependency: {} -> {key}", self.active.join(" -> ")),
            ));
        }
        if self.active.len() >= 32 {
            return Err(Error::new(span, "dependency nesting limit exceeded"));
        }
        self.active.push(key.into());
        Ok(())
    }

    fn eval(&mut self, expr: &Expr, env: &Environment) -> Result<Value> {
        self.tick(&expr.span)?;
        self.depth += 1;
        if self.depth > 32 {
            return Err(Error::new(&expr.span, "evaluation nesting limit exceeded"));
        }
        let result = self.eval_inner(expr, env);
        self.depth -= 1;
        result
    }

    fn eval_inner(&mut self, expr: &Expr, env: &Environment) -> Result<Value> {
        let span = &expr.span;
        Ok(match &expr.kind {
            ExprKind::Int(value) => Value::new(Data::Int(*value)),
            ExprKind::Real(value) => Value::new(Data::Real(*value)),
            ExprKind::String(value) => Value::new(Data::String(value.clone())),
            ExprKind::Bool(value) => Value::new(Data::Bool(*value)),
            ExprKind::None => Value::new(Data::None),
            ExprKind::Name(name) => {
                if let Some(value) = env.locals.get(name) {
                    value.clone()
                } else if let Some(binding) = self.world.bindings.get(name) {
                    if binding.kind == TypeKind::Value {
                        self.resolve(name, span)?
                    } else {
                        Value::new(Data::Ref(name.clone()))
                    }
                } else if let Some(fields) = self.world.constructors.get(name) {
                    if fields.is_empty() {
                        Value::new(Data::Tag(name.clone()))
                    } else {
                        Value::new(Data::Function(name.clone(), Vec::new()))
                    }
                } else if builtin(name).is_some()
                    || self.world.definitions.contains_key(name)
                    || matches!(name.as_str(), "range" | "Set" | "List")
                {
                    Value::new(Data::Function(name.clone(), Vec::new()))
                } else {
                    return Err(Error::new(span, format!("unknown name '{name}'")));
                }
            }
            ExprKind::Record(fields) => {
                let mut values = BTreeMap::new();
                for (name, expr) in fields {
                    let value = self.eval(expr, env)?;
                    if values.insert(name.clone(), value).is_some() {
                        return Err(Error::new(&expr.span, format!("duplicate field '{name}'")));
                    }
                }
                Value::new(Data::Record(values))
            }
            ExprKind::Set(expressions) => {
                let mut values: Vec<Value> = Vec::new();
                for expr in expressions {
                    let value = self.eval(expr, env)?;
                    if values.iter().any(|previous| previous.equal(&value)) {
                        return Err(Error::new(&expr.span, "duplicate set entry"));
                    }
                    values.push(value);
                }
                Value::new(Data::Set(values))
            }
            ExprKind::Field(receiver, name) => {
                let value = self.eval(receiver, env)?;
                self.field(value, name, span)?
            }
            ExprKind::Call(function, argument) => {
                let function = self.eval(function, env)?;
                let argument = self.eval(argument, env)?;
                self.apply(function, argument, span)?
            }
            ExprKind::Unary(operator, operand) => {
                let value = self.eval(operand, env)?;
                match (operator.as_str(), value.data) {
                    ("-", Data::Int(value)) => Value::new(Data::Int(
                        value
                            .checked_neg()
                            .ok_or_else(|| Error::new(span, "integer overflow"))?,
                    )),
                    ("-", Data::Real(value)) => real(-value, span)?,
                    ("+", Data::Int(value)) => Value::new(Data::Int(value)),
                    ("+", Data::Real(value)) => real(value, span)?,
                    ("not", Data::Bool(value)) => Value::new(Data::Bool(!value)),
                    _ => return Err(Error::new(span, "invalid unary operand type")),
                }
            }
            ExprKind::Binary(operator, left, right) => {
                let left = self.eval(left, env)?;
                if matches!(operator.as_str(), "&&" | "and") && !boolean(&left, span)? {
                    Value::new(Data::Bool(false))
                } else if matches!(operator.as_str(), "||" | "or") && boolean(&left, span)? {
                    Value::new(Data::Bool(true))
                } else {
                    let right = self.eval(right, env)?;
                    self.binary(operator, left, right, span)?
                }
            }
            ExprKind::If(condition, yes, no) => {
                if boolean(&self.eval(condition, env)?, span)? {
                    self.eval(yes, env)?
                } else {
                    self.eval(no, env)?
                }
            }
            ExprKind::Let(name, expression, body) => {
                let value = self.eval(expression, env)?;
                let mut env = env.clone();
                env.locals.insert(name.clone(), value);
                self.eval(body, &env)?
            }
            ExprKind::Match(expression, arms) => {
                let value = self.eval(expression, env)?;
                let mut result = None;
                for (pattern, body) in arms {
                    if let Some(locals) = self.pattern(pattern, &value, span)? {
                        let mut env = env.clone();
                        env.locals.extend(locals);
                        result = Some(self.eval(body, &env)?);
                        break;
                    }
                }
                result.ok_or_else(|| {
                    Error::new(span, "non-exhaustive match: no pattern accepts this value")
                })?
            }
            ExprKind::Query(query) => self.eval_query(query, env, span)?,
        })
    }

    fn normalize(&mut self, mut value: Value, ty: &Type, span: &Span) -> Result<Value> {
        self.tick(span)?;
        self.depth += 1;
        if self.depth > 32 {
            return Err(Error::new(span, "value/type nesting limit exceeded"));
        }
        let result = self.normalize_inner(&mut value, ty, span);
        self.depth -= 1;
        result?;
        if !value.types.contains(ty) {
            value.types.push(ty.clone());
        }
        Ok(value)
    }

    fn normalize_inner(&mut self, value: &mut Value, ty: &Type, span: &Span) -> Result<()> {
        if let Data::Ref(name) = &value.data {
            let binding = &self.world.bindings[name];
            if binding.ty.as_ref() == Some(ty) {
                return Ok(());
            }
            if !matches!(ty, Type::Option(_) | Type::Set(_, _, _)) {
                return Err(Error::new(
                    span,
                    format!(
                        "entity '{name}' has type {}, expected {ty}",
                        binding.ty.as_ref().unwrap()
                    ),
                ));
            }
        }
        match ty {
            Type::Int if matches!(value.data, Data::Int(_)) => {}
            Type::Real if matches!(value.data, Data::Real(_)) => {}
            Type::Real if matches!(value.data, Data::Int(_)) => {
                let Data::Int(number) = value.data else {
                    unreachable!()
                };
                value.data = Data::Real(number as f64);
            }
            Type::String if matches!(value.data, Data::String(_)) => {}
            Type::Bool if matches!(value.data, Data::Bool(_)) => {}
            Type::Option(inner) => {
                if !matches!(value.data, Data::None) {
                    *value = self.normalize(value.clone(), inner, span)?;
                }
            }
            Type::Set(inner, min, max) => {
                let Data::Set(values) = &mut value.data else {
                    return Err(Error::new(span, format!("expected {ty}")));
                };
                if values.len() < *min || max.is_some_and(|max| values.len() > max) {
                    return Err(Error::new(
                        span,
                        format!("set cardinality {} does not satisfy {ty}", values.len()),
                    ));
                }
                for (i, item) in values.iter_mut().enumerate() {
                    item.owner = value.owner.clone();
                    item.path = value.path.as_ref().map(|path| format!("{path}[{i}]"));
                    *item = self.normalize(item.clone(), inner, span)?;
                }
                for i in 0..values.len() {
                    if values[..i]
                        .iter()
                        .any(|previous| previous.equal(&values[i]))
                    {
                        return Err(Error::new(
                            span,
                            "duplicate set entry after type conversion",
                        ));
                    }
                }
            }
            Type::Named(name, args) => {
                if value.types.iter().any(|actual| actual == ty) {
                    return Ok(());
                }
                if let Some(definition) = self.world.definitions.get(name).cloned() {
                    let substitutions = substitutions(&definition, args);
                    match &definition.body {
                        Body::Record(fields) => {
                            if value.types.iter().any(
                                |actual| matches!(actual, Type::Named(other, _) if other != name),
                            ) {
                                return Err(Error::new(
                                    span,
                                    format!("expected nominal type {ty}"),
                                ));
                            }
                            self.normalize_fields(value, fields, &substitutions, span)?;
                        }
                        Body::Alias(inner) => {
                            *value = self.normalize(
                                value.clone(),
                                &substitute(inner, &substitutions),
                                span,
                            )?
                        }
                        Body::Union(alternatives) => {
                            let mut accepted = None;
                            let mut reasons = Vec::new();
                            for alternative in alternatives {
                                let result = match alternative {
                                    Alternative::Type(Type::Named(case, args))
                                        if args.is_empty()
                                            && self.world.constructors.contains_key(case) =>
                                    {
                                        if matches!(&value.data, Data::Tag(tag) if tag == case) {
                                            Ok(value.clone())
                                        } else {
                                            Err(Error::new(span, format!("expected {case}")))
                                        }
                                    }
                                    Alternative::Constructor(case, fields) => {
                                        if value
                                            .types
                                            .contains(&Type::Named(case.clone(), Vec::new()))
                                        {
                                            let mut result = value.clone();
                                            self.normalize_fields(
                                                &mut result,
                                                fields,
                                                &substitutions,
                                                span,
                                            )
                                            .map(|_| result)
                                        } else {
                                            Err(Error::new(span, format!("expected {case}")))
                                        }
                                    }
                                    Alternative::Type(inner) => self.normalize(
                                        value.clone(),
                                        &substitute(inner, &substitutions),
                                        span,
                                    ),
                                };
                                match result {
                                    Ok(value) => {
                                        accepted = Some(value);
                                        break;
                                    }
                                    Err(error) => reasons.push(error.message),
                                }
                            }
                            *value = accepted.ok_or_else(|| {
                                Error::new(span, format!("expected {ty}: {}", reasons.join("; ")))
                            })?;
                        }
                    }
                } else if name == "range" && args.len() == 1 {
                    let fields = vec![
                        Field {
                            name: "lower".into(),
                            ty: args[0].clone(),
                            expr: None,
                            span: span.clone(),
                        },
                        Field {
                            name: "upper".into(),
                            ty: args[0].clone(),
                            expr: None,
                            span: span.clone(),
                        },
                    ];
                    self.normalize_fields(value, &fields, &BTreeMap::new(), span)?;
                } else {
                    return Err(Error::new(span, format!("unknown type '{name}'")));
                }
                if name == "range" {
                    let Data::Record(fields) = &value.data else {
                        return Err(Error::new(span, "range needs lower and upper fields"));
                    };
                    let lower = fields
                        .get("lower")
                        .ok_or_else(|| Error::new(span, "range needs a lower field"))?;
                    let upper = fields
                        .get("upper")
                        .ok_or_else(|| Error::new(span, "range needs an upper field"))?;
                    if compare(lower, upper, span)? == Ordering::Greater {
                        return Err(Error::new(span, "range lower bound exceeds upper bound"));
                    }
                }
            }
            _ => return Err(Error::new(span, format!("expected {ty}, got {value}"))),
        }
        Ok(())
    }

    fn normalize_fields(
        &mut self,
        value: &mut Value,
        fields: &[Field],
        substitutions: &BTreeMap<String, Type>,
        span: &Span,
    ) -> Result<()> {
        if matches!(&value.data, Data::Set(values) if values.is_empty()) {
            value.data = Data::Record(BTreeMap::new());
        }
        let Data::Record(values) = &mut value.data else {
            return Err(Error::new(span, "expected a collection of named fields"));
        };
        for name in values.keys() {
            if !fields.iter().any(|field| &field.name == name) {
                return Err(Error::new(
                    span,
                    format!("unknown or derived field '{name}'"),
                ));
            }
        }
        for field in fields {
            let ty = substitute(&field.ty, substitutions);
            let mut item = if let Some(item) = values.remove(&field.name) {
                item
            } else {
                match ty {
                    Type::Option(_) => Value::new(Data::None),
                    Type::Set(_, 0, _) => Value::new(Data::Set(Vec::new())),
                    _ => {
                        return Err(Error::new(
                            span,
                            format!("missing required field '{}'", field.name),
                        ));
                    }
                }
            };
            let path = value
                .path
                .as_ref()
                .map(|path| format!("{path}.{}", field.name));
            attach(&mut item, value.owner.clone(), path);
            let item = self.normalize(item, &ty, span).map_err(|mut error| {
                error.message = format!("field '{}': {}", field.name, error.message);
                error
            })?;
            values.insert(field.name.clone(), item);
        }
        Ok(())
    }

    fn context(&self, value: &Value, definition: &Definition, span: &Span) -> Result<Environment> {
        let mut env = Environment::default();
        env.locals.insert("self".into(), value.clone());
        if let Some((name, ty)) = &definition.context {
            let owner = value.owner.as_ref().ok_or_else(|| {
                Error::new(
                    span,
                    format!(
                        "{} needs an owning {ty}; access it through an object",
                        definition.name
                    ),
                )
            })?;
            let binding = &self.world.bindings[owner];
            if binding.ty.as_ref() != Some(ty) {
                return Err(Error::new(
                    span,
                    format!("{} expects owner {ty}", definition.name),
                ));
            }
            env.locals
                .insert(name.clone(), Value::new(Data::Ref(owner.clone())));
        }
        Ok(env)
    }

    fn validate_value(&mut self, value: &Value, span: &Span) -> Result<()> {
        self.tick(span)?;
        for ty in &value.types {
            let Type::Named(name, _) = ty else {
                continue;
            };
            let Some(definition) = self.world.definitions.get(name).cloned() else {
                continue;
            };
            if definition.requires.is_empty() && definition.derives.is_empty() {
                continue;
            }
            // A reusable value gains its owner when attached to an entity.
            if definition.context.is_some() && value.owner.is_none() {
                continue;
            }
            let env = self.context(value, &definition, span)?;
            for requirement in &definition.requires {
                if !boolean(&self.eval(requirement, &env)?, &requirement.span)? {
                    return Err(Error::new(
                        &requirement.span,
                        format!(
                            "require failed for {} ({name})",
                            value.path.as_deref().unwrap_or("value")
                        ),
                    ));
                }
            }
            for field in &definition.derives {
                self.field(value.clone(), &field.name, &field.span)?;
            }
        }
        match &value.data {
            Data::Record(fields) => {
                for value in fields.values() {
                    self.validate_value(value, span)?;
                }
            }
            Data::Set(values) | Data::List(values) => {
                for value in values {
                    self.validate_value(value, span)?;
                }
            }
            _ => {}
        }
        Ok(())
    }

    fn field(&mut self, mut receiver: Value, name: &str, span: &Span) -> Result<Value> {
        if let Data::Function(module, args) = &receiver.data
            && args.is_empty()
            && matches!(module.as_str(), "Set" | "List")
        {
            let name = format!("{module}.{name}");
            if builtin(&name).is_some() {
                return Ok(Value::new(Data::Function(name, Vec::new())));
            }
        }
        if let Data::Ref(reference) = &receiver.data {
            receiver = self.resolve(reference, span)?;
        }
        if let Data::Record(fields) = &receiver.data
            && let Some(value) = fields.get(name)
        {
            return Ok(value.clone());
        }
        for ty in receiver.types.iter().rev() {
            let Type::Named(type_name, args) = ty else {
                continue;
            };
            let Some(definition) = self.world.definitions.get(type_name).cloned() else {
                continue;
            };
            if let Some(field) = definition.derives.iter().find(|field| field.name == name) {
                let key = format!(
                    "{}.{}",
                    receiver
                        .path
                        .clone()
                        .unwrap_or_else(|| format!("{type_name}:{receiver}")),
                    name
                );
                self.enter(&key, span)?;
                let env = self.context(&receiver, &definition, span)?;
                let mut value = self.eval(field.expr.as_ref().unwrap(), &env)?;
                let ty = substitute(&field.ty, &substitutions(&definition, args));
                attach(&mut value, receiver.owner.clone(), Some(key));
                let value = self.normalize(value, &ty, &field.span)?;
                self.validate_value(&value, &field.span)?;
                self.active.pop();
                return Ok(value);
            }
        }
        Err(Error::new(
            span,
            format!("unknown field '{name}' on {receiver}"),
        ))
    }

    fn apply(&mut self, function: Value, argument: Value, span: &Span) -> Result<Value> {
        let Data::Function(name, mut args) = function.data else {
            return Err(Error::new(
                span,
                "value is not a function or type constructor",
            ));
        };
        args.push(argument);
        if let Some(arity) = builtin(&name) {
            if args.len() < arity {
                return Ok(Value::new(Data::Function(name, args)));
            }
            return self.builtin(&name, &args, span);
        }
        if args.len() != 1 {
            return Err(Error::new(
                span,
                "type constructors accept one named-field collection",
            ));
        }
        let mut value = args.remove(0);
        if let Some(fields) = self.world.constructors.get(&name) {
            if !matches!(value.data, Data::Record(_) | Data::Set(_)) {
                return Err(Error::new(span, "constructor needs named fields"));
            }
            self.normalize_fields(&mut value, fields, &BTreeMap::new(), span)?;
            value.types.push(Type::Named(name, Vec::new()));
            return Ok(value);
        }
        let args = if let Some(definition) = self.world.definitions.get(&name) {
            if definition.kind != TypeKind::Value {
                return Err(Error::new(
                    span,
                    "named entities must be declared at the top level",
                ));
            }
            let mut inferred = BTreeMap::new();
            if let (Body::Record(fields), Data::Record(values)) = (&definition.body, &value.data) {
                for field in fields {
                    if let (Type::Var(param), Some(item)) = (&field.ty, values.get(&field.name)) {
                        let ty = self.value_type(item, span)?;
                        if let Some(previous) = inferred.insert(param.clone(), ty.clone())
                            && previous != ty
                        {
                            return Err(Error::new(
                                span,
                                "inconsistent constructor type arguments",
                            ));
                        }
                    }
                }
            }
            definition
                .params
                .iter()
                .map(|param| {
                    inferred.get(param).cloned().ok_or_else(|| {
                        Error::new(span, format!("cannot infer type parameter '{param}"))
                    })
                })
                .collect::<Result<Vec<_>>>()?
        } else if name == "range" {
            let Data::Record(fields) = &value.data else {
                return Err(Error::new(span, "range needs named fields"));
            };
            let lower = fields
                .get("lower")
                .ok_or_else(|| Error::new(span, "missing lower bound"))?;
            vec![self.value_type(lower, span)?]
        } else {
            return Err(Error::new(span, format!("unknown constructor '{name}'")));
        };
        let value = self.normalize(value, &Type::Named(name, args), span)?;
        self.validate_value(&value, span)?;
        Ok(value)
    }

    fn value_type(&self, value: &Value, span: &Span) -> Result<Type> {
        if let Some(ty) = value.types.last() {
            return Ok(ty.clone());
        }
        match &value.data {
            Data::Int(_) => Ok(Type::Int),
            Data::Real(_) => Ok(Type::Real),
            Data::String(_) => Ok(Type::String),
            Data::Bool(_) => Ok(Type::Bool),
            Data::Ref(name) => Ok(self.world.bindings[name].ty.clone().unwrap()),
            _ => Err(Error::new(span, "cannot infer this value's type")),
        }
    }

    fn binary(&mut self, operator: &str, left: Value, right: Value, span: &Span) -> Result<Value> {
        match operator {
            "=" | "==" => return Ok(Value::new(Data::Bool(left.equal(&right)))),
            "!=" | "<>" => return Ok(Value::new(Data::Bool(!left.equal(&right)))),
            "<" | ">" | "<=" | ">=" => {
                let ordering = compare(&left, &right, span)?;
                return Ok(Value::new(Data::Bool(match operator {
                    "<" => ordering == Ordering::Less,
                    ">" => ordering == Ordering::Greater,
                    "<=" => ordering != Ordering::Greater,
                    _ => ordering != Ordering::Less,
                })));
            }
            "&&" | "and" | "||" | "or" => {
                return Ok(Value::new(Data::Bool(boolean(&right, span)?)));
            }
            ".." => {
                let (left, right, ty) = match (&left.data, &right.data) {
                    (Data::Int(_), Data::Int(_)) => (left, right, Type::Int),
                    (Data::Int(_) | Data::Real(_), Data::Int(_) | Data::Real(_)) => (
                        self.normalize(left, &Type::Real, span)?,
                        self.normalize(right, &Type::Real, span)?,
                        Type::Real,
                    ),
                    _ => return Err(Error::new(span, "range endpoints must be numeric")),
                };
                let value = Value::new(Data::Record(BTreeMap::from([
                    ("lower".into(), left),
                    ("upper".into(), right),
                ])));
                let value = self.normalize(
                    value,
                    &Type::Named(
                        "range".into(),
                        if self
                            .world
                            .definitions
                            .get("range")
                            .is_some_and(|def| def.params.is_empty())
                        {
                            Vec::new()
                        } else {
                            vec![ty]
                        },
                    ),
                    span,
                )?;
                self.validate_value(&value, span)?;
                return Ok(value);
            }
            _ => {}
        }
        if let (Data::Int(a), Data::Int(b)) = (&left.data, &right.data) {
            let value = match operator {
                "+" => a.checked_add(*b),
                "-" => a.checked_sub(*b),
                "*" => a.checked_mul(*b),
                "/" => a.checked_div(*b),
                "%" => a.checked_rem(*b),
                _ => None,
            }
            .ok_or_else(|| Error::new(span, "integer overflow or division by zero"))?;
            return Ok(Value::new(Data::Int(value)));
        }
        if let (Data::String(a), Data::String(b)) = (&left.data, &right.data)
            && operator == "+"
        {
            return Ok(Value::new(Data::String(format!("{a}{b}"))));
        }
        let a = numeric(&left, span)?;
        let b = numeric(&right, span)?;
        if matches!(operator, "/" | "%") && b == 0.0 {
            return Err(Error::new(span, "division by zero"));
        }
        real(
            match operator {
                "+" => a + b,
                "-" => a - b,
                "*" => a * b,
                "/" => a / b,
                "%" => a % b,
                _ => return Err(Error::new(span, "unsupported operator")),
            },
            span,
        )
    }

    fn builtin(&mut self, name: &str, args: &[Value], span: &Span) -> Result<Value> {
        match name {
            "round" | "floor" | "ceil" | "int" | "integer" => {
                if let Data::Int(number) = args[0].data {
                    return Ok(Value::new(Data::Int(number)));
                }
                let number = numeric(&args[0], span)?;
                let number = match name {
                    "round" => number.round(),
                    "floor" => number.floor(),
                    "ceil" => number.ceil(),
                    _ => number.trunc(),
                };
                if !(-9223372036854775808.0..9223372036854775808.0).contains(&number) {
                    return Err(Error::new(span, "integer conversion overflow"));
                }
                Ok(Value::new(Data::Int(number as i64)))
            }
            "real" => real(numeric(&args[0], span)?, span),
            "abs" => match args[0].data {
                Data::Int(number) => Ok(Value::new(Data::Int(
                    number
                        .checked_abs()
                        .ok_or_else(|| Error::new(span, "integer overflow"))?,
                ))),
                Data::Real(number) => real(number.abs(), span),
                _ => Err(Error::new(span, "abs expects a number")),
            },
            "min" | "max" => {
                let ordering = compare(&args[0], &args[1], span)?;
                Ok(
                    if (name == "min" && ordering != Ordering::Greater)
                        || (name == "max" && ordering != Ordering::Less)
                    {
                        args[0].clone()
                    } else {
                        args[1].clone()
                    },
                )
            }
            "Set.contains" => {
                let Data::Set(values) = &args[1].data else {
                    return Err(Error::new(span, "Set.contains expects a value and a set"));
                };
                Ok(Value::new(Data::Bool(
                    values.iter().any(|value| value.equal(&args[0])),
                )))
            }
            "Set.size" => {
                let Data::Set(values) = &args[0].data else {
                    return Err(Error::new(span, "Set.size expects a set"));
                };
                Ok(Value::new(Data::Int(values.len() as i64)))
            }
            "List.length" => {
                let Data::List(values) = &args[0].data else {
                    return Err(Error::new(span, "List.length expects a query result"));
                };
                Ok(Value::new(Data::Int(values.len() as i64)))
            }
            "List.sum" => {
                let Data::List(values) = &args[0].data else {
                    return Err(Error::new(span, "List.sum expects a numeric query result"));
                };
                let mut result = if args[0].item_type == Some(Type::Real) {
                    real(0.0, span)?
                } else {
                    Value::new(Data::Int(0))
                };
                for value in values {
                    result = self.binary("+", result, value.clone(), span)?;
                }
                Ok(result)
            }
            _ => Err(Error::new(span, format!("unknown function '{name}'"))),
        }
    }

    fn pattern(
        &mut self,
        pattern: &Pattern,
        value: &Value,
        span: &Span,
    ) -> Result<Option<BTreeMap<String, Value>>> {
        match pattern {
            Pattern::Wildcard => Ok(Some(BTreeMap::new())),
            Pattern::Case(name, fields) => {
                let matched = match &value.data {
                    Data::Tag(tag) => tag == name,
                    Data::None => name == "none",
                    _ => value
                        .types
                        .iter()
                        .any(|ty| matches!(ty, Type::Named(type_name, _) if type_name == name)),
                };
                if !matched {
                    return Ok(None);
                }
                let mut locals = BTreeMap::new();
                if let Some(fields) = fields {
                    for (field, binding) in fields {
                        let value = self.field(value.clone(), field, span)?;
                        if locals.insert(binding.clone(), value).is_some() {
                            return Err(Error::new(span, "duplicate pattern binding"));
                        }
                    }
                }
                Ok(Some(locals))
            }
        }
    }

    fn eval_query(&mut self, query: &Query, env: &Environment, span: &Span) -> Result<Value> {
        let mut rows = vec![env.clone()];
        let mut names = BTreeSet::new();
        for (name, ty) in &query.bindings {
            self.world.check_type(ty, &[], span)?;
            if !names.insert(name) {
                return Err(Error::new(
                    span,
                    format!("duplicate query binding '{name}'"),
                ));
            }
            if !matches!(ty, Type::Named(type_name, _) if self.world.definitions.get(type_name).is_some_and(|definition| definition.kind != TypeKind::Value))
            {
                return Err(Error::new(span, "from expects an object or trait type"));
            }
            let entities: Vec<String> = self
                .world
                .bindings
                .values()
                .filter(|binding| {
                    binding.kind != TypeKind::Value && binding.ty.as_ref() == Some(ty)
                })
                .map(|binding| binding.name.clone())
                .collect();
            let mut next = Vec::new();
            for row in &rows {
                for entity in &entities {
                    self.tick(span)?;
                    if next.len() >= MAX_ROWS {
                        return Err(Error::new(span, "query exceeds 10000 intermediate rows"));
                    }
                    let mut row = row.clone();
                    row.locals
                        .insert(name.clone(), Value::new(Data::Ref(entity.clone())));
                    next.push(row);
                }
            }
            rows = next;
        }
        let mut selected = Vec::new();
        for row in rows {
            let mut accepted = true;
            for filter in &query.filters {
                if !boolean(&self.eval(filter, &row)?, &filter.span)? {
                    accepted = false;
                    break;
                }
            }
            if accepted {
                let key = if let Some((order, _)) = &query.order {
                    Some(self.eval(order, &row)?)
                } else {
                    None
                };
                if let Some(key) = &key {
                    compare(key, key, span)?;
                }
                selected.push((key, self.eval(&query.select, &row)?));
            }
        }
        if let Some((_, descending)) = &query.order {
            // Validate the common key type before sort_by, whose callback cannot return errors.
            if let Some((Some(first), _)) = selected.first() {
                for (key, _) in &selected {
                    compare(first, key.as_ref().unwrap(), span)?;
                }
            }
            selected.sort_by(|(left, _), (right, _)| {
                let ordering =
                    compare(left.as_ref().unwrap(), right.as_ref().unwrap(), span).unwrap();
                if *descending {
                    ordering.reverse()
                } else {
                    ordering
                }
            });
        }
        let mut value = Value::new(Data::List(
            selected.into_iter().map(|(_, value)| value).collect(),
        ));
        value.item_type = crate::check::query_item_type(self.world, query, &env.locals);
        Ok(value)
    }
}

pub(crate) fn substitute(ty: &Type, substitutions: &BTreeMap<String, Type>) -> Type {
    match ty {
        Type::Var(name) => substitutions
            .get(name)
            .cloned()
            .unwrap_or_else(|| ty.clone()),
        Type::Named(name, args) => Type::Named(
            name.clone(),
            args.iter()
                .map(|arg| substitute(arg, substitutions))
                .collect(),
        ),
        Type::Option(inner) => Type::Option(Box::new(substitute(inner, substitutions))),
        Type::Set(inner, min, max) => {
            Type::Set(Box::new(substitute(inner, substitutions)), *min, *max)
        }
        _ => ty.clone(),
    }
}

fn attach(value: &mut Value, owner: Option<String>, path: Option<String>) {
    value.owner = owner.clone();
    value.path = path.clone();
    match &mut value.data {
        Data::Record(fields) => {
            for (name, value) in fields {
                attach(
                    value,
                    owner.clone(),
                    path.as_ref().map(|path| format!("{path}.{name}")),
                );
            }
        }
        Data::Set(values) | Data::List(values) => {
            for (i, value) in values.iter_mut().enumerate() {
                attach(
                    value,
                    owner.clone(),
                    path.as_ref().map(|path| format!("{path}[{i}]")),
                );
            }
        }
        _ => {}
    }
}

pub(crate) fn substitutions(definition: &Definition, args: &[Type]) -> BTreeMap<String, Type> {
    definition
        .params
        .iter()
        .cloned()
        .zip(args.iter().cloned())
        .collect()
}

fn primitive(name: &str) -> Option<Type> {
    match name {
        "int" | "integer" => Some(Type::Int),
        "real" => Some(Type::Real),
        "string" => Some(Type::String),
        "bool" => Some(Type::Bool),
        _ => None,
    }
}

pub(crate) fn builtin(name: &str) -> Option<usize> {
    match name {
        "round" | "floor" | "ceil" | "int" | "integer" | "real" | "abs" | "Set.size"
        | "List.length" | "List.sum" => Some(1),
        "min" | "max" | "Set.contains" => Some(2),
        _ => None,
    }
}

fn real(number: f64, span: &Span) -> Result<Value> {
    if !number.is_finite() {
        Err(Error::new(
            span,
            "real operation produced a non-finite value",
        ))
    } else {
        Ok(Value::new(Data::Real(number)))
    }
}

fn numeric(value: &Value, span: &Span) -> Result<f64> {
    match value.data {
        Data::Int(number) => Ok(number as f64),
        Data::Real(number) => Ok(number),
        _ => Err(Error::new(span, "expected a number")),
    }
}

fn boolean(value: &Value, span: &Span) -> Result<bool> {
    if let Data::Bool(value) = value.data {
        Ok(value)
    } else {
        Err(Error::new(span, "expected a boolean condition"))
    }
}

fn compare(left: &Value, right: &Value, span: &Span) -> Result<Ordering> {
    match (&left.data, &right.data) {
        (Data::Int(a), Data::Int(b)) => Ok(a.cmp(b)),
        (Data::Real(a), Data::Real(b)) => Ok(a.partial_cmp(b).unwrap()),
        (Data::Int(_), Data::Real(_)) | (Data::Real(_), Data::Int(_)) => Err(Error::new(
            span,
            "compare int and real with an explicit conversion",
        )),
        (Data::String(a), Data::String(b)) => Ok(a.cmp(b)),
        (Data::Bool(a), Data::Bool(b)) => Ok(a.cmp(b)),
        _ => Err(Error::new(
            span,
            "values do not have compatible ordered types",
        )),
    }
}
