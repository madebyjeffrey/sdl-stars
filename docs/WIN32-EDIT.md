# Narrow edit controls over SDL3

The runtime now supplies the narrow `Edit` class used by Stars' name, password,
numeric and message fields. Basic API inventory coverage is **178/178 (100%)**;
control qualification extends the behavior behind already-counted APIs. Six CRT
helpers are counted separately. The native Linux client is linked.

## Implemented behavior

Text is stored as CP1252 bytes; selection indexes are byte offsets. `WM_GETTEXT`,
`WM_GETTEXTLENGTH` and `WM_SETTEXT` use the normal window text path. Programmatic
`WM_SETTEXT` bypasses the input limit and clears the modified flag. Single-line
text replacement sends `EN_UPDATE` and `EN_CHANGE`; multiline `WM_SETTEXT` does
not send `EN_CHANGE`. A parent callback may destroy the control,
and the implementation checks its lifetime before further notifications.

The class supports:

- `EM_GETSEL` with packed offsets and optional DWORD output pointers;
  `EM_SETSEL` with anchor/active-end orientation, clamping and select-all.
- `EM_LIMITTEXT`/`EM_GETLIMITTEXT`, selected-text replacement, and
  `EN_MAXTEXT` when insertion exceeds the limit.
- `EM_GETMODIFY`/`EM_SETMODIFY`, `EM_SETREADONLY`, font get/set and
  focus/change/update notifications through `WM_COMMAND`.
- Printable CP1252 input, Backspace, Delete, Left/Right, Home/End and Shift
  extension of selections. CRLF pairs are crossed/deleted together.
- Mouse positioning and captured drag selection; read-only fields reject
  user edits, while programmatic text replacement remains available.
- `ES_NUMBER` filtering for typed characters, ASCII upper/lower conversion
  during insertion, password masking, left/center/right alignment, selection
  highlighting and parent color/brush callbacks.
- Single-line automatic horizontal scrolling and explicit-CRLF multiline
  display with automatic vertical scrolling to the caret.

With selected text, unmodified Left/Right collapses the selection to its left
or right edge without moving an extra character, regardless of the active end.
Shift+Left/Right continues from the active end and keeps the anchor. Checks
cover both selection directions, select-all, existing carets, UTF-8 character
boundaries and multiline CRLF pairs.

Control+Left/Right moves by word on Windows and Linux. On macOS, Option+Left/Right
uses the same word boundaries. Adding Shift extends from the existing selection
anchor, including when a selection is already active. Native queued-input checks
cover both modifier keys, both field types, both directions and UTF-8 words.

On macOS, Command+Left/Right uses the same navigation as Home/End: the start or
end of a single-line field, or the current visual line in a multiline field.
Shift extends the selection from its existing anchor. Both Command keys work.
The native edit check routes SDL key events through the input queue and verifies
UTF-8 boundaries, both field types, selection anchors and modifier release.

macOS text fields also accept Command+A/C/X/V/Z for Select All, Copy, Cut,
Paste and Undo. These shortcuts reuse the existing selection, clipboard,
password/read-only and undo handling. Application tables reserve Command+A/C
for editing instead of Save and Submit or Continue.

The shared SDK test checks selection, limits, CRLF navigation, read-only fields,
modified state, notification behavior and destruction during `EN_UPDATE`.
Native tests also check that equal-length passwords render identically and that
SDL UTF-8 input reaches the edit as CP1252 text.

Contracts follow Microsoft's [selection documentation](https://learn.microsoft.com/en-us/windows/win32/controls/em-setsel)
and [edit text operations](https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Controls/edit-controls-text-operations.md).

## October 11 follow-up

Clipboard cut/copy/paste/clear and Ctrl+C/X/V now convert between CP1252 and SDL's
UTF-8 clipboard; password controls suppress copying. One saved edit operation
supports undo/redo toggling, `EM_CANUNDO`, `EM_EMPTYUNDOBUFFER`, `EM_UNDO`,
`WM_UNDO` and Ctrl+Z. `EM_REPLACESEL` honors its undo flag; `WM_SETTEXT` clears
undo. Explicit line count/index/length/text queries, Up/Down, Ctrl+Left/Right and
Ctrl+Home/End are implemented and tested. See [current qualification](WIN32-CONTROL-QUALIFICATION.md).

## Further behavior and limits

Visual-line wrapping, formatting rectangles, vertical/horizontal scrollbars and
scroll messages, Page keys, punctuation-aware words, context menus, blinking
carets and native typing-group undo are implemented. SDL preedit paints pending
CP1252 composition separately until text input commits it. Inserted text supports
CP1252 upper/lower conversion. Initial text and `WM_SETTEXT` also normalize
according to `ES_UPPERCASE`/`ES_LOWERCASE`, including read-only controls. Shared
SDK/native tests cover single/multiline creation and replacement, bypassing
the input limit, clearing modified/undo state, and preserving notification
behavior. Native assertions cover CP1252 accented initial/programmatic text. See [qualification](WIN32-CONTROL-QUALIFICATION.md).

Consecutive Backspace/Delete operations now share an undo record, with the
complete deleted range selected on undo. Cursor/focus changes end groups;
clipboard/programmatic replacements remain separate operations. Rejecting input
at the text limit does not create an empty undo record or discard prior history,
while retaining SDK limit/update/change notifications and modified state. Shared
single-line and multiline assertions cover these contracts, undo/redo toggling,
cursor barriers, and CRLF deletion with both lines visible.

Additional mixed-operation undo boundaries, short multiline fields without automatic scrolling, and complex
composition layout remain to be qualified. Password
fields retain text for programmatic reads while suppressing copying and painting
asterisks. Rendering uses the CPU GDI/font path and SDL presentation; real desktop
qualification currently uses X11/Xwayland.

## Build and debug

No new dependencies are needed. In CLion's SDL Debug profile select
`stars-compat-edit`. Headless debugging uses
`SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software`. Compatibility windows on a
real desktop currently require X11/Xwayland.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_edit --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_edit_reference --output-on-failure
```

## Verification

On October 10, 2026:

- SDL Debug and Release: all 24 compatibility tests pass in each build.
- The shared edit test passes against the Windows SDK under Wine.
- The edit test passes with SDL's X11 backend on an isolated Xvfb display.
- Fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks pass.
- Windows SDK/game: all 33 unit tests pass under Wine.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.

- The required tutorial check failed at year 2406, page 21, instruction 166:
  `Instruction did not advance after two attempts`, after 101 actions.
  Artifacts: `dist/scaffold/tutorial/20261010T204357.731209Z`. This is a
  Windows/Wine automation failure, not a passing tutorial result.

Windows and Linux host builds do not link this SDL implementation. Game logic, saved-file layouts and regression
baseline fixtures are unchanged.


Empty editable `EM_UNDO`/`WM_UNDO` preserves text, modified state and empty
history while restoring the retained undo position and sending `EN_CHANGE`
without `EN_UPDATE`. Read-only fields preserve pending history and send no
notifications; single-line undo returns true and multiline undo returns false.
Shared SDK/native tests verify these states and the original undo after editing
is re-enabled. The retained position clamps after shorter `WM_SETTEXT` input.

Ctrl+Z with empty history has the same retained-position and change-notification
effects as an explicit empty undo message; a shared SDK/native keyboard/character
sequence verifies the shortcut.


Text-changing undo/redo sets the modified flag and selects the restored
replacement range. The update/first change observes a collapsed replacement
caret; the final change observes the restored range. Explicit replacement
lengths retain the correct range for identical text. Shared SDK/native tests
also destroy the control from an update callback and verify safe completion.
