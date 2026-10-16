# Changelog

Changes from the original Stars! 2.6jrc3 (tag `2.6jrc3`, branch `2.6j`).
Entries marked **host results** change turn generation, so a 2.8 host can
produce different results from a 2.6j host for the same turn.

## Unreleased

- `FInspectImportFile` accepts the empty end records written by the game's
  orders and history writers. Complete game exports containing saved orders
  can now be reimported; trailing bytes are still rejected. File formats,
  host results and native regression baselines are unchanged.

- `FExportSelectedLibraryGame` asks for a destination before resolving dirty
  orders. Canceling the folder chooser no longer saves the loaded game, and
  choosing Don't Save is requested only once per export. Host results and
  native regression baselines are unchanged.

- `FExportLibraryGame` rejects destinations whose full game or backup paths
  exceed the loader's capacity, instead of silently truncating filenames and
  reporting an incomplete export as successful. Host results and native
  regression baselines are unchanged.

- The desktop Games library can export a complete game family to a folder named
  for its library entry. Exports include all player files, host state, and
  backups, retain the managed original, and leave existing destinations intact.
  Game behavior and disk record layouts are unchanged.

- Existing managed games now accept a valid later standalone player turn,
  including skipped years. The importer matches by game identity, asks which
  managed copy to update when several match, treats identical turns as no-ops,
  and retires only that player's old orders after the staged turn is installed.
  History, backups, other players and imported source files are preserved.
  Game simulation and disk record layouts are unchanged.

- Race designs can now be duplicated to the next free `.rN` filename, exported
  as ordinary race files, and deleted after a confirmation naming the selected
  design. Export validates the copied file and retains its existing checksum
  and password data; failed exports preserve any existing destination, and
  cancelling deletion leaves the managed design in place.
  Game behavior and record layouts are unchanged.

- The desktop title screen now manages race designs in a separate Race Designs
  folder. New and Edit use the race wizard and save directly to the selected
  design; Import opens the copied design in the wizard, and cancelling keeps
  the imported file. Imports preserve the original bytes and receive a unique
  `.rN` filename. Cancelled edits restore the loaded race, and saves retain
  the existing checksum and password behavior. Game behavior and record
  layouts are unchanged.

- Managed race saves and imports now publish from private staging directories,
  preserving existing designs with names that resemble the former staging
  suffix.

- Managed race filenames now reject Windows reserved device stems, including
  `CON`, `COM1` through `COM9`, `LPT1` through `LPT9`, and `CLOCK$`.

- Race Designs now lists only nonempty race files, not matching directory names.

- macOS UTF-8 and profile compatibility tests now link the C++ filesystem bridge
  that provides their C-callable path helpers.

- The desktop Games library can import a game family from a selected file or
  folder. It validates format, record framing, game identity and player slots
  before copying the universe, available turns, companions and backups into a
  uniquely named managed folder. Imported source files stay untouched, and
  player rows show their own turn years. Simulation behavior and disk record
  layouts are unchanged.

- `TestUtf8Profile` was compiled but never called on Windows SDL, failing its
  warning-as-error build and omitting Unicode profile coverage. The call now
  uses the same platform guard as the definition; the fixture uses a portable
  temporary directory and a Unicode filename instead of a hard-coded `/tmp`
  path. Game behavior, save layouts and baselines are unchanged.

- `TestPicker` used native Win32 filesystem APIs through SDL wrapper headers,
  preventing Windows SDL test builds. A small C++17 filesystem bridge with a C
  ABI now supplies strict UTF-8 fixture paths and portable directory operations.
  Picker coverage includes Unicode directories, and hierarchy capture avoids
  the Windows CRT's narrow temporary-file path. Game behavior, save layouts and
  baselines are unchanged.

- `FOpenFile` checked for format 2.85 before classifying unsupported versions,
  making its older/newer version messages unreachable. It now reports valid
  older and newer BOF versions accurately and treats malformed BOF records as
  corrupt, preserving that diagnostic across read-error recovery. Reader tests
  cover older, newer, malformed and current-format host files; save layouts
  and simulation results are unchanged.
- `FWasRaceFile` now imports protected legacy races only after validating the
  original records and checksum, confirming any shortened names and accepting
  a replacement UTF-8 password. The old password is not authenticated, and
  its hash is replaced before the race can be saved or used directly in New
  Game creation. Cancellation stops both paths; race writes remain format
  2.85. Race/player state, native regression baselines and turn generation
  contain no copied legacy hash and are otherwise unchanged.
- `FWasRaceFile` now imports unprotected race designs from formats 2.49–2.84.
  A bounded legacy parser verifies record framing and the original CP1252
  checksum before converting names; pre-2.55 singular-only records gain a
  plural form. Names that exceed 31 UTF-8 bytes are shortened only after the
  player confirms the proposed complete-code-point names. Protected legacy
  races follow the replacement-password workflow in the next entry; legacy
  game, turn, history and orders files remain rejected. Imports leave their
  source files unchanged, and saves use 2.85. Gameplay attributes, native
  baselines and host results are unchanged.
- `NewPasswordDlg` let the edit control silently keep a shortened prefix when
  password input exceeded 15 UTF-8 bytes. Password fields now reject the whole
  oversized edit or paste and ask the user to enter the password again; other
  text fields retain truncation. The rejection remains until the field changes,
  so no-op Backspace or Delete cannot clear it. Exact-limit Unicode passwords
  remain valid. Native baselines and turn generation are unchanged.
- `WriteBattlePlan` used a 36-byte buffer for raw names whose length marker,
  31-byte text and terminator require a 37-byte record. Its temporary buffer
  now covers that existing record size. Disk fields, native baselines and
  host results are unchanged.
- `FFinishPlrMsgEntry` passed a negative raw-message length to `memcmp`,
  reading past unchanged uncompressed UTF-8 messages and marking them changed.
  It now compares the stored byte count. Native baselines and host results
  are unchanged.
- `FWriteScanString` still interpreted scanner snapshot names as CP1252,
  double-encoding UTF-8 in exported JSON. It now validates and preserves UTF-8
  bytes while escaping JSON quotes and controls. Unterminated snapshot names
  are rejected. Native baselines and turn generation are unchanged.
- `GenNewGameFromFile` checked the 16-bit truncated file size, allowing a
  large definition to wrap into a small readable prefix. It now checks the
  actual native file size before allocation. Native baselines and valid
  definitions are unchanged.
