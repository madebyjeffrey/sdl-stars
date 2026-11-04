use std::collections::BTreeMap;
use std::fmt;

use crate::ast::Type;

#[derive(Clone, Debug)]
pub struct Value {
    pub(crate) data: Data,
    pub(crate) types: Vec<Type>,
    pub(crate) owner: Option<String>,
    pub(crate) path: Option<String>,
    pub(crate) item_type: Option<Type>,
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
    Record(BTreeMap<String, Value>),
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
        }
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
        if let Data::Record(fields) = &self.data {
            Some(fields)
        } else {
            None
        }
    }
    pub fn reference(&self) -> Option<&str> {
        if let Data::Ref(name) = &self.data {
            Some(name)
        } else {
            None
        }
    }

    pub(crate) fn equal(&self, other: &Self) -> bool {
        match (&self.data, &other.data) {
            (Data::Int(a), Data::Int(b)) => a == b,
            (Data::Real(a), Data::Real(b)) => a == b,
            (Data::String(a), Data::String(b)) => a == b,
            (Data::Bool(a), Data::Bool(b)) => a == b,
            (Data::None, Data::None) => true,
            (Data::Tag(a), Data::Tag(b)) | (Data::Ref(a), Data::Ref(b)) => a == b,
            (Data::Record(a), Data::Record(b)) => {
                a.len() == b.len()
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
