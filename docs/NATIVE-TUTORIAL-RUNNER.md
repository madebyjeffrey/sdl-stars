# Native tutorial runner

The October 12 Windows SDL Merge cycle qualification reaches year 2413/page 42
and 413 actions, then times out at stage 216/instruction 332 during the Mini-Miner
engine drag. The default Windows smoke passes. This longer Windows run is a
failure; the later Linux checkpoint evidence below does not qualify it. See
[Windows verification](WINDOWS-SDL3.md) for artifacts and remaining coverage.

`stars-native-tutorial` links the real SDL client, its resources and the game
core. It runs the normal `WinMain` message loop and queues SDL mouse/keyboard
events on the UI thread. No external desktop automation or AutoHotkey is used.
The runner supports **`--smoke`, `--first-year`, `--two-years`, `--scouting`,
`--mining`, `--colonization`, `--logistics`, `--colonizer-production`, `--second-colonization`,
`--transport-cycle`, `--expansion`, `--fleet-split`, `--research-cycle`, `--oxygen-colonization`, `--production-templates`, `--battle-cycle`, `--terraform-cycle`, `--design-cycle`, `--automation-cycle`, `--report-cycle` and `--merge-cycle`**, not the complete tutorial.
Calling it without an explicit supported scenario returns an error.

The smoke opens New Game and Tutorial through real buttons, presses F9 before
the first task is complete, observes the real rejection message and clicks its
OK button. It then reads messages through Next, uses Goto to select Armed Probe
and finds Prune through a copied scanner snapshot. Shift-clicking the observed
map position must produce the correct fleet waypoint and reach page 3 while
remaining at turn zero. Auto-completion is rejected throughout. This checks
three of 80 pages (3.75% by page count); the pages differ in complexity, so this
is not a percentage of total implementation work.

`--first-year` continues through the N-key fleet selections, Shift-click orders
to 90210 and Alexander, F5 research selection of Weapons, and a successful F9
generation. It checks all first-year tasks before generating and stops at year
2401/page 6 with the production task still pending. Six of 80 pages have been
visited (7.5% by page count), and five pages' first-year tasks completed. The
runner returns **exit code 2 and `status: partial`** for this intentional
development stop. The first-year CTest launcher verifies that precise partial
outcome and its checkpoint; a passed first-year test is not a full tutorial pass.

`--two-years` continues through the year-2401 production task: it selects Stove
Top through the real scanner popup, opens production with Q, navigates the
available-items list to Factory, and clicks Add twice with Shift held. The live
queue must contain 10, then 20 factories; accepting it must save the correct
planet production entry and complete the tutorial task before F9. The runner
stops at year 2402/page 7, still reporting `partial` and exit code 2. Seven of
80 pages have been visited (8.75%), with six pages' tasks completed. This is
bounded development coverage, not a complete walkthrough.

`--scouting` follows the year-2402 messages through Next/Goto and Shift-clicks
three multi-stop scout routes. Its checkpoint independently compares all 14
new waypoint IDs, tasks and universe coordinates against the copied map. It
stops at page 10, with nine pages completed (10/80 visited, 12.5%). `--mining`
continues through the fleet popup, adds Prune as a waypoint and clicks Mine in
the real task combo. The accepted order must belong to fleet 5, point to planet
12 and have task 3. It reaches page 11 with ten pages completed (11/80 visited,
13.75%). Both scenarios retain `partial`, exit code 2 and `fullTutorial: false`.

The mining scenario resizes a short main frame to 900×700 through `SetWindowPos`
before opening the fleet panel; an already tall or maximized frame needs no resize.
At the initial 640×480 size, the task combo is
below its parent's client bounds and cannot receive a normal mouse click. The
larger layout qualifies the workflow; the short-window layout remains an open
application issue in the deferred testing list.