- `PszFormatString` copied expanded names and paths past its 256-byte buffer.
  Its expansions are now bounded, and message/printf truncation preserves
  complete UTF-8 code points. Normal messages, native regression baselines
  and turn generation are unchanged.
- `UnpackBattlePlan` could leave an existing plan partially changed on a
  rejected name. It now validates into temporary text and reports failure;
  `FRunLogRecord` increments the plan count only after success. Native
  regression baselines and valid host results are unchanged.
- Wrapper list/menu/class setters and `OpenFile` could retain malformed UTF-8,
  and picker navigation could retain an unsupported resolved directory.
  These inputs and print destinations are now validated; picker extension
  abbreviations preserve complete code points. Native baselines are unchanged.
- `GenNewGameFromFile` validated only its title and copied its final output
  path without a capacity check. It now validates the whole UTF-8 definition,
  accepts an optional BOM and bounds the output path. Native regression
  baselines and results for valid definitions are unchanged.
- `ReadRtPlr`, `FReadShDef`, `FReadFleet` and `UnpackBattlePlan` could read
  names past their record boundary; `FRunLogRecord` treated failed compressed
  fleet names as raw bytes. Readers now require complete, bounded UTF-8 text,
  and `FDecompressUserString` rejects incomplete escapes. Player messages are
  validated before entering memory, and game/header/race-checksum records
  require their fixed sizes. Valid files, native baselines and host results
  are unchanged; malformed records are rejected or skipped as appropriate.
- Generated fleet names and clipped mineral/message titles could split UTF-8
  characters, causing strict drawing to reject whole labels. These cuts and
  bounded menu, class, file-title and font/printer label copies now preserve
  complete code points. Native regression baselines and gameplay are unchanged.
- `NewPasswordDlg` allowed 16-byte UTF-8 passwords, which the 15-byte login
  field and command line could not accept. Both change-password fields now
  use the login byte limit and preserve complete code points. Native regression
  baselines and turn generation are unchanged.
- Application text now uses UTF-8 on all platforms: generated catalogs/data,
  resources, controls, clipboard/composition, drawing, reports and passwords.
  `WriteBOF`, `FOpenFile`, `FWasRaceFile`, `FMarkFile` and `FLoadLogFile`
  write/require format 2.85 instead of accepting legacy encoded text. Record
  sizes stay fixed; text and password limits count UTF-8 bytes. Existing game,
  race and non-ASCII password compatibility is deliberately dropped. Settings
  start fresh in `StarsUtf8.ini` with a UTF-8 BOM. Unsupported native paths
  report conversion errors. The help build uses a UTF-8 bundle, and tutorial
  assets use 2.85 with their old battle event headers converted in place.
  **Serialized host results:** all nine native scenarios move from checkpoint
  0 solely because BOF version 2.84 becomes 2.85; no simulation fields change.
- `LibraryNameDlg`, `FChooseLibraryGameName` and `FFinishLibraryGame` passed
  two compressed strings from the same shared buffer to `MessageBox`, making
  the body and caption identical according to compiler argument order. Library
  alerts now preserve the error message separately from the Games caption,
  fixing GCC new-game workflow checks. Turn generation, file formats and
  regression baselines are unchanged.

- `FGetNewGameName` now asks for a portable, unique library name. Desktop games
  and all their generated companions live in per-user application data and can
  be reopened from the startup Games list after restarting. Failed or cancelled
  creation does not publish a library entry. Session metadata is stored outside
  game records; host paths, game titles, formats and simulation baselines are
  unchanged.

- Removed the suspended Rust/Bevy startup experiment and its optional build
  integration. `WinMain` uses the original SDL title-screen workflow for every
  desktop build. Turn generation, file formats and regression baselines are
  unchanged.

- New-game checks waited for the title after cancelling the save picker,
  although cancellation now returns to the live wizard page. The checks
  verify the chosen size remains and no file was created, then cancel the
  wizard and verify the title and original game state before retrying Finish.
  Production code and regression baselines are unchanged.

- Scanner-drag checks counted presentations before the completed tree paint
  pass, reporting zero frames under batched presentation. Each simulated
  drag tick now finishes that pass and still requires one frame per update.
  Production code and regression baselines are unchanged.

- `ShowTutor` reactivated the visible tutorial window on instruction updates,
  taking keyboard focus from the player's controls. It now shows the tutor
  only when hidden, preserving focus while instructions change. Turn
  generation, file formats and regression baselines are unchanged.

- `CloseCombo` allowed focus changes during selection callbacks to reenter
  cancellation, reverting the displayed row after the game accepted the new
  task. Closing now guards against re-entry, preserving the accepted selection.
  A regression test moves focus during `CBN_SELCHANGE`. Turn generation,
  file formats and regression baselines are unchanged.

- Child-window composition and positioning checks read the presented image
  before pending window-tree paints completed. Their inspection helpers now
  finish pending paints before asserting pixels, matching the batched
  presentation contract. Production code and regression baselines are unchanged.

- `ShowCompatWindow` repeatedly applied deferred maximization on SDL
  dummy displays, which cannot maximize native windows, leaving the frame
  hidden and breaking client/tutorial checks. Deferred maximize/minimize
  requests now show normally on that headless driver. Desktop behavior,
  turn generation, file formats and regression baselines are unchanged.

- Windows SDL unit and release CI let CMake select the runner Python without
  PyYAML instead of the installed MSYS2 interpreter. Debug and Release
  configuration now explicitly use UCRT64 Python and its PyYAML package.
  Game behavior, file formats and regression baselines are unchanged.

- Linux SDL CI omitted desktop backend development packages and stopped at
  SDL configuration with a missing XCURSOR dependency. Unit, regression and
  tutorial workflows now install the documented desktop backend dependencies.
  Game behavior, file formats and regression baselines are unchanged.

- `DialogBoxParamA` restored only logical focus when closing dialogs such as
  the ship designer, leaving their macOS owner without native keyboard focus.
  Modal dialogs, completed wizard sequences and active modeless owned windows
  now reactivate a visible, enabled owner and restore its previous control.
  Shutdown/quit, hidden or disabled owners, nonactivating cards and explicit
  close-handler focus choices are preserved. Game behavior and baselines are
  unchanged.

