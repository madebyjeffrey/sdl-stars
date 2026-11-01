# M04 — Mouse tracking, cursor and appearance messages

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the
working collection. This companion specializes [M04-US01](M04-race-design.md#m04-us01),
[US02](M04-race-design.md#m04-us02) and [US03](M04-race-design.md#m04-us03).
It specifies all parent mouse/down/double-click and cursor arms, the complete
`IrcRaceDlgHitTest`, `FTrackRaceDlg2` and `FTrackRaceDlg3` programs, portrait
repeat tracking, and all color/background arms. [Initialization](M04-page-initialization.md)
owns entry/control geometry; [commands](M04-page-commands.md) own setting commits;
the [controller](M04-wizard-controller.md) owns modal completion. M04-AC35–47
remain at the entry. Runtime game behavior, source, records and baselines are unchanged.

Canonical model: [M04MouseTracking](dafny/M04-mouse-tracking.dfy), reusing S01
signed/byte arithmetic, M04 native stat/portrait/buffer adapters and initialization
state, and S04 `LMulDiv`. Source: [raceui.c](../src/ui/raceui.c) named functions
above and `RaceWizardDlg1`…`RaceWizardDlg6`;
[race.c](../src/core/race.c) stat bounds/map/helpers;
[services.c](../compat/win32/src/services.c) `MulDiv`;
[utilgenui.h](../src/ui/utilgenui.h) actual tracking/blit parameter widths.
The [collection record](README.md#race-mouse-and-message-continuation) reports checks.

## State, input, replies and source domains

`State` extends the full initialization/command/native projection with raw active
panel and the two shared portrait spin rectangles plus initialized knowledge.
Logical array shapes do not establish native pointers, live windows, storage or
all world invariants. This contract uses the wizard's physical 17-rectangle
frame and native PLAYER byte layout. Producers must supply defined reached
fields. `Unqualified` distinguishes missing mapping/lifetime/point evidence from
an established source array/arithmetic undefined boundary (`UndefinedNative`).
Neither is a new C rejection or failed callback result.

Mouse coordinates decode LOWORD/HIWORD as signed 16 bits. Keyboard flags narrow
to signed 16 bits; only bit 4 selects the larger habitat/economy step. Portrait
step size does not use that modifier. Actual point writes/conversions and input
dispatch stay S04/S05-owned. Cursor/mouse receipts record whether the reached
point was written; ignored API booleans do not invent a zero point.

Start, repeat, mouse, redraw, cleanup and cursor replies carry explicit focused
post-state. Native helper/callback changes survive in the result; there is no
tracking snapshot rollback. Start/cursor bundles include the reached prefix
length for a nonlocal exit, so an escape in SetCapture/GetCursorPos does not
claim later SetCursor/ScreenToClient calls happened. `Escaped` bypasses later
controller effects. `InvalidEvidence` rejects inconsistent service kinds/frames.
`Pending` is a finite missing-reply prefix, not native FALSE or completion.

`Run`, `Start`, `DrawResume` and `End` check shapes and consumed-reply bounds and
evaluate a supplied finite trace. They do not claim a held button must stop,
that all helpers return, or that real native messages obey every supplied reply.
Timer/capture/window/DC lifetime, actual callback/reentrancy and source-to-state
extraction retain S05/S06 qualification. The timer argument 80 is a source call
value, not a measured responsiveness guarantee.

## Hit testing and parent dispatch

`HitTest` returns no hit before reading rectangles for any nonzero read-only
flag. Otherwise `Scan` searches ascending indices while index < raw signed count,
using left/top inclusive and right/bottom exclusive containment. A negative/zero
count performs no rectangle reads. A count above 17 is not rejected in advance:
an earlier found hit can still be defined; reaching slot 17 crosses the known
physical frame. Missing initialized fields are separately unqualified.

On the habitat panel, the first found rectangle is then filtered by the signed
maximum environment for `index/5`. A negative maximum returns no hit without
trying later overlapping rectangles. Growth slots 15/16 reach axis index 3 and
retain [GAP-27](README.md#gap-27)'s undefined read. No index/boolean result is
assigned after that access. Initializer geometry does not repair this rule.

Mouse down and double-click use the same arms: identity routes to portrait
tracking, habitat/economy route to their trackers, and other pages fall through
with zero. A tracker hit is distinct from modal completion; none of these arms
calls EndDialog. Habitat/economy cursor handling first gets/converts the point,
then tests it and sets Hand only on a defined hit. These calls occur even when
the later hit test returns no hit for read-only. Unsupported pages have no
cursor arm. The callback result is one/zero independently of any saved design.

## Portrait repeat tracking

The left spin rectangle has priority, including overlaps. Source query order
is 0,0 for a left hit and 0,1,0 for a right hit; a miss stops after 0,1.
Read-only performs no spin queries. Native portrait is a five-bit field; the
source's >=32 fallback is unreachable under that layout (`Portrait`'s bound).

The handler remembers the portrait before GetDC/palette/InitBtnTrack. Each true
repeat changes only its **local** image by -1/+1 modulo 32 and requests a blit
from the 8-by-4 sheet. It does not write the global portrait on each tick. The
bitmap coordinates come from the current first spin rectangle; destination
subtractions must be defined signed-32 operations, then narrow to the actual
signed-16 DibBlt parameters. Callback-updated geometry is not frozen at entry.

After the false repeat reply, the local image is written to the current native
record, then the original DC is released. Even zero true ticks restore that
remembered image over a callback's intermediate portrait change. Cleanup replies
may have their own effects; `PortraitSetBeforeCleanup` proves the assignment at
that stage without promising that every later callback preserves it. Escaping
before this point does not perform the final assignment/release.

## Habitat buttons, widths and growth

| Hit | Adjustment and InitBtnTrack arguments |
| --- | --- |
| Axis slot modulo 5 = 0 / 2 | Shift -1/+1, width 0, button 2/3, NULL text. |
| Axis slot modulo 5 = 3 / 4 | Width +1/-1, shift 0, button 8, wider/narrower label pointer. |
| Growth 15 / 16 | Growth shift +1/-1, button 160/161, NULL text. Their ordinary habitat hit qualification still has GAP-27. |

Bit 4 multiplies habitat width/shift by ten after InitBtnTrack. `Resize` preserves
**every signed-byte store**, not a mathematical clamp with postponed conversion:
first form new endpoints as int8, compensate a maximum above 100, compensate a
minimum below zero, then apply the half-width correction when the difference is
below 20. That last integer correction can retain an odd width 19. No universal
minimum-20 or valid-endpoint property is inferred for arbitrary raw native values.
`TrackingVectors` retains that boundary.

Endpoint changes write both stored endpoints and their midpoint before redraw;
unchanged endpoints do not redraw. When the repeat loop ends, the midpoint is
unconditionally recomputed from the then-current endpoints, even with zero true
ticks and no redraw. `NoTickRecomputesCenter` captures that distinct final step.

Growth adds its shift into **int8 before clamping** to 1…20. Raw 127 plus a shift
of 10 wraps before the clamp and produces 1. `GrowthStepBounds` proves the stored
step result is within 1…20, while `TrackingVectors` records the cast boundary.
These numeric branch contracts do not claim a normal habitat growth click can
pass GAP-27. The source fixture uses a defined other-panel entry to exercise the
tracker's growth branches without executing the invalid helper read.

## Habitat dragging

Axis slot modulo 5 = 1 captures the mouse and selects the grab cursor, then
captures half the current width using signed-16 difference and C division.
That local half-width survives later redraws/callbacks. Every true mouse reply
uses the **current** global rectangle and written POINT16 value. It performs
the two source comparisons sequentially, narrowing an assigned rectangle edge
back into the point's int16 field after each assignment.

`DragStep` checks the two reached signed-32 subtraction domains before the actual
MulDiv call. The shared arithmetic rounds half away from zero and returns -1
for zero denominator/out-of-range result; that returned value still narrows to
signed 16 bits and passes through the source centre clamp. A zero-width rectangle
introduced during tracking is not automatically a failed C return.

The redraw condition compares only current maximum against the **untruncated**
target-plus-half expression. On a change, endpoints store to int8 and midpoint
is computed from those stored bytes; an odd original width can shrink by one.
If maximum already matches, minimum/midpoint are not separately repaired. After
the false mouse reply, ReleaseCapture runs; this drag branch has no unconditional
end-of-loop midpoint step. Missing/escaping replies do not imply that cleanup ran.

## Economy repeat tracking and redraw handoff

`EconomyStep` uses the fixed seven-entry `rgRW3IStat` map, current signed-byte
stat, even-index increment / odd-index decrement and step three for bit 4,
otherwise one. `SetRaceStat` clamps through the existing M04 bounds. Redraw is
requested only when its returned value differs from the prior stat. The map is
read only after a true repeat: hits 14…16 followed by an immediate false reply
can return TRUE without a map access, while a true reply crosses that table.
This retains the source domain instead of adding an early C guard.

Axis/growth/economy redraws carry the precise pre-call state and mask. Literal
NULL is used for dragging; other paths pass the BTNT.hdc expression. The model's
`trackDC` flag denotes this expression origin, not a guarantee that the handle
is nonzero/live. `DrawFrame` preserves the focused non-race binding/password and
command/panel/spin groups while allowing the existing drawing/pricing owner to
supply native race, rectangle/count/cache changes. The next tick uses that
post-state. Complete DrawRace2/3 and native pricing refinement remain explicit
work, not a second algorithm hidden in a receipt.

## Color and background callbacks

`ColorIds` records each page's exact child-query list: identity 271…278,
primary 271…280, lesser 291…304, habitat 291…293, economy 291, and research
271…288 followed by 291 only if needed. `FirstHandle` compares actual supplied
handle identities, including null/duplicates, and preserves first-match stopping.
Static-color messages still execute the source queries before their fallback;
they do not skip directly to background assignment. `StaticColorStillQueries`
and the at-most-20-request bound cover those calls.

A match or static message requests button-face background and returns the face
brush handle; otherwise callback zero delegates to default handling. `FaceBrush`
is an opaque return value, not Boolean TRUE; a zero handle can have the same
numeric return as zero with different requests. Every erase arm requests a
client rectangle and FillRect and returns one regardless of the ignored fill
result. Reached rectangle fields must have valid written native values; the model
does not invent a rectangle when the client query left them unwritten. These are appearance requests, not proof of successful drawing, readable
contrast, valid handles, or complete native control behavior.

## Evidence, interaction quality and remaining work

M04-IR01/02 and QR01–03 remain authoritative. Pointer operations now have explicit
dispatch/step/cancellation/callback states; full keyboard alternatives, focus,
accessibility, display and latency targets remain S05/Open work. M02 owns later
save/version/caller snapshot behavior; this contract neither saves a race nor
restores a cancelled wizard's outer state.

The [tracking fixture](../tests/spec/check_race_tracking_source.py) compiles actual
hit/track/mouse/cursor bodies, MulDiv/stat/bounds helpers and native PLAYER/BTNT
types with warnings as errors and address/undefined-behavior checks. Its
independent oracle checks **2,261 cases**, complete native bytes and request
order, raw flags, signed-byte/portrait/coordinate boundaries, zero ticks, odd
width, overlapping hits, live zero/reversed rectangles, callback state retention,
redraw normalization and escaping start/tick/draw/end/cursor calls. Known invalid
growth/map reads are excluded before C execution, not counted as ordinary results.
Drawing services execute BoundsCheckPlayer but do not execute full pricing or
painting. Event/button/capture/redraw/handle services remain scripted.

The [appearance fixture](../tests/spec/check_race_appearance_source.py) compiles
all six actual color and erase arms and checks **1,882 cases**, all seven color
message types, null/duplicate/unmatched handles, short-circuit query order and
brush versus numeric returns. Its failed fill reply confirms the ignored-return
contract. This is not a native rendering or assistive-technology test. The
[collection record](README.md#race-mouse-and-message-continuation) records full
formal verification, links, handbook and required repository checks.

Remaining D/A/V work is all WM_PAINT arms, DrawRace2/3 and advantage-point
drawing/invalidation (including full geometry/text/cache/numeric effects),
general catalog/format extraction and native event/timer/capture/window/
lifetime/refinement. [Text producer contracts](S03-player-labels-and-race-titles.md)
now specify PszPlayerName and pinned title/default/grammar/format bindings. GAP-26/27 and existing policy/quality questions retain
their owners. A specified nonpaint handler is not a complete native UI proof.

The next paint contract must carry the chosen pricing target. Page-1 WM_PAINT
passes the first checked preset's native record to DrawRaceAdvantagePoints, or
the working race for Custom/no preset. That helper calls CAdvantagePoints on
the supplied pointer. Its normalization can therefore affect the mutable native
preset copy; a future paint model must not assume that every price display only
touches vplr or that every preset copy is immutable. Catalog definitions/IDs
remain distinct from these runtime copies. This is a named remaining state
contract, not a runtime change or a new claim of complete paint coverage.
