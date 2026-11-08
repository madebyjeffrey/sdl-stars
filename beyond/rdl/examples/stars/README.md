# Stars! race designs and calculations

`../predefined.rdl` loads six complete fixed race designs, the Random generation
seed, the component/trait catalog, and the contextual calculation types. This
library is independent of the game executable and does not write game files.

From `beyond/rdl`:

```sh
cargo build --offline --release
./target/release/rdl run examples/predefined.rdl
./target/release/rdl repl examples/predefined.rdl
```

The fixed objects are `humanoid`, `rabbitoid`, `insectoid`, `nucleotid`,
`silicanoid`, and `antetheral`, all `Race`. `random` is a
`RandomRaceTemplate`: its HE field is a placeholder, not a selected final PRT.
The source random generator chooses a design before play. RDL exposes its seed
and generation policy; it does not execute the game's random-design search.

## Files and composition

| Module | Contents |
| --- | --- |
| `schema.rdl` | Shared PRT/LRT contracts and scalar population components |
| `design.rdl` | Validated design components, race schemas, shared habitat calculations and wizard pricing (`Race.points`) |
| `prt.rdl` | Ten PRT components and selection traits, with stable source IDs, descriptions and raw signed prices |
| `lrt.rdl` | Fourteen LRT components and selection traits, bit indexes/masks and raw signed price adjustments |
| `settings.rdl` | Reusable traits for individual habitat, growth, economy, research, portrait and leftover choices |
| `predefined/*.rdl` | Race objects selecting generic traits, plus their own identity/source metadata |
| `predefined.rdl` | Imports the seven individual preset files |
| `catalog.rdl` | Equipment names, source IDs, costs, technology prerequisites and fitted starting templates |
| `rules.rdl` | Imports all calculation modules in `rules/` |

Each preset is a `Race` object. Names and source-table indexes belong to the
object; gameplay choices select reusable traits. The full Humanoid definition is:

```ocaml
Race humanoid = {
  name = "Humanoid";
  plural_name = "Humanoids";
  source_index = 0;
  portrait(1)
  gravity(15, 85)
  temperature(15, 85)
  radiation(15, 85)
  growth(15)
  population_efficiency(1000)
  factory_output(10)
  factory_cost(10)
  factory_operation(10)
  mine_output(10)
  mine_cost(5)
  mine_operation(10)
  energy_research(standard)
  weapons_research(standard)
  propulsion_research(standard)
  construction_research(standard)
  electronics_research(standard)
  biotechnology_research(standard)
  research_allocation(15)
  leftover_points(surface_minerals)
  standard_capacity
  jack_of_all_trades
}
```

`settings.rdl` defines parameterized traits. A selection binds typed arguments
to a declarative template and supplies components:

```ocaml
trait gravity(lower: integer, upper: integer) {
  require lower >= 0 && upper <= 100 && lower < upper && upper - lower >= 2
  provides GravityTolerance {
    range = {
      lower = lower;
      preferred = lower + (upper - lower) / 2;
      upper = upper;
    };
  }
}
trait factory_cost(value: integer) {
  provides FactoryCost = value
}
```

Humanoid uses `factory_cost(10)` and Rabbitoid uses `factory_cost(9)`, both
applications of the same template. No separate trait for every numeric value is
needed. Immunity uses the unary overloads `gravity(immune)`,
`temperature(immune)` and `radiation(immune)`. The binary overloads accept lower
and upper bounds; `Immunity` is a singleton type containing only `immune`.
Research choices use `energy_research(standard)` or `(cheap)` / `(expensive)`;
leftovers use `leftover_points(surface_minerals)` and the other source options.

Trait selections appear directly inside the object. Assignment fields such as
`name = "Humanoid";` keep their separators; the trait lines need no semicolons.
Each application is an immutable value identified by the template and its typed
arguments. The template can supply values, modifiers and validation requirements;
it cannot mutate the world or run an imperative callback. Existing pure RDL
expressions still calculate values such as a range midpoint.

`growth(15)` supplies the stored selection and its real fraction baseline,
two representations of the same choice. HE modifies the fraction without
rewriting the stored selection used for pricing. Population efficiency takes
colonists/resource and requires a multiple of 100, such as
`population_efficiency(1000)`.

`RaceIdentity`, `DesignSettings`, `Habitability`, `Economy` and
`ResearchPreferences` are read-only views assembled from the object's metadata
and resolved individual components. Existing queries and contextual calculations
can still use these grouped views. The individual components are also directly
queryable, such as `humanoid.FactoryCost`.

