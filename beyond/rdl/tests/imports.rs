use std::path::{Path, PathBuf};

use rdl::{
    World,
    ast::Statement,
    imports::{Source, SourceLoader, read_file},
    parser,
};

struct Fixture(PathBuf);
impl Fixture {
    fn new() -> Self {
        static NEXT: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
        let path = std::env::temp_dir().join(format!(
            "rdl-imports-{}-{}-{} 星",
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
    fn path(&self, name: &str) -> PathBuf {
        self.0.join(name)
    }
    fn write(&self, name: &str, text: &str) {
        let path = self.path(name);
        std::fs::create_dir_all(path.parent().unwrap()).unwrap();
        std::fs::write(path, text).unwrap();
    }
    fn graph(&self, name: &str) -> rdl::imports::SourceGraph {
        SourceLoader::default().load_files(&[self.path(name)])
    }
    fn world(&self, name: &str) -> World {
        let mut world = World::new();
        self.graph(name).load_into(&mut world).unwrap();
        world
    }
}
impl Drop for Fixture {
    fn drop(&mut self) {
        std::fs::remove_dir_all(&self.0).unwrap();
    }
}

#[test]
fn parser_accepts_named_and_dotted_imports_with_optional_semicolons() {
    let statements = parser::parse("s", "import schema\nimport stars.traits;\nlet x = 1;").unwrap();
    assert!(matches!(&statements[0],Statement::Import(import) if import.name=="schema"));
    assert!(matches!(&statements[1],Statement::Import(import) if import.name=="stars.traits"));
    assert_eq!(
        parser::imports("s", "import schema\nlet text = \"unfinished").unwrap()[0].name,
        "schema"
    );
}

#[test]
fn imports_are_not_inferred_from_comments_strings_or_nested_expressions() {
    let text = "// import missing\n(* import other *)\nlet text = \"import absent\";\nlet raw = \"\"\"import nope\"\"\";";
    assert!(parser::imports("s", text).unwrap().is_empty());
    assert!(
        parser::imports("s", "object type race = { import missing; }")
            .unwrap()
            .is_empty()
    );
}

#[test]
fn imports_cannot_use_file_paths_or_escape_search_roots() {
    for text in [
        "import ../schema",
        "import foo/bar",
        "import foo..bar",
        "import \"schema.rdl\"",
        "import foo.",
    ] {
        assert!(parser::parse("s", text).is_err(), "accepted {text}");
    }
}

#[test]
fn memory_loading_and_the_query_only_repl_never_read_imports() {
    let mut world = World::new();
    assert!(
        world
            .load("s", "import schema")
            .unwrap_err()
            .message
            .contains("SourceLoader")
    );
    assert!(
        world
            .query("q", "import schema")
            .unwrap_err()
            .message
            .contains("query-only")
    );
}

#[test]
fn a_local_import_loads_definitions_and_facts_into_the_same_world() {
    let f = Fixture::new();
    f.write("schema.rdl", "object type race = { points : int; }");
    f.write("main.rdl", "import schema\nrace terrans = { points = 42; }");
    assert_eq!(
        f.world("main.rdl")
            .query("q", "terrans.points")
            .unwrap()
            .as_int(),
        Some(42)
    );
}

#[test]
fn transitive_imports_can_refer_to_definitions_in_the_entry_file() {
    let f = Fixture::new();
    f.write(
        "main.rdl",
        "import data\nobject type race = { points : int; }",
    );
    f.write(
        "data.rdl",
        "import defaults\nrace terrans = { points = starting_points; }",
    );
    f.write("defaults.rdl", "let starting_points = 42;");
    assert_eq!(
        f.world("main.rdl")
            .query("q", "terrans.points")
            .unwrap()
            .as_int(),
        Some(42)
    );
}

#[test]
fn dotted_names_resolve_through_configured_search_directories() {
    let f = Fixture::new();
    f.write("app/main.rdl", "import stars.defaults");
    f.write("lib/stars/defaults.rdl", "let points = 42;");
    let graph = SourceLoader::new(vec![f.path("lib")]).load_files(&[f.path("app/main.rdl")]);
    let mut world = World::new();
    graph.load_into(&mut world).unwrap();
    assert_eq!(world.query("q", "points").unwrap().as_int(), Some(42));
}

#[test]
fn entry_directories_remain_module_roots_for_transitive_dotted_names() {
    let f = Fixture::new();
    f.write("main.rdl", "import stars.data");
    f.write("stars/data.rdl", "import stars.defaults");
    f.write("stars/defaults.rdl", "let points = 42;");
    assert_eq!(
        f.world("main.rdl").query("q", "points").unwrap().as_int(),
        Some(42)
    );
}

#[test]
fn importer_local_modules_precede_other_search_paths() {
    let f = Fixture::new();
    f.write("app/main.rdl", "import defaults");
    f.write("app/defaults.rdl", "let points = 42;");
    f.write("lib/defaults.rdl", "let points = 99;");
    let graph = SourceLoader::new(vec![f.path("lib")]).load_files(&[f.path("app/main.rdl")]);
    let mut world = World::new();
    graph.load_into(&mut world).unwrap();
    assert_eq!(world.query("q", "points").unwrap().as_int(), Some(42));
}

#[test]
fn explicit_search_directories_are_tried_in_order() {
    let f = Fixture::new();
    f.write("app/main.rdl", "import defaults");
    f.write("a/defaults.rdl", "let points = 1;");
    f.write("b/defaults.rdl", "let points = 2;");
    for (paths, expected) in [
        (vec![f.path("a"), f.path("b")], 1),
        (vec![f.path("b"), f.path("a")], 2),
    ] {
        let mut world = World::new();
        SourceLoader::new(paths)
            .load_files(&[f.path("app/main.rdl")])
            .load_into(&mut world)
            .unwrap();
        assert_eq!(world.query("q", "points").unwrap().as_int(), Some(expected));
    }
}

#[test]
fn diamonds_repeated_imports_and_repeated_entries_load_each_file_once() {
    let f = Fixture::new();
    f.write("main.rdl", "import left\nimport right\nimport left");
    f.write("left.rdl", "import shared");
    f.write("right.rdl", "import shared");
    f.write("shared.rdl", "object type race = {} race r = {}");
    let graph = SourceLoader::default().load_files(&[
        f.path("main.rdl"),
        f.path("shared.rdl"),
        f.path("main.rdl"),
    ]);
    assert_eq!(graph.sources().len(), 4);
    let mut world = World::new();
    graph.load_into(&mut world).unwrap();
    assert_eq!(world.object_count(), 1);
}

#[test]
fn cycles_report_the_chain_at_the_import_that_closes_it() {
    let f = Fixture::new();
    f.write("a.rdl", "import b");
    f.write("b.rdl", "\nimport a");
    let graph = f.graph("a.rdl");
    let error = &graph.errors()[0];
    assert!(
        error.message.contains("cyclic import")
            && error.message.contains("a.rdl")
            && error.message.contains("b.rdl")
    );
    assert!(error.span.source.ends_with("b.rdl"));
    assert_eq!(error.span.line, 2);
    assert_eq!(error.span.column, 8);
    f.write("a.rdl", "import a");
    assert!(
        f.graph("a.rdl").errors()[0]
            .message
            .contains("cyclic import")
    );
}

#[test]
fn missing_imports_report_all_search_candidates_at_the_import_location() {
    let f = Fixture::new();
    f.write("main.rdl", "\nimport missing");
    let graph = SourceLoader::new(vec![f.path("lib")]).load_files(&[f.path("main.rdl")]);
    let error = &graph.errors()[0];
    assert!(error.message.contains("missing.rdl") && error.message.contains("lib"));
    assert_eq!(error.span.line, 2);
    assert_eq!(error.span.column, 8);
}

#[test]
fn a_partial_graph_retains_sources_and_multiple_import_errors_for_editors() {
    let f = Fixture::new();
    f.write("main.rdl", "import absent\nimport library\nimport missing");
    f.write("library.rdl", "let x = 1;");
    let graph = f.graph("main.rdl");
    assert_eq!(graph.errors().len(), 2);
    assert_eq!(graph.sources().len(), 2);
    assert_eq!(graph.links().len(), 1);
}

#[test]
fn import_resolution_and_validation_failures_leave_an_existing_world_unchanged() {
    let f = Fixture::new();
    let mut world = World::new();
    world.load("prior", "let prior = 7;").unwrap();
    f.write("main.rdl", "import absent\nlet next = 1;");
    assert!(f.graph("main.rdl").load_into(&mut world).is_err());
    f.write("main.rdl", "import invalid\nlet next = 1;");
    f.write("invalid.rdl", "let bad = missing;");
    assert!(f.graph("main.rdl").load_into(&mut world).is_err());
    assert_eq!(world.query("q", "prior").unwrap().as_int(), Some(7));
    assert!(world.query("q", "next").is_err());
}

#[test]
fn imported_bare_expressions_are_validated_but_only_entries_produce_output() {
    let f = Fixture::new();
    f.write("main.rdl", "import library\n42");
    f.write("library.rdl", "99");
    let values = f.graph("main.rdl").load_into(&mut World::new()).unwrap();
    assert_eq!(values.len(), 1);
    assert_eq!(values[0].as_int(), Some(42));
    f.write("library.rdl", "missing");
    assert!(f.graph("main.rdl").load_into(&mut World::new()).is_err());
}

#[test]
fn invalid_utf8_imports_are_reported_at_the_importing_source() {
    let f = Fixture::new();
    f.write("main.rdl", "import bad");
    std::fs::write(f.path("bad.rdl"), [0xff]).unwrap();
    let graph = f.graph("main.rdl");
    assert!(graph.errors()[0].span.source.ends_with("main.rdl"));
    assert!(graph.errors()[0].message.contains("cannot import 'bad'"));
}

#[test]
fn virtual_buffers_can_resolve_imports_from_a_search_path() {
    let f = Fixture::new();
    f.write("defaults.rdl", "let points = 42;");
    let graph = SourceLoader::new(vec![f.0.clone()]).resolve(
        vec![Source {
            name: "untitled:main".into(),
            text: "import defaults".into(),
            path: None,
        }],
        read_file,
    );
    assert!(graph.errors().is_empty());
    let mut world = World::new();
    graph.load_into(&mut world).unwrap();
    assert_eq!(world.query("q", "points").unwrap().as_int(), Some(42));
}

#[cfg(unix)]
#[test]
fn symlink_aliases_are_deduplicated_by_canonical_file_identity() {
    let f = Fixture::new();
    f.write("main.rdl", "import original\nimport alias");
    f.write("original.rdl", "let points = 42;");
    std::os::unix::fs::symlink(f.path("original.rdl"), f.path("alias.rdl")).unwrap();
    let graph = f.graph("main.rdl");
    assert_eq!(graph.sources().len(), 2);
    assert!(graph.errors().is_empty());
    graph.load_into(&mut World::new()).unwrap();
}

#[test]
fn imports_have_a_bounded_depth_without_stack_overflow() {
    let f = Fixture::new();
    for i in 0..70 {
        f.write(&format!("m{i}.rdl"), &format!("import m{}", i + 1));
    }
    assert!(f.graph("m0.rdl").errors()[0].message.contains("64 levels"));
}

#[test]
fn a_reader_is_called_only_once_for_each_resolved_file() {
    let f = Fixture::new();
    let calls = std::cell::Cell::new(0);
    let graph = SourceLoader::default().resolve(
        vec![Source {
            name: "main".into(),
            text: "import shared\nimport shared".into(),
            path: Some(f.path("main.rdl")),
        }],
        |path: &Path| {
            calls.set(calls.get() + 1);
            Ok(Some(Source {
                name: path.to_str().unwrap().into(),
                text: "let x = 1;".into(),
                path: Some(path.into()),
            }))
        },
    );
    assert!(graph.errors().is_empty());
    assert_eq!(calls.get(), 1);
}
