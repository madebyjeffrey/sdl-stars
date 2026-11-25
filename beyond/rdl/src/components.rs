//! Component inheritance and composition. Components are resolved per object;
//! trait declarations remain immutable facts in the world.
use super::*;

pub(super) fn trait_definition() -> Definition {
    Definition {
        name: "Trait".into(),
        params: Vec::new(),
        kind: TypeKind::Trait,
        body: Body::Record(Vec::new()),
        context: None,
        derives: Vec::new(),
        requires: Vec::new(),
        span: Span {
            source: "<builtin>".into(),
            line: 1,
            column: 1,
        },
        component: None,
    }
}

pub(crate) fn component_type(ty: &Type) -> Option<&str> {
    match ty {
        Type::Named(name, args) if args.is_empty() => Some(name),
        Type::Option(inner) | Type::Set(inner, _, _) => match inner.as_ref() {
            Type::Named(name, args) if args.is_empty() => Some(name),
            _ => None,
        },
        _ => None,
    }
}

impl World {
    pub(crate) fn subtype(&self, actual: &Type, expected: &Type) -> bool {
        if actual == expected {
            return true;
        }
        let (Type::Named(actual, a), Type::Named(expected, b)) = (actual, expected) else {
            return false;
        };
        if !a.is_empty() || !b.is_empty() {
            return false;
        }
        if expected == "Trait"
            && self
                .definitions
                .get(actual)
                .is_some_and(|d| d.kind == TypeKind::Trait)
        {
            return true;
        }
        self.component_ancestor(actual, expected, 0)
    }

    fn component_ancestor(&self, actual: &str, expected: &str, depth: usize) -> bool {
        if depth > 32 {
            return false;
        }
        self.definitions
            .get(actual)
            .and_then(|d| d.component.as_ref())
            .is_some_and(|component| {
                component.parents.iter().any(|parent| {
                    parent == expected || self.component_ancestor(parent, expected, depth + 1)
                })
            })
    }

    /// Scalar components are nominal at assignment boundaries and primitive in expressions.
    pub(crate) fn scalar_type(&self, ty: &Type) -> Option<Type> {
        let Type::Named(name, args) = ty else {
            return None;
        };
        if !args.is_empty() {
            return None;
        }
        self.definitions
            .get(name)?
            .component
            .as_ref()?
            .scalar
            .clone()
    }

    pub(crate) fn component_definition(&self, name: &str, span: &Span) -> Result<&Definition> {
        self.definitions
            .get(name)
            .filter(|d| d.component.is_some())
            .ok_or_else(|| Error::new(span, format!("unknown component '{name}'")))
    }

    pub(super) fn prepare_components(&mut self) -> Result<()> {
        let names: Vec<_> = self.definitions.keys().cloned().collect();
        for name in names {
            self.expand_component(&name, &mut Vec::new())?;
        }
        for definition in self.definitions.values() {
            for field in &definition.derives {
                if let Some(Expr {
                    kind: ExprKind::Compose(ty),
                    ..
                }) = &field.expr
                {
                    if definition.kind != TypeKind::Object {
                        return Err(Error::new(
                            &field.span,
                            "component slots belong to object types",
                        ));
                    }
                    let name = component_type(ty).ok_or_else(|| {
                        Error::new(
                            &field.span,
                            "a component slot needs a component type, option, or set",
                        )
                    })?;
                    self.component_definition(name, &field.span)?;
                }
            }
        }
        for binding in self.all_bindings() {
            let mut provided = BTreeSet::new();
            for effect in &binding.effects {
                match effect {
                    Effect::Provides { component, value } => {
                        let definition = self.component_definition(component, &value.span)?;
                        if definition.component.as_ref().unwrap().abstract_ {
                            return Err(Error::new(
                                &value.span,
                                format!("cannot provide abstract component '{component}'"),
                            ));
                        }
                        if !provided.insert(component) {
                            return Err(Error::new(
                                &value.span,
                                format!(
                                    "duplicate provider for '{component}' in trait '{}'",
                                    binding.name
                                ),
                            ));
                        }
                    }
                    Effect::Modifies {
                        component,
                        field,
                        value,
                        ..
                    } => {
                        self.modifier_field(component, field.as_deref(), &value.span)?;
                    }
                }
            }
        }
        Ok(())
    }

