//! Checks rules and queries even when their source collection is empty.
use std::collections::{BTreeMap, BTreeSet};

use crate::ast::*;
use crate::diagnostic::{Error, Result, Span};
use crate::engine::{World, builtin, substitute, substitutions};

#[derive(Clone, Debug, PartialEq)]
enum Shape {
    Known(Type),
    Record(BTreeMap<String, Shape>),
    Set(Box<Shape>),
    List(Box<Shape>),
    None,
    Tag(String),
    Function(String, Vec<Shape>),
    Empty,
}

type Locals = BTreeMap<String, Shape>;

pub(crate) fn check_world(world: &World) -> Result<()> {
    let mut checker = Checker::new(world);
    for definition in world.definitions.values() {
        let ty = Type::Named(
            definition.name.clone(),
            definition
                .params
                .iter()
                .map(|name| Type::Var(name.clone()))
                .collect(),
        );
        let mut locals = Locals::from([("self".into(), Shape::Known(ty))]);
        if let Some((name, ty)) = &definition.context {
            locals.insert(name.clone(), Shape::Known(ty.clone()));
        }
        for field in &definition.derives {
            let shape = checker.infer(field.expr.as_ref().unwrap(), &locals)?;
            checker.accepts(&shape, &field.ty, &field.span, 0)?;
        }
        for requirement in &definition.requires {
            let shape = checker.infer(requirement, &locals)?;
            checker.accepts(&shape, &Type::Bool, &requirement.span, 0)?;
        }
    }
    for binding in world.bindings.values() {
        checker.binding(&binding.name, &binding.span)?;
    }
    Ok(())
}

pub(crate) fn check_expression(world: &World, expression: &Expr) -> Result<()> {
    Checker::new(world)
        .infer(expression, &Locals::new())
        .map(|_| ())
}

pub(crate) fn query_item_type(
    world: &World,
    query: &Query,
    locals: &BTreeMap<String, crate::Value>,
) -> Option<Type> {
    fn shape(world: &World, value: &crate::Value) -> Shape {
        use crate::value::Data;
        if let Some(ty) = value.types.last() {
            return Shape::Known(ty.clone());
        }
        match &value.data {
            Data::Int(_) => Shape::Known(Type::Int),
            Data::Real(_) => Shape::Known(Type::Real),
            Data::String(_) => Shape::Known(Type::String),
            Data::Bool(_) => Shape::Known(Type::Bool),
            Data::Ref(name) => Shape::Known(world.bindings[name].ty.clone().unwrap()),
            Data::None => Shape::None,
            Data::Tag(name) => Shape::Tag(name.clone()),
            Data::Record(fields) => Shape::Record(
                fields
                    .iter()
                    .map(|(name, value)| (name.clone(), shape(world, value)))
                    .collect(),
            ),
            Data::Set(values) => Shape::Set(Box::new(
                values
                    .first()
                    .map(|value| shape(world, value))
                    .unwrap_or(Shape::Empty),
            )),
            Data::List(_) => Shape::List(Box::new(
                value
                    .item_type
                    .clone()
                    .map(Shape::Known)
                    .unwrap_or(Shape::Empty),
            )),
            Data::Function(name, _) => Shape::Function(name.clone(), Vec::new()),
        }
    }
    let mut locals: Locals = locals
        .iter()
        .map(|(name, value)| (name.clone(), shape(world, value)))
        .collect();
    for (name, ty) in &query.bindings {
        locals.insert(name.clone(), Shape::Known(ty.clone()));
    }
    match Checker::new(world).infer(&query.select, &locals).ok()? {
        Shape::Known(ty) => Some(ty),
        _ => None,
    }
}

struct Checker<'a> {
    world: &'a World,
    cache: BTreeMap<String, Shape>,
    active: BTreeSet<String>,
    depth: usize,
}

impl<'a> Checker<'a> {
    fn new(world: &'a World) -> Self {
        Self {
            world,
            cache: BTreeMap::new(),
            active: BTreeSet::new(),
            depth: 0,
        }
    }

