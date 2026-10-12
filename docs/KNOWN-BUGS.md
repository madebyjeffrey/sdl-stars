# Known original Stars! bugs and features

This maps the user-supplied release bug list to the current reconstructed
sources. Locations use filenames and original function names, not line numbers.
The bugs planned for 2.8, and their status, are tracked in
[ROADMAP.md](ROADMAP.md), step 5. The balance items (chaff, split fleet dodge,
battle board overload, 0.2% minimum damage, profitable scrapping, mine damage
allocation) are kept here for reference only; 2.8 doesn't change them. False
public scores, once listed with them, is fixed in 2.8.

**Located** means the relevant mechanism is visible in the source; it does not
mean the complete player-reported scenario has been reproduced in a test.
**Candidate** means the processing path is identified but the exact cause or
trigger remains unconfirmed. **Different here** identifies a material difference
from the supplied description, including existing native repairs. Historical
J/JRC3/JRC4 fix claims below come from the supplied list and are not independently
verified release history. See [WIN16-PARITY.md](WIN16-PARITY.md) for native changes
and [ROADMAP.md](ROADMAP.md) for the post-tag work.

## Native heap alignment and byte-addressed storage — fixed on main

`memory.c`: ordinary allocation payloads now satisfy `max_align_t`, including
free-list reuse, reset and realloc growth. The battle heap keeps its original
two-byte boundaries, which its record traversal requires. Allocation rounding
checks capacity before narrowing to the heap's 16-bit counters.
`WriteFleet`, `DoCyberPackets` and `FEnumPktAttack` now access unaligned record
and AI-workspace scalars through the existing byte-safe helpers. Record widths
and serialized layouts are unchanged.

## Host sentinel reads and empty records — fixed on main

`SetVisPFInit` and `FLookupPart` no longer index `rgplr` with the all-player
sentinel (-1). Host part lookup has no player race or technology restrictions.
`WriteRt` skips copying empty payloads, avoiding a null argument to `memmove`
when writing an empty EOF record. `PtToScan` replaces negative signed shifts
with multiplication, preserving scanner scaling and rounding.

## Habitat-page stack corruption — fixed on main

`raceui.c`: `DrawRace2` copied `idsCanExpect1DPlanetsWillHabitable`
(a 67-character template) into `szT[32]` before formatting it. Painting a
non-immune race’s habitat page overwrote the stack; Linux stack protection
reliably aborts this path. The function now formats directly from the text
table. `test_RaceWizardDlg2_paints_habitat` covers real dialog painting on
Windows and SDL. No save layout or turn-generation behavior changes.

## Technology Browser close lifecycle — fixed on main

`researchui.c`: `BrowserDlg` called `EndDialog` for a window made by
`CreateDialog`. The SDL wrapper rejected this modal-only operation, leaving
the browser open after its globals had been cleared. The Close handler now
uses `DestroyWindow`, as the browser's existing menu-toggle handler does.
The client integration test opens, closes by button, reopens and closes through
an SDL window-close event, checking handle destruction and owner availability.
Turn generation and saved data are unchanged.

## Continuing a game with native paths — fixed on main

`mdi.c`: `FOpenGame` dereferenced `strrchr(szBase, 92)` without checking it.
Linux paths and relative Windows filenames may contain no backslash, causing
a crash before game loading. It now checks the platform separator before
truncating the directory and uses that separator when restoring `szDirName`.
The client Continue Game test loads copied frozen fixtures using relative and
absolute Linux paths. File layouts and turn generation are unchanged.

## Saved panel expansion flags — fixed on main

`init.c`: `ReadIniTileSettings` stored `isupper` directly in the one-bit
`TILE.fPopped` field. The Linux C library returns a mask whose low bit is zero,
so saved uppercase (expanded) sections became collapsed. The parser now
normalizes classification to a boolean, retaining field widths and the
existing uppercase/lowercase settings format. Continue Game tests restore
expanded planet/fleet settings and assert the resulting flags.

## Player-exploitable bugs / features


### Chaff — located

`battle.c`: `FAttack`, `CTorpHit`, `FDamageTok`, `CheckTarget`.
`FAttack` scores targets using resource plus boranium value relative to remaining
defenses, subject to the battle plan's target classes. Cheap armed hulls can
therefore attract missile fire. `FDamageTok` limits kills to the supplied torpedo
count (`cKillMax`) and discards remaining damage once that count is exhausted.
Beam attacks pass no torpedo kill limit; gatling beams attack each eligible token
in range. This is the mechanic behind chaff, rather than a separate chaff rule.

