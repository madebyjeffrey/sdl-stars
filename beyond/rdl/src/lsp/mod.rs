//! A synchronous stdio LSP server using the same validation pipeline as `rdl check`.
mod documents;
mod highlight;
mod index;
mod transport;

use std::collections::{BTreeMap, BTreeSet};
use std::io::{self, BufRead, Write};
use std::path::PathBuf;

use serde_json::{Value, json};

use crate::{World, parser};
use documents::{Document, Documents, diagnostic_range, document_uri, file_path, offset};
use index::Index;

/// Runs LSP over framed JSON-RPC messages. This function never writes ordinary
/// CLI output to the protocol stream and never changes source files.
pub fn serve(input: impl BufRead, output: impl Write, files: Vec<String>) -> io::Result<()> {
    serve_with_import_paths(input, output, files, Vec::new())
}

pub fn serve_with_import_paths(
    mut input: impl BufRead,
    mut output: impl Write,
    files: Vec<String>,
    mut import_paths: Vec<PathBuf>,
) -> io::Result<()> {
    import_paths.extend(crate::imports::environment_paths());
    let mut server = Server {
        documents: Documents {
            files: files.into_iter().map(PathBuf::from).collect(),
            import_paths,
            ..Documents::default()
        },
        ..Server::default()
    };
    while let Some(bytes) = transport::read(&mut input)? {
        let message: Value = match serde_json::from_slice(&bytes) {
            Ok(message) => message,
            Err(error) => {
                transport::write(
                    &mut output,
                    &failure(Value::Null, -32700, &error.to_string()),
                )?;
                continue;
            }
        };
        let Some(method) = message["method"].as_str() else {
            if message.get("result").is_some() || message.get("error").is_some() {
                continue;
            }
            transport::write(
                &mut output,
                &failure(
                    message.get("id").cloned().unwrap_or(Value::Null),
                    -32600,
                    "invalid JSON-RPC request",
                ),
            )?;
            continue;
        };
        if message["jsonrpc"] != "2.0" {
            transport::write(
                &mut output,
                &failure(
                    message.get("id").cloned().unwrap_or(Value::Null),
                    -32600,
                    "expected JSON-RPC 2.0",
                ),
            )?;
            continue;
        }
        let id = message.get("id").cloned();
        let params = &message["params"];
        if method == "exit" && id.is_none() {
            return if server.shutdown {
                Ok(())
            } else {
                Err(io::Error::other("language server exited without shutdown"))
            };
        }
        if !server.initialized && method != "initialize" {
            if let Some(id) = id {
                transport::write(
                    &mut output,
                    &failure(id, -32002, "server is not initialized"),
                )?;
            }
            continue;
        }
        if server.shutdown {
            if let Some(id) = id {
                transport::write(&mut output, &failure(id, -32600, "server has shut down"))?;
            }
            continue;
        }
        if let Some(id) = id {
            let response = if !matches!(
                method,
                "initialize"
                    | "shutdown"
                    | "textDocument/hover"
                    | "textDocument/definition"
                    | "textDocument/completion"
                    | "textDocument/documentSymbol"
                    | "textDocument/semanticTokens/full"
            ) {
                failure(id, -32601, &format!("method not supported: {method}"))
            } else {
                match server.request(method, params) {
                    Ok(result) => json!({ "jsonrpc": "2.0", "id": id, "result": result }),
                    Err(error) => failure(id, -32602, &error),
                }
            };
            transport::write(&mut output, &response)?;
        } else {
            match server.notification(method, params) {
                Ok(messages) => {
                    for message in messages {
                        transport::write(&mut output, &message)?;
                    }
                }
                Err(message) => transport::write(
                    &mut output,
                    &notification(
                        "window/logMessage",
                        json!({ "type": 1, "message": message }),
                    ),
                )?,
            }
        }
    }
    if server.shutdown {
        Ok(())
    } else {
        Err(io::Error::other(
            "language server input closed without shutdown",
        ))
    }
}