    fn binding(&mut self, name: &str, span: &Span) -> Result<Shape> {
        if let Some(shape) = self.cache.get(name) {
            return Ok(shape.clone());
        }
        let binding = &self.world.bindings[name];
        if !self.active.insert(name.into()) {
            return Err(Error::new(span, format!("cyclic value binding '{name}'")));
        }
        let shape = self.infer(&binding.expr, &Locals::new())?;
        let result = if let Some(ty) = &binding.ty {
            self.accepts(&shape, ty, &binding.expr.span, 0)?;
            Shape::Known(ty.clone())
        } else {
            shape
        };
        self.active.remove(name);
        self.cache.insert(name.into(), result.clone());
        Ok(result)
    }

    fn infer(&mut self, expression: &Expr, locals: &Locals) -> Result<Shape> {
        self.depth += 1;
        if self.depth > 32 {
            return Err(Error::new(
                &expression.span,
                "type-check nesting limit exceeded",
            ));
        }
        let result = self.infer_inner(expression, locals);
        self.depth -= 1;
        result
    }

    fn infer_inner(&mut self, expression: &Expr, locals: &Locals) -> Result<Shape> {
        let span = &expression.span;
        Ok(match &expression.kind {
            ExprKind::Int(_) => Shape::Known(Type::Int),
            ExprKind::Real(_) => Shape::Known(Type::Real),
            ExprKind::String(_) => Shape::Known(Type::String),
            ExprKind::Bool(_) => Shape::Known(Type::Bool),
            ExprKind::None => Shape::None,
            ExprKind::Name(name) => {
                if let Some(shape) = locals.get(name) {
                    shape.clone()
                } else if let Some(binding) = self.world.bindings.get(name) {
                    if binding.kind == TypeKind::Value {
                        self.binding(name, span)?
                    } else {
                        Shape::Known(binding.ty.clone().unwrap())
                    }
                } else if let Some(fields) = self.world.constructors.get(name) {
                    if fields.is_empty() {
                        Shape::Tag(name.clone())
                    } else {
                        Shape::Function(name.clone(), Vec::new())
                    }
                } else if builtin(name).is_some()
                    || self.world.definitions.contains_key(name)
                    || matches!(name.as_str(), "range" | "Set" | "List")
                {
                    Shape::Function(name.clone(), Vec::new())
                } else {
                    return Err(Error::new(span, format!("unknown name '{name}'")));
                }
            }
            ExprKind::Record(fields) => {
                let mut shapes = BTreeMap::new();
                for (name, value) in fields {
                    let shape = self.infer(value, locals)?;
                    if shapes.insert(name.clone(), shape).is_some() {
                        return Err(Error::new(&value.span, format!("duplicate field '{name}'")));
                    }
                }
                Shape::Record(shapes)
            }
            ExprKind::Set(values) => {
                let mut element = Shape::Empty;
                for value in values {
                    let shape = self.infer(value, locals)?;
                    element = self.unify(&element, &shape, &value.span)?;
                }
                Shape::Set(Box::new(element))
            }
            ExprKind::Field(receiver, name) => {
                let receiver = self.infer(receiver, locals)?;
                self.field(&receiver, name, span, 0)?
            }
            ExprKind::Call(function, argument) => {
                let function = self.infer(function, locals)?;
                let argument = self.infer(argument, locals)?;
                self.apply(function, argument, span)?
            }
            ExprKind::Unary(operator, value) => {
                let shape = self.infer(value, locals)?;
                if operator == "not" {
                    self.accepts(&shape, &Type::Bool, span, 0)?;
                    Shape::Known(Type::Bool)
                } else {
                    self.number(&shape, span)?;
                    shape
                }
            }
            ExprKind::Binary(operator, left, right) => {
                let left = self.infer(left, locals)?;
                let right = self.infer(right, locals)?;
                match operator.as_str() {
                    "=" | "==" | "!=" | "<>" => {
                        self.compatible(&left, &right, span)?;
                        Shape::Known(Type::Bool)
                    }
                    "<" | ">" | "<=" | ">=" => {
                        self.ordered(&left, span)?;
                        self.unify(&left, &right, span)?;
                        Shape::Known(Type::Bool)
                    }
                    "&&" | "and" | "||" | "or" => {
                        self.accepts(&left, &Type::Bool, span, 0)?;
                        self.accepts(&right, &Type::Bool, span, 0)?;
                        Shape::Known(Type::Bool)
                    }
                    "+" if left == Shape::Known(Type::String) && right == left => left,
                    ".." => {
                        self.number(&left, span)?;
                        self.number(&right, span)?;
                        let ty = if left == Shape::Known(Type::Real)
                            || right == Shape::Known(Type::Real)
                        {
                            Type::Real
                        } else {
                            Type::Int
                        };
                        Shape::Known(Type::Named(
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
                        ))
                    }
                    _ => {
                        self.number(&left, span)?;
                        self.number(&right, span)?;
                        if left == Shape::Known(Type::Real) || right == Shape::Known(Type::Real) {
                            Shape::Known(Type::Real)
                        } else {
                            self.unify(&left, &right, span)?
                        }
                    }
                }
            }
            ExprKind::If(condition, yes, no) => {
                let condition = self.infer(condition, locals)?;
                self.accepts(&condition, &Type::Bool, span, 0)?;
                let yes = self.infer(yes, locals)?;
                let no = self.infer(no, locals)?;
                self.unify(&yes, &no, span)?
            }
            ExprKind::Let(name, value, body) => {
                let value = self.infer(value, locals)?;
                let mut locals = locals.clone();
                locals.insert(name.clone(), value);
                self.infer(body, &locals)?
            }
            ExprKind::Match(value, arms) => {
                let shape = self.infer(value, locals)?;
                let cases = self.cases(&shape, span, 0)?;
                let mut seen = BTreeSet::new();
                let mut wildcard = false;
                let mut result = Shape::Empty;
                for (pattern, body) in arms {
                    if wildcard {
                        return Err(Error::new(
                            &body.span,
                            "unreachable match arm after wildcard",
                        ));
                    }
                    let mut arm_locals = locals.clone();
                    match pattern {
                        Pattern::Wildcard => wildcard = true,
                        Pattern::Case(name, fields) => {
                            if !seen.insert(name.clone()) {
                                return Err(Error::new(
                                    &body.span,
                                    format!("duplicate match case '{name}'"),
                                ));
                            }
                            let case = cases.get(name).ok_or_else(|| {
                                Error::new(span, format!("'{name}' is not a case of this type"))
                            })?;
                            if let ExprKind::Name(name) = &value.kind {
                                arm_locals.insert(name.clone(), case.clone());
                            }
                            let mut bindings = BTreeSet::new();
                            if let Some(fields) = fields {
                                for (field, binding) in fields {
                                    if !bindings.insert(binding) {
                                        return Err(Error::new(span, "duplicate pattern binding"));
                                    }
                                    arm_locals
                                        .insert(binding.clone(), self.field(case, field, span, 0)?);
                                }
                            }
                        }
                    }
                    let arm = self.infer(body, &arm_locals)?;
                    result = self.unify(&result, &arm, &body.span)?;
                }
                if !wildcard && cases.keys().any(|name| !seen.contains(name)) {
                    return Err(Error::new(span, "non-exhaustive match"));
                }
                result
            }
            ExprKind::Query(query) => {
                let mut locals = locals.clone();
                let mut names = BTreeSet::new();
                for (name, ty) in &query.bindings {
                    self.world.check_type(ty, &[], span)?;
                    if !names.insert(name) {
                        return Err(Error::new(
                            span,
                            format!("duplicate query binding '{name}'"),
                        ));
                    }
                    if !matches!(ty, Type::Named(name, _) if self.world.definitions.get(name).is_some_and(|definition| definition.kind != TypeKind::Value))
                    {
                        return Err(Error::new(span, "from expects an object or trait type"));
                    }
                    locals.insert(name.clone(), Shape::Known(ty.clone()));
                }
                for filter in &query.filters {
                    let shape = self.infer(filter, &locals)?;
                    self.accepts(&shape, &Type::Bool, &filter.span, 0)?;
                }
                if let Some((order, _)) = &query.order {
                    let shape = self.infer(order, &locals)?;
                    self.ordered(&shape, &order.span)?;
                }
                Shape::List(Box::new(self.infer(&query.select, &locals)?))
            }
        })
    }

