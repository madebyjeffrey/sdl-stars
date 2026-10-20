# Message boxes and alert audio

`MessageBoxA` and `MessageBeep` now have local implementations. Coverage rises
from **171/178 (96.1%) to 173/178 (97.2%)**, with six CRT helpers counted
separately. This measures inventoried APIs with tested basic implementations,
not full Win32 semantics or a working game client.

## Message-box contracts

Message boxes use the existing dialog, control, GDI and modal message machinery.
All seven standard button sets are supported, including Cancel/Try Again/Continue.
Default buttons, Return, Tab, left/right navigation and button mnemonics select
actual displayed buttons. Escape and the close button choose Cancel when present,
choose OK for an OK-only prompt, and leave a prompt without either button open.

A child owner is normalized to its top-level window. Owner modality disables
that root while other roots remain available. An ownerless `MB_TASKMODAL` prompt
temporarily disables the existing top-level windows. Previously disabled windows
stay disabled. Nested prompts and timers continue through the modal loop; a
surviving owner and its saved focus are restored afterward. Owner destruction
ends its prompt, and a pending quit is reposted for the outer loop.

Text wraps in the existing CP1252 font path. Long messages get a scrolling body
with wheel, scrollbar and keyboard navigation. Layout is clamped to the primary
work area and text is bounded to 64 KiB. Error, question, warning and information
icons are drawn locally. `MB_RIGHT` and topmost styles are supported. Unsupported
help, service/desktop and RTL flags return failure rather than fabricated choices.

Top-level SDL renderers are now initialized before showing their windows.
Renderer creation can recreate a native window; doing this on an owner's first
paint during a modal loop previously let the owner take focus from its prompt.
Disabled roots also suppress SDL focusability and delayed focus activation.

The intended button and modality contracts follow Microsoft's
[MessageBox documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messageboxa).
The shared SDK tests under this laptop's Wine observed differences: synthetic
Escape returned `IDCANCEL` for OK-only and Yes/No prompts, and the prior child
edit did not regain focus. The reference tests report those results explicitly;
the local tests enforce the documented Escape behavior and saved-focus contract.
These paths are not claimed identical to Wine.

## Alert audio

`MessageBeep` queues a short synthesized PCM alert asynchronously through SDL3.
It accepts the standard alert types and the simple-beep value, rejects invalid
types, and replaces an earlier queued alert rather than accumulating stale sound.
The operating system controls the playback device and volume. Set
`STARS_SDL_ALERTS=off` to intentionally mute alerts.

`StarsCompatSetWindowAlertSounds(hwnd, FALSE)` silences message boxes whose owner
chain reaches that window, including child controls and nested owned dialogs.
The library window opts out during initialization, so management confirmations
and error prompts are silent. Other windows and explicit `MessageBeep` calls
retain their sounds; silence does not initialize an audio device.

Audio initializes lazily and is released at compatibility-runtime shutdown.
An SDL audio subsystem initialized by the embedding application remains owned
by that application. Missing devices or queue failures return failure. The tests
inspect finite, bounded, nonzero PCM on SDL's dummy audio backend and cover muted,
missing-driver and external-ownership paths without playing physical sound.

This implements an audible equivalent of
[MessageBeep](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebeep),
not Windows registry-selected sounds or a PC-speaker fallback. No extra libraries
or packages are needed beyond the existing SDL3, FreeType and fontconfig setup.

## Limits and remaining work

Native desktop themes, localized button labels, exact Windows icon geometry,
full DPI/multi-monitor placement, narrow-screen three-button layouts, accessibility
and unusual message-box flags remain unqualified. The modal and audio runtime
shares the existing UI-thread requirement.

The five remaining inventoried APIs are `GetOpenFileName`, `GetSaveFileName`,
`PrintDlg`, `Escape` and `WinHelp`. Full-client linking also needs the legacy
`ExitWindowsEx` dependency and an SDL entry point that calls `WinMain`. The
`stars-sdl` game executable remains unlinked; deeper UI and service behavior
still needs client-level qualification after those APIs exist.

## Build and verification

Select `stars-compat-messagebox` in CLion under `sdl-debug` or `sdl-release`.
Use `test_win32_messagebox_reference` for the Windows SDK counterpart.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R 'win32_messagebox|win32_alert_' --output-on-failure
```

After this milestone:

- SDL Debug and Release: **50/50** compatibility/smoke tests passed each.
- Windows units: **48/48** passed; MinGW Debug built successfully.
- Native regression: **519 matches**, 22 existing warnings, zero differences/errors.
- Linux host units: **12/12** passed.
- AddressSanitizer, UndefinedBehaviorSanitizer and leak detection passed the native
  message-box/audio harness.
- Tutorial validation failed twice. The first run failed during startup at page 1;
  the retry reached page 9 and 31 actions before Stars exited. Both Wine logs contain
  `free(): invalid next size (fast)`. These Windows executables do not link this SDL
  runtime. The heap failure's cause remains unresolved; no successful tutorial
  validation is claimed for this batch.

Tutorial artifacts are retained under
`dist/scaffold/tutorial/20261011T011857.423497Z` and
`dist/scaffold/tutorial/20261011T012727.443541Z`. Game sources, save layouts and
regression baselines were not changed by this milestone.
