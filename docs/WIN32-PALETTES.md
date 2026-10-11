# Logical palettes on the SDL true-color backend

`CreatePalette`, `SelectPalette`, `RealizePalette` and `UnrealizeObject` now
have tested basic implementations. Inventoried coverage is **171/178 (96.1%)**,
with seven APIs remaining and six CRT helpers counted separately. The full
SDL game client remains unlinked.

## Contracts

Logical palettes own copies of their entries and use the existing GDI handle
registry, lifetime tracking and stale-handle rejection. Each DC starts with
the stock 20-entry default palette and retains its own selection. A palette
can be selected into multiple compatible DCs; it must be deselected from all
native DCs before deletion. DC teardown releases its selection reference.
`GetObject` returns the palette's entry count, and the supporting
`GetPaletteEntries` API exposes copied entries.

The display and bitmap canvases use RGBA pixels and do not advertise
`RC_PALETTE`. Consequently `RealizePalette` reports zero physical entries
changed for a valid DC and `GDI_ERROR` for an invalid native DC. Background
selection does not change this true-color behavior. `UnrealizeObject` accepts
live application palettes and brushes; brushes retain the DC's brush origin.
There is no physical palette or brush realization cache to reset.

`PALETTEINDEX` colors resolve through the selected logical palette when drawn;
`PALETTERGB` colors behave as conventional RGB on this true-color device.
Resolution applies to solid brushes, cosmetic pens, pixels, text/background
colors and monochrome conversion paths. Changing a palette affects subsequent
drawing without recoloring already rendered RGB pixels.

`StretchDIBits` additionally accepts `DIB_PAL_COLORS` for 1/4/8-bit `BI_RGB`
input. Its WORD color table indexes the selected logical palette. The decoder
copies the resolved colors before drawing, retaining existing clipping,
scanline orientation and ROP behavior. Invalid palette indices are rejected
without modifying destination pixels.

These contracts follow Microsoft's documentation for
[logical palettes](https://learn.microsoft.com/en-us/windows/win32/gdi/logical-palette),
[SelectPalette](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectpalette),
[RealizePalette](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-realizepalette),
[UnrealizeObject](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-unrealizeobject)
and [PALETTERGB](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-palettergb).

## Limits

Physical indexed display modes, palette animation/mutation APIs, palette-change
notifications and `PC_EXPLICIT` physical-system-palette mapping remain
unqualified. Entry flags are retained; reserved/no-collapse flags have no
physical palette allocation effect on this backend. `GetDIBits` still does
not produce 4/8-bit indexed output or `DIB_PAL_COLORS` output. Invalid or stock
object behavior outside the documented contract is checked only natively,
not claimed identical across Windows implementations.

The seven remaining inventoried APIs are `GetOpenFileName`, `GetSaveFileName`,
`PrintDlg`, `Escape`, `MessageBox`, `MessageBeep` and `WinHelp`. Completing them
will still require full-client integration and qualification of deeper UI,
platform and service behavior; API coverage alone is not client readiness.

## CLion and verification

Select `stars-compat-palette` under `sdl-debug` or `sdl-release`; no additional
packages are needed. Its Windows SDK counterpart is
`test_win32_palette_reference`.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R win32_palette --output-on-failure
```

The shared test compares entry ownership and readback, the default palette,
independent DC selections, true-color realization, indexed brushes/pens/pixels,
palette-relative RGB, opaque text backgrounds and indexed DIB input. Native
checks cover malformed inputs, selected-object lifetime, unsupported object
types and stale handles.

A relocatable link probe combines all 64 current game-core/UI objects with
`stars_win32`. Its unresolved Win32 calls are exactly the seven inventoried
APIs above, plus `ExitWindowsEx`, the runtime dependency of the legacy
`ExitWindows` macro. The native executable also needs an SDL entry point
calling `WinMain`. This probe does not prove runnable-client behavior.

The SDL Debug and Release suites pass **45/45** each. The Windows SDK/unit
suite passes **47/47**, and the Linux host build and **12/12** unit tests pass.
Final expanded palette tests pass on dummy and X11 backends and under full-runtime
address, undefined-behavior and leak sanitizers. No game sources or regression
baselines changed.

The final expanded Windows SDK comparison also passes. A fresh Windows tutorial
completed all **80 pages through year 2437** (346 actions); artifacts are in
`dist/scaffold/tutorial/20261011T010033.459039Z`. The tutorial validates the
existing Windows client, which does not link this SDL runtime.

Full native regression matches all **519** checkpoints, with 22 existing
warnings, zero differences/errors and zero skipped reference inputs.