    fn field(&self, shape: &Shape, name: &str, span: &Span, depth: usize) -> Result<Shape> {
        if depth > 32 {
            return Err(Error::new(span, "field type nesting limit exceeded"));
        }
        match shape {
            Shape::Function(module, args)
                if args.is_empty() && matches!(module.as_str(), "Set" | "List") =>
            {
                let name = format!("{module}.{name}");
                if builtin(&name).is_some() {
                    return Ok(Shape::Function(name, Vec::new()));
                }
            }
            Shape::Record(fields) => {
                if let Some(shape) = fields.get(name) {
                    return Ok(shape.clone());
                }
            }
            Shape::Known(Type::Named(type_name, args)) => {
                if let Some(definition) = self.world.definitions.get(type_name) {
                    let substitutions = substitutions(definition, args);
                    if let Some(field) = definition.derives.iter().find(|field| field.name == name)
                    {
                        return Ok(Shape::Known(substitute(&field.ty, &substitutions)));
                    }
                    match &definition.body {
                        Body::Record(fields) => {
                            if let Some(field) = fields.iter().find(|field| field.name == name) {
                                return Ok(Shape::Known(substitute(&field.ty, &substitutions)));
                            }
                        }
                        Body::Alias(ty) => {
                            return self.field(
                                &Shape::Known(substitute(ty, &substitutions)),
                                name,
                                span,
                                depth + 1,
                            );
                        }
                        _ => {}
                    }
                } else if type_name == "range"
                    && args.len() == 1
                    && matches!(name, "lower" | "upper")
                {
                    return Ok(Shape::Known(args[0].clone()));
                } else if let Some(fields) = self.world.constructors.get(type_name)
                    && let Some(field) = fields.iter().find(|field| field.name == name)
                {
                    return Ok(Shape::Known(field.ty.clone()));
                }
            }
            _ => {}
        }
        Err(Error::new(
            span,
            format!("unknown field '{name}' on {shape:?}"),
        ))
    }

