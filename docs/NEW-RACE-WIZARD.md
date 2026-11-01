# New Race wizard: workflow and complete calculation reference

This document describes the implemented New Race wizard on `main`, inspected
at `e2eae18cff0f990db20415c1c2262aed2022f95b` on October 23, 2026. It is a
reference for reproducing the existing wizard, not a proposal to change its
rules. It brings the six pages, stored inputs, numerical calculations,
random-race generation, and starting-point allocation together in one file.

The arithmetic below is current **binary64** arithmetic. The older
[race calculations study](RACE-WIZARD-CALCULATIONS.md) describes historical
extended intermediates; see [the migration record](DOUBLE-PRECISION.md).
Related specifications cover [the race design module](../spec/M04-race-design.md),
[the data model](../spec/M04-race-data-model.md), and
[advantage points](../spec/M04-race-advantage-points.md). Those documents and their
README links are maintained separately. This file is self-contained for wizard
math and does not require adopting their proposed module or database designs.

Scope: calculations for wizard inputs and displayed gameplay values, point
balance, random designs, credentials/checksums, and leftover-point bonuses.
Selected downstream economy and research formulas explain what the page labels
mean. Combat, ship design, mining depletion, and the full turn-by-turn effects
of every racial trait remain separate game systems.

## Relationship to user stories

