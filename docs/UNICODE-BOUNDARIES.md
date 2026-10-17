# UTF-8 application encoding

Main uses UTF-8 for all decoded application text on Windows, Linux and macOS.
CP1252 handling is removed from general product paths. The race-design importer
has a bounded exception for 2.49–2.84 race files; it converts only their names
after validating their record and checksum. A protected legacy race requires
an accepted replacement password before saving or direct New Game use. Its
resulting hash comes from that UTF-8 credential; the old hash is discarded.
Legacy game, turn, history and orders files and historical non-ASCII password
hashes remain unsupported. See [ADR 0002](adr/0002-utf8-application-text.md).
The 2.8 branch retains its historical compatibility contract.

## Contracts

| Boundary | Contract |
| --- | --- |
| Catalogs and data | UTF-8 source text becomes UTF-8 bytes in generated C/compressed tables. IDs, arithmetic ranges and part numbering remain stable. Shipped `±` and `©` retain their characters. |
| Resources | The resource source declares UTF-8. Native dialog/menu UTF-16 payloads decode strictly to UTF-8, including surrogate pairs. |
| Controls and clipboard | All controls use UTF-8. SDL text events deliver Unicode scalar values. Paste, composition and setters validate text; selection, deletion, bounded copies and wrapping respect code-point boundaries. Edit offsets/capacities remain byte counts. Case styles change ASCII only. |
| Drawing and inspection | Window and memory DCs measure/draw UTF-8 through FreeType, with bounded fallback caching. Captions, JSON inspection and PDF titles use the same text. Snapshot `fUtf8Text` remains an always-true compatibility field. |
| Files | New writes use BOF format 2.85, which identifies UTF-8. Game, turn, history and orders readers report older and newer BOF versions distinctly; malformed headers report corruption. The race-design importer alone accepts 2.49–2.84 files, parses bounded CP1252 name fields (including pre-2.55 singular-only records), verifies framing/checksum before conversion, and asks before shortening a UTF-8 name to its 31-byte field. Protected legacy races require a confirmed replacement password before save or direct New Game use; the new hash is derived from the accepted UTF-8 credential. Record layouts, widths and sizes stay unchanged; compressed payloads remain binary. |
| Passwords | UTF-8 bytes feed the original signed-byte/modulo-32-bit hash directly. ASCII hashes stay unchanged; historical non-ASCII hashes are intentionally incompatible. The password buffer accepts at most 15 UTF-8 bytes. |
| Settings | `StarsUtf8.ini` starts fresh, without old settings migration. Writes identify UTF-8 with a BOM; readers validate UTF-8, support the BOM and copy complete code points. Paths, password text, font faces, preset names and numeric preferences are retained. ASCII order prefixes remain encoded order data. The host's explicit `--ini` uses UTF-8 too. |
| Native paths | Only paths that round-trip losslessly through UTF-8 are supported. Windows converts strictly to UTF-16 at native APIs. POSIX rejects malformed UTF-8 with `EILSEQ`. No Unicode normalization or filesystem case folding changes identity. Pickers report unsupported names; CLI paths reject invalid encoding before opening. |
| Help | The canonical UTF-8 HTML/assets are in `res/help-utf8.zip`. The build stages them without an HLP parser; runtime checks the packaged guide and bundle integrity. The original HLP and its decoders are historical reference material, excluded from product deployment/build. |
| Tutorial | All three count-prefixed tutorial streams use 2.85. The original 2.60 battle event headers are converted to the current layout in place, preserving framing/encryption. `tests/savecli/cmd/utf8-tutorial` records that offline asset conversion. Loaded names/designs/fleets/battle plans in all 38 turn snapshots were verified ASCII. |

`encoding.c` owns strict UTF-8 validation, bounded copying, resource UTF-16
conversion and the race importer's CP1252-to-UTF-8 conversion. `native.c` owns
game filesystem operations. The C-compatible C++17 `filesystem.h` bridge supplies
strict UTF-8 filesystem operations for portable test fixtures, without SDK types
or C++ exceptions crossing its interface. The SDL wrapper owns rendering and
controls. No general
legacy code-page heuristic or substitution is used. Popup separator sentinels
and compressed/hash bytes are binary protocol representations, not decoded text.

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

## Future manual desktop qualification

Automated UTF-8 and profile checks run through CTest on Linux, macOS and Windows.
Manual desktop qualification is future work and does not block merge. On real
Windows desktops, open and save files with accented, CJK and supplementary
Unicode names through the native picker; check selection, cancellation,
overwrite confirmation and unchanged process directory. Type and paste the same
characters into text fields and verify the displayed and saved text.

On Linux desktops, repeat Unicode typing, paste and file selection on both X11
and Wayland. Check that native/portal picker results preserve the complete UTF-8
filename, and that editing, clipboard round trips and saved settings preserve
the same text. Record desktop environment, SDL video driver and picker backend
for each run.