    fn accepts(&self, shape: &Shape, ty: &Type, span: &Span, depth: usize) -> Result<()> {
        if depth > 32 {
            return Err(Error::new(
                span,
                "type compatibility nesting limit exceeded",
            ));
        }
        if let Shape::Known(actual) = shape
            && (actual == ty || matches!((actual, ty), (Type::Int, Type::Real)))
        {
            return Ok(());
        }
        match ty {
            Type::Var(_) => return Ok(()),
            Type::Option(inner) => {
                if matches!(shape, Shape::None) {
                    return Ok(());
                }
                return self.accepts(shape, inner, span, depth + 1);
            }
            Type::Set(inner, _, _) => {
                if let Shape::Set(shape) = shape {
                    if **shape == Shape::Empty {
                        return Ok(());
                    }
                    return self.accepts(shape, inner, span, depth + 1);
                }
            }
            Type::Named(name, args) => {
                if let Some(definition) = self.world.definitions.get(name) {
                    let substitutions = substitutions(definition, args);
                    match &definition.body {
                        Body::Record(fields) => {
                            if let Shape::Record(shapes) = shape {
                                return self.accept_fields(
                                    shapes,
                                    fields,
                                    &substitutions,
                                    span,
                                    depth + 1,
                                );
                            }
                            if matches!(shape, Shape::Set(inner) if **inner == Shape::Empty) {
                                return self.accept_fields(
                                    &BTreeMap::new(),
                                    fields,
                                    &substitutions,
                                    span,
                                    depth + 1,
                                );
                            }
                        }
                        Body::Alias(inner) => {
                            return self.accepts(
                                shape,
                                &substitute(inner, &substitutions),
                                span,
                                depth + 1,
                            );
                        }
                        Body::Union(alternatives) => {
                            for alternative in alternatives {
                                let accepted = match alternative {
                                    Alternative::Type(Type::Named(tag, args))
                                        if args.is_empty()
                                            && self.world.constructors.contains_key(tag) =>
                                    {
                                        matches!(shape, Shape::Tag(name) if name == tag)
                                    }
                                    Alternative::Constructor(name, _) => {
                                        matches!(shape, Shape::Known(Type::Named(actual, _)) if actual == name)
                                    }
                                    Alternative::Type(inner) => self
                                        .accepts(
                                            shape,
                                            &substitute(inner, &substitutions),
                                            span,
                                            depth + 1,
                                        )
                                        .is_ok(),
                                };
                                if accepted {
                                    return Ok(());
                                }
                            }
                        }
                    }
                } else if name == "range"
                    && args.len() == 1
                    && let Shape::Record(shapes) = shape
                    && shapes.len() == 2
                    && ["lower", "upper"]
                        .iter()
                        .all(|name| shapes.contains_key(*name))
                {
                    for shape in shapes.values() {
                        self.accepts(shape, &args[0], span, depth + 1)?;
                    }
                    return Ok(());
                }
            }
            _ => {}
        }
        Err(Error::new(span, format!("expected {ty}, got {shape:?}")))
    }

