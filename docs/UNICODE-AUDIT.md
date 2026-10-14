# Stars! Unicode portability audit

## UTF-8 follow-up audit for PR #23

The follow-up audit starts from the UTF-8 migration at `27a1107` and covers
application text truncation and text-bearing file readers. Decoded application
text remains UTF-8 throughout; native UTF-16 APIs are conversion boundaries,
and compressed payloads, order prefixes and popup separator sentinels remain
binary or ASCII protocols. No disk struct, record width, text ID or part number
changes. Original regression fixtures remain frozen.

| Area | Resolution and coverage |
| --- | --- |
| Passwords | `NewPasswordDlg` uses the same 15-byte edit limit as login, race creation and CLI input. A dialog test pastes four emoji and checks both fields stop at complete code points, with room for ASCII up to the limit. |
| Names and pane titles | `PszGetFleetName` and `PszFleetNameFromWord` bound design names before appending `+`; mineral/message title clipping uses complete prefixes. A real generated-fleet test uses a 29-byte design ending in `é` across the old byte-28 cutoff. |
| Bounded wrapper text | Menu/class getters, picker file titles, dialog font faces and printer labels end at code-point boundaries. Font constructors reject malformed or unterminated face names. Tests cover short menu/class/title destinations and invalid list/menu/file inputs. |
| Formatting | `PszFormatString` bounds all name/path/list/numeric expansions to its existing 256-byte buffer. Literal truncation and the 1024-byte `CchSprintf`/`wsprintfA` output limit preserve UTF-8. Tests exercise supplementary characters across the cutoff, a full-length path and ordinary formatting. The printf APIs retain their existing caller-owned destination-capacity contract. |
| Scanner export | `FWriteScanString` no longer maps UTF-8 bytes through its remaining CP1252 lookup table. JSON preserves validated UTF-8 names, escaping only quotes, backslashes and controls; unterminated snapshot names fail. A focused snapshot test verifies accents, Euro and supplementary characters survive unchanged, plus malformed-name rejection. The scanner startup test now supplies UTF-8, and the paint test requires rejection of malformed bytes. |
| Saved names | `FReadUserString` bounds raw terminators and compressed lengths to the containing record and destination byte budget. Player, design, fleet and battle-plan readers receive the actual record extent. Tests reject missing terminators, truncated compressed input and malformed UTF-8, while accepting supplementary characters. |
| Orders | Failed fleet-name decompression is rejected before replacing the existing name; compressed bytes are never retried as raw text. Battle-plan decoding validates temporary text before changing a plan or its count. Tests verify a malformed fleet rename retains the original name and a rejected battle-plan name leaves the destination untouched. |
| Player messages | Raw terminators and UTF-8 validity, compressed escape completeness and decoded output limits are checked before retaining messages. Malformed records continue to be skipped by the existing message-reader policy. Serialization tests now use valid text/codec payloads, while explicit malformed-message cases exercise rejection. |
| File framing and definitions | BOF/game/race-checksum records require their fixed sizes; appended turns require 2.85. Universe definitions validate the entire text before parsing, accept an initial BOM, check the native file size before narrowing it, and bound output paths and copied race lines. Tests cover invalid title/later-line bytes, embedded NULs and a file beyond 64 KiB. |
| Native paths and settings | Core native file APIs, command paths, profiles and host INI parsing retain strict UTF-8 validation. Wrapper `OpenFile`, print inputs and resolved picker directories now enforce the same contract. Optional picker titles and three-byte extension abbreviations preserve complete prefixes. |

Remaining byte-oriented cuts were checked by purpose: ASCII numeric/status
fields, delimiter-based path suffix removal, English suffix removal, whitespace
trimming and binary separator construction do not split current UTF-8 text.
`WrapTextOut`/`ChopLastWord` cut at ASCII spaces; existing edit selection and
caret helpers round offsets to complete code points. Fixed byte lengths in
layout samples refer to unchanged ASCII catalog text. Translation, Unicode
normalization/case folding, grapheme editing and complex-script layout remain
outside the accepted migration, as specified in ADR 0002.

### Verification

- Platform SDL Debug build, all 17 game unit programs and all 39 macOS desktop
  checks pass, including the packaged-help and tutorial smoke checks.
