# Win32 painting milestone

The local C11 runtime now paints two independent SDL windows through Win32
APIs. Each top-level window owns an RGBA canvas, SDL renderer and streaming texture.
[Logical children](WIN32-CHILDREN.md) own canvases and are composited into their
root window for presentation.
GDI drawing runs on the CPU; SDL uploads and presents the canvas through its
selected renderer. This provides accelerated presentation when available,
with a software fallback. The shared Stars! desktop uses this same path.

Presentation waits for active paints and pending visible pane paints to finish,
so icon/DC operations cannot expose intermediate frames. Deferred drawing that
is validated without `WM_PAINT` flushes when the message queue reaches idle.
New top-level windows receive their show notification with logical visibility
set while the native window remains hidden. Their show-time layout and visible
child tree are painted before native show; the completed frame is submitted
again immediately afterward for renderers that acquire a new native drawable.
The paint-batch check covers controls created by the show notification, and
the real-client popup check compares the starbase card's first canvas with a
subsequent full repaint.
Creation explicitly hides each new native window after SDL setup (except a
reused wizard window). The native Cocoa test found an owned tooltip visible
before wrapper show despite requesting `SDL_WINDOW_HIDDEN`. macOS compositor
popups now have a clear backing until their opaque CPU canvas arrives; renderer
clears preserve that initial transparency. The real-client test checks native
`NSWindow` visibility/background before rendering and the opaque yellow first
frame of a `starstt` shown through `SetWindowPos`, matching the scanner combobox.
Native exposure immediately resubmits the last completed texture, including
inside window/menu dispatch or with pending invalidation. Cocoa's view can
clear to black when exposed after an initial presentation. This refresh performs
no application paint or texture upload; pending work still follows the normal
paint path. A presentation guard prevents recursive native refresh. The macOS
wizard handoff also refreshes while the next logical page is hidden, provided
its reused native window is visible and the previous completed frame is retained.
The macOS paint-batch check counts actual SDL presentations to verify that exposure inside
a window procedure refreshes immediately without painting an invalidated child.
SDL's live-resize expose events are handled by a main-thread event watcher:
it sends the current client size, batches layout/painting and presents the new
frame during the native tracking loop. Dispatch and resize guards prevent
reentrant painting. The watcher is removed during runtime shutdown.

## Build and debug in CLion