The PRT trait provides a concrete `PrimaryRacialTrait`. `standard_capacity`
provides the 1,000,000-colonist reference; HE, JOAT and OBRM modify that scalar.
The PRT is identified by its component's `source_id`. LRT flags likewise resolve
from provided components. PRTs, LRTs and switches are selected as bare trait lines,
for example `jack_of_all_trades`, `lrt_ife` or `cheaper_factories`. The read-only
`humanoid.traits` set exposes the inline selections for inspection. Legacy object
schemas with explicitly declared trait-valued fields remain supported.

Use `examples/predefined.rdl` as the editor/REPL entry file. To load an individual
preset directly, add `-I examples` to the CLI or the equivalent LSP import path.

A `Race` requires exactly one of each mandatory component and allows zero
or more secondary components. Higher starting technology and cheaper factories
are separate optional components; neither is an LRT. The Random seed uses its
own object schema and does not resolve final PRT effects.

To design another race, select any combination of the shared settings traits,
a PRT and LRTs. To change one numeric choice, replace that trait: for example,
change `factory_cost(10)` to `factory_cost(9)` without changing factory output,
operation limits or mining. Any value satisfying the component constraints can be passed; a new numeric
value does not require another trait declaration. Missing settings and conflicting choices are
validation errors; declaration order does not override an earlier choice.

## Inputs, units and validation

All stored wizard inputs are preserved: identity/portrait, three habitat
minimum/preferred/maximum triples, selected growth, seven economy choices,
six research cost classes, initial research allocation, leftover allocation,
PRT, fourteen LRT flags, and the Tech3/CheapFact switches. Source player index,
visibility, current research/technology, controller, credentials and live
colony/fleet state are not extra reusable wizard parameters. Preset credentials
are unset. `DesignSettings.source_index` identifies the source table row.

Habitat coordinates are integers on the original 0..100 scale. An immune axis
is `{ lower = -1; preferred = -1; upper = -1; }`. A nonimmune axis must have an
interior preferred value equal to the source midpoint. Degenerate, zero-width
ranges are rejected, rather than entering the original divide-by-zero path.
`EnvironmentDisplay` converts coordinates to displayed gravity, °C and mR.
Nucleotid's radiation minimum is **0**, because the omitted YAML array element
is zero-initialized by `data/datagen.py`.

Population and population deltas in calculation contexts are **hundreds of
colonists**, matching the game. Fields ending in `_colonists` convert to people.
`GrowthRate` is a real fraction; `selected_growth` and
`effective_growth_percent` are percentage integers. `CarryingCapacity` is the
100%-habitat surface reference in people. Actual capacity comes from `Colony`;
AR uses its supplied orbital hull rather than the surface reference.

Research cost classes retain the source encoding: 0 expensive, 1 normal,
2 cheap. `TechVector` field names are energy, weapons, propulsion,
construction, electronics and biotechnology. Orbital hull indexes 0..4 mean
Orbital Fort, Space Dock, Space Station, Ultra Station and Death Star.

Each calculation takes an explicit snapshot. Random outcomes are supplied as
draws or named success booleans, so evaluating a query cannot change the world
or consume hidden random state. For a whole year, feed each step's returned
state/remainders into the next step in the source's order. The library calculates
rules; a host must still schedule events, select targets, apply mutations and
iterate fleets/planets. That distinction matters for correlated PP terraforming,
CA's single random axis, minefield detonation, gift values and recycling pools.

Optional calculation fields have ordinary checked defaults, for example
`delta_remainder: integer = 0`. Defaults are evaluated in global scope, checked
even when unused, and may be overridden by the caller. Race settings remain explicit through their selected generic traits.
Identity fields and source indexes are stored directly on each object.

## Queries

Inspect stored and resolved values directly:

```ocaml
nucleotid.Habitability.radiation
silicanoid.CarryingCapacity
rabbitoid.flag_mask
rabbitoid.PrimaryRacialTrait
```

Compare all fixed designs:

```ocaml
query {
  from r: Race
  order by r.DesignSettings.source_index ascending
  select {
    name = r.name;
    prt = r.PrimaryRacialTrait.name;
    growth = r.effective_growth_percent;
    capacity_at_ideal_surface_habitat = r.CarryingCapacity;
  }
}
```

Evaluate actual planet and starting-game contexts:

```ocaml
(PlanetHabitat {
  race = humanoid;
  gravity = 50; temperature = 50; radiation = 50;
}).percent

(Colony {
  race = silicanoid;
  habitat = 100;
  population = 250;
  factories = 10; mines = 10;
}).capacity_colonists

(StartingState {
  race = rabbitoid;
  galaxy_size = 1;
  accelerated = false;
}).technology

(StartingState { race = rabbitoid; galaxy_size = 1; }).home_colonists
(StartingState { race = rabbitoid; galaxy_size = 1; }).second_colonists

humanoid.points
```

These return 100% habitat, 550,000 capacity, technology E/W/P/C/L/B =
0/0/7/5/0/0, 20,000 homeworld colonists, 10,000 second-world colonists, and
25 wizard points respectively. `Race.points` is the read-only remaining advantage
point balance, derived using `WizardPricing`. The explicit calculation type remains
available for inspecting its intermediate terms:

```ocaml
(WizardPricing { race = humanoid; }).internal_points
query { from r: Race select { name = r.name; points = r.points; } }
```

Pricing and its habitat samples are included by `design.rdl`, so an individual
preset import also supports `.points`. `rules/pricing.rdl` remains a compatibility
entry for those types. `points` is lazy: world loading checks stored constraints
and component cardinalities without calculating scores. The first `humanoid.points`
query computes and validates the score; subsequent reads reuse it until a successful
reload or world extension. A `require` rule that reads `points` forces that
calculation during validation. Failed reloads preserve the current world and cache.

Population 250 means 25,000 people. Tiny is
`galaxy_size = 0` and does not give IT/PP a second world.

Find eligible starting equipment, or inspect the fitted template catalog:

```ocaml
query {
  from p: CatalogPart
  where (EquipmentEligibility {
    race = rabbitoid;
    part = p;
    technology = (StartingState { race = rabbitoid; }).technology;
  }).eligible
  select p.name
}

query {
  from t: StartingTemplate
  where not t.starbase
  where (StartingFleetCount {
    state = StartingState { race = nucleotid; };
    template_index = t.index;
  }).home_fleets > 0
  select { name = t.name; hull = t.hull; slots = t.slots; }
}
```

`StartingPartUpgrade` applies the source's ordered candidate lists to a fitted
slot. Supply the template category/index and whether its hull is Colony Ship.
It preserves a placeholder when no candidate qualifies. Starbase templates skip
this pass. `StartingBaseSlot` applies the separate homeworld PP driver / IT gate
installation; empty template slots are retained with quantity zero. `EquipmentEligibility` applies traits, Trader acquisition and all
six prerequisites; `EquipmentMount` adds the separate UI hull restrictions.
Trader acquisitions are a set of source part symbols, such as
`{"iengineEnigmaPulsar"}`. They default to none for starting players.

## Calculation inventory

The implementation follows [the property report](../../../../docs/RACE-PROPERTIES-AND-TRAIT-EFFECTS.md)
and the current binary64 game code. Each module cites its owning functions.
These are calculation interfaces: scalar fields named `sum`, `total`, `ability`
or `previous_*` are explicitly supplied aggregated equipment or live state,
not additional race constants.

| Module / report section | Executable calculation types |
| --- | --- |
| Population, §3 | `HabitatEvaluation`, `PlanetHabitat`, `Colony` (capacity, operation limits, resources, growth, persistent fractional remainder and AR base dependency), `FleetPopulation` (IS breeding, excess transfer, AR movement loss and radiating-engine mortality) |
| Habitat and terraforming, §4 | `EnvironmentDisplay`, `TerraformReach`, `TerraformAxis`, `TerraformSelection`, `ClaimAdjustment`, `RemoteTerraform` |
| Mining, §5.2 | `MiningYield` (whole/fractional yield and distinct depletion input), `RemoteMining` |
| Installations and recovery, §5.1/5.3 | `InstallationCosts`, `ScrapDesignValue`, `Recycling` (destination-owner UR, u16 pool wrap and diminishing production benefit) |
| Research, §5.4 | `ResearchCost`, `ResearchAllocation`, `ResearchLevelCost` table (GR distribution, slow-tech ordering, SS annual pool) |
| Construction, §5.5/5.6 | `EquipmentEligibility.excess`, `PartCost`, `EquipmentCost` (four channels), `StarbaseCost` (one shared AR/ISB discount followed by common halving) |
| Information, §6.1 | `CloakCurve`, `ComponentCloak`, `DesignCloak`, `FleetCloakContribution`, `FleetCloak`, `ComponentScanning`, `ScannerContribution`, `ShipScanning`, `PlanetScanning`, `FleetScanStep`, `Detection`, `Intelligence`, `TachyonFactor` table |
| Combat, §6.2 | `BattleMovement`, `GroundCombat`, `Repair`, `TransportRepairStep`, `ShieldArmor` |
| Movement and gates, §6.3 | `FleetFuel`, `DesignCapacity`, `GateJump` (permission/cargo rules, hard limits, armor damage and separate vanishing probability) |
| Minefields, §6.4 | `MineTravelDistance`, `MineTraversal`, `FleetMineMinimum`, `MineHit`, `MineLaying`, `MineDecay`, `MineDetonation` |
| Packets, §6.5 | `PacketProduction`, `PacketDecay`, `PacketImpact`, `PacketTerraformChunk`, `PacketTerraformAxis` |
| Equipment and starting state, §7/8 | `EquipmentEligibility`, `EquipmentMount`, `StartingState`, `StartingMineralSeed`, `StartingMinerals`, `StartingEnvironment`, `SecondWorldCandidate`, `StartingFleetCount`, `StartingPartUpgrade`, and `StartingTemplate` catalog |
| Wizard points, §9 | `HabitatSampleAxis`, ordered row/plane/stage integral types, `IntegratedHabitability`, `WizardPricing`, `HabitatWeightPrefix`, `HabitablePlanetEstimate` |

