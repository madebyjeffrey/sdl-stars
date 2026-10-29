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

## Final linkage ledger

Application-owned headers and implementation functions now have native C++
linkage. The temporary C declarations are removed. The only explicit C linkage
in ordinary tests is for named SDL symbol providers, whose foreign ABI remains
owned by SDL. SDK types remain isolated in the Windows picker implementation;
AppKit glue uses Objective-C++ and borrowed native object references.

`test_cpp_compilation_manifest` audits the actual compiler commands, C++20
language flags and remaining C reference exceptions. `stars-cpp-header-checks`
compiles every public header independently with standard headers before/after it.
Frozen C layout inputs live in `tests/reference/cpp-a`; SDK test wrappers remain
C translation units even when their portable bodies also serve C++ tests.

C++ compilation preserves current allocation and error contracts. The callback
containment and pending-failure delivery protocol remains in place, including
cleanup and owner re-enabling before delivery.
