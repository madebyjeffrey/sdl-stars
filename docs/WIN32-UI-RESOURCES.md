# Menu resources and accelerators

This milestone adds `LoadAcceleratorsA` and `TranslateAcceleratorA`, bringing
verified basic coverage to **157/178 inventoried Win32 API spellings (88.2%)**.
There are **21 spellings remaining**. Six CRT helpers and supporting APIs
outside the inventory are counted separately. The full SDL client is still
not linked. Visible menu bars were added in the [following milestone](WIN32-MENU-BARS.md).

## Canonical resources

The native generator now also embeds the three numeric custom-file declarations
for the recorded tutorial host/log and turn data under `res/data`. Omitting these
blocked `FWriteTutorialMFile` during first-year generation in the SDL client.
The shared resource test requires all three and compares their exact bytes with
the Windows SDK resource module. CMake tracks those binary inputs for both native
generation and the SDK comparison manifest. Current custom-file declarations
must use numeric types 256–65535 and paths inside `res/data`; this is a bounded
extension of the existing application-specific RC grammar.

`compat/win32/uigen.py` compiles the application's `MENU` and `ACCELERATORS`
declarations in `res/stars.rc`. Resource names and command IDs come from the
same RC/header files used by `windres` for Windows. CMake regenerates embedded
bytes when these sources or virtual-key definitions change.

The generated normal menu template uses a version-zero header, little-endian
WORD fields, UTF-16 captions, nested popups and end-of-level flags. Separators
use empty captions with ID zero. Accelerator resources contain eight-byte
records; public `ACCEL` structures retain their six-byte Win32 ABI.

The generator intentionally accepts the grammar and flags used in the current
application. Unsupported menu/accelerator declarations fail generation;
`MENUEX`, extended templates, preprocessing and arbitrary RC compilation are
outside its scope. Caption conversion is restricted to CP1252, matching the
existing narrow-character wrapper.

The existing resource handles now cover bitmap, menu and accelerator types.
`FindResource`, `SizeofResource`, `LoadResource`, `LockResource` and resource
`GlobalLock` return immutable module-owned bytes. Only the embedded current
module and numeric resource types are supported.

## Menu lifecycle

Supporting `LoadMenuA` parses bounded resource bytes and returns a fresh mutable
menu tree for each call. It rejects malformed/truncated templates and cleans
up partial trees. The parser limits nesting to 64 levels and never assumes
aligned resource storage.

Top-level `CreateWindowExA` loads its registered class menu when no explicit
menu is supplied. An explicit menu overrides the class default; child windows
continue to treat the menu parameter as a control ID. Attached menus are
destroyed with their owner window. Rendering and interacting with a menu bar
are covered by the [menu-bar milestone](WIN32-MENU-BARS.md); the existing popup implementation can display
loaded submenus.

## Accelerator delivery

Both application tables load: `IDA_MAIN` has 18 bindings, `IDA_TITLE` has four.
The runtime owns copied entries and caches resource-loaded tables until
shutdown. Supporting create/copy/destroy table APIs allow custom tables;
`SetKeyboardState` changes the calling UI thread's queue-visible key state,
without changing physical or asynchronous state.

`TranslateAcceleratorA` matches virtual keys from `WM_KEYDOWN`/`WM_SYSKEYDOWN`
and character entries from `WM_CHAR`/`WM_SYSCHAR`. Virtual bindings require
exact Shift/Ctrl/Alt state. Matching dispatches `WM_COMMAND` synchronously with
command ID in the low word and accelerator marker 1 in the high word.
Matched keys remain consumed when disabled/captured windows suppress dispatch.

Menu commands send `WM_INITMENU` and ancestor `WM_INITMENUPOPUP` callbacks and
recheck menu state after callbacks, allowing replacement, disabling or window
destruction. Disabled commands and minimized menu owners suppress commands;
commands absent from the menu still work while minimized. Copied entries and
fresh path lookup protect against callbacks reallocating menu/table storage.

Remaining semantics include system-menu `WM_SYSCOMMAND` routing, the active
window/no-focus `WM_SYSKEYUP` case, and comprehensive character-accelerator
modifier/code-page behavior. The game uses virtual bindings only. This is
basic application coverage, not a complete Win32 accelerator implementation.
See Microsoft's [TranslateAccelerator contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translateacceleratora).

## Build and verification

The CLion target is `stars-compat-ui-resources` in `sdl-debug` and `sdl-release`:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
SDL_VIDEODRIVER=dummy STARS_SDL_RENDERER=software \
  dist/sdl-debug/bin/stars-compat-ui-resources
```

`win32_ui_resources` runs under CTest. The Windows SDK reference target is
`test_win32_ui_resources_reference`, compiled against actual Windows headers
and the application's original RC file.

The shared test compares every generated menu/accelerator resource byte with
the Windows resource compiler's output, checks menu independence and default
class attachment, exercises all 22 game bindings, and verifies modifier
mismatches, key-up rejection, custom character tables, synchronous commands,
menu callbacks, suppression and destructive callbacks. The native test also
rejects every truncated prefix of the menu template. The dummy SDL driver
cannot minimize native windows, so this test explicitly sets logical minimized
state after attempting `ShowWindow`; the SDK reference uses native minimization.

Verification for this batch:

- SDL Debug and Release compatibility suites: 35/35 each.
- Windows SDK/unit suite: 42/42.
- Linux host build/unit suite: 12/12.
- Address, undefined-behavior and leak checks: passed, including all truncated menu prefixes.
- Native regression: 519 matches, 22 existing warnings, zero differences/errors or skipped reference inputs.
- The fresh tutorial run failed at year 2407, page 24, instruction 186 after 94 actions. Wine reported `free(): invalid next size (fast)` and the observer timed out. Artifacts: `dist/scaffold/tutorial/20261010T233734.051509Z`.

The tutorial failure remains an unresolved verification limitation. Windows
and host targets do not link this compatibility runtime.
