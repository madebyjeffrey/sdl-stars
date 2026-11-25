//! Definitions are Rust data and functions; instances contain only selected values.
use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet};
use std::fmt;

macro_rules! ids {
    ($($name:ident),+ $(,)?) => {$(
        #[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
        pub struct $name(pub u64);
    )+};
}

ids!(
    EntityId,
    EntityTypeId,
    PartId,
    FamilyId,
    SlotId,
    AttributeId,
    ParameterId,
    GameId
);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Scope {
    Global,
    Game(GameId),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum ValueType {
    Integer,
    Real,
    Boolean,
    Text,
    Entity,
}

#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    Integer(i64),
    Real(f64),
    Boolean(bool),
    Text(String),
    Entity(EntityId),
}

impl Value {
    pub fn value_type(&self) -> ValueType {
        match self {
            Self::Integer(_) => ValueType::Integer,
            Self::Real(_) => ValueType::Real,
            Self::Boolean(_) => ValueType::Boolean,
            Self::Text(_) => ValueType::Text,
            Self::Entity(_) => ValueType::Entity,
        }
    }

    pub(crate) fn finite(&self) -> bool {
        !matches!(self, Self::Real(r_value) if !r_value.is_finite())
    }

    fn canonical_cmp(&self, other: &Self) -> Ordering {
        match (self, other) {
            (Self::Integer(a), Self::Integer(b)) => a.cmp(b),
            (Self::Real(a), Self::Real(b)) => a.total_cmp(b),
            (Self::Boolean(a), Self::Boolean(b)) => a.cmp(b),
            (Self::Text(a), Self::Text(b)) => a.cmp(b),
            (Self::Entity(a), Self::Entity(b)) => a.cmp(b),
            _ => self.value_type().cmp(&other.value_type()),
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Integer(n_value) => write!(f, "{n_value}"),
            Self::Real(r_value) => write!(f, "{r_value}"),
            Self::Boolean(f_value) => write!(f, "{f_value}"),
            Self::Text(sz_value) => write!(f, "{sz_value:?}"),
            Self::Entity(id) => write!(f, "entity {}", id.0),
        }
    }
}

#[derive(Clone, Debug)]
pub struct PartInstance {
    pub id: PartId,
    pub rg_arguments: BTreeMap<ParameterId, Value>,
}

impl PartialEq for PartInstance {
    fn eq(&self, other: &Self) -> bool {
        // Rust rules can distinguish signed zero; selection identity must too.
        self.canonical_cmp(other).is_eq()
    }
}

impl PartInstance {
    pub fn new(id: PartId) -> Self {
        Self {
            id,
            rg_arguments: BTreeMap::new(),
        }
    }

    pub fn argument(mut self, id: ParameterId, value: Value) -> Self {
        self.rg_arguments.insert(id, value);
        self
    }

    pub fn integer(&self, id: ParameterId) -> Result<i64, String> {
        match self.rg_arguments.get(&id) {
            Some(Value::Integer(n_value)) => Ok(*n_value),
            _ => Err(format!("parameter {} requires an integer", id.0)),
        }
    }