Use [M04 stories and requirements](../spec/M04-race-design.md#user-stories-and-requirements)
for player goals, workflow acceptance, interaction/accessibility and quality
requirements. The [race data](../spec/M04-race-data-model.md#story-linked-requirements)
and [pricing](../spec/M04-race-advantage-points.md#story-linked-requirements)
companions own the focused criteria and formal traceability for implementers.
This reference supports M04-US01–US04; it does not define a second set of stories,
a new UI contract or additional acceptance guarantees. Existing revision and
historical arithmetic caveats in this document remain in effect.

## Contents

- [Workflow and page order](#workflow-and-page-order)
- [Stored inputs and bounds](#stored-inputs-and-bounds)
- [Arithmetic contract](#arithmetic-contract)
- [Environmental controls and displays](#environmental-controls-and-displays)
- [Planet desirability](#planet-desirability)
- [Integrated innate habitability](#integrated-innate-habitability)
- [Complete advantage-point calculation](#complete-advantage-point-calculation)
- [Starting benefits](#starting-benefits)
- [Random race construction](#random-race-construction)
- [Related economy and research math](#related-economy-and-research-math)
- [Password and file checksum math](#password-and-file-checksum-math)
- [Worked examples and verification](#worked-examples-and-verification)
- [Source map](#source-map)

## Workflow and page order

`RaceCreationWizard` in [raceui.cpp](../src/ui/raceui.cpp) edits the working
`PLAYER vplr`. The visible page order is different from the dialog numbering:

| Step | Dialog | Inputs |
| --- | --- | --- |
| 1 | `RaceWizardDlg1` | Singular/plural names, preset or Custom, portrait, password, leftover allocation |
| 2 | `RaceWizardDlg4` | One of ten primary racial traits |
| 3 | `RaceWizardDlg5` | Fourteen independently selectable lesser racial traits |
| 4 | `RaceWizardDlg2` | Gravity, temperature, radiation, immunity, ideal growth |
| 5 | `RaceWizardDlg3` | Population efficiency, factory and mine settings, factory germanium discount |
| 6 | `RaceWizardDlg6` | Six research prices and higher starting technology |

Back and Next preserve the working design. Cancel or a failed dialog returns
failure. Finish is available before the last page and follows the same checks:

1. Read-only viewing cannot finish a save.
2. Recalculate `CAdvantagePoints(&vplr)`. A negative returned balance produces
   an error and returns to the active page.
3. Hash/check the password; a failed password check returns to page 1.
4. Unless `fDontWrite` is set, save through the current race library entry or
   `FSaveRace`. A failed/cancelled save returns to page 1.
5. Return success. `fDontWrite` performs validation without writing a race file.

The library manages race versions under the
[accepted library decision](adr/0001-desktop-managed-libraries.md). An unchanged
design can reuse a matching version; a changed existing design can update or
save a new version. The wizard's successful completion and a library version
number are separate concepts.

Every balance repaint calls `CAdvantagePoints`, which normalizes inputs. This
is not a pure display query. On page 1 the selected preset may be the object
being priced; leaving that page copies the preset to `vplr`, then applies its
identity, password, portrait, and allocation controls. Custom clears
`ibitRaceAIPlayer` so a formerly Random design is not regenerated at world
creation. Selecting Random disables Next and selects a template for later
generation; it does not run the random generator immediately.

Presets are Humanoid, Rabbitoid, Insectoid, Nucleotid, Silicanoid, Antetheral,
and Random. Their complete values live in
[predefined.yaml](../data/races/predefined.yaml), generated as `vrgplrDef`.
Humanoid is the new-design default: all intervals 15..85 centered at 50,
growth 15, JOAT, no lesser traits, economy `10,10,10,10,10,5,10`, standard
research, and surface-mineral allocation.

Names, portrait, password, and allocation have no direct point-price term.
Portrait arrows wrap as `(index + 32 + direction) % 32`, where direction is
`-1` or `+1`; an existing index >=32 is first treated as zero.
Page 1 sets a 15-byte edit limit for each name and password. The stored name
buffers hold 31 UTF-8 bytes plus a terminator; that larger storage limit is
distinct from the wizard's edit limit. Complete UTF-8 code points must be
preserved. See [Unicode boundaries](UNICODE-BOUNDARIES.md).

## Stored inputs and bounds

`PLAYER` has a fixed 192-byte layout. Environment values, growth, and
`rgAttr[16]` are signed 8-bit fields; `grbitAttr` is a 32-bit trait mask.
File format 2.85 uses UTF-8 without widening these fields or records.
The narrow legacy race-import exception, including replacement credentials
for protected designs, is specified in the
[UTF-8 decision](adr/0002-utf8-application-text.md).

Environment axes are ordered gravity, temperature, radiation. A normal axis
stores minimum `a`, preferred `p`, and maximum `b`, in integer coordinates
0..100. Its midpoint is `p = a + div(b-a,2)`. Immunity uses `a=p=b=-1`.

| Slot | Source name | Bounds | Meaning |
| --- | --- | --- | --- |
| 0 | `rsResGen` | 7..25 | Normal races: 100 times this many colonists per resource; AR: divisor in its resource formula |
| 1 | `rsFactProd` | 5..15 | Resources per ten operating factories |
| 2 | `rsFactBuild` | 5..25 | Resources to build one factory |
| 3 | `rsFactOperate` | 5..25 | Operable factories per 10,000 colonists |
| 4 | `rsMineProd` | 5..25 | Nominal kT per ten operating mines |
| 5 | `rsMineBuild` | 2..15 | Resources to build one mine |
| 6 | `rsMineOperate` | 5..25 | Operable mines per 10,000 colonists |
| 7 | `rsUseLeftover` | 0..6 | UI exposes 0..4; 5/6 use the surface-mineral fallback |
| 8..13 | `rsTechBonus1`..`rsTechBonus6` | 0..2 | Energy, weapons, propulsion, construction, electronics, biotechnology |
| 14 | `rsMajorAdv` | 0..9 | Primary trait, in the table below |
| 15 | Unnamed slot | 0 | Zero-initialized bounds-table entry |

Research values are 0 = 75% extra cost, 1 = standard, 2 = 50% less cost.
Growth is 1..20. Lesser traits occupy bits 0..13; higher starting technology is
bit 29, Random is bit 30, and cheaper factories is bit 31. The last three
bits are not part of the lesser-trait count penalties.

`SetRaceStat` clamps a value before storing it. Economy buttons change a
setting by one, or three with Ctrl. Selecting Alternate Reality (AR) on the
primary-trait page resets slots 1..6 to `10,10,10,10,5,10` and clears the
cheaper-factory bit. Its economy page disables the factory/mine controls and
keeps the population-efficiency control active.

`BoundsCheckPlayer`, called by pricing, performs these corrections:

- Minimum `-1` forces maximum and preferred to `-1`.
- Otherwise clamp minimum to 0..100; cap maximum at 100, raise it to minimum
  if necessary, and recompute the midpoint.
- Cap growth at 20; clamp every attribute to its table bounds.
- Mark `fHacker` for corrections. Pricing separately raises growth below 1
  to 1 and marks it too.

These corrections are not a complete validator. Equal endpoints can survive,
and desirability then can divide by zero. The normal UI produces wide positive
intervals; do not assume arbitrary loaded or hand-constructed data has that
property. The integral also converts inconsistent/out-of-range environment
triples to immunity and marks `fHacker`.

## Arithmetic contract

In this document `div(x,y)` is C integer division, truncating toward zero.
For nonnegative values it is floor division. Keep each division where shown:
combining products or divisions can change the result. `trunc(x)` converts a
floating value toward zero. `min`, `max`, and `abs` have their usual meanings.

`CAdvantagePoints` accumulates a signed 32-bit internal balance `C`. Most
setting deltas, counts, growth values, and economy subtotal `cCur` are signed
16-bit values. The integral uses signed 32-bit integer radiation sums and
binary64 `double` temperature/gravity/stage accumulators. Planet desirability
uses signed 32-bit positive, negative, and modifier accumulators.

The production floating expressions use `double`, including the ordinary
`sqrt(double)`. Preserve operation and loop order, integer narrowing, and
explicit conversion boundaries. Do not reassociate the integral or replace
its integer radiation scaling with floating scaling. The formulas target
normal bounded wizard inputs; malformed-input overflow and zero-width
division are not a portable wrapping specification.

All listed point adjustments are **internal points**, unless explicitly called
displayed points. Divide the final total by three only once. In particular,
an internal total of -1 or -2 returns zero and passes Finish; -3 returns -1.
Do not reject a design based on the pre-division total.

## Environmental controls and displays

### Editing intervals

`FTrackRaceDlg2` uses a center shift `s` and half-width change `w`:

| Action | `s` | `w` |
| --- | --- | --- |
| Move left/right | -1 / +1 | 0 |
| Widen | 0 | +1 |
| Narrow | 0 | -1 |

Ctrl multiplies both deltas by ten. For each tracked step:

```text
a' = a - w + s
b' = b + w + s
if b' > 100: a' -= b'-100; b' = 100
if a' < 0:   b' = min(100,b'-a'); a' = 0
if b'-a' < 20:
    d = (20-(b'-a')) >> 1
    a' -= d; b' += d
p' = a' + div(b'-a',2)
```

This is the actual sequence, not a general final clamp. Odd loaded widths can
remain odd, and expansion occurs after endpoint correction.

Dragging retains `h=div(b-a,2)`, maps the horizontal position to 0..100 with
`MulDiv(pixelOffset,100,barWidth)`, clamps the center to `[h,100-h]`, and
sets endpoints to `center-h` and `center+h`. `MulDiv` uses nearest-integer
rounding, distinct from `div`; an odd loaded width becomes even. Bars paint
their offset as `MulDiv(a,interiorWidth,100)` and their width as
`MulDiv(b-a,interiorWidth,100)`.

Growth arrows change by one (ten with Ctrl), clamped to 1..20. Turning
immunity on sets all three fields to -1. Turning it off resets to
`a=20,p=50,b=80`; the previous range is not restored.

### Physical units

`PszCalcEnvVar` / `PszCalcGravity` format coordinates without changing pricing:

- Temperature: `4*x - 200` degrees Celsius.
- Radiation: `x` mR.
- Gravity: set `d=abs(x-50)`; `v=100+4*d` if `d<=25`, otherwise
  `v=200+24*(d-25)`. If `x<50`, replace `v` with `div(10000,v)`.
  Print `div(v,100)` followed by a decimal point and two digits `v%100`.

Thus coordinates 0,25,50,75,100 display gravity
0.12g,0.50g,1.00g,2.00g,8.00g. This is an integer piecewise scale, not a
logarithm inferred from the labels.

### The “one in N planets” estimate

`DrawRace2` calculates this independently of point pricing, growth, and
terraforming. Start `L=1`. For axes in gravity/temperature/radiation order:

1. Immunity or width 100: `L *= 100`.
2. Nonimmune radiation: `L *= b-a`.
3. Other nonimmune axes: sum `W = sum(weight(x), x=a..b inclusive)` with
   `weight(x)=x` below 10, `10` from 10 through 89, and `100-x` from 90
   onward. Set `L=div(L*W,9)`.

Then `L=max(L,1)` and `N=div(1000000+(L>>1),L)`. Display all planets for
`L=1000000`, virtually all for `N=1` with a different `L`, otherwise one in
`N`. The inclusive gravity/temperature sum and radiation's endpoint difference
are deliberately distinct. This display is not the integral described next.

## Planet desirability

`PctPlanetDesirability` is used by the integral and by gameplay. For a planet
with coordinates `x[i]`, initialize positive score `A=0`, red penalty `R=0`,
and modifier `M=10000`. Visit the three axes in order:

- Immune axis: add 10000 to `A`.
- In range (`a<=x<=b`): choose `d=p-a` if `x<p`, otherwise `d=b-p`.
  Compute `q=100-div(abs(x-p)*100,d)` and add `q*q` to `A`.
  Compute `e=2*abs(x-p)-d`. If `e>0`, set
  `M=div(M*(2*d-e),2*d)` immediately; each axis truncates separately.
- Below range: add `min(a-x,15)` to `R`.
- Above range: add `min(x-b,15)` to `R`.

If `R!=0`, return `-R`. Otherwise:

```text
B = trunc(sqrt(double(A)/3.0) + 0.9)
desirability = div(B*M,10000)
```

Return as signed 16-bit. Valid positive-width inputs give -45..100. The
`+0.9` is an explicit bias, not ordinary rounding; do not substitute `+0.5`
or a rounding-library function. At all three preferred coordinates the result
is 100. Any out-of-range axis makes the result negative, regardless of the
positive contribution from other axes.

## Integrated innate habitability

`LInnateRaceHabitability` integrates squared desirability across three stages.
It temporarily copies the race into global `rgplr[0]` for the desirability
calls, restoring the previous player afterward unless the input was already
that player. Calls sharing this global state cannot run concurrently safely.

### Stages and coordinates

| Stage | Ordinary allowance `t` | Total Terraforming allowance `t` | Weight `w` |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 7 |
| 1 | 5 | 8 | 5 |
| 2 | 15 | 17 | 6 |

For each nonimmune axis set `base=max(0,a-t)`,
`span=min(100,b+t)-base`, and use eleven samples indexed 0..10:
`x=base+div(index*span,10)`. Repeated coordinates are allowed.
For an immune axis use a single sample at 50; its integration factor is 11.

For stages 1 and 2, move a nonimmune sample toward its preferred coordinate:

```text
delta = p-x
if abs(delta) <= t: delta = 0
else if delta < 0:  delta += t
else:              delta -= t
adjusted_x = p-delta
```

Residuals are zero in stage 0 and on immune axes. For each sample triple,
obtain desirability `D` at the adjusted coordinates. Let
`S=deltaGravity+deltaTemperature+deltaRadiation`. If `S>t`, replace
`D` by `max(0,D-(S-t))`. Then contribute `w*D*D`.

**The residual sum is signed.** Opposite directions can cancel, and a large
negative sum incurs no extra penalty. Do not replace it with a sum of absolute
distances. The clamp to zero occurs only inside the `S>t` branch.

### Ordered accumulation

The loop nesting is gravity outside, temperature next, radiation inside:

```text
innate = double(0)
for stage = 0,1,2:
    gravitySum = double(0)
    for gravity sample in increasing index order:
        temperatureSum = double(0)
        for temperature sample in increasing index order:
            radiationSum = int32(0)
            for radiation sample in increasing index order:
                radiationSum += weight * adjustedDesirability^2
            if radiation immune:
                radiationSum *= 11
            else:
                radiationSum = div(radiationSum * radiationSpan,100)
            temperatureSum = double(double(radiationSum)+temperatureSum)
        if temperature immune:
            temperatureSum = double(temperatureSum*11)
        else:
            temperatureSum = double(temperatureSum*temperatureSpan/100)
        gravitySum = double(gravitySum+temperatureSum)
    if gravity immune:
        gravitySum = double(gravitySum*11)
    else:
        gravitySum = double(gravitySum*gravitySpan/100)
    innate = double(innate+gravitySum)
H = trunc(innate/10.0+0.5)
```

`H` is the returned signed 32-bit innate habitability. Only radiation scaling
uses integer division at that point. The temperature and gravity products and
divisions stay binary64. A fully nonimmune race uses 3*11^3 = 3993 sampled
planet evaluations. Immunity removes sample loops but multiplies their sums
by 11; three immune axes give `H=23958000`.

## Complete advantage-point calculation

Apply the following sections in order after normalization. Start `C=1650`.
All settings in these formulas are stored values, not displayed physical units.

### Growth and habitat

Let `g` be stored growth and `h=div(H,2000)`. Add the growth credit and choose
the spread multiplier from this table:

| Growth `g` | Credit added to `C` | Spread multiplier `s` |
| --- | --- | --- |
| 1..5 | `(6-g)*4200` | `g` |
| 6 | 3600 | 7 |
| 7 | 2250 | 9 |
| 8 | 600 | 11 |
| 9 | 225 | 13 |
| 10..13 | 0 | `2*(g-5)+5` |
| 14..19 | 0 | `3*(g-13)+21` |
| 20 | 0 | 45 |

Subtract `div(h*s,24)`. For each nonimmune axis add `4*abs(p-50)`.
Count immune axes as `I`; if `I>1`, subtract 150 once.

### Factory/growth interaction

This precedes the AR branch and therefore can apply even to an AR record with
nondefault factory values supplied outside the normal UI.
If factory operation limit `O>10` or productivity `F>10`, set
`u=max(1,O-9)`, `v=max(1,F-9)`, then multiply `v` by 3 for Hyper Expansion
(HE), otherwise 2. Subtract `div(u*v*g,2)` for `I>=2`, otherwise
`div(u*v*g,9)`. Use stored growth, not HE's doubled gameplay growth.

### Population efficiency

For population divisor `E=rsResGen`:

| `E` | Adjustment added to `C` |
| --- | --- |
| 7 | -2400 |
| 8 | -1260 |
| 9 | -600 |
| 10 | 0 |
| 11..25 | `120*(E-10)` |

### Factory settings

Skip this section and mine pricing for AR, adding 210 to `C` instead.
For other races define `a=10-F`, `b=10-factoryBuildCost`, `c=10-O`.
Build signed 16-bit subtotal `Q` sequentially:

```text
Q  = (a>0 ? 100*a : 121*a)
Q += (b<0 ? -55*b : -60*b*b)
Q += (c>0 ? 40*c : 35*c)
if Q>700: Q = 700+div(Q-700,3)
if -11<=c<=-7:  Q -= (-6-c)*30
else if -14<=c<=-12: Q -= (-12-c)*45+225
else if c<=-15: Q -= 360
if a<=-3: Q -= (-2-a)*60
C += Q
if cheaper factories selected: C -= 175
```

The diminishing credit above 700 occurs before the high-operation and
high-productivity surcharges. The operation surcharge at limits 17..25 is
30,60,90,120,150,225,270,315,360 respectively. Productivity 13..15 adds a
further cost of 60,120,180 respectively.

### Mine settings

For non-AR races set `a=10-mineProductivity`, `b=3-mineBuildCost`,
`c=10-mineOperationLimit`:

```text
Q  = (a>0 ? 100*a : 169*a)
Q += (b<=0 ? -65*b+80 : -360)
Q += (c>0 ? 40*c : 35*c)
C += Q
```

The cost reference here is 3, not the Humanoid default of 5. A mine cost of 3
adds 80; cost 2 subtracts 360. Do not discard the constant 80.

### Primary traits

Subtract the table entry from `C`; negative entries give credit.

| ID | Source symbol | Primary trait | Entry to subtract |
| --- | --- | --- | --- |
| 0 | `raCheapCol` | Hyper Expansion (HE) | 40 |
| 1 | `raStealth` | Super Stealth (SS) | 95 |
| 2 | `raAttack` | War Monger (WM) | 45 |
| 3 | `raTerra` | Claim Adjuster (CA) | 10 |
| 4 | `raDefend` | Inner Strength (IS) | -100 |
| 5 | `raMines` | Space Demolition (SD) | -150 |
| 6 | `raMassAccel` | Packet Physics (PP) | 120 |
| 7 | `raStargate` | Interstellar Traveller (IT) | 180 |
| 8 | `raMacintosh` | Alternate Reality (AR) | 90 |
| 9 | `raNone` | Jack of All Trades (JOAT) | -66 |

### Lesser traits and interactions

Add each selected entry to `C`:

| Bit | Lesser trait | Adjustment |
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

Let `nMinus` count selected negative entries, `nPlus` selected positive
entries, and `n=nMinus+nPlus`:

- If `n>4`, subtract `10*n*(n-4)`.
- If `nPlus-nMinus>3`, subtract `60*(nPlus-nMinus-3)`.
- If `nMinus-nPlus>3`, subtract `40*(nMinus-nPlus-3)`.

These counts are by price sign, not a judgment of gameplay benefit. The source
calls the negative-entry count `cBad` and the positive-entry count `cGood`.
Factory discount and higher starting technology do not enter these counts.

No Advanced Scanners additionally costs 280 for PP, 200 for SS, or 40 for
JOAT. Total Terraforming's -25 entry is additional to its effect on the
habitability integral; it is not its whole effective price.

### Research and higher starting technology

Let `S=sum(researchSetting-1)` across all six fields. This is cheap-field
count minus expensive-field count; matching cheap and expensive choices cancel.

| `S` | Adjustment added to `C` |
| --- | --- |
| -6 | +1380 |
| -5 | +1050 |
| -4 | +780 |
| -3 | +540 |
| -2 | +330 |
| -1 | +150 |
| 0 | 0 |
| 1 | -130 |
| 2 | -520 |
| 3 | -1170 |
| 4 | -2080 |
| 5 | -2730 |
| 6 | -3250 |

The positive-`S` rule is `-130*S*S`, with credits of 520 at `S=5` and 1430
at `S=6`. For `S<-4` and population divisor `<10`, subtract another 190.
Higher starting technology independently costs 180. AR with cheap energy
(`rsTechBonus1=2`) costs another 100.

### Final result

Return the low 16 bits of `div(C,3)`, interpreted as signed `int16_t`.
`DrawRaceAdvantagePoints` displays that integer, using a different text color
when negative. Finish accepts a nonnegative returned value. Fractional thirds
are not carried into the starting-bonus calculation.

## Starting benefits

### Higher starting technology

`GenerateWorld` first assigns primary-trait starting technology in the order
energy, weapons, propulsion, construction, electronics, biotechnology:

| Primary trait | Initial levels |
| --- | --- |
| HE, IS | `[0,0,0,0,0,0]` |
| SS | `[0,0,0,0,5,0]` |
| WM | `[1,6,1,0,0,0]` |
| CA | `[1,1,1,2,0,6]` |
| SD | `[0,0,2,0,0,2]` |
| PP | `[4,0,0,0,0,0]` |
| IT | `[0,0,5,5,0,0]` |
| AR | `[1,0,0,0,0,0]` |
| JOAT | `[3,3,3,3,3,3]` |

If bit 29 is set, each **expensive** research field is raised to at least 3,
or 4 for JOAT.
An already higher level stays higher. Standard and cheap fields receive no
benefit from this checkbox. Cheap Engines then adds one propulsion level;
Improved Fuel Efficiency adds another outside the tutorial.

### Leftover points

For a human race take displayed balance `P=min(CAdvantagePoints(race),50)`.
Normal creation rejects negative human balances before this stage. AI players
receive `P=50`. More than 50 unspent displayed points produce no additional
allocation bonus. Apply the selected allocation to the homeworld:

| Value | Allocation | Calculation |
| --- | --- | --- |
| 0 | Surface minerals | Let `K=10*P`, `q=K>>2`, `r=K&3`. Add `q+r` to the lowest stock, then add `q` to each of the three stocks. Total is `10*P` kT. |
| 1 | Mineral concentrations | Let `K=1` for `P=1` or 2, otherwise `div(P,2)`. Add `K` to the lowest concentration, then add `div(K+1,2)` to all three. |
| 2 | Mines | Add `P>>1` mines. |
| 3 | Factories | Add `div(P,5)` factories. |
| 4 | Defenses | Add `div(P+5,10)` defenses. |

Other allocation values use the surface-mineral case. Mineral indices are
0 ironium, 1 boranium, 2 germanium. Surface-stock ties choose the **last**
minimum in that order, whereas concentration ties choose the **first**
minimum. Select the minimum before adding any bonus.

Tough/Expert AI receiving the surface-mineral allocation also receives the
concentration bonus. AR resets starting mines, factories, and defenses to zero
after the allocation. Other races start with ten of each before their bonus.
At `P=50`, alternatives are 500 kT, concentration additions of 38 to the
lowest and 13 to each other mineral, 25 mines, 10 factories, or 5 defenses.

### Growth and initial population

`PctTrueMaxGrowth` returns `2*g` for HE and `g` otherwise. Wizard pricing always
uses `g` and its separate HE factory multiplier. The homeworld starts with
250 population units (25,000 colonists), or 175 (17,500) with Low Starting
Population. Expert AI adds `div(population,10)` units. Accelerated/BBS play
then replaces population with
`div(population*(2*PctTrueMaxGrowth(player)+10),10)`. These bonuses do not
feed back into the wizard point balance.

## Random race construction

`CreateRandomRace` in [race.cpp](../src/core/race.cpp) receives a preexisting
`PLAYER`, clears its plural name, and uses `Random(n)` results in `0..n-1`.
Preserve the RNG implementation, draw order, and even draws whose results are
later overwritten when reproducing seeded games. The result is not a uniform
sample of valid race designs.

### Initial habitat

Draw `v=Random(25)`:

| `v` | Construction |
| --- | --- |
| 0..3 | All three axes immune; growth `Random(4)+2` (2..5) |
| 4..6 | All axes 0..100 centered at 50; growth `Random(4)+3` (3..6) |
| 7..8 | Special mixed branch described below; final growth `Random(5)+2` (2..6) |
| 9..24 | For each axis: width `w=20+2*Random(40)` (20..98), minimum `a=Random(101-w)`, maximum `a+w`, preferred `a+w/2`; growth `Random(9)+7` (7..15) |

Within the 9..24 branch, 9..11 additionally replaces a random axis with
immunity; 12..13 replaces one with 0..100 centered at 50; 14..16 replaces
one with width 20 at minimum `Random(81)`; 17..24 leaves all three as drawn.

For 7..8, draw `j=Random(2)` for each axis. On the third axis, if the first
two preferred values match, override `j` with `(firstPreferred!=0)`.
If `j==0`, set that axis to 0..100 centered at 50. Otherwise consume
`Random(4)+2` and assign it to growth, **leaving the incoming axis fields
unchanged**. Finally overwrite growth with `Random(5)+2`. This branch does
not explicitly install immunity; its dependence on the incoming record is
part of the implementation.

### Remaining settings

In order:

1. Draw a `Random(3)` gate. Zero gives all-standard research; otherwise draw
   `Random(3)` independently for each of the six fields.
2. Choose primary trait with `Random(10)`.
3. Draw a `Random(4)` gate. Zero clears all fourteen lesser traits; otherwise
   draw `Random(2)` for each bit.
4. Draw independent `Random(2)` values for higher technology and cheap factories.
5. Draw a `Random(3)` gate. Zero copies economy slots 0..6 from Humanoid and
   chooses allocation with `Random(5)`. Otherwise choose each slot 0..7 as
   `minimum+Random(maximum+1-minimum)`, including allocation values 5 and 6.
6. If the singular name is exactly the localized Random label, choose one
   of 24 names using `idsBerserker+Random(24)`.

### Search for a balance in 0..50

If pricing lies outside 0..50, calculate candidate quality as
`distance(points)=max(-points,points-50)`. This expression can be negative
inside the target interval; do not replace it with a clamped distance.
Accept a candidate only if its quality is strictly lower than the current
quality. Equal-quality trials are rejected.

Choose a mutation using `Random(10)` on each iteration:

| Draw | Mutation and trial order |
| --- | --- |
| 0..2 | Pick `Random(6)` research field; try one step lower if >0, then one higher if <2, restoring rejected settings |
| 3..5 | Pick `Random(14)` lesser bit; try false then true; restore original if neither improves |
| 6..8 | Pick `Random(7)` economy stat; try original-1 then original+1 via clamping `SetRaceStat`; restore if neither improves |
| 9, next `Random(2)` nonzero | Try growth-1 if >1, then original growth+1 if <15; restore if neither improves |
| 9, next `Random(2)` zero | Pick an axis; immune becomes width 70 with minimum `Random(31)`, otherwise becomes immune; restore if rejected |

The loop test is `if (cPass++ > 250)`: up to 251 mutation iterations may
run. If the next iteration is needed, replace the design with Humanoid while
preserving its generated singular name. The fallback is not a guarantee of
a final balance in 0..50. Every evaluation can normalize the record, and
floating or point differences can change accepted mutations and subsequent
RNG state. A rewrite must preserve those effects as well as formulas.

## Related economy and research math

These are downstream meanings of the wizard's labels, not additional point
charges. Full production and turn-generation rules have further constraints.

`CResourcesAtPlanet` in [planet.cpp](../src/core/planet.cpp) uses population `N`
in units of 100 colonists and maximum population `Nmax`. If `N>Nmax`, replace
`N` with `min(Nmax+div(N-Nmax,2),2*Nmax)` first. With divisor `E`:

- Ordinary population resources: `div(N,E)`.
- Factory resources: `div(operatingFactories*rsFactProd+9,10)`; the operating
  count is the lesser of built factories and `CMaxOperableFactories`.
- AR resources: set `energy=max(energyLevel,1)` and
  `value=max(PctPlanetDesirability(planet,player),25)`, then compute
  `trunc(sqrt(double(N)*energy/E)*value/10+0.999)`.

A zero-population planet returns zero resources immediately; otherwise a
computed zero result becomes one. Preserve the source's signed 16-bit result
and narrowing when implementing the full gameplay routine. The AR page's
divisor is the raw setting 7..25, not 700..2500 colonists per resource.

`GetProductionCosts` in [produce.cpp](../src/core/produce.cpp) charges a normal
factory its selected resource cost and `4-cheapFactoryBit` kT germanium.
Tutorial factories instead require `2-cheapFactoryBit` of each mineral.
Mines have the selected resource cost and no mineral cost.

`GetTechLevelCost` in [research.cpp](../src/core/research.cpp) starts with
`B=baseCost[targetLevel]+10*sum(currentTechLevels)` across six fields.
The base-cost array for levels 0..26 is:

```text
0,50,80,130,210,340,550,890,1440,2330,3770,6100,9870,13850,
18040,22440,27050,31870,36900,42140,47590,53250,59120,65200,
71490,77990,84700
```

Research setting 0 produces `2*B-(B>>2)` (75% extra with this exact integer
rounding); setting 1 leaves `B`; setting 2 gives `div(B,2)`. Slow technology
doubles the resulting cost afterward. This actual resource price is separate
from the net-count research price in the wizard.

## Password and file checksum math

`LSaltFromSz` in [utilgen.cpp](../src/core/utilgen.cpp) processes UTF-8 bytes as
**signed** 8-bit values. Empty input returns zero. For nonempty input, start
`s=0`; add the first byte, multiply by the next byte if present, add the next,
multiply by the next, and continue. Each add/multiply wraps modulo 2^32.
Interpret the final bits as signed 32-bit, replacing a zero result with 1.
For example, ASCII `AB` gives `(0+65)*66=4290`; `ABC` gives 4357.
This describes the existing compatibility hash, not a new password scheme.

`IRaceChecksum` in [race.cpp](../src/core/race.cpp) copies the 192-byte `PLAYER`
and zeroes each name buffer from its first NUL through its end. It XORs the
96 unsigned 16-bit words of that canonicalized record, starting from zero.
That 16-bit value is written in the race EOF record. Name bytes after a
terminator must not influence it: they are not restored from the saved name.
The checksum is not the advantage balance and does not establish that two
designs are identical. Serialization remains governed by `WriteRtPlr` and
the fixed file contract, not a native struct dump invented by a new UI.

## Worked examples and verification

### Current binary64 witness

The existing `test_LInnateRaceHabitability_double_rounding` in
[test_race.cpp](../tests/unit/test_race.cpp) fixes this complete input:

```text
minimum   = [31,61,9]
preferred = [51,79,41]
maximum   = [71,97,73]
growth    = 10
traitBits = 0
attributes = [10,10,10,10,10,5,10,0,1,1,1,1,1,1,9,0]
```

Current `H=1470287`, so `h=div(H,2000)=735`, spread is 15, and the habitat
charge is `div(735*15,24)=459`. The off-center credit is
`4*(1+29+9)=156`. Factories add zero; mines add 210; JOAT adds 66. Hence:

```text
C = 1650-459+156+210+66 = 1623
displayed balance = div(1623,3) = 541
```

Historical extended arithmetic returned `H=1470286` for the same race,
still giving 541 displayed points. That coincidence does not establish
equivalence for all possible races.

For this race the separate display estimate has gravity weight 410,
temperature weight `29*10+(10+9+8+7+6+5+4+3)=342`, and radiation width 64:
`L=div(410,9)=45`, then `L=div(45*342,9)=1710`, then `L=109440`.
`N=div(1000000+54720,109440)=9`: the UI says one in nine planets.

### Boundary examples

- Three immunities: `H=(7+5+6)*10000*11^3/10=23958000`.
- Five selected negative-price lesser traits: the count surcharge is
  `10*5*(5-4)+40*(5-0-3)=130`, in addition to their individual prices.
- Three cheap and three expensive research fields: `S=0`, so the aggregate
  research charge is zero; higher-tech and AR cheap-energy terms still apply.
- One leftover point spent on concentrations adds two to the first-lowest
  mineral and one to each other mineral. One point spent on factories adds none.
- Internal point totals -2,-1,0,1,2 all display zero. At -3, Finish fails.

Existing race unit tests also exercise trait bit 31, name/checksum handling,
legacy import, UTF-8 password limits, Custom clearing Random, and habitat
painting. They do not exhaust the complete pricing domain. See the
[precision guide](../tests/scaffold/PRECISION.md) for the measured corpus.

Useful future conformance coverage includes all immunity masks, clipped
terraforming, odd-width intervals, signed residual cancellation, growth
thresholds, factory thresholds, every primary trait, all 16384 lesser-trait
masks, all 729 research combinations, and final totals around zero. These are
test suggestions, not claims that this document adds or runs those cases.

## Source map

| Responsibility | Implementation |
| --- | --- |
| Navigation, validation, controls, point/planet displays | `RaceCreationWizard`, `RaceWizardDlg1`..`RaceWizardDlg6`, `FTrackRaceDlg2`, `FTrackRaceDlg3`, `DrawRace2`, `DrawRace3`, `DrawRaceAdvantagePoints` in [raceui.cpp](../src/ui/raceui.cpp) |
| Bounds, price tables, scoring, integral, random design, checksum | `BoundsCheckPlayer`, `CAdvantagePoints`, `LInnateRaceHabitability`, `CreateRandomRace`, `IRaceChecksum` in [race.cpp](../src/core/race.cpp) |
| Units, desirability, resource production | `PszCalcEnvVar`, `PszCalcGravity`, `PctPlanetDesirability`, `CResourcesAtPlanet` in [planet.cpp](../src/core/planet.cpp) |
| Starting technologies, population, leftover allocation | `GenerateWorld` in [create.cpp](../src/core/create.cpp) |
| Research prices | `GetTechLevelCost` in [research.cpp](../src/core/research.cpp) |
| Construction costs | `GetProductionCosts` in [produce.cpp](../src/core/produce.cpp) |
| Password hash | `LSaltFromSz` in [utilgen.cpp](../src/core/utilgen.cpp) |
| Fields, IDs, masks | [structs.h](../src/core/structs.h), [enums.h](../src/core/enums.h) |
| Preset values | [predefined.yaml](../data/races/predefined.yaml) |
| Regression cases | [test_race.cpp](../tests/unit/test_race.cpp), [precision guide](../tests/scaffold/PRECISION.md) |

Source is the authority for runtime behavior. Recheck this reference whenever
the listed functions, tables, text limits, or arithmetic contract change.
