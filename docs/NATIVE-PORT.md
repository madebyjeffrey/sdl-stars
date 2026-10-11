# Native port

The reconstructed sources build as a native Win32/Win64 executable with
MinGW, and the game code alone builds as `stars-host` with any C11 compiler
(see Platforms below). This document covers what the port changes and what
it must keep.
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
  Native pointer or handle sizes never reach a save file.
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
  paths with `chDirSep`. On Win32 these are the original `OpenFile`,
  `_lread` and related calls; on POSIX they use `open`, `read` and the like,
  which have no share modes, so a file another program has open is not
  refused. The game's heaps (`memory.c`) come from `calloc` and `realloc`.
- **Formatting and rounding** (`native.c`): the game code formats with
  `CchSprintf`, which reads `%ld` as 32 bits like Win32 `wsprintf` (the
  string table passes `int32_t` to `%ld`; `long` is 64 bits on 64-bit POSIX)
  and, like `wsprintf`, writes at most 1024 bytes,
  and rounds with `LMulDiv`, which matches Win32 `MulDiv`. Every format in
  the string table formats the same through both.
- **Platforms:** the game code (`common.h`) builds without Windows headers;
  `stars-host` links it alone. Its turns match `stars.exe`'s where
  `long double` is x87 extended precision: x86-64 Linux and macOS (on
  Apple silicon, built for x86_64 and run by Rosetta). ARM's `long double`
  is 64 bits (macOS) or 128 bits (Linux), so the rounding casts round
  differently; CMake warns about such builds.
- **Toolchain parity (keep):** `qsort16` (`native.c`) reproduces the
  Win16 CRT's tie order, and the x87 rounding casts to `double`/`float` are
  deliberate, so don't simplify them.
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
repeat saving, and the SDL cache boundary preserves complete UTF-8 characters.
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
