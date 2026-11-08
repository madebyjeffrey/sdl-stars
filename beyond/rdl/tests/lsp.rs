#![cfg(feature = "lsp")]

use std::io::{Cursor, Write};
use std::process::{Command, Stdio};

use rdl::lsp;
use serde_json::{Value, json};
use url::Url;

const SCHEMA: &str = "trait type primary = { name : string; }\ntrait hyper : primary = { name = \"Hyper\"; }\nobject type race = {\n name : string;\n primary : primary option;\n population : int;\n require self.population >= 0;\n derive points : int = self.population / 100;\n}\n";
const FACTS: &str = "race terrans = { name = \"Terrans\"; primary = hyper; population = 25000; }\nquery { from r : race select r.points }\n";

fn uri(name: &str) -> String {
    Url::from_file_path(std::env::temp_dir().join("rdl lsp tests 星").join(name))
        .unwrap()
        .into()
}
fn request(id: Value, method: &str, params: Value) -> Value {
    json!({"jsonrpc":"2.0", "id":id, "method":method, "params":params})
}
fn notify(method: &str, params: Value) -> Value {
    json!({"jsonrpc":"2.0", "method":method, "params":params})
}
fn initialize() -> Value {
    request(
        json!(0),
        "initialize",
        json!({"rootUri":null,"capabilities":{"textDocument":{"documentSymbol":{"hierarchicalDocumentSymbolSupport":true}}}}),
    )
}
fn open(name: &str, text: &str) -> Value {
    notify(
        "textDocument/didOpen",
        json!({"textDocument":{"uri":uri(name),"languageId":"rdl","version":1,"text":text}}),
    )
}
fn full_change(name: &str, version: i64, text: &str) -> Value {
    notify(
        "textDocument/didChange",
        json!({"textDocument":{"uri":uri(name),"version":version},"contentChanges":[{"text":text}]}),
    )
}
fn position(text: &str, byte: usize) -> Value {
    let prefix = &text[..byte];
    json!({"line":prefix.bytes().filter(|ch| *ch == b'\n').count(), "character":prefix.rsplit('\n').next().unwrap().encode_utf16().count()})
}
fn at(name: &str, text: &str, needle: &str) -> Value {
    json!({"textDocument":{"uri":uri(name)},"position":position(text, text.rfind(needle).unwrap())})
}
fn encode(messages: &[Value]) -> Vec<u8> {
    let mut bytes = Vec::new();
    for message in messages {
        let body = serde_json::to_vec(message).unwrap();
        write!(bytes, "Content-Length: {}\r\n\r\n", body.len()).unwrap();
        bytes.extend(body);
    }
    bytes
}
fn decode(mut bytes: &[u8]) -> Vec<Value> {
    let mut messages = Vec::new();
    while !bytes.is_empty() {
        let header_end = bytes
            .windows(4)
            .position(|window| window == b"\r\n\r\n")
            .expect("stdout must contain only framed LSP messages");
        let header = std::str::from_utf8(&bytes[..header_end]).unwrap();
        let length: usize = header
            .strip_prefix("Content-Length: ")
            .unwrap()
            .parse()
            .unwrap();
        bytes = &bytes[header_end + 4..];
        messages.push(serde_json::from_slice(&bytes[..length]).unwrap());
        bytes = &bytes[length..];
    }
    messages
}
fn transcript(mut messages: Vec<Value>) -> Vec<Value> {
    messages.push(request(json!(9999), "shutdown", Value::Null));
    messages.push(notify("exit", Value::Null));
    let mut output = Vec::new();
    lsp::serve(Cursor::new(encode(&messages)), &mut output, Vec::new()).unwrap();
    decode(&output)
}
fn session(messages: Vec<Value>) -> Vec<Value> {
    let mut all = vec![initialize(), notify("initialized", json!({}))];
    all.extend(messages);
    transcript(all)
}
fn response(messages: &[Value], id: Value) -> &Value {
    messages
        .iter()
        .find(|message| message.get("id") == Some(&id))
        .unwrap()
}
fn latest_diagnostics<'a>(messages: &'a [Value], name: &str) -> &'a Value {
    messages
        .iter()
        .rev()
        .find(|message| {
            message["method"] == "textDocument/publishDiagnostics"
                && message["params"]["uri"] == uri(name)
        })
        .unwrap()
        .get("params")
        .unwrap()
}

#[test]
fn lifecycle_and_capabilities_are_standard_json_rpc() {
    let messages = transcript(vec![
        request(json!("early"), "textDocument/hover", json!({})),
        initialize(),
        request(json!(2), "unknown", json!({})),
    ]);
    assert_eq!(response(&messages, json!("early"))["error"]["code"], -32002);
    assert_eq!(response(&messages, json!(2))["error"]["code"], -32601);
    let capabilities = &response(&messages, json!(0))["result"]["capabilities"];
    assert_eq!(capabilities["positionEncoding"], "utf-16");
    assert_eq!(capabilities["textDocumentSync"]["change"], 2);
    assert_eq!(capabilities["hoverProvider"], true);
}

