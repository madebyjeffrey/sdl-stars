use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Path, PathBuf};

use serde_json::{Value, json};
use url::Url;

use crate::diagnostic::Span;
use crate::imports::{Source, SourceGraph, SourceLoader, absolute_path};

#[derive(Clone, Debug)]
pub(super) struct Document {
    pub text: String,
    pub version: Option<i64>,
}

#[derive(Default)]
pub(super) struct Documents {
    pub sources: BTreeMap<String, Document>,
    pub open: BTreeMap<String, Document>,
    pub roots: BTreeSet<PathBuf>,
    pub files: Vec<PathBuf>,
    pub read_errors: BTreeMap<String, String>,
    pub import_paths: Vec<PathBuf>,
    pub graph: SourceGraph,
    pub disk_sources: BTreeMap<String, Document>,
}

impl Documents {
    pub fn import_completions(&self, uri: &str, byte: usize) -> Option<Value> {
        let document = self.sources.get(uri)?;
        let line = document.text[..byte].rsplit('\n').next()?.trim_start();
        let rest = line.strip_prefix("import")?;
        if !rest.starts_with(char::is_whitespace) {
            return None;
        }
        let name = rest.trim_start();
        if !name
            .chars()
            .all(|ch| ch.is_ascii_alphanumeric() || ch == '_' || ch == '.')
        {
            return None;
        }
        let (parent, fragment) = name
            .rsplit_once('.')
            .map(|(parent, fragment)| (format!("{parent}."), fragment))
            .unwrap_or((String::new(), name));
        let mut directories = Vec::new();
        for path in std::iter::once(file_path(uri).ok())
            .chain(
                self.open
                    .keys()
                    .chain(self.disk_sources.keys())
                    .map(|uri| file_path(uri).ok()),
            )
            .flatten()
        {
            if let Some(parent) = path.parent() {
                directories.push(parent.to_path_buf());
            }
        }
        directories.extend(self.roots.iter().cloned());
        directories.extend(self.import_paths.iter().cloned());
        let mut seen = BTreeSet::new();
        let mut items = BTreeMap::new();
        for directory in directories {
            let mut base = absolute_path(&directory).ok()?;
            for part in parent
                .trim_end_matches('.')
                .split('.')
                .filter(|part| !part.is_empty())
            {
                base.push(part);
            }
            if !seen.insert(base.clone()) {
                continue;
            }
            let mut candidates = Vec::new();
            if let Ok(entries) = fs::read_dir(&base) {
                for entry in entries.flatten() {
                    let path = entry.path();
                    if path.is_dir() {
                        candidates.push((entry.file_name().to_string_lossy().into_owned(), true));
                    } else if path.extension().is_some_and(|extension| extension == "rdl")
                        && let Some(stem) = path.file_stem().and_then(|name| name.to_str())
                    {
                        candidates.push((stem.into(), false));
                    }
                }
            }
            for uri in self.open.keys() {
                if let Ok(path) = file_path(uri)
                    .and_then(|path| absolute_path(&path).map_err(|error| error.to_string()))
                    && let Ok(relative) = path.strip_prefix(&base)
                {
                    let components: Vec<_> = relative.components().collect();
                    if let Some(first) = components
                        .first()
                        .and_then(|component| component.as_os_str().to_str())
                    {
                        if components.len() > 1 {
                            candidates.push((first.into(), true));
                        } else if path.extension().is_some_and(|extension| extension == "rdl")
                            && let Some(stem) = path.file_stem().and_then(|name| name.to_str())
                        {
                            candidates.push((stem.into(), false));
                        }
                    }
                }
            }
            for (candidate, directory) in candidates {
                if !candidate.starts_with(fragment) || !module_segment(&candidate) {
                    continue;
                }
                let label = format!("{parent}{candidate}{}", if directory { "." } else { "" });
                items.entry(label.clone()).or_insert_with(|| json!({ "label": label, "kind": 9, "detail": if directory { "RDL module directory" } else { "RDL module" },
                    "textEdit": { "range": range(&document.text, byte - name.len(), byte), "newText": label } }));
            }
        }
        Some(json!({ "isIncomplete": false, "items": items.into_values().collect::<Vec<_>>() }))
    }

