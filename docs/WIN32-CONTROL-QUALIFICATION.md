# Control follow-up and memory qualification

Basic API inventory coverage remains **178/178 (100%)**, plus six CRT helpers.
Control semantics extend already-counted APIs. This batch does not complete the
entire Windows control contract.

## Current product scope

The current priority is finishing Stars' application-relevant controls, without
extending the tutorial. The acceptance scope in
[the goal report](LINUX-CLIENT-GOAL.md) distinguishes used styles from generic
Win32 extensions. The shared edit suite now checks adjacent Delete/Backspace
in both directions, replacement followed by typing, redo and non-undoable
replacement in single-line and automatically scrolling multiline controls.
The old implementation restores only the last removal/typed insertion; the
SDK restores the entire adjacent group. The native repair passes both suites.
This resolves the additional mixed-operation undo qualification item for the
tested application styles. The latest failed-cursor-query safety test also
passes both native and SDK probes.

Final product verification passes 102/102 non-tutorial native sanitizer cases,
52/52 Windows units/reference tests under Wine, 13/13 host units and 16/16 focused
Release file/control checks. Windows-native and Linux-host regressions each
match all 519 checkpoints (87 existing unused-storage warnings, no errors,
differences or skipped inputs). All builds pass without new touched-file
warnings. This completes milestone 2 for the used application styles described
above, not the entire Win32 control specification. Existing tutorial profiles
and their known Wine failure remain recorded; no tutorial profile was executed
or extended by this batch.

## Memory safety completed first

The memory batch fixes native alignment in ordinary heaps while retaining
battle heaps' two-byte record boundaries. It also repairs host-sentinel player
reads, unaligned scalar workspace/file-buffer stores, empty-record copying and
signed negative scanner shifts. Silent process exit frees list/combo storage
without application callbacks. Persisted widths and formats remain unchanged.

Qualification: 91/91 fatal ASan/UBSan/LSan tests with no suppressions, 50/50
Windows unit tests and 13/13 Linux host unit tests. Full Windows-native and Linux
host regressions each return 519 matches, 87 unused-storage warnings and no
behavioral differences/errors/skipped inputs. The previous run had 22 warnings;
the comparator identifies the additional differences as unused storage.
No regression baseline was changed. See [native port](NATIVE-PORT.md).

## Implemented control follow-up

- Edit cut/copy/paste/clear and Ctrl+C/X/V convert CP1252 to/from SDL's UTF-8
  clipboard. Password copying is suppressed; read-only fields reject user edits.
  Undo stores one edit operation and toggles it for undo/redo; setting text clears
  it, and `EM_REPLACESEL` honors its undo flag. Line count/index/length/text
  queries, multiline Up/Down and Ctrl-word/document navigation are available.
- Extended lists support anchored Shift ranges, Ctrl caret movement and Ctrl+Space
  toggling. Captured dragging selects rows with timer-driven autoscroll. Native
  character search accumulates a one-second prefix and cycles repeated first
  letters. Vertical scroll messages move the viewport; font changes recompute
  plain-list row height.
- Child scrollbar arrows/pages repeat after 400 ms and then every 50 ms. Moving
  outside the pressed region pauses repeat; parent updates moving the thumb under
  a page-click stop further page requests. Release/cancel/capture transfer remove
  the internal timer. Position changes remain the parent's responsibility.

