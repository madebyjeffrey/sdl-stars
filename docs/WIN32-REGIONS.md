# Win32 clipping regions

This milestone reaches **101/178 inventoried Win32 APIs (56.7%)**. The full
Linux game client still needs drawing state, remaining geometry, child windows,
controls, dialogs, menus and the other services in the implementation plan.

The local C11 GDI implements `CreateRectRgn`, `SelectClipRgn`,
`IntersectClipRect` and `ExcludeClipRect`. Rectangle edges are left/top
inclusive and right/bottom exclusive. Region handles use the existing GDI
ownership and stale-handle protection. Creation normalizes reversed corners
and rejects coordinates outside GDI's signed 27-bit range.

Selection copies the region into the DC: the caller may delete the original
handle immediately, and two DCs can subsequently change their clips
independently. Selecting NULL removes the application clip. It does not remove
the surface bounds or the update rectangle imposed by BeginPaint.

Each DC stores disjoint rectangles. Intersections reduce them; exclusions split
them around holes; adjacent rectangles merge again. Empty clips, single
rectangles and compound clips return NULLREGION, SIMPLEREGION and COMPLEXREGION.
Allocation failure leaves the previous clip intact. FillRect intersects each
piece directly; the shared pixel path clips lines, rectangles, text and ROP3
blits against the same region. Source blits retain their previous source-clipping
behavior: destination clipping controls the output.

The subsequent [drawing-state milestone](WIN32-DRAWING-STATE.md) adds window
origins and preserves device-unit region selection versus logical-unit rectangle
clipping. This is the application's
rectangle-region subset; polygon/ellipse region construction and arbitrary
region combination are not implemented. The subsequent
[content-scrolling milestone](WIN32-SCROLL-WINDOW.md) adds window update regions
and partial `ValidateRect` subtraction.
Desktop DCs remain measurement devices, without a drawable desktop surface.

## Build and debug

No additional dependency is required. Reload the SDL CMake profile in CLion and
select `stars-compat-regions`, or run:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_regions --output-on-failure
```

The same source builds as `test_win32_regions_reference` with the real MinGW
SDK. Tests compare every pixel around an excluded hole, deletion after region
selection, two independent DC copies, nested intersections and exclusions,
empty and reversed rectangles, invalid handle types, clipped text/lines/blits
and a grid of 49 isolated holes. This validates the contracts used by the
scanner's excluded planet markers and scroll bars without relying on matching
font raster shapes.

The API contract follows Microsoft's [clipping documentation](https://learn.microsoft.com/en-us/windows/win32/gdi/clipping-functions)
and [copied region semantics](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-extselectcliprgn).

## Verification

- SDL Debug and Release: all 15 compatibility tests pass.
- Windows/Wine: all 25 unit tests pass; the expanded region reference also passes.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches, 22 existing warnings, zero differences,
  errors or skipped reference inputs. Baselines and game sources are unchanged.
- Address, undefined-behavior and leak sanitizers pass the region test.

Wine permits GetPixel to read a memory-bitmap pixel outside an application clip.
The native implementation follows Microsoft's [documented rejection](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getpixel);
that particular assertion is native-only. Shared pixel comparisons remove the
clip first, and therefore test drawing results independently of this difference.

The required Wine tutorial run failed at year 2406, page 21, instruction 166
("Now shift click back on Stove Top") after two attempts, with 101 actions.
Artifacts: `dist/scaffold/tutorial/20261010T113148.005664Z`. Earlier runs reached
year 2408 before failing at the ship transfer arrow; the automation still needs
reliable completion. The Windows tutorial executable does not link the local
SDL GDI implementation. No tutorial or game code was changed in this batch.
