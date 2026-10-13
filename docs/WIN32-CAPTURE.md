# Win32 mouse capture

The capture milestone reached **120/178 APIs (67.4%)**, up from 118/178 (66.3%).
This milestone adds `SetCapture` and `ReleaseCapture`; `GetCapture` is a supporting
API outside the original inventory. Six CRT helpers remain counted separately.
Current coverage is **124/178 (69.7%)** after the [input milestone](WIN32-INPUT.md).
API coverage counts implemented bodies with documented scope, not full client
readiness or complete Win32 conformance.

## Behavior

Capture belongs to the application UI thread. `SetCapture` returns the previous
window, commits the new target, then synchronously sends `WM_CAPTURECHANGED` to
the previous target with the new HWND in `lParam`. Recapturing the same window
also notifies it, as verified against the SDK under Wine. `ReleaseCapture`
clears capture and notifies the previous window with a null target. Releasing
when there is no capture succeeds.

Captured motion and left/right/middle button messages bypass child hit testing
and convert the event's SDL root coordinates to the capturing window's client
coordinates. Coordinates can be negative or beyond the client's bounds. A
foreground capture can receive events originating from another root in the same
application. Background capture receives input only when hit testing places the
cursor in the capturing window or its descendants. Other input follows normal
hit testing.

Destroying the capturing window or an ancestor releases capture before teardown.
`DefWindowProc(WM_CANCELMODE)` releases capture held by that window. Disabling
a captured window therefore releases through the existing cancel-mode callback.
SDL focus loss releases capture in the affected root. Capture-change callbacks
may destroy the new target: teardown clears it without retaining a stale HWND.
Foreign-thread operations and invalid/destroying targets are rejected.

These contracts follow Microsoft's [SetCapture documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcapture)
and [WM_CAPTURECHANGED documentation](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-capturechanged).

## SDL backend limits

The runtime requests [SDL mouse capture](https://wiki.libsdl.org/SDL3/SDL_CaptureMouse)
for a root that has local keyboard focus. Capture outside native windows depends
on the backend and desktop focus policy. SDL can reject capture on unsupported
backends; the logical capture target and routing of events already delivered to
the application remain available, and SDL retains the backend error. Dummy-driver
tests prove logical routing, not desktop-wide delivery. Interactive compatibility
windows currently require X11/Xwayland, as described in [positioning](WIN32-POSITIONING.md).

This does not implement a desktop-wide hook or capture input destined for another
process. Native activation policy, menu/accelerator suppression during capture,
restoring child focus and cross-thread capture remain
unfinished. Standard buttons, dialogs and other controls still need implementation.

## Build and verification

CLion can build/debug `stars-compat-capture` with the SDL debug preset. Set
`SDL_VIDEODRIVER=x11` and `STARS_SDL_RENDERER=software` for an X11 debug run, or
use `SDL_VIDEODRIVER=dummy` for deterministic headless tests. No additional
packages are required beyond the existing SDL dependency setup.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_capture --output-on-failure
```

`tests/compat/win32_capture.c` shares capture return-value, notification,
cancel-mode, reentrant destruction and ancestor-destruction checks with the
Windows SDK. Native checks exercise signed coordinates outside bounds, cross-root
conversion, mouse-button release, restored normal hit testing, invalid handles
and focus-loss cleanup. The Windows game and Linux host do not link the SDL
runtime; no game logic, save layouts or baseline fixtures changed.

Verified on October 10, 2026:

- SDL Debug and Release: 20/20 compatibility tests pass in each build.
- Windows SDK/game units under Wine: 30/30 pass.
- Linux host units: 12/12 pass.
- The final capture test passes with SDL X11 under Xvfb. This checks delivered
  events and capture lifecycle, not hardware dragging outside the application.
- Fatal address/undefined-behavior/leak sanitizer checks pass.
- Builds have no new compiler warnings and `git diff --check` passes.
- Full native regression: 519 matches (22 with existing warnings), zero differences/errors and zero skipped reference inputs.
- The required Wine tutorial run fails at year 2408, page 26, instruction 203:
  `No ship transfer arrow is available`, after 123 actions. Artifacts are in
  `dist/scaffold/tutorial/20261010T194924.312588Z`. This repeats the preceding
  milestone's automation failure; the Windows tutorial does not link the SDL
  compatibility runtime.
