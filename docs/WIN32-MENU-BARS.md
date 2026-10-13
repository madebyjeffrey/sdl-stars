# Visible menu bars

`DrawMenuBar` brings basic inventoried Win32 API coverage to **158/178 (88.8%)**,
with **20 APIs remaining**. The six CRT helpers and supporting APIs are counted
separately. The full `stars-sdl` game client is still not linked.

## Rendering and geometry

Attached top-level menus now render above the client canvas. The runtime
measures captions with the existing GUI font and wraps items into additional
rows when a window is narrow. `DrawMenuBar` refreshes layout and cached pixels
after menu mutations; `SetMenu` updates attachment and layout. Captions use
mnemonic prefixes and disabled items use gray text. Active headings are
highlighted while selecting or tracking a dropdown.

The menu has a separate color bitmap; client pixels are preserved when it is
attached or removed. Presentation composes the client canvas and children
below the menu bitmap, then uploads the whole SDL window texture through the
existing renderer. This preserves both SDL renderer modes and per-window
resources.

`GetClientRect` subtracts menu height, `ClientToScreen`/`ScreenToClient` and
`MapWindowPoints` include it, and child origins remain relative to the client.
Root window rectangles and positioning retain the outer SDL dimensions.
Resizes update the client height and wrapping. Menu-height changes notify
created owners with `WM_SIZE`; class-menu setup avoids callbacks before
`WM_NCCREATE`/`WM_CREATE`. Menu areas participate in `WindowFromPoint`.

The supporting `GetMenuItemRect` implementation reports attached bar item
rectangles in screen coordinates. Popup item rectangles through this API are
not implemented; popup tracking uses its own measured rectangles internally.

## Input and dropdown tracking

SDL mouse input over an uncaptured menu queues nonclient messages with
`HTMENU` and screen coordinates. Captured input converts from outer SDL
coordinates to client coordinates, so existing popup tracking can locate both
menu headings and popup rows. Client input is shifted below the bar.

The default window procedure opens headings on left click or an Alt mnemonic.
Raw SDL F10 system-key events and Alt release select the menu; Left/Right move the selection and
Down/Enter open a dropdown. Escape cancels selection. A focused child control
forwards system-key menu handling to its top-level owner. Alt-letter key events
can open headings even when SDL omits a text-input event; a matching later
system character is consumed once to avoid duplicate activation. During dropdown tracking,
Left/Right switch top-level headings where appropriate, and moving over another
heading switches its popup. Existing popup navigation and command selection
apply inside each dropdown.

Menu-bar tracking uses the same guarded modal machinery as `TrackPopupMenu`:
owner initialization/selection callbacks, popup lifetime, capture restoration,
`EndMenu`, destruction handling and queued `WM_COMMAND` delivery. Bar loops
report `WM_ENTERMENULOOP`/`WM_EXITMENULOOP` with a false popup flag. Each window
owns its layout, cached bitmap and selection state.

## Scope and remaining work

The implementation supports the game's string menus and nested dropdowns.
Owner-drawn/bitmap menu items, system-menu integration, right-justified entries,
platform theme matching and complete nonclient caption/frame behavior remain
outside this milestone. Extremely small windows that cannot contain their
menu rows are not a qualified application layout. Menu bars use the local
classic drawing style rather than OS-native widgets. Dialog templates and
remaining client integration are still required.

The shared geometry test uses the menu's owning window, as specified by the
[GetMenuItemRect contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmenuitemrect).
The SDK also accepts an unrelated window argument but returns different
coordinates; those unsupported caller combinations are not a portability
contract. See Microsoft's [DrawMenuBar contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawmenubar)
for refresh behavior.

## CLion and tests

Select `stars-compat-menubar` under `sdl-debug` or `sdl-release` in CLion. Add
`--demo` to program arguments for an interactive File/Edit menu window.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
SDL_VIDEODRIVER=dummy STARS_SDL_RENDERER=software \
  dist/sdl-debug/bin/stars-compat-menubar
# Interactive desktop:
dist/sdl-debug/bin/stars-compat-menubar --demo
```

`win32_menubar` and `win32_menubar_x11` run the native tests. The SDK counterpart
is `test_win32_menubar_reference`. Shared tests cover owner/client/child
geometry, attachment/removal, keyboard activation, dropdown switching,
cancellation, loop notifications and destructive initialization. Native tests
also inspect composed pixels, raw F10/Alt input with child focus, SDL pointer routing, hover switching and wrapped
rows. The X11 test shows/paints the window before baseline measurements so
renderer initialization does not invalidate its geometry comparison.

`STARS_MENUBAR_CAPTURE=/absolute/path.bmp` saves the native composed test window
for visual inspection. Address/undefined/leak sanitizers cover bitmap lifetime
and destructive tracking callbacks.

## Verification results

- SDL Debug and Release compatibility suites: 37/37 each.
- Windows SDK/unit suite: 43/43.
- Native regression: 519 matches, 22 existing warnings, zero differences/errors or skipped reference inputs.
- Linux host build and tests: 12/12.
- Address, undefined-behavior and leak sanitizer checks: passed.
- Composed menu/client pixels were inspected in the saved test image.
- The fresh Wine tutorial run failed at year 2407, page 24, instruction 186 after 94 actions with `free(): invalid next size (fast)` and an observer timeout. Artifacts: `dist/scaffold/tutorial/20261010T234924.212002Z`.

The tutorial failure remains unresolved. The Windows tutorial does not link
this SDL compatibility runtime.
