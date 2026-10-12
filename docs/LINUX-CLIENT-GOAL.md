# Linux client completion goal

The intro screen now uses a centered, decorated desktop window instead of
fullscreen, including return-to-title and failed-load paths. Debug and fatal
sanitizer startup/race-wizard tests pass on dummy and private X11 displays;
the native file-workflow tests also pass. The main game layout is unchanged.

Current work order: graphics dependency leak repair, gameplay workflows (3),
UI polish (8), then Flatpak/CMake packaging (9), with local commits for completed
batches and no pushes. Desktop/platform and immediate Unicode path/password
implementation batches are committed. The saved broader goal
is paused; tutorial expansion is paused. Existing tutorial work and artifacts
are retained, but this product batch does not run or extend tutorial profiles.
Previously selected scope: milestones 1, 2, 3, 4, 7, 8 and 9.
PDF printing/viewer integration and offline help verification are deferred.
Native filename/password encoding is implemented and qualified by 108 fatal
sanitizer cases, 55 Windows tests, 15 host tests, 11 Release checks and both
519-checkpoint regressions; see [encoding contracts](UNICODE-BOUNDARIES.md).
Basic API coverage is 178/178 (100%); this measures API bodies, not completion
of the behavior and release requirements below.
The graphics leak is now traced to Mesa's empty runtime-assembler executable
heap; a private dependency patch passes concurrent lifetime/unload tests,
SDL-only reproduction and all six native Wayland sanitizer cases. The matching
unpatched build still reproduces 112 bytes. System drivers remain unchanged;
see [repair and explicit qualification prefix](MESA-GRAPHICS-LEAK.md).
The completed graphics batch passes 115/115 fatal sanitizer cases (including
seven Wayland cases), 55/55 Windows tests and all 519 native regression
comparisons, with 87 existing warnings and no differences/errors or skips.

| Order | Milestone | Status | Completion evidence needed |
| --- | --- | --- | --- |
| 1 | Open/save dialog overhaul | Implementation complete; desktop/macOS qualification pending | Native selection, validation/retries/errors, Unicode game/race I/O and fullscreen ownership pass automated tests; saved compositor feedback and macOS execution remain unverified |
| 2 | Application-relevant control behavior | Complete for Stars' used styles | Shared SDK/native control contracts, application workflows, fatal sanitizers and regression checks pass; generic extensions outside the product scope remain documented |
| 3 | Gameplay UI workflow testing | Critical product workflows automated and qualified | Designer component/hull/starbase, Save/MRU reload and Basic/Advanced creation join the existing gameplay matrix; real-desktop acceptance remains milestone 7 |
| 4 | Native tutorial runner | In progress | Full tutorial driven in process through hierarchy and map inspection, without AutoHotkey; checkpoint/action evidence and failure artifacts |
| 7 | Wayland and desktop qualification | Native Wayland implementation and Debug tests added; qualification incomplete | Native Wayland positions/popups, focus/capture, scale/multi-monitor and GPU/software presentation; documented platform limits |
| 8 | UI polish | Current product polish implemented and qualified | Message wheel navigation and scrollable planet/fleet sections pass dummy/private X11 checks, fatal sanitizers, Windows units and unchanged native regression; desktop feedback remains queued |
| 9 | Flatpak and relocatable Linux packaging | Pending | Flatpak manifest plus CMake install rules, resources/fonts/libraries, launch outside repository and clean-install verification |

### Product completion scope for milestones 1 and 2

Milestone 1 delivers Linux native open/save selection and the existing Windows
standard dialogs, preserving application filename, filter, cancellation and
validation contracts. Linux portal placement and scaling belong to the desktop
picker; Stars supplies its correct parent and retains owner visibility/input
modality. The macOS adapter now has an x86_64 SDL product build preset; a Mac must
qualify compilation and execution. Native Wayland Debug tests now pass on private
Weston at integer scales; the unresolved SDL-only graphics-stack LSan report and
real desktop/hardware acceptance remain milestone 7. See [desktop qualification](DESKTOP-QUALIFICATION.md).

Milestone 2 covers the control styles used by Stars' resource templates and
dynamic UI: single-line edits, automatically scrolling multiline message edits,
fixed-height lists (including ordinary multiple selection and single-selection
owner draw), combos, buttons/radios, menus/toolbars and scrollbars. Generic
Win32 extensions not used by Stars (variable-height/multicolumn lists, complex
IME and owner-drawn multiple-selection action sequences) are explicitly outside
this product acceptance scope. Short multiline fields without automatic
scrolling are also outside it; the application's message edits use
`ES_AUTOVSCROLL`. Broad gameplay workflow coverage remains milestone 3.

The current product batch adds fullscreen native-modal ownership tests in dummy
and private X11, long exported Wayland parent-handle wire tests, Unicode
game paths and actual portal-driven game open/cancel and race save/cancel/reload
and race-only open. Long race basenames exposed unchecked copies into
`szRaceFile[16]`; both UI paths now use a bounded setter, without altering the
chosen path or saved records. The legacy cache retains its size and complete
UTF-8 characters; separate UI storage retains the full native suggestion.
Repeat saving checks the actual portal `current_name` and validates the reloaded
race checksum. Native backend errors and oversized selected paths show a visible
error; cancellation stays quiet and preserves caller state.

Additional shared edit tests exposed mixed-direction deletion and
replacement-followed-by-typing undo differences. The SDK restores the complete
adjacent deletion/replacement group; the native implementation now does too.
SDK and focused Debug checks pass, including a short automatically scrolling
multiline viewport. Final sequential qualification passes 102/102 fatal
ASan/UBSan/LSan non-tutorial tests, 52/52 Windows unit/reference tests under Wine,
13/13 Linux host units and 16/16 focused Release checks. Windows-native and
Linux-host regressions each return 519 matches, 87 existing unused-storage
warnings, zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW builds pass without new warnings in touched files. No baseline
or on-disk layout changed. All private-display test processes have completed.

The first full sanitizer attempt passed 101/102, with the expanded four-path
Continue test hitting its old combined 100-second wrapper deadline. Each child
retains its original 45-second limit; the combined allowance is now 200 seconds.
The final full run passes all 102 cases, including the X11 Continue case in
123.51 seconds. The later full-name/error-feedback changes are included in that
final run, not inferred from the earlier batch.

The earlier legacy Wine tutorial failure and saved desktop title-screen feedback
remain separate outstanding qualifications. No tutorial profiles were executed
or extended in this product batch. The title repair passes private X11 ownership
checks but needs confirmation on the user's compositor; macOS remains untested
on this Linux machine. These checks stay queued for later, as requested.

Packaging prerequisite audit: `flatpak`, `appstreamcli` and
`desktop-file-validate` are installed; `flatpak-builder` is absent. The local
runtime list has Freedesktop Platform 26.08 and SDK 25.08, so the packaging
verification needs a matching SDK/platform pair as well as the builder. Save
`sudo apt install flatpak-builder` for when the packaging milestone needs it;
no installation or desktop prompt was requested during the tutorial test run.

Windows and Linux host builds, save layouts and game behavior must remain intact.
UI suites run sequentially; desktop automation uses private displays. Coverage
and milestone completion are reported separately. Unavailable desktop hardware
or compositor checks must be recorded as unverified rather than claimed passed.

## Milestone 1 work log

The first implementation batch adds resizable picker layout, editable location,
Back/Up/Home navigation, hidden-file visibility and cancellable folder creation. Tests cover navigation,
history, resizing, hidden entries and existing filename contracts. The first batch passes 96/96 fatal ASan/UBSan/LSan tests, 51/51 Windows units,
2/2 focused Release picker tests and the final Debug picker capture check.
The native regression returns 519 matches, 87 unused-storage warnings, zero
differences/errors and zero skipped inputs. No baseline changed. Offscreen
layout was visually reviewed. This batch alone does not complete milestone 1.

The user selected native file pickers: Linux `org.freedesktop.portal.FileChooser`
through libdbus, standard Windows dialogs and a macOS native backend behind
platform defines. This supersedes further custom-picker redesign for ordinary
desktop use. Preserve the compatibility picker for explicit deterministic tests.

Linux portal implementation and its private-bus protocol test are added.
The macOS AppKit bridge is added but unverified on this Linux-only host.
`--headless` keeps the compatibility picker invisible. Native desktop picker,
actual game/race workflows and macOS qualification are still pending; Wayland
parent export remains in milestone 7. The updated batch passes all 97 tests with
fatal ASan/UBSan/LSan, including the private-bus portal fixture, and all 51 Windows
unit/reference tests. Debug, sanitizer, Release and MinGW Debug builds pass.
The full native regression returns 519 matches (87 existing unused-storage
warnings), zero differences/errors and zero skipped reference inputs. No baseline
changed. All three focused Release picker/portal tests pass. The legacy Wine
tutorial remains deferred to milestone 4's native runner.

The next file-selection fix retains the folder, filename and filter after a
missing-file validation error or declined overwrite. Directory selections become
the retry folder. Private-bus assertions pass under fatal sanitizers and check
that overwrite refusal leaves the existing file untouched. Full batch
verification passes: 97/97 fatal sanitizer tests, 51/51 Windows units/reference
tests, and 519 native regression matches (87 unused-storage warnings, zero
differences/errors/skips). Three focused Release picker tests also pass. A final
focused sanitizer fixture verifies backend method errors and failed portal
responses return an explicit dialog error, preserve caller output and restore
the owner. No game results or baseline changed.