#[test]
fn unsaved_files_validate_as_one_world_and_diagnostics_clear_after_fix() {
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("facts.rdl", FACTS),
        full_change("facts.rdl", 2, &FACTS.replace("25000", "-1")),
        full_change("facts.rdl", 3, FACTS),
    ]);
    assert!(messages.iter().any(|message| {
        message["params"]["diagnostics"]
            .as_array()
            .is_some_and(|values| {
                values.iter().any(|value| {
                    value["message"]
                        .as_str()
                        .is_some_and(|text| text.contains("require failed"))
                })
            })
    }));
    assert_eq!(latest_diagnostics(&messages, "facts.rdl")["version"], 3);
    assert!(
        latest_diagnostics(&messages, "facts.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert!(
        latest_diagnostics(&messages, "schema.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn hover_and_definition_resolve_fields_across_files() {
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("facts.rdl", FACTS),
        request(
            json!(1),
            "textDocument/hover",
            at("facts.rdl", FACTS, "points"),
        ),
        request(
            json!(2),
            "textDocument/definition",
            at("facts.rdl", FACTS, "points"),
        ),
        request(
            json!(3),
            "textDocument/definition",
            at("facts.rdl", FACTS, "hyper"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "derive points : int"
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["uri"],
        uri("schema.rdl")
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["range"]["start"],
        position(SCHEMA, SCHEMA.find("points").unwrap())
    );
    assert_eq!(
        response(&messages, json!(3))["result"]["range"]["start"],
        position(SCHEMA, SCHEMA.find("hyper").unwrap())
    );
}

#[test]
fn declarations_hover_and_navigate_to_their_own_name() {
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("facts.rdl", FACTS),
        request(
            json!(1),
            "textDocument/definition",
            at("facts.rdl", FACTS, "terrans"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["range"]["start"],
        position(FACTS, FACTS.find("terrans").unwrap())
    );
}

#[test]
fn field_completion_works_while_a_query_is_incomplete() {
    let text = "query { from r : race select r. }";
    let mut params = at("query.rdl", text, "r.");
    params["position"] = position(text, text.find("r.").unwrap() + 2);
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("query.rdl", text),
        request(json!(1), "textDocument/completion", params),
    ]);
    let labels: Vec<_> = response(&messages, json!(1))["result"]["items"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| item["label"].as_str().unwrap())
        .collect();
    assert!(labels.contains(&"population"));
    assert!(labels.contains(&"points"));
    assert!(!labels.contains(&"query"));
}

#[test]
fn self_and_module_completion_work_in_incomplete_expressions() {
    let schema = "object type race = { population : int; derive points : int = self.; }";
    let mut params = at("schema.rdl", schema, "self.");
    params["position"] = position(schema, schema.find("self.").unwrap() + 5);
    let text = "let size = Set.";
    let mut module = at("module.rdl", text, "Set.");
    module["position"] = position(text, text.len());
    let messages = session(vec![
        open("schema.rdl", schema),
        request(json!(1), "textDocument/completion", params),
        open("module.rdl", text),
        request(json!(2), "textDocument/completion", module),
    ]);
    assert!(
        response(&messages, json!(1))["result"]["items"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["label"] == "population")
    );
    assert!(
        response(&messages, json!(2))["result"]["items"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["label"] == "contains")
    );
}

#[test]
fn completion_replaces_the_full_partial_field_word() {
    let text = "query { from r : race select r.pop }";
    let mut params = at("q.rdl", text, "pop");
    params["position"] = position(text, text.find("pop").unwrap() + 2);
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("q.rdl", text),
        request(json!(1), "textDocument/completion", params),
    ]);
    let item = response(&messages, json!(1))["result"]["items"]
        .as_array()
        .unwrap()
        .iter()
        .find(|item| item["label"] == "population")
        .unwrap();
    assert_eq!(item["label"], "population");
    assert_eq!(
        item["textEdit"]["range"]["end"],
        position(text, text.find("pop").unwrap() + 3)
    );
}

