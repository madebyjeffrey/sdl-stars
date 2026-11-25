# Standard toolbar support

Stars now creates a `ToolbarWindow32` child inside its existing toolbar layout
container. All desktop platforms use the SDL compatibility control; optional
SDK comparison tests use native Windows `comctl32`.
The container retains the scanner-range combo and routes commands to the existing
`ExecuteButton` game UI handler. The existing icons, scanner flags and zoom
commands are preserved. Game logic and saved records do not change.

The design and enemy-class controls are standard split buttons: the image toggles
the filter and the arrow opens its menu. Minefields and zoom use whole-dropdown
buttons. Split buttons paint adjacent full-height bevels at the menu-arrow hit boundary,
with a five-pixel-wide, three-row dropdown triangle.
Opening a split menu depresses only its arrow area and offsets the triangle;
the action area and its checked state remain unchanged throughout the menu loop.
`WM_NOTIFY` / `TBN_DROPDOWN` identifies the triggering command; menu
placement uses `TB_GETRECT` and coordinate mapping. Button clicks use `WM_COMMAND`.
Handled dropdowns return `TBDDRET_DEFAULT`. Returning `TBDDRET_NODEFAULT` or
`TBDDRET_TREATPRESSED` falls back to an ordinary button press, with mouse capture
and a command on release. The notification guarantees only `hdr` and `iItem`;
callers query the rectangle instead of reading `NMTOOLBAR.rcButton`.
The six scanner views form a check group. Toolbar check state is synchronized
from the game's scanner flags whenever the surrounding toolbar repaints.

The previous hand-painted button edges, mouse polling and bitmap-button hit
arithmetic no longer drive the toolbar. The original named entry points remain
as adapters for the standard control. The inspector identifies the new child as
`toolbar`; its command IDs are `IDC_STARS_TOOLBUTTON_FIRST + ToolbarButton`.
`TB_GETBUTTON`, `TB_COMMANDTOINDEX`, `TB_GETRECT` and `TB_HITTEST` can address the
items in-process; toolbar buttons are items, not individual HWNDs.

## Compatibility contracts

The narrow `commctrl.h` defines SDK-compatible `NMHDR`, `INITCOMMONCONTROLSEX`,
`TBBUTTON`, `TBADDBITMAP`, `NMTOOLBARA` and the implemented message/style values.
`InitCommonControlsEx` accepts `ICC_BAR_CLASSES`; class creation is lazy.
The implementation provides:

- Button addition, insertion, deletion, count and command/index queries.
- Button state, enabled/checked/pressed/hidden queries and changes.
- Contiguous check groups, ordinary buttons, split and whole-dropdown buttons.
- Bitmap-strip resources and caller bitmaps, copied into control-owned storage.
- Item rectangles, hit testing, button/bitmap dimensions, indentation, text-row
  visibility and dropdown-arrow extended style.
- Mouse capture with cancellation outside a button, dropdown notification on
  press, keyboard navigation/activation and callback-safe control destruction.
- Direct ANSI button text and delayed tooltips anchored below button rectangles,
  with clipping to the owning frame and native SDL tooltip parenting. The radar
  combo retains the game's tooltip helper, also anchored to its control rectangle.

The native toolbar owns its copied strings, bitmaps and tooltip window. Silent
process teardown releases that storage even when application callbacks are skipped.

This is the toolbar subset used by Stars, not complete `comctl32` conformance.
Image-list APIs, string-pool/resource/callback text, Unicode messages, customization,
multiple-row wrapping, themes, custom draw and the full general-purpose tooltip
control remain outside this milestone. Unsupported messages take the default
window-procedure path; style declarations do not promise every style behavior.
The SDL tooltip uses an internal window class, not `tooltips_class32`.
Interactive desktop qualification currently uses X11/Xwayland; headless tests use
the dummy backend.

## Sources and verification

The implementation is project C++20 code. Wine's
[toolbar implementation](https://github.com/wine-mirror/wine/blob/master/dlls/comctl32/toolbar.c)
was consulted for button styles and dropdown notification timing; its source was
not copied into this control. Microsoft documents the
[toolbar interface](https://learn.microsoft.com/en-us/windows/win32/controls/toolbar-controls-overview)
and [dropdown notification](https://learn.microsoft.com/en-us/windows/win32/controls/tbn-dropdown).

`tests/compat/win32_toolbar.inc` shares ABI and behavior checks with the actual
MinGW/Windows common control. Native additions check keyboard dropdowns, copied
bitmap lifetime, tooltip position/role/focus and cleanup. Native pixel checks
also verify the split button's independent beveled frames.
Menu-notification tests compare every action-area pixel before and during menu
activation, for both checked and unchecked buttons; they verify the arrow's
pressed bevel, triangle offset and release state. The actual-client
Continue Game workflow clicks each of the four menu triggers and checks the
anchor passed to `TrackPopupMenu`. Dummy and private Xvfb/Openbox variants cover
relative and absolute game filenames without touching the user's desktop.

Build and debug `stars-sdl` using the existing `sdl-debug` CLion profile. No new
Linux library is required; the implementation uses the existing SDL/GDI backend.
The independent Windows SDK reference links `comctl32`. A focused invisible run is:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --preset sdl-headless -R 'win32_(toolbar|client_continue_dummy|controls)$'
```

Coverage of the original inventory remains **178/178 APIs (100% basic coverage)**.
This milestone adds `InitCommonControlsEx` and toolbar message contracts beyond
that original inventory; it does not imply complete Win32 behavior coverage.

The complete fatal AddressSanitizer/UndefinedBehaviorSanitizer/LeakSanitizer suite
passes **96/96** tests in a private Xvfb/Openbox environment, including the new
actual-client X11 Continue Game workflow. The standard toolbar reference case
previously passed under Wine. The corrected dropdown return/fallback vector now
also passes on native Windows; see the [SDK diagnosis](CPP-C09-SDK-FINDINGS.md#toolbar-diagnosis-and-correction).
The final visual check uses
the real desktop compositor; the automated tests do not open desktop windows.
