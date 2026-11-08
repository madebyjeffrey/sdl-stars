# RDL syntax highlighting

This directory is a TextMate grammar bundle packaged as a declarative VS Code
extension. JetBrains supports importing this format through its
[TextMate Bundles settings](https://www.jetbrains.com/help/idea/textmate.html).
No npm installation, compilation, language server, or JavaScript execution is
needed to use the grammar.

## JetBrains setup

1. Enable **TextMate Bundles** under **Settings → Plugins → Installed** if it is
   disabled.
2. Open **Settings → Editor → TextMate Bundles**, click **+**, and select this
   entire directory, containing `package.json`:

   ```text
   /Users/jdrake/Projects/sdl-stars/beyond/rdl/editors/textmate
   ```

3. Apply the settings, then reopen a `.rdl` file. The grammar uses scope
   `source.rdl` and the IDE's existing color theme.

If `.rdl` files are associated with a manually created plain-text file type,
remove that conflicting association under **Editor → File Types**. Let the
TextMate bundle supply the file type. To keep the language server connected,
use the `*.rdl` file-name mapping and language ID `rdl` in
[LSP4IJ](../jetbrains/README.md).

The grammar colors named imports, abstract/concrete components, trait provisions
and modifiers, scalar components, cardinalities (`requires one`, `allows many`),
declarations, fields, keywords, primitive types, type
variables, literals, numbers (including `1_000_000`) and exponents, operators, builtin functions,
ordinary strings with escapes, raw triple-quoted strings, line comments, and
nested `(* ... *)` comments. An unfinished ordinary string ends at the line;
unfinished raw strings and block comments retain their scopes on following
lines. Semantic invalidity is reported separately by the language server.

With `rdl lsp` connected, semantic tokens additionally distinguish named types,
trait categories, properties, variables, declarations, and builtin symbols.
These tokens follow the current buffer and use UTF-16 positions. Exact colors
depend on the editor theme and enabled semantic-highlighting settings. The
grammar is tested with the TextMate/Oniguruma tokenizer; a live JetBrains UI
session has not been tested.

## Grammar tests

The two npm development dependencies are used only for testing the grammar with
the actual TextMate and Oniguruma engines. From this directory:

```sh
npm ci --ignore-scripts --no-audit --no-fund
npm test
```

The tests cover nested comments, raw strings, escapes, Unicode, incomplete input,
declarations, field access, type variables, and compact ranges such as `20..80`.
They also tokenize the complete `examples/races.rdl` sample.
