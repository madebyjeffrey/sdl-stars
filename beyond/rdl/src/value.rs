use std::collections::{BTreeMap, HashSet};
use std::fmt;
use std::sync::Arc;

use crate::ast::Type;

#[derive(Clone, Debug)]
pub struct Value {
    pub(crate) data: Data,
    pub(crate) types: Vec<Type>,
    pub(crate) owner: Option<String>,
    pub(crate) path: Option<String>,
    pub(crate) item_type: Option<Type>,
    pub(crate) component: Option<String>,
    pub(crate) evaluation_id: Option<usize>,
    pub(crate) inline_traits: Vec<Value>,
}

// An evaluation ID belongs to one normalized value revision. Type and owner
// metadata remain part of the key when a value is reused in a different context.
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub(crate) struct ValueKey {
    id: usize,
    types: Vec<Type>,
    owner: Option<String>,
    component: Option<String>,
}

impl ValueKey {
    pub(crate) fn revision(&self) -> usize {
        self.id
    }
}

#[derive(Clone, Debug)]
pub(crate) enum Data {
    Int(i64),
    Real(f64),
    String(String),
    Bool(bool),
    None,
    Tag(String),
    Ref(String),
    TraitInstance(String, Vec<Value>, Arc<Value>),
    Record(Arc<BTreeMap<String, Value>>),
    Set(Vec<Value>),
    List(Vec<Value>),
    Function(String, Vec<Value>),
}

impl Value {
    pub(crate) fn new(data: Data) -> Self {
        Self {
            data,
            types: Vec::new(),
            owner: None,
            path: None,
            item_type: None,
            component: None,
            evaluation_id: None,
            inline_traits: Vec::new(),
        }
    }
    // Cloning a record shares its immutable payload. Mutation sites use
    // Arc::make_mut so normalizing or attaching one copy cannot alter another.
    pub(crate) fn record(fields: BTreeMap<String, Value>) -> Self {
        Self::new(Data::Record(Arc::new(fields)))
    }

    pub fn as_int(&self) -> Option<i64> {
        if let Data::Int(value) = self.data {
            Some(value)
        } else {
            None
        }
    }
    pub fn as_real(&self) -> Option<f64> {
        if let Data::Real(value) = self.data {
            Some(value)
        } else {
            None
        }
    }
    pub fn as_str(&self) -> Option<&str> {
        if let Data::String(value) = &self.data {
            Some(value)
        } else {
            None
        }
    }
    pub fn as_bool(&self) -> Option<bool> {
        if let Data::Bool(value) = self.data {
            Some(value)
        } else {
            None
        }
    }
    pub fn items(&self) -> Option<&[Value]> {
        match &self.data {
            Data::Set(values) | Data::List(values) => Some(values),
            _ => None,
        }
    }
    pub fn fields(&self) -> Option<&BTreeMap<String, Value>> {
        match &self.data {
            Data::Record(fields) => Some(fields),
            Data::TraitInstance(_, _, metadata) => metadata.fields(),
            _ => None,
        }
    }
    pub fn reference(&self) -> Option<&str> {
        if let Data::Ref(name) = &self.data {
            Some(name)
        } else {
            None
        }
    }

    pub(crate) fn retain_ids(&self, retained: &mut HashSet<usize>) {
        if let Some(id) = self.evaluation_id
            && !retained.insert(id)
        {
            return;
        }
        for selection in &self.inline_traits {
            selection.retain_ids(retained);
        }
        match &self.data {
            Data::Record(fields) => {
                for value in fields.values() {
                    value.retain_ids(retained);
                }
            }
            Data::TraitInstance(_, args, metadata) => {
                metadata.retain_ids(retained);
                for value in args {
                    value.retain_ids(retained);
                }
            }
            Data::Set(values) | Data::List(values) | Data::Function(_, values) => {
                for value in values {
                    value.retain_ids(retained);
                }
            }
            _ => {}
        }
    }

    pub(crate) fn cache_key(&self) -> Option<ValueKey> {
        self.evaluation_id.map(|id| ValueKey {
            id,
            types: self.types.clone(),
            owner: self.owner.clone(),
            component: self.component.clone(),
        })
    }

    pub(crate) fn identity_key(&self) -> String {
        self.key(false)
    }

    // Logical trait identity is independent of attachment. Evaluation must also
    // distinguish owners, including contextual traits nested inside arguments.
    pub(crate) fn argument_key(&self) -> String {
        self.key(true)
    }