The user verified the rebuilt Debug client on the real Linux desktop: File →
Open and race saving show native dialogs, succeed, and cancel without affecting
the game. This supplies the desktop workflow evidence for ordinary open/save;
macOS qualification and Wayland parent export remain unverified. The retry
batch passes all 97 fatal sanitizer tests and the qualifications above. Native
portal placement/scaling and the title-screen feedback below remain open.

## Milestone 2 work log

Owner-drawn lists now send focus actions on focus gain/loss before list focus
notifications, and paint an empty focused list through `itemID == -1`. The SDK
comparison proves nonempty focus state and ordering under Wine and reproduced
the missing native callback before the fix. Native checks cover empty row pixels
and callback reset/destruction. Full batch verification passes 97/97 fatal
sanitizer tests, 51/51 Windows unit/reference tests, and 519 native regression
matches (87 existing unused-storage warnings, no differences/errors/skips).
Debug, Release and MinGW Debug builds pass. Two focused Release tests (list and
race workflow) and a direct private-X11 Release list check pass. No baseline
changed. Selection
actions, edit notification/undo boundaries and remaining control workflows stay
open; this does not complete milestone 2.

The edit undo follow-up groups adjacent Backspace/Delete operations and restores
their complete deleted range. Rejected input at the text limit preserves prior
undo history without adding an empty record; SDK notifications and modified
state remain intact. Shared single/multiline tests reproduced the old differences
and cover cursor barriers, undo/redo and CRLF deletion when both lines are visible.
Short multiline fields without automatic scrolling behave differently in the
SDK probe and remain an explicit qualification item. Full verification passes
97/97 fatal sanitizer tests and 51/51 Windows unit/reference tests. The native
regression returns 519 matches, 87 existing unused-storage warnings, zero
differences/errors and zero skipped inputs. Debug, Release and MinGW Debug builds
pass without new warnings. No saved layouts, game results or baseline changed.
Two focused Release tests (edit and real race workflow) and a direct private-X11
Release edit check also pass. UI suites ran sequentially.

The next control batch adds synchronous single-selection owner-draw selection
actions for old/new visible rows. Shared SDK/native assertions cover selection,
clearing, repeated selection and keyboard changes. Native assertions reset and
destroy the list from a drawing callback. Multi-selection action sequences,
caret-focus transitions and broader drawing-state qualification remain open.
Full batch verification passes 97/97 fatal ASan/UBSan/LSan tests and 51/51
Windows unit/reference tests. The native regression returns 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. Debug, Release, sanitizer and MinGW Debug builds pass without new
warnings. No game code, saved layouts or baseline changed. Two focused Release tests
(list and race workflow) and a direct private-X11 Release list check also pass.
UI suites ran sequentially.
Basic API coverage remains 178/178 (100%); milestone 2 is still in progress.

Upper/lower edit styles now normalize initial and programmatically set text,
including read-only fields. Shared SDK/native checks cover both line modes,
notification counts, input-limit bypass and modified/undo reset; native checks
also cover CP1252 accented initial/programmatic text. The SDK reference passed
while the old native implementation failed at initial text. Focused tests and
all builds pass without new warnings. Full batch verification passes 97/97
fatal sanitizer tests and 51/51 Windows unit/reference tests. Native regression
returns 519 matches, 87 existing unused-storage warnings, zero differences/errors
and no skipped inputs. No baseline changed. Empty/read-only undo return values
and broader workflow/control edges remain open. Two focused Release tests
(edit and race workflow), plus a direct private-X11 Release edit check, pass.
UI suites ran sequentially.

Empty-history/read-only undo follow-up tests cover `EM_UNDO` and `WM_UNDO` for
single-line and multiline fields. Empty editable undo returns success, restores
the retained/clamped undo position, preserves text and modified state, leaves
history empty and sends only `EN_CHANGE`. Read-only undo preserves pending
history, selection and text; its result is true for single-line and false for
multiline fields, with no notifications. Re-enabling editing permits the prior
undo. Empty-history Ctrl+Z has the same selection and change-notification
effects, verified by a shared keyboard/character assertion. The first validation
batch was stopped to investigate this behavior; the SDK disproved an initial
quiet-shortcut assumption.
Focused native and SDK reference assertions pass after reproducing the old
native failure. Final verification passes 97/97 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and all 519 native regression matches,
with 87 existing unused-storage warnings and zero differences/errors/skips.
Two focused Release checks (edit and race workflow), plus a private-X11 Release
edit check, pass. Debug, sanitizer, Release and MinGW Debug builds pass without
new warnings in touched files. UI suites ran sequentially. No game code, disk
layout or baseline changed. The legacy Wine tutorial remains deferred to the
native runner; this does not complete milestone 2.

Text-changing undo/redo now sets the modified flag and selects the actual
restored replacement range. Explicit before/after lengths retain identical
replacement ranges too. Shared single/multiline SDK/native assertions verify
update/change/final-change ordering and the selection/modified state seen by
each callback, plus safe destruction during an update. The old native handler
failed the modified-state assertion; the SDK and repaired focused native tests
pass. Final verification passes 97/97 fatal ASan/UBSan/LSan tests and 51/51
Windows unit/reference tests. Native regression returns 519 matches, 87 existing
unused-storage warnings, zero differences/errors and no skipped inputs. Two
focused Release checks (edit and race workflow), plus a private-X11 Release edit
check, pass. Debug, sanitizer, Release and MinGW Debug builds pass without new
warnings in touched files. UI suites ran sequentially. No game code, saved layout
or baseline changed. The legacy Wine tutorial remains deferred to the native
runner. Application style auditing finds fixed owner-drawn and plain multiple-
selection lists, dropdown-list combos and single/multiline edits; no variable-
height or multicolumn list construction is declared by the application.

## Milestone 3 work log

Loaded-game tests now find modal windows and controls through copied hierarchy
snapshots, resolve stable IDs and drive button/radio actions through SDL mouse
events. Research changes commit, reopen with the persisted radio choice, and
restore the original choice. Production Add changes the working queue; Cancel
must preserve the original queue and research flag. A second edit accepts with
OK and reopens with the committed item count. Each modal must restore its owner
and invalidate its inspection ID after closing. Tests operate only on temporary
copies of frozen fixtures, in dummy/headless and private X11 environments.

The production cancellation assertion exposed a compatibility bug: the dialog
callback ended with result zero and returned false, so default command handling
replaced the result with nonzero `IDCANCEL`. The runtime now preserves an ended
dialog's result. Focused shared SDK tests verify zero/custom results despite a
false callback return; the old native implementation failed the zero-result
assertion. Private X11 also exposed scanner resize refreshes replaying stale
zoom commands indefinitely and starving the new modal timer. `ScannerWndProc`
now posts a scanner refresh message and reads the current zoom at dispatch,
instead of queuing a captured zoom choice. A debugger observed alternating old
zoom commands while the modal timer never ran; the same private-X11 workflows
pass after the repair. Full batch qualification passes 97/97 fatal
ASan/UBSan/LSan tests, 51/51 Windows units/reference tests and 13/13 Linux host
units. Both Windows-native and host regressions return 519 matches, 87 existing
unused-storage warnings, zero differences/errors and no skipped inputs. Debug,
sanitizer, Release and MinGW Debug builds pass with no new warnings in touched
files. Existing test-harness format warnings remain. No baseline or saved-file
layouts changed; game turn generation is unchanged. The legacy Wine tutorial
remains deferred to milestone 4's native runner. Three focused Release checks
(dialog, loaded-game headless and loaded-game private X11) pass. All UI suites
ran sequentially. Other gameplay workflows and the full native tutorial remain
outstanding. Next workflow coverage should include ship design/edit/accept, fleet orders/cargo routing,
message navigation and new-game completion, then the native tutorial runner.

Battle-plan workflows now exercise the Dump Cargo checkbox and tactic combo
through translated SDL mouse input, Close, reopen and restoration. Full saved
plan bytes must match after restoration. Relation workflows exercise radio
changes, immediate game-state updates, Close/reopen and restoration; the copied
human/AI fixture is switched to multiplayer mode only in memory for this check,
then restored. Single-player mode must reject the Relations command without
opening a modal. These checks do not qualify opening a multiplayer save file.
Focused Debug headless and private-X11 checks pass. The extension passes all
97 fatal ASan/UBSan/LSan tests and all 51 Windows unit/reference tests. Full
native regression returns 519 matches (87 existing unused-storage warnings),
zero differences/errors and no skipped inputs. Debug, sanitizer, Release and
MinGW Debug builds pass without new warnings in touched files. No application
code, file layout or baseline changed in this extension. All three focused
Release checks (dialog, headless and private-X11 loaded-game workflows) pass.
All UI suites ran sequentially.

