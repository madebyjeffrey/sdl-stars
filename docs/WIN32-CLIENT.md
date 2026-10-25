# Native SDL client preview

`stars-sdl` links the existing game core, shared Windows UI sources and local
Win32 compatibility implementation into a native Linux executable. Windows
continues to use the real SDK and the Linux host uses its existing host UI.
Basic inventory coverage is **178/178 Win32 APIs (100%)**, plus six CRT helpers.
This count does not establish full API conformance or client qualification.

## Build and debug

Follow [dependency installation](SDL3-DEVELOPMENT.md), then run:

```bash
cmake --preset sdl-debug
cmake --build --preset sdl-debug
./dist/sdl-debug/bin/stars-sdl
```

Reload CMake in CLion, choose the `sdl-debug` profile and select **stars-sdl**.
The same target is available under `sdl-release`. `--version` prints the
git-derived version without starting a graphical session.

The Linux entry point forwards arguments to the shared `WinMain`. The original
command parser does not understand quoting: each argument must contain no
whitespace and be shorter than 80 bytes; the combined command line is limited
to 1,023 bytes. Rejected input exits with status 2. The game parser and on-disk
structures are unchanged.

## Fullscreen title and owned dialogs

The original title screen is a borderless popup covering the display. An exact
display-sized popup without a caption, resize frame or menu now requests native
SDL fullscreen so desktop panels cannot cover its buttons. Ordinary windows
and dialogs retain their windowed presentation. Dialogs owned by a fullscreen
window also receive SDL native parenting, keeping them associated with that
window for desktop stacking. This does not make every application window
globally always on top.

`TitleWndProc` paints the splash and its buttons during creation, while the
native window is hidden. The startup test intercepts the first native show
and requires a populated splash canvas and composite before it becomes visible.
The user confirmed that the rebuilt Debug title background appears immediately
on the real desktop.

SDL's dummy backend has no native parenting support. HWND ownership, modal
disable/enable and input routing still apply in that backend; native-parent
assertions run in the X11 desktop test instead.

## Exit and resource lifetime

`ExitWindowsEx` rejects desktop logoff with `ERROR_NOT_SUPPORTED`. The legacy
`ExitWindows` macro follows the application's exit request by posting quit,
with a diagnostic that only Stars is exiting. No desktop session is terminated.

`WinMain` releases game heaps before returning. `StarsCompatProcessExit` then
releases compatibility windows, handles and services without invoking window
procedures that would access freed game state. Normal `StarsCompatShutdown`
retains destroy-message dispatch. A dedicated lifetime test checks this
distinction and subsequent runtime reinitialization.

## Tests and remaining qualification

The latest memory/control/inspection batch passes 93/93 tests in both Debug and
Release, and 93/93 fatal ASan/UBSan/LSan tests without suppressions. Headless
Debug and fatal sanitizers pass 80/80 each; Windows units pass 50/50. The final
inspector-only background paint is rechecked in both backends in all three builds
and in the headless preset, with a pixel assertion and offscreen visual check.
See [control qualification](WIN32-CONTROL-QUALIFICATION.md) and
[headless testing](SDL3-HEADLESS.md). Earlier client-startup results below are
retained as history.

For the fullscreen test install the test-only tools:

```bash
sudo apt install -y openbox x11-utils xvfb xauth
xvfb-run -a ctest --test-dir dist/sdl-debug --output-on-failure
```

The client startup tests execute the shared `WinMain`, paint the real title and
four buttons, inject SDL input to open New Game, verify the modal owner, cancel
with Escape, check owner re-enabling and close the title. Captures are written
under `dist/sdl-debug/test-client/{dummy,x11}`. The X11 test launches a separate
Xvfb display and Openbox, waits for window-manager readiness, and cleans up both.
Openbox is not used by the application's real desktop session.

Before the memory/control/inspection batch, Debug and Release each passed
all 89 registered tests (61 compatibility,
16 client and 12 host tests). Existing unit-test path-truncation warnings
remain; no new warning category was introduced. The rebuilt native SDL executable
matches the full native regression baseline: 519 comparisons, 22 existing
warnings and no differences; the report is
`dist/scaffold/regression/sdl6-comparison.json`. The MinGW Debug build passes. Windows unit tests
passed 49/49. After the splash fix, the full Windows regression also passed
519 comparisons with 22 existing warnings and no differences or errors.
The latest Wine tutorial exited at year 2402, page 9, instruction 68
after 32 actions while Shift-selecting Kalamazoo; artifacts are in
`dist/scaffold/tutorial/20261011T043042.072275Z`. The prior run exited while
Shift-selecting Shaggy Dog at instruction 64 after 28 actions, in
`dist/scaffold/tutorial/20261011T040410.047884Z`. The preceding run timed out at
year 2407, page 24, instruction 186 after 94 actions. An earlier run exited during
QuikDrop selection at year 2403, page 15, instruction 114 after 55 actions.
Neither run passed. That failure is not counted as
a passing tutorial check.

Further qualification includes the corrected title and wizard on the user's
desktop, Wayland and hardware rendering, client-linked UI unit cases, native
tutorial automation, deeper control behavior and installation outside the
source tree. The client remains a preview until those gates pass.

## Main frame, menus and Continue Game

