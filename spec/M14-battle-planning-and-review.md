# M14: Battle Planning and Review

Status: source-derived detailed specification; logical ownership is proposed.
Inspected: `e2eae18cff0f990db20415c1c2262aed2022f95b`, 2026-10-24.

Canonical entry: [M14-battle-review-specification.dfy](M14-battle-review-specification.dfy);
[rules and proofs](dafny/M14-battle-review.dfy). Behavior statuses and evidence follow
[the collection conventions](README.md#requirement-ids-and-status).

## 1. Purpose, actors and scope

Manage battle plans and inspect recorded outcomes. Planning and replay are
separate workflows within this entry. [M13](M13-player-relations.md) owns directed
stances and the shared attack-intent predicate; [M10](M10-fleet-operations.md)
assigns plan indices to fleets. M19 resolves combat, S01 stores plans/records,
S06 owns their logical representation and M15 links messages/reports to replay.
Playback changes presentation state, not the already resolved battle.

Primary actors: A player assigning fleet battle policy and reviewing a recorded battle.

### User stories

These source-derived stories describe intent, not evidence of user research.
The prerequisites and triggers are the corresponding workflows below. All
stories describe Observed capabilities with the stated limitations.

<a id="m14-us01"></a>
**M14-US01 — Edit a battle plan.** As a player, I want target and tactic choices so that fleets have a deliberate combat policy.

<a id="m14-us02"></a>
**M14-US02 — Manage reusable plans.** As a player, I want copy, rename and delete with reference handling so that fleets keep meaningful assignments.

<a id="m14-us03"></a>
**M14-US03 — Review the battle.** As a player, I want step, rewind and playback controls so that I can inspect recorded actions and losses.

<a id="m14-us04"></a>
**M14-US04 — Return safely from review.** As a player, I want playback stopped and temporary state released on close so that review does not alter my orders or combat outcome.

### Story entry and completion conditions

| Story | Trigger and prerequisites | Completion / alternate outcome |
| --- | --- | --- |
| [M14-US01](#m14-us01) | Open plans for a loaded player/fleet with an existing plan. | Commit the dirty working plan on switch/close. |
| [M14-US02](#m14-us02) | Choose Copy/Rename/Delete with the selected index and count available. | Apply count/protected-index rules; decline deletion leaves references intact. |
| [M14-US03](#m14-us03) | Open a battle whose record is present in the current view. | Step/play bounded recorded events; failed lookup/allocation cleans up. |
| [M14-US04](#m14-us04) | Close an active replay or reach its last step. | Stop the timer and release replay-owned state; combat is not recomputed. |

## 2. Inputs, outputs and state

| Area | Contract |
| --- | --- |
| Plans | Up to 15 entries; index zero has protected rename/delete UI, and each fleet stores a plan index. |
| Working plan | `btlplan`, selected index and dirty flag; switching/closing may commit the working entry. |
| Policy | Primary/secondary target, tactic, attack-who code, cargo-dump flag and bounded name. |
| Replay | Battle record/token arrays, current step from -1 through final step, round and timer/speed state. |

Persistent records remain [S01](S01-game-file-formats.md); logical state and
identity remain [S06](S06-shared-game-state.md). A derived/working value must not
be treated as a separately committed record or as information known to every player.

## 3. Workflows, rules and handoffs

### Plan editing and plan lifecycle

[battleui.c](../src/ui/battleui.c) `BattlePlansDlg` opens the selected fleet's
plan or plan zero. Controls edit the working `btlplan`; dirty entries are written
and logged on both IDOK and IDCANCEL. Switching plans and copying also flush
pending work. `Commit` specifies this close behavior. Single-player mode fixes
attack-who to everyone and disables its selector; multiplayer list mapping
skips self (`AttackCode`). M13's `Attack` gives intent semantics, while target
classification/tactics remain combat contracts.

Copy refuses at 15 plans (`CanCopy`), appends a plan, suggests a bounded suffix
and opens the rename dialog. Rename Cancel does not imply deletion of a copy
already appended. Plan zero's rename/delete controls are disabled
(`CanRenameOrDelete`). `FDeleteBattlePlan` in [battle.c](../src/core/battle.c)
warns when fleets use the plan. Decline leaves references intact; acceptance
shifts subsequent plans down and decrements matching or greater fleet indices.
A fleet assigned the removed nonzero plan therefore falls to the preceding plan,
not automatically to zero (`AfterDelete`). Log and confirmation side effects
remain E14-1.

### Load and play a recorded battle

[vcr.c](../src/ui/vcr.c) `BattleVCR`, `BtlDataGet`, `SetVCRBoard`, `VCRDlg` locate
the battle across record blocks, allocate token/damage state and replay recorded
steps. A missing battle or allocation failure follows its cleanup path. The
board starts before the first event at step -1; final step is inclusive.
Rewind/forward clamp to that interval (`Step`). Control requests 100 steps,
Shift ten, otherwise one (`Quantum`). Playback uses a timer interval of
570 minus 120 times speed, for speeds 0–4 (`TimerMillis`); faster playback also
changes animation behavior. Manual transport controls stop the active timer
before acting. Reaching the end or closing kills the playback timer.

Replay reconstructs token positions, damage and kills from the stored record;
it does not call combat resolution again. Source record validity and token-index
bounds remain S01/E14-2. Summaries distinguish own/other units and ship categories;
they cannot reveal information not retained in the player's battle record.
Close releases temporary arrays and restores the caller's UI flow. Tutorial
progress can be marked by viewing; that is a separate M16 side effect.

## 4. Acceptance and traceability

All criteria below have **Observed** behavior status except where a rule is
explicitly a caller convention or repository maintenance constraint. A named
model/lemma supplies mathematical evidence within its preconditions. An E-boundary
supplies source evidence and an explicit implementation-test/refinement gap,
not a proved runtime outcome. The dated collection run records actual checks.

| Criterion / story | Given / When / Then | Authoritative rule, source and evidence scope |
| --- | --- | --- |
| <a id="m14-ac01"></a>**M14-AC01** / [M14-US01](#m14-us01) | Given a dirty plan, when either close command runs, then commit/log it rather than roll it back. | `Commit`; BattlePlansDlg |
| <a id="m14-ac02"></a>**M14-AC02** / [M14-US01](#m14-us01) | Given a single-player game, when choosing attack-who, then use everyone code 3 regardless of disabled selector row. | `AttackCode`, vectors |
| <a id="m14-ac03"></a>**M14-AC03** / [M14-US02](#m14-us02) | Given 15 plans, when copying, then refuse; plan zero cannot be renamed/deleted through its protected controls. | `CanCopy`, `CanRenameOrDelete` |
| <a id="m14-ac04"></a>**M14-AC04** / [M14-US02](#m14-us02) | Given deletion of nonzero index k, when accepted, then fleet plans at or above k decrement and later plan indices shift. | `AfterDelete`; FDeleteBattlePlan; E14-1 |
| <a id="m14-ac05"></a>**M14-AC05** / [M14-US03](#m14-us03) | Given step 2 and final step 10, when advancing 100, then stop at 10; rewinding ten stops at -1. | `Step`, `ReplayBound`, vectors |
| <a id="m14-ac06"></a>**M14-AC06** / [M14-US04](#m14-us04) | Given active playback, when closing, then kill the timer and release temporary replay arrays without resolving combat again. | E14-2; VCRDlg/BattleVCR |

## 5. Interaction and quality requirements

<a id="m14-ir01"></a>
**M14-IR01 — Caller and interaction requirement (Proposed).** Expose active plan, target/tactic choice, replay step/round and play/pause state in text as well as graphics. Focus after nested rename and keyboard-only replay require the S05 accessibility matrix.

<a id="m14-ir02"></a>
**M14-IR02 — Feedback and recovery (Proposed).**
Use the success, no-change, cancellation and failure distinctions in §3. The
owning caller must not imply that a forecast, accepted request or attempted write
is a confirmed final result. Section 3 records current outcomes and their limitations; this requirement does
not establish implementation. Complete keyboard/focus/error-announcement acceptance remains Open with the
caller and S05. No accessibility conformance level is asserted.

<a id="m14-qr01"></a>
**M14-QR01 — Integrity and compatibility (Observed constraints).**
Preserve plan indices during deletion and immutable recorded outcomes during review. Playback timing is an observed requested interval, not a measured frame-rate guarantee; latency/display targets remain Open.

Scenario/check: source/model boundary review and the native regression/build
checks below. Target: preserve the specified existing results and file/catalog
contracts; any intentional runtime change is separate work. Evidence: named
definitions, source pointers and dated checks; uncovered adapters are in §6.

<a id="m14-qr02"></a>
**M14-QR02 — Performance and resource budgets (Open).**
Scenario: the largest supported workload for these workflows on each supported
backend. Method: measure wall time, peak allocation and caller-visible response
time with controlled inputs. Target: no agreed numeric budget or new measurement
in this specification batch. Owner: M14 and the consuming module/platform owner;
absence of a budget does not establish responsiveness.

<a id="m14-qr03"></a>
**M14-QR03 — Text and presentation (Observed boundary / Open qualification).**
Use UTF-8 byte capacities and complete-code-point truncation from
[S04](S04-platform-services.md) and [Unicode boundaries](../docs/UNICODE-BOUNDARIES.md).
Native controls and rendering use [S05](S05-sdl-desktop-implementation.md);
headless/internal APIs have no independent viewport. Check non-ASCII boundary
inputs and the actual caller on supported displays. Target: valid text and the
stated capacities; complete font, large-text, IME and assistive-technology coverage
remains Open with S05 and the caller.

## 6. Coverage inventory and open decisions

The definitions named in §§3–4 are canonical for their stated projections.
The following inventory accounts for rules requiring native inputs/effects or
further decomposition; it is not a claim that their algorithms are proved.

| Boundary | Scope and limitation | Owner / next evidence or decision |
| --- | --- | --- |
| E14-1 | Plan copy naming, rename cancellation after copy, deletion warning/log order and roster/fleet adapters need a complete UI/state trace. | M14/M10/S01; populated-plan delete/decline and copy/cancel fixtures. |
| E14-2 | Record traversal, token reconstruction, kill/damage aggregation, rendering, timer cleanup and missing/corrupt-record recovery consume packed S01 data and native objects. | M14/S01/S05; replay round-trip and lifetime fixtures; current model covers control bounds only. |

## 7. Evidence and verification

[battle-plan UI](../src/ui/battleui.c), [replay](../src/ui/vcr.c), [battle units](../tests/unit/test_battle.c) and [Continue workflows](../tests/compat/win32_client_startup.c). Unit combat tests are not exhaustive replay or plan-deletion tests.

[Collection verification](README.md#remaining-specifications-verification)
records exact commands and outcomes for this batch. Proofs establish only the
named model properties and witnesses, with their preconditions; they do not prove
that C refines the models or that every UI/platform workflow has been tested.
The boundary inventory and Open interaction/quality requirements remain visible
after a successful formal/build/regression run.
