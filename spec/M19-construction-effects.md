# M19 — Applying completed construction

Status: observed behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion supports [M19-US01](M19-game-rules-and-resolution.md#m19-us01),
[M19-US03](M19-game-rules-and-resolution.md#m19-us03) and
[M19-US04](M19-game-rules-and-resolution.md#m19-us04). Fleet-stack rules also
support [M10-US01](M10-fleet-operations.md#m10-us01) and
[M10-US05](M10-fleet-operations.md#m10-us05); queue/application handoffs retain
[M09-US05](M09-production-management.md#m09-us05). Criteria stay at those owners.

Canonical definitions are [M19 effects](dafny/M19-construction-effects.dfy),
[M10 construction-stack damage](dafny/M10-construction-damage.dfy) and
[M09 queue invalidation](dafny/M09-construction-queues.dfy). They specialize
the [builder](M09-production-builder.md) and [host caller](M09-production-execution.md)
contracts. They do not replace player fleet merging, packet movement, combat
damage, capacity producers, catalog eligibility or routing algorithms.
The [M08 native economy companion](M08-colony-economy.md) now owns capacity and
installation producers; construction effects consume their signed-word limits.
The [native mass-driver companion](M08-mass-driver-capability.md) now supplies
actual old/new capability results to `DesignFling`, which reuses `BaseFling`;
returning-helper state preservation and full caller frames remain explicit.
[Native terraforming selection](M08-terraform-selection.md) now supplies actual
Can/Best/habitat/remote/optimum rules; SelectTerraform and RunTerraform reuse
the existing step/count semantics for repeated application.

The driver integration fixture also runs the real design-check/availability/
catalog helpers together with actual base/packet construction and Produce.
Its [evidence](README.md#mass-driver-capability-continuation) supplements the
controlled-helper matrix described below; full native/annual qualification
remains open.

Sources: [turn2.cpp](../src/core/turn2.cpp) `FBuildObject`, `CreateShip`, `Produce`;
[build.cpp](../src/core/build.cpp) `KillQueuedShips`, `KillQueuedMassPackets`;
[thing.cpp](../src/core/thing.cpp) `LpthNew`; [utilgen.cpp](../src/core/utilgen.cpp)
`Random`; [structs.h](../src/core/structs.h) native planet, design, fleet,
damage, order and thing layouts; [enums.h](../src/core/enums.h) type/item IDs.
[Verification](README.md#construction-application-continuation) records actual
proof, source-fixture and repository results.

## Interface, outcome and ownership

The actor is the host applying already charged construction for a player.
`FBuildObject` receives a live planet, unsigned-16 category, signed-16 item and
quantity, and a resource pointer. Ordinary `Produce` calls it only for positive
built counts. Direct-call behavior for other native signed quantities is
distinguished from a supported player order. The body does not directly read or
write its resource-pointer parameter. It does modify planet fields and invokes
helpers with their own side effects; those effects are not an atomic transaction.

`Route` follows the initial branch order. Fleet items 16..25 select ten base
slots; 0..15 select ship slots only after the starbase test. A negative ship item
with no base rejects before design access; with a base it reaches an invalid
array index. Planet items outside 0..27 reject; other categories reject. Formal
`InvalidDesign` and undefined arithmetic/axis outcomes are source-domain markers,
not newly implemented validation errors. Active player/design/pointer/storage
validity must be established where those values are actually dereferenced.

Native widths matter: installations 12 bits, scanner 5, planet fling target 10,
fling warp 4, fleet stacks signed 16, design counts unsigned 32, damage share 7
and damage fraction 9, packet minerals signed 16, packet weight 14, decay 2 and
warp 4. Signed narrowing and undefined signed arithmetic are separate. Shared
S01 word helpers and the existing native conversion functions are reused;
proposed S06 strict-game invariants do not silently validate every native input.

This is a headless effect contract. Caller messages/next-turn observations are
the player interface; keyboard, focus, accessibility, presentation and unagreed
latency/display targets remain with M09/M10/M15/S05. No cancellation occurs
inside the application routine. Escaping callbacks retain earlier effects and
have no general local rollback guarantee.

## Planetary installations and no-op items

`Install` computes signed-16 `(absolute maximum - installed)`, then clips the
requested built quantity to that value. A nonpositive result returns false.
Otherwise add the actual quantity through the 12-bit installation field, remove
the existing corresponding player message, add its signed-16 returned count
through signed-16 storage and choose singular/plural by whether the result is
greater than one. Factory/mine/defense automatic/manual pairs share these rules.
Message counts can differ from the installation delta and can narrow; they are
not an extra installation increment.

This is later than the builder's charging and uses absolute capacity, whereas
host validation compares absolute and operable limits. With supplied limits 2/5,
a charged five-factory request can install two and retain the full charge.
Actual derivation/coherence of those helper values remains M08/M19 work.
Alchemy IDs 3/11 and unused ID 10 return true without applying another mineral
increment here; the builder already handled alchemy. A successful boolean alone
does not imply a positive new object quantity.

## Starbase replacement and queue invalidation

`BaseMessage` selects the built-base message variant from the new design's stored
cargo (zero, positive, or 65535); its payload uses the separately looked-up hull
catalog cargo. Free designs short-circuit availability checking.
[M11's design check](M11-design-buildability.md) now owns the complete availability
producer, including hull/occupied-slot ordering, exact status=1 and ordinary
player-context restoration. The [eligibility integration companion](M19-construction-buildability.md)
now binds that exact outcome into FBuildObject's roster/free/guard order and
separate notification metadata read. Its new actual-checker/catalog construction
fixture supplements the original controlled-availability matrix; complete native
roster/owner/lifetime/annual refinement remains explicit.

`BasePlan` specifies ordinary event order: availability, notice, optional ship invalidation,
old warp query, optional old-existence decrement, set base, optional new warp
query/fling adjustment and packet invalidation, then new design counters.
Metadata read before and after callbacks is explicit; it is not a helper-frame
purity proof.

`Downgrade` compares old/new hull IDs as signed 16-bit values. It invokes
`KillQueuedShips` before switching the base. `BaseCounters` decrements the old
existence count first and then increments the new built/existence counters by
one, independent of the supplied construction quantity. Same-slot replacement
therefore preserves existence and increments built, including unsigned wrapping.
The sequential update preserves old/new slot aliasing.

If old ability is positive, retain fling settings without a second query. Otherwise
query the newly selected base: positive ability sets low-four-bit
`new ability + two-accelerator flag - 4`; nonpositive ability clears warp and
target and invalidates queued manual packets. `BaseFling` preserves these native
conditions, including the difference between old and new helper results.

`Compact` models invalidation using physical queue words. Ship invalidation drops
fleet-category items below 16, retains base items and resets their progress to
zero **at the source slot before copying**. Thus a surviving base's old inactive
source word can also change; a logical filter alone loses that effect. Packet
invalidation drops only planet items 14..17, retaining automatic packet 6.
Compaction keeps stable active order and other raw bits, frees an empty result,
and preserves maximum/capacity metadata otherwise. Empty/null entry queues skip
selection refresh. Processed nonempty queues refresh matching selected planets
through lookup then list fill even if no rows were removed. Actual selected-copy,
logging/tutorial, allocator and callback frames remain S06/S05 obligations.

## Ships, fleet capacity and packed damage

`ShipPlan` rejects unavailable designs. Only a player fleet count **equal to 512**
enters the existing-fleet search; another native count follows the new-fleet
allocation path. `Candidate` scans array order, stopping at the first null slot
or greater owner. Match the owner's **first order point**, not the fleet point,
and require `32766 - built > prior stack`. Equality rejects. The body does not
test the fleet dead flag here; full live-array invariants remain a separate
adapter. No candidate means a lost-construction notice and false return, after
the host already paid. `AllocateFleet` is a helper request, not a proof that an
allocation cannot escape or that registration is complete.

For a candidate, `Rebalance` follows the actual packed-damage sequence. Derive
damaged ships with signed-16 narrowing and minimum one; retain original damage
through the uint32 hull-hitpoint product, signed multiplication and divisions
by 10/50. Recalculate low-seven-bit damaged share using the narrowed new stack
total, forcing zero to one; recompute affected ships, then low-nine-bit damage.
Signed overflow and zero division stop at an explicit undefined boundary.
If a later calculation is undefined, retain the already written share as the
prefix state. Zero prior stack or zero damage fraction clears the packed word.
This algorithm is distinct from player `Merge2Fleets` and combat damage.

`CreateStack` applies the signed-16 stack addition and unsigned-32 design built/
existence additions. `CreateShip` performs those same source updates. New-fleet
construction then stores the returned fuel statistic and calls route/default
order helpers before choosing `ShipNotice`. Route notices distinguish a zero
second-order warp and singular/multiple quantities. Allocation, registration,
fuel-stat derivation, routing and full caller/availability integration beyond the now specified M11 checker remain
their existing owners.

## Mineral packets

`PlanPacket` rejects zero accelerator ability before a missing target. Automatic
packet 6 maps to mixed 17. Packet Physics uses 25 mixed or 70 single-mineral units;
other primary traits use 40/100. Multiply by built count, saturate above 32760
and retain signed-16 mineral words. Negative raw inputs are native-domain
observations rather than approved packet orders.

Use configured fling warp+4 unless below five or above ability+3, then use the
signed-16 ability/two-accelerator sum. Compute unsigned-16 decay and the Interstellar
Traveler adjustment through its signed-16 comparison. Keep raw requested warp/
decay for existing-object matching; a newly stored packet narrows them to four/
two bits. `FindPacket` returns the first exact owner/type/point/target/raw-speed/
decay match with weight **below 1630**.

`MergePacket` adds each signed mineral word, replacing a negative narrowed result
with 32760. Rebuild weight by signed-16 narrowing of `(mineral+9)` before dividing
by ten, storing each addition through 14 bits. In particular, three 32760 stocks
produce weight 6556, not a mathematical ceiling-of-mass total. `NewPacket` starts
from the zeroed object created by `LpthNew` and applies the same weight rule;
this zeroing dependency is source-backed, not assumed for an arbitrary allocator.

`PacketApplication` distinguishes rejection, merge, new object and allocation
failure. `PacketAccepted` returns true for allocation failure after its failure
notice. This differs from lost ship construction at the fleet limit; neither
boolean is a resource refund. S06 retains full thing-array/ID/heap refinement;
M19 retains subsequent packet movement, decay and interactions.

## Terraforming, Genesis and scanners

`TerraformCount` preserves the native 16-bit post-decrement loop, including raw
negative counts. `TerraformTrace` consumes one actual best-terraform result per
iteration; zero performs no change, ±1..3 selects the axis/direction and clamps
the signed-byte environment to 1..99 before messaging. An out-of-range axis is
undefined before access; insufficient supplied choices remains pending. The
trace proves calls plus remaining count equals the requested iteration count.
`SelectTerraform` and `RunTerraform` now supply actual M08 selection through
the [native pipeline](M08-terraform-selection.md), reusing that step/count
semantics. Message-helper preservation and full annual/caller frames remain open.

`GenesisPlan` broadcasts before mutation; `Rebirth` retains AR installations/scanner but clears
other races' installations and sets scanner 31, and clears the planet's three
mineral stock fields. `GenesisBounds` records twelve RNG calls, interleaving
50/50 environment draws then 40/40 concentration draws for each axis. New/original
environment is the same sum+1 (1..99); concentration is sum+25 (25..103). The
construction quantity does not repeat the Genesis operation.

Direct Genesis effects differ from the final host state: `Produce` later uses
canonical M09 `FinalStock` to publish its separate working mineral array. Actual
`Produce`/builder/application/RNG fixtures confirm that preexisting working
stocks can overwrite Genesis's direct zeroing. [GAP-17](README.md#gap-17) records
this mechanism, with historical comparison, full game qualification and intended
behavior left open. No mineral-erasure fix or gameplay change is made here.

`ScannerInstall` maps explicit 18..26 to scanner index and preserves global
`idPlayer`. Generic 27 sets that global to the owner for lookup, then sets it to
unowned (-1), rather than restoring its prior value. It messages before writing
the low-five-bit scanner field. Ordinary lookup return and exceptional escape
have different global-state frames. The [actual scanner selector](M08-colony-capabilities.md)
now supplies M11-derived results to SelectScanner, bounded to 0..8 with an
unavailable-zero fallback. Its effect is planned until the message returns;
complete message/alias/lifetime/caller frames stay M15/S06.

## Evidence and remaining qualification

[Source fixtures](../tests/spec/check_construction_effects_source.py) compile
actual application, stack creation, invalidation, thing creation, builder,
host-production and RNG bodies against actual `common.h` layouts and freshly
generated text/data IDs. Economy, availability/catalog, routing, messages,
low-level heap and selected-view services remain providers. The exact bodies
share the project's existing unused-parameter and pointer-sign warning exclusions;
all other C warnings and ASan/UBSan findings are fatal. An axis guard stops before
invalid indexing. Damage vectors execute only defined numeric traces.

Coverage includes all signed-16 installation maxima, paired item IDs, all 65,536
damage words at a declared stack/hitpoint context, every signed-byte environment
with valid axis choices, negative terraform iteration, native RNG Genesis,
all byte-valued generic scanner results, packet trait/warp/quantity/target cases,
the strict merge/weight thresholds, actual per-kind ID exhaustion, base-slot
alias/counter/warp/invalidation cases, and actual full-loop post-charge failures.
The 512-fleet failure fixture uses a consistent 512-entry owned fleet roster and
stack/design counts with controlled availability and costs; it is not a saved-game
or cross-platform qualification run. A separate actual-host witness checks
absolute-cap clipping and another checks Genesis stock write-back.

Remaining work includes source-to-model refinement over complete native state,
helper/row-pointer/selection/message frames and escapes; complete native caller
integration beyond the now specified M11 availability and M19 eligibility handoff,
capacity-producer lookup/frame refinement, complete terraform caller/float-trace
qualification, new-fleet registration/fuel/routing and allocator
contracts; saved-game and UI/report qualification, original comparison and any
intentional behavior decisions. The new family rules do not close unrelated
movement, combat, economy-state integration or AI/tutorial gaps in the collection.
