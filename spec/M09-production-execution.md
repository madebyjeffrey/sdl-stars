# M09 — Construction handoffs and host production

Status: observed behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion supports [M09-US02](M09-production-management.md#m09-us02) and
[M09-US05](M09-production-management.md#m09-us05). Criteria remain in M09.
Canonical definitions are the [construction handoffs](dafny/M09-construction-handoffs.dfy)
and [host caller rules](dafny/M09-host-production.dfy). Verification is recorded
in the [collection continuation](README.md#host-production-handoff-continuation).

Sources: [turn2.cpp](../src/core/turn2.cpp) `Produce`, `CBuildProdItem`,
`FBuildObject`; [produce.cpp](../src/core/produce.cpp) `EstimateItemProdSched`;
[memory.cpp](../src/core/memory.cpp) `LpplReAlloc`, `FreePl`;
[structs.h](../src/core/structs.h) `PLANET`, `PROD`, `PLPROD`, `PL`;
[enums.h](../src/core/enums.h) production IDs/statuses.
The host caller assumes live player/planet IDs, valid supplementary arrays and
production lists with four-byte items. Queue primitives expose malformed header
counts/extents without making invalid native pointers valid. The fixture uses
one planet and projected player/phase globals; full multi-planet state remains
a separate adapter obligation.

The [builder](M09-production-builder.md), [forecast](M09-production-forecast.md),
[cost](M09-production-costs.md) and [S06 copy](S06-planet-editor-contracts.md)
contracts retain their existing ownership. This is not a new screen or a runtime
refactor. UI feedback, keyboard/focus, accessibility and unagreed latency/display
targets stay with M09/S05; M18 owns turn orchestration and M19 object/economy rules.

## Builder-to-forecast handoff

`Begin` selects the actual current forecast row and four remaining amounts,
then invokes canonical `M09Builder.Start`. A visible queue row gets the current
partial slot (count-only empty or known row). The inherited partial at index -1
gets an absent partial-output pointer. `Presence` proves this pointer-presence
distinction survives every finite builder trace, including undefined boundaries.
Forecast building uses `calcOnly=false`, the carried alchemy flag and actual
trait/generation context. A copied forecast does not suppress global effects.

`Execute` runs the canonical finite trace and returns one of three explicit
outcomes: ordinary return, pending construction, or undefined builder execution.
Only `Done` reaches `Resume`. Exhausting the supplied trace budget does not
fabricate a built quantity or a completion forecast. Alchemy termination remains
a separate obligation; the no-alchemy termination theorem remains available.

`Resume` installs the builder's caller-row value, partial and all four remaining
resource words, then passes its bounded built count and canonical status word
to the existing forecast `AfterBuild` transition. Index -1 writes the inherited
partial itself; visible rows write the queue and the separate partial output.
It reuses the forecast's early selected-row exit, first/last assignment and
post-build installation rules. `Resources` proves the signed-32 projection
without widening or narrowing the values. No second construction algorithm or
status table is introduced.

This is a checked logical adapter. Native quote/limit extraction, helper effects,
queue/pointer lifetime and correspondence of a real C trace to the model still
require their named native adapters. Source fixtures execute actual forecast/
builder bodies in the [forecast evidence](M09-production-forecast.md#native-evidence-and-open-work);
they are finite evidence rather than a universal C-to-Dafny refinement theorem.

## Host entry and resources

`Enter` preserves distinct native entry paths; `EntryPlan` specifies snapshot,
query, zero-resource gate, reservation, queue and publication order:

| Queue / owner | Observed action |
| --- | --- |
| Null / unowned | No resource query or empty-queue notice. |
| Null / owned | Send empty-queue notice, query resources, apply extra resources, and add all remaining resources to the owner's annual pool. Research suppression does not reserve production work on this path. |
| Allocated with count zero / any owner | Skip production, resource query and notice. An allocated empty queue is not equivalent to a null queue. |
| Allocated nonempty / unowned | Skip production. |
| Allocated nonempty / owned | Capture research suppression and three mineral stocks **before** querying resources. Apply extra resources; only a nonzero result enters reservation and queue execution. |

The resource helper returns signed 16-bit; the extra-resource array is unsigned
16-bit. `Extra` preserves the source's mixed arithmetic: multiply after unsigned
conversion, retain 32 bits, reinterpret as signed numerator; add extra and gross
as unsigned, reinterpret as signed denominator; divide with truncation toward
zero, then add to signed gross. Either zero input bypasses the adjustment.
For gross/extra 100/100 the result is 150. For -10/10 the denominator is zero:
the model marks undefined division, not a successful resource result.
The [native colony economy contract](M08-colony-economy.md) supplies the signed
resource producer and checked `HostQueueResources` handoff. Actual callback
context, lookup snapshot and full helper frames remain separate refinement work.

`Reserve` consumes the captured suppression flag, actual post-resource player
percentage (signed eight-bit), adjusted signed-32 resources and current annual
pool. Suppression leaves that pool unchanged. Otherwise charge signed product/100
to research and subtract it from available production resources. Unlike the
forecast's signed-16 budget, extra resources can make this multiplication
overflow; signed arithmetic boundaries remain explicit. `ResearchConservation`
proves available plus reserved equals the input on defined outcomes. Annual
accumulation and later leftover addition still require defined signed arithmetic.

The native source does not obtain a fresh stock/suppression snapshot after the
resource callback. Fixtures that mutate these fields during the query verify
that captured values govern this host path. Full callback frame/owner mapping
remains a separate obligation rather than an inferred purity guarantee.

## Validation before construction

`Validate` runs on the current row before any builder call; `ValidationCalls`
preserves the ordered helper queries. Zero-count rows are removed without a
notice. Nonplanet categories bypass planetary validation. Planetary rules are:

| Row | Validation / resulting notice |
| --- | --- |
| Scanner 18..27 | Remove with scanner notice when the planet's scanner field is not 31. |
| Manual packet 14..17 | Query warp, then require both nonzero warp and destination; otherwise remove with packet notice. |
| Manual factory/mine/defense 7/8/9 | Query absolute and operable maxima in that order, choose their larger signed-16 return, subtract the corresponding 12-bit installed count and narrow to signed 16-bit. If below requested count, send the installation-limit notice; nonpositive removes, otherwise reduce count. |
| Manual terraform 12 | Query terraformable count; below requested count sends the terraform-limit notice, then removes or reduces as above. |
| Other items | Continue unchanged. Automatic caps remain the builder's responsibility. |

The limit notice occurs even when a row is merely reduced. A valid count-zero
slot and every native item/category value remain distinct from proposed stricter
input validation. Helpers' full capacity/terraform formulas belong to M08/M19.

## Application result and queue control

`CarryAlchemy` skips automatic alchemy/item 3 when another queue row follows and
sets the carry flag. Empty/invalid following work goes through removal, which
can consume that immediately preceding alchemy row. Consecutive alchemy rows
retain a single boolean carry; removal does not erase every preceding alchemy
entry. The final item-3 row is built normally, using the builder's forced cap.

Each actual host construction call initializes only the partial count to zero
and passes a partial-output pointer with `calcOnly=false`. `ExecuteBuilder`
connects this to the canonical builder and distinguishes pending/undefined traces
from returned quantities, partials, statuses and application eligibility. `AutoDone` latches
false after status SomeAuto/NoneAuto (3/4), remaining false for the rest of that
planet's production visit. Object application runs only for positive built count.
`FBuildObject` receives the post-builder row and the already charged resources.
[M19's eligibility handoff](M19-construction-buildability.md) now composes the
actual M11 result with roster/free/guard/metadata rules. Native integration also
runs the real build-check/catalog graph inside Produce's exact fleet-limit
failure case; this strengthens service evidence without proving all economy,
message/allocator/fleet-registration or annual frames.
The [M08 mass-driver companion](M08-mass-driver-capability.md) now adds the
complete capability controller and actual checker/catalog/driver/application
integration, including six post-charge Produce base cases. M19 `DesignFling`
connects its outcomes to the existing conditional old/new base decision; full
economy/helper/annual frames remain open.
The [native terraforming pipeline](M08-terraform-selection.md) now supplies
catalog/bounds/habitat/selection/remote/optimum and repeated FBuildObject
application. The [capability companion](M08-colony-capabilities.md) now derives
native Ipct aggregation and connects it to ValidateTerraform, with actual
Produce clipping/removal/builder/application cases. Complete production-limit/
cost/annual and message/allocator/caller-frame integration remains open.

`AfterApplication` records the native ordering:

1. For positive built count and rejected application, read the captured row
   pointer. Clear its count if category is nonplanet or item is at least 7;
   automatic planet items below 7 are retained. A dead captured pointer at this
   read is an undefined boundary. A successful application short-circuits this
   read, so it must not be rejected merely because that pointer later died.
2. If the actual post-call owner is unowned and queue is null, restart at the
   top-of-queue label before inspecting status. This source branch does not
   establish that every destructive callback outcome is safe.
3. Only status Complete (0) removes work. CompleteAuto/SkippedAuto (1/2) do not
   take that removal branch. Otherwise signed-16 status >=5 stops; all other
   status words advance. Negative raw status words are not >=5.

Clearing a count after failed application does not change the builder's status.
For example, a rejected partially completed ordinary row can retain its slot
with count zero until the next visit removes it. There is no caller refund.
The actual application helper may itself modify the supplied resources or queue;
the caller uses its returned state, not a restored pre-build snapshot.

## Physical queue operations and ordinary exit

`RemoveRow` operates on S06's `Block` for a live production list whose item size
is four bytes, keeping maximum/count headers separate
from actual physical row extent. If count equals carry+1, free the entire queue.
Otherwise copy the tail over the current row and at most one preceding alchemy
row, decrement count through its eight-bit header, and back up the cursor before
the loop's ordinary increment. Inactive tail words are retained. A callback may
have changed count relative to the captured cursor: the source does not recheck
that index is active before decrementing. `RemoveRow` preserves that native
header arithmetic, including underflow, rather than asserting a universal
post-callback cursor invariant. Required copy extents are explicit guards; malformed item-size/heap headers,
actual pointer aliasing and allocator metadata remain native adapter boundaries.

`Prepend` runs only on a stopping status with a positive partial count. Resize
only when used equals maximum, requesting used+1 physical rows through the
existing list allocator. Reuse canonical S06 `Resized` to preserve old physical
extent, spare allocation and low-eight-bit maximum. Move all active words one
slot right, write the partial at zero, then increment used through eight bits.
At 255 rows this can produce used=0 and maximum=0 despite 256 physical words;
when maximum differs from used, no resize is requested and a missing spare row
reaches an out-of-allocation boundary. These mechanisms extend
[GAP-15](README.md#gap-15); no runtime fix or loader rejection is introduced.
`PrependPreservesOldActive` proves the shifted active prefix on defined inputs;
`PackedRoundTrip` connects canonical row words to their fields.

Resize escape follows construction and any application/count clearing, before
mineral-stock write-back and leftover annual accumulation. Existing mutations
remain; the caller has no local catch/rollback/cleanup guarantee. Invalid copy
extents and dead-pointer reads are model boundaries, not implemented error paths.

`CompletedNotice` sends the completed/empty notice for a null queue, or when the
cursor has reached the current count and the automatic-done latch remains true.
It does not merely test whether every physical row was removed. Ordinary exit
then writes the three working mineral stocks and adds leftover resources to the
actual owner's annual pool. Helper effects must be accounted for before this
handoff. M18 owns surrounding turn phases; full `FBuildObject`/economy effects
remain M19/M09/M10/M11 dependencies.

## Evidence, failure coverage and remaining work

[Native fixtures](../tests/spec/check_host_production_source.py) compile actual
`Produce`, `CBuildProdItem`, `LpplReAlloc` and `FreePl` bodies with native packed
planet/list/row layouts and production enum values. Player/phase globals are
field projections; economy, object application, low-level heap and final turn
helpers are controlled providers. Scripted builder pairs
exercise every 65,536 status words with/without a partial and accepted/rejected
application; those pairs do not claim native builder reachability. Separate
actual-builder cases check charges, progress 59, rejection/count clearing,
next-visit cleanup and resource changes returned by the application provider.
The [construction-effects companion](M19-construction-effects.md) now specifies
applied family algorithms and supplies actual-body 512-fleet, packet and other
application fixtures. Complete saved-game/native-state/helper refinement remains
explicit integration work.

The corpus also covers all item/category combinations and manual validation
guards, extreme signed absolute/operable capacities with distinct installed
fields, notices before count reduction, actual automatic caller retention after
rejection, all signed-16 gross resource values with seven extra-resource words,
captured query inputs, consecutive alchemy, every prepend count 1..255, spare
versus exact extent, guarded copy failure and resize escape after actual
construction charges as well as scripted post-build count clearing.
Four zero-denominator extra-resource cases are excluded before C execution;
they are not successful outcomes. Heap movement is guarded before undefined
access, while C11 warnings, ASan and UBSan findings are fatal. Final phase spies
check population/research/random-event order and post-phase scoring snapshots.

Remaining work: full source/model correspondence and native callback/row-pointer
frames; complete source refinement of the [now specified FBuildObject families](M19-construction-effects.md),
including their design/fleet/packet effects; full annual/planet loop state including escaping helpers; retrying
alchemy termination and complete native-state integration of the
[specified economic producers](M08-colony-economy.md); real creation/failure game
fixtures and UI/report qualification. The documented controller and finite
fixtures do not close those obligations or authorize gameplay changes.