`--colonization` completes the remaining year-2402 tasks. It follows Next/Goto
to Alexander, presses a summary graph, captures the visible popup with mouse
capture, then releases the button after the popup has opened. It continues to
90210, selects Santa Maria #3 from the real fleet menu, opens cargo transfer and
drags the Colonists gauge to fill the hold with 25kT. After accepting the cargo,
it adds 90210 as a waypoint and clicks Colonize in the task combo. The copied
cargo and order must match the tutorial before F9, with all previous scout and
mining orders preserved. It stops at year 2403/page 13: 13/80 pages visited
(16.25%), with twelve completed, still `partial`, exit code 2 and
`fullTutorial: false`.

`--logistics` completes year 2403's message filter, automatic factory queue on
Stove Top, three factories/three mines on 90210 with leftover-only research,
Teamster cargo loading to 210kT, a QuikDrop transport order and scout waypoint
deletion. The launcher compares both production queues and research flags,
the filtered-message bit, all five transport actions and the exact deletion
against the copied starting orders. Other fleet orders must remain unchanged.
It reaches year 2404/page 17: 17/80 pages visited (21.25%), with sixteen completed,
still `partial`, exit code 2 and `fullTutorial: false`.

`--colonizer-production` completes year 2404. It selects Stalwart Defender
from the scanner popup, clicks its Shaggy Dog waypoint and presses Delete.
It then follows the next message, selects Stove Top, opens production and selects
the second queue row before adding one Santa Maria through the available-items
list and Add button. The checkpoint requires exactly the one waypoint deletion
and one queue insertion, preserving other orders, production and research flags.
It stops at year 2405/page 19: 19/80 pages visited (23.75%), with eighteen completed,
still `partial`, exit code 2 and `fullTutorial: false`.

`--second-colonization` completes year 2405. It opens transfer through the
cargo gauge and drags Colonists to fill the new Santa Maria with 25kT. It clicks
the toolbar's habitability view, adds Shaggy Dog and chooses Colonize, then
restores normal scanner view. It follows messages to Teamster, adds a return
waypoint to Stove Top, reads the remaining messages and deletes Armed Probe's
No Vacancy waypoint. Checkpoints compare cargo, scanner transitions, the
colonization order and exact route deletion, preserving other orders and queues.
It reaches year 2406/page 21 (21/80 visited, 26.25%; twenty completed).

`--transport-cycle` completes year 2406. Teamster receives a Prune waypoint,
Transport and QuikLoad. The runner selects that waypoint before adding the
return leg to Stove Top. An intermediate checkpoint requires QuikLoad to have
been copied to the new leg. It then chooses QuikDrop for that leg, clicks Repeat
Orders and adds another Santa Maria at position 1 in Stove Top's production queue.
The launcher compares both transport action arrays and map coordinates, the
repeat flag, preserved cargo/routes and exact production insertion. It reaches
year 2407/page 23 (23/80 visited, 28.75%; twenty-two completed). Both new scenarios
retain `partial`, exit code 2 and `fullTutorial: false`.

`--expansion` completes year 2407. It fills the new Santa Maria with 25kT of
colonists, uses habitability view to route it to Red Storm and chooses Colonize.
It inserts three Santa Marias at position 1 in Stove Top's queue, restores normal
view, filters mine messages and inserts automatic Factory/Mine queues of 10 at
90210 with leftover-only research. It reads the remaining messages, selects the
enemy scout in the scanner and presses/releases the summary image to open and
close its design popup. The copied popup identity must belong to that scout,
with counts hidden and the summary view enabled. It then inserts two Armed Probes
at queue position 2. Before generation, the launcher compares cargo, the complete
queues, research flags, the mine filter, scanner modes and preserved other fleets.
It reaches year 2408/page 26 (26/80 visited, 32.5%; twenty-five completed), still
`partial`, exit code 2 and `fullTutorial: false`.

The menu wrapper chooses by observed item text, queues normal menu navigation,
and calls the real `TrackPopupMenu`; it never fabricates a selected command.
For Shift+Add, the harness holds SDL's device modifier state until the real
callbacks finish, then releases it. Pushed SDL key events alone do not update
the state used by the application's `GetAsyncKeyState` calls. No game flags or
production orders are assigned by the harness.