- Added `make mac-bundle` / the `macos-bundle` preset to package and sign an
  Apple-silicon app with its icon, offline guide and relocatable runtime
  libraries. Signing discovers an eligible keychain identity on each build,
  signs nested code with Hardened Runtime, verifies the result and preserves
  the previous app on failure. Ordinary platform builds, game behavior and
  file formats are unchanged; notarization remains separate work.

- Replaced GNU linker wrapping in active unit/desktop tests and the optional
  regression tracer with private C test adapters. macOS Debug now builds the
  unit suite; test directory cleanup supports the Windows CRT, and the heap
  reuse test accommodates native alignment and forces free-list coalescing.
  Production symbols, game behavior, disk formats and baselines are unchanged.

- Ordinary SDL windows now request focus when shown, including the modeless
  technology browser, reports and tutorial. Disabled/minimized windows, child
  controls and nonactivating detail cards remain excluded; `SWP_NOACTIVATE`
  is respected. Game logic, file formats and regression baselines are unchanged.

### Added

- The approved planet, spacecraft and sans-serif "2" artwork now identifies the
  SDL desktop on macOS, Windows and Linux. Windows embeds it as the executable
  icon, and Linux installs matching launcher artwork. The original small game
  icons and turn generation remain unchanged; the macOS bundle uses the same artwork.

- `GetCursorPos` in the SDL wrapper previously rejected the Windows video driver,
  preventing native desktop cursor queries. It now reads SDL's desktop-relative
  mouse state on Windows as on X11/Cocoa, retaining coordinate validation and
  unsupported-backend errors. Turn generation and regression baselines are unchanged.

- Added an opt-in Windows desktop check that opens actual system open/save
  pickers and automatically cancels them, verifying native ownership, message
  pumping and directory preservation. Final source-layout qualification records
  the successful builds/regressions and the Windows tutorial's page-42 drag stop.

- Archived the retired Wine/AutoHotkey tutorial and native new-game harness in
  `tests/reference/win32-tutorial`, retaining gestures and qualification evidence.
  Active test instructions use the SDL workflow runners; generated CLion build
  directories are ignored. Third-party origins, licenses and frozen checkpoints
  remain intact.

- Moved game, desktop, host and platform sources and headers into their own
  `src/` directories, preserving filenames and contents. Build paths, text ID
  usage scanning, precision experiments and the tutorial parser follow the new
  layout. Generated output stays under `dist/`; disk formats and baselines stay
  unchanged.

- Removed the retired native UI command-line, Unicode-dialog and label adapters.
  Desktop sources now directly use the SDL wrapper; Windows filesystem and
  argument services remain available to the host and SDL entry point. The SDL
  preprocessor output of the six affected UI files is unchanged.

- The desktop client now builds only through SDL3 and the Win32 wrapper on
  Windows, Linux and macOS. The native Win32 product target is retired; SDK
  contract tests are opt-in. Presets, CI, game tests, regression commands and
  Windows runtime packaging use the SDL client. Source locations are unchanged.
- Windows wrapper open/save dialogs now use native Unicode system pickers,
  preserving Stars wildcard filters and shared filename validation. Headless
  tests retain the constructed picker. Game rules, disk layouts and regression
  baselines are unchanged.

- `FJoinFile` in the Windows SDL file browser now normalizes backslash paths before splitting
  filenames and changes Unicode directories through the wide CRT. New Game
  qualification now opens the actual save picker, tests Cancel, then creates
  Basic/Advanced games with ASCII and Unicode filenames/directories. This
  corrects the Windows adapter; host results and the baseline do not change.

- Native Windows x64 SDL3 builds now use the same Win32 source wrapper as the
  Linux/macOS client. UCRT64 Debug/Release presets and a PowerShell packaging
  script produce a portable runtime without WSL or MSYS2 on the user's PC.
  Wrapper symbols are isolated from SDK imports; Windows paths/settings use
  UTF-8 at the boundary. Game rules, disk layouts and regression baselines
  remain unchanged. See `docs/WINDOWS-SDL3.md` for scope and qualification.

- The native SDL tutorial runner now drives Mini-Miner design and component
  drags, automatic terraforming templates, planet Value sorting, merge orders
  and further transport/production/research tasks through year 2418/page 50.
  Checkpoints verify exact designs, cargo, routes and queues through normal UI
  input. Coverage remains partial; the final year-2437 walkthrough is unfinished.

- `CommandHandler` now starts one Print Map document and checks page/document
  completion instead of restarting each page and ignoring failures. The SDL
  PDF backend opens only completed output in the platform viewer; viewer
  rejection retains the PDF and reports its UTF-8 path. Actual Letter/tiled/A4
  map tests verify all planet marks and unchanged game state.
- Offline help now exports one self-contained document with all 414 topics,
  original diagrams, fonts, navigation and popups. Context selection survives
  OpenFile portal export without relative assets or URL fragments. Real
  headless Firefox acceptance walks the entire guide without a content server.

- The SDL entry point now sets the `org.starsgame.Stars` application identity
  and product version before creating windows, matching its installed launcher
  instead of SDL's executable-name fallback. Entry/exit sanitizer tests pass.

- Private X11 install verification now tolerates windows destroyed during
  startup discovery while retaining fatal checks for other protocol errors
  and cleanup of its owned application/window-manager processes.

- Linux product packaging now provides the `sdl-package` CLion/CMake preset,
  relocatable runtime install rules and a local Flatpak bundle on Platform
  26.08. Installed help finds its assets relative to the executable; tracked
  source exports are reproducible, versioned and hashed for reliable caching.

- The planet/fleet pane now scrolls expanded sections at small window sizes,
  with proportional thumbs, live dragging and accumulated wheel input. Its
  existing controls keep their content coordinates and remain reachable.

- `MessageWndProc` now supports wheel navigation between messages, accumulating
  partial notches and preserving the current message when Ctrl/Shift is held or
  a message is being composed. Long message edit controls retain text scrolling.
- A pinned, private Mesa dependency patch fixes the runtime assembler's empty
  executable-heap leak on driver unload. A concurrent lifetime regression,
  SDL-only Wayland check and optional CMake test prefix qualify the fix without
  replacing system graphics libraries or suppressing sanitizer reports.
- Native Windows paths and file selection use UTF-16 APIs behind UTF-8 application
  boundaries. Open/save offsets remain byte offsets; Unicode paths work through
  core reads/writes, existence checks, backups and cleanup. Fallback file-picker
  fields now edit/draw UTF-8 with font fallback, preserving clipboard and filename
  bytes. Native captions and hierarchy inspection retain their encoding.
