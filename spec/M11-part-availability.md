# M11 — Native part availability and technology status

Status: observed behavior reviewed 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the working specification collection.
This companion specializes [M11-US01](M11-ship-and-starbase-design.md#m11-us01)
and [M11-US06](M11-ship-and-starbase-design.md#m11-us06), enabling the equipment
browser, [design/build checks](M11-design-buildability.md) and
[M12 research announcements](M12-host-research-resolution.md).
M11 retains the stories and criteria. S03 owns generated identities/catalog rows;
M04 supplies race attributes/traits; M12 supplies current technology; S06 owns
native player/part/global lifetimes. UI presentation, selection, focus, keyboard,
accessibility and display qualification remain with M11/M12/M15/S05. Timing and
memory targets remain Open under those entries' quality requirements.

Canonical definitions: [native availability](dafny/M11-native-availability.dfy),
[versioned named identities](dafny/S03-lookup-identities.dfy),
[shared masks/extents](dafny/S03-research-catalog.dfy) and
[research lookup adapter](dafny/M12-equipment-lookup.dfy).
`TechProjection` connects the native numeric status to
[M11's existing semantic status](dafny/M11-design.dfy).
[Collection evidence](README.md#native-part-availability-continuation) records
actual runs and their scope.

Sources: [parts.c](../src/core/parts.c) `FLookupPart`, `TechStatus`;
[research.c](../src/core/research.c) `FShouldPartBeHidden`;
[race.c](../src/core/race.c) `GetRaceStat`, `GetRaceGrbit`;
[structs.h](../src/core/structs.h) `PLAYER`, `PART`, `HS` and catalog structures;
[enums.h](../src/core/enums.h) plus fresh generated `dataids.h`/data tables.
These contracts introduce no runtime or import-policy changes.

## Interface, storage and caller domain

`Context` projects a selected player's signed-byte primary/technology levels,
uint32 race flags, uint16 trader grants and unsigned four-bit current field.
Headless means global `idPlayer==iplrNone`: the lookup selects primary `raNone`
and skips race/player access. Other global indices require a live PLAYER within
its sixteen-entry storage. The model receives that already selected context;
it does not add a native global-index validator.

`PART.hs` supplies a uint16 slot mask, eight-bit item and eight-bit quantity.
`Kind` recognizes exactly one of the sixteen native masks; zero, combined and
other masks are Invalid for this part-lookup interface. This does not impose a
single-bit rule on a hull slot's allowed-category mask; slot compatibility has
its own M11 contract. Quantity does not affect these helpers. The caller
supplies a live writable PART and live generated catalog/player storage. Invalid
native addresses or lifetimes have no fabricated error/recovery result here.

`PartRow` contains six signed-byte requirements and a signed-word planetary
ability. `CatalogShape` requires all sixteen current extents and six requirements
per row. `LookupRow` selects that row before invoking the canonical native
`Lookup` decision. `Address.Row(kind,item)` denotes the corresponding native
common-prefix catalog pointer; `Prior(token)` retains the caller's prior pointer
without dereferencing it. These logical addresses do not change saved layouts
or place native pointers in a file.

`Result` returns numeric status, pointer effect and ordered helper-call tags:
0 is the primary getter, 1 the hidden-part helper, 2 technology status, and
100+bit the corresponding race-bit getter. They describe calls, rather than a
trace of every C memory read. Native getter bodies are included in the fixture.

## Lookup workflow and pointer effects

1. Unless headless, query the selected player's primary attribute before checking
   the mask or item. Source assigns that signed getter result to its unsigned
   RaceAttribute; comparisons against the defined 0..9 primary values retain
   the same result for every signed-byte input.
2. Resolve the exact category and test its byte item against the current extent.
   Invalid returns zero and leaves the prior union pointer intact. It performs
   no trait, hidden-part or technology check after that boundary.
3. A valid item assigns its typed catalog pointer before any denial. Apply the
   category's ordered trait/hidden gates. Denial returns Restricted (-1) with
   that assigned pointer; it does not query technology afterward.
4. An allowed headless lookup returns Available (1). An allowed player lookup
   queries `TechStatus` using that row's requirements and selected player state.

Most categories bypass policy checks when headless. **Shield, armor and beam
primary restrictions still run**, with `raNone` as primary; therefore a valid
headless item can be Restricted. Their remaining hidden-helper calls return
false in headless context. The source does not normalize this into unconditional
headless availability. There is no cancellation, allocation or retry in these
helpers; caller workflows own subsequent failure handling and presentation.

`Lookup` proves Invalid exactly for an unrecognized mask or out-of-range item,
retaining Prior in that case and assigning Row for every other status. This
uses the proved nonzero technology-status range, not an assumed terminator.

## Category policy inventory

`Policy` preserves the source's short-circuit ordering. The named S03 constants
retain source spellings and are bound to fresh enums by the native fixture.
This table locates the rules; the checked definition is authoritative.

| Category / ascending bit | Gates and source ordering accounted for by Policy |
| --- | --- |
| Engine / 0 | Settler primary restriction; scoop/NoRamscoops; Galaxy/Fuel Mizer IFE; Interspace/NoRamscoops; trader hiding. |
| Scanner / 1 | Advanced-scanner NAS query; stealth-only scanners. There is no hidden-helper call in this branch. |
| Shield / 2 | Shadow/Stealth and Croby/Defend; trader hiding. Primary checks also apply headless. |
| Armor / 3 | Depleted Neutronium/Stealth and Fielded Kelarium/Defend; trader hiding. Primary checks also apply headless. |
| Beam / 4 | Mini Gun/Defend and Blunderbuss/Gatling/Attack; trader hiding. Primary checks also apply headless. |
| Torpedo / 5 | Trader hiding for player context. |
| Bomb / 6 | Defend smart-bomb exclusion; Retro/Terra restriction; trader hiding. |
| Mining / 7 | OBRM mining set; ARM small/ultra set; Orbital Adjuster/Terra; trader hiding. |
| Mine layer / 8 | Mines-only set, Speed Trap 20 Mines/Defend, Mine Dispenser 50 Attack exclusion. |
| Base special / 9 | Mass-driver exceptions and MassAccel; stargate special ranges and CheapCol exclusion. |
| Base hull / 10 | ISB for dock/ultra; Death Star/Macintosh restriction. |
| Electrical special / 11 | Trader hiding first, then individual Stealth/Mines/Stargate/CheapCol/Defend gates. |
| Mechanical special / 12 | Trader hiding first; Macintosh colonization exclusion and orbital-construction restriction. |
| Terraforming / 13 | TT query for the Total Terraform range; remaining items pass policy. |
| Ship hull / 14 | CheapCol/Defend hull sets; OBRM and ARM mining sets in order; Attack/Stealth/Mines hull sets; trader hiding. |
| Planetary / 15 | Negative-ability scanner NAS query; Macintosh scanner/defense and Attack defense exclusions; trader hiding. |

`TraderFlag` maps the twelve source part/category pairs to their cargo, electrical,
shield, armor, mining, bomb, torpedo, beam, hull, engine, Genesis or jumpgate grant.
`Hidden` returns true only for a matching pair whose grant bit is absent in
player context. Unrelated/high grant bits do not authorize that pair. Lifeboat
has no additional hidden-part pair in this function. `HiddenMask` retains the
standalone helper's false result for unrecognized masks; it does not validate
catalog item bounds or dereference the part's union pointer.

## Technology status and numeric encoding

`TechStatus` compares all six signed requirement/current-level bytes. It returns:

- 1 when no field is missing;
- 2 when the sole missing field is current and exactly one level away;
- deficit plus one when that sole current field is further away;
- 99 for the remaining missing-field combinations.

Current fields 6..15 cannot identify a missing field and therefore use 99 when
anything is missing. Raw signed-byte inputs give **1..256**, so this helper never
returns Invalid zero or Restricted -1. A raw deficit of 98 also encodes as 99;
that numeric encoding does not distinguish the semantic Later case from
OtherTech. `Code`/`TechProjection` prove agreement with the existing natural-level
M11 model on its domain without claiming an injective status encoding. The raw
vectors are width tests, not approved game levels or demonstrated catalog states.

## Research scan handoff

[M12EquipmentLookup](dafny/M12-equipment-lookup.dfy) uses `LookupRow` for successive
items and masks. `AppendTrace` preserves an existing reply prefix, derives each
status and requirement from the context/row, and proves `Complete` for the appended
segment. `MaskKindAt` connects the shared masks to their category indices.
`ScanCatalog` then invokes the canonical announcement `Scan` and proves Done with
exactly **255** consumed replies. It adds no independent announcement algorithm.
The trace's pointer bit certifies a readable row for Available replies. Invalid's
unused requirement/pointer metadata is normalized to zero/false; Scan never reads
it. This does not clear or describe the native prior pointer: the separate Lookup
result retains that exact logical pointer effect. Actual trace extraction may
retain different ignored metadata and must preserve all fields Scan does read.

This closes the model's need to assume an arbitrary supplied Complete trace
when using the shaped catalog adapter. Actual host integration must still map
its current global player, technology, all race flags/trader grants, row addresses
and helper frames into that context. The host transition machine retains its
explicit trace interface and failure-prefix semantics; its complete native-domain
Done theorem remains open. Browser/design callers have their own context and
knowledge/restoration obligations.

## Evidence and remaining qualification

[Source fixture](../tests/spec/check_part_availability_source.py) compiles actual
lookup, technology, hidden-part and race getter bodies with native common.h
records and freshly generated catalog tables. An independent typed-array export
supplies all 239 rows' requirements and planetary abilities to the reference.
Every one of the 116 named model IDs, the sixteen masks/extents and native
primary/trait/grant protocols is bound to fresh native enums or exported shape.

Cases exercise all normal primary/relevant-trait combinations, raw primary bytes,
all 65536 slot masks in player/headless contexts, all byte item IDs at each
category, all 65536 trader words for each special pair, signed technology-byte
pairs, missing-field/current combinations and seeded raw contexts. Exact statuses
and getter order are compared. Native checks compare PART including preserved
HS/quantity/padding and expected direct-array pointer, every PLAYER/GAME byte,
global identity and the supplied technology-requirement bytes. They preserve
observed headless restrictions and denied-pointer assignment.

The fixture is finite evidence at the compiled platform/char conventions, not a
universal C memory, strict-alias, lifetime or concurrency refinement. The model
makes catalog/context inputs explicit; source pointers, full caller construction,
actual message/heap frames, full caller integration beyond the now specified
ordinary build-check restoration, enemy knowledge, populated
design workflows, real UI/platform qualification and agreed performance/accessibility
targets retain their existing owners. Source/model completeness here covers the
three named decision helpers, not every design or research workflow. No game
source, catalog identity/layout, saved format or baseline changes are introduced.
