# Tutorial UI tests

The SDL client now also has an [in-process native runner](../../../docs/NATIVE-TUTORIAL-RUNNER.md).
Its `--smoke` scenario checks startup, rejected generation and the first map
order through three pages; `--first-year` continues through research and
generation to year 2401/page 6; `--two-years` adds the 20-factory production task
and reaches year 2402/page 7, retaining orders before both generations.
`--scouting` checks three multi-stop routes through page 10; `--mining` checks
the real fleet menu and task combo through page 11. `--colonization` adds summary
popup inspection, cargo loading and a Colonize order before generation to year
2403/page 13. `--logistics` adds message filtering, both planets' queues, cargo,
QuikDrop and waypoint deletion through year 2404/page 17. `--colonizer-production`
adds Stalwart Defender waypoint deletion and Santa Maria production through
year 2405/page 19. `--second-colonization` adds the cargo gauge, scanner toolbar
and Shaggy Dog orders through year 2406/page 21; `--transport-cycle` adds
QuikLoad/copy/QuikDrop, Repeat Orders and another colonizer through year 2407/page 23. `--expansion` adds Red Storm orders, larger production queues,
mine filtering and enemy design-popup inspection through year 2408/page 26.
`--fleet-split` adds ship transfer, cargo loading, Slime orders and Split All
through page 27 (27/80 visited, 33.75%; twenty-six completed), checking ship/cargo
conservation and inherited orders. `--research-cycle` finishes year 2408
through waypoint dragging, unloading filtering, radiation popups and the 30%
research budget, reaching year 2409/page 30 (37.5% visited; twenty-nine
completed). `--oxygen-colonization` adds message Goto actions, Construction
next-field selection, mining-message filtering, Oxygen colonization and a
Mozart waypoint drag through year 2410/page 32 (40% visited; thirty-one completed).
`--production-templates` extends this through preset Import, nested production
dialogs and queue insertion through year 2411/page 34 (42.5%; thirty-three
completed). Cross-build qualification is recorded in the goal report.
`--battle-cycle` adds year 2411 scout/scrap routes, colonization, troop unloading,
mine production, research, browser Close and Battle VCR playback toward year
2412/page 38 (47.5%; qualification pending in the goal report).
Default CTest profiles run smoke and the longest scenario on both backends;
`STARS_TUTORIAL_CHECKPOINT_TESTS=ON` registers all earlier stop-only profiles.
These
bounded scenarios are not full walkthrough passes and do not
replace the Windows reference suite described below yet.

The suite runs AutoHotkey **v2.0.28** and the rebuilt Stars! executable inside the same isolated Wine prefix. It drives normal Windows controls, menus, keyboard shortcuts and mouse gestures. The test build adds a hidden, read-only observation window; it does not complete tasks or modify game orders.

## Run

Install Wine, Python 3, CMake, Ninja and the x86_64 MinGW toolchain used by `mingw-debug`. 

```sh
make tutorial
make tutorial-reject
```

`--download-ahk` fetches the official portable release and verifies its pinned SHA-256 before extracting it under `dist/tools/autohotkey`. To supply an existing interpreter:

```sh
python3 tests/scaffold/tutorial/run.py --ahk /path/to/AutoHotkey64.exe
```

The runner builds `dist/tutorial-build/bin/stars.exe` with `STARS_TEST_TUTORIAL=ON` using `mingw-debug` by default. To test the optimized Release build, as the GitHub workflow does:

```sh
make tutorial TUTORIAL_ARGS='--build-preset mingw-release'
make tutorial-reject TUTORIAL_ARGS='--build-preset mingw-release'
```

This uses `dist/tutorial-release-build/bin/stars.exe` and records the selected preset in `metadata.json`. The observer option defaults to OFF for normal builds. `--exe PATH` accepts a prebuilt executable with that option enabled. Each run copies the executable and scripts into a fresh directory and uses its own Wine prefix, INI settings and game files. Wine prefixes are created under `/tmp/stars-tutorial-*`, outside the repository so Wine’s filesystem symlinks are not scanned by workspace tools. The absolute prefix path is printed at startup and recorded as `wine_prefix` in `metadata.json`. The original registration and game files are never modified.

