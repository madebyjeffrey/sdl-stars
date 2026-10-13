# Bundled fonts

These are unmodified Liberation Fonts from the installed `fonts-liberation`
package, redistributed under SIL Open Font License 1.1. Keep `LICENSE` with
the font files and packaged application. `ORIGIN.json` records their exact
package version, sizes and SHA-256 hashes.

`compat/win32/fontgen.py` embeds the twelve faces only for SDL compatibility
builds. Their order is Sans, Serif, Mono, each regular/bold/italic/bold-italic.
Changing that order requires changing the explicit mapping in `font.c`.
Do not replace a face without updating its provenance and license notice.

Windows continues to use the GDI font mapper. See
[font implementation report](../../docs/WIN32-FONTS.md) for native mappings,
contracts, limits and verification.