Arithmetic pipelines retain separate integer divisions. RDL `sqrt` uses binary64
and rejects negative/non-finite results. The habitat integral uses eleven sample
indexes even when coordinates repeat, signed residual sums, integer radiation
scaling, and ordered binary64 temperature/gravity scaling. It is deliberately
more expensive than looking up a stored number; use the release executable for
pricing queries.

Construction's source BET bypass path reads an uninitialized local for
terraformers/planetary scanners/defenses. The model requires an explicit
`bypass_decision_made` and `bypass_excess` for that ambiguous BET case; it does not
claim a recovered deterministic value. This is the open decision already called
out in the report. It does not change the game code. Normal equipment costs
retain miniaturization, PRT/CE shifts, then the BET surcharge in source order.

For `ShieldArmor`, `armor_part_dp` is the sum of raw armor-part contributions and
`armor_odd_part_count` is the sum of quantities of parts with odd DP. The latter
preserves halving each armor part before slot-count multiplication. Intrinsic
hull and non-armor-part bonuses are supplied separately and are not halved.
For fleet cloak accumulation, use the source threshold flag from
`FleetCloakContribution` to switch integer accumulation to binary64; fuel never
enters the cargo denominator. Equipment aggregate inputs must come from the
actual fitted designs; catalog eligibility alone does not install a part.

This is a model of the report's race-dependent rules on valid game inputs,
including explicit narrow conversions where described. It is not a serializer,
password implementation, UI state machine, path planner, seeded RNG, random-race
optimizer, entire battle resolver or entire turn generator. Live mineral state,
planet positions, fleet contents, technology, relations and orders must come
from the caller. More than one step may be needed to model an event.

## Regeneration and checks

Preset and catalog facts are generated from the game's YAML; formula modules
are maintained as RDL. PyYAML is the same build dependency used by the repository.
From `beyond/rdl`:

```sh
python3 tools/generate_stars_presets.py
python3 tools/generate_stars_catalog.py
python3 tools/generate_stars_presets.py --check
python3 tools/generate_stars_catalog.py --check
cargo test --offline
cargo test --offline --no-default-features
```

`tools/stars_oracle.cpp` links actual native host objects. It emits RDL validation
requirements for all preset input fields, innate habitat/pricing, sampled
colonies, and equipment eligibility across every PRT with no LRTs and with all
fourteen. Assertions and executables stay in `target/stars-verification/`.
Use a configured Ninja host build; from the repository root:

```sh
make test-host
cargo build --offline --release --manifest-path beyond/rdl/Cargo.toml
python3 beyond/rdl/tools/verify_stars_native.py
```

The native harness also compares four-channel part costs, gate boundary cases
and the known binary64 habitat boundary (1,470,287 innate / 541 points). It runs
8,586 comparisons in the current catalog.

Use `--build PATH` for another configured host build and `--rdl PATH` for another
RDL executable. `--section presets`, `--section colonies` and
`--section equipment` select a subset; `costs`, `gates` and `boundary` are also
accepted. This native comparison complements the
focused Rust scenario tests; it does not claim exhaustive turn-engine parity.
The older `examples/races.rdl` remains an illustrative language demo with
invented point rules, separate from these source-derived definitions.
