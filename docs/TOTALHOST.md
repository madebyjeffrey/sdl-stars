# TotalHost file fixes

TotalHost (Rick Steeves) is a
web host for Stars! games. Because it can't change `stars.exe`, it rewrites
the game's files to fix or block known bugs and exploits. This page lists
what it changes, says whether 2.8 already handles each item, and suggests
what 2.8 could take from it. Entries are cross-referenced to
[KNOWN-BUGS.md](KNOWN-BUGS.md) and [ROADMAP.md](ROADMAP.md), step 5.

Read from a local clone at commit `ca05ede`. The newer copy is
`scripts.dbi/` (Linux/MariaDB); `scripts.odbc/` is the older Windows copy
with the same logic. The work is in `StarsBlock.pm`:

| Tool | Entry point | Runs on | When TotalHost runs it |
| --- | --- | --- | --- |
| StarsClean | `decryptClean` | every `.m` file | after each turn is generated, if the game has **Sanitize** set (`TurnMake.pl` → `cleanFiles`). The original is kept as `.m.preclean`. |
| StarsFix | `decryptFix` | `.hst` (builds the state lists) and each uploaded `.x` | on each `.x` upload, if the game has **Exploit** set (`upload.pl` → `StarsFix`). The original is kept as `.x.preFix`; warnings go to the game's page. |
| StarsRace | `decryptBlockRace`, `checkRaceCorrupt`, `raceCheckSum` | `.r` race files | race uploads are rejected when the checksum is wrong (`upload.pl`); the standalone `StarsRace.exe` rewrites the checksum. |

StarsFix can't see game state in a `.x` file, so a pass over the `.hst`
first writes list files (`.hst.fleet`, `.hst.queue`, `.hst.design`,
`.hst.waypoint`, `.hst.planet`, `.hst.last`). The `.x` pass checks orders
against them.

## Summary

| TotalHost item | File | What it rewrites | 2.8 status |
| --- | --- | --- | --- |
| Mystery Trader knowledge | `.m` | MT "met" mask cut down to the file's player; MT item cleared | fixed |
| Minefield knowledge | `.m` | minefield "seen by" mask cut down to the file's player | fixed |
| Wormhole knowledge | `.m` | wormhole "seen by" and "travelled by" masks cut down to the file's player | fixed |
| Other players' race data | `.m` | full-detail player records reset to Humanoid defaults, relations to neutral (CA keeps habitat) | fixed (Claim Adjuster turn files) |
| Cheap Colonizer | `.x` | empty slot still tagged as a colonization module set to truly empty | fixed (Colonization Module Check) |
| Space Dock armor overflow | `.x` | more than 21 Superlatanium clamped to 21, armor recomputed | fixed |
| SS Pop Steal | `.x` | Robber Baron transport order to load colonists from someone else's planet set to no action | fixed |
| Starbase Friendly Fire | `.x` | default battle plan "attack player N" reset to Neutral/Enemies | fixed (native repair, tested) |
| Cheap Starbase | `.x` | edit of a partly built starbase design blanked | fixed |
| Mineral Upload | `.x` | manual transfer to a foreign fleet beyond its cargo capacity cancelled | fixed |
| 32k Merge | `.x` | waypoint merge that would pass 32767 of one design set to no task; manual merge only warned | fixed |
| 10th starbase design crash | `.x` | warning only | **open** (Crash Stars) |
| Race file corruption | `.r` | checksum rewritten | fixed |
| FreePop | `.x` | none (commented out, untested) | candidate |

## Turn files (`.m`): knowledge leaks