Map inspection reveals an offscreen target through the normal secondary
scanner selection/pan interface, then recaptures its coordinates before clicking.
The main fleet selection and orders are not assigned by the harness. Printable
keys include SDL text-input events as well as their physical key events.

The runner resolves logical window IDs from fresh hierarchy snapshots. The
message-box wrapper observes its text and then calls the real implementation;
it never supplies a fabricated result or suppresses the dialog. No game orders,
tutorial progress or task-completion flags are written by the harness.

Build and run the isolated headless scenario:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_smoke_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_merge-cycle_dummy$' --output-on-failure
```

Routine validation runs smoke and the longest scenario on both backends. The
current longest scenario is Merge cycle; its build/backend qualification after
the binary64 migration is recorded below. The longest scenario
replays every earlier action and checks every prior orders-before-generation
checkpoint, so these four profiles retain all existing workflow assertions.
Earlier bounded stop-only profiles can be registered for focused development:

```sh
cmake --preset sdl-debug -DSTARS_TUTORIAL_CHECKPOINT_TESTS=ON
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_first_year_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_two_years_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_scouting_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_mining_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_colonization_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_logistics_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_colonizer-production_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_second-colonization_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_transport-cycle_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_expansion_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_fleet-split_dummy$' --output-on-failure
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_research-cycle_dummy$' --output-on-failure
```

`win32_native_tutorial_smoke_x11` runs the same scenario with a private Xvfb and
Openbox session. `win32_native_tutorial_first_year_x11` does the same for the
first-year scenario; `win32_native_tutorial_two_years_x11` tests production.
The scouting, mining, colonization, logistics, colonizer-production,
second-colonization and transport-cycle scenarios each have matching private-X11
profiles when `STARS_TUTORIAL_CHECKPOINT_TESTS=ON`. Expansion, fleet splitting, the research cycle and Oxygen colonization
also have matching private-X11 profiles. The default four tutorial profiles, or all forty-two
opt-in profiles, are serial and do not touch the desktop session. Restore the
default registration with:

```sh
cmake --preset sdl-debug -DSTARS_TUTORIAL_CHECKPOINT_TESTS=OFF
```

The sanitizer and Release build presets include the runner too. CLion can use
the `stars-native-tutorial` target with argument `--smoke`; use a separate empty
working directory and set `STARS_SETTINGS_DIR` to that directory so tutorial
files/settings do not affect a user's game. The Python launcher does this
automatically for CTest.

Artifacts live under the build's `test-client/tutorial-dummy` or `tutorial-x11`
for smoke, and `tutorial-first-year-dummy`/`tutorial-first-year-x11` for first year:
`client.log`, `events.jsonl`, `result.json`, and hierarchy/map snapshots at
startup, rejection and pages two/three. Failures capture the last hierarchy and
map; launch errors/timeouts also write `launcher-failure.txt`. A smoke success
always records `fullTutorial: false`. The launcher validates the outcome and
snapshots and returns nonzero for failures.

The first-year scenario adds `year-2400-orders.json`, hierarchy/map snapshots
before generation, and `year-2401` snapshots. The launcher checks Weapons/next
field/budget, fleet waypoint IDs/tasks and coordinates against the independent
map snapshot, task completion before Generate, all six page visits, and the
new turn number. The native resources include the same three recorded tutorial
data files as the Windows RC; shared SDK/native tests compare every embedded byte.
The two-year scenario adds `year-2401-orders.json`, a production checkpoint and
`before-generate-2401`/`year-2402` snapshots under `tutorial-two-years-dummy`
or `tutorial-two-years-x11`. Its launcher requires exactly 20 factories on
Stove Top, both pre-generation task checks, all seven page visits and turn 2.
Scouting adds `year-2402-scout-orders.json` and `scouting` hierarchy/map snapshots;
mining adds `year-2402-mining-orders.json` and `mining` snapshots. Artifacts are
stored in `tutorial-scouting-{dummy,x11}` and `tutorial-mining-{dummy,x11}`.
The launcher checks every visited page and requires both earlier generation
checkpoints as well as the route/mining orders; turn 2's remaining tasks are
still pending when these scenarios stop.

Colonization adds `summary-popup` hierarchy/map snapshots, `year-2402-orders.json`
with all five cargo quantities per owned fleet, and `before-generate-2402`/
`year-2403` snapshots. These are stored under `tutorial-colonization-{dummy,x11}`.
Its launcher checks the visible captured popup, the exact colonist quantity,
Colonize task/planet/coordinates, preservation of all three scouting routes and
the mining order, all three generation checkpoints and all thirteen page visits.

Logistics adds `year-2403-start-orders.json`, `year-2403-orders.json`,
`before-generate-2403` and `year-2404` snapshots under
`tutorial-logistics-{dummy,x11}`. Production checkpoints enumerate every owned
planet; `planetFlags` carries each research-leftover flag and `filteredFactories`
records the message filter. Transport waypoints include five action/quantity
pairs. The launcher requires all four generation checkpoints and seventeen visits.

Colonizer production adds `year-2404-start-orders.json`, `year-2404-orders.json`,
`before-generate-2404` and `year-2405` snapshots under
`tutorial-colonizer-production-{dummy,x11}`. The launcher compares the exact
route deletion and queue insertion against the copied initial state and requires
all five generation checkpoints and nineteen page visits.

Second colonization adds `year-2405-start-orders.json`, `year-2405-cargo.json`,
`year-2405-orders.json`, `habitability`, `before-generate-2405` and `year-2406`
snapshots under `tutorial-second-colonization-{dummy,x11}`. Checkpoints include
scanner view and next-message state. Event records include the scanner mode.
Transport cycling adds `year-2406-start-orders.json`, `year-2406-copied-orders.json`,
`year-2406-orders.json`, `before-generate-2406` and `year-2407` snapshots under
`tutorial-transport-cycle-{dummy,x11}`. Fleet checkpoints include Repeat Orders.
Expansion adds `year-2407-start-orders.json`, `year-2407-cargo.json`,
`year-2407-orders.json`, `enemy-design.json`, `red-storm-view`, `enemy-summary`,
`enemy-design-popup`, `before-generate-2407` and `year-2408` snapshots under
`tutorial-expansion-{dummy,x11}`. Checkpoints include the mine-message filter.
The longest scenario requires all ten pre-generation task checks and all
32 page visits. Fleet splitting adds five `year-2408-*.json` order snapshots
and `ship-transfer`/`fleet-split` hierarchy and map snapshots under
`tutorial-fleet-split-{dummy,x11}`. Schema additions affect test JSON only, not game/save records.

`--fleet-split` continues within year 2408 through the real Split button,
ship-transfer arrow, cargo gauge, Slime waypoint/task combo and Split All button.
It stops at page 27, with twenty-six pages completed (27/80 visited, 33.75%),
still returning partial/exit 2. Five copied order checkpoints independently
verify nine → ten → eleven owned fleets, conservation of all sixteen design
counts, the single-ship transfer, cargo conservation through Split All and
identical inherited colonization orders. Other fleets, production and research
must remain unchanged. Fleet snapshots now include `ships` and
`ownedFleetCount`; these are test JSON additions only.

`--research-cycle` finishes year 2408. It drags fleet 7's existing Slime
waypoint to Sea Squared, routes fleet 8 to Hiho, filters the unloading message,
reads the remaining messages and opens Wallaby's radiation popup. Research
inspection must open the Radiation Terraform 7 component popup before the real
budget arrows set 30% and Done commits it. Popup mouse-up is sent only after
capturing the visible popup, letting the game record normal tutorial progress.
Orders-before-generation must preserve fleet 10's Slime orders and all ship/cargo
counts, with only the intended routes and research budget changing. It stops at
year 2409/page 30 (30/80 visited, 37.5%; twenty-nine completed), still partial,
exit 2 and `fullTutorial: false`. Cross-build qualification is recorded in the
[goal report](LINUX-CLIENT-GOAL.md).

New artifacts include `year-2408-drag-orders.json`, `year-2408-scout-orders.json`,
`year-2408-orders.json`, `wallaby-radiation.json`, `radiation-benefit.json` and
`waypoint-drag`, `wallaby-radiation`, `research-benefits`, `radiation-benefit`,
`before-generate-2408`, `year-2409` hierarchy/map snapshots under
`tutorial-research-cycle-{dummy,x11}`. Checkpoints now include `filteredUnloads`.
Popup identity records use observed game fields, not inferred display text.

`--oxygen-colonization` finishes year 2409 through the message pane's actual
Goto actions, Research's next-field dropdown, mining-message filtering, cargo
loading, Oxygen colonization orders and the scanner drag from Oxygen to Mozart.
The research must remain Weapons/30%, with Construction selected as the next
field. The cargo checkpoint requires 25kT colonists in fleet 9, and the final
orders must preserve ships, cargo, production, planet flags and unrelated fleets.
It stops at year 2410/page 32 (32/80 visited, 40%; thirty-one completed), still
partial, exit 2 and `fullTutorial: false`. Cross-build qualification is recorded
in the [goal report](LINUX-CLIENT-GOAL.md).

New artifacts include `year-2409-start-orders.json`, `year-2409-research.json`,
`year-2409-cargo.json`, `year-2409-orders.json`, and `oxygen-summary`,
`mozart-waypoint`, `before-generate-2409`, `year-2410` hierarchy/map snapshots
under `tutorial-oxygen-colonization-{dummy,x11}`. Checkpoints now include
`filteredMiningLoads`. Goto must select the actual Oxygen summary; the dragged
waypoint must target Mozart (planet 4) at its copied universe coordinates.

`--production-templates` adds year 2410 dismantling-message filtering, Shaggy
Dog automatic production, preset 1 Import and Bloop Goto. It inserts Santa Maria
and Teamster at Stove Top queue positions 1 and 2. The qualified bounded stop is
year 2411/page 34 (34/80 visited, 42.5%; thirty-three completed), still partial,
exit 2 and `fullTutorial: false`. Qualification is tracked in the goal report.

Fresh hierarchy snapshots resolve the newest visible enabled modal. For the
nested template workflow, the menu driver sends real SDL keys to the enabled
modal when the frame receiving commands is disabled. Captures assert that
Customize Production Templates owns the disabled production dialog, closing
it restores that owner, and both logical dialogs disappear after their OK
buttons. No dialog result or menu command is fabricated.

Artifacts include `year-2410-start-orders.json`, `year-2410-shaggy-queue.json`,
`year-2410-imported-template.json`, `year-2410-template-orders.json` and
`year-2410-orders.json`; hierarchy/map captures include `template-open`,
`template-imported`, `template-owner-restored`, `template-closed`,
`shaggy-dog-summary`, `bloop-summary`, `before-generate-2410` and `year-2411`.
Orders JSON includes `filteredDismantles` and semantic `defaultTemplate`
validity, leftover-research flag, count and item/quantity pairs. The launcher
checks every earlier checkpoint and preservation of other fleets, research,
queues, planet flags and filters.

`--battle-cycle` drives year 2411's scout routes and Scrap Fleet task, Mozart
colonization, Wallaby troop transport with explicit Colonists/Unload All, 70
regular mines, Construction/Biotechnology research, Technology Browser Close
and Battle VCR controls. Its qualified stop is year 2412/page 38 (38/80 visited,
47.5%; thirty-seven completed; see the goal report). Captures check every route,
cargo, queue and unchanged unrelated state, modeless browser destruction and
battle identity through timer-driven play, pause, seek, rewind, step and Done.
A battle message's first Goto selects a fleet; its subsequent View opens the VCR.

Artifacts include `year-2411-start-orders.json`, `year-2411-colonizer-cargo.json`,
`year-2411-troop-cargo.json`, `year-2411-mines.json`, `year-2411-research.json`,
`year-2411-orders.json`, `battle-{open,playing,end,rewound,stepped}.json` and
fresh browser/battle/before-generation/year-2412 hierarchy and map snapshots.
Battle captures copy identity, token count, current/final step and playing flag;
the harness never assigns these game globals.

## Remaining execution

The longest implemented scenario is `--merge-cycle`, stopping at year 2418/page
50 (50/80 visited, 62.5%; forty-nine pages completed). It replays every earlier
workflow and checks all eighteen before-generation checkpoints. This is a
bounded stop: thirty pages and the final completion remain outstanding. Current
build/backend results and the continuation point are recorded below.
Event records now include `elapsedMs` for diagnosing slow or stalled actions.
The existing Windows instruction table in
`tests/reference/win32-tutorial/steps.ahk` is a source reference for the gestures, not
an execution dependency of the native runner.

The full runner still needs the remaining highlighted actions, dialogs, menu choices,
later orders-before-generation checkpoints, all 80 pages, years 2400–2436 and final
completion at turn 37. Unknown instructions, repeated actions, unexpected
dialogs and crashes must fail with artifacts. A partial development run cannot
be reported as a complete tutorial pass. The existing Windows/AHK tutorial
remains a separate reference; its previous Wine failure is not resolved by these
bounded native scenarios. Native Wayland/compositor qualification remains separate.

## Binary64 migration and extended checkpoints

After migration to double, the Terraform cycle passes all assertions through
41 pages/year 2413: Debug dummy (301.35 seconds), Release dummy (253.28 seconds)
and Release private X11 (469.16 seconds). Release smoke also passes on both
backends. The first sandboxed X11 launch failed before Openbox startup;
rerunning with the display permissions required by Xvfb passed. These bounded
runs retain `status: partial` and `fullTutorial: false`.

The Windows walkthrough was attempted in Debug and Release. Stars exited with
Wine's `free(): invalid next size (fast)` at page 15/QuikDrop and page 8/Dwarte
respectively. A third Debug attempt stopped at page 23/Red Storm colonization
with `free(): invalid pointer`. None is a complete tutorial pass. Their retained
artifacts are in `dist/scaffold/tutorial/20261012T061159.054289Z` and
`dist/scaffold/tutorial/20261012T061408.486019Z`, and
`dist/scaffold/tutorial/20261012T062913.577094Z`.

The new `--design-cycle` targets year 2414/page 45. It drives Teamster #1 cargo
and Slime unloading, Biotechnology/Propulsion research, browser Close, F4,
Mini-Miner hull copying and real component drags for Long Hump 6, Rhino Scanner
and two Robo-Miners. It queues the new miner and uses Ctrl+Add for 100 mines,
selects every production item and routes Armed Probe #9 to Stove Top before F9.
Copied ship designs, cargo, routes, exact queues and unrelated state are checked
before generation. `designs` is a test JSON addition; no game records change.

Early development attempts reached page 42 but queued keyboard navigation before
the preceding click established focus. Enter closed the designer, and arrow
keys could target its radio buttons. The harness now waits for combo focus,
then navigates and commits by clicking the observed popup row. No game dialog
behavior was changed for that correction. Design-cycle passes Debug dummy (343.59 seconds), Release dummy (309.66
seconds), Release private X11 (576.61 seconds) and fatal ASan/UBSan/LSan dummy
(474.12 seconds). LeakSanitizer required execution outside the tracing sandbox.
All 45 pages and fourteen before-generation checkpoints pass; the stop remains
partial, with `fullTutorial: false`.

`--automation-cycle` extends to year 2415/page 46 (57.5% visited), with 456
actions in the Debug headless development run. It inspects Sea Squared and
Oxygen through message Goto, inserts two Min Terraform (Auto Build) items,
imports the resulting queue into the default preset through nested dialogs,
and verifies exact queues, template items, owner restoration and unchanged
fleets/designs/research before generation. All current launcher assertions pass
against the retained run; Merge cycle repeats these checks in its longer run.

`--report-cycle` stops at year 2416/page 48. It covers Neil
scouting, Teamster colonists, Value sorting in the planet report, Wallaby
unloading, Research inspection and a return freighter route. These modes
retain partial coverage. The default CTest stop is now Merge cycle, which
replays all earlier workflow assertions. Earlier stops can be registered through
`STARS_TUTORIAL_CHECKPOINT_TESTS=ON`.

`--report-cycle` reaches year 2416/page 48 in the Debug headless development
run with 480 actions. All current launcher assertions pass against its saved
artifacts. Value sorting puts Wallaby first; the report closes through Escape,
and cargo, routes, production, research and unrelated state are checked.

`--merge-cycle` adds years 2416 and 2417. It routes Stalwart Defender to Wallaby,
sends Mini-Miner #8 to merge with Cotton Picker #6 at Prune, grows Stove Top's
automatic queues to 60 factories/60 mines, selects Construction as the next
research field and sends Teamster #1 home. The Debug development run reaches
year 2418/page 50 (62.5% visited), with 514 actions and eighteen successful
before-generation checkpoints. All current assertions pass against its saved
artifacts. Fresh Debug dummy (562.02 seconds) and Release dummy (521.47 seconds)
runs pass all launcher assertions, as do fatal ASan/UBSan/LSan dummy (709.39
seconds) and Release private X11 (908.64 seconds). Debug/Release dummy smoke
and Release private-X11 smoke also pass. Remaining
execution starts with instruction 392 and includes the later transport,
starbase/ship designs, reports, minefields, combat and final year-2437 completion.

## Session handoff, 2026-10-12

Work is on branch `win32_adapter`. Commit `95a20da` adds the differential
precision/Craig comparison harness; `51b6d38` converts all 178 production
extended-precision casts to double and updates the affected native baselines.
The subsequent native tutorial extension changes only its test harness,
registration and documentation. No game logic, tutorial observer signatures or
on-disk layouts change in that extension.

### Sources and reports

All paths below are relative to `/home/jdrake/Projects/stars`:

| Location | Contents |
| --- | --- |
| `docs/DOUBLE-PRECISION.md` | Migration results, known gameplay differences, possible rounding effects and platform limits |
| `tests/scaffold/PRECISION.md` | Reproduction commands and exhaustive versus sampled domain definitions |
| `tests/scaffold/precision*.py`, `precision_probe.c`, `precision_race.c`, `race_precision.py`, `craig_habitability.py`, `test_precision.py` | Historical x87 oracle, double comparisons, actual Craig Go comparison and harness tests |
| `dist/scaffold/precision/report.json`, `report.md` | 9,382,918 expression inputs and whole-calculator measurements |
| `dist/scaffold/precision/craig-habitability.json`, `craig-habitability.md` | 140,665 races and 49,454,448 planet inputs compared to Craig Stars |
| `dist/scaffold/precision/turns/summary.json`, `comparison.json`, `ext/`, `dbl/` | Independent fixed-seed host runs, nine scenarios through turn 150 |
| `dist/scaffold/precision/migration/` | Before/after native and host regression comparisons |
| `tests/scaffold/fixtures/regression/native/` | Updated native baseline; only `oneai5` and `smallai6` move, from turn 1 |
| `tests/compat/native_tutorial.cpp` | SDL gesture driver, copied state snapshots and stages 203–248 for the new workflows |
| `tests/compat/run_native_tutorial.py` | Isolated launcher and exact cargo/design/order/queue/template/report assertions |
| `cmake/sdl-client.cmake` | Default smoke plus Merge cycle on dummy/X11; earlier stops opt-in |
| `tests/reference/win32-tutorial/steps.ahk`, `actions.ahk`, `tutor.c`, `text/` | Original walkthrough gestures and actual tutorial task/text references |

The Craig checkout is read-only at `/home/jdrake/Projects/craig-stars`, pinned
in the comparison to `85b83c422f14529b61ec45716261c53d1ad515ce`. The old Stars
precision source is pinned to `3c67394973aacf6b0bf24d0a302f42b50a87bfa0`.
Generated `dist/` artifacts are ignored by Git; the scripts and documentation
needed to reproduce them are committed.

### Validation and retained artifacts

The MinGW Debug build passes without new warnings in touched files. Windows
units pass 55/55, host units 15/15 and precision harness tests 6/6. Both migrated
Windows and Linux host regressions match all 519 checkpoint files through turn
150; their 55 and 69 unused-storage warnings respectively remain documented
warnings, not differences. The Windows regression and unit suite were repeated
after the tutorial harness edits with the same result. Frozen original fixtures
are unchanged.

Each native tutorial artifact directory contains `result.json`, `events.jsonl`,
`client.log`, copied per-year orders and hierarchy/map JSON snapshots:

- `dist/sdl-debug/test-client/tutorial-merge-cycle-dummy/`
- `dist/sdl-release/test-client/tutorial-merge-cycle-dummy/`
- `dist/sdl-release/test-client/tutorial-merge-cycle-x11/`
- `dist/sdl-sanitize/test-client/tutorial-merge-cycle-dummy/`

All four final Merge cycle profiles pass the current launcher assertions with
514 actions, 50 pages visited, eighteen generation checkpoints,
`status: partial`, `fullTutorial: false` and auto-completion disabled. Their
final event is stage 248, turn 18, instruction 392. The bounded executable
returns 2 intentionally; the launcher validates that outcome and returns success.

Earlier development stops are also retained in
`dist/sdl-debug/test-client/tutorial-{design,automation,report}-cycle-dummy/`.
Important new evidence includes `value-report.json`,
`year-2413-orders.json` (Mini-Miner design), `year-2414-imported-template.json`,
`year-2416-merge.json`, `year-2416-orders.json`, `year-2417-orders.json` and
`year-2418-{hierarchy,map}.json`. Full-suite logs for this final checkpoint are
`/tmp/stars-merge-{debug-dummy,release-dummy,release-x11,sanitize}.log`;
required Windows regression/unit logs are
`/tmp/stars-tutorial-advance-{regression,unit,host-unit}.log`. `/tmp` logs may
disappear; the retained `dist/` artifacts are the useful evidence for resuming.

The three failed Windows/Wine walkthroughs listed above remain unresolved.
They are not complete tutorial passes. Earlier historical full Wine runs do
not validate the current executable. Native Wayland, ARM execution and the
entire SDL compatibility suite were not qualified by this session.

To reproduce the current native stop:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug --target stars-native-tutorial
ctest --test-dir dist/sdl-debug -R '^win32_native_tutorial_(smoke|merge-cycle)_dummy$' --output-on-failure
```

