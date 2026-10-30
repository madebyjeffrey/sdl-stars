# Native port

The SDL client now has a native Windows x64 build using the same source wrapper
as Linux/macOS, and is the only maintained desktop build. Its filesystem service is isolated in
`src/platform/fileio.cpp`, and its wrapper symbols are namespaced
to avoid SDK import collisions. Native Unicode open/save lives in the separate
`filedialog_windows_sdk.c` bridge. This does not change disk layouts or core
native I/O. The shared UI has no native-client fallback branches; captions and
legacy input use the wrapper encoding boundary. See [Windows SDL3](WINDOWS-SDL3.md) for builds and qualification.

Desktop game creation uses a managed library under SDL's per-user preference
root: `~/Library/Application Support/Stars!/Games` on macOS,
`$XDG_DATA_HOME/Stars!/Games` (default `~/.local/share/Stars!/Games`) on Linux,
and `%APPDATA%/Stars!/Games` on Windows. `STARS_LIBRARY_DIR` overrides the parent
of `Games`; isolated workflow tests use this override. When only
`STARS_SETTINGS_DIR` is set, library storage follows that isolated settings root.
Each game folder contains its existing file family plus a `.library` metadata
file with a schema version, exchange basename, remembered player and last-played
Unix time. Generated files must have complete record framing before the entry
is committed. Metadata is published by replacing a flushed temporary file through
`ReplaceNativeFile`; Windows uses `MoveFileExW` so failed replacement preserves
previous metadata. The command-line host never resolves or requires this library.

The desktop client uses the SDL-backed Win32 wrapper on every supported platform.
The shared game also builds as `stars-host`, independently of SDL. Owned production
and ordinary test translation units compile as GNU C++20 under their original
source filenames; AppKit glue compiles as Objective-C++20. Independent C storage
fixtures, real SDK contract tests and third-party libraries remain separate.

Application headers expose native C++ declarations. The platform file and
filesystem interfaces retain their buffer/handle shapes and error contracts.
Inputs and outputs are strict UTF-8; `path_utf8.h` copies application bytes into
C++20 UTF-8 code units and copies returned code units back to char storage.
File-service exceptions are contained to preserve the existing failure receipts.
Output buffers are caller-owned, must not overlap input paths, and remain empty
on failure. Canonical paths use forward slashes. Temporary directories are
claimed atomically. Native adapters still supply exclusive creation, sharing,
exact private modes, durable writes and live-handle identity/truncation.
CMake preserves signed `char`, binary64 operation boundaries and contraction/
fast-math settings. GNU storage extensions remain until milestone B.

Stored enums have explicit C++ underlying types (`uint16_t`, `int16_t` or
`uint32_t`) with their original constant and type names. Text/data generators
emit native C++ declarations and `constinit` catalogs. Historical integer/wire
inputs are frozen in `tests/reference/cpp-a`; 145 enum storage contracts and
259 layout/prefix measurements remain checked independently. The application
has no parallel C declaration branch. `STARS_CONSTANTS` is removed.
Integer fields, indexes, masked identifiers and
arithmetic cross into enum types through explicit casts. Partial switches retain
explicit no-op defaults; splitter masks switch on their underlying integer.
`test_enum_abi` requires real C++ enums with the captured underlying type and
checks C/C++ argument/return values, callbacks and pointer writes for all 145
stored types using six boundary inputs each. The fixed underlying type admits
all of its storage bit patterns, including legacy sentinels and combined flags.
`test_record_layout` retains all 259 frozen C size/prefix measurements.
Codecs now separate migrated file bytes from runtime storage: six task/order
types retain their widths while requiring explicitly checked two-byte runtime
alignment. All remaining alignment comparisons stay exact. `RTFLEETIDS` is an aligned call-scoped byte-span view; its `rgid`
offset still matches the frozen zero-prefix measurement. The wire prefix is
explicitly zero, independently of the runtime view size. The full native and host regressions check serialized results.

Memory and stream failures use `StarsError` and normal C++ stack unwinding.
The 24 former jump-recovery sites now have operation-specific catches; the
`penvMem` global and jump-buffer alias are removed. `StarsRecoveryScope` records
only whether optional record validation is inside a recoverable operation, so
standalone validators retain their existing status-return behavior. It holds
no destination address. Nested scopes restore that state automatically.
`StarsRestore` and `StarsFileGuard` provide non-throwing cleanup; resource
release does not substitute for checked writes, publication or game-state
rollback.

The C SDL runtime calls application window/dialog/timer/inspector callbacks
through C++ containment thunks. A thrown failure becomes pending state, an
active modal dialog ends, and the C dispatcher finishes its counters, window
and owner cleanup. Typed wrappers around callback-dispatching UI APIs deliver
that pending failure only after the C call returns. Native file-picker pumps
cancel on a pending failure too. Desktop and host entrypoints contain remaining
failures; no C++ exception is allowed to escape through the C entrypoint.
The callback fault test exercises nested dialogs, owner re-enabling, allocation
exceptions, timers, synchronous delivery and automatic destruction.