- All 16 host checks pass.
- Full native and host regressions each match all 519 comparisons through turn
  150: 21 existing unused-storage warnings, no differences/errors and no skipped
  reference inputs. No scenario or checkpoint moves; native baselines remain
  unchanged from the migration.
- The Linux picker test's expected-path buffer now accommodates `PATH_MAX`
  plus its Unicode basename, addressing the GCC truncation warning that blocked
  the Linux unit/tutorial builds. The help test's preference-path stub also
  reserves room for its appended export suffix; GCC exposed that warning after
  the picker blocker was removed. Actual Linux/Windows CI results are separate
  evidence; this local run qualifies macOS and does not qualify native desktop
  interactions on other platforms. The full 80-page tutorial remains separate
  unfinished coverage.

## Historical pre-migration audit

**Historical audit:** these findings describe the pre-migration snapshot.
The UTF-8 migration supersedes its CP1252 storage/input recommendations; see
[current encoding contracts](UNICODE-BOUNDARIES.md) and
[ADR 0002](adr/0002-utf8-application-text.md). Original filenames/links below
are retained as historical evidence, not current implementation claims.

The native migration preserves the original byte-oriented text model. It does
not yet provide general Unicode support. Linux's native file picker already
opens and saves games on UTF-8 paths, but the fallback browser mixes those paths
with CP1252 controls. Windows still opens files through an API that explicitly
does not support Unicode filenames. Passwords are another immediate boundary
problem: a UTF-8 command-line password hashes differently from the same visible
CP1252 password entered in the UI.

The recommended first deliverable is reliable Unicode filenames on every
platform, with an explicit distinction between filesystem paths, display text,
and legacy game text. Unicode names and player messages need a separate storage
policy. Reinterpreting saved bytes as UTF-8 would silently change existing
games, even if every record retained its current size.

## Scope and platform status

Reviewed on October 11, 2026, against working-tree sources on `win32_adapter`,
HEAD `e7245dd`, including local modifications and the new compatibility layer.
Findings describe this checkout, not just the committed revision. This is a
portability and text audit; priorities describe implementation urgency, not
security severity. Runtime checks used Linux and SDL's dummy video backend.
Windows findings are based on source and documented API contracts; macOS
findings are based on source and filesystem contracts.

The macOS desktop migration is not complete in this checkout:
[CMakeLists.txt](/home/jdrake/Projects/stars/CMakeLists.txt:21) rejects the SDL
client on every platform except Linux. macOS host presets and a Cocoa chooser
implementation exist, but that implementation is not evidence of a qualified
macOS client. Enabling the client also requires reviewing Linux-specific test
dependencies and linker options in `cmake/sdl-client.cmake`.

Coverage includes core text and saves, text/data generation, Windows calls,
SDL input and controls, FreeType drawing, native and fallback file pickers,
passwords, command-line and universe-definition input, settings, reports,
printing, help, and inspection output. This is a broad source survey with
focused probes, not a proof that every possible input is safe.

## Encoding map

| Boundary | Current representation | Consequence |
| --- | --- | --- |
| `text/*.txt` | Printable ASCII enforced by `textgen.py` | Even accented translations are rejected. |
| `data/**/*.yaml` | UTF-8 source converted to CP1252 C literals | Source encoding does not imply Unicode runtime data. |
| Windows resources | Code page 1252 RC; UTF-16 resource strings | The SDL resource readers require CP1252 representability. |
| Core names and player messages | NUL-terminated byte strings; fixed or bounded storage | No explicit Unicode interpretation or encoding tag. |
| Native Windows UI | Narrow Win32 APIs and `char` buffers | Behavior depends on Windows code-page settings. |
| SDL UI and renderer | CP1252 bytes | One stored byte is treated as one character/glyph. |
| SDL input and clipboard | UTF-8 at SDL; converted to CP1252 | Unsupported characters become `?`. |
| POSIX filesystem | Native path bytes | UTF-8 paths work when passed through unchanged. |
| Portal and Cocoa chooser | CP1252 labels converted; paths preserved | Labels and paths have different contracts despite `char *` types. |
| Settings and plain-text reports | Unlabelled byte strings | Native UTF-8 paths and CP1252 game text can coexist. |

