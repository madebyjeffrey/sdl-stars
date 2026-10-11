# String list boxes over SDL3

Stars' fleet, design and production lists now have a local narrow `ListBox`
class. Coverage remains **131/178 inventoried Win32 APIs (73.6%)**, because this
adds control behavior behind existing window/message APIs. Six CRT helpers are
counted separately. Full-client linking and remaining controls are still pending.

## Supported contracts

The class owns copied strings and pointer-sized application item data. It
supports add/insert/delete/reset, count and text retrieval, case-insensitive
prefix/exact searches, item data, current selection and caret indexes,
multiple-selection flags/counts/index arrays, fixed item height, item rectangles
and the top visible index. Insertion/deletion preserve shifted selection indexes.
Programmatic selection does not send `LBN_SELCHANGE`.

Keyboard navigation covers Up/Down, Home/End and Page Up/Down. Multiple-selection
lists move their caret independently and toggle the caret item with Space.
Pointer presses update selection; mouse release sends `LBN_SELCHANGE` after
capture has been released. Double clicks send `LBN_DBLCLK`. Focus notifications
use the control ID and HWND through `WM_COMMAND`. Wheel input scrolls rows, and
selection/top-index changes keep the relevant item visible.

`LBS_OWNERDRAWFIXED | LBS_HASSTRINGS`, used by Stars' production and design
lists, sends `WM_MEASUREITEM` once at creation. The requested height applies to
all rows. Paint sends `WM_DRAWITEM` with the item ID, rectangle, selected/focus/
disabled flags, HWND, DC and application data. Reset, deletion and destruction
send `WM_DELETEITEM` for owner-drawn entries. `DELETEITEMSTRUCT` now has shared
64-bit SDK/native ABI assertions.

The implementation checks lifetime after synchronous parent callbacks. A drawing
callback may reset the list or destroy it; mutation stops the current row walk
and requests another paint. Reset detaches its old array before notifications,
so callbacks cannot invalidate the cleanup traversal. Parent color brushes and
control fonts are borrowed; copied strings and list storage belong to the class.

The shared SDK test covers string/index operations, sorting, item data,
multiple-selection APIs, top-index/rectangle results, keyboard notifications,
owner measurement/drawing/deletion and destruction on mouse release. Native tests
also check actual owner-drawn pixels, SDL pointer routing and reset/destruction
inside drawing callbacks.

The contracts follow Microsoft's [list-box documentation](https://learn.microsoft.com/en-us/windows/win32/controls/about-list-boxes),
[item rectangles](https://learn.microsoft.com/en-us/windows/win32/controls/lb-getitemrect)
and [item data](https://learn.microsoft.com/en-us/windows/win32/controls/lb-getitemdata).
The SDK comparison confirms success value 1 for both item-data writes and item
rectangles; callers must not assume all successful list messages return zero.

## October 11 follow-up

Shift selects anchored ranges; Ctrl moves the extended-list caret without clearing
selection and Ctrl+Space toggles its item. Mouse drag selects rows and timer-driven
autoscroll reaches offscreen rows. Native typeahead accumulates a one-second
prefix and repeated first letters cycle matches. Vertical scroll messages move
the viewport. Plain-list font changes recompute row height. See
[current qualification](WIN32-CONTROL-QUALIFICATION.md). The inventory is now
178/178 and the native client is linked; earlier counts describe the first milestone.

## Further behavior and limits

Visible vertical/horizontal bars, page ranges, horizontal extent/scrolling,
Ctrl-drag grow/shrink selection, integral-height creation/resizing and viewport
clipping for owner-drawn rows are implemented. See
[current qualification](WIN32-CONTROL-QUALIFICATION.md).

Variable-height, multicolumn, no-data and owner-drawn lists without strings fail
creation. Locale-aware sorting/search, exact cancellation/notification ordering
and broader owner-draw selection-action sequences still need qualification.

Fixed owner-drawn lists now send `ODA_FOCUS` on focus gain/loss before their
focus notifications. Empty focused lists also send `itemID == -1` when painted,
allowing Stars' owner to draw its focus indicator. Shared SDK tests cover nonempty
focus state and notification ordering; native tests cover empty focus painting,
row bounds and callbacks resetting/destroying the list. Wine draws the empty
focus rectangle itself, so that documented native case is not claimed as Wine
equivalence. See [qualification evidence](WIN32-CONTROL-QUALIFICATION.md#milestone-2-qualification-targets).

Single-selection fixed owner-drawn lists send synchronous `ODA_SELECT` actions
for the old and new visible rows. Re-selecting the same row sends no action.
Keyboard selection draws before notifying the owner. Shared SDK/native tests
cover these transitions; native tests cover resetting and destroying the list
from a drawing callback. Multi-selection action sequences and broader drawing
state qualification remain open.

## CLion and verification

No additional dependencies are needed. Select `stars-compat-list` in CLion's
SDL Debug profile. For headless debugging use
`SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software`. Real compatibility windows
currently require X11/Xwayland.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_list --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_list_reference --output-on-failure
```

## Verification

On October 10, 2026:

- SDL Debug and Release: all 25 compatibility tests pass in each build.
- The shared list test passes against the Windows SDK under Wine.
- The list test passes with SDL's X11 backend on an isolated Xvfb display.
- Fatal AddressSanitizer, UndefinedBehaviorSanitizer and leak checks pass.
- Windows SDK/game: all 34 unit tests pass under Wine.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.

- The required tutorial check failed at year 2408, page 26, instruction 203:
  `No ship transfer arrow is available`, after 121 actions. Artifacts:
  `dist/scaffold/tutorial/20261010T205726.573772Z`. This is a Windows/Wine
  automation failure, not a passing tutorial result.

Windows and Linux host builds do not link this SDL implementation. Game logic,
saved-file layouts and baseline fixtures are unchanged.
