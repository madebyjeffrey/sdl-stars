# Open/save file dialogs

Windows SDL defaults to native Unicode open/save dialogs through an isolated SDK
bridge. It preserves wildcard filters, converts CP1252 labels and UTF-8 paths,
pumps wrapper messages during native modality, and leaves final validation and
overwrite confirmation to the shared wrapper. Browsing/cancellation preserve CWD.
`STARS_FILE_DIALOG_BACKEND=compat` explicitly selects the constructed picker for
headless tests. SDK bridge tests cover open/save dispatch, labels, wildcard and
filter-index round trips, Unicode paths, cancellation/error and short buffers.
Interactive Windows desktop qualification remains pending.
See [Windows builds](WINDOWS-SDL3.md).

`GetOpenFileNameA` and `GetSaveFileNameA` now provide real modal file pickers.
Current API body coverage is **178/178 (100%)**, with six CRT helpers counted
separately. This is basic API coverage, not full Win32 semantics or release
readiness. The client links and native desktop selection is the default.

## Planned dialog overhaul

The user identified open/save usability as a dedicated remaining milestone.
Review and improve layout, directory navigation, file and filter presentation,
filename editing, keyboard interaction and validation/overwrite feedback.
Qualify non-ASCII filenames and scaling, with tests of actual game/race open
and save workflows. Preserve the existing Stars wildcard filters, cancellation,
buffer sizing, extension and directory contracts.

The native backend supersedes custom-browser redesign for desktop use. Linux
open/save/cancel has user confirmation; tests cover portal protocol, retries,
errors, UTF-8 paths and actual game/race file workflows. Sequential private-display
checks cover fullscreen native-modal ownership. Desktop compositor confirmation,
native Wayland qualification and macOS execution remain outstanding; see
[current product acceptance scope](LINUX-CLIENT-GOAL.md).

## Browser and filename contracts

