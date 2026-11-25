//! RDL is a standalone experiment; it does not read or write Stars! game files.
pub mod ast;
mod check;
pub mod diagnostic;
mod engine;
pub mod imports;
mod lexer;
#[cfg(feature = "lsp")]
pub mod lsp;
pub mod parser;
mod value;

pub use diagnostic::{Error, Result};
pub use engine::World;
pub use value::Value;