#[test]
fn local_let_bindings_shadow_globals_for_navigation() {
    let text = "let x = 1; let y = (let x = 2 in x + 1);";
    let messages = session(vec![
        open("local.rdl", text),
        request(
            json!(1),
            "textDocument/definition",
            at("local.rdl", text, "x +"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["range"]["start"],
        position(text, text.find("x = 2").unwrap())
    );
}

#[test]
fn document_symbols_include_fields_and_derived_properties() {
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        request(
            json!(1),
            "textDocument/documentSymbol",
            json!({"textDocument":{"uri":uri("schema.rdl")}}),
        ),
    ]);
    let symbols = response(&messages, json!(1))["result"].as_array().unwrap();
    let race = symbols
        .iter()
        .find(|symbol| symbol["name"] == "race")
        .unwrap();
    assert!(
        race["children"]
            .as_array()
            .unwrap()
            .iter()
            .any(|field| field["name"] == "points")
    );
    assert_eq!(
        race["selectionRange"]["start"],
        position(SCHEMA, SCHEMA.find("race =").unwrap())
    );
}

#[test]
fn syntax_errors_in_several_documents_are_all_reported() {
    let messages = session(vec![open("a.rdl", "let a = ;"), open("b.rdl", "let b = ;")]);
    assert_eq!(
        latest_diagnostics(&messages, "a.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .len(),
        1
    );
    assert_eq!(
        latest_diagnostics(&messages, "b.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .len(),
        1
    );
}

#[test]
fn closing_an_unsaved_document_clears_its_diagnostics() {
    let messages = session(vec![
        open("bad.rdl", "let x = missing;"),
        notify(
            "textDocument/didClose",
            json!({"textDocument":{"uri":uri("bad.rdl")}}),
        ),
    ]);
    assert!(
        latest_diagnostics(&messages, "bad.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn diagnostic_columns_count_utf16_units_after_an_emoji() {
    let text = "let label = \"🌍\"; let x = missing;";
    let messages = session(vec![open("unicode.rdl", text)]);
    assert_eq!(
        latest_diagnostics(&messages, "unicode.rdl")["diagnostics"][0]["range"]["start"],
        position(text, text.find("missing").unwrap())
    );
}

#[test]
fn incremental_changes_apply_sequentially_using_utf16_positions() {
    let initial = "let label = \"🌍\"; let x = 1;";
    let intermediate = initial.replace('🌍', "星");
    let emoji = initial.find('🌍').unwrap();
    let number = intermediate.rfind('1').unwrap();
    let messages = session(vec![
        open("unicode.rdl", initial),
        notify(
            "textDocument/didChange",
            json!({"textDocument":{"uri":uri("unicode.rdl"),"version":2},"contentChanges":[
                {"range":{"start":position(initial,emoji),"end":position(initial,emoji+4)},"text":"星"},
                {"range":{"start":position(&intermediate,number),"end":position(&intermediate,number+1)},"text":"missing"}
            ]}),
        ),
    ]);
    assert!(
        latest_diagnostics(&messages, "unicode.rdl")["diagnostics"][0]["message"]
            .as_str()
            .unwrap()
            .contains("missing")
    );
    assert_eq!(latest_diagnostics(&messages, "unicode.rdl")["version"], 2);
}

#[test]
fn invalid_edits_are_atomic_and_old_versions_are_ignored() {
    let text = "let label = \"🌍\"; let x = 1;";
    let mut midpoint = position(text, text.find('🌍').unwrap());
    midpoint["character"] = json!(midpoint["character"].as_u64().unwrap() + 1);
    let messages = session(vec![
        open("unicode.rdl", text),
        notify(
            "textDocument/didChange",
            json!({"textDocument":{"uri":uri("unicode.rdl"),"version":2},"contentChanges":[{"range":{"start":midpoint,"end":midpoint},"text":"bad"}]}),
        ),
        full_change("unicode.rdl", 0, "let x = missing;"),
        request(
            json!(1),
            "textDocument/hover",
            at("unicode.rdl", text, "x ="),
        ),
    ]);
    assert!(
        messages
            .iter()
            .any(|message| message["method"] == "window/logMessage")
    );
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "let x : int"
    );
    assert_eq!(latest_diagnostics(&messages, "unicode.rdl")["version"], 1);
}

#[test]
fn malformed_json_gets_a_parse_error_without_desynchronizing_transport() {
    let mut bytes = b"Content-Length: 1\r\n\r\n{".to_vec();
    bytes.extend(encode(&[
        initialize(),
        request(json!(2), "shutdown", Value::Null),
        notify("exit", Value::Null),
    ]));
    let mut output = Vec::new();
    lsp::serve(Cursor::new(bytes), &mut output, Vec::new()).unwrap();
    let messages = decode(&output);
    assert_eq!(messages[0]["error"]["code"], -32700);
    assert_eq!(
        response(&messages, json!(0))["result"]["serverInfo"]["name"],
        "rdl"
    );
}

#[test]
fn malformed_or_oversized_headers_fail_without_allocating_a_body() {
    for bytes in [
        "Content-Length: 999999999\r\n\r\n",
        "Content-Length: 1\r\nContent-Length: 1\r\n\r\n{}",
        "Other: 1\r\n\r\n{}",
        "Content-Length: 5\r\n\r\n{",
    ] {
        assert!(lsp::serve(Cursor::new(bytes), Vec::new(), Vec::new()).is_err());
    }
}

#[test]
fn binary_stdio_contains_only_protocol_messages() {
    let input = encode(&[
        initialize(),
        notify("initialized", json!({})),
        open("schema.rdl", SCHEMA),
        request(json!(1), "shutdown", Value::Null),
        notify("exit", Value::Null),
    ]);
    let mut child = Command::new(env!("CARGO_BIN_EXE_rdl"))
        .arg("lsp")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    child.stdin.take().unwrap().write_all(&input).unwrap();
    let output = child.wait_with_output().unwrap();
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    assert!(output.stderr.is_empty());
    assert!(decode(&output.stdout).len() >= 3);
}

#[test]
fn exit_without_shutdown_is_unsuccessful() {
    assert!(
        lsp::serve(
            Cursor::new(encode(&[notify("exit", Value::Null)])),
            Vec::new(),
            Vec::new()
        )
        .is_err()
    );
}

struct Fixture(std::path::PathBuf);
impl Fixture {
    fn new() -> Self {
        static NEXT: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
        let path = std::env::temp_dir().join(format!(
            "rdl-lsp-{}-{}-{} 星",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos(),
            NEXT.fetch_add(1, std::sync::atomic::Ordering::Relaxed)
        ));
        std::fs::create_dir(&path).unwrap();
        Self(path)
    }
    fn uri(&self, name: &str) -> String {
        Url::from_file_path(self.0.join(name)).unwrap().into()
    }
    fn write(&self, name: &str, text: &str) {
        std::fs::write(self.0.join(name), text).unwrap();
    }
}
impl Drop for Fixture {
    fn drop(&mut self) {
        std::fs::remove_dir_all(&self.0).unwrap();
    }
}

#[test]
fn workspace_discovery_reads_disk_and_skips_build_directories() {
    let fixture = Fixture::new();
    fixture.write("schema.rdl", SCHEMA);
    fixture.write("facts.rdl", FACTS);
    std::fs::create_dir(fixture.0.join("target")).unwrap();
    fixture.write("target/bad.rdl", "let x = ;");
    let mut initialize = initialize();
    initialize["params"]["rootUri"] = json!(fixture.uri(""));
    let messages = transcript(vec![
        initialize,
        notify("initialized", json!({})),
        request(
            json!(1),
            "textDocument/hover",
            json!({"textDocument":{"uri":fixture.uri("facts.rdl")},"position":position(FACTS,FACTS.rfind("points").unwrap())}),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "derive points : int"
    );
    assert!(
        !messages
            .iter()
            .any(|message| message["params"]["uri"] == fixture.uri("target/bad.rdl"))
    );
    assert!(
        messages
            .iter()
            .filter(|message| message["method"] == "textDocument/publishDiagnostics")
            .all(|message| message["params"]["diagnostics"]
                .as_array()
                .unwrap()
                .is_empty())
    );
}

#[test]
fn explicit_initialization_files_disable_disk_discovery() {
    let fixture = Fixture::new();
    fixture.write("schema.rdl", SCHEMA);
    fixture.write("facts.rdl", FACTS);
    fixture.write("bad.rdl", "let bad = ;");
    let mut initialize = initialize();
    initialize["params"]["rootUri"] = json!(fixture.uri(""));
    initialize["params"]["initializationOptions"] = json!({"files":["schema.rdl","facts.rdl"]});
    let messages = transcript(vec![initialize, notify("initialized", json!({}))]);
    assert!(
        messages
            .iter()
            .any(|message| message["params"]["uri"] == fixture.uri("facts.rdl"))
    );
    assert!(
        !messages
            .iter()
            .any(|message| message["params"]["uri"] == fixture.uri("bad.rdl"))
    );
}

#[test]
fn closing_a_disk_document_restores_its_saved_facts() {
    let fixture = Fixture::new();
    fixture.write("facts.rdl", "let points = 42;");
    let messages=transcript(vec![
        initialize(),notify("initialized",json!({})),
        notify("textDocument/didOpen",json!({"textDocument":{"uri":fixture.uri("facts.rdl"),"version":1,"text":"let points = missing;"}})),
        notify("textDocument/didClose",json!({"textDocument":{"uri":fixture.uri("facts.rdl")}})),
        request(json!(1),"textDocument/hover",json!({"textDocument":{"uri":fixture.uri("facts.rdl")},"position":{"line":0,"character":4}}))
    ].into_iter().enumerate().map(|(i,mut message)| { if i==0 {message["params"]["initializationOptions"]=json!({"files":[fixture.0.join("facts.rdl").to_str().unwrap()]});} message }).collect());
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "let points : int"
    );
}

#[test]
fn removing_workspace_folders_clears_old_diagnostics() {
    let fixture = Fixture::new();
    fixture.write("bad.rdl", "let x = missing;");
    let mut initialize = initialize();
    initialize["params"]["workspaceFolders"] = json!([{"uri":fixture.uri(""),"name":"fixture"}]);
    let messages = transcript(vec![
        initialize,
        notify("initialized", json!({})),
        notify(
            "workspace/didChangeWorkspaceFolders",
            json!({"event":{"removed":[{"uri":fixture.uri(""),"name":"fixture"}],"added":[]}}),
        ),
    ]);
    let publications: Vec<_> = messages
        .iter()
        .filter(|message| message["params"]["uri"] == fixture.uri("bad.rdl"))
        .collect();
    assert!(
        !publications.first().unwrap()["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert!(
        publications.last().unwrap()["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn dynamic_file_watch_registration_is_requested_only_when_supported() {
    let mut initialize = initialize();
    initialize["params"]["capabilities"]["workspace"]["didChangeWatchedFiles"]["dynamicRegistration"] =
        json!(true);
    let messages = transcript(vec![initialize, notify("initialized", json!({}))]);
    assert!(messages.iter().any(|message|message["method"]=="client/registerCapability" && message["params"]["registrations"][0]["registerOptions"]["watchers"][0]["globPattern"]=="**/*.rdl"));
}

#[test]
fn owner_context_and_chained_derived_fields_have_navigation_and_completion() {
    let schema = "type h = | immune | int range with (owner : race) { derive points : int = owner.population; } object type race = { population : int; habitat : h; } race r = { population = 1; habitat = 20 .. 80; } r.habitat.";
    let mut completion = at("context.rdl", schema, "r.habitat.");
    completion["position"] = position(schema, schema.len());
    let messages = session(vec![
        open("context.rdl", schema),
        request(
            json!(1),
            "textDocument/definition",
            at("context.rdl", schema, "owner.population"),
        ),
        request(json!(2), "textDocument/completion", completion),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["range"]["start"],
        position(schema, schema.find("owner :").unwrap())
    );
    assert!(
        response(&messages, json!(2))["result"]["items"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["label"] == "points")
    );
}

#[test]
fn alias_navigation_follows_original_field_declarations() {
    let original = "type settings = { value : int; }";
    let alias = "type alias = settings with { derive twice : int = self.value * 2; }";
    let messages = session(vec![
        open("original.rdl", original),
        open("alias.rdl", alias),
        request(
            json!(1),
            "textDocument/definition",
            at("alias.rdl", alias, "value"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["uri"],
        uri("original.rdl")
    );
    assert_eq!(
        response(&messages, json!(1))["result"]["range"]["start"],
        position(original, original.find("value").unwrap())
    );
}

#[test]
fn deeply_nested_invalid_documents_do_not_crash_the_symbol_index() {
    let text = format!("let x = {}1{};", "(".repeat(200), ")".repeat(200));
    let messages = session(vec![open("deep.rdl", &text)]);
    assert!(
        !latest_diagnostics(&messages, "deep.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn relative_initialization_paths_use_the_first_client_workspace_folder() {
    let fixture = Fixture::new();
    std::fs::create_dir(fixture.0.join("a")).unwrap();
    std::fs::create_dir(fixture.0.join("z")).unwrap();
    fixture.write("z/facts.rdl", "let points = 42;");
    fixture.write("a/facts.rdl", "let bad = ;");
    let mut initialize = initialize();
    initialize["params"]["workspaceFolders"] =
        json!([{"uri":fixture.uri("z"),"name":"z"},{"uri":fixture.uri("a"),"name":"a"}]);
    initialize["params"]["initializationOptions"] = json!({"files":["facts.rdl"]});
    let messages = transcript(vec![initialize, notify("initialized", json!({}))]);
    assert!(
        messages
            .iter()
            .any(|message| message["params"]["uri"] == fixture.uri("z/facts.rdl"))
    );
    assert!(
        !messages
            .iter()
            .any(|message| message["params"]["uri"] == fixture.uri("a/facts.rdl"))
    );
}

#[test]
fn open_buffers_override_invalid_utf8_disk_sources() {
    let fixture = Fixture::new();
    std::fs::write(fixture.0.join("facts.rdl"), [0xff]).unwrap();
    let mut initialize = initialize();
    initialize["params"]["rootUri"] = json!(fixture.uri(""));
    let messages = transcript(vec![
        initialize,
        notify("initialized", json!({})),
        notify(
            "textDocument/didOpen",
            json!({"textDocument":{"uri":fixture.uri("facts.rdl"),"version":1,"text":"let points = 42;"}}),
        ),
    ]);
    let publications: Vec<_> = messages
        .iter()
        .filter(|message| message["params"]["uri"] == fixture.uri("facts.rdl"))
        .collect();
    assert!(
        !publications.first().unwrap()["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert!(
        publications.last().unwrap()["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[derive(Debug)]
struct Colored {
    text: String,
    kind: String,
    modifiers: Vec<String>,
    line: usize,
    column: usize,
    length: usize,
}

fn colors(messages: &[Value], id: Value, text: &str) -> Vec<Colored> {
    let capabilities =
        &response(messages, json!(0))["result"]["capabilities"]["semanticTokensProvider"];
    assert_eq!(capabilities["full"], true);
    let types = capabilities["legend"]["tokenTypes"].as_array().unwrap();
    let modifiers = capabilities["legend"]["tokenModifiers"].as_array().unwrap();
    let data = response(messages, id)["result"]["data"].as_array().unwrap();
    assert_eq!(data.len() % 5, 0);
    let lines: Vec<_> = text.split('\n').collect();
    let (mut line, mut column, mut previous_end) = (0, 0, 0);
    let mut result = Vec::new();
    for token in data.as_chunks::<5>().0 {
        let delta = token[0].as_u64().unwrap() as usize;
        line += delta;
        column = if delta == 0 {
            column + token[1].as_u64().unwrap() as usize
        } else {
            token[1].as_u64().unwrap() as usize
        };
        if delta != 0 {
            previous_end = 0;
        }
        let length = token[2].as_u64().unwrap() as usize;
        assert!(
            length > 0 && column >= previous_end,
            "tokens must be nonempty and non-overlapping"
        );
        previous_end = column + length;
        let units: Vec<_> = lines[line].encode_utf16().collect();
        let source = String::from_utf16(&units[column..column + length]).unwrap();
        assert!(
            !source.contains(['\r', '\n']),
            "multiline spans must be split into single-line tokens"
        );
        let bits = token[4].as_u64().unwrap();
        assert!(bits < (1 << modifiers.len()));
        result.push(Colored {
            text: source,
            kind: types[token[3].as_u64().unwrap() as usize]
                .as_str()
                .unwrap()
                .into(),
            modifiers: modifiers
                .iter()
                .enumerate()
                .filter(|(i, _)| bits & (1 << i) != 0)
                .map(|(_, name)| name.as_str().unwrap().into())
                .collect(),
            line,
            column,
            length,
        });
    }
    result
}

fn semantic_request(id: i64, name: &str) -> Value {
    request(
        json!(id),
        "textDocument/semanticTokens/full",
        json!({"textDocument":{"uri":uri(name)}}),
    )
}

#[test]
fn semantic_tokens_distinguish_declarations_properties_and_references() {
    let messages = session(vec![
        open("schema.rdl", SCHEMA),
        open("facts.rdl", FACTS),
        semantic_request(1, "schema.rdl"),
        semantic_request(2, "facts.rdl"),
    ]);
    let schema = colors(&messages, json!(1), SCHEMA);
    assert!(
        schema
            .iter()
            .any(|token| token.text == "self" && token.kind == "variable")
    );
    assert!(
        schema
            .iter()
            .any(|token| token.text == "hyper" && token.kind == "enumMember")
    );
    assert!(schema.iter().any(|token| token.text == "race"
        && token.kind == "class"
        && token.modifiers.contains(&"declaration".into())));
    assert!(
        schema
            .iter()
            .any(|token| token.text == "primary" && token.kind == "interface")
    );
    assert!(schema.iter().any(|token| token.text == "population"
        && token.kind == "property"
        && token.modifiers.contains(&"declaration".into())));
    assert!(
        schema
            .iter()
            .any(|token| token.text == "int" && token.modifiers.contains(&"defaultLibrary".into()))
    );
    let facts = colors(&messages, json!(2), FACTS);
    assert!(facts.iter().any(|token| token.text == "terrans"
        && token.kind == "variable"
        && token.modifiers.contains(&"readonly".into())));
    assert!(facts.iter().any(|token| token.text == "points"
        && token.kind == "property"
        && !token.modifiers.contains(&"declaration".into())));
    assert!(
        facts
            .iter()
            .any(|token| token.text == "25000" && token.kind == "number")
    );
}

#[test]
fn semantic_tokens_split_multiline_strings_and_nested_comments() {
    let text = "(* outer\n(* nested 🌍 *) still outer\n*)\nlet text = \"\"\"first 🌍\nselect 20 .. 80 // raw\n\"\"\";\nlet n = 20..80;";
    let messages = session(vec![
        open("colors.rdl", text),
        semantic_request(1, "colors.rdl"),
    ]);
    let tokens = colors(&messages, json!(1), text);
    assert!(tokens.iter().any(|token| token.line == 1
        && token.kind == "comment"
        && token.text.contains("nested 🌍")));
    assert!(tokens.iter().any(|token| token.line == 4
        && token.kind == "string"
        && token.text == "select 20 .. 80 // raw"));
    assert!(
        tokens
            .iter()
            .any(|token| token.text == ".." && token.kind == "operator")
    );
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "20" && token.kind == "number")
    );
    let string = tokens
        .iter()
        .find(|token| token.text == "\"\"\"first 🌍")
        .unwrap();
    assert_eq!(string.length, "\"\"\"first 🌍".encode_utf16().count());
}

#[test]
fn semantic_positions_handle_crlf_and_astral_unicode() {
    let text = "let label = \"🌍\"; let x = 1;\r\n// 星\r\nlet y = 2;";
    let messages = session(vec![
        open("colors.rdl", text),
        semantic_request(1, "colors.rdl"),
    ]);
    let tokens = colors(&messages, json!(1), text);
    let x = tokens.iter().find(|token| token.text == "x").unwrap();
    assert_eq!(
        x.column,
        text[..text.find("x =").unwrap()].encode_utf16().count()
    );
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "let" && token.line == 2 && token.column == 0)
    );
}

#[test]
fn highlighting_continues_through_invalid_or_incomplete_input() {
    let text = "let text = \"unfinished\nlet next = 20..80; (* open (* nested";
    let messages = session(vec![
        open("colors.rdl", text),
        semantic_request(1, "colors.rdl"),
    ]);
    let tokens = colors(&messages, json!(1), text);
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "let" && token.line == 1 && token.kind == "keyword")
    );
    assert!(
        tokens
            .iter()
            .any(|token| token.text.contains("open (* nested") && token.kind == "comment")
    );
    assert!(
        !latest_diagnostics(&messages, "colors.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn semantic_legend_respects_the_clients_supported_types_and_modifiers() {
    let mut init = initialize();
    init["params"]["capabilities"]["textDocument"]["semanticTokens"] =
        json!({"tokenTypes":["keyword","number","type"],"tokenModifiers":["declaration"]});
    let text = "object type race = { population : int; }";
    let messages = transcript(vec![
        init,
        notify("initialized", json!({})),
        open("colors.rdl", text),
        semantic_request(1, "colors.rdl"),
    ]);
    let tokens = colors(&messages, json!(1), text);
    assert!(
        tokens
            .iter()
            .all(|token| ["keyword", "number", "type"].contains(&token.kind.as_str()))
    );
    assert!(tokens.iter().any(|token| token.text == "race"
        && token.kind == "type"
        && token.modifiers == ["declaration"]));
}

#[test]
fn semantic_tokens_follow_the_current_buffer_after_an_edit() {
    let before = "let x = 20;";
    let after = "let x = \"new 🌍\";";
    let messages = session(vec![
        open("colors.rdl", before),
        semantic_request(1, "colors.rdl"),
        full_change("colors.rdl", 2, after),
        semantic_request(2, "colors.rdl"),
    ]);
    assert!(
        colors(&messages, json!(1), before)
            .iter()
            .any(|token| token.text == "20" && token.kind == "number")
    );
    let tokens = colors(&messages, json!(2), after);
    assert!(!tokens.iter().any(|token| token.kind == "number"));
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "\"new 🌍\"" && token.kind == "string")
    );
}

#[test]
fn semantic_highlighting_refreshes_after_world_changes_when_supported() {
    let mut init = initialize();
    init["params"]["capabilities"]["workspace"]["semanticTokens"]["refreshSupport"] = json!(true);
    let messages = transcript(vec![
        init,
        notify("initialized", json!({})),
        open("colors.rdl", "let x = 1;"),
        full_change("colors.rdl", 2, "let x = 2;"),
    ]);
    assert_eq!(
        messages
            .iter()
            .filter(|message| message["method"] == "workspace/semanticTokens/refresh")
            .count(),
        2
    );
}

fn import_initialize(fixture: &Fixture) -> Value {
    let mut init = initialize();
    init["params"]["rootUri"] = json!(fixture.uri(""));
    init["params"]["initializationOptions"] =
        json!({"files":["app/main.rdl"],"importPaths":["lib"]});
    init
}

fn diagnostics_for<'a>(messages: &'a [Value], uri: &str) -> &'a Value {
    &messages
        .iter()
        .rev()
        .find(|message| {
            message["method"] == "textDocument/publishDiagnostics"
                && message["params"]["uri"] == uri
        })
        .unwrap()["params"]["diagnostics"]
}

#[test]
fn named_imports_load_external_schema_and_navigate_to_modules_and_fields() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    let main = "import schema\nrace r = { points = 42; }\nquery { from r : race select r.points }";
    let schema = "object type race = { points : int; }";
    f.write("app/main.rdl", main);
    f.write("lib/schema.rdl", schema);
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        request(
            json!(1),
            "textDocument/definition",
            json!({"textDocument":{"uri":f.uri("app/main.rdl")},"position":position(main,main.find("schema").unwrap())}),
        ),
        request(
            json!(2),
            "textDocument/definition",
            json!({"textDocument":{"uri":f.uri("app/main.rdl")},"position":position(main,main.rfind("points").unwrap())}),
        ),
    ]);
    assert!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert_eq!(
        response(&messages, json!(1))["result"]["uri"],
        f.uri("lib/schema.rdl")
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["range"]["start"],
        position(schema, schema.find("points").unwrap())
    );
}

#[test]
fn imported_unsaved_buffers_override_disk_and_trigger_world_validation() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    f.write("app/main.rdl", "import schema\nrace r = { points = 42; }");
    let schema = "object type race = { points : int; }";
    f.write("lib/schema.rdl", schema);
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        notify(
            "textDocument/didOpen",
            json!({"textDocument":{"uri":f.uri("lib/schema.rdl"),"version":1,"text":"object type race = { points : string; }"}}),
        ),
        notify(
            "textDocument/didChange",
            json!({"textDocument":{"uri":f.uri("lib/schema.rdl"),"version":2},"contentChanges":[{"text":schema}]}),
        ),
    ]);
    assert!(messages.iter().any(|message| {
        message["params"]["diagnostics"]
            .as_array()
            .is_some_and(|values| {
                values.iter().any(|value| {
                    value["message"]
                        .as_str()
                        .is_some_and(|text| text.contains("expected string"))
                })
            })
    }));
    assert!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn a_new_unsaved_module_can_satisfy_a_missing_import() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    f.write("app/main.rdl", "import missing");
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        notify(
            "textDocument/didOpen",
            json!({"textDocument":{"uri":f.uri("lib/missing.rdl"),"version":1,"text":"let points = 42;"}}),
        ),
    ]);
    assert!(messages.iter().any(|message| {
        message["params"]["uri"] == f.uri("app/main.rdl")
            && message["params"]["diagnostics"]
                .as_array()
                .is_some_and(|values| {
                    values.iter().any(|value| {
                        value["message"]
                            .as_str()
                            .is_some_and(|text| text.contains("cannot find import"))
                    })
                })
    }));
    assert!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn removing_an_import_discards_its_unopened_dependencies() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    let main = "import defaults\nlet other = points;";
    f.write("app/main.rdl", main);
    f.write("lib/defaults.rdl", "let points = 42;");
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        notify(
            "textDocument/didOpen",
            json!({"textDocument":{"uri":f.uri("app/main.rdl"),"version":1,"text":main}}),
        ),
        notify(
            "textDocument/didChange",
            json!({"textDocument":{"uri":f.uri("app/main.rdl"),"version":2},"contentChanges":[{"text":"let other = points;"}]}),
        ),
    ]);
    assert!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))[0]["message"]
            .as_str()
            .unwrap()
            .contains("unknown name 'points'")
    );
    assert!(
        diagnostics_for(&messages, &f.uri("lib/defaults.rdl"))
            .as_array()
            .unwrap()
            .is_empty()
    );
}