    fn key(&self, contextual: bool) -> String {
        let key = |value: &Self| value.key(contextual);
        let data = match &self.data {
            Data::Ref(name) => return format!("reference:{name:?}"),
            Data::TraitInstance(name, args, _) => {
                let data = format!(
                    "trait:{name:?}:{:?}",
                    args.iter().map(key).collect::<Vec<_>>()
                );
                if !contextual {
                    return data;
                }
                data
            }
            Data::Record(fields) => format!(
                "record:{:?}",
                fields
                    .iter()
                    .map(|(name, value)| (name, key(value)))
                    .collect::<Vec<_>>()
            ),
            Data::Set(values) => {
                let mut keys: Vec<_> = values.iter().map(key).collect();
                keys.sort();
                format!("set:{keys:?}")
            }
            Data::List(values) => format!("list:{:?}", values.iter().map(key).collect::<Vec<_>>()),
            Data::Real(value) if *value == 0.0 => "0.0".into(),
            _ => self.to_string(),
        };
        if contextual {
            return format!(
                "{:?}:{:?}:{:?}:{:?}:{data}",
                self.component,
                self.types,
                self.owner,
                self.inline_traits.iter().map(key).collect::<Vec<_>>()
            );
        }
        let nominal = self.types.iter().any(|ty| matches!(ty, Type::Named(_, _)));
        if !nominal && self.component.is_none() {
            return format!("primitive:{data}");
        }
        format!(
            "{:?}:{:?}:{:?}:{data}",
            self.component,
            self.types,
            if nominal { self.owner.as_ref() } else { None }
        )
    }

    pub(crate) fn equal(&self, other: &Self) -> bool {
        if self.component.is_some()
            && other.component.is_some()
            && self.component != other.component
        {
            return false;
        }
        match (&self.data, &other.data) {
            (Data::Int(a), Data::Int(b)) => a == b,
            (Data::Real(a), Data::Real(b)) => a == b,
            (Data::String(a), Data::String(b)) => a == b,
            (Data::Bool(a), Data::Bool(b)) => a == b,
            (Data::None, Data::None) => true,
            (Data::Tag(a), Data::Tag(b)) | (Data::Ref(a), Data::Ref(b)) => a == b,
            (Data::TraitInstance(a, xs, _), Data::TraitInstance(b, ys, _)) => {
                a == b
                    && xs.len() == ys.len()
                    && xs
                        .iter()
                        .zip(ys)
                        .all(|(x, y)| x.identity_key() == y.identity_key())
            }
            (Data::Record(a), Data::Record(b)) => {
                self.component == other.component
                    && self.inline_traits.len() == other.inline_traits.len()
                    && self
                        .inline_traits
                        .iter()
                        .all(|x| other.inline_traits.iter().any(|y| x.equal(y)))
                    && a.len() == b.len()
                    && a.iter()
                        .all(|(key, value)| b.get(key).is_some_and(|other| value.equal(other)))
            }
            (Data::Set(a), Data::Set(b)) => {
                a.len() == b.len()
                    && a.iter()
                        .all(|value| b.iter().any(|other| value.equal(other)))
            }
            (Data::List(a), Data::List(b)) => {
                a.len() == b.len() && a.iter().zip(b).all(|(a, b)| a.equal(b))
            }
            _ => false,
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self.data {
            Data::Int(value) => write!(f, "{value}"),
            Data::Real(value) => write!(f, "{value:?}"),
            Data::String(value) => write!(f, "{value:?}"),
            Data::Bool(value) => write!(f, "{value}"),
            Data::None => write!(f, "none"),
            Data::Tag(name) | Data::Ref(name) => write!(f, "{name}"),
            Data::TraitInstance(name, args, _) => {
                write!(f, "{name}(")?;
                for (i, value) in args.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{value}")?;
                }
                write!(f, ")")
            }
            Data::Record(fields) => {
                write!(f, "{{")?;
                for (i, (name, value)) in fields.iter().enumerate() {
                    if i > 0 {
                        write!(f, ";")?;
                    }
                    write!(f, " {name} = {value}")?;
                }
                write!(f, " }}")
            }
            Data::Set(values) | Data::List(values) => {
                let (open, close) = if matches!(self.data, Data::Set(_)) {
                    ("{", "}")
                } else {
                    ("[", "]")
                };
                write!(f, "{open}")?;
                for (i, value) in values.iter().enumerate() {
                    if i > 0 {
                        write!(f, "; ")?;
                    }
                    write!(f, "{value}")?;
                }
                write!(f, "{close}")
            }
            Data::Function(name, _) => write!(f, "<function {name}>"),
        }
    }
}
