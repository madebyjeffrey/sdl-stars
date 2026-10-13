# Standard buttons and text labels over SDL3

This milestone reaches **131/178 inventoried Win32 APIs (73.6%)**, up from
124/178 (69.7%). Six CRT helpers remain counted separately. The percentage
counts tested API bodies within their documented scope; the full Linux game
client is still compile-only and unlinked.

## Implemented contracts

The runtime lazily supplies the narrow `Button` and `Static` classes. Button
styles include push/default push, manual/automatic checkbox, manual/automatic
three-state checkbox, manual/automatic radio and group box. Static labels
support left, center, right, simple and left-without-wrapping text.

Buttons support check/pressed/focus state messages, mouse capture, cancellation
when capture changes, drag-out cancellation, Space-key activation and
`BN_CLICKED` parent notifications with the control ID and HWND. Automatic
controls update check state before notifying the parent. Automatic radios use
sibling `WS_GROUP` boundaries; checked radios acquire `WS_TABSTOP`. A callback
may destroy the clicked control or its parent. Explicit `BM_CLICK` can notify a
disabled button, as the shared SDK test confirms; physical pointer routing
filters disabled windows.

`WM_SETFONT` borrows a caller-owned font, and `WM_GETFONT` returns it. Painting
uses the existing GDI/text renderer and parent `WM_CTLCOLORBTN` or
`WM_CTLCOLORSTATIC` callbacks. Parent brushes are borrowed. Callback destruction
during painting is tested. Non-notifying text labels and group boxes are
transparent to pointer routing; `WindowFromPoint` skips text labels.

Seven inventory entries now have local implementations:

- `GetSysColor`: a fixed classic palette.
- `SendDlgItemMessageA`: synchronous forwarding to an immediate child by ID.
- `SetDlgItemTextA` and `GetDlgItemTextA`: child text access and bounded copying.
- `CheckDlgButton` and `IsDlgButtonChecked`: child check-state messages.
- `CheckRadioButton`: check/uncheck immediate children within an inclusive ID range.

A missing child yields zero for reads/messages and failure for text writes.
`CheckDlgButton` still returns success for a missing ID, matching the SDK test.
`CheckRadioButton` snapshots child handles before invoking procedures, so
callbacks cannot invalidate the traversal by changing or destroying siblings.

## Limits and remaining work

Rendering is an independent classic approximation: fixed colors, simplified
check/radio marks and a solid focus frame. It does not reproduce Windows theme,
DPI or dotted focus rendering. `GetSysColor` does not read the desktop theme.

This is not a dialog manager. Dialog templates, modal loops, tab traversal,
mnemonics, default-button routing and dialog-specific keyboard handling remain.
Child-ID helpers currently search immediate children. Edit, list, combo and
scrollbar classes, owner drawing, icon/bitmap buttons and nontext static styles
remain. Unsupported creation styles fail where checked; other style combinations
may be accepted without full behavior. Static `SS_NOTIFY` notifications and
button double-click notification variants remain unfinished.

Automatic radio grouping currently traverses all siblings within the group,
including disabled or hidden radio controls. Class atoms for built-in controls
are not supplied. Button style changes are limited; changing to an unsupported
style does not provide its behavior. Exact input/message ordering beyond the
shared cases remains to be qualified.

## CLion and verification

No additional libraries are needed beyond the existing SDL3, FreeType and
fontconfig setup. In CLion's SDL Debug profile select `stars-compat-controls`.
For headless debugging use environment variables
`SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software`; interactive compatibility
windows require X11/Xwayland. The Windows SDK counterpart is
`test_win32_controls_reference`.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_controls --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_controls_reference --output-on-failure
```

## Verification

On October 10, 2026:

- SDL Debug and Release: all 23 compatibility tests pass in each configuration.
- The shared controls test passes unchanged against the Windows SDK under Wine.
- The controls test also passes with SDL's X11 backend on an isolated Xvfb display.
- Fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks pass for controls.
- Windows SDK/game: all 32 unit tests pass under Wine.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.

- The required tutorial check failed at year 2408, page 26, instruction 203:
  `No ship transfer arrow is available`, after 124 actions. Artifacts:
  `dist/scaffold/tutorial/20261010T203119.332994Z`. This Windows/Wine automation
  failure has occurred in earlier batches; it is not a passing tutorial result.

No game core, saved-file layout or regression baseline has changed. The Windows
and host builds do not link this SDL control implementation.
