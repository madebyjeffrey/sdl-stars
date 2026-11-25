# M04 - Race advantage points

Status: source-derived specification; suggested Rust/SQL models are proposals.
Reviewed: 2026-10-23. Source: `e2eae18cff0f990db20415c1c2262aed2022f95b`.


This integrates the current-production algorithm from the earlier race report
into [M04 Race Design Wizard](M04-race-design.md). Its complete input model and
suggested Rust/SQL representations are in [Race data model](M04-race-data-model.md).
The older [calculation notes](../docs/RACE-WIZARD-CALCULATIONS.md) describe
historical extended intermediates; the current contract is binary64. See
[DOUBLE-PRECISION.md](../docs/DOUBLE-PRECISION.md).

The canonical formal definitions for normalization, ordered habitat integration and the complete point formula are in
[M04-race-specification.dfy](M04-race-specification.dfy). The prose and worked examples below
explain those definitions and retain their source evidence. See the
[coverage inventory](README.md#dafny-coverage-and-verification) for exact scope,
proof strength, dependency boundaries and verification commands.

## Story-linked requirements

This companion supplies the numerical acceptance behind
[M04-US02](M04-race-design.md#m04-us02) and the pricing-input distinction in
[M04-US04](M04-race-design.md#m04-us04). The direct actor is a calculator developer;
the player sees the balance through M04. Preconditions are the complete race
input, the specified arithmetic domain and current binary64 behavior.

Workflow: normalize with observed side effects, calculate innate habitability
in order, accumulate each price/interaction, then divide/narrow the returned
balance. Finish consumes that balance; M05 consumes capped leftover points.
Unsafe zero-width inputs need a proposed validation error, not a guessed numeric
result. Cancellation/save behavior belongs to the wizard, not this calculation.

| ID / parent story / status | Acceptance criterion | Rule, evidence and coverage |
| --- | --- | --- |
| <a id="m04-price-ac01"></a>M04-PRICE-AC01 / US02 / Observed | Given the full golden race in this document, when the current calculator runs, then innate habitability is 1470287 and balance 541. | [Acceptance](#acceptance-and-evidence); `FInnate`, `FPrice`, `PricingVectors` in [pricing model](dafny/M04-race-pricing.dfy); `test_LInnateRaceHabitability_double_rounding`. Formal subtotal is conditional on innate value; native witness supplies end-to-end evidence. |
| <a id="m04-price-ac02"></a>M04-PRICE-AC02 / US02 / Observed | Given internal point totals -2 and -3, when final division/narrowing runs, then return 0 and -1 respectively; the wizard tests the returned balance. | [Final balance](#research-starting-technology-and-final-balance); `Balance`, M04 model `Finish`; source `CAdvantagePoints`. Boundary definition; full UI witness remains missing. |
| <a id="m04-price-ac03"></a>M04-PRICE-AC03 / US02, US04 / Observed | Given unchanged pricing settings but changed name/portrait/credential/allocation, when price is calculated, then no direct price term changes; allocation affects the separate creation handoff. | [Handoff](#leftover-allocation-handoff), race model `MetadataIndependent`; [M05 bonuses](M05-universe-creation.md#45-homeworld-resources-and-leftover-points). Formal input independence; no claim that arbitrary invalid metadata bypasses validation. |

| ID / status | Interaction or quality requirement and check | Owner / gap |
| --- | --- | --- |
| <a id="m04-price-ir01"></a>M04-PRICE-IR01 / Observed interface | Return the signed displayed balance while exposing normalization side effects in the specification; a proposed pure calculator must handle those effects explicitly. Display/focus/color-independent feedback is [M04-IR01/02](M04-race-design.md#m04-ir01). | M04 wizard; no independent calculator screen or keyboard workflow. |
| <a id="m04-price-qr01"></a>M04-PRICE-QR01 / Observed | Preserve integer widths, signed division, sample order and binary64 boundaries. Compare intermediate innate values as well as final balance over the declared corpus. | [Arithmetic](#interface-and-arithmetic), S04 evaluation model and precision harness; equal final balance in one case is not general equivalence. |
| <a id="m04-price-qr02"></a>M04-PRICE-QR02 / Open | Extend conformance to the immunity/trait/research/threshold corpus in [Acceptance](#acceptance-and-evidence); measure calculation cost separately from UI latency. Exhaustive coverage and performance budgets remain unestablished. | M04, [M04-QR02/03](M04-race-design.md#m04-qr02); no new calculator or benchmark is delivered. |

S01 owns bytes, M04 data owns inputs and M05 owns starting bonuses. Proposed
safe-domain validation and a future Rust implementation remain separate decisions;
this document records current arithmetic rather than approving a change.

Checks for this story expansion are recorded in the
[collection verification](README.md#story-expansion-verification).

## Interface and arithmetic

Observed entry point: `int16_t CAdvantagePoints(PLAYER *pplr)` in
[race.cpp](../src/core/race.cpp). Inputs: three ordered habitat axes, growth,
16 attribute slots and unsigned trait mask. It normalizes the supplied race,
sets `fHacker` on corrections, and returns a signed advantage balance.
Names, password, portrait and leftover allocation have no direct pricing term.

All adjustments below are internal points. Divide by three once at the end.
`div(a,b)` denotes signed integer division truncating toward zero, including
negative values. Use i32 working integers; C economy/research subtotals are i16,
with bounded valid inputs fitting the documented intermediates. The final cast
is `(C / 3) as i16` in Rust. Use f64 for floating operations and binary64 sqrt;
preserve grouping, loop order and conversion boundaries. No fast math,
reassociation, fused multiply-add, unordered reductions or generic ROUND.
`+0.9` and `+0.5` below are intentional conversion biases.
Sources: `CAdvantagePoints`, `LInnateRaceHabitability` in [race.cpp](../src/core/race.cpp),
`PctPlanetDesirability` in [planet.cpp](../src/core/planet.cpp),
[binary64 contract](../docs/DOUBLE-PRECISION.md#platform-contract).

## Normalization

`BoundsCheckPlayer` performs these corrections:

1. If an axis minimum is -1, force preferred and maximum to -1.
2. Otherwise clamp minimum to 0..100, cap maximum at 100 and raise it to minimum
   if lower. Recompute preferred as `minimum+div(maximum-minimum,2)`.
3. Cap growth at 20 and clamp all 16 attributes to their
   [slot bounds](M04-race-data-model.md#attribute-slots-and-bounds).
4. In the point calculator, raise growth below 1 to 1.

Every changed value sets `fHacker`; an existing flag is never cleared.
The innate helper additionally converts inconsistent out-of-range axes to
immunity, marking corrections and updating its temporary player. This fallback
normally cannot be reached after the preceding normalization.

For a safe port require `minimum < maximum` on every ordinary axis and its
normalized midpoint. C normalization permits zero width, which can divide by
zero; there is no defined compatible numeric result for that case. Width one
is safe because its unused lower half never divides; preserve odd widths and
do not impose a new width-20 limit on imports. Source: `BoundsCheckPlayer`,
`LInnateRaceHabitability` in [race.cpp](../src/core/race.cpp).

## Planet desirability

Process axes in gravity, temperature, radiation order. Initialize integer
`A=0`, `R=0`, `M=10000`. For each coordinate x with race minimum a,
preferred p and maximum b:

1. If `b<0`, add 10000 to A (immune).
2. Otherwise, if `a<=x<=b`, set `d=p-a` if `x<p`, else `d=b-p`.
   Set `q=100-div(abs(x-p)*100,d)` and add `q*q` to A.
   Let `e=2*abs(x-p)-d`. If `e>0`, immediately set
   `M=div(M*(2*d-e),2*d)` before the next axis.
3. Below range add `min(a-x,15)` to R; above range add `min(x-b,15)`.

If R is nonzero return -R. Otherwise set
`V=trunc_to_i32(sqrt(f64(A)/3.0)+0.9)` and return `div(V*M,10000)`.
Everything except the sqrt expression is integer arithmetic. Valid desirability
is -45..100. Source: `PctPlanetDesirability` in [planet.cpp](../src/core/planet.cpp).

## Integrated innate habitability

Initialize f64 `innate=0.0` and signed residuals `[0,0,0]`. Use three stages:

| Stage | Ordinary terraform allowance t | With TT (bit 1) | Weight |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 7 |
| 1 | 5 | 8 | 5 |
| 2 | 15 | 17 | 6 |

For each stage prepare each axis. Immune axes have base=50, width=11, steps=1.
Ordinary axes have `base=max(0,minimum-t)`,
`width=min(100,maximum+t)-base`, steps=11. Sample index j=0..10 at
`base+div(j*width,10)`; an immune axis has the single base sample.
Repeated coordinates for narrow habitats are intentional.

For an ordinary axis in stages 1/2 let `delta=preferred-sample`. If
`abs(delta)<=t`, replace delta with zero; otherwise add t if negative or
subtract t if positive. Store this signed residual for the axis and evaluate
coordinate `preferred-delta`. Stage 0 evaluates unmodified samples with zero
residuals; immune residuals remain zero.

For each sample triple calculate desirability D. Set r to the signed sum of
all three residuals (not absolute values). Only if `r>t`, replace D by
`max(0,D-(r-t))`. Then sample term is `D*D*weight`. Negative D is otherwise
squared without a preceding clamp.

```text
innate: f64 = 0.0
for stage in [0,1,2]:
    prepare axis bases, widths, steps
    l3: f64 = 0.0
    for gravity sample in increasing index order:
        l2: f64 = 0.0
        for temperature sample in increasing index order:
            l1: i32 = 0
            for radiation sample in increasing index order:
                l1 += sample_term
            if radiation ordinary: l1 = div(l1 * width[2],100)
            else:                  l1 = l1 * 11
            l2 = f64(l1) + l2
        if temperature ordinary: l2 = (l2 * f64(width[1])) / 100.0
        else:                    l2 = l2 * 11.0
        l3 = l3 + l2
    if gravity ordinary: l3 = (l3 * f64(width[0])) / 100.0
    else:                l3 = l3 * 11.0
    innate = innate + l3
return trunc_to_i32((innate / 10.0) + 0.5)
```

Radiation scaling truncates as integer; temperature/gravity scaling is f64.
Do not permute axes or algebraically combine the divisions. Three immune axes
give f64 accumulated 239580000.0 and integer return 23958000.
The source temporarily replaces global `rgplr[0]` to call desirability, then
restores it unless input is itself `rgplr[0]`. Concurrent use of that shared
global is unsafe; a proposed Rust helper can accept a race reference directly.
Source: `LInnateRaceHabitability` in [race.cpp](../src/core/race.cpp).

## Complete pricing formula

Normalize first. Set `C=1650`, `H=div(integrated_innate,2000)`, g=normalized
growth and I=count of immune axes. Apply the following sections in order.
All terms follow `CAdvantagePoints` and its constant arrays in
[race.cpp](../src/core/race.cpp). C denotes the running internal point total.

### Growth and habitat

| Growth g | Direct credit added to C | Multiplier s |
| --- | --- | --- |
| 1..5 | (6-g)*4200 | g |
| 6 | 3600 | 7 |
| 7 | 2250 | 9 |
| 8 | 600 | 11 |
| 9 | 225 | 13 |
| 10..13 | 0 | 2*(g-5)+5 |
| 14..19 | 0 | 3*(g-13)+21 |
| 20 | 0 | 45 |

Subtract `div(H*s,24)`. For each ordinary preferred coordinate p add
`4*abs(p-50)`. If I>1 subtract 150.

### Factory/growth interaction and population efficiency

Let O=factory operation and P=factory output. If O>10 or P>10 set
`o=max(1,O-9)`, `p=max(1,P-9)*(3 if primary=HE else 2)`.
Subtract `div(o*p*g,2)` for I>=2, otherwise `div(o*p*g,9)`.
This applies even to AR if saved factory settings trigger it, regardless of
what the UI normally resets.

Population divisor R: subtract 2400 for 7, 1260 for 8, 600 for 9;
no change for 10; add `120*(R-10)` for 11..25.

### Factory and mine settings

If primary is AR (ID 8), add 210 and skip both subtotals and the factory
germanium discount price below. Otherwise:

Factory subtotal with `a=10-factory_output`, `b=10-factory_cost`,
`c=10-factory_operation`:

1. `T=100*a` if a>0, else `121*a`.
2. Subtract `55*b` if b<0, otherwise `60*b*b`.
3. Add `40*c` if c>0, otherwise `35*c`.
4. If T>700 set `T=700+div(T-700,3)`.
5. If -11<=c<=-7 subtract `30*(-6-c)`; if -14<=c<=-12 subtract
   `45*(-12-c)+225`; if c<-14 subtract 360.
6. If a<=-3 subtract `60*(-2-a)`.
7. Add T to C. If cheaper factories (bit 31) selected, subtract 175 from C.

Mine subtotal with `a=10-mine_output`, `b=3-mine_cost`,
`c=10-mine_operation`:

1. `T=100*a` if a>0, else `169*a`.
2. Subtract `65*b-80` if b<=0, otherwise 360.
3. Add `40*c` if c>0, otherwise `35*c`.
4. Add T to C.

The mine-cost branch gives +80 when cost is 3; do not replace it with a
supposedly neutral baseline.

### Primary and lesser traits

Subtract the primary table entry (negative entries give credit):

| ID | Trait | Subtract |
| --- | --- | --- |
| 0 | HE | 40 |
| 1 | SS | 95 |
| 2 | WM | 45 |
| 3 | CA | 10 |
| 4 | IS | -100 |
| 5 | SD | -150 |
| 6 | PP | 120 |
| 7 | IT | 180 |
| 8 | AR | 90 |
| 9 | JOAT | -66 |

For each selected bit 0..13 add the corresponding adjustment:

| Bit | Trait | Add |
| --- | --- | --- |
| 0 | Improved Fuel Efficiency | -235 |
| 1 | Total Terraforming | -25 |
| 2 | Advanced Remote Mining | -159 |
| 3 | Improved Starbases | -201 |
| 4 | Generalized Research | 40 |
| 5 | Ultimate Recycling | -240 |
| 6 | Mineral Alchemy | -155 |
| 7 | No Ram Scoop Engines | 160 |
| 8 | Cheap Engines | 240 |
| 9 | Only Basic Remote Mining | 255 |
| 10 | No Advanced Scanners | 325 |
| 11 | Low Starting Population | 180 |
| 12 | Bleeding Edge Technology | 70 |
| 13 | Regenerating Shields | 30 |

Let nminus count selected negative entries, nplus selected positive entries,
n=nminus+nplus. If n>4 subtract `10*n*(n-4)`.
If nplus-nminus>3 subtract `60*(nplus-nminus-3)`.
If nminus-nplus>3 subtract `40*(nminus-nplus-3)`.
Only bits 0..13 enter these price-sign counts; they do not classify all
possible gameplay benefits.

If No Advanced Scanners is selected, additionally subtract 280 for PP,
200 for SS or 40 for JOAT. TT's table adjustment is in addition to its
changed habitat integral.

### Research, starting technology and final balance

Set `S=sum(attribute[i]-1 for i=8..13)`.

- S>0: subtract `130*S*S`, then add 1430 for S=6 or 520 for S=5.
- S<0: add `[150,330,540,780,1050,1380][-S-1]`.
  If -S>4 and population divisor<10 subtract another 190.
- S=0: no aggregate research adjustment.

Cheap and expensive fields cancel in this net count. If higher starting
technology (bit 29) is selected, subtract 180 independently. If primary is AR
and energy cost choice is 2, subtract 100.

Return signed low 16 bits of `div(C,3)`: Rust `(C/3) as i16`.
Bounded valid inputs fit the return. Finish accepts returned balance >=0.
Internal totals -1 and -2 return zero; -3 returns -1. Do not test C<0
instead of the returned balance. Source: `CAdvantagePoints` in
[race.cpp](../src/core/race.cpp), `RaceCreationWizard` in [raceui.cpp](../src/ui/raceui.cpp).

## Leftover allocation handoff

Allocation does not change balance. For human world creation M05 uses
`min(balance,50)`; AI players receive 50. Choices 0..4 mean surface minerals,
concentrations, mines, factories, defenses; 5/6 use the surface-minerals default.
Exact bonus and tie rules belong to `GenerateWorld` in [create.cpp](../src/core/create.cpp),
also described in [the bonus notes](../docs/RACE-WIZARD-CALCULATIONS.md#leftover-points-and-related-gameplay-quantities).
This spec does not duplicate those gameplay generation rules.

## Acceptance and evidence

Existing test `test_LInnateRaceHabitability_double_rounding` in
[test_race.cpp](../tests/unit/test_race.cpp) defines this complete witness:

```text
minimum   = [31,61,9]
preferred = [51,79,41]
maximum   = [71,97,73]
growth = 10
trait_bits = 0
attributes = [10,10,10,10,10,5,10,0,1,1,1,1,1,1,9,0]
expected innate habitability = 1470287
expected balance = 541
```

Historical extended innate was 1470286, with the same balance. New implementations
should cover all immunity masks, clipped terraforming, odd widths, signed
residual sums, factory thresholds, all primary traits, all 16384 lesser-trait
masks, all 729 research combinations and negative totals near division by three.
These are proposed conformance cases, not claims of exhaustive existing coverage.
See [the precision guide](../tests/scaffold/PRECISION.md) for measured coverage.
No new Rust or SQL calculator is implemented by this documentation.