### Split Fleet Dodge — located; exact retarget choice remains a candidate

`ship.c`: `FleetOrdersChangeTarget`, `Merge2Fleets`; `turn.c`: `MoveFleets`;
`battle.c`: `DoBattles`, `CplrBattle`.
Pursuit orders name one fleet ID, and combat operates on fleets at the same
location. Splitting and giving different destinations prevents one pursuer from
following every resulting fleet. `FleetOrdersChangeTarget` replaces references
to a removed fleet using a nearby object. The supplied claim that the largest
mass always becomes the target, and the JRC3/JRC4 changes, are not established
by this mapping.

**Retargeting across releases:** `ValidateWaypoints` picks a new target when a
pursued fleet is gone or changed, among fleets of the same owner where it was
last seen. 2.6j RC3 ("fleets that split up ... will all be chased down", in
[26JFIN.txt](26JFIN.txt)) first tries only fleets no other pursuer has taken,
spreading pursuers over the pieces. The jrc4 binary (`0008:497a`) dropped
that pass again: every pursuer takes the heaviest matching fleet. 2.8
follows jrc4 (`tests/unit/test_util.c`).

### Profitable Scrapping — different here

`turn3.c`: `SatisfyOrders` (give and scrap tasks); `util.c`: `UpdateShdefCost`;
`ship.c`: `GetTruePartCost`.
Transferred designs are marked `fGift`; recycling uses the scrapping player's
design costs and the normal location/Ultimate Recycling recovery multipliers.
However, this source divides gifted mineral and resource values by **four**
before recovery, rather than applying the supplied list's 30% cost reduction.
Race-dependent cost calculation is located, but profitable combinations under
this source's gift penalty have not been demonstrated.

### Battle Board Overload — located; capacity wording differs

`battle.c`: `DoBattles`, `CplrBattle`, `InitializeBoard`, `FDoCoolBattle`.
The board allocates 256 `TOK` entries. Admission in `CplrBattle` starts pruning
above **255** design tokens, assigns `255 / cplr` quotas, and then fills remaining
capacity. It admits or excludes whole fleets, marking excluded fleets
`fInclude = FALSE`, `fBombed`, and `fSkipped`. Cheap fleets encountered earlier
can consume the quota and leave valuable fleets outside the battle. The precise
fleet-number ordering claim needs a scenario test; the pruning loop itself
walks the location's fleet list.

### 0.2% Minimum Damage — located; denominator differs

`battle.c`: `FAttack`, `FDamageTok`; `structs.h`: `DV`.
Each weapon slot fires separately. Nonzero residual armor damage is converted
to per-ship damage and rounded upward to `pctDp`, with a minimum of one unit.
The calculation uses **500**, not 512, as the full-damage denominator: one unit
is exactly 0.2%. Many small salvos can accumulate disproportionately large
damage on a large token. Shield damage uses a separate calculation, and the
torpedo kill cap still applies. The supplied report describes this as an
architectural feature explicitly retained by the original developers.

### False Public Player Scores — fixed in 2.8

`turn.c`: `FGenerateTurn`, `DoOrders`; `turn3.c`: `SatisfyOrders`;
`turn2.c`: `Produce`, `UpdatePlayerScores`; `util.c`: `CalcPlayerScore`.
Production computes resources from planetary population during production;
the later score calculation calls `CResourcesAtPlanet` again on the final
planet state rather than using the production total. WP1 loading can therefore
lower the published resource score after resources were already produced;
WP0 unloading next year restores population before production.

**2.8:** confirmed by `tests/unit/test_turn2.c`: two freighters loading
50 kT of colonists at a homeworld after production cut the published
resources from 38 to 33. `Produce` now records each planet's population
and owner when it finishes, and `CalcPlayerScore` scores a planet with
the same owner at that population, for both its population points and its
resources. Planets that changed hands after production are scored as
before.

### North/South Minefield Immunity — fixed in 2.8

`utilgen.c`: `FIntersectCircleLine`; `turn.c`: `FTravelThroughMineFields`.
The intersection projection solves for `xI`, then derives `yI` by dividing by
`dx`. For a vertical route it instead sets `yI = ptL1.y`, so the perpendicular
projection collapses to the starting point. This can reject fields crossed
farther along a north/south route. The source supports a vertical intersection
bug, but does not justify a universal immunity when the starting point is
already inside a field. The list reports a JRC4 fix.