SDL explicitly delivers UTF-8 text events. The local code changes that encoding
before delivering `WM_CHAR`. [SDL text input contract](https://wiki.libsdl.org/SDL3/SDL_TextInputEvent).

## Current portability defects

### 1 Windows cannot reliably open Unicode paths

**High priority.**
[HfOpenFile](/home/jdrake/Projects/stars/native.c:173) uses `OpenFile`, while
existence/access checks and directory creation use `_access` and `_mkdir`.
Backup and cleanup paths additionally call narrow `rename` and `remove` in
`turn.c` and `util.c`. Native file dialogs also use narrow strings in
`mdi.c`, `createui.c`, and `raceui.c`.

Selecting or launching a game in a directory containing characters outside the
active Windows code page can lose the pathname before the core opens it.
Supporting only the picker is insufficient: load, save, backups, dumps,
existence checks, and deletion all need the same path contract.

`OpenFile` is particularly limiting: Microsoft documents that it does not
support Unicode filenames and has a 128-character path restriction.
[OpenFile contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfile).

Use explicit UTF-8-to-UTF-16 conversion at Windows filesystem boundaries and
wide APIs, with an owned handle table behind the existing small core handle
interface. Keep native handles out of saved structs. Route backup and cleanup
operations through the same native abstraction.

### 2 Windows text interpretation differs from the SDL CP1252 policy

**High priority for cross-platform interchange.** The SDL renderer explicitly
maps CP1252 in
[DwCompatAnsiChar](/home/jdrake/Projects/stars/compat/win32/src/font.c:27).
`res/stars.rc` specifies code page 1252 and `datagen.py` emits CP1252 bytes.
Native Windows narrow UI calls use Windows code-page conversion instead of an
explicit CP1252 boundary.

The same saved byte can therefore display differently, or input can produce
different saved bytes, on Windows installations using other code pages.
Multibyte Windows code pages also break the assumption that every character
occupies one byte. The original files carry no encoding metadata, so CP1252 is
a current implementation policy rather than proof of the encoding of every
historical international save.

Use wide Windows display/input APIs and explicit conversion for legacy text.
Do not try to solve this by changing the process code page alone: Microsoft
documents a separate GDI limitation for the UTF-8 process manifest setting.
[Windows code pages](https://learn.microsoft.com/en-us/windows/win32/intl/code-pages),
[UTF-8 process and GDI behavior](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page).

### 3 The fallback file browser corrupts the interpretation of native paths

**High priority, runtime confirmed.**
[FRefreshFiles](/home/jdrake/Projects/stars/compat/win32/src/filedialog.c:80)
passes `readdir` names and directory paths directly into CP1252 lists and edit
controls. Selecting a file similarly copies its native basename into the name
edit. [AcceptFile](/home/jdrake/Projects/stars/compat/win32/src/filedialog.c:205)
then reads that edit as filesystem bytes. The path edit and new-folder edit
have the same mismatch.

A runtime probe with `café-Ω` demonstrated:

| Operation | Bytes or result |
| --- | --- |
| Programmatic native filename | `63 61 66 c3 a9 2d ce a9` |
| Copy to UTF-8 clipboard | `63 61 66 c3 83 c2 a9 2d c3 8e c2 a9`, displaying `cafÃ©-Î©` |
| Delete at the start of `é` | `63 61 66 a9 2d ce a9`, an invalid UTF-8 string |
| Type `é` into an empty filename edit | `e9`, CP1252 rather than UTF-8 |

Selecting an existing entry without editing can preserve the original bytes
and still succeed. That does not make its drawing, clipboard, or editing
correct. On Linux a typed CP1252 byte may name a different byte-level file; on
macOS a malformed UTF-8 name may be refused.

Give filesystem controls a Unicode display/edit path while retaining original
path bytes for file identity. A lossy CP1252 conversion of filenames would
prevent opening names such as `東京.m1` and is not a sufficient repair.

### 4 Command-line passwords do not match UI passwords

**High priority, runtime confirmed.**
[ParseCmdLine](/home/jdrake/Projects/stars/turn.c:598) copies `-p` text directly
into `szPassLast` and calls
[LSaltFromSz](/home/jdrake/Projects/stars/utilgen.c:680).
The SDL entry and host pass native `argv` bytes through unchanged. UI password
entry produces CP1252 bytes instead.

For the visible password `é`, the actual core function returns `-23` for
CP1252 `e9` and `5307` for UTF-8 `c3 a9`. A valid password created through the
UI can consequently fail through `stars-host -p`. The 15-byte CLI cutoff can
also split UTF-8. Existing password salts depend on the original signed-byte
algorithm; changing that algorithm would break authentication for old files.

Convert command-line password text strictly to the chosen legacy encoding
before hashing, reject unrepresentable characters clearly, and keep paths in
their own encoding. Preserve the original hash for existing game files.

### 5 Unsupported input and clipboard text are silently replaced

**High priority for preventing text loss; an intentional current limitation.**
[FAnsiInput](/home/jdrake/Projects/stars/compat/win32/src/input.c:141),
[PasteEdit](/home/jdrake/Projects/stars/compat/win32/src/edit.c:183), and
[CompatEditComposition](/home/jdrake/Projects/stars/compat/win32/src/edit.c:16)
replace characters absent from CP1252 with `?`. This affects names, messages,
passwords, and the fallback filename fields. The existing input test explicitly
expects an emoji to become `?`.

Canonically equivalent input differs too: precomposed `é` is representable,
but a probe of decomposed `e` plus U+0301 produced bytes `65 3f`, or `e?`.
Clipboard and IME input can therefore lose a seemingly ordinary accent.

For legacy game fields, validate the entire proposed edit before accepting it
and explain representability failures. If canonical composition is chosen for
new UI input, apply it before conversion and test it explicitly; do not
normalize existing saved text or filesystem identities indiscriminately.

### 6 Some native filename caches still cut through multibyte characters

**Medium priority, source confirmed.**
[SetSzRaceFile](/home/jdrake/Projects/stars/raceui.c:1377) avoids a UTF-8
continuation-byte cutoff for the short SDL UI cache, but
[FWasRaceFile](/home/jdrake/Projects/stars/race.c:563) independently copies
15 bytes with `strncpy`. A successful direct race-file load can still leave
that cache ending halfway through a UTF-8 character. This is a remaining
encoding defect in a bounded copy, not the previously fixed global overrun.

The full suggestion copy in `SetSzRaceFile` and optional file-title copy in
`filedialog.c` are byte-truncated too; their boundary problem matters when the
source exceeds their destination capacity.

[CreateChildWindows](/home/jdrake/Projects/stars/mdi.c:110) retains an eight-byte
filename abbreviation and searches only for Windows separators. On POSIX it
can abbreviate the directory instead of the basename, cut UTF-8, and then pass
the result through CP1252 title conversion. The inspector and PDF metadata can
also inherit garbled path text from these titles.

Keep a full native path separately from any shortened display label. Shorten
decoded display text at a valid boundary; use `chDirSep` for basename handling.

### 7 Case comparison and character classification are inconsistent

**Medium priority, partly runtime confirmed.**
[lstrcmpiA and CRT shims](/home/jdrake/Projects/stars/compat/win32/src/services.c:44)
use `strcasecmp`/`strncasecmp`; `_strlwr` uses byte-wise `tolower`. They do not
implement a defined CP1252 case map or Unicode case folding. A runtime probe
compared CP1252 `é` and `É` as unequal, returning `32`, although the edit's
CP1252 uppercase mapping handles them. Planet prefix search in
`scan.c:2783` and MRU matching in `fileui.c:70` depend on these comparisons.

`reportui.c` sorts names with `strcmp`; the fallback browser uses
`SDL_strcasecmp` and a byte-oriented wildcard matcher, where `?` consumes one
byte. These have no unified normalization, collation, or character-unit policy.
MRU's unconditional case-insensitive matching also conflates distinct paths on
case-sensitive filesystems. That is a separate identity issue from display
sorting.

There is also an existing C portability hazard:
[ReadRtPlr](/home/jdrake/Projects/stars/file.c:124) passes a possibly negative
signed `char` to `isupper`. `isdigit` has similar unchecked arguments in
`scan.c:2813`, `utilgenui.c:36`, `battleui.c:312`, and `buildui.c:1791`.
Such calls require an unsigned-byte value or EOF; a cast repairs the C contract
but does not make the operation Unicode-aware.

Define separate helpers for ASCII protocol tokens, legacy CP1252 text,
Unicode display search, and filesystem identity. Keep UI collation changes
away from simulation ordering and `qsort16`. Locale changes alone are not a
text migration.

### 8 Settings and universe definitions have ambiguous encoding

**Medium priority.**
[profile.c](/home/jdrake/Projects/stars/compat/win32/src/profile.c:63) reads and
writes raw bytes. Values include native paths, legacy passwords, saved
templates, and font names. String returns truncate by byte count. The parser
does not recognize a UTF-8 BOM or decode UTF-16 settings; a BOM before the first
section prevents the ordinary section parse. Windows profile APIs introduce a
different encoding boundary on the native Windows build.

[GenNewGameFromFile](/home/jdrake/Projects/stars/create.c:965) likewise treats a
universe definition as raw bytes. It copies the first line directly into
`game.szName` after a 31-byte check, while other lines identify native race
paths. A normal UTF-8 editor can produce correct paths but a game title that
the CP1252 UI garbles. A BOM would also enter that first-line title.

Specify text-file encodings and migration behavior per format. Paths and
passwords need explicit conversion on ingestion. Preserve legacy definitions
through an explicit legacy-reading mode rather than guessing solely from
whether their bytes form valid UTF-8.

### 9 Reports and host diagnostics emit legacy bytes without conversion

**Medium priority.**
[DumpPlanets and DumpFleets](/home/jdrake/Projects/stars/report.c:311) write
formatted strings through `RgToStream` without encoding conversion.
`FRunCmdLine` writes game and player names to host logs, and
[IdAlertBox](/home/jdrake/Projects/stars/host.c:15) sends strings directly to
`stderr`. Accented CP1252 names therefore yield non-UTF-8 output on an ordinary
UTF-8 terminal or text consumer. Diagnostics that include a native pathname can
mix UTF-8 and CP1252 in one string.

Retain any existing machine-consumed report format deliberately. Provide an
explicit UTF-8 output mode or a documented export boundary, and convert
diagnostics according to each interpolated string's domain. A whole-string
conversion after mixing domains cannot repair the ambiguity.

## Blockers to general Unicode text

### 10 Saved text has no Unicode contract

**Architectural blocker.** The game, player, hull/design, battle-plan, and
other name fields retain fixed byte storage. For example,
[GAME](/home/jdrake/Projects/stars/structs.h:292) has `szName[32]` and is written
as a 64-byte `rtGame` record in `create.c:909`.
[PLAYER](/home/jdrake/Projects/stars/structs.h:1581) has two 32-byte names.
[WriteRtPlr](/home/jdrake/Projects/stars/save.c:22), `WriteRtShDef`, and
[WriteRtString](/home/jdrake/Projects/stars/save.c:868) serialize byte strings
with compression or a raw fallback. Player messages use the same compressor
and legacy record length fields.

The compressor itself preserves arbitrary nonzero bytes: a core probe
round-tripped bytes 1 through 255, compressed into 340 bytes. Thus UTF-8 bytes
could pass mechanically, subject to capacity. Readers still need to know how
to interpret them. CP1252 bytes `c3 a9` mean `Ã©`, while UTF-8 bytes `c3 a9`
mean `é`; validity-based auto-detection cannot distinguish those meanings.
Already-written format-2.84 files do not become UTF-8 just because they are
newer than the original game.

Under the repository's unchanged-format requirements, preserve legacy stored
text and convert it for display. Full Unicode persisted names/messages need a
separately specified compatibility mechanism with unambiguous identification
and host/client round-trip rules. Widening `char` fields to `wchar_t` would
change layouts and record sizes and violates the current rules. A sidecar is
also incomplete unless moving games and hosting turns preserves its data.

### 11 Edit controls assume one byte per character

**Architectural blocker; also exercised by finding 3.**
[edit.c](/home/jdrake/Projects/stars/compat/win32/src/edit.c:37) uses byte counts
for length, caret, selection, hit testing, deletion, wrapping, undo, and limits.
`WM_CHAR` accepts only values through 255 and appends one byte. `WM_GETTEXT`
and combo returns can truncate inside a multibyte sequence.

Replacing CP1252 input conversion with UTF-8 insertion alone would make arrows
and Delete split encoded characters. Code-point navigation alone would still
split combining sequences and emoji sequences. Unicode editing needs grapheme
boundaries and a mapping between storage offsets, shaped text clusters, and
the public control-message indexing convention.
[Unicode text segmentation](https://www.unicode.org/reports/tr29/).

Decide explicitly whether compatibility `A` messages keep legacy semantics or
whether new Unicode controls expose another interface. Changing the meaning of
every `cch`/`ich` silently would break application and test callers.

### 12 Drawing is limited to 256 byte-indexed glyphs

**Architectural blocker.**
[font.c](/home/jdrake/Projects/stars/compat/win32/src/font.c:21) caches 256
glyphs, maps each input byte through CP1252, and measures/draws one glyph at a
time from one face. UTF-8 strings are drawn as several CP1252 characters.
There is no shaped-run pipeline, per-run fallback, or bidirectional layout.
`paint.c:1635` and edit layout additionally measure individual bytes for
wrapping, caret placement, and mnemonic underlines.

Accepting Unicode scalar values is only the first step. Arabic/Indic scripts,
combining marks, mixed right-to-left text, and emoji require suitable shaping,
fallback, segmentation, and rendering policy. FreeType documents that a
one-character-to-one-glyph mapping is insufficient for many scripts.
[FreeType character mapping](https://freetype.org/freetype2/docs/reference/ft2-character_mapping.html).

Use one measured/shaped text layout result for drawing, wrapping, selection,
and hit testing. Qualify fallback fonts and map labels, rotated labels,
messages, menus, and printed output. The current fonts are adequate evidence
for the existing CP1252 path, not universal character coverage.

### 13 The text and data generators reject broader characters

**Build-time blocker, runtime probes confirmed.**
[ParseTable](/home/jdrake/Projects/stars/text/textgen.py:81) opens text sources
as ASCII and permits only printable ASCII. A temporary UTF-8 row containing
`Café` raised `UnicodeDecodeError`.
[CString](/home/jdrake/Projects/stars/data/datagen.py:129) accepts `Café` and
emits `"Caf\351"`, but rejects `東京` as not Windows-1252.

The static compressed text tables also have a byte lookup alphabet, a maximum
255-nibble string length, 16-bit chunk offsets, and small decode caches.
Relaxing only the source-file encoding does not remove these constraints.
Unicode code points cannot simply be emitted into the current byte lookup
array. A UTF-8 byte alphabet is a different generation design and still needs
output capacity checks.

Keep stable text IDs and arithmetic ranges. Presentation translations can use
a separate Unicode catalog without changing disk IDs. Static names that reach
saved design/race storage must still respect the legacy storage policy.

### 14 UTF-16 resources are deliberately narrowed back to CP1252

**Build/runtime blocker.**
[FDlgField](/home/jdrake/Projects/stars/compat/win32/src/dialog_template.c:25)
and [menu decoding](/home/jdrake/Projects/stars/compat/win32/src/resource.c:94)
read 16-bit resource units and require each to map to a CP1252 byte. A
nonrepresentable title fails decoding; supplementary characters represented by
surrogate pairs are not decoded as pairs.
[uigen.py](/home/jdrake/Projects/stars/compat/win32/uigen.py:51) interprets RC
escapes as CP1252 and enforces CP1252 menu representability. Native resources
being UTF-16 does not make the end-to-end UI Unicode-capable.

The compatibility header expressly rejects `UNICODE` and `_UNICODE` in
`compat/win32/include/windows.h:4`. Its active API aliases and callers use
narrow structures; the separate upstream reference code is not compiled.
Defining `UNICODE` globally is not a supported migration.

Decode complete Unicode resource strings into a Unicode UI representation,
including surrogate validation, once controls and rendering can consume it.
Keep resource IDs, dialog procedures, and the tutorial observer contracts
stable or update their consumers together.

### 15 Byte capacities must remain separate from visible character limits

**High priority prerequisite for accepting more text.** Name edits currently
use limits such as 15 or 31 in `raceui.c`, `ship2ui.c`, `createui.c`, and
`utilgenui.c`; message entry uses 968 in `msgui.c`. Under the current SDL
encoding those are byte budgets. A 31-byte UTF-8 name budget holds at most ten
three-byte CJK characters or seven four-byte scalar values, possibly fewer
visible graphemes for sequences.

Increasing those limits as if they were character counts would outrun fixed
destinations. `WriteRtString` has a 33-byte stack buffer and an unchecked raw
`strcpy` fallback. Paths have 256-byte buffers in the UI/core, a separate
80-byte argument limit in the SDL adapter, and the Windows `OpenFile` limit.
Native picker `nMaxFile` checks are useful but do not cover every later
formatted derivative or command-line path.

There are existing formatting capacity hazards too:
[CchSprintf](/home/jdrake/Projects/stars/native.c:101) always supplies 1024 to
`vsnprintf`, irrespective of the actual destination;
[PszFormatString](/home/jdrake/Projects/stars/msg.c:261) builds into
`szMsgBuf[256]` without a capacity argument and interpolates names and
`szBase`. Long paths can exhaust that buffer already; translated strings and
UTF-8 expansion increase the exposure. This survey identified the unchecked
paths, not a new sanitizer-confirmed overflow exploit.

Track byte capacity and display length independently. Add capacity-aware
formatting for variable text, validate before copying into legacy fields, and
truncate only at valid boundaries where truncation is appropriate. Do not
widen disk structs or change compression records as a buffer repair.

### 16 IME composition needs more than committed UTF-8 input

**Unicode input blocker.** The current input route handles SDL composition
events, but stores their preview as CP1252 with replacement. It draws a local
composition underline; this is useful partial support. The code does not call
`SDL_SetTextInputArea`, so it does not provide the focused child edit's caret
rectangle to place the native IME candidate interface. Candidate positioning
and lifecycle need qualification on actual desktop backends.

Keep preedit Unicode separate from committed legacy text, commit atomically,
cancel it on appropriate focus transitions, and update the text-input area as
the caret or window moves. SDL provides that area specifically for the active
text input location.
[SDL text input area](https://wiki.libsdl.org/SDL3/SDL_SetTextInputArea).

### 17 Native service strings and instrumentation need encoding contracts

**Medium priority.**
[PszNativeTitle](/home/jdrake/Projects/stars/compat/win32/src/window.c:22) and
[inspection JSON](/home/jdrake/Projects/stars/compat/win32/src/inspect.c:155)
correctly convert CP1252 to UTF-8 when their inputs are CP1252. They double
encode native UTF-8 paths if those have been inserted into window text.
Tests relying on hierarchy JSON can consequently observe misleading names.

`font.c` passes a narrow face name directly to Fontconfig; non-ASCII face names
from legacy settings have no explicit conversion. `print.c:86` accepts native
printer names, `print.c:96` truncates them into narrow `DEVMODEA` storage, and
`print.c:146` interprets the document title as CP1252 for PDF metadata. Native
UTF-8 and legacy text need separate handling in those services too.

Help is a useful example of an explicit boundary: `hlp_font.py` decodes legacy
text, `hlp_render.py` emits UTF-8 HTML, and `help.c:73` percent-encodes path
bytes in a file URI. Preserve that separation rather than applying a new
global conversion to already converted help content. Printing currently
embeds rasterized pages, so Unicode rendering would also need to reach the
printer's drawing pipeline.

### 18 Localization changes require semantic and layout review

**Additional scope for multilingual support.**
[PszPlayerName](/home/jdrake/Projects/stars/util.c:344) assembles English
articles, suffix plurals, and verbs. `msg.c` formats custom backslash tokens;
other code uses printf-style formats and arithmetic text-ID ranges. Menus and
controls use ampersand mnemonics, ASCII key comparisons, fixed dialog geometry,
and English word-break assumptions. `WrapTextOut` and `ChopLastWord` in
`utilgenui.c` break at literal spaces.

Unicode transport alone does not provide translated grammar, word boundaries,
right-to-left layout, mnemonic handling, or room for longer text. Preserve
format-token contracts and stable IDs, validate translations at generation
time, and qualify layout separately. Any intentional changes to generated
names or other host-visible behavior need their own baseline and changelog
entry under the repository rules.

## macOS and filesystem identity

The Cocoa bridge converts labels from CP1252 to UTF-8 and obtains selected
paths from `NSURL.fileSystemRepresentation`. That is an appropriate boundary
to preserve. `NSString stringWithUTF8String:` for the initial folder/name also
requires valid UTF-8, so an already-truncated or CP1252 filename suggestion can
fail before a picker appears.

File identity should follow the filesystem rather than a single portable
lowercase/normalization function. APFS and HFS+ have different storage
normalization behavior; Linux can expose distinct byte names for canonically
equivalent strings. Apple's documented APFS behavior preserves filename
normalization and performs normalization-insensitive lookup, unlike HFS+'s
normalized storage. Keep native path representations for access, and treat
display normalization and MRU identity as separate decisions.
[Apple filesystem filename behavior](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/APFS_Guide/FAQ/FAQ.html).

Also define a Linux policy for filenames containing invalid UTF-8. They can
still be legitimate native path bytes. A Unicode-only display should escape
or otherwise identify them without silently changing which file is opened.

## Implementation sequence

1. **Specify the boundaries.** Document UTF-8 display text, native paths, and
   legacy stored text separately. Preserve arbitrary path bytes where required
   on POSIX. Use strict conversion results that distinguish malformed input,
   unrepresentable input, and insufficient output capacity.
2. **Fix current path and password defects.** Replace Windows narrow file
   boundaries consistently; give the fallback browser Unicode path controls;
   fix race/title abbreviation boundaries; convert native CLI passwords to
   legacy bytes. Keep the simulation and existing saved text interpretation.
3. **Make variable-text operations bounded.** Audit caller capacities for
   formatting, decoded strings, caches, exports, and derivatives of long
   paths. Retain byte budgets in legacy storage.
4. **Extend presentation.** Add Unicode resource/catalog support and a text
   layout implementation shared by rendering and editing, with grapheme
   handling, shaping, fallback, IME placement, and explicit Windows wide calls.
5. **Decide persisted Unicode separately.** Establish how files identify their
   encoding and how all hosts/clients preserve it. This requires reconciling
   the unchanged-format rule before implementation. Unicode filenames and
   translated presentation can be delivered independently.

Each bug fix should be a focused commit. Preserve debug-symbol names and
disk widths. Follow the repository's full Windows build, regression, unit,
tutorial, host regression, and native unit checks for implementation changes;
update baselines only for intentional behavior changes. Original regression
fixtures remain frozen.

## Validation and missing coverage

Rebuilt the focused SDL compatibility targets and ran ten existing tests:
`win32_font`, `win32_file`, `win32_profile`, `win32_services`, `win32_input`,
`win32_edit`, `win32_ui_resources`, `win32_filedialog`, `win32_fileportal`, and
`win32_fileworkflow`. All passed. The two portal tests initially could not
create their private D-Bus sockets in the sandbox; both passed after rerunning
with that local socket permission. This was an environment restriction, not
an application test failure.

The portal workflow uses actual copied native regression games under
`étoiles-Ω/café-Ω.*`, plus long Unicode race basenames, and exercises
open/save/cancel/reload. This verifies an existing working Unicode-path route.
It does not verify Unicode in game names/messages or fallback browser editing.

Temporary probes against the actual core functions and rebuilt compatibility
library confirmed password-byte mismatch, all-byte compression round-trip,
fallback edit/clipboard corruption, CP1252 case comparison, and decomposed
accent replacement. Generator probes confirmed ASCII/CP1252 source limits.
Probe sources are in `/tmp/stars-unicode-audit/`. No production code, save
records, or regression baselines were changed. The roadmap adds a link and
open follow-up items; pre-existing working-tree edits are preserved. Full
gameplay regression was not run for this documentation-only audit.

Future acceptance tests should cover:

| Area | Required cases |
| --- | --- |
| Filesystem operations | `café`, `Ω`, `東京`, supplementary characters, spaces, non-ASCII home/install/config directories, long paths, backup/restore/rename/delete/export |
| Filesystem identity | NFC/NFD pairs, case-sensitive distinct names, case-insensitive volumes, symlinks, invalid UTF-8 POSIX names, Windows reserved names |
| Fallback browser | Display, navigation, typing, paste, copy, selection, one-character deletion, new folder, save, reopening the exact path |
| Legacy compatibility | Original and 2.84 files; CP1252 high bytes including Euro/smart punctuation; byte-identical save and compression behavior |
| Passwords | Matching UI/host credentials for representable accents and Euro; explicit rejection outside legacy encoding; byte limits; old salts unchanged |
| Unicode UI | Combining accents, grapheme emoji sequences, CJK, Arabic/Hebrew mixed with numbers, Indic scripts, glyph fallback, wrapping, rotated/printed labels |
| Input methods | Dead keys; actual IME preedit/commit/cancel; caret candidate positioning; focus changes; paste at capacity; undo/redo |
| External text | Settings BOM policy and migration; universe definitions; terminal diagnostics; reports; font/printer names; hierarchy JSON; help URIs |
| Platforms | Real Windows with CP1252, another ACP, and UTF-8 system settings; Linux X11/Wayland; macOS once the desktop build is enabled |

Passing ASCII regression or the current CP1252 contract tests does not establish
these Unicode capabilities. Preserve those tests and add boundary-specific
cases for each implementation step.
