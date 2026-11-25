# DIBs and bitmap resources

The SDL compatibility runtime now loads all 38 bitmap resources declared in
`res/stars.rc` and supports the uncompressed DIB formats used by the game.
The Windows build continues to use the real SDK and resource compiler; the
Linux host does not link this runtime or SDL. Game sources, save formats and
regression baselines are unchanged.

## Build and debug in CLion

No new packages are needed beyond [SDL3 development setup](SDL3-DEVELOPMENT.md).
Python 3 generates embedded resources during the normal CMake build. MinGW
is needed only for the Windows build and independent reference tests.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

Reload CMake in CLion with the existing `sdl-debug` profile. Select
`stars-compat-dib` to debug resource conversion and readback contracts, or
`stars-compat-paint` to inspect two independently resizable windows drawing
from the real indexed icon atlas. Run the latter without arguments for an
interactive demo; `--self-test` checks resize/close and survivor repaint.
These are compatibility targets; the full game UI is not runnable yet.

## Resource ownership and generation

`compat/win32/resourcegen.py` reads the canonical bitmap declarations in
`res/stars.rc`, resolves numeric identifiers from `res/resource.h`, validates
the BMP headers, and embeds the DIB payloads without their 14-byte BMP file
headers. CMake depends on the script, resource declarations and bitmap files,
so asset changes regenerate the table. Files outside the bitmap directory,
unsupported declarations, duplicate identities and malformed images fail
the build. No runtime file paths or working-directory assumptions are used.

All current assets have 40-byte `BITMAPINFOHEADER` headers and `BI_RGB`
compression: eight are 1-bit, five 4-bit, and 25 8-bit. The decoder accepts
these indexed formats and 24/32-bit BGR, with DWORD-aligned scanlines,
`biClrUsed` palettes, and positive bottom-up or negative top-down heights.
For 32-bit `BI_RGB`, the fourth byte is unused, not alpha.

| API | Implemented contract |
| --- | --- |
| `GetModuleHandle(NULL)` | Current embedded resource module; named modules are unsupported. |
| `FindResource` | `RT_BITMAP`, numeric IDs and case-insensitive named bitmap identities; null/current module. |
| `SizeofResource`, `LoadResource`, `LockResource` | Immutable module-owned DIB bytes, with exact payload size. Resource handles are not heap allocations. |
| `FreeResource` | Obsolete Win32 no-op returning false; resource bytes stay valid. |
| `LoadBitmap` | Current-module assets only, with no OEM bitmap support. Independently owned GDI bitmap on each load, selectable into a memory DC and released with `DeleteObject`. Indexed colors become RGB surface pixels; 1-bit assets retain monochrome bitmap metadata. |
| `StretchDIBits` | Positive source/destination extents, nearest-neighbor scaling, source cropping, destination clipping and ROP3 drawing into color memory/window DCs. Bottom-up source Y is measured from the bottom of the DIB. |
| `GetDIBits` | Header query and bounded requested scanline ranges, top-down/bottom-up 24/32-bit RGB output, and 1-bit output from monochrome bitmaps. The bitmap must be deselected before the call. |

Decoded resources are bounded by their byte lengths. The common decoder
rejects truncated palettes/pixel rows, invalid palette indices, unsupported
headers and compression, and images above 16,384 pixels per axis or 64M
pixels total. Public Win32 pointer APIs have no buffer-length argument;
callers of `StretchDIBits` and `GetDIBits` must still provide sufficiently
large buffers. A zero-bit-depth header query writes only the header, so it
also works with no palette storage.

`StretchDIBits` now additionally supports palette-indexed `DIB_PAL_COLORS`
input using the selected logical palette; see [palette contracts](WIN32-PALETTES.md).

## Remaining scope

Compressed RLE, bitfields/16-bit pixels, CORE/V4/V5 headers,
mirrored stretches, Win32 reduction/stretch modes beyond
nearest-neighbor sampling, and
`CreateDIBSection` are not implemented. `GetDIBits` does not generate 4/8-bit
indexed output or convert color bitmaps to 1-bit output. `StretchDIBits`
rejects monochrome destinations rather than approximating their color
conversion. True-color DIBs with an additional color table are unsupported.

This resource module handles bitmaps only. Icons/cursors, menus, dialogs and
accelerators now have separate resource implementations; general string resources
remain unfinished.
[Global/local memory APIs are now implemented](WIN32-MEMORY-TIMERS.md). The legacy UI's `GetObject(hbm, 14, &bm)` queries also remain a
separate migration item; the compatibility layer uses native `BITMAP` sizes.

The contracts follow Microsoft's [DIB format documentation](https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps),
[GetDIBits](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getdibits),
[StretchDIBits](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits)
and [LoadBitmap](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadbitmapa).

## Verification

`tests/compat/win32_dib.inc` runs unchanged against both the native runtime and
the real Windows SDK/Wine. The SDK reference independently embeds the real
RC resources, rather than linking the native loader. Checks cover all 38
resource identities and payload sizes, independent bitmap ownership,
all resource pixels exported through 32-bit DIBs, 1/4/8/24/32-bit synthetic
input, palettes, BGR components, row padding/orientation, source cropping,
2x stretching, clipping, ROP drawing, header queries and partial/monochrome
readback. Native-only decoder tests cover malformed and truncated buffers.

Results on October 10, 2026:

- SDL Debug and Release: all eight compatibility tests pass; existing UI
  sources compile against the compatibility headers without new warnings.
- MinGW Debug: all 18 Windows tests pass, including the shared DIB reference.
- Linux host: all 12 unit tests pass.
- X11/Xvfb: DIB/resource contracts and resource-backed two-window painting
  pass, with OpenGL renderers for both windows. This verifies the accelerated
  renderer path, not a physical GPU.
- Address, undefined-behavior and leak sanitizer checks pass for DIB/resource
  contracts with the dummy/software backend.
- Coverage audit: 67 of 178 inventoried Win32 spellings now have local bodies
  (about 38%). Coverage counts basic implementations, not full Win32 parity.
- Full Windows regression through turn 150: 519 matches, 22 existing
  unused-storage warnings, zero differences/errors. No baseline changed.
- `make tutorial` failed after 107 actions at year 2406, page 21, instruction
  166 ("Now shift click back on Stove Top."): the instruction did not advance
  after two attempts. This is not counted as passing. Artifacts:
  `dist/scaffold/tutorial/20261010T100703.532605Z/`.