Ship-design workflows now copy an existing design, cancel the copy, accept a
second copy with a name entered through SDL text input, reopen it, cancel and
accept further renames, then delete the temporary design. Original active
design bytes must remain unchanged except for the lazy combat-power cache,
which drawing computes and the test verifies against `LComputePower`.
Free-slot flags and the player's design count must return to their originals.
The new workflow exposed default OK/Cancel handling ending the entire designer
when its callback only leaves edit mode. Shared SDK/native assertions require
unhandled commands to leave the modal running until an explicit completion;
the SDK passes while the previous local handler fails. Default command handling
now leaves termination to the application. Focused Debug headless and private-X11
checks pass. The first fatal-sanitizer batch passed 96 tests but hit the
30-second per-process limit in the final ship-deletion workflow. Process limits
are now 45 seconds, with 100/110-second outer limits for two copied-game runs;
per-modal watchdogs and all assertions remain intact. The full rerun passes
97/97 fatal ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests and
13/13 Linux host units. Native and host regressions each return 519 matches,
87 existing unused-storage warnings, zero differences/errors and no skipped
inputs. All three focused Release checks (dialog, headless and private-X11
loaded-game workflows) pass. Debug, sanitizer, Release and MinGW Debug builds
pass without new warnings in touched files. UI suites ran sequentially. No game
logic, saved layout or baseline changed. The legacy Wine tutorial remains
deferred to milestone 4's native runner. Hull/component editing,
starbase design and other broad gameplay workflows remain outstanding.

## Manual feedback saved for later

Product milestone 3 now adds hull selection and component drag/remove/reinsert
for ships and starbases, cancellation/acceptance/deletion with original-design
preservation, actual Save/MRU reload and Basic/Advanced creation. All five focused
cases and the full 120-case fatal sanitizer gate pass, alongside 55 Windows
tests and 519 matching native regression comparisons. See
[gameplay matrix](GAMEPLAY-QUALIFICATION.md). Tutorial expansion remains paused.

The user is leaving the agent running and requests that further manual testing
feedback/checks be saved for later rather than requested during unattended work.

- Review tutorial start, unfinished-task rejection, map input, research and
  factory production, the initial generations, scouting routes, remote
  mining, the summary popup, colonist cargo loading, the Colonize task, message
  filtering, automatic production, QuikDrop and waypoint deletion
  on the real desktop after native automation qualification. Keep this deferred along with
  all further manual feedback; private-display smoke tests do not qualify the
  desktop compositor.

- Opening the native file dialog from the title screen makes the title screen
  disappear. The game does not explicitly hide or destroy it before selection;
  investigate fullscreen focus/compositor handling and native dialog ownership.
  On Wayland the portal currently receives no parent identifier (milestone 7).
  A real desktop check of the eventual repair remains pending. Do not claim this
  fixed solely from private X11 tests.
  Source trace: `FFileDialog` disables its root through `EnableWindow` before
  opening the portal; `EnableWindow` changes SDL native focusability. Neither
  path explicitly hides the title window. Check fullscreen compositor behavior
  at this focusability transition and the missing Wayland portal parent export;
  this is an investigation lead, not a proven cause or repair.
  Bundled SDL 3.4.18 source (`SDL_ShouldMinimizeOnFocusLoss`) defaults to
  minimizing only exclusive fullscreen. The wrapper uses desktop fullscreen
  without `SDL_SetWindowFullscreenMode`; no minimize-on-focus-loss hint is
  set here. Default SDL minimization is therefore not established as the cause;
  user environment overrides and actual compositor behavior remain unverified.
  `TitleWndProc` calls `FOpenGame(hwnd, FALSE)` and destroys the title only
  after success; `FOpenGame` passes that title as the dialog owner and does
  not hide it before `GetOpenFileName`. This source path does not establish
  the reported disappearance as an intentional title-screen destroy/hide.
  Further bundled-source evidence: SDL 3.4.18 publishes
  `SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING` from its
  xdg-foreign export callback. Its own portal adapter prefers that handle
  with the `wayland:` prefix, falling back to X11. Our `filedialog_portal.c`
  reads only `SDL_PROP_WINDOW_X11_WINDOW_NUMBER`. A parent-handle follow-up
  can use SDL's existing property without adding another Wayland library;
  handle readiness/lifetime and private portal assertions still need tests.
  This omission remains a source finding, not a proven disappearance cause.
- Include Stalwart Defender waypoint deletion and Santa Maria queue insertion
  in the eventual desktop tutorial check. Keep this deferred; the native
  year-2404 automation uses headless and private-X11 input only.
- Include cargo-gauge opening, scanner habitability/normal toolbar clicks,
  Shaggy Dog colonization, transport presets and Repeat Orders in the eventual
  desktop tutorial check. These year-2405/2406 actions remain deferred for
  manual review; no desktop prompt is requested during unattended work.
- Include Red Storm orders, larger/automatic production queues, mine-message
  filtering and the enemy scout's summary design popup in the eventual desktop
  tutorial check. Keep these year-2407 actions deferred during unattended work.
- Check the blue-diamond production menu, preset 1 Import, nested template
  dialog ownership and restoration of the production owner on the real desktop.
  Save this for later; unattended runs use headless and private X11 displays.
- Recheck native dialog placement and scaling on the real desktop after Wayland
  parent export is implemented.
- Recheck research radio/combobox layout and production Add/Cancel/OK on the
  real desktop after automated workflow qualification. Tests currently supply
  headless and private-X11 behavioral evidence; no new manual check was requested.
- Review ship-designer copy/edit/accept/cancel and main/edit transitions on the
  desktop after automated qualification; keep this check deferred too.
- Review transport preset menus and scanner waypoint append/drag/delete on the
  desktop after automated qualification; keep this check deferred too.
- Review cargo transfer and waypoint task dropdown placement/selection on the
  desktop after automated qualification; keep this check deferred too.
- Review fleet Repeat Orders and Split transfer/Cancel/OK on the desktop after
  automated qualification. Check fleet-panel reachability in a short window too.
- At 640×480 the fleet task combo is below its parent's client area and cannot
  receive a mouse click. Mining automation uses a real resize to 900×700; this
  qualifies the larger layout without resolving the short-window issue.
- Qualify programmatic resizing of an already maximized window against the
  Windows SDK. The native mining harness observed SDL rejecting a smaller-size
  request under Openbox; skipping the unnecessary test setup request does not
  establish compatibility of that API behavior.
- Review fleet Merge list selection and placement on the desktop after automated
  qualification; keep this check deferred too.
- Review battle-plan checkbox/combobox and player-relation radio layouts on the
  desktop after automated qualification; keep this check deferred too.
- Include year-2408 ship-transfer arrows, cargo loading, Split All, waypoint
  dragging, environment/research popup presses and research budget arrows in
  the eventual desktop tutorial check. Keep it deferred during unattended work;
  private-display results do not qualify the user’s compositor.
- Include year-2409 message Goto actions, the Construction next-field combo,
  mining-message filter, Oxygen orders and Mozart waypoint drag in the eventual
  desktop tutorial check. Keep it deferred during unattended work.

## Fleet multiple-selection workflow

A separate loaded-game profile now selects an owned fleet through the normal
fleet UI entry, opens Merge using translated SDL mouse input, and locates its
list and buttons through hierarchy inspection. It verifies Select All, Unselect
All, individual row selection and Cancel before reopening and merging two
fleets. Cancellation preserves fleet IDs, locations, ship counts, cargo and
fuel; acceptance removes one owned fleet while preserving aggregate ships,
cargo and fuel. Each run uses a temporary copy of the frozen fixture, once with
a relative game path and once with an absolute path. No user game is modified.

The mouse-injection helpers now include the root menu-bar height when converting
Win32 client coordinates to SDL window coordinates. Dialog-only tests masked
that difference because their roots have no menu. Focused Debug headless and
private-X11 fleet checks pass. Fleet splitting, cargo transfer, repeat orders
and waypoint editing remain unqualified. Native tutorial completion remains
outstanding; basic API coverage stays 178/178 (100%).

Final fleet-batch verification passes 99/99 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and both focused Release fleet checks.
Full native regression returns 519 matches, 87 existing unused-storage warnings,
zero differences/errors and no skipped inputs. Debug, sanitizer, Release and
MinGW Debug builds pass without new warnings in touched files. UI suites ran
sequentially on private displays. No application code, saved layout or baseline
changed. The legacy Wine tutorial remains deferred to the native runner.

## Fleet repeat orders and splitting

The fleet profile now clicks Repeat Orders through SDL input, checks checkbox,
selected fleet and stored fleet state, reselects the fleet to check persistence,
then restores the original flag through another click. It also opens Split
using the real fleet-panel button, locates an enabled drawn transfer arrow
through the application's `BTN` rectangles and checks that one ship moves into
the pending split. Cancel must remove the temporary fleet and preserve total
ships/cargo/fuel. A second split transfers one ship and accepts; the new fleet
must exist with that ship and the aggregate totals must still match. These are
UI actions on temporary fixture copies, not direct edits to fleet orders.

The initial 640×480 startup-test frame clips the Repeat Orders checkbox below
its parent client area despite its visible flag. The fleet profile now resizes
the frame through `SetWindowPos` to 900×700 before operating those controls.
Focused Debug headless checks pass. Low-height fleet-panel reachability needs
further layout qualification; this test-size change does not claim a desktop
layout repair. Cargo transfer, waypoint editing and broader fleet workflows
remain outstanding. Basic API coverage stays 178/178 (100%).

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

## Cargo transfer and waypoint task workflows

