# Race creation wizard calculations and numerical compatibility

Current module specifications: [M04 Race Design Wizard](../spec/M04-race-design.md),
[logical race data and Rust/SQL models](../spec/M04-race-data-model.md), and
[current binary64 advantage calculation](../spec/M04-race-advantage-points.md).
These supersede the historical arithmetic and unused-`fDontWrite` description
below; this document remains the historical numerical/UI research record.

**Historical arithmetic specification:** the calculations below describe the
pre-migration extended intermediates. Production now uses binary64 `double`;
see [DOUBLE-PRECISION.md](DOUBLE-PRECISION.md) for measured impacts. The diagnostic
extracts the pinned pre-migration commit, not current production source.

The race wizard stores a race definition in `PLAYER`, recalculates its advantage
point balance after option changes, and refuses to finish with a negative
balance. This specification describes the pre-migration calculations, their C types,
their rounding boundaries, and the evidence required to change their arithmetic
without changing results. It covers wizard pricing, displayed habitability,
random race construction, and the conversion of leftover points into starting
bonuses. The subsequent gameplay effects of every racial trait are outside this
scope; selected related formulas are included to distinguish them from pricing.

**The pre-migration x86 builds use 80-bit extended precision for some
intermediate expressions. Simply replacing those expressions with `double`
does change an integer result.** A reproduced race returns innate habitability
`1470286` with the pre-migration code and `1470287` with the experimental replacement.
Its displayed advantage balance is `541` in both versions. Preservation of all
integer intermediates is therefore disproved for this replacement; preservation
of the final displayed balance for all accepted races remains unproven.

The specification describes `race.c`, `raceui.c`, and `planet.c` at commit
`e7245dd3599e3a5dcf5e6ba74e90896321a251a5`, examined on October 10, 2026.
The diagnostic below extracts the pinned historical source rather than maintaining a
separate handwritten implementation. No production arithmetic or baseline was
changed to produce this specification.

## Relationship to user stories