Linux runs need an X11 display. For unattended runs:

```sh
xvfb-run -a -s '-screen 0 1600x1200x24' make tutorial
```

The virtual Wine desktop is 1600×1200. The game occupies 1280×960, and the tutor sits beside it so it cannot intercept scanner clicks. macOS Wine can execute the controls and gestures, but its GDI screen capture can return blank images. The suite logs that limitation instead of saving misleading screenshots or failing an otherwise valid walkthrough.

## Coverage and pass conditions

`run.py` reads all 640 fragments (80 pages) from `text/tutorial.txt`. It exports numeric IDs from `res/resource.h`, and checks that every highlighted instruction assigned by `FTutorTaskDone` (a `TutorId` name or number, resolved through `text/tutorial.txt`) has a dispatcher action. Missing actions fail before launching Wine. `coverage.json` records that inventory.

`steps.ahk` maps highlighted instructions to actions for years 2400–2436: navigation, message filtering, exploration routes, production and templates, colonization and transport, research, ship and starbase design, fleet splitting/merging, reports, battle playback and the final score report. `actions.ahk` contains the reusable UI operations. It uses resource IDs and observed HWNDs for standard controls, and current layout rectangles for custom controls. No RC caption changes are needed. The source tutorial calls planet 7 “Moholdi”; the actual planet name is “Mohlodi”, which the action uses.

At instruction 166, the script explicitly selects Teamster #4's Prune waypoint and checks the observed selection before shift-clicking Stove Top to append the return leg. Modified mouse gestures use Windows `SendInput` with an absolute mouse position in the same packet as button-down. They wait for the game to observe Shift or Ctrl held, verify delivered scanner coordinates and modifier bits, and wait for modifier release before continuing. The observer records both button-down and double-click messages with a delivery counter. Transfer button availability is observed from `bt & 4`, the flag used by the actual transfer UI, rather than the separate `fDisabled` field. These operations use the normal UI; the observer still only reads state.

A full pass requires all 80 pages, every required task before each Generate, no panic auto-completion, the final completion message and departure from tutorial mode at turn 37. Most tutorial Generate operations load canned turn files, so the suite checks the live task state and records the orders **before** generation. The final generation runs the normal simulation. Early victory messages do not count as tutorial completion.

The separate `reject-generate` scenario tries Generate before the first task is complete, checks that the turn does not advance, and verifies that the tutorial remains usable afterward.

For incremental development:

```sh
python3 tests/scaffold/tutorial/run.py --until-year 2403 --timeout 180
```

An intentional early stop reports `partial`, produces a skipped JUnit case, and exits nonzero. It cannot be mistaken for full coverage. Unknown instructions, covered click targets, unexpected tutorial errors, stalls, crashes, syntax failures and timeouts also produce nonzero results.

For debugging later actions without replaying every earlier year, launch with `--keep-game-on-failure`. After fixing the action, use `--continue-run /absolute/path/to/the/original/run --keep-game-on-failure` to attach to that live game in its existing prefix recorded in `metadata.json`. The prefix must still exist; clearing `/tmp` prevents continuation. Runs created before prefix paths were recorded require a fresh run. A continuation can report failures or `partial`; it cannot report a full walkthrough pass. It requires the game to remain running, and does not reopen a saved game. Finish debugging with a fresh normal run. To close a retained game explicitly, run `WINEPREFIX=/absolute/prefix/path/from/metadata.json wineserver -k`.

## Artifacts

Each run is retained under `dist/scaffold/tutorial/<UTC timestamp>/`:

- `result.json` and `results.xml`: explicit outcome and failure location.
- `events.jsonl`: instruction text, action times, queue contents and diagnostics.
- `year-2400.txt`, etc.: live orders and state before each generation.
- `last-state.txt` and `windows.txt`: observer snapshot and window/control inventory on failure.
- `screenshots/`: BMP captures where supported by the Wine display driver.
- `wine.log` and `metadata.json`: interpreter diagnostics, versions and executable/source hashes.
- `game/coverage.json`: required instruction inventory.

