# Race properties and PRT/LRT effects

Source-derived research report. Inspected on **November 5, 2026**, against
`main` source at `34f9b46482a5edba009ce91b9cd550fa4ac4602b`.
The working tree also contains RDL implementation changes; the game sources
cited here were unchanged during this audit. This report changes no game rules.

This inventory is organized by **property**, rather than wizard page or source
file. It covers the reusable race design inputs and the trait-dependent policies
that turn those inputs into population, colony, fleet, equipment and starting
conditions. A calculated property can require a planet, fleet, technology level,
game setting or event; it is not necessarily one constant that can be stored on
a race. Current player technology, accumulated research, planet/fleet counts,
diplomatic relations, orders and scores are mutable game state, not additional
intrinsic race design properties merely because they share `PLAYER` storage.

Names in the property column are descriptive labels, not existing RDL field
names or new implementation requirements. **Ordinary baseline** means the
behavior without the named exception, not a PRT-free race: a completed Stars!
race always selects a PRT. **Wizard default** means the Humanoid preset, which
is JOAT and already has that PRT's effects. A dash in an effect column means no
direct trait rewrite of that input was found; downstream calculations may still
use it differently.

## 1. Property inventory

### 1.1 Stored design inputs

The authoritative field/bounds definitions are [`PLAYER`](../src/core/structs.h#L1884),
[`RaceStat`, `RaceGrbit`, `RaceAttribute`](../src/core/enums.h#L145),
[`BoundsCheckPlayer`](../src/core/race.cpp#L55), and the
[Humanoid defaults](../data/races/predefined.yaml#L8).

| Property | Source / ordinary value or wizard default | PRT effect on the input | LRT effect on the input |
| --- | --- | --- | --- |
| Singular race name | `szName`; identity text, independent of price | — | — |
| Plural race name | `szNames`; identity text, independent of price | — | — |
| Portrait | `iPlrBmp`, 0..31; default 1 | — | — |
| Password verifier / protection | `lSalt`; credential, not a gameplay bonus | — | — |
| Random-template selection | Bit 30, misleadingly named `ibitRaceAIPlayer`; distinct from actual AI controller `fAi` | Random generation selects a PRT later | Random generation chooses LRT flags later |
| Selected primary racial trait | `rsMajorAdv`, one enum 0..9; default JOAT | Selects the PRT rule bundle | — |
| Selected lesser racial traits | `grbitAttr` bits 0..13; default none | No automatic LRT selection | Fourteen independently selectable flags; effective capabilities can override one another |
| Gravity minimum, preferred, maximum, immunity | `rgEnvVarMin[0]`, `rgEnvVar[0]`, `rgEnvVarMax[0]`; default 15,50,85 | Not rewritten merely by choosing HE; CA/PP can change a **planet**, not the race's tolerance | TT changes available/priced terraforming, not the stored tolerance |
| Temperature minimum, preferred, maximum, immunity | Same arrays, index 1; default 15,50,85 | Same distinction | Same distinction |
| Radiation minimum, preferred, maximum, immunity | Same arrays, index 2; default 15,50,85 | Same distinction | Same distinction |
| Selected ideal growth | `pctIdealGrowth`, 1..20; default 15 percent | HE's effective growth doubles; the stored choice remains unchanged | LSP changes starting population, not this setting |
| Population resource divisor | `rsResGen`, 7..25 hundreds of colonists per annual resource; default 10 means 1,000 colonists/resource | AR uses this divisor inside a different resource formula | No direct rewrite |
| Factory output | `rsFactProd`, 5..15 resources per ten operating factories; default 10 | AR disables normal factory economy | — |
| Factory resource construction cost | `rsFactBuild`, 5..25; default 10 | AR disables normal factory construction | — |
| Factory operation limit | `rsFactOperate`, 5..25 per 10,000 colonists; default 10 | AR replaces ordinary installation operation | OBRM can raise maximum **buildable** count via population capacity |
| Factory germanium discount | Bit 31, `ibitRaceCheapFact`; default off | Choosing AR in the wizard clears it | Not an LRT; reduces factory germanium requirement by one |
| Mine output | `rsMineProd`, 5..25 nominal kT per ten operating mines; default 10 | AR uses automatic mining | — |
| Mine resource construction cost | `rsMineBuild`, 2..15; default 5 | AR has no ordinary mine construction | — |
| Mine operation limit | `rsMineOperate`, 5..25 per 10,000 colonists; default 10 | AR uses automatic mining | OBRM affects buildable count through capacity, not this stored number |
| Energy research cost class | `rsTechBonus1`, 0 expensive / 1 normal / 2 cheap; default 1 | AR cheap Energy has an extra wizard charge | GR redistributes research, rather than changing this price class |
| Weapons research cost class | `rsTechBonus2`, same encoding/default | — | Same GR distinction |
| Propulsion research cost class | `rsTechBonus3`, same encoding/default | — | Same GR distinction |
| Construction research cost class | `rsTechBonus4`, same encoding/default | — | Same GR distinction |
| Electronics research cost class | `rsTechBonus5`, same encoding/default | — | Same GR distinction |
| Biotechnology research cost class | `rsTechBonus6`, same encoding/default | — | Same GR distinction |
| Higher starting technology | Bit 29, `ibitRaceTech3`; default off | JOAT uses minimum 4 instead of 3 in qualifying fields | IFE/CE add Propulsion afterward; not LRT-counted itself |
| Leftover-point allocation | `rsUseLeftover`; UI 0..4, source bounds 0..6; default minerals | AR removes starting installations after allocation | LRTs change the point balance feeding allocation |

The stored name buffers allow 31 UTF-8 bytes plus a terminator; the wizard's
name and password edit controls impose a separate 15-byte limit. Byte limits
must preserve complete code points. These identity constraints have no PRT/LRT
multiplier. Sources: [name buffers](../src/core/structs.h#L1947),
[identity and input limits](NEW-RACE-WIZARD.md#workflow-and-page-order).

Normal habitat coordinates are integers 0..100 with preferred value recomputed
as `min + (max-min)/2`. Immunity is the triple `(-1,-1,-1)`, not a wide interval.
Temperature display is `4*x-200` °C; radiation display is `x` mR; gravity display
uses a nonlinear integer conversion. Input limits, units, mutation during
pricing, and the unsafe zero-width input boundary are detailed in
[the wizard reference](NEW-RACE-WIZARD.md#stored-inputs-and-bounds).

Selecting AR in the wizard resets factory/mine slots 1..6 to
`10,10,10,10,5,10`, clears cheaper factories and disables those controls;
this is UI normalization, not a guarantee about arbitrary hand-built records.
See [`RaceWizardDlg4`](../src/ui/raceui.cpp#L1107).

There is **no free-form race description field** in the original `PLAYER`
design. PRT/LRT display names and descriptions are resource/catalog text; raw
trait point prices are constant arrays, not extra per-race fields. RDL's
`description` is useful added metadata, not a recovered Stars! storage field.

### 1.2 Effective population, habitat and economy properties

These rows identify the resulting properties; their exact rules and source
citations follow in the detailed sections.

| Property | Ordinary baseline / dependencies | PRT effects | LRT effects |
| --- | --- | --- | --- |
| Effective ideal growth rate | Selected growth `g` | HE: `2*g` | None directly |
| Planet carrying capacity | Habitat-dependent; 1,000,000 colonists at 100% | HE half; JOAT +20%; AR orbital-hull model | OBRM +10% after PRT |
| Orbital habitation / need for a starbase | Ordinary races live on the surface | AR lives in orbital bases; base loss can kill colony | OBRM still affects AR capacity |
| Annual colony population change | Habitat, current population, capacity and growth | HE rate; AR capacity affects density | OBRM affects density through capacity |
| Population growth in fleet cargo | None | IS breeds in transit and can unload excess to friendly owned worlds | None directly |
| Initial homeworld population | 25,000 before setup modifiers | IT/PP second-world split; HE affects accelerated-start factor | LSP selects 17,500; game mode/AI modifiers also apply |
| Number and population of starting colonies | One world normally | IT/PP get second world outside Tiny; 40%/80% allocation | LSP influences the population entering that split |
| Planet habitability / desirability | Three-axis tolerance calculation | CA/PP change environments; AR uses habitat differently for resources | TT changes available terraforming, not tolerance |
| Displayed one-in-N planet estimate | Separate wizard estimate from current tolerance | No direct PRT pricing substitution | Not the TT future-habitat integral |
| Innate habitability used for pricing | Ordered sampled integral with future terraforming | HE uses its result with a different factory/growth charge, not a rewritten tolerance | TT changes integral allowances |
| Terraforming technology allowance | Tech- and axis-dependent | CA automatic/orbital behavior; PP packet effects | TT technology/cost/axis rules |
| Paid terraforming cost | Resource cost per selected improvement | CA free annual adjustment is a separate mechanism | TT reduces paid terraforming cost |
| Permanent environment change | No generic racial annual improvement | CA probability branch; PP uncaught-packet branch | Not TT's ordinary temporary terraforming |
| Population resource output | Population divisor and crowding adjustment | AR square-root/Energy/habitat formula | Capacity effects can change crowding; no direct LRT population-output factor |
| Factory resources | Operating factories and selected output | AR replaces factory economy | No direct factory-output factor |
| Buildable / operable factories | Capacity vs actual population; distinct limits | HE/JOAT/AR differences through model | OBRM changes capacity-derived ceiling |
| Buildable / operable mines | Capacity vs actual population; selected efficiency | AR automatic mines | OBRM capacity-derived ceiling |
| Buildable / operable planetary defenses | Habitat bound 10..100; population operation bound | AR zero; WM restricts available defense types | No direct LRT count factor |
| Factory construction resources/minerals | Selected resource cost; normal 4 kT germanium | AR installation exclusion | Separate cheaper-factories flag changes 4 to 3 |
| Mine construction resources/minerals | Selected resource cost; no mineral requirement | AR installation exclusion | None directly |
| Automatic/remote mining | Mining equipment plus concentration/depletion | AR automatic mines and remote mining of own colonies | ARM grants/restrictions; OBRM overrides ARM eligibility |
| Mineral alchemy conversion cost | 100 resources for one unit of each mineral | No direct PRT factor | MA: 25 resources |
| Research level resource cost | Selected cost class, level and attained tech | Starting tech differs; AR Energy matters for output/price | GR allocation; SS research gain is a PRT rule |
| Research allocation/spillover | Chosen field receives normal research | SS research benefit | GR changes main-field and other-field distribution |
| Wizard advantage-point balance | Full nonlinear calculation, not sum of standalone displayed trait prices | All PRT raw prices; HE/AR/NAS interactions | All LRT raw adjustments; count/sign/TT/NAS interactions |

### 1.3 Effective ship, fleet, information and starting properties

| Property | Ordinary baseline / dependencies | PRT effects | LRT effects |
| --- | --- | --- | --- |
| Allowed ship hulls | Hull technology and race eligibility | HE, SS, WM, IS, SD, AR exclusive/restricted hulls | ARM/OBRM remote-mining hulls |
| Allowed engines and fuel scoops | Engine technology and eligibility | HE Settler's Delight | IFE Fuel Mizer/Galaxy Scoop; NRSE exclusions/Interspace-10 |
| Allowed weapons, armor, shields and special devices | Component technology and eligibility | Unique SS/WM/IS/CA/SD/PP/IT/AR equipment and exclusions | ISB, ARM, NAS, TT restrictions/grants |
| Component construction cost | Base part costs, technology miniaturization, minimum costs | WM weapons discount; IS weapons surcharge; IT gates; CA terraformer | CE engines; BET miniaturization/new-tech penalty |
| Completed starbase construction cost | Separate starbase reduction pipeline | AR discount | ISB shares the same discount condition; does not stack twice |
| Ship fuel consumption | Engine, speed, hull, weight | HE's special engine is equipment-specific | IFE adjustment |
| Engine departure failure | No general failure at ordinary warp | — | CE 10% departure failure above Warp 6, excluding requested speed 11 |
| Battle movement speed | Engine, thrusters, mass/cargo and clamp | WM adds two internal speed units | Equipment grants can affect it; not a blanket CE speed penalty |
| Armor strength | Hull + component armor | Unique racial equipment changes base armor | RS halves applicable component armor, not bare hull armor |
| Shield strength and regeneration | Equipped shields, battle state | Unique racial equipment | RS increases shield strength and regenerates during combat |
| Repair/healing rate | Location, movement and transport-healing equipment | IS doubles location rate before additive equipment bonus | No general RS post-battle repair multiplier |
| Ground attack strength | Ordinary multiplier uses 110 | WM 165; AR special colonization branch | No direct LRT multiplier |
| Ground defense strength | Ordinary multiplier 100 | IS 200; AR habitation changes invasion outcome | No direct LRT multiplier |
| Cloaking contribution and cargo treatment | Equipped cloak points; weighted fleet calculation | SS +300 points and cargo exclusion | ISB bases +40 points |
| Nonpenetrating/penetrating scan range | Scanner equipment, hull and technology | JOAT built-ins; AR innate colony scan; PP packet scan; IT gates | NAS ordinary-range bonus and penetrating-scanner restrictions |
| Remote cargo/mineral theft | Ordinarily unavailable | SS special scanners enable rules | NAS does not simply remove SS equipment eligibility |
| Enemy ship/base design disclosure | Included designs normally partial | WM full design detail; SD mine hits teach affected designs | No direct LRT disclosure factor |
| Cloaked-target detection factor | Sensor range and cloak efficacy | IS Tachyon Detector grant supplies lookup-table factor | No generic NAS anti-cloak multiplier beyond normal-range changes |
| Fuel tank/cargo capacity and passive fuel generation | Hull and fitted modules | IT generator and IS transport hull grants; no blanket SS cargo multiplier | IFE/NRSE affect eligible engines, not cargo space |
| Colonist losses aboard moving ships | Movement/engine-dependent | AR first-pass population loss | NRSE can exclude a radiating engine; not a direct survival multiplier |
| Available intelligence about foreign colonies/fleets | Normal scanner/alliance/visibility policy | CA/IT/SS/AR special handling | NAS restrictions affect what can be learned |
| Safe minefield speed / collision policy | Mine-type safe speed | SS +1; SD +2 | No direct LRT adjustment |
| Mine laying quantity | Equipped minelayers and hull | SD-exclusive hulls double laying; SD moving-lay rules | Not a universal multiplier on every SD hull |
| Minefield detection / scanning | Normal visibility | SD minefields serve as scanners | — |
| Minefield decay | Field geometry/planets/type | SD changes planet-density contribution | — |
| Minefield remote detonation | Normal UI does not offer it | SD-owned standard fields expose it | — |
| Gate cargo permission | Ordinary gate policy excludes cargo transport | IT cargo permissions | — |
| Gate range/mass safety and ship losses | Gate limits, masses, distance, damage rolls | IT altered overgating/loss policy and gate information | Equipment eligibility/ISB can affect available bases |
| Packet launch cost, batch size and safe speed | Driver/packet type | PP batches/drivers, IT overhead/speed behavior | — |
| Packet decay/catching/damage/terraforming | Driver speed, catcher, mass and flight | PP reduced decay/scans/terraforming; IT catching penalty; AR skips colony damage branch | — |
| Scrapping/recycling recovery | Scrap/recycle task, destination, damage and part costs | Cost modifiers change recovered base | UR raises mineral recovery and supplies resource recovery |
| Starting technologies | PRT vector, then setup additions | Each PRT's vector | CE/IFE add Propulsion; separate higher-tech flag |
| Starting ships/designs/base/drivers/gates | PRT, actual starting tech, AI/tutorial | PRT-specific fleets, orbital equipment and extra colonies | ARM starter miners unless OBRM; IFE/CE affect fitted tech |
| Starting factories/mines/defenses | Ten each before point allocation | AR resets all three to zero; second worlds differ | Price balance affects allocation, not a flat LRT structure factor |
| Starting surface minerals/concentrations | Generated world and leftover-point allocation | AR can suppress installation bonuses; AI exceptions | Prices change available leftover points |

## 2. Reading the calculations

Population is normally stored in **100-colonist units** (`rgwtMin[3]`). A
value of 250 means 25,000 colonists. Mineral quantities are kT. Habitat
coordinates are not physical gravity or temperature units. Use `div(a,b)` for
C++ signed integer division toward zero; nonnegative shifts/divisions truncate.
A displayed percentage, internal speed unit, cloak point, and raw wizard point
are different units and must not share one generic multiplier accidentally.

Current game arithmetic uses binary64 at its existing conversion boundaries.
Expressions below describe source order; they are not permission to combine
successive integer operations algebraically. See
[the precision contract](DOUBLE-PRECISION.md) and
[existing wizard arithmetic reference](NEW-RACE-WIZARD.md#arithmetic-contract).
The existing [race-data](../spec/M04-race-data-model.md) and
[pricing](../spec/M04-race-advantage-points.md) specifications own their formal
contracts. This document is a research inventory of observed source paths,
not a new executable/formally verified specification of every turn system.

## 3. Population and carrying capacity

### 3.1 Effective ideal growth

`g = pctIdealGrowth` is the selected percentage, normally 1..20. The effective
ideal percentage is `2*g` for HE, `g` otherwise. This does **not** rewrite the
stored setting, and point pricing continues to use the stored setting.
Source: [`PctTrueMaxGrowth`](../src/core/race.cpp#L925).

### 3.2 Planet carrying capacity

Compute in internal 100-colonist units, in this order:

```text
if AR:
    if planet is not owned by the queried player or has no starbase: return 0
    C = orbital_hull_capacity[starbase hull]
else:
    H = PctPlanetDesirability(planet, race)
    C = 500 if H < 5 else H * 100
    if HE:   C = C - div(C, 2)
    if JOAT: C = C + div(C, 5)

if OBRM: C = C + div(C, 10)
return C
```

HE's subtraction rounds a half unit **up**, unlike `div(C,2)` for odd `C`.
OBRM follows the PRT and also applies to AR. Convert to actual colonists only
after these operations. The 50,000-colonist floor is before PRT/LRT modifiers.
This is a growth/economy reference capacity, not a hard prohibition on storing
more colonists on the planet.
Sources: [`CalcPlanetMaxPop`](../src/core/planet.cpp#L436),
[AR capacity array](../src/core/appstate.h#L136),
[orbital hull order](../data/parts/starbase-hulls.yaml).

| Race/model at 100% habitat | Without OBRM | With OBRM |
| --- | ---: | ---: |
| Ordinary surface model | 1,000,000 | 1,100,000 |
| HE | 500,000 | 550,000 |
| JOAT | 1,200,000 | 1,320,000 |
| AR Orbital Fort | 250,000 | 275,000 |
| AR Space Dock | 500,000 | 550,000 |
| AR Space Station | 1,000,000 | 1,100,000 |
| AR Ultra Station | 2,000,000 | 2,200,000 |
| AR Death Star | 3,000,000 | 3,300,000 |

For an ordinary surface race at 50% habitat, the pre-trait result is 500,000,
not 1,000,000. AR hull capacity does not depend on habitat, but AR growth and
resource production still do. A static `CarryingCapacity.value = 1_000_000`
therefore describes a reference baseline, not every planet's resolved capacity.

AR's habitation dependency also affects colony survival. When its starbase
dies in battle, the later battle-message/resolution path records the old
population and calls `UninhabitPlanet`, removing the colony; ordinary races
lose the base while retaining their surface colony. Sources:
[base destruction](../src/core/battle.cpp#L2132),
[`SendBattleMessages`](../src/core/battle.cpp#L2589).

### 3.3 Annual colony growth and overcrowding

Let `P` be current population units, `C` the capacity above, `H` desirability,
and `G` effective ideal growth. Unowned or empty planets have no change.
For `H<0`, loss is based on `max(1,div(P*(-H),10))` in hundredths of an
internal population unit, with the remainder subtracted from persistent
`iDeltaPop`; borrowing can produce a whole population-unit loss. This is
approximately `(-H)/10` percent annual death, not negative growth at `G`.
Source: [`ChgPopFromPlanet`](../src/core/util.cpp#L2246).

For `H>=0`, initialize `g100=G*H`. Up to `div(C,4)` population units there is
no crowding taper. Above it calculate `full=div(P*1000,C)`:

- At/above capacity, `P<C+10` returns zero change. Otherwise replace
  `g100` by `4*max(-300,99-div(full,10))` for overcrowding losses.
- Below capacity, let `r=(1000-full)^2`. If `g100<1000`, use
  `div(g100*r,562500)`; otherwise use
  `div(div(g100,10)*r,562500)*10`. Preserve these separate divisions.

The source then computes `Q=P*div(g100,100)` and, if `Q<10000000`, replaces
it with `div(P*g100,100)`. Whole change is `div(Q,100)` and remainder `Q%100`.
It accumulates the remainder in `iDeltaPop` and carries/borrows at 100. A zero
quotient and remainder is forced to remainder +1. Mutation is conditional on
`fUpdate`. The PRT/LRT effects here are HE's `G` and HE/JOAT/AR/OBRM's `C`;
there is no separate generic AR growth-rate multiplier in this routine.
Source: [`ChgPopFromPlanet`](../src/core/util.cpp#L2284).

### 3.4 Colonists aboard fleets

Ordinary races do not breed aboard fleets. IS's yearly gain is
`div(P*g,200)` population units: half the **selected** growth percentage. If
that integer result is zero, it awards one unit with probability 1/3 instead.
`ChgCargo` limits gain by available fleet capacity. Excess goes to an orbiting
colony only when owned by the same player, and can take it over normal colony
capacity; otherwise excess is discarded. The function includes stationary
loaded fleets too; its name is not a movement requirement.
Source: [`BreedColonistsInTransit`](../src/core/turn2.cpp#L2052).

AR has a separate first movement-pass loss when cargo population exceeds ten
internal units: subtract `div(3*P+33,100)`. This is not the same calculation as
IS breeding or planetary red-habitat deaths.
Source: [`MoveFleets`](../src/core/turn.cpp#L1497).

The Radiating Hydro-Ram Scoop also has an **equipment-mediated** colonist
mortality path. With a radiating engine, nonzero cargo population, movement
pass at most 1, nonimmune radiation and `radMin+radMax<170`, calculate:

```text
center = div(radMin + radMax, 2)
rate   = (86 - center) >> 1
D      = div(rate * P, 100), stored in the source's signed 16-bit pct local
deaths = min(P, max(1, D))
```

Subtract deaths from cargo population. The source's narrow conversion before
clamping must be retained if reproducing large-fleet boundary behavior. This
is controlled by actual engines and tolerance endpoints, not a universal
NRSE/IFE/HE population factor. The engine flag is set in
[`EstFuelUse`](../src/core/ship.cpp#L398), and deaths are applied in
[`MoveFleets`](../src/core/turn.cpp#L1686).

## 4. Habitability and terraforming

### 4.1 Planet desirability

This is a calculated planet/race relationship. Initialize positive sum `S=0`,
red penalty `N=0`, and multiplier `M=10000`. For each of three axes:

- Immunity adds 10000 to `S`.
- Inside the interval, let `a=abs(environment-preferred)` and `d` be the
  distance from preferred to the bound on that side. Set
  `v=100-div(100*a,d)` and add `v*v` to `S`. If `q=2*a-d>0`, replace
  `M` with `div(M*(2*d-q),2*d)`.
- Outside the interval, add `min(distance outside,15)` to `N`.

If `N>0`, return `-N` (down to -45). Otherwise return
`div(trunc(sqrt(S/3.0)+0.9)*M,10000)`. There is no direct HE/JOAT/LRT
replacement of this function. CA/PP change its planet inputs; AR uses its
result in growth and resources despite hull-based capacity. Malformed zero-width
ranges can divide by zero in this source; bounds correction is not a strict
safe-domain validator.
Source: [`PctPlanetDesirability`](../src/core/planet.cpp#L380).

The wizard's **one-in-N planets display** is a different tolerance calculation,
not this function at a specific planet and not future-habitat pricing.
Its exact gravity/temperature/radiation weights are documented in
[the display reference](NEW-RACE-WIZARD.md#the-one-in-n-planets-estimate),
owned by [`DrawRace2`](../src/ui/raceui.cpp).

### 4.2 Available terraforming, cost and selection

Terraforming reach is an amount of **environment-coordinate change from the
planet's original environment**, not final habitability percentage. The runtime
chooses the maximum available general terraformer ability, then the maximum
of that and an available per-axis ability; it does not add their allowances.
Source: [`FCanTerraformLppl`](../src/core/planet.cpp#L785).

| Terraforming property | Ordinary | TT | CA interaction |
| --- | --- | --- | --- |
| Axis-specific reach | ±3/7/11/15; relevant field level 1/5/10/16 and Bio 1/2/3/4 | These remain available | Available tech controls free adjustment too |
| General all-axis reach | No Total Terraform items | ±3/5/7/10/15/20/25/30 at Bio 0/3/6/9/13/17/22/25 | Shares the same maximum selection |
| Relevant field for ordinary axis | Gravity: Propulsion; temperature: Energy; radiation: Weapons | Bio requirements for TT instead | No extra independent CA reach factor |
| Paid cost per coordinate tick | 100 resources | 70 resources | Halves the selected cost: 50 ordinary / 35 with TT |

No mineral cost is charged. Sources: [terraformer catalog](../data/parts/terraformers.yaml),
[`FLookupPart`](../src/core/parts.cpp#L384),
[`GetProductionCosts`](../src/core/produce.cpp#L427).

Reach is clamped to coordinates 1..99. Immune axes are excluded from ordinary
terraforming. Helpful mode clips at the target race's preferred value; harmful
mode chooses a reachable direction away from it. Paid production selects the
axis with highest `div(100*abs(habitabilityDelta),coordinateDistance)+1`, keeping
the first axis on ties, then applies one tick, not the entire allowance.
Sources: [`FCanTerraformLppl`](../src/core/planet.cpp#L837),
[`IBestTerraform`](../src/core/planet.cpp#L240),
[`FBuildObject`](../src/core/turn2.cpp#L666).

### 4.3 CA automatic, permanent and remote changes

CA annually moves each helpful axis to its currently available helpful extreme
without paying production resources; this is separate from the paid-cost
halving above. For permanent improvement, the source first chooses one random
axis. If nonimmune and original environment differs from racial preference,
it succeeds with probability 1/10, additionally scaled by `min(P,1000)/1000`
for fewer than 100,000 colonists, then moves **original** environment one tick
toward preference. The selected axis may make an attempt ineligible; “10% each
axis every year” would be wrong. Sources:
[`AutoTerraform`](../src/core/turn2.cpp#L1108),
[`UninhabitPlanet`](../src/core/planet.cpp#L903) (current environment reverts to
original when a CA world becomes uninhabited; permanent changes remain).

CA's Orbital Adjuster supplies one attempted remote tick per module per year,
multiplied by ships. It can help self/friends, and harm a nonfriend world only
without a target starbase. Capability comes from the adjuster owner, while
planet preference comes from the target owner. Sources:
[`PctTerraFromLpfl`](../src/core/ship2.cpp#L505),
[`RemoteTerraforming`](../src/core/turn2.cpp#L1159),
[`IBestRemoteTerra`](../src/core/turn2.cpp#L2305).
PP's stochastic packet terraforming is described separately in the packet
section; it is not this annual CA policy.

## 5. Colony economy, research and costs

### 5.1 Resources, factories and mines

For resources, over-capacity population contributes at half rate: use
`E=C+div(P-C,2)` when `P>C`, capped at `2*C`; otherwise `E=P`.
Empty worlds produce zero resources. Ordinary output is:

```text
population resources = div(E, rsResGen)
factory resources    = div(operatingFactories * rsFactProd + 9, 10)
resources            = population resources + factory resources
```

If a nonempty planet's result is zero, force one. AR replaces both terms with:

```text
trunc(sqrt(double(E) * max(EnergyLevel,1) / rsResGen)
      * max(habitability,25) / 10 + 0.999)
```

The AR divisor is the raw 7..25 choice, not 700..2500. Sources:
[`CResourcesAtPlanet`](../src/core/planet.cpp#L628).

For surface installations, buildable capacity is
`max(10,div(C*selectedOperationRatio,100))`; currently operable count is
`max(1,min(buildableCapacity,div(P*ratio,100)))`, with actual operating count
also capped by constructed installations. Factories and mines have separate
ratios. HE/JOAT/OBRM therefore change buildable ceilings and crowding, not the
stored operation ratios. AR has no constructed/operable normal factories or
mines; its automatic operating mines are `trunc(sqrt(P))`. Sources:
[`CMaxMines` through `CMinesOperating`](../src/core/planet.cpp#L468),
[`CMaxFactories` through `CFactoriesOperating`](../src/core/planet.cpp#L535).

Planetary defenses have a separate count rule: buildable limit is
`clamp(4*habitability,10,100)`; current operation limit is the lesser of that
and `min(1000,div(P+24,25))`. AR returns zero for both. A next-year estimate
adds projected population change before applying the population operation
bound. WM changes eligible defense **types** and IS changes cost, not this
count formula. Sources: [`CMaxDefenses`](../src/core/planet.cpp#L595),
[`CMaxOperableDefenses`](../src/core/planet.cpp#L607).

Factory resource cost is the selected `rsFactBuild`; normal mineral cost is
`4-CheapFact` kT germanium, no ironium/boranium. Tutorial factories instead
cost `2-CheapFact` of **each** mineral. Mine resource cost is selected
`rsMineBuild`, with no mineral input. IS defenses multiply all four base costs
by `3/5` with integer truncation. AR cannot build the ordinary installations
or planetary defenses. Sources:
[`GetProductionCosts`](../src/core/produce.cpp#L358),
[`GetProductionCosts` defenses](../src/core/produce.cpp#L388),
[`production availability`](../src/core/produce.cpp#L103).

### 5.2 Local and remote mineral yield

At concentration `c`, with `m` operating mines, ordinary mining uses selected
output `e=rsMineProd`. Compute `A=m*c`, `Q=div(A*e,10)`, then output
`div(Q,100)` kT plus a `(Q%100)/100` chance of one extra kT during generation.
Remote mining uses `Q=A`; AR's automatic mining uses efficiency 10 rather
than its disabled mine-output setting. Local homeworld extraction uses at
least concentration 30; this floor also applies to remote mining of an AR
homeworld. Depletion uses `div(A,100)`, independently of `e`, before updating
concentration/depletion state. Thus output efficiency is not a multiplier of
the depletion input. Source: [`EstMineralsMined`](../src/core/mine.cpp#L82).

Remote-mining quantity comes from module ability × modules × ships, capped at
4,000 effective mines per fleet. AR may remote-mine its own inhabited worlds;
its local-mining accounting includes eligible own orbiting remote miners.
ARM is an equipment grant, not an independent yield multiplier; OBRM
restricts equipment and takes precedence over ARM. Sources:
[`CMineFromLpfl`](../src/core/ship2.cpp#L469),
[`AutoFleetOrder`](../src/core/ship2.cpp#L437),
[`EstMineralsMined` AR remote addition](../src/core/mine.cpp#L157).

### 5.3 Alchemy and recycling

Alchemy costs 100 resources per unit, or 25 with MA; one unit produces one kT
of **each** mineral without consuming mineral inputs. Sources:
[`GetProductionCosts`](../src/core/produce.cpp#L394),
[`CBuildProdItem`](../src/core/turn2.cpp#L317).

Ordinary scrapping recovers 80% minerals at a planet with a base, one third
without a base or in deep space; colonization uses 75%. Cargo minerals are
added intact. Gifted design mineral/resource values are divided by four first.
UR is checked on the **destination planet owner**, not necessarily the fleet
owner: scrapping there gives 90% minerals with a base or 45% without, plus
resource recovery. Aggregate resource recovery is capped at 65,535 per fleet
and enters the planet's **unsigned 16-bit** extra-resource counter; multiple
fleet additions can wrap that counter modulo 65,536. Its production benefit is
`div(baseResources*extra,baseResources+extra)`, not an unconditional flat amount
added to every future year's output. This is a generation-local pool, zeroed
when generation begins and freed when it finishes
([allocation/reset](../src/core/turn.cpp#L94),
[cleanup](../src/core/turn.cpp#L369),
[16-bit addition](../src/core/turn3.cpp#L639)). BET's newest-tech surcharge is suppressed
while scrapping values are recalculated. Sources:
[`SatisfyOrders`](../src/core/turn3.cpp#L575),
[`Produce`](../src/core/turn2.cpp#L44).

### 5.4 Research prices, allocation and SS research gain

For target level `L`, begin with
`B = rglTechCost[L] + 10*sum(all six current tech levels)`.
Per-field setting 0 gives `2*B-(B>>2)`, setting 1 leaves `B`, and setting 2
gives `div(B,2)`. Slow technology doubles this afterward. The exact 27-entry
level-cost table is in [`research.cpp`](../src/core/research.cpp#L7);
[`GetTechLevelCost`](../src/core/research.cpp#L10) owns evaluation. This is a
runtime resource price, distinct from the wizard's net-count research charge.

Normal annual research is credited to the selected field, with resource
production, research percentage, unused production, current/next-field choice,
and breakthroughs contributing context. GR changes distribution: selected
field gets `div(R+1,2)` and **each** other field gets `div(3*R+19,20)`.
Slow-tech halves the stored side-field allocations afterward. The counters
used by SS spying retain the full side-field amounts.
Source: [`UpdateResearchStatus`](../src/core/turn2.cpp#L2134).

With more than one living player and the annual pool enabled, SS receives per
field `div(div(totalTrackedFieldSpending,livingPlayers),2)` if that value is
above one. Slow tech then stores `div(gain+1,2)`. Research is processed again
without the annual pool to consume these awards. The tracked spending includes
SS's own allocations and GR side allocations; the formula is **not** literally
“half of everyone else's research.” Source:
[`UpdateResearchStatus`](../src/core/turn2.cpp#L2280).

### 5.5 Component miniaturization and construction cost order

Copy each part's three mineral costs and resource cost. For normally processed
parts, find minimum excess technology over nonzero required fields; for an
all-zero requirement use the minimum current technology field. Positive excess
is capped at 19 before applying:

| Cost property | Ordinary | Trait change |
| --- | --- | --- |
| Miniaturization per surplus level | 4 percentage points | BET: 5 |
| Maximum miniaturization | 75% | BET: 80% |
| Weapons cost after miniaturization | Unchanged | WM subtracts `cost>>2`; IS adds `cost>>2` |
| Stargate cost after miniaturization | Unchanged | IT subtracts `cost>>2` |
| Engine cost after miniaturization | Unchanged | CE subtracts `cost>>1` |
| Terraformer resource cost | Catalog value | CA divides by two; normal item/production handling is special |
| Newest tech-bearing component surcharge | None | BET doubles at no surplus, unless suppressed by caller |

Miniaturization subtracts `LMulDiv(cost,discount,100)` and retains minimum one
for initially positive costs. `LMulDiv` rounds nearest, halves away from zero;
subsequent PRT/CE shifts truncate differently. The newest-tech BET surcharge
comes **after** these part-specific adjustments. This pipeline must not be
collapsed into one binary64 cost multiplier. Sources:
[`GetTruePartCost`](../src/core/ship.cpp#L1127),
[`LMulDiv`](../src/platform/native.cpp#L99).

Special-category boundary: terraforming and planetary defenses/scanners jump
over the miniaturization setup. In the inspected source that jump also bypasses
assignment of local `cExcess`, although a later BET condition reads it. No
determinate BET rule is claimed for that bypass path here; it needs a separate
implementation/verification decision. This report does not change that code.
Sources: [bypass](../src/core/ship.cpp#L1139),
[later condition](../src/core/ship.cpp#L1223).

### 5.6 Assembled starbase costs

After hull/part and any upgrade-credit calculations, AR **or** ISB subtracts
`div(cost,5)` from each of the four cost channels once. If both apply, there is
still only one discount. All starbases then use `div(cost+1,2)` for general
starbase accounting; that latter step is not an additional ISB-only discount.
Part-level weapon/gate/engine/BET changes precede assembled-base treatment.
Source: [`GetProductionCosts`](../src/core/produce.cpp#L280),
[discount and final half](../src/core/produce.cpp#L342).

## 6. Ships, fleets, combat and information

### 6.1 Cloaking, scanning and intelligence

`PctCloakFromLpfl` [ship2.cpp:653](../src/core/ship2.cpp#L653) starts each occupied design with component cloak points, adds SS300, multiplies score by total empty mass of ships in that design and divides sum by total empty fleet mass. For non-SS, cargo mineral slots0..3 are added only to denominator; SS skips this. Fuel is excluded for everyone. Integer accumulation switches to double at large thresholds. With no equipment SS300→75% cloak independent of cargo. There is no general SS cargo-capacity multiplier in `WtMaxShdefStat` [ship.cpp:137](../src/core/ship.cpp#L137); special cargo hulls/modules supply capacity.

`PctCloakFromHuldef` [planet.cpp:922](../src/core/planet.cpp#L922) evaluates one design directly: ISB base starts40points, SS adds300, then components. Therefore an unmodified ISB base is20%; SS+ISB340points gives76%, not95% or stacked percentages. Both paths use the same curve: for original score p, p≤100→floor(p/2); p≤300→50+floor((p−100)/8); p≤612→75+floor((p−300)/24); p≤1124→88+floor((p−612)/64); p<1380→96; p<1612→97; otherwise98. Design path rejects negative/>25000 scores; fleet path rejects negative. Keep exact inclusive boundaries from source.

`CPtsCloakFromLphs` [ship2.cpp:753](../src/core/ship2.cpp#L753) multiplies a component's points by slot count. SS-specific options: Transport Cloaking300 and Ultra-Stealth540 ([electrical.yaml:7](../data/parts/electrical.yaml#L7)); Shadow Shield70, Depleted Neutronium50, Chameleon40 are explicit code cases. Transport Cloaking insertion is additionally restricted in UI to hulls whose `imdAttack==hullAttackNone`, [buildui.cpp:911](../src/ui/buildui.cpp#L911). The core score function itself does not recheck that hull permission.

`GetShdefScannerRange` [util.cpp:1755](../src/core/util.cpp#L1755): scanners add range⁴×quantity to separate normal and penetrating totals, then floor(fourth-root). Ordinary penetrating parts map ability1/2/other-positive→50/100/200ly. Chameleon supplies45ly penetrating and normal160; Pick Pocket normal80, penetrating0, theft bit1; Robber Baron normal220, penetrating120, theft bits3. [scanners.yaml:50](../data/parts/scanners.yaml#L50), [scanners.yaml:136](../data/parts/scanners.yaml#L136). Chameleon's raw YAML `scannerSteals` does **not** make it a thief: the source special case never sets `iSteal` for it. A design without any normal scan contribution or installed scanner returns normal −1; an installed zero-range Bat returns 0.

JOAT built-in scanners on Scout, Destroyer, Frigate supply normal20×Electronics and penetrating10×Electronics before fourth-power stacking. Tutorial substitutes40/20ly. NAS doubles normal range after root/rounding, preserves penetrating total from special parts/built-ins, and denies Ferret/Dolphin/Elephant plus negative-ability planetary scanners in `FLookupPart` [parts.cpp:295](../src/core/parts.cpp#L295). Fleet scan result is maximum normal and maximum penetrating across designs, OR of theft permissions, minimum `pctDetect`: [util.cpp:1711](../src/core/util.cpp#L1711). A Tachyon Detector count≥18 clamps17 then uses `vrgbTachyon` rather than an arithmetic multiplier. Exact table for counts0..17 is `[100,95,93,91,90,89,88,87,86,86,85,84,84,83,83,82,82,81]`, [util.cpp:23](../src/core/util.cpp#L23). Fleet visibility computes effective target cloak=floor(cloak×tableValue/100), then applies squared `(100−effectiveCloak)%` range factor [SetVisPFFleets, save.cpp:1351](../src/core/save.cpp#L1351).

Planet scanner ordinary range=absolute best available planetary scanner ability; negative ability supplies penetrating half, NAS doubles normal. AR has no scanner installation: normal=floor(sqrt(10×planet colonist cargo units)), NAS×1412/1000 (integer), or without NAS penetrating normal/2 when an Ultra Station-or-later hull is present. [GetPlanetScannerRange, util.cpp:1641](../src/core/util.cpp#L1641). Population units in planet `rgwtMin[3]` are hundreds of colonists, so AR ordinary range=floor(sqrt(population/10)), subject to exact code conversion.

Theft is cargo transfer permission, not a percentage boost: enemy fleet allows `iSteal!=0`; enemy planet requires bit2 (`iSteal==1` explicitly denied). Attempts to steal cargo slots3/4 (colonists/fuel) fail. [SatisfyOrders, turn3.cpp:102](../src/core/turn3.cpp#L102), [turn3.cpp:336](../src/core/turn3.cpp#L336). Other normal transfer location/capacity/order conditions still apply.

Theft scanner also exposes enemy fleet cargo detail at exactly co-located coordinates when theft bit1 is set [save.cpp:1341](../src/core/save.cpp#L1341), and Robber Baron marks the orbiting planet DetMore [save.cpp:1411](../src/core/save.cpp#L1411).

WM visibility finish uses DetAll7 instead of ordinary DetSome3 for included enemy ship/base designs; explicit `grbitPlr` learned bit also grants7 to anyone. [SetVisPFFinish, save.cpp:1841](../src/core/save.cpp#L1841). SD field hits set the learned bit for affected designs (or all present designs if no affected-design list), [turn.cpp:2012](../src/core/turn.cpp#L2012).

SD own minefield visibility checks only deep-space fleets inside radius²=cMines; uncapped cloak test is `Random(100)>=PctCloakFromLpfl`, so an uncloaked fleet is guaranteed and a75%-cloaked fleet has25% detection per check. This is a probability, unlike scanner range shrinkage. [SetVisPFThings, save.cpp:1808](../src/core/save.cpp#L1808).

PP globally includes **all** mineral packets (including salvage represented as warp0 packets) and their owners, [SetVisPFInit, save.cpp:1260](../src/core/save.cpp#L1260). Own moving packets additionally scan at radius=(storedWarp+4)²=actualWarp² ly, with separate fleet cloak range test, penetrating planet information subject to base visibility, and thing detection. No `idPlanetDeepSpace` restriction in that fleet loop, so this packet scan can see orbiting fleets. [SetVisPFThings, save.cpp:1702](../src/core/save.cpp#L1702).

IT own bases with gate equipment detect foreign planets that have a gate within **source** gate range. Unlimited range is represented as10000 for this scan, and goes directly to mark before finite-range cloak checking. Finite-range detection respects target base `lVisible` squared factor. It marks planet DetSome, not a generic fleet sensor. [SetVisPFPlanets, save.cpp:1478](../src/core/save.cpp#L1478), [save.cpp:1598](../src/core/save.cpp#L1598), [StargateRangeFromLppl, planet.cpp:706](../src/core/planet.cpp#L706).

CA habitat intelligence is a separate race property. During turn-file write, each included other living player record for a CA viewer is copied and cleared, then habitat center/min/max arrays are restored and full-detail encoding selected. The source comment explicitly prevents leaking tech/research/traits/production/relations. [FWriteDataFile, save.cpp:333](../src/core/save.cpp#L333). In `DrawMineSurvey`, CA substitutes self/friendly planet owner habitat to calculate displayed planet values [mineui.cpp:512](../src/ui/mineui.cpp#L512); map value view uses optimal value for CA [scan.cpp:1044](../src/ui/scan.cpp#L1044). This is not evidence that all current/original planet environments are CA-exclusive: normal planet record serialization beyond minimal detail writes current environment and original values when different [WritePlanet, save.cpp:676](../src/core/save.cpp#L676).

### 6.2 Combat and repair

`SpdOfShip` [battle.cpp:508](../src/core/battle.cpp#L508) chooses engine effective warp: listed warp10 engines→10, otherwise largest warp≤9 whose table fuel entry≤120. Internal speed=warp−4+thrusters; Enigma engine and Alien Miner each add a half-thruster (rounded up together), Maneuvering Jet and Multi Function Pod+1, Overthruster+2. A real fleet adds WM2. Loaded mass allocates cargo proportionally by design capacity, cargo dumping−1, then subtracts `floor(wt/70)/engineCount` with integer operations and clamps0..8. Dxy schedule maps four internal units to one movement/round on average; WM2 represents0.5 movement/round before cap. `DxyFromSpdRound` [battle.cpp:2249](../src/core/battle.cpp#L2249) uses periodic0..3 rounding, not float speed. Any Energy Dampener in battle subtracts4 from **every** non-base token, floor0, after speed evaluation; [battle.cpp:1039](../src/core/battle.cpp#L1039). This is equipment behavior enabled by SD part availability, not a passive SD aura.

`DropColonists` ground resolution [turn2.cpp:813](../src/core/turn2.cpp#L813): attacker base110, WM165 (1.5×ordinary), AR0, using `trunc(div(colonistCount*attackMultiplier,100)*adjustedDefenseSurvival)`, where `adjustedDefenseSurvival = CalcPctSurvive + (1−CalcPctSurvive)/4`. Multiplication precedes integer division, then the binary64 survival adjustment. Defender resident colonists×100/100, IS×200/100. Orbiting starbase kills arriving attackers first; AR may only colonize unowned worlds with colonization permission, not invade. These are ground troop modifiers, not weapon damage modifiers.

`HealShips` [turn2.cpp:1016](../src/core/turn2.cpp#L1016) requires !dead&&!fNoHeal. Its rates are stored damage **fifths of a percent** (`pctDp` maps armor damage fraction through denominator500; see [FStargateJump, ship2.cpp:113](../src/core/ship2.cpp#L113)). Raw rate5 when moved (1% armor),10 stationary deep space(2%),15 stationary at other/unowned planet(3%),25 own planet(5%),40 own repair-eligible base with hull cargo0(8%),100 own repair-eligible base with hull cargo!=0(20%). IS doubles this location rate before transport addition. Fuel Transport adds25raw=5% armor, Super Fuel Xport50raw=10%. Exact code chooses transport bonus while iterating ascending design IDs: Super sets50; Fuel Transport sets25 only if current bonus<5 (so it cannot replace an existing25/50). Each damaged design subtracts raw rate from `pctDp`, floor0. Base self-repair raw50=10% armor, IS75=15%, requires !fNoHeal. Do not misreport raw5/100 as5%/100% repair.

`DpShieldOfShdef` [util.cpp:297](../src/core/util.cpp#L297): summed shields include installed shields plus Fielded Kelarium50/item and Mega Poly Shell100/item; RS adds floor(2×sum/5), caps65535. `UpdateShdefCost` [util.cpp:1572](../src/core/util.cpp#L1572) halves only armor-part DP contribution (`dpT>>1`) when full-detail design belongs to current RS player; intrinsic hull armor, Croby/Langston65/item and Multi Cargo Pod50/item unaffected. [RegenShield, battle.cpp:673](../src/core/battle.cpp#L673) restores floor(original shield/10), capped original, only if current shield!=0. Called at start of rounds1..15 for active tokens with shield>0 and RS [battle.cpp:2366](../src/core/battle.cpp#L2366); zero shields never regenerate.

### 6.3 Movement, fuel and gates

IFE reduces each design engine's selected-warp fuel table entry `e` to `e−floor(15e/100)` before cargo allocation; [EstFuelUse, ship.cpp:402](../src/core/ship.cpp#L402). CE checks the fleet once on movement pass0 at requested warp>6 excluding11; `Random(10)==0` skips movement for balky engines. Eligibility also requires ordinary movement order, not transfer/lay-mines. [MoveFleets, turn.cpp:1371](../src/core/turn.cpp#L1371). This is one10% fleet event, not per-engine probability.

`WtMaxShdefStat` [ship.cpp:137](../src/core/ship.cpp#L137) has no race switch: hull cargo plus Cargo Pod50/Super100/Multi250; hull fuel plus Fuel Tank250/Super500, Antimatter Generator200 each. `FuelFleets` [turn.cpp:1254](../src/core/turn.cpp#L1254) fills tank at own/friendly cargo-capable starbase; otherwise adds200 fuel/year per Fuel Transport or Super Fuel Xport and50 per Antimatter Generator, capped fleet capacity. Transport hull restrictions and IT generator restriction therefore enable abilities rather than changing a race fuel-generation multiplier.

Gate transport baseline uses source/destination gates owned by self or owner regards traveler as friend. Ordinary non-jumpgate/non-IT cargo is unloaded on source; colonists aboard at a foreign source prevent use. IT keeps mineral/colonist cargo; jumpgate source exception also skips this unloading rule. [MoveFleets, turn.cpp:1387](../src/core/turn.cpp#L1387), [turn.cpp:1437](../src/core/turn.cpp#L1437). Both preview and jump calculation use design **empty** mass, excluding retained IT cargo [ship2.cpp:48](../src/core/ship2.cpp#L48).

`MdCalcStargateDamage` [ship2.cpp:169](../src/core/ship2.cpp#L169): source gate rangeR (unlimited−1→8000); distance>5R returns impossible, ship empty mass>5×either finite mass rating returns impossible. Start survival10000. For each exceeded boundary (source distance, source mass, destination mass), factor=`(5×rating−value)×2500/rating`; multiply survival by each mass factor/10000 after range factor. Nonpositive factor→100% damage/total death. Output damage=floor((10000−survival)/100). Destination gate range does **not** enter distance calculation. IT does not override any of these limits/damage.

`FStargateJump` [ship2.cpp:73](../src/core/ship2.cpp#L73) sets ordinary per-ship vanishing chance=floor(damage/3)%; IT sets chance0. Survivors still acquire armor damage, and previously damaged ships can die from combined damage. 100%-damage design branch kills everything even for IT. Thus “safe overgating” must separate random disappearance, armor damage and hard limits.

### 6.4 Minefields

[turn.cpp:17](../src/core/turn.cpp#L17) fixes standard/heavy/speedbump baseline safe warps4/6/5; per-extra-warp per-unit hit tests3/10/35 per1000. `FTravelThroughMineFields` [turn.cpp:1799](../src/core/turn.cpp#L1799) sets expertise SS1/SD2/other0, adds to safe threshold and subtracts from extra-warp chance. It derives effective warp from actual distance (start3, increase while warp²<dTravel−1), so this is not simply requested-order warp. Own fields and friendly field-owner relation excluded in travel path.

Standard per-ship damage100, if any fleet ship has a scoop engine (fuel table at warp4==0)125; minimum fleet 500/600 for the homogeneous case. Heavy damage is 500/600, minimum 2000/2500. For mixed fleets the exact per-design expression is `(shipsInDesign*dmgPer+dmgExtra)*engineCount`; the fleet-minimum extra is assigned only to the first affected design and then cleared. Speedbump damage0. Shield absorbs min(shields,half incoming); rest armor. No direct PRT damage reduction; equipment/RS affect shield capacity. [turn.cpp:1909](../src/core/turn.cpp#L1909).

`CLayMinesFromLpfl` [ship2.cpp:535](../src/core/ship2.cpp#L535) sums dispenser ability×quantity (and Multi Contained Munition4/item for standard), Mini/Super Mine Layer hull×2, ship count, then×10; overflow guard returns100000000. This is hull-specific doubling, usable by a transferred hull as well. `SatisfyOrders` [turn3.cpp:849](../src/core/turn3.cpp#L849) permits moving SD fleets to lay at half cMine; ordinary fleets must remain all turn. SD can use a next-waypoint mine order even while the current task is None; moving laying occurs at the fleet's current coordinates and does not require arrival.

Detonation UI visible only for own SD **standard** field [mineui.cpp:1203](../src/ui/mineui.cpp#L1203). Core `ThingDecay` simply checks stored fDetonate and applies contact to fleets in field radius; it does not revalidate SD/type [turn2.cpp:1877](../src/core/turn2.cpp#L1877). Own Mini/Super Mine Layer designs are immune to own field's detonation; other owner designs remain vulnerable, including other SD hulls [turn.cpp:1923](../src/core/turn.cpp#L1923). Each fleet hit by at most one effective detonation in this decay pass (`fBombed`). Fully immune field returns safe so another detonation can be considered.

Decay raw percent=min(50,2+planetCount×(SD?1:4)); detonating adds25 after cap. Lost mines=max(floor(cMines×percent/100),percent), additionally minimum10 for non-speedbump, field deleted if exhausted. [turn2.cpp:1896](../src/core/turn2.cpp#L1896). SD decay advantage is on **field owner**, traversal expertise on **fleet owner**.

### 6.5 Mineral packets

`GetProductionCosts` [produce.cpp:409](../src/core/produce.cpp#L409): ordinary single packet consumes110 of chosen mineral+10resources; IT120+10; PP70+5. Mixed ordinary44 each+10resources; IT48 each+10; PP25 each+5. `FBuildObject` [turn2.cpp:588](../src/core/turn2.cpp#L588) outputs ordinary/IT100kt single or40kt each mixed, PP70kt single or25kt each mixed. PP production has zero mineral overhead, ordinary10%, IT20%.

Driver effective warp is highest driver `iItem−2`; another equal best installed slot sets fTwo and gives+1 safe warp [IWarpMAFromLppl, planet.cpp:675](../src/core/planet.cpp#L675). Requested launch capped through validity check5..driver+3; default driver+fTwo. Decay tier=requested−driver−fTwo if positive, elseNone; IT increases tier1 capped50% [turn2.cpp:603](../src/core/turn2.cpp#L603). `FPacketDecay` [turn2.cpp:1775](../src/core/turn2.cpp#L1775) tiers10/25/50, PP integer halve→5/12/25, minimum loss per nonempty mineral5kt vs10. Loss=`max(minimum,floor(mass×rate×pctRate/10000))`, capped available mass. No-decay early return skips minimum. Stationary salvage decay is separate10%/min10 for all races, so PP doesn't halve it [turn2.cpp:1845](../src/core/turn2.cpp#L1845).

On impact driver warp gains+1 for twin drivers, IT receiver halves **squared** capacity; captured fraction in thousandths=min(1000, floor(1000×effectiveDriverWarp²/packetWarp²)), zero if no driver. Uncaught mass by mineral=floor(original×(1000−caught)/1000). Surface minerals kept fraction=caught+floor((1000−caught)/9), applied to original mineral mass [MoveThings, turn.cpp:1064](../src/core/turn.cpp#L1064). PP sender learns receiving base design when catcher driver present [turn.cpp:1071](../src/core/turn.cpp#L1071).

PP terraforming runs only if not fully caught. Per mineral i (matching environment index i), process uncaught mass in100kt chunks (final smaller allowed), `Random(200)<chunkMass` adds a temporary point, nested `Random(10)==0` also adds permanent point [turn.cpp:1104](../src/core/turn.cpp#L1104). Thus full100kt chunk50% temporary success and5% unconditional permanent success, correlated. Permanent points adjust original environment toward sender center and never beyond; immune axis moves away from50 toward1 or99 based on original's side. Temporary points require sender terraforming capability (`pfnCanTerraform`), capped by sender available min/max; immune axis halves generated temporary count and uses1..99 boundaries [turn.cpp:1117](../src/core/turn.cpp#L1117), [turn.cpp:1145](../src/core/turn.cpp#L1145). This is not a fixed terraforming-per-100kt conversion and ignores caught mass. Packet damage still happens independently afterward; AR planet skips colony damage branch [turn.cpp:1184](../src/core/turn.cpp#L1184).

## 7. Equipment grants and restrictions

The authoritative implementation is `FLookupPart`. Eligibility is an AND of trait rules, any Mystery Trader acquisition rule, and all six tech prerequisites; satisfying a trait does not grant an item before tech. `idPlayer == iplrNone` is a catalog/query mode with several bypasses and must not be used to infer player eligibility. `TechStatus` requires every tech field to reach the catalog requirement, and returns separate future/next-tech statuses otherwise. [lookup](../src/core/parts.cpp#L32), [final status](../src/core/parts.cpp#L394), [TechStatus](../src/core/parts.cpp#L406).

### 7.1 PRT equipment gates

| PRT | Exclusive eligibility (full list in lookup) | Explicit denial |
|---|---|---|
|HE|Settler's Delight engine; Mini-Colony Ship and Meta Morph hulls; Flux Capacitor|all seven stargates|
|SS|Rogue and Stealth Bomber hulls; Shadow Shield; Depleted Neutronium armor; Transport Cloaking and Ultra-Stealth Cloak; Chameleon Scanner, Pick Pocket Scanner, Robber Baron Scanner|none further in lookup|
|WM|Battle Cruiser and Dreadnought hulls; Gatling Neutrino Cannon and Blunderbuss|Mine Dispenser 50; Laser Battery, Planetary Shield, Neutron Shield|
|CA|Orbital Adjuster; Retro Bomb|none further in lookup|
|IS|Fuel Transport and Super Freighter hulls; Croby Sharmor; Fielded Kelarium; Mini Gun; Jammer 10, Jammer 50, Tachyon Detector; shared Speed Trap 20 with SD|Smart Bomb, Neutron Bomb, Enriched Neutron Bomb, Peerless Bomb, Annihilator Bomb|
|SD|Mini Mine Layer and Super Mine Layer hulls; Energy Dampener; Mine Dispenser 40/80/130, Heavy Dispenser 50/110/200, Speed Trap 30/50; shared Speed Trap 20 with IS|none further in lookup|
|PP|Mass Driver 5/6, Super Driver 8/9, Ultra Driver 11/12/13|none further in lookup; Mass Driver 7 and Ultra Driver 10 are public|
|IT|Stargate any/300, 100/any, any/800, any/any; Anti-matter Generator|none further in lookup; finite Stargate 100/250, 150/600, 300/500 are public except HE|
|AR|Death Star hull; Orbital Construction Module|Colonization Module; all nine planetary scanners Viewer 50/90, Scoper 150/220/280, Snooper 320X/400X/500X/620X; all five planetary defenses SDI, Missile Battery, Laser Battery, Planetary Shield, Neutron Shield|
|JOAT|none in part lookup (built-in scanners are runtime hull abilities)|none further in lookup|

Evidence: [engines/shields](../src/core/parts.cpp#L41), [ship hulls](../src/core/parts.cpp#L78), [starbases/armor/electrical](../src/core/parts.cpp#L128), [mechanical/starbase specials](../src/core/parts.cpp#L201), [mine dispensers/mining robots](../src/core/parts.cpp#L237), [scanners/beams/bombs](../src/core/parts.cpp#L295), [planetary](../src/core/parts.cpp#L361). Numeric requirements and exact display spellings appear in the catalog appendix below.

Important precision: WM can still build the two basic defenses SDI/Missile Battery, while all eight SD-exclusive mine layers are already denied to WM by exclusivity. IS's Super Fuel Xport is NOT exclusive: only Fuel Transport and Super Freighter are checked. Mini Morph is a Mystery Trader hull, separate from HE's Meta Morph. These distinctions follow the exact conditions, not broad descriptive prose.

### 7.2 LRT equipment gates and interactions

- IFE grants Fuel Mizer (P2) and Galaxy Scoop (E5/P20). Galaxy Scoop additionally requires absence of NRSE. Fuel Mizer survives NRSE because the deny list omits it. NRSE denies Radiating Hydro-Ram Scoop and every scoop from Sub-Galactic Fuel Scoop through Galaxy Scoop; NRSE grants Interspace-10 (P11). It does not deny Settler's Delight or Mystery Trader Enigma Pulsar. [engine gates](../src/core/parts.cpp#L48).
- ARM grants Midget Miner, Miner, Ultra Miner hulls and Robo-Midget Miner/Robo-Ultra-Miner robots. OBRM denies Midget Miner, Miner, Maxi-Miner, Ultra-Miner hulls and Robo-Midget Miner, Robo-Miner, Robo-Maxi-Miner, Robo-Super-Miner, Robo-Ultra-Miner. Thus OBRM overrides ARM in availability and starting fleet. Mini Miner hull + Robo-Mini-Miner remain; acquired Alien Miner and CA Orbital Adjuster are not blocked. [hull conditions](../src/core/parts.cpp#L91), [robot conditions](../src/core/parts.cpp#L273), [starting miners](../src/core/create.cpp#L757).
- ISB grants Space Dock and Ultra Station hulls. It does not gate Orbital Fort, Space Station, or AR's separate Death Star. [starbase gates](../src/core/parts.cpp#L128).
- NAS denies Ferret, Dolphin, Elephant ship scanners and the four negative-ability planetary Snoopers (320X/400X/500X/620X). It does not make SS Chameleon/Pick Pocket/Robber Baron catalog items ineligible. Built-in JOAT/AR scanners do not pass through this item gate; actual penetrating/conventional scanner functionality under NAS is a separate runtime question covered in section 6.1. [ship scanners](../src/core/parts.cpp#L302), [planetary scanners](../src/core/parts.cpp#L368).
- TT grants all eight Total Terraform ±3/5/7/10/15/20/25/30 items (Bio requirements 0/3/6/9/13/17/22/25; base resource cost 70). Ordinary per-variable terraformers remain eligible (base resource cost 100), with steps ±3/7/11/15 requiring respective field 1/5/10/16 and Bio 1/2/3/4; field is P for gravity, E for temperature, W for radiation. [TT gate](../src/core/parts.cpp#L384), [catalog](../data/parts/terraformers.yaml#L6).
- CE, UR, BET, RS, GR, MA, and LSP do not add an eligibility gate in `FLookupPart`. Their effects elsewhere are not additional equipment grants. CE and IFE each add one starting Propulsion level (IFE excluded in tutorial), and LSP reduces starting population, detailed below.
- Mountability adds UI constraints: Settler's Delight only Mini-Colony Ship; Orbital Construction Module only Colony Ship; Transport Cloaking only a hull whose attack category is `hullAttackNone`. These are separate from race eligibility. [design UI checks](../src/ui/buildui.cpp#L895).

### 7.3 Nontrait acquisition requirements

`FShouldPartBeHidden` additionally hides Multi Contained Munition, Anti-Matter Torpedo, Mega Poly Shell, Langston Shell, Hush-a-Boom, Alien Miner, Enigma Pulsar, Mini Morph, Multi Function Pod, Multi Cargo Pod, Jump Gate, Genesis Device until the corresponding `grbitTrader` acquisition bit is present. All new players begin with no Trader acquisitions. [hidden parts](../src/core/research.cpp#L100), [creation reset](../src/core/create.cpp#L391). No LRT substitutes for this flag.

## 8. Starting state

### 8.1 Base technology and ordered modifiers

`GenerateWorld` first zeroes all tech and accumulated research, then assigns this PRT base vector:

|PRT|E/W/P/C/L/B|
|---|---|
|HE|0/0/0/0/0/0|
|SS|0/0/0/0/5/0|
|WM|1/6/1/0/0/0|
|CA|1/1/1/2/0/6|
|IS|0/0/0/0/0/0|
|SD|0/0/2/0/0/2|
|PP|4/0/0/0/0/0|
|IT|0/0/5/5/0/0|
|AR|1/0/0/0/0/0|
|JOAT|3/3/3/3/3/3|

Then Tech3 raises **only fields with `rsTechBonusN == 0`** to minimum 3, or 4 for JOAT; it never reduces a higher PRT tech. Then CE increments P; then IFE increments P unless `game.fTutorial`. Therefore CE+IFE stack (+2 P outside tutorial), and P can exceed the Tech3 floor. [all assignments/modifiers](../src/core/create.cpp#L392).

Humanoid/new-race defaults are JOAT; environment center 50, range 15–85 each; growth 15%; resource divisor 10; factories 10 production/10 cost/10 operation; mines 10 production/5 cost/10 operation; surface-mineral leftovers; all research preferences standard; no LRTs. [default record](../data/races/predefined.yaml#L8).

### 8.2 Homeworld and population

All players initially get a homeworld, base slot 0, no artifact, 10 factories, 10 mines, 10 defenses, Viewer 50 scanner index 0. Initial population is internal 250 = 25,000 colonists; LSP uses 175 = 17,500. Each homeworld environment equals its racial range midpoint, or a uniform random 1–99 on an immune axis. Human research allocation becomes 15%; current field Energy, next field 6; score and last-year resources zero; initial relations neutral. AR clears all three installations and removes its planetary scanner (`iScanner=31`) after fleet setup. [homeworld setup](../src/core/create.cpp#L449), [AR removal](../src/core/create.cpp#L546), [environment/research](../src/core/create.cpp#L553), [AR scanner](../src/core/create.cpp#L834). The internal population unit is independently confirmed by [report colonist conversion ×100](../src/core/report.cpp#L409).

AI gets leftover-point allowance 50; Expert and higher AI gets +floor(population/10) before accelerated-start adjustment (250→275; LSP 175→192). Accelerated BBS start then multiplies population by `(2*PctTrueMaxGrowth + 10)/10`, with integer truncation. `PctTrueMaxGrowth` is selected growth doubled for HE, selected growth otherwise ([function](../src/core/race.cpp#L925)). This precedes the PP/IT world split. [AI/BBS modifiers](../src/core/create.cpp#L475).

During each player's homeworld setup, minerals and concentrations are copied from the **live** planet-array element 0, before that player's leftover allocation. The initially generated pile there uses: each mineral `Random(concentration*10)+10`, adding `Random(150)+155` if below 200; BBS adds one quarter. The homeworld concentration copy floors to 30 (25 when `gd.fTutorial`; note the code uses that flag, distinct spelling from `game.fTutorial`). [mineral seed](../src/core/create.cpp#L268), [copy/floors](../src/core/create.cpp#L465). If element 0 itself becomes a homeworld and an allocation changes it, later homeworld copies can inherit that modified source; identical immutable starting piles are not guaranteed.

Let `p=min(50,CAdvantagePoints)` (AI forces p=50). Leftover options are applied before AR clears its installations:

|Choice|Exact allocation|
|---|---|
|0/default surface|Total 10p kt: let t=10p; add floor(t/4)+(t mod 4) to the least mineral (ties follow nested comparisons); then add floor(t/4) to all three. Tough+ AI also gets concentration option afterwards.|
|1 concentrations|k=1 if 0<p<3, otherwise floor(p/2); add k to the least concentration (first minimal wins), then add floor((k+1)/2) to all three.|
|2 mines|add p>>1 mines|
|3 factories|add floor(p/5) factories|
|4 defenses|add floor((p+5)/10) defenses|

[allowance/AI](../src/core/create.cpp#L475), [allocations](../src/core/create.cpp#L492). This uses `CAdvantagePoints`' returned integer, which is its internal points divided by 3 with integer truncation. [return scaling](../src/core/race.cpp#L312).

### 8.3 Second world: PP and IT only outside Tiny

Both obtain an additional world only when `mdSize > sizeTiny`. Candidate unowned worlds are reservoir sampled in the radial band 15–23% of galaxy width from HW; fallback is nearest unowned world. Environment is rerolled until desirability ≥10, up to 100 attempts, then copies HW environment if the attempt counter reaches 100. The second world receives base slot 1, no artifact, 4 factories, 10 mines, no explicitly added defenses, Viewer 50; minerals independently random 100–299 kt each. Population becomes floor(2*P/5) there, while HW becomes floor(4*P/5). Thus the allocation totals 120% of pre-split P (minus integer rounding), not a conservation split. Ordinary P=250 produces HW20,000 + second10,000 colonists; LSP P=175 produces HW14,000 + second7,000. A duplicate of the **first created ship design** (index 0) starts at the second world. [selection and allocation](../src/core/create.cpp#L685). Galaxy width is `400*size+400` ly. [width](../src/core/create.cpp#L100).

### 8.4 Fleet inventory before component upgrading

Each row includes all PRT grants, before ARM additions. Each item is a separate 1-ship fleet. The table uses exact template class names; `Smaugarian Peeping Tom` is the scout template even for races with other names.

|PRT|Homeworld fleets|Additional non-Tiny world fleet|
|---|---|---|
|HE|Smaugarian Peeping Tom; 3×Spore Cloud|none|
|SS|Smaugarian Peeping Tom if E<2, else Shadow Sleuth; Shadow Transport **only if human**; Santa Maria|none|
|WM|Armed Probe; Santa Maria; additionally Stalwart Defender + Gadfly if C≥3|none|
|CA|Smaugarian Peeping Tom; Santa Maria; Change of Heart|none|
|IS|Smaugarian Peeping Tom; Santa Maria|none|
|SD|Smaugarian Peeping Tom; Santa Maria; Little Hen; Speed Turtle|none|
|PP|Long Range Scout; Santa Maria|Long Range Scout|
|IT|Smaugarian Peeping Tom; Mayflower; Stalwart Defender; Swashbuckler|Smaugarian Peeping Tom|
|AR|Smaugarian Peeping Tom; Pinta|none|
|JOAT|Armed Probe; Long Range Scout; Santa Maria; Teamster if C<4 else Swashbuckler; Stalwart Defender; Cotton Picker|none|

ARM without OBRM adds two Potato Bug fleets to every PRT; OBRM suppresses both. JOAT's Cotton Picker remains even with OBRM (basic miner). [PRT fleet logic](../src/core/create.cpp#L635), [extra fleets](../src/core/create.cpp#L677), [ARM fleet logic](../src/core/create.cpp#L757).

`CreateStartupShip` either copies the requested template to the next design slot or reuses an existing player design; increments design built/existing counts; creates one new fleet; sets one ship; fills fuel via fleet stat; assigns battle plan 0. This code does not load colonists or minerals into the new fleets; `LpflNew` zero-initializes the entire fleet first. [fleet initialization](../src/core/util.cpp#L1045). [CreateStartupShip](../src/core/create.cpp#L970). Template class/hull/equipment details are listed in appendix B below.

### 8.5 Starting starbases

Everyone starts with template Starbase (Space Station hull), except AR's live home base is a copy moved to slot 1, while slot 0 becomes the unbuilt Starter Colony (Orbital Fort) design. PP home Starbase gets one Mass Driver 5; non-Tiny PP second world gets Accelerator Platform (Orbital Fort, Mass Driver 5). IT home gets Stargate 100/250 except in tutorial; non-Tiny IT second world gets Porthole to Beyond (Orbital Fort, Stargate 100/250). These base templates are copied before ship setup and are **not** passed through the ship component upgrade loop. [initial bases](../src/core/create.cpp#L581), [template data](../data/parts/starting-starbases.yaml#L4).

### 8.6 Automatic starting ship component upgrades

Every starting ship design is traversed, then the first available candidate in each list replaces its eligible placeholder. Full lookup (PRT/LRT/Trader/tech) decides availability. If none qualifies, template component remains; this is not general optimization or revalidation of all template parts. [upgrade loop](../src/core/create.cpp#L761).

|Template placeholder|Candidate priority|
|---|---|
|Quick Jump 5|Radiating Hydro-Ram Scoop conditionally first, then Alpha Drive 8, Daddy Long Legs 7, Fuel Mizer, Long Hump 6|
|Mole-skin or Cow-hide Shield|Wolverine Diffuse Shield, Cow-hide Shield|
|Tritanium or Crobmnium|Carbonic Armor, Crobmnium|
|Laser or X-Ray Laser|Yakimora Light Phaser, X-Ray Laser|
|Alpha Torpedo|Beta Torpedo|
|Lady Finger Bomb|Black Cat Bomb|
|Robo-Midget or Robo-Mini-Miner|Robo-Miner, Robo-Midget Miner|
|Bat or Rhino Scanner|Possum Scanner, Mole Scanner, Rhino Scanner|

Radiating Hydro-Ram Scoop is considered if radiation is immune OR hull is not Colony Ship OR racial radiation center ≥85. This spares low-radiation conventional colony ships from that engine. Settler's Delight is not a Quick Jump placeholder and stays in HE colony designs. [engine candidates](../src/core/create.cpp#L771), [remaining candidates](../src/core/create.cpp#L782).

### 8.7 Tutorial and random/AI context

`CreateTutorWorld` selects Tiny/Sparse, BBS accelerated start, visible scores/no random events, predefined Humanoid and Easy TurinDrone, fixed seed1234567890. Tutorial suppresses IFE's +1 starting P and suppresses IT's home stargate installation; Tiny independently rules out second worlds. [tutorial setup](../src/core/create.cpp#L1368), [IFE exception](../src/core/create.cpp#L442), [IT exception](../src/core/create.cpp#L600).

`ibitRaceAIPlayer` triggers `CreateRandomRace` during generation. It is distinct from `fAi`, which controls AI bonuses and SS starting transport omission. Randomization selects PRT0–9, per-field cost preferences, 14 LRT flags, Tech3 and CheapFact switches; tries to reach advantage points 0–50, with a bounded-attempt fallback to Humanoid after `cPass++ > 250`, preserving only the incoming singular name. Fixed AI templates are in `data/races/ai.yaml`, not extra PRT types. [random invocation](../src/core/create.cpp#L386), [random race generation](../src/core/race.cpp#L726), [random trait switches](../src/core/race.cpp#L795).

## 9. Wizard points and trait prices

Advantage points are a calculated property of the whole candidate design.
`CAdvantagePoints` starts with **1650 internal points**, normalizes the candidate,
then applies habitat/growth, environmental-center, immunity, factory/growth,
population efficiency, factory/mine, PRT, LRT/count, and research terms.
The returned displayed balance is the low signed 16 bits of `div(total,3)`.
Do not divide and round each trait independently. Names, portrait, credentials,
and leftover-allocation choice have no direct point-price term. Sources:
[`CAdvantagePoints`](../src/core/race.cpp#L105),
[complete ordered formula](NEW-RACE-WIZARD.md#complete-advantage-point-calculation).

### 9.1 PRT raw price

Each entry below is **subtracted** from the internal total. Negative entries
grant points. `raNone` means JOAT; it is not the absence of a PRT.
Source: [`rgRacePrimaryTrait`](../src/core/race.cpp#L14),
[`RaceAttribute`](../src/core/enums.h#L206).

| ID | PRT | Source symbol | Raw cost to subtract | Additional point dependencies |
| --- | --- | --- | ---: | --- |
| 0 | Hyper-Expansion (HE) | `raCheapCol` | 40 | High factory-output/operation interaction uses multiplier 3 instead of 2; stored growth is still used |
| 1 | Super Stealth (SS) | `raStealth` | 95 | NAS additionally subtracts 200 |
| 2 | War Monger (WM) | `raAttack` | 45 | No additional PRT-specific point branch found |
| 3 | Claim Adjuster (CA) | `raTerra` | 10 | No additional PRT-specific point branch found |
| 4 | Inner-Strength (IS) | `raDefend` | -100 | No additional PRT-specific point branch found |
| 5 | Space Demolition (SD) | `raMines` | -150 | No additional PRT-specific point branch found |
| 6 | Packet Physics (PP) | `raMassAccel` | 120 | NAS additionally subtracts 280 |
| 7 | Interstellar Traveler (IT) | `raStargate` | 180 | No additional PRT-specific point branch found |
| 8 | Alternate Reality (AR) | `raMacintosh` | 90 | Skips ordinary factory/mine price terms, adds 210; cheap Energy additionally subtracts 100 |
| 9 | Jack of All Trades (JOAT) | `raNone` | -66 | NAS additionally subtracts 40 |

The HE factory interaction runs before the AR price branch: outside normal UI
normalization, malformed/nondefault AR factory settings can still affect that
earlier interaction. Source: [factory/growth branch](../src/core/race.cpp#L167),
[AR economy branch](../src/core/race.cpp#L191),
[NAS/research branches](../src/core/race.cpp#L275).

### 9.2 LRT raw adjustments and combination pricing

These entries are **added** to the internal total, so negative values are costs.
The sign convention differs from the PRT cost column above.
Source: [`rgRaceAdvDisPts`](../src/core/race.cpp#L16),
[LRT indices/masks](../src/core/enums.h#L145).

| Bit | LRT | Abbreviation | Raw adjustment to add |
| --- | --- | --- | ---: |
| 0 | Improved Fuel Efficiency | IFE | -235 |
| 1 | Total Terraforming | TT | -25 |
| 2 | Advanced Remote Mining | ARM | -159 |
| 3 | Improved Starbases | ISB | -201 |
| 4 | Generalized Research | GR | 40 |
| 5 | Ultimate Recycling | UR | -240 |
| 6 | Mineral Alchemy | MA | -155 |
| 7 | No Ram Scoop Engines | NRSE | 160 |
| 8 | Cheap Engines | CE | 240 |
| 9 | Only Basic Remote Mining | OBRM | 255 |
| 10 | No Advanced Scanners | NAS | 325 |
| 11 | Low Starting Population | LSP | 180 |
| 12 | Bleeding Edge Technology | BET | 70 |
| 13 | Regenerating Shields | RS | 30 |

If `n` LRTs are selected, with `p` positive-price entries and `m` negative-price
entries, apply these additional deductions independently:

```text
if n > 4:     subtract 10*n*(n-4)
if p-m > 3:   subtract 60*(p-m-3)
if m-p > 3:   subtract 40*(m-p-3)
```

These are counts by **price sign**, not a semantic classification of beneficial
and harmful effects. OBRM can override ARM's equipment effect while **both**
selected flags still participate in price and count rules. NAS still has its
PRT-specific extra costs. TT changes future-habitat pricing samples from
allowances `[0,5,15]` to `[0,8,17]`, in addition to its -25 raw entry; its effect
is not a constant point subtraction. Sources:
[LRT aggregation](../src/core/race.cpp#L253),
[`LInnateRaceHabitability`](../src/core/race.cpp#L337).

Factory discount (bit 31) costs 175 internal points inside the non-AR factory
branch. Higher starting tech (bit 29) costs 180. Random (bit 30) has no direct
price entry. None of these three switches participates in the fourteen-LRT
count penalties. The research term uses the net count of cheap versus expensive
fields, with high-count and population-efficiency interactions; it does not
price each field independently. Sources:
[factory-discount price](../src/core/race.cpp#L227),
[research and extra-tech price](../src/core/race.cpp#L289),
[ordered formula reference](NEW-RACE-WIZARD.md#research-and-higher-starting-technology).

## 10. Reverse index: every PRT and LRT

This is a check on the property-first inventory: every selectable trait must
have its direct effects, granted/restricted equipment, initial state, and price
accounted for. These are references to the observed rules above, not a second
set of stacking definitions.

### 10.1 Primary traits

| PRT | Properties and rule families to resolve |
| --- | --- |
| HE | Effective growth; habitat-dependent capacity and resulting crowding/economy; exclusive engine/hulls/Flux Capacitor; no gates; three colony ships; price and factory/growth interaction. Sections 3, 5, 7, 8, 9. |
| SS | Innate cloak points/cargo weighting; special scanners and theft; mine safe speed; research gain; special hulls/armor/shield/cloaks; Electronics start and human-only extra transport; price/NAS interaction. Sections 5.4, 6, 7, 8, 9. |
| WM | Weapon cost; battle speed; attacking colonists; enemy design disclosure; advanced defense/minelayer restrictions; exclusive combat hulls/weapons; starting tech/fleets; price. Sections 5.5, 6, 7, 8, 9. |
| CA | Free annual/permanent/remote terraforming; paid terraforming cost; abandonment reversion; foreign habitat intelligence; Orbital Adjuster/Retro Bomb; starting tech/adjuster; price. Sections 4, 5.5, 6.1, 7, 8, 9. |
| IS | Fleet breeding/overflow; defending colonists; ship/base repair; defense/weapon cost; special transport/freighter, sharmor, armor, mine, jamming/detection/weapon grants; smart-bomb restrictions; price. Sections 3.4, 5, 6, 7, 8, 9. |
| SD | Safe mine speed, minefield scanners, design learning on contact, field decay, detonation UI, moving laying; exclusive mine hulls/dispensers/Energy Dampener; initial tech/minelayers; price. Sections 6, 7, 8, 9. |
| PP | Packet production/batches/drivers, launch/decay/capture-related context, global packet visibility/moving packet scanners, sender packet terraforming and catcher-base learning; extra start world/driver/fleets/tech; price/NAS. Sections 6.1, 6.5, 7, 8, 9. |
| IT | Gates/cargo/overgating-disappearance policy/gate-linked information; gate cost; Antimatter Generator; packet production overhead/decay class/catching penalty; extra starting world/gates/fleets/tech; price. Sections 5.5, 6, 7, 8, 9. |
| AR | Orbital capacity/habitation and colony loss on base destruction; no ordinary planetary installations/scanners/defenses; population/Energy/habitat resources; automatic/own-world remote mining; base cost; innate colony scanners; colonization/packet-damage/cargo-population rules; Death Star/orbital module; initial state; price/cheap Energy. Sections 3, 5, 6, 7, 8, 9. |
| JOAT | Capacity and indirect economy; Scout/Destroyer/Frigate built-in scans; tech3 baseline and higher-tech4 rule; broad starting fleet; price/NAS. Sections 3, 5, 6.1, 8, 9. |

### 10.2 Lesser traits

| LRT | Properties and rule families to resolve |
| --- | --- |
| IFE | Fuel consumption; Fuel Mizer/Galaxy Scoop access including NRSE interaction; starting Propulsion increment outside tutorial; price. Sections 6.3, 7.2, 8.1, 9. |
| TT | General terraforming tech/available per-axis reach; paid cost including CA interaction; future-habitat pricing allowance plus raw price/count. Sections 4, 7.2, 9. |
| ARM | Remote-mining hull/module eligibility; starter miners; OBRM override; raw price/count still retained if overridden. Sections 5.2, 7.2, 8.4, 9. |
| ISB | Space Dock/Ultra Station eligibility; one OR-shared AR/ISB base construction discount; base cloak points. Sections 5.6, 6.1, 7.2, 9. |
| GR | Main/side research distribution, rounding and SS pool interaction; price. Sections 5.4, 9. |
| UR | Destination-owner recycling permission; mineral recovery/location; resource salvage and production conversion; gifts/BET costing context; price. Sections 5.3, 9. |
| MA | Alchemy conversion resource cost; price. Sections 5.3, 9. |
| NRSE | Engine exclusions/Interspace-10 grant; IFE and special-engine exceptions; equipment-mediated fuel/safety behavior, not a generic growth factor; price. Sections 6.3, 7.2, 8.6, 9. |
| CE | Engine construction cost; fleet departure reliability; starting Propulsion increment; price. Sections 5.5, 6.3, 8.1, 9. |
| OBRM | Capacity after PRT including AR; indirect crowding/installation limits; remote-miner exclusions overriding ARM and starter grant; price. Sections 3.2, 5, 7.2, 8.4, 9. |
| NAS | Normal scan range; penetrating equipment restrictions; special/innate scanner behavior including AR distinct rule; raw price plus SS/PP/JOAT extra cost. Sections 6.1, 7.2, 9. |
| LSP | Initial population before AI/BBS/extra-world processing; downstream start population; price. Sections 8.2, 8.3, 9. |
| BET | Surplus miniaturization rate/cap; new-tech cost doubling; scrapping suppression; special-category source concern; price. Sections 5.3, 5.5, 9. |
| RS | Shield capacity/regeneration; armor-component contribution; equipment/hull distinction; price. Sections 6.2, 9. |

## 11. Composition implications and audit boundaries

The observations suggest several different kinds of component policy, rather
than one universal stacking operator:

| Observed property family | Information a component resolver needs |
| --- | --- |
| Capacity/growth/resource output | Race choices plus planet, population, habitat, hull, technology and source-order integer stages |
| PRT/LRT equipment eligibility | Grants, denials, precedence (OBRM over ARM), technology and independent acquired-item flags |
| Construction cost | Miniaturization stage, PRT/category adjustment, CE/BET stage, design/base upgrade stage and caller suppression flags |
| Scanners/cloaking | Nonlinear score/range combination, hull/equipment, player context and target visibility; not sums of percentages |
| Initial state | Ordered PRT base vector → higher-tech floor → CE/IFE increments; population baseline → AI → BBS → extra-world adjustment |
| Event abilities | Typed capabilities plus context/RNG/turn phase: theft, breeding, detonation, terraforming, gate losses and repairs |
| Point balance | Selected facts and nonlinear interactions, including selected-but-overridden trait effects |

This is an interpretation for future RDL work, **not** a new approved language
syntax or a claim that the existing RDL examples implement these policies.
The current RDL population examples cover only selected baseline multipliers.
Their `GrowthRate.value = 0.15` uses a fractional rate, whereas Stars! stores
`pctIdealGrowth = 15` as an integer percentage. Any mapping must explicitly
convert units before applying the source formulas.

Observed turn order matters: the main sequence unmarks mine visibility, moves
preproduction packets and fleets, decays/detonates fields and packets, breeds IS
cargo, produces, moves postproduction packets, replenishes fuel, applies orders,
sweeps mines, heals, then auto/remote terraforms. Sources:
[`FGenerateTurn` sequence](../src/core/turn.cpp#L239),
[late research pass](../src/core/turn.cpp#L891).
A modifier that is correct in one phase must not be applied again in every
other phase merely because its trait is present.

### Evidence limits

- Inventory coverage: all ten PRT identities, all fourteen LRT flags, all wizard
  design input families, trait-gated equipment in `FLookupPart`, and the direct
  trait branches located in the population/economy/research, ship/fleet/battle,
  creation, visibility/save and relevant UI paths. This is a source audit,
  not exhaustive enumeration of every reachable game state or a proof of
  behavioral equivalence for a new runtime.
- Equipment grants are separated from equipped-item effects. Appendix A gives
  exact catalog labels and technology prerequisites; complete hull slots,
  engine fuel curves, weapon damage/range and other ordinary item statistics
  remain in the linked catalogs. Every possible generated ship design is not
  an extra independent race property.
- UI restrictions are identified as such. No claim is made that the host
  independently rejects every hacked/imported illegal mount or detonation flag.
- Unsafe/out-of-domain source paths remain explicit: zero-width habitat can
  divide by zero; the `GetTruePartCost` special-category `cExcess` path is not
  assigned a guessed deterministic BET rule.
- Research pool membership, scanner special cases, field-owner versus
  fleet-owner effects, template inheritance and rounded units follow inspected
  code, even where shorthand descriptions of the original game suggest a
  different simplification. No unseen original-binary behavior is asserted.
- General AI strategy, every universe-generation probability, every combat
  interaction and every ordinary equipment statistic are outside this race
  property inventory. AI/tutorial/world-size exceptions affecting the listed
  initial race properties are included.

### Verification record

The audit traced named PRT values and LRT masks through `src/core` and relevant
`src/ui` consumers, checked resource names against `text/strings.txt`, and
checked granted/starting items against `data/parts/*.yaml`. Source references
pin the inspected game revision above. Documentation links and line anchors
were checked locally. Worked capacity and population examples were checked
against the stated integer order. No gameplay implementation or baseline was
changed; a source citation is evidence of the rule's location, not a new test
or formal proof of that rule.

Checks performed for this documentation change:

| Check | Result |
| --- | --- |
| Local report links and one-based source line anchors | All valid |
| Ten PRT and fourteen LRT raw price entries against source arrays | Exact match |
| Capacity/OBRM and IT/PP population worked examples | Integer-order arithmetic checked |
| `git diff --check` | Pass |
| `make compile regression test-unit tutorial` | SDL Debug build passes; 519 regression matches (25 with existing warnings), zero differences/errors/skipped inputs; 18 unit tests and one tutorial checkpoint pass |

## Appendix A. Exact equipment labels and technology prerequisites

These rows supply exact names/IDs and prerequisites used above; the presence of a row here is not itself a claim that it is trait-exclusive. Requirements are E/W/P/C/L/B.

|Catalog ID|Exact display label|Minimum tech|Source|
|---|---|---|---|
|iarmorFieldedKelarium (index 6)|Fielded Kelarium|4/0/0/10/0/0|[armor:60](../data/parts/armor.yaml#L60)|
|iarmorDepletedNeutronium (index 7)|Depleted Neutronium|0/0/0/10/3/0|[armor:69](../data/parts/armor.yaml#L69)|
|iarmorMegaPolyShell (index 9)|Mega Poly Shell|14/0/0/14/14/6|[armor:87](../data/parts/armor.yaml#L87)|
|ibeamMiniGun (index 2)|Mini Gun|0/5/0/0/0/0|[beams:28](../data/parts/beams.yaml#L28)|
|ibeamGatlingNeutrinoCannon (index 14)|Gatling Neutrino Cannon|0/17/0/0/0/0|[beams:162](../data/parts/beams.yaml#L162)|
|ibeamBlunderbuss (index 16)|Blunderbuss|0/19/0/0/0/0|[beams:185](../data/parts/beams.yaml#L185)|
|ibeamMultiContainedMunition (index 18)|Multi Contained Munition|21/21/0/0/16/12|[beams:206](../data/parts/beams.yaml#L206)|
|ibombHushABoom (index 8)|Hush-a-Boom|0/12/0/0/12/12|[bombs:95](../data/parts/bombs.yaml#L95)|
|ibombRetroBomb (index 9)|Retro Bomb|0/10/0/0/0/12|[bombs:106](../data/parts/bombs.yaml#L106)|
|ibombSmartBomb (index 10)|Smart Bomb|0/5/0/0/0/7|[bombs:115](../data/parts/bombs.yaml#L115)|
|ibombNeutronBomb (index 11)|Neutron Bomb|0/10/0/0/0/10|[bombs:125](../data/parts/bombs.yaml#L125)|
|ibombEnrichedNeutronBomb (index 12)|Enriched Neutron Bomb|0/15/0/0/0/12|[bombs:135](../data/parts/bombs.yaml#L135)|
|ibombPeerlessBomb (index 13)|Peerless Bomb|0/22/0/0/0/15|[bombs:145](../data/parts/bombs.yaml#L145)|
|ibombAnnihilatorBomb (index 14)|Annihilator Bomb|0/26/0/0/0/17|[bombs:155](../data/parts/bombs.yaml#L155)|
|ispecialETransportCloaking (index 0)|Transport Cloaking|0/0/0/0/0/0|[electrical:7](../data/parts/electrical.yaml#L7)|
|ispecialEStealthCloak (index 1)|Stealth Cloak|2/0/0/0/5/0|[electrical:15](../data/parts/electrical.yaml#L15)|
|ispecialESuperStealthCloak (index 2)|Super-Stealth Cloak|4/0/0/0/10/0|[electrical:24](../data/parts/electrical.yaml#L24)|
|ispecialEUltraStealthCloak (index 3)|Ultra-Stealth Cloak|10/0/0/0/12/0|[electrical:33](../data/parts/electrical.yaml#L33)|
|ispecialEMultiFunctionPod (index 4)|Multi Function Pod|11/0/11/0/11/0|[electrical:42](../data/parts/electrical.yaml#L42)|
|ispecialEBattleComputer (index 5)|Battle Computer|0/0/0/0/0/0|[electrical:51](../data/parts/electrical.yaml#L51)|
|ispecialEBattleSuperComputer (index 6)|Battle Super Computer|5/0/0/0/11/0|[electrical:59](../data/parts/electrical.yaml#L59)|
|ispecialEBattleNexus (index 7)|Battle Nexus|10/0/0/0/19/0|[electrical:68](../data/parts/electrical.yaml#L68)|
|ispecialEJammer10 (index 8)|Jammer 10|2/0/0/0/6/0|[electrical:77](../data/parts/electrical.yaml#L77)|
|ispecialEJammer20 (index 9)|Jammer 20|4/0/0/0/10/0|[electrical:86](../data/parts/electrical.yaml#L86)|
|ispecialEJammer30 (index 10)|Jammer 30|8/0/0/0/16/0|[electrical:95](../data/parts/electrical.yaml#L95)|
|ispecialEJammer50 (index 11)|Jammer 50|16/0/0/0/22/0|[electrical:104](../data/parts/electrical.yaml#L104)|
|ispecialEEnergyCapacitor (index 12)|Energy Capacitor|7/0/0/0/4/0|[electrical:113](../data/parts/electrical.yaml#L113)|
|ispecialEFluxCapacitor (index 13)|Flux Capacitor|14/0/0/0/8/0|[electrical:122](../data/parts/electrical.yaml#L122)|
|ispecialEEnergyDampener (index 14)|Energy Dampener|14/0/8/0/0/0|[electrical:131](../data/parts/electrical.yaml#L131)|
|ispecialETachyonDetector (index 15)|Tachyon Detector|8/0/0/0/14/0|[electrical:140](../data/parts/electrical.yaml#L140)|
|ispecialEAntiMatterGenerator (index 16)|Anti-matter Generator|0/12/0/0/0/7|[electrical:149](../data/parts/electrical.yaml#L149)|
|iengineSettlersDelight (index 0)|Settler's Delight|0/0/0/0/0/0|[engines:7](../data/parts/engines.yaml#L7)|
|iengineQuickJump5 (index 1)|Quick Jump 5|0/0/0/0/0/0|[engines:16](../data/parts/engines.yaml#L16)|
|iengineFuelMizer (index 2)|Fuel Mizer|0/0/2/0/0/0|[engines:23](../data/parts/engines.yaml#L23)|
|iengineLongHump6 (index 3)|Long Hump 6|0/0/3/0/0/0|[engines:33](../data/parts/engines.yaml#L33)|
|iengineDaddyLongLegs7 (index 4)|Daddy Long Legs 7|0/0/5/0/0/0|[engines:42](../data/parts/engines.yaml#L42)|
|iengineAlphaDrive8 (index 5)|Alpha Drive 8|0/0/7/0/0/0|[engines:51](../data/parts/engines.yaml#L51)|
|iengineTransGalacticDrive (index 6)|Trans-Galactic Drive|0/0/9/0/0/0|[engines:60](../data/parts/engines.yaml#L60)|
|iengineInterspace10 (index 7)|Interspace-10|0/0/11/0/0/0|[engines:69](../data/parts/engines.yaml#L69)|
|iengineEnigmaPulsar (index 8)|Enigma Pulsar|7/0/13/5/9/0|[engines:79](../data/parts/engines.yaml#L79)|
|iengineTransStar10 (index 9)|Trans-Star 10|0/0/23/0/0/0|[engines:89](../data/parts/engines.yaml#L89)|
|iengineRadiatingHydroRamScoop (index 10)|Radiating Hydro-Ram Scoop|2/0/6/0/0/0|[engines:98](../data/parts/engines.yaml#L98)|
|iengineSubGalacticFuelScoop (index 11)|Sub-Galactic Fuel Scoop|2/0/8/0/0/0|[engines:108](../data/parts/engines.yaml#L108)|
|iengineTransGalacticFuelScoop (index 12)|Trans-Galactic Fuel Scoop|3/0/9/0/0/0|[engines:117](../data/parts/engines.yaml#L117)|
|iengineTransGalacticSuperScoop (index 13)|Trans-Galactic Super Scoop|4/0/12/0/0/0|[engines:126](../data/parts/engines.yaml#L126)|
|iengineTransGalacticMizerScoop (index 14)|Trans-Galactic Mizer Scoop|4/0/16/0/0/0|[engines:135](../data/parts/engines.yaml#L135)|
|iengineGalaxyScoop (index 15)|Galaxy Scoop|5/0/20/0/0/0|[engines:144](../data/parts/engines.yaml#L144)|
|ihuldefSuperFreighter (index 3)|Super Freighter|0/0/0/13/0/0|[hulls:60](../data/parts/hulls.yaml#L60)|
|ihuldefBattleCruiser (index 8)|Battle Cruiser|0/0/0/10/0/0|[hulls:168](../data/parts/hulls.yaml#L168)|
|ihuldefDreadnought (index 10)|Dreadnought|0/0/0/16/0/0|[hulls:220](../data/parts/hulls.yaml#L220)|
|ihuldefRogue (index 12)|Rogue|0/0/0/8/0/0|[hulls:273](../data/parts/hulls.yaml#L273)|
|ihuldefMiniColonyShip (index 14)|Mini-Colony Ship|0/0/0/0/0/0|[hulls:326](../data/parts/hulls.yaml#L326)|
|ihuldefStealthBomber (index 18)|Stealth Bomber|0/0/0/8/0/0|[hulls:396](../data/parts/hulls.yaml#L396)|
|ihuldefMidgetMiner (index 20)|Midget Miner|0/0/0/0/0/0|[hulls:440](../data/parts/hulls.yaml#L440)|
|ihuldefMiner (index 22)|Miner|0/0/0/6/0/0|[hulls:475](../data/parts/hulls.yaml#L475)|
|ihuldefMaxiMiner (index 23)|Maxi-Miner|0/0/0/11/0/0|[hulls:496](../data/parts/hulls.yaml#L496)|
|ihuldefUltraMiner (index 24)|Ultra-Miner|0/0/0/14/0/0|[hulls:517](../data/parts/hulls.yaml#L517)|
|ihuldefFuelTransport (index 25)|Fuel Transport|0/0/0/4/0/0|[hulls:538](../data/parts/hulls.yaml#L538)|
|ihuldefMiniMineLayer (index 27)|Mini Mine Layer|0/0/0/0/0/0|[hulls:573](../data/parts/hulls.yaml#L573)|
|ihuldefSuperMineLayer (index 28)|Super Mine Layer|0/0/0/15/0/0|[hulls:591](../data/parts/hulls.yaml#L591)|
|ihuldefMiniMorph (index 30)|Mini Morph|0/0/0/8/0/0|[hulls:642](../data/parts/hulls.yaml#L642)|
|ihuldefMetaMorph (index 31)|Meta Morph|0/0/0/10/0/0|[hulls:667](../data/parts/hulls.yaml#L667)|
|ispecialMColonizationModule (index 0)|Colonization Module|0/0/0/0/0/0|[mechanical:7](../data/parts/mechanical.yaml#L7)|
|ispecialMOrbitalConstructionModule (index 1)|Orbital Construction Module|0/0/0/0/0/0|[mechanical:15](../data/parts/mechanical.yaml#L15)|
|ispecialMMultiCargoPod (index 4)|Multi Cargo Pod|5/0/0/11/5/0|[mechanical:41](../data/parts/mechanical.yaml#L41)|
|ispecialMJumpGate (index 9)|Jump Gate|16/0/20/20/16/0|[mechanical:85](../data/parts/mechanical.yaml#L85)|
|iminesMineDispenser40 (index 0)|Mine Dispenser 40|0/0/0/0/0/0|[mine-layers:7](../data/parts/mine-layers.yaml#L7)|
|iminesMineDispenser50 (index 1)|Mine Dispenser 50|2/0/0/0/0/4|[mine-layers:15](../data/parts/mine-layers.yaml#L15)|
|iminesMineDispenser80 (index 2)|Mine Dispenser 80|3/0/0/0/0/7|[mine-layers:24](../data/parts/mine-layers.yaml#L24)|
|iminesMineDispenser130 (index 3)|Mine Dispenser 130|6/0/0/0/0/12|[mine-layers:33](../data/parts/mine-layers.yaml#L33)|
|iminesHeavyDispenser50 (index 4)|Heavy Dispenser 50|5/0/0/0/0/3|[mine-layers:42](../data/parts/mine-layers.yaml#L42)|
|iminesHeavyDispenser110 (index 5)|Heavy Dispenser 110|9/0/0/0/0/5|[mine-layers:51](../data/parts/mine-layers.yaml#L51)|
|iminesHeavyDispenser200 (index 6)|Heavy Dispenser 200|14/0/0/0/0/7|[mine-layers:60](../data/parts/mine-layers.yaml#L60)|
|iminesSpeedTrap20 (index 7)|Speed Trap 20|0/0/2/0/0/2|[mine-layers:69](../data/parts/mine-layers.yaml#L69)|
|iminesSpeedTrap30 (index 8)|Speed Trap 30|0/0/3/0/0/6|[mine-layers:78](../data/parts/mine-layers.yaml#L78)|
|iminesSpeedTrap50 (index 9)|Speed Trap 50|0/0/5/0/0/11|[mine-layers:87](../data/parts/mine-layers.yaml#L87)|
|iminingRoboMidgetMiner (index 0)|Robo-Midget Miner|0/0/0/0/0/0|[mining-robots:7](../data/parts/mining-robots.yaml#L7)|
|iminingRoboMiniMiner (index 1)|Robo-Mini-Miner|0/0/0/2/1/0|[mining-robots:15](../data/parts/mining-robots.yaml#L15)|
|iminingRoboMiner (index 2)|Robo-Miner|0/0/0/4/2/0|[mining-robots:24](../data/parts/mining-robots.yaml#L24)|
|iminingRoboMaxiMiner (index 3)|Robo-Maxi-Miner|0/0/0/7/4/0|[mining-robots:33](../data/parts/mining-robots.yaml#L33)|
|iminingRoboSuperMiner (index 4)|Robo-Super-Miner|0/0/0/12/6/0|[mining-robots:42](../data/parts/mining-robots.yaml#L42)|
|iminingRoboUltraMiner (index 5)|Robo-Ultra-Miner|0/0/0/15/8/0|[mining-robots:51](../data/parts/mining-robots.yaml#L51)|
|iminingAlienMiner (index 6)|Alien Miner|5/0/0/10/5/5|[mining-robots:60](../data/parts/mining-robots.yaml#L60)|
|iminingOrbitalAdjuster (index 7)|Orbital Adjuster|0/0/0/0/0/6|[mining-robots:69](../data/parts/mining-robots.yaml#L69)|
|iplanetaryViewer50 (index 0)|Viewer 50|0/0/0/0/0/0|[planetary:7](../data/parts/planetary.yaml#L7)|
|iplanetaryViewer90 (index 1)|Viewer 90|0/0/0/0/1/0|[planetary:14](../data/parts/planetary.yaml#L14)|
|iplanetaryScoper150 (index 2)|Scoper 150|0/0/0/0/3/0|[planetary:22](../data/parts/planetary.yaml#L22)|
|iplanetaryScoper220 (index 3)|Scoper 220|0/0/0/0/6/0|[planetary:30](../data/parts/planetary.yaml#L30)|
|iplanetaryScoper280 (index 4)|Scoper 280|0/0/0/0/8/0|[planetary:38](../data/parts/planetary.yaml#L38)|
|iplanetarySnooper320X (index 5)|Snooper 320X|3/0/0/0/10/3|[planetary:46](../data/parts/planetary.yaml#L46)|
|iplanetarySnooper400X (index 6)|Snooper 400X|4/0/0/0/13/6|[planetary:54](../data/parts/planetary.yaml#L54)|
|iplanetarySnooper500X (index 7)|Snooper 500X|5/0/0/0/16/7|[planetary:62](../data/parts/planetary.yaml#L62)|
|iplanetarySnooper620X (index 8)|Snooper 620X|7/0/0/0/23/9|[planetary:70](../data/parts/planetary.yaml#L70)|
|iplanetarySDI (index 9)|SDI|0/0/0/0/0/0|[planetary:78](../data/parts/planetary.yaml#L78)|
|iplanetaryMissileBattery (index 10)|Missile Battery|5/0/0/0/0/0|[planetary:85](../data/parts/planetary.yaml#L85)|
|iplanetaryLaserBattery (index 11)|Laser Battery|10/0/0/0/0/0|[planetary:93](../data/parts/planetary.yaml#L93)|
|iplanetaryPlanetaryShield (index 12)|Planetary Shield|16/0/0/0/0/0|[planetary:101](../data/parts/planetary.yaml#L101)|
|iplanetaryNeutronShield (index 13)|Neutron Shield|23/0/0/0/0/0|[planetary:109](../data/parts/planetary.yaml#L109)|
|iplanetaryGenesisDevice (index 14)|Genesis Device|20/10/10/20/10/20|[planetary:117](../data/parts/planetary.yaml#L117)|
|iscannerPickPocketScanner (index 5)|Pick Pocket Scanner|4/0/0/0/4/4|[scanners:50](../data/parts/scanners.yaml#L50)|
|iscannerChameleonScanner (index 6)|Chameleon Scanner|3/0/0/0/6/0|[scanners:60](../data/parts/scanners.yaml#L60)|
|iscannerFerretScanner (index 7)|Ferret Scanner|3/0/0/0/7/2|[scanners:70](../data/parts/scanners.yaml#L70)|
|iscannerDolphinScanner (index 8)|Dolphin Scanner|5/0/0/0/10/4|[scanners:80](../data/parts/scanners.yaml#L80)|
|iscannerElephantScanner (index 12)|Elephant Scanner|6/0/0/0/16/7|[scanners:117](../data/parts/scanners.yaml#L117)|
|iscannerRobberBaronScanner (index 14)|Robber Baron Scanner|10/0/0/0/15/10|[scanners:136](../data/parts/scanners.yaml#L136)|
|ishieldCrobySharmor (index 3)|Croby Sharmor|7/0/0/4/0/0|[shields:33](../data/parts/shields.yaml#L33)|
|ishieldShadowShield (index 4)|Shadow Shield|7/0/0/0/3/0|[shields:42](../data/parts/shields.yaml#L42)|
|ishieldLangstonShell (index 6)|Langston Shell|12/0/9/0/9/0|[shields:60](../data/parts/shields.yaml#L60)|
|isbhullSpaceDock (index 1)|Space Dock|0/0/0/4/0/0|[starbase-hulls:25](../data/parts/starbase-hulls.yaml#L25)|
|isbhullUltraStation (index 3)|Ultra Station|0/0/0/12/0/0|[starbase-hulls:76](../data/parts/starbase-hulls.yaml#L76)|
|isbhullDeathStar (index 4)|Death Star|0/0/0/17/0/0|[starbase-hulls:108](../data/parts/starbase-hulls.yaml#L108)|
|ispecialSBStargate100250 (index 0)|Stargate 100/250|0/0/5/5/0/0|[starbase-specials:7](../data/parts/starbase-specials.yaml#L7)|
|ispecialSBStargateAny300 (index 1)|Stargate any/300|0/0/6/10/0/0|[starbase-specials:16](../data/parts/starbase-specials.yaml#L16)|
|ispecialSBStargate150600 (index 2)|Stargate 150/600|0/0/11/7/0/0|[starbase-specials:25](../data/parts/starbase-specials.yaml#L25)|
|ispecialSBStargate300500 (index 3)|Stargate 300/500|0/0/9/13/0/0|[starbase-specials:34](../data/parts/starbase-specials.yaml#L34)|
|ispecialSBStargate100Any (index 4)|Stargate 100/any|0/0/16/12/0/0|[starbase-specials:43](../data/parts/starbase-specials.yaml#L43)|
|ispecialSBStargateAny800 (index 5)|Stargate any/800|0/0/12/18/0/0|[starbase-specials:52](../data/parts/starbase-specials.yaml#L52)|
|ispecialSBStargateAnyAny (index 6)|Stargate any/any|0/0/19/24/0/0|[starbase-specials:61](../data/parts/starbase-specials.yaml#L61)|
|ispecialSBMassDriver5 (index 7)|Mass Driver 5|4/0/0/0/0/0|[starbase-specials:70](../data/parts/starbase-specials.yaml#L70)|
|ispecialSBMassDriver6 (index 8)|Mass Driver 6|7/0/0/0/0/0|[starbase-specials:78](../data/parts/starbase-specials.yaml#L78)|
|ispecialSBMassDriver7 (index 9)|Mass Driver 7|9/0/0/0/0/0|[starbase-specials:86](../data/parts/starbase-specials.yaml#L86)|
|ispecialSBSuperDriver8 (index 10)|Super Driver 8|11/0/0/0/0/0|[starbase-specials:94](../data/parts/starbase-specials.yaml#L94)|
|ispecialSBSuperDriver9 (index 11)|Super Driver 9|13/0/0/0/0/0|[starbase-specials:102](../data/parts/starbase-specials.yaml#L102)|
|ispecialSBUltraDriver10 (index 12)|Ultra Driver 10|15/0/0/0/0/0|[starbase-specials:110](../data/parts/starbase-specials.yaml#L110)|
|ispecialSBUltraDriver11 (index 13)|Ultra Driver 11|17/0/0/0/0/0|[starbase-specials:118](../data/parts/starbase-specials.yaml#L118)|
|ispecialSBUltraDriver12 (index 14)|Ultra Driver 12|20/0/0/0/0/0|[starbase-specials:126](../data/parts/starbase-specials.yaml#L126)|
|ispecialSBUltraDriver13 (index 15)|Ultra Driver 13|24/0/0/0/0/0|[starbase-specials:134](../data/parts/starbase-specials.yaml#L134)|
|iterraTotalTerraform3 (index 0)|Total Terraform ±3|0/0/0/0/0/0|[terraformers:7](../data/parts/terraformers.yaml#L7)|
|iterraTotalTerraform5 (index 1)|Total Terraform ±5|0/0/0/0/0/3|[terraformers:13](../data/parts/terraformers.yaml#L13)|
|iterraTotalTerraform7 (index 2)|Total Terraform ±7|0/0/0/0/0/6|[terraformers:20](../data/parts/terraformers.yaml#L20)|
|iterraTotalTerraform10 (index 3)|Total Terraform ±10|0/0/0/0/0/9|[terraformers:27](../data/parts/terraformers.yaml#L27)|
|iterraTotalTerraform15 (index 4)|Total Terraform ±15|0/0/0/0/0/13|[terraformers:34](../data/parts/terraformers.yaml#L34)|
|iterraTotalTerraform20 (index 5)|Total Terraform ±20|0/0/0/0/0/17|[terraformers:41](../data/parts/terraformers.yaml#L41)|
|iterraTotalTerraform25 (index 6)|Total Terraform ±25|0/0/0/0/0/22|[terraformers:48](../data/parts/terraformers.yaml#L48)|
|iterraTotalTerraform30 (index 7)|Total Terraform ±30|0/0/0/0/0/25|[terraformers:55](../data/parts/terraformers.yaml#L55)|
|iterraGravityTerraform3 (index 8)|Gravity Terraform ±3|0/0/1/0/0/1|[terraformers:62](../data/parts/terraformers.yaml#L62)|
|iterraGravityTerraform7 (index 9)|Gravity Terraform ±7|0/0/5/0/0/2|[terraformers:69](../data/parts/terraformers.yaml#L69)|
|iterraGravityTerraform11 (index 10)|Gravity Terraform ±11|0/0/10/0/0/3|[terraformers:76](../data/parts/terraformers.yaml#L76)|
|iterraGravityTerraform15 (index 11)|Gravity Terraform ±15|0/0/16/0/0/4|[terraformers:83](../data/parts/terraformers.yaml#L83)|
|iterraTempTerraform3 (index 12)|Temp Terraform ±3|1/0/0/0/0/1|[terraformers:90](../data/parts/terraformers.yaml#L90)|
|iterraTempTerraform7 (index 13)|Temp Terraform ±7|5/0/0/0/0/2|[terraformers:97](../data/parts/terraformers.yaml#L97)|
|iterraTempTerraform11 (index 14)|Temp Terraform ±11|10/0/0/0/0/3|[terraformers:104](../data/parts/terraformers.yaml#L104)|
|iterraTempTerraform15 (index 15)|Temp Terraform ±15|16/0/0/0/0/4|[terraformers:111](../data/parts/terraformers.yaml#L111)|
|iterraRadiationTerraform3 (index 16)|Radiation Terraform ±3|0/1/0/0/0/1|[terraformers:118](../data/parts/terraformers.yaml#L118)|
|iterraRadiationTerraform7 (index 17)|Radiation Terraform ±7|0/5/0/0/0/2|[terraformers:125](../data/parts/terraformers.yaml#L125)|
|iterraRadiationTerraform11 (index 18)|Radiation Terraform ±11|0/10/0/0/0/3|[terraformers:132](../data/parts/terraformers.yaml#L132)|
|iterraRadiationTerraform15 (index 19)|Radiation Terraform ±15|0/16/0/0/0/4|[terraformers:139](../data/parts/terraformers.yaml#L139)|
|itorpAntiMatterTorpedo (index 7)|Anti Matter Torpedo|0/11/12/0/0/21|[torpedoes:89](../data/parts/torpedoes.yaml#L89)|

## Appendix B. Starting templates, hulls and fitted slots

The listed slot counts/components are template values before the creation upgrades described above. Unspecified `iItem` is zero. Unspecified ship `ihuldef` resolves to zero (Small Freighter). Slot-only types with count zero are omitted below. Other template fields (fuel/cargo overrides etc.) remain in the linked rows.

|Index|Exact class label|Hull|Nonempty template slots|Source|
|---|---|---|---|---|
|0|Lilliputian Freighter|ihuldefSmallFreighter|1×Quick Jump 5; 1×Bat Scanner; 1×Mole-skin Shield|[starting-ships:4](../data/parts/starting-ships.yaml#L4)|
|1|Shadow Transport|ihuldefSmallFreighter|1×Quick Jump 5; 1×Transport Cloaking; 1×Mole-skin Shield|[starting-ships:17](../data/parts/starting-ships.yaml#L17)|
|2|Smaugarian Peeping Tom|ihuldefScout|1×Quick Jump 5; 1×Bat Scanner; 1×Fuel Tank|[starting-ships:32](../data/parts/starting-ships.yaml#L32)|
|3|Armed Probe|ihuldefScout|1×Quick Jump 5; 1×Bat Scanner; 1×X-Ray Laser|[starting-ships:47](../data/parts/starting-ships.yaml#L47)|
|4|Long Range Scout|ihuldefScout|1×Quick Jump 5; 1×Bat Scanner; 1×Fuel Tank|[starting-ships:62](../data/parts/starting-ships.yaml#L62)|
|5|Shadow Sleuth|ihuldefScout|1×Quick Jump 5; 1×Bat Scanner; 1×Stealth Cloak|[starting-ships:77](../data/parts/starting-ships.yaml#L77)|
|6|Teamster|ihuldefMediumFreighter|1×Quick Jump 5; 1×Bat Scanner; 1×Tritanium|[starting-ships:92](../data/parts/starting-ships.yaml#L92)|
|7|Stalwart Defender|ihuldefDestroyer|1×Quick Jump 5; 1×Laser; 1×Alpha Torpedo; 1×Bat Scanner; 2×Tritanium; 1×Fuel Tank; 1×Battle Computer|[starting-ships:107](../data/parts/starting-ships.yaml#L107)|
|8|Swashbuckler|ihuldefPrivateer|1×Quick Jump 5; 2×Crobmnium; 1×Bat Scanner; 1×Laser; 1×Alpha Torpedo|[starting-ships:127](../data/parts/starting-ships.yaml#L127)|
|9|Santa Maria|ihuldefColonyShip|1×Quick Jump 5; 1×Colonization Module|[starting-ships:143](../data/parts/starting-ships.yaml#L143)|
|10|Pinta|ihuldefColonyShip|1×Quick Jump 5; 1×Orbital Construction Module|[starting-ships:158](../data/parts/starting-ships.yaml#L158)|
|11|Mayflower|ihuldefColonyShip|1×Quick Jump 5; 1×Colonization Module|[starting-ships:173](../data/parts/starting-ships.yaml#L173)|
|12|Spore Cloud|ihuldefMiniColonyShip|1×Settler's Delight; 1×Colonization Module|[starting-ships:188](../data/parts/starting-ships.yaml#L188)|
|13|Gadfly|ihuldefMiniBomber|1×Quick Jump 5; 2×Lady Finger Bomb|[starting-ships:203](../data/parts/starting-ships.yaml#L203)|
|14|Cotton Picker|ihuldefMiniMiner|1×Quick Jump 5; 1×Bat Scanner; 1×Robo-Mini-Miner; 1×Robo-Mini-Miner|[starting-ships:216](../data/parts/starting-ships.yaml#L216)|
|15|Potato Bug|ihuldefMidgetMiner|1×Quick Jump 5; 2×Robo-Midget Miner|[starting-ships:231](../data/parts/starting-ships.yaml#L231)|
|16|Little Hen|ihuldefMiniMineLayer|1×Quick Jump 5; 2×Mine Dispenser 40; 2×Mine Dispenser 40; 1×Bat Scanner|[starting-ships:244](../data/parts/starting-ships.yaml#L244)|
|17|Change of Heart|ihuldefMiniMiner|1×Quick Jump 5; 1×Bat Scanner; 1×Orbital Adjuster; 1×Orbital Adjuster|[starting-ships:260](../data/parts/starting-ships.yaml#L260)|
|18|Speed Turtle|ihuldefMiniMineLayer|1×Quick Jump 5; 2×Speed Trap 20; 2×Speed Trap 20; 1×Bat Scanner|[starting-ships:276](../data/parts/starting-ships.yaml#L276)|

|Base template index|Exact class label|Hull|Source|
|---|---|---|---|
|0|Starbase|ihuldefSpaceStation|[starting-starbases:4](../data/parts/starting-starbases.yaml#L4)|
|1|Accelerator Platform|ihuldefOrbitalFort|[starting-starbases:31](../data/parts/starting-starbases.yaml#L31)|
|2|Porthole to Beyond|ihuldefOrbitalFort|[starting-starbases:50](../data/parts/starting-starbases.yaml#L50)|
|3|Starter Colony|ihuldefOrbitalFort|[starting-starbases:70](../data/parts/starting-starbases.yaml#L70)|
