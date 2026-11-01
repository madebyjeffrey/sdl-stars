# Native SDK probe findings during C09

The expanded SDK-only run at `6e547f05` built every independent C entry and
executed 39 checks: SDK type isolation plus 38 shared vectors. Twenty passed and
19 failed. The [retained run/artifact](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41220497527)
contains exact compiler manifests and complete per-test logs. These results are
not reported as passing.

At that initial run, all 38 shared `.inc` inputs, their resources and generators
were byte-identical to C07 `bcdd77bf`. Each SDK C entry has the same four-line
preamble as its earlier generated entry after resolving the include path. These executables link native
SDK libraries, not application implementations. Thus the failures are evidence
about existing probes/native behavior, not an observed regression in the C++
application. Later investigations found actual compatibility/test defects;
corrections are recorded below. The established ABI/native-point runtime probes
and SDK type-isolation check pass.

## First observed failures

| Probe | First failure from the native runner |
| --- | --- |
| Toolbar | Original line 131: extra command after the dropdown. Diagnosed and corrected below; the full native vector is requalified as a gate. |
| List | Line 72: owner-selection draw count; caller line 210 |
| Edit | Line 55: multiline case-style undo restores empty text instead of `ABC DEF`; caller line 452 |
| Controls | Original failure: `CheckDlgButton` on missing control 999. That expectation and the SDL return value are now corrected to failure; native requalification is pending. |
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

Eighteen individually named, hash-pinned probes remain diagnostic. Their
assertions execute in both SDK configurations and raw CTest results and
JUnit reports are retained; intentional corrections are documented. The other
21 selected SDK checks are gates, including the requalified toolbar vector,
ABI/native points and SDK isolation. Missing/skipped executions, changed diagnostic
inputs and unexpected failures remain errors; four policy tests exercise those
rules. Follow-up: [#78](https://github.com/madebyjeffrey/sdl-stars/issues/78).

Ordinary Windows qualification runs even when a probe fails, so one diagnostic
cannot hide application, standalone-header or linked-witness results. The final
matrix distinguishes passing contracts from recorded diagnostic failures; it does
not claim that all 39 SDK probes pass.

The `CheckDlgButton` correction is an intentional compatibility bug fix after
Milestone C. Its shared expectation now rejects missing controls on both backends;
the full native controls vector remains diagnostic until requalified.

## Toolbar diagnosis and correction

The [original vector repeated three times on native Windows](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41238708225)
reported `drops=1`, `id=103`, matching rectangles `(66,2,104,23)`, and
`commands=3` instead of two. The compound assertion failed on command count,
not notification identity or geometry.

A [one-button native reproduction](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41238830560)
isolated the same failure three times. Each run received the dropdown on mouse
down, retained capture, and sent the extra command on mouse up. Changing only
the handler's return value distinguished the cause:

| Handler result | Capture after mouse down | Command after mouse up |
| --- | --- | --- |
| `TBDDRET_DEFAULT` | None | None |
| `TBDDRET_NODEFAULT` | Toolbar | One |
| `TBDDRET_TREATPRESSED` | Toolbar | One |

Releasing capture or opening/cancelling a real popup inside the notification did
not suppress the later command with `NODEFAULT`. Removing the check-button style,
using a whole dropdown, and activating common-controls v6 gave the same outcome.
Microsoft's [dropdown return contract](https://learn.microsoft.com/en-us/windows/win32/controls/tbn-dropdown)
identifies `DEFAULT` as handled and `NODEFAULT` as unhandled. The SDL wrapper
previously suppressed both, and completed `TREATPRESSED` on mouse down.

`ClickToolbar` now retains the mouse press for both fallback results and sends
the ordinary command on release. `TbWndProc` and the shared test handler return
`DEFAULT` for handled menus, preserving the game's menu/filter behavior. The
vector also uses `TB_GETRECT`, as the notification documentation guarantees only
`hdr` and `iItem`; its former read of `rcButton` was an additional test assumption,
although it did not cause this observed failure.

The corrected complete shared vector, including both fallback results for split
and whole dropdowns, [passes three native Windows executions](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41238997354).
The new fallback assertion fails against the old SDL wrapper and passes after
the fix, including ASan/UBSan. The full macOS Debug collection passes 151/151,
including the actual-client menu workflow, tutorial smoke checkpoint and 29 leak
checks. Native regression matches all 519 files, with 25 accepted unused-storage
warnings and no differences/errors/skips; no baseline is changed. Both touched
C++ sources and the independent SDK C vector compile for Windows under their
production warning policies.

The toolbar is removed from `diagnostics.json` and must now pass in both SDK
configurations. This resolves the toolbar finding; it does not diagnose the
remaining probes.