#[derive(Default)]
struct Server {
    documents: Documents,
    index: Index,
    published: BTreeSet<String>,
    initialized: bool,
    ready: bool,
    shutdown: bool,
    hierarchical_symbols: bool,
    dynamic_watches: bool,
    semantic_legend: highlight::Legend,
    semantic_refresh: bool,
    refresh_counter: u64,
}

impl Server {
    fn request(&mut self, method: &str, params: &Value) -> Result<Value, String> {
        match method {
            "initialize" => {
                if self.initialized {
                    return Err("initialize may only be sent once".into());
                }
                if !params.is_object() {
                    return Err("initialize needs an object of parameters".into());
                }
                let mut documents = Documents {
                    files: self.documents.files.clone(),
                    import_paths: self.documents.import_paths.clone(),
                    ..Documents::default()
                };
                let mut first_root = None;
                if let Some(folders) = params["workspaceFolders"].as_array() {
                    for folder in folders {
                        let root = file_path(required_str(folder, "uri")?)?;
                        first_root.get_or_insert_with(|| root.clone());
                        documents.roots.insert(root);
                    }
                } else if let Some(uri) = params["rootUri"].as_str() {
                    let root = file_path(uri)?;
                    first_root = Some(root.clone());
                    documents.roots.insert(root);
                }
                if let Some(paths) = params["initializationOptions"].get("importPaths") {
                    let paths = paths.as_array().ok_or(
                        "initializationOptions.importPaths must be an array of directories",
                    )?;
                    let root = first_root
                        .clone()
                        .unwrap_or(std::env::current_dir().map_err(|error| error.to_string())?);
                    let mut configured = paths
                        .iter()
                        .map(|path| {
                            path.as_str().map(|path| root.join(path)).ok_or_else(|| {
                                "importPaths must contain directory paths".to_string()
                            })
                        })
                        .collect::<Result<Vec<_>, _>>()?;
                    configured.extend(documents.import_paths);
                    documents.import_paths = configured;
                }
                if documents.files.is_empty()
                    && let Some(files) = params["initializationOptions"].get("files")
                {
                    let files = files
                        .as_array()
                        .ok_or("initializationOptions.files must be an array of paths")?;
                    let root = match first_root {
                        Some(root) => root,
                        None => std::env::current_dir().map_err(|error| error.to_string())?,
                    };
                    documents.files = files
                        .iter()
                        .map(|file| {
                            file.as_str().map(|file| root.join(file)).ok_or_else(|| {
                                "initializationOptions.files must contain paths".into()
                            })
                        })
                        .collect::<Result<_, String>>()?;
                }
                self.hierarchical_symbols = params["capabilities"]["textDocument"]["documentSymbol"]["hierarchicalDocumentSymbolSupport"].as_bool().unwrap_or(false);
                self.dynamic_watches = params["capabilities"]["workspace"]["didChangeWatchedFiles"]
                    ["dynamicRegistration"]
                    .as_bool()
                    .unwrap_or(false);
                self.semantic_legend = highlight::Legend::new(
                    &params["capabilities"]["textDocument"]["semanticTokens"],
                );
                self.semantic_refresh =
                    params["capabilities"]["workspace"]["semanticTokens"]["refreshSupport"]
                        .as_bool()
                        .unwrap_or(false);
                documents.reload();
                self.documents = documents;
                self.index = Index::build(&self.documents.sources);
                self.index.import_links(self.documents.graph.links());
                self.initialized = true;
                Ok(
                    json!({ "serverInfo": { "name": "rdl", "version": env!("CARGO_PKG_VERSION") }, "capabilities": {
                    "positionEncoding": "utf-16",
                    "textDocumentSync": { "openClose": true, "change": 2, "save": { "includeText": false } },
                    "hoverProvider": true, "definitionProvider": true, "documentSymbolProvider": true,
                    "completionProvider": { "triggerCharacters": ["."], "resolveProvider": false },
                    "semanticTokensProvider": { "legend": self.semantic_legend.json(), "full": true, "range": false },
                    "workspace": { "workspaceFolders": { "supported": true, "changeNotifications": true } }
                } }),
                )
            }
            "shutdown" => {
                self.shutdown = true;
                Ok(Value::Null)
            }
            "textDocument/documentSymbol" => {
                let uri = request_uri(params)?;
                Ok(self.index.symbols(&uri, self.hierarchical_symbols))
            }
            "textDocument/semanticTokens/full" => {
                let uri = request_uri(params)?;
                let document = self
                    .documents
                    .sources
                    .get(&uri)
                    .ok_or("document is not loaded")?;
                Ok(highlight::full(
                    &uri,
                    &document.text,
                    &self.index,
                    &self.semantic_legend,
                ))
            }
            _ => {
                let uri = request_uri(params)?;
                let document = self
                    .documents
                    .sources
                    .get(&uri)
                    .ok_or("document is not loaded")?;
                let byte = offset(&document.text, &params["position"])?;
                Ok(match method {
                    "textDocument/hover" => self.index.hover(&uri, byte),
                    "textDocument/definition" => self.index.definition(&uri, byte),
                    "textDocument/completion" => self
                        .documents
                        .import_completions(&uri, byte)
                        .unwrap_or_else(|| self.index.completion(&uri, byte)),
                    _ => unreachable!(),
                })
            }
        }
    }

