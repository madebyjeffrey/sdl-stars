# Window attention over SDL3

Coverage is **140/178 inventoried Win32 APIs (78.7%)**, up from 139/178
(78.1%). This adds `FlashWindow`, used by `mdi.c` to announce a new turn and
repeat that notification through the existing timer. Six CRT helpers remain
separate. The full Linux game client is still unlinked.

## Behavior

The runtime tracks caption activation independently from keyboard focus and
from the active HWND. `WM_NCACTIVATE` updates that caption state, while real
activation changes reset the old/new root's caption state. `FlashWindow(TRUE)`
inverts it; `FlashWindow(FALSE)` restores it according to the active root.
Successful calls send `WM_NCACTIVATE` without changing focus or activation.
Caption callbacks may destroy the window safely.

For an inactive root, inversion to the active appearance requests
`SDL_FLASH_BRIEFLY`; inversion back or explicit restoration requests
`SDL_FLASH_CANCEL`. An active root does not request an urgency indication.
SDL owns the brief request's duration, and Stars' existing timer repeats the
calls. There is no new timer or thread in the wrapper.

The return value is the caption state **before** the operation, following
[Microsoft's documented contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-flashwindow).
A successful inactive-to-active flash can therefore return FALSE. A backend
failure returns FALSE with `ERROR_NOT_SUPPORTED`, leaves caption state unchanged
and sends no caption message. Invalid/stale/destroying HWNDs fail with
`ERROR_INVALID_WINDOW_HANDLE`. Child flashing is explicitly unsupported; Stars'
two call sites both use `hwndFrame`.

## Platform and visual limits

[SDL's attention API](https://wiki.libsdl.org/SDL3/SDL_FlashWindow) requests the
desktop's attention indication. On the pinned SDL X11 backend this sets or
clears `XUrgencyHint`; the test verifies the hint and actual X11 input focus.
The desktop/window manager determines how it presents that indication. A
backend that lacks attention support fails rather than silently succeeding.

Caption-state bookkeeping and notification do not yet render exact Windows
caption-color inversion, child icon highlighting or a Windows taskbar button.
Those visuals remain part of the unfinished nonclient implementation.
Minimized roots use the same SDL attention path; a shell's minimized-window
presentation and native Wayland behavior are not independently qualified.
Full Windows nonclient message ordering remains unfinished.

The Windows SDK test runs under Wine 9. Its observed return follows the new
caption state, whereas Microsoft documents the previous state. The matching
[Wine 9 source](https://github.com/wine-mirror/wine/blob/wine-9.0/dlls/win32u/window.c)
returns the requested activation value in `NtUserFlashWindowEx`. The shared
test checks caption messages, restoration, unchanged focus and callback
destruction under Wine; native-only assertions check the documented previous
state return. Wine's return value is not used as the reference for that part.

## CLion and tests

Select `stars-compat-attention` in the SDL Debug profile. The headless test wraps
`SDL_FlashWindow` to inspect requests and inject failures. With the existing
X11/XTest development packages and Xvfb available, `win32_attention_x11` runs
the same executable with `--x11` and verifies real urgency hints. No additional
dependency is required on the current development machine.

The X11 test completes initial painting before reading native window IDs:
SDL's accelerated framebuffer setup can recreate the underlying X windows.
It gives the peer actual X11 focus, then synchronizes SDL's X connection before
querying hints through its observer connection. All synthesized focus changes
occur on a private Xvfb display.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_attention --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_attention_reference --output-on-failure
```

## Verification

On October 10, 2026, SDL Debug and Release each pass all 29 compatibility tests,
including actual X11 urgency-hint changes and physical focus preservation.
Windows SDK/game units pass all 37 tests under Wine; Linux host units pass
all 12. Fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks pass.
Full native regression returns 519 matches (22 existing warnings), zero
differences/errors and zero skipped reference inputs. The fresh Wine tutorial
check failed at year 2400, page 2, instruction 15, after three actions:
`Missing or unsupported tutorial snapshot` during the shift-click on Prune.
Artifacts: `dist/scaffold/tutorial/20261010T214721.709910Z`. This stopped earlier
than the preceding batch's instruction-166 failure. The Windows/Wine observer
and automation remain unresolved; the tutorial does not link the SDL runtime.
Its executable, tutorial-text and resource SHA-256 values match the preceding
window-state batch's run (`20261010T213546.535469Z`), despite the different
failure point.

Game logic, persisted layouts and baseline
fixtures are unchanged; Windows and Linux host builds do not link this runtime.
