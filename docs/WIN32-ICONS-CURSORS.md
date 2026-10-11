# Icons and cursors

This milestone adds `LoadIcon`, `LoadCursor`, `DrawIcon`, `SetCursor`,
`DestroyIcon` and `DestroyCursor`, bringing tested basic coverage to
**164/178 inventoried Win32 APIs (92.1%)**, with **14 APIs remaining**.
Supporting `CopyIcon`, `GetIconInfo` and `GetCursor` are outside that count.
The full SDL game client is still not linked.

## Resource and drawing contracts

`compat/win32/imagegen.py` embeds all 10 canonical icons and 11 cursors from
the RC source as grouped and raw image resources. The SDK comparison checks
every raw image byte, group identity, dimensions and cursor hotspot against
the original resources compiled by MinGW windres.

The runtime bounds DIB and mask reads, retains shared module image handles,
and makes independent owned copies. Destroying a shared loaded handle is a
successful no-op; destroying an owned copy releases its pixels and native
cursor. `GetIconInfo` returns caller-owned bitmap copies. Shutdown releases
the cache before the SDL video subsystem closes.

`DrawIcon` applies the image's AND/XOR masks to a 32-by-32 destination through
the DC origin and clipping state. Pixel tests cover every canonical image on
black and colored backgrounds. Class icons are supplied to each SDL window.

System and ordinary custom cursors use SDL native cursors. The custom help
arrow contains inverted pixels, so it is composed over the window surface
using the original AND/XOR operation. Moving it between windows or leaving a
window removes the old overlay without changing stored client pixels.
`WM_SETCURSOR` restores the class cursor through the default procedure.

## Qualified scope

Canonical assets are single-entry, uncompressed 32-by-32 DIB images: 4-bit
icons and monochrome cursors. The loader selects the first group entry;
multi-resolution selection, PNG-compressed icons, alpha-icon blending and
arbitrary module loading remain outside this milestone. System cursor copies
and `GetIconInfo` for system cursors are unsupported. Native pointer behavior
is tested on X11; native Wayland remains unqualified.

SDL pointer translation queues `WM_SETCURSOR` before client mouse motion;
complete Win32 synchronous event ordering remains a compatibility limit.

## Build and debug

Select `stars-compat-images` in CLion's `sdl-debug` profile. This is a finite
test executable; breakpoints can inspect decoding, painting and cursor routing.
No additional libraries are required.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
SDL_VIDEODRIVER=dummy STARS_SDL_RENDERER=software \
  dist/sdl-debug/bin/stars-compat-images
xvfb-run -a ctest --test-dir dist/sdl-debug -R win32_images --output-on-failure
```

## Remaining inventory

`CreateDialog`, `DialogBox`, `EndDialog`, `CreatePalette`, `SelectPalette`,
`RealizePalette`, `UnrealizeObject`, `Escape`, `PrintDlg`, `GetOpenFileName`,
`GetSaveFileName`, `MessageBeep`, `MessageBox` and `WinHelp` remain.
Dialog templates, fuller control semantics, service integration and linking
the actual SDL game client are still required beyond this API count.

## Verification

- SDL Debug and Release compatibility suites: 39/39 each.
- Windows SDK/unit suite: 44/44.
- Linux host build and unit tests: 12/12.
- Icon/cursor address, undefined-behavior and leak sanitizer checks: passed.

The fresh automated tutorial failed at year 2408, page 28, instruction 216
after 109 actions: AutoHotkey `ControlClick` on the next-message button
reported error 87 (invalid parameter). Artifacts are in
`dist/scaffold/tutorial/20261011T001035.532424Z`. This Windows tutorial does
not link the SDL runtime; the failure's origin remains unresolved.

Native regression matched all 519 reference checkpoints, with 22 existing
warnings, zero differences/errors and no skipped reference inputs. No
regression baseline or game behavior changed.
