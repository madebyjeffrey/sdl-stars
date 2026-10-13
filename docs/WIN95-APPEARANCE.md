# Windows 95 appearance on SDL

The compatibility layer uses the supplied Windows 95 screenshot as its visual
reference. Linux retains its desktop-managed title bars, window buttons and
resize frames, as requested. The Windows build continues using its native
controls; these drawing changes apply to the SDL compatibility implementation.

Menus use the classic gray system face and navy selection, white selected text
and glyphs, raised popup frames and etched separators. Toolbars and push buttons
use two-level bevels. Split toolbar buttons use adjacent full-height beveled frames and an independently
pressed menu area, including the displaced triangle; opening a menu leaves the
action/check state unchanged.

Mnemonic underlines leave a pixel below the text baseline, matching ordinary
font underline placement so shortcut markers do not touch the letters.

Edit and list borders, combo fields and static sunken panels use recessed edges.
Combo arrows, scrollbar arrows and thumbs use classic bevels. Checked boxes draw
ticks instead of solid squares. Radio selections retain round dots. Existing
Stars panel frames already use its classic system highlight and shadow brushes.
The combo arrow button sits inside the two-pixel sunken field frame, preserving
its complete raised edge. Opening the list depresses that button and offsets its
triangle; closing restores the raised appearance.
The shared internal `CompatDrawClassicEdge` painter keeps edge colors consistent
without adding a new public Win32 API or changing control geometry and hit tests.

Existing bundled fonts, application icons, save formats and game rules are
preserved. This is a classic appearance pass rather than a pixel-exact emulation
of Windows 95's desktop, bitmap fonts or every common-control theme detail.
No additional libraries are needed. Rebuild and debug `stars-sdl` with the
existing `sdl-debug` CLion profile.

Original inventoried coverage remains **178/178 (100% basic API coverage)**;
appearance refinements do not increase that count or imply full Win32 conformance.

The complete fatal ASan/UBSan/LSan suite passes **96/96** tests in sequential
private virtual displays. Pixel checks cover menu colors and outer bevels,
toolbar edge layers, the adjacent split frames and independent menu press state.
Captured game and popup surfaces were inspected visually. SDL Debug/Release,
MinGW Debug and Linux host Debug builds pass without new warnings in this batch.

The reserved square where horizontal and vertical scrollbars meet uses
`COLOR_BTNFACE`, including nested windows. It is painted during composition
because application client painting excludes the nonclient scrollbar strips.