#[test]
fn imported_modules_are_found_even_when_an_entry_file_has_unfinished_input() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    let main = "import schema\nlet text = \"unfinished";
    f.write("app/main.rdl", main);
    f.write("lib/schema.rdl", "object type race = { points : int; }");
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        request(
            json!(1),
            "textDocument/definition",
            json!({"textDocument":{"uri":f.uri("app/main.rdl")},"position":position(main,main.find("schema").unwrap())}),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["uri"],
        f.uri("lib/schema.rdl")
    );
    assert!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))[0]["message"]
            .as_str()
            .unwrap()
            .contains("unterminated string")
    );
}

#[test]
fn import_completion_uses_the_search_path_and_dotted_directory_names() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir_all(f.0.join("lib/stars")).unwrap();
    f.write("lib/stars/traits.rdl", "let points = 42;");
    let main = "import stars.tr";
    f.write("app/main.rdl", main);
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
        request(
            json!(1),
            "textDocument/completion",
            json!({"textDocument":{"uri":f.uri("app/main.rdl")},"position":position(main,main.len())}),
        ),
    ]);
    let items = response(&messages, json!(1))["result"]["items"]
        .as_array()
        .unwrap();
    let item = items
        .iter()
        .find(|item| item["label"] == "stars.traits")
        .unwrap();
    assert_eq!(item["textEdit"]["newText"], "stars.traits");
    assert_eq!(item["textEdit"]["range"]["start"], position(main, 7));
}

