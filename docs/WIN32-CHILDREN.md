# Win32 child windows over SDL3

The child-window batch reached **117/178 inventoried Win32 APIs (65.7%)**,
up from 109/178 (61.2%). The six inventoried CRT helpers are counted separately.
The percentage counts local implementations of documented subsets; it is not a
percentage of full Windows compatibility or of a working Linux game client.
The full game UI still has compile-only checking and does not yet link.

This batch adds `GetParent`, `GetWindow`, `GetDlgItem`, `GetWindowRect`,
`ClientToScreen`, `ScreenToClient`, `EnableWindow` and `SetFocus`. Supporting
`IsWindow`, `IsChild`, `IsWindowEnabled` and `GetDlgCtrlID` are outside the original
178-API denominator. Existing creation, painting, text, movement, visibility,
message filtering and timers now understand children.

## Build and CLion

No new libraries or packages are required. Reload the SDL CMake profiles and
select **stars-compat-children** in `sdl-debug` or `sdl-release`. It runs an
automated test; it does not yet display interactive standard controls.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_children --output-on-failure
```

The CTest case selects SDL's dummy video driver and software renderer. In CLion,
set `SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software` in the run configuration
for the same deterministic test. Breakpoints in `CreateWindowExA`, `GetWindow`,
`FComposeChildren`, `HwndCompatMouse`, `SetFocus` and `DestroyCompatWindow` cover
creation, drawing, input and lifetime.

The shared Windows SDK executable is `test_win32_children_reference`:

```sh
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_children_reference --output-on-failure --timeout 60
```

## Implemented behavior

Child HWNDs are logical objects with their own parent-relative geometry, title,
control ID, window procedure, extra data, update region and drawing surface.
They do not create SDL desktop windows, renderers or textures. Top-level windows
may have owners; owned popups remain separate SDL windows.

Presentation copies the top-level drawing surface into a separate composition
surface and draws visible descendants in sibling order, clipped to every
ancestor and the root client area. Parent pixels remain intact when a child is
moved, hidden or destroyed. Child creation inserts beneath existing siblings,
as verified against the SDK. `GetWindow` traverses the corresponding hierarchy.
[Z-order changes through `SetWindowPos`](WIN32-POSITIONING.md) now have local
implementation and shared tests.

Mouse input targets the foremost visible, enabled child under the root-relative
point and translates coordinates to that child's client area. Keyboard and CP1252
text events target the focused descendant of their SDL root. Disabling a window
blocks its descendant mouse input and clears descendant focus in the local
runtime; `IsWindowEnabled` reports the queried window's own disabled flag.
`SetFocus` sends synchronous focus callbacks and guards callback-driven target
destruction or redirection. Full desktop activation policy remains to do.

Parent-filtered posted messages, synthesized paint messages and timers include
descendants. Hidden ancestors suppress child paint synthesis. Destruction sends
parent `WM_DESTROY` while children are still valid, destroys descendants and
owned windows, then sends parent `WM_NCDESTROY`. Timers, queued messages, DCs,
canvases and composition surfaces are reclaimed. Rejected `WM_CREATE` receives
`WM_NCDESTROY` without the normal `WM_DESTROY`, as checked against the SDK.
Handle tokens remain until
shutdown to prevent stale pointer reuse. Child creation passes the parent/ID in
`CREATESTRUCT`, delivers initial move/size messages and notifies its parent.

These contracts follow Microsoft's [window features](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features),
[WM_DESTROY](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-destroy),
[WM_NCDESTROY](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-ncdestroy),
[EnableWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow)
and [SetFocus](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus)
documentation.

## Limits and remaining work

- Only registered application window classes are available. Built-in buttons,
  static/edit/list/combo controls, notifications, dialogs and navigation are next.
- Geometry uses client-sized surfaces. Native Win32 nonclient frames, borders,
  caption sizing and DPI virtualization are not reproduced. Shared coordinate
  tests use borderless popup roots and borderless children.
- Child painting is composited for presentation; parent DC reads and source blits
  still access their own backing surface. `WS_CLIPCHILDREN`/`WS_CLIPSIBLINGS` DC
  clipping, parent-background erasure for transparent controls and full visible
  region semantics remain to do.
- Zero-sized windows, sibling reordering and topmost bands are now supported;
  see [positioning](WIN32-POSITIONING.md). The hierarchy limit is 64, and
  reparenting remains to do. Only `WS_EX_TOPMOST` extended style is accepted.
  Top-level menu handles are rejected; a child `hMenu` supplies its integer ID.
- Focus is local to the application UI thread. OS activation, restoring child
  focus after switching desktop windows, IME, full Unicode text input and
  cross-thread calls remain to do. Queue key states, wheels, double-clicks and
  CP1252 conversion are now supported; see [input](WIN32-INPUT.md).
- Mouse capture is now supported; see [capture](WIN32-CAPTURE.md) for routing,
  notifications and backend limits.
- Window operations do not yet consistently set Win32 `GetLastError`.

Wine 9.0 retains nested-child focus when an intermediate parent is disabled,
and accepts refocusing that child in the shared test. Microsoft's documentation
says descendant focus is lost when a parent is disabled. The native-only tests
assert that documented behavior; the Wine reference checks disabling the actual
focused child and the common enable-state/message contracts. This difference is
recorded rather than treating Wine as the definition of Windows behavior.
Wine also excludes descendant `WM_TIMER` messages from an ancestor HWND
filter, even after a query without an HWND filter generates the message. The
SDK reference therefore checks delivery using the timer's own HWND; the native
test checks ancestor filtering directly. Posted descendant messages are checked
with the ancestor filter on both implementations.

## Verification

`tests/compat/win32_children.inc` checks parent/owner relationships, retained IDs and
titles, sibling traversal, nested screen/client conversion, effective visibility,
enable/focus callbacks, descendant queue/timer filtering, rejected creation,
destruction during movement and parent-before-child destruction messages.
Native-only checks also verify one SDL window for the hierarchy, composition,
clipping, restoration after hiding/destruction, translated mouse coordinates and
focused-child keyboard/text routing. The existing independent-window and GDI
suites remain part of the compatibility checks.

Verified on October 10, 2026:

- SDL debug and release: 18/18 compatibility tests pass in each build.
- Windows/SDK under Wine: 28/28 unit tests pass.
- Linux host: 12/12 unit tests pass.
- Full native regression: 519 matches, 22 existing warnings, zero differences/errors
  and zero skipped reference inputs. The saved baseline remains unchanged.
- Child hierarchy tests also pass using actual X11 windows under Xvfb.
- Address, undefined-behavior and leak sanitizers pass with errors made fatal.
- MinGW and SDL builds have no new compiler warnings; `git diff --check` passes.

The required Wine tutorial check still **fails** at year 2406, page 21,
instruction 166, “Now shift click back on Stove Top,” after two attempts
(100 actions, 21 pages). This is the same failing step recorded for the previous
geometry batch. Artifact directory:
`dist/scaffold/tutorial/20261010T185945.215384Z`. The Windows tutorial build does
not link this SDL compatibility runtime. This result remains an outstanding
verification issue; it is not counted as a passing check.
