//! Tolerant lexical coloring plus symbol roles from the editor index. Unlike
//! validation, coloring continues through unfinished strings and comments.
use serde_json::{Value, json};

use super::index::Index;

const TYPES: &[&str] = &[
    "keyword",
    "type",
    "class",
    "interface",
    "struct",
    "typeParameter",
    "variable",
    "property",
    "enumMember",
    "function",
    "namespace",
    "string",
    "number",
    "comment",
    "operator",
];
const MODIFIERS: &[&str] = &["declaration", "readonly", "defaultLibrary"];

pub(super) struct Legend {
    types: Vec<&'static str>,
    modifiers: Vec<&'static str>,
}

impl Default for Legend {
    fn default() -> Self {
        Self {
            types: TYPES.to_vec(),
            modifiers: MODIFIERS.to_vec(),
        }
    }
}

impl Legend {
    pub fn new(capabilities: &Value) -> Self {
        let choose = |all: &[&'static str], supported: &Value| {
            all.iter()
                .copied()
                .filter(|name| {
                    supported.as_array().is_none_or(|values| {
                        values.iter().any(|value| value.as_str() == Some(name))
                    })
                })
                .collect()
        };
        Self {
            types: choose(TYPES, &capabilities["tokenTypes"]),
            modifiers: choose(MODIFIERS, &capabilities["tokenModifiers"]),
        }
    }

    pub fn json(&self) -> Value {
        json!({ "tokenTypes": self.types, "tokenModifiers": self.modifiers })
    }

    fn kind(&self, name: &str) -> Option<u32> {
        self.types
            .iter()
            .position(|ty| *ty == name)
            .or_else(|| {
                if matches!(name, "class" | "interface" | "struct" | "typeParameter") {
                    self.types.iter().position(|ty| *ty == "type")
                } else {
                    None
                }
            })
            .map(|i| i as u32)
    }

    fn flags(&self, flags: u32) -> u32 {
        self.modifiers
            .iter()
            .enumerate()
            .fold(0, |result, (i, name)| {
                let bit = match *name {
                    "declaration" => 1,
                    "readonly" => 2,
                    "defaultLibrary" => 4,
                    _ => 0,
                };
                if flags & bit != 0 {
                    result | (1 << i)
                } else {
                    result
                }
            })
    }
}

