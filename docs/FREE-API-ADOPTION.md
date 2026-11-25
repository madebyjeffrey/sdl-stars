# Free API adoption for Stars

Source: [openeggbert/free-api](https://github.com/openeggbert/free-api),
commit `8337b2499ca28423623f624d4351992ca3f2a22f` (develop), imported October 10,
2026. The source is MIT licensed, copyright Robert Vokac; the notice is
retained in `compat/win32/LICENSE.free-api` and the upstream reference copy.

## What is in this repository

`compat/win32/upstream/free-api/` holds unmodified public headers, runtime
source, and scope documentation from that commit. `ORIGIN.json` records the
repository, commit and SHA-256 of every copied file. This is a source reference,
not a standalone build: upstream tests, CMake helpers and external music
libraries are not included. Nothing in that directory is compiled or added
to any include search path.

The working implementation is our local C++20 library, `stars_win32`, under
`compat/win32/src/`. It uses our existing ABI-checked Win32 headers and SDL3.
This first adaptation contains:

| Functions | Origin and local changes |
| --- | --- |
| `SetRect`, `IntersectRect` | Adapted from upstream `include/winuser.h`; SDK signatures retained. `IntersectRect` computes into a temporary for source/destination aliasing and clears empty results to match Win32. |
| `CopyRect`, `EqualRect`, `OffsetRect`, `InflateRect`, `PtInRect` | Local extensions completing Stars' seven inventoried rectangle operations. Point inclusion uses left/top inclusive and right/bottom exclusive edges. |
| `GetTickCount` | Adapted from the monotonic millisecond conversion in upstream `src/winbase.cpp`, using SDL's monotonic clock and 32-bit truncation rather than C++ chrono. |

The clock currently has SDL's process-relative epoch, rather than Windows'
system-uptime epoch. Elapsed-time use and rollover conversion are supported;
the core's `DwTickCount` remains unchanged.

Game code is unchanged. The shared UI now links into the native `stars-sdl`
preview; `stars-ui-compat-check` remains a separate compile check. See
[client status](WIN32-CLIENT.md). Only SDL compatibility consumers see
`stars_win32_headers`, including the Windows SDL client. Independent Windows
SDK references use the actual SDK, and `stars-host` keeps its SDL-free native
implementation. The application and wrapper now use C++20;
SDL3_image/SDL3_mixer remain unnecessary. See the [migration plan](CPP-MIGRATION.md).
The unmodified C++ source reference is kept as attribution and material for subsequent adaptations.

The subsequent [window and message milestone](WIN32-WINDOWS.md) adapts
top-level window creation and dispatch and adds per-window state, filters
and safe independent teardown. The current CSV records local definition
locations and tested subsets; the table below reflects both milestones.

## Inventory comparison

[FREE-API-COVERAGE.csv](FREE-API-COVERAGE.csv) contains one row for each of
our 189 source spellings: 178 Win32 functions, five legacy macros and six
CRT functions. Each row records the call count, matching upstream definition
and source line, upstream header status where annotated, local decision and
local definition location. A local body may implement only the documented subset.

| Decision | Source spellings |
| --- | ---: |
| Adapted into the local C++20 runtime | 17 |
| Implemented as local C++20 extensions | 167 |
| Legacy macro supplied by our headers; no separate implementation | 4 |
| Platform difference: reject desktop logoff and exit Stars only | 1 |

There are 46 upstream definition matches. This is a lexical source comparison,
not a claim that 46 APIs meet Stars' needs. Definitions include partial behavior
and stubs. The script follows narrow-character aliases and looks for bodies in
runtime source and inline headers; it does not preprocess every conditional
branch or prove signatures and behavior. Missing definitions may still have
header declarations. Header annotations are upstream's own assessments and
are not treated as acceptance criteria.

Regenerate and verify the original snapshot hashes with:

```sh
python3 compat/win32/audit_free_api.py
```

The local runtime currently qualifies **178 of 178 inventoried
Win32 API spellings (100%)**, plus all six inventoried CRT helpers
and APIs outside that original inventory. The decision table includes CRT
helpers; the Win32 percentage excludes them and the five legacy macros.
These counts mean tested basic implementations, not complete Win32 semantics.
The audit recognizes local API definitions automatically and now finds 178/178.
That lexical count includes the new WinHelp adapter; the user confirmed its
offline desktop browser launch. See [help](WIN32-HELP.md) for remaining checks. See
[printing](WIN32-PRINTING.md), [file dialogs](WIN32-FILE-DIALOGS.md), [message boxes and alerts](WIN32-MESSAGE-BOXES.md), [logical palettes](WIN32-PALETTES.md), [combo boxes](WIN32-COMBOS.md), [dialog lifecycles](WIN32-DIALOGS.md), [dialog templates](WIN32-DIALOG-TEMPLATES.md), [icons and cursors](WIN32-ICONS-CURSORS.md), [menu bars](WIN32-MENU-BARS.md), [menu resources and accelerators](WIN32-UI-RESOURCES.md), [popup menus](WIN32-POPUPS.md), [menu data](WIN32-MENUS.md), [content scrolling](WIN32-SCROLL-WINDOW.md), [scroll state](WIN32-SCROLL-STATE.md), [attention](WIN32-ATTENTION.md), [window state](WIN32-STATE.md), [activation](WIN32-ACTIVATION.md), [desktop services](WIN32-DESKTOP.md), [list boxes](WIN32-LISTS.md), [edit fields](WIN32-EDIT.md), [standard controls](WIN32-CONTROLS.md), [input state](WIN32-INPUT.md), [mouse capture](WIN32-CAPTURE.md), [DIB/resource scope](WIN32-DIBS.md), [memory/timer scope](WIN32-MEMORY-TIMERS.md),
[UI-service scope](WIN32-SERVICES.md), [file/profile scope](WIN32-FILES-PROFILES.md)
[font/text scope](WIN32-FONTS.md), [region scope](WIN32-REGIONS.md)
[drawing-state scope](WIN32-DRAWING-STATE.md), [geometry scope](WIN32-GEOMETRY.md)
[child-window scope](WIN32-CHILDREN.md) and [positioning scope](WIN32-POSITIONING.md)
for supported contracts and remaining work.

## Useful starting points and gaps

| Area | What upstream offers | Work required for Stars |
| --- | --- | --- |
| Window lifecycle | SDL-backed creation, class/WNDPROC lookup and SDL window-ID mapping | The local window milestone now retains class metadata and per-window data, and tests independent lifetimes and reentrant teardown. Child/owned windows, nested drawing/input, coordinates, IDs and positioning/z-order now work; full style handling and desktop activation remain. Upstream `RegisterClassA` retains only the procedure. |
| Message dispatch and input | SDL input translation, queue, message filtering, mouse coordinates and keyboard mapping | The local milestone now tests filtering, focus transfer/loss and synchronous sends. Full Win32 ordering, capture and dialog routing remain. Upstream deliberately suppresses focus-loss activation messages and coalesces an internal `WM_USER+1` redraw signal; do not inherit those policies automatically. |
| Timers | Timer polling and queued messages | Local timers now use `(HWND, ID)`, dispatch callbacks on the UI thread, and are removed on window destruction. Upstream keys by ID alone and ignores the callback. |
| GDI | Memory DCs, bitmap storage, pixel operations and `StretchBlt` | Local basic paint/DC lifecycle, solid pens/brushes, rectangular clipping and selectable CP1252 fonts/text layout are now implemented. Local memory DCs, compatible bitmaps and ROP3 blits now work. Uncompressed DIB conversion and bitmap resource loading now work. Bundled faces, styled/rotated fonts, wrapped layout and clipped text now work. Copied application regions and intersections/exclusions now clip all drawing paths. Logical origins, all ROP2 modes, pattern brushes and frame/pixel output now work. Solid/null-pen ellipses and cosmetic dotted/dashed lines and rectangles now work. Other pen/geometry modes and extended printer capabilities remain. The retained `StretchBlt` accepts `SRCCOPY` only and a limited DC pairing. |
| Resources | Resource API declarations and placeholders, string-table extraction for two specific games | All 38 Stars bitmap resources now load through the local runtime. Menu templates and both accelerator tables now load. All 10 icons and 11 cursors now load, draw and preserve hotspots. All 34 dialog templates now load and parse; basic modal/modeless creation and lifecycle now work, with documented control limits. Local interactive message boxes now replace the upstream placeholder; unsupported service flags remain explicit failures. |
| Dialogs, controls and menus | Little overlap with the Stars inventory | Local push/check/radio buttons, group boxes, text labels and basic command notifications now work. Basic narrow edits and fixed-height string/owner-drawn lists now work. Menu trees, insertion/deletion, check/enable state and submenu ownership now work. Basic modal/modeless dialogs now work. Editable and fixed-height string/owner-drawn combo boxes now work; all 34 canonical dialog templates create. Deeper edit/list/combo behavior remains. Visible menu bars and their keyboard/pointer tracking now work. Canonical menu resources and game accelerators now work. Interactive popup rendering/tracking and window menu association now work. |
| File dialogs | Extension-only SDL dialogs cannot express Stars filters such as `*.R*` | Local modal browsing now supports game wildcard filters, directory navigation, selection/default extensions, overwrite confirmation and bounded output. Default-printer PDF/CUPS output now works; the original help guide opens through the desktop browser. |
| Files and configuration | Some legacy file wrappers and filesystem normalization | Existing `native.c` remains authoritative for the host. Do not adopt upstream normalization unchanged: it removes drive letters and leading slashes, which can break Linux absolute paths. Local profile/INI services now preserve settings defaults, updates and deletions and use native configuration directories; legacy file wrappers enforce sharing between local handles. |

The registry documentation explicitly assumes one live window. SDL window-ID
routing is useful, but it is not evidence of complete multi-window behavior.
The [window milestone](WIN32-WINDOWS.md) now provides owned records, retained
class metadata, per-window state and safe teardown, with a two-window test
through the compatibility API. [Parent/child relationships](WIN32-CHILDREN.md) now include logical child
canvases, coordinates, identifiers and descendant input routing. Complete input
semantics and deeper control/dialog behavior remain to do. The original SDL smoke test still exercises SDL directly.

## Build and verify

No additional dependencies are required for this initial adaptation.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

In CLion's SDL Debug profile, `stars_win32` is the library and
`stars-compat-runtime` is the runtime test executable. The preset builds it
alongside `stars-compat-abi`, `stars-ui-compat-check` and `stars-compat-smoke`.
The Release preset builds the same targets.

The geometry test runs unchanged against actual Win32 under Wine as
`test_win32_runtime_reference`. Cases include negative coordinates, exclusive
edges, inverted and empty rectangles, shrinking, source/destination aliasing,
and zeroing a disjoint or edge-touching intersection. The Linux-only clock
translation unit uses private C test adapters for `SDL_GetTicks` to inject ticks across
32-bit rollover and check truncation without waiting 49 days. No game or
regression baseline data is altered.

## Verification on October 10, 2026

See the current [content-scrolling verification report](WIN32-SCROLL-WINDOW.md#verification)
for native/Wine tests, the full game regression and the tutorial result.
Snapshot checksums and reproducible coverage generation pass. No regression
baseline changed.
