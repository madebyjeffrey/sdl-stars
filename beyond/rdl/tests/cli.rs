use std::io::{BufRead, BufReader, Write};
use std::process::{Command, Stdio};

fn command() -> Command {
    Command::new(env!("CARGO_BIN_EXE_rdl"))
}
fn example() -> String {
    format!("{}/examples/races.rdl", env!("CARGO_MANIFEST_DIR"))
}

#[test]
fn check_and_query_commands_use_real_source_files() {
    let output = command().args(["check", &example()]).output().unwrap();
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    assert_eq!(
        String::from_utf8_lossy(&output.stdout),
        "valid: 3 objects, 11 traits\n"
    );
    let output = command()
        .args(["query", &example(), "--", "terrans.habitability_points"])
        .output()
        .unwrap();
    assert!(output.status.success());
    assert_eq!(String::from_utf8_lossy(&output.stdout), "129\n");
}

#[test]
fn piped_repl_accepts_multiline_queries_and_reports_write_attempts() {
    let mut child = command()
        .args(["repl", &example()])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    child.stdin.take().unwrap().write_all(b"query {\nfrom r : race\nwhere r.primary != none\nselect r.name\n}\nlet bad = 1\n:world\n:quit\n").unwrap();
    let output = child.wait_with_output().unwrap();
    assert!(!output.status.success());
    assert_eq!(
        String::from_utf8_lossy(&output.stdout),
        "[\"Terrans\"]\n3 objects, 11 traits\n"
    );
    assert!(String::from_utf8_lossy(&output.stderr).contains("query-only"));
}

#[test]
fn invalid_command_and_missing_files_have_nonzero_status() {
    assert!(
        !command()
            .args(["check", "does-not-exist.rdl"])
            .output()
            .unwrap()
            .status
            .success()
    );
    assert!(!command().arg("unknown").output().unwrap().status.success());
}

#[test]
fn invalid_reload_preserves_the_last_valid_world() {
    let path = std::env::temp_dir().join(format!(
        "rdl-reload-{}-{}.rdl",
        std::process::id(),
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos()
    ));
    std::fs::write(
        &path,
        "object type race = { points : int; } race r = { points = 42; }",
    )
    .unwrap();
    let mut child = command()
        .arg("repl")
        .arg(&path)
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    let mut input = child.stdin.take().unwrap();
    let mut output = BufReader::new(child.stdout.take().unwrap());
    let mut errors = BufReader::new(child.stderr.take().unwrap());
    let mut line = String::new();
    input.write_all(b":world\n").unwrap();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "1 objects, 0 traits\n");
    std::fs::write(
        &path,
        "object type race = { points : int; } race r = { points = \"bad\"; }",
    )
    .unwrap();
    input.write_all(b":reload\n").unwrap();
    line.clear();
    errors.read_line(&mut line).unwrap();
    assert!(line.contains("expected int"), "{line}");
    input.write_all(b"r.points\n:quit\n").unwrap();
    line.clear();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "42\n");
    drop(input);
    assert!(!child.wait().unwrap().success());
    std::fs::remove_file(path).unwrap();
}

struct ImportFixture(std::path::PathBuf);
impl ImportFixture {
    fn new() -> Self {
        static NEXT: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
        let path = std::env::temp_dir().join(format!(
            "rdl-cli-import-{}-{}-{} 星",
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
    fn path(&self, name: &str) -> std::path::PathBuf {
        self.0.join(name)
    }
    fn write(&self, name: &str, text: &str) {
        let path = self.path(name);
        std::fs::create_dir_all(path.parent().unwrap()).unwrap();
        std::fs::write(path, text).unwrap();
    }
}
impl Drop for ImportFixture {
    fn drop(&mut self) {
        std::fs::remove_dir_all(&self.0).unwrap();
    }
}

#[test]
fn cli_import_paths_support_short_long_and_attached_forms() {
    let f = ImportFixture::new();
    f.write("app/main.rdl", "import stars.defaults");
    f.write("lib/stars/defaults.rdl", "let points = 42;");
    let directory = f.path("lib");
    let directory = directory.to_str().unwrap();
    for flags in [
        vec!["-I".into(), directory.into()],
        vec![format!("-I{directory}")],
        vec!["--import-path".into(), directory.into()],
        vec![format!("--import-path={directory}")],
    ] {
        let output = command()
            .arg("query")
            .args(flags)
            .arg(f.path("app/main.rdl"))
            .args(["--", "points"])
            .env_remove("RDL_PATH")
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}",
            String::from_utf8_lossy(&output.stderr)
        );
        assert_eq!(String::from_utf8_lossy(&output.stdout), "42\n");
    }
}

#[test]
fn cli_search_paths_precede_the_platform_separated_environment_path() {
    let f = ImportFixture::new();
    f.write("app/main.rdl", "import defaults");
    f.write("a/defaults.rdl", "let points = 1;");
    f.write("b/defaults.rdl", "let points = 2;");
    let env = std::env::join_paths([f.path("missing"), f.path("a")]).unwrap();
    let output = command()
        .arg("query")
        .arg(f.path("app/main.rdl"))
        .args(["--", "points"])
        .env("RDL_PATH", &env)
        .output()
        .unwrap();
    assert!(output.status.success());
    assert_eq!(String::from_utf8_lossy(&output.stdout), "1\n");
    let output = command()
        .arg("query")
        .arg("-I")
        .arg(f.path("b"))
        .arg(f.path("app/main.rdl"))
        .args(["--", "points"])
        .env("RDL_PATH", env)
        .output()
        .unwrap();
    assert!(output.status.success());
    assert_eq!(String::from_utf8_lossy(&output.stdout), "2\n");
}

#[test]
fn run_prints_entry_expressions_and_suppresses_imported_bare_expressions() {
    let f = ImportFixture::new();
    f.write("main.rdl", "import library\n42");
    f.write("library.rdl", "99");
    let output = command()
        .arg("run")
        .arg(f.path("main.rdl"))
        .output()
        .unwrap();
    assert!(output.status.success());
    assert_eq!(String::from_utf8_lossy(&output.stdout), "42\n");
}

#[test]
fn repl_reload_rereads_dependencies_and_keeps_the_world_after_a_failed_import() {
    let f = ImportFixture::new();
    f.write("app/main.rdl", "import defaults");
    f.write("lib/defaults.rdl", "let points = 42;");
    let mut child = command()
        .args(["repl", "-I"])
        .arg(f.path("lib"))
        .arg(f.path("app/main.rdl"))
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    let mut input = child.stdin.take().unwrap();
    let mut output = BufReader::new(child.stdout.take().unwrap());
    let mut errors = BufReader::new(child.stderr.take().unwrap());
    let mut line = String::new();
    input.write_all(b"points\n").unwrap();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "42\n");
    f.write("lib/defaults.rdl", "let points = 43;");
    input.write_all(b":reload\npoints\n").unwrap();
    line.clear();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "reloaded\n");
    line.clear();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "43\n");
    std::fs::remove_file(f.path("lib/defaults.rdl")).unwrap();
    input.write_all(b":reload\n").unwrap();
    line.clear();
    errors.read_line(&mut line).unwrap();
    assert!(line.contains("cannot find import"), "{line}");
    input.write_all(b"points\n:quit\n").unwrap();
    line.clear();
    output.read_line(&mut line).unwrap();
    assert_eq!(line, "43\n");
    drop(input);
    assert!(!child.wait().unwrap().success());
}