pub(super) fn full(uri: &str, text: &str, index: &Index, legend: &Legend) -> Value {
    let bytes = text.as_bytes();
    let mut cursor = 0;
    let mut encoder = Encoder {
        legend,
        data: Vec::new(),
        line: 0,
        column: 0,
        last_line: 0,
        last_column: 0,
    };
    while cursor < bytes.len() {
        let start = cursor;
        let ch = text[cursor..].chars().next().unwrap();
        let (kind, modifiers) = if ch.is_whitespace() {
            cursor += ch.len_utf8();
            ("", 0)
        } else if text[cursor..].starts_with("//") {
            cursor += 2;
            while cursor < bytes.len() && bytes[cursor] != b'\n' {
                cursor += 1;
            }
            ("comment", 0)
        } else if text[cursor..].starts_with("(*") {
            cursor += 2;
            let mut depth = 1;
            while cursor < bytes.len() && depth != 0 {
                if bytes[cursor..].starts_with(b"(*") {
                    depth += 1;
                    cursor += 2;
                } else if bytes[cursor..].starts_with(b"*)") {
                    depth -= 1;
                    cursor += 2;
                } else {
                    cursor += 1;
                }
            }
            ("comment", 0)
        } else if ch == '"' {
            let triple = text[cursor..].starts_with("\"\"\"");
            cursor += if triple { 3 } else { 1 };
            while cursor < bytes.len() {
                if triple && bytes[cursor..].starts_with(b"\"\"\"") {
                    cursor += 3;
                    break;
                }
                if !triple {
                    if matches!(bytes[cursor], b'\n' | b'\r') {
                        break;
                    }
                    if bytes[cursor] == b'"' {
                        cursor += 1;
                        break;
                    }
                    if bytes[cursor] == b'\\' {
                        cursor += 1;
                        if cursor == bytes.len() || matches!(bytes[cursor], b'\n' | b'\r') {
                            break;
                        }
                    }
                }
                // Byte iteration is safe here: ASCII delimiters cannot occur inside
                // a UTF-8 continuation sequence, and the final boundary is complete.
                cursor += 1;
            }
            ("string", 0)
        } else if ch == '\''
            && bytes
                .get(cursor + 1)
                .is_some_and(|ch| ch.is_ascii_alphabetic() || *ch == b'_')
        {
            cursor += 2;
            while bytes
                .get(cursor)
                .is_some_and(|ch| ch.is_ascii_alphanumeric() || *ch == b'_')
            {
                cursor += 1;
            }
            ("typeParameter", 0)
        } else if ch.is_ascii_alphabetic() || ch == '_' {
            cursor += 1;
            while bytes
                .get(cursor)
                .is_some_and(|ch| ch.is_ascii_alphanumeric() || *ch == b'_')
            {
                cursor += 1;
            }
            let word = &text[start..cursor];
            if matches!(word, "true" | "false" | "none") {
                ("enumMember", 4)
            } else if crate::parser::reserved(word) {
                ("keyword", 0)
            } else if let Some(role) = index.semantic_role(uri, start) {
                role
            } else if matches!(
                word,
                "int" | "integer" | "real" | "string" | "bool" | "option" | "set" | "range"
            ) {
                ("type", 4)
            } else if word == "self" {
                ("variable", 2)
            } else if matches!(word, "Set" | "List") {
                ("namespace", 4)
            } else if matches!(word, "round" | "floor" | "ceil" | "abs" | "min" | "max") {
                ("function", 4)
            } else if word == "immune" {
                ("enumMember", 0)
            } else {
                ("variable", 0)
            }
        } else if ch.is_ascii_digit() {
            cursor += 1;
            while bytes.get(cursor).is_some_and(u8::is_ascii_digit) {
                cursor += 1;
            }
            if bytes.get(cursor) == Some(&b'.')
                && bytes.get(cursor + 1).is_some_and(u8::is_ascii_digit)
            {
                cursor += 2;
                while bytes.get(cursor).is_some_and(u8::is_ascii_digit) {
                    cursor += 1;
                }
            }
            if matches!(bytes.get(cursor), Some(b'e' | b'E')) {
                cursor += 1;
                if matches!(bytes.get(cursor), Some(b'+' | b'-')) {
                    cursor += 1;
                }
                while bytes.get(cursor).is_some_and(u8::is_ascii_digit) {
                    cursor += 1;
                }
            }
            ("number", 0)
        } else if "{}()[];:,.=|+-*/%<>".contains(ch) || matches!(ch, '&' | '!') {
            cursor += 1;
            if [
                b"..", b"->", b"<=", b">=", b"!=", b"<>", b"==", b"&&", b"||",
            ]
            .iter()
            .any(|pair| bytes[start..].starts_with(*pair))
            {
                cursor += 1;
            }
            ("operator", 0)
        } else {
            cursor += ch.len_utf8();
            ("", 0)
        };
        encoder.push(&text[start..cursor], kind, modifiers);
    }
    json!({ "data": encoder.data })
}

struct Encoder<'a> {
    legend: &'a Legend,
    data: Vec<u32>,
    line: u32,
    column: u32,
    last_line: u32,
    last_column: u32,
}

impl Encoder<'_> {
    fn push(&mut self, text: &str, kind: &str, modifiers: u32) {
        for segment in text.split_inclusive('\n') {
            let content = segment
                .strip_suffix('\n')
                .unwrap_or(segment)
                .trim_end_matches('\r');
            let length = content.encode_utf16().count() as u32;
            if length != 0
                && let Some(kind) = self.legend.kind(kind)
            {
                self.data.extend([
                    self.line - self.last_line,
                    if self.line == self.last_line {
                        self.column - self.last_column
                    } else {
                        self.column
                    },
                    length,
                    kind,
                    self.legend.flags(modifiers),
                ]);
                self.last_line = self.line;
                self.last_column = self.column;
            }
            if segment.ends_with('\n') {
                self.line += 1;
                self.column = 0;
            } else {
                self.column += segment.encode_utf16().count() as u32;
            }
        }
    }
}
