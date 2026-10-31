# M19 — Mining yields, depletion and ordered remote miners

Status: observed native behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion supports [M19-US01](M19-game-rules-and-resolution.md#m19-us01),
[M19-US03](M19-game-rules-and-resolution.md#m19-us03) and
[M19-US04](M19-game-rules-and-resolution.md#m19-us04), with
[M08-US02](M08-planet-and-colony-management.md#m08-us02),
[M09-US02](M09-production-management.md#m09-us02) and
[M10-US02](M10-fleet-operations.md#m10-us02)/[M10-US06](M10-fleet-operations.md#m10-us06)
consuming colony estimates and fleet capability. Criteria remain at those entries.

Canonical definitions: [M19 mining](dafny/M19-mining.dfy),
[M10 mining capacity](dafny/M10-mining-capacity.dfy). They reuse
[M19 concentration/yield/depletion-band rules](dafny/M19-resolution.dfy),
[M08 native operating mines](M08-colony-economy.md), S01 integer codecs and S04
arithmetic/finite floating evaluation. Stronger caller validation is not imposed
on the runtime by documenting raw native fields.

Source: [mine.cpp](../src/core/mine.cpp) `EstMineralsMined`,
[ship2.cpp](../src/core/ship2.cpp) `CMineFromLpfl`,
[planet.cpp](../src/core/planet.cpp) `CMinesOperating`,
[race.cpp](../src/core/race.cpp) `GetRaceStat`,
[utilgen.cpp](../src/core/utilgen.cpp) `Random`,
[structs.h](../src/core/structs.h) `PLANET`/`FLEET`/`HUL`/`HS`/`MINING`/`PLORD`.
The [collection record](README.md#mining-and-depletion-continuation) records
actual proofs, native fixtures and repository checks.

## Interface, state and workflows

`EstMineralsMined` takes a writable planet, three signed-32 output locations,
signed-32 mine count and an apply flag. Count **-1 alone** selects local mining;
every other count selects remote arithmetic, including zero and raw negative
counts. Planet population is in hundreds of colonists; the three outputs/stocks
use the existing mineral units. Concentrations and fractional depletion levels
are stored unsigned bytes. A stored level zero represents 256 when entering a
depletion iteration. Local efficiency is a sign-extended race byte.
Fleet order count is signed-16; the first task comes from a four-bit field.
The model's natural task projection still needs that native field adapter.

`State` carries the three ores (stock/concentration/level), output triple,
returning Random(100) trace/cursor, remote indices marked for clearing, and
ordered events. `Shape` requires three entries and valid supplied draws 0..99.
It is a logical call state, not a saved record or replacement for S06 arrays.
Stock and output locations are assumed distinct and valid; output/planet memory
aliasing and escaping native helpers need their own adapters.

`Outcome.Done` means the declared numeric call completed. `Pending` means a
required returning random result has not been supplied; it does not establish
that native RNG state was untouched. `Undefined` names an invalid arithmetic,
missing helper/pointer or indexing boundary. It is not a source error return.
Prefix state records preceding modeled output/stock operations, not deterministic
C behavior after undefined arithmetic. Supported targets' signed narrowing uses
the same two's complement interpretation as S01 codecs.

The routine is synchronous and has no cancellation branch. Normal flow:

1. Read the owner's primary trait if owned, including before local empty checks.
2. For count -1, return (-1,-1,-1) for unowned or zero-population planets. This
   does not query operating mines/efficiency, run remote fleets or mutate ores.
3. Otherwise obtain local operating mines; AR uses efficiency 10, other local
   mining queries `rsMineProd`. Explicit remote calls use the count and 10.
4. Process mineral indices 0, 1, 2: compute yield, publish its output, then apply
   stock/depletion when requested. Later minerals see any earlier prefix writes.
5. For owned AR local mining only, process eligible remote fleets in array order.
   Finish each recursive call, add its returned triple, then clear its live
   here-all-turn flag if apply=true before visiting the next fleet.

`FromOperating` accepts the existing M08 integer operating result or a checked
finite binary64 conversion for AR sqrt. The unowned/zero path does not consume
that result. Native pointer/catalog/lookup correspondence is still an adapter;
this composition does not prove that an arbitrary supplied helper is correct.

## Yield, rounding and application

`Axis` obtains the current stored concentration. Below 30 on a homeworld it uses
30 for local mining, and also for AR remote mining. An ordinary remote call uses
the stored concentration without that floor. `Concentration` remains canonical.

Multiply mine count by effective concentration as signed-32; a cast to unsigned
on the assignment does not repair an overflowing multiplication. For local
mining multiply by signed efficiency with the same guard, then truncate /10.
Remote mining uses the first product directly. Split that scaled value with C
signed /100 and remainder, then draw **only when remainder is nonzero and
`fGeneratingTurn` is true**. Increment only if the returned draw is below the
signed remainder. A negative remainder still consumes a draw and never wins the
comparison with a valid nonnegative draw. `NativeYield`/`UsesDraw` preserve this;
`YieldProjection` proves agreement with canonical M19 `Yield`/`MiningDraws` for
nonnegative scaled values. Merely calling this a forecast does not disable RNG.

Publish the quantity before the apply branch. With apply=false, ores and fleet
marks are unchanged, while outputs/events and possibly the RNG trace advance.
With apply=true, first add yield to the signed-32 mineral stock, then perform
depletion using **the original mine/concentration product divided by 100**.
Efficiency and the randomized yield increment do not alter that demand. Raw
negative productivity can reduce stock while positive demand still depletes;
this is width/domain evidence, not an approved race setting.

## Complete concentration and level loop

`Deplete` implements the full native loop, not only a depletion-band formula:

- Stop when demand is nonpositive or stored concentration is at most one. Stored
  concentrations zero/one retain their level, even if homeworld effective yield
  uses 30.
- Interpret level zero as 256. Reuse `DepletionBand`: concentrations above 100
  use 100; below five use 10; five through 24 use 25; others use themselves.
- Compute left as `normalizedLevel*12500/256/band`, preserving both integer
  divisions. If left <= demand, subtract it, decrement stored concentration,
  reset level to zero and continue. Left can be zero; concentration still falls.
- Otherwise compute length `12500/band` and remaining level
  `(left-demand)*256/length`. Floor to one first; if this is at least the prior
  normalized level, replace it with normalizedLevel-1. Write the byte; a zero
  result decrements concentration. End this mineral's loop after that branch.

For concentration 100, level 1 and demand 1, the first left is zero. The result
is concentration 99, level 253 after two iterations. Treating level 1 as one
whole unit or forcing demand to decrease on every iteration changes this case.
For concentration 100, level zero and demand 125, the result is 99/0.

`Deplete` proves concentration never increases, positive concentration never
falls below one, and iterations are bounded by max(0,initialConcentration-1),
independently of demand. At unchanged concentration with positive demand the
normalized level strictly decreases. `DepletionArithmetic` establishes the
positive divisor and bounded fractional-product domain. The finite byte fields
make this loop terminating; this is not a bound for external helpers or the
whole turn generator.

## Remote traversal, eligibility and prefix effects

`Remotes` stops at the first null fleet pointer or the declared extent. It skips
wrong planet, wrong owner, dead fleets and signed order counts above one before
dereferencing the first order. Counts zero or negative do not avoid that source
dereference. A missing first order is therefore a named boundary only after the
earlier guards; it is not automatically an ineligible fleet. Task must be code 3.
Then query mining capacity, skipping nonpositive returned values.
`EligibilityProjection` links the valid nonnegative-order domain to the existing
M19 `RemoteEligible`; the raw signed model does not silently widen its proof.

Each positive result runs remote mining against the **current** ore state, with
homeworld AR flooring and efficiency bypass. Outputs are local to that recursive
call. Parent output additions happen after all three child minerals return and
can have their own signed-overflow boundary; child stock/depletion prefixes are
already visible then. Mark the live fleet only after child return and all output
additions. A later hole or failure does not reverse previous work.

Order can change quantities. At initial concentration 100/level zero for each
mineral, population 10000 (AR local mines 100), then remote capacities 100 and
4000, apply=true without RNG yields 4160 per mineral and final 72/118. Reversing
those remote capacities yields 4172 and final 72/102. These fixture contexts
illustrate sequential calculation; summing all mine counts first is not equivalent.

## Fleet mining capacity

M10 `SlotSum`/`DesignSum` implement `CMineFromLpfl` over all sixteen ship-design
counts. Skip a nonpositive signed-16 count before accessing its hull. Otherwise
read the byte slot extent and scan in order; dereferencing beyond the native
sixteen-slot array is undefined, not silently clamped.

Only exact slot kind hstMining (128) with item 0..6 (Robo-Midget through Alien
Miner) invokes `FLookupPart`; excluded slots do not access an ability, even if
count is zero. Included zero-count slots still query the part. Returned
`MINING.grAbility` is signed-16; multiply by packed unsigned-byte count, and add
to per-ship capacity with unsigned-32 compound addition/assignment. Multiply
that signed per-ship capacity by positive ship count **before** unsigned
conversion, so the product must be defined. Accumulate fleet totals modulo 32
bits through the source's unsigned compound addition, then return total if below
4000, otherwise 4000. `CapacityCap` proves the upper cap; it does not prove that
arbitrary malformed abilities produce a nonnegative capacity.

Lookup return is ignored in the source before using the resulting part pointer.
`returned=false` denotes missing usable pointer/result, not a newly checked error
return. Full part availability, catalog mapping and native hull/pointer lifetime
remain M11/S03/S06 obligations. The native fixture uses the actual capacity body
with an explicit part-pointer/ability provider and fresh generated item IDs.

## Callers, evidence and remaining obligations

`MineMinerals` visits the planet array in order and requests local apply=true
mining; M18 retains phase/array lifetime and annual failure state. M09's estimator
also requests local apply=true on its working planet; the new contract explains
both numeric depletion and global fleet marking/RNG consequences. M08 mineral
popups and M15 report columns request apply=false. Their disclosure, formatting,
keyboard/focus/accessibility, supported displays and unagreed latency targets
remain with the UI owners. There is no independent mining screen or cancellation
policy in these headless routines.

[Native fixtures](../tests/spec/check_mining_source.py) compare actual common.h
records, freshly generated IDs and source mining/capacity/operating/RNG/race
bodies with an independent width-aware interpreter. Operating-limit and mining
part-pointer/ability providers are explicit. Every byte concentration/level pair
is exercised at three mine counts, alongside negative/sentinel counts, signed
productivity, application/generation modes, seeded actual RNG, conditional
waypoint access, holes, ordered remote quantities, part lookup counts and native
slot/ship-count/ability boundaries. Undefined candidates are filtered before C;
ASan/UBSan findings and all C11 warnings are fatal. A draft fixture initially failed
to allocate a flexible-array first order; its corrected storage is test-only.

The model does not prove complete C refinement, actual RNG extraction for every
caller, heap/pointer/aliasing/selection frames, catalog correctness or saved-game
reachability of every raw value. Full remote task eligibility/cancellation and
phase scheduling in `SatisfyOrders`, all GUI/AI contexts, and cross-platform/native
qualification remain open. There are no new runtime changes, transaction/purity
promises or agreed performance budgets in this specification work.
