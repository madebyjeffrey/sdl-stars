# Memory DC and bitmap milestone

The native C11 runtime now supports memory DCs, compatible bitmaps, bitmap
selection and unscaled bitmap transfers. Memory DCs draw through the same
geometry and text APIs as window DCs, without creating SDL windows or
renderers. Their lifetime is independent of the window used as a reference.
No game code, save layouts or regression baselines change.

## Build and debug

No additional libraries are needed. Use the existing SDL dependencies and
reload the CMake project in CLion's **Linux SDL3 compatibility Debug** profile.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
./dist/sdl-debug/bin/stars-compat-paint
./dist/sdl-debug/bin/stars-compat-bitmap
```

`stars-compat-paint` remains the interactive two-window target. It now draws
into a compatible memory bitmap before blitting to each paint DC. Resize a
window, press keys to change its color, and close one while the other remains
usable. `--self-test` checks the same path automatically.

`stars-compat-bitmap` is the focused automated contract test; it exits after
checking pixels and ownership. Its source is also compiled as the Windows
`test_win32_bitmap_reference` target, using the actual SDK and GDI under Wine.
Useful breakpoints are `CreateCompatibleDC`, `SelectObject`, `BitBlt` and
`DeleteDC` in `compat/win32/src/paint.c`.

## Supported behavior

| API | Scope |
| --- | --- |
| `CreateCompatibleDC` | Accepts a valid reference DC or NULL. Creates independent default drawing state and a private 1×1 monochrome default bitmap. It does not copy the reference's selected objects, colors or current position. |
| `CreateCompatibleBitmap` | Creates an owned bitmap in the reference DC's current color format: monochrome for a memory DC containing a monochrome bitmap, otherwise 32-bit color. A zero dimension creates a 1×1 monochrome bitmap; negative dimensions fail. |
| `SelectObject` | Selects a bitmap into a memory DC and returns the previous bitmap. A dynamic bitmap cannot be selected in two DCs simultaneously. Bitmap selection into a window DC is rejected. The runtime's private default bitmap may only be restored to its own DC. |
| `GetObjectA` | Returns native `BITMAP` metadata for a bitmap, or the required structure size when the output pointer is NULL. Requires a complete `BITMAP` buffer; DDB `bmBits` is NULL. Other object types are not supported yet. |
| `DeleteDC` | Releases a memory DC's selected-object references and its private default bitmap. It does not delete the application's selected bitmap. Invalidates the DC token. Window DCs still use `ReleaseDC` or `EndPaint`. |
| `DeleteObject` | Releases the bitmap's heavy storage once deselected. Rejects selected/deleted bitmaps and runtime-owned defaults. Small object tombstones remain until shutdown so stale handles cannot alias new objects. |
| `BitBlt` | Supports memory-to-memory, window-to-memory, memory-to-window and window-to-window transfers. Applies all 256 ROP3 truth tables, including the UI's `SRCCOPY`, `SRCAND`, `SRCPAINT` and `0x00220326`. Uses the selected brush for pattern-dependent operations, now including copied color/monochrome patterns. Clips destination writes and captures source pixels before writing to support overlap. |
| `PatBlt` | Applies source-independent ROP3 operations, including `PATCOPY`, `PATINVERT`, `DSTINVERT`, `BLACKNESS` and `WHITENESS`. Rejects source-dependent operations. |

Color-to-monochrome transfers compare source pixels with the source DC's
background color. Monochrome-to-color transfers use destination text and
background colors. All operations remain on the UI thread. Closing a window
does not invalidate independent memory DCs; `StarsCompatShutdown` releases any
remaining DCs and bitmaps.

## Boundaries

Transfers are unscaled, with positive extents. Logical origins now translate
source and destination independently; see [drawing state](WIN32-DRAWING-STATE.md).
Source coordinates outside bitmap storage are safely clipped; that boundary
is a local policy, not a claim of identical Windows behavior for invalid
source rectangles. Source DC clipping is not applied. Null brushes in
pattern-dependent blits are rejected. Capture/mirroring flags and mapping
modes beyond one-to-one translation remain unsupported.

Bitmaps use RGBA CPU storage; no logical-palette or alpha-blending contract is
implemented. Monochrome drawing uses a simple nearest black/white reduction,
not Windows dithering. Selected-bitmap deletion is deliberately rejected;
Windows drivers may defer deletion in some cases. Private default bitmap
handles are scoped to one memory DC rather than shared system-wide.

Uncompressed DIB conversion, `GetDIBits`, `StretchDIBits` and resource bitmap
loading now work; see [DIB milestone](WIN32-DIBS.md). `CreateBitmap`/
`CreateDIBSection`, `StretchBlt`, logical palettes and arbitrary formats remain
later work. The existing UI still compiles against the headers but cannot
yet link as a native client. In particular, the legacy `GetObject(hbm, 14, ...)`
call in `utilgenui.c` is not a supported full native `BITMAP` query; that DIB
export path needs attention in its own milestone.

## Tests

The shared bitmap test checks initial/zero-size monochrome metadata, color
format inheritance, selection exclusivity, DC state independence, clipping,
all 256 truth tables with all eight binary P/S/D combinations, overlapping
horizontal/vertical copies, color/monochrome conversion, window capture,
paint clipping, deletion order and memory DC survival after window closure.
Linux-only checks cover stale handles, selected-object deletion rejection,
wrong DC release methods, unsupported capture flags and extreme coordinates.

The Wine reference initially differed when a white source pixel was seeded
with `WHITENESS` before monochrome conversion: it converted to black, whereas
a brush-filled white pixel converted to white. The shared conversion test
uses the brush-filled input and checks both the monochrome pixels and their
expansion to color. The ROP3 truth tables are tested separately. This records
the observed Wine difference without adopting it as the runtime's policy.

Sources: [compatible bitmap format rules](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatiblebitmap),
[BitBlt and destination clipping](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt),
and [GetObject metadata](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getobjecta).

## Verification

Results on October 10, 2026:

- Native SDL Debug and Release: all seven compatibility tests pass, including
  bitmap contracts and offscreen-backed two-window painting. Existing UI
  sources compile against the compatibility declarations. Strict compiler
  warnings are enabled; no new warnings appear in touched sources.
- Real X11 under Xvfb: bitmap contracts and two-window resize/close painting
  pass through OpenGL renderers. This verifies the renderer path, not use of
  a physical GPU.
- Address, undefined-behavior and leak sanitizers: both bitmap and painting
  tests pass with the software/dummy backend.
- MinGW Debug builds and all 17 Windows unit tests pass, including the shared
  bitmap reference. All 12 Linux host unit tests pass.
- Full Windows game regression: 519 matches, 22 existing unused-storage
  warnings, zero differences/errors. No game sources or baselines changed.
- Snapshot hashes and coverage generation pass: 59 of the 178 inventoried
  Win32 API spellings have local runtime bodies, about 33%. Coverage still
  means basic implementations, not complete Windows semantics.
- `make tutorial` failed at year 2403, page 13, instruction 101 after 67
  actions. It encountered the dialog: "The production queue does not contain
  the right count of items." This failure has not been diagnosed. Artifacts:
  `dist/scaffold/tutorial/20261010T094717.229646Z/`. The tutorial is not counted
  as passing.
