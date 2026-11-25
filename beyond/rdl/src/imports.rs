//! Resolves named source files before a world transaction is validated.
use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::io;
use std::path::{Component, Path, PathBuf};

use crate::ast::Import;
use crate::diagnostic::{Error, Result, Span};
use crate::{Value, World, parser};

#[derive(Clone, Debug)]
pub struct Source {
    pub name: String,
    pub text: String,
    /// None permits a virtual editor buffer whose imports use the search path.
    pub path: Option<PathBuf>,
}

#[derive(Clone, Debug)]
pub struct ResolvedImport {
    pub import: Import,
    pub target: String,
}

#[derive(Clone, Debug, Default)]
pub struct SourceGraph {
    sources: Vec<Source>,
    entries: BTreeSet<String>,
    links: Vec<ResolvedImport>,
    errors: Vec<Error>,
}

impl SourceGraph {
    pub fn sources(&self) -> &[Source] {
        &self.sources
    }
    pub fn links(&self) -> &[ResolvedImport] {
        &self.links
    }
    pub fn errors(&self) -> &[Error] {
        &self.errors
    }

    /// A partial graph is useful to editors. World mutation requires a completely
    /// resolved graph; any import or validation failure leaves the world intact.
    pub fn load_into(&self, world: &mut World) -> Result<Vec<Value>> {
        if let Some(error) = self.errors.first() {
            return Err(error.clone());
        }
        let sources: Vec<_> = self
            .sources
            .iter()
            .map(|source| (source.name.as_str(), source.text.as_str()))
            .collect();
        world.load_resolved_sources(&sources, &self.entries)
    }
}

#[derive(Clone, Debug, Default)]
pub struct SourceLoader {
    pub search_paths: Vec<PathBuf>,
}

impl SourceLoader {
    pub fn new(search_paths: Vec<PathBuf>) -> Self {
        Self { search_paths }
    }

    pub fn load_files(&self, files: &[PathBuf]) -> SourceGraph {
        let mut roots = Vec::new();
        let mut errors = Vec::new();
        for path in files {
            let name = path.display().to_string();
            let span = Span {
                source: name,
                line: 1,
                column: 1,
            };
            match absolute_path(path).and_then(|path| read_file(&path)) {
                Ok(Some(source)) => roots.push(source),
                Ok(None) => errors.push(Error::new(&span, "source file does not exist")),
                Err(error) => errors.push(Error::new(&span, error.to_string())),
            }
        }
        let mut graph = self.resolve(roots, read_file);
        graph.errors.splice(0..0, errors);
        graph
    }

    /// The reader can supply editor overlays and URI source names. None means
    /// that a candidate is absent; an I/O error stops that import's search.
    pub fn resolve(
        &self,
        roots: Vec<Source>,
        reader: impl FnMut(&Path) -> io::Result<Option<Source>>,
    ) -> SourceGraph {
        let mut resolver = Resolver {
            available: BTreeMap::new(),
            complete: BTreeSet::new(),
            active: Vec::new(),
            order: Vec::new(),
            search_paths: Vec::new(),
            reader,
            graph: SourceGraph::default(),
        };
        let mut entries = Vec::new();
        for mut source in roots {
            if let Some(path) = &source.path {
                match absolute_path(path) {
                    Ok(path) => {
                        if let Some(parent) = path.parent() {
                            push_unique(&mut resolver.search_paths, parent.to_path_buf());
                        }
                        source.path = Some(path);
                    }
                    Err(error) => {
                        resolver
                            .graph
                            .errors
                            .push(Error::new(&source_span(&source), error.to_string()));
                        continue;
                    }
                }
            }
            let key = identity(&source);
            let root = resolver.available.entry(key.clone()).or_insert(source);
            resolver.graph.entries.insert(root.name.clone());
            if !entries.contains(&key) {
                entries.push(key);
            }
        }
        for path in &self.search_paths {
            match absolute_path(path) {
                Ok(path) => push_unique(&mut resolver.search_paths, path),
                Err(error) => resolver.graph.errors.push(Error::new(
                    &Span {
                        source: "<search path>".into(),
                        line: 1,
                        column: 1,
                    },
                    error.to_string(),
                )),
            }
        }
        for key in entries {
            let span = source_span(&resolver.available[&key]);
            resolver.visit(key, &span);
        }
        resolver.graph.sources = resolver
            .order
            .iter()
            .filter_map(|key| resolver.available.get(key).cloned())
            .collect();
        resolver.graph
    }
}

/// Explicit -I paths precede this platform-separated environment search path.
pub fn environment_paths() -> Vec<PathBuf> {
    std::env::var_os("RDL_PATH")
        .map(|value| {
            std::env::split_paths(&value)
                .filter(|path| !path.as_os_str().is_empty())
                .collect()
        })
        .unwrap_or_default()
}

