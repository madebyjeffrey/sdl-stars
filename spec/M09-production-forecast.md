# M09 — Production forecast controller and effect boundaries

Status: observed caller contracts, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the existing local work.
This companion supports [M09-US02](M09-production-management.md#m09-us02),
[M09-US04](M09-production-management.md#m09-us04) and
[M09-US05](M09-production-management.md#m09-us05). Acceptance criteria remain
in M09. The [canonical controller](dafny/M09-forecast.dfy) specifies every
`EstimateItemProdSched` caller branch over explicit returning helper states.
The full helper algorithms and their native refinement remain separately owned;
this is not a claim of complete forecast/simulation verification.
[Verification](README.md#production-forecast-controller-continuation) records
actual checks and their limits.

Sources: [produce.c](../src/core/produce.c) `EstimateItemProdSched`;
[turn2.c](../src/core/turn2.c) `CBuildProdItem`;
[mine.c](../src/core/mine.c) `EstMineralsMined`;
[planet.c](../src/core/planet.c) `CResourcesAtPlanet`;
[util.c](../src/core/util.c) `ChgPopFromPlanet`;
[research.c](../src/core/research.c) `ProjectedResearchSpending`.
M08/M19 own mining, capacity, resources and growth; M12 owns spending/quotes;
M09 owns row costs and production; S06/S04 own storage/heap/RNG adapters.

## Interface, input domain and copied storage

A production list asks for a zero-based queue row; research asks for a signed
resource-query word. The visible UI, interaction/accessibility requirements and
unagreed responsiveness targets remain in [M09](M09-production-management.md).

| Input / state | Native mapping and contract |
| --- | --- |
| Queue argument | A nonnull explicit `lpplprod` overrides the planet's queue; null falls back to `lppl->lpplprod` (`Source`). If both are null, the native code dereferences null before initializing outputs; `NoQueue` is a boundary marker, not an implemented result. `ProjectedResearchSpending` guards absent/empty queues before calling. |
| Copy | Copy planet value, allocate source **maximum header**, copy used-count × four raw bytes, set used header, then snapshot that count in signed `iMac`. `Prepare` reuses [S06 queue transfer](S06-planet-editor-contracts.md); `RowsMatch`/`Word` bind logical rows to the complete native words, including unused bits. |
| Usable extent | S06 distinguishes requested count, eight-bit maximum header and actual spare backing. A wrapped source maximum can make the forecast copy exceed its supplied usable extent; the fixture guards before that copy. This is the forecast-specific continuation of GAP-15, not a new queue limit. |
| Outputs | Two valid writable signed 16-bit result locations, separate from input/queue storage. They may alias each other: `First`/`Last` preserve that alias effect. Initialize last then first to zero only after successful copy. |
| Partial row | Native code initializes only `prodPartial.cItem=0`. `CountOnlyZero` represents unspecified unread fields, not a fully zeroed C struct. Once a builder creates a partial, `Known` retains its entire row even if its count later reaches zero. |
| Local fields | Stocks/population are signed 32-bit cargo values; mine/factory counts are 12-bit fields. Research suppression is a live copied flag. Queue rows use the canonical S01/M09 raw fields; cached `iMac`, not a later header read, controls iteration. |
| Helper continuation | `Continuation` preserves the local year/index, cached row extent, research amount, output values and output-alias identity across a returning helper. Actual colony/queue/resource/partial state resumes at each event. Live pointers/storage and full native helper frames still need their adapters. |

Allocation, copy, mining, resource, build or growth escape has no local catch or
cleanup in this routine. Allocation/copy failure precedes output initialization;
later escape can leave zero/partial outputs and a live local allocation until an
outer recovery boundary handles it. `Cleanup` frees the **current local queue
pointer**, if nonnull, on ordinary exits. It does not clear that pointer afterward.
No crash-atomic rollback or source-heap recovery is inferred.

## Year and row ordering

`Step` advances years 1 through 99, with explicit callback events:

1. Apply `EstMineralsMined(local, ..., -1, TRUE)`. Then read the local mineral
   stocks; the estimator does not use the separate returned quantity array.
2. Call `CResourcesAtPlanet(local, original->iPlayer)`. Its return type is signed
   **16-bit**, widened into `rgRes[3]`. Read the original owner's signed eight-bit
   research percentage after that call returns. If the current copied suppression
   flag is clear, reserve signed-product/100 with truncation toward zero;
   otherwise reserve zero (`ReserveResources`). The bounded input widths make
   this caller multiplication/subtraction fit signed 32-bit. The model proves
   available plus reserved equals gross, including out-of-policy signed fields.
3. Clear alchemy intent and visit inherited partial index -1, then cached queue
   indexes 0..`iMac-1`. Zero-count rows are skipped without clearing alchemy intent.
4. For planetary **automatic alchemy/item 3** before the final row, a matching
   requested row returns -1/-1; otherwise arm alchemy and continue without calling
   the builder for that row. A final item-3 row is changed to count 1020 before
   building. Manual alchemy/item 11 is a different row and does not take this path.
5. Call `CBuildProdItem` with `fCalcOnly=FALSE`, current alchemy flag and either
   null partial output for index -1 or the shared partial slot for a queue row.
   Resume from the actual changed row/partial/resources/colony and status.

The builder's status encoding is [M09 `StatusWord`](dafny/M09-production.dfy),
not the declaration order of its logical datatype. Ordinary `CBuildProdItem`
produces codes 0..7. The controller additionally preserves its native unsigned
word comparisons and signed-16 break test, without assigning extra product
meaning to other words.

## First/last results and post-build effects

For the selected row, positive `cBuilt` sets first to this pass only when it was
zero. Skipped-auto status 2 ends forecasting and sets last to pass minus one only
when first is nonzero; with no prior production, the ordinary separate outputs
remain 0/0. Complete/complete-auto 0/1 ends with last equal to this pass. These
exits occur **before** updating local installations or copying remaining mineral
stocks back (`TargetStopsBeforeInstall`).

For a nonterminal result, clear alchemy intent, then inspect the **post-builder
row**. Planetary mine 0/8 and factory 1/7 add signed built count into the 12-bit
local field modulo 4096 (`CompleteRow`). This includes an inherited partial whose
count has just become zero; its item/category remain defined. A status whose
signed-16 value is at least 5 stops the row loop; other statuses continue.

A negative signed interpretation of the request (32768..65535) exits after the
first row pass. Set first to the low-16 signed resource remainder. Only request
65535 adds the low-16 reserved-research word and narrows again (`Query`); last
stays at its current value. Other requests copy remaining minerals into the local
planet, apply growth, then advance. After pass 99, set first to 100 only if it is
still zero and set last to 100 (`VisitBound`). Callbacks have no guaranteed time
bound, and the year bound alone does not prove builder termination.

When both outputs share a location, assigning last also replaces first and vice
versa. In particular, the horizon's final last=100 overwrites an earlier first,
and a signed resource query changes both observations. The model and native
fixtures retain this behavior; the normal UI/research consumers use separate
output variables.

## Local copying does not establish global purity

The estimator neither clears `fGeneratingTurn` nor pushes/restores RNG state.
Actual mining can therefore call `Random(100)` for nonzero fractional yield during
generation. [M19 `MiningDraws`](dafny/M19-resolution.dfy) captures that condition
within its stated non-overflowing scaled-yield domain. With generation false,
that particular draw/increment is absent; it is not omitted merely because the
caller is forecasting.

AR local mining also traverses the **global fleet array**, stops at its first
null slot, and recursively mines eligible owned orbiting fleets with task Remote
Mine, at most one order and positive returned mining capacity. With apply=true,
it clears each such live fleet's `fHereAllTurn` after the recursive call returns.
`RemoteEligible`, `MiningMarks` and `MiningHoleStops` preserve this effect and the
first-null boundary. The forecast's planet copy does not copy that global array.
The [mining companion](M19-mining-and-depletion.md) now specifies signed-order
guards, full recursive quantities/depletion and the [M10 capacity algorithm](dafny/M10-mining-capacity.dfy).
Readable fleet/order/part storage and caller-state extraction remain M10/S06/S03
adapters.

`CBuildProdItem` can emit an alchemy message when conversions are nonzero,
calculation-only is false and generation is true (`AlchemyMessage`). The forecast
passes false for calculation-only. Consequently the copied planet/queue can
remain unchanged in the original while RNG, live fleet flags or message state
change. M12's temporary research-percentage restoration is a separate guarantee.
[Known-bug evidence](../docs/KNOWN-BUGS.md#forecast-helper-side-effects--located-on-main)
and GAP-16 track these source mechanisms; no suppression or runtime behavior
change is introduced by this specification.

## Native evidence and open work

[Source fixtures](../tests/spec/check_production_forecast_source.py) compile the
actual estimator, builder, mining body, race accessors and list helpers, with
packed native `PLANET`/`PROD`/queue structs. Builder/mining names are renamed only
for wrapper instrumentation. Scripted builder results isolate controller branches,
including all 65,536 status words; those synthetic combinations are not claimed
to be reachable outputs of the actual builder. Separate cases execute actual
builder/mining bodies with bounded service inputs.

The integrated partial example reserves five resources against a ten-resource
item, producing progress **59**, not 50: the canonical paid-cost rounding permits
that progress with the same integer charge. Next year the inherited manual
factory runs before the selected automatic row and retains its unused bits.
Other cases cover 255 cached rows, all pass/completion/skip combinations, signed
resource words and percentages, aliased outputs, alchemy carry across an empty
row, post-build row changes, allocation/helper escapes, wrapped copy extents,
post-resource context changes, generation messages and AR fleet/RNG effects.
A 9,216-case actual-mining matrix checks task/owner/planet/dead/order/capacity,
apply/AR and first-null eligibility. The actual RNG body runs from controlled native seeds in those cases. Full
fleet/order layouts and mining capacity remain projections; these finite seed
traces do not prove every game-state/RNG refinement.

| Remaining owner / boundary | Concrete acceptance work |
| --- | --- |
| M09 — builder refinement (E09-4) | The [builder companion](M09-production-builder.md) now specifies operational cost/paid/progress/alchemy/auto-cap/partial transitions and undefined numeric boundaries. Complete retrying-alchemy termination, helper/pointer/frame mapping and complete source refinement of the [applied construction families](M19-construction-effects.md), outer queue/application/failure frames; native fixtures are not universal refinement proofs. |
| M08/M19 — economy helpers | The [native economy companion](M08-colony-economy.md) now specifies operating installations, capacity/effective population/resources, next-year growth/carry and numeric budget handoffs. Bind these inputs to actual copied/lookup states and every helper's side effects; bind the [complete numeric mining/depletion/AR remote loop](M19-mining-and-depletion.md) and produce floating traces. Finite producer fixtures do not prove full pointer/state refinement. |
| S06/S04/M10 — copy, heap and mining inputs | Bind `RowsMatch`, actual usable allocation extent, pointer/array/order lifetime and remote-miner capacity to caller state. Complete normal and escaping cleanup frames beyond bounded heap doubles. |
| S06/M12/M18 — generation context | The [M12 budget caller](M12-development-and-budget-quotes.md) now specifies allocation narrowing, unconditional gross query, post-query queue branch, captured bounds and current-player restoration. Bind actual helper/global state and trace every estimator caller's generating/RNG/message/fleet context; reproduce full UI/AI workflows before deciding a separately scoped side-effect repair. |
| M09/S05 — presentation | Bind completion/0/0/-1/query/horizon outcomes to list/research display and feedback, keyboard/focus/accessibility and the supported display/latency matrix. No measured responsiveness budget is inferred from proof or fixture runtime. |
