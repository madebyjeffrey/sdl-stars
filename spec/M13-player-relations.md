# M13: Player Relations

Status: source-derived detailed specification; logical ownership is proposed.
Inspected: `e2eae18cff0f990db20415c1c2262aed2022f95b`, 2026-10-24.

Canonical entry: [M13-relations-specification.dfy](M13-relations-specification.dfy);
[rules and proofs](dafny/M13-relations.dfy). Behavior statuses and evidence follow
[the collection conventions](README.md#requirement-ids-and-status).

## 1. Purpose, actors and scope

Maintain directed neutral/friend/enemy stances and explain which rules consume
them. A player's chosen friendship is not a reciprocal agreement. M14 owns
battle-plan attack policy; M19 owns resulting combat/gate/mine/transfer rules;
M15 reports outcomes and M07 displays relationship cues. S06 owns the relation
array and S01 its order encoding. This entry does not create a negotiation,
alliance treaty or messaging service.

Primary actors: A player setting its own stance toward another player in a multiplayer turn.

### User stories

These source-derived stories describe intent, not evidence of user research.
The prerequisites and triggers are the corresponding workflows below. All
stories describe Observed capabilities with the stated limitations.

<a id="m13-us01"></a>
**M13-US01 — Choose a stance.** As a player, I want to select another player and mark neutral, friend or enemy so that my orders reflect my intentions.

<a id="m13-us02"></a>
**M13-US02 — Understand directed effects.** As a player, I want to know whose stance applies so that I do not assume another player reciprocates friendship.

<a id="m13-us03"></a>
**M13-US03 — Retain and deliver changes.** As a player, I want edited relations saved with my turn so that the host uses them on generation.

### Story entry and completion conditions

| Story | Trigger and prerequisites | Completion / alternate outcome |
| --- | --- | --- |
| [M13-US01](#m13-us01) | Choose a different player in a loaded multiplayer session. | Update only the directed selected stance; close retains live edits. |
| [M13-US02](#m13-us02) | Evaluate an encounter/access rule with both directed relation rows. | Consult the stance of the owner named by that rule, not an inferred treaty. |
| [M13-US03](#m13-us03) | Close/edit-save-submit after a live relation change. | Log pending relations; M06/S01 own confirmed persistence and host replay. |

## 2. Inputs, outputs and state

| Area | Contract |
| --- | --- |
| Relations | `rgplr[self].rgmdRelation[other]`: neutral 0, friend 1, enemy 2; directed by row owner. |
| Selection | The player list excludes self; UI row and player ID are different. |
| Dirty state | Radio commands mutate live relations and mark dirty; destruction logs current relations. |
| Consumers | Battle-plan target mode and the relevant owner’s stance determine each interaction separately. |

Persistent records remain [S01](S01-game-file-formats.md); logical state and
identity remain [S06](S06-shared-game-state.md). A derived/working value must not
be treated as a separately committed record or as information known to every player.

## 3. Workflows, rules and handoffs

### Edit, select and close

[battleui.c](../src/ui/battleui.c) `RelationsDlg` lists all other players.
`PlayerAt` skips self when converting a selected row to a player index. Selecting
a row changes the displayed radio state. Selecting a radio writes the current
player's relation immediately and sets `fDirtyPlan`; `Set` models the one-entry
mutation. Closing through IDCANCEL returns the selected player code; it does
not restore an entry snapshot. On `WM_DESTROY`, `LogChangeRelations` records the
relations and the scanner is invalidated. Missing selection and one-player
invocation are UI-precondition cases E13-1, not validated by PlayerAt alone.

### Directed use in game rules

`FAttackPlayer` in [battle.c](../src/core/battle.c) evaluates the attacking
fleet's plan: nobody, enemies, enemies plus neutrals, everyone or an explicitly
selected player. `Attack` is its canonical predicate. Friendship blocks mode 2,
neutrality blocks enemies-only mode 1, and an explicit player target can select
a friend. This predicate is one prerequisite, not proof that a battle occurs.
Combat assembly has additional presence, target and initiative rules.

Gate access in [turn.c](../src/core/turn.c) consults source/destination planet
owner relations toward the traveler. Minefield interaction consults field-owner
stance; remote terraforming in [turn2.c](../src/core/turn2.c) uses the operating
fleet owner's stance to choose help/harm. Ownership transfer in
[turn3.c](../src/core/turn3.c) has receiver hostility/AI restrictions. These
consumers are individually owned E13-2 contracts; they must not be collapsed into
one symmetric `allied(a,b)` rule. No automatic reciprocal update occurs in the
dialog.

### Persistence and recovery

Live edits are pending player state. M06 save/submission and S01 replay govern
publication and failure; closing the relations dialog neither sends network
messages nor generates a turn. Reopening reflects the edited state. To reverse
a live edit the player selects the prior stance again or uses the session's
explicit reload/discard behavior. Reload has a larger scope than this dialog.

## 4. Acceptance and traceability

All criteria below have **Observed** behavior status except where a rule is
explicitly a caller convention or repository maintenance constraint. A named
model/lemma supplies mathematical evidence within its preconditions. An E-boundary
supplies source evidence and an explicit implementation-test/refinement gap,
not a proved runtime outcome. The dated collection run records actual checks.

| Criterion / story | Given / When / Then | Authoritative rule, source and evidence scope |
| --- | --- | --- |
| <a id="m13-ac01"></a>**M13-AC01** / [M13-US01](#m13-us01) | Given self player 2 and list row 2, when selecting, then edit player 3; self is never the mapped target. | `PlayerAt`, `SelfExcluded`; RelationsDlg |
| <a id="m13-ac02"></a>**M13-AC02** / [M13-US01](#m13-us01) | Given a relation array, when changing one other player, then preserve every other entry. | `Set`, `OtherRelationsPreserved` |
| <a id="m13-ac03"></a>**M13-AC03** / [M13-US02](#m13-us02) | Given a friendly target and enemies-plus-neutrals plan, when evaluating attack intent, then reject; explicit target mode can select it. | `Attack`, vectors; FAttackPlayer |
| <a id="m13-ac04"></a>**M13-AC04** / [M13-US02](#m13-us02) | Given only A marks B friendly, when applying a B-owned access rule, then use B’s stance, not infer reciprocity. | E13-2; gate/mine consumers |
| <a id="m13-ac05"></a>**M13-AC05** / [M13-US03](#m13-us03) | Given a radio edit, when closing, then retain it and log on destruction; ordinary close is not undo. | E13-1/S01; RelationsDlg, LogChangeRelations |

## 5. Interaction and quality requirements

<a id="m13-ir01"></a>
**M13-IR01 — Caller and interaction requirement (Proposed).** Keep selected player and stance clear through keyboard/radio changes; disclose direction in explanatory text. Accessible relationship cues beyond color and a complete one-player/no-selection audit remain Open.

<a id="m13-ir02"></a>
**M13-IR02 — Feedback and recovery (Proposed).**
Use the success, no-change, cancellation and failure distinctions in §3. The
owning caller must not imply that a forecast, accepted request or attempted write
is a confirmed final result. Section 3 records current outcomes and their limitations; this requirement does
not establish implementation. Complete keyboard/focus/error-announcement acceptance remains Open with the
caller and S05. No accessibility conformance level is asserted.

<a id="m13-qr01"></a>
**M13-QR01 — Integrity and compatibility (Observed constraints).**
Preserve directed entries and S01 replay widths. M13/S05 owns unmeasured update latency and the full multilingual/focus matrix; no treaty or reciprocity guarantee is added.

Scenario/check: source/model boundary review and the native regression/build
checks below. Target: preserve the specified existing results and file/catalog
contracts; any intentional runtime change is separate work. Evidence: named
definitions, source pointers and dated checks; uncovered adapters are in §6.

<a id="m13-qr02"></a>
**M13-QR02 — Performance and resource budgets (Open).**
Scenario: the largest supported workload for these workflows on each supported
backend. Method: measure wall time, peak allocation and caller-visible response
time with controlled inputs. Target: no agreed numeric budget or new measurement
in this specification batch. Owner: M13 and the consuming module/platform owner;
absence of a budget does not establish responsiveness.

<a id="m13-qr03"></a>
**M13-QR03 — Text and presentation (Observed boundary / Open qualification).**
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
| E13-1 | Native selected-row guards, live dirty/log lifecycle, close return code and UI feedback remain adapters; mapping and one-entry update are formal. | M13/S05/S01; reopen, no-selection and save/replay scenarios. |
| E13-2 | Every diplomacy consumer retains its own direction and additional conditions: battles, gates, minefields, remote terraforming and transfer. | M13/M14/M19; directed truth-table integration fixtures, not a generic alliance assumption. |

## 7. Evidence and verification

[relations dialog](../src/ui/battleui.c), [battle rules](../src/core/battle.c) and the [Continue workflows](../tests/compat/win32_client_startup.c) exercise ordinary relation editing. Broad regression is indirect evidence for consumer behavior.

[Collection verification](README.md#remaining-specifications-verification)
records exact commands and outcomes for this batch. Proofs establish only the
named model properties and witnesses, with their preconditions; they do not prove
that C refines the models or that every UI/platform workflow has been tested.
The boundary inventory and Open interaction/quality requirements remain visible
after a successful formal/build/regression run.