    fn accept_fields(
        &self,
        shapes: &BTreeMap<String, Shape>,
        fields: &[Field],
        substitutions: &BTreeMap<String, Type>,
        span: &Span,
        depth: usize,
    ) -> Result<()> {
        for name in shapes.keys() {
            if !fields.iter().any(|field| &field.name == name) {
                return Err(Error::new(
                    span,
                    format!("unknown or derived field '{name}'"),
                ));
            }
        }
        for field in fields {
            let ty = substitute(&field.ty, substitutions);
            if let Some(shape) = shapes.get(&field.name) {
                self.accepts(shape, &ty, span, depth).map_err(|mut error| {
                    error.message = format!("field '{}': {}", field.name, error.message);
                    error
                })?;
            } else if !matches!(ty, Type::Option(_) | Type::Set(_, 0, _)) {
                return Err(Error::new(
                    span,
                    format!("missing required field '{}'", field.name),
                ));
            }
        }
        Ok(())
    }

    fn compatible(&self, left: &Shape, right: &Shape, span: &Span) -> Result<()> {
        if left == right {
            return Ok(());
        }
        if matches!(
            (left, right),
            (Shape::Known(Type::Int), Shape::Known(Type::Real))
                | (Shape::Known(Type::Real), Shape::Known(Type::Int))
        ) {
            return Err(Error::new(
                span,
                "compare int and real with an explicit conversion",
            ));
        }
        if let Shape::Known(ty) = left
            && self.accepts(right, ty, span, 0).is_ok()
        {
            return Ok(());
        }
        if let Shape::Known(ty) = right
            && self.accepts(left, ty, span, 0).is_ok()
        {
            return Ok(());
        }
        Err(Error::new(span, "comparison requires compatible types"))
    }

    fn unify(&self, left: &Shape, right: &Shape, span: &Span) -> Result<Shape> {
        if left == &Shape::Empty {
            return Ok(right.clone());
        }
        if right == &Shape::Empty || left == right {
            return Ok(left.clone());
        }
        match (left, right) {
            (Shape::Known(Type::Var(_)), Shape::Known(Type::Int | Type::Real)) => Ok(right.clone()),
            (Shape::Known(Type::Int | Type::Real), Shape::Known(Type::Var(_))) => Ok(left.clone()),
            (Shape::Known(ty), Shape::None) | (Shape::None, Shape::Known(ty)) => {
                Ok(Shape::Known(if matches!(ty, Type::Option(_)) {
                    ty.clone()
                } else {
                    Type::Option(Box::new(ty.clone()))
                }))
            }
            _ => Err(Error::new(
                span,
                format!("incompatible expression types: {left:?} and {right:?}"),
            )),
        }
    }

