# M08 — Terraforming totals, gate range and scanner selection

Observed source behavior inspected 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the working collection.
This companion specializes [M08-US03](M08-planet-and-colony-management.md#m08-us03)
and [M08-US04](M08-planet-and-colony-management.md#m08-us04): a player needs colony
capabilities to plan work, orbital travel and scanners. It supplies host
[M09-US05](M09-production-management.md#m09-us05) limits and
[M19-US01](M19-game-rules-and-resolution.md#m19-us01) scanner application.
Criteria remain at those entries. M08 owns these producers, M11 lookup decisions,
S03 catalogs, S06 native state/lifetimes, M09 budget/queue validation, M19
application, M10 gate usability/travel and M15 message delivery. No runtime,
format, data, layout, validation policy or baseline change is made.

Canonical [M08ColonyCapabilities](dafny/M08-colony-capabilities.dfy) reuses
[terraforming selection](M08-terraform-selection.md), the existing M11 lookup,
M08 `TerraformSteps` and the mass-driver companion's physical roster projection.
`ValidateTerraform` in [M09HostProduction](dafny/M09-host-production.dfy) connects
the actual count result to existing `Validate`; `SelectScanner` in
[M19ConstructionEffects](dafny/M19-construction-effects.dfy) connects the selector
to existing `ScannerInstall`. These are producer handoffs, not second formulas
for queue clipping or installation. [Collection evidence](README.md#colony-capability-producers-continuation)
records actual checks and remaining obligations.

Sources: [planet.cpp](../src/core/planet.cpp) `IpctCanTerraformLppl`,
`StargateRangeFromLppl`; [parts.cpp](../src/core/parts.cpp)
`LookupBestPlanetaryScanner`, `FLookupPart`, `TechStatus`;
[turn2.cpp](../src/core/turn2.cpp) `Produce`, `CBuildProdItem`, `FBuildObject`;
native [structs.h](../src/core/structs.h),
[gate rows](../data/parts/starbase-specials.yaml) and
[planetary rows](../data/parts/planetary.yaml). Existing M11 owns exact race,
technology and hidden-part rules rather than this companion copying them.

## Inputs, state, ordinary returns and domain boundaries

Current/original environments are signed bytes; identities and direct design
indices are signed words. A planet's base index is four bits, while the physical
base roster has ten entries. Sixteen owner pointers each identify a live roster
or a missing pointer. A design projects byte `chs`, sixteen physical slots and
three-bit detail. Gate lookup intentionally ignores the detail and free flags;
these are not extra eligibility checks in this helper.

Catalog metadata and availability share their actual row indices. SPECIALSB
range (`grAbility2`) is a signed word; planetary requirement/ability rows come
from the existing M11 catalog projection. Native fixtures export fresh metadata
and bind enum/mask/sentinel/count constants. No on-disk design number is changed.

Pointers and accessed arrays must be live, aligned, readable and sufficiently
large; outputs are disjoint from input/global/catalog storage. PART contains
caller-owned slot fields and a pointer union. Direct scanner selection preserves
quantity while replacing category/item/pointer; a generic construction local's
unobserved initial quantity/pointer is not asserted to contain a numeric zero.
`Prior(0)` is an abstract unobserved token where that local value is unused, not
a C initialization claim. S06 retains complete alias/lifetime and admission mapping.

Done means an ordinary return; Undefined records the first reached native
read-domain boundary, without adding a C error return, recovery or rollback.
These three producers require no floating witness: terraforming totals call
Can, not Best/desirability. Normal immutable-model frames are not universal C
memory refinement or complete message/callback frames.

## Terraforming total and production limits

`TerraformCount` calls canonical `Can` with helpful TRUE under the current
global viewer. Can substitutes the owner only for headless -1, and restores the
saved identity on ordinary return. FALSE yields zero without reading the local
bound/cost arrays. TRUE has initialized all bounds; costs remain unused here.

`SumSteps` visits gravity, temperature and radiation in order. For each axis,
add current-low if low is not -1, narrow the running total to int16, then add
high-current if high is not -1 and narrow again. No nonnegative clamp or habitat
test is applied. `SumProjection` proves agreement with the word-narrowed
mathematical sum; `ScalarSteps` links a representable endpoint pair to existing
M08 `TerraformSteps`. A raw current -23, original -23, ideal -50 with available
technology can produce clamped low 1 and total -24. This is a defined raw-field
case, not a claim of an admitted ordinary race design or a new supported UI state.

For positive nonempty planetary item 12, `ValidateTerraform` calls that actual
producer and passes its signed result into existing host `Validate`. A smaller
positive limit clips the queued count and emits the terraform-limit notice;
zero or a negative limit drops the row. Empty rows and other item/category
branches omit the count query according to `ValidationCalls`. An undefined
producer prefix gives no completed validation decision. Native Produce fixtures
compose validation, builder and actual selection/application under declared
returning quote/economy/message/heap providers. No refund or whole-year guarantee
is inferred from these cases.

## Stargate range: first slot and ignored availability

`Gate` distinguishes a planet pointer from the direct owner/design path.
An unowned planet (-1) or one without a base returns zero before any roster
read. A null planet instead selects the supplied owner and signed design index;
owner -1 there is an invalid roster access, not a headless bypass. Reached owner,
missing-pointer and design-index errors remain distinct boundaries.

`GateScan` reads slots in order up to `chs`. A candidate must have category
exactly special-base mask 512, positive byte quantity and gate item 0..6. It
copies that slot to PART and calls the canonical lookup under the existing
global `idPlayer`, without switching to the design owner. The first candidate
ends the scan, even when a later gate has a longer range or the first lookup
returns a denied/too-early status. There is no free flag, design visibility,
maximum-range or buildability guard in this helper.

`GatePointer` proves that a valid gate item assigns its row pointer even when
denied. The caller ignores the status and reads `grAbility2`: exactly -1 maps
to 10000; every other signed value is returned unchanged. This is a metadata
result, not permission to travel or evidence that a UI may disclose a foreign
design. M10 and the consuming UI own those checks.

No candidate yields zero with no lookup/player-context access. A raw count above
sixteen can return normally if an earlier candidate ends the scan; otherwise
slot sixteen is the reached array boundary. An invalid global viewer is reached
only when a candidate calls lookup, even if the owner roster itself was valid.
Normal returns leave planet, roster, player, global identity and RNG unchanged
in the declared nonaliasing domain.

## Scanner selection and construction order

`Scanner` sets item 8 and planetary mask 32768, preserving the incoming quantity.
`ScannerScan` calls lookup from item 8 downwards. Available status 1 stops; a
nonavailable result decrements unless item is zero. The final lookup at zero
still happens, and the void helper returns at zero even when it is denied.
The pointer still identifies the selected valid row. `ScannerResult` retains
each actual lookup result/call trace, so Done does not mean the final item was
available. There is no cancellation/failure Boolean returned by this C helper.
For example an AR availability context can deny every planetary scanner yet
return item zero. This describes the helper, not permission to construct one.

An invalid non-headless global player reaches the first lookup after the
category/item writes; the abstract prior pointer remains its recorded prefix.
Headless -1 is valid lookup context. Ordinary selection changes only PART's
category/item/pointer; global identity and input quantity stay unchanged.
There are at most nine returning lookups, including the all-denied floor case.

Generic construction item 27 sets `idPlayer` to the planet owner, invokes this
selector, then sets it to -1, irrespective of the previously saved viewer.
`SelectScanner` records that pre-message global prefix and a **planned**
`ScannerInstall` effect using the actual selected index, bounded 0..8. It does
not reject an all-denied floor result. The source sends the scanner notice
before storing the five-bit `iScanner` field; only after a returning message
helper is that installation performed. An escaping helper skips this caller's
scanner store; its own mutations and the earlier global assignment remain possible
prefix effects, without automatic restoration. Explicit scanner items retain their existing
mapping and do not use this selector. M15/S06 own delivery/escape/caller frames.

## Interaction, quality and evidence limits

These are synchronous core helpers, with no new controls or internal cancellation
point. Planetary planning, focus/keyboard, information disclosure, accessibility,
text and display behavior remain M08/M09/M10/M15/S05. Twenty terraforming catalog
calls, sixteen physical gate-slot reads before a boundary and nine scanner
lookups are structural bounds, not measured latency or resource targets. No new
performance/platform/assistive-technology conformance target is agreed here.

The native fixture combines actual common.h layouts, fresh catalog data,
lookup/technology/race/terraform bodies, both selectors, Produce and FBuildObject.
Independent references use a mathematical word sum and explicit descending/
first-slot selection. Scripted availability/metadata modes isolate outer loops;
real modes compose the full existing lookup. The fixture compares outputs,
pointer identities, category/quantity, lookup context/order/status, complete
native frames and message-time scanner state under ASan/UBSan. Required nonzero
per-operation case counts prevent a passing run that skips a producer family.

Undefined owner/roster/design/hull-slot/lookup domains are filtered before C.
The returning message provider's owner domain is recorded separately from a
native undefined-read claim. Quotation, economy, delivery and low-level allocator
services remain declared providers. Full native pointer/alias/lifetime, admitted
file mapping, UI disclosure/destination eligibility, actual messages and entire
production/annual/gating workflows remain open. A proved helper or passing native
fixture does not close those other contracts.
