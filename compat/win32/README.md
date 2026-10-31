# SDL Win32 source compatibility headers

The same wrapper now builds on Linux, macOS and Windows x64. Native Windows
setup and portable packaging are documented in [Windows SDL3](../../docs/WINDOWS-SDL3.md).

Milestone 2 supplies declarations for the Win32 UI surface recorded in
`docs/WIN32-APIS.md`. The subsequent Free API adaptation adds the local C++20
`stars_win32` runtime with rectangle operations, tick counts and the initial
SDL-backed window/message subset. See [window milestone](../../docs/WIN32-WINDOWS.md)
for the `stars-compat-windows` interactive/self-test target and supported APIs.
See [Free API adoption](../../docs/FREE-API-ADOPTION.md) for the pinned source,
coverage comparison and remaining work. The shared game UI now links into
the native `stars-sdl` preview; see [client status](../../docs/WIN32-CLIENT.md). `direct.h` and `io.h` declare the
Windows CRT helpers used by the UI; all six inventoried helpers now have local implementations.

The native client also has an in-process [window inspector](../../docs/WIN32-INSPECTION.md),
opened with Ctrl+Shift+F12. Its API in `stars_compat.h` supplies snapshots, stable
logical IDs, ID resolution and unique child lookup. It is separate from the SDK
API inventory; drawn map objects need a semantic adapter.

The local headers also declare 934 additional [SDK-checked constants](../../docs/WIN32-CONSTANTS.md).
The inspector expands window/control styles and control kinds into symbolic
names; numeric values and unknown bits remain available.

`stars_win32_headers` is an interface target. Only native SDL compatibility
consumers inherit its include directory and `STARS_UI_SDL` definition.
SDK-backed Windows targets continue to use the actual SDK; Windows SDL targets
use these headers with generated `StarsCompat_` symbol aliases. The host never
includes these headers. Do not add this directory to global include paths or define
`_WIN32` for a native build.

Application UI code uses `wincompat_calls.h` for operations that can dispatch
callbacks: for example, `LUiSendMessage` and `FUiDestroyWindow` call the raw APIs
and deliver captured exceptions after dispatch/modal cleanup returns. Runtime
internals and independent raw API contracts keep the raw calls. The 23 checked
entry functions have compile-time signature checks against their underlying
APIs; application call sites name them explicitly.

Profile saving accepts the private `PROFILEIO` provider in `src/profile_io.h`.
Defaults call the native file service; scoped overrides supply only I/O effects
and restore through nesting and exceptions. The linked save-stage witness and
file/profile guard tests use the product implementation, preserving serializer
and cleanup order without compiling a private wrapper clone for those guards.

The supported source interface is 64-bit, narrow-character C++20, with Win32 LLP64
widths: `LONG`, `DWORD` and `BOOL` are 32 bits; handles and message parameters
are pointer-sized. Structures use SDK packing, including packed bitmap file
headers. Callback functions use the C calling convention on every platform. This is
source compatibility, not the binary ABI needed to load Windows DLLs. Unicode
and 32-bit native builds are not validated.

`wincompat_structs.h`, `wincompat_constants.h` and `wincompat_api.h` were
adapted from the installed mingw-w64 SDK headers (Debian mingw-w64 11.0.1).
The subset is selected from the UI inventory, with the supporting types,
macro dependencies and legacy aliases needed by existing UI files. Long
constants and scalar types are adjusted for Linux LP64. See `LICENSE` for
the original notice and redistribution terms. New APIs should keep the
SDK signature and receive appropriate ABI assertions.

Build and test:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

On macOS, build with `macos-sdl-debug` and run the native allocator checks with:

```sh
ctest --test-dir dist/macos-sdl-debug -L leaks --output-on-failure
```