Each native-port shim is marked `NATIVE` in the source and described in
detail in [WIN16-PARITY.md](WIN16-PARITY.md).

- **Shims:** the `NATIVE` changes cover malformed player-message records,
  battle heap capacity, and two original uninitialized reads. Window
  procedures handle the Win32 `WM_CTLCOLORMSGBOX`…`WM_CTLCOLORSTATIC`
  messages directly where the original handled Win16 `WM_CTLCOLOR`.
- **Struct sizes:** `structs.h` layouts and `WriteRt`/`ReadRt` record sizes are
  the file format. 17 structs and `RECT` grow under Win64 (HB, MSGPLR, OBJ,
  PART, PLANET, FLEET, BTN, BTNT, DRAWCIR, RPT, SBAR, SEL, TILE, WN, INI,
  XFER, TUTOR). FLEET and PLANET records pack their fields explicitly, partial
  copies stop before the pointers, and other records keep their Win16 sizes.
  Native pointer or handle sizes never reach a save file. UI-only `TILE` records
  use native callback alignment and array strides; no record I/O uses this type.
- **Storage widths:** see
  [RECONSTRUCTION.md](RECONSTRUCTION.md#storage-widths-and-boolean-conventions).
  Don't widen `int16_t` to `BOOL`/`int` where storage, addresses or file I/O
  depend on the width.
- **POINT16:** Stars' `POINT` is `POINT16`. Win32 calls use the native `POINT`,
  converted through `PointFrom16`/`PointTo16`. `ShowScanSelChange` and
  `DrawBuildSelComp` pass 32-bit `RECT` fields through `POINT16`/`int16_t`
  locals (marked `NATIVE`). Keep this split when adding Win32 calls.
  `POINT16` is in `native.h`; its conversions (`GetCursorPos16`,
  `ScreenToClient16` and the like) are in `nativeui.h`.
- **Other helpers** (`nativeui.c`): `GetTextExtent` keeps Win16's packed
  width/height result, which about 140 callers split with `LOWORD`/`HIWORD`;
  `FrameWndProcDeferred` posts the frame's restore and maximize commands back
  to the message loop so Wine's macOS driver can't deadlock the
  load-or-unsubmit `MessageBox`.
- **Platform layer** (`native.h`/`native.c`): the game code reaches files,
  the clock and directories only through `HfOpenFile`, `CbReadFile`,
  `CbWriteFile`, `LSeekFile`, `CbFileSize`, `CloseFile`, `FFileExists`,
  `FFileReadOnly`, `MakeDir`, `DwTickCount` and `GetDateTimeSz`, and builds
  paths with `chDirSep`. The adapters forward to `fileio.h`/`fileio.cpp`, shared
  with profiles, help, PDF output, inspection and managed-library operations.
  `std::fstream` owns ordinary binary streams and `std::filesystem` owns paths,
  directory enumeration and publication. C-compatible opaque streams contain
  exceptions and expose errors through return values and `errno`. Small owned
  tokens retain the game's `int16_t` handle storage.
  C++20 cannot express exclusive creation, exact private modes, sharing,
  descriptor identity, fsync or live-handle truncation; narrow native adapters
  preserve those contracts inside the service. Windows converts UTF-8 paths
  through `u8path` to UTF-16. POSIX retains its existing lack of share modes.
  Temporary publication checks flush, sync and close before replacing a path.
  Raw game writes flush before returning to preserve immediate error reporting
  through the legacy void close interface. Process logging and third-party
  libraries' internally owned files remain outside this service. The game's
  heaps (`memory.c`) retain their `calloc`/`realloc` allocation geometry under an
  explicit chain owner and C++20 typed lifetime operations.
- **Formatting and rounding** (`native.c`): the game code formats with
  `CchSprintf`, which reads `%ld` as 32 bits like Win32 `wsprintf` (the
  string table passes `int32_t` to `%ld`; `long` is 64 bits on 64-bit POSIX)
  and, like `wsprintf`, writes at most 1024 bytes,
  and rounds with `LMulDiv`, which matches Win32 `MulDiv`. Every format in
  the string table formats the same through both.
- **Platforms:** the game code (`common.h`) builds without Windows headers;
  `stars-host` links it alone. Both host and client use binary64 `double`,
  with contraction and fast-math disabled. The migration's intentional turn
  changes and platform qualification limits are in
  [DOUBLE-PRECISION.md](DOUBLE-PRECISION.md).
  The SDL wrapper uses the C library's null and integer-limit definitions.
  Its remaining `FARPROC` instances are dialog callbacks and use the explicit
  `DLGPROC` signature. UI-only `TILE` tables now use native callback-pointer
  alignment instead of the surrounding byte packing. A compile-time check guards pointer placement and
  alignment; macOS uses normal chained fixups without a linker workaround or
  pointer-alignment warnings. Persisted structures keep their layouts.
  Startup prepares the main frame's initial show command while it stays hidden,
  so native window animations cannot expose it before the title screen.
  New-game setup treats aborted modal dialogs as cancellation, and the wrapper
  leaves a pending quit message for the outer loop without opening another page.
  Modal dialogs request native input focus after painting and showing, then
  assign logical control focus; no-activate windows are excluded.
- **Toolchain parity (keep):** `qsort16` (`native.c`) reproduces the
  Win16 CRT's tie order. The remaining casts and operation boundaries are
  deliberate, so don't simplify them without checking turn results.
- **Warnings kept** (`-Wall -Wextra -Wno-unused-parameter`, 118):
  - **Unused-but-set (79):** debug-info locals the original also stores to,
    probably for asserts or debug output that was compiled out.
  - **Sign-compare (23):** casts such as `(uint32_t)(dx * dx)` record the
    original's unsigned arithmetic. Two are `turn.c`'s `min(...)` of a
    signed and an unsigned value, reported since `min` moved from
    `windows.h` to `native.h`.
  - **Type-limits (14):** enum range checks on unsigned fields.
  - **Tautological compare (1):** `aiutil.c` `IroEnsureAi`
    `(iTechCur & 0xf) == 0x1a` is an original dead branch.
  - **Function cast (1):** `shipui.c` `TransferStuff` casts
    `FEnumCalcJettison`. Both signatures come from the debug info and are
    ABI-compatible.
- **Original uninitialized reads left as is** (harmless): `ScoreXDlg` and
  `VCRDlg` call `EndDialog(hwnd, i)` with `i` unset, and `CreateChildWindows`
  creates the mine window with an unset `pt` as its size.

## Race-bit masks and sanitizer qualification

`ReadIniTileSettings` normalizes the C library's `isupper` mask before writing
the one-bit panel expansion flag. Field widths and settings syntax remain
unchanged. `FOpenGame` uses `chDirSep` and guards a missing separator; UI-loading
tests exercise relative and absolute Linux filenames. Modeless Technology
Browser teardown now uses `DestroyWindow` instead of `EndDialog`.

Native file-workflow tests open games and save/reload races through a private
FileChooser portal using UTF-8 directories and basenames. `FSaveRace` and
race-only `FOpenGame` now bound copies into the existing `szRaceFile[16]`
suggestion cache through `SetSzRaceFile`; a long native basename previously
overwrote adjacent globals. Separate UI storage retains the full suggestion for
repeat saving, and every native cache boundary preserves complete UTF-8 characters.
Direct `FWasRaceFile` loads also use that UI/host setter.
The selected file path is not truncated, and no saved record changes.

`GetRaceGrbit` and `SetRaceGrbit` now construct masks from `UINT32_C(1)` so
bit 31 has defined unsigned-shift semantics. The 32-bit field and on-disk
representation remain unchanged. `test_RaceGrbit_high_bit` checks independent
setting, reading and clearing of bits 0 and 31.

Ordinary heap payloads now satisfy `max_align_t`; header padding and allocation
strides preserve alignment through reuse, reset and growth. Battle heaps retain
two-byte boundaries because their record traversal depends on them. Heap
counters and persisted structures retain their original widths.
The subsequent sanitizer passes exposed host-sentinel reads in `SetVisPFInit`
and `FLookupPart`, unaligned scalar accesses in `WriteFleet` and the Cybertron
workspace, an empty-record null copy and negative scanner shifts. Those paths
now use checked sentinel handling, byte-safe storage and defined multiplication.
Silent process exit also frees list/combo storage without game callbacks.
The memory-safety batch passes all 91 fatal ASan/UBSan/LSan tests without
suppressions, 50 Windows units and 13 Linux host units. Both full native and
host regressions return 519 matches, 87 unused-storage warnings and no behavioral
differences/errors/skipped inputs. The previous run had 22 unused-storage warnings;
the comparator confines those differences to unused bytes, not interpreted records. Baselines remain
unchanged. Subsequent UI/inspector qualification is recorded separately in
[window inspection](WIN32-INSPECTION.md).


Packed `BTN`/`BTNT` records keep their two-byte packing, so native `RECT`
members can be misaligned within arrays. `DrawBtn`, `InitBtnTrack`, `FTrackBtn`
and `FTrackXfer` use byte copies into aligned rectangle storage before drawing
or hit testing. This fixes the fatal UBSan load exposed by the fleet Split
workflow without changing record sizes, hit targets or game behavior.

The repair passes 99 fatal sanitizer tests, 51 Windows unit/reference tests and
both Release fleet profiles. Full native regression still returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs; the baseline is unchanged. See [goal evidence](LINUX-CLIENT-GOAL.md).

See [encoding contracts](UNICODE-BOUNDARIES.md) for the main client's UTF-8
text, UTF-8 password bytes, fixed field budgets and format 2.85. Legacy file
encoding and non-ASCII password compatibility are deliberately removed from
main. Record widths remain unchanged; the native baseline moves only at BOF
version fields, from checkpoint 0 in every scenario.