- `ParseCmdLine` converts native UTF-8 passwords to the existing CP1252 bytes and
  rejects malformed, unrepresentable or overlong input. Password fields use
  explicit conversion and SDL rejects unsupported password input/paste instead
  of substituting `?`. Existing file layouts and password hashes are preserved.
- `LSaltFromSz` explicitly performs the original 32-bit wrapping arithmetic,
  removing signed-overflow undefined behavior exposed by long accented passwords.
  Direct race loads retain complete UTF-8 cache boundaries and full UI suggestions;
  filename abbreviations no longer lowercase or cut inside UTF-8 bytes.

- Native Wayland windows leave desktop placement to the compositor instead of
  failing creation. Menus/tooltips retain parent-relative coordinates; pointer
  queries and captured input use surface coordinates. High-density windows keep
  logical control sizes while presenting at the display pixel density. Private
  Weston checks exercise software and explicit OpenGL presentation at 100%/200%.
- macOS SDL client presets enable the product and Cocoa picker without the
  Linux/GNU-linker test harness. macOS execution remains unqualified.

- The intro screen opens in a centered desktop window with a title bar, close
  and minimize buttons instead of covering the screen. Returning to the intro
  uses the same window layout; its artwork and version text use client coordinates.

- Linux SDL open/save calls use the desktop FileChooser portal through libdbus
  by default, preserving Stars filters and filename validation while returning
  native UTF-8 paths. Platform defines select an AppKit panel adapter on macOS;
  the existing Windows build keeps system common dialogs. Headless mode selects
  the deterministic compatibility browser.

- SDL open/save pickers resize their file list and fields, accept editable
  locations, keep Back history, navigate Up/Home, toggle hidden files and create
  folders through a cancellable name dialog. Existing filename contracts remain.

- The SDL compatibility layer adds standard scrollbars with page-aware thumbs,
  list horizontal scrolling and Ctrl-drag selection, multiline edit wrapping and
  formatting rectangles, caret/context menus and basic SDL IME preedit. Combo
  dropdown scroll styles retain the closed field's normal arrow and width.
  Scanner and report bars now supply visible page sizes for proportional thumbs,
  preserving their existing last scroll positions.

- A built-in scanner inspector exposes copied player-client map objects and JSON
  snapshots with stable game/turn/player/kind/ID keys, coordinates and filter
  state. Select/reveal uses the normal secondary scanner selection path without
  issuing orders. The hierarchy inspector opens it through a game UI callback.

- The SDL compatibility UI uses Windows 95 colors and two-level raised/sunken
  borders for menus, toolbars, buttons, fields, lists and scrollbars. Checkboxes
  draw ticks; split menu arrows retain independent press state. Linux desktop
  title bars and resize frames remain native.

- Stars uses a standard Win32 toolbar with split filter buttons, dropdown menus
  anchored to button rectangles, check groups and tooltips. The SDL compatibility
  layer implements the toolbar subset; Windows uses `comctl32`. Scanner commands,
  turn results and save layouts are unchanged.

- The native compatibility headers declare additional standard styles, messages,
  keys and GDI constants with shared MinGW value checks. The hierarchy inspector
  expands styles and control kinds into names while preserving numeric values.

- The SDL entry point accepts `--headless`; headless CTest presets run invisible
  UI workflows sequentially with dummy video/audio and software rendering.

- The SDL client has a built-in window/control hierarchy inspector, opened with
  Ctrl+Shift+F12, with stable logical IDs and JSON snapshots for in-process
  automation. Drawn scanner objects remain separate inspection work.
- SDL edit controls support clipboard operations, undo, line queries and multiline
  vertical navigation. Lists support extended ranges, drag/autoscroll and
  incremental search; child scrollbars repeat held arrow/page requests.

- The mouse wheel zooms the scanner at the cursor, one zoom level a notch;
  Ctrl+wheel (a touchpad pinch) zooms too. The tilt wheel and Shift+wheel
  scroll the scanner sideways.
- The mouse wheel scrolls the report tables by rows, as many a notch as the
  mouse settings say; the tilt wheel and Shift+wheel scroll a column a
  notch.
- Alt+click (Cmd+click on a Mac) with a fleet selected adds a waypoint at
  the fastest useful speed: the speed a fleet with a colonize task gets,
  the fastest up to warp 9 whose fuel fits, slowed to the lowest that
  arrives as soon. Alt held while dropping a dragged waypoint does the
  same. Players set colonize tasks on scouts to get this speed, at the cost
  of a warning every turn. Turn generation is unchanged.
- The selected fleet's path in the scanner has a tick where each year of
  travel ends: a fleet moves its warp squared in light years a year, each
  leg at its own warp, and stops at every waypoint.
- Dragging the scanner with the left button pans it. A press still selects
  what is under it, as a click does; the view moves once the mouse moves
  past the system's drag distance. Dragging a waypoint, Shift and Ctrl
  clicks and the measuring tape (Shift+right-drag) are unchanged.
- The wheel goes to the window under the cursor rather than the window with
  the focus, so the panes and lists scroll where the player points, as with
  Windows 10's "scroll inactive windows" setting. A drop-down list the player
  only points at ignores the wheel, so it can't change an order by accident.

### Changed

- **Host results:** all 178 extended floating intermediates now use binary64
  `double`, with fast-math and contraction disabled. Cybertron AI scenarios
  `oneai5` and `smallai6` change from turn 1; the native baseline moves with
  this change. Save layouts and field widths remain unchanged. Habitability
  can change by one before wizard pricing, while the 140,665 tested races
  retain their point balance. See [DOUBLE-PRECISION.md](docs/DOUBLE-PRECISION.md)
  for coverage, other possible rounding effects and platform limits.

- Race-attribute masks now use an unsigned 32-bit operand, giving bit 31
  defined shift behavior under native C sanitizers. Stored bits, file layouts
  and turn results are unchanged.

- The game's text and static data are now built from source files rather
  than checked-in C tables: the strings, messages, tutorial and planet names
  from `text/*.txt`, and the hulls, parts, starting designs, predefined and
  AI races, default battle plans and the AI's designs, part preferences and
  research orders from `data/**/*.yaml`. Building needs Python 3 and PyYAML.
  The text, data, turns and files are unchanged.

### Fixed