    pub fn reload(&mut self) {
        let mut paths: BTreeSet<PathBuf> = self.files.iter().cloned().collect();
        self.read_errors.clear();
        if self.files.is_empty() {
            for root in &self.roots {
                if let Err(error) = discover(root, &mut paths, 0)
                    && let Ok(uri) = file_uri(root)
                {
                    self.read_errors.insert(uri, error);
                }
            }
        }
        self.disk_sources.clear();
        for path in paths {
            let Ok(uri) = file_uri(&path) else {
                continue;
            };
            match fs::read_to_string(&path) {
                Ok(text) => {
                    self.disk_sources.insert(
                        uri,
                        Document {
                            text,
                            version: None,
                        },
                    );
                }
                Err(error) => {
                    self.read_errors.insert(uri, error.to_string());
                }
            }
        }
        for uri in self.open.keys() {
            self.read_errors.remove(uri);
        }
        self.resolve();
    }

    pub fn resolve(&mut self) {
        let mut overlays = BTreeMap::new();
        let mut entries = Vec::new();
        let mut seen = BTreeSet::new();
        // Open buffers win even when an editor uses a symlink URI for a disk file.
        for (uri, document) in self.open.iter().chain(&self.disk_sources) {
            let path = file_path(uri)
                .ok()
                .and_then(|path| absolute_path(&path).ok());
            if let Some(path) = &path {
                if !seen.insert(path.clone()) {
                    continue;
                }
            } else if self.open.contains_key(uri)
                && entries.iter().any(|source: &Source| source.name == *uri)
            {
                continue;
            }
            let source = Source {
                name: uri.clone(),
                text: document.text.clone(),
                path: path.clone(),
            };
            if let Some(path) = path {
                overlays.insert(path, source.clone());
            }
            entries.push(source);
        }
        let paths = self
            .roots
            .iter()
            .cloned()
            .chain(self.import_paths.iter().cloned())
            .collect();
        // Use canonical identities for loading, but retain the URI spelling the
        // workspace/editor uses (for example macOS /var versus /private/var).
        let mut aliases: Vec<_> = self
            .open
            .keys()
            .chain(self.disk_sources.keys())
            .filter_map(|uri| file_path(uri).ok())
            .filter_map(|path| path.parent().map(Path::to_path_buf))
            .chain(self.roots.iter().cloned())
            .chain(self.import_paths.iter().cloned())
            .filter_map(|path| absolute_path(&path).ok().map(|canonical| (canonical, path)))
            .collect();
        aliases.sort_by_key(|(canonical, _)| std::cmp::Reverse(canonical.components().count()));
        self.graph = SourceLoader::new(paths).resolve(entries, |path| {
            if let Some(source) = overlays.get(path) {
                return Ok(Some(source.clone()));
            }
            crate::imports::read_file(path)?
                .map(|mut source| {
                    let preferred = aliases
                        .iter()
                        .find_map(|(canonical, original)| {
                            path.strip_prefix(canonical)
                                .ok()
                                .map(|relative| original.join(relative))
                        })
                        .unwrap_or_else(|| path.into());
                    source.name = file_uri(&preferred).map_err(std::io::Error::other)?;
                    Ok(source)
                })
                .transpose()
        });
        self.sources = self
            .graph
            .sources()
            .iter()
            .map(|source| {
                let version = self
                    .open
                    .get(&source.name)
                    .and_then(|document| document.version);
                (
                    source.name.clone(),
                    Document {
                        text: source.text.clone(),
                        version,
                    },
                )
            })
            .collect();
    }

    pub fn change(&mut self, uri: &str, version: i64, changes: &[Value]) -> Result<bool, String> {
        let document = self
            .open
            .get(uri)
            .ok_or("didChange requires an open document")?;
        if document.version.is_some_and(|current| version <= current) {
            return Ok(false);
        }
        let mut text = document.text.clone();
        for change in changes {
            let replacement = change["text"].as_str().ok_or("content change needs text")?;
            if let Some(range) = change.get("range") {
                let start = offset(&text, &range["start"])?;
                let end = offset(&text, &range["end"])?;
                if start > end {
                    return Err("change range is reversed".into());
                }
                text.replace_range(start..end, replacement);
            } else {
                text = replacement.into();
            }
        }
        let document = Document {
            text,
            version: Some(version),
        };
        self.open.insert(uri.into(), document.clone());
        self.sources.insert(uri.into(), document);
        Ok(true)
    }
}

fn module_segment(name: &str) -> bool {
    let mut chars = name.chars();
    chars
        .next()
        .is_some_and(|ch| ch.is_ascii_alphabetic() || ch == '_')
        && chars.all(|ch| ch.is_ascii_alphanumeric() || ch == '_')
}