#[test]
fn import_cycles_are_reported_on_the_dependency_that_closes_them() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    std::fs::create_dir(f.0.join("lib")).unwrap();
    f.write("app/main.rdl", "import schema");
    f.write("lib/schema.rdl", "import main");
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
    ]);
    assert!(
        diagnostics_for(&messages, &f.uri("lib/schema.rdl"))[0]["message"]
            .as_str()
            .unwrap()
            .contains("cyclic import")
    );
}

#[test]
fn malformed_imports_produce_one_syntax_diagnostic() {
    let f = Fixture::new();
    std::fs::create_dir(f.0.join("app")).unwrap();
    f.write("app/main.rdl", "import stars.");
    let messages = transcript(vec![
        import_initialize(&f),
        notify("initialized", json!({})),
    ]);
    assert_eq!(
        diagnostics_for(&messages, &f.uri("app/main.rdl"))
            .as_array()
            .unwrap()
            .len(),
        1
    );
}

#[test]
fn components_inherit_fields_and_trait_contributions_have_editor_support() {
    let schema = r#"
abstract component Primary { name: string }
component Hyper : Primary { name = "Hyper" }
component Capacity { value: integer }
trait Baseline { provides Capacity { value: 1_000_000 } }
trait HyperPRT { provides Hyper modifies Capacity.value { multiply 0.5 } }
object type Race = {
  traits: Trait set;
  component primary: Primary;
  component capacity: Capacity;
}
"#;
    let facts = "Race r = { traits = { Baseline; HyperPRT }; }\nquery { from x : Race select { name = x.primary.name; capacity = x.capacity.value; } }";
    let messages = session(vec![
        open("components.rdl", schema),
        open("component-facts.rdl", facts),
        request(
            json!(1),
            "textDocument/hover",
            at("component-facts.rdl", facts, "value"),
        ),
        request(
            json!(2),
            "textDocument/definition",
            at("component-facts.rdl", facts, "name;"),
        ),
        request(
            json!(3),
            "textDocument/hover",
            at("components.rdl", schema, "Hyper modifies"),
        ),
        request(
            json!(4),
            "textDocument/definition",
            at("components.rdl", schema, "value { multiply"),
        ),
    ]);
    assert!(
        latest_diagnostics(&messages, "component-facts.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "value : int"
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["uri"],
        uri("components.rdl")
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["range"]["start"],
        position(schema, schema.find("name:").unwrap())
    );
    assert_eq!(
        response(&messages, json!(3))["result"]["contents"]["value"],
        "component Hyper : Primary"
    );
    assert_eq!(
        response(&messages, json!(4))["result"]["range"]["start"],
        position(schema, schema.find("value: integer").unwrap())
    );
}

#[test]
fn component_syntax_and_grouped_numbers_receive_semantic_tokens() {
    let source = "abstract component Base { value: Integer }\ncomponent C : Base { value = 1_000_000 }\ntrait T { provides C modifies C.value { multiply 0.5 } }";
    let messages = session(vec![
        open("components.rdl", source),
        semantic_request(1, "components.rdl"),
    ]);
    let tokens = colors(&messages, json!(1), source);
    for word in ["abstract", "component", "provides", "modifies", "multiply"] {
        assert!(
            tokens
                .iter()
                .any(|token| token.text == word && token.kind == "keyword"),
            "{word}"
        );
    }
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "1_000_000" && token.kind == "number")
    );
    assert!(
        tokens
            .iter()
            .any(|token| token.text == "C" && token.kind == "class")
    );
}