StarsClean's header comment says it removes "privileged" information about
other players. Each `rtThing` record (type 43 in TotalHost's numbering) is
written whole by `FWriteDataFile` (`save.c`): `WriteRt(rtThing, 18, lpth)`.
The per-player bitmasks in it therefore go to every player who can see the
object, though the client uses only its own bit. The original wrote them;
2.8 doesn't (below).

| Object | Field (`structs.h`) | Offset in the record | What it leaks | TotalHost |
| --- | --- | --- | --- | --- |
| Mystery Trader | `tht.grbitPlr` | 12 | which players have met the trader | keeps only the file player's bit (`resetPlayers`) |
| Mystery Trader | `tht.grbitTrader` | 14 | the part the trader is carrying | set to 0 |
| Minefield | `thm.grbitPlr` | 10 | which players have seen the field | keeps only the file player's bit |
| Minefield | `thm.grbitPlrNow` | 14 | which players see it this turn | **not cleaned** (decoded but left alone) |
| Wormhole | `thw.grbitPlr` | 8 | which players have seen the wormhole | keeps only the file player's bit |
| Wormhole | `thw.grbitPlrTrav` | 10 | which players have gone through it | keeps only the file player's bit |
| Mineral packet | — | — | — | left alone; only written for packets the player can see |

Only the player's own bit seems to be read on the client: `mineui.c` tests
`1 << idPlayer & tht.grbitPlr` to show the trader's "send a fleet" text,
and `util.c` tests the fleet owner's bit in `thw.grbitPlr`. No client
use of `tht.grbitTrader` was found.

TotalHost doesn't check whether the file player's bit was set: if it was, it
writes back just that bit; if not, it writes 0.

**2.8:** when `FWriteDataFile` writes a player's file
(`iPlayer != iplrNone`), it writes a copy of the `THING` with each mask
ANDed with `1 << iPlayer` and `tht.grbitTrader` cleared; the `.hst` keeps
everything. That covers what StarsClean does, plus `thm.grbitPlrNow`. The
record stays 18 bytes. The AI reads only its own bits
(`aiutil.c`'s wormhole search), so the native regression's AI play doesn't
change; only the turn files' thing records do (`tests/unit/test_save.cpp`).

### Other players' race records

StarsClean also resets every full-detail (`det == detAll`) record of
another player, bytes 8–111, to Humanoid defaults, and zeroes its
relations; for a CA player it keeps the habitat bytes (16–24). This is the
Claim Adjuster leak already fixed in 2.8 (`FWriteDataFile` writes a
full-detail copy with only the habitat filled in; see `CHANGELOG.md`).
Nothing left to take.

## Order files (`.x`): exploit blocking

StarsFix edits the player's orders before the host sees them. 2.8 can fix
the cause in turn generation instead, so these only point at causes and
show how the exploits are carried out.

### Already fixed in 2.8

- **Cheap Colonizer** (design blocks 26/27): a slot with category
  `0x1000`, item 0, count 0 gets its category zeroed. 2.8 fixed the host
  check (`SatisfyOrders` needs `cItem > 0`), so a doctored design gets
  nothing.
- **Space Dock armor overflow** (design blocks): a Space Dock (hull 33) with
  more than 21 Superlatanium and armor ≥ 49518 has the count set to 21 and
  armor recomputed as `250 + 1500*21/2` plus 65 per Croby/Langston. 2.8
  fixed the overflow in `UpdateShdefCost`, which removes the need to cap.
- **SS Pop Steal** (waypoint change block 5): a transport order with a
  population action at a planet the player doesn't own, from a fleet with a
  Robber Baron, gets bytes 18–19 (the population action and amount)
  cleared. 2.8 refuses the load in `SatisfyOrders` and sends the
  "attempt was unsuccessful" message.

### Starbase Friendly Fire: fixed by the native repair

Battle plan block 30: if plan 0 (the default plan, which starbases use) has
`attackWho > 3` (a specific player), TotalHost sets it to 2
(Neutral/Enemies). This avoids the Win16 bug in `CplrBattle` where the
starbase's attack-player case indexed `rggrfAttack` with uninitialized
`iplrCur`. The native build already uses `iplrStarbase` (see
[WIN16-PARITY.md](WIN16-PARITY.md), "Starbase attack mask index").
`tests/unit/test_battle.cpp` gives a starbase's default plan one target, with
a friend and the target in orbit, for every order of three players: the
starbase attacks only its target and nobody fires on the friend.
TotalHost's workaround isn't needed with 2.8.

### Cheap Starbase: fixed in 2.8

How it works, from StarsFix: the `.hst` pass records production queues
(block 28: `cItem`, `iItem`, `grobj`, `pct`). On the `.x` pass, a design
change (block 27) to a starbase design with `totalBuilt == 0` that appears
partly built (`pct > 0`) in any of that player's queues is caught; it zeroes
every slot and resets armor to the bare hull. A queue change (block 29)
naming an edited base design is also warned.

This matches KNOWN-BUGS.md: `PROD.pct` keeps the work done on the old
design, and the queued-design checks in `ship.c` only guard planets that
already have a starbase. **2.8:** `FRunLogRecord`'s design-change record clears the
partial progress of every queued item of that design, ship or starbase,
on all the player's planets. That also covers deleting and recreating a
design.

### Mineral Upload: fixed in 2.8

Manual transfer blocks 1, 2 and 25 (small, medium and large amounts): for
transfers from the player's planet to another player's fleet, StarsFix sums
the minerals sent to each fleet across the file. When the total exceeds
the fleet's cargo capacity (from `.hst.fleet`, with capacity worked out
from the designs), it zeroes the cargo mask (byte 5), which cancels that
order. It uses total capacity, not free space, so a partly loaded fleet can
still lose minerals.

The host replays the transfer in `FRunLogRecord`: it takes the cargo from
the source, then delivers it through `ChgCargo`, which caps at free hold
space; the rest was destroyed. **2.8:** what doesn't fit goes back to the
source, which fixes it without blocking legal transfers (see
KNOWN-BUGS.md).

### 32k Merge: fixed in 2.8

- Waypoint merge (block 5, task 4, target a fleet): if any design's
  combined count would pass 32767, the task is cleared (low nibble of byte
  10).
- Manual merge/move (blocks 23/24/37): StarsFix tracks counts and warns but
  doesn't edit; its comment says the client caps at 32,767 itself.

The cause is in ROADMAP 5.2: `Merge2Fleets` adds into `int16_t rgcsh` with
no limit. TotalHost's split (fix the waypoint path, trust the manual path)
suggests the client's merge dialog already caps and the host's waypoint
merge doesn't. In fact the manual merge (`FFleetMergeAll`) clamped the
count to 32766 and lost the rest. **2.8:** `Merge2Fleets` moves only the
ships that fit, and `FFleetMergeAll` leaves out a fleet that wouldn't fit.

### 10th starbase design: warning only

When the last player in the game has a starbase design in slot 10
(`isb == 9`), StarsFix warns: "Potential Crash if Player 1 Fleet 1 refuels
at Last Player 10th starbase design." It has no fix.

This is the "Crash Stars" report (KNOWN-BUGS.md, ROADMAP 5.3). The
**refuel** detail is new: the current starting points are battle and
minefield damage. Base designs are `rglpshdefSB[iplr][0..9]`; for the last
player, slot 9 is the end of the allocation, so reading one entry too far
from there leaves the array, and player 1's fleet 1 may be what comes
next. Look at code that fuels a fleet from a starbase (the
"automatically refueled by your starbase" path) for an off-by-one or a
`ishdef - 16` used without the offset.

### FreePop: not implemented

StarsFix has an untested check comparing cargo moved against fleet cargo,
commented out. Its header notes that edited `.x`/`.m` files are rejected
during generation, which matches KNOWN-BUGS.md's finding that the host
already checks `COLDROP`. Nothing to take.

## Race files (`.r`): bad checksum

On upload TotalHost recomputes the race checksum and rejects the file if
it differs: "This race file is corrupt! Caused by making the plural name
too short." `StarsRace.exe` writes a `.fixed` copy with the checksum in the
`rtEOF` record replaced by the correct one.

TotalHost's checksum (`raceCheckSum`) treats both names as zero-padded
after their terminators. Our source shows why that differs from what
`stars.exe` writes:

- `IRaceChecksum` (`race.c`) XORs the first 96 words of the in-memory
  `PLAYER`, which covers all 32 bytes of `szName` and `szNames`, including
  anything after the terminator.
- `RaceWizardDlg1`'s OK/Next path reads the names with
  `GetDlgItemText(…, vplr.szName, 32)` and `(…, vplr.szNames, 32)` without
  clearing the buffers first. The radio-button path a few lines later does
  `memset(…, 0, 32)` first. When a name is shortened, or a template's
  name is replaced with a shorter one, the old bytes stay after the new
  terminator.
- `FSaveRace` checksums that buffer, but `WriteRtPlr` saves only the
  string. On load, `ReadRtPlr` zeroes the `PLAYER` before filling it, the
  stale bytes are gone, and the checksum test in `mdi.c` fails.

So the checksum covers bytes that aren't in the file. This explains the
"shorter name" reports and why not every short-name save is affected: the
old name has to have been longer at the same position.

**2.8:** fixed in `IRaceChecksum`, which now checksums a copy with the
names zeroed after their terminators, so the editor and every other caller
agree with what `ReadRtPlr` loads (`tests/unit/test_race.cpp`). The file
format and checksum algorithm are unchanged, so races saved by 2.8 load in
2.6j. Files already saved with a bad checksum are still rejected:
TotalHost's repair just recomputes the checksum, which is also all a
tamperer would need.

## Other TotalHost features (not game fixes)

These aren't fixes for 2.8 but explain other things TotalHost writes:

- `checkSerials`: compares the serial/hardware block (type 9) across a
  game's `.x` files and flags the same serial on different hardware. The
  game's own check is `SpankTheCheaters`/`FLoadLogFile`.
- `trimM` (`StarsTrimM.pl`): cuts a multi-turn `.m` file down to its last
  turn to make it smaller.
- `StarsAI`/`decryptAI`: switches a player between human, inactive and
  housekeeping AI in the `.hst`.
- `StarsPWD`: resets a player's password in the `.hst`.

## Running TotalHost on stars-host

TotalHost runs `stars.exe` under Wine (`$WINE_executable` in
`scripts.dbi/config.base.pl`) for five things, all of which `stars-host`
does natively on x86-64 Linux, with the same exit codes:

| TotalHost | Command | Reads back |
| --- | --- | --- |
| `page.pl` creates a game | `-a game.df2` | the game files |
| `Make_CHK` | `-v game.hst` | `game.chk` (identical but for the version line) |
| `GenerateTurn` | `-g<n> game.hst` | the turn files |
| `movie_starmapper.pl` | `-dm game.m1`, `-dp game.mN` | `game.map`, `game.pN` |

To switch:

- Point `$WINE_executable` at `stars-host --ini <path>/stars.ini`, the
  `stars.ini` TotalHost keeps for `stars.exe`. The movie script copies
  `game.pN`, which needs `[Misc] NewReports=1` there; without `--ini`,
  `stars-host` writes `game.pla`.
- Pass Unix paths: the command lines above build `$Dir_WINE\\…` paths, and
  `page.pl` writes them into the `.df2` race lines too. `stars-host` reads
  `/` as the directory separator and takes switches only after `-`.
- The waits for `stars.exe` (the `sleep 2` between movie launches and
  `Make_CHK`'s poll for a newer `.chk`) can go: `stars-host` has written its
  files when it exits.
- As with `stars.exe`, a turn file's dumps need its password (`-p`) or none;
  the movie script removes them first (`StarsPWD`).

