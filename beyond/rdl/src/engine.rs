use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::sync::{Arc, Mutex, MutexGuard};

use crate::ast::*;
use crate::diagnostic::{Error, Result, Span};
use crate::parser::parse;
use crate::value::{Data, Value, ValueKey};

#[path = "components.rs"]
mod components;

// Nested, sampled calculations (including the Stars! habitat integral) need
// more work than a flat query. Keep both work and call depth bounded.
const BUDGET: usize = 20_000_000;
const MAX_EVALUATION_DEPTH: usize = 128;
const MAX_ROWS: usize = 10_000;

#[derive(Clone, Debug)]
pub struct World {
    pub(crate) definitions: BTreeMap<String, Definition>,
    pub(crate) bindings: BTreeMap<String, Binding>,
    pub(crate) constructors: BTreeMap<String, Constructor>,
    pub(crate) templates: BTreeMap<(String, usize), Binding>,
    evaluation_cache: Arc<Mutex<EvaluationCache>>,
}

#[derive(Clone, Debug)]
pub(crate) struct Constructor {
    pub(crate) params: Vec<String>,
    pub(crate) fields: Vec<Field>,
}

impl Constructor {
    pub(crate) fn infer_arguments(
        &self,
        mut field_type: impl FnMut(&str) -> Result<Option<Type>>,
        span: &Span,
    ) -> Result<Vec<Type>> {
        let mut inferred = BTreeMap::new();
        for field in &self.fields {
            if let Type::Var(param) = &field.ty
                && let Some(ty) = field_type(&field.name)?
                && let Some(previous) = inferred.insert(param.clone(), ty.clone())
                && previous != ty
            {
                return Err(Error::new(span, "inconsistent constructor type arguments"));
            }
        }
        self.params
            .iter()
            .map(|param| {
                inferred.get(param).cloned().ok_or_else(|| {
                    Error::new(span, format!("cannot infer type parameter '{param}"))
                })
            })
            .collect()
    }

    pub(crate) fn substitutions(&self, args: &[Type]) -> BTreeMap<String, Type> {
        self.params
            .iter()
            .cloned()
            .zip(args.iter().cloned())
            .collect()
    }
}