The shared edit, list and scrollbar tests pass against both the local runtime
and the Windows SDK under Wine. Shift ranges are shared assertions. Ctrl-caret
and accumulated-prefix tests are native assertions: Wine's list implementation
handles those inputs differently, so these are not claimed as SDK-verified.
The primary [Wine list implementation](https://github.com/wine-mirror/wine/blob/master/dlls/user32/listbox.c)
was inspected to explain this comparison limitation; its source was not copied.

## Further control follow-up

Standard vertical/horizontal bars now reserve client space and route requests to
owners. `SCROLLINFO` supports page sizes, page-aware thumbs and 32-bit tracking;
`ShowScrollBar` changes visibility. Combo scroll styles apply to the dropdown,
without adding bars to the closed field. Fixed-height lists now show scrollbars,
scroll horizontal extents, clip owner-drawn rows, round integral heights and
restore prior selections as Ctrl-drag ranges shrink.

Multiline edits wrap visual lines and support formatting rectangles, explicit
scrolling, Page keys, punctuation-aware word navigation/selection, blinking
carets, context menus and CP1252 case conversion on inserted text. Contiguous
native typing shares one undo group. SDL IME preedit appears without changing
stored text until commit; this is basic composition support, not IMM32.
System-color pseudo-brushes now work for class backgrounds.

Shared SDK tests cover wrapping/format rectangles and scrollbar/list page state.
Native tests cover Ctrl-drag, caret timers, context-menu cancellation, typing
groups and pending IME composition. Exact notification ordering, complete undo
boundaries, complex IME layout,
variable-height/multicolumn lists and full owner-draw action sequences still
need broader qualification. Accepted style bits do not imply complete behavior.

The inspector's non-ASCII title test also exposed a native title boundary bug.
Ordinary native window titles now convert CP1252 to UTF-8; narrow stored text
is unchanged. SDL popup roles retain logical text without attempting unsupported
native caption updates. Focused X11 popup/menu/inspector tests pass fatal leak
checks after this repair.

## Build and qualification

No new dependencies. The existing `stars-compat-edit`, `stars-compat-list` and
`stars-compat-scroll` targets remain debuggable in CLion. The built-in
[window and map inspectors](WIN32-INSPECTION.md) now expose controls and drawn
scanner objects before work on the native tutorial runner.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --preset sdl-headless -R 'win32_(edit|list|scroll|inspect)$'
```

Before this further control batch, Debug and Release passed 93/93 tests each, and the full fatal
ASan/UBSan/LSan suite passes 93/93 without suppressions. Headless Debug passes
80/80 with both display variables cleared; headless fatal sanitizers also pass
80/80. Windows units pass 50/50. A final viewer-only background paint/pixel
assertion passes both inspector tests in Debug/Release/sanitizers (6/6) and the
headless inspector case (1/1), with offscreen visual verification. The tutorial is intentionally deferred for the inspection feature.

## October 11 controls and scanner inspection qualification

The further control batch adds the behaviors described above and the game-aware
[map inspector](WIN32-INSPECTION.md). Debug/Release and MinGW Debug builds pass
without new warnings in the changed files. The full fatal ASan/UBSan/LSan suite
passes 96/96 without suppressions; Windows units pass 51/51 and Linux host units
13/13. Full Windows-native and host regressions each return 519 matches,
87 unused-storage warnings, zero differences/errors and zero skipped inputs.
No baseline or saved-file layout changed. Tutorial work remains intentionally
deferred by the user.

The closed-combo regression has shared SDK width checks and native arrow paint
checks. Proportional thumbs have 20%/50% pixel checks, scanner/report page and
limit checks, and live thumb tests that inspect pixels before release or draining
paint messages. A burst of captured SDL mouse motions resolves to the latest
position. Unchanged scrollbar layout no longer repeats resize/show work.

The final headless Debug preset passes 81/81 with both display variables cleared.
Release control/Continue checks pass 6/6. A final fatal sanitizer check of input,
capture, scroll, combo, list, edit and loaded-game inspection passes 7/7, including
the latest nested scrollbar painting and motion-burst assertions. The offscreen
scanner inspector was rendered and visually checked. All UI suites ran
sequentially; desktop cases used private virtual displays.

## Scroll smoothness and corner qualification

The follow-up batches owner and thumb presentation, keeps position-only changes
from invalidating the scanner client, and copies overlapping canvas rows directly.
The two-bar nonclient corner uses Windows 95 button-face gray; pixel tests cover
top-level and nested windows. See [scroll controls](WIN32-SCROLL-CONTROLS.md#drag-smoothness-follow-up).

Qualification: 96/96 fatal ASan/UBSan/LSan tests, 51/51 Windows units, 81/81
headless Debug and 5/5 focused Release checks. The full native regression returns
519 matches, 87 unused-storage warnings, zero differences/errors and zero skipped
inputs. No game code, saved layouts or baselines changed in this follow-up; the
previous host qualification remains applicable. Basic API coverage stays
178/178 (100%). Tutorial work remains deferred.

## Milestone 2 qualification targets

The resource templates and dynamic controls use fixed owner-drawn lists and
combos with strings (`res/stars.rc`, `planetui.c`). Production and planet drawing
handlers explicitly handle `ODA_DRAWENTIRE`, `ODA_SELECT`, `ODA_FOCUS`, and an
empty-list `itemID == -1` focus action (`produceui.c`, `planetui.c`).

The list runtime now sends synchronous `ODA_FOCUS` actions on focus gain/loss,
before `LBN_SETFOCUS`/`LBN_KILLFOCUS`. It also sends the empty-list focus action
when repainting an empty focused owner-drawn list. The shared SDK test proves
nonempty focus action/state and notification ordering under Wine. Before the
fix, the same test failed in the native runtime because it only queued a full
repaint. Native assertions cover empty focus painting and callbacks resetting or
destroying the list. Full batch qualification passes all 97 tests under fatal
ASan/UBSan/LSan and all 51 Windows unit/reference tests. The native regression
returns 519 matches, 87 existing unused-storage warnings, zero differences/errors
and zero skipped inputs. Debug, Release and MinGW Debug builds pass with no new
warnings. No game results, saved layouts or regression baselines changed.
Two focused Release tests (list and real race workflow) and a direct Release
list check on a private X11/Openbox display also pass. UI suites ran sequentially.

The [Microsoft DRAWITEMSTRUCT contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-drawitemstruct)
describes `itemID == -1` for an empty control's focus rectangle. Wine's installed
runtime emits no owner callback in that empty case, and its
[list implementation](https://github.com/wine-mirror/wine/blob/master/dlls/user32/listbox.c)
draws the empty focus rectangle itself. This native assertion is therefore based
on the documented contract and Stars' handler, not claimed Wine equivalence.
No Wine source was copied. The
[WM_DRAWITEM contract](https://learn.microsoft.com/en-us/windows/win32/controls/wm-drawitem)
requires the application to restore its drawing context state before returning;
broader clipping/state and selection-action qualification remains open.
Variable-height and multicolumn list styles remain unsupported; the current
game templates do not use them.

The edit follow-up uses separate typing, Backspace and Delete undo groups.
Adjacent deletions restore the full removed selection; cursor/focus changes end
the group. A text-limit rejection retains SDK update/change/max-text notifications
and modified state without fabricating a new undo record or losing prior history.
Shared single-line and multiline assertions cover grouped deletion, redo,
restored ranges, cursor barriers, limit history and CRLF deletion with both lines
visible. Before the repair, the probe found a spurious undo record at the limit
and undo restored only the last deleted character. Full batch verification passes
97/97 fatal ASan/UBSan/LSan tests and 51/51 Windows unit/reference tests. The
native regression returns 519 matches, 87 existing unused-storage warnings, zero
differences/errors and zero skipped inputs. Debug, Release and MinGW Debug builds
pass without new warnings. No baseline or saved layouts changed.
Two focused Release tests (edit and real race workflow), plus a direct Release
edit check on a private X11/Openbox display, also pass. UI suites ran sequentially.
Short multiline fields without automatic scrolling and additional
mixed-operation undo boundaries remain open qualification items; the short
viewport SDK probe did not match the visible-line case.

Single-selection fixed owner-drawn lists now send synchronous `ODA_SELECT`
callbacks for the old and new visible rows. Repeated selection sends no callback;
keyboard selection redraws before notification. Shared SDK/native tests cover
select/change/clear/repeat and keyboard transitions; native assertions reset and
destroy the list during drawing. The runtime stops the second draw if the owner
changes storage or reselects during the first callback. Wine's multi-selection
`LB_SETSEL` probe does not send `ODA_SELECT`, so the single-selection fix does not
extend that action sequence to multiple-selection lists. Full multi-selection,
caret-focus and drawing-state sequences remain qualification items. No Wine
source was copied.

This selection batch passes 97/97 fatal ASan/UBSan/LSan tests and 51/51 Windows
unit/reference tests. The full native regression returns 519 matches, 87
existing unused-storage warnings, zero differences/errors and no skipped inputs.
Debug, sanitizer, Release and MinGW Debug builds pass without new warnings.
No game code, saved-file layouts or baselines changed. The legacy Wine tutorial
remains deferred to milestone 4's native runner. Basic API coverage stays
178/178 (100%); this batch does not complete milestone 2.
Two focused Release tests (list and race workflow) and a direct Release list
check on a private X11/Openbox display also pass. UI suites ran sequentially.

The edit case-style follow-up normalizes initial text and `WM_SETTEXT`, in
addition to inserted text. Shared SDK/native tests cover uppercase/lowercase
single-line and multiline controls, read-only replacement, input-limit bypass,
modified/undo reset and notification counts. The native test failed on initial
text before the repair, while the SDK reference passed. Native assertions also
cover CP1252 accented text at creation and programmatic replacement. The tests
leave input strings unchanged and normalize the control's owned text buffer.

A diagnostic SDK/Wine probe returns TRUE for empty undo in both single-line
and multiline controls, and returns TRUE/FALSE for read-only undo in single-line/
multiline controls respectively. Before the follow-up below, the native runtime returned FALSE
for both empty history and read-only undo. Microsoft's
[EM_UNDO documentation](https://learn.microsoft.com/en-us/windows/win32/controls/em-undo)
requires TRUE for single-line controls and success/failure for multiline ones;
the subsequent undo qualification below covers return values, selection,
modified state, notification counts and retained history.

Case-style batch verification passes 97/97 fatal ASan/UBSan/LSan tests and 51/51
Windows unit/reference tests. Full native regression returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. Debug, sanitizer, Release and MinGW Debug builds pass with no new
warnings. No game code, saved layouts or baselines changed. Basic API coverage
remains 178/178 (100%); milestone 2 is still in progress.
Two focused Release tests (edit and race workflow), plus a direct Release edit
check on a private X11/Openbox display, also pass. UI suites ran sequentially.

## Loaded-game modal and resize qualification

Hierarchy-driven loaded-game workflows exercise research radio changes through
commit/reopen/restore, and production Add through Cancel, OK and reopening. SDL
mouse events activate inspected controls; owner restoration and stale window-ID
rejection are checked after every modal. Temporary fixture copies protect the
frozen inputs. These workflows exposed default dialog handling replacing an
application's zero `EndDialog` result, and private-X11 scanner resize refreshes
replaying captured old zoom values indefinitely. The dialog runtime now respects
an ended dialog, and the scanner resolves current zoom at refresh dispatch.
Shared SDK/native dialog assertions and headless/private-X11 workflows pass;
full qualification passes 97/97 fatal ASan/UBSan/LSan tests, 51/51 Windows
units/reference tests and 13/13 Linux host units. Windows-native and host
regressions each return 519 matches, 87 existing unused-storage warnings, zero
differences/errors and no skipped inputs. Debug, sanitizer, Release and MinGW
Debug builds pass with no new warnings in touched files. Existing test-harness
format warnings remain. No turn-generation logic, saved layouts or baselines
changed. The legacy Wine tutorial remains deferred to milestone 4's native
runner. Basic API coverage remains 178/178 (100%).
Three focused Release checks (dialog, loaded-game headless and loaded-game
private X11) also pass. All UI suites ran sequentially. Broader gameplay
workflows, the native tutorial and desktop qualification remain outstanding.


## Battle-plan and player-relation workflows

The loaded-game harness now drives the Dump Cargo checkbox and tactic combo
with translated SDL mouse events. Close must persist the changed setting;
reopening verifies it before restoring the original. Full plan bytes must match
after restoration. Player-relation radio changes must update the selected
player immediately, survive Close/reopen and restore the original relation.
The copied single-player human/AI fixture must reject the Relations menu command;
its game-mode flag is then switched only in memory to test multiplayer diplomacy,
and restored afterward. Loading a multiplayer save remains unqualified.
Owner enablement and stale inspection-ID rejection are checked for every modal.

Debug headless/private-X11 checks and all 97 fatal ASan/UBSan/LSan tests pass.
All 51 Windows unit/reference tests pass. Native regression returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. Debug, sanitizer, Release and MinGW Debug builds pass without new
warnings in touched files. This extension changes tests and documentation only;
no runtime behavior, saved layout or baseline changed. All three focused Release
checks (dialog, loaded-game headless and loaded-game private X11) also pass. UI
suites ran sequentially. Basic API coverage remains 178/178 (100%); broad gameplay,
control and native tutorial qualification remain incomplete. Desktop visual
feedback is deferred in the goal report at the user's request.


## Ship-designer edit transitions

The loaded-game harness copies an existing ship design, cancels the copy, then
copies and accepts a second design after translated SDL text input. It reopens
the copy for cancelled and accepted renames, reopens again and deletes the test
design. Free-slot flags, original active designs and the player's design count
must be restored. Drawing materializes the lazy combat-power cache; the check
verifies that field against `LComputePower` instead of mistaking it for an edit.
Every closed modal restores its owner and invalidates its inspection ID.

The workflow exposed default command handling ending the whole designer when
OK/Cancel only switches out of edit mode. New SDK/native modal tests post each
unhandled command followed by explicit completion; only the latter may end the
modal. The SDK passes and the old native handler fails. The repaired runtime
leaves OK/Cancel termination to the application. Focused Debug headless and
private-X11 checks pass. The first full fatal-sanitizer run passed 96 tests but
timed out at the final deletion workflow's 30-second process boundary. The
expanded suite now allows 45 seconds per copied game, retaining each modal's
watchdog and all assertions. Outer headless/X11 limits cover both path cases.
The full rerun passes all 97 fatal ASan/UBSan/LSan tests, all 51 Windows
unit/reference tests and all 13 Linux host units. Native and host regressions
each return 519 matches, 87 existing unused-storage warnings, zero
differences/errors and no skipped inputs. All three focused Release checks
(dialog, loaded-game headless and private X11) pass. Debug, sanitizer, Release
and MinGW Debug builds pass without new warnings in touched files. UI suites
ran sequentially. No game logic, saved layout or baseline changed; the legacy
Wine tutorial remains deferred to the native runner. Desktop appearance feedback is saved for later, and hull/component/starbase workflows
remain unqualified. Basic API coverage stays 178/178 (100%).


## Empty-history and read-only undo

Shared SDK/native tests now qualify both `EM_UNDO` and `WM_UNDO` on single-line
and multiline controls with empty history and read-only fields. Empty editable
undo succeeds, moves the selection to its retained undo position, preserves text
and modified state, keeps history empty and sends `EN_CHANGE` without `EN_UPDATE`.
Clearing history and setting new text preserve the position, clamped to the new
text length. Read-only undo succeeds for single-line fields and fails for
multiline ones, preserves text/selection/modified state and history, and sends
no update/change/max-text notifications. Re-enabling editing permits the original
undo. Empty-history Ctrl+Z has the same selection and change-notification
effects, verified with a shared key/character sequence. The initial full
validation was intentionally stopped to investigate shortcut behavior; the SDK
probe disproved an initial quiet-shortcut assumption, and the final assertion
matches its observable effects. The old native runtime failed the first
empty-history assertion; focused
Debug and SDK reference tests now pass. Final qualification passes 97/97 fatal
ASan/UBSan/LSan tests and 51/51 Windows unit/reference tests. Native regression
returns 519 matches, 87 existing unused-storage warnings, zero differences/errors
and no skipped inputs. Debug, sanitizer, Release and MinGW Debug builds pass
without new warnings in touched files. Two focused Release checks (edit and race
workflow) and a private-X11 Release edit check also pass. UI suites ran
sequentially. No game code, saved layout or baseline changed. The legacy Wine
tutorial remains deferred to the native runner.

Microsoft's [EM_UNDO contract](https://learn.microsoft.com/en-us/windows/win32/controls/em-undo)
specifies the single-line return rule; the SDK/Wine probe supplies the empty
history's observable state and notification evidence. Wine's
[primary edit implementation](https://github.com/wine-mirror/wine/blob/master/dlls/user32/edit.c)
was inspected to understand its alias handling and retained position. No source
was copied. The position is internal window state, never a saved-file field.
Basic API coverage remains 178/178 (100%); wider undo and control qualification
still remains.


## Text-changing undo/redo and application styles

The application uses single-line edits and multiline message fields, fixed
owner-drawn lists, a plain multiple-selection merge list and dropdown-list
combos. The resource/dynamic construction audit finds no variable-height or
multicolumn list declarations. Those broad runtime styles are not evidence of
completed application control qualification; the application workflows still
need their remaining checks.

Shared SDK/native assertions now cover `EM_UNDO` and `WM_UNDO` on single-line
and multiline controls when text changes. Undo/redo sets the modified flag even
when the caller cleared it immediately before undo. Notifications arrive as
`EN_UPDATE`, `EN_CHANGE`, then another `EN_CHANGE`: the first pair sees the
collapsed replacement caret, and the final change sees the restored range.
Insertion, identical replacement and deletion cases verify undo/redo selections.
Destruction in the update callback must stop subsequent native notifications
without accessing the destroyed control. The SDK reference passes, the old
native handler fails the modified-state assertion, and focused native checks
pass after repair. Final qualification passes 97/97 fatal ASan/UBSan/LSan tests
and 51/51 Windows unit/reference tests. Native regression returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. Two focused Release checks (edit and race workflow) and a private-X11
Release edit check also pass. Debug, sanitizer, Release and MinGW Debug builds
pass without new warnings in touched files. UI suites ran sequentially. No game
code, saved layout or baseline changed; the legacy Wine tutorial remains
deferred to the native runner.

Undo retains explicit before/after replacement lengths; comparing snapshots
cannot identify an identical replacement's intended selection. These fields
belong to internal native window state, never a saved record. The
[modified-flag documentation](https://learn.microsoft.com/en-us/windows/win32/controls/em-getmodify)
explains why an edit sets the flag; SDK/Wine notification/selection traces supply
the remaining observed contract. Basic API coverage remains 178/178 (100%).


## Application multiple-selection fleet list

The fleet profile opens the application's actual Merge dialog with an SDL
mouse click on its fleet-panel button. Hierarchy inspection locates the plain
multiple-selection list and dialog controls. Select All/Unselect All and two
individual row clicks must produce the expected selected counts and row states.
Cancel preserves copied fleet state; OK merges exactly two fleets while
preserving aggregate ships, cargo and fuel. Focused Debug dummy/private-X11
checks pass. This covers the application's merge-list style, not all possible
listbox styles or the full fleet-order workflow.

Final fleet-batch verification passes 99/99 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and both focused Release fleet checks.
Full native regression returns 519 matches, 87 existing unused-storage warnings,
zero differences/errors and no skipped inputs. Debug, sanitizer, Release and
MinGW Debug builds pass without new warnings in touched files. UI suites ran
sequentially on private displays. No application code, saved layout or baseline
changed. The legacy Wine tutorial remains deferred to the native runner.


## Fleet checkbox and custom split controls

Repeat Orders now has an application workflow assertion: SDL mouse clicks must
change the auto-checkbox, selected fleet and stored fleet consistently. Fleet
reselection must preserve the change, and a second click restores it. The fleet
profile uses a 900×700 frame because the smaller startup-test frame clips this
control below the fleet panel. This does not qualify all constrained layouts.

Split checks operate an enabled custom transfer arrow using its application
`BTN` rectangle. The pending source/target counts must change by exactly one
ship. Cancel removes the temporary target; OK preserves it with the transferred
ship. Both paths conserve aggregate ship counts, cargo and fuel. Focused Debug
headless checks pass; ordinary cargo transfer and waypoint editing still need
coverage. These checks complement child-window hierarchy inspection for drawn
controls that have no individual HWND.

The focused automated profile can be run after building `sdl-debug`:

```sh
ctest --test-dir dist/sdl-debug -j 1 --output-on-failure \
  -R '^win32_client_fleet_(dummy|x11)$'
```

The X11 test launches its own Xvfb/Openbox display. The dummy test displays no
windows. Both run relative and absolute Continue Game paths against temporary
fixture copies. Keep `-j 1` when running UI suites.

The new Split workflow exposed a fatal UBSan alignment error in `DrawBtn`:
packed `BTN` arrays can place Win32 `RECT` members on two-byte boundaries.
`DrawBtn` now copies the input rectangle into aligned local storage before
drawing; `InitBtnTrack`, `FTrackBtn` and `FTrackXfer` also use byte-safe copies
before rectangle access/hit testing. The test's drawn-control input helper
likewise copies packed rectangles before reading their coordinates. Button
records, layouts and saved record widths remain unchanged. The first full
sanitizer batch failed both fleet profiles on this error; other pass counts
from that batch are not a complete qualification result.

Final repeat-order/split qualification after the alignment repair passes all
99/99 fatal ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests and both
focused Release fleet profiles. Full native regression returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. Debug, sanitizer, Release and MinGW Debug builds pass without new
warnings in touched files. All UI suites ran sequentially on private displays.
No core game logic, packed record size, saved layout or regression baseline
changed. The legacy Wine tutorial remains deferred to the native runner; cargo
transfer, waypoint editing and broader workflow qualification remain pending.


## Cargo arrows, waypoint task combo and empty scrollbar paint

The fleet profile exercises planet/fleet mineral Transfer with real SDL mouse
input: Cancel preserves both cargo arrays, OK moves one unit with equal/opposite
stored changes, and reopening/reversing restores the arrays. A waypoint task
combo changes None to Transport, survives fleet reselection, and restores the
original order bytes when switched back to None. Other transfer types, route
creation/drag/deletion and transport presets remain unqualified.

The combined workflow exposed timer starvation from a dirty zero-width
scrollbar: the scrollbar paint handler returned before validating its region.
It now begins/ends paint even when geometry is empty. Shared SDK/native scroll
assertions invalidate a normal bar, resize it to zero width or height, paint and
require the update region to clear, then verify painting after restoring size.
The previous native handler fails the zero-width assertion; the repaired
headless scroll test and combined fleet profile pass.

Final cargo/task/empty-scrollbar qualification passes 99/99 fatal
ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests and all three focused
Release checks (scrollbar, headless fleet and private-X11 fleet). Full native
regression returns 519 matches, 87 existing unused-storage warnings, zero
differences/errors and no skipped inputs. Debug, sanitizer, Release and MinGW
Debug builds pass without new warnings in touched files. UI suites ran
sequentially on private displays. No core game logic, saved layout or baseline
changed. The legacy Wine tutorial remains deferred to the native runner; route
creation/drag/deletion, transport presets and broader cargo workflows remain
outstanding. Basic API coverage stays 178/178 (100%).


## Transport popup and scanner waypoint input

The fleet profile uses a right-click on the drawn transport trigger and SDL
keyboard navigation in the real popup for QuikLoad, QuikDrop, WaitLoad and
Clear. Stored actions must match each choice; WaitLoad quantities are 100 and
Clear resets actions/quantities to zero. The popup wrapper supplies input events
only and delegates menu execution to the compatibility implementation.

Scanner snapshots supply visible unfiltered planet coordinates for Shift-click
append and a plain mouse destination drag. Both actions must reach the intended
planet ID and universe coordinates through the normal scanner handler. A
waypoint-list row click followed by Delete uses the same key/character handlers
as the main game loop and restores the original order. Focused headless Debug
checks pass. Multiple route legs, Ctrl gestures and customized transport
presets still need coverage; desktop visual checks are deferred.

The first full Windows batch passed 50/51 tests but failed the unchanged
attention reference at its post-`FlashWindow` active/focus assertion. A fresh
private-display run of that same test passes. This suggests an intermittent
Wine/window-manager focus issue, but its cause remains unproven; the assertions
have not been weakened. The complete Windows gate is rerunning, and the first
failure remains part of the qualification record.

Final transport-preset/route qualification passes 99/99 fatal ASan/UBSan/LSan
tests and both focused Release fleet profiles. The complete Windows rerun passes
51/51 unit/reference tests; the initial 50/51 attention/focus failure and passing
isolated recheck remain recorded above. Full native regression returns 519
matches, 87 existing unused-storage warnings, zero differences/errors and no
skipped inputs. Debug, sanitizer, Release and MinGW Debug builds pass without
new warnings in touched files. UI suites ran sequentially on private displays.
No application code, saved layout or baseline changed in this extension. The
legacy Wine tutorial remains deferred to the native runner. Multiple route legs,
Ctrl gestures, customized presets, broader cargo workflows and the complete
native tutorial remain outstanding. Basic API coverage stays 178/178 (100%).
