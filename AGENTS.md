# Repository Instructions

## Goal

These sources began as a reconstruction of the original Stars! 2.6jrc3 C
source, made from the debug-rich Win16 `stars.exe`. The faithful
reconstruction is finished: it is tagged `2.6jrc3` and kept on the `2.6j`
branch, which still follows the reconstruction rules in its own `AGENTS.md`
(`git show 2.6j:AGENTS.md`).

2.8 is released (tag `v2.8.0`): it fixes the original's bugs and turns the
Win16 shims into native Win32 code, while staying a game that plays like
2.6j and reads its files. The `2.8` branch takes bug fixes for 2.8 patch
releases. `main` is the 2.9 line, which splits the game into core, UI and
host builds so the host can run on Linux; see `docs/ROADMAP.md`. These
rules carry over from 2.8. The desktop client now uses SDL3 through the
Win32 wrapper on every platform; a native Win32 client is no longer maintained.
The command-line host remains independent of SDL.

When goals conflict, apply them in this order:

1. **File contract:** main uses UTF-8 throughout and writes file format 2.85.
   It imports unprotected legacy race designs from 2.49–2.84; legacy games,
   turns, history, orders and protected races remain unsupported. Record
   layouts and field sizes stay fixed. Text limits count UTF-8 bytes, and
   truncation must preserve complete code points. See
   `docs/UNICODE-BOUNDARIES.md`.
2. **Behavior:** game behavior changes only on purpose. Each intentional
   change is its own commit, moves the native regression baseline in that
   commit, and is recorded in `CHANGELOG.md`.
3. **Readability:** clear native C++. Prefer the existing structure and idiom;
   change it when that makes the code clearer or removes Win16 baggage.

## Code rules

- Names from the debug symbols (functions, params, locals, globals, struct
  fields, enums) stay. They tie the code to the original's debug symbols
  and `docs/KNOWN-BUGS.md`. New names use the same Hungarian style: `c` count,
  `i` index, `f` flag, `lp` pointer, `rg` array, `h` handle, `psz`/`sz`
  strings, `id`/`ish`/`ipl` and the like. Check `structs.h` and nearby code
  for existing prefixes before you invent one.
- Build owned production and ordinary test sources as C++20 (GNU extensions on),
  and AppKit adapters as Objective-C++20. Independent C/SDK references and
  third-party dependencies remain C. Application headers use native C++ linkage.
  Preserve explicit enum underlying widths/signedness and record layouts.
  Keep scalar locals at the top of a function and construct RAII guards at
  their operation boundary. Keep the
  surrounding style: `//` or `/* */` comments, no reformatting beyond the
  lines you change. Helpers are fine when they replace a Win16 shim or remove
  real duplication; keep them small and next to their callers.
- Truth tests stay bare for flags (`f…`), `F…()` calls, pointers, handles and
  bit tests; keep explicit `== 0`/`!= 0` for counts, indices and IDs.
- Comments explain what a reader can't see in the code: why a fix exists,
  what the original did, a file-format constraint.
- Don't mix kinds of change in one commit. Behavior-neutral cleanup (shim
  removal, warnings) must leave the regression baseline unchanged.

## Bug fixes

- Fix one bug per commit. Name the function and the original behavior in the
  commit message, add a `CHANGELOG.md` entry, and update or remove its entry
  in `docs/KNOWN-BUGS.md`, `docs/WIN16-PARITY.md` or `docs/ROADMAP.md`.
- A fix that changes turn generation changes host results; note it in
  `CHANGELOG.md`. On the 2.8 branch, fixes must also work on 2.6j state.
- Regenerate the native baseline in the same commit (see Verification) and
  state which scenarios moved and from which turn.

## Marking code

- `/* NATIVE: ... */` marks code needed only because the build is Win32 or
  64-bit (pointer size, `POINT16`, heap headers, CRT differences). As shims
  are replaced with plain Win32 code, the marker and its
  `docs/WIN16-PARITY.md` or `docs/NATIVE-PORT.md` entry go with them.
- Don't add `/* PARITY: ... */` markers on `main`. Reproducing Win16 bugs
  for the regression belongs on the `2.6j` branch.
- `#ifdef STARS_TEST_*` marks test-harness code.

## Code layout

- Sources and their headers live in `src/core`, `src/ui`, `src/host` and
  `src/platform`; the SDL Win32 implementation stays in `compat/win32`.
  Keep original filenames within these directories. `common.h`, `structs.h`
  and the game-to-UI contract `ui.h` are in `src/core`; `win.h` is in `src/ui`.

- The game code (`STARS_CORE_SOURCES` in `CMakeLists.txt`) includes only
  `common.h`, never `windows.h`, and builds without Windows headers. It
  reaches the player only through the functions in `ui.h`, which the
  SDL-backed desktop UI and `hostui.c` implement. Put dialogs, drawing and window
  state in the UI files (`*ui.c` and the others in `STARS_UI_SOURCES`,
  which include `win.h`); when a game file needs one, add a `ui.h` call.
- A file split for 2.9 keeps its game logic under its original name, and
  its window code moves to `<name>ui.c`.
- The game's text (strings, messages, tutorial, planet names) is in
  `text/*.txt`; `text/textgen.py` builds the ID enums (`textids.h`) and the
  compressed tables from it at build time. IDs never move: delete a row
  instead of renumbering, and cover rows reached by arithmetic from a named
  row with `@range`.