Use [M04 stories and requirements](../spec/M04-race-design.md#user-stories-and-requirements)
for player goals, workflow acceptance, interaction/accessibility and quality
requirements. The [race data](../spec/M04-race-data-model.md#story-linked-requirements)
and [pricing](../spec/M04-race-advantage-points.md#story-linked-requirements)
companions own the focused criteria and formal traceability for implementers.
This reference supports M04-US01–US04; it does not define a second set of stories,
a new UI contract or additional acceptance guarantees. Existing revision and
historical arithmetic caveats in this document remain in effect.

## Source and execution flow

| Responsibility | Source |
| --- | --- |
| Page navigation and finish validation | [`RaceCreationWizard`](../src/ui/raceui.c) |
| Option controls and displayed balance | `RaceWizardDlg1` through `RaceWizardDlg6`, `FTrackRaceDlg2`, `FTrackRaceDlg3`, `DrawRaceAdvantagePoints` in [`raceui.c`](../src/ui/raceui.c) |
| Limits, fixed prices, full advantage calculation | `BoundsCheckPlayer`, `CAdvantagePoints`, arrays at the start of [`race.c`](../src/core/race.c) |
| Integrated habitability used for pricing | `LInnateRaceHabitability` in `race.c` |
| Habitability of a sampled planet | `PctPlanetDesirability` in [`planet.c`](../src/core/planet.c) |
| Names of stat indices and trait bits | `RaceStat`, `RaceGrbit`, `RaceAttribute` in [`enums.h`](../src/core/enums.h) |
| Race storage | `struct _player` in [`structs.h`](../src/core/structs.h) |
| Presets | [`data/races/predefined.yaml`](../data/races/predefined.yaml), generated as `vrgplrDef` |
| Random race generation | `CreateRandomRace` in `race.c` |
| Starting bonuses from leftover points | `GenerateWorld` in [`create.c`](../src/core/create.c) |
| Dialog layout and text | [`res/stars.rc`](../res/stars.rc), [`text/strings.txt`](../text/strings.txt) |

The visible page order differs from the dialog function numbering:

| Visible step | Dialog function | Options |
| --- | --- | --- |
| 1 | `RaceWizardDlg1` | Identity, predefined or custom race, portrait, password, leftover allocation |
| 2 | `RaceWizardDlg4` | Primary racial trait |
| 3 | `RaceWizardDlg5` | Fourteen lesser racial traits |
| 4 | `RaceWizardDlg2` | Gravity, temperature, radiation, immunities, growth |
| 5 | `RaceWizardDlg3` | Population efficiency, factories, mines, factory germanium discount |
| 6 | `RaceWizardDlg6` | Six research cost choices and higher starting technology |

Custom option changes update the global working race `vplr`. Drawing the balance
calls `CAdvantagePoints`; selecting a preset on page 1 can draw the selected
`vrgplrDef` entry instead. Repainting can therefore normalize the race, because
the calculator calls `BoundsCheckPlayer`. The calculator is not a pure query.
Finish recalculates, returns to the current page if the result is negative,
checks the password, and saves the race. Read-only viewing cannot finish a save.
The `fDontWrite` parameter exists in the signature but is not consulted by the
current `RaceCreationWizard` body.

Names, password, portrait, and leftover allocation have no direct term in the
advantage formula. Selecting a predefined race replaces the definition. Choosing
Custom clears the random-race bit so later world generation does not replace the
customized race. Random is a preset/template selection with Next disabled; its
eventual generated values are covered below.

## Types and arithmetic rules

### Persistent fields and option bounds

`PLAYER` remains a 192-byte on-disk layout. These fields do not store floating
point values:

| Field | C type | Meaning |
| --- | --- | --- |
| `rgEnvVar[3]` | `int8_t` | Preferred gravity, temperature, radiation |
| `rgEnvVarMin[3]`, `rgEnvVarMax[3]` | `int8_t[3]` | Environmental interval endpoints |
| `pctIdealGrowth` | `int8_t` | Growth setting, normally 1 through 20 |
| `rgAttr[16]` | `int8_t[16]` | Economy, leftover allocation, research, primary trait |
| `grbitAttr` | `RaceTraitBits`, alias of `uint32_t` | Lesser traits and other race flags |
| `fHacker` | One bit in a `uint16_t` union | Marks corrections to invalid values |

An immune axis has all three environmental fields equal to `envImmune`, which
is `-1`. Ordinary coordinates are integers from 0 through 100. The preferred
coordinate is `min + trunc((max - min) / 2)`.

| Stat index | Meaning | Stored range | Wizard units |
| --- | --- | --- | --- |
| 0, `rsResGen` | Population resource divisor | 7 through 25 | 100 colonists per unit; 10 means 1000 colonists per resource |
| 1, `rsFactProd` | Factory productivity | 5 through 15 | Resources per 10 factories |
| 2, `rsFactBuild` | Factory resource cost | 5 through 25 | Resources per factory |
| 3, `rsFactOperate` | Factory operation limit | 5 through 25 | Factories per 10000 colonists |
| 4, `rsMineProd` | Mine productivity | 5 through 25 | kT per 10 mines |
| 5, `rsMineBuild` | Mine resource cost | 2 through 15 | Resources per mine |
| 6, `rsMineOperate` | Mine operation limit | 5 through 25 | Mines per 10000 colonists |
| 7, `rsUseLeftover` | Starting bonus selection | 0 through 6 in bounds table | UI exposes 0 through 4; other values fall back to surface minerals |
| 8 through 13 | Energy, weapons, propulsion, construction, electronics, biotechnology | 0 through 2 | 0 extra cost, 1 standard, 2 reduced cost |
| 14, `rsMajorAdv` | Primary trait | 0 through 9 | Trait enumeration order |
| 15 | Unnamed slot | 0 only | Bounds tables implicitly initialize it to zero |

The bounds arrays use signed plain `char`; CMake explicitly uses
`-fsigned-char`. `SetRaceStat` accepts and returns `int16_t` and clamps to these
bounds before writing the `int8_t` slot. `GetRaceStat` returns `int16_t`.
`RaceStat`, `RaceGrbit`, and `RaceAttribute` are aliases of `uint16_t` rather than
native C enum types. Trait tests return `int16_t` flags.

`BoundsCheckPlayer` normalizes inconsistent immunity, clamps ordinary minima to
0 through 100, caps maxima at 100 and raises them to the minimum when necessary,
recomputes the midpoint, caps growth above 20, and clamps all 16 stat slots.
Each correction sets `fHacker`. `CAdvantagePoints` subsequently resets growth
below 1 to 1 and marks it. These checks do **not** guarantee a positive-width
habitat: equal endpoints can survive and cause division by zero in planet
desirability. The normal wizard interval starts with positive width and its
buttons aim to keep width at least 20. Malformed-file arithmetic needs separate
validation and must not be treated as a well-defined precision oracle.

### Intermediate types and rounding

Throughout the formulas below, `trunc(x / y)` means the source's integer
division, which truncates toward zero. Positive quantities therefore behave
like floor division. Apply truncation at the specified step; algebraically
combining divisions changes behavior.

| Calculation | Types |
| --- | --- |
| Overall point accumulator `cPoints` | `int32_t` |
| Growth, counts, economy deltas, local point subtotal `cCur` | `int16_t` |
| Integer habitability in the point calculator | `int32_t` |
| Habitability sampling indices, coordinates, widths, residuals | `int16_t` and `int16_t[3]` |
| Habitability innermost sum `l1`, sampled desirability | `int32_t` |
| Habitability stored accumulators `l2`, `l3`, `lInnate` | `double` |
| Specified habitability expression intermediates | `long double`, then explicit conversion to `double` |
| Planet positive score `pctPos`, negative score `pctNeg`, modifier `pctMod` | `int32_t` |
| Planet coordinate calculations and per-axis percentages | `int16_t` |
| Planet square root | `sqrt(double)` returns `double`; surrounding expressions use `long double` |
| Returned planet desirability and displayed advantage points | `int16_t` |

In the native and MinGW x86-64 compilers examined, ordinary integer promotions
make arithmetic on `int8_t` and `int16_t` operands occur as 32-bit `int` unless
an explicit unsigned or wider operand changes it. Explicit casts such as
`(uint32_t)` and `(int16_t)` are retained by the implementation. `cCur += ...`
narrows back to `int16_t` after each assignment. A faithful implementation must
also retain the source's explicit narrowing, rather than assume that every
intermediate is an unbounded mathematical integer.

`LOWORD` extracts `uint16_t` low bits. Returning that value through `int16_t`
restores the signed two's-complement interpretation on the supported compilers.
It does not clamp negative points to zero. Signed conversions outside their
representable ranges are implementation-defined; signed overflow and an invalid
signed left shift are not portable wrapping rules. Bit 31 of the race flags
deserves a portability test because the current trait helpers form masks with
`1 << ibit`. These observations are test targets, not changes to storage widths.

### What 80 bit means here

The examined native GCC and `x86_64-w64-mingw32-gcc` report:

| Property | `double` | `long double` |
| --- | --- | --- |
| Storage size | 8 bytes | 16 bytes including ABI padding |
| Format used for values | IEEE binary64 | x87 extended, 80 meaningful bits |
| Significand precision | `DBL_MANT_DIG = 53` | `LDBL_MANT_DIG = 64` |
| Maximum binary exponent macro | 1024 | `LDBL_MAX_EXP = 16384` |

`FLT_EVAL_METHOD` is 0 in both compilers. The native probe runs under
`FE_TONEAREST`, with x87 control word `0x037f` (full extended precision) and
SSE MXCSR `0x00001f80`. Its generated machine code contains x87 arithmetic for the
extended expressions and SSE arithmetic for the double-only expressions.
Sixteen bytes of storage do **not** imply IEEE binary128. The GCC
[x86 options documentation](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html)
distinguishes storage alignment from the 80-bit format.

The production build uses GNU C11 and does not explicitly request fast math or
a nondefault floating-point precision in `stars_options`. CMake checks
`LDBL_MANT_DIG == 64` for native host builds and warns when it is different.
That check alone does not establish the exponent format, the floating-point
control word, the rounding mode, or every compiler optimization behavior.
`long double` is not an architecture-independent synonym for 80-bit arithmetic.

The explicit outer conversions to `double` round **stored** accumulators to
binary64; they do not remove the extended evaluation of the expression leading
to that conversion. Compiler options affecting excess precision, reassociation,
or fused operations can alter these boundaries; see GCC's
[optimization options documentation](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html).
The compatibility diagnostic disables fast math and contraction explicitly.
These probe flags are not a change to the production CMake flags.

## Environmental controls and displayed habitability

### Coordinate changes and unit conversion

`FTrackRaceDlg2` moves the center, widens or narrows the interval, or changes
growth. Environmental arrow changes normally move one coordinate unit; widening
changes each endpoint by one, so width changes by two. Ctrl multiplies these
button deltas by ten. Endpoints are adjusted at 0 and 100 to preserve the
interval where possible. If the resulting width is below 20, each side is
expanded by `(20 - width) >> 1`. Keep this exact rule for odd-width loaded races,
including its interaction with endpoint corrections; it is not a general clamp
of the final width to exactly 20.

Dragging derives half-width as `trunc((max - min) / 2)`, maps the mouse position
to 0 through 100 with Win32 `MulDiv`, clamps the center to fit that half-width,
and sets endpoints to center minus/plus half-width. An odd loaded width can
therefore become even. The growth spinner changes by one, or ten with Ctrl,
and clamps to 1 through 20. Enabling immunity sets all three axis fields to
`-1`; disabling it resets the axis to min 20, center 50, max 80 rather than
restoring its previous range.

Pricing uses integer coordinates, not the formatted physical units.
`PszCalcEnvVar` and `PszCalcGravity` format them as follows:

* Temperature: `4 * coordinate - 200` degrees C.
* Radiation: the coordinate itself in mR.
* Gravity: let `d = abs(coordinate - 50)`. Set `v = 100 + 4*d` for `d <= 25`,
  otherwise `v = 200 + 24*(d - 25)`. For coordinates below 50, replace `v`
  with integer `10000 / v`. Display `v / 100` and `v % 100` as decimal g.
  This conversion uses integer arithmetic, including the reciprocal truncation.

### The one in N planets estimate

`DrawRace2` separately estimates the fraction of initially habitable planets.
It uses `int32_t` accumulators `l`, `l2` and `int16_t` loop values. It is not
`LInnateRaceHabitability`, does not use growth, and does not include future
terraforming.

Start `l = 1` and process gravity, temperature, radiation in that order:

1. For an immune axis or interval width 100, multiply `l` by 100.
2. For a nonimmune radiation axis, multiply `l` by `max - min`.
3. For nonimmune gravity or temperature, sum a weight over every coordinate
   from min through max **inclusive**. Its weight is `x` for `x < 10`, 10 for
   `10 <= x < 90`, and `100 - x` otherwise. Replace `l` with integer
   `l * weight_sum / 9`.

Clamp `l` to at least 1. Compute the displayed denominator as integer
`(1000000 + (l >> 1)) / l`. This is a rounded reciprocal estimate, not a
floating-point probability calculation. For `l == 1000000`, display that all
planets are habitable; for denominator 1 but a smaller `l`, display virtually
all planets. Otherwise display one in the calculated denominator.

The different handling of inclusive gravity/temperature coordinates and
radiation width is part of the existing calculation. Do not unify them during
a precision migration.

## Planet desirability used by the pricing integral

`PctPlanetDesirability` reads the specified race from `rgplr[iPlr]`. For a valid
positive-width habitat, start `A = 0`, `R = 0`, `M = 10000` as `int32_t`.
For each axis, let `x` be the planet coordinate, `p` the preferred coordinate,
and `[a,b]` the race interval.

* Immune (`b < 0`): add 10000 to `A`.
* In range: choose `d = p-a` if `x < p`, otherwise `d = b-p`. Calculate
  `q = 100 - trunc(abs(x-p)*100 / d)` and add `q*q` to `A`. Let
  `e = 2*abs(x-p) - d`. If `e > 0`, replace `M` with integer
  `M * (2*d-e) / (2*d)` at this axis. Modifiers truncate sequentially.
* Below range: add `min(a-x,15)` to `R`.
* Above range: add `min(x-b,15)` to `R`.

If `R != 0`, return `-R`; otherwise calculate:

```c
pctPos = (int32_t)((long double)sqrt((double)((long double)A / 3.0)) + 0.9);
pctPos = (int32_t)(pctPos * M) / 10000;
```

Return its low 16 bits through `int16_t`. For valid coordinates the result is
between -45 and 100. The `+0.9` is the existing rounding bias; it must not be
replaced by `round`, `ceil`, or `+0.5`. The root is evaluated by the binary64
math function `sqrt`, not `sqrtl`; division before it and addition after it use
extended intermediates. The literal `0.9` itself has type `double`, with its
binary64 representation promoted to `long double` for the addition.

For this root conversion the input `A` is an integer from 0 through 30000.
The diagnostic exhaustively compares both expressions for all 30001 values.
This isolates the floating portion independently of how a particular planet
reaches `A`. The remaining modifier calculation is integer arithmetic.

## Integrated innate habitability

`LInnateRaceHabitability` performs an ordered numerical integration over three
environmental dimensions. It saves `rgplr[0]`, copies the working race there,
and calls `PctPlanetDesirability` for player 0. It restores the saved player
afterward unless `pplr == rgplr`. This temporary global mutation means calls
cannot safely run concurrently against the same global player array.

If the supplied race has out-of-range environmental fields without a consistent
three-field immunity sentinel, this function converts the affected axis to
immunity, marks `fHacker`, and updates the temporary player copy. Test these
mutations separately from the normal positive-width domain.

### Terraforming stages and sample placement

| Stage | Ordinary terraforming allowance `t` | Total Terraforming allowance `t` | Desirability square weight |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 7 |
| 1 | 5 | 8 | 5 |
| 2 | 15 | 17 | 6 |

For each nonimmune axis, set `base = max(0, min-t)`,
`width = min(100, max+t) - base`, and `steps = 11`. Sample index `j` from 0
through 10 at `base + trunc(j*width / 10)`. Integer sampling can repeat
coordinates for narrow intervals. An immune axis uses base 50, width 11,
steps 1. Width 11 is a special weight, not an environmental interval.

In stages 1 and 2, move each nonimmune sample toward its preferred coordinate
by up to `t`. Let `delta = preferred - sample`. If `abs(delta) <= t`, set delta
to zero. If delta is negative, add `t`; otherwise subtract `t`. Record this
**signed residual** in `rgDelta` and evaluate the coordinate
`preferred - delta`. Stage 0 leaves the residual array zero.

After obtaining planet desirability `D`, add the three signed residuals.
If their sum exceeds `t`, subtract `sum - t` from `D` and clamp `D` to at
least zero. Do not replace the signed sum with a sum of absolute values.
Then calculate `D*D*stage_weight`, using the source's `uint32_t` casts and
`int32_t` destination. This squared, weighted value enters the integral.

### Ordered accumulation and conversions

The inner loop is radiation, the middle loop temperature, and the outer loop
gravity. Preserve this order and every conversion:

1. For each gravity and temperature sample, sum all radiation terms into
   `int32_t l1`. If radiation is nonimmune, replace it with integer
   `l1 * radiation_width / 100`. Otherwise multiply it by 11.
2. Add each `l1` into `double l2` with
   `l2 = (double)((long double)l1 + l2)`.
3. After the temperature loop, use
   `l2 = (double)((long double)l2 * temperature_width / 100)` for nonimmunity,
   or `l2 = (double)((long double)l2 * 11)` for immunity.
4. Add that `l2` into `double l3` with
   `l3 = (double)((long double)l3 + l2)`.
5. After the gravity loop, scale `l3` by gravity width / 100 or by 11 with
   the same extended expression followed by conversion to `double`.
6. Add each stage to `double lInnate` with
   `lInnate = (double)((long double)lInnate + l3)`.
7. Return `(int32_t)((long double)lInnate / 10.0 + 0.5)`.

That final cast truncates the positive result toward zero after adding 0.5.
It usually resembles rounding to nearest with half values rounded upward,
but the accumulated approximation can lie just below a mathematical half.
Preserving historical results requires preserving that approximation too.

The integration is not symmetric under arbitrary permutations of the axes:
radiation scaling truncates to an integer, whereas the other axes scale stored
doubles. Reordering the loops, replacing sums with a parallel reduction, or
keeping every accumulator in extended precision changes the algorithm's
rounding schedule.

For valid wizard habitats, a conservative bound on the stored total is
239580000; three immunities attain it. The integer return is at most 23958000.
The innermost integer accumulation and its width multiplication remain within
`int32_t` for this domain. These bounds prevent floating-point overflow or
underflow here; they do not guarantee identical integer rounding.

## Advantage point calculation

`CAdvantagePoints` returns an `int16_t`. All constants in this section are
**internal points**. The final balance is integer `cPoints / 3`; individual
adjustments must not be independently divided by three and rounded.

### Initial balance and growth

Normalize the race and start `cPoints = 1650`. First calculate integer
`H = LInnateRaceHabitability(pplr) / 2000`. Preserve this division before
applying growth. Let `g` be the validated growth setting and `s` its multiplier.

| Growth `g` | Direct credit | Multiplier `s` |
| --- | --- | --- |
| 1 through 5 | `(6-g)*4200` | `g` |
| 6 | 3600 | 7 |
| 7 | 2250 | 9 |
| 8 | 600 | 11 |
| 9 | 225 | 13 |
| 10 through 13 | 0 | `2*(g-5)+5` |
| 14 through 19 | 0 | `3*(g-13)+21` |
| 20 | 0 | 45 |

Add the direct credit, then subtract integer `H*s/24`. For each nonimmune
preferred coordinate, add `4*abs(preferred-50)`; off-center habitats earn this
credit. Count immune axes as `I` and subtract 150 if `I > 1`.

### Growth and factory interaction

If factory operation count `O > 10` or productivity `P > 10`, set
`o = max(1,O-9)` and `p = max(1,P-9)`. Multiply `p` by 3 for Hyper Expansion,
otherwise by 2. Subtract integer `o*p*g/2` if `I >= 2`, otherwise `o*p*g/9`.
This term is calculated before the Alternate Reality economy branch and is
not skipped solely because Alternate Reality was selected. The UI normally
resets its factory settings to 10, preventing the term there.

### Population efficiency

For `R = rsResGen`:

| `R` | Adjustment |
| --- | --- |
| 7 | -2400 |
| 8 | -1260 |
| 9 | -600 |
| 10 | 0 |
| 11 through 25 | `120*(R-10)` |

The source tests `R <= 7`, but normalization gives a minimum of 7.

### Factory settings

For non-Alternate-Reality races, let `a = 10-rsFactProd`,
`b = 10-rsFactBuild`, and `c = 10-rsFactOperate`. Build `int16_t cCur` in order:

1. Add `100*a` if `a > 0`, otherwise `121*a`.
2. Subtract `55*b` if `b < 0`, otherwise subtract `60*b*b`.
3. Add `40*c` if `c > 0`, otherwise `35*c`.
4. If the subtotal exceeds 700, replace it with
   `700 + trunc((int16_t)(subtotal-700)/3)`.
5. If `c <= -7`, apply the operation-limit surcharge:
   subtract `30*(-6-c)` for `-11 <= c <= -7`;
   subtract `45*(-12-c)+225` for `-14 <= c <= -12`;
   otherwise subtract 360.
6. If `a <= -3`, subtract `60*(-2-a)`.

Add the subtotal to `cPoints`. If the factory germanium discount bit is set,
subtract another 175. The discount's price is independent of the factory
resource-cost setting, but its displayed-point effect still depends on the
total's remainder when divided by three.

In ordinary gameplay the option changes a factory's germanium cost from 4 kT
to 3 kT. The factory cost code in `produce.c` has a tutorial exception:
factories use 2 or 1 kT of each mineral instead. Its resource cost still comes
from `rsFactBuild`.

### Mine settings and Alternate Reality

For non-Alternate-Reality races, let `a = 10-rsMineProd`,
`b = 3-rsMineBuild`, and `c = 10-rsMineOperate`. Start a fresh `cCur = 0`:

1. Add `100*a` if `a > 0`, otherwise `169*a`.
2. If `b <= 0`, subtract `65*b-80`; otherwise subtract 360.
3. Add `40*c` if `c > 0`, otherwise `35*c`.

Add this subtotal. There is no factory-style high-subtotal compression here.

Alternate Reality skips these factory/mine subtotals and the germanium-discount
price, and receives a flat +210 instead. Selecting it in `RaceWizardDlg4`
resets factory productivity/cost/operation to 10/10/10, mine
productivity/cost/operation to 10/5/10, and clears the discount. The economy UI
then exposes only the population-efficiency spinner and disables the discount.
Those resets do not automatically restore previous values when switching away.

### Primary trait prices

Subtract the corresponding `rgRacePrimaryTrait` value:

| Index | Internal name | Displayed trait | Value subtracted |
| --- | --- | --- | --- |
| 0 | `raCheapCol` | Hyper Expansion | 40 |
| 1 | `raStealth` | Super Stealth | 95 |
| 2 | `raAttack` | War Monger | 45 |
| 3 | `raTerra` | Claim Adjuster | 10 |
| 4 | `raDefend` | Inner Strength | -100 |
| 5 | `raMines` | Space Demolition | -150 |
| 6 | `raMassAccel` | Packet Physics | 120 |
| 7 | `raStargate` | Interstellar Traveler | 180 |
| 8 | `raMacintosh` | Alternate Reality | 90 |
| 9 | `raNone` | Jack of All Trades | -66 |

A negative value is a credit because it is subtracted. Other interactions
still apply; these are not complete standalone prices.

### Lesser traits and combination penalties

Add the selected `rgRaceAdvDisPts` values:

| Bit | Trait | Adjustment |
| --- | --- | --- |
| 0 | Improved Fuel Efficiency | -235 |
| 1 | Total Terraforming | -25 |
| 2 | Advanced Remote Mining | -159 |
| 3 | Improved Starbases | -201 |
| 4 | Generalized Research | +40 |
| 5 | Ultimate Recycling | -240 |
| 6 | Mineral Alchemy | -155 |
| 7 | No Ram Scoop Engines | +160 |
| 8 | Cheap Engines | +240 |
| 9 | Only Basic Remote Mining | +255 |
| 10 | No Advanced Scanners | +325 |
| 11 | Low Starting Population | +180 |
| 12 | Bleeding Edge Technology | +70 |
| 13 | Regenerating Shields | +30 |

Let `Nminus` be the number of selected traits with negative adjustments and
`Nplus` the number with nonnegative adjustments. The source calls these
`cBad` and `cGood`, respectively; those names must not be interpreted as the
player-facing distinction between advantages and disadvantages.

For `N = Nminus + Nplus`, subtract `10*N*(N-4)` if `N > 4`.
Subtract another `60*(Nplus-Nminus-3)` if `Nplus-Nminus > 3`.
Subtract another `40*(Nminus-Nplus-3)` if `Nminus-Nplus > 3`.

No Advanced Scanners additionally subtracts 280 for Packet Physics, 200 for
Super Stealth, or 40 for Jack of All Trades. Total Terraforming also changes
the earlier integration allowances; its -25 table entry is only one part of
its cost. Factory discount and higher starting technology are separate bits
and are not counted among these fourteen lesser traits.

### Research choices and starting technology

The research page stores 0 for 75 percent extra cost, 1 for standard, and 2
for 50 percent less. Calculate `S = sum(choice-1)` over all six fields.

* If `S > 0`, subtract `130*S*S`, then add 1430 if `S == 6`, or 520 if
  `S == 5`.
* If `S < 0`, add the entry for `-S` from the table below. If `-S > 4`
  and `rsResGen < 10`, subtract a further 190.
* If `S == 0`, apply no aggregate research adjustment.

| Net expensive fields `-S` | Credit |
| --- | --- |
| 1 | 150 |
| 2 | 330 |
| 3 | 540 |
| 4 | 780 |
| 5 | 1050 |
| 6 | 1380 |

The aggregate depends on the **net** of cheap and expensive choices, not just
their individual counts. Separately, selecting higher starting technology
(`ibitRaceTech3`) subtracts 180. The label specifies starting level 3 in fields
with extra cost, or level 4 for Jack of All Trades. Alternate Reality with
reduced-cost energy research (`rsTechBonus1 == 2`) subtracts another 100.

In `GenerateWorld`, starting technology is first initialized by primary trait.
Then the starting-tech option raises each extra-cost field to at least 3,
or at least 4 for Jack of All Trades; fields already above that level remain
unchanged. Cheap Engines subsequently adds one propulsion level, and Improved
Fuel Efficiency adds another outside the tutorial. These changes use `int8_t`
technology storage and integer arithmetic. The pricing term remains a flat
180 even when the primary trait already supplies some qualifying technology.

These are wizard prices. Actual research spending uses `GetTechLevelCost` in
`research.c`: start with `base_level_cost + (int16_t)(10*sum(current_tech))`;
extra cost adds `cost - (cost >> 2)`, reduced cost uses integer `cost/2`, and
Slow Tech doubles the result. The extra-cost rule is `2*cost-floor(cost/4)`,
so it is not necessarily the same as truncating floating-point `1.75*cost`.

### Final balance and validation

Return `LOWORD((int32_t)(cPoints / 3))` through `int16_t`.
This is truncation toward zero, so internal totals -1 and -2 both display zero,
and -3 displays -1. Finish checks the returned balance rather than the internal
accumulator. The threshold for accepting a race must preserve this behavior.

## Leftover points and related gameplay quantities

`GenerateWorld` takes `P = min(50, CAdvantagePoints(race))` for a human race.
It gives AI races 50 for these bonuses. Normal creation rejects negative human
balances elsewhere before applying bonuses. Allocate `P` according to
`rsUseLeftover`:

| Selection | Calculation |
| --- | --- |
| 0, surface minerals; unknown selections also use this | Set `K=10*P`. Add `floor(K/4)+(K mod 4)` to the least-stocked mineral, then add `floor(K/4)` to each of the three minerals. Total added is `10*P` kT. |
| 1, mineral concentrations | Set `K=1` for `P=1` or `P=2`, otherwise `K=P/2`. Add `K` to the lowest concentration, then add integer `(K+1)/2` to each of the three concentrations. |
| 2, mines | Add `P >> 1` mines. |
| 3, factories | Add integer `P/5` factories. |
| 4, defenses | Add integer `(P+5)/10` defenses. |

Surface mineral ties follow the nested comparisons in `GenerateWorld`:
all-equal stocks choose radiation/mineral index 2; pairwise ties depend on the
other stock. Concentration ties choose the first lowest index because the
scan only updates on strict `<`. Preserve those orders. Tough or Expert AI
races receiving surface minerals also receive the concentration bonus.
Alternate Reality sets starting mines, factories, and defenses back to zero
afterward. The concentration array is `uint8_t[3]`, starting mineral stocks
are `int32_t[4]`, and mine/factory/defense counts are unsigned 12-bit fields
in `PLANET`; the arithmetic locals are chiefly `int16_t`.

`PctTrueMaxGrowth` doubles the growth setting for Hyper Expansion. This does
not substitute doubled growth in the earlier wizard formulas: they use the
stored setting and the separate Hyper Expansion factory multiplier.

Ordinary population resources in `CResourcesAtPlanet` use population in units
of 100 colonists divided by `rsResGen`; factory resources use
`(operating_factories*rsFactProd+9)/10`. Alternate Reality instead uses a
square-root expression involving population, energy technology, the efficiency
divisor, and planet desirability, with further `long double` intermediates.
That downstream formula is outside a race-pricing-only precision replacement.
Removing every `long double` in `planet.c` would expand the scope substantially.

## Random race construction

`CreateRandomRace` uses integer RNG choices and repeatedly calls the same point
calculator. Random-number draw order and rejected attempts are part of its
behavior; a point difference can change the generated race and subsequent RNG
state even if no save format changes.

First draw `iVal = Random(25)`:

* Values 0 through 3 set three immunities and growth 2 through 5.
* Values 4 through 6 set three full 0 through 100 ranges and growth 3 through 6.
* Values 7 through 8 choose per axis between full range and an alternative
  branch that only assigns growth. That alternative leaves the supplied axis
  fields intact; it does not explicitly assign immunity. The code also biases
  the third choice when the first two preferred coordinates match. Growth is
  ultimately overwritten with 2 through 6 in this branch.
* Values 9 through 24 create each width as `20+2*Random(40)` (20 through 98),
  choose a minimum with `Random(101-width)`, and set its midpoint. Values 9
  through 11 replace one axis with immunity; 12 through 13 replace one with
  full range; 14 through 16 replace one with a width-20 interval. Growth is
  7 through 15.

Then a `Random(3)` gate gives either all standard research or individually
random research choices. A primary trait is drawn uniformly from ten.
A `Random(4)` gate gives either no lesser traits or independent binary draws
for all fourteen. Higher starting technology and cheaper factories are each
independent binary choices. A further `Random(3)` gate chooses either the
Humanoid economy defaults and a leftover choice 0 through 4, or individually
random bounded economy/leftover stats. Random names are selected separately.

If the resulting balance is outside 0 through 50, attempt changes selected
by `Random(10)`: research for 0 through 2, lesser traits for 3 through 5,
economy for 6 through 8, or growth/immunity for 9. Candidate quality is
`max(-points, points-50)`. Keep a candidate only when this value is strictly
smaller than the previous value; preserve each branch's original trial order
and restoration behavior. The loop is written with `if (cPass++ > 250)`;
it can execute 251 mutation iterations before falling back to the Humanoid
definition while retaining the generated singular name. No floating-point
arithmetic is used in the search itself, but each evaluation invokes the
habitability integral.

## Numerical stability and the double replacement question

### Stability of the values versus stability of integers

The integral mostly sums nonnegative bounded quantities. It has no large
subtractive cancellation, extreme exponents, or ill-conditioned division for
valid wizard habitats. Binary64 represents every integer in its intermediate
integer ranges exactly. This makes the floating values numerically well behaved
in an ordinary error-analysis sense.

Nevertheless, multiply/divide by 100 introduces fractions that are generally
not exactly representable in binary. A binary64 multiply followed by a
binary64 divide has an additional rounding compared with an extended multiply
and divide followed by one conversion to binary64. Extended-to-double
conversion itself can also double-round near a binary64 midpoint. Small
differences can survive the three ordered stage sums.

The final positive integer conversion is discontinuous at half-integer
habitability values. An error much smaller than one integer unit can change
the returned integer. A tolerance comparison between floating values therefore
cannot certify compatibility.

The planet root conversion has a useful separation from integer thresholds.
For integer `A`, a boundary for `sqrt(A/3)+0.9` occurs at a square of
`n-0.9`. Those squared thresholds do not coincide with integer thirds:
their separation is at least `1/300` before taking the root, or conservatively
about `1/60000` after taking it in this domain. Normal binary64 error is much
smaller. This explains why that subexpression is a stronger candidate for
removing extended casts than the integral's final conversion. The finite
30001-input comparison is still required on each target math library/compiler.

### Reproduced integer difference

The following custom race uses representable wizard settings and a positive
balance. It has no lesser traits, no germanium discount, and no higher starting
technology:

| Option | Value |
| --- | --- |
| Primary trait | Jack of All Trades |
| Gravity min / preferred / max | 31 / 51 / 71 |
| Temperature min / preferred / max | 61 / 79 / 97 |
| Radiation min / preferred / max | 9 / 41 / 73 |
| Growth | 10 |
| Economy stats 0 through 6 | 10, 10, 10, 10, 10, 5, 10 |
| Research | Six standard-cost fields |
| Leftover allocation | Surface minerals |

The experiment makes only one semantic replacement in the extracted
calculation functions: every `long double` token becomes `double`. It keeps
the stored accumulator types, operation order, integer casts, and source
functions. A read-only trace records the stored sum before the final return.

| Stage | Existing extended expressions | Experimental double expressions |
| --- | --- | --- |
| Stored `lInnate` | `14702864.999999998` | `14702865` |
| Hexadecimal stored value | `0x1.c0b221fffffffp+23` | `0x1.c0b222p+23` |
| Value immediately before integer cast | approximately `1470286.99999999981378` | `1470287` |
| Integer innate habitability | 1470286 | 1470287 |
| Integer habitability divided by 2000 | 735 | 735 |
| Returned advantage points | 541 | 541 |

The existing stored sum is one binary64 ULP below the candidate's sum.
For this race the exact-looking candidate lands on an upward rounding boundary
while the current expression stays just below it. Choosing the apparently more
accurate value is not equivalent to preserving the established behavior.

### Three different compatibility claims

1. **Every integer intermediate remains identical.** The plain replacement
   fails this requirement, as the witness demonstrates.
2. **Final displayed advantage points remain identical for every valid race.**
   The witness does not refute this narrower requirement. A difference must
   first cross the integer `/2000` boundary, then survive multiplication by
   the growth multiplier and `/24`, and finally the point total's `/3`.
   The random experiment has not proved that such a crossing is impossible.
3. **The entire application's integer behavior remains identical.** A
   race-only study cannot establish this. Planet resources, generation, AI,
   and other files contain additional extended expressions. Each needs its
   own analysis and the normal regression suite.

For illustration, if two innate results were 1999 and 2000, `/2000` would
produce 0 and 1. At growth 20 their habitability charges would be 0 and 1.
That can change displayed points when the remaining total has the appropriate
remainder modulo 3. This is a propagation example, **not** a reproduced race.

### Migration options and decision

Retain the existing casts until the chosen compatibility requirement is met.
A global textual replacement has already failed claim 1 and has not established
claim 2. Changing every stored accumulator to `double` is not a solution: they
already have that type.

A portable implementation without hardware 80-bit support is possible in
principle, but may need explicitly rounded software extended arithmetic or a
binary64 expansion that emulates each extended operation and each historical
store. Two-double arithmetic is not automatically an x87 emulator: precision,
exponent behavior, rounding, conversion, and operation order must be specified.

An exact integer/rational integral could remove ordinary floating error but
would not automatically preserve the current result at a half boundary.
Similarly, an epsilon, unconditional `nextafter`, or a changed rounding bias
would correct some races and change others. Do not adopt them without an
equivalence argument over the intended domain.

If the requirement is only the final point balance, a double implementation
with a certified error interval and a compatible fallback near thresholds is
another candidate. It must detect uncertainty at every relevant conversion,
not only after final floating-point accumulation. It must also preserve
normalization effects and deterministic random-race decisions.

## Reproduction and future tests

### Current diagnostic

[`tests/scaffold/race_precision.py`](../tests/scaffold/race_precision.py)
extracts the calculator and planet-desirability functions from current source,
compiles two separately named versions, and runs them on the same native
machine. It uses the real project headers and `PLAYER` layout. It rejects a
non-extended `long double`, excess evaluation precision, or a rounding mode
other than nearest, and checks the x87 precision control. It prints the compiler
version, extracted-source SHA-256,
compile command, type properties, witness, and optional random differences.

```sh
cmake --preset host-release
cmake --build --preset host-release
python3 tests/scaffold/race_precision.py --optimization O0
python3 tests/scaffold/race_precision.py --optimization O2
python3 tests/scaffold/race_precision.py --optimization O3 --samples 100000
```

The script is a diagnostic, not a production calculator or a test that certifies
migration. Exit zero means it ran and its isolated root comparison matched;
it can still report a habitability or point difference. Random races use an
explicit xorshift32 seed `0x12345678`, positive even widths 20 through 100,
centers consistent with endpoints, all growth values, bounded economy/research
values, all primary traits, and Total Terraforming on/off. Other lesser traits
are not sampled because they do not enter the floating integral; their integer
pricing interactions still need the tests below. Odd-width loaded races are
also outside this random corpus.

### Tests required before replacing precision

The broader [precision suite](../tests/scaffold/PRECISION.md) now extracts all
production extended expressions, reports exact differences and counterexamples,
and runs whole race-calculator axis sweeps, random races, and optional Cartesian
habitat shards. Run `make precision` for measurements and `make test-precision`
for its oracle/coverage checks. Its stated operand grids and habitat sweeps do
not exhaust the full migration requirements below.

| Test family | Cases and evidence required |
| --- | --- |
| Golden integer witness | Pin the race above, its innate return, `/2000` result, points, and unchanged normalized `PLAYER`; compare against the production function as well as the extracted reference. |
| Planet desirability | Exhaust all 30001 positive-score root inputs, plus coordinates at minima, preferred values, maxima, and one step outside. Cover immunity and sequential modifier truncation. Record the compiler and math library. |
| Habitat domains | All immunity masks, Total Terraforming both ways, endpoint clipping at 0 and 100, widths 20 and 100, all midpoints, repeated sample coordinates, odd widths loaded from supported files, and both signs of terraforming residuals. Do not assume axis permutation is safe. |
| Integral boundaries | Record decimal and hex accumulators and distance to half-integer return boundaries. Seek cases where the integer result is `2000*k-1`, `2000*k`, or `2000*k+1`. Retain every counterexample as a regression fixture. |
| Point propagation | For each sensitive habitat, sweep growth 1 through 20 and integer point offsets modulo 3 using real trait/economy/research combinations. Compare innate, `/2000`, `/24`, internal total, final points, and finish acceptance. Prioritize balances -1, 0, 1, 49, 50, 51. |
| Economy boundaries | Resource divisor 7/8/9/10/25; factory productivity 10/11/12/13/15; factory operation 10/11/16/17/21/22/24/25; cost 5/10/25; subtotal 699/700/701; mine cost 2/3/4/15. Cover immunity and Hyper Expansion interaction, and AR resets. |
| Traits | Exhaust 16384 lesser-trait masks for representative habitats, all primary traits, scanner interactions, total count 4/5 and imbalance 3/4. Test discount and starting-tech bits independently, including bit 31 portability. |
| Research | Exhaust 729 six-field choices for every primary trait, resource divisors 9 and 10, and starting-tech both ways. Verify the net-choice rule, five/six-field corrections, and AR energy surcharge. |
| Leftovers | Every allocation and point count 0 through 50, ties in mineral stocks and concentrations, P=1/2 concentration special case, factory/defense rounding, AR zeroing, AI bonuses, and unsupported selection fallback. |
| Normalization and files | Old supported race files, consistent and inconsistent immunity, growth extremes, out-of-range stats, odd widths, and zero-width habitats under a separate invalid-input policy. Compare mutation of the supplied race and restoration of `rgplr[0]`. |
| UI transitions | Toggle immunity twice, enter and leave AR, preset to Custom, Ctrl increments, drag odd widths, read-only view, and save/reload. Verify that points and selected stats reflect the actual mutation order. |
| Determinism | Fixed seeds through `CreateRandomRace` and world generation. Compare the full generated race, number of RNG draws, fallback decisions, and resulting host records. |
| Platforms and compilers | Production MinGW debug/release and native GCC/Clang O0/O2/O3; intended ARM targets; record `DBL_MANT_DIG`, `LDBL_MANT_DIG`, `LDBL_MAX_EXP`, `FLT_EVAL_METHOD`, FPU control state, rounding, and compiler flags. Use a trustworthy independent extended oracle on targets whose `long double` is binary64 or binary128. |

A finite exhaustive habitat search is much larger than the research or trait
spaces. Even restricting each axis to positive even widths 20 through 100
allows `sum(101-width) = 1681` ordinary intervals plus immunity, giving
`1682^3 * 2 = 9517173136` combinations with Total Terraforming. Use caching,
boundary-directed enumeration, and formal bounds to reduce this work. A random
sample, including a million matching cases, is not proof for that space.

For each candidate, compare exact integers rather than epsilon tolerances.
Report the first difference with the complete race and every rounding stage.
If preserving all intermediates is required, the existing witness alone rejects
the plain double candidate. If preserving only displayed points is sufficient,
the acceptance gate needs exhaustive or otherwise certified treatment of
threshold crossings, plus fixed-seed generation and file compatibility checks.

For a production change, also run the repository's required MinGW build, full
native-baseline regression, unit tests, tutorial, host regression, and native
unit tests. Do not regenerate the baseline to conceal a difference during an
equivalence experiment. An intentional behavior change must instead follow
the repository's separate commit, changelog, and baseline rules.

### Verification results for this specification

On x86-64 Linux with GCC 13.3.0, the positive-balance witness reproduced at
O0, O2, and O3. The 30001-input root comparison had zero integer differences at
all three optimization levels. An additional deterministic O3 sample of 100000
races produced one innate-integer difference, zero `/2000` differences, and
zero displayed-point differences. That counterexample used the same habitat
as the witness but different integer economy/research settings. The extracted
source SHA-256 was
`1f16aadfdacbb314f7fb298284cb968f75f08e9c06ce4ae9278310fd4736d645`.

The original complete `race.c` and `planet.c`, compiled without extraction at
the native release optimization level, independently returned habitability
1470286 and points 541 for the witness. A MinGW O3 build of both extracted
variants, executed under Wine, reproduced the same difference and matched
all 30001 root inputs. Both native and MinGW probes recorded x87 `0x037f`,
MXCSR `0x00001f80`, and the type properties listed above.

| Repository check | Result |
| --- | --- |
| MinGW debug configure and build | Passed; no source compilation warnings introduced |
| Full native-baseline regression, all nine scenarios through turn 150 | 519 matches, 22 existing warning-bearing matches, zero differences/errors |
| Windows unit suite | 29 of 29 passed, including the existing local compatibility targets |
| Native host unit suite | 12 of 12 passed |
| Full tutorial | Failed at year 2408, page 26, instruction 203, with `No ship transfer arrow is available`, matching the already documented local tutorial limitation |

The regression report is
`dist/scaffold/regression/race-spec-comparison.json`. The tutorial artifacts
are under `dist/scaffold/tutorial/20261010T193059.276226Z/`.
These checks used the unchanged production calculator. They do not validate
the experimental double replacement. The native baseline and frozen original
checkpoints were not changed.
