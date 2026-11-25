use std::env;
use std::io::{self, BufRead, IsTerminal, Write};
use std::path::PathBuf;
use std::process::ExitCode;

use rdl::{
    World,
    imports::{SourceLoader, environment_paths},
    parser,
};

const HELP: &str = "RDL — typed facts, validation, and queries

Usage:
  rdl check FILE...              Validate a complete world
  rdl run FILE...                Validate and print file expressions
  rdl query FILE... -- EXPR      Evaluate an expression in a validated world
  rdl repl [FILE...]             Open the query-only REPL
  rdl lsp [FILE...]              Start the stdio language server

All commands accept -I DIR (repeatable), or --import-path DIR.
Imports also search RDL_PATH, separated like the platform PATH.

REPL commands: :help, :types, :world, :reload, :cancel, :quit
Multiline input continues until complete. A blank line forces evaluation.
";

fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("{message}");
            ExitCode::FAILURE
        }
    }
}

fn load(
    paths: &[String],
    search_paths: &[PathBuf],
) -> std::result::Result<(World, Vec<rdl::Value>), String> {
    let graph = SourceLoader::new(search_paths.to_vec())
        .load_files(&paths.iter().map(PathBuf::from).collect::<Vec<_>>());
    let mut world = World::new();
    let outputs = graph
        .load_into(&mut world)
        .map_err(|error| error.to_string())?;
    Ok((world, outputs))
}

fn run() -> std::result::Result<(), String> {
    let mut args = env::args().skip(1);
    let command = args.next().unwrap_or_else(|| "repl".into());
    if command == "--help" || command == "-h" {
        print!("{HELP}");
        return Ok(());
    }
    if command == "--version" {
        println!("rdl {}", env!("CARGO_PKG_VERSION"));
        return Ok(());
    }
    if !matches!(command.as_str(), "check" | "run" | "query" | "repl" | "lsp") {
        return Err(format!("unknown command '{command}'\n\n{HELP}"));
    }
    let mut paths = Vec::new();
    let mut expression = Vec::new();
    let mut search_paths = Vec::new();
    let mut separator = false;
    while let Some(arg) = args.next() {
        if !separator && arg == "--" {
            separator = true;
        } else if separator {
            expression.push(arg);
        } else if arg == "-I" || arg == "--import-path" {
            search_paths.push(PathBuf::from(
                args.next().ok_or("-I / --import-path needs a directory")?,
            ));
        } else if let Some(path) = arg
            .strip_prefix("--import-path=")
            .or_else(|| arg.strip_prefix("-I"))
        {
            if path.is_empty() {
                return Err("import path must not be empty".into());
            }
            search_paths.push(PathBuf::from(path));
        } else if arg == "--help" || arg == "-h" {
            print!("{HELP}");
            return Ok(());
        } else if arg.starts_with('-') {
            return Err(format!("unknown option '{arg}'"));
        } else {
            paths.push(arg);
        }
    }
    if command != "query" && separator {
        return Err("only the query command accepts -- EXPR".into());
    }
    if command == "lsp" {
        #[cfg(feature = "lsp")]
        return rdl::lsp::serve_with_import_paths(
            io::stdin().lock(),
            io::stdout().lock(),
            paths,
            search_paths,
        )
        .map_err(|error| error.to_string());
        #[cfg(not(feature = "lsp"))]
        return Err("language server support requires the lsp Cargo feature".into());
    }
    if command != "repl" && paths.is_empty() {
        return Err("provide at least one source file".into());
    }
    search_paths.extend(environment_paths());
    let (world, outputs) = load(&paths, &search_paths)?;
    match command.as_str() {
        "check" => println!(
            "valid: {} objects, {} traits",
            world.object_count(),
            world.trait_count()
        ),
        "run" => {
            for value in outputs {
                println!("{value}");
            }
        }
        "query" => {
            if expression.is_empty() {
                return Err("provide a query after --".into());
            }
            println!(
                "{}",
                world
                    .query("<query>", &expression.join(" "))
                    .map_err(|error| error.to_string())?
            );
        }
        "repl" => return repl(world, &paths, &search_paths),
        _ => unreachable!(),
    }
    Ok(())
}

fn repl(
    mut world: World,
    paths: &[String],
    search_paths: &[PathBuf],
) -> std::result::Result<(), String> {
    let input = io::stdin();
    let interactive = input.is_terminal() && io::stdout().is_terminal();
    if interactive {
        println!("RDL {} — :help for commands", env!("CARGO_PKG_VERSION"));
    }
    let mut lines = input.lock().lines();
    let mut buffer = String::new();
    let mut had_error = false;
    loop {
        if interactive {
            print!(
                "{}",
                if buffer.is_empty() {
                    "rdl> "
                } else {
                    "     .. "
                }
            );
            io::stdout().flush().map_err(|error| error.to_string())?;
        }
        let Some(line) = lines.next() else {
            if !buffer.trim().is_empty() {
                evaluate(&world, &buffer, &mut had_error);
            }
            break;
        };
        let line = line.map_err(|error| error.to_string())?;
        let trimmed = line.trim();
        if trimmed.starts_with(':') {
            match trimmed {
                ":quit" | ":q" => break,
                ":help" => print!("{HELP}"),
                ":types" => {
                    for name in world.type_names() {
                        println!("{name}");
                    }
                }
                ":world" => println!(
                    "{} objects, {} traits",
                    world.object_count(),
                    world.trait_count()
                ),
                ":cancel" => buffer.clear(),
                ":reload" => match load(paths, search_paths) {
                    Ok((replacement, _)) => {
                        world = replacement;
                        buffer.clear();
                        println!("reloaded");
                    }
                    Err(error) => {
                        eprintln!("{error}");
                        had_error = true;
                    }
                },
                _ => {
                    eprintln!("unknown command '{trimmed}'; use :help");
                    had_error = true;
                }
            }
            continue;
        }
        if trimmed.is_empty() {
            if !buffer.trim().is_empty() {
                evaluate(&world, &buffer, &mut had_error);
                buffer.clear();
            }
            continue;
        }
        buffer.push_str(&line);
        buffer.push('\n');
        if parser::parse("<repl>", &buffer).is_ok() {
            evaluate(&world, &buffer, &mut had_error);
            buffer.clear();
        }
    }
    if had_error {
        Err("session contained errors".into())
    } else {
        Ok(())
    }
}

fn evaluate(world: &World, text: &str, had_error: &mut bool) {
    match world.query("<repl>", text) {
        Ok(value) => println!("{value}"),
        Err(error) => {
            eprintln!("{error}");
            *had_error = true;
        }
    }
}
