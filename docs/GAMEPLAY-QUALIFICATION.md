# Native gameplay workflow qualification

The product workflow gate runs the shared WinMain, real resource templates,
message loop and game UI. It uses copied regression fixtures or creates fresh
games in temporary directories. No player saves or desktop settings are edited.
Desktop cases run sequentially on private X11 or Wayland displays; dummy cases
exercise the same UI without visible windows.

| Workflow | Automated evidence |
| --- | --- |
| Continue/open | Relative and absolute ASCII/UTF-8 paths; map, toolbar and pane initialization |
| Race files | Portal open/save/cancel/reload, complete suggested names and checksums; wizard habitat pages |
| Research/production | Commit, cancel, reopen and state assertions |
| Plans/relations | Checkbox, combo and radio input; restored plan/relation bytes |
| Ship/starbase designer | Hull selection, component drag/remove/reinsert, name input, cancel/accept/delete; original active designs and counts preserved |
| Fleet workflows | Cargo transfer/reversal/cancel, repeat orders, transport presets, route append/delete, merge and split |
| Reports/browser/map | Report and scanner scrollbar behavior, map objects/hit testing, toolbar dropdowns/tooltips, Technology Browser Close/X |
| Save/reload | Save the changed research order through File Save, reload through the MRU command, assert persisted order and game identity |
| New Game | Basic and Advanced creation in ASCII/UTF-8 paths, chooser cancellation restores title/game state, wizard Back/Next, Finish creates nonempty XY/HST/M1 files and opens the map |

The designer and save/reload scenarios run four copied-game paths per backend.
The New Game launcher runs Basic and Advanced variants with ASCII and UTF-8
names. Its native chooser wrapper supplies a deterministic cancel/result; the
real portal transport and selection contracts are tested separately by
`win32_fileworkflow`. The tests drive buttons, combos, edits and dragging through
translated SDL events. Menu commands enter the actual application handlers.

The five focused fatal ASan/UBSan/LSan cases pass on dummy/private X11/native
Wayland. The full non-tutorial gate passes 120/120, including eight Wayland cases
with the explicit repaired Mesa dependency. Windows unit/reference tests pass
55/55; full native regression matches all 519 checkpoints, with 87 existing
unused-storage warnings and no differences/errors or skipped inputs. Debug,
MinGW and Release builds pass without new warnings in the changed harness.
No application behavior, saved record layout or regression baseline changed.

Real compositor/hardware checks remain in desktop qualification. The full
tutorial, PDF printing and offline help acceptance remain separate milestones;
this product gate does not claim those are finished. Earlier qualified Battle
VCR evidence is retained with the tutorial work rather than extended here.

```bash
cmake --build dist/sdl-debug --target stars-compat-client-startup
ctest --test-dir dist/sdl-debug -R 'win32_client_(designer|newgame)' \
  --output-on-failure -j 1
```

Headless presets exclude both X11 and Wayland qualification cases, so they do
not require a display server. The broader desktop suite uses private displays.
