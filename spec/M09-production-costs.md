# M09 — Production cost vectors and native arithmetic

Status: observed caller behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the existing specification work.
This companion supports [M09-US02](M09-production-management.md#m09-us02) and
[M09-US05](M09-production-management.md#m09-us05). Its acceptance criteria remain
in M09. [Canonical cost contracts](dafny/M09-costs.dfy) specify the complete
numeric `GetProductionCosts` caller over explicit native pricing inputs.
[Verification](README.md#production-cost-vector-continuation) records proof,
source comparison, handbook and repository results.

Sources: [produce.c](../src/core/produce.c) `GetProductionCosts`;
[util.c](../src/core/util.c) `GetTrueHullCost`; [race.c](../src/core/race.c)
`GetRaceStat`, `GetRaceGrbit`; [structs.h](../src/core/structs.h) `HUL`, `HS`,
`PROD`, `PLANETARY`, `PLAYER`; [enums.h](../src/core/enums.h) `ProdItemType`.
[M11](M11-ship-and-starbase-design.md#6-coverage-inventory-evidence-and-open-decisions)
retains component pricing, derived-design totals and the known uninitialized
`cExcess` boundary. M09 consumes returned quote words; no deterministic result is
assigned to an undefined or invalid lookup/pricing call.

## Caller interface and domain

The consumers are production-list cost display, AI planning and
`CBuildProdItem`; [M09](M09-production-management.md#33-costs-and-estimation)
owns their UI/story context. The four dimensions are Ironium, Boranium,
Germanium and resources, in that order. Interaction and unagreed display/
performance budgets remain the owning M09/S05 requirements, not a new screen.

| Input | Mapping / validity boundary |
| --- | --- |
| `PROD` | Reuse S01 `Prod`: ten-bit count, seven-bit item, three-bit object category; progress is present but ignored by this routine. Only object category fleet (2) enters design pricing; every other category uses the planetary switch. |
| Planet and pricing player | `lppl->iPlayer` supplies the colony primary trait; explicit `iplr` supplies design rosters and pricing-player race values. Both indexes must address live player storage. They need not be equal for the caller algorithm. |
| Stored design totals | Four unsigned 16-bit words from `GetTrueHullCost`, which reads `HUL.rgwtOreCost[3]` and `resCost` without repricing. Design arrays have 16 ship and 10 base slots. |
| Slots | Sixteen physical `HS` slots; category u16, item/count eight-bit fields. The existing base's `chs` determines the loop bound. M09 marks a bound above 16 rather than inventing a successful out-of-array result. |
| Part/hull quotes | Four returned unsigned 16-bit words per actual `GetTruePartCost` call. Bare-hull and paired old/new slot quotes are explicit inputs in source order. C writes some of them into signed 16-bit arrays; the model reinterprets those words before arithmetic. |
| Race fields | Resource build costs and primary traits come from signed eight-bit `rgAttr` through `GetRaceStat`; trait getters return 0/1. The signed field domain is preserved separately from proposed legal race-setting ranges. |
| Defense catalog | Three signed 16-bit ore values and one unsigned 16-bit resource value. Normal catalog entries are nonnegative; native-width cases do not approve negative costs as a game feature. |
| Incoming/output buffer | Four defined unsigned 32-bit words. The switch has no default assignment for unknown items, so a defined incoming buffer is required to model that path. No value is assigned to a caller's indeterminate stack buffer. |

`InvalidDesign`, `InvalidCurrentBase` and `InvalidSlots` are formal native-domain
markers. They are not implemented error returns. A free valid design returns all
zero costs before current-base access or part quotation; the colony race getter
still runs before that early return. Native roster/pointer construction and
actual pricing/helper failure behavior remain explicit adapters.

## Design costs and ordered base credits

`Ship` selects ship IDs 0..15 or base IDs 16..25, subtracting 16 for a base.
It begins with the new design's stored totals. An existing base changes the
calculation as follows:

1. **Different hull ID:** for each dimension, compare half the unsigned new
   total with a signed-16 stored upgrade difference. The old total is first
   reinterpreted as signed 16-bit and divided by two with truncation toward zero;
   the new-minus-half-old difference is then narrowed to signed 16-bit. Choose
   the greater value and store its low 16 bits (`ChangedHull`). For example,
   raw new/old 65535 yields 32767, while the older unbounded scalar expression
   yields 32768. The scalar model remains a no-wrap arithmetic projection.
2. **Same hull ID:** price the bare hull using category `hstNone`, subtract
   those signed quote words from the unsigned totals and retain low 16 bits.
   Iterate slots in increasing index through the existing base's `chs`.
   Apply a credit only when both old and new slot counts are nonzero. Different
   category uses 30%/70%, different item within a category uses 20%/80%, and an
   identical part uses the nonnegative difference. Slots outside that bound or
   without a paired occupancy get no credit (`Slots`, `SlotCost`, `Credit`).
3. **Base adjustment:** if the pricing player has ISB or primary trait AR,
   subtract one fifth of each remaining word. Then halve with upward rounding.
   Ship designs receive neither base adjustment. The final base calculation
   reuses canonical `BaseCost`; it is not a second component discount.

Native slot arithmetic is more specific than the percentage labels. Reinterpret
quote words as signed 16-bit, multiply by each slot count, narrow again, then
narrow the 3/7 or 2/8 products **before** signed division by ten. The chosen
retained cost itself is narrowed to signed 16-bit. Compare the remaining total
against the unsigned low-16-bit credit and subtract, limiting each slot to the
remaining total. `NativeCreditEquivalent` proves the clamped mathematical form
matches that native comparison/subtraction; each `Credit` cannot increase its
input, and `Slots` proves the complete ordered pass cannot increase any dimension.
The initial bare-hull subtraction can wrap and is not covered by that monotonic
credit property.

## Planetary costs and player contexts

`Planetary` preserves every switch branch, including automatic/manual pairs.
The values below are per item; quantity multiplication follows afterward.

| Items | Observed unit vector / context |
| --- | --- |
| Factory 1/7 | Normal minerals `[0,0,4-cheapFactory]`; tutorial minerals all `2-cheapFactory`. Resources use pricing-player factory build cost. |
| Mine 0/8 | Zero minerals; pricing-player mine build resources. |
| Defense 2/9 | Defense catalog costs; pricing-player Inner Strength multiplies each unsigned 32-bit dimension by three, retaining low 32 bits, then divides by five. |
| Alchemy 3/11 | Zero minerals; 25 resources with pricing-player Mineral Alchemy, otherwise 100. Reuses `AlchemyCost`. |
| Terraforming 4/5/12 | Zero minerals; 70 resources with pricing-player Total Terraforming, otherwise 100; **colony-owner** Claim Adjuster halves it. Reuses `TerraformCost` with that explicit context. |
| Genesis 13 | Look up Genesis Device and consume its four quoted part costs. |
| Pure packets 14/15/16 | Corresponding mineral only: colony-owner Packet Physics 70, Interstellar Traveller 120, otherwise 110; resources 5 for Packet Physics, otherwise 10. |
| Mixed/automatic packet 6/17 | All three minerals: colony-owner Packet Physics 25, Interstellar Traveller 48, otherwise 44; resources as above. |
| Scanners 18..26 / generic scanner 27 | Explicit IDs map to planetary index `item-18`; generic scanner maps to index 0. Consume four quoted part words. Lookup success/pricing pointer validity remains M11/S03. |
| Any unhandled item | Preserve all four incoming buffer words. The native switch supplies no zero initialization or rejection. |

For known nonnegative catalog/race values the usual costs follow directly. The
model also retains native sign extension and unsigned wrap where a stored field
contains an out-of-policy bit pattern. It does not silently replace those values
with proposed validation or an unlimited mathematical integer.

## Quantity and observations

`Cost` computes a unit quote before applying quantity. Nonzero `fOnlyOne` leaves
it unchanged even when the row count is zero; zero multiplies each dimension by
the ten-bit row count modulo 2^32 (`Scale`). `ScaleIdentity` proves single-item
preservation and zero-quantity results. A free design's early zero return remains
zero without pricing its parts. With an unknown item and a defined buffer,
quantity still multiplies the prior words; with an indeterminate prior buffer
there is no defined output claim.

Pricing/call effects remain distinct from numerical output. Same-hull upgrade
quotes the bare hull first, then old/new occupied slots in order. `FLookupPart`
return values are ignored by the caller; the quote contract requires the actual
resolved pointer/output and does not infer safety from an unsuccessful lookup.
`GetTruePartCost` may change pricing flags or encounter its documented undefined
path. Those effects and quote derivation are still M11/S03/S04 obligations.
No queue, mineral inventory, save file or submitted orders are modified by these
numeric contracts; full helper side effects remain outside the stated scope.

## Verification and remaining work

[Native fixtures](../tests/spec/check_production_cost_source.py) extract the
actual production-cost routine, packed cost-bearing structs, enum constants,
race accessors and stored-hull getter. An unused-parameter void-use annotation
in the getter permits warnings-as-errors without changing its returned words.
Part lookup/pricing and hull-catalog access are controlled quote providers.
An independent explicit-width arithmetic oracle compares all four outputs.
The fixture also checks lookup item/category, pricing player and old/new quote
request order, so fixed quote outputs cannot hide a wrong selected part.

The matrix covers every planetary item ID, all ten colony/pricing primary traits,
16 combinations of the relevant traits, tutorial and quantity modes; all 65,536
word values across three slot match classes and three count pairs; changed-hull
width cases; 20,000 deterministic multi-slot designs; all nonfleet categories,
negative stored input cases, free design and generic-scanner boundaries.
C11 warnings, address/undefined-behavior instrumentation and mismatches are fatal.
These finite cases validate the projected caller, not every source/helper state.

| Remaining owner | Concrete acceptance work |
| --- | --- |
| M11/S03 — quote extraction | Bind stored hull totals and every priced bare/slot/planetary quote to catalog, tech, trait and native context; retain the undefined `cExcess` branch until separately resolved. Test actual helper error/flag/pointer outcomes. |
| M09/S06 — native identity/domain | Bind owner/pricing player, roster occupancy, current-base index, old slot count and live arrays to each UI/AI/host caller. Undefined input markers do not add validation in C. |
| M09/M08/M12/M19 — forecasting | Complete the 99-year copied-state controller, mining/growth/research adapters and production mutations, including alchemy -1/-1, signed query 65535, automatic status/partial rows and generation-time message effects; include the capacity-header copy boundary from S06. Cost quotation alone does not prove forecast purity or completion dates. |
| M09/M18/M19 — execution | Complete charge/progress/alchemy/build/partial-row state, invalidation and failure after charge, reusing these cost vectors. Quotation is not a construction outcome. |
| S05/M09 — presentation | Validate real list/dialog formatting, focus, keyboard and supported-display/accessibility behavior against the parent requirements. No numeric responsiveness target is inferred from fixture runtime. |
