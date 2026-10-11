# Original WinHelp content and Linux viewer work

Basic API coverage is **178/178 (100%)**. `WinHelpA` has a local implementation
and automated adapter tests. The user confirmed that the real offline viewer
opens in the desktop browser. Offline popup, font and diagram behavior has not
yet been confirmed manually; HTTP browser checks cover those renderer paths.
This percentage does not mean complete Win32 semantics or a qualified full client.

The adapter supports the application's `HELP_INDEX` and `HELP_CONTEXT`
requests, following the [WinHelp contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-winhelpa).
It validates the original archive size and CRC, selects a generated context
page, checks all packaged assets and the generation stamp, then sends an
escaped `file://` URL to SDL's desktop launcher. Context 13002 is absent and
returns an error. Unsupported commands, corrupt or missing files, and launch
failures also return errors. Desktop acceptance does not prove page rendering.

The build places `stars!.hlp` beside the executable and the HTML guide in
`../help`. Explicit HLP paths are checked directly; a plain filename can use
the executable-adjacent copy. Zlib supplies CRC checking (`zlib1g-dev`).
The automated tests capture launches, covering all 413 numeric mappings,
contents, Unicode paths, corrupt archives, missing assets and launch failures.

To launch the guide manually, build the SDL preset and run:

```sh
dist/sdl-debug/bin/stars-compat-help-viewer --contents
dist/sdl-debug/bin/stars-compat-help-viewer 4501
```

The same `stars-compat-help-viewer` target is available in CLion. Verify the
contents diagram links, a popup, fonts and images without an HTTP server.

`compat/win32/hlp.py` independently reads the archive header, internal stream
bounds, B+ tree directory and title index, system records and numeric context
map. It now also reads the complete original topic-record chain, keeping
character formatting and text separate for the renderer. It uses explicit little-endian widths, checks truncated structures,
rejects cyclic page chains and verifies entry counts. No Wine installation,
third-party Python package or downloaded decompiler is required.

Inspect the bundled file and run the checks:

```sh
python3 compat/win32/hlp.py 'res/stars!.hlp' > /tmp/stars-help-metadata.json
python3 compat/win32/hlp.py 'res/stars!.hlp' --topics > /tmp/stars-help-topics.json
python3 tests/compat/test_hlp.py
ctest --test-dir dist/sdl-debug -R win32_help_archive --output-on-failure
```

The original file contains:

| Item | Observed value |
| --- | --- |
| Title | Stars! Player's Guide, v2.6b |
| Format | HC31 / Windows 3.1, minor 21 |
| Topic-block compression flags | 0 (uncompressed) |
| Declared archive size | 1,662,571 bytes |
| Trailing bytes outside the archive | 72 |
| Internal streams | 142 |
| Image streams | 133 |
| Indexed topic titles | 414 |
| Numeric context mappings | 413 |
| Contents topic offset | 0 |

The declared archive's SHA-256 is
`43d99794da2d12e67ddbeefb3ec81b8c24309f40982b2aa7daa8e4dc5c669309`.
The reader preserves the original bytes and offsets; it does not modify the
help file or treat its trailing bytes as archive content.

Context 4501 resolves to “Welcome to Stars!” and 1070 to “Research Dialog”.
The `mdi.c` menu path also requests context **13002**, which is absent from
the bundled context map. A viewer must report that missing topic or offer an
explicit contents fallback; it must not claim to have opened that context.
Five context mappings have offsets without an exact title-index key. The
decoder now resolves them through the character ranges of the original
display records, preserving the owning topic when it spans multiple blocks:

| Context | Resolved original topic |
| --- | --- |
| 1514 | Location Tile |
| 3004 | Population |
| 3130 | Fleet Reports on Enemies and other Players |
| 5514 | Load from Fleet |
| 1017 | Slower Tech Advances |

All **413 context mappings** resolve. All **414 topic headers** agree with the
original title index. The decoded chain contains **4,200 records**: 414 topic
headers, 3,638 display-text records and 148 table records, with 415,602 bytes of
text-stream data including NUL delimiters and title/macro data. That byte count
does not describe fully formatted text or rendered tables.

The original HC31 file uses 16 KiB virtual position slots even with uncompressed
4 KiB physical blocks. The decoder strips each physical block's 12-byte header
when a record spans blocks. It rejects bad links, cycles, invalid record sizes,
unsupported record types and disagreement with the title index. The current
scope is this uncompressed HC31 format; LZ77/phrase-compressed help archives
are explicitly rejected. The original `res/stars!.hlp` uses neither.

`compat/win32/hlp_format.py` now decodes all **3,786 display/table records** into
**4,270 paragraph/cell groups**. It preserves paragraph spacing, indentation,
alignment, borders and tab stops, table column data, font changes, line/paragraph
breaks and separate text runs. It requires both binary streams to be consumed
completely, rather than dropping commands or inventing spaces between runs.
All original text runs reconstruct their source text stream byte for byte.

