# Desktop geometry and settings over SDL3

This milestone reaches **135/178 inventoried Win32 APIs (75.8%)**, up from
131/178 (73.6%). Six CRT helpers are counted separately. The added APIs are
`GetSystemMetrics`, `SystemParametersInfoA`, `MapWindowPoints` and
`GetWindowThreadProcessId`. Coverage counts tested implementations within the
limits below; the full native Linux Stars client is still unlinked.

## Display and layout queries

`GetSystemMetrics` reads primary screen dimensions from SDL display bounds.
Virtual-screen metrics use the union of all SDL display bounds, including
negative origins, and `SM_CMONITORS` reports their count. Failed display queries
return zero. `SystemParametersInfo(SPI_GETWORKAREA)` returns the primary display's
SDL usable bounds, leaving the caller's rectangle unchanged on failure.

Classic layout constants supply the metrics used by Stars: one-pixel borders,
three-pixel dialog frames, eight-pixel sizing frames, a 23-pixel caption,
17-pixel scrollbars and four-pixel drag thresholds. These are explicit local
layout choices, not queries of Windows themes or the Linux window manager.
They do not complete nonclient frame or scrollbar rendering.

The supported settings actions are read-only: work area, wheel scroll lines and
wheel scroll characters. Both wheel queries return the classic default of three;
SDL has no portable query for the desktop's line/character scrolling preference.
Unsupported actions or null outputs fail. No desktop setting is modified.

Display queries can precede window creation. SDL video initialization is now
shared with the window constructor and its ownership is tracked. Shutdown quits
video when the wrapper initialized it, and leaves caller-owned SDL video running.

## Coordinate mapping and identity

`MapWindowPoints` maps arrays between logical child/root client coordinate
systems and the desktop (`NULL`). It includes nested child offsets and returns
the packed signed 16-bit displacement. A zero displacement is a successful zero
return; callers should clear/check last error when distinguishing failure.
Ordinary two-point rectangles preserve their orientation.

Native mapping validates the entire array before changing it. An out-of-range
32-bit coordinate fails with `ERROR_ARITHMETIC_OVERFLOW` and leaves all points
unchanged. Invalid window handles fail with `ERROR_INVALID_WINDOW_HANDLE`.
The shared SDK test checks inverse/identity mapping, screen conversions, signed
displacements and two-point rectangles. Overflow rejection is a documented local
boundary, not a claim that Windows rejects every overflowing input identically.

`GetWindowThreadProcessId` supplies the Linux process ID and UI-thread ID for a
live compatibility HWND. It supports a null process-output pointer and rejects
stale handles. This supplies the identity check in Stars' wheel-event router.

Contracts follow Microsoft's [coordinate-mapping documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-mapwindowpoints)
and [mouse input settings](https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/inputdev/about-mouse-input.md).

## Limits and remaining work

The wrapper uses SDL desktop coordinate units, matching its window-position
and client-coordinate paths. Per-monitor Windows DPI awareness, mirrored/RTL
windows, desktop HWNDs belonging to other applications, theme metrics and Linux
desktop wheel preferences remain unqualified. Window identity queries and these
UI services currently run on the SDL UI thread. Unsupported metric indexes
return zero; unsupported settings actions fail rather than claim success.

Real compatibility windows still require X11/Xwayland. Display queries alone do
not remove the documented Wayland root-position and global-coordinate limits.
The subsequent [activation batch](WIN32-ACTIVATION.md) adds active-window queries,
and the [window-state batch](WIN32-STATE.md) adds basic minimize/maximize and
placement queries. Nonclient rendering, scrolling,
combo boxes, dialogs, menus and full-client linking remain separate work.

## CLion and tests

No additional dependencies are needed. Select `stars-compat-desktop` in CLion's
SDL Debug profile. Headless runs use
`SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software`. The native test compares live
SDL geometry, then injects two displays with a negative origin and display-query
failures. Injection is confined to private test-library copies through C compile definitions.
The shared SDK test is `test_win32_desktop_reference`.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_desktop --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_desktop_reference --output-on-failure
```

## Verification

On October 10, 2026:

- SDL Debug and Release: all 26 compatibility tests pass in each build.
- The shared desktop test passes against the Windows SDK under Wine.
- The test passes with SDL's X11 backend on an isolated Xvfb display.
- Fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks pass.
- Windows SDK/game: all 35 unit tests pass under Wine.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.

- The required tutorial check failed at year 2408, page 26, instruction 203:
  `No ship transfer arrow is available`, after 124 actions. Artifacts:
  `dist/scaffold/tutorial/20261010T211041.925868Z`. This is a Windows/Wine
  automation failure, not a passing tutorial result.

Game logic, disk layouts and baseline fixtures are unchanged. Windows and Linux
host builds do not link this SDL implementation.
