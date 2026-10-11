# Native window and control inspection

Basic Win32 inventory coverage remains **178/178 (100%)**, plus six CRT helpers.
This feature is an application-specific debugging/automation interface, not an
additional Win32 API or a claim of complete control conformance.

## Built-in viewer

Run **stars-sdl** in CLion's SDL Debug profile, then press **Ctrl+Shift+F12**.
The chord avoids the game's F12 turn-generation shortcuts. It opens one modeless
inspector; pressing it again refreshes that window. Select a row to see its
logical ID, parent and owner IDs, control/resource ID, class, styles, visibility,
enable/focus/capture state, screen bounds, client bounds and text. Children are
indented; owned top-level dialogs have separate rows with an explicit owner ID.
Hidden controls are included. Use Refresh after opening/closing dialogs or
changing pages. The details field is read-only and supports copying.

Styles show both the original hexadecimal value and expanded symbolic names.
The decoder distinguishes top-level `WS_MINIMIZEBOX`/`WS_MAXIMIZEBOX` from child
`WS_GROUP`/`WS_TABSTOP`, and interprets low style bits using the control kind or
dialog flag. Masked fields such as `BS_AUTOCHECKBOX` and `CBS_DROPDOWNLIST` use
one name, rather than treating their component bits as independent flags.
Unknown bits remain hexadecimal. Extended styles are decoded separately.
Names describe the stored flags; they do not certify that a behavior is supported.
JSON retains numeric `style`, `exStyle` and `controlKind`, and adds string fields
`styleNames`, `exStyleNames` and `controlKindName` without changing schema 1.
See [header constants](WIN32-CONSTANTS.md) for the expanded declaration surface.

No additional libraries or external desktop automation tools are needed.
The same hierarchy API works in [headless mode](SDL3-HEADLESS.md).
The viewer lists itself and its controls too. It is a point-in-time view, not a
live event stream. Opening it changes focus like opening another application
window; snapshot capture itself does not change focus or call game procedures.

## In-process interface

`compat/win32/include/stars_compat.h` supplies:

| Function | Purpose |
| --- | --- |
| `StarsCompatCaptureHierarchy` | Copy all live logical HWNDs and metadata into a snapshot |
| `StarsCompatFreeHierarchy` | Release copied strings/rows and zero the snapshot |
| `StarsCompatWriteHierarchy` | Write schema-1 UTF-8 JSON to a caller-owned `FILE` |
| `StarsCompatWindowId` | Obtain a logical ID for a live HWND |
| `StarsCompatResolveWindow` | Resolve an ID to a currently live HWND |
| `StarsCompatFindControl` | Find a unique immediate child by control ID and optional class |
| `StarsCompatShowInspector` | Open or refresh the built-in viewer |

Capture/lookup/viewer calls run on the UI thread. Snapshot copying never invokes
`WM_GETTEXT`, subclass procedures, enumeration callbacks or event pumping.
The copied snapshot survives window destruction and runtime shutdown. Initialize
it to zero and free it before reuse. Freeing and serializing an independently
owned snapshot do not require a live runtime.

IDs are decimal strings in JSON to preserve all 64 bits. They are unique for
the process lifetime, including compatibility-runtime restarts, and are never
native pointer values or SDL window IDs. Reusing one desktop window across wizard
pages still retires each old logical ID. Resolve returns NULL after destruction;
IDs are not persistent locators across application runs. Use class/title and
control-ID paths to discover a fresh ID on each run. Child lookup returns zero
for an absent **or ambiguous** match, rather than picking an arbitrary control.
Password edit text is redacted in both snapshots and exports.

A tutorial harness linked into the native client can address controls directly:

```c
uint64_t idButton = StarsCompatFindControl(idDialog, IDCANCEL, "Button");
HWND hwndButton = StarsCompatResolveWindow(idButton);
if (hwndButton && IsWindowVisible(hwndButton) && IsWindowEnabled(hwndButton))
    SendMessage(hwndButton, BM_CLICK, 0, 0);
```

A synchronous action may destroy a dialog or open another one. Resolve IDs again
after callbacks, and do not retain HWNDs as serialized locators. List selection,
text entry and menu commands can use their existing message contracts from the
UI thread. The [native tutorial smoke runner](NATIVE-TUTORIAL-RUNNER.md) uses a
UI-thread timer, fresh hierarchy resolution, copied scanner coordinates and
real SDL input, waiting for specific tutorial state changes. It currently covers
premature-generation rejection and fleet/research/production actions through
generation, three scouting routes, remote mining, summary popup inspection,
cargo loading, colonization, logistics, colonizer production, scanner view
changes, repeat transport orders, expanded production and enemy design-popup
inspection, ship transfer and Split All, waypoint dragging and research
popup/budget checks, then Construction research and Oxygen/Mozart orders
through page 32. Copied checkpoints
include orders, cargo, sixteen design counts, owned fleet count, popup identities, all owned production queues, research flags, transport
actions and map coordinates. Full tutorial execution remains
pending. Inspection provides no external IPC service or AutoHotkey dependency.

## Map-object inspection