    pub(crate) fn modifier_field(
        &self,
        component: &str,
        field: Option<&str>,
        span: &Span,
    ) -> Result<(String, Type)> {
        let definition = self.component_definition(component, span)?;
        if let Some(ty) = &definition.component.as_ref().unwrap().scalar {
            if field.is_some() {
                return Err(Error::new(
                    span,
                    "scalar component modifiers target the component directly, without .value",
                ));
            }
            if !matches!(ty, Type::Int | Type::Real) {
                return Err(Error::new(
                    span,
                    "add and multiply require a numeric scalar component",
                ));
            }
            return Ok((String::new(), ty.clone()));
        }
        let Body::Record(fields) = &definition.body else {
            unreachable!()
        };
        let field = if let Some(name) = field {
            fields.iter().find(|f| f.name == name).ok_or_else(|| {
                Error::new(
                    span,
                    format!("unknown component field '{component}.{name}'"),
                )
            })?
        } else if fields.len() == 1 {
            &fields[0]
        } else {
            return Err(Error::new(
                span,
                format!("specify a field when modifying '{component}'"),
            ));
        };
        if !matches!(field.ty, Type::Int | Type::Real) {
            return Err(Error::new(
                span,
                "add and multiply require a numeric component field",
            ));
        }
        Ok((field.name.clone(), field.ty.clone()))
    }

    fn expand_component(&mut self, name: &str, stack: &mut Vec<String>) -> Result<()> {
        let Some(mut definition) = self.definitions.get(name).cloned() else {
            return Ok(());
        };
        let Some(mut component) = definition.component.clone() else {
            return Ok(());
        };
        if component.expanded {
            return Ok(());
        }
        if stack.iter().any(|n| n == name) || stack.len() >= 32 {
            return Err(Error::new(
                &definition.span,
                format!("cyclic or excessively deep component inheritance involving '{name}'"),
            ));
        }
        stack.push(name.into());
        let own_fields = match &definition.body {
            Body::Record(fields) => fields.clone(),
            _ => Vec::new(),
        };
        let mut fields: Vec<Field> = Vec::new();
        let mut derives: Vec<Field> = Vec::new();
        let mut requires = Vec::new();
        let mut parents = BTreeSet::new();
        let mut inherited_compose: Option<Expr> = None;
        for parent in &component.parents {
            if !parents.insert(parent) {
                return Err(Error::new(&definition.span, "duplicate component base"));
            }
            self.component_definition(parent, &definition.span)?;
            self.expand_component(parent, stack)?;
            let base = &self.definitions[parent];
            if let Some(ty) = &base.component.as_ref().unwrap().scalar {
                if component.scalar.as_ref().is_some_and(|own| own != ty) {
                    return Err(Error::new(
                        &definition.span,
                        "incompatible scalar component base types",
                    ));
                }
                component.scalar = Some(ty.clone());
            }
            let base_fields = match &base.body {
                Body::Record(fields) => fields.as_slice(),
                _ => &[],
            };
            for field in base_fields {
                if let Some(previous) = fields.iter().find(|f| f.name == field.name) {
                    let same_value = previous.expr.as_ref().map(|expr| &expr.span)
                        == field.expr.as_ref().map(|expr| &expr.span);
                    let explicit = component.values.iter().any(|(name, _)| *name == field.name);
                    if previous.ty != field.ty || (!same_value && !explicit) {
                        return Err(Error::new(
                            &definition.span,
                            format!("conflicting inherited field '{}'", field.name),
                        ));
                    }
                } else {
                    fields.push(field.clone());
                }
            }
            for field in &base.derives {
                if let Some(previous) = derives.iter().find(|f| f.name == field.name) {
                    if previous.span != field.span {
                        return Err(Error::new(
                            &definition.span,
                            format!("conflicting inherited derived field '{}'", field.name),
                        ));
                    }
                } else {
                    derives.push(field.clone());
                }
            }
            requires.extend(base.requires.clone());
            if let Some(expr) = &base.component.as_ref().unwrap().compose {
                if inherited_compose
                    .as_ref()
                    .is_some_and(|previous| previous.span != expr.span)
                    && component.compose.is_none()
                {
                    return Err(Error::new(
                        &definition.span,
                        "multiple inherited composition rules need an explicit compose rule",
                    ));
                }
                inherited_compose = Some(expr.clone());
            }
            if let Some(context) = &base.context {
                if definition
                    .context
                    .as_ref()
                    .is_some_and(|own| own != context)
                {
                    return Err(Error::new(
                        &definition.span,
                        "incompatible component owner contexts",
                    ));
                }
                definition.context = Some(context.clone());
            }
        }
        let inherited: BTreeSet<_> = fields.iter().map(|f| f.name.clone()).collect();
        fields.extend(own_fields.clone());
        let mut assigned = BTreeSet::new();
        for (name, expr) in &component.values {
            if !assigned.insert(name) {
                return Err(Error::new(
                    &expr.span,
                    format!("duplicate component value '{name}'"),
                ));
            }
            let field = fields.iter_mut().find(|f| f.name == *name).ok_or_else(|| {
                Error::new(&expr.span, format!("unknown component field '{name}'"))
            })?;
            field.expr = Some(expr.clone());
        }
        if !component.abstract_ {
            for field in &fields {
                if inherited.contains(&field.name) && field.expr.is_none() {
                    return Err(Error::new(
                        &definition.span,
                        format!(
                            "component '{name}' must supply inherited value '{}'",
                            field.name
                        ),
                    ));
                }
            }
        }
        derives.extend(definition.derives);
        requires.extend(definition.requires);
        definition.body = if let Some(ty) = &component.scalar {
            if !fields.is_empty() || !component.values.is_empty() {
                return Err(Error::new(
                    &definition.span,
                    "scalar components cannot declare or inherit stored fields; provide their value through a trait",
                ));
            }
            Body::Alias(ty.clone())
        } else {
            Body::Record(fields)
        };
        definition.derives = derives;
        definition.requires = requires;
        if component.compose.is_none() {
            component.compose = inherited_compose;
        }
        component.expanded = true;
        definition.component = Some(component);
        self.definitions.insert(name.into(), definition);
        stack.pop();
        Ok(())
    }
}

