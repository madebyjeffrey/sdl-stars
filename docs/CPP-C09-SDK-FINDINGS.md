# Native SDK probe findings during C09

The expanded SDK-only run at `6e547f05` built every independent C entry and
executed 39 checks: SDK type isolation plus 38 shared vectors. Twenty passed and
19 failed. The [retained run/artifact](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41220497527)
contains exact compiler manifests and complete per-test logs. These results are
not reported as passing.

All 38 shared `.inc` inputs, their resources and generators are byte-identical to
C07 `bcdd77bf`. Each SDK C entry has the same four-line preamble as its earlier
generated entry after resolving the include path. These executables link native
SDK libraries, not application implementations. Thus the failures are evidence
about existing probes/native behavior, not an observed regression in the C++
application. No input assertion, expected value or application behavior has been
changed to turn the run green. The established ABI/native-point runtime probes
and SDK type-isolation check pass.

## First observed failures

| Probe | First failure from the native runner |
| --- | --- |
| Toolbar | Line 131: split-dropdown notification/rectangle and command counts |
| List | Line 72: owner-selection draw count; caller line 210 |
| Edit | Line 55: multiline case-style undo restores empty text instead of `ABC DEF`; caller line 452 |
| Controls | Line 192: `CheckDlgButton` on missing control 999 |
| Scroll | Line 56: `SetScrollRange` with reversed endpoints; caller line 320 |
| UI resources | Resource 6 differs at byte 452: `a9 / c2` |
| Popup | Line 147: result, command and menu-loop notification counts |
| File dialog | Native picker timer deadline in mode 2 / phase 0 |
| Message box | Callback lines 61/66: selected/default child and focus; then test timeout |
| Combo | Native process segmentation fault while creating resource dialog 2430, after earlier combo contracts pass |
| Menubar | Line 182: expected destruction has not occurred |
| Attention | Line 143: caption state/notification count |
| Position | Line 239: previous top-level/owned window ordering |
| Children | Line 234: failed creation destruction/control lookup contract |
| Geometry | Line 75: `IntersectClipRect` region classification |
| Drawing state | Line 57: invalid `SetROP2` operations and retained mode |
| Regions | Line 44: `IntersectClipRect` region classification |
| Timers | Line 62: callback count / timer ID |
| Bitmap | Line 140: pixel validity and source DC deletion |

These are first-failure observations, not diagnoses of every later assertion in
each program. Resource encoding, native event delivery and automation have not
been conflated with API-return differences. Full repair of the probes would need
separate investigation while preserving their independent expectations.

The 19 individually named, hash-pinned probes are diagnostic. Every original
assertion still executes in both SDK configurations and its raw CTest result and
JUnit report are retained. The other 20 selected SDK checks remain gates, including
ABI/native points and SDK isolation. Missing/skipped executions, changed diagnostic
inputs and unexpected failures remain errors; four policy tests exercise those
rules. This retains behavior-neutral C scope without rewriting independent
expectations. Follow-up: [#78](https://github.com/madebyjeffrey/sdl-stars/issues/78).

Ordinary Windows qualification runs even when a probe fails, so one diagnostic
cannot hide application, standalone-header or linked-witness results. The final
matrix distinguishes passing contracts from recorded diagnostic failures; it does
not claim that all 39 SDK probes pass.
