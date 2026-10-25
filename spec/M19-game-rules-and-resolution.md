# M19: Game Rules and Resolution

Status: source-derived detailed specification; logical ownership is proposed.
Inspected: `e2eae18cff0f990db20415c1c2262aed2022f95b`, 2026-10-24.

Canonical entry: [M19-resolution-specification.dfy](M19-resolution-specification.dfy);
[rules and proofs](dafny/M19-resolution.dfy). Behavior statuses and evidence follow
[the collection conventions](README.md#requirement-ids-and-status).

## 1. Purpose, actors and scope

Own the shared resolution surface while retaining specialized rules in their
existing owners. This provisional group accounts for every major host rule
family; it is not a second production, research or fleet algorithm.
[M18](M18-turn-generation.md) fixes when effects occur. M08/M09/M10/M11/M12/M13
own colony, production, fleet, design, research and relation contracts; M14 owns
plans/replay rather than combat simulation. S06 state, S03 catalogs and S01
persistence remain independent. Detailed inventory boundaries below identify
native algorithms still requiring stronger models and dedicated qualification.

Primary actors: Players whose plans become game outcomes, and hosts running the authoritative simulation.

### User stories

These source-derived stories describe intent, not evidence of user research.
The prerequisites and triggers are the corresponding workflows below. All
stories describe Observed capabilities with the stated limitations.

<a id="m19-us01"></a>
**M19-US01 — Resolve colony economy.** As a player, I want growth, mining and construction to follow my colony’s conditions so that economic planning is meaningful.

<a id="m19-us02"></a>
**M19-US02 — Resolve encounters.** As a player, I want movement, minefields and combat to use the relevant designs, relations and orders so that conflict has explainable outcomes.

<a id="m19-us03"></a>
**M19-US03 — Resolve tasks and objects.** As a player, I want colonization, transports, packets and universe objects resolved in the defined phases so that my orders affect the next year.

<a id="m19-us04"></a>
**M19-US04 — Preserve reproducibility.** As a host, I want the same complete input state to reproduce results so that changes can be detected and failures diagnosed.

### Story entry and completion conditions

| Story | Trigger and prerequisites | Completion / alternate outcome |
| --- | --- | --- |
| [M19-US01](#m19-us01) | Enter an economic phase with an owned populated colony and trait/capacity inputs. | Apply the appropriate normal/hostile/crowding/mining branch and owner-specific effects. |
| [M19-US02](#m19-us02) | Reach movement/combat with relevant fleets, plans and relations. | Apply eligibility then native encounter resolution; losses/events become host state. |
| [M19-US03](#m19-us03) | Execute an order/object phase with current target and object state. | Apply task-specific success/partial/skip/failure effects; changed targets can invalidate plans. |
| [M19-US04](#m19-us04) | Run a controlled regression with full state/catalog/RNG/platform inputs. | Compare decoded outputs and identify the first divergence without changing the frozen oracle. |

## 2. Inputs, outputs and state

| Area | Contract |
| --- | --- |
| Authoritative state | Host players, colonies, designs, fleets, space objects, orders, relations and race traits; player knowledge is a projection. |
| Numeric domains | Population/cargo hundredths and stored units, mineral concentration/depletion carry, resources, fuel, damage, coordinates and binary64 intermediates. |
| Effects | State mutation, messages, battle records, resource/technology changes, ownership changes and derived visibility. |
| Randomness/order | Shared RNG stream, sorted iteration and phase-local order are inputs to outcomes, not incidental implementation details. |

Persistent records remain [S01](S01-game-file-formats.md); logical state and
identity remain [S06](S06-shared-game-state.md). A derived/working value must not
be treated as a separately committed record or as information known to every player.

## 3. Workflows, rules and handoffs

### Economy: canonical rules and domains

`EstMineralsMined` in [mine.c](../src/core/mine.c) uses the local sentinel `cMines=-1`
to compute operating mines. An unowned/empty local colony produces the sentinel
triple (-1,-1,-1), not three zero yields. For a productive colony, the `Concentration`
rule applies a homeworld floor of 30 except non-AR remote mining; `MiningScaled`
retains local efficiency division order and remote mining's direct product.
`Yield` uses the integer quotient plus one only when generation is active and a
0–99 draw is below the nonzero remainder. Forecasts omit that random increment.
The modeled domain excludes native overflow/cast effects (E19-1).

Applying mining credits stocks and depletes concentration using the unadjusted
mines/concentration product, not efficiency-adjusted production. `DepletionBand`
and `DepletionLeft` specify the band and stored-zero-as-256 calculation. The
subsequent concentration reduction/fractional carry loop and AR remote fleets
are E19-1; these definitions alone do not specify the entire mine depletion loop.

`ChgPopFromPlanet` in [util.c](../src/core/util.c) returns zero for unowned/empty colonies.
For populated colonies, hostile habitat uses `HostileLoss`; non-hostile growth
uses `GrowthAt`, given the owner's capacity and growth/habitat-derived rate.
`CrowdedRate` preserves quarter-capacity cutoff, quadratic crowding, truncation
order, maximum overcrowding loss and the `[capacity, capacity+10)` no-change
band. `Carry` preserves signed C division and normalized hundredths 0–99;
`CarryBound` proves that range. These functions require non-overflowing native
inputs, positive population/capacity for the non-hostile branch and an existing
valid remainder. Owner/zero checks, death handling and optional writeback remain
E19-1. The rate/capacity definitions are reused from M08, not independently set.
Production payment/completion is [M09](M09-production-management.md); research
allocation/breakthroughs are [M12](M12-research-and-technology.md).

### Complete resolution ownership inventory

| Rule family | Entry/effect and authoritative specification | Remaining native obligation |
| --- | --- | --- |
| Planet economy | `ChgPopFromPlanet`, `EstMineralsMined`, `Produce`; M08/M09 and the arithmetic above | E19-1 |
| Movement, fuel, following and gates | `MoveFleets`, movement helpers, `ship2.c`; [M10](M10-fleet-operations.md) | E19-2: route execution, hazards, arrival and packed damage |
| Orders and cargo | `SatisfyOrders`, `DropColonists`, `TransferToOthers`; M10/S01 task/transport discriminators | E19-3: mode-specific effects, conflicts and partial transfers |
| Production, technology and designs | `Produce`, `CBuildProdItem`, `UpdateResearchStatus`; M09/M12/M11 | Their execution/breakthrough/deletion boundaries |
| Battle participation and targeting | `DoBattles`, `FAttackPlayer`; M13 relation/attack policy and M14 plans | E19-4: battle setup, rounds, targeting and retreat |
| Weapons, losses and battle record | `battle.c`, damage helpers and S01 battle serialization | E19-4: initiative, movement, hit/kill and salvage ordering |
| Bombing, invasion and ownership | `battle.c`, `DropColonists`, task handlers; M08/M10/S06 | E19-3/E19-4: defenses, ground combat, extinction and messages |
| Minefields | `MoveFleets`, `SweepForMines`, `UnmarkMineFields`, `mine.c` | E19-2: detection, laying/decay, intersection RNG, losses and sweeping |
| Packets, wormholes and trader | `MoveThings`, `ThingDecay`, `DoThingInteractions`, `thing.c` | E19-5: object-specific movement/collisions/events |
| Transit growth, repair and terraforming | `BreedColonistsInTransit`, `HealShips`, `AutoTerraform`, `RemoteTerraforming`; M08/M10/M13 | E19-1/E19-2: per-fleet/planet mutation and order |
| Knowledge, score and victory | `UpdateGuesses`, `UpdatePlayerScores`; S06 knowledge, [M03 scoring/victory](dafny/M03-scoring.dfy), M15 presentation | E19-6: actual visibility/score/end-condition evaluation |
| Race/trait effects and random events | `SetRaceStat` and consumers across these families; M04 trait data | E19-1–6: exact contextual effects, no blanket trait equivalence |

`WormholeMove` specifies the bounded movement magnitude from last movement and
stability, including clamp 0–6; it does not specify displacement coordinates,
re-pairing or discovery. Each family uses source state and consumes randomness
at its actual phase, not an independent per-rule stream. S03 owns numeric catalog
values; S04 owns native arithmetic/rounding adapters.

### Failure, cancellation and observable outcomes

Planning acceptance does not promise host success: target ownership, availability,
relations or resources can change before resolution. Apply the owning task's
skip, partial-success, abort and message behavior, preserving earlier effects.
This entry does not impose an all-or-nothing simulation transaction. M18 handles
load/allocation/replay failure and cleanup; S01 handles output failure; M17 owns
recovery. There is no generic player cancellation once host execution starts.
M15 exposes messages/reports; M14 replays recorded battles without re-resolving
them. Any intentional rule change needs its own code change, changelog and moved
native regression baseline; this specification changes no gameplay.

## 4. Acceptance and traceability

All criteria below have **Observed** behavior status except where a rule is
explicitly a caller convention or repository maintenance constraint. A named
model/lemma supplies mathematical evidence within its preconditions. An E-boundary
supplies source evidence and an explicit implementation-test/refinement gap,
not a proved runtime outcome. The dated collection run records actual checks.

| Criterion / story | Given / When / Then | Authoritative rule, source and evidence scope |
| --- | --- | --- |
| <a id="m19-ac01"></a>**M19-AC01** / [M19-US01](#m19-us01) | Given homeworld concentration 1, when locally mining, then use 30; ordinary non-AR remote mining uses 1. | `Concentration`, vectors; EstMineralsMined |
| <a id="m19-ac02"></a>**M19-AC02** / [M19-US01](#m19-us01) | Given scaled yield 1234, when generating with draw 33, then produce 13; draw 34 or a forecast produces 12. | `Yield`, `YieldBound`, vectors |
| <a id="m19-ac03"></a>**M19-AC03** / [M19-US01](#m19-us01) | Given signed hundredths -1 and carry 0, when normalizing growth, then produce change -1 and remainder 99. | `Carry`, `CarryBound`, vectors |
| <a id="m19-ac04"></a>**M19-AC04** / [M19-US01](#m19-us01) | Given population capacity+5 and valid carry, when calculating non-hostile growth, then preserve carry with zero change. | `GrowthAt`, vectors |
| <a id="m19-ac05"></a>**M19-AC05** / [M19-US02](#m19-us02) | Given a fleet encounter, when selecting attack eligibility, then use directed relations and its battle policy before resolving weapons. | M13 `Attack`; E19-2/E19-4 |
| <a id="m19-ac06"></a>**M19-AC06** / [M19-US03](#m19-us03) | Given extreme wormhole movement inputs, when computing magnitude, then clamp to 0–6; retain object-specific movement as an explicit dependency. | `WormholeMove`, vectors; E19-5 |
| <a id="m19-ac07"></a>**M19-AC07** / [M19-US04](#m19-us04) | Given a regression scenario and complete controlled state, when running current generation, then compare decoded checkpoints to the native baseline and report the first unexpected difference. | E19-6; regression harness; S06 RNG |

## 5. Interaction and quality requirements

<a id="m19-ir01"></a>
**M19-IR01 — Caller and interaction requirement (Proposed).** Host rules accept game state and return mutations/events, with no direct keyboard or viewport contract. M08–M16 own explanations, selection and accessible outcome presentation. Explanations must distinguish forecast, known state and actual host result; comprehensive explanations of every hidden/random effect remain Open.

<a id="m19-ir02"></a>
**M19-IR02 — Feedback and recovery (Proposed).**
Use the success, no-change, cancellation and failure distinctions in §3. The
owning caller must not imply that a forecast, accepted request or attempted write
is a confirmed final result. Section 3 records current outcomes and their limitations; this requirement does
not establish implementation. Complete keyboard/focus/error-announcement acceptance remains Open with the
caller and S05. No accessibility conformance level is asserted.

<a id="m19-qr01"></a>
**M19-QR01 — Integrity and compatibility (Observed constraints).**
Preserve integer units, signed division, storage widths, binary64 boundaries, iteration order and exact RNG consumption. No probabilistic fairness or strategic-balance claim is proved. Performance and complete rule-family test coverage remain measurable Open work for the named owners.

Scenario/check: source/model boundary review and the native regression/build
checks below. Target: preserve the specified existing results and file/catalog
contracts; any intentional runtime change is separate work. Evidence: named
definitions, source pointers and dated checks; uncovered adapters are in §6.

<a id="m19-qr02"></a>
**M19-QR02 — Performance and resource budgets (Open).**
Scenario: the largest supported workload for these workflows on each supported
backend. Method: measure wall time, peak allocation and caller-visible response
time with controlled inputs. Target: no agreed numeric budget or new measurement
in this specification batch. Owner: M19 and the consuming module/platform owner;
absence of a budget does not establish responsiveness.

<a id="m19-qr03"></a>
**M19-QR03 — Text and presentation (Observed boundary / Open qualification).**
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
| E19-1 | Full mine depletion/AR remote loops; population death/writeback; economic trait interactions and exact finite-width/binary64 behavior outside the modeled safe arithmetic domain. | M19/M08/M09/M12/S04; boundary fixtures, overflow-domain review and per-phase refinement. |
| E19-2 | Complete movement, gates, minefields, fuel, repair, transit growth and packed per-ship damage algorithms. Existing M10 formulas cover selected projections, not every host branch. | M19/M10; route/hazard/repair state fixtures with controlled RNG. |
| E19-3 | Every mode-specific task, invasion/drop, transfer, colonization and transport state mutation including contention and changing targets. S01 owns command bytes, not successful execution. | M19/M10/M08; one fixture per task/mode/outcome and invalidated target. |
| E19-4 | Combat rounds, targeting/initiative, beam/torpedo/bomb calculations, damage allocation, retreat, destruction and battle recording use mutable arrays, sort order and floating boundaries. | M19/M14/S04; full combat-state model and record/state comparison; native regression is partial coverage. |
| E19-5 | Packet/trader/wormhole/salvage movement, decay, interactions and random events beyond the bounded magnitude rule. | M19/S06; object-type/phase/RNG fixtures and explicit decomposition decision. |
| E19-6 | Full knowledge projection, score/victory input extraction and integration, and global reproducibility across platforms. Existing M03 scoring/victory formulas remain canonical; their model proofs are not native refinement or an exhaustive simulation proof. | M19/S06/M15/M03; visibility/score/victory vectors and expanded regression coverage. |

## 7. Evidence and verification

[resolution source](../src/core/turn.c), [economy/research](../src/core/turn2.c), [task resolution](../src/core/turn3.c), [combat](../src/core/battle.c), [mining](../src/core/mine.c), [objects](../src/core/thing.c), [population arithmetic](../src/core/util.c), and [known bugs](../docs/KNOWN-BUGS.md). Existing source behavior, including known bugs, is not an endorsement of a new rule.

[Collection verification](README.md#remaining-specifications-verification)
records exact commands and outcomes for this batch. Proofs establish only the
named model properties and witnesses, with their preconditions; they do not prove
that C refines the models or that every UI/platform workflow has been tested.
The boundary inventory and Open interaction/quality requirements remain visible
after a successful formal/build/regression run.