By default, the runner closes only its own game and Wine server. The explicit debugging option retains a failed game. Prefixes contain registration data, remain in `/tmp` for debugging, and may be removed by the operating system; the GitHub workflow uploads diagnostic files only. It runs the full walkthrough and rejection check on pull requests and supports manual runs. 

A populated instruction dispatcher is a coverage inventory, not proof that every gesture works on every Wine driver. `result.json` is the runtime authority; a full walkthrough is validated only when it reports `passed` with 80 pages at the final year.

The font/image help-tooling batch has a fresh passing full walkthrough in
`dist/scaffold/tutorial/20261011T024603.973906Z`: all 80 pages through year 2437,
346 actions. The MinGW build and all 49 Windows unit tests pass; the native
regression has 519 matches, 22 existing warnings and zero differences/errors.
That tooling does not modify the Windows game runtime, so this pass does not
establish that the intermittent Wine failures recorded below have been fixed.

The October 10, 2026 input/observer batch passes the fresh rejection check (`dist/scaffold/tutorial/20261010T225218.219258Z/`). Windows units pass all 39 tests, and the full native-baseline regression returns 519 matches (22 existing warnings) with zero differences/errors and no skipped reference inputs. No game behavior or baseline changed.

Instruction 166 passes in multiple fresh runs with the verified modifier and coordinate delivery. The full walkthrough is still not passing: the guarded observer run `dist/scaffold/tutorial/20261010T224041.252602Z/` exits at year 2407, page 23, instruction 179 with a Wine `user_check_not_lock` assertion. Other fresh runs abort with native heap errors. A diagnostic GDB launch (`dist/scaffold/tutorial/20261010T224935.761588Z/wine.log`) captures a native abort while selecting QuikDrop at instruction 114: `free` → `XDeleteContext` → Wine `winex11.so` → `NtUserDestroyWindow` → `NtUserTrackPopupMenuEx`. This identifies the failure path, not the origin of heap corruption. That diagnostic launch is not a passing walkthrough. The corrected transfer-button availability observation has not yet been validated at instruction 203 in a fresh full run.

The fresh menu-milestone run `dist/scaffold/tutorial/20261010T230200.331212Z/`
passes instruction 166 and reaches year 2407, page 24, instruction 186 after 94
actions. It times out on the observer's synchronous `SendMessage`; Wine reports
`free(): invalid next size (fast)`. The Windows executable does not link the SDL
menu implementation. Windows units now pass 40/40 and full native regression
still returns 519 matches, 22 existing warnings and zero differences/errors or
skipped inputs. This run remains a failure, not a full walkthrough pass.

The fresh popup-milestone run `dist/scaffold/tutorial/20261010T231900.117928Z/`
also fails at year 2407, page 24, instruction 186 after 94 actions, timing out on
the observer with `free(): invalid next size (fast)` in Wine. Instruction 166
passes. The Windows executable does not link the SDL popup implementation.
The final Windows unit suite passes 41/41, and full regression remains 519 matches,
22 existing warnings, zero differences/errors and no skipped inputs.

The October 11 battle-cycle batch passes all 103 fatal sanitizer tests, 51
Windows unit/reference tests, 519 native regression matches (87 existing
unused-storage warnings; zero differences/errors/skips) and four Release native
tutorial profiles. Its bounded native stop is year 2412/page 38 (38/80 visited,
47.5%; thirty-seven completed), not the full tutorial. The required fresh legacy
Wine run `dist/scaffold/tutorial/20261011T220906.282422Z/` fails at year 2407,
page 23/instruction 181 after 91 actions while selecting Stove Top for production;
the game exits and Wine reports `free(): invalid next size (fast)`. The origin
of this corruption remains unknown. Native passing results do not establish a
passing legacy Wine walkthrough, and the Windows executable does not link the
repaired SDL combo implementation.
