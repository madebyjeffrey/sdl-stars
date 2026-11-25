# Win32 drawing state and pattern brushes

This milestone reaches **108/178 inventoried Win32 APIs (60.7%)**. It adds
`GetROP2`, `SetROP2`, `SetWindowOrgEx`, `SetBrushOrgEx`, `SetPixel`, `FrameRect`
and `CreatePatternBrush`. These are local C++20 implementations; the retained
Free API reference remains unmodified and uncompiled.

## Supported behavior

Each DC owns its drawing mode and origins. New DCs start at R2_COPYPEN and
origins (0,0). Setters return the previous mode/origin, invalid ROP2 modes leave
state unchanged, and changing one DC does not change another.

All 16 ROP2 boolean operations apply to line pixels and rectangle pen/brush
pixels. Rectangle interiors and edges are drawn separately, and shared corners
are drawn once. Repeating XOR drawing therefore restores the original pixels,
including the corners. SetPixel also applies the selected ROP2 mode and returns
the mapped input color, as verified against the SDK executable in Wine.
FillRect and FrameRect copy their explicitly supplied brush independently of
ROP2. Blits continue to use their own ROP3 operation.

Window origins map logical point (xOrigin,yOrigin) to device point (0,0).
Translation covers pixels, lines, rectangles, fills/frames, text and its clip
rectangles, rectangle clip operations, bitmap blits and DIB destination output.
BitBlt translates source and destination using their independent DC origins.
DIB source coordinates remain bitmap-relative. MoveToEx retains its position
in logical coordinates. Region handles and brush origins use device units;
changing the window origin does not move an already selected clipping region.
Intermediate arithmetic is promoted before subtraction and surface clipping,
so extreme signed origins do not wrap into an address outside the canvas.

Pattern brushes copy their source bitmap. The application can deselect and
delete that bitmap without invalidating the brush. Color patterns tile their
own pixels; monochrome patterns map zero bits to the destination DC's text
color and one bits to its background color. Brush origins align the tile in
device coordinates, including negative origins. Pattern sampling is shared by
FillRect, FrameRect, Rectangle and pattern-dependent ROP3 blits. The solid-copy
path still fills clipped spans in bulk rather than visiting every pixel.

## Build and debug

No additional package is required beyond the SDL/font dependencies already
listed in [SDL3 development](SDL3-DEVELOPMENT.md). Reload the SDL CMake profile
in CLion and select `stars-compat-drawing-state`, or run:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_drawing_state --output-on-failure
```

The same test source builds as `test_win32_drawing_state_reference` using the
real MinGW SDK. It tests exact ROP2 output for all 16 modes, XOR restoration,
previous-state returns, DC independence, frame edges and line endpoints,
logical/device clipping distinctions, different source/destination origins,
DIB translation, whole-image equality for translated TextOut/DrawText, copied
color/monochrome patterns and negative brush origins. Native-only checks also
cover extreme signed origins and documented clipped-pixel rejection.

## Remaining work

The coordinate mapping is one logical unit per device pixel (MM_TEXT); other
mapping modes, viewport transforms and DPI scaling remain outside this batch.
The subsequent [geometry milestone](WIN32-GEOMETRY.md) adds the dotted mass
path pen, other cosmetic dash styles and solid/null-pen ellipses. Wider pens
and dashed/dotted ellipse outlines remain unsupported. Palettes, system-color brush handles,
window update-region subtraction, child windows, controls and dialogs also
remain. Desktop DCs still provide measurement/capabilities without a drawable
desktop canvas. API-count progress does not prove the full Linux client can
link or run yet.

The origin contracts follow Microsoft's [SetWindowOrgEx](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setwindoworgex)
and [SetBrushOrgEx](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbrushorgex)
documentation. Their different coordinate spaces are tested explicitly.

## Verification

- SDL Debug and Release: all 16 compatibility tests pass.
- Windows/Wine: all 26 unit tests pass, including the shared drawing-state test.
- Linux host: all 12 unit tests pass.
- Full native regression: 519 matches (22 existing warnings), zero differences,
  errors or skipped reference inputs. No game source or baseline changed.
- Address, undefined-behavior and leak sanitizers pass the drawing-state test.

The required Wine tutorial reached year 2408, page 26, instruction 203 and
failed with "No ship transfer arrow is available" after 125 actions. Artifacts:
`dist/scaffold/tutorial/20261010T114254.084319Z`. Reliable tutorial completion
remains outstanding. The Windows tutorial does not link the SDL compatibility
library, and this batch did not change the tutorial automation.