Default width in `CreateWindowExA` now selects both dimensions even when the
caller supplies zero height, matching the Win32 contract. The startup test
checks usable frame dimensions. Menu dropdowns use `SDL_CreatePopupWindow`
with the owner and parent-relative placement on native backends; the dummy
backend retains logical ownership. Screen/client conversion accounts for SDL
popup offsets. This prevents compositor focus changes from cancelling menus.
Creation also sends the initial top-level `WM_SIZE` notification, so the
frame's layout state is ready before game panes are created. The Continue Game
test checks the frame dimensions against that state and requires a usable
scanner area, then paints all panes and saves `test-client/continue-dummy/game.bmp`.
The menu test checks native popup ownership, placement and SDL pointer-driven
command selection. Its X11 CTest runs inside private Xvfb/Openbox. The menu
contract also passes on the user's Xwayland desktop with accelerated rendering.

`FOpenGame` now checks `chDirSep` instead of dereferencing an unchecked search
for a Windows backslash. The Continue Game test copies frozen game fixtures,
uses isolated settings, clicks Continue, and checks the populated scanner and
visible main frame for both relative and absolute paths. The fixture's password
prompt is bypassed only by the test executable's linker wrapper. Saved data
and turn generation are unchanged. The user confirmed that Continue Game opens
the game. That test also opens Technology Browser twice and closes it by button
and native close request. `BrowserDlg` now uses `DestroyWindow` for its modeless
dialog instead of modal-only `EndDialog`.
The user confirmed both the Close button and the window's X now close it.
The initial pane layout was also confirmed on the desktop. `ReadIniTileSettings`
now normalizes the C library's uppercase classification mask to a boolean,
preserving expanded panel flags in their one-bit fields. The Continue Game test
loads saved uppercase tile settings and checks all planet/fleet expansion flags.
All startup/wizard tests now use separate settings directories as well as
separate virtual displays; desktop tests run sequentially.

Focused window, menu, popup and process-exit sanitizer tests passed 10/10.
The subsequent memory-safety batch repairs ordinary heap alignment and other
undefined accesses, passing all 91 fatal sanitizer tests without suppressions;
see [native port](NATIVE-PORT.md).

## Race-wizard habitat repair

The reported habitat-page crash was reproduced by painting the shared
`RaceWizardDlg2`: `DrawRace2` copied a 67-character template into `szT[32]`.
It now formats directly from the compressed-text table; turn behavior and
save layouts are unchanged. The shared race unit test covers initial painting,
all three immunity transitions, restoring normal ranges and repainting.

The wrapper also paints initialized dialogs and their children before showing
the native window, preventing an empty black first frame. The SDL input test
opens Customize Race, advances through all six pages, checks the habitat
background immediately before `SDL_ShowWindow`, saves `habitat.bmp`, and cancels
both nested wizards. Captures are under `test-client/race-{dummy,x11}`. The
`stars-sdl-unit-tests` target links all 14 unit-test programs against
the client UI; these are separate from the host tests. The user confirmed that
the habitat page works and page transitions retain one desktop window.

## Game and player selection

The main library lists each managed game once. Its separate Player dropdown
uses player IDs as item data and displays each available turn's year. Selecting
a game rebuilds that dropdown, using the remembered player where available;
refreshes preserve a still-valid game/player selection. Open Game and game
double-click use the dropdown's player. If Continue's remembered turn is missing,
it selects the game, clears the dropdown selection and focuses it for an explicit
choice. Host Games hides both the dropdown and its label.
The Continue Game button has a fixed short caption; its remembered game, player
and year (or Choose Player prompt) appear on a separate line below the selector.

## Continuous race-wizard pages

`RaceCreationWizard` now brackets its modal pages with an SDL dialog sequence.
Back and Next retire each logical HWND and its controls while retaining the
native SDL window, renderer and completed frame for the next same-sized page.
The existing native owner attachment stays in place instead of reattaching
the visible window on every transition. The owner stays disabled across
transitions; new drawing is presented only after initialization
and painting finish. Cancel/Finish or a failed creation release retained
presentation and restore the original owner/focus. Ordinary dialogs retain
their existing behavior.

Native exposure during initialization also resubmits the retained frame: the
next page is logically hidden while the reused native window remains visible.
Caption updates happen after the replacement page is registered so exposure
events can resolve it. The creation path skips native synchronization and icon
setup that the retained window has already completed.

The SDL input test checks that all six pages, including Back and then Next,
share one SDL window ID and retain the display texture while the previous HWND
becomes invalid. Cancellation checks that the SDL window disappears and the
New Game owner is enabled again.
An injected exposure during caption changes checks that the previous frame is
redrawn before the next page is shown. Every submitted wizard frame is also
checked for its navigation controls, catching background-only presentations.
Focused dialog tests check nested-sequence rejection, pending-window cleanup
and resource-creation failure. The process-exit test also checks silent cleanup
of a pending native wizard window after application state is released. The user
confirmed that transitions now stay in one desktop window.

The `sdl-sanitize` preset is available in CLion. Initial qualification exposed
signed race masks and unaligned native heap payloads. Both are repaired, along
with the later sentinel/scalar findings. The memory batch passes 91/91 fatal
ASan/UBSan/LSan tests without suppressions, 50/50 Windows units and 13/13 host
units. Full Windows and Linux host regressions each return 519 matches,
87 unused-storage warnings and no behavioral differences; baselines remain
unchanged. This qualifies the exercised paths, not every possible game state.

The [built-in inspector](WIN32-INSPECTION.md) opens with Ctrl+Shift+F12 and exposes
stable logical IDs for in-process automation. The [control follow-up](WIN32-CONTROL-QUALIFICATION.md)
adds clipboard/undo, line queries, extended list input and held-scroll repeat.
Their final qualification is recorded in those reports. Native map-object
inspection and the tutorial driver remain next steps.

The native entry point accepts `--headless`, and `ctest --preset sdl-headless`
runs invisible UI workflows sequentially; see [headless testing](SDL3-HEADLESS.md).