#[derive(Clone, Debug)]
struct Modifier {
    target: String,
    field: String,
    operation: String,
    value: Value,
}

#[derive(Debug)]
pub(super) struct ComponentInputs {
    providers: BTreeMap<String, Value>,
    modifiers: Vec<Modifier>,
}

impl Evaluator<'_> {
    fn selected_traits(&self, value: &Value, names: &mut BTreeMap<String, (String, Vec<Value>)>) {
        for selection in &value.inline_traits {
            self.selected_traits(selection, names);
        }
        match &value.data {
            Data::TraitInstance(name, args, _) => {
                names.insert(value.identity_key(), (name.clone(), args.clone()));
            }
            Data::Ref(name)
                if self
                    .world
                    .bindings
                    .get(name)
                    .is_some_and(|b| b.kind == TypeKind::Trait && b.parameters.is_none()) =>
            {
                names.insert(name.clone(), (name.clone(), Vec::new()));
            }
            Data::Record(fields) => {
                for value in fields.values() {
                    self.selected_traits(value, names);
                }
            }
            Data::Set(values) => {
                for value in values {
                    self.selected_traits(value, names);
                }
            }
            _ => {}
        }
    }

    fn component_inputs(&mut self, object: &Value, span: &Span) -> Result<Arc<ComponentInputs>> {
        let cache_key = object.cache_key();
        if let Some(inputs) = cache_key
            .as_ref()
            .and_then(|key| self.cache.component_inputs.get(key))
        {
            return Ok(Arc::clone(inputs));
        }
        let mut traits = BTreeMap::new();
        self.selected_traits(object, &mut traits);
        let mut providers = BTreeMap::new();
        let mut modifiers = Vec::new();
        for (_, (name, args)) in traits {
            self.tick(span)?;
            let binding = self
                .world
                .template(&name, args.len())
                .or_else(|| self.world.bindings.get(&name))
                .expect("validated trait selection");
            let env = Environment {
                locals: binding
                    .parameters
                    .iter()
                    .flatten()
                    .zip(args)
                    .map(|(p, v)| (p.name.clone(), v))
                    .collect(),
            };
            for effect in &binding.effects {
                match effect {
                    Effect::Provides { component, value } => {
                        let value = self.eval(value, &env)?;
                        let value = self.normalize(
                            value,
                            &Type::Named(component.clone(), Vec::new()),
                            &binding.span,
                        )?;
                        if let Some(previous) = providers.insert(component.clone(), value.clone()) {
                            let marker =
                                matches!(&value.data, Data::Record(fields) if fields.is_empty());
                            if !marker || !previous.equal(&value) {
                                return Err(Error::new(
                                    &binding.span,
                                    format!("multiple providers for component '{component}'"),
                                ));
                            }
                        }
                    }
                    Effect::Modifies {
                        component,
                        field,
                        operation,
                        value,
                    } => {
                        let field = self
                            .world
                            .modifier_field(component, field.as_deref(), &value.span)?
                            .0;
                        modifiers.push(Modifier {
                            target: component.clone(),
                            field,
                            operation: operation.clone(),
                            value: self.eval(value, &env)?,
                        });
                    }
                }
            }
        }
        for modifier in &modifiers {
            let target = Type::Named(modifier.target.clone(), Vec::new());
            if !providers.keys().any(|name| {
                self.world
                    .subtype(&Type::Named(name.clone(), Vec::new()), &target)
            }) {
                return Err(Error::new(
                    span,
                    format!("modifier targets missing component '{}'", modifier.target),
                ));
            }
        }
        let inputs = Arc::new(ComponentInputs {
            providers,
            modifiers,
        });
        if let Some(key) = cache_key {
            for value in inputs.providers.values() {
                self.retain_result(&key, value);
            }
            for modifier in &inputs.modifiers {
                self.retain_result(&key, &modifier.value);
            }
            self.cache.component_inputs.insert(key, Arc::clone(&inputs));
        }
        Ok(inputs)
    }

    pub(super) fn validate_components(&mut self, object: &Value, span: &Span) -> Result<()> {
        let inputs = self.component_inputs(object, span)?;
        for (name, base) in &inputs.providers {
            self.compose_one(object, name, base.clone(), &inputs.modifiers, span)?;
        }
        // Component slots encode required/optional/set cardinalities. They are
        // validation obligations even though ordinary derived properties are lazy.
        for ty in &object.types {
            let Type::Named(name, _) = ty else {
                continue;
            };
            let Some(definition) = self.world.definitions.get(name) else {
                continue;
            };
            for field in &definition.derives {
                if matches!(
                    field.expr.as_ref().map(|expr| &expr.kind),
                    Some(ExprKind::Compose(_))
                ) {
                    self.field(object.clone(), &field.name, &field.span)?;
                }
            }
        }
        Ok(())
    }

    pub(super) fn compose_slot(&mut self, object: &Value, ty: &Type, span: &Span) -> Result<Value> {
        let name = component_type(ty).ok_or_else(|| Error::new(span, "invalid component slot"))?;
        let target = Type::Named(name.into(), Vec::new());
        let inputs = self.component_inputs(object, span)?;
        let mut values = Vec::new();
        for (name, base) in &inputs.providers {
            if self
                .world
                .subtype(&Type::Named(name.clone(), Vec::new()), &target)
            {
                values.push(self.compose_one(
                    object,
                    name,
                    base.clone(),
                    &inputs.modifiers,
                    span,
                )?);
            }
        }
        let value = match ty {
            Type::Set(_, _, _) => Value::new(Data::Set(values)),
            Type::Option(_) if values.is_empty() => Value::new(Data::None),
            _ if values.len() == 1 => values.remove(0),
            _ => {
                return Err(Error::new(
                    span,
                    format!(
                        "component slot '{name}' expects {} provider, found {}",
                        if matches!(ty, Type::Option(_)) {
                            "at most one"
                        } else {
                            "exactly one"
                        },
                        values.len()
                    ),
                ));
            }
        };
        self.normalize(value, ty, span)
    }

    fn modifier_values(
        &self,
        component: &str,
        field: &str,
        modifiers: &[Modifier],
    ) -> (Vec<Value>, Vec<Value>) {
        let mut adds = Vec::new();
        let mut multiplies = Vec::new();
        for modifier in modifiers {
            if modifier.field == field
                && self.world.subtype(
                    &Type::Named(component.into(), Vec::new()),
                    &Type::Named(modifier.target.clone(), Vec::new()),
                )
            {
                if modifier.operation == "add" {
                    adds.push(modifier.value.clone());
                } else {
                    multiplies.push(modifier.value.clone());
                }
            }
        }
        (adds, multiplies)
    }

    fn modifier_list(items: Vec<Value>, span: &Span) -> Result<Value> {
        let mut list = Value::new(Data::List(
            items
                .iter()
                .map(|v| real(numeric(v, span)?, span))
                .collect::<Result<Vec<_>>>()?,
        ));
        list.item_type = Some(Type::Real);
        Ok(list)
    }

    fn default_scalar(
        &mut self,
        mut value: Value,
        ty: &Type,
        adds: &[Value],
        multiplies: &[Value],
        span: &Span,
    ) -> Result<Value> {
        for item in adds {
            value = self.binary("+", value, item.clone(), span)?;
        }
        for item in multiplies {
            value = self.binary("*", value, item.clone(), span)?;
        }
        if *ty == Type::Int && matches!(value.data, Data::Real(_)) {
            value = self.builtin("round", &[value], span)?;
        }
        Ok(value)
    }

    fn compose_one(
        &mut self,
        object: &Value,
        name: &str,
        mut base: Value,
        modifiers: &[Modifier],
        span: &Span,
    ) -> Result<Value> {
        let key = format!(
            "component {}:{name}",
            object.path.as_deref().unwrap_or("object")
        );
        let cache_key = object.cache_key().map(|key| (key, name.to_owned()));
        if let Some(value) = cache_key
            .as_ref()
            .and_then(|key| self.cache.component_values.get(key))
        {
            return Ok(value.clone());
        }
        self.enter(&key, span)?;
        attach(&mut base, object.owner.clone(), Some(key.clone()));
        let definition = &self.world.definitions[name];
        if let Some(ty) = &definition.component.as_ref().unwrap().scalar {
            let (adds, multiplies) = self.modifier_values(name, "", modifiers);
            let mut result = if let Some(expr) = &definition.component.as_ref().unwrap().compose {
                let mut env = self.context(&base, definition, span)?;
                env.locals.insert("base".into(), base);
                env.locals
                    .insert("additions".into(), Self::modifier_list(adds, span)?);
                env.locals
                    .insert("multipliers".into(), Self::modifier_list(multiplies, span)?);
                self.eval(expr, &env)?
            } else {
                self.default_scalar(base, ty, &adds, &multiplies, span)?
            };
            result.types.clear();
            result.component = None;
            attach(&mut result, object.owner.clone(), Some(key.clone()));
            result = self.normalize(result, &Type::Named(name.into(), Vec::new()), span)?;
            self.validate_value(&result, span)?;
            self.active.pop();
            if let Some(cache_key) = cache_key {
                self.retain_result(&cache_key.0, &result);
                self.cache
                    .component_values
                    .insert(cache_key, result.clone());
            }
            return Ok(result);
        }
        let Body::Record(fields) = &definition.body else {
            unreachable!()
        };
        let mut additions = BTreeMap::new();
        let mut multipliers = BTreeMap::new();
        let mut result = base.clone();
        for field in fields {
            if !matches!(field.ty, Type::Int | Type::Real) {
                continue;
            }
            let (adds, multiplies) = self.modifier_values(name, &field.name, modifiers);
            if definition.component.as_ref().unwrap().compose.is_none() {
                let Data::Record(values) = &mut result.data else {
                    unreachable!()
                };
                let values = Arc::make_mut(values);
                let value = self.default_scalar(
                    values[&field.name].clone(),
                    &field.ty,
                    &adds,
                    &multiplies,
                    span,
                )?;
                values.insert(field.name.clone(), value);
            }
            additions.insert(field.name.clone(), Self::modifier_list(adds, span)?);
            multipliers.insert(field.name.clone(), Self::modifier_list(multiplies, span)?);
        }
        if let Some(expr) = &definition.component.as_ref().unwrap().compose {
            let mut env = self.context(&base, definition, span)?;
            env.locals.insert("base".into(), base);
            env.locals
                .insert("additions".into(), Value::record(additions));
            env.locals
                .insert("multipliers".into(), Value::record(multipliers));
            result = self.eval(expr, &env)?;
            attach(&mut result, object.owner.clone(), Some(key.clone()));
        }
        result.types.clear();
        result = self.normalize(result, &Type::Named(name.into(), Vec::new()), span)?;
        self.validate_value(&result, span)?;
        self.active.pop();
        if let Some(cache_key) = cache_key {
            self.retain_result(&cache_key.0, &result);
            self.cache
                .component_values
                .insert(cache_key, result.clone());
        }
        Ok(result)
    }
}
