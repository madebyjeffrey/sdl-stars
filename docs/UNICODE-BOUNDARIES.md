# UTF-8 application encoding

Main uses UTF-8 for all decoded application text on Windows, Linux and macOS.
CP1252 handling is removed from the product and its build. The new client uses
newly created files; existing game/race files and historical non-ASCII passwords
are deliberately unsupported. See [ADR 0002](adr/0002-utf8-application-text.md).
The 2.8 branch retains its historical compatibility contract.

## Contracts

| Boundary | Contract |
| --- | --- |
| Catalogs and data | UTF-8 source text becomes UTF-8 bytes in generated C/compressed tables. IDs, arithmetic ranges and part numbering remain stable. Shipped `±` and `©` retain their characters. |
| Resources | The resource source declares UTF-8. Native dialog/menu UTF-16 payloads decode strictly to UTF-8, including surrogate pairs. |
| Controls and clipboard | All controls use UTF-8. SDL text events deliver Unicode scalar values. Paste, composition and setters validate text; selection, deletion, bounded copies and wrapping respect code-point boundaries. Edit offsets/capacities remain byte counts. Case styles change ASCII only. |
| Drawing and inspection | Window and memory DCs measure/draw UTF-8 through FreeType, with bounded fallback caching. Captions, JSON inspection and PDF titles use the same text. Snapshot `fUtf8Text` remains an always-true compatibility field. |
| Files | BOF format 2.85 explicitly identifies UTF-8. Game, race, history and orders readers reject older/newer versions. Record layouts, widths and sizes stay unchanged; saved text limits count UTF-8 bytes. Oversized text cannot spill into records or split a code point. Compressed payloads remain binary, with arbitrary UTF-8 bytes escaped by the existing codec. |
| Passwords | UTF-8 bytes feed the original signed-byte/modulo-32-bit hash directly. ASCII hashes stay unchanged; historical non-ASCII hashes are intentionally incompatible. The password buffer accepts at most 15 UTF-8 bytes. |
| Settings | `StarsUtf8.ini` starts fresh, without old settings migration. Writes identify UTF-8 with a BOM; readers validate UTF-8, support the BOM and copy complete code points. Paths, password text, font faces, preset names and numeric preferences are retained. ASCII order prefixes remain encoded order data. The host's explicit `--ini` uses UTF-8 too. |
| Native paths | Only paths that round-trip losslessly through UTF-8 are supported. Windows converts strictly to UTF-16 at native APIs. POSIX rejects malformed UTF-8 with `EILSEQ`. No Unicode normalization or filesystem case folding changes identity. Pickers report unsupported names; CLI paths reject invalid encoding before opening. |
| Help | The canonical UTF-8 HTML/assets are in `res/help-utf8.zip`. The build stages them without an HLP parser; runtime checks the packaged guide and bundle integrity. The original HLP and its decoders are historical reference material, excluded from product deployment/build. |
| Tutorial | All three count-prefixed tutorial streams use 2.85. The original 2.60 battle event headers are converted to the current layout in place, preserving framing/encryption. `tests/savecli/cmd/utf8-tutorial` records that offline asset conversion. Loaded names/designs/fleets/battle plans in all 38 turn snapshots were verified ASCII. |

`encoding.c` owns strict UTF-8 validation, bounded copying and resource UTF-16
conversion. `native.c` owns filesystem operations. The SDL wrapper owns rendering
and controls. No legacy code-page heuristic, substitution or importer remains
in the product. Popup separator sentinels and compressed/hash bytes are binary
protocol representations, not decoded text.

Universe definitions are validated as complete UTF-8 text, including their
path lines, before parsing. An initial UTF-8 BOM is accepted. Embedded NULs,
malformed text and output paths beyond their existing byte capacity fail.

## Verification

The native and host regressions each compare all 519 checkpoints with the updated
native baseline. The baseline moves in all nine scenarios from checkpoint 0,
solely at BOF version 2.84 → 2.85; no simulation field moves. Both comparisons
have 21 unused-storage warnings and no differences/errors or skipped inputs.
Frozen original fixtures remain untouched; modern ASCII race fixtures are
retagged only in generated test working copies.

Focused checks cover malformed UTF-8, surrogate pairs, capacity failures,
complete-prefix copying, Unicode password bytes/hashes, 31-byte saved names,
compression round trips, ordinary/control text, clipboard/editing, rendering,
UTF-8 profiles, native paths and packaged help. The SDL tutorial walkthrough
passes through page 50 (merge cycle); this remains partial coverage of 80 pages.

Translation, grapheme editing, bidirectional text, shaping and comprehensive IME
layout remain outside this encoding migration. Glyph coverage depends on fonts.
Local execution qualifies macOS; real Windows/Linux runtime qualification must
be run on those platforms, including their native dialogs.
