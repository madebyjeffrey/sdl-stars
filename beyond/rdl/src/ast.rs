use crate::diagnostic::Span;

#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub enum Type {
    Int,
    Real,
    String,
    Bool,
    Var(String),
    Named(String, Vec<Type>),
    Option(Box<Type>),
    Set(Box<Type>, usize, Option<usize>),
}

impl std::fmt::Display for Type {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Int => write!(f, "int"),
            Self::Real => write!(f, "real"),
            Self::String => write!(f, "string"),
            Self::Bool => write!(f, "bool"),
            Self::Var(name) => write!(f, "'{name}"),
            Self::Named(name, args) => {
                for arg in args {
                    write!(f, "{arg} ")?;
                }
                write!(f, "{name}")
            }
            Self::Option(ty) => write!(f, "{ty} option"),
            Self::Set(ty, 0, None) => write!(f, "{ty} set"),
            Self::Set(ty, min, max) => {
                write!(f, "{ty} set[{min}..")?;
                match max {
                    Some(max) => write!(f, "{max}]"),
                    None => write!(f, "*]"),
                }
            }
        }
    }
}

#[derive(Clone, Debug)]
pub struct Expr {
    pub kind: ExprKind,
    pub span: Span,
}

#[derive(Clone, Debug)]
pub enum ExprKind {
    Int(i64),
    Real(f64),
    String(String),
    Bool(bool),
    None,
    Name(String),
    Record(Vec<(String, Expr)>),
    ObjectRecord(Vec<(String, Expr)>, Vec<Expr>),
    Set(Vec<Expr>),
    Field(Box<Expr>, String),
    Call(Box<Expr>, Box<Expr>),
    CallArgs(Box<Expr>, Vec<Expr>),
    Unary(String, Box<Expr>),
    Binary(String, Box<Expr>, Box<Expr>),
    If(Box<Expr>, Box<Expr>, Box<Expr>),
    Let(String, Box<Expr>, Box<Expr>),
    Match(Box<Expr>, Vec<(Pattern, Expr)>),
    Query(Query),
    Compose(Type),
}

#[derive(Clone, Debug)]
pub enum Pattern {
    Wildcard,
    Case(String, Option<Vec<(String, String)>>),
}

#[derive(Clone, Debug)]
pub struct Query {
    pub bindings: Vec<(String, Type)>,
    pub filters: Vec<Expr>,
    pub order: Option<(Box<Expr>, bool)>,
    pub select: Box<Expr>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TypeKind {
    Value,
    Trait,
    Object,
}

#[derive(Clone, Debug)]
pub struct Field {
    pub name: String,
    pub ty: Type,
    pub expr: Option<Expr>,
    pub span: Span,
}

#[derive(Clone, Debug)]
pub enum Alternative {
    Type(Type),
    Constructor(String, Vec<Field>),
}

#[derive(Clone, Debug)]
pub enum Body {
    Record(Vec<Field>),
    Union(Vec<Alternative>),
    Alias(Type),
}

#[derive(Clone, Debug)]
pub struct Definition {
    pub name: String,
    pub params: Vec<String>,
    pub kind: TypeKind,
    pub body: Body,
    pub context: Option<(String, Type)>,
    pub derives: Vec<Field>,
    pub requires: Vec<Expr>,
    pub span: Span,
    pub component: Option<Box<ComponentDefinition>>,
}

#[derive(Clone, Debug, Default)]
pub struct ComponentDefinition {
    pub abstract_: bool,
    pub scalar: Option<Type>,
    pub parents: Vec<String>,
    pub values: Vec<(String, Expr)>,
    pub compose: Option<Expr>,
    pub expanded: bool,
}

#[derive(Clone, Debug)]
pub enum Effect {
    Provides {
        component: String,
        value: Expr,
    },
    Modifies {
        component: String,
        field: Option<String>,
        operation: String,
        value: Expr,
    },
}

#[derive(Clone, Debug)]
pub struct Binding {
    pub name: String,
    pub ty: Option<Type>,
    pub kind: TypeKind,
    pub expr: Expr,
    pub span: Span,
    pub effects: Vec<Effect>,
    pub parameters: Option<Vec<Field>>,
    pub requires: Vec<Expr>,
}

#[derive(Clone, Debug)]
pub struct Import {
    pub name: String,
    pub span: Span,
    pub name_span: Span,
}

#[derive(Clone, Debug)]
pub enum Statement {
    Import(Import),
    Definition(Definition),
    Binding(Binding),
    Expression(Expr),
}