These checks use `/usr/bin/leaks --quiet --noContent --atExit` on native test
executables and fail on leaks. They run separately from ASan/UBSan, whose allocator
differs from the native allocator. Linux sanitizer CI enables leak detection too.
The [milestone B ledger](../../docs/CPP-B-IMPLEMENTATION.md) records measured
coverage, including separate desktop and host turn-generation runs.

The preset builds `stars-compat-runtime` for runtime tests and `stars-ui-compat-check`, an object library that compiles
all existing UI sources and generated tutorial text against these headers,
without linking unresolved functions. Missing declarations and incompatible
pointer types are errors. In CLion, select the SDL Debug profile and build
this target; select `stars-compat-abi` to debug the assertions and runtime
pointer tests. `stars-compat-smoke` remains available for the two-window test.

`tests/compat/win32_abi.cpp` is shared with the Windows target
`test_win32_abi_reference`, which uses real MinGW headers. It checks sizes,
field offsets, constants and signatures at compile time, then exercises
macros and high-bit callback parameters without calling OS APIs. A Linux-only
second translation unit checks core-first inclusion and restoration of the
caller's packing. Run the reference and shared geometry tests with `make test-unit` or:

```sh
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --test-dir dist/mingw-debug -L compat --output-on-failure
```

No additional libraries are needed beyond the SDL milestone's development
packages. Normal native builds do not require the MinGW SDK; it is used only
for the Windows build and independent ABI reference.

## Verification on October 10, 2026

