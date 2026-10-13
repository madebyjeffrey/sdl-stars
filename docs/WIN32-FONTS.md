# Fonts and text layout

Coverage is **97/178 inventoried Win32 APIs (54.5%)**, plus six CRT helpers
counted separately. This batch adds `CreateFontIndirect`, `DrawText`,
`ExtTextOut`, `GetBkColor` and `GetDeviceCaps`, and replaces the fixed ASCII
renderer behind the existing text measurement/drawing APIs.

## Dependencies and CLion

FreeType and Fontconfig development packages are required by the SDL presets.
Both are already installed on this laptop. On another Ubuntu/Pop!_OS machine:

```sh
sudo apt install -y pkg-config libfreetype-dev libfontconfig1-dev
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

Reload CLion's SDL Debug profile and select `stars-compat-font` to debug the
shared font test. `stars-compat-paint` now displays proportional text in the
existing interactive two-window demo. Windows and host presets do not require
these font libraries.

## Selection and ownership

Twelve unmodified Liberation Sans, Serif and Mono faces are embedded from
`res/fonts/`, each with regular, bold, italic and bold-italic variants.
`ORIGIN.json` records the package version, byte lengths and SHA-256 hashes.
`LICENSE` retains the packaged copyright and SIL Open Font License notice.
`fontgen.py` validates checksums and table order before generating C data.
No runtime font path is needed for these faces.
The SDL configure copies the notice and provenance to `bin/LICENSE.fonts`
and `bin/FONTS-ORIGIN.json`; retain both when packaging these executables.

Arial, Tahoma, MS Sans Serif and sans-serif map to Liberation Sans;
Times New Roman/serif map to Liberation Serif; Courier New/monospace map to
Liberation Mono. Liberation family names also map directly. The game's styled
Arial names select the appropriate faces even with zero LOGFONT style flags.
Fontconfig resolves other requested family names.

Fonts retain their logical `LOGFONT` for `GetObject`, have independent cached
glyphs and can be selected into multiple DCs. Selection counts protect owned
objects from premature deletion. Stock fonts remain runtime-owned. Deletion
and shutdown release raster buffers, faces and the FreeType library.

## Measurement and rendering

Negative heights select character/em sizes; positive heights select approximate
cell sizes. Width requests scale glyphs horizontally. Metrics and advances come
from the same face and size used for rasterization. ANSI bytes decode as CP1252,
including accented letters, euro and typographic punctuation. Original strings
and save data retain their bytes. Coverage blends into the CPU canvas before
SDL presentation. Escapement rotates glyph placement and opaque backgrounds;
the game's 3150-tenths-degree labels are tested. Underline/strikeout use font
metrics.

On Cocoa Retina displays, window canvases use the integral physical/logical
pixel ratio (up to four). A cached larger FreeType raster supplies physical
glyph coverage, while the original logical advances and ascent position each
character. Measurements, dialog units and control bounds therefore stay the
same. Menu-bar bitmaps use an internal scaled DC so their text is also sharp.
Clipping regions and origins remain logical; glyph blending uses physical pixels.
Windows, Linux and ordinary-density Cocoa displays keep the original scale-one
path. Font objects free their cached larger raster with the logical face.
`win32_sdl_retina` checks glyph detail within logical pixel cells, unchanged
metrics, custom advances, clipping and preservation through blits/inversion.

`ExtTextOut` supports rectangle clipping, independent opaque fills and custom
character advances. Empty text with `ETO_OPAQUE` still fills its rectangle.
`DrawText` supports CR/LF lines, word wrapping, `DT_CALCRECT`, clipping,
horizontal alignment, single-line vertical alignment, tab expansion and
ampersand prefixes/underlining. Measurement does not paint.

Desktop DCs support capability queries, font selection and measurement for
initialization and tooltip sizing. Desktop drawing/pixel reads fail safely.
Logical coordinates use 96 DPI, one plane and a 32-bit display device; bitmap
metadata remains separate from device capabilities. Resolution queries use
SDL's current display mode when video has been initialized.

## Limits

Liberation is an explicit substitute for Windows fonts; metrics, antialiasing
and rotated rasters need not match Windows pixel-for-pixel. Height/width are
bounded at 512 pixels and text at one million bytes. Advanced independent
orientation/world transforms, Unicode entry points, shaping/RTL, charset
selection beyond CP1252, glyph-index output, DrawText ellipsis/edit-control
extensions and printer capabilities remain outside this batch. Unsupported
text flags fail rather than report successful rendering.

Child windows/controls, dialogs, menus, other resources and remaining APIs are
still required before `stars-sdl` runs. No game source, disk layout or baseline
changed.

## Verification

Verified October 10, 2026:

- Debug/Release: all 14 native compatibility tests pass.
- Windows: all 24 unit tests pass, including the shared font test under Wine.
- Linux host: all 12 unit tests pass.
- Fonts pass address, undefined-behavior and leak sanitizers.
- Full regression: 519 matches, 22 existing warnings, zero differences/errors
  and no skipped reference inputs.
- Tutorial: the year-2408, page-26, instruction-203 transfer automation failure
  persists. Artifacts: `dist/scaffold/tutorial/20261010T111235.609626Z`.

The shared test checks metadata, proportional advances, scaling, styles, ANSI,
clipping/fills, spacing, wrapped heights, prefixes, newlines and rotation. It
compares contracts/layout relationships rather than identical glyph bitmaps.
The paint test checks actual metrics and blended ink instead of fixed 8x8 pixels.

For a native snapshot of the tested surface:

```sh
SDL_VIDEODRIVER=dummy STARS_SDL_RENDERER=software \
  STARS_FONT_SNAPSHOT=/tmp/stars-font.bmp \
  ./dist/sdl-debug/bin/stars-compat-font
```

The snapshot was visually inspected for clipping, accented/euro punctuation,
word-wrapped lines, prefix underlining and rotation. This is optional test output.

Reference contracts: Microsoft's [logical fonts](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfontindirecta)
and the [Fontconfig reference](https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-devel/).