    fn number(&self, shape: &Shape, span: &Span) -> Result<()> {
        if matches!(shape, Shape::Known(Type::Int | Type::Real | Type::Var(_))) {
            Ok(())
        } else {
            Err(Error::new(span, "expected a numeric expression"))
        }
    }

    fn ordered(&self, shape: &Shape, span: &Span) -> Result<()> {
        if matches!(
            shape,
            Shape::Known(Type::Int | Type::Real | Type::String | Type::Bool | Type::Var(_))
        ) {
            Ok(())
        } else {
            Err(Error::new(span, "expected an ordered scalar expression"))
        }
    }

    fn cases(&self, shape: &Shape, span: &Span, depth: usize) -> Result<BTreeMap<String, Shape>> {
        if depth > 32 {
            return Err(Error::new(span, "match type nesting limit exceeded"));
        }
        let mut cases = BTreeMap::new();
        match shape {
            Shape::Known(Type::Named(name, args)) => {
                if let Some(definition) = self.world.definitions.get(name) {
                    let substitutions = substitutions(definition, args);
                    match &definition.body {
                        Body::Union(alternatives) => {
                            for alternative in alternatives {
                                match alternative {
                                    Alternative::Type(Type::Named(name, args))
                                        if args.is_empty()
                                            && self.world.constructors.contains_key(name) =>
                                    {
                                        cases.insert(name.clone(), Shape::Tag(name.clone()));
                                    }
                                    Alternative::Constructor(name, fields) => {
                                        cases.insert(
                                            name.clone(),
                                            Shape::Record(
                                                fields
                                                    .iter()
                                                    .map(|field| {
                                                        (
                                                            field.name.clone(),
                                                            Shape::Known(substitute(
                                                                &field.ty,
                                                                &substitutions,
                                                            )),
                                                        )
                                                    })
                                                    .collect(),
                                            ),
                                        );
                                    }
                                    Alternative::Type(ty) => {
                                        let ty = substitute(ty, &substitutions);
                                        if let Type::Named(name, _) = &ty {
                                            cases.insert(name.clone(), Shape::Known(ty));
                                        } else {
                                            return Err(Error::new(
                                                span,
                                                "match currently requires named alternatives",
                                            ));
                                        }
                                    }
                                }
                            }
                        }
                        Body::Alias(ty) => {
                            return self.cases(
                                &Shape::Known(substitute(ty, &substitutions)),
                                span,
                                depth + 1,
                            );
                        }
                        _ => {
                            cases.insert(name.clone(), shape.clone());
                        }
                    }
                } else {
                    cases.insert(name.clone(), shape.clone());
                }
            }
            Shape::Known(Type::Option(inner)) => {
                cases = self.cases(&Shape::Known(*inner.clone()), span, depth + 1)?;
                cases.insert("none".into(), Shape::None);
            }
            Shape::Tag(name) => {
                cases.insert(name.clone(), shape.clone());
            }
            Shape::None => {
                cases.insert("none".into(), Shape::None);
            }
            _ => {
                return Err(Error::new(
                    span,
                    "match expects a variant or optional named type",
                ));
            }
        }
        Ok(cases)
    }