    fn notification(&mut self, method: &str, params: &Value) -> Result<Vec<Value>, String> {
        match method {
            "initialized" => {
                if self.ready {
                    return Ok(Vec::new());
                }
                self.ready = true;
                let mut messages = self.validate();
                if self.dynamic_watches {
                    messages.push(json!({ "jsonrpc": "2.0", "id": "rdl-watch", "method": "client/registerCapability", "params": {
                        "registrations": [{ "id": "rdl-sources", "method": "workspace/didChangeWatchedFiles", "registerOptions": { "watchers": [{ "globPattern": "**/*.rdl", "kind": 7 }] } }]
                    } }));
                }
                Ok(messages)
            }
            "textDocument/didOpen" => {
                let item = &params["textDocument"];
                let uri = document_uri(required_str(item, "uri")?)?;
                let document = Document {
                    text: required_str(item, "text")?.into(),
                    version: Some(required_version(item)?),
                };
                if self.documents.open.contains_key(&uri) {
                    return Err("document is already open".into());
                }
                self.documents.open.insert(uri.clone(), document.clone());
                self.documents.read_errors.remove(&uri);
                self.documents.sources.insert(uri, document);
                Ok(self.validate())
            }
            "textDocument/didChange" => {
                let uri = request_uri(params)?;
                let changes = params["contentChanges"]
                    .as_array()
                    .ok_or("didChange needs contentChanges")?;
                if self.documents.change(
                    &uri,
                    required_version(&params["textDocument"])?,
                    changes,
                )? {
                    Ok(self.validate())
                } else {
                    Ok(Vec::new())
                }
            }
            "textDocument/didClose" => {
                let uri = request_uri(params)?;
                self.documents.open.remove(&uri);
                self.documents.reload();
                Ok(self.validate())
            }
            "textDocument/didSave" | "workspace/didChangeWatchedFiles" => {
                self.documents.reload();
                Ok(self.validate())
            }
            "workspace/didChangeWorkspaceFolders" => {
                if let Some(removed) = params["event"]["removed"].as_array() {
                    for folder in removed {
                        self.documents
                            .roots
                            .remove(&file_path(required_str(folder, "uri")?)?);
                    }
                }
                if let Some(added) = params["event"]["added"].as_array() {
                    for folder in added {
                        self.documents
                            .roots
                            .insert(file_path(required_str(folder, "uri")?)?);
                    }
                }
                self.documents.reload();
                Ok(self.validate())
            }
            _ => Ok(Vec::new()),
        }
    }

