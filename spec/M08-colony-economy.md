# M08 — Native colony capacity, output and growth handoffs

Status: observed native caller behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion supports [M08-US02](M08-planet-and-colony-management.md#m08-us02),
[M19-US01](M19-game-rules-and-resolution.md#m19-us01) and the consuming
[M09-US02](M09-production-management.md#m09-us02)/[M09-US05](M09-production-management.md#m09-us05)
forecasts and host construction. Criteria remain at their owning entries.
This is a headless producer/consumer contract; M08/M09/M12/S05 retain display,
interaction, keyboard/focus/accessibility and unagreed performance targets.

Canonical native definitions are [M08 economy](dafny/M08-native-economy.dfy),
[M19 growth](dafny/M19-native-growth.dfy) and [M08 handoffs](dafny/M08-economy-handoffs.dfy).
The [native terraforming companion](M08-terraform-selection.md) now supplies the
complete native habitat prefix and its checked M04 safe-axis refinement,
using the same canonical binary64 expression and trace contract. Complete caller
field/memory/trace production remains separate from this numeric correspondence.
They reuse existing [M03 capacity/effective population/AR expression](dafny/M03-scoring.dfy),
[M08 scalar installation rules](dafny/M08-colony.dfy),
[M19 carry](dafny/M19-resolution.dfy) and [S04 finite binary64 evaluation](dafny/S04-arithmetic.dfy).
The older formulas remain normal-domain projections; their physical filenames
and existing IDs do not create another gameplay owner or a universal native
width guarantee. The new contracts supply native reads, casts and boundaries.

Sources: [planet.cpp](../src/core/planet.cpp) `CalcPlanetMaxPop`, `CMaxMines`,
`CMaxFactories`, both corresponding `CMaxOperable*`, `CMinesOperating`,
`CFactoriesOperating`, `CMaxDefenses`, `CMaxOperableDefenses`,
`CResourcesAtPlanet`, `PctPlanetCapacity`; [util.cpp](../src/core/util.cpp)
`ChgPopFromPlanet`; [race.cpp](../src/core/race.cpp) `GetRaceStat`, `GetRaceGrbit`,
`PctTrueMaxGrowth`; [globals.cpp](../src/core/globals.cpp) `rglPopMac`;
[structs.h](../src/core/structs.h) native `PLANET`/`PLAYER`/`HUL`.
[Verification](README.md#native-colony-economy-continuation) records actual
proof, source comparison and repository results.

## Inputs, domain and ordinary/error workflows

Population is signed-32 storage in **hundreds of colonists**. Race operation,
resource efficiency/productivity, primary trait, ideal growth and energy fields
are signed bytes; getter returns sign-extend them. These raw field domains are
broader than approved M04 race settings. Built installations are 12-bit, delta
population is eight-bit, base slot four-bit, hull ID unsigned-16, and most public
installation/resource/display returns are signed-16. Requested player and owner
are different contexts; array indices and pointer/record lifetimes must be valid
at actual dereference sites. Source arithmetic does not silently validate a
malformed saved game or approve a raw negative race/population value.
Signed narrowing/reinterpretation uses the supported native targets' two's
complement result, as in S01 codecs; this is not a claim that every C11
implementation chooses that result for out-of-range signed conversions.

`Number.Value` carries a defined signed-32 numeric result. `Undefined(reason)`
marks the first invalid integer arithmetic or missing/invalid helper input; it
is not an implemented error return. `Calculation.Floating` requests an exact
finite binary64 evaluation and an int32 conversion witness before `FinishFloat`
can supply a signed native result. It is not asynchronous C behavior. NaN,
infinity, invalid square root, zero division leading to nonfinite values, invalid
float-to-int conversion and unproduced platform traces have no fabricated value.

The native functions run synchronously. Normal flow reads helpers in source
order, computes and narrows, then returns; next-year operability may call growth
with update=false. There is no user cancellation inside these routines. Failed
lookup, invalid base/table indices, arithmetic faults and escaping helpers do not
gain local cleanup/rollback guarantees from the model. Native lookup/selection,
catalog, allocation, disclosure and callback frames remain explicit adapters.

## Population capacity and stored versus passed planets

`CalcPlanetMaxPop` looks up a local planet by ID; it does not take the passed
planet used by the consumer as its capacity snapshot. `Capacity` preserves that
boundary and marks a missing defined lookup result. The C routine ignores the
lookup return; the marker is not a new rejection branch.

For AR, a different owner or missing base returns zero before base/table/OBRM
access. Otherwise the four-bit base slot must address the ten-base array, and
the narrowed hull offset from Orbital Fort must index the five `rglPopMac` values
2500/5000/10000/20000/30000. Normal races use canonical M03 capacity: habitability
below five gives 500, otherwise habitability*100; HE subtracts half, JOAT adds a
fifth, then OBRM adds a tenth. `CapacityCalls` preserves the short-circuit trait
reads, including the extra non-HE comparison and skipped OBRM on early AR zero.
`CapacityBound`/`CapacityDefined` establish the defined result and signed-32 bound
for raw signed-16 habitability and the fixed source table.

Capacity uses the looked-up snapshot and requested race. Resources later use
the passed planet's population, actual built factories and, for AR, its own
habitability. A selected/local copy can therefore combine different source
snapshots; equality is a required adapter fact where a caller relies on it.

## Absolute, operable and operating installations

`Absolute` multiplies capacity by signed-byte operation rate as signed-32,
divides by 100, floors to ten, applies the final AR zero override and narrows the
return to signed 16 bits. The AR test does not justify skipping earlier source
arithmetic. `AbsoluteProjection` proves agreement with the old nonnegative
`Installations` formula when its required products and narrowing are valid.

`Operable` obtains that already narrowed maximum, captures the requested race's
operation rate and current population, optionally adds returned growth, then
computes signed-32 population*rate/100. It compares the signed-16 maximum to this
signed-32 current count **before** narrowing the selected current value. Only
then floor nonpositive results to one and apply the final AR override. A raw
current count of -32769 can narrow to 32767; narrowing both sides before the
comparison would change the source rule. `OperableProjection` checks agreement
with the scalar model on the declared nonnegative nonwrapping domain.

`NextPopulation` guards the native population addition. `NextIndustry` connects
the actual non-updating growth model to this calculation and retains different
owner-growth and requested-player operation contexts. With next=false, growth
is uncalled and cannot inject a failure/result into the calculation.

Defense maxima clamp four times signed habitability to 10..100, then AR to zero.
`DefenseOperable` adds growth if requested, checks population+24 before /25,
caps positive current at 1000, compares signed-16 current, then applies AR zero.
It has no industry floor of one. Negative raw population can expose signed
narrowing behavior; the unbounded scalar minimum is not a universal adapter.

`OperatingFactories`/`OperatingMines` first handle an unowned planet. AR factories
are zero; AR mines use the signed-16 cast of an int32-convertible binary64 square
root of population, ignoring constructed mines. Other races take the smaller
built count and current operable word. A zero-population ordinary installation
query can still have the operable floor; resource production's early-zero path
is a separate rule.

## Resource production and native publication

`Resources` returns zero for population zero **before** race/player/lookup access.
For nonzero population, `Effective` validates native subtraction, halving/addition
and doubled-capacity intermediates, then reuses canonical `EffectivePopulation`.

Ordinary production divides effective population by raw resource efficiency and
narrows it. Obtain current operable factories, limit by actual built count, then
multiply by signed-byte productivity. The factory term adds nine in unsigned-32
arithmetic, reinterprets the sum as signed-32, divides with signed truncation by
ten and participates in the final signed-16 addition. The resulting word is
changed to one **only if zero**. Nonzero negative words are preserved; replacing
this with an unbounded `max(1,total)` changes the native domain.

AR reuses `ARExpr` with each explicit binary64 boundary: convert effective
population, multiply by max(1,energy), divide by efficiency, square root, multiply
by max(25,habitability), divide by ten, add the actual 0.999 binary64 literal,
truncate to int32, then narrow to signed-16 and replace zero with one.
`FinishFloat` consumes the S04 checked evaluation relation, not a mathematical
real square-root approximation. The older `FARResources` is a restricted
projection; `NormalResourceFinish` establishes its final-step agreement in the
nonnegative no-wrap range. A typed helper context yielding integer 65536 narrows
to zero and becomes one at the native step, so applying the minimum before
narrowing is not generally equivalent.

`NonzeroOrdinary` and `FloatingResourceNonzero` establish nonzero defined resource
returns on those paths. They do not imply positivity, successful helper execution
or validity of an arbitrary race/planet. `ForecastResources` sign-extends the
native word into the existing M09 forecast reservation. `HostQueueResources`
uses the separate native extra-resource rule, skips a zero adjusted queue budget
and delegates research reservation to the host contract. Host suppression is the
captured pre-query flag; the forecast uses its post-query local flag. M12 research
projections consume those M09 outcomes, not a separate resource formula.

## Capacity percentage

`DisplayCapacity` returns zero for nonpositive capacity. Otherwise it preserves
signed population*100 multiplication, unsigned addition of half capacity,
signed-32 reinterpretation/division, clamp only when the quotient is at least
1000, and signed-16 return. `CapacityPercent` remains the normal-domain display
projection. Unsigned rounding addition can wrap despite a defined first product;
the full native word domain has no universal 0..999 guarantee. No new formatting
or input rejection policy is imposed by this arithmetic specification.

## Growth, carry and optional writes

`Grow` first returns zero for unowned or zero-population planets. A hostile
world computes at least one hundredth of loss using the narrowed negated habitat,
subtracts the loss fraction from the raw eight-bit carry, borrows once and
negates whole population loss. This branch does not query capacity or true growth.

Otherwise `TrueRate` doubles HE ideal growth, and native source narrows rate*
habitability to signed 16 bits before crowding. Above one-quarter capacity,
compute fullness with guarded signed population*1000/capacity. Preserve the
capacity..capacity+9 dead band without touching carry. At/above capacity use the
bounded loss rate; below capacity square retardation and select the source's
rate<1000 versus /10/rescale path. `FinishGrowth` computes the rough product
first and uses the fine product only when rough is below 10000000. Every signed
intermediate and division boundary is explicit.

Zero computed hundredths become one hundredth on the non-hostile carry path.
`CarryGrowth`/`CarryLoss` retain raw eight-bit carry and the source's single carry
adjustment; they do not normalize every malformed carry into 0..99. The
`NormalGrowthCarry`/`NormalLossCarry` properties connect valid prior carry to
canonical M19 `Carry`. `NoUpdatePreserves` proves the modeled population/carry
stay unchanged when update=false. This is not global helper purity.

On update=true, source stores the carry first, then adds population change.
`Publish` preserves that prefix if the later addition is undefined. An undefined
marker is not a deterministic C post-state or a returned error code. Native
frames and pointer/helper effects remain separate from these numeric contracts.

## Evidence and remaining work

[Source differential fixtures](../tests/spec/check_native_colony_source.py)
compile actual race, habitat, capacity, installation, resource, display, growth
and true-growth bodies against native common.h records and fresh generated IDs.
Lookup is a declared snapshot provider; another habitat mode supplies raw signed
helper results, and another growth mode supplies returned changes. The normal
mode executes actual habitat and growth calculations. The interpreter checks
undefined arithmetic/indices before native execution; excluded cases are not
native errors recovered by these functions.

Each defined vector compares return, final population and carry. Selected AR
resource/mining paths also compare the actual final binary64 bits with an
independent staged reference, using a transparent outer-value logging wrapper
and disabled floating contraction/fast math. This does not prove every platform's
sqrt/rounding implementation refines the S04 relation. Instrumented race getters
and capacity counters check ordinary/AR early-query order; growth-call counters
check that only direct/next-year queries invoke growth. The corpus includes
all signed-16 supplied habitat values, all signed-byte operation/efficiency
patterns at declared contexts, raw carry values, negative/zero/large populations,
next-year and update modes, AR table/ownership branches, current/stored snapshots
and owner/query-player differences.

Remaining work includes actual lookup/selected-copy and lifetime/frame refinement,
habitat extraction beyond its current normal-domain model, full binary64 trace
production/qualification, catalog/base mapping, saved-game/UI/report checks and
the [mining/depletion caller-state integration](M19-mining-and-depletion.md) and research/turn algorithms. Passing these numeric
models and finite fixtures does not close those dependent obligations or approve
runtime behavior changes. Counts and actual checks are in the collection record.