#[test]
fn scalar_components_and_cardinality_declarations_work_in_the_editor() {
    let schema = "component Capacity : integer { require self >= 0 }\ntrait T { provides Capacity = 100 }\nobject type O = { requires one Capacity traits: Trait set; }\n";
    let facts = "O o = { traits = { T }; }\nquery { from x: O select x.Capacity + 1 }";
    let messages = session(vec![
        open("scalar-schema.rdl", schema),
        open("scalar-facts.rdl", facts),
        request(
            json!(1),
            "textDocument/hover",
            at("scalar-schema.rdl", schema, "Capacity :"),
        ),
        request(
            json!(2),
            "textDocument/definition",
            at("scalar-facts.rdl", facts, "Capacity"),
        ),
        semantic_request(3, "scalar-schema.rdl"),
        full_change(
            "scalar-schema.rdl",
            2,
            &schema.replace("provides Capacity = 100", "provides Capacity = -1"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["contents"]["value"],
        "component Capacity : int"
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["uri"],
        uri("scalar-schema.rdl")
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["range"]["start"],
        position(schema, schema.find("Capacity traits").unwrap())
    );
    let tokens = colors(&messages, json!(3), schema);
    for word in ["requires", "one"] {
        assert!(
            tokens
                .iter()
                .any(|token| token.text == word && token.kind == "keyword"),
            "{word}"
        );
    }
    let all: Vec<_> = messages
        .iter()
        .filter(|message| message["method"] == "textDocument/publishDiagnostics")
        .collect();
    assert!(all.iter().any(|message| {
        message["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .iter()
            .any(|diagnostic| {
                diagnostic["message"]
                    .as_str()
                    .unwrap()
                    .contains("require failed")
            })
    }));
}

#[test]
fn parameterized_traits_navigate_and_expose_parameters_to_completion() {
    let schema = "component Band {lower:integer;upper:integer;} trait band(lower:integer,upper:integer) { provides Band {lower=lower;upper=upper;} } object type Example={requires one Band}";
    let facts = "Example a={band(15,85)} a.traits";
    let messages = session(vec![
        open("schema.rdl", schema),
        open("facts.rdl", facts),
        request(
            json!(1),
            "textDocument/definition",
            at("facts.rdl", facts, "band"),
        ),
        request(
            json!(2),
            "textDocument/definition",
            at("schema.rdl", schema, "lower;"),
        ),
        request(
            json!(3),
            "textDocument/hover",
            at("facts.rdl", facts, "band"),
        ),
        request(
            json!(4),
            "textDocument/hover",
            at("facts.rdl", facts, "traits"),
        ),
        request(
            json!(5),
            "textDocument/completion",
            at("schema.rdl", schema, "lower;"),
        ),
    ]);
    assert_eq!(
        response(&messages, json!(1))["result"]["range"]["start"],
        position(schema, schema.find("band").unwrap())
    );
    assert_eq!(
        response(&messages, json!(2))["result"]["range"]["start"],
        position(schema, schema.find("lower:integer,upper").unwrap())
    );
    assert!(
        response(&messages, json!(3))["result"]["contents"]["value"]
            .as_str()
            .unwrap()
            .contains("band(lower: int, upper: int)")
    );
    assert!(
        response(&messages, json!(4))["result"]["contents"]["value"]
            .as_str()
            .unwrap()
            .contains("Trait set")
    );
    assert!(
        response(&messages, json!(5))["result"]["items"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["label"] == "lower")
    );
}

#[test]
fn overloaded_traits_navigate_to_the_selected_arity_and_keep_parameter_scopes() {
    let unary = "type Immunity=|immune\ntrait type Label={label:string;}\ntrait habitat(mode:Immunity):Label {label=\"immune\";}";
    let binary = "trait type Bounds={lower:integer;}\ntrait habitat(lower:integer,upper:integer):Bounds {lower=lower;}";
    let facts = "let a=habitat(immune);\nlet b=habitat(15,85);\na.label;\nb.lower";
    let call = |needle: &str| json!({"textDocument":{"uri":uri("overload-facts.rdl")},"position":position(facts,facts.find(needle).unwrap())});
    let messages = session(vec![
        open("overload-unary.rdl", unary),
        open("overload-binary.rdl", binary),
        open("overload-facts.rdl", facts),
        request(json!(1), "textDocument/definition", call("habitat(immune)")),
        request(json!(2), "textDocument/definition", call("habitat(15,85)")),
        request(json!(3), "textDocument/hover", call("habitat(immune)")),
        request(json!(4), "textDocument/hover", call("habitat(15,85)")),
        request(
            json!(5),
            "textDocument/definition",
            at("overload-binary.rdl", binary, "lower;}"),
        ),
        request(
            json!(6),
            "textDocument/documentSymbol",
            json!({"textDocument":{"uri":uri("overload-unary.rdl")}}),
        ),
        request(json!(7), "textDocument/completion", call("habitat(15,85)")),
    ]);
    assert_eq!(
        latest_diagnostics(&messages, "overload-facts.rdl")["diagnostics"]
            .as_array()
            .unwrap()
            .len(),
        0
    );
    for (id, name, text) in [
        (1, "overload-unary.rdl", unary),
        (2, "overload-binary.rdl", binary),
    ] {
        let result = &response(&messages, json!(id))["result"];
        assert_eq!(result["uri"], uri(name));
        assert_eq!(
            result["range"]["start"],
            position(text, text.find("habitat").unwrap())
        );
    }
    assert!(
        response(&messages, json!(3))["result"]["contents"]["value"]
            .as_str()
            .unwrap()
            .contains("habitat(mode: Immunity) : Label")
    );
    assert!(
        response(&messages, json!(4))["result"]["contents"]["value"]
            .as_str()
            .unwrap()
            .contains("habitat(lower: int, upper: int) : Bounds")
    );
    assert_eq!(
        response(&messages, json!(5))["result"]["range"]["start"],
        position(binary, binary.find("lower:integer,upper").unwrap())
    );
    assert!(
        response(&messages, json!(6))["result"]
            .as_array()
            .unwrap()
            .iter()
            .any(|s| s["name"] == "habitat")
    );
    let completion = response(&messages, json!(7))["result"]["items"]
        .as_array()
        .unwrap()
        .iter()
        .find(|s| s["label"] == "habitat")
        .unwrap();
    assert!(
        completion["detail"]
            .as_str()
            .unwrap()
            .contains("mode: Immunity")
    );
    assert!(
        completion["detail"]
            .as_str()
            .unwrap()
            .contains("upper: int")
    );
}