The fleet profile now chooses a real owned fleet with available cargo capacity
at an owned planet, opens Transfer using its normal fleet-panel button and
clicks an enabled custom mineral-transfer arrow. Cancel must leave the fleet
and planet cargo records unchanged. Accept must add one mineral unit to the
fleet and remove one from the planet, without changing other cargo slots or
fuel. Reopening and reversing the transfer through the opposite arrow must
restore both complete cargo arrays. Temporary fixture copies supply this state;
no orders are fabricated to make the test pass. Fuel, colonist and fleet/fleet
transfers still need separate workflow qualification.

Waypoint task coverage now chooses Transport through the live task dropdown,
checks selected/stored fleet order state, reselects the fleet to verify the
combo retains it, then restores None and requires the original order bytes.
Waypoint creation, deletion, dragging, modifier input and transport presets
remain outstanding; this task-combo check does not complete route editing.

The first combined run passed cargo/task assertions but stalled while opening
Merge afterward. A debugger stack found the modal loop repeatedly dispatching
paint. A visible zero-width scrollbar retained dirty pixels, while `PaintScroll`
returned before validating them. `PaintScroll` now runs `BeginPaint`/`EndPaint`
even when geometry has no drawable area. Shared SDK/native zero-width and
zero-height assertions were added to the scrollbar test. The old native handler
fails that assertion; focused repaired Debug scroll/fleet checks pass. UI/game
record layouts and baselines are unchanged. Basic API coverage stays 178/178
(100%); full behavioral and release qualification remains incomplete.

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

## Transport presets and scanner route input

The loaded fleet profile now opens the transport-preset popup by right-clicking
its actual drawn trigger. The test-only popup wrapper queues SDL Home/Down/Enter
key events and invokes the real `TrackPopupMenu`; it does not substitute a menu
return value. QuikLoad, QuikDrop, WaitLoad and Clear must produce their expected
stored transport actions. WaitLoad must set mineral quantities to 100%, and
Clear must reset all actions/quantities. Fleet reselection and the original
None-task order restoration remain checked afterward.

Route coverage captures the current scanner snapshot, chooses a visible
unfiltered planet other than the fleet's starting planet, and posts Shift plus
mouse input to append a waypoint. It checks the destination ID/type and universe
coordinates. A fresh snapshot supplies a second destination and new client
coordinates; a normal mouse drag must change the waypoint to that planet while
keeping the order count at two. A real list-row click followed by Delete through
the application's normal key handlers removes the waypoint and requires the
original order bytes/count. No game orders or map coordinates are assigned by
the harness to achieve these outcomes.

Focused Debug headless coverage passes. Multiple appended legs, Ctrl-modified
gestures, presets with user customization, fuel/colonist transfers and the full
native tutorial remain unqualified. Basic API coverage remains 178/178 (100%).

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


## Milestone 4 work log

The native `stars-native-tutorial --smoke` target now drives the actual SDL
client on the UI thread without AutoHotkey. Fresh temporary game/settings
storage, hierarchy IDs and copied map coordinates support real button clicks,
keyboard input and Shift-click orders. The smoke starts Tutorial through New
Game, verifies the real premature-generation rejection, recovers through message
Next/Goto and issues Armed Probe's order to Prune. It reaches page 3 at turn zero
without assigning game orders, tutorial progress or auto-completion flags.

This is 3/80 pages visited (3.75% by page count), not a complete walkthrough or
an overall project-completion estimate. All highlighted actions, checkpoints,
remaining pages and final completion still need implementation and evidence.
The target accepts only the explicit smoke scenario and records
`fullTutorial: false`; failure logs and hierarchy/map artifacts are retained.
See [native tutorial runner](NATIVE-TUTORIAL-RUNNER.md). Manual desktop checks
remain deferred as requested.

This batch passes 101/101 fatal ASan/UBSan/LSan tests, 51/51 Windows
unit/reference tests and both focused Release tutorial smoke profiles. Full
native regression returns 519 matches (87 existing unused-storage warnings),
zero differences/errors and no skipped inputs. Debug, sanitizer, Release and
MinGW Debug builds pass without new warnings. The attention reference passes
in this batch; its prior intermittent failure is still unresolved and retained
in the record above. UI suites ran sequentially on private displays. No game
logic, saved layout or baseline changed. The legacy Wine tutorial is not rerun
or claimed repaired by this native smoke. Basic API bodies remain 178/178 (100%).


The native runner's `--first-year` scenario now completes the five year-2400
pages through real fleet Next selections, Shift-click orders to 90210 and
Alexander, Weapons research via F5/radio/Done, and successful F9 generation.
It reaches year 2401/page 6 with the production task pending: 6/80 pages visited
(7.5% by page count), not full tutorial completion. The explicit development
stop returns exit code 2 and `status: partial`, with `fullTutorial: false`.
Its test validates this precise bounded outcome, all six visits, task completion
before Generate, and copied fleet/research orders against the map snapshot.

This exposed missing native tutorial data resources. A file-operation trace
confirmed `tutorial.h1` was successfully created despite the misleading later
“Can't open file tutorial.h1” alert. `FWriteTutorialMFile` could not find its
recorded-turn resource because the native generator only embedded standard UI
types. The generator now embeds all three canonical numeric custom-file
resources from the same RC/data files as Windows; both CMake paths track their
binary inputs. The shared SDK/native resource test requires all three and
compares their exact bytes. No game code or saved record layout changed.

Final verification passes 103/103 fatal ASan/UBSan/LSan tests, 51/51 Windows
unit/reference tests and five focused Release tutorial/resource tests. The full
native regression returns 519 matches, 87 existing unused-storage warnings,
zero differences/errors and no skipped inputs. Debug, sanitizer, Release and
MinGW Debug builds pass without new warnings. The attention reference passes
again; its historical intermittent failure is not claimed fixed. UI suites ran
sequentially on private displays. The native tutorial still needs production,
all later tasks/checkpoints, the remaining pages/years and final completion;
legacy Wine tutorial failures remain separate and unqualified. Manual desktop
checks remain deferred. Basic API coverage stays 178/178 (100%).


The native `--two-years` scenario now completes year 2401's production task
through the actual scanner menu, Q shortcut, available-items list, Shift+Add
buttons and OK. The pending queue must contain 10 and then 20 factories; the
accepted queue and task completion are checked before F9. The harness holds
SDL modifier device state while the real Add callbacks run, because pushed
key events alone do not change the state read by `GetAsyncKeyState`. It never
assigns game orders or tutorial completion flags. The real popup executes with
normal Home/Down/Return input; no fabricated menu result is returned.

The scenario reaches year 2402/page 7: 7/80 pages visited (8.75%), with six
pages' tasks completed. It reports `partial`, exit code 2 and
`fullTutorial: false`. Copied orders now include Stove Top's production entry;
`year-2401-orders.json` and before/after-generation hierarchy/map snapshots
prove exactly 20 factories, both pre-generation task checks and turn 2.
All later pages/actions and full completion at turn 37 remain outstanding.

Final production-runner verification passes 105/105 fatal ASan/UBSan/LSan
tests, 51/51 Windows unit/reference tests and all six focused Release tutorial
profiles. Full native regression returns 519 matches (87 existing unused-storage
warnings), zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. UI suites ran
sequentially on private displays. This extension changes the test harness only;
no application behavior, save layout or baseline changed. The earlier attention
flakiness and legacy Wine tutorial failures are not claimed repaired. Manual
checks remain deferred. Basic API body coverage remains 178/178 (100%).


The native runner now has bounded `--scouting` and `--mining` scenarios. Message
Next/Goto controls select the three scouts, and real Shift-clicks append all 14
route stops. Copied order IDs, tasks and universe coordinates are independently
checked against the map snapshot. Mining selects Cotton Picker #6 through the
real scanner popup, adds Prune and clicks Mine in the real task combo. Its
accepted order is fleet 5 → planet 12 with task 3. No game orders or tutorial
completion flags are assigned by the harness.

The headless Debug mining run reaches year 2402/page 11: 11/80 pages visited
(13.75%), with ten pages completed. It reports `partial`, exit code 2 and
`fullTutorial: false`; later year-2402 tasks and all subsequent years remain.
Scouting and mining each have serial headless and private-X11 CTest profiles.
The menu input wrapper now counts selectable entries rather than separators
when queuing normal Home/Down/Return navigation.

Qualification found that 640×480 clips the fleet task combo outside its parent's
client bounds. Mining automation resizes a short frame to 900×700; the application
layout issue remains deferred. The first sanitizer private-X11 mining run failed
in that setup because Openbox had already maximized the frame and SDL rejected
an unnecessary smaller-size request. The harness now checks the client height
and resizes only when needed. The corrected private-X11 mining test passes
in isolation under fatal sanitizers and in the full rerun. Qualification of
programmatic resizing while maximized remains separate and open.

Final scouting/mining verification passes 109/109 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and all ten focused Release tutorial
profiles. Full native regression returns 519 matches, 87 existing unused-storage
warnings, zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. All UI suites ran
sequentially on private displays. No game logic, saved record layout or baseline
changed. The prior attention flakiness and legacy Wine tutorial failures are
not claimed repaired; the attention test passes in this batch. Manual desktop
checks remain deferred. Basic API body coverage remains 178/178 (100%); native
tutorial coverage is 11/80 pages visited (13.75%), with ten completed.


