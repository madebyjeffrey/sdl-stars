# Beyond

Additional projects and experiments live in their own subdirectories here.

- [RDL](rdl/README.md): a Rust implementation of the typed fact language,
  validator, query REPL, and language server.
- [Trellis prototype](trellis/prototype/README.md): a standalone in-memory Rust
  model of entities, parts, attributes, read-only contexts, validation and
  explanations. Grammar and persistence are outside this prototype.
- [Society Designer prototype](society-prototype/README.md): a standalone Rust
  SCARS wizard using the existing RDL design rules and society sigils.
- `scars/`: the SCARS UI, proc-macro and native-window crates used by the
  prototype. The Cargo workspace, lockfile and build configuration live here
  alongside them, so the prototype no longer depends on a sibling checkout.
