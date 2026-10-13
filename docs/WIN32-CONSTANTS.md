# Native Win32 header constants

The local compatibility headers now add **934 constants** from the installed
mingw-w64 11.0.1 SDK, beyond the previously declared application subset. These
are local source declarations; Linux builds do not depend on MinGW headers.

The expansion covers window/extended/class/dialog styles, standard button,
static, edit, list, combo and scrollbar styles; messages and notifications;
window positioning, activation and hit tests; keys and mouse flags; colors and
metrics; menu/message-box flags; and GDI pens, brushes, fonts, text, mapping,
regions and bitmaps. Aliases retain standard names, high style bits retain
32-bit unsigned values, and negative sentinel values remain signed.

All definitions in `compat/win32/include/wincompat_constants.h` now belong to its
include guard. A repeated-inclusion test checks this, including declarations
that previously appeared after the guard.

`tests/compat/win32_constant_values.h` checks every newly added value in the shared
ABI source. The same assertions compile against the local Linux headers and the
MinGW Windows SDK. They validate declarations, not control semantics. Constants
for unimplemented messages, styles or GDI modes do not add runtime support.
Basic API inventory coverage remains **178/178 (100%)**, plus six CRT helpers;
complete behavior qualification remains separate.

The [inspector](WIN32-INSPECTION.md) expands stored window/control styles into
readable names and retains unknown bits in hexadecimal. This works for copied
snapshots in headless mode too.

Build and check the declaration surface:

```sh
cmake --build --preset sdl-debug
ctest --preset sdl-headless -R '^win32_(abi|inspect)$'
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a ctest --test-dir dist/mingw-debug -R '^test_win32_abi_reference$' --output-on-failure
```

No additional libraries are required for this expansion. MinGW is only needed
to build the Windows application and SDK reference tests.

The combo input fixture now waits for its initial native X11 position before
routing a synthetic click. Previously it could convert coordinates while the
shown popup still reported `(0,0)`, then process the click after mapping moved
the popup. This corrects test readiness without changing the application runtime.

Verification: Debug and fatal ASan/UBSan/LSan suites each pass 93/93, with no
sanitizer suppressions; Windows units and SDK reference tests pass 50/50.
Both full SDL runs used a private outer Xvfb/Openbox desktop for all X11 cases.
The final Release ABI, inspector and combo checks pass 5/5 there as well.
The Windows, SDL Debug/Release/sanitizer and Linux host builds succeed. Core
logic, on-disk layouts and regression baselines are unchanged by this expansion.

Use the headless preset for invisible checks. A plain full SDL CTest run still
has legacy X11 cases that inherit `DISPLAY`; run those inside a private virtual
desktop when they must not interact with an active desktop session.
