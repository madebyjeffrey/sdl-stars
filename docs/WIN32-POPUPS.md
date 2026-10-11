# Window menu ownership and interactive popup menus

Coverage is **155/178 inventoried Win32 APIs (87.1%)**, up from 153/178 (86.0%).
`GetMenu` and `TrackPopupMenu` are newly counted. Supporting `SetMenu` and
`EndMenu` are outside the original inventory. The six CRT helpers remain separate.
The percentage measures tested basic contracts; the complete SDL client remains
unlinked.

## Implemented behavior

Top-level `CreateWindow` accepts an explicit menu. `GetMenu` retrieves its live
handle; changing or removing the menu retains the previous tree for the caller
to destroy. Window destruction destroys its attached menu. Child creation keeps
the menu argument as its control ID. Submenus and menus already attached to a
different window cannot be reused as window menus. Destroying an attached tree
clears the window association.

`TrackPopupMenu` opens independent borderless SDL windows and renders copied
labels, mnemonic prefixes, shortcut text, separators, check marks, highlights,
disabled text and submenu arrows using the existing DC/font/texture path.
Submenus open beside their parents and flip left at the display's right edge;
root alignment and display work-area clamping use screen coordinates. Popup
windows are created with `WS_EX_NOACTIVATE` and do not become the application's
logical active or focused window, including SDL backend focus transitions.

Pointer movement, left/right selection, outside clicks, arrow/Home/End navigation,
Enter, Escape and text mnemonics drive selection. Keyboard navigation can
highlight disabled items but cannot execute them. Repeated mnemonic letters cycle
among matching enabled items. Selection and submenu notifications reach the
owner, and `WM_COMMAND` is queued after tracking ends. `TPM_RETURNCMD` returns the
chosen ID without a command message; `TPM_NONOTIFY` suppresses notifications.
`EndMenu`, owner/menu destruction, loss of capture and quit messages terminate
tracking safely. The modal loop dispatches unrelated messages and timers, retains
`WM_QUIT`, restores prior capture where valid, and releases transient windows and
layout allocations. `TPM_RECURSE` supports nested tracking of a different menu;
reusing an already active menu is rejected.

These contracts follow Microsoft's [TrackPopupMenu](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenu)
and [SetMenu](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setmenu)
documentation. Microsoft's [mnemonic discussion](https://devblogs.microsoft.com/oldnewthing/20171208-00/?p=97545)
describes cycling among duplicate text mnemonics. Wine 9 instead executes its
first textual match, which the SDK test observes and its
[menu source](https://github.com/wine-mirror/wine/blob/wine-9.0/dlls/win32u/menu.c)
confirms. Only the detected-Wine SDK branch accepts that first-match outcome;
local tests and native Windows require cycling. No Wine source was copied.

## Build and debug in CLion

Select `stars-compat-popup` in the SDL Debug profile. Its default run executes
contract tests; use `--demo` to interact with a popup. No new libraries are needed.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R '^win32_popup' --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_popup_reference --output-on-failure
```

Tests move the real pointer away from the popup before keyboard input and space
input events so hover and translated characters cannot alter the intended gesture.
`STARS_POPUP_CAPTURE=/absolute/path.bmp` saves an actual native popup canvas during
the mouse test for visual inspection.

## Remaining scope

Window menu association is currently data/lifetime support. `SetMenu` invalidates
the window, but menu-bar layout/drawing, nonclient geometry and menu-bar mouse/Alt
activation are not implemented. Class-default menu loading and RC menu resources
are also pending; `DrawMenuBar` remains unimplemented and uncounted. The demo's
popup is interactive while its attached window menu bar is not yet visible.

Owner-drawn/bitmap and multi-column items, RTL layout, animated tracking,
oversized-menu scrolling, hover-open delays, unmatched-character `WM_MENUCHAR`
handling and the full system menu remain unfinished. Layout rejects oversized
menus instead of overflowing coordinates. This is an application subset rather
than complete User32 conformance. Dialogs, accelerators and full-client integration
remain required work.

## Verification

SDL Debug and Release pass all 34 compatibility tests each, including the
popup X11 integration test. Windows units pass all 41 tests, including the shared
SDK popup suite with the detected-Wine mnemonic distinction. The Linux host
builds and passes all 12 unit tests. Fatal AddressSanitizer, UndefinedBehaviorSanitizer
and leak checks pass the full native popup suite. Actual popup rendering was
captured and visually inspected. The touched files introduce no new warnings.

Full native regression returns 519 matches (22 existing warnings), zero
differences/errors and zero skipped reference inputs. No game behavior, disk
layout or baseline changed.

The fresh Windows tutorial run `dist/scaffold/tutorial/20261010T231900.117928Z`
remains failing at year 2407, page 24, instruction 186 after 94 actions. It times
out querying the observer, and Wine reports `free(): invalid next size (fast)`.
It passes instruction 166. The Windows executable does not link this SDL popup
implementation. See the [tutorial report](../tests/scaffold/tutorial/README.md)
for earlier native Wine/X11 failure-stack evidence. The full tutorial must pass
before final client completion.