/// Canonicalize existing files to deduplicate aliases and symlinks. For an
/// unsaved editor file, normalize its absolute path without requiring existence.
pub fn absolute_path(path: &Path) -> io::Result<PathBuf> {
    let absolute = std::path::absolute(path)?;
    if let Ok(canonical) = absolute.canonicalize() {
        return Ok(canonical);
    }
    let mut normalized = PathBuf::new();
    for component in absolute.components() {
        match component {
            Component::CurDir => {}
            Component::ParentDir => {
                normalized.pop();
            }
            other => normalized.push(other.as_os_str()),
        }
    }
    let mut ancestor = normalized.clone();
    let mut suffix = Vec::new();
    loop {
        if let Ok(mut canonical) = ancestor.canonicalize() {
            for part in suffix.into_iter().rev() {
                canonical.push(part);
            }
            return Ok(canonical);
        }
        let Some(name) = ancestor.file_name() else {
            break;
        };
        suffix.push(name.to_os_string());
        if !ancestor.pop() {
            break;
        }
    }
    Ok(normalized)
}

pub fn read_file(path: &Path) -> io::Result<Option<Source>> {
    let name = path.to_str().ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "RDL source paths must be UTF-8",
        )
    })?;
    match fs::read_to_string(path) {
        Ok(text) => Ok(Some(Source {
            name: name.into(),
            text,
            path: Some(path.into()),
        })),
        Err(error)
            if matches!(
                error.kind(),
                io::ErrorKind::NotFound | io::ErrorKind::NotADirectory
            ) =>
        {
            Ok(None)
        }
        Err(error) => Err(error),
    }
}

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
enum Identity {
    File(PathBuf),
    Virtual(String),
}
fn identity(source: &Source) -> Identity {
    source
        .path
        .clone()
        .map(Identity::File)
        .unwrap_or_else(|| Identity::Virtual(source.name.clone()))
}
fn source_span(source: &Source) -> Span {
    Span {
        source: source.name.clone(),
        line: 1,
        column: 1,
    }
}
fn push_unique(paths: &mut Vec<PathBuf>, path: PathBuf) {
    if !paths.contains(&path) {
        paths.push(path);
    }
}

struct Resolver<F> {
    available: BTreeMap<Identity, Source>,
    complete: BTreeSet<Identity>,
    active: Vec<Identity>,
    order: Vec<Identity>,
    search_paths: Vec<PathBuf>,
    reader: F,
    graph: SourceGraph,
}

impl<F: FnMut(&Path) -> io::Result<Option<Source>>> Resolver<F> {
    fn visit(&mut self, key: Identity, origin: &Span) {
        if self.complete.contains(&key) {
            return;
        }
        if self.active.contains(&key) {
            let chain: Vec<_> = self
                .active
                .iter()
                .chain(std::iter::once(&key))
                .map(|key| self.available[key].name.as_str())
                .collect();
            self.graph.errors.push(Error::new(
                origin,
                format!("cyclic import: {}", chain.join(" -> ")),
            ));
            return;
        }
        if self.active.len() >= 64 || self.complete.len() >= 1000 {
            self.graph.errors.push(Error::new(
                origin,
                "import graph exceeds 64 levels or 1000 files",
            ));
            return;
        }
        self.active.push(key.clone());
        let source = self.available[&key].clone();
        match parser::imports(&source.name, &source.text) {
            Ok(imports) => {
                for import in imports {
                    match self.find(&source, &import) {
                        Ok(target) => {
                            self.graph.links.push(ResolvedImport {
                                import: import.clone(),
                                target: self.available[&target].name.clone(),
                            });
                            self.visit(target, &import.name_span);
                        }
                        Err(error) => self.graph.errors.push(error),
                    }
                }
            }
            Err(error) => self.graph.errors.push(error),
        }
        self.active.pop();
        self.complete.insert(key.clone());
        self.order.push(key);
    }

    fn find(&mut self, source: &Source, import: &Import) -> Result<Identity> {
        let mut relative = PathBuf::new();
        for part in import.name.split('.') {
            relative.push(part);
        }
        relative.set_extension("rdl");
        let mut paths = Vec::new();
        if let Some(parent) = source.path.as_ref().and_then(|path| path.parent()) {
            paths.push(parent.to_path_buf());
        }
        for path in &self.search_paths {
            push_unique(&mut paths, path.clone());
        }
        let mut searched = Vec::new();
        for directory in paths {
            let candidate = absolute_path(&directory.join(&relative))
                .map_err(|error| Error::new(&import.name_span, error.to_string()))?;
            let key = Identity::File(candidate.clone());
            if self.available.contains_key(&key) {
                return Ok(key);
            }
            searched.push(candidate.display().to_string());
            let loaded = (self.reader)(&candidate).map_err(|error| {
                Error::new(
                    &import.name_span,
                    format!(
                        "cannot import '{}': {}: {error}",
                        import.name,
                        candidate.display()
                    ),
                )
            })?;
            if let Some(mut source) = loaded {
                if self.available.len() >= 1000 {
                    return Err(Error::new(
                        &import.name_span,
                        "import graph exceeds 1000 files",
                    ));
                }
                source.path = Some(candidate);
                self.available.insert(key.clone(), source);
                return Ok(key);
            }
        }
        Err(Error::new(
            &import.name_span,
            format!(
                "cannot find import '{}'; searched: {}",
                import.name,
                if searched.is_empty() {
                    "no search directories configured".into()
                } else {
                    searched.join(", ")
                }
            ),
        ))
    }
}