The native `--colonization` scenario now completes year 2402 through the real
summary graph popup, message Next/Goto controls, fleet menu, cargo gauge and
waypoint task combo. Its snapshot records the visible summary popup with mouse
capture; the button release is sent on a later timer tick, after the new popup
exists. Queuing press/release together initially left the popup open because
both events were translated before its capture was established. The harness
also decodes the cargo gauge's high-bit-tagged item index before selecting the
Colonists gauge. These are test-input changes; application logic is unchanged.

The headless Debug scenario reaches year 2403/page 13: 13/80 pages visited
(16.25%), with twelve completed. Copied orders now include all five cargo
quantities per owned fleet. The launcher requires 25kT colonists in Santa Maria
#3, a Colonize waypoint at 90210 with independently checked coordinates, all
three generation checkpoints and preservation of the scouting/mining orders.
The development stop still reports `partial`, exit code 2 and
`fullTutorial: false`. All later pages/years and final completion remain open.
The private-X11 scenario passes under fatal sanitizers both in isolation and
in the full suite, and both backends pass in Release.

Final colonization-runner verification passes 111/111 fatal ASan/UBSan/LSan
tests, 51/51 Windows unit/reference tests and all twelve focused Release tutorial
profiles. Full native regression returns 519 matches, 87 existing unused-storage
warnings, zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. All UI suites ran
sequentially on private displays. No game logic, saved record layout or baseline
changed. The historical attention flakiness and legacy Wine tutorial failures
are not claimed repaired; attention passes in this batch. Manual checks remain
deferred. Basic API body coverage stays 178/178 (100%); native tutorial coverage
is 13/80 pages visited (16.25%), with twelve completed. Year-2403 actions, all
later pages/years and full completion at turn 37 remain outstanding.


The native `--logistics` scenario now completes year 2403 through the actual
message-title filter, production lists/Add buttons/research checkbox, fleet
menu, cargo gauge, task combo, QuikDrop popup and waypoint list/Delete key.
Production list focus is allowed to change before keyboard navigation is queued;
otherwise SDL translates the keys for the previously focused Add/queue control.
The initial failed development run stopped selecting Mine for this reason.
The 90210 research-leftover checkbox is set before adding its complete queue,
and the available-list click uses the currently visible top row.

Checkpoints now enumerate production for every owned planet, export each
research-leftover flag, record the factory-message filter and include all five
transport action/quantity pairs. The launcher checks the automatic 30-factory
queue at Stove Top, 3 factories/3 mines at 90210 with leftover-only research,
Teamster's 210kT colonist load, QuikDrop's four Unload All actions and fuel Load
Dunnage, and exactly one deleted scout waypoint against the initial route.
All other fleet orders must be preserved. No orders or task flags are assigned
by the harness; the normal tutorial checks must pass before F9.

The headless Debug and focused fatal-sanitizer runs reach year 2404/page 17:
17/80 pages visited (21.25%), with sixteen completed. The result remains
`partial`, exit code 2 and `fullTutorial: false`. Both focused fatal-sanitizer
profiles pass, including private X11, and both backends pass in the full suite
and in Release. All later actions/pages and final completion at turn 37 remain
outstanding. Basic API body coverage stays 178/178 (100%).

Final logistics-runner verification passes 113/113 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and all fourteen focused Release tutorial
profiles. Full native regression returns 519 matches, 87 existing unused-storage
warnings, zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. All UI suites ran
sequentially on private displays. No game logic, saved record layout or baseline
changed. The historical attention flakiness and legacy Wine tutorial failures
are not claimed repaired; attention passes in this batch. Manual checks stay
deferred. Native tutorial coverage is 17/80 pages visited (21.25%), with sixteen
completed; year-2404 actions and all later pages/years remain outstanding.

The native `--colonizer-production` scenario extends the tutorial through year
2404's two pages. Real scanner menu selection, waypoint-list click/Delete,
Next/Goto, Q, production-list selection and Add insert one Santa Maria after
Stove Top's pending factory and before its automatic factory queue. The launcher
compares initial/final orders: only Stalwart Defender's Shaggy Dog waypoint is
removed; every other route is unchanged. It compares all production entries,
requires exactly one Santa Maria insertion at index 1, and preserves research
and message-filter settings. No tutorial completion flags or orders are written.

The Debug headless run and both focused fatal-sanitizer profiles pass, including
private X11. The runner reaches year 2405/page 19: 19/80 visited (23.75%), with
eighteen pages completed. It retains `partial`, exit code 2 and
`fullTutorial: false`. This is tutorial page coverage, not overall milestone
completion. Basic API body coverage remains 178/178 (100%). Year-2405 actions,
all later years and full turn-37 completion remain outstanding. Desktop checks
remain saved for later, including the reported title-screen/native-dialog issue.

Final colonizer-production verification passes 115/115 fatal ASan/UBSan/LSan
checks, 51/51 Windows unit/reference checks and all sixteen Release tutorial
profiles. Full native regression returns 519 matches, 87 existing unused-storage
warnings, zero differences/errors and no skipped inputs. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. The sequential private
validation pipeline completed successfully; no test process from this batch
remains running. Game code, disk layouts and regression baselines are unchanged.
The historical attention flakiness and legacy Wine tutorial failure are not
claimed repaired. Native tutorial coverage remains 19/80 visited (23.75%), with
eighteen completed; full tutorial and desktop qualification remain open.

The native `--second-colonization` scenario completes year 2405 through the
cargo gauge, Colonists drag/accept, habitability/normal toolbar buttons, Shaggy
Dog Colonize order, Teamster return waypoint, message reading and No Vacancy
waypoint deletion. Checkpoints independently compare the 25kT colonist load,
scanner-mode transitions, orders/coordinates and exact deletion, preserving
other cargo, routes, production and research. It reaches year 2406/page 21
(21/80 visited, 26.25%; twenty completed), retaining the bounded partial result.

`--transport-cycle` extends through year 2406's Prune/Stove Top transport route,
QuikLoad, inherited waypoint task, QuikDrop, Repeat Orders and another Santa
Maria production insertion. The route's return leg is appended only after
selecting the Prune waypoint. An intermediate copied-order snapshot verifies
QuikLoad on both legs before QuikDrop changes the return leg. Final checkpoints
require all five transport actions/quantities, the repeat flag, preserved cargo
and other fleet records, and exactly one production insertion. Test JSON now
exports scanner mode, next-message state and fleet repeat flags; no saved game
layout, orders or tutorial completion flags are assigned by the harness.

Debug headless execution and both focused fatal-sanitizer profiles pass through
year 2407/page 23: 23/80 visited (28.75%), with twenty-two completed. Results
remain `partial`, exit code 2 and `fullTutorial: false`. Full later tutorial
execution, turn-37 completion and desktop qualification remain outstanding.
Basic API body coverage stays 178/178 (100%). Manual feedback remains saved for
later. Broader verification results will be recorded after the live pipeline.

Final second-colonization/transport-cycle verification passes 119/119 fatal
ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests and all twenty Release
tutorial profiles. Native regression returns 519 matches, 87 existing
unused-storage warnings, zero differences/errors and no skipped inputs. Debug,
sanitizer, Release and MinGW Debug builds pass without new warnings. The
sequential private-display pipeline completed with exit code 0; no test
application from this batch is left running. No game code, saved record layout
or regression baseline changed. Historical attention flakiness and the legacy
Wine tutorial failure are not claimed repaired; this batch's checks pass.

Native tutorial coverage is 23/80 pages visited (28.75%), with twenty-two
completed. The next year needs Red Storm colonization, production expansion,
mine-message filtering, enemy fleet summary/design inspection and Armed Probe
production. The full tutorial through turn 37 remains unqualified. Basic API
body coverage remains 178/178 (100%); this does not make the selected milestones
complete. Manual checks and the title-screen issue remain saved for later.

The native `--expansion` scenario completes year 2407 through real cargo,
scanner toolbar, waypoint/task combo, production queue, filter and popup input.
It loads Santa Maria #7 with 25kT of colonists, orders Red Storm colonization,
inserts three Santa Marias and two Armed Probes at the intended Stove Top queue
positions, and inserts automatic Factory/Mine queues of 10 at 90210 with
leftover-only research. It reads the remaining messages, selects the enemy
scout and presses/releases its summary image to inspect its design popup.
The launcher requires that popup to belong to fleet 512/player 1/design 0,
with counts hidden, summary enabled and no token/damage view. The visible
captured popup must close before generation. No popup identity, game orders or
tutorial progress flags are assigned by the harness.

Production input descriptors now specify the expected item, kind, position,
quantity, modifiers and research flag, rather than assuming one queue layout.
The headless Debug run and focused headless fatal-sanitizer profile pass through
year 2408/page 26: 26/80 pages visited (32.5%), with twenty-five completed.
The result remains `partial`, exit code 2 and `fullTutorial: false`. Private-X11
and broader verification are still running; their results are recorded only
once confirmed.