The original file's **1,498 text links** (1,094 topic jumps and 404 popups) resolve
through the 413 named-context hashes to existing topics. Its **140 image
placements** reference all 133 image streams. These checks establish the source
graph; they do not mean the images or tables have been rendered. Font-specific
character decoding remains separate from the byte-level format parser.

`compat/win32/hlp_font.py` decodes all **32 font descriptors**, retaining size,
weight, italics, decoration and color. Font selections carry across records and
apply after the preceding text run. All **20,793 runs** now have Unicode text;
the guide's Symbol bytes become 256 diamonds and 116 rightward double arrows.
CSS styles use the bundled Liberation Sans family for Arial/Helv and these
Symbol glyphs.

`compat/win32/hlp_image.py` decodes all **125 DIB bitmaps** and **eight WMFs**.
It checks palette indices, row padding, bottom-up storage, offsets, dimensions,
RLE input/output bounds and exact decompressed size. All original images use
RLE; other compression, multi-resolution variants, transparent bitmaps and
unsupported picture formats are explicitly rejected. Four bitmap figures have
**28 image hotspots**; every target resolves to an original topic.

Bitmap output is lossless PNG generated with Python's standard library. The
eight WMFs are converted to SVG with `wmf2svg` from `libwmf-bin`:

```sh
sudo apt install libwmf-bin
```

This is a build/conversion dependency. The converter receives argument vectors
without a shell and has a checked exit status and a timeout. Its output embeds
PNG data rather than referring to external image files. The normalization adds
missing SVG/xlink namespaces and corrects libwmf's duplicate image scaling;
the final viewport retains the WMF's HIMETRIC dimensions at 96 dpi.

The pixel references in `tests/compat/help_bitmap_reference.json` were obtained
independently from helpdeco's 121 exported BMPs and splitmrb's four hotspot
bitmap exports (reference commit
`785273788af17fde4fba7df7162d547f8645d7d8`), decoded to RGB with Pillow. All
125 match the local decoder byte for byte. The reference splitmrb was built
only in `/tmp`, with its on-disk `long` fields fixed to 32 bits, 16-bit I/O
replacing the host libc's 32-bit `getw`/`putw`, and packed bitmap headers.
Its decompression and palette logic were unchanged. No reference tool or
Pillow installation is needed to run these
fixture checks. The eight converted SVGs are checked for embedded images and
single scaling, and an icon was visually checked through librsvg.

`compat/win32/hlp_render.py` now generates **414 HTML topic pages**, a filterable
topic index and a manifest for **413 numeric contexts**. The renderer preserves
every decoded Unicode text run, all 1,498 text links (including 404 popups),
140 image placements, 28 image hotspots and 148 tables. Fonts, paragraph
alignment/spacing/indentation, borders, line breaks and left tab stops are
rendered. Text links retain font/color changes; popups open in a keyboard
dismissible dialog while the parent topic stays open. Browser Back navigation,
the original clickable contents figure and topic filtering work.

The generated directory contains its own CSS, JavaScript, images and four
Liberation Sans faces with their license. It needs no network content or
Python server at runtime. The current renderer supports the original file's
fixed-width tables and left-aligned tabs; unsupported commands and alternative
table/tab formats fail explicitly. Border strokes follow the thin/thick/double
flags rather than interpreting the compiler's packed border-width property as
a CSS line thickness. This is not a claim of pixel-identical WinHelp layout.

Both SDL build presets now include the **`stars-help-guide` CMake target**:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
# Build just the guide, also available as a CLion CMake target:
cmake --build --preset sdl-debug --target stars-help-guide
# Independent generation without configuring SDL:
python3 compat/win32/hlp_render.py 'res/stars!.hlp' /tmp/stars-help-guide
```

Open `dist/sdl-debug/help/index.html` (or the equivalent Release path) in a
browser. `manifest.json` maps context IDs to generated topic files. Context
13002 remains absent; no replacement topic is invented. Browser verification
under local HTTP covers contents-image navigation, two-column layout, a real
popup and its close button, Back, topic filtering and hanging-indent bullets.
The browser tool disallows `file://` URLs. The user independently confirmed
that the manual viewer opens in the desktop browser. Detailed offline content
checks, packaging/integration and full-client qualification remain separate
acceptance gates. The adapter supports `HELP_INDEX` and `HELP_CONTEXT`; it is
now counted in basic API coverage.