- macOS Retina windows now use backing canvases at the display's physical pixel
  density and rasterize text at that density, instead of enlarging low-resolution
  glyphs. Logical font metrics, advances, control geometry and mouse coordinates
  stay unchanged. Menu text, child composition, scrolling and raster operations
  use the same backing scale; existing game bitmaps retain nearest-neighbor
  rendering. Windows, Linux and non-Retina displays retain scale-one rendering.
  Turn generation, file formats and regression baselines are unchanged.

- Window creation now explicitly hides new native windows after SDL setup.
  Cocoa could expose owned popups despite the requested hidden flag, showing
  black during renderer setup before the wrapper's first paint. macOS menu,
  combobox and tooltip popup windows also start with a clear native backing
  until their completed opaque canvas is submitted. A native test checks the
  scanner-combobox tooltip is actually hidden before rendering and its first
  frame has the opaque tooltip background. Turn generation and regression
  baselines are unchanged.

- Native SDL exposure now immediately resubmits the last completed window
  texture, including exposure during menu/window dispatch and while a new paint
  is pending. Cocoa can clear a newly exposed view to black after its prepared
  frame was presented; previously ordinary exposure waited for the message
  queue to reach painting. Refreshing the completed texture does not reenter
  application drawing or upload unfinished panes. Turn generation and regression
  baselines are unchanged.

- New SDL windows now complete show-time layout and paint their visible controls
  while the native window remains hidden, then submit the finished frame again
  immediately after native show. This avoids waiting for queued exposure to
  replace a black drawable. `Popup` also assigns its handle and font before
  showing; starbase detail previously painted before `hwndPopup` was available,
  producing different first and subsequent frames. Game rules, file formats
  and regression baselines are unchanged.

- The SDL window wrapper now requests native and logical focus when its first
  ordinary window is shown. Terminal launches on macOS previously left the title
  screen inactive. Minimized, disabled and nonactivating windows do not consume
  this startup request. Turn generation and regression baselines are unchanged.

- Mineral and other `starspopup` detail cards now use an owned, nonactivating
  native tooltip window. They previously used an ordinary desktop window on
  macOS; native close left `hwndPopup` stale and blocked subsequent mineral
  popups. Destruction now clears the popup state on every close path.
  Border-only popups no longer acquire a title bar, and nonactivating cards
  retain mouse capture on behalf of their focused owner.
  Turn generation and regression baselines are unchanged.
- The SDL painting wrapper now presents completed pane paints together and
  paints the first frame before showing a native window. Previously icon draws
  and individual pane paints uploaded incomplete frames, exposing black areas
  and slowing startup. A live-resize event watcher also lays out and paints at
  the current size during native resize tracking instead of waiting for the
  outer message loop. Game rules, file formats and regression baselines are unchanged.

- New-game setup now keeps its current page open while choosing the game
  filename and makes that page the save picker's owner. Previously the wizard
  closed first and attached the picker to the hidden main frame. Canceling
  filename selection returns to the same page. Turn generation and regression
  baselines are unchanged.

- The SDL modal dialog wrapper now requests native input focus after showing
  an initialized dialog. Previously it focused only the logical control,
  leaving new-game dialogs inactive on macOS and consuming the first click
  for activation. No-activate windows retain their policy; turn generation and
  regression baselines are unchanged.

- The Makefile's platform selection now closes with Make's `endif` syntax;
  the previous `endif()` prevented every Make target from parsing.

- `NewGameWizard` treated a modal dialog's `-1` result on application quit as
  permission to advance, repeatedly opening pages against the pending quit
  message. It and the nested race wizard now unwind like cancellation, and the
  SDL modal wrapper rejects new dialogs while quit is pending. This fixes Cmd-Q during new-game setup;
  turn generation and regression baselines are unchanged.

- The Cocoa file picker previously failed before opening when SDL's framework
  lacked CP1252 iconv support, including when choosing a new game's name.
  It now converts picker titles and filters with the game's fixed CP1252
  converter. UTF-8 paths, save formats and turn generation are unchanged.

- `InitInstance` previously showed and immediately hid the main frame before
  opening the title screen, exposing a brief window and maximize animation on
  macOS. It now keeps the frame hidden and defers its saved show command until
  the game first displays it. Turn generation and regression baselines are unchanged.

- The SDL wrapper headers now use standard C null and integer-limit definitions
  and an explicit dialog callback signature for their remaining `FARPROC`
  instances, avoiding Apple Clang prototype and macro-redefinition errors.
  macOS SDL links use classic pointer fixups for the original packed UI tables.
  Structure layouts, file formats and turn generation are unchanged.

- `FSaveRace` and race-only `FOpenGame` bound the remembered race basename to
  its existing 16-byte buffer. Native long filenames previously overwrote
  adjacent globals; saving and reopening still use the full selected path.
  Separate UI storage retains the complete repeat-save suggestion, while the
  SDL cache boundary preserves complete UTF-8 characters.

- SDL native file pickers preserve their owner's native focusability while
  disabling game input, and use SDL's exported Wayland parent handle when
  available. Fullscreen title ownership and cancellation have private-display
  tests; the reported desktop disappearance still needs compositor verification.
  Backend failures and overly long selected paths now show an error message;
  cancellation remains quiet.

- Edit undo combines adjacent Delete/Backspace removals in either direction
  and extends an undoable replacement when typing continues at its end,
  matching the shared Windows SDK control checks.

- `GetCursorPos16` initializes its point when a backend cannot report a cursor
  position, preventing an uninitialized coordinate read by legacy UI callers.
  Successful cursor queries and on-disk point widths are unchanged.

- `CompatComboProc` no longer invalidates an open dropdown on read-only item
  queries. Owner-drawn Transport fields read their rows during painting; those
  queries previously caused continuous repainting and starved UI timers.


- The SDL resource generator now embeds the tutorial's three recorded data
  resources. `FWriteTutorialMFile` previously could not load them, preventing
  the native tutorial from generating its next year.

- `PaintScroll` validates pending paint even after a scrollbar is resized to
  zero width or height. Previously it returned before `BeginPaint`, repeatedly
  producing paint messages and starving timers after waypoint task changes.

- `DrawBtn`, `InitBtnTrack`, `FTrackBtn` and `FTrackXfer` copy rectangles from
  packed button records before aligned Win32 access. Split/transfer rendering
  and input previously loaded misaligned `RECT` objects under fatal UBSan.
  Button layouts, hit targets and game/file behavior remain unchanged.