Routine CTest now runs smoke and the newest/longest scenario on both backends.
Expansion retains every earlier workflow/action and checkpoint assertion; this
avoids repeatedly replaying the same prefix for each stop-only scenario.
`STARS_TUTORIAL_CHECKPOINT_TESTS=ON` restores all earlier individual stop profiles
for focused development. Registration is verified at 103 default tests and
121 with the option enabled (four or twenty-two tutorial profiles). All UI
profiles remain serial/private. The count change is test deduplication, not
completion or reduced tutorial requirements. Full 80-page execution through
turn 37 and desktop qualification remain outstanding. Basic API body coverage
stays 178/178 (100%); manual checks remain saved for later.

Final expansion verification passes 103/103 routine fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests and all four default Release tutorial
profiles. The longest profile checks every earlier action and all eight
pre-generation checkpoints, so the reduced test count reflects removal of
repeated prefix replays rather than removal of workflow assertions. Optional
registration is verified at 121 tests/22 tutorial profiles; those earlier
stop-only profiles were not all separately rerun in this batch. Native
regression returns 519 matches, 87 existing unused-storage warnings, zero
differences/errors and no skipped inputs. Debug, sanitizer, Release and MinGW
Debug builds pass without new warnings. The private-display pipeline completed
with exit code 0; no UI test from this batch remains running.

No game code, saved record layout or baseline changed. Historical attention
flakiness and the legacy Wine tutorial failure are not claimed repaired.
Native tutorial coverage remains 26/80 visited (32.5%), with twenty-five
completed. Full tutorial completion through turn 37 remains outstanding; the
next year adds ship transfer/splitting, waypoint dragging, environment/research
popups, unloading-message filtering and research-budget changes. Basic API body
coverage stays 178/178 (100%). The selected goal remains active, and manual
checks—including the title-screen/native-dialog issue—remain saved for later.


## Fleet splitting checkpoint (qualified)

The native `--fleet-split` scenario extends every earlier checkpoint through
year 2408's Split and Split All operations. Real ship-transfer arrows move one
Santa Maria into fleet 9; the remaining pair loads 50kT colonists, receives
Slime colonization orders, and splits into fleets 7 and 10 with 25kT each.
Copied checkpoints verify all sixteen ship-design totals, fleet counts
9 → 10 → 11, cargo conservation, inherited orders and unchanged other fleets,
research and production. The initial Debug/dummy development run passes.
Default tutorial profiles now run smoke and fleet splitting on both backends;
earlier stop-only modes, including expansion, remain available via
`STARS_TUTORIAL_CHECKPOINT_TESTS=ON`. Final cross-build qualification passes 103/103 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests, 519 regression matches (87 existing
unused-storage warnings, zero differences/errors/skipped inputs) and all four
default Release tutorial profiles. Debug, sanitizer, Release and MinGW Debug
builds pass without new warnings. Optional registration is verified at 123
tests/24 tutorial profiles; the earlier stop-only profiles were not individually
rerun in this batch. Default registration remains 103 tests/four tutorial
profiles. The sequential private-display pipeline completed with exit code 0;
no UI test from this batch remains running. No game logic, save records or
baseline changed. Historical attention flakiness, the legacy Wine tutorial
failure and title-screen/native-dialog desktop behavior remain open.
The bounded stop is year 2408/page 27: 27/80 visited (33.75%), with twenty-six
pages completed. Waypoint dragging, unloading filtering, environment/research
popups and the research budget are still pending in this year. Basic API body
coverage remains 178/178 (100%). Manual checks remain deferred as requested.


Next automation work finishes year 2408 from the page-27 stop: use copied map
coordinates and actual scanner drag input to move fleet 7's existing Slime
waypoint to Sea Squared; fleet 10 must retain Slime. Route fleet 8 to Hiho,
filter the actual unloading message, read all remaining messages, inspect
Wallaby's radiation popup, then inspect the Radiation Terraform 7 research
benefit and set the budget to 30 through its arrows. Verify popup identity,
ship/cargo/order conservation and pre-generation state before F9. The expected
next bounded stop is year 2409/page 30, not full tutorial completion. Manual
checks remain deferred; the selected goal remains active.


## Research cycle checkpoint (qualified)

The new `--research-cycle` scenario finishes year 2408 by driving the scanner
waypoint drag, armed scout route, unloading-message filter, Wallaby radiation
popup, Radiation Terraform 7 benefit popup and research budget arrows. It
captures independent route, cargo, ship-count, popup-identity and research
checkpoints before generation to year 2409/page 30. The verified bounded
coverage is 30/80 visited (37.5%), with twenty-nine completed. Final qualification
passes 103/103 fatal ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests,
519 native regression matches (87 existing unused-storage warnings, zero
differences/errors/skipped inputs) and all four default Release tutorial
profiles. Both new sanitizer profiles pass (176.17s headless, 362.31s private
X11), as do both Release profiles (109.27s/233.06s). Debug, sanitizer, Release
and MinGW Debug builds pass without new warnings. Optional registration is
verified at 125 tests/26 tutorial profiles; earlier stop-only profiles were
not separately rerun. Default registration remains 103 tests/four tutorial
profiles. The sequential private-display pipeline completed with exit code 0;
no UI test from this batch remains running. No game logic, saved record layout
or baseline changed. Historical attention flakiness, the legacy Wine tutorial
failure and native-dialog/desktop issues remain open. No game/save-state assignments complete tasks.
Default tutorial profiles run smoke and the research cycle on both backends;
all earlier stop-only modes remain opt-in. Basic API body coverage remains
178/178 (100%). Full tutorial/goal completion is still unverified. Manual
checks remain deferred.


Next automation work covers year 2409: open Research through F5, choose
Construction as the next field using the real combo, keep Weapons/30% and
commit through Done. Filter the mining-robots loading message, read/Goto the
Oxygen message, load fleet 9 with 25kT colonists and give it an Oxygen Colonize
order, then drag fleet 0's existing Oxygen waypoint to Mozart (planet 4).
Capture start/final orders, cargo, fleet composition, research and message
filter state before generation. Expected next bounded stop is year 2410/page
32. Verify the destination against `text/tutorial.txt` instruction 245 and the
copied map; it is Mozart, not No Vacancy. Full execution through all 80 pages
and turn 37 is still required. The selected goal remains active, and all manual
checks remain saved for later.


## Oxygen colonization checkpoint (qualified)

The new `--oxygen-colonization` scenario completes year 2409 using the real
message Goto controls, research dropdown, mining-message checkbox, cargo gauge,
Colonize task dropdown and waypoint drag. It captures the Construction next
field, 25kT colonists, Oxygen/Mozart route identities and preservation of other
fleets, queues and flags before generation. The verified bounded stop is year
2410/page 32 (32/80 visited, 40%; thirty-one completed). Final qualification
passes 103/103 fatal ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests,
519 native regression matches (87 existing unused-storage warnings, zero
differences/errors/skipped inputs) and all four default Release tutorial
profiles. Both new sanitizer profiles pass (201.53s headless, 414.55s private
X11), as do both Release profiles (123.27s/259.38s). Debug, sanitizer, Release
and MinGW Debug builds pass without new warnings. Optional registration is
verified at 127 tests/28 tutorial profiles; earlier stop-only profiles were
not separately rerun. Default registration remains 103 tests/four tutorial
profiles. The sequential private-display pipeline completed with exit code 0;
no UI test from this batch remains running. No game logic, saved record layout
or baseline changed. Historical attention flakiness, the legacy Wine tutorial
failure and native-dialog/desktop issues remain open. All earlier scenario actions/checkpoints remain in the longest mode.
Basic API body coverage remains 178/178 (100%); full tutorial and selected
milestones remain unfinished. Manual checks remain deferred.


Year-2410 source preparation: `ProductionDlg` opens `ZipProdDlg` with the
production HWND as owner, so the parent is disabled while the template is open.
`StarsCompatCaptureHierarchy` emits oldest windows first; the current runner's
`HwndModal` stops at the first visible modal, which would select that older
parent. Before adding the nested-template workflow, resolve the newest visible
enabled modal through fresh stable IDs, then verify ownership/parent disabled
state and restoration after closing. The template's first preset imports
without a Rename dialog (`iResTechNow == Energy`); use the real preset radio,
Import and OK controls. Capture `vrgZipProd[0]` semantically (valid flag,
leftover-research flag, count and `mdIdle`/`cQuan` pairs), without changing its
packed structs or saved widths. These are source findings for the next batch,
not a repair or qualification of that workflow yet.


Next automation work covers year 2410's production templates. Fix the runner's
fresh modal resolution for nested dialogs first. Filter dismantled-colonizer
messages and use the real Next/Goto action to Shaggy Dog. Add three automatic
factories and three automatic mines, with leftover-only research, then reopen
production and use the blue diamond's observed `<Customize>` menu. Import the
queue into preset 1 through `ZipProdDlg` and close both dialogs through their
real OK controls. Read/Goto Bloop, select Stove Top and insert one Santa Maria
at queue position 1 and one Teamster (design 3) at position 2. Capture the
actual template, production queues, research flags and unchanged fleets before
F9. Expected next bounded stop is year 2411/page 34 (42.5% visited), with all
prior checkpoints retained. Full 80-page completion through turn 37 and the
other selected milestones remain required. The goal stays active and all
manual checks remain deferred.


## Production template checkpoint (qualified)