impl Default for World {
    fn default() -> Self {
        let definition = components::trait_definition();
        Self {
            definitions: BTreeMap::from([("Trait".into(), definition)]),
            bindings: BTreeMap::new(),
            constructors: BTreeMap::new(),
            templates: BTreeMap::new(),
            evaluation_cache: Arc::new(Mutex::new(EvaluationCache::default())),
        }
    }
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
        // Queries and constraints may depend on the complete fact set. A failed
        // transaction leaves the original snapshot and its warm cache untouched.
        candidate.evaluation_cache = Arc::new(Mutex::new(EvaluationCache::default()));
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
                        let arity = binding.parameters.as_ref().map(Vec::len);
                        if candidate.bindings.contains_key(&binding.name)
                            || builtin(&binding.name).is_some()
                            || match arity {
                                Some(arity) => candidate.template(&binding.name, arity).is_some(),
                                None => candidate.has_templates(&binding.name),
                            }
                        {
                            return Err(Error::new(
                                &binding.span,
                                match arity {
                                    Some(arity) => format!(
                                        "duplicate or reserved trait '{}/{arity}'",
                                        binding.name
                                    ),
                                    None => {
                                        format!("duplicate or reserved name '{}'", binding.name)
                                    }
                                },
                            ));
                        }
                        if let Some(arity) = arity {
                            candidate
                                .templates
                                .insert((binding.name.clone(), arity), binding);
                        } else {
                            candidate.bindings.insert(binding.name.clone(), binding);
                        }
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
        drop(evaluator);
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
            .filter(|binding| binding.kind == TypeKind::Trait && binding.parameters.is_none())
            .count()
    }
    pub fn type_names(&self) -> impl Iterator<Item = &str> {
        self.definitions
            .keys()
            .filter(|name| name.as_str() != "Trait")
            .map(String::as_str)
    }

    pub(crate) fn all_bindings(&self) -> impl Iterator<Item = &Binding> {
        self.bindings.values().chain(self.templates.values())
    }

    pub(crate) fn template(&self, name: &str, arity: usize) -> Option<&Binding> {
        self.templates.get(&(name.to_owned(), arity))
    }

    pub(crate) fn template_arities(&self, name: &str) -> Vec<usize> {
        self.templates
            .range((name.to_owned(), 0)..=(name.to_owned(), usize::MAX))
            .map(|((_, arity), _)| *arity)
            .collect()
    }

    pub(crate) fn has_templates(&self, name: &str) -> bool {
        self.templates
            .range((name.to_owned(), 0)..=(name.to_owned(), usize::MAX))
            .next()
            .is_some()
    }

    pub(crate) fn trait_arity_error(&self, name: &str, arity: usize, span: &Span) -> Error {
        let available = self
            .template_arities(name)
            .iter()
            .map(|arity| format!("{name}/{arity}"))
            .collect::<Vec<_>>()
            .join(", ");
        Error::new(
            span,
            format!("no trait '{name}/{arity}'; available: {available}"),
        )
    }

    pub(crate) fn curried_template(&self, name: &str, span: &Span) -> Result<Option<&Binding>> {
        let arities = self.template_arities(name);
        match arities.as_slice() {
            [] => Ok(None),
            [arity] => Ok(self.template(name, *arity)),
            _ => Err(Error::new(
                span,
                format!("overloaded trait '{name}' needs an explicit argument list: {name}(...)"),
            )),
        }
    }

    fn prepare(&mut self) -> Result<()> {
        self.prepare_components()?;
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
                            || self.has_templates(name)
                            || builtin(name).is_some()
                        {
                            return Err(Error::new(
                                &definition.span,
                                format!("constructor '{name}' conflicts with another declaration"),
                            ));
                        }
                        let constructor = Constructor {
                            params: definition.params.clone(),
                            fields,
                        };
                        if let Some(previous) =
                            self.constructors.insert(name.clone(), constructor.clone())
                            && (!previous.fields.is_empty() || !constructor.fields.is_empty())
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
        for binding in self.all_bindings() {
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
        for definition in self.definitions.values() {
            if let (Some(component), Body::Record(fields)) =
                (&definition.component, &definition.body)
                && !component.abstract_
                && fields.iter().all(|field| {
                    field.expr.is_some() || matches!(field.ty, Type::Option(_) | Type::Set(_, 0, _))
                })
            {
                let value = evaluator.normalize(
                    Value::record(BTreeMap::new()),
                    &Type::Named(definition.name.clone(), Vec::new()),
                    &definition.span,
                )?;
                evaluator.validate_value(&value, &definition.span)?;
            }
        }
        for binding in self.bindings.values() {
            if binding.parameters.is_some() {
                continue;
            }
            for effect in &binding.effects {
                match effect {
                    Effect::Provides { component, value } => {
                        let evaluated = evaluator.eval(value, &Environment::default())?;
                        evaluator.normalize(
                            evaluated,
                            &Type::Named(component.clone(), Vec::new()),
                            &value.span,
                        )?;
                    }
                    Effect::Modifies { value, .. } => {
                        evaluator.eval(value, &Environment::default())?;
                    }
                }
            }
            evaluator.resolve(&binding.name, &binding.span)?;
        }
        for binding in self.bindings.values() {
            if binding.parameters.is_some() {
                continue;
            }
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

#[derive(Debug, Default)]
struct EvaluationCache {
    values: BTreeMap<String, Value>,
    component_values: HashMap<(ValueKey, String), Value>,
    component_inputs: HashMap<ValueKey, Arc<components::ComponentInputs>>,
    derived_values: HashMap<(ValueKey, String), Value>,
    validated_values: HashSet<ValueKey>,
    next_value_id: usize,
    trait_instances: BTreeMap<String, Value>,
    retained_ids: HashSet<usize>,
    #[cfg(test)]
    derived_evaluations: usize,
}

impl EvaluationCache {
    fn prune(&mut self) {
        // Keep memoized properties of world values and their published results.
        // Anonymous query intermediates must not accumulate until the next reload.
        // Constraints can force properties before their receiver is published as
        // a world value. Retain that receiver's cached result graph as well.
        loop {
            let before = self.retained_ids.len();
            for ((key, _), value) in &self.derived_values {
                if self.retained_ids.contains(&key.revision()) {
                    value.retain_ids(&mut self.retained_ids);
                }
            }
            if self.retained_ids.len() == before {
                break;
            }
        }
        let retained = &self.retained_ids;
        self.derived_values
            .retain(|(key, _), _| retained.contains(&key.revision()));
        self.component_values
            .retain(|(key, _), _| retained.contains(&key.revision()));
        self.component_inputs
            .retain(|key, _| retained.contains(&key.revision()));
        self.validated_values
            .retain(|key| retained.contains(&key.revision()));
        if self.derived_values.capacity() > 4 * self.derived_values.len().max(1024) {
            self.derived_values.shrink_to_fit();
        }
        if self.validated_values.capacity() > 4 * self.validated_values.len().max(1024) {
            self.validated_values.shrink_to_fit();
        }
        self.trait_instances.retain(|_, value| {
            let Data::TraitInstance(_, _, metadata) = &value.data else {
                return false;
            };
            metadata
                .evaluation_id
                .is_some_and(|id| retained.contains(&id))
        });
    }
}

struct Evaluator<'a> {
    world: &'a World,
    cache: MutexGuard<'a, EvaluationCache>,
    active: Vec<String>,
    budget: usize,
    depth: usize,
}

impl Drop for Evaluator<'_> {
    fn drop(&mut self) {
        self.cache.prune();
    }
}

impl<'a> Evaluator<'a> {
    fn new(world: &'a World) -> Self {
        let cache = world.evaluation_cache.lock().unwrap_or_else(|poisoned| {
            let mut cache = poisoned.into_inner();
            *cache = EvaluationCache::default();
            world.evaluation_cache.clear_poison();
            cache
        });
        Self {
            world,
            cache,
            active: Vec::new(),
            budget: BUDGET,
            depth: 0,
        }
    }

    fn retain_result(&mut self, receiver: &ValueKey, result: &Value) {
        if self.cache.retained_ids.contains(&receiver.revision()) {
            result.retain_ids(&mut self.cache.retained_ids);
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
        if let Some(value) = self.cache.values.get(name) {
            return Ok(value.clone());
        }
        let key = format!("binding {name}");
        self.enter(&key, span)?;
        let binding = self
            .world
            .bindings
            .get(name)
            .ok_or_else(|| Error::new(span, format!("unknown name '{name}'")))?;
        for requirement in &binding.requires {
            if !boolean(
                &self.eval(requirement, &Environment::default())?,
                &requirement.span,
            )? {
                return Err(Error::new(
                    &requirement.span,
                    format!("require failed for trait '{name}'"),
                ));
            }
        }
        let mut value = self.eval(&binding.expr, &Environment::default())?;
        if binding.kind != TypeKind::Value {
            if matches!(value.data, Data::Ref(_) | Data::TraitInstance(_, _, _)) {
                return Err(Error::new(
                    span,
                    "entity declarations require a field collection, not another entity",
                ));
            }
            value.owner = Some(name.into());
            value.path = Some(name.into());
            for (index, selection) in value.inline_traits.iter_mut().enumerate() {
                attach(
                    selection,
                    Some(name.into()),
                    Some(format!("{name}.traits[{index}]")),
                );
            }
        }
        if let Some(ty) = &binding.ty {
            value = self.normalize(value, ty, &binding.expr.span)?;
        }
        self.active.pop();
        value.retain_ids(&mut self.cache.retained_ids);
        self.cache.values.insert(name.into(), value.clone());
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
        if self.depth > MAX_EVALUATION_DEPTH {
            return Err(Error::new(&expr.span, "evaluation nesting limit exceeded"));
        }
        let result = self.eval_inner(expr, env);
        self.depth -= 1;
        result
    }

    fn eval_inner(&mut self, expr: &Expr, env: &Environment) -> Result<Value> {
        let span = &expr.span;
        Ok(match &expr.kind {
            ExprKind::Compose(ty) => self.eval_component(ty, env, span)?,
            ExprKind::Int(value) => Value::new(Data::Int(*value)),
            ExprKind::Real(value) => Value::new(Data::Real(*value)),
            ExprKind::String(value) => Value::new(Data::String(value.clone())),
            ExprKind::Bool(value) => Value::new(Data::Bool(*value)),
            ExprKind::None => Value::new(Data::None),
            ExprKind::Name(name) => self.eval_name(name, env, span)?,
            ExprKind::ObjectRecord(fields, selections) => {
                self.eval_object_record(fields, selections, env, span)?
            }
            ExprKind::Record(fields) => self.eval_record(fields, env)?,
            ExprKind::Set(expressions) => self.eval_set(expressions, env)?,
            ExprKind::Field(receiver, name) => {
                let value = self.eval(receiver, env)?;
                self.field(value, name, span)?
            }
            ExprKind::CallArgs(function, arguments) => {
                self.eval_arguments(function, arguments, env, span)?
            }
            ExprKind::Call(function, argument) => self.eval_call(function, argument, env, span)?,
            ExprKind::Unary(operator, operand) => self.eval_unary(operator, operand, env, span)?,
            ExprKind::Binary(operator, left, right) => {
                self.eval_binary(operator, left, right, env, span)?
            }
            ExprKind::If(condition, yes, no) => {
                if boolean(&self.eval(condition, env)?, span)? {
                    self.eval(yes, env)?
                } else {
                    self.eval(no, env)?
                }
            }
            ExprKind::Let(name, expression, body) => self.eval_let(name, expression, body, env)?,
            ExprKind::Match(expression, arms) => self.eval_match(expression, arms, env, span)?,
            ExprKind::Query(query) => self.eval_query(query, env, span)?,
        })
    }

    // Keep recursive evaluation frames small in debug builds. A lazy property
    // chain can otherwise exhaust the native stack before the language limit.
    fn eval_component(&mut self, ty: &Type, env: &Environment, span: &Span) -> Result<Value> {
        Ok({
            let object = env
                .locals
                .get("self")
                .ok_or_else(|| Error::new(span, "component slots need an object"))?;
            if let Data::Ref(name) = &object.data {
                let object = self.resolve(name, span)?;
                self.compose_slot(&object, ty, span)?
            } else {
                self.compose_slot(object, ty, span)?
            }
        })
    }

    fn eval_name(&mut self, name: &str, env: &Environment, span: &Span) -> Result<Value> {
        Ok({
            if let Some(value) = env.locals.get(name) {
                value.clone()
            } else if let Some(binding) = self.world.bindings.get(name) {
                if binding.parameters.is_some() {
                    Value::new(Data::Function(name.to_owned(), Vec::new()))
                } else if binding.kind == TypeKind::Value {
                    self.resolve(name, span)?
                } else {
                    Value::new(Data::Ref(name.to_owned()))
                }
            } else if self.world.has_templates(name) {
                Value::new(Data::Function(name.to_owned(), Vec::new()))
            } else if let Some(constructor) = self.world.constructors.get(name) {
                if constructor.fields.is_empty() {
                    Value::new(Data::Tag(name.to_owned()))
                } else {
                    Value::new(Data::Function(name.to_owned(), Vec::new()))
                }
            } else if builtin(name).is_some()
                || self.world.definitions.contains_key(name)
                || matches!(name, "range" | "Set" | "List")
            {
                Value::new(Data::Function(name.to_owned(), Vec::new()))
            } else {
                return Err(Error::new(span, format!("unknown name '{name}'")));
            }
        })
    }

    fn eval_object_record(
        &mut self,
        fields: &[(String, Expr)],
        selections: &[Expr],
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
            let mut value = self.eval(
                &Expr {
                    kind: ExprKind::Record(fields.to_vec()),
                    span: span.clone(),
                },
                env,
            )?;
            for selection in selections {
                let selected = self.eval(selection, env)?;
                let selected = self.normalize(
                    selected,
                    &Type::Named("Trait".into(), Vec::new()),
                    &selection.span,
                )?;
                if value
                    .inline_traits
                    .iter()
                    .any(|other| other.equal(&selected))
                {
                    return Err(Error::new(
                        &selection.span,
                        "duplicate inline trait application",
                    ));
                }
                value.inline_traits.push(selected);
            }
            value
        })
    }

    fn eval_record(&mut self, fields: &[(String, Expr)], env: &Environment) -> Result<Value> {
        Ok({
            let mut values = BTreeMap::new();
            for (name, expr) in fields {
                let value = self.eval(expr, env)?;
                if values.insert(name.clone(), value).is_some() {
                    return Err(Error::new(&expr.span, format!("duplicate field '{name}'")));
                }
            }
            Value::record(values)
        })
    }

    fn eval_set(&mut self, expressions: &[Expr], env: &Environment) -> Result<Value> {
        Ok({
            let mut values: Vec<Value> = Vec::new();
            for expr in expressions {
                let value = self.eval(expr, env)?;
                if values.iter().any(|previous| previous.equal(&value)) {
                    return Err(Error::new(&expr.span, "duplicate set entry"));
                }
                values.push(value);
            }
            Value::new(Data::Set(values))
        })
    }

    fn eval_arguments(
        &mut self,
        function: &Expr,
        arguments: &[Expr],
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
            let mut function = self.eval(function, env)?;
            if let Data::Function(name, previous) = &function.data
                && self.world.has_templates(name)
            {
                let name = name.clone();
                let mut args = previous.clone();
                for argument in arguments {
                    args.push(self.eval(argument, env)?);
                }
                self.instantiate_trait(&name, args, span)?
            } else {
                for argument in arguments {
                    let argument = self.eval(argument, env)?;
                    function = self.apply(function, argument, span)?;
                }
                function
            }
        })
    }

    fn eval_call(
        &mut self,
        function: &Expr,
        argument: &Expr,
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
            let function = self.eval(function, env)?;
            let argument = self.eval(argument, env)?;
            self.apply(function, argument, span)?
        })
    }

    fn eval_unary(
        &mut self,
        operator: &str,
        operand: &Expr,
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
            let value = self.eval(operand, env)?;
            match (operator, value.data) {
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
        })
    }

    fn eval_binary(
        &mut self,
        operator: &str,
        left: &Expr,
        right: &Expr,
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
            let left = self.eval(left, env)?;
            if matches!(operator, "&&" | "and") && !boolean(&left, span)? {
                Value::new(Data::Bool(false))
            } else if matches!(operator, "||" | "or") && boolean(&left, span)? {
                Value::new(Data::Bool(true))
            } else {
                let right = self.eval(right, env)?;
                self.binary(operator, left, right, span)?
            }
        })
    }

    fn eval_let(
        &mut self,
        name: &str,
        expression: &Expr,
        body: &Expr,
        env: &Environment,
    ) -> Result<Value> {
        Ok({
            let value = self.eval(expression, env)?;
            let mut env = env.clone();
            env.locals.insert(name.to_owned(), value);
            self.eval(body, &env)?
        })
    }

    fn eval_match(
        &mut self,
        expression: &Expr,
        arms: &[(Pattern, Expr)],
        env: &Environment,
        span: &Span,
    ) -> Result<Value> {
        Ok({
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
        })
    }

    fn normalize(&mut self, mut value: Value, ty: &Type, span: &Span) -> Result<Value> {
        self.tick(span)?;
        // A coercion can change stored fields, primitive representation or nominal
        // identity. Its result must not inherit a cached pre-coercion calculation.
        if value.evaluation_id.is_none() || !value.types.contains(ty) {
            value.evaluation_id = Some(self.cache.next_value_id);
            self.cache.next_value_id += 1;
        }
        if matches!(ty, Type::Int | Type::Real | Type::String | Type::Bool)
            && value.component.as_ref().is_some_and(|name| {
                self.world
                    .scalar_type(&Type::Named(name.clone(), Vec::new()))
                    .is_some()
            })
        {
            value.component = None;
            value.types.clear();
        }
        self.depth += 1;
        if self.depth > MAX_EVALUATION_DEPTH {
            return Err(Error::new(span, "value/type nesting limit exceeded"));
        }
        let result = self.normalize_inner(&mut value, ty, span);
        self.depth -= 1;
        result?;
        if let Type::Named(name, _) = ty
            && self
                .world
                .definitions
                .get(name)
                .and_then(|d| d.component.as_ref())
                .is_some_and(|c| !c.abstract_)
            && value.component.is_none()
        {
            value.component = Some(name.clone());
        }
        if !value.types.contains(ty) {
            value.types.push(ty.clone());
        }
        Ok(value)
    }

    fn normalize_inner(&mut self, value: &mut Value, ty: &Type, span: &Span) -> Result<()> {
        if matches!(value.data, Data::TraitInstance(_, _, _))
            && value
                .types
                .iter()
                .any(|actual| self.world.subtype(actual, ty))
        {
            return Ok(());
        }
        if let Data::Ref(name) = &value.data {
            let binding = &self.world.bindings[name];
            if binding
                .ty
                .as_ref()
                .is_some_and(|actual| self.world.subtype(actual, ty))
            {
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
                if value
                    .types
                    .iter()
                    .any(|actual| self.world.subtype(actual, ty))
                {
                    return Ok(());
                }
                if let Some(definition) = self.world.definitions.get(name) {
                    if definition.component.as_ref().is_some_and(|c| c.abstract_) {
                        return Err(Error::new(
                            span,
                            format!("cannot instantiate abstract component '{name}'"),
                        ));
                    }
                    let substitutions = substitutions(definition, args);
                    match &definition.body {
                        Body::Record(fields) => {
                            if definition.kind == TypeKind::Object
                                && fields.iter().any(|f| f.name == "traits")
                                && !value.inline_traits.is_empty()
                            {
                                let Data::Record(values) = &mut value.data else {
                                    return Err(Error::new(span, "object needs named fields"));
                                };
                                let bag = Arc::make_mut(values)
                                    .entry("traits".into())
                                    .or_insert_with(|| Value::new(Data::Set(Vec::new())));
                                let Data::Set(traits) = &mut bag.data else {
                                    return Err(Error::new(span, "traits must be a set"));
                                };
                                for selection in std::mem::take(&mut value.inline_traits) {
                                    if !traits.iter().any(|other| other.equal(&selection)) {
                                        traits.push(selection);
                                    }
                                }
                            }
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
                                            .contains(&Type::Named(case.clone(), args.clone()))
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
            value.data = Data::Record(Arc::new(BTreeMap::new()));
        }
        let Data::Record(values) = &mut value.data else {
            return Err(Error::new(span, "expected a collection of named fields"));
        };
        let values = Arc::make_mut(values);
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
            } else if let Some(expr) = &field.expr {
                self.eval(expr, &Environment::default())?
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
        // Named objects are world facts. Passing self to a calculation must
        // preserve that reference rather than embed and revalidate the object.
        let receiver = match (&value.owner, definition.kind) {
            (Some(name), TypeKind::Object)
                if self.world.bindings.get(name).is_some_and(|binding| {
                    matches!(&binding.ty, Some(Type::Named(ty, _)) if ty == &definition.name)
                }) => Value::new(Data::Ref(name.clone())),
            _ => value.clone(),
        };
        env.locals.insert("self".into(), receiver);
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
        // References do not own another copy of a fact. Its declaration is
        // validated by the world transaction; following it here would recursively
        // validate an object passed from its derived property to a calculation.
        if matches!(value.data, Data::Ref(_)) {
            return Ok(());
        }
        let cache_key = value
            .types
            .iter()
            .any(|ty| matches!(ty, Type::Named(_, _)))
            .then(|| value.cache_key())
            .flatten();
        if cache_key
            .as_ref()
            .is_some_and(|key| self.cache.validated_values.contains(key))
        {
            return Ok(());
        }
        if value.types.iter().any(|ty| matches!(ty, Type::Named(name, _) if self.world.definitions.get(name).is_some_and(|d| d.kind == TypeKind::Object))) {
            self.validate_components(value, span)?;
        }
        for ty in &value.types {
            let Type::Named(name, _) = ty else {
                continue;
            };
            let Some(definition) = self.world.definitions.get(name) else {
                continue;
            };
            if definition.requires.is_empty() {
                continue;
            }
            // A reusable value gains its owner when attached to an entity.
            if definition.context.is_some() && value.owner.is_none() {
                continue;
            }
            let env = self.context(value, definition, span)?;
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
        }
        for selection in &value.inline_traits {
            self.validate_value(selection, span)?;
        }
        match &value.data {
            Data::TraitInstance(_, _, metadata) => self.validate_value(metadata, span)?,
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
        // Never cache partial validation, failures, or a value awaiting an owner
        // under a different owner's key. Stored values and constraints finish first.
        if let Some(key) = cache_key {
            self.cache.validated_values.insert(key);
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
        if let Data::TraitInstance(_, _, metadata) = &receiver.data {
            receiver = metadata.as_ref().clone();
        }
        if let Data::Ref(reference) = &receiver.data {
            receiver = self.resolve(reference, span)?;
        }
        if name == "traits" && receiver.types.iter().any(|ty| {
            let Type::Named(type_name, _) = ty else { return false; };
            self.world.definitions.get(type_name).is_some_and(|d| d.kind == TypeKind::Object
                && !d.derives.iter().any(|f| f.name == "traits")
                && matches!(&d.body, Body::Record(fields) if !fields.iter().any(|f| f.name == "traits")))
        }) {
            return Ok(Value::new(Data::Set(receiver.inline_traits.clone())));
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
            let Some(definition) = self.world.definitions.get(type_name) else {
                continue;
            };
            if let Some(field) = definition.derives.iter().find(|field| field.name == name) {
                let cache_key = receiver.cache_key().map(|key| (key, name.to_owned()));
                if let Some(value) = cache_key
                    .as_ref()
                    .and_then(|key| self.cache.derived_values.get(key))
                {
                    return Ok(value.clone());
                }
                let key = format!(
                    "{}.{}",
                    receiver
                        .path
                        .clone()
                        .unwrap_or_else(|| format!("{type_name}:{receiver}")),
                    name
                );
                self.enter(&key, span)?;
                #[cfg(test)]
                {
                    self.cache.derived_evaluations += 1;
                }
                let env = self.context(&receiver, definition, span)?;
                let mut value = self.eval(field.expr.as_ref().unwrap(), &env)?;
                let ty = substitute(&field.ty, &substitutions(definition, args));
                attach(&mut value, receiver.owner.clone(), Some(key));
                let value = self.normalize(value, &ty, &field.span)?;
                self.validate_value(&value, &field.span)?;
                self.active.pop();
                if let Some(cache_key) = cache_key {
                    self.retain_result(&cache_key.0, &value);
                    self.cache.derived_values.insert(cache_key, value.clone());
                }
                return Ok(value);
            }
        }
        Err(Error::new(
            span,
            format!("unknown field '{name}' on {receiver}"),
        ))
    }

    fn instantiate_trait(
        &mut self,
        name: &str,
        mut args: Vec<Value>,
        span: &Span,
    ) -> Result<Value> {
        let binding = self
            .world
            .template(name, args.len())
            .ok_or_else(|| self.world.trait_arity_error(name, args.len(), span))?;
        let parameters = binding
            .parameters
            .as_ref()
            .ok_or_else(|| Error::new(span, "not a trait template"))?;
        let mut env = Environment::default();
        for (value, parameter) in args.iter_mut().zip(parameters) {
            *value = self.normalize(value.clone(), &parameter.ty, span)?;
            self.validate_value(value, span)?;
            if matches!(
                parameter.ty,
                Type::Int | Type::Real | Type::String | Type::Bool
            ) {
                value.types = vec![parameter.ty.clone()];
                value.component = None;
                value.owner = None;
                value.path = None;
            }
            env.locals.insert(parameter.name.clone(), value.clone());
        }
        let display = format!(
            "{name}({})",
            args.iter()
                .map(ToString::to_string)
                .collect::<Vec<_>>()
                .join(", ")
        );
        let key = format!(
            "{name}:{:?}",
            args.iter().map(Value::argument_key).collect::<Vec<_>>()
        );
        if let Some(value) = self.cache.trait_instances.get(&key) {
            return Ok(value.clone());
        }
        self.enter(&format!("trait {key}"), span)?;
        let result = (|| {
            for requirement in &binding.requires {
                if !boolean(&self.eval(requirement, &env)?, &requirement.span)? {
                    return Err(Error::new(&requirement.span, "trait requirement failed"));
                }
            }
            let metadata = self.eval(&binding.expr, &env)?;
            let metadata = self.normalize(metadata, binding.ty.as_ref().unwrap(), span)?;
            self.validate_value(&metadata, span)?;
            for effect in &binding.effects {
                match effect {
                    Effect::Provides { component, value } => {
                        let value = self.eval(value, &env)?;
                        // Component requirements apply after all modifiers, as
                        // they do for named trait providers.
                        self.normalize(value, &Type::Named(component.clone(), Vec::new()), span)?;
                    }
                    Effect::Modifies { value, .. } => {
                        self.eval(value, &env)?;
                    }
                }
            }
            let mut value = Value::new(Data::TraitInstance(
                name.into(),
                args,
                Arc::new(metadata.clone()),
            ));
            value.types = metadata.types;
            Ok(value)
        })();
        self.active.pop();
        let value =
            result.map_err(|error: Error| Error::new(span, format!("{display}: {error}")))?;
        self.cache.trait_instances.insert(key, value.clone());
        Ok(value)
    }

    fn apply(&mut self, function: Value, argument: Value, span: &Span) -> Result<Value> {
        let Data::Function(name, mut args) = function.data else {
            return Err(Error::new(
                span,
                "value is not a function or type constructor",
            ));
        };
        args.push(argument);
        if let Some(binding) = self.world.curried_template(&name, span)?
            && let Some(parameters) = &binding.parameters
        {
            if args.len() < parameters.len() {
                return Ok(Value::new(Data::Function(name, args)));
            }
            return self.instantiate_trait(&name, args, span);
        }
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
        if let Some(constructor) = self.world.constructors.get(&name) {
            if !matches!(value.data, Data::Record(_) | Data::Set(_)) {
                return Err(Error::new(span, "constructor needs named fields"));
            }
            let args = constructor.infer_arguments(
                |name| {
                    value
                        .fields()
                        .and_then(|fields| fields.get(name))
                        .map(|value| self.value_type(value, span))
                        .transpose()
                },
                span,
            )?;
            self.normalize_fields(
                &mut value,
                &constructor.fields,
                &constructor.substitutions(&args),
                span,
            )?;
            value.types.push(Type::Named(name, args));
            self.validate_value(&value, span)?;
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
            "=" | "==" | "!=" | "<>" => {
                // Expressions compare scalar values; collections retain component identity.
                let mut left = left;
                let mut right = right;
                for value in [&mut left, &mut right] {
                    if value.component.as_ref().is_some_and(|name| {
                        self.world
                            .scalar_type(&Type::Named(name.clone(), Vec::new()))
                            .is_some()
                    }) {
                        value.component = None;
                    }
                }
                let equal = left.equal(&right);
                return Ok(Value::new(Data::Bool(if matches!(operator, "=" | "==") {
                    equal
                } else {
                    !equal
                })));
            }
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
                let value = Value::record(BTreeMap::from([
                    ("lower".into(), left),
                    ("upper".into(), right),
                ]));
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
            "sqrt" => real(numeric(&args[0], span)?.sqrt(), span),
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
                let mut result = if (name == "min" && ordering != Ordering::Greater)
                    || (name == "max" && ordering != Ordering::Less)
                {
                    args[0].clone()
                } else {
                    args[1].clone()
                };
                if result.component.as_ref().is_some_and(|name| {
                    self.world
                        .scalar_type(&Type::Named(name.clone(), Vec::new()))
                        .is_some()
                }) {
                    result.component = None;
                    result.types.clear();
                }
                Ok(result)
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
            "List.sum" | "List.product" => {
                let Data::List(values) = &args[0].data else {
                    return Err(Error::new(
                        span,
                        format!("{name} expects a numeric query result"),
                    ));
                };
                let mut result = if args[0]
                    .item_type
                    .as_ref()
                    .map(|ty| self.world.scalar_type(ty).unwrap_or_else(|| ty.clone()))
                    == Some(Type::Real)
                {
                    real(if name == "List.product" { 1.0 } else { 0.0 }, span)?
                } else {
                    Value::new(Data::Int(if name == "List.product" { 1 } else { 0 }))
                };
                for value in values {
                    result = self.binary(
                        if name == "List.product" { "*" } else { "+" },
                        result,
                        value.clone(),
                        span,
                    )?;
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
                let matched =
                    match &value.data {
                        Data::Tag(tag) => tag == name,
                        Data::None => name == "none",
                        Data::Ref(reference) => self.world.bindings[reference]
                            .ty
                            .as_ref()
                            .is_some_and(|ty| {
                                self.world
                                    .subtype(ty, &Type::Named(name.clone(), Vec::new()))
                            }),
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
                    binding.parameters.is_none()
                        && binding.kind != TypeKind::Value
                        && binding
                            .ty
                            .as_ref()
                            .is_some_and(|actual| self.world.subtype(actual, ty))
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
    for (i, selection) in value.inline_traits.iter_mut().enumerate() {
        attach(
            selection,
            owner.clone(),
            path.as_ref().map(|p| format!("{p}.traits[{i}]")),
        );
    }
    match &mut value.data {
        Data::TraitInstance(_, _, metadata) => {
            attach(Arc::make_mut(metadata), owner.clone(), path.clone());
        }
        Data::Record(fields) => {
            for (name, value) in Arc::make_mut(fields) {
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
        "int" | "integer" | "Int" | "Integer" => Some(Type::Int),
        "real" | "Real" => Some(Type::Real),
        "string" | "String" => Some(Type::String),
        "bool" | "Bool" => Some(Type::Bool),
        _ => None,
    }
}

pub(crate) fn builtin(name: &str) -> Option<usize> {
    match name {
        "round" | "floor" | "ceil" | "int" | "integer" | "real" | "sqrt" | "abs" | "Set.size"
        | "List.length" | "List.sum" | "List.product" => Some(1),
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

#[cfg(test)]
#[path = "cache_tests.rs"]
mod cache_tests;