    pub(crate) fn canonical_cmp(&self, other: &Self) -> Ordering {
        self.id.cmp(&other.id).then_with(|| {
            self.rg_arguments
                .iter()
                .zip(&other.rg_arguments)
                .map(|((id_a, a), (id_b, b))| id_a.cmp(id_b).then_with(|| a.canonical_cmp(b)))
                .find(|order| !order.is_eq())
                .unwrap_or_else(|| self.rg_arguments.len().cmp(&other.rg_arguments.len()))
        })
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct Entity {
    pub id: EntityId,
    pub id_type: EntityTypeId,
    pub scope: Scope,
    pub rg_inputs: BTreeMap<AttributeId, Value>,
    pub rg_slots: BTreeMap<SlotId, Vec<PartInstance>>,
}

impl Entity {
    pub fn new(id: EntityId, id_type: EntityTypeId, scope: Scope) -> Self {
        Self {
            id,
            id_type,
            scope,
            rg_inputs: BTreeMap::new(),
            rg_slots: BTreeMap::new(),
        }
    }

    pub fn input(mut self, id: AttributeId, value: Value) -> Self {
        self.rg_inputs.insert(id, value);
        self
    }

    pub fn parts(mut self, id: SlotId, rg_parts: Vec<PartInstance>) -> Self {
        self.rg_slots.insert(id, rg_parts);
        self
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Base,
    Add,
    Multiply,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Contribution {
    pub id_attribute: AttributeId,
    pub operation: Operation,
    pub value: Value,
}

impl Contribution {
    pub fn new(id_attribute: AttributeId, operation: Operation, value: Value) -> Self {
        Self {
            id_attribute,
            operation,
            value,
        }
    }
}

pub type CheckValue = fn(&Value) -> Result<(), String>;
pub type ExpandPart = fn(&PartInstance) -> Result<Vec<Contribution>, String>;
pub type CheckPart = fn(&PartInstance) -> Result<(), String>;
pub type DerivedRule = fn(&mut dyn Inputs, EntityId) -> Result<Value, EvalError>;

pub trait Inputs {
    fn get(&mut self, id_entity: EntityId, id_attribute: AttributeId) -> Result<Value, EvalError>;
    fn note(&mut self, sz_description: &str, value: Value);
}

#[derive(Clone)]
pub enum AttributeSource {
    Input,
    /// One base, checked integer additions or real additions, product of real factors.
    /// An integer destination rounds once, half away from zero, after scaling.
    Numeric,
    Derived(DerivedRule),
}

#[derive(Clone)]
pub struct AttributeDefinition {
    pub sz_name: String,
    pub value_type: ValueType,
    pub source: AttributeSource,
    pub check: Option<CheckValue>,
}

#[derive(Clone)]
pub struct ParameterDefinition {
    pub sz_name: String,
    pub value_type: ValueType,
}

#[derive(Clone)]
pub struct PartDefinition {
    pub sz_name: String,
    pub id_family: FamilyId,
    pub rg_parameters: BTreeMap<ParameterId, ParameterDefinition>,
    pub check: Option<CheckPart>,
    pub expand: ExpandPart,
}

#[derive(Clone)]
pub struct SlotDefinition {
    pub sz_name: String,
    pub id_family: FamilyId,
    pub c_min: usize,
    pub c_max: Option<usize>,
}

#[derive(Clone)]
pub struct EntityDefinition {
    pub sz_name: String,
    pub rg_slots: BTreeMap<SlotId, SlotDefinition>,
    pub rg_attributes: BTreeSet<AttributeId>,
}

#[derive(Clone, Default)]
pub struct Definitions {
    pub rg_attributes: BTreeMap<AttributeId, AttributeDefinition>,
    pub rg_parts: BTreeMap<PartId, PartDefinition>,
    pub rg_entities: BTreeMap<EntityTypeId, EntityDefinition>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Violation {
    pub sz_path: String,
    pub sz_message: String,
}

impl Violation {
    pub(crate) fn new(sz_path: impl Into<String>, sz_message: impl Into<String>) -> Self {
        Self {
            sz_path: sz_path.into(),
            sz_message: sz_message.into(),
        }
    }
}

impl fmt::Display for Violation {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}: {}", self.sz_path, self.sz_message)
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum EvalError {
    NotVisible(EntityId),
    UnknownAttribute(AttributeId),
    Cycle(Vec<(EntityId, AttributeId)>),
    Invalid(String),
    Arithmetic(String),
}

impl fmt::Display for EvalError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::NotVisible(id) => write!(f, "entity {} is unavailable in this context", id.0),
            Self::UnknownAttribute(id) => {
                write!(f, "attribute {} is unavailable on this entity", id.0)
            }
            Self::Cycle(rg_path) => write!(f, "attribute dependency cycle: {rg_path:?}"),
            Self::Invalid(sz_message) | Self::Arithmetic(sz_message) => write!(f, "{sz_message}"),
        }
    }
}

impl std::error::Error for EvalError {}
