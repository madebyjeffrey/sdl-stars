# Roadmap

The `2.6jrc3` tag is the faithful reconstruction, original bugs included, and
the `2.6j` branch keeps it. 2.8 is released (tag `v2.8.0`, branch `2.8` for
patch releases). 2.9 is released (tag `v2.9.0`); `main` is the 2.9 line,
and its builds count toward 2.9.1.

## SDL desktop consolidation

The desktop product uses SDL3 through `compat/win32` on every platform. The native
Win32 client build is retired; the host remains independent of SDL. Desktop
presets, unit tests, regression entry points and release packaging use `stars-sdl`.
Windows SDK comparison tests remain opt-in, with no second product UI. Retired
native-client branches are removed from the shared UI sources; OS-specific
filesystem and argument services remain in the platform layer.
Windows now uses Unicode native open/save dialogs through an isolated SDK bridge;
Linux portal and macOS AppKit pickers retain their existing backends. Headless
tests select the constructed picker with `STARS_FILE_DIALOG_BACKEND=compat`.

Desktop identity uses the approved `res/icons/stars-app.png` artwork through SDL,
the Windows executable's `stars-app.ico`, and Linux launcher icons. Original
small game icons remain separate. `make mac-bundle` now builds a signed arm64
`Stars!.app` with the approved ICNS, offline help and relocatable runtime
libraries. Signing discovers a usable keychain identity on every build and
verifies nested signatures; see [macOS bundling](MACOS-BUNDLE.md). Finder and
Terminal launches of a relocated development-signed bundle are verified.
Developer ID distribution/notarization, document associations and older macOS
runtime qualification remain outstanding. The host stays a separate CLI.

Source reorganization is complete: `src/core`, `src/ui`, `src/host` and
`src/platform` retain the original filenames. CMake, generators and source-reading
test tools follow these paths. The retired Wine/AutoHotkey tutorial and native
new-game scripts are archived in `tests/reference/win32-tutorial`; active SDL
workflows remain in `tests/compat`. Generated build trees are confined to `dist/`. Local cleanup removed 23 verified
untracked CMake trees and the empty backup directory, and archived 154 old scratch
files plus 11 scratch/install directories under `dist/archive/pre-sdl-consolidation`.
Current qualification logs are grouped under `dist/verification/sdl-migration`.
Actual Windows native open/save creation, owned modality and cancellation are
verified. `GetCursorPos` now accepts the Windows SDL driver, repairing desktop
cursor queries; driver/coordinate contracts have a regression test. Manual selection/overwrite/scaling and fresh Linux/macOS execution remain
outstanding. Windows Merge cycle reaches year 2413/page 42, then fails at stage
216/instruction 332 during the Mini-Miner engine drag; the default smoke passes.
Active unit/desktop tests and optional regression tracing now use private C
adapters instead of GNU linker wrapping. macOS Debug enables the unit suite.
The C adapter contract is compiler-independent; full MSVC product qualification
still requires replacing the existing MinGW-only build gate/flags and validating
the dependencies. Full Apple-silicon desktop and host regressions both match
all 519 comparisons through turn 150.

Linux SDL unit, regression and tutorial CI install the desktop backend
development dependencies from the SDL development guide; the earlier package
list stopped fetched SDL configuration at XCURSOR. Windows SDL unit and release
CI explicitly select UCRT64 Python so CMake uses the installed MSYS2 PyYAML
package rather than the runner interpreter.

Deferred frame maximize/minimize commands now show normally on the SDL dummy
driver, which cannot implement those native window operations. This keeps
headless client and tutorial workflows visible. Child composition and position
checks finish pending tree paints before inspecting the presented image,
matching batched presentation. Scanner drag checks also complete the tree
paint pass for each simulated input tick before counting its presented frame.

Combo selection callbacks can return focus to the frame without cancelling
the accepted row; a re-entry regression test covers that path. Instruction
updates no longer reactivate an already visible tutor, preserving focus in
the player's current control. New-game cancellation checks cover the live wizard
page returned by the save picker before verifying the restored title and
game state after cancelling the wizard itself.

Full SDL tutorial coverage remains outstanding. Historical Win32 milestones below describe the wrapper's API work,
not an additional maintained desktop backend.

## 2.9

Split the game into separate builds so the host can run on Linux:

- **Core:** turn generation and file I/O, with no Windows UI dependencies.
  Done: the game files (`STARS_CORE_SOURCES`) include only `common.h` and
  reach the player only through `ui.h`.
