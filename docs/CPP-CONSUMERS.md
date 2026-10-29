# Milestone A compilation and linkage inventory

Implementation ticket: #63. A01–A13 are checkpoints in one PR against `develop`.
Original source filenames are retained; CMake source languages determine compilation.

| Consumer at the start of A | Migration checkpoint | Final classification |
| --- | --- | --- |
| Core, UI, host, error and file services | A01 | Owned C++20 |
| `data/datagen.py`, `text/textgen.py` outputs | A02 | Owned C++20, static initialization |
| Ordinary unit, scenario, tutorial and compatibility harnesses | A03 | C++20; target-specific wrappers where shared with SDK references |
| 17 simulation source witnesses | A04 | C++20; independent vector expectations retained |
| 10 workflow source witnesses | A05 | C++20; historical C reference inputs detached |
| `src/platform/encoding.c` | A06 | Owned C++20 |
| `compat/win32/src/*.c` foundation, drawing, controls and dispatch | A07–A10 | Owned C++20 |
| Four wrapper resource generators | A11 | Owned C++20 output |
| `entry.c`, Windows SDK and portal pickers | A11 | Owned C++20; SDK headers remain foreign |
| `filedialog_cocoa.m`, Cocoa test adapters | A11 | Owned Objective-C++20 |
| Application-owned header C linkage/fallbacks | A12–A13 | Removed after the last application C consumer migrates |
| SDL3, Cairo, FreeType, zlib, Fontconfig, D-Bus, AppKit, Windows SDK | Foreign | Supplied headers and genuine callback/entry conventions retained |
| SDK reference tests, frozen wire/enum C inputs, isolated CMake redirect fixture | Independent reference | C11; cannot require application headers to expose a C interface |
| `compat/win32/upstream/`, archived Win32 tutorial observer | Historical reference | Outside production compilation |

## Temporary linkage ledger

Until A13, `common.h`, `win.h`, fixed enum generators, platform file/error
headers and wrapper public headers retain existing C declarations. Private wrapper
helpers acquire explicit temporary mixed linkage when a group has both C and C++
callers. These declarations must be removed together at A13. SDK types remain
isolated from identically named wrapper types in the Windows picker bridge.

C++ compilation preserves current allocation and error contracts. The callback
containment and pending-failure delivery protocol remains in place, including
cleanup and owner re-enabling before delivery.