- `UndoEdit` now marks text-changing undo/redo as modified and restores the
  actual replacement range. It sends the SDK's final change notification after
  selecting that range; previously redo collapsed the selection and restored an
  outdated modified flag.


- `UndoEdit` preserves the SDK's empty-history and read-only contracts. Empty
  undo returns success, restores its retained position and sends only a change
  notification; read-only single-line undo returns success without altering
  pending history, while read-only multiline undo returns failure.


- `CompatDialogProc` no longer ends a modal for an unhandled OK/Cancel command.
  Stars' ship designer uses those buttons to leave edit mode while keeping the
  designer open; default handling previously closed the whole window.


- `ScannerWndProc` applies the current zoom when a deferred resize refresh
  runs. Previously resize messages posted captured zoom commands; rapid zoom
  changes and scrollbar visibility changes could replay alternating stale
  values indefinitely, starving dialogs and timers. Loaded-game private-X11
  workflows reproduced and verify the repair. Turn generation is unchanged.

- `CompatDialogProc` preserves an application's `EndDialog` result when its
  callback returns false. Default command handling previously overwrote zero
  cancellation results with `IDCANCEL`, causing production Cancel to commit
  edits. Shared SDK dialog checks and loaded-game SDL workflows cover cancel,
  accept and reopening. Game rules and saved-file layouts are unchanged.

- `CompatEditProc` normalizes initial text and `WM_SETTEXT` for uppercase and
  lowercase edit styles. Previously only inserted text was converted. Both
  single-line and multiline controls retain SDK text-limit, modified/undo and
  notification behavior; read-only controls still accept programmatic text.
  Game results and saved-file layouts are unchanged.

- `CompatListProc` redraws single-selection owner-drawn rows synchronously with
  `ODA_SELECT` when selection changes, instead of only queuing a full repaint.
  Keyboard changes draw before notification, repeated selections draw nothing,
  and owner callbacks may reset or destroy the control safely. Game results
  and saved-file layouts are unchanged.

- SDL edits group consecutive Backspace/Delete operations for undo and restore
  the complete deleted selection, instead of undoing only the final character.
  Cursor changes and focus changes end the group. Game results are unchanged.
- SDL edits no longer add a spurious undo record when text-limit rejection
  inserts nothing. Existing undo history survives; limit/update/change
  notifications and the modified flag retain their SDK behavior.

- The SDL list-box runtime sends owner-draw focus actions before focus
  notifications, including the empty-list focus indicator used by production
  and fleet panels. Previously it only queued a full repaint and omitted empty
  owner-drawn lists. Focus drawing tolerates callbacks resetting or destroying
  the control. Game results and saved-file layouts are unchanged.

- `FFileDialog` keeps the selected folder, filename and filter when native file
  validation fails or overwrite is declined, instead of reopening at the initial
  location. A selected directory becomes the retry location. Game state and
  saved-file formats are unchanged.

- The SDL compositor fills the corner shared by horizontal and vertical
  scrollbars with Windows 95 button-face gray instead of leaving it black.

- `ScrollWindow` moves overlapping RGBA window pixels by rows instead of the
  generic per-pixel raster-operation path. Captured scrollbar dragging presents
  the completed content and thumb together once per update. `SetScrollPos`
  invalidates only the changed standard bar, preserving the scanner's incremental
  scrolling path instead of forcing a full map redraw.

- `CompatScrollProc` repaints the scrollbar thumb during captured mouse movement,
  instead of waiting for the mouse queue to empty. Unchanged nonclient bar layout
  no longer repeats resize/show work. Consecutive native motion messages retain
  the newest position to avoid drag lag behind expensive map redraws.

- SDL combo arrow buttons retain their full raised bevel inside the sunken
  field border, with a depressed button and offset triangle while open.

- SDL mnemonic underlines leave a pixel below the text baseline instead of
  touching the letter, matching ordinary font underline placement.

- SDL split toolbar menu activation presses only the arrow area and shifts its
  triangle, preserving the action button's appearance and checked state.

- SDL split toolbar buttons use adjacent full-height classic bevels for the
  action and menu-arrow areas, with a compact dropdown triangle.

- SDL combo dropdowns keep row highlights separate from the selected value.
  Hovering no longer changes game settings; clicking a row or pressing Enter
  confirms it, while Escape and outside clicks cancel without selection changes.

- SDL combo lists use owned compositor popups and paint their rows before
  opening, fixing dropdowns appearing empty at the edge of the main window.

- SDL radio buttons paint a round selected dot instead of a square.
- `ExecuteButton` anchors toolbar filter, minefield and zoom menus below the
  triggering button instead of at the mouse position. Turn results are unchanged.
- `TooltipWndProc` anchors tips beside their control rectangles, keeping them
  inside the frame. The SDL window adapter uses an owned, nonactivating native
  tooltip role and converts moves to owner-relative coordinates.

- The SDL window boundary converts CP1252 titles to UTF-8 on creation, updates
  and wizard reuse, preserving narrow game text and avoiding X11 conversion
  failures/leaks on non-ASCII titles.

- `LpAlloc` and `LpReAlloc` align ordinary heap payloads for native pointers
  and other C types, including reused and grown blocks. Battle allocation
  boundaries and saved-file layouts remain unchanged.
- `SetVisPFInit` skips the player-race lookup for the host's all-player
  sentinel, avoiding an out-of-bounds read of player -1. Visibility rules
  remain unchanged.
- `PtToScan` multiplies signed coordinates instead of left-shifting negative
  values. Scanner rounding and turn results remain unchanged.
- `WriteFleet` writes compressed 16-bit values through byte-safe stores;
  variable-length records can place them at odd offsets. Serialized bytes
  remain unchanged.
- Silent SDL process cleanup now frees list and combo storage even when
  application destroy callbacks are skipped after game-state teardown.
- `FLookupPart` skips player race/technology lookups for the host sentinel;
  host part access has no player restrictions. `WriteRt` skips the memory
  copy for empty records, whose caller may supply a null payload.
- `DoCyberPackets` and `FEnumPktAttack` use byte-safe accesses to the
  four-byte mineral limit in the two-byte-aligned AI workspace.
- Packet movement skips the receiving-race lookup for an unowned planet,
  whose nonexistent player was previously indexed as -1.

- `TitleWndProc` paints the splash image and buttons during creation, before
  the native window appears. Startup cleanup no longer leaves a blank title
  screen while the first queued paint waits.