Format reference: Manfred Winterhoff's
[Windows Help File Format](https://github.com/pmachapman/helpdeco/blob/master/helpfile.txt).
The reader was written locally from the format description. No helpdeco or Wine
source has been incorporated into the wrapper.

## Verification of the rendering batch

The rendering batch passes **37/37 help checks**, including exact Unicode
text preservation across all 414 topics, all 1,498 logical text hotspots,
404 popups, 140 image placements, 28 image hotspots and 148 tables. Both SDL
configurations pass **67/67**; the changed help suite passed again after the tab
origin correction. Both build presets generate the guide through
`stars-help-guide`. MinGW builds successfully and the Windows unit suite passes
**49/49**. The native regression has **519 matches**, 22 existing warnings,
zero differences/errors and no skipped reference inputs.

The fresh rendering-batch tutorial **fails** at year 2402, page 7, instruction
55, after 22 actions: Stars exited unexpectedly and Wine logged
`free(): invalid next size (fast)`. Artifacts:
`dist/scaffold/tutorial/20261011T025739.569520Z`. This preserves evidence of the
intermittent failure despite the preceding complete passing run. The rendering
tooling does not modify the Windows game runtime or its regression baseline.

## Verification of the font and image batch

- Help archive/font/image checks: **32/32**, including all 125 independent
  bitmap pixel references, eight WMF conversions, 28 hotspot targets, font
  selection timing, Unicode Symbol glyphs and malformed-input cases.
- SDL Debug and Release: **67/67 each**. The changed help checks also passed
  again in both configurations after the SVG scaling correction.
- MinGW build: passed. Windows unit tests: **49/49**.
- Native Windows regression: **519 matches**, 22 existing warnings,
  zero differences/errors and no skipped reference inputs.
- Tutorial: **passed in this run**, all 80 pages through year 2437,
  346 actions. Artifact directory:
  `dist/scaffold/tutorial/20261011T024603.973906Z`.

The font/image Python tooling does not change Windows game code. This passing
tutorial run does not establish that the earlier intermittent Wine crashes
have been fixed. No game behavior, save format or regression baseline changed.

## Verification of the archive-reader batch

- Original archive-reader batch: 9/9 checks, including metadata and malformed-input cases.
- SDL Debug and Release: 67/67 each (55 compatibility and 12 host tests).
- MinGW build and Windows unit tests: build passed, 49/49 tests passed.
- Native Windows regression: 519 matches, 22 existing warnings, zero differences.
- Tutorial: failed at year 2411, page 34, instruction 269, after 137 actions.
  Stars exited unexpectedly; Wine logged `free(): invalid next size (fast)`.
  This is not a passing tutorial gate and the allocator message alone does not
  establish the underlying cause. Artifacts are in
  `dist/scaffold/tutorial/20261011T021448.519371Z`.

Both SDL presets now build `stars-host` and the `stars-host-unit-tests` aggregate
alongside the compatibility executables. This fixes a fresh Release build that
previously registered host tests without building their executables.

The topic-decoder batch passes **16/16 reader checks**, including exact title
agreement, all context resolutions, a record crossing physical blocks, damaged
links, unsupported compression and title-index disagreement. Debug and Release
again pass 67/67 tests, Windows passes 49/49, and the regression again has 519
matches with 22 existing warnings and zero differences. The tutorial still fails:
year 2406, page 22, instruction 173, after 86 actions. The observer timed out
and Wine logged `free(): invalid next size (fast)`. Its artifacts are in
`dist/scaffold/tutorial/20261011T022236.307992Z`.

The formatter checks now total **24**, covering all original runs, command counts,
link and image references, packed values, truncation, trailing data and unsupported
commands. Debug and Release pass 67/67 tests; the native Windows regression has
519 matches, 22 existing warnings and zero differences. The first Windows unit
run passed 48/49: the caption-attention reference test failed its peer-focus
assertion. That test passed individually under Xvfb, and the complete isolated
Windows suite then passed 49/49. Preserve the initial failure when assessing
desktop-focus stability; no test assertion was removed or relaxed.

The formatter-batch tutorial failed at year 2402, page 7, instruction 52, after
19 actions. Stars exited unexpectedly and Wine logged an assertion in
`dlls/win32u/sysparams.c:439: user_check_not_lock`. Artifacts are in
`dist/scaffold/tutorial/20261011T023022.795562Z`. The tutorial gate is still open.

## Verification of the WinHelp adapter batch

Debug and Release each pass **68/68 CTests** (56 compatibility checks and
12 host checks), including all 413 context launches and 37 archive/rendering
checks. MinGW builds without new warnings. The native regression has
**519 matches, 22 existing warnings, zero differences/errors and no skipped
reference inputs**. The user confirmed that `stars-compat-help-viewer` opens
in the desktop browser.

The first adapter test exceeded its 30-second limit with SDL's bitwise CRC
routine. Zlib CRC checking reduced the full context test to about 0.5 seconds;
the timeout and assertions were retained. The initial Debug suite had 15
loader failures (`file too short`/`invalid ELF header`) while its SDL shared
library was being replaced. A complete rerun after the library settled passes
68/68; the initial failure log is retained in
`/tmp/stars-help-adapter-debug-tests.log`.

The Windows unit suite passes **49/49** (271.97 seconds). A fresh relocatable
link probe combining the core, UI objects and compatibility library has no
unresolved names from the 178-API inventory. Legacy `ExitWindowsEx` still
requires a platform decision, and the Linux entry point/full executable are
not supplied by that probe.

The adapter-batch tutorial **fails** at year 2407, page 24, instruction 186,
after 94 actions. The observer times out in `FilterMessage`/`ReadState`; Wine
logs `free(): invalid next size (fast)`. Artifacts are in
`dist/scaffold/tutorial/20261011T031808.268260Z`. This remains an open gate,
and neither the allocator message nor this help change establishes its cause.
No game behavior or native baseline was changed.
