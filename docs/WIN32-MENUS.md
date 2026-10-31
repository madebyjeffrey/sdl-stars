# Menu data and ownership

Coverage is **153/178 inventoried Win32 APIs (86.0%)**, up from 144/178 (80.9%).
The nine new inventoried implementations are `CreatePopupMenu`, `AppendMenu`,
`InsertMenu`, `DeleteMenu`, `DestroyMenu`, `GetMenuItemCount`, `GetSubMenu`,
`CheckMenuItem` and `EnableMenuItem`. Six CRT helpers remain counted separately.
These are tested API contracts, not a percentage of full-client readiness.

## Contracts

Menus retain copied narrow labels, separators, command IDs, submenu handles,
check flags and enabled/disabled/grayed state. Command lookup walks nested
submenus; position lookup operates on the specified menu. Insertion preserves
item order and returns failure without modifying the tree when allocation,
lookup or validation fails. Returned check/enable states describe the previous
state, including the documented `-1` failure result. Deleting a submenu destroys
its tree; removing it detaches the tree so it can be reattached or independently
destroyed. Destroying a root releases its descendants and owned strings.

Supporting `CreateMenu`, `IsMenu`, `RemoveMenu`, `GetMenuState`, `GetMenuItemID`
and `GetMenuString` are outside the original inventory. Dead handles remain
recognizable until runtime shutdown, and shutdown frees all remaining trees and
handle records. The local runtime rejects cycles, duplicate submenu ownership,
and trees deeper than 64 levels, including attachment of an already deep tree.

The contracts follow Microsoft's [InsertMenu](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-insertmenua),
[EnableMenuItem](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablemenuitem)
and [GetMenuState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmenustate)
documentation, with SDK comparisons for the application's string/separator/popup
usage. The observed Wine 9 SDK implementation retains a separator's supplied ID
and reports it disabled/grayed. Its truncated ANSI string query returns the
expected copied-character count but does not terminate the destination correctly.
Shared tests enforce the count and prefix; local tests additionally enforce null
termination. No upstream source was copied for these implementations.

## Build and debug

Select `stars-compat-menu` in the SDL Debug profile in CLion. No new dependency is
needed. The shared SDK test is `test_win32_menu_reference`.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R '^win32_menu$' --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_menu_reference --output-on-failure
```

## Remaining work

The subsequent [popup milestone](WIN32-POPUPS.md) implements window attachment,
popup windows, pointer/keyboard navigation, notifications and nested tracking.
Coverage is now 155/178 (87.1%). Menu-bar layout/drawing, accelerators and menu
resources remain. `DrawMenuBar` is not implemented or counted yet.
Owner-drawn/bitmap items and multi-column menu layout are rejected rather than
silently rendered incorrectly. The application currently uses string menus,
separators and nested popup menus. The complete SDL game client remains unlinked.

## Verification

SDL Debug and Release pass all 32 compatibility tests each, including X11
integration. Windows units pass all 40 tests, including the shared SDK menu
suite. The Linux host builds and passes all 12 unit tests. The menu ownership
suite passes fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks.
No new compiler warnings occur in the touched files. Full native regression
returns 519 matches (22 existing warnings), zero differences/errors and zero
skipped reference inputs. No baseline or game behavior changed.

The fresh Windows tutorial remains failing: run
`dist/scaffold/tutorial/20261010T230200.331212Z` passes instruction 166 and reaches
year 2407, page 24, instruction 186 after 94 actions, then times out querying the
observer. Its Wine log reports `free(): invalid next size (fast)`. The Windows
tutorial does not link the SDL runtime or this menu implementation. Earlier
diagnostic evidence captures a native Wine/X11 popup destruction failure, as
recorded in the [tutorial report](../tests/reference/win32-tutorial/README.md). The full
tutorial gate is not passing; it must be resolved before final client completion.