The hierarchy viewer's **Map objects** button opens a modeless scanner inspector
when Stars registers its game adapter. Refresh captures copied object data;
Select/reveal changes the secondary scanner selection through the normal game
UI path and pans if needed. It issues no orders and leaves the main selection
and waypoint editing mode alone. This also works with the headless backend.

[scaninspect.h](../scaninspect.h) exposes `FCaptureScanner`,
`FWriteScannerSnapshot`, `FSelectScannerObject` and `FreeScannerSnapshot`.
Call them on the UI thread with a loaded player game. A snapshot contains game,
turn, player and zoom, the scanner viewport, and each object's kind, ID, name,
owner/detail, universe/client/screen coordinates, filter state and selection
flags. Keys are `(game, turn, player, kind, id)`; array indices and pointers are
not locators. Selection rejects stale game/turn/player keys and absent objects.
Re-capture after loading a turn or changing the view.

Planet names/coordinates are public even before exploration. Fleet and other
object entries come from the current player's loaded client data; host-mode
inspection is rejected. `fOnScreen` means the object's center is within the
viewport, not that a visible pixel is guaranteed: filters and aggregated orbital
fleets affect rendering, and large overlays can extend beyond their centers.
`fFiltered` reports the existing scanner filter rules separately. JSON schema 1
converts copied CP1252 names to UTF-8 and preserves numeric IDs.

```c
SCANSNAPSHOT snap = {0};
if (FCaptureScanner(&snap)) {
    FWriteScannerSnapshot(&snap, stdout);
    /* Use an enumerated kind/id; TRUE pans to reveal the selected object. */
    if (snap.cObject != 0)
        FSelectScannerObject(&snap, snap.rgobj[0].grType, snap.rgobj[0].id, TRUE);
    FreeScannerSnapshot(&snap);
}
```

The adapter lives beside the scanner UI; the generic compatibility runtime only
stores a callback for opening it. No external tool or IPC service is required.
The Continue Game test checks copied state, coordinates, unique keys, stale-key
rejection, JSON escaping and selection/reveal, plus viewer close/reopen.

## Verification

`stars-compat-inspect` checks nested children and separate owners, hidden/disabled
state, focus/capture, duplicate control-ID rejection, off-thread rejection,
password redaction, CP1252-to-UTF-8 JSON escaping, IDs above 2^53, snapshot lifetime,
stale-ID rejection and monotonically increasing IDs across runtime restart.
Non-ASCII native titles are converted to UTF-8 while snapshots and game text
remain CP1252; title creation/update assertions cover that boundary.
It also checks distinct logical IDs over a reused native wizard window, direct
button activation, viewer refresh/details/painting, the diagnostic chord,
close/reopen, and normal/silent teardown. Python parses the exported JSON.

The dummy-backend test is `win32_inspect`; `win32_inspect_x11` runs the same C
contracts inside a private Xvfb/Openbox desktop. UI test processes run sequentially.
Before style expansion, Debug and Release qualification passed 93/93 tests in each build. The
full fatal ASan/UBSan/LSan suite also passes 93/93 without suppressions, including
the private X11 inspector test and non-ASCII title boundary. The Debug headless
and sanitizer presets each pass 80/80 with both display variables cleared.
Windows units pass 50/50. The final viewer-only background paint and pixel
assertion were then verified by both inspector tests in all three builds (6/6),
plus the headless inspector case (1/1); its offscreen rendered layout was visually
checked. No game baseline changed.

The style expansion adds cases for masked button/static/combo fields, edit/list
flags, dialog flags, scrollbar alignment, child versus top-level shared bits,
and unknown bits. Viewer and parsed JSON assertions check symbolic names and
numeric values together. The expanded ABI checks every one of the 934 new header
constants against the installed MinGW SDK. The subsequent full Debug and fatal
ASan/UBSan/LSan suites each pass 93/93, without sanitizer suppressions.
Both full runs now enclose legacy X11 cases in a private outer virtual desktop.
Final Release ABI/inspector/combo checks pass 5/5; Windows units pass 50/50.

The scanner adapter is covered by the loaded-game Continue tests on dummy and
private X11 backends, including fatal sanitizers. These enumerate planet/fleet/
thing keys, validate coordinates, reject stale/absent selections, preserve dirty
state during capture and selection, verify UTF-8 JSON, and exercise map-viewer
selection, close/reopen and nested control painting. Full native and host game
regressions remain unchanged (519 matches each; 87 unused-storage warnings).


The native tutorial production-template checkpoint also captures fresh hierarchy
IDs and owner relationships while a template dialog disables its production
owner, after restoring that owner, and after both OK actions destroy the logical
dialogs. It copies the default production template as semantic fields alongside
orders snapshots; packed game structures and saved record widths are unchanged.
Cross-build qualification is recorded in [the goal report](LINUX-CLIENT-GOAL.md).


The battle-cycle tutorial extension copies browser and VCR window identities,
then verifies destruction after Close/Done. Read-only battle snapshots include
battle ID, token count, playback step, final step and timer-playing flag. The
launcher compares identity across play, pause, seek, rewind and stepping;
opening/closing alone is insufficient. Qualification remains tracked in the
[goal report](LINUX-CLIENT-GOAL.md).