    fn validate(&mut self) -> Vec<Value> {
        self.documents.resolve();
        self.index = Index::build(&self.documents.sources);
        self.index.import_links(self.documents.graph.links());
        let mut diagnostics: BTreeMap<String, Vec<Value>> = self
            .documents
            .sources
            .keys()
            .map(|uri| (uri.clone(), Vec::new()))
            .collect();
        for (uri, message) in &self.documents.read_errors {
            diagnostics.entry(uri.clone()).or_default().push(json!({ "range": { "start": { "line": 0, "character": 0 }, "end": { "line": 0, "character": 0 } }, "severity": 1, "source": "rdl", "message": message }));
        }
        for error in self.documents.graph.errors() {
            if let Some(document) = self.documents.sources.get(&error.span.source) {
                diagnostics.entry(error.span.source.clone()).or_default().push(json!({ "range": diagnostic_range(&document.text, &error.span), "severity": 1, "source": "rdl", "message": error.message }));
            }
        }
        let mut syntax_ok = true;
        for (uri, document) in &self.documents.sources {
            if let Err(error) = parser::parse(uri, &document.text) {
                syntax_ok = false;
                diagnostics.get_mut(uri).unwrap().push(json!({ "range": diagnostic_range(&document.text, &error.span), "severity": 1, "source": "rdl", "message": error.message }));
            }
        }
        if syntax_ok
            && self.documents.read_errors.is_empty()
            && self.documents.graph.errors().is_empty()
            && let Err(error) = self.documents.graph.load_into(&mut World::new())
            && let Some(document) = self.documents.sources.get(&error.span.source)
        {
            diagnostics.get_mut(&error.span.source).unwrap().push(json!({ "range": diagnostic_range(&document.text, &error.span), "severity": 1, "source": "rdl", "message": error.message }));
        }
        for uri in &self.published {
            diagnostics.entry(uri.clone()).or_default();
        }
        for values in diagnostics.values_mut() {
            let mut unique = Vec::new();
            for value in std::mem::take(values) {
                if !unique.contains(&value) {
                    unique.push(value);
                }
            }
            *values = unique;
        }
        let mut messages = Vec::new();
        for (uri, values) in &diagnostics {
            let mut params = json!({ "uri": uri, "diagnostics": values });
            if let Some(version) = self
                .documents
                .open
                .get(uri)
                .and_then(|document| document.version)
            {
                params["version"] = json!(version);
            }
            messages.push(notification("textDocument/publishDiagnostics", params));
        }
        self.published = self
            .documents
            .sources
            .keys()
            .chain(self.documents.read_errors.keys())
            .cloned()
            .collect();
        if self.semantic_refresh && !self.documents.open.is_empty() {
            self.refresh_counter += 1;
            messages.push(json!({ "jsonrpc": "2.0", "id": format!("rdl-semantic-{}", self.refresh_counter), "method": "workspace/semanticTokens/refresh" }));
        }
        messages
    }
}

fn required_str<'a>(value: &'a Value, key: &str) -> Result<&'a str, String> {
    value[key]
        .as_str()
        .ok_or_else(|| format!("expected string '{key}'"))
}

fn required_version(value: &Value) -> Result<i64, String> {
    value["version"]
        .as_i64()
        .ok_or_else(|| "expected an integer document version".into())
}

fn request_uri(params: &Value) -> Result<String, String> {
    document_uri(required_str(&params["textDocument"], "uri")?)
}
fn notification(method: &str, params: Value) -> Value {
    json!({ "jsonrpc": "2.0", "method": method, "params": params })
}
fn failure(id: Value, code: i64, message: &str) -> Value {
    json!({ "jsonrpc": "2.0", "id": id, "error": { "code": code, "message": message } })
}