The picker uses the existing dialog, edit, list, combo and button controls. Stars
uses filters such as `*.R*` and `*.M*;*.HST;*.R*`. SDL's
[file-dialog filters](https://wiki.libsdl.org/SDL3/SDL_DialogFileFilter) accept
extension lists rather than these wildcard patterns, so directly substituting an
SDL native picker would change the file list. The local browser retains ordered
filter descriptions, case-insensitive wildcard matching and semicolon-separated
patterns. `*.*` also includes extensionless files.

Directories are listed before files and remain available regardless of the active
filter. Double-click enters a directory or chooses a file; Up navigates to the
parent. Typed absolute and relative paths work. Initial-directory and suggested
filename inputs are used, including suggestions containing a parent path. An
invalid initial directory falls back to the current directory. Return activates
Open/Save, Escape cancels, and normal Tab navigation uses the dialog machinery.

Successful selection returns an absolute Linux path and fills the byte offsets
`nFileOffset` and `nFileExtension`, the basename title and the one-based filter
index. Missing extensions receive up to the first three characters of
`lpstrDefExt`; a typed extension is retained. `OFN_EXTENSIONDIFFERENT` reports a
difference from the requested default. The filename buffer is never truncated.
An insufficient buffer returns `FALSE`, sets `FNERR_BUFFERTOOSMALL`, and writes
the required size to its first WORD when two bytes are available.

`CommDlgExtendedError` supplies per-thread dialog errors. Cancellation returns
`FALSE` with a zero dialog error and leaves the filename unchanged. Invalid
structure sizes, invalid input buffers, unsupported options, allocation failures
and initialization failures are distinguished from cancellation.

These contracts are based on Microsoft's
[OPENFILENAMEA documentation](https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea).
The shared Windows SDK test checks selection, cancellation, default extension,
returned offsets/filter index and insufficient-buffer behavior. Wine uses a nested
filename combo; the test driver locates its actual edit child. No Wine source is
copied into the runtime.

## Validation, modality and ownership

The browser checks the parent directory and rejects non-regular selected files.
`OFN_FILEMUSTEXIST` rejects missing input files, and existing open selections are
checked for read access. Save selections with `OFN_NOREADONLYRETURN` reject a
read-only file or unwritable destination directory. `OFN_OVERWRITEPROMPT` asks
before selecting an existing save destination; refusal keeps the picker open.
Choosing a save path does not create or truncate the file. The application's
subsequent file operation remains responsible for I/O errors and filesystem races.

The owner root is temporarily disabled while other windows remain available.
Its prior enabled state and surviving focus are restored afterward. Nested error
and overwrite prompts use the message-box runtime. Timers continue to dispatch.
Destroying the owner ends its picker, and a pending quit is reposted to the outer
loop. Owned filter strings, directory entries, controls and fonts are released.

On success, the selected parent becomes the current directory unless
`OFN_NOCHANGEDIR` is set. This matters because Stars opens a race file using the
returned basename. Browsing and cancellation do not change the working directory.

## Limits and remaining work

The implementation supports the flags used by Stars, plus `OFN_NOCHANGEDIR` and
single-file `OFN_EXPLORER` output semantics. It uses its own consistent layout
for both legacy and Explorer calls. Multi-selection, hooks, custom templates,
custom filter buffers, help buttons, network-share notifications, shell shortcut
resolution and a Read Only checkbox are not implemented. Unsupported flag paths
return failure. The parent directory must exist even without `OFN_PATHMUSTEXIST`.

Paths use native Linux separators and filesystem bytes; this is not Windows
drive-letter translation. Typed filename lookup is Linux case-sensitive even
though filtering is case-insensitive. Non-ASCII filesystem names, exact ANSI/UTF-8
conversion, Windows-specific trailing-dot/space normalization, accessibility and
full DPI/multi-monitor layout remain unqualified. The UI requires the main thread.

Printing, help, process services and the shared `WinMain` entry point are now
implemented, and the SDL game client links. Their remaining product qualification
is tracked separately from file dialogs in the roadmap.

## Build and verification

No extra libraries or system packages are required. Select
`stars-compat-filedialog` under `sdl-debug` or `sdl-release` in CLion. Its SDK
counterpart is `test_win32_filedialog_reference`.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R '^win32_filedialog' --output-on-failure
```

Native tests exercise the actual modal controls, not a fabricated picker result.
They check Stars filters, filter switching, directory double-click navigation,
overwrite refusal followed by confirmation, read-only rejection, missing-file
recovery, filename fields, buffer sizing, current-directory policy, quit and owner
destruction. The full SDL Debug and Release suites passed **52/52** tests each.
The SDK file-picker contract test passed under Wine. The final address,
undefined-behavior and leak-sanitized native harness passed. A relocatable client
link probe confirms that open/save calls resolve; the remaining Win32 service
references are `PrintDlgA`, `Escape`, `WinHelpA` and legacy `ExitWindowsEx`.
Final batch validation:

- MinGW Debug built with no new warnings in the touched code.
- Windows units passed **49/49**; the strengthened SDK Return-key test also passed.
- Linux host built and its units passed **12/12**.
- Native regression matched **519 checkpoints**, with 22 existing warnings and
  zero differences/errors or skipped inputs.
- Tutorial validation failed at year 2407, page 23, instruction 179, after 90
  actions. Stars exited during a Shift-click gesture. The Wine log reports
  `dlls/win32u/sysparams.c:439: user_check_not_lock: Assertion '0' failed`.
  Artifacts remain in `dist/scaffold/tutorial/20261011T014358.831986Z`. This differs
  from the previous milestone's heap failures. The cause of these Wine failures
  remains unresolved; no successful tutorial run is claimed for this batch.

The Windows tutorial executable does not link the SDL compatibility runtime.
Game code, serialized layouts and regression baselines remain unchanged.

## Dialog overhaul: first batch

Pickers now resize with a minimum tracking size, with anchored fields/actions
and an expanding file list. The location is editable; Enter while it has focus
navigates without choosing a file. Back retains up to 32 visited locations; Up
and Home navigate normally. Dot-prefixed entries are hidden by default and a
checkbox reveals them. New folder opens a cancellable name dialog, creates only
a single named directory and enters it on success. Creation failures are reported
without selecting a file. File-name/type mnemonics are available.

Native interaction tests cover resizing, location Enter, Home/Back/Up history,
hidden entries, new-folder cancellation/creation and subsequent file selection.
Unicode filesystem conversion, richer presentation and desktop/scaling review
remain part of the milestone; no completion claim is made yet.

First-batch qualification: Debug/Release/sanitizer and MinGW Debug builds pass
without warnings in changed files. All 96 fatal ASan/UBSan/LSan tests and 51
Windows units pass; focused Release picker checks pass 2/2. A final Debug picker
test and offscreen rendering also pass visual review. Full native regression
returns 519 matches (87 unused-storage warnings), zero differences/errors and
zero skipped inputs. UI suites ran sequentially on private displays/headless.
No game code, persisted layouts or baselines changed. Native tutorial work is
queued at milestone 4 of the current goal.

## Platform-native file selection

This supersedes the custom-browser overhaul for ordinary desktop use. CMake's
`STARS_NATIVE_FILE_DIALOGS` defaults to ON. `STARS_FILE_DIALOG_PORTAL` selects
Linux's libdbus adapter; `STARS_FILE_DIALOG_COCOA` selects the macOS AppKit bridge.
The existing Windows build continues calling system common dialogs. Turning the
CMake option OFF retains the compatibility browser. `STARS_FILE_DIALOG_BACKEND=compat`
explicitly selects that browser; `--headless` and the headless presets set it so
file selection cannot open an external desktop window. Private portal tests
explicitly override it within their isolated child process.

The Linux adapter requests the desktop portal's OpenFile/SaveFile operations.
It supplies case-expanded ASCII globs, splits semicolon patterns and maps `*.*`
to `*`, preserving the Stars filter behavior. Labels cross CP1252 to UTF-8;
selected filesystem paths remain bytes, including UTF-8 names. Returned local
file URIs are decoded once. Remote authorities, malformed escapes, embedded NUL,
missing/multiple results and wrong value types fail explicitly.

The request is subscribed before dispatch; early responses are retained until
the method reply identifies the request and service sender. Timers and game
messages keep dispatching while the owner is disabled. Quit/owner destruction
close the request and restore surviving state. Shared validation still handles
extensions, buffer sizes, access, overwrite and directory policy. Missing portal
services return a dialog error rather than silently switching UI.

After a validation error or declined overwrite, native selection reopens in the
last selected folder with the filename and filter retained. Selecting a directory
reopens inside it. Private-bus tests verify missing-file recovery, directory
recovery and overwrite refusal without modifying the existing file.

X11 supplies the SDL window XID as parent. Wayland currently supplies the allowed
empty parent identifier; xdg-foreign export and real compositor stacking remain
part of milestone 7. The native macOS adapter uses AppKit open/save panels with
filter accessory controls, directory creation and sheet ownership. Its labels
use the shared CP1252 converter because SDL frameworks may omit CP1252 iconv
support; filenames remain UTF-8. The Apple silicon product builds, and an adapter
test covers labels, paths and cancellation. Full native-panel interaction
qualification remains outstanding.

New-game setup chooses a filename before ending its current wizard page. That
page owns the save picker and stays disabled while selection runs; cancellation
returns to the same page. The hidden main frame is not its owner.
Simple/advanced ownership and cancellation pass the compatibility tests; an
opt-in Mac desktop check also verifies the native sheet attaches to the simple
new-game page and cancels back to it.

The private D-Bus fixture checks actual request types/options, Unicode paths,
case-expanded filters, filter results, default extensions, cancellation, small
buffers, invalid responses, early responses and request cleanup on quit/destroy.
It never connects to the user's bus or opens desktop dialogs.

Backend method errors (including an unavailable FileChooser method) and failure
response code 2 are verified to return `CDERR_INITIALIZATION`, leave caller
filename output unchanged, and restore the disabled owner. Cancellation remains
a false return with no dialog error.

Scripted picker and PDF-output tests explicitly select the compatibility backend;
they do not depend on a desktop portal or allow a native dialog to wait for input.

Protocol references: [FileChooser](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html),
[Request lifecycle](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html),
[window identifiers](https://flatpak.github.io/xdg-desktop-portal/docs/window-identifiers.html).

The packaging milestone now targets Flatpak plus CMake installation. It must
verify access to related `Game.*` files and saving sidecars inside the sandbox,
not just selection of one file. Desktop portal interaction and the broader game/
race open/save workflows still need qualification before milestone 1 is complete.
