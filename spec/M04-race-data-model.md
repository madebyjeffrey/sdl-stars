# M04 - Race data model

Status: source-derived specification; suggested Rust/SQL models are proposals.
Reviewed: 2026-10-23. Source: `e2eae18cff0f990db20415c1c2262aed2022f95b`.


This incorporates the logical race specification prepared on October 20 into
[M04 Race Design Wizard](M04-race-design.md). It describes data required to reproduce
a reusable race design in Rust and PostgreSQL, without binary file layout.
The complete [advantage calculation](M04-race-advantage-points.md) owns pricing rules.

[S06](S06-shared-game-state.md#3-game-players-and-designs) separates these reusable
race attributes from mutable player research, relations, plans and runtime state.
This document remains the canonical race definition.

The canonical formal definitions for race values, habitat constraints, credentials and versioned caches are in
[M04-race-specification.dfy](M04-race-specification.dfy). The prose and worked examples below
explain those definitions and retain their source evidence. See the
[coverage inventory](README.md#dafny-coverage-and-verification) for exact scope,
proof strength, dependency boundaries and verification commands.

## Story-linked requirements

This technical companion supports [M04-US01](M04-race-design.md#m04-us01)
(edit a race), [M04-US02](M04-race-design.md#m04-us02) (price it) and
[M04-US04](M04-race-design.md#m04-us04) (retain a reusable definition).
The direct actor is the storage/calculator developer. Entry requires a race
candidate plus its provenance; current player progress is not a race input.

The workflow is to map identity/attributes/traits/habitat with their units,
validate the appropriate observed or proposed boundary, then price or persist
through the owning modules. Invalid text, inconsistent immunity or an unsafe
habitat is diagnosed at the chosen boundary; C normalization and a proposed
strict model do not have identical rejection behavior. Rust/SQL storage and
cache invalidation remain proposals, not delivered persistence.

| ID / parent story / status | Acceptance criterion | Rule, evidence and coverage |
| --- | --- | --- |
| <a id="m04-data-ac01"></a>M04-DATA-AC01 / US01, US04 / Observed values | Given reusable race inputs and separate running-player progress, when mapping the race, then retain only the specified design fields and preserve their widths/units; names are limited by UTF-8 bytes, not characters. | [Scope](#scope-and-inputs), [identity](#identity-habitat-and-units); `Race`, `FIdentity`, `FShape` in [race model](dafny/M04-race-model.dfy). Encoding/high-bit tests cover selected adapters; a new store still needs round-trip tests. |
| <a id="m04-data-ac02"></a>M04-DATA-AC02 / US01, US02 / Proposed safe boundary | Given an immune axis, validate the complete (-1,-1,-1) variant; given an ordinary axis, require positive width before safe pricing. An ordinary 0…100 range remains distinct from immunity. | `FAxis`, `FPricing`; [habitat](#identity-habitat-and-units). Existing C correction can retain zero width; the strict boundary is not claimed as universal runtime enforcement. |
| <a id="m04-data-ac03"></a>M04-DATA-AC03 / US04 / Proposed | Given a changed race version with identical pricing inputs, when checking a proposed price cache, then metadata alone does not force a different computed price; pricing-input changes follow the declared cache predicate. | `RaceVersion`, `FVersion`, `FSamePriceInputs`, `FRepriceRequired`, `MetadataIndependent`; [proposed representation](#proposed-rust-representation). Formal property, no deployed Rust/SQL cache. |

| ID / status | Interaction or quality requirement and check | Owner / gap |
| --- | --- | --- |
| <a id="m04-data-ir01"></a>M04-DATA-IR01 / Proposed interface | Supply named units/variants and validation outcomes to the wizard/store. The developer API has no focus/viewport; inherit player controls and feedback from [M04-IR01/02](M04-race-design.md#m04-ir01). | M04/M02/S01/S06; typed store/SQL integration remains open. |
| <a id="m04-data-qr01"></a>M04-DATA-QR01 / Observed constraints | Test identity byte limits, signed attribute bounds, reserved bits and storage-to-logical adapters against this schema; do not widen disk fields or turn research choices into attained technology. | Source field map, S01 and race tests; exact persistence stays S01. |
| <a id="m04-data-qr02"></a>M04-DATA-QR02 / Open | For a future store, measure round-trip fidelity and cache hit/invalidation behavior over a declared corpus; latency/memory and database version targets await an implementation decision. | M02/M04; inherit [M04-QR03](M04-race-design.md#m04-qr03), no SQL validation/performance result implied. |

The [evidence/open work](#evidence-and-remaining-work) owns unresolved storage
and credential decisions. Pricing belongs to the pricing companion; player-state
fields belong to S06 and binary layout to S01. The story layer adds no new field
or normalization policy.

Checks for this story expansion are recorded in the
[collection verification](README.md#story-expansion-verification).

## Scope and inputs

A race design is distinct from a player's state in a running game. Current tech
levels, research resources spent, relations, homeworld, fleets, production queue,
AI difficulty and score are player state and are unnecessary for race pricing.
`PLAYER` contains both kinds of state; a new model need not reproduce that coupling.
Names, portrait, credentials and leftover allocation do not affect point balance.
Library race-version identifiers are metadata, not race settings or player slots.
Sources: [`PLAYER`](../src/core/structs.h), `CAdvantagePoints` in
[race.cpp](../src/core/race.cpp), [library ADR](../docs/adr/0001-desktop-managed-libraries.md).

| Data | Pricing input? | Design purpose |
| --- | --- | --- |
| Three ordered habitat axes | Yes | Gravity, temperature, radiation |
| Growth | Yes | Stored wizard setting, including for HE |
| Population resource divisor | Yes | Population economy |
| Factory output, cost and operation limit | Yes | Factory economy |
| Mine output, cost and operation limit | Yes | Mining economy |
| Six research choices | Yes | Research cost settings |
| Primary trait and lesser trait mask | Yes | Racial abilities and pricing |
| Higher starting technology and cheaper factories | Yes | Extra race options |
| Leftover allocation | No | Starting bonus selection |
| Singular/plural names and portrait | No | Identity |
| Password hash | No | Optional game credential interoperability |
| Random-template bit | No direct term | Requests later random race generation |
| Reserved slot and trait bits | No | Preserve source settings where needed |

## Identity, habitat and units

Names are UTF-8, at most 31 bytes each, with no embedded NUL. Count bytes, not
characters; truncation must keep complete code points. Portrait is 0..31.
Source: [Unicode contract](../docs/UNICODE-BOUNDARIES.md), `PLAYER`.

For each axis store signed coordinates `minimum`, `preferred`, `maximum`.
Immunity is exactly `(-1,-1,-1)`. Ordinary coordinates are 0..100 and the
normalized midpoint is `minimum + (maximum-minimum)/2`, truncating.
An ordinary [0,100] interval is not immunity. Positive odd widths are supported.
Require `minimum < maximum` for a safe new calculator; the C bounds corrector
can leave width zero, which causes division by zero. Width one is safe: midpoint
is at minimum, and the zero-width lower half is not selected by an in-range point.
Normal wizard controls aim to keep interval widths at least 20; that is not a
universal import constraint. Source: `BoundsCheckPlayer` in [race.cpp](../src/core/race.cpp),
`PctPlanetDesirability` in [planet.cpp](../src/core/planet.cpp).

Price coordinates directly. Physical display conversions are:

- Temperature: `4*x-200` degrees C.
- Radiation: `x` mR.
- Gravity: `d=abs(x-50)`; `v=100+4*d` if `d<=25`, otherwise
  `v=200+24*(d-25)`. If `x<50`, set `v=10000/v` with integer division.
  Display `v/100` and two-digit `v%100` as g.

Source: `PszCalcEnvVar`, `PszCalcGravity` in [planet.cpp](../src/core/planet.cpp).

## Attribute slots and bounds

The source `rgAttr[16]` is signed i8. Slot numbers are stable identifiers; named
fields in another language can map to them explicitly. Bounds arrays omitted
initializers are zero, including reserved slot 15.

| Slot | Source name | Meaning | Inclusive range |
| --- | --- | --- | --- |
| 0 | rsResGen | Hundreds of colonists per resource; 10 means 1000 people/resource | 7..25 |
| 1 | rsFactProd | Resources per 10 factories | 5..15 |
| 2 | rsFactBuild | Resources to build one factory | 5..25 |
| 3 | rsFactOperate | Factories per 10000 colonists | 5..25 |
| 4 | rsMineProd | kT per 10 mines | 5..25 |
| 5 | rsMineBuild | Resources to build one mine | 2..15 |
| 6 | rsMineOperate | Mines per 10000 colonists | 5..25 |
| 7 | rsUseLeftover | Starting bonus allocation | 0..6; UI offers 0..4 |
| 8 | rsTechBonus1 | Energy research cost | 0..2 |
| 9 | rsTechBonus2 | Weapons research cost | 0..2 |
| 10 | rsTechBonus3 | Propulsion research cost | 0..2 |
| 11 | rsTechBonus4 | Construction research cost | 0..2 |
| 12 | rsTechBonus5 | Electronics research cost | 0..2 |
| 13 | rsTechBonus6 | Biotechnology research cost | 0..2 |
| 14 | rsMajorAdv | Primary trait | 0..9 |
| 15 | unnamed | Reserved | 0 |

Research: 0 = 75% extra cost, 1 = standard, 2 = 50% reduced cost. These are cost
choices, not attained technology levels. Growth is a separate i8 field, 1..20.
Source: `RaceStat` in [enums.h](../src/core/enums.h), bounds and `SetRaceStat`
in [race.cpp](../src/core/race.cpp), `RaceWizardDlg6` in [raceui.cpp](../src/ui/raceui.cpp).

Leftover choices: 0 surface minerals, 1 mineral concentrations, 2 mines,
3 factories, 4 defenses. Values 5/6 take the surface-minerals default. Choice
has no point price. M05 consumes at most 50 remaining points for a human's
starting bonus; see `GenerateWorld` in [create.cpp](../src/core/create.cpp) and
[historical bonus details](../docs/RACE-WIZARD-CALCULATIONS.md#leftover-points-and-related-gameplay-quantities).

## Primary traits

| ID | Source symbol | Display name |
| --- | --- | --- |
| 0 | raCheapCol | Hyper Expansion (HE) |
| 1 | raStealth | Super Stealth (SS) |
| 2 | raAttack | War Monger (WM) |
| 3 | raTerra | Claim Adjuster (CA) |
| 4 | raDefend | Inner Strength (IS) |
| 5 | raMines | Space Demolition (SD) |
| 6 | raMassAccel | Packet Physics (PP) |
| 7 | raStargate | Interstellar Traveller (IT) |
| 8 | raMacintosh | Alternate Reality (AR) |
| 9 | raNone | Jack of All Trades (JOAT) |

`raNone` means JOAT, not an absent primary trait. Source: `RaceAttribute`
in [enums.h](../src/core/enums.h), [wizard labels](../text/strings.txt).

## Trait mask

Use an unsigned 32-bit mask. Each index is a bit number, not a mask value.

| Bit | Source suffix (ibitRace/grbitRace) | Meaning |
| --- | --- | --- |
| 0 | IFE | Improved Fuel Efficiency |
| 1 | TT | Total Terraforming |
| 2 | ARM | Advanced Remote Mining |
| 3 | ISB | Improved Starbases |
| 4 | GeneralizedResearch | Generalized Research |
| 5 | UltimateRecycling | Ultimate Recycling |
| 6 | MineralAlchemy | Mineral Alchemy |
| 7 | NoRamscoops | No Ram Scoop Engines |
| 8 | CheapEngines | Cheap Engines |
| 9 | OBRM | Only Basic Remote Mining |
| 10 | NoAdvScanner | No Advanced Scanners |
| 11 | LowStartingPop | Low Starting Population |
| 12 | BleedingEdgeTech | Bleeding Edge Technology |
| 13 | RegeneratingShields | Regenerating Shields |
| 14..28 | unnamed | Reserved, preserve where needed |
| 29 | Tech3 | Higher starting technology option |
| 30 | AIPlayer | Random design template, distinct from PLAYER.fAi |
| 31 | CheapFact | Factories use 3 rather than 4 germanium |

Use `1u32 << bit`, especially for bit 31. Only 0..13 enter lesser-trait count
penalties. Bit 30 causes M05 to generate a new race, so a template's directly
calculated balance is not the final generated race price. Source: `RaceGrbit`,
`RaceTraitBits` in [enums.h](../src/core/enums.h); `CAdvantagePoints`,
`CreateRandomRace` in [race.cpp](../src/core/race.cpp); `GenerateWorld` in
[create.cpp](../src/core/create.cpp); `RaceWizardDlg1` in [raceui.cpp](../src/ui/raceui.cpp).

## Optional credential interoperability

Saved credential is a signed i32, zero for no password. `LSaltFromSz` processes
UTF-8 bytes as signed i8: start accumulator zero, alternately add one
sign-extended byte and multiply by the next sign-extended byte. Wrap each step
modulo 2^32. Empty input returns zero; nonempty final zero becomes one.
Reinterpret final bits as i32. The UI password buffer allows 15 UTF-8 bytes
plus NUL. This game hash is not suitable as a service login password hash.
Legacy credential replacement policy belongs to M02/S01, not this data model.
Source: `LSaltFromSz` in [utilgen.cpp](../src/core/utilgen.cpp),
`RaceWizardDlg1` in [raceui.cpp](../src/ui/raceui.cpp).

## Proposed Rust representation

This is a suggested logical representation, not a new existing game API.
Constructors must enforce the bounds above; Rust field types alone do not.
Named accessors can expose the indexed attribute array.

The former Rust struct sketch is represented by `Axis`, `Race`, `FShape`,
`FIdentity`, `FPricing` and `FRace` in
[M04-race-model.dfy](dafny/M04-race-model.dfy). A Rust implementation can use
fixed arrays and named accessors, but must enforce these predicates explicitly.

Use i32/i64 working integers with the specified narrowing boundaries, and f64
for the current habitat integral. Preserve operation order; do not substitute
FMA or decimal arithmetic. A useful proposed pricing interface returns a
normalized design, diagnostics, innate habitability and signed point balance.
The source calculator mutates its argument; a pure replacement should expose
that normalization explicitly. Negative balance is valid editor draft state.

## Proposed PostgreSQL schema

Each row is a race version; IDs are application metadata. Use a UTF-8 database.
`octet_length` enforces byte budgets; PostgreSQL text already excludes NUL.
BIGINT stores the entire unsigned trait mask. Drop password_hash if game
credential interoperability is unnecessary. Calculated values are optional,
versioned caches, never independent source settings.

The former SQL column checks are represented by `RaceVersion`, `FVersion`,
`HabitatRow` and `FHabitatRows` in
[M04-race-model.dfy](dafny/M04-race-model.dfy), reusing the same race constraints.
`FHabitatRows` checks all three axes and their common version identity.
Database primary keys and cascading deletion remain implementation mechanisms;
they must preserve that identity and child-row contract. `FRepriceRequired`
expresses cache invalidation by calculator version and pricing inputs.

The transaction must create exactly three habitat rows; ordinary row CHECKs
cannot enforce child count. The schema permits negative balances for drafts.
It excludes malformed and zero-width inputs rather than reproducing undefined
C behavior. Invalidate cached prices on any pricing input change and bind the
calculator version to a source revision. Calculate in Rust; SQL aggregate order,
casts and generic rounding do not guarantee C arithmetic equivalence.

## Evidence and remaining work

The [calculation acceptance criteria](M04-race-advantage-points.md#acceptance-and-evidence)
include a complete golden vector. The Rust/SQL mapping notes are design guidance;
no SQL migration or ported calculator is implemented by this specification.
The full wizard workflow and cancellation guarantees remain M04 work, as
listed in [its module entry](M04-race-design.md#evidence-and-open-questions).