The `--production-templates` extension adds year 2410 message filtering,
Shaggy Dog automatic factories/mines, preset 1 Import, Bloop Goto and Santa
Maria/Teamster queue insertion. The runner resolves the newest enabled modal
from a fresh hierarchy snapshot. Its observed menu driver uses that modal's
SDL input window when the command recipient is a disabled frame; the real
menu implementation still chooses and sends the command. Captures check the
actual template, disabled production owner, restored owner and unchanged
unrelated state. The qualified bounded stop is year 2411/page 34 (34/80 visited, 42.5%;
thirty-three completed), with all prior checkpoints retained. Debug, sanitizer,
Release and MinGW Debug builds pass without new warnings. Optional registration
is verified at 129 tests/30 tutorial profiles; defaults remain 103 tests/four
profiles. Final qualification passes 103/103 fatal ASan/UBSan/LSan tests,
51/51 Windows unit/reference tests, 519 native regression matches (87 existing
unused-storage warnings, zero differences/errors/skipped inputs) and all four
default Release tutorial profiles. The new sanitizer profiles pass in
238.54s headless and 487.82s private X11; Release profiles pass in
148.36s/302.12s. The sequential pipeline completed with exit code 0, and no
UI test from this batch remains running. Earlier stop-only profiles were not
separately rerun. Historical attention flakiness, the legacy Wine tutorial
failure and native-dialog/desktop issues remain open. API body coverage stays
178/178 (100%); the full tutorial and selected milestones remain incomplete.
No game logic, packed struct
or saved-record layout is changed. Manual feedback remains deferred.


Year-2411 preparation: `FTutorTaskDone` requires fleet 0 to Hacker (planet 10),
fleet 1 to Stove Top with Scrap Fleet (task 5), fleet 4 to Stove Top and fleet 8
to enemy fleet 512. Fleet 2 must carry colonists and colonize Mozart (planet 4);
fleet 11 must carry 210kT colonists and have a Wallaby Transport waypoint with
Colonists/Unload All. Add 70 regular mines at Stove Top queue position 0 and
check Construction/Biotechnology/30% research. Message 11 opens the Technology
Browser; close it, read through message 13 and Goto fleet 8. Then use the actual
View control and Battle VCR playback/Done before reading all remaining messages
and generating. Page 37 includes battle playback; the next generated-year stop
should be year 2412/page 38 (47.5% visited), not page 37. Capture identities,
orders, cargo, queues, research, browser and battle state before generation.
These are source findings, not implemented or qualified workflow evidence.
Full completion still requires all 80 pages and turn 37.


Packaging source preparation: neither `CMakeLists.txt` nor
`cmake/sdl-client.cmake` has install rules, and no desktop entry, AppStream
metadata or Flatpak manifest is present. `help.c` first checks the archive
basename, falls back to `stars!.hlp` beside the executable, then locates the
generated guide at `../help` relative to `SDL_GetBasePath`. A conventional
`bin`/`share/stars` install must update and test these lookups while preserving
build-tree launches and explicit archive paths. Bare profile names already use
`XDG_CONFIG_HOME/stars` or `$HOME/.config/stars`; explicit settings paths stay
literal. Include the rendered help assets and font licenses, not just the
client executable. This is preparation for the selected Flatpak plus CMake
install milestone, not delivered packaging. `flatpak-builder` remains absent
and matching Platform/SDK runtimes are still required.

The current local runtime inventory confirms Platform 26.08 and SDK 25.08;
these are not a matching build pair. Release `ldd` also resolves `libSDL3.so.0`
inside `dist/sdl-release/_deps/sdl3-build`, and that cache has `SDL_INSTALL=OFF`.
Adding an executable install rule alone would therefore leave the installed
client dependent on the build tree. The install milestone must explicitly
handle the fetched SDL shared library and its installed search path, then
verify launch from a different directory with the build tree unavailable.
Other directly linked libraries are FreeType, Fontconfig, Cairo and libdbus;
the Flatpak build must resolve these against its chosen SDK/runtime rather
than copy the host's system libraries into the sandbox. This is dependency
inspection evidence, not a successful clean-install or Flatpak test.

Inspection of the installed SDK 25.08 deployment finds CMake, Ninja, Python,
pkg-config and development metadata for libdbus, FreeType, Fontconfig and Cairo.
Its SDL3 is 3.2.30, below the project's existing 3.4 minimum. Python's `yaml`
package and `wmf2svg` are absent. The eventual sandbox manifest therefore needs
the pinned SDL 3.4.18 module, PyYAML for data generation, and the WMF converter
for generating the packaged help images. Host-side installations do not supply
these tools inside a Flatpak build sandbox. This filesystem inventory is not a
sandbox build or dependency-resolution test.


Year-2411 dialog source notes: `BrowserDlg` is modeless, so fresh hierarchy
inspection must use `hwndBrowser`/its stable ID rather than the modal resolver.
Its Close (`IDCANCEL`) action destroys the logical window, clears browser state
and advances the tutor. `VCRDlg` exposes actual rewind/play/step/forward buttons
(`IDC_VCR_REW_ALL` through `IDC_VCR_FWD_ALL`); the playback timer advances
`viStepVCRCur` toward `vcStepVCR`. Done/Cancel stops that timer, sets tutorial
progress through the real handler and ends the dialog. The next driver should
observe a nonzero battle identity and advancing playback before pressing Done;
closing immediately would not establish playback control support. Do not assign
these globals or progress flags in the harness.


## Battle cycle checkpoint (qualified)

The new `--battle-cycle` driver extends year 2411 through the scout routes,
Scrap Fleet task, Mozart colonization, Wallaby Transport/Colonists/Unload All,
70 regular mines, Construction/Biotechnology/30% research, Technology Browser
Close and Battle VCR playback controls. It generates year 2412/page 38
(38/80 visited, 47.5%; thirty-seven completed). This is the latest qualified
bounded checkpoint; the full tutorial remains incomplete. Semantic captures
check routes, cargo, queue preservation, browser destruction, battle identity,
timer-driven advancement, pause, seek, rewind, stepping and Done cleanup.
The harness does not assign game orders or tutorial progress. Source review
shows that a battle message's first Goto selects the fleet at its coordinates;
the next View opens the VCR. The dispatcher must recognize `mdMsgObjBattle`
when handling the later Armed Probe selection. Manual desktop checks for these
controls are deferred alongside all other feedback.


The first battle-cycle run exposed a real compatibility defect at Transport's
Colonists dropdown. A read-only debugger attachment found continuous popup
painting inside the owner callback, with runner stage 173 and `cWait == 0`;
`CompatComboProc` invalidated the popup on every mapped getter. The identified
headless process was deliberately stopped after diagnosis, preserving its
artifacts; it was not restarted because of an observation timeout. Popup
invalidation now covers mapped write operations only. The new owner-draw
query/update-region regression fails against the old runtime and passes with
the repair. Debug, sanitizer, Release and MinGW Debug builds pass without new
warnings. Registration is verified at 131 tests/32 optional tutorial profiles,
with 103 tests/four profiles by default. The second development run includes
the full semantic assertions and recognizes battle messages for fleet Goto.
Final bounded qualification is recorded below. No game logic or saved-record layout changes.


Year-2412 source preparation: select Armed Probe #9 through the message Goto,
then right-click its location to choose salvage (a `grobjThing` in scanner
inspection). Verify the copied thing identity and summary minerals. Use the
actual View/Find dialog to locate Teamster #4, retaining fleet selection/summary
evidence. Read through the messages to Red Storm and Slime. Add two Terraform
Environments at Slime queue position 0 with leftover-only research, and two
Teamsters at Stove Top queue position 1. The expected next bounded stop is
year 2413/page 41 (51.25% visited). These are source findings only; battle-cycle
qualification and the remaining full 80-page/turn-37 walkthrough stay required.


The second development run reaches year 2412/page 38 and passes route/cargo,
production, research and all VCR semantic assertions. It records battle 2561,
two tokens, play advancement from step -1 to 1, seek to final step 12, rewind
to -1 and step to 0, then real Done cleanup. The launcher rejects missing
browser captures: `FTutorTaskDone` advances its highlighted prompt when the
modeless browser opens, before the next runner tick. Capture and Close now
occur in the browser-open waiting stage rather than relying on that prompt
remaining highlighted. A third development run is verifying all assertions.
This remains partial development evidence, not final qualification.


The third development run confirms the browser waiting stage was never reached:
message navigation advanced instruction 283 to 285 without a Goto click. Saved
hierarchies contain no browser before or after generation. `PlanetWndProc` can
set `tutor.fProgress` after a production-list selection; the tutor accepts that
flag after message 11 is read. The runner now completes the explicit message-11
Goto independently of whether its highlight advances, then captures and closes
the modeless browser. This is the same real-input strategy used for Bloop, not
a write to tutorial flags. The fourth development run is verifying this path.


The fourth headless Debug development run passes every assertion through year
2412/page 38 (38/80 visited, 47.5%; thirty-seven completed). Explicit message-11
Goto now opens the modeless browser and its real Close destroys it; all VCR
controls and cleanup checks pass. The final driver additionally requires the
browser's selected part to be `hstTorp`/`itorpBetaTorpedo`. Debug, sanitizer,
Release and MinGW Debug builds pass with that guard and no new warnings. Final
qualification passes sequentially on private displays. Qualified visited-page
coverage is 47.5%. API bodies remain 178/178 (100%); manual
feedback stays deferred and the full selected goal remains active.

