use crate::diagnostic::{Error, Result, Span};

#[derive(Clone, Debug, PartialEq)]
pub enum TokenKind {
    Word(String),
    Variable(String),
    Number(String),
    String(String),
    Symbol(String),
    End,
}

#[derive(Clone, Debug)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
}

pub fn lex(source: &str, text: &str) -> Result<Vec<Token>> {
    Lexer {
        source,
        chars: text.chars().collect(),
        pos: 0,
        line: 1,
        column: 1,
    }
    .run()
}

struct Lexer<'a> {
    source: &'a str,
    chars: Vec<char>,
    pos: usize,
    line: usize,
    column: usize,
}

impl Lexer<'_> {
    fn span(&self) -> Span {
        Span {
            source: self.source.into(),
            line: self.line,
            column: self.column,
        }
    }

    fn peek(&self, offset: usize) -> Option<char> {
        self.chars.get(self.pos + offset).copied()
    }

    fn advance(&mut self) -> Option<char> {
        let ch = self.peek(0)?;
        self.pos += 1;
        if ch == '\n' {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
        Some(ch)
    }

    fn starts(&self, text: &str) -> bool {
        text.chars()
            .enumerate()
            .all(|(i, ch)| self.peek(i) == Some(ch))
    }

    fn run(mut self) -> Result<Vec<Token>> {
        let mut tokens = Vec::new();
        while let Some(ch) = self.peek(0) {
            if ch.is_whitespace() {
                self.advance();
                continue;
            }
            if self.starts("//") {
                while self.peek(0).is_some_and(|ch| ch != '\n') {
                    self.advance();
                }
                continue;
            }
            if self.starts("(*") {
                let span = self.span();
                self.advance();
                self.advance();
                let mut depth = 1;
                while depth > 0 {
                    if self.starts("(*") {
                        self.advance();
                        self.advance();
                        depth += 1;
                    } else if self.starts("*)") {
                        self.advance();
                        self.advance();
                        depth -= 1;
                    } else if self.advance().is_none() {
                        return Err(Error::new(&span, "unterminated comment"));
                    }
                }
                continue;
            }
            let span = self.span();
            let kind = if ch == '"' {
                TokenKind::String(self.string(&span)?)
            } else if ch == '\'' {
                self.advance();
                let name = self.word();
                if name.is_empty() {
                    return Err(Error::new(
                        &span,
                        "expected a type variable after apostrophe",
                    ));
                }
                TokenKind::Variable(name)
            } else if ch.is_ascii_alphabetic() || ch == '_' {
                TokenKind::Word(self.word())
            } else if ch.is_ascii_digit() {
                let mut number = String::new();
                while self.peek(0).is_some_and(|ch| ch.is_ascii_digit()) {
                    number.push(self.advance().unwrap());
                }
                if self.peek(0) == Some('.') && self.peek(1).is_some_and(|ch| ch.is_ascii_digit()) {
                    number.push(self.advance().unwrap());
                    while self.peek(0).is_some_and(|ch| ch.is_ascii_digit()) {
                        number.push(self.advance().unwrap());
                    }
                }
                if matches!(self.peek(0), Some('e' | 'E')) {
                    number.push(self.advance().unwrap());
                    if matches!(self.peek(0), Some('+' | '-')) {
                        number.push(self.advance().unwrap());
                    }
                    if !self.peek(0).is_some_and(|ch| ch.is_ascii_digit()) {
                        return Err(Error::new(&span, "expected exponent digits"));
                    }
                    while self.peek(0).is_some_and(|ch| ch.is_ascii_digit()) {
                        number.push(self.advance().unwrap());
                    }
                }
                TokenKind::Number(number)
            } else {
                let pair = ["..", "->", "<=", ">=", "!=", "<>", "==", "&&", "||"]
                    .into_iter()
                    .find(|symbol| self.starts(symbol));
                if let Some(pair) = pair {
                    self.advance();
                    self.advance();
                    TokenKind::Symbol(pair.into())
                } else if "{}()[];:,.=|+-*/%<>".contains(ch) {
                    self.advance();
                    TokenKind::Symbol(ch.to_string())
                } else {
                    return Err(Error::new(&span, format!("unexpected character {ch:?}")));
                }
            };
            tokens.push(Token { kind, span });
        }
        tokens.push(Token {
            kind: TokenKind::End,
            span: self.span(),
        });
        Ok(tokens)
    }

    fn word(&mut self) -> String {
        let mut text = String::new();
        while self
            .peek(0)
            .is_some_and(|ch| ch.is_ascii_alphanumeric() || ch == '_')
        {
            text.push(self.advance().unwrap());
        }
        text
    }

    fn string(&mut self, span: &Span) -> Result<String> {
        let multiline = self.starts("\"\"\"");
        self.advance();
        if multiline {
            self.advance();
            self.advance();
        }
        let mut text = String::new();
        loop {
            if multiline && self.starts("\"\"\"") {
                self.advance();
                self.advance();
                self.advance();
                return Ok(dedent(&text));
            }
            let ch = self
                .advance()
                .ok_or_else(|| Error::new(span, "unterminated string"))?;
            if !multiline && ch == '"' {
                return Ok(text);
            }
            if !multiline && ch == '\n' {
                return Err(Error::new(span, "use triple quotes for multiline strings"));
            }
            if ch == '\\' && !multiline {
                text.push(match self.advance() {
                    Some('n') => '\n',
                    Some('r') => '\r',
                    Some('t') => '\t',
                    Some('"') => '"',
                    Some('\\') => '\\',
                    _ => return Err(Error::new(span, "unsupported string escape")),
                });
            } else {
                text.push(ch);
            }
        }
    }
}

fn dedent(text: &str) -> String {
    let text = text
        .strip_prefix("\r\n")
        .or_else(|| text.strip_prefix('\n'))
        .unwrap_or(text);
    let mut lines: Vec<&str> = text.split('\n').collect();
    if lines.last().is_some_and(|line| line.trim().is_empty()) {
        lines.pop();
    }
    let indent = lines
        .iter()
        .filter(|line| !line.trim().is_empty())
        .map(|line| {
            line.chars()
                .take_while(|ch| *ch == ' ' || *ch == '\t')
                .count()
        })
        .min()
        .unwrap_or(0);
    lines
        .iter()
        .map(|line| {
            if line.trim().is_empty() {
                String::new()
            } else {
                line.chars().skip(indent).collect()
            }
        })
        .collect::<Vec<_>>()
        .join("\n")
}