- Static game data (hulls, parts, starting designs, predefined and AI
  races, default battle plans, AI design recipes, part preferences and
  research orders) is in `data/**/*.yaml`; `data/datagen.py` builds its
  enums (`dataids.h`) and `data_tables.cpp`. Saved ship designs store hull
  and part numbers, so the parts files are append-only: never reorder or
  remove an entry.
- Application-owned file streams and paths use the C++20 service
  in `src/platform/fileio.h`/`fileio.cpp`. Native descriptors stay private to that
  service for exclusive creation, sharing, durable writes and live-handle operations.
  Game files, the clock and formatting go through `native.c` (`HfOpenFile`,
  `DwTickCount`, `CchSprintf`, `LMulDiv`, `chDirSep`), never Win32 or
  `wsprintf` directly.

## Things that must not change

- `structs.h` layouts that reach disk, `WriteRt`/`ReadRt` record sizes and
  on-disk formats stay at their Win16 sizes. Native pointer or handle sizes
  must never reach a save file. See `docs/NATIVE-PORT.md`.
- Element widths of fields, arrays and file records stay as they are. Don't
  widen `int16_t` to `BOOL`/`int` where storage, addresses (`&f…`) or file
  I/O depend on the width. See `docs/RECONSTRUCTION.md`, "Storage widths and
  boolean conventions".
- `qsort16` and the floating operation boundaries decide tie order and rounding
  in turn generation. The intentional migration to binary64 is documented in
  `docs/DOUBLE-PRECISION.md`; changing them is a behavior change, not a cleanup.
- The archived native tutorial observer (`tests/reference/win32-tutorial`) depends on `InitInstance`, `ScannerWndProc`, the
  globals and struct layouts it reads, and the literal `tutor.c` signatures
  `int16_t FTutorTaskDone() {` and `int16_t FCheck`. Update the observer and
  parser in the same change if any of these move.

## Versioning

- The product version comes from git: release tags are `vMAJOR.MINOR.PATCH`
  (`v2.8.0`), and builds between tags are numbered from the last tag. CMake
  generates `version.h`; see `docs/VERSIONING.md`.
- The file format version is separate: main writes and reads only 2.85
  (UTF-8). The 2.8 branch writes 2.84 and reads 2.49 through 2.84.

## Reference material

- If present, read [AGENTS-private.md](AGENTS-private.md) for additional local
  instructions. It is intentionally untracked; keep its contents and the
  materials it describes private.
- `structs.h` defines every struct. Use it to interpret offsets, bitfields and
  `LOWORD`/`HIWORD` splits.
- To see what the original did before changing it, especially for a bug
  fix, list the function from the binary with `stars-asm dasm asm|sem -n
  <Func>`; the `2.6j` branch keeps the faithful reconstruction.
- `docs/KNOWN-BUGS.md` maps reported original bugs to source.
  `docs/WIN16-PARITY.md` lists the remaining Win16 guards and boundaries.
  `docs/NATIVE-PORT.md` covers the native port. `docs/ROADMAP.md` lists the
  work left. Update them when an item is finished or found.

## Verification

After each batch of edits:

1. Build with the platform SDL Debug preset (`windows-sdl-debug`, `sdl-debug`
   or `macos-sdl-debug`). `mingw-debug` is a Windows SDL preset alias.
   Don't introduce new warnings in the files you touched.
2. Run the native regression and compare it against the native baseline in
   `tests/scaffold/fixtures/regression/native/`: `make regression` (details
   in `tests/scaffold/REGRESSION.md`). A quick check is `make
   regression-quick`, one scenario through checkpoint 10; a batch is done
   only after the full suite.
3. Run the unit tests (`make test-unit`, `tests/unit/README.md`).
4. Run the SDL tutorial checkpoints (`make tutorial`). They provide partial
   coverage; the full 80-page SDL tutorial remains unfinished.
5. For changes to the game code, also run the host regression and the
   native unit tests (`make regression-host`, `make test-host`).

Behavior-neutral changes must match the baseline exactly (unused-storage
warnings excepted). A behavior change regenerates the baseline with
`make regression-export` in the same commit. Report regression
results as they are: if a scenario diverges unexpectedly, show the first diff.

The original game's checkpoints in `fixtures/regression/original/` are the
frozen record of 2.6j behavior. The DOSBox setup that made them is gone, so
they can't be regenerated; don't change them.

Keep commits small and focused on one kind of change so that a regression
can be bisected to a single commit.

## After changes

Review the diff against these rules. In particular, check for renamed
debug-symbol names, file-format or record-size changes, behavior changes
without a baseline update and `CHANGELOG.md` entry, and stale entries in
`docs/WIN16-PARITY.md`, `docs/NATIVE-PORT.md` and `docs/ROADMAP.md`.

## Agent skills

### Issue tracker

Before issue or spec operations, read `docs/agents/issue-tracker.md`.
Work is tracked in GitHub Issues for madebyjeffrey/sdl-stars.

### Specifications

Before creating or changing specifications, user stories or acceptance criteria,
read `spec/AGENTS.md` and follow the outline in `spec/README.md`. Trace actor
goals through workflows, criteria, canonical Dafny rules and evidence; distinguish
observed behavior, proposals and open decisions.

### Triage labels

Before triage, read the default label mapping in
`docs/agents/triage-labels.md`.

### Domain docs

Before codebase exploration, read `docs/agents/domain.md`.
This repo uses a root glossary and docs/adr/ for domain decisions.