The current final gate has passed both battle-cycle profiles with fatal
ASan/UBSan/LSan settings: headless dummy in 344.01 seconds and private X11 in
675.05 seconds. Both launchers pass their state assertions through year
2412/page 38. All 103 sanitizer tests subsequently pass (1295.32 seconds).
All 51 Windows unit/reference tests pass (95.17 seconds). Native regression
returns 519 matches, 87 existing unused-storage warnings, zero differences/errors
and zero skipped reference inputs; no baseline changes. All four Release
tutorial profiles pass (628.60 seconds); battle cycle takes 212.76 seconds
headless and 407.89 seconds on private X11. The final pipeline exits zero.
The repository-required legacy Wine walkthrough fails separately at year 2407,
page 23/instruction 181 after 91 actions: the game exits and Wine reports
`free(): invalid next size (fast)`. Retained artifacts are in
`dist/scaffold/tutorial/20261011T220906.282422Z/`. This repeats the prior
heap-failure class at a different popup action; the origin of the corruption is
not established. These passing native profiles remain partial and do not
establish a passing legacy Wine walkthrough. No unchanged retry was started.

## Year 2412 input and verification preparation

Source review confirms that the next checkpoint must exercise selection through
the actual scanner context menu and the actual Find dialog. `ScannerWndProc`
builds its right-click list from fleets and things at the same logical position;
choosing Salvage changes `sel.scan` to `grobjThing`. A copied scanner snapshot
already exposes each thing's full ID, subtype, position and selection flag, so
the driver can resolve the Armed Probe #9 location without changing game state.
Capture the selected salvage's identity and mineral contents before advancing;
merely selecting the fleet does not cover this task.

View/Find has the real Ctrl+F accelerator (`IDM_VIEW_FIND2`), opens `FindDlg`,
and accepts up to 39 characters in `IDC_EDIT1`. The next driver should enter
`Teamster #4` using SDL text input, click its actual OK control, assert fleet 3
is selected, and capture the dialog's destruction and resulting map selection.
It should not substitute the inspection API's direct selection helper for this
explicit tutorial action.

The year-12 branch in `FTutorTaskDone` requires Red Storm (planet 18), Slime
(planet 8), two regular Terraform Environments at Slime queue index 0 with
leftovers-only research, and two Teamsters (design 3) at Stove Top (planet 13)
queue index 1. Assert those exact queue records, preserve unrelated queues,
fleets, cargo and research state, capture a completed year-12 checkpoint before
F9, then require year 2413/page 41. That would visit 41/80 pages (51.25%); it is
the next intended checkpoint, not implemented or qualified coverage. The
following year introduces the Ship Designer and drag/drop, so page 41 remains
a bounded stop rather than full tutorial completion.

## Native Wayland and pointer failure source audit

The fetched SDL build enables Wayland, but the compatibility runtime still
requires X11/Xwayland for interactive windows, as documented in
`WIN32-INPUT.md`. Its top-level creation path treats a failed
`SDL_SetWindowPosition` as fatal. SDL 3.4.18's Wayland backend explicitly refuses
to position ordinary top-level windows; its popup roles support positioning
relative to their parent. The compatibility runtime already creates menu,
combo-list and tooltip popup roles, so native Wayland work should preserve those
roles while letting the compositor place ordinary windows. This is a source
finding, not a native Wayland startup test.

`GetCursorPos` currently rejects Wayland. `GetCursorPos16` then narrows its
uninitialized local `POINT` even when the API returns false; many UI callers
ignore that return value. A standalone mock failure probe compiled with GCC's
`-ftrivial-auto-var-init=pattern` reproduces the copied indeterminate position.
A draft that initializes the local point to zero passes the same probe and the
successful-coordinate case. After the battle gate and required legacy run
finish, the initialization is applied with a persistent `nativeui_points.c`
regression. That test fails against the exact pre-fix header and passes against
the repair in native Linux and a fresh private-prefix Windows SDK run.
Deterministic failure output prevents this uninitialized read; it
does not supply the logical pointer-coordinate semantics still needed for
native Wayland. No native Wayland compositor is available here yet (`weston`
is absent), and desktop qualification remains pending.

## Ship Designer input preparation

The following year-13 task uses `SlotDlg` (`IDD_SLOT`), the real F4 accelerator,
`IDC_DESIGNER_HULLS`, the `IDC_COMBOBOX` hull selector and `IDC_IMPORT`. The
tutorial expects hull category 1/row 7 (Mini-Miner) and edit mode 4. The required
parts are Long Hump 6 in slot 0, Rhino Scanner in slot 1, and Robo-Miner in slots
2 and 3. Read the copied `vrgrcSlot` rectangles for targets; these slots are
drawn in the dialog rather than represented by child HWNDs. Component list
strings have a four-byte metadata prefix; find the visible part name after it,
and begin dragging inside the first 64 pixels of that list row so `FakeListProc`
enters `FTrackSlot`.

`FTrackSlot` converts the list source point to designer coordinates and captures
the designer. Its `FGetMouseMove` loop consumes queued mouse messages without
dispatching timer callbacks. The driver must therefore queue down, motion and
up for a drag before entering that loop; splitting release onto a later driver
timer would leave the tracking loop waiting. The target is the dialog's painted
slot area, so motion/up coordinates must be relative to that dialog. Assert
exact part IDs and counts in each resulting slot, including Robo-Miner identity:
the page-42 progress check merely requires nonempty mining slots, while the
actual OK handler calls the strict `FCheckBuilderPart` checks for all four parts.
Then assert
the accepted design, prior designs unchanged, editor closure and production
queue placement. This is source preparation only, not tested designer coverage.

The accepted recipe is one Long Hump 6, one Rhino Scanner and one Robo-Miner
in each of the two mining slots. Accept with the editor's actual OK, then use
its Done (`IDCANCEL`) after it returns to the normal designer view. Preserve all
six earlier designs and require the new design at index 6. The year-13 queue
tasks place one Mini-Miner at Stove Top index 1 and add 100 regular mines at
index 0 with a real Ctrl+Add gesture. The production-tile action uses the real
`hwndPlanetProdLB`: its `LBN_SELCHANGE` handler advances the tutorial. Read every
remaining message, route Armed Probe #9 back to Stove Top, capture completed
orders, and generate year 2414/page 45 (45/80 visited, 56.25%; forty-four
completed). These are the intended next requirements, not implemented coverage.

## Terraform cycle checkpoint (development)

The new `--terraform-cycle` mode targets year 2413/page 41 (41/80 visited,
51.25%; forty completed). It adds copied salvage identity/minerals, the actual
Ctrl+F/Find dialog, Red Storm and Slime message actions, two regular Terraform
Environments at Slime and two Teamsters at Stove Top. Exact queues and unrelated
state have assertions; no game orders or tutorial flags are assigned by the
harness. Final qualification remains pending, and the qualified stop remains
year 2412/page 38 (47.5% visited).

The first development run passes every earlier action, then fails at the
salvage menu: the original `idsSalvage` string has a trailing space. The driver
now finds a co-located salvage object in the copied map and uses its exact
copied label, preserving the resource text. Missing-menu failures also retain
entry labels and hierarchy/map artifacts. Source review additionally finds
that Ctrl+F needs modifier bits on its SDL F-key events; the new shortcut helper
supplies them. The rebuilt second development run is in progress.

The second run confirms salvage selection, the real Ctrl+F/Find dialog and
Slime's Terraform queue, then stops at stage 194/instruction 318. Its map shows
Stove Top selected in the summary but Slime still selected in the main panel:
a single scanner click is summary-only. The third run uses the normal scanner
context menu for the main planet selection. It also explicitly clicks Armed
Probe #9's message Goto after Next, even if the fleet left selected by the
previous VCR lets the tutorial advance without that click. Its after-Goto
snapshot and event are independently required. The third run is in progress;
no full or new bounded pass is claimed yet.

Debug, sanitizer, Release and MinGW Debug builds pass. Header rebuilding
re-exposes existing format-truncation warnings in unchanged unit-harness files;
there are no new warnings in changed files. Linux host units pass 13/13. The
new pointer regression is registered in both SDL build target lists (including
the inherited sanitizer preset); default native registration is 104 tests and
Windows registration is 52. Full suites and the new tutorial checkpoint still
need final verification after the development fixes.

The third headless Debug development run passes all launcher assertions and
stops at year 2413/page 41 (41/80 visited, 51.25%; forty completed), with 385
actions and `fullTutorial: false`. Salvage identity/minerals, explicit message
Goto, Find entry/OK/destruction, exact Terraform and Teamster queues, research,
fleet/cargo preservation and all thirteen before-generation checks pass.
Artifacts are `/tmp/stars-terraform-dev-three/`. Sanitizer and Release targets
are rebuilt with the final driver. Optional registration verifies 134 tests/34
tutorial profiles, then is restored OFF (104 tests/four profiles). Final
qualification is running sequentially; Windows unit/regression stages now use
fresh private Wine prefixes as well as private displays. Qualified visited-page
coverage remains 47.5% until every gate passes. API bodies remain 178/178 (100%).
