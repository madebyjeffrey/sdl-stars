# Ellipses and cosmetic pen styles

This milestone reaches **109/178 inventoried Win32 APIs (61.2%)**. `Ellipse`
is the new inventoried API. The existing CreatePen/LineTo/Rectangle APIs now
also support cosmetic DASH, DOT, DASHDOT and DASHDOTDOT styles. This improves
the implemented subset beyond the one-function increase in the API count.

## Behavior

Ellipse uses the selected pen, brush and ROP2 mode. Its current position stays
unchanged, reversed corners normalize, right/bottom edges are excluded, and
window origins and application clipping apply. Solid or null cosmetic pens
are supported. Solid, null and copied pattern brushes use the existing brush
sampling path. Outline pixels are accumulated once before drawing, so mirrored
endpoints cannot cancel under XOR; repeated XOR ellipses restore their pixels.
Only the visible bounding rectangle needs mask storage, including for large
partially visible shapes.

The outline adapts Alois Zingl's MIT-licensed `plotEllipseRect`, with Win32
rounding/degenerate-shape adjustments, private 128-bit error accumulators, clipped
span accumulation and separate pen/brush painting. The source comment links the
algorithm and `compat/win32/LICENSE.zingl`; CMake copies the license beside the
SDL executables. We did not copy Wine implementation code or change Free API's
retained source.

The application's dotted mass-path pen uses three foreground pixels followed
by three gaps, as measured with the real SDK executable in Wine. Other cosmetic
styles use the SDK's measured dash patterns. Transparent mode leaves gap pixels
alone; opaque mode applies the DC background color and ROP2 to them. Each LineTo
starts its own pattern; clipping retains the original phase. Rectangles traverse
counterclockwise from top-right with one shared dash phase and each corner drawn
once. Width 0 and width 1 are cosmetic; wider pens remain unsupported.

## SDK comparison and narrow-fill differences

[The saved reference table](../tests/compat/fixtures/ellipse_sdk.h) contains
1,728 SDK/Wine hashes: widths and heights 1 through 24 with solid pen/brush,
null pen or null brush, rendered at (3,3) into a 40x40 color memory bitmap.
The capture used wine-9.0 (Ubuntu 9.0~repack-4build3).

All 576 outline-only cases match. In a clean capture sequence, 1,703 of the
1,728 complete images matched, with 25 differences in very narrow filled
ellipses. Pixel inspection found stray SDK fill spans outside their bounding
rectangles; the spans also change when the sequence follows earlier drawing
operations. For example, the in-batch null-pen 3x8 ellipse produced blue pixels
at x=9..20, outside its x=3..5 bounds, although its isolated mask did not.

The regression therefore compares **1,680 stable hashes** directly. The 48
potentially affected records cover both filled modes for width 3 / height >=8
and width 4 / height >=18. Native tests check bounds for all 1,728 records,
including these 48, instead of reproducing stray SDK pixels. Those thin filled
cases are a documented rasterization difference; this does not establish
pixel identity with every Windows driver. Our output remains bounded and
supports the game's ordinary circles and larger dialog ellipses.

The reference can be inspected without overwriting it:

```sh
WINEDEBUG=-all wine dist/mingw-debug/tests/test_win32_geometry_reference.exe --dump > /tmp/ellipse-sdk.csv
dist/sdl-debug/bin/stars-compat-geometry --dump > /tmp/ellipse-native.csv
# Text masks for a specific width, height and mode (0/1/2):
dist/sdl-debug/bin/stars-compat-geometry --pixels 8 8 0
```

## Build and debug

No new system package is required. Reload the SDL CMake profile in CLion and
select `stars-compat-geometry`, or run:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_geometry --output-on-failure
```

The same source builds as `test_win32_geometry_reference` with the MinGW SDK.
The tests cover the reference grid, fill/outline separation, bounds, current
position, origin translation, XOR restoration, transparent/opaque dots,
clipped dash phase, connected rectangle phase and resource ownership. Native
checks also exercise a 1,048,576-pixel clipped ellipse and reject an oversized
INT_MIN/INT_MAX ellipse without overflowing arithmetic.

## Limits and next work

Ellipse dimensions above 1,048,576 fail when visible; entirely offscreen shapes
are a no-op. Dashed/dotted ellipse outlines, thick pens, non-default mapping
modes, palettes and system-color brush pseudo-handles remain unsupported.
The full Linux client still needs child windows, controls, dialogs, menus,
remaining resources/services and application integration. Passing these
primitive tests does not establish a working full client.

Contracts follow Microsoft's [Ellipse documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-ellipse)
and [CreatePen documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createpen).
The adapted algorithm and its license are published by [Alois Zingl](https://github.com/zingl/Bresenham).

## Verification

- SDL Debug and Release: all 17 compatibility tests pass.
- Windows/Wine: all 27 unit tests pass, including the shared geometry reference.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 existing warnings), zero differences,
  errors or skipped reference inputs. No game source or baseline changed.
- Address, undefined-behavior and leak sanitizers pass, including the maximum
  supported clipped ellipse size, with sanitizer errors made fatal. Both SDL output directories contain the
  algorithm's exact MIT license.

The required Wine tutorial failed at year 2406, page 21, instruction 166,
"Now shift click back on Stove Top", after two attempts and 102 actions.
Artifacts: `dist/scaffold/tutorial/20261010T183121.126443Z`. Earlier runs reached
year 2408 before failing at instruction 203. Reliable tutorial completion
remains outstanding. This batch did not change tutorial/game code, and the
Windows tutorial does not link the local SDL GDI implementation.
