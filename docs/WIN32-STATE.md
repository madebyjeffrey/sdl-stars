# Window state and saved placement

This batch reaches **139/178 inventoried Win32 APIs (78.1%)**, up from
136/178 (76.4%). It adds `IsIconic`, `IsZoomed` and `GetWindowPlacement` and
extends `ShowWindow` and size-event routing. Six CRT helpers remain separate.
The full SDL Stars client is still unlinked.

## State and geometry

Each HWND tracks normal, minimized and maximized state. Transitions update
`WS_MINIMIZE`/`WS_MAXIMIZE` before size callbacks. Minimized clients report a
zero-sized client rectangle. `SW_SHOW` and `SW_HIDE` preserve state;
`SW_SHOWNORMAL` selects normal state. Normal bounds are saved before leaving
normal state and updated by normal positioning and SDL geometry events.
`SW_RESTORE` returns a window minimized from maximized state to maximized;
restoring an already maximized window selects its normal bounds. The shared
SDK test verifies this distinction.

Child maximization fills the parent's client rectangle. Restoration uses the
saved parent-client bounds. Minimize/maximize/restore size notifications use
`SIZE_MINIMIZED`, `SIZE_MAXIMIZED` and `SIZE_RESTORED`, respectively. Window
destruction inside a size callback terminates the remaining show operation
safely. HWND queries reject stale tokens.

Top-level requests use SDL minimize/maximize/restore functions. Rejected
requests do not commit a new state. Accepted requests record the requested
state; SDL state events reconcile it with desktop changes and queue size
messages through the existing dispatcher. Actual desktop size and placement
remain subject to the window manager. Resize events use the current state
and do not replace minimized client dimensions with a queued nonzero size.

## Placement ABI and coordinates

`GetWindowPlacement` returns the native 44-byte structure, normal bounds,
normalized show state, zero flags and default (-1, -1) minimum/maximum
positions. Child bounds use parent-client coordinates. Root normal bounds
use workspace coordinates: the work-area offset relative to its display
origin is subtracted from screen coordinates. A secondary display's negative
screen origin is retained; the whole display origin is not subtracted.

The wrapper accepts `length == sizeof(WINDOWPLACEMENT)` and explicitly accepts
the legacy value 22 still passed by `mdi.c:GetWindowRc`. This is a local Stars
compatibility extension using the full native output structure, not a Win16
buffer format. Other lengths and null outputs fail. Invalid handles,
display-query failures and coordinate overflow leave the caller's structure
unchanged. Game source and persisted layouts are untouched.

## Limits and remaining work

Minimized child icons, nonclient frames/captions and shell placement histories
are not rendered yet. Minimum/maximum position fields do not reproduce desktop
icon placement, and minimized `GetWindowRect` still follows the existing client
geometry path. Automatically resizing a maximized child when its parent changes
size and initially hidden windows created with minimized/maximized styles need
further work. Full Windows state/message ordering and native Wayland placement
remain unqualified. `FlashWindow`, dialogs, menus and full-client linking remain
unfinished. These API counts describe tested basic contracts, not complete
Windows behavior or full-client readiness.
The subsequent [attention batch](WIN32-ATTENTION.md) adds root `FlashWindow`
requests; exact nonclient caption visuals remain unfinished.

## CLion and tests

Select `stars-compat-state` in the SDL Debug profile. No new library is needed.
The SDK counterpart is `test_win32_state_reference`. Shared cases cover child
normal placement, minimizing, maximizing, restoration, saved bounds after
positioning, hide/show state preservation, style flags and callback destruction.
Native-only tests cover SDL state routing, repeated state events, the legacy
length exception, stale/invalid inputs and atomic display-query failure. GNU
linker wrapping injects a negative-origin display and a work-area offset only
into the native test executable.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_state --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_state_reference --output-on-failure
```

## Verification

The SDL Debug and Release suites pass all 27 compatibility tests, including
X11 input. The Windows SDK/game suite passes all 36 tests under Wine. Linux
host units pass all 12 tests. The extended SDK restoration case also passes
in an isolated Wine prefix. The state contracts pass fatal AddressSanitizer,
UndefinedBehaviorSanitizer and leak checks and a separate X11 run.
Full native regression returns 519 matches (22 existing warnings), zero
differences/errors and zero skipped reference inputs. The fresh Wine tutorial
check failed at year 2406, page 21, instruction 166, after 94 actions:
`Instruction did not advance after two attempts` for
`Now shift click back on Stove Top.` Artifacts:
`dist/scaffold/tutorial/20261010T213546.535469Z`. This Windows/Wine test does not
link the SDL runtime, and its automation failure remains unresolved.
No game logic, file layouts or regression baseline is changed.
