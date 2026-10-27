# M19 — Construction eligibility and design-check handoff

Status: observed source behavior reviewed 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the working specification collection.
This companion specializes [M19-US01](M19-game-rules-and-resolution.md#m19-us01)
and [M19-US03](M19-game-rules-and-resolution.md#m19-us03), consuming
[M11 design buildability](M11-design-buildability.md) for
[M09 host application](M09-production-execution.md). The host actor applies
already charged work; a player observes notices and resulting planet/fleet state.
M19 retains criteria; M11 owns eligibility, M09 charging/queue response, S03
catalog identities, S06 native pointer/state lifetimes, S01 admitted records and
M18 annual sequencing. Message delivery, UI/focus/accessibility/display and
unagreed timing/memory targets retain M15/S05 and the consuming entries' owners.

Canonical [M19ConstructionBuildability](dafny/M19-construction-buildability.dfy)
composes the existing M11 `Check` with the fleet branch's guard/roster/metadata
steps. `ReadyUsesRoute` connects it to the existing
[construction Route](dafny/M19-construction-effects.dfy). A Ready result means
continue application; fleet limits, allocation and subsequent effects can still
prevent an object. It is not another definition of successful construction.
[Collection evidence](README.md#construction-eligibility-integration-continuation)
records actual checks. No runtime behavior, file-format or policy change is made.

Sources: [turn2.c](../src/core/turn2.c) `FBuildObject`, `Produce`, `CreateShip`;
[util.c](../src/core/util.c) `FCanBuildShdef`;
[parts.c](../src/core/parts.c) `FLookupPart`, `TechStatus`, `LphuldefFromId`,
`LphuldefSBFromId`; [research.c](../src/core/research.c) hidden-part logic;
[race.c](../src/core/race.c) race getters; [file.c](../src/core/file.c)
`FReadShDef`; native structures and fresh S03 catalogs.

## Interface and projected state

The specified entry is FBuildObject's fleet-category branch. Item and saved
global identity are signed words. Native `PLANET.iPlayer` is a signed-16
in-memory field; S01's packed owner widths are a separate boundary. Ship/base
roster pointers form sixteen-entry arrays. A live roster supplies sixteen ship
or ten base SHDEF entries; `Missing` describes a missing pointer. `Entry` carries
free metadata, the M11 design projection and stored cargo.

`RostersFit` describes these physical projections and native field widths. It
does not establish active-game ownership, pointer liveness/aliasing or file/UI
reachability. `Gate` returns a stage, known global-player prefix, optional exact
M11 outcome and ordered roster/design/availability/metadata/notice events.
Undefined stages mark a first native read-domain boundary; they introduce no
native check, FALSE result, recovery, rollback or defined C post-state after UB.
FailureNotice is a planned call attempt: Gate's saved global at that point is
the eligibility context before message-helper effects. Applying it as a complete
caller-return frame also needs the actual message return/post-state contract.
The native fixture supplies a returning context-preserving message provider;
M15 delivery, mutation and escape qualification remains explicit.

## Guard and lookup order

1. For items below 16, test the planet's starbase flag first. Missing base returns
   FALSE with no failure notice, roster read or checker call. Negative items and
   arbitrary owner words also take this defined early return. `NoBaseSkipsOwner`
   proves the model's guard order; the native matrix exercises every owner word.
2. Otherwise choose the owner's ship or base roster; base items subtract 16.
   Invalid owner, missing roster and reached out-of-array design indices are
   separate boundaries. An unowned target (-1) cannot reach the checker's
   headless mode through this caller: the roster access happens first.
3. Read the chosen entry's free flag. A free entry skips M11 entirely.
   `FreeSkipsCheck` proves this, preserving the saved global. Ship rejection
   attempts its failure notice; base rejection returns without that notice.
4. For a nonfree entry, call the canonical M11 Check under the planet owner.
   `Gate.check` is that exact outcome, not a supplied availability boolean.
   Ordinary checker completion restores the saved global. An undefined checker
   read retains its earlier selected-player/probe prefix without a caller notice.
5. An ordinary false checker result rejects, using the same ship/base notice
   distinction. An ordinary true result reaches ShipReady or proceeds through
   the base notification's separate hull-metadata read before BaseReady.

The existing Route owns item/branch classification. Ready stages refine its ship
or base result, while this companion adds the earlier owner/pointer/free/check
ordering. M11 preserves raw signed-hull/byte-item behavior and short-circuit slot
traversal; this caller adds no general design or compatibility validator.

## Availability and notification use different hull projections

`FCanBuildShdef` compares the hull as signed-16, then assigns a byte lookup item.
The base notification instead calls `LphuldefFromId` with the signed word and
uses its full index. `HullMetadata` records that separate address selection:
nonnegative IDs below 32 select ship rows; 32..36 select the five base rows;
negative or larger IDs reach an out-of-array metadata domain.

For example, raw hull 288 can pass an available base-item-zero check after byte
narrowing, then reach metadata base index 256. Raw 32768 can check ship item zero
but later use a negative signed metadata index. These are direct raw-input
boundaries, not confirmed normal-game reachability or new accepted design IDs.
`Gate` retains the completed check and restored global before the metadata
boundary, with no attempted built notice after that argument read.

Current `FReadShDef` rejects hull IDs above `ihuldefDeathStart` (36).
`DecodedHullSafe` proves metadata address safety under that actual upper-bound
condition; the native fixture binds the bound to base offset plus generated base
count. This one condition is not proof of the complete decoder, every state
writer, roster kind, owner or runtime lifetime. Those remain S01/M11/S06/M18
integration work. The [gap register](README.md#gap-20) retains this distinction.

## Base notification data and narrowing

`BaseNotice` composes the existing BaseMessage variant rule with the catalog
argument's signed-word assignment. Stored design cargo selects offset 0 for
zero, 1 for other values, or 2 for 65535. The payload comes from the separately
selected catalog hull's cargo field, not that stored cargo. Its uint16 value is
narrowed through the message's signed-16 parameter: 65535 arrives as -1 with the
same low sixteen bits. MetadataVectors cover independent variant/payload values.
A notice event means an attempted source call; real packaging, delivery and
presentation remain M15 obligations.

## Ready eligibility, application and charged work

[Construction effects](M19-construction-effects.md) retain base replacement,
queue invalidation, counters and ship-stack behavior after the gate. `Gate`
does not promise an allocation, fleet slot, registry entry, route or full object.
The exact 512-fleet branch can fail after a true actual M11 check when no eligible
existing fleet is found. [M09 AfterApplication](M09-production-execution.md#application-result-and-queue-control)
then handles the failed application and captured queue row, without undoing the
builder's charges. Larger or malformed fleet counts have their existing source
rules; this companion does not normalize them into the 512 case.

The composed native full-Produce case supplies an owned 512-entry fleet roster
with no matching first-order point, an available design and a one-ship queue.
Actual builder charging, real eligibility and real application reject at the
fleet limit, remove the queue work, leave design counts unchanged and retain
90 of the original 100 resource units for research after a cost of ten. Quote,
economy and message services remain declared providers; saved-game/annual/native
allocator and all-helper refinement is not inferred from that one composition.

## Native evidence and remaining qualification

[Integration fixture](../tests/spec/check_construction_buildability_source.py)
compiles the complete actual FBuildObject/Produce/builder/stack/queue bodies,
actual FCanBuildShdef, lookup/trait/hidden/technology functions, both actual hull
metadata helpers and fresh generated tables. The original
[construction fixture](../tests/spec/check_construction_effects_source.py)
shares its source builder and is rerun; the
[availability source builder](../tests/spec/check_part_availability_source.py)
shares its typed catalog selector and retains its full matrix.

Native cases include every signed item with/without a base, every owner word on
the no-base path, owner/roster/free alternatives, raw hull words, trait/technology/
quantity/cargo variations, exact fleet-limit cases and seeded contexts. Exact
checker requests and getter order, ordinary restoration, messages/payloads,
actual catalog pointers and complete expected design/player/game/planet byte
frames are checked. The local hull quantity/prior pointer remains unobserved
until initialized, as required by the source checker.

Undefined owner/roster/design/check/metadata candidates are excluded before C,
not handled as a new source error return. Compiler warnings and ASan/UBSan are
fatal, retaining only existing unused-resource-parameter and pointer-sign source
exclusions. Remaining providers include economy, warp, messages, low-level
allocation and fleet registration/fuel/routing; their full contracts, native
array/lifetime/alias and real annual/caller qualification remain with the
existing owners. No game source, catalogs, saved layouts or baselines change.
