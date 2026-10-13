# Native filename and password encoding

This batch fixes filesystem and password boundaries while preserving the game's
legacy text, hash results, record layouts and file-format versions. It does not
introduce Unicode names or messages into saved games.

## Current contracts

| Boundary | Contract |
| --- | --- |
| Native paths | UTF-8 on Windows; native bytes on POSIX, normally UTF-8. No normalization or case folding is applied to filesystem identity. |
| Windows filesystem | UTF-8 paths convert strictly to UTF-16 for `_wsopen`, `_waccess`, `_wmkdir`, `_wrename`, `_wremove` and `_wfopen`. Core handles remain signed 16-bit CRT descriptors; kernel handles never enter records. Existing sharing flags remain effective. |
| Windows file chooser | `OPENFILENAMEW` selects files. Selected filenames, file titles and offsets return as UTF-8 bytes to the existing application buffers. Labels and filters convert explicitly from CP1252. Cancellation retains caller state; conversion/capacity failures return an error and the application reports them. |
| POSIX filesystem | Path bytes pass through unchanged. Native portal/Cocoa selection remains the default. |
| Fallback picker | Filename, location, directory and file-list fields use UTF-8. Typing, copy/paste, selection, deletion, undo, measurement and drawing preserve whole code points and filename bytes. Capacity and selection offsets remain byte counts. |
| Fonts | Unicode path fields use FreeType code points and Fontconfig fallback when needed. Extra glyph caching is bounded and its configuration/face resources are released. Available fonts determine coverage; complex shaping remains separate work. |
| Captions and inspection | Game and player text converts from CP1252 before being combined with a native UTF-8 filename. Abbreviations stop at complete code points and lowercase ASCII only. Snapshot text carries `fUtf8Text`; JSON remains UTF-8. |
| Race filename cache | The short cache retains its original size, ending at a complete UTF-8 boundary. UI storage retains the full suggestion, including after a direct race-file load. |
| Command-line passwords | Native command lines are UTF-8. Windows GUI/host obtain UTF-16 system arguments first. Passwords convert strictly to the existing CP1252 byte budget before hashing; malformed, unrepresentable or oversized input aborts the command with an explanation. |
| Password controls | Windows reads/writes these fields through wide APIs with explicit CP1252 conversion. SDL password input and paste reject unrepresentable text rather than substituting `?`; submission reports a rejected input attempt. |
| Stored text/password hashes | CP1252 bytes and the original signed-byte, modulo-32-bit hash remain unchanged. Arithmetic wrapping is explicit, preventing signed-overflow UB for long passwords. |

`encoding.c` supplies strict conversion and boundary helpers independently of
game state. `native.c` owns filesystem/platform operations. The UI owns native
file selection and path-field presentation. Invalid command lines return failure
from `ParseCmdLine`, so callers cannot execute partially parsed flags.

The Windows CRT descriptor table replaces the old narrow `OpenFile` boundary,
including its 128-character restriction. Tests cover filenames longer than 128
characters, Unicode open/read/write/existence/rename/delete, and missing-file
classification. This does not promise unlimited paths: application buffers and
Windows filesystem/path policies still apply.

## Validation

Focused tests cover all nonzero CP1252 bytes, malformed/overlong UTF-8, surrogates,
unrepresentable characters, output capacities and complete-prefix copying. Fixed
password expectations include `é` (`-23`) and fifteen `é` characters
(`2033583528`); their results match the existing signed-byte wrapping algorithm.
Tests exercise actual fallback-picker open/save through clipboard and typed
Unicode input, whole-code-point editing, drawing/measurement, native captions,
JSON inspection and strict password input.

Windows tests independently intercept the wide common-dialog entry points to
check UTF-16 arguments, CP1252 labels, UTF-8 results, byte offsets and cancellation.
They also check actual SDK window text, Unicode filesystem operations and Windows
argument decoding under Wine. Real Windows desktop/common-dialog execution with
different system code pages and macOS execution remain manual qualifications.

The six system-driver Wayland graphics-stack LSan failures recorded in
[desktop qualification](DESKTOP-QUALIFICATION.md) are separate from this batch.
The later [private Mesa patch](MESA-GRAPHICS-LEAK.md) repairs their executable-heap
cleanup and qualifies those cases with an explicitly selected dependency. The
original Unicode gate below excluded them and the paused tutorial tests.

## Deliberate limits and later work

- Saved race/player/planet names, messages, resource/catalog text and ordinary
  controls retain CP1252. A persisted Unicode encoding policy and full Unicode
  presentation require a separate design; records must not silently change.
- Full grapheme editing, bidirectional text, shaping and comprehensive IME layout
  are not supplied by these path-field changes. Native platform pickers provide
  their own text layout; the fallback renderer depends on available fonts.
- Invalid UTF-8 POSIX filename bytes retain file identity and display replacement
  glyphs in the fallback renderer. Distinct invalid-byte names may look alike;
  an explicit escaping/display policy remains future work.
- Local settings have historical encoding ambiguity. A migration policy for old
  INI values across different Windows code pages remains separate from native
  filesystem and picker conversion. Ordinary Windows UI text outside the password
  and native-caption boundaries still uses the existing narrow API behavior.
- CLI parsing retains its existing quoting/space limitations. The SDL command-line
  adapter retains its existing conservative filename limit. The new core parser
  rejects overflow rather than truncating a filename into a different identity.

Final qualification passes 108/108 fatal ASan/UBSan/LSan non-tutorial/non-Wayland
cases without suppressions, 55/55 Windows unit/reference tests under a private
Wine display/prefix, 15/15 Linux host units and 11/11 focused Release checks.
Windows-native and Linux-host regressions each match all 519 checkpoints, with
87 existing unused-storage warnings, no differences/errors and no skipped inputs.
Debug, sanitizer, Release and MinGW builds pass without new warnings in touched
sources. No regression baseline or saved struct/record layout changed. Tutorial
verification remains paused under the product-first scope.
