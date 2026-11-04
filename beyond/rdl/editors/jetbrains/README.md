# RDL in JetBrains IDEs

Use the [LSP4IJ plugin](https://github.com/redhat-developer/lsp4ij). Its
[user-defined server interface](https://github.com/redhat-developer/lsp4ij/blob/main/docs/UserDefinedLanguageServer.md)
can run RDL without a dedicated IDE plugin. This directory contains an
[importable template](https://github.com/redhat-developer/lsp4ij/blob/main/docs/UserDefinedLanguageServerTemplate.md).

1. Build the executable from the RDL directory:

   ```sh
   cargo build --offline --locked
   ```

2. Install LSP4IJ using the IDE's Plugins settings. Open **Settings →
   Languages & Frameworks → Language Servers**, then add a user-defined server.
3. In the Template selector, choose **Import from custom template…**, and select
   this `editors/jetbrains` directory.
4. The template starts `rdl lsp`, or `rdl.exe lsp` on Windows. If RDL is not on
   the IDE's PATH, replace the command with the absolute path to the executable
   you just built, followed by `lsp`. Quote paths containing spaces. For this
   checkout, on macOS:

   ```text
   /Users/jdrake/Projects/sdl-stars/beyond/rdl/target/debug/rdl lsp
   ```

5. Check the mapping: `*.rdl`, language ID `rdl`. Apply the settings and open
   `examples/races.rdl`.

The server supplies diagnostics, completion, hover information, go-to-definition,
the Structure outline, and semantic highlighting. For immediate syntax coloring,
import the [RDL TextMate bundle](../textmate/README.md) through **Settings →
Editor → TextMate Bundles**. Select the `editors/textmate` directory containing
`package.json`. No npm installation is needed to use it. Let that bundle own the
`.rdl` file type and keep the LSP4IJ mapping on file-name pattern `*.rdl` with
language ID `rdl`.

The grammar colors strings, comments, keywords, numbers, and declarations while
the server adds symbol roles such as properties and type references. Protocol
and grammar behavior are covered by automated tests; this setup has not been
exercised in a live JetBrains session.

Semantic colors can be adjusted under **Editor → Color Scheme → Language
Server**. LSP4IJ's [Semantic Tokens Inspector](https://github.com/redhat-developer/lsp4ij/blob/main/docs/UserGuide.md#semantic-tokens-inspector)
shows the token types and modifiers received from RDL.

## Choose the world

By default, all `.rdl` files in the editor workspace folders form one world.
Hidden directories, symlinks, `target`, `node_modules`, `dist`, and `build` are
excluded. Open buffers override the corresponding disk files.

For separate worlds inside one IDE project, use an explicit source list in the
command:

```text
/absolute/path/to/rdl lsp /absolute/path/to/schema.rdl /absolute/path/to/facts.rdl
```

Alternatively, set LSP4IJ's initialization options:

```json
{
  "files": ["main.rdl"],
  "importPaths": ["library", "../shared-rdl"]
}
```

Initialization paths are relative to the first workspace folder; absolute paths
also work. The search path lets the source use names such as `import stars.traits`
for `library/stars/traits.rdl`. The importing and entry-file directories are
searched first, followed by workspace roots, initialization `importPaths`,
command-line `-I` paths, and `RDL_PATH`. Command-line and environment paths are
relative to the server process working directory; use initialization options or
absolute paths when launching the server from its build directory.

An explicit file list disables automatic disk discovery; named imports are
still followed transitively, including imports outside the workspace. Open RDL
buffers are still included, so keep unrelated worlds in separate IDE projects
or server sessions. Without workspace folders or a file list, only open buffers
are loaded. Save or reopen a file to refresh disk sources when the IDE does not
support dynamic file watching.

Import names support completion, hover, and go-to-file. Unsaved dependencies
participate in validation. Missing modules and import cycles are reported at
their declarations. See the [import example](../../examples/main.rdl)
and the [language reference](../../README.md#named-imports-and-search-paths).

Use the LSP4IJ console to inspect startup failures or protocol traces. The server
reserves stdout for framed LSP messages; startup errors go to stderr. It reads
source files and never changes or saves them.