Use the same target and CTest pattern with `sdl-release` and `sdl-sanitize`;
Release X11 uses suffix `_x11`. Xvfb/Openbox need display permissions, and
fatal ASan/UBSan/LSan needs execution outside the tracing sandbox. Earlier
stop-only profiles require `STARS_TUTORIAL_CHECKPOINT_TESTS=ON`.

### Exact continuation point

Continue from native stage 248's bounded stop: **year 2418, page 50, highlighted
instruction 392**, `tutor.c` case 18 and Windows instructions 392–399. The next
workflow queues one Teamster at the front of Stove Top production, reads message
5 and uses Goto to 90210, opens Research with F5, inspects the expected-benefit
items, closes without changes, reads the remaining messages and generates year
2419/page 51. Later workflows include transport templates, starbase/stargate
design, report/mineral sorting, mine layers/minefields, further ship designs,
combat and the final completion at year 2437.

Reuse the focused combo helper (stages 221–223), cargo stages 205–207, order
stages 209–210, research stages 211–212 and shared production stages. Add a
bounded scenario with cascading flags, CLI support, page/turn/timeout mapping,
exact launcher assertions and CMake registration. Preserve real SDL events and
the rejection of auto-completion; copy evidence before every generation.

There is **no live native game to resume**. The launcher creates an isolated
temporary game, replays earlier actions, copies artifacts and exits. Saved JSON
snapshots are evidence, not resumable game state. Extend the next scenario from
the existing driver and replay from a fresh launch; do not assign tutorial flags
or game orders to skip earlier tasks. Finish all 80 pages and turn 37 before
reporting a complete tutorial pass.