    fn apply(&self, function: Shape, argument: Shape, span: &Span) -> Result<Shape> {
        let Shape::Function(name, mut args) = function else {
            return Err(Error::new(
                span,
                "value is not a function or type constructor",
            ));
        };
        args.push(argument);
        if let Some(arity) = builtin(&name) {
            if args.len() < arity {
                return Ok(Shape::Function(name, args));
            }
            return self.builtin(&name, &args, span);
        }
        if args.len() != 1 {
            return Err(Error::new(span, "constructor expects one field collection"));
        }
        let shape = &args[0];
        if let Some(fields) = self.world.constructors.get(&name) {
            let Shape::Record(shapes) = shape else {
                return Err(Error::new(span, "constructor expects named fields"));
            };
            self.accept_fields(shapes, fields, &BTreeMap::new(), span, 0)?;
            return Ok(Shape::Known(Type::Named(name, Vec::new())));
        }
        let mut inferred = BTreeMap::new();
        let params = if let Some(definition) = self.world.definitions.get(&name) {
            if definition.kind != TypeKind::Value {
                return Err(Error::new(
                    span,
                    "named entities must be declared at the top level",
                ));
            }
            if let (Body::Record(fields), Shape::Record(shapes)) = (&definition.body, shape) {
                for field in fields {
                    if let (Type::Var(param), Some(Shape::Known(ty))) =
                        (&field.ty, shapes.get(&field.name))
                        && let Some(previous) = inferred.insert(param.clone(), ty.clone())
                        && previous != *ty
                    {
                        return Err(Error::new(span, "inconsistent constructor type arguments"));
                    }
                }
            }
            definition.params.clone()
        } else if name == "range" {
            if let Shape::Record(shapes) = shape
                && let Some(Shape::Known(ty)) = shapes.get("lower")
            {
                inferred.insert("a".into(), ty.clone());
            }
            vec!["a".into()]
        } else {
            return Err(Error::new(span, format!("unknown constructor '{name}'")));
        };
        let args = params
            .iter()
            .map(|param| {
                inferred.get(param).cloned().ok_or_else(|| {
                    Error::new(span, format!("cannot infer type parameter '{param}"))
                })
            })
            .collect::<Result<Vec<_>>>()?;
        let ty = Type::Named(name, args);
        self.world.check_type(&ty, &[], span)?;
        self.accepts(shape, &ty, span, 0)?;
        Ok(Shape::Known(ty))
    }

    fn builtin(&self, name: &str, args: &[Shape], span: &Span) -> Result<Shape> {
        match name {
            "round" | "floor" | "ceil" | "int" | "integer" => {
                self.number(&args[0], span)?;
                Ok(Shape::Known(Type::Int))
            }
            "real" => {
                self.number(&args[0], span)?;
                Ok(Shape::Known(Type::Real))
            }
            "abs" => {
                self.number(&args[0], span)?;
                Ok(args[0].clone())
            }
            "min" | "max" => {
                self.ordered(&args[0], span)?;
                self.unify(&args[0], &args[1], span)
            }
            "Set.size" => {
                if !matches!(args[0], Shape::Set(_) | Shape::Known(Type::Set(_, _, _))) {
                    return Err(Error::new(span, "Set.size expects a set"));
                }
                Ok(Shape::Known(Type::Int))
            }
            "List.length" => {
                if !matches!(args[0], Shape::List(_)) {
                    return Err(Error::new(span, "List.length expects a query result"));
                }
                Ok(Shape::Known(Type::Int))
            }
            "List.sum" => {
                let Shape::List(inner) = &args[0] else {
                    return Err(Error::new(span, "List.sum expects a numeric query result"));
                };
                self.number(inner, span)?;
                Ok(*inner.clone())
            }
            "Set.contains" => {
                match &args[1] {
                    Shape::Known(Type::Set(inner, _, _)) => {
                        self.accepts(&args[0], inner, span, 0)?
                    }
                    Shape::Set(inner) if **inner == Shape::Empty => {}
                    Shape::Set(inner) => self.compatible(&args[0], inner, span)?,
                    _ => return Err(Error::new(span, "Set.contains expects a value and a set")),
                }
                Ok(Shape::Known(Type::Bool))
            }
            _ => Err(Error::new(span, "unknown builtin function")),
        }
    }
}
