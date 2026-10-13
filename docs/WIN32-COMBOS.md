# Combo boxes

Basic `CBS_DROPDOWNLIST` and editable `CBS_DROPDOWN` controls now work,
including `CBS_OWNERDRAWFIXED | CBS_HASSTRINGS`. All **34/34** canonical
dialog templates create in both the native runtime and Windows SDK tests,
up from 28/34 before this milestone. Current inventoried coverage is
**178/178 (100% basic coverage)**: combo messages use existing APIs. This
count does not imply full Win32 control conformance.

## Implemented contracts

Combo boxes reuse the list implementation's string storage, sorting, searches,
selection and pointer-width item data. Controls retain independent state.
Programmatic selection does not send user selection notifications. Editable
combos use an embedded Edit control; selection updates its text, typing clears
the selected index, and adding an item preserves unselected typed text.

The field collapses to its text height while retaining the template height for
the dropdown. `WS_VSCROLL` applies to the dropdown; it does not reserve client
space or add a scrollbar beside the closed field. Shared SDK/native tests check
full field width, and native paint tests check the single raised dropdown button.
An owned SDL popup displays fixed-height rows without taking
keyboard focus. Capture routes popup input back to the combo, and closing
releases or restores capture. Pointer release commits the selected row.
Hover and navigation in an open list change a pending highlight rather than
the field value. Clicking a row or pressing Return commits the highlight and
sends selection notifications. Escape or an outside click discards it without
notifying the application of a selection change. Closed-field keyboard
navigation continues to select directly.
Keyboard navigation, F4, Alt+arrow, Return/Escape and basic prefix matching
are supported. Extended UI disables F4 and wheel navigation and lets Down
open the list, following the
[documented contract](https://learn.microsoft.com/en-us/windows/win32/controls/cb-setextendedui).
Dialog Tab/Shift-Tab traversal recognizes the embedded edit's owning control.

Combo lists use SDL's native popup-menu role on desktop backends, with positions
relative to the owning window. This prevents a compositor from treating the
list as an independent top-level window and moving it or taking focus. Rows are
painted before showing the list; owner-draw callbacks are revalidated before
mapping in case they close or destroy the control. The same path serves toolbar,
editable, fixed-list and owner-drawn combos.

Owner measurement and drawing use `ODT_COMBOBOX`; drawing the collapsed field
sets `ODS_COMBOBOXEDIT`. Notifications and owner callbacks can mutate or
destroy the control. Callback guards prevent continuing with freed state,
and a storage mutation counter stops drawing stale popup rows.

## Limits

`CBS_SIMPLE`, variable-height owner drawing and owner drawing without strings
remain unsupported. Complete notification ordering, locale-aware typeahead,
dropdown scrollbar dragging/repeat, root movement/hiding while dropped,
theme/DPI fidelity and multiple-monitor placement remain unqualified.
Popup positioning currently uses primary-screen coordinates. X11 is tested;
Wayland remains unqualified. Creating every template does not establish that
every game dialog's callbacks or the full SDL game client work.

## Build and debug

Select `stars-compat-combo` in CLion's `sdl-debug` or `sdl-release` profile.
It runs a finite test with breakpoints available in combo callbacks and popup
dispatch. No additional libraries are needed beyond the existing SDL/font setup.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R win32_combo --output-on-failure
```

The Windows comparison target is `test_win32_combo_reference`; its longer
timeout accommodates the complete resource-dialog creation sweep under Wine.
Shared tests cover data messages, editable text, keyboard selection, extended
UI, owner drawing, embedded edit tab traversal and all 34 templates. Native
tests additionally exercise SDL popup mouse routing and mutation/destruction
during owner drawing.

## Verification

After the owned-popup and deferred-selection fixes, the full fatal ASan/UBSan/LSan suite passes **96/96**
tests in private virtual displays. Combo tests check the native popup role and
parent, placement below the field, text painted before mapping, focus/capture,
mouse selection and owner-draw destruction while opening. Hover and open-list
keyboard navigation leave the value and selection-notification count unchanged;
Escape and outside clicks cancel, while mouse release or Return commits once.
The actual-client
Continue Game tests open the scanner-range list and exercise keyboard selection
on both dummy and X11 backends. Windows SDK/unit tests pass **51/51**; the full
native regression matches **519** checkpoints (87 existing warnings), with zero
differences/errors. Debug, Release and MinGW Debug builds pass. The earlier
milestone results below are retained as history.

SDL Debug and Release suites pass **43/43** each on dummy and X11 backends.
The Windows SDK/unit suite passes **46/46**; a final focused combo comparison
also passes after the editable-text and Shift-Tab additions. Full-runtime
address, undefined-behavior and leak sanitizer checks pass. The Linux host
build and **12/12** tests pass.

The fresh Windows tutorial completed all **80 pages through year 2437**
(346 actions); artifacts are in
`dist/scaffold/tutorial/20261011T004754.017180Z`. This successful run does not
explain the prior intermittent Wine heap-corruption failure recorded in the
[dialog milestone](WIN32-DIALOGS.md), and the tutorial does not link the SDL
compatibility runtime.

The full native regression matches all **519** checkpoints, with 22 existing
warnings, zero differences/errors and zero skipped reference inputs. No game
behavior or regression baseline changed.


Owner-drawn dropdown painting must not schedule another paint merely because
the callback reads `CB_GETCOUNT`, `CB_GETLBTEXT` or item data. `CompatComboProc`
now invalidates popup content only for mapped write operations. A real tutorial
Transport/Colonists dropdown exposed continuous `WM_PAINT` delivery starving
the tutorial timer; a debugger confirmed the owner callback's item-text query
and `cWait == 0`. The combo contract test queries text while drawing and checks
that the popup has no update region after painting or subsequent getters.
The battle-cycle qualification passes all 103 fatal sanitizer tests, 51 Windows
unit/reference tests, the full native regression (519 matches, 87 existing
unused-storage warnings, zero differences/errors/skips) and four Release
tutorial profiles. No game baseline changes; see the goal report.