See the current [font/text verification report](../../docs/WIN32-FONTS.md#verification).
Debug/Release compatibility, Windows reference/unit, host unit, and full
Windows regression checks pass. The latest tutorial run stopped on a year-2408 ship-transfer
action; it is not recorded as passing. Game baselines remain unchanged.

## Basic painting

`stars-compat-paint` now exercises paint/DC lifecycle, basic solid pens and
brushes, geometry and proportional text in two independent windows. Run it
without arguments in CLion's SDL Debug profile, or use `--self-test`.
See [painting scope and checks](../../docs/WIN32-PAINTING.md). No additional
dependencies are required. `stars-compat-gdi-reference` shares its source
with a real SDK/Wine test of the supported drawing contracts.

## Memory DCs and bitmaps

Compatible memory DCs/bitmaps, bitmap selection and metadata, `BitBlt` and
`PatBlt` now work through the local C++20 runtime. `stars-compat-bitmap` shares
ownership and pixel tests with Windows; `stars-compat-paint` now draws into
memory bitmaps and blits to independent windows. See
[scope, CLion use and verification](../../docs/WIN32-BITMAPS.md).

## DIBs and resource bitmaps

All 38 RC bitmap resources are embedded at build time. The local runtime now
supports uncompressed indexed/RGB DIB input, stretched drawing and RGB/mono
readback. `stars-compat-dib` shares resource/pixel contracts with a real SDK
reference; the painting demo draws the game's indexed icon atlas. See
[formats, limitations and verification](../../docs/WIN32-DIBS.md).

## Memory and timers

Fixed/movable global and local heap handles, thread-local last errors, and
window/thread timers now have shared native and Windows SDK tests. Timer
callbacks run on the UI thread through message dispatch. See the
[scope and verification report](../../docs/WIN32-MEMORY-TIMERS.md).

## UI services

Narrow string formatting, 32-bit `MulDiv`, thread IDs and the inventoried CRT
helpers now have shared native/Windows tests in `stars-compat-services`. See
[contracts and limits](../../docs/WIN32-SERVICES.md). The UI-service milestone reached
83/178 Win32 APIs (47%), plus six CRT helpers counted separately.

## Files and settings

Owned legacy file handles and INI/profile services now pass native/Windows
SDK comparisons. Bare settings names use the user configuration directory;
explicit POSIX paths are retained. The new CLion targets are
`stars-compat-file` and `stars-compat-profile`. See the
[scope and verification report](../../docs/WIN32-FILES-PROFILES.md). The file/profile batch reached
92/178 Win32 APIs (52%).

## Fonts and text

Embedded Liberation faces, FreeType rasterization, UTF-8 text, rotation and
wrapped/clipped layout now pass native/Windows contract tests. Select
`stars-compat-font` in CLion. The SDL presets require FreeType and Fontconfig
development packages; Windows/host builds do not. See the
[scope and verification report](../../docs/WIN32-FONTS.md). The font batch reached
97/178 Win32 APIs (54.5%).

## Clipping regions

Copied rectangle regions, intersections and excluded holes now apply across
fills, lines, text and blits. Select `stars-compat-regions` in CLion; its contracts
also run against the Windows SDK. See the [region report](../../docs/WIN32-REGIONS.md).
The region batch reached **101/178 Win32 APIs (56.7%)**.

## Drawing state and pattern brushes

Logical origins, all 16 ROP2 modes, copied color/monochrome pattern brushes,
brush origins, SetPixel and FrameRect pass shared native/SDK tests. Select
`stars-compat-drawing-state` in CLion; see the [drawing-state report](../../docs/WIN32-DRAWING-STATE.md).
The drawing-state batch reached **108/178 Win32 APIs (60.7%)**.

## Ellipses and cosmetic pen styles

Solid/null-pen ellipses and cosmetic dash/dot lines and rectangles now pass
shared SDK/native tests. Select `stars-compat-geometry` in CLion; see the
[geometry report](../../docs/WIN32-GEOMETRY.md) for the saved SDK reference,
narrow-fill differences and the MIT algorithm license.
The geometry batch reached **109/178 Win32 APIs (61.2%)**.

## Child windows

`stars-compat-children` tests logical child windows sharing one root SDL window,
with per-child drawing, nested clipping, coordinate conversion, IDs, enable/focus
state, descendant message/timer filtering and lifetime contracts. The shared
Windows SDK test also checks ownership and destruction order. See
[contracts and limits](../../docs/WIN32-CHILDREN.md).

The child-window batch reached **117/178 Win32 APIs (65.7%)**.

## Positioning and sibling order

`stars-compat-position` tests `SetWindowPos`, callback-adjusted requests,
minimum/maximum sizing, zero-sized windows, sibling ordering and repaint
suppression. `WINDOWPOS` is covered by native/SDK ABI assertions. See
[contracts and limits](../../docs/WIN32-POSITIONING.md), including the observed
Wine owner-family discrepancy and platform-dependent desktop stacking.

The positioning batch reached **118/178 Win32 APIs (66.3%)**. This counts documented local
implementations; it does not mean the complete Linux game client is running.

Mouse capture adds `SetCapture`, `ReleaseCapture` and the supporting `GetCapture`
API. Captured child input keeps signed client coordinates outside its bounds;
capture changes notify the former target synchronously and release on destruction
or SDL focus loss. The capture batch reached **120/178 (67.4%)**. See
[contracts and backend limits](../../docs/WIN32-CAPTURE.md).

Input-state support adds `GetKeyState`, `GetAsyncKeyState`, `GetCursorPos` and
`WindowFromPoint`, reaching **124/178 (69.7%)**. Input queue snapshots preserve
modifier state, text events deliver Unicode scalar values, and control characters, wheels,
input-message screen points and double-clicks are delivered. See
[input tests, CLion targets and backend limits](../../docs/WIN32-INPUT.md).

## Standard buttons and text labels

The standard-control batch reaches **131/178 Win32 APIs (73.6%)**. Select
`stars-compat-controls` in CLion to debug push/check/radio buttons, group boxes,
text labels, dialog-item helpers, mouse capture, Space activation and parent
notifications. The shared SDK target is `test_win32_controls_reference`. See
[contracts, visual limits and verification](../../docs/WIN32-CONTROLS.md).

Edit/list/combo controls, dialog templates/routing, menus and full-client linking
remain. This percentage counts tested API bodies with documented limits.

## Edit fields

`stars-compat-edit` now tests narrow text editing, selection, limits, read-only
fields, password rendering, input delivery and notification lifetime safety.
See [edit contracts and remaining work](../../docs/WIN32-EDIT.md). This extends
existing APIs, so inventory coverage remains **131/178 (73.6%)**.

## String list boxes

`stars-compat-list` tests copied strings, single/multiple selection, fixed-height
owner drawing, item data, keyboard/mouse notifications and callback lifetime
safety. See [list contracts and remaining work](../../docs/WIN32-LISTS.md).
Inventory coverage remains **131/178 (73.6%)**.

## Desktop geometry and settings

`stars-compat-desktop` tests primary/virtual display geometry, work-area queries,
point-array mapping, window process/thread identity and SDL video ownership.
See [supported metrics and limits](../../docs/WIN32-DESKTOP.md). This batch reaches
**135/178 Win32 APIs (75.8%)**.

## Active window queries

`GetActiveWindow` tracks the active top-level window independently of keyboard
focus. Child focus selects its root; clearing focus preserves activation. SDL
focus loss and active-window destruction clear activation. Tests extend
`stars-compat-input` (also run against the Windows SDK) and `stars-compat-windows`.
Coverage is **136/178 Win32 APIs (76.4%)**. See
[activation contracts and limits](../../docs/WIN32-ACTIVATION.md).

## Window states and placement

`stars-compat-state` tests saved normal placement, child minimize/maximize/restore
behavior, style bits, size notifications and callback destruction. Its shared
SDK counterpart is `test_win32_state_reference`; native cases also cover SDL
events and workspace conversion with negative display origins. Coverage is
**139/178 Win32 APIs (78.1%)**. See
[state contracts and limits](../../docs/WIN32-STATE.md).

## Window attention

`stars-compat-attention` tests `FlashWindow`, prior caption-state returns,
request/cancellation, failure handling and callback destruction. Its optional
private-X11 test checks real urgency hints and unchanged physical focus.
The SDK counterpart checks messages and focus under Wine, whose return-value
difference is documented in [attention contracts and limits](../../docs/WIN32-ATTENTION.md).
Coverage is **140/178 Win32 APIs (78.7%)**.

## Scroll state

`stars-compat-scroll` tests ranges, clamping, full-width positions, independent
state and control messages. Its SDK counterpart is `test_win32_scroll_reference`.
The `SCROLLBAR` class owns range/position state and paints arrows, a shaft and a
thumb. Arrow/page clicks, thumb dragging and keyboard requests notify the parent;
the parent retains responsibility for changing the position. Standard nonclient
bars and mouse repeat timers remain unfinished. Coverage is **143/178 Win32 APIs (80.3%)**.
See [scroll state](../../docs/WIN32-SCROLL-STATE.md) and
[scroll controls and limits](../../docs/WIN32-SCROLL-CONTROLS.md).

## Content scrolling and update regions

`stars-compat-scrollwindow` checks client pixel movement, exposed damage,
clipping, pending damage, child relocation and paint regions with holes. Its SDK
counterpart is `test_win32_scrollwindow_reference`. `ScrollWindow` now works;
paint clipping remains separate from application DC clipping, and partial
`ValidateRect` removes only the specified damage. Coverage is **144/178 Win32
APIs (80.9%)**. See [content scrolling](../../docs/WIN32-SCROLL-WINDOW.md).

## Menu data and ownership

`stars-compat-menu` tests menu trees, copied labels, command/position lookup,
insertion/deletion, check and enabled state, submenu ownership and teardown.
Its Windows SDK counterpart is `test_win32_menu_reference`. Coverage is now
**153/178 Win32 APIs (86.0%)**, with six CRT helpers separate. Menu rendering,
window menu attachment, resources and popup tracking remain unfinished; see
[menu contracts](../../docs/WIN32-MENUS.md).

## Window menus and interactive popups

`stars-compat-popup` exercises `GetMenu`, window attachment/lifetime and
`TrackPopupMenu` with command delivery, nested submenus, keyboard/character
navigation, SDL pointer input, rendering, cancellation and reentrant teardown.
Its SDK counterpart is `test_win32_popup_reference`; `--demo` opens an interactive
popup in CLion. Coverage is **155/178 Win32 APIs (87.1%)**. Menu-bar rendering and
menu resources remain unfinished. See [popup contracts](../../docs/WIN32-POPUPS.md).

## Canonical menu resources and accelerators

`stars-compat-ui-resources` compares the menu and both accelerator resources
with the Windows SDK target `test_win32_ui_resources_reference`, then exercises
class-default menus and all 22 game shortcut bindings. Current coverage is
**157/178 Win32 APIs (88.2%)**, with six CRT helpers counted separately and
21 inventoried APIs remaining. Menu bars and full client integration remain;
see [resource contracts and limits](../../docs/WIN32-UI-RESOURCES.md).

## Visible menu bars

`stars-compat-menubar` tests reserved client space, child/screen coordinates,
composed pixels, wrapping and pointer/keyboard dropdown tracking on dummy and
X11 SDL backends. `--demo` opens an interactive menu window in CLion. Its SDK
counterpart is `test_win32_menubar_reference`. Coverage at this milestone was
**158/178 Win32 APIs (88.8%)**, with 20 APIs remaining; the full SDL client
is still not linked. See [menu-bar scope](../../docs/WIN32-MENU-BARS.md).

## Icons and cursors

`stars-compat-images` verifies all 10 canonical icons and 11 cursors against
the SDK target `test_win32_images_reference`. Tests cover resource bytes,
hotspots, ownership, AND/XOR drawing and two-window cursor routing on dummy
and X11 SDL backends. Basic inventoried coverage is now **164/178 (92.1%)**,
with **14 APIs remaining** and six CRT helpers counted separately. The full
SDL game client is still not linked. See [contracts and limits](../../docs/WIN32-ICONS-CURSORS.md).

## Dialog templates

All 34 canonical dialog resources (335 controls) now match windres bytes and
pass the bounded runtime parser in `stars-compat-ui-resources`. Every truncated
resource prefix is rejected. Dialog creation and lifecycle remain unfinished;
coverage stays **164/178 (92.1%)**. See [template scope](../../docs/WIN32-DIALOG-TEMPLATES.md).

## Dialog creation and lifecycle

`stars-compat-dialog` tests modeless isolation, font-based geometry, initialization,
Tab navigation, deferred modal completion, nesting and owner restoration on dummy
and X11 SDL backends. The SDK counterpart is `test_win32_dialog_reference`.
Coverage is now **167/178 (93.8%)**, with **11 APIs remaining**; deeper dialog semantics remain. See [contracts and limits](../../docs/WIN32-DIALOGS.md).

## Combo boxes

`stars-compat-combo` tests editable dropdowns and fixed-height string/owner-drawn
lists, independent selection, item data, keyboard routing, SDL popup input and
reentrant mutation/destruction. Its SDK counterpart is `test_win32_combo_reference`.
All **34/34** canonical dialog templates now create, up from 28/34. API coverage
stays **167/178 (93.8%)** because these control messages use existing APIs.
Select the target under `sdl-debug` or `sdl-release` in CLion. See
[contracts and limits](../../docs/WIN32-COMBOS.md).

## Logical palettes

`stars-compat-palette` tests copied palette entries, independent DC selections,
true-color realization, indexed drawing and palette-indexed DIB input against
`test_win32_palette_reference`. Current inventoried coverage is **171/178 (96.1%)**,
with seven APIs remaining. Select the target under `sdl-debug` or `sdl-release`
in CLion. See [contracts and limits](../../docs/WIN32-PALETTES.md).

## Message boxes and alert audio

`stars-compat-messagebox` tests seven button sets, default focus, keyboard and
mnemonic routing, owner/task modality, nested prompts, scrolling text, quit and
owner destruction on dummy and X11 SDL backends. The audio tests inspect queued
PCM and cover missing audio, intentional muting and externally owned SDL audio.
The SDK counterpart is `test_win32_messagebox_reference`. Select the native
target under `sdl-debug` or `sdl-release` in CLion. No new packages are needed.

Coverage is now **173/178 Win32 APIs (97.2%)**, with five APIs remaining. This
is basic tested coverage; the full SDL client remains unlinked. See
[contracts, platform differences and verification](../../docs/WIN32-MESSAGE-BOXES.md).

## Open/save file pickers

`stars-compat-filedialog` tests the actual modal picker, wildcard filters, directory
navigation, Return/Escape, overwrite refusal/confirmation, read-only rejection,
default extensions, filename offsets, filter indices and output-buffer errors.
It also covers current-directory policy, pending quit and owner destruction on
dummy and X11 backends. The SDK counterpart is `test_win32_filedialog_reference`.
Select the native target in CLion under `sdl-debug` or `sdl-release`.

Coverage is now **175/178 APIs (98.3%)**, with printing and help still to implement.
The full SDL client remains unlinked. No extra packages are needed; the picker
uses the existing compatibility controls. See
[contracts and limits](../../docs/WIN32-FILE-DIALOGS.md).

## Default-printer DCs and document escapes

`stars-compat-print` tests default-printer metadata, 300-DPI GDI canvases, page
transitions, repeated STARTDOC rejection, atomic PDF output, abort/deletion,
Letter/A4 orientation, destination browsing and CUPS process acceptance/failure.
`win32_print_pdf` independently checks the generated documents with Poppler,
including rendered colors, text, page counts and staging cleanup. Tests use a
fake spool executable and never print physical pages.

Coverage is now **178/178 Win32 APIs (100%)**. `WinHelp` now opens the original guide in the desktop browser. Remaining work includes
deeper full-client qualification, real-desktop checks and native tutorial
automation. Application entry and process cleanup are implemented; legacy
desktop logoff is explicitly unsupported and exits Stars only. Select the target under `sdl-debug` or `sdl-release` in CLion.
Cairo/PDF and Poppler are now required for this tested configuration; CUPS's `lp`
is optional for system-printer output. See
[setup, supported flags and limits](../../docs/WIN32-PRINTING.md).

Original help content now has checked topic, paragraph/table, font and image
decoders: 414 topics, 32 font descriptors, 125 bitmaps, eight WMFs and 28 image
hotspots. All bitmap pixels match independent reference exports. Install
`libwmf-bin` for the checked WMF-to-SVG conversion path. The 37 help checks pass
in both SDL configurations. The `stars-help-guide` target now generates 414
formatted HTML topics with clickable links, popups and a filterable index;
the `WinHelp` adapter now has tests for all 413 contexts; the user confirmed offline browser launch, with offline content checks still remaining. See [help content and verification](../../docs/WIN32-HELP.md).

## Common-control toolbar

The application now uses `ToolbarWindow32`, with a scoped `commctrl.h`, owned
bitmap/text storage, check groups, split/whole dropdown notifications and anchored
tooltips. Windows uses its native `comctl32` control. See
[toolbar contracts, reference tests and remaining scope](../../docs/WIN32-TOOLBARS.md).

All decoded application text is UTF-8, including controls, captions, clipboard,
resources and inspection. Hierarchy snapshots always mark `fUtf8Text` true for
API compatibility. Native Windows text/chooser boundaries convert strictly to
UTF-16. See [encoding contracts](../../docs/UNICODE-BOUNDARIES.md).

The canonical guide is `res/help-utf8.zip`, staged without legacy decoding by
`helpgen.py`. Original HLP decoding tools live in `tests/reference/winhelp` for
historical inspection and are excluded from the product build.