**2.8:** confirmed by `tests/unit/test_utilgen.c` (a vertical route through
a field's center found no intersection) and fixed: a vertical route's
closest point is level with the field's center (`yI = ptC.y`).

### East/West Speed Bump Minefield Immunity — not reproduced in 2.8

`turn.c`: `FTravelThroughMineFields`; `utilgen.c`: `FIntersectCircleLine`.
These contain route intersection, per-field-type safe speeds, hit rolls, and
movement truncation after a hit. The intersection code has a horizontal-route
calculation, and there is no obvious blanket horizontal speed-bump exemption.
The reported east/west-only defect is not yet isolated. The list reports a
JRC4 fix; do not equate this report with the vertical projection defect above.

**2.8:** `tests/unit/test_turn.c` sends a warp 9 fleet 81 ly east, west,
north and south through another player's speed bump field; it is stopped
in every direction. Whatever the report saw may have been the vertical
projection defect, fixed above, or something since changed.

### SS Pop Steal — fixed in 2.8

`turn3.c`: `SatisfyOrders`; `util.c`: `GetFleetScannerRange`;
`ship.c`: `ChgCargo`; `shipui.c`: `TransferStuff`.
Waypoint transport obtains theft permission from the scanner and sets
`fStealing`. In its load path, the colonist/fuel check merely sets `fDone = TRUE`
and then continues into cargo removal and loading; it does not reject the
colonist transfer from an enemy planet. This differs from the intended manual
transfer restrictions. The list reports a JRC4 fix and a caveat about zip orders
that include population loading.

**2.8:** confirmed by `tests/unit/test_turn3.c`: a freighter with a Robber
Baron scanner loaded 10 kT of colonists from an enemy homeworld by waypoint
order. The binary's string table has "attempted to shanghai colonists ...
The attempt was unsuccessful" and the matching fuel message, but never sends
them. Loading colonists or fuel while stealing now sends those messages and
loads nothing; stealing minerals is unchanged.

### [freepop] Hack — candidate; host validation is present

`ship.c`: `FEnumCalcJettison`, `ChgCargo`; `shipui.c`: `TransferStuff`;
`log.c`: `LogMakeValidXfer`, `FRunLogRecord` (cargo-transfer records);
`turn2.c`: `FQueueColonistDrop`, `DropColonists`.
Manual population on an uninhabited world is represented through pending drops
and transfer logs, rather than ordinary owned planetary population. This is the
path a forged client-side drop amount would attack. However, host log replay
tracks and consumes pending `COLDROP` amounts, and `ChgCargo` clamps withdrawals
against available cargo. A missing validation that reproduces the supplied
memory-editor exploit has not been identified. The report itself says the
exploit no longer appears to work in JRC4.

### Cheap Starbase — fixed in 2.8

`buildui.c`: `FCheckQueuedShip`, `SlotDlg`; `ship.c`: `CshQueued`,
`RemoveIshdefFromAllQueues`; `log.c`: `FRunLogRecord` (design changes);
`turn2.c`: `CBuildProdItem`; `produce.c`: `GetProductionCosts`.
Partial production stores a completion fraction (`PROD.pct`), and the next
build calculation applies it to the current design's cost. Queue-removal and
queued-design checks in `ship.c` require an existing starbase on the planet,
leaving the first base's partial work vulnerable to design edits. Host design
replacement checks existing design counts, not every partial queue investment.
This supports retaining cheap partial work when the design becomes expensive.
The delete-and-recreate UltraStation variant depends on builder-close timing
and remains unconfirmed.

**2.8:** confirmed by `tests/unit/test_log.c`: a starbase design half built
at a colony kept its 50% when the design was replaced. The host's replay
of a design change or deletion (`FRunLogRecord`) now clears the progress
of every queued item of that design on the player's planets, which also
covers the delete-and-recreate variant.

### Mineral Upload — fixed in 2.8

`ship.c`: `ChgCargo`; `shipui.c`: `TransferStuff`; `log.c`: `FRunLogRecord`;
`turn2.c`: `TransferToOthers`.
When the host replays a transfer to another player's fleet or planet,
`FRunLogRecord` removes the cargo from the source in its first pass and
delivers it in the second (its `StealCargo` path) through `ChgCargo`, which
caps receipt at the destination's free hold space. The undelivered
remainder is reported ("unable to transfer") but not refunded, so sending
minerals to a foreign fleet without room for them destroys them.

The deferred transfer list that `TransferToOthers` delivers (with its "the
remainder was lost in space" messages) is never filled: `FRunLogRecord`
only reaches the code that adds to it for a zero quantity, which it skips.
Transfers to other players are delivered during replay instead.

**2.8:** confirmed by `tests/unit/test_log.c`: 100 kT of ironium sent from
a homeworld to an AI ship with a 10 kT hold left the ship with 10 kT and
the planet 100 kT poorer. What doesn't fit now goes back to the source,
as the "unable to transfer" message says.

### Target List Overload — fixed in 2.8

`scan.c`: `ScannerWndProc`; `shipui.c`: `ClickInShipOrders`.
Both target-popup paths use `rgid[100]` and stop collecting entries at 100.
Objects beyond the popup limit cannot be chosen through those menus.
`scan.c`'s `FGetNextObjHere` provides a separate selection traversal, explaining
why viewing another fleet need not imply it can be selected as a popup target.

**2.8:** both popups list every fleet and thing at the location, and
`PopupMenu` accepts a choice past the hundredth item. This is client UI,
so there is no unit test; check it by right-clicking a location with more
than 100 fleets.

### Space Dock Armor Slot Buffer Overflow — fixed in 2.8

`util.c`: `UpdateShdefCost`; `parts.c`: armor and Space Dock definitions.
Armor-slot strength is first assigned to signed 16-bit `dpT`. With RS,
`dpT >>= 1` halves that already-truncated signed value before adding it to the
hull's armor. Superlatanium has enough strength for a large slot to overflow
that intermediate; the negative half then wraps when accumulated into hull
armor. The frozen `UpdateShdefCost` assembly confirms a signed `SAR` instruction.
This is integer overflow rather than an identified overwrite of a buffer.
The reported Space Dock/ISB combination supplies the large armor slot; ISB is
not an explicit condition in this arithmetic.

**2.8:** confirmed by `tests/unit/test_util.c`: an RS Space Dock with 24
Superlatanium had 51018 dp instead of 18250. `dpT` is now 32-bit.

### ISB Trumps IT Gate Scanning — fixed in 2.8

`save.c`: `SetVisPFPlanets`; `planet.c`: `StargateRangeFromLppl`;
`turn.c`: `FGenerateTurn` (starbase `lVisible` calculation).
IT gate scanning applies starbase visibility to squared gate range with
`(lRadius2 * lVis2) / 10000`. The multiplication uses a 32-bit intermediate
and can overflow before division for the reported 600/800ly ranges. ISB
cloaking makes `lVis2 < 10000`, activating this test. Unlimited-range gates
take the bypass path. Exact observed ranges still merit a scenario test, but
the range/visibility arithmetic explains why gate types behave differently.

**2.8:** the same product appears in all four cloaked-starbase checks
(`SetVisPFFleets`, two in `SetVisPFPlanets`, `SetVisPFThings`), so any
scanner range above about 463 ly could fail against a cloaked starbase, not
only gates. `tests/unit/test_save.c` showed a 600 ly penetrating scanner
100 ly away only obscuring a starbase with `lVisible` 9000 while seeing one
with 2500. The product is now 64-bit.

### Starbase Friendly Fire — fixed in 2.8 by the native repair

`battle.c`: `CplrBattle`; [WIN16-PARITY.md](WIN16-PARITY.md),
“Starbase attack mask index”.
In Win16, the starbase's attack-everyone and attack-one-player cases indexed
`rggrfAttack` with uninitialized `iplrCur`, allowing another player's mask or
memory beyond the array to be modified. The current `NATIVE` repair uses
`iplrStarbase`. Attack-mask propagation and reciprocal hostility remain in
`CplrBattle`, but the supplied precise highest/lowest-player friendly-fire
scenario has not been reproduced under the repaired code.

**2.8:** `tests/unit/test_battle.c` gives a starbase's default plan one
target player, with a friend of the owner and the target in orbit, for all
six ways to cast three players in those roles. The starbase attacks only
its target, and nobody attacks the friend or is attacked by it.

### Repair After Gating Loophole — fixed in 2.8

`ship2.c`: `FStargateJump`; `turn3.c`: `SatisfyOrders` (merge task);
`ship.c`: `Merge2Fleets`; `turn2.c`: `HealShips`; `turn.c`: `FGenerateTurn`.
Gating sets the traveling fleet's `fNoHeal`. WP1 merging precedes healing.
`Merge2Fleets` combines ships and damage into the destination fleet without
propagating the deleted source fleet's `fNoHeal`. `HealShips` tests the surviving
fleet's flag, so a stationary destination can repair the newly merged ships.

**2.8:** confirmed by `tests/unit/test_ship.c`: a damaged fleet marked
`fNoHeal` merged into another healed fully in `HealShips`. `Merge2Fleets`
now marks the surviving fleet `fNoHeal` when the merged fleet was.

### Mine Damage Dodge / Mine Damage Allocation — located

`turn.c`: `FTravelThroughMineFields`.
For fewer than five ships, the function calculates extra damage to meet the
minimum mine-hit damage. It walks design slots in ascending order and adds all
`dmgExtra` to the first eligible design, then clears it. Two single-engine
ships therefore place four shares on the first design and one on the second,
before shield reduction. Excess damage on a destroyed cheap design is not
redistributed; a tough first design can instead absorb it. Engine counts and
shield reduction affect actual results.

### Exploding Minefield Dodge — fixed in 2.8

`turn2.c`: `ThingDecay`; `turn.c`: `FTravelThroughMineFields`.
`ThingDecay` clears fleet `fBombed` once, then processes detonating fields.
After calling the damage function it sets `fBombed` even if no damage occurred,
preventing later fields from hitting that fleet. The damage function excludes
the owner's Mini Mine Layer and Super Mine Layer hulls from its own explosions.
Consequently an immune encounter can consume the fleet's one explosion check.
The exact player-number ordering depends on the `lpThings` traversal order.

**2.8:** confirmed by `tests/unit/test_turn2.c`: a mine layer inside its
owner's detonating field and another player's took no damage. A fleet
immune to a detonation is no longer marked `fBombed`, so the next
detonating field still checks it. The same test found that any detonation
over a fleet crashed the native build (a NULL travel-distance read in
`FTravelThroughMineFields`), fixed separately.

### Colonization Module Check — fixed in 2.8

`turn3.c`: `SatisfyOrders` (colonize task); `ship2.c`: `FColonizer`;
`buildui.c`: `IDropPart`, `SlotDlg`.
The host colonization check tests a slot's `grhst` and `iItem` against the two
colonization modules without checking `cItem > 0`. An empty slot retaining its
module identity can therefore qualify. `FColonizer` is a separate hull-based
classification and is not proof that the host requires an installed module.

**2.8:** confirmed by `tests/unit/test_turn3.c`: a copy of the colony ship
with its module count set to 0 still colonized. The check now requires
`cItem > 0`.

## Coding bugs

### Race File Corruption — fixed in 2.8

`raceui.c`: `RaceWizardDlg1`, `FSaveRace`; `race.c`: `IRaceChecksum`,
`FWasRaceFile` (race load check); `save.c`: `WriteRtPlr`; `file.c`: `ReadRtPlr`.
`IRaceChecksum` XORs the whole in-memory name buffers, including bytes
after the terminator. `RaceWizardDlg1`'s OK path reads the names with
`GetDlgItemText` without clearing the buffers (its radio-button path does
clear them), so a name shorter than the one it replaces leaves stale bytes
that are checksummed but not saved. `ReadRtPlr` zeroes the record on load,
and the checksum no longer matches. Only a save where the old name was
longer at the same position is affected. TotalHost rejects these files and
can rewrite their checksum; see [TOTALHOST.md](TOTALHOST.md).

**2.8:** confirmed by `tests/unit/test_race.c`: a race saved after "Bos"
was written over "Longnames" failed `FWasRaceFile`. `IRaceChecksum` now
checksums a copy with the names zeroed after their terminators, as
`ReadRtPlr` restores them. Files with a bad checksum still fail to load.

### Random Race — fixed in 2.8

`raceui.c`: `RaceWizardDlg1`, `RaceCreationWizard`; `create.c`: `GenerateWorld`,
`InitNewGamePlr`; `save.c`: `WriteRtPlr`.
The wizard selects predefined race templates, including Random, and stores
the resulting player data. `GenerateWorld` calls `CreateRandomRace` when
`ibitRaceAIPlayer` is set. Template/flag persistence is the relevant path, but
the reported failure to clear the random setting after deselection has not
been demonstrated. The supplied report also notes a failed reproduction.

**2.8:** confirmed by `tests/unit/test_race.c`. The Random template is
only `ibitRaceAIPlayer`. Leaving the wizard's first page copies the chosen
template into the race, but with Custom chosen it keeps the race being
edited, so a race that had been Random kept the bit through every later
page and was still replaced at game creation. Custom now clears the bit.

### 32k Ship Limit Per Fleet — fixed in 2.8

`structs.h`: `FLEET.rgcsh` (`int16_t[16]`); `ship.c`: `Merge2Fleets`,
`FleetTransferCargoBalance`; `ship2ui.c`: `MergeFleetsDlg`;
`turn3.c`: `SatisfyOrders` (merge task); `log.c`: `FRunLogRecord` (merge records).
`Merge2Fleets` adds ship counts directly into signed 16-bit elements without
a 32767 limit check. Counts crossing that boundary become negative, while many
generation loops only process positive counts. This locates the overflow;
the distinct manual-versus-waypoint recovery/loss outcomes require reproduction.
Other stated limits are represented by the 512-fleet checks, 16 ship-design
and 10 base-design slots, and battle token admission described above; the
complete 512-minefield allocation limit has not been audited here.

**2.8:** confirmed by `tests/unit/test_ship.c`. A waypoint merge of 32000
and 1000 ships of a design left -32536, and merging all fleets at a
location (`FFleetMergeAll`, the client's Merge and its log replay) clamped
the count to 32766 and lost the rest. `Merge2Fleets` now moves only the
ships that fit and leaves the others in their fleet; `FFleetMergeAll`
leaves out a fleet whose ships wouldn't fit. Moving ships between two
fleets in the client's transfer dialog was not changed.

### Race File Name Overrun — fixed in 2.9

`race.c`: `FWasRaceFile`; `create.c`: `GenNewGameFromFile`.
`FWasRaceFile` copies the race file's whole path into `szRaceFile`, a
16-byte buffer the race dialogs fill with a file name. Creating a universe
from a definition file (`-a`) names each race by path, so the copy runs
over the globals after it (`szPassLast`, the `-p` password, and the message
buffer in the 2.8 build). Nothing read them before they were set again, so
results did not change; a hardened `strcpy` (macOS) stops the program.

**2.9:** found running `stars-host -a` on macOS. `FWasRaceFile` keeps only
the file name, cut to fit at a complete UTF-8 boundary, as the race dialogs
store it. UI storage retains the full native suggestion after direct race loads.

### AR Starter Colonies — candidate

`turn3.c`: `SatisfyOrders` (AR colonization/base setup); `turn2.c`: `Produce`,
`CBuildProdItem`, `FBuildObject`; `produce.c`: `FinishProduction`,
`GetProductionCosts`; `produceui.c`: `ChangeProduction`.
These create and process the initial base and production queue, and implement
queue clearing. `Produce` distinguishes an absent queue from a nonempty one
and calculates research allocation while walking production. A completed
starter-base entry blocking surplus research is the reported failure to trace;
the exact persistent queue entry has not been confirmed.

### Failing to Close to Range 2 with Sappers and R2 Beams — candidate

`battle.c`: `DxyMoveTokTo`, `DzMoveRangeToConsider`, `ScoreGuessBattleDamage`,
`ScoreFromGiveAndTakeAndTactic`, `DpFromPtokBrcToBrc`.
Movement scores candidate positions using estimated outgoing/incoming damage.
`DpFromPtokBrcToBrc` caps sapper damage at the target's remaining shields and
combines it with other weapons' estimates. This is the relevant interaction
for a range-3 sapper score to flatten before range-2 beams become attractive.
The exact threshold and tie handling responsible for the reported refusal
to close have not been demonstrated.

### Font Problems on Non-English Windows — located configuration path

`init.c`: `ReadIniSettings`, `FCreateFonts`; `reportui.c`: `DrawScoreReport`;
`utilgenui.c`: `DiaganolTextOut`.
Font selection comes from INI settings and Windows font creation; score labels
use the rotated-text rendering path. A missing/localized face or substituted
font can change the labels' orientation and layout. Exact locale-specific
failures require running that Windows/font configuration.

### Netscape Email Attachment Corruption — external

No Netscape/MIME attachment handling is implemented in this repository.
`file.c`'s `FLoadGame`/`ReadRt` and `log.c`'s `FLoadLogFile` consume the binary
files after delivery and can encounter the corruption. The reported mail
client's text conversion happens outside Stars!; there is no game-source
function to identify as its cause.

### "Stuck" Bug — fixed in 2.8

`turn.c`: `MoveFleets`.
Fleet pursuit uses `lpflNext` to reference the target and makes multiple passes
using `fDone`, `dMoveLeft` and `dMoveUsed`. On reaching a pursued target during
a later pass, the pursuer sets `lpfl->lpflNext->fDone = TRUE`: it marks the
target done, even if that target has its own unfinished pursuit. This is the
mechanism by which an earlier-processed secondary pursuer can stop its main
pursuer. The reported ID ordering and own-race restriction need a scenario
test before being treated as universal constraints.

**2.8:** confirmed by `tests/unit/test_turn.c`: with A chasing B and B
chasing C, all starting together, B never left the start. A pursuer that
catches a fleet still on its own pursuit now keeps following it with the
rest of its move, and the caught fleet finishes its pursuit.

### WP0 Pop Reload Ignored — candidate

`ship.c`: `FEnumCalcJettison`; `shipui.c`: `TransferStuff`, `ClickInShipOrders`;
`log.c`: `LogMakeValidXfer`, `FRunLogRecord`;
`turn2.c`: `FQueueColonistDrop`, `DropColonists`;
`turn3.c`: `SatisfyOrders` (colonize task).
Manual unloading/reloading over an uninhabited planet uses pending population
drops and cargo log consolidation, while colonization is a waypoint task.
Host replay adjusts pending drops, then colonization consumes the fleet.
These are the paths to compare when the client reload is omitted from the
submitted log; the exact order-dependent omission is not yet isolated.

### Crash Stars Bug — unresolved

The supplied fleet-1/player-1/final-player/base-slot-10 setup does not yet map
to a proven invalid access. Candidate paths are `battle.c`'s `CplrBattle`,
`InitializeBoard`, `LpshdefFromTok`, `KillShips`, and `FDamageTok`, and
`turn.c`'s `FTravelThroughMineFields`, which remove ships and resolve base
designs during damage processing. Base tokens use ship-design indices offset
by 16 and resolve into ten-entry base arrays. This is an investigation starting
point, not an assertion that any particular function causes the reported crash.
The native attack-mask repair above is a separate known corruption site.

### Shields Displayed Incorrectly in VCR — candidate; owner handling exists

`vcr.c`: `SetVCRBoard`, `GetVCRStats`, `DrawVCR`; `battle.c`: `RegenShield`,
`LpshdefFromTok`; `util.c`: `DpShieldOfShdef`.
Playback subtracts logged shield damage and regenerates shields between rounds.
`RegenShield` calls `DpShieldOfShdef` with the token owner's `iplr`, and initial
tokens are restored from the recorded battle, so the blanket claim that all
playback shield values use the viewer's RS setting is not established here.
The differing later-round displays need reproduction and inspection of the
foreign-design/display path. Combat damage calculations are a separate path.

## Cosmetic bugs

### Battle VCR Point of View — candidate, overlaps the shield report

`vcr.c`: `VCRDlg`, `DrawVCR`, `GetVCRStats`; `buildui.c`: `DrawBuildSelHull`;
`util.c`: `UpdateShdefCost`, `GetFleetScannerRange`;
`ship2.c`: `PctCloakFromLpfl`; `popup.c`: `DrawPopup`.
The VCR opens ship-design inspection for its focused token. Several shared
design/stat calculations depend on global `idPlayer` (notably
`UpdateShdefCost` for RS armor), whereas fleet scanner and cloak calculations
also have owner-aware paths. These are the places to check for the reported
viewer-dependent RS armor/shields, SS cloaking and JOAT scanner displays.
The complete set of misleading values and the exact viewer-context handoff
have not been confirmed; this report does not imply combat itself uses the
viewer race's traits for foreign ships.

## Native SDL title encoding

Native creation and `WM_SETTEXT` previously passed CP1252 strings directly to
SDL's UTF-8 title API. The inspector's Euro-sign title exposed an X11 conversion
failure and two leaked text-property allocations. The boundary now converts
native titles to UTF-8 while stored/returned narrow text remains CP1252. Tests
check creation and title changes, including retained narrow text; wizard reuse
uses the same conversion helper.