- `ReadIniTileSettings` normalizes `isupper` to a boolean before storing the
  one-bit expansion flag. Linux builds now restore expanded planet/fleet
  sections from saved settings instead of silently collapsing them.

- The SDL wrapper's `CreateWindowExA` now sends the initial top-level client
  size notification. Stars can lay out its toolbar, scanner and other panes
  immediately after opening a game, without requiring a manual resize.

- `BrowserDlg` now destroys its modeless Technology Browser on Close or the
  window's X instead of using `EndDialog`, which left the SDL browser open.
  Turn generation and saved files are unchanged.

- `FOpenGame` uses the platform directory separator and checks for a missing
  separator when continuing a saved game. Linux Continue Game and relative
  filenames no longer dereference a null pointer. Saved files and turns are unchanged.

- The SDL client's `CreateWindowExA` now chooses both default dimensions
  when the width is `CW_USEDEFAULT`, so a new main window has usable height.
  Menu dropdowns use SDL's owned popup role so the desktop compositor does
  not cancel them by deactivating their owner during creation.

- The SDL race wizard retains one desktop window across Back/Next page
  transitions, avoiding repeated close/open animations. Each page still
  creates and destroys its own logical dialog and controls; Windows dialog
  behavior and turn generation are unchanged.

- `DrawRace2` no longer copies the habitat explanation into a 32-byte stack
  buffer. The 67-character template corrupted the stack while painting the
  race wizard’s habitat page and crashed the native Linux client. Displayed
  text, game rules and turn generation are unchanged.

- The scanner no longer leaves a band drawn out of place when it scrolls
  while a redraw is still pending (`ScrollScanner`), as after a wheel zoom
  followed at once by a trackpad scroll. The original drew the pending
  area at the new position and then scrolled it with the rest.

## 2.9.0 (2026-10-05)

### Added

- `stars-host`, the game's host without windows, for Windows, Linux and
  macOS. It takes `stars.exe`'s host command line (`-a`, `-g`, `-b`, `-t`,
  `-v`, `-s`, `-p`, `-l`) and generates the same turns where `long double`
  is x87 extended precision (x86-64; on Apple silicon, an x86_64 build run
  by Rosetta). Messages go to stderr; it never waits for a player.
  `stars-host --version` prints its version, which names its platform
  (`2.9 linux-x64`, `2.9 macos-x64`); the Windows build keeps `2.9x64`.
  It writes the universe, planet and fleet dumps (`-dm`, `-dp`, `-df`) as
  `stars.exe` does, and `--ini <file>` reads the `stars.ini` settings a
  host uses (`[Files] Logging`; `[Misc] DefaultPassword`, `NewReports`,
  `NoHostNames`, `Backups`).

### Fixed

- Creating a universe from a definition file no longer writes the race
  file's path past the end of a 16-byte name buffer (`FWasRaceFile`). Only
  the file name is kept, as the race dialogs keep it. Results are unchanged;
  macOS stopped the program at the overrun.

## 2.8.1

### Changed

- Removed serial number requirement and various cheater checks during turn
  generation. Still writing empty serial bytes in place of old rgbConfig/lSerial
  in rtlog records for backwards compatibility with tools that load it.

### Fixed

- Patch releases now display `Version 2.8.1x64` instead of
  `Version 2.8x64 (2.8.1)` (`SzVersion`, `cmake/version.cmake`). Initial
  releases still omit `.0`, and development builds retain their full version.

## 2.8.0 (2026-10-04)

### Changed

- Files written by 2.8 (games, turns, orders, histories and races) carry
  file format version 2.84 instead of 2.83, so Stars! 2.6j and 2.7 refuse
  them. 2.8 still loads 2.6j and 2.7 files, so a 2.8 host can take over a
  2.6j game. The records are unchanged (`WriteBOF`). Opening a file from a
  later version now says it is newer; the original called 2.84 files older
  (`FOpenFile`).

### Added

- Product version from git tags and build numbers, stored in a
  `VERSIONINFO` resource. It is shown as `Version 2.8x64` (or
  `2.8x64 (2.8.1-dev.N+gSHA)` between releases) in the About box, splash
  screen and report headers, marking the 64-bit Win32 build.
  The save-file format version is separate (see Changed). See
  [docs/VERSIONING.md](docs/VERSIONING.md).

### Fixed

- Macinti AI: deciding whether to build mine layers no longer reads past
  the start of its design list when it has no miner design; a missing design
  counts as zero ships (`DoMacintiAiTurn`). **Host results.**
- Robotoid and Macinti AI: deciding whether an attack fleet joins up with
  others no longer reads a fleet's position as a ship count when the AI has
  no destroyer design (`DoRobotoidAiTurn`, `DoMacintiAiTurn`). **Host
  results.**
- Cybertron AI: mine-laying fleets sent to a random planet now lay mines
  there indefinitely, like the AI's other mine-laying orders. The original
  left the order's countdown as uninitialized stack bytes, so whether the
  fleet laid at all depended on the save path and, after turn 80, it
  usually stopped on arrival (`DoCyberAiTurn`). **Host results.**
- Macinti AI: the same fix for its mine-laying fleets, whose order's
  countdown was uninitialized stack bytes (`DoMacintiAiTurn`). **Host
  results.**
- Macinti AI: an armada too weak to count as a war fleet is now judged by
  its actual strength when choosing whether to attack, retreat or wait. The
  original compared an uninitialized value (`FPotentMacWarFleet`,
  `TargetMacArmada`). **Host results.**
- Message text: a quantity formatted without a preceding mineral no longer
  reads a unit string from before the unit table; it gets no unit
  (`PszFormatString`). Display only.
- Minefields: fleets travelling due north or south now hit minefields along
  their route. The route/field intersection measured a vertical route from
  its start point instead of its closest point to the field
  (`FIntersectCircleLine`). **Host results.**
- Cybertron AI: no longer scraps the ships of its newest starbase-defender
  design once that design passes the recycle age. After checking its
  defender designs it protected the newest destroyer design again instead
  (`DoCyberAiTurn`). **Host results.**
- Ship design: with Regenerating Shields, a large armor slot (such as 24
  Superlatanium on a Space Dock) no longer overflows into a huge armor
  value; it gets half its armor like any other (`UpdateShdefCost`).
  **Host results.**