fn discover(root: &Path, paths: &mut BTreeSet<PathBuf>, depth: usize) -> Result<(), String> {
    if depth > 32 {
        return Err("RDL discovery exceeds 32 directories deep; pass explicit source files".into());
    }
    for entry in fs::read_dir(root).map_err(|error| format!("{}: {error}", root.display()))? {
        let entry = entry.map_err(|error| error.to_string())?;
        let path = entry.path();
        let kind = entry.file_type().map_err(|error| error.to_string())?;
        // Do not follow links: an editor workspace can contain directory cycles.
        if kind.is_symlink() {
            continue;
        }
        if kind.is_dir() {
            let name = entry.file_name();
            let name = name.to_string_lossy();
            if !name.starts_with('.')
                && !matches!(name.as_ref(), "target" | "node_modules" | "dist" | "build")
            {
                discover(&path, paths, depth + 1)?;
            }
        } else if kind.is_file() && path.extension().is_some_and(|extension| extension == "rdl") {
            paths.insert(path);
            if paths.len() > 1000 {
                return Err("RDL discovery exceeds 1000 files; pass explicit source files".into());
            }
        }
    }
    Ok(())
}

pub(super) fn file_uri(path: &Path) -> Result<String, String> {
    let path = std::path::absolute(path).map_err(|error| error.to_string())?;
    Url::from_file_path(path)
        .map(String::from)
        .map_err(|_| "cannot represent path as a file URI".into())
}

pub(super) fn file_path(uri: &str) -> Result<PathBuf, String> {
    Url::parse(uri)
        .map_err(|error| error.to_string())?
        .to_file_path()
        .map_err(|_| "expected a local file URI".into())
}

pub(super) fn document_uri(uri: &str) -> Result<String, String> {
    let parsed = Url::parse(uri).map_err(|error| error.to_string())?;
    if parsed.scheme() == "file" {
        file_uri(&file_path(uri)?)
    } else {
        Ok(parsed.into())
    }
}

/// LSP positions count UTF-16 code units, while the lexer counts Unicode scalars.
pub(super) fn offset(text: &str, position: &Value) -> Result<usize, String> {
    let line = position["line"]
        .as_u64()
        .ok_or("position needs a nonnegative line")?;
    let character = position["character"]
        .as_u64()
        .ok_or("position needs a nonnegative character")?;
    let mut start = 0;
    for _ in 0..line {
        start += text[start..]
            .find('\n')
            .ok_or("position line is outside the document")?
            + 1;
    }
    let line = text[start..]
        .split('\n')
        .next()
        .unwrap_or("")
        .trim_end_matches('\r');
    let mut units = 0;
    for (i, ch) in line.char_indices() {
        if units == character {
            return Ok(start + i);
        }
        units += ch.len_utf16() as u64;
        if units > character {
            return Err("position splits a UTF-16 surrogate pair".into());
        }
    }
    if units == character {
        Ok(start + line.len())
    } else {
        Err("position character is outside the line".into())
    }
}

pub(super) fn position(text: &str, byte: usize) -> Value {
    let mut byte = byte.min(text.len());
    while !text.is_char_boundary(byte) {
        byte -= 1;
    }
    let prefix = &text[..byte];
    let line = prefix.bytes().filter(|ch| *ch == b'\n').count();
    let column = prefix
        .rsplit('\n')
        .next()
        .unwrap_or("")
        .encode_utf16()
        .count();
    json!({ "line": line, "character": column })
}

pub(super) fn span_offset(text: &str, span: &Span) -> usize {
    let mut start = 0;
    for _ in 1..span.line {
        let Some(next) = text[start..].find('\n') else {
            return text.len();
        };
        start += next + 1;
    }
    let line = text[start..].split('\n').next().unwrap_or("");
    start
        + line
            .char_indices()
            .nth(span.column.saturating_sub(1))
            .map(|(i, _)| i)
            .unwrap_or(line.len())
}

pub(super) fn range(text: &str, start: usize, end: usize) -> Value {
    json!({ "start": position(text, start), "end": position(text, end.max(start)) })
}

pub(super) fn diagnostic_range(text: &str, span: &Span) -> Value {
    let start = span_offset(text, span);
    let mut length = text[start..]
        .chars()
        .take_while(|ch| ch.is_ascii_alphanumeric() || *ch == '_')
        .count();
    if length == 0 {
        length = text[start..]
            .chars()
            .next()
            .filter(|ch| *ch != '\n' && *ch != '\r')
            .map(char::len_utf8)
            .unwrap_or(0);
    }
    range(text, start, start + length)
}