- **UI:** the SDL client using the Win32 wrapper on top of the core. Done: the `*ui.c` files and
  the other UI files (`STARS_UI_SOURCES`) include `win.h`.
- **Host:** a command-line build of the core (`-g`, `-a` and the like) for
  Windows and Linux. Done: `stars-host` (`host.c`, `hostui.c`), released for
  x86-64 Linux; the MinGW build also makes `stars-host.exe`.

To keep in mind:

- The Linux host must generate the same turns as the Windows build. The
  native regression baseline is the test: the Linux host's run must match
  it. Watch the floating operation boundaries (`double`), `qsort16` tie order and
  the RNG first.
- Each build shows its platform in its version name (done; see
  [VERSIONING.md](VERSIONING.md)).

- The file format is 2.85 for UTF-8 text; record widths remain fixed.

Verified: `stars-host` matches the native baseline under Wine and on x86-64
macOS (`make regression-host`), and its `-v` check file matches
`stars.exe`'s. CI runs the regression and the native unit tests (`make
test-host`) on x86-64 Linux.

The dumps (`-dm`, `-dp`, `-df`, which TotalHost's movie scripts use) are
game code too (`report.c`, with the report window in `reportui.c`):
`stars-host` writes them, byte for byte as `stars.exe` does, and
`test_report` checks them against golden files on both builds. With `--ini`
it reads the `stars.ini` settings a host uses, so TotalHost can run it
instead of `stars.exe` under Wine ([TOTALHOST.md](TOTALHOST.md)).

The release workflow publishes `stars-host-linux-x64.tar.gz`, linked
statically, beside `stars.exe`. 2.9.0 is released (tag `v2.9.0`).

The 2.8 reproduce-first items (5.3 below) still apply; fix them on `main`
and cherry-pick to `2.8`.

## Linux desktop compatibility

The native product workflow matrix now covers Basic/Advanced game creation,
ship/starbase hulls and component drag/remove/reinsert, design accept/cancel/delete,
and actual Save/MRU reload alongside the existing fleet, research, production,
reports and map checks. See [gameplay qualification](GAMEPLAY-QUALIFICATION.md).
The full tutorial and real desktop/hardware qualification remain separate.

The Wayland graphics leak is traced to Mesa's runtime assembler retaining its
empty executable heap across library unload. A private, pinned dependency patch
releases the heap on its last free; lifetime/SDL/native Wayland sanitizer tests
verify the repair. See [Mesa qualification](MESA-GRAPHICS-LEAK.md). System drivers
remain unchanged; hardware and desktop acceptance still require qualification.

The first SDL3 milestone is available through the `sdl-debug` and
`sdl-release` presets: `stars-compat-smoke` exercises two independent
windows, CPU surfaces, GPU presentation and a software fallback. It does
not yet run the game UI. Milestone 2 adds scoped Win32 compatibility
headers, `stars-ui-compat-check` to compile the existing UI on Linux, and
ABI assertions shared with a real MinGW SDK reference test. The first Free
API adaptation adds C11 rectangle operations and tick counts. The next
milestone adds top-level window creation and message dispatch, verified by
`stars-compat-windows` with two independent windows. See
[window milestone](WIN32-WINDOWS.md). Basic GDI painting, pens/brushes,
fixed-font text and a resize/close drawing test are now available through
`stars-compat-paint`; see [painting milestone](WIN32-PAINTING.md). Memory DCs,
compatible bitmaps and ROP3 bitmap blits are now implemented and checked
against Windows; see [bitmap milestone](WIN32-BITMAPS.md). Uncompressed DIB
conversion and all 38 bitmap resources now work and share pixel tests with
Windows; see [DIB milestone](WIN32-DIBS.md). Fixed/movable heap handles and
window/thread timers now pass shared native/Windows tests; see
[memory/timer milestone](WIN32-MEMORY-TIMERS.md). Narrow UI formatting, strings,
arithmetic and CRT helpers now pass SDK comparisons; see
[UI services](WIN32-SERVICES.md). Profile settings and legacy file adapters now
pass shared tests; see [files/profiles](WIN32-FILES-PROFILES.md). Bundled
proportional/styled/rotated fonts and text layout now work; see
[fonts/text](WIN32-FONTS.md). Copied application clipping regions and
excluded holes now work across drawing paths; see [regions](WIN32-REGIONS.md).
Logical origins, ROP2 operations and copied pattern brushes now work; see
[drawing state](WIN32-DRAWING-STATE.md). Ellipses and cosmetic dash/dot
lines and rectangles now work; see [geometry](WIN32-GEOMETRY.md).
Logical child windows, nested composition, coordinate conversion, IDs, focus
and enable state now work; see [child windows](WIN32-CHILDREN.md). Menu data, mutation and submenu ownership now work; see [menus](WIN32-MENUS.md). Window menu attachment and interactive popup menus now work; see [popup menus](WIN32-POPUPS.md). Menu resource loading and accelerator dispatch now work; see [UI resources](WIN32-UI-RESOURCES.md). Visible menu bars and dropdown routing now work; see [menu bars](WIN32-MENU-BARS.md). Icons/cursors now load and draw with preserved AND/XOR masks and hotspots; see [icons and cursors](WIN32-ICONS-CURSORS.md).
Logical palettes and palette-indexed drawing now work on the true-color backend;
see [palettes](WIN32-PALETTES.md). Interactive modal message boxes and asynchronous
SDL audio alerts now work; see [message boxes](WIN32-MESSAGE-BOXES.md). Modal open/save
pickers now preserve Stars wildcard filters and filename contracts; see
[file dialogs](WIN32-FILE-DIALOGS.md). Default-printer DCs and legacy document/page
escapes now emit PDFs and can submit them through CUPS; see [printing](WIN32-PRINTING.md). The original help archive now has a checked native reader and a CMake-generated HTML guide with 414 topics, formatted text/tables and all 133 images; all 413 numeric contexts, 1,498 text links and 28 image hotspots resolve. The `WinHelp` adapter now has tests for all 413 contexts; the user confirmed offline browser launch, with offline content checks still remaining; see [help](WIN32-HELP.md). Win32 subset coverage is **178/178 (100%)**, with six CRT helpers counted
separately. This is basic inventory coverage, not complete Win32 conformance.
The native `stars-sdl` preview now links the shared client; title-screen painting,
New Game/wizard cancellation and process cleanup have integration tests.
The splash image and buttons are painted before the first native show, with
an integration check at that boundary.
The intro now opens as a centered, decorated desktop window with an 800x600
client area, bounded by the desktop work area. Its startup, failed-load and
return-to-title paths share the same creation function. The SDL presets now link all 14 game unit-test programs against the client UI.
The race wizard retains one native window across page transitions. A
default-width request now selects both frame dimensions, and native SDL menu
popups retain their owner relationship. `FOpenGame` now accepts Linux paths;
Continue Game is tested with relative and absolute fixture paths. The
initial top-level size notification now initializes pane layout, and the
modeless Technology Browser closes correctly by button and native close request.
A separate `sdl-sanitize` preset now instruments project code; unsigned race-bit masks are
repaired. Ordinary heap alignment, host-sentinel access, unaligned scalar stores,
empty-record copying and negative scanner shifts are repaired. The memory batch
passes 91/91 fatal ASan/UBSan/LSan tests without suppressions and both full
regressions match all 519 comparisons (87 unused-storage warnings).
`DrawRace2` habitat-page stack corruption is fixed and tested; dialogs paint
before native exposure to avoid an initial black frame. SDL input tests traverse
the race wizard to the habitat page. Real-desktop qualification remains; see
[client](WIN32-CLIENT.md).
Positioning, sibling ordering, no-redraw behavior and minimum/maximum
size callbacks now work; see [positioning](WIN32-POSITIONING.md). Mouse capture
now routes child and cross-root input; see [capture](WIN32-CAPTURE.md). Queue-based
keyboard state, asynchronous queries, point hit testing, UTF-8 input, wheel
delivery and double-click messages now work; see [input](WIN32-INPUT.md). Push/check/radio
buttons, group boxes, text labels and dialog-item helpers now work. Stars now
uses `ToolbarWindow32` with common-control commands, split/dropdown buttons and
anchored tooltips; Windows uses `comctl32`. See [toolbars](WIN32-TOOLBARS.md). Radio
selection dots are round, toolbar menus anchor below their triggers, and tooltips
use control-relative placement with an owned native tooltip role; see
[controls](WIN32-CONTROLS.md). Basic narrow edit fields now support selection, limits,
read-only/password behavior and input; see [edit fields](WIN32-EDIT.md). Complete
edit conformance remains. Fixed-height string lists, multiple-selection messages
and owner drawing now work; see [list boxes](WIN32-LISTS.md). Additional list
conformance remains. Editable and fixed-height string/owner-drawn combos now work;
see [combo boxes](WIN32-COMBOS.md). All 34 canonical dialogs now create. Basic modal/modeless dialog lifecycles
now work; see [dialogs](WIN32-DIALOGS.md). All 34 dialog templates
now load and parse; see [dialog templates](WIN32-DIALOG-TEMPLATES.md).
Display/work-area metrics, point-array mapping and window identity now work;
see [desktop services](WIN32-DESKTOP.md). Active top-level window queries now
distinguish keyboard focus from desktop activation; see [activation](WIN32-ACTIVATION.md).
Minimize/maximize queries, child state transitions and saved normal placement
now work; see [window state](WIN32-STATE.md). Root attention requests now use SDL
without changing focus; see [attention](WIN32-ATTENTION.md). Standard/control
scroll positions and ranges now work. Child scrollbar drawing, arrow/page clicks,
thumb dragging, keyboard requests and held-arrow/page repeat now work; standard
nonclient bars, client reflow and page-size thumb sizing now work. Drag updates
present the completed map and thumb once; standard bar position changes no longer
force full map invalidation, and window pixel scrolling uses overlap-safe row copies.
The shared corner of both bars uses Windows 95 button-face gray. See [scroll controls](WIN32-SCROLL-CONTROLS.md).
Edit controls now support clipboard/undo, line queries, Ctrl-word navigation and
multiline Up/Down. Lists now support Shift ranges, Ctrl caret/toggle behavior,
mouse drag/autoscroll and incremental search. Multiline wrapping, formatting rectangles, edit scrolling, caret/context menus,
basic IME preedit, visible list bars, horizontal extents and Ctrl-drag now work.
Broader control conformance remains; see [control qualification](WIN32-CONTROL-QUALIFICATION.md).
A built-in [window inspector](WIN32-INSPECTION.md) exposes logical IDs,
parent/owner relationships, state and JSON snapshots for in-process automation.
Styles and control kinds now expand into symbolic names. The local headers add
934 SDK-checked constants; declarations do not imply runtime support. See
[constants](WIN32-CONSTANTS.md).
The `--headless` launch mode and headless CTest presets now run the same UI
invisibly and sequentially; see [headless testing](SDL3-HEADLESS.md).
A game-aware map inspector now enumerates copied player-client object IDs, names,
coordinates, filter/selection flags and supports selection/reveal with stale-key
checks. Drawn objects are exposed separately from HWNDs. The
[native tutorial runner](NATIVE-TUTORIAL-RUNNER.md) now drives a bounded
startup/rejection/map-order smoke and bounded year-2400/2401 scenarios through
real SDL input and copied inspection data. They check research, fleet orders and
the 20-factory production task before generation to year 2402/page 7. Further
bounded scenarios check three scouting routes and a remote-mining order through
page 11, and summary-popup/cargo/colonization actions through generation to
year 2403/page 13. Logistics actions cover message filtering, automatic production,
cargo, transport presets and waypoint deletion through year 2404/page 17
(17/80 visited, 21.25%; sixteen pages completed). Colonizer production covers
Stalwart Defender waypoint deletion and Santa Maria queue insertion through
year 2405/page 19 (19/80 visited, 23.75%; eighteen completed). Further scenarios
cover cargo-gauge loading, scanner view buttons and Shaggy Dog colonization
through year 2406/page 21, then QuikLoad/copy/QuikDrop, repeat orders and colonizer
production through year 2407/page 23 (23/80 visited, 28.75%; twenty-two completed).
Expansion covers Red Storm orders, larger production queues, mine filtering and
enemy design-popup inspection through year 2408/page 26 (26/80 visited, 32.5%;
twenty-five completed). Fleet splitting checks ship transfer, cargo loading,
Slime orders and Split All through page 27 (27/80 visited, 33.75%; twenty-six
completed), preserving ships, cargo and inherited orders. The research cycle
adds waypoint dragging, unloading filtering, planet/component popup identity
and research-budget checks through year 2409/page 30 (30/80 visited, 37.5%;
twenty-nine completed). Oxygen colonization covers message Goto actions,
Construction next-field selection, mining-message filtering, cargo and the
Oxygen/Mozart routes through year 2410/page 32 (32/80 visited, 40%; thirty-one
completed). Production templates add automatic factories/mines, nested preset
Import, Bloop Goto and colonizer/transport queue insertion; the new bounded
checkpoint reaches year 2411/page 34 (34/80 visited, 42.5%; thirty-three
completed). Battle cycle adds scrap/scout routes, colonization, troop unloading,
mine production, research, browser Close and VCR playback through a qualified
stop at year 2412/page 38 (38/80 visited, 47.5%; thirty-seven completed). Cross-build
qualification is recorded in the goal report. Subsequent native runner development
extends through Mini-Miner design, terraforming templates, Value sorting and merge
orders to year 2418/page 50 (50/80 visited, 62.5%). The extended checkpoint still
reports partial coverage; later workflows and final year 2437 remain outstanding.
See [NATIVE-TUTORIAL-RUNNER.md](NATIVE-TUTORIAL-RUNNER.md) for current verification.
The default longest scenario checks every earlier
checkpoint; replaying earlier stop-only profiles is opt-in.
Mining uses a 900×700 frame;
the task combo is clipped by its parent at 640×480, which remains an open layout
issue. The native resource generator
now includes the three recorded tutorial data resources, previously omitted.
Full 80-page execution and later checkpoint evidence remain pending.
`ScrollWindow` now moves client pixels, pending damage and whole-client children;
paint regions preserve holes and support partial validation. See
[content scrolling](WIN32-SCROLL-WINDOW.md). Compatibility window
targets now open on native Wayland with owner-local geometry and native popup
roles. Private Weston Debug tests cover integer scaling and software/OpenGL
presentation; the SDL-only LSan report, real hardware/compositor acceptance and
macOS execution remain unqualified. See [desktop qualification](DESKTOP-QUALIFICATION.md). See
[Free API adoption and coverage](FREE-API-ADOPTION.md),
[compatibility headers](../compat/win32/README.md) and
[SDL3 development](SDL3-DEVELOPMENT.md) for dependencies, CLion setup and
smoke checks, and the [implementation plan](LINUX-SDL3-WIN32-PLAN.md) for
remaining work.

### Linux client follow-up priorities

The broader goal is paused. Current work completes gameplay workflows, UI polish
and packaging (milestones 3, 8 and 9), after the private Mesa leak repair; tutorial
expansion is paused.
Message wheel navigation and a scrollable planet/fleet viewport are implemented;
see [UI polish](UI-POLISH.md) for qualification.
The previously selected scope is milestones 1, 2, 3, 4, 7, 8 and 9. Track
completion and evidence in [Linux client goal](LINUX-CLIENT-GOAL.md).
PDF printing/viewer handoff and self-contained offline help acceptance are now
implemented; see [printing](WIN32-PRINTING.md) and
[help acceptance](OFFLINE-HELP-ACCEPTANCE.md).
Flatpak and the relocatable CMake runtime layout are now built and locally
qualified: both launch and close normally outside the checkout on private X11.
See [packaging](LINUX-PACKAGING.md) for commands, permissions and remaining
desktop/help/GPU-driver qualification.


- **Open/save dialog overhaul:** use native platform pickers (Linux libdbus
  FileChooser portal, Unicode Windows dialogs (implemented), macOS AppKit behind defines).
  Retain the custom picker for deterministic headless tests; improve its layout and usability,
  directory navigation, file/filter presentation, filename editing, keyboard
  interaction and error/overwrite feedback. Qualify non-ASCII filenames and
  scaling. Preserve Stars' wildcard filters and open/save filename contracts;
  cover actual game/race open and save flows with sequential headless tests and
  desktop checks. See [file dialogs](WIN32-FILE-DIALOGS.md).
- **Print Map to PDF and open (implemented):** checked document/page lifecycle,
  atomic PDF output and platform viewer handoff, with retained output and a
  visible recovery path on viewer rejection. Actual Letter/tiled/A4 game-map
  tests and Poppler verification cover every planet. Physical printing is done
  in the user's viewer. See [printing](WIN32-PRINTING.md).
- **Control qualification:** owner-drawn lists now send focus actions, including
  the empty-list focus indicator used by Stars. Shared SDK checks cover nonempty
  focus and notification ordering; native checks cover empty painting and
  callback mutation/destruction. Single-selection owner-drawn rows also send
  synchronous selection actions, including keyboard changes. Multi-selection
  action sequences, broader drawing state, notification and undo edges remain. Edit undo now groups adjacent Backspace/Delete operations
  and retains history when insertion is rejected at the text limit; shared SDK
  tests cover these boundaries. Upper/lower edit styles now normalize initial
  and programmatically set text too. Empty/read-only undo now preserves SDK
  return, selection, history and notification contracts. Text-changing undo/redo
  now sets modified state and selects restored ranges with SDK notification
  ordering. See [control qualification](WIN32-CONTROL-QUALIFICATION.md).

Loaded-game workflow tests now cover research commit/reopen/restore and
production edit/cancel/commit/reopen, battle-plan checkbox/tactic changes and
player-relation radio changes through hierarchy inspection and SDL input.
Ship-design copy/cancel/accept, rename/cancel/accept and deletion checks also
pass headless/private-X11 qualification. They exposed default OK/Cancel commands
prematurely ending the modal; default handling now leaves termination to the
application. A dedicated fleet profile checks Merge list selection, Cancel
without fleet changes and acceptance preserving aggregate ships/cargo/fuel.
The fleet profile also checks Repeat Orders persistence/restoration and Split
transfer/Cancel/OK with conserved ship/cargo/fuel totals. Hull/component editing,
starbase design, broader cargo transfers and route editing remain outstanding.
Planet/fleet mineral transfer now has Cancel/accept/reverse checks, and waypoint
task changes have commit/reselection/restoration checks. Four built-in transport
presets now have real popup-input assertions, and scanner Shift-click append,
destination dragging and waypoint-list deletion restore the original order.
Multiple route legs, Ctrl gestures and customized presets remain unqualified. Combined workflow
qualification exposed paint starvation from empty scrollbar geometry; the
handler now validates pending paint even for zero-width/height controls.
Low-height fleet-panel reachability still needs layout qualification. Split
qualification exposed misaligned packed-button rectangles; drawing and tracking
now copy those rectangles before native access without widening records.
Relations are tested by temporarily switching the copied human/AI fixture to
multiplayer mode in memory; loading a multiplayer save remains unqualified.
They exposed and guard against modal default handling overwriting an application
cancellation result; see [goal evidence](LINUX-CLIENT-GOAL.md). Broader gameplay
workflows and the native tutorial remain outstanding.

These priorities remain subject to the completion evidence in the goal tracker. Basic API coverage stays
178/178 (100%); the work improves behavior and user experience.

## Unicode portability

UTF-8 text is implemented throughout the new client and host, including
catalog/data generation, resources, controls, rendering, clipboard and settings.
Format 2.85 explicitly identifies UTF-8 saves; older game and race files are
unsupported. Native paths must round-trip losslessly, and fixed text fields
retain byte budgets. See [encoding contracts](UNICODE-BOUNDARIES.md) and
[ADR 0002](adr/0002-utf8-application-text.md). Translation, shaping,
bidirectional layout and grapheme editing remain separate work.

Password creation and changes use the same 15-byte UTF-8 limit as login and
the command line, including complete-code-point truncation at the edit limit.
Generated fleet names, clipped pane titles and bounded wrapper labels preserve
complete code points as well; byte counts remain the public API convention.
Name readers use the actual record extent, including orders; compressed bytes
are never retried as raw text. Player-message readers validate decoded UTF-8
and raw terminators before retaining a message.

## 2.8 (released)

Work proceeded in this order:

1. Native regression baseline and 2.8 repository rules (done).
2. Product versioning from git tags and build numbers (done).
3. Reverting the Win16 parity emulation, one commit each (done).
4. Replacing the `win16defines.h` shims with native Win32 code, one group at
   a time, behavior-neutral (done; what the port still needs is in
   `native.h`/`native.c` and `nativeui.h`/`nativeui.c`).
5. Original bug fixes from [KNOWN-BUGS.md](KNOWN-BUGS.md) (below).

## 5. Bug fixes

Every fix follows the same steps, in one commit:

1. Add a unit test that reproduces the bug and fails on the current code.
   This is what turns a "located" mechanism into a confirmed bug.
2. Fix it. The test passes.
3. Regenerate the native regression baseline for any scenario that moves
   (`tests/scaffold/REGRESSION.md`, "Update the native baseline") and name
   the moved scenarios and first turn in the commit message.
4. Add a `CHANGELOG.md` entry (mark it **host results** if turn generation
   changes), update the bug's entry in [KNOWN-BUGS.md](KNOWN-BUGS.md), and
   update its row below.

Fixes apply whenever a 2.8 host generates the turn; there is no rules option.
File records don't change, so a 2.8 host can take over a 2.6j game; 2.8
writes file version 2.84, which 2.6j refuses.

The balance items in [KNOWN-BUGS.md](KNOWN-BUGS.md) (chaff, battle board
overload, 0.2% minimum damage, profitable scrapping, mine damage allocation)
are legitimate strategy and stay as they are. Split fleet dodge keeps the
jrc4 retargeting (done). False public scores was on this list; it is fixed
as an exploit.

### 5.1 Unit test harness (done)

Ported the acutest setup from the earlier `stars-decompile` effort; see
[tests/unit/README.md](../tests/unit/README.md). The game sources build once
as the `stars_core` and `stars_ui` object libraries, linked into `stars.exe`
and into one acutest executable per `tests/unit/test_<file>.c`. CTest runs
the tests under Wine (`make test-unit`, and the unit-test CI workflow); 2.9
also builds those that need no windows natively (`make test-host`). Acutest
runs each test in its own process, so tests start from fresh globals.

- Function tests call one function with crafted inputs.
- Turn tests use `stars_test.h` to create a tiny game (optionally with AI
  players), load the host file and generate turns.
- The native-port checks moved into `test_native_ports.c`.

Turn-test setup helpers so far: `LpplStarsTestHomeworld`,
`LpflStarsTestAddFleet` and `FStarsTestSaveHost`. Add planet, minefield
and order helpers as fixes need them.

### 5.2 Confirmed in code: test, then fix

| Bug | Mechanism | Test | Status |
| --- | --- | --- | --- |
| Cybertron starbase defenders | `ai4.c` `DoCyberAiTurn`: after `CheckAiShdefStatus(14, 15, …, &iLatestSBDefender, …)` the recycle clear tests and clears `iLatestDestroyer` again | turn | fixed (`test_ai4.c`) |
| North/South minefield immunity | `FIntersectCircleLine`: for a vertical route `yI = ptL1.y` instead of `ptC.y` | function | fixed (`test_utilgen.c`) |
| Space Dock armor overflow | `UpdateShdefCost`: armor slot strength overflows signed 16-bit `dpT` before the RS halving | function | fixed (`test_util.c`) |
| ISB vs IT gate scanning | `SetVisPFFleets`, `SetVisPFPlanets`, `SetVisPFThings`: `lRadius2 * lVis2` overflows 32 bits before `/ 10000` | function | fixed (`test_save.c`) |
| Repair after gating | `Merge2Fleets` drops the merged fleet's `fNoHeal` before `HealShips` | function | fixed (`test_ship.c`) |
| Exploding minefield dodge | `ThingDecay` sets `fBombed` even when the fleet was immune | function | fixed (`test_turn2.c`) |
| Colonization module check | `SatisfyOrders` colonize task doesn't test `cItem > 0` | turn | fixed (`test_turn3.c`) |
| 32k ships per fleet | `Merge2Fleets` adds into `int16_t rgcsh` without a limit; `FFleetMergeAll` clamps to 32766 and loses ships | function | fixed (`test_ship.c`) |
| "Stuck" pursuit | `MoveFleets` marks the pursued fleet `fDone` though it has its own pursuit | function | fixed (`test_turn.c`) |
| SS pop steal | `SatisfyOrders` transport load sets `fDone` but still loads colonists or fuel it may only steal; the binary never uses its "attempted to shanghai colonists" and "attempted to steal fuel" messages (0x124, 0x125) | turn | fixed (`test_turn3.c`) |
| Mineral upload | `FRunLogRecord` destroys cargo another player's fleet can't hold after taking it from the source | function | fixed (`test_log.c`) |
| Claim Adjuster turn files | `FWriteDataFile` raises every included player's record to full detail for a CA player so it carries their habitat (`DrawMineSurvey` ally planet values), which also sends their tech, research, traits, production template and relations | turn | fixed (`test_save.c`) |
| Target list overload | `ScannerWndProc`, `ClickInShipOrders` popups stop at 100 entries | UI | fixed (no unit test) |
| Race file corruption | `RaceWizardDlg1` reads the names without clearing their buffers, `IRaceChecksum` covers the stale bytes after the terminator, and `ReadRtPlr` zeroes them on load | function | fixed (`test_race.c`) |
| Turn-file knowledge leaks | `FWriteDataFile` writes whole `THING` records, so a player's file carries other players' bits in the Mystery Trader, minefield and wormhole masks and the trader's part (see [TOTALHOST.md](TOTALHOST.md)) | function | fixed (`test_save.c`) |
| False public scores | `CalcPlayerScore` reads populations after waypoint 1 cargo moves, not as they produced | turn | fixed (`test_turn2.c`) |

### 5.3 Needs reproduction first

Build the reported setup as a turn test. If it misbehaves, find the cause
and move the bug to 5.2. If it doesn't, record that in
[KNOWN-BUGS.md](KNOWN-BUGS.md). The release notes in
[26JFIN.txt](26JFIN.txt) list the original developers' fixes through 2.6j
(2.6 and 2.7 shared code: one was the CD release with battle sounds, the
other the serial-unlocked shareware), which can point at causes.

| Bug | Starting point (see KNOWN-BUGS.md) | Status |
| --- | --- | --- |
| East/West speed bump immunity | `FTravelThroughMineFields`, `FIntersectCircleLine` horizontal case | not reproduced (`test_turn.c`) |
| Cheap starbase (UltraStation variant) | `PROD.pct` against edited designs | fixed (`test_log.c`): a design change or deletion clears queued progress |
| [freepop] hack | `COLDROP` replay in `FRunLogRecord` | open |
| Random race | `RaceWizardDlg1` template persistence | fixed (`test_race.c`) |
| AR starter colonies | starter-base queue entry in `Produce` | open |
| Sappers fail to close to range 2 | `DpFromPtokBrcToBrc` move scoring | open |
| WP0 pop reload ignored | pending drops vs colonize task | open |
| Crash Stars | base-design indices in battle and minefield damage; TotalHost's warning points at a fleet refuelling at the last player's 10th base design ([TOTALHOST.md](TOTALHOST.md)). `FuelFleets` indexes base designs correctly, and the AI's `isb + 2` upgrade check in `aiutil.c` stays within the ten slots | open |
| Starbase friendly fire | `CplrBattle` attack masks (already changed by the native repair) | not reproduced in 2.8 (`test_battle.c`) |
| VCR shields and point of view | `RegenShield`, `UpdateShdefCost` use of `idPlayer` | open |
| Stargate mineral transmutation | the 2.70i notes list "Fixed mineral transmutation bug when unloading for stargate jumps"; 26JFIN.txt's copy of that list omits it. Check `FStargateJump`'s cargo unloading. `MoveFleets` unloads each mineral into the same type on the source planet before the jump, and `FStargateJump` doesn't touch cargo | open |

Font problems on non-English Windows and the Netscape attachment corruption
are outside the game code and are not planned.

## Other

- **Tutorial drawing artifact:** an automated tutorial run sometimes drew
  black squares on selected planets, around turn 26 (year 2426). The
  pre-cleanup build did it too, and it hasn't been reproduced by hand. A
  full run after the step 4 cleanup didn't show it; close this after a few
  more clean runs. The likely draw sites are the selected-planet marker in
  `scan.c` `DrawScanner` (an 11×11 `SRCAND` mask, then an `SRCPAINT` color
  blit) and the starbase and stargate indicators (a 5×5 `BLACKNESS`
  `PatBlt`, then a `PATCOPY` fill). Tracking it down needs screenshot
  capture in the tutorial runner. macOS Wine's GDI capture returns blank
  images, so use another capture method or a Linux/Xvfb run.
- **Mouse wheel and panning:** the wheel zooms the scanner at the cursor,
  scrolls the report tables and goes to the window under the cursor, and
  left-drag pans the scanner. Message-pane wheel navigation and scrolling
  expanded planet/fleet sections are also implemented and tested; see
  [UI polish](UI-POLISH.md).


The year-2411 native tutorial development found and repaired an owner-drawn
combo repaint loop: read-only row queries invalidated its popup while painting,
starving timers. The combo regression now checks query-driven drawing leaves
no pending popup damage. Qualification passes 103 fatal sanitizer tests,
51 Windows unit/reference tests, the full 519-match native regression and all
four Release tutorial profiles; see the [goal report](LINUX-CLIENT-GOAL.md).
No game baseline changes.

The initial Unicode filename/password boundary work is superseded on main by
UTF-8 throughout and format 2.85. See [encoding contracts](UNICODE-BOUNDARIES.md).
Legacy settings migration is deliberately omitted; fresh UTF-8 settings are
used. Shaping and Windows/Linux/macOS platform qualification remain separate
from the encoding contract.

The SDL wrapper now also builds natively on Windows x64 through MSYS2 UCRT64,
with separate SDK and wrapper symbols, Unicode filesystem services and portable
DLL packaging; see [Windows SDL3](WINDOWS-SDL3.md). Windows ARM64/32-bit and
MSVC qualification, native chooser/printer integration and a public Rust FFI
remain open. Save layouts and the native regression baseline stay unchanged.
The SDL Windows headless checks include actual New Game save-picker workflows;
the separate SDK reference suite
still has native desktop failures, including the Unicode caption/password
probe in `test_native_fileui`. Their results are recorded in the Windows SDL
build guide rather than treated as passing Windows SDK qualification.