- Scanning: scanners and stargates with ranges above about 463 ly now see
  cloaked starbases (such as ISB races') within their reduced range. The
  range-times-visibility product overflowed 32 bits, so lightly cloaked
  starbases were often only obscured (`SetVisPFFleets`, `SetVisPFPlanets`,
  `SetVisPFThings`). **Host results.**
- Colonizing: a ship needs an installed colonization or orbital
  construction module. A design slot that once held a module but now holds
  none no longer counts (`SatisfyOrders`). **Host results.**
- Native build: the Battle Plans drop-downs (targets, attack who, tactic)
  saved 0 (None/Disengage, Nobody) whatever was chosen, and the production
  dialog's Required Minerals panel ignored the selected item. Both sent the
  Win16 message numbers for `CB_GETCURSEL`/`LB_GETCURSEL`, which Win32
  controls ignore (`BattlePlansDlg`, `DrawProductionDlg`). Now as in the
  original.
- Native build: the planet and fleet dumps no longer crash. `PszFromLong`
  tested the value behind its optional count pointer instead of the
  pointer, which the dumps pass as NULL; Win16 silently read the start of
  its data segment.
- Native build: the planet dump's Factories column printed 0 and shifted
  the factory count into Def %. The mine and factory counts were passed as
  two 16-bit words each, as the Win16 binary pushed them (`DumpPlanets`).
- File copies (`StarsCopyFile`, used for turn-file backups) close the copy
  when done. The original closed the wrong handle, leaving each backup
  open with exclusive sharing for the rest of the session, and a copy whose
  destination couldn't be created left the error-recovery state pointing at
  a finished call.
- Turn generation keeps a backup of the turn file of a player who submitted
  no turn, as the original did. The reconstruction had the copy's source
  and destination reversed (`FGenerateTurn`).
- Claim Adjuster turn files carry only the other players' habitat. To give
  a CA player their habitat, the original sent every visible player's full
  record, including tech levels, research spending and target, racial
  traits, production template and diplomatic relations (`FWriteDataFile`).
  **Host results** for AI Claim Adjusters, which plan from their own turn
  file.
- Stealing: a waypoint transport order can no longer load colonists or fuel
  from a planet or fleet it may only rob with a Robber Baron or Pick Pocket
  scanner; the player gets the "attempt was unsuccessful" message instead.
  Minerals can still be stolen (`SatisfyOrders`). **Host results.**
- Race files: shortening a race name no longer saves a race file that Stars!
  rejects as corrupt. The checksum covered leftover characters of the
  longer name in memory, which aren't saved (`IRaceChecksum`). Race files
  already saved with a bad checksum still have to be recreated.
- Pursuit: when a pursued fleet splits or disappears, every pursuer
  retargets the heaviest matching fleet where it was last seen, as in 2.6j
  RC4. RC3 (the reconstructed release) spread pursuers over fleets nobody
  else was chasing (`ValidateWaypoints`). **Host results.**
- `FCheckFile`'s AI-player check reads the requested player's record. The
  original stopped at the first player record, or at any earlier record
  whose first byte matched the player number, and read its AI bit. No
  caller uses this check yet, so nothing changes in play.
- Loading a corrupt player record no longer writes past the player's data.
  The original trusted the record's relation count and name lengths, so a
  count above 16 or a name of 32 or more characters overran the relations
  and names (`ReadRtPlr`). Valid files load as before.
- Native build: a fleet's "has completed its assigned orders" message is
  removed again when the fleet is used up colonizing, scrapping or meeting
  the Mystery Trader, or is replaced when it completes new orders, as in
  the original. The stored goto was compared as unsigned and the fleet's
  goto, which has its high bit set, as signed, so they never matched
  (`FRemovePlayerMessage`, `FFindPlayerMessage`).
- Healing: ships that gated, fought or hit mines no longer heal by merging
  into a fleet that didn't; the fleet they join doesn't heal that turn
  either (`Merge2Fleets`). **Host results.**
- Native build: a detonating minefield no longer crashes turn generation
  when a fleet is inside it. `FTravelThroughMineFields` read the travel
  distance through the NULL pointer `ThingDecay` passes for a detonation,
  which Win16 read harmlessly from the start of its data segment.
- Minefields: a fleet immune to a detonating minefield (its owner's mine
  layers in their own field) can still be hit by another player's
  detonating field that turn. The immune check used up the fleet's one
  detonation check (`ThingDecay`, `FTravelThroughMineFields`). **Host
  results.**
- Fleets: merging can no longer pass 32767 ships of one design in a
  fleet. A waypoint or AI merge wrapped the count negative; now the ships
  that don't fit stay in their own fleet (`Merge2Fleets`). Merging all
  fleets at a location clamped the count to 32766 and lost the rest; now a
  fleet that doesn't fit is left out (`FFleetMergeAll`). **Host results.**
- Pursuit: a fleet chasing another no longer stops when a third fleet
  chasing it catches up. The pursuer marked the caught fleet as done
  moving; now it keeps following with the rest of its move (`MoveFleets`).
  **Host results.**
- Cargo: minerals, fuel or colonists sent to another player's fleet that
  don't fit in its hold stay with the sender, as the "unable to transfer"
  message says. The original took them from the sender and destroyed them
  (`FRunLogRecord`). **Host results.**
- Turn files no longer tell a player which other players have seen a
  minefield, a wormhole or the Mystery Trader, who has travelled through a
  wormhole, or which part the trader carries. Each player's file keeps only
  that player's bit of these masks (`FWriteDataFile`). Turn files only; AI
  play is unchanged.
- Production: changing or deleting a ship or starbase design clears the
  progress of queued items of that design. The original kept the
  completion percentage and charged only the rest at the new design's cost,
  so a mostly built cheap starbase could be finished as an expensive one
  (`FRunLogRecord`). **Host results.**
- Scores: a planet's population and resources in the published scores are
  taken as it produced and grew that year. Loading colonists onto ships at
  waypoint 1, after production, no longer lowers them (`Produce`,
  `CalcPlayerScore`). Scores only; play is unchanged.
- Popups: right-clicking a location on the map, and choosing a waypoint's
  target, list every fleet and object there. The original stopped at 100
  entries, so objects past them couldn't be selected or targeted
  (`ScannerWndProc`, `ClickInShipOrders`, `PopupMenu`). Client only.
- Race wizard: a race that was Random and is then customized is no longer
  replaced by a random race when the game is created. Choosing Custom kept
  the random-race setting (`RaceWizardDlg1`).
