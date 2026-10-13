# Active top-level windows

Coverage is **136/178 inventoried Win32 APIs (76.4%)**, up from 135/178
(75.8%). This adds `GetActiveWindow`. The six CRT helpers remain separate.
The full SDL game client is still unlinked.

## Supported behavior

The runtime maintains active-window state separately from keyboard focus.
Focusing a child selects its top-level ancestor as the active window. Calling
`SetFocus(NULL)` removes keyboard focus without deactivating that ancestor.
Destroying a focused child likewise does not deactivate its surviving root.
Destroying the active root clears activation unless a teardown callback has
already selected another root.

SDL focus gain selects the event's top-level window. SDL focus loss clears
activation when it belongs to that root, including when keyboard focus was
previously cleared. A delayed loss event for a different root does not clear
the current active window. Shutdown clears both focus and activation.

`GetActiveWindow` returns a live local top-level HWND on the SDL UI thread;
other threads receive NULL under the existing thread-bound handle lookup.
It does not query windows owned by other processes.

`ShowWindow` requests native focus for each visible, enabled, nonminimized
ordinary top-level window with `SDL_RaiseWindow`, preserving an already focused
child or assigning logical focus to its root. This covers terminal startup,
modeless technology/tutorial dialogs, reports and the scanner inspector as well
as modal dialogs. Child windows and `WS_EX_NOACTIVATE` detail cards do not
activate. `SetWindowPos` respects `SWP_NOACTIVATE` and the same exclusions.
The real-client desktop checks verify native and logical launch, dialog and
modeless technology-browser focus.

## Remaining work

This implements the active-window query used by Stars' main loop to close the
browser with Escape only when that browser is active. Full
Win32 activation message ordering, automatic owner/sibling activation after
destruction, foreground arbitration, minimized/maximized state, saved window
placement and attention flashing remain unfinished. Changing logical focus
does not promise that the Linux window manager grants foreground ownership.
The backend continues to require X11/Xwayland for real compatibility windows.
The subsequent [window-state batch](WIN32-STATE.md) adds basic minimize/maximize
state and saved-placement queries; its report records the remaining limits.

The coverage audit leaves 42 inventoried APIs unfinished. Inspection of the
28 compiled UI objects confirms unresolved references for those operations,
including the macro-expanded `CreateDialogParamA` and `DialogBoxParamA` entry
points. A successful compile-only UI target is therefore not a successful
client link. Existing implementations also retain the behavioral limits in
their individual reports.

## Tests

The shared `stars-compat-input` / `test_win32_input_reference` test checks child
focus, active-root identity, focus clearing and child teardown. The Windows
build uses the real SDK and runs under Wine. `stars-compat-windows` checks SDL
focus gain/loss across two roots, loss after explicit focus clearing and root
destruction. Both executables remain available in the existing CLion profiles;
no additional dependency is required.

## Verification

On October 10, 2026:

- SDL Debug and Release: all 26 compatibility tests pass in each build,
  including the isolated X11 input test.
- Windows SDK/game: all 35 unit tests pass under Wine, including the shared
  child-focus and active-root checks.
- Linux host: all 12 unit tests pass.
- Window contracts pass fatal AddressSanitizer, UndefinedBehaviorSanitizer and
  leak checks.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.

- The required Wine tutorial check failed at year 2406, page 21, instruction
  166, after 99 actions: `Instruction did not advance after two attempts` for
  `Now shift click back on Stove Top.` Artifacts:
  `dist/scaffold/tutorial/20261010T212647.916954Z`. This fresh run stopped
  earlier than the desktop batch's instruction-203 failure. The Windows
  tutorial does not link the SDL runtime; its failure remains unresolved.

No game logic, on-disk layout or regression baseline is changed.