No new libraries or font packages are required. The existing SDL3 dependency
and [desktop development packages](SDL3-DEVELOPMENT.md#dependencies) suffice.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
./dist/sdl-debug/bin/stars-compat-paint
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

Reload CMake in CLion, enable the imported **Linux SDL3 compatibility Debug**
profile, and select **stars-compat-paint** as the run/debug target. Leave program
arguments empty for two interactive windows. Resize either window, press a key
to change its color, and close one while the other remains usable. Use
`--self-test` for the automated checks. Useful breakpoints are `PaintWndProc`,
`BeginPaint`, `TextOutA` and `EndPaint`.

`STARS_SDL_RENDERER=auto` (the default) lets SDL choose the renderer and retries
software if renderer creation fails. Set `STARS_SDL_RENDERER=software` in the
CLion run environment for deterministic software presentation. The older SDL
GPU smoke executable also accepts `gpu`; the GDI executable does not.
Renderer names identify the SDL backend, not whether the driver uses physical
GPU hardware. Under Xvfb, OpenGL can be Mesa software rendering.

```sh
SDL_VIDEODRIVER=dummy STARS_SDL_RENDERER=software \
  ./dist/sdl-debug/bin/stars-compat-paint --self-test
xvfb-run -a env SDL_VIDEODRIVER=x11 STARS_SDL_RENDERER=auto \
  ./dist/sdl-debug/bin/stars-compat-paint --self-test
```

## Implemented contracts

| Area | APIs and scope |
| --- | --- |
| Painting | `InvalidateRect`, `GetUpdateRect`, `ValidateRect`, `UpdateWindow`, `BeginPaint`, `EndPaint`. Pending paint is generated after ordinary queued messages; `UpdateWindow` dispatches synchronously. `BeginPaint` validates the captured update rectangle and clips drawing to it. A new invalidation inside the callback remains pending for the next paint. |
| Device contexts | `GetDC` / `ReleaseDC`, with independent default state per acquired DC. `EndPaint` and `ReleaseDC` upload/present. Window destruction invalidates its DCs and releases the renderer, texture and canvas. Frequent DC allocations are freed on release; monotonic opaque tokens reject stale handles. |
| Drawing objects | `CreatePen` for cosmetic solid/dash/dot width 0/1 and null pens, `CreateSolidBrush`, `GetStockObject`, `SelectObject`, `DeleteObject`. Selected objects cannot be deleted; stock objects remain owned by the runtime. Deleted dynamic object tokens remain until shutdown to prevent handle reuse. |
| Geometry | `MoveToEx`, `LineTo`, `Rectangle`, `FillRect`. Lines exclude their final endpoint; rectangles exclude right/bottom edges. Client clipping bounds work even for extreme signed coordinates. `GetPixel` supports verification of the canvas. |
| Text | `TextOutA`, `GetTextExtentPoint32A`, `GetTextMetricsA`, `SetTextColor`, `SetBkColor`, `SetBkMode`. The initial 8x8 font has been replaced by selectable proportional CP1252 fonts with opaque/transparent backgrounds; see [fonts/text](WIN32-FONTS.md). Drawing and measurement use the same face. |
| Window integration | Showing, SDL exposure and resize events invalidate the window; `MoveWindow(..., TRUE)` repaints synchronously. `DefWindowProc` validates unhandled paint messages and erases through a registered background brush. Each window retains its own canvas when another closes. |

All APIs run on the UI thread. Coordinates use client logical pixels; the
canvas scales to SDL's output size. The backing canvas retains unaffected
pixels between paints and preserves overlapping pixels on resize.

On Cocoa, backing canvases and textures instead match the display's integral
pixel density. Window coordinates, update regions and layout remain logical;
ordinary raster operations replicate logical pixels while text renders at the
physical density. Child composition, menu-bar caching, cursor overlays, blits
and scrolling preserve that density. Density changes invalidate the visible
tree for repaint. Other backends keep density one. See [fonts/text](WIN32-FONTS.md).

The original, now-uncompiled text glyph reference in `compat/win32/src/font8x8.h` is an explicitly marked
ASCII-only adaptation of SDL 3.4.18's `src/render/SDL_render_debug_font.h`.
Its original SDL license notice and public-domain VGA font attribution are
retained. No Free API source is compiled for this milestone: its snapshot has
no needed paint implementation, and its `UpdateWindow` only raises a window.

## Limits and next work

This is a basic painting subset, not complete GDI. The subsequent
[content-scrolling milestone](WIN32-SCROLL-WINDOW.md) adds exact window update
regions, disjoint painting and partial `ValidateRect` subtraction.
System-color brush pseudo-handles, non-client painting and DPI-aware Windows
scaling are not implemented. Copied application clipping
regions, intersections and excluded holes are now supported; see
[the region report](WIN32-REGIONS.md).

Cosmetic dash/dot pens and solid/null-pen ellipses now work; see
[geometry](WIN32-GEOMETRY.md). Wider pens remain unsupported. Brushes now support copied color
and monochrome patterns, and drawing state includes logical origins and all
ROP2 modes; see [drawing state](WIN32-DRAWING-STATE.md). The subsequent [bitmap milestone](WIN32-BITMAPS.md) adds memory DCs, bitmap
selection and ROP3 blits. There are still no palettes,
viewport transforms or `SaveDC`/`RestoreDC`. Selectable fonts, CP1252 text,
multiline layout, `DrawText` and `ExtTextOut` are now implemented in the
[font milestone](WIN32-FONTS.md). Desktop DCs support measurement/capabilities
but do not draw to the desktop. Unsupported text
and pen styles fail rather than silently claiming equivalent behavior.

Memory DCs, bitmap blitting, DIB conversion and bitmap resource loading are
now implemented; see [DIB milestone](WIN32-DIBS.md). The next useful work is
font selection/layout and other resource types. Child windows, dialogs, controls and menus
remain separate milestones. The existing UI compilation target continues to
check declarations but cannot yet link a Linux game client.

## Verification

`stars-compat-paint --self-test` checks pixel colors, independent DC/object
state, selected-object deletion, stale handles, exclusive geometry edges,
extreme coordinates, font measurement and actual glyph pixels, paint clipping,
message ordering, invalidation during painting, resize/exposure and survivor
input/repainting after one window closes.

`tests/compat/win32_gdi_reference.c` exercises the same basic geometry, DC
state and painting contracts against our runtime and actual Win32 under Wine.
Font tests check valid measurement and drawing without asserting identical
Windows font pixels or metrics. Native target: `stars-compat-gdi-reference`;
Windows target: `test_win32_gdi_reference`.

Sources: [Microsoft painting lifecycle](https://learn.microsoft.com/en-us/windows/win32/learnwin32/painting-the-window),
[WM_PAINT](https://learn.microsoft.com/en-us/windows/win32/gdi/wm-paint),
and [SDL rendering API](https://wiki.libsdl.org/SDL3/CategoryRender).

### Results on October 10, 2026

- Debug and Release: all six compatibility tests pass, including the full UI
  declaration compilation target. New compatibility sources build with strict
  warnings treated as errors.
- Native X11/Xvfb: the two-window self-test passes with separate OpenGL
  renderers. This checks the accelerated renderer path, not physical GPU use.
- Address, undefined-behavior and leak sanitizers: software self-test passes,
  including destruction inside `WM_ERASEBKGND`.
- MinGW Debug builds; all 16 Windows unit tests pass, including the shared
  GDI reference with dirty-rectangle background erasure. All 12 host tests pass.
- Full Windows regression: 519 matches, 22 existing unused-storage warnings,
  zero differences/errors. Game sources and frozen baselines are unchanged.
- Snapshot checksum verification and updated coverage generation pass:
  53 of the 178 inventoried Win32 API spellings now have local runtime bodies.
- Tutorial **fails**. The first run exited at year 2403, page 16, instruction
  127 after 80 actions; an isolated retry exited at year 2400, page 5,
  instruction 37 after 15 actions. Both Wine logs contain
  `free(): invalid next size (fast)`. This differs from the older year-2406
  tutorial stall and has not been diagnosed. Artifacts:
  `dist/scaffold/tutorial/20261010T093315.963374Z/` and
  `dist/scaffold/tutorial/20261010T093518.760628Z/`.
