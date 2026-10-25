# S01 binary records and codecs

Part of [S01](S01-game-file-formats.md). Source revision and policy distinctions are
recorded there. Offsets below are **payload-relative decimal byte offsets**,
except explicitly hexadecimal values. Bit 0 is the least significant bit.
`u8/u16/u32` are unsigned, `i8/i16/i32` signed two's-complement integers. All
multi-byte values are little-endian; unaligned values are valid. No native
pointer, alignment padding or compiler enum layout defines a new disk field.

Primary evidence: [structs.h](../src/core/structs.h),
[enums.h](../src/core/enums.h), [save.c](../src/core/save.c),
[file.c](../src/core/file.c), [utilgen.c](../src/core/utilgen.c).
The layouts below follow the writer/readers' actual slicing and packing, not
`sizeof(PLANET)` or another pointer-bearing native struct.

The [Dafny specification](S01-game-file-specification.dfy) formalizes these data and
conditions. [S01 §1.2](S01-game-file-formats.md#12-what-the-formal-model-establishes) maps
each area to its definitions and states the proof boundary. These tables retain
the source evidence and distinguish writer output from observed reader behavior.

## Story-linked requirements

This companion enables [S01-US01](S01-game-file-formats.md#s01-us01)
(preserve meaning) and [S01-US04](S01-game-file-formats.md#s01-us04)
(bounded acceptance). The actor is a codec developer; inputs are bounded bytes,
file kind/version and catalog/schema context, or validated values for writing.

Workflow: identify framing/discriminator, decode the declared payload and text
transforms, then hand logical values to S06/caller validation. Writing reverses
the declared mapping with kind-specific canonicalization. Physical truncation,
invalid UTF-8 and unsupported variants follow the actual reader/strict-policy
boundary; do not infer one rejection policy from every field table. Direct
stream failure may leave output partially written.

| ID / parent story / status | Acceptance criterion | Rule, evidence and coverage |
| --- | --- | --- |
| <a id="s01-rec-ac01"></a>S01-REC-AC01 / US01 / Observed | Given tag/length in the declared domains, when framing a canonical record, then use the two-byte packed header and exact payload length; native pointer/alignment sizes add no bytes. | [Framing](#1-framing-and-tag-inventory); `Header`, `Frame`, `ReadRecord` in [S01 contracts](S01-game-file-contracts.dfy). Existing S01-A wire vectors/model properties; no universal raw-reader validation claim. |
| <a id="s01-rec-ac02"></a>S01-REC-AC02 / US01 / Observed | Given valid bounded UTF-8 user text, when encoding/decoding the nibble representation, then retain its bytes/code points; malformed text is not converted with replacement characters by the bounded adapter. | [Text](#32-user-strings); `FUtf8`, `ReadText`, `ReadCompressed`, `CodecVectors` in [codecs](dafny/S01-codecs.dfy); encoding tests. Full codec/C equivalence remains separate. |
| <a id="s01-rec-ac03"></a>S01-REC-AC03 / US01, US04 / Observed schema, Proposed complete validation | Given a partial fleet/planet/design payload, decode according to its discriminator and retain absent information; a strict adapter rejects inconsistent size/domain/reference combinations before publication. | [Planet](#6-planet-and-production-queue), [fleet](#8-fleets), [record model](dafny/S01-records.dfy), [validation](S01-game-file-validation.md#2-proposed-strict-decoder-contract). Schema predicates and selected vectors; current readers are incompletely bounded. |

| ID / status | Interaction or quality requirement and check | Owner / gap |
| --- | --- | --- |
| <a id="s01-rec-ir01"></a>S01-REC-IR01 / Proposed interface | Return record/tag/offset context and a candidate or failure through the decoder boundary. Caller UI owns readable/accessibility-aware messages; codecs have no focus or display size. | [S01-IR02](S01-game-file-formats.md#s01-ir02); error schema integration remains proposed. |
| <a id="s01-rec-qr01"></a>S01-REC-QR01 / Observed | Verify fixed widths, endian order, signed interpretation, UTF-8 limits and canonicalization with byte vectors at boundaries. | S01/S04; existing field tables/lemmas/tests. Native struct round trips are not a valid substitute. |
| <a id="s01-rec-qr02"></a>S01-REC-QR02 / Open | Exercise maximum legal payloads and accumulated multi-record results, measuring peak memory/time and checking precise truncation failures. Agree aggregate budgets through [S01-QR03](S01-game-file-formats.md#s01-qr03). | S01; per-record 1023-byte length does not bound whole-file allocations. |

Game/race compatibility policy stays in the S01 entry; commands stay in the orders
companion. This record inventory is not a security certification or an approval
to change how unknown tags or malformed legacy input are handled.

Checks for this story expansion are recorded in the
[collection verification](README.md#story-expansion-verification).

## 1. Framing and tag inventory

Each record begins with `u16 header = cb | (rt << 10)`. Payload length is
`header & 0x03ff`, tag is `header >> 10`; length is 0–1023 bytes excluding the
two-byte header. Headers are plaintext. BOF and EOF payloads are plaintext;
other record payloads use the XOR stream in §3. Physical short reads fail.
There is no universal record CRC. `.xy` has an unframed coordinate array.

| Tag | Current use | Payload contract |
| ---: | --- | --- |
| 0 | EOF | Empty for orders/history; u16 turn for host/turn; u16 player count for universe; u16 checksum for race. |
| 1, 2 | Cargo-transfer orders | [Orders reference](S01-game-file-orders.md), signed byte/word quantities. |
| 3, 4, 5 | Waypoint delete/insert/update orders | Orders reference. |
| 6 | `rtPlr` | Full/partial PLAYER plus two encoded strings, §5. |
| 7 | `rtGame` | 64 bytes, §4. |
| 8 | `rtBOF` | 16 bytes, §2. |
| 9 | Log header (literal tag in writer) | 17 bytes, orders reference. |
| 10, 11 | Repeat-orders flag / waypoint task change | Orders reference. |
| 12 | `rtMsg` | Packed catalog messages, §11. |
| 13, 14 | `rtPlanet`, `rtPlanetB` | Packed full/observed planet, §6. |
| 15 | No distinct current output | History passes 15 to `WritePlanet` for low detail; its early path writes 14. |
| 16, 17 | `rtFleetA`, `rtFleetB` | Packed full/observed fleet, §8. |
| 18 | No current writer/reader dispatch identified | Unassigned in current `RecordType`; do not invent a payload. |
| 19, 20 | `rtOrderA`, `rtOrderB` | 18-/8-byte snapshot waypoint, §9. |
| 21 | `rtString` | Encoded fleet name, §3. |
| 22 | `rtSel` | One reader-only record is skipped after things; current writer emits none and loader discards its payload. |
| 23, 24, 25 | Ship transfer, fleet split, 32-bit cargo transfer | Orders reference. |
| 26 | `rtShDef` | Packed design, §7. |
| 27 | `rtLogShDef` | Design mutation, orders reference. |
| 28 | `rtProdQ` | Array of 4-byte PROD, §6. |
| 29 | `rtLogPlanetProdQ` | Queue mutation, orders reference. |
| 30 | `rtBtlPlan` | Battle plan or log deletion, §10. |
| 31 | `rtBtlData` | Battle bytes, first chunk, §12. |
| 32 | `rtHistHdr` | Two i16: cPlanet at 0; cPlanetExtra at 2. |
| 33 | `rtMsgFilt` | `cbbitfMsg` filter bytes; current value 49, bit n selects message ID n. |
| 34, 35 | Research / planet routing orders | Orders reference. |
| 36 | `rtChgPassword` | i32 host credential in `.hst`; i32 player credential change in `.xN`. |
| 37, 38 | Fleet merge / relations orders | Orders reference. |
| 39 | `rtContinue` | Continuation of battle bytes, §12. |
| 40 | `rtPlrMsg` | Player-composed message, §11. |
| 41 | `rtAiData` | AI history chunks, §14. |
| 42 | Fleet battle-plan selection order | Orders reference. |
| 43 | `rtThing` / `rtLogThingByteParam` | Count/entity records in snapshots; minefield detonation mutation in orders. Context is mandatory. |
| 44 | Fleet rename order | Orders reference. |
| 45 | `rtScore` | 24-byte SCOREX, §13. |
| 46 | Player production template order | Orders reference. |
| 47–63 | No current emitted tag | `rtMax=47` is an enum boundary, not a file record. |

Unknown snapshot tags usually break expected sequencing. Unknown log tags fall
through replay as successful no-ops. Neither is a versioned extension system or
a promise of lossless unknown-record preservation. Tag 43 cannot be decoded from
the number alone. See [validation](S01-game-file-validation.md) for actual checks.

## 2. BOF

| Offset | Width | Meaning |
| ---: | --- | --- |
| 0 | 4 raw bytes | ASCII `J3J3`. |
| 4 | i32 | `lidGame`; identity bits also feed stream initialization. |
| 8 | u16 | `verInc` bits 0–4, `verMinor` 5–11, `verMajor` 12–15. Writer emits `0x2aa0`. |
| 10 | u16 | `turn`; calendar year is 2400 + turn. |
| 12 | u16 bit container | Signed five-bit `iPlayer` at 0–4 (-1 encoded as 31); eleven salt bits at 5–15 (`lSaltTime`, signed in C but consumed as masked bits). |
| 14 | u16 | `dt` 0–7; `fDone` 8, `fInUse` 9, `fMulti` 10, `fGameOverMan` 11, `fCrippled` 12, `wGen` 13–15. |

The salt comes from low clock bits plus `Random(2000)`, truncated to eleven
bits. `wGen` identifies a generation of orders for the turn, not a cryptographic
nonce. `fDone` means submitted; `fInUse` marks host use; `fMulti` describes
concatenated turn segments. `fGameOverMan` is emitted only for host kind.
Writer sets `fCrippled=0`. `WriteBOF` zeroes its local record before packing it.
A canonical BOF record begins `10 20 4a 33 4a 33` (HDR then signature).

`ReadRt` enforces BOF size 16 before initializing the stream. The general
`FOpenFile` path does not itself check `rgid` or `dt`; the library inspector does.
Do not confuse canonical required fields with uniformly enforced validation.

## 3. Stream transform and text

### 3.1 XOR stream

Evidence: `SetFileXorStream`, `LGetNextFileXor`, `XorFileBuf` in
[utilgen.c](../src/core/utilgen.c). Use these exact first 64 entries of
`rgPrimes`; the literal **279** is intentional for compatibility despite the
array's name:

```text
3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59
61 67 71 73 79 83 89 97 101 103 107 109 113 127 131 137
139 149 151 157 163 167 173 179 181 191 193 197 199 211 223 227
229 233 239 241 251 257 263 279 271 277 281 283 293 307 311 313
```

Initialize `a=salt&31`, `b=(salt>>5)&31`. If `salt&0x400`, add 32 to `a`;
otherwise add 32 to `b`. Set `s1=table[a]`, `s2=table[b]`. Discard
`((lidGame&3)+1)*((turn&3)+1)*((iPlayer&3)+1)+fCrippled` outputs.
Each output uses these operations, with signed division truncating toward zero
and 32-bit arithmetic wrapping where needed:

```text
k = s1 / 53668
s1 = 40014 * (s1 - k*53668) - k*12211
if s1 < 0: s1 += 2147483563
k = s2 / 52774
s2 = 40692 * (s2 - k*52774) - k*3791
if s2 < 0: s2 += 2147483399
return (s1 - s2) modulo 2^32
```

XOR each payload with successive output words, low byte first. A record with
1–3 remaining bytes consumes a complete additional word and discards its unused
high bytes. Each record therefore advances `ceil(cb/4)` times, not a continuous
byte stream across boundaries. Headers/BOF/EOF consume no words; BOF resets the
seeds. Raw star packs consume none. Encryption and decryption are the same
operation. The transform supplies obfuscation, not authenticated encryption.

Example: `lid=0, turn=0, player=0, salt=0, crippled=0` starts from `(3,139)`
and discards one output. A payload of length 5 then consumes two more words;
a following 1-byte payload starts with a fresh word, not byte 2 of the previous
word. Reframing identical plaintext into different record lengths changes its
ciphertext stream placement.

### 3.2 User strings

`FReadUserString` decodes the format used by player names, design names, battle
plans and fleet names:

- First byte 0: following bytes are raw UTF-8 terminated by NUL within this
  record and the destination's capacity. Consumed size is `2 + text_bytes`.
- First byte `n>0`: exactly `n` following bytes are nibble-compressed; consumed
  size is `1+n`. There is no raw terminator. Decoded text must fit and be valid
  UTF-8 without embedded NUL.

Names have a 32-byte memory field: at most 31 UTF-8 bytes plus NUL. Writers
normally try compression with a 31-byte compressed budget, then raw fallback;
this need not select the shortest representation. Empty names use `00 00`.
`WriteRtString` emits no record for a null/empty fleet name. Other helpers
serialize empty strings because their field is required. Several current name
readers accept unused trailing record bytes; strict whole-record consumption
is not universal.

Compression traverses input UTF-8 **bytes**, without Unicode interpretation.
Pack nibbles high half first in each output byte:

| First nibble | Decode |
| --- | --- |
| 0–10 | Index into `" aehilnorst"`. |
| 11 (`B`) plus nibble v | `A` through `P` for v=0–15. |
| 12 (`C`) plus v | `Q`–`Z` for v=0–9; `0`–`5` for v=10–15. |
| 13 (`D`) plus v | `6`–`9` for v=0–3; `bcdfgjkmpquv` for v=4–15. |
| 14 (`E`) plus v | `wxyz` for v=0–3; `+-,!.?:;'*%$` for v=4–15. |
| 15 (`F`) plus two nibbles lo,hi | Literal byte `lo | hi<<4`. |

When an odd number of nibbles remains, pad the final low half with `F`.
Only that final low-half `F` is padding; an incomplete escape elsewhere fails.
`FDecompressUserStringBytes` uses the same byte codec without UTF-8 validation
for the bounded legacy-race conversion path. `FDecompressUserString` rejects
malformed UTF-8, embedded NUL, incomplete escapes and capacity overflow.
Examples: raw `A` is `00 41 00`; compressed `A` is `01 b0`; compressed `a` is
`01 1f`; the UTF-8 bytes `c3 a9` encode as nibble sequence `f 3 c f 9 a`.

### 3.3 Fixed text and credentials

GAME title is raw UTF-8 in 32 bytes and requires a terminator. Raw name fields
in the reconstructed PLAYER used for checksumming are zero-filled after their
terminators. Password input is at most 15 UTF-8 bytes; only its i32 hash is
stored. Native filenames must round-trip losslessly through UTF-8 under
[the encoding boundary](../docs/UNICODE-BOUNDARIES.md). Neither a compressed
byte nor a salt byte is decoded application text.

## 4. Universe GAME and STARPACK

GAME is always 64 bytes:

| Offset | Type | Field / model mapping |
| ---: | --- | --- |
| 0 | i32 | `lid`, game identity. |
| 4,6 | u16 each | `mdSize` (0–4), `mdDensity` (0–3), M03 settings. |
| 8,10 | i16 each | `cPlayer` (1–16), `cPlanMax` (supported array capacity 999). |
| 12 | u16 | `mdStartDist` (0–3). |
| 14 | i16 | `fDirty`; loader clears it. It is not durable unsaved-order state. |
| 16 | u16 | Flags: extra fuel 0, slow tech 1, single player 2, tutorial 3, AI alliance 4, BBS 5, visible scores 6, no random events 7, clumping 8, wGen 9–11, unused 12–15. |
| 18 | u16 | Turn; universe creation ordinarily writes zero. |
| 20 | u8[12] | `rgvc`, low seven bits hold each raw value; high bits at indices {0,1,3,4,5,6,7} enable the seven conditions. Indices 0–9 meanings/scales in [M03](M03-new-game-wizard.md#43-victory-controls-exact-transforms-ranges-and-defaults), 10–11 unused by current controls. |
| 32 | char[32] | Game title, UTF-8 + NUL. |

Each raw STARPACK is one u32: dx bits 0–9, y bits 10–21, planet-name ID bits
22–31. X is cumulative from 1000. No mutable planet fields occur here.
IDs and name catalog order remain stable. The filename and GAME title do not
participate in stream identity matching.

## 5. PLAYER and race payload

`WriteRtPlr` writes the following prefix, followed by strings. For `det != 7`,
only bytes 0–7 occur. For `det == 7`, bytes 0–111 occur, then a **u8 relation
count**, that many signed relation bytes (normally 0–16), then both names.
The writer drops trailing zero relations. It does not write fixed 16-byte
relation storage nor the fixed name arrays verbatim.

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | i8 | `iPlayer`, zero-based. |
| 1 | i8 | `cShDef`, count of subsequent ship designs for this owner. |
| 2 | i16 | `cPlanet`, number of included planets owned by this player. |
| 4 | u16 | `cFleet` bits 0–11; `cshdefSB` 12–15. |
| 6 | u16 | `det` 0–2, icon `iPlrBmp` 3–7, include 8, `mdPlayer` 9–15. AI overlay: `fAi` 9, `lvlAi` 10–12, `idAi` 13–15. |
| 8 | i16 | `idPlanetHome`. |
| 10 | u16 | `wScore`. |
| 12 | i32 | `lSalt`, password hash. |
| 16,19,22 | i8[3] each | Habitat center/min/max; -1 means immune. |
| 25 | i8 | `pctIdealGrowth`. |
| 26 | i8[6] | `rgTech`. |
| 32 | u32[6] | `rgResSpent`. |
| 56 | i8 | `pctResearch`. |
| 57 | u8 | Current tech low nibble; signed `iTechNext` high nibble in PLAYER. |
| 58 | i32 | `lResLastYear`. |
| 62 | i8[16] | `rgAttr`, economy/research/primary trait/leftover fields, mapped by [M04](M04-race-data-model.md). |
| 78 | u32 | `grbitAttr`, trait flags. |
| 82 | u16 | `grbitTrader`. |
| 84 | u16 | dead 0, crippled 1, unused 2, learned 3, hacker 4, unused 5–15. |
| 86 | 26 bytes | ZIPPRODQ1, described below. |
| 112 | variable | Relation count + bytes, then singular and plural encoded names. |

ZIPPRODQ1: u8 `fNoResearch`, u8 `cpq`, then 12 u16 PRODQ1 slots. Each slot
contains `mdIdle` bits 0–5 and `cQuan` bits 6–15. The structure is a production
template, not a ZIP archive.

`ReadRtPlr` zeroes all 192 in-memory bytes, restores the prefix/relations/names,
then clears `fLearned`. Modern parsing copies at most 16 relations but skips the
entire declared count if record-bounded; legacy parsing rejects counts >16.
Race definitions use this encoding but do not turn player runtime fields into
independent reusable race properties. See [M04's logical mapping](M04-race-data-model.md).

### Race checksum and password hash

`IRaceChecksum` takes the 192-byte PLAYER representation, clears the bytes from
each name terminator through the end of its 32-byte field, then XORs all 96
little-endian u16 words. Relations omitted on wire decode to zero. For a valid
race, the plaintext EOF u16 equals that reconstructed checksum. Names need to
be decoded **before** computing it. Legacy checksum precedes conversion; an
old singular-only record has a zero plural array at this stage. The loader
also clears `fLearned`, so a writer cannot depend on preserving that transient
bit through checksum validation.

`LSaltFromSz`: start u32 accumulator at zero; repeatedly add a sign-extended
input byte, then, if another byte exists, multiply by that sign-extended byte.
Every operation wraps modulo 2^32. Empty password returns zero; nonempty input
whose result is zero returns one. For ASCII `abc`, `(97*98)+99=9605`.
Use signed byte values for UTF-8 high bytes; never substitute Unicode code
points or a modern hash while claiming format equivalence. This credential hash
is separate from BOF's eleven-bit stream salt.

## 6. PLANET and production queue

Both planet variants begin with four bytes:

- u16 at 0: signed 11-bit planet `id` in bits 0–10; signed five-bit `iPlayer`
  in bits 11–15 (-1 unowned/unknown according to detail).
- u16 at 2: `det` 0–6, `fHomeworld` 7, `fInclude` 8, `fStarbase` 9,
  `fIncEVO` 10, `fIncImp` 11, `fIsArtifact` 12, `fIncSurfMin` 13,
  `fRouting` 14, `fFirstYear` 15.

Detail codes are none=0, minimal=1, obscure=2, some=3, more=4, all=7.
For `rtPlanetB` the writer maps detail >3 to 4 in a current observation and 3
in history. `detObscure` current output is temporarily reduced to `detSome`
without a starbase by `FWriteDataFile`.

After the prefix, decode in order:

1. If encoded detail <=1, skip directly to B suffix.
2. u8 mineral-level presence mask: two bits per mineral (Ironium, Boranium,
   Germanium), low pairs first. 0 means zero; 1 consumes a u8
   `rgpctMinLevel[i]`. Reader rejects pairs 2 or 3.
3. Three u8 `rgMinConc`, then three habitat bytes `rgEnvVar` (0–100).
4. If `fIncEVO`, three original habitat bytes (0–100); otherwise originals
   equal current habitat.
5. If owner !=-1, u16 `uGuesses`: population estimate bits 0–11, defense
   estimate bits 12–15. These remain estimates, not exact population/defenses.
6. For detail >3, if `fIncSurfMin`, a u8 width mask and packed cargo values:
   two bits per slot, in Ironium/Boranium/Germanium/population order.
   Code 0=zero/no bytes, 1=u8, 2=u16, 3=i32 bit pattern. The writer selects
   positive values only: <=255 byte, <=65535 word, otherwise dword.
   Population is included only for actual `lppl->det >=7`. If all absent,
   omit both the mask and data and clear `fIncSurfMin`.
7. B suffix: if starbase flag, one byte `isb`; if history, i16 observation
   turn. End record.
8. A suffix (tag 13, rather than B): optional 8-byte improvements block if
   `fIncImp`; then, if owned, optional 4-byte starbase block and 2-byte routing
   block selected by their flags. No observation turn suffix.

Improvement block contains two u32 words. First: iDeltaPop 0–7, mines 8–19,
factories 20–31. Second: defenses 0–11, scanner 12–16, unused 17–21,
artifact 22, no-research 23, unused 24–31. All are packed unsigned bitfields.
When omitted on a fresh load, scanner becomes sentinel 31, defenses zero and
artifact comes from header `fIsArtifact`; other zeroed fields stay zero.
The writer includes improvements when owned and delta-pop/no-research is set,
or any mines/factories/defenses exist, or scanner differs from 31.

Starbase block: u16 `isb` 0–3 and damage percentage `pctDp` 4–15; u16
`idFling` 0–9, `iWarpFling` 10–13, no-heal 14, unused 15. Reader clears no-heal.
Routing word: `idRoute` bits 0–9, remaining six unused. A B-record exposes only
the base-design index, not its full damage/driver state.

On history overlay the reader retains the greater existing/new detail and
some omitted fields; it does not blindly zero the whole planet. Unowned
starbase claims are cleared. Current records set observation turn to game.turn.
Exact quantity conversions and estimate formulas belong to M08/S06; the
serialized cargo units remain the core's `rgwtMin` units, with population in
hundreds of colonists (one cargo unit) and minerals in kT.

`rtProdQ` is a sequence of u32 PROD entries, with no heap/list header:
`cItem` bits 0–9, `iItem` 10–16, `grobj` 17–19, `pct` 20–26, unused 27–31.
Record length is `4*count`; it belongs to the immediately preceding full planet.
Order matters. `pct` is carried production progress; legal changes and refunds
belong to M09 and [log replay](S01-game-file-orders.md). Neither a PLPROD pointer
nor allocation capacity is saved.

## 7. Ship and starbase designs

Common prefix, six bytes:

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | u16 | det 0–7, include 8, free 9, `ishdef` 10–14, gift 15. |
| 2 | u8 | Hull catalog ID `ihuldef`. |
| 3 | u8 | Bitmap index `ibmp`. |
| 4 | u16 | Full: armor `dp`; partial: empty mass `wtEmpty`. |

For detail 7, append u8 slot count at 6, u16 design turn at 7, u32 built count
at 9, u32 existing count at 13, then `slot_count` four-byte HS entries from
17. Each HS is u16 hull-slot category bits, u8 item index, u8 item count.
Append the encoded design name. Full length is `17 + 4*slot_count + string_size`.
There are at most 16 slots in HUL; hull/part numbers refer to append-only S03
catalogs. `ishdef` 0–15 means ship slot; 16–25 means starbase slot minus 16.

For partial detail, append the encoded name **at offset 6**; there is no slot
count or full-design suffix. Writer uses the catalog hull class name rather
than the private custom class name. Owner is inferred from record ordering and
PLAYER counts, or supplied by the enclosing design-change order.

`FReadShDef` bounds the prefix/slots/name and hull ID; repairs bitmap to the
hull's four-image group; preserves an existing full design over a matching
partial one unless hull/bitmap changed; recalculates cost where applicable and
mass for full designs. It clears slot counts for incompatible category,
unavailable technology without gift permission, or over-capacity items, and
restores Quick Jump 5 engines when the engine slot would otherwise be empty.
It is not a pure bit-preserving decoder. Validate roster index and part domains
in any new bounded adapter; not every such constraint is checked here.

## 8. Fleets

The fixed prefix is 14 bytes:

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | u16 | Fleet ID: slot 0–8, player 9–12, remaining three bits legacy storage. |
| 2 | i16 | `iPlayer`. |
| 4 | u16 | det 0–7, include 8, repeat 9, dead 10, **byte-count encoding** 11, bombed 12, here-all-turn 13, no-heal 14, mark 15. |
| 6 | i16 | `idPlanet`; deep-space sentinel -1 (`0xffff`). |
| 8,10 | i16 each | x,y. |
| 12 | u16 | `grbitCsh`, presence of each of 16 ship-design counts. |

Then, ascending slot order, store each present positive ship count. If every
count <=255, bit 11 is set and counts are u8; otherwise all are u16 (loaded into
i16 count storage). Do not widen the stored count contract. Bit 11 overlays
`FLEET.fDone` in native memory; it is not an extra durable gameplay flag.

If detail >=4, append u16 width mask, two bits per cargo slot across five slots:
Ironium, Boranium, Germanium, colonists, fuel. The same 0/1/2/3 widths as planets
apply. Writer supplies population/fuel only at detail 7. Minerals/colonists use
cargo units; fuel is in mg as presented by core UI/report code.

If detail <7, append i32 `dirLong`, then i32 total fleet mass and finish as
`rtFleetB`. Direction word bytes 0,1 are dirFltX/Y; bits 16–19 warp, 20 valid,
21 composition-change, 22 targeted, 23 skipped, 24–31 unused. Total mass sums
ship empty masses plus mineral/colonist cargo, excluding fuel. There are no
orders, damage vector, battle-plan choice or custom-name record in this variant.

If detail 7, append u16 damage presence mask, each present DV u16 in ascending
slot order, u8 battle-plan index, u8 order count, and finish `rtFleetA`. Each DV
has pctSh bits 0–6 and pctDp bits 7–15. Reader clamps pctDp >=500 to 499.
Exactly `cord` following waypoint records and optional `rtString` belong to
this fleet. Their records have their own framing and XOR consumption.
A fresh load zeroes fleet storage, allocates orders, marks a fleet dead if all
ship counts are zero, clears order no-auto-track flags and validates/repairs
planet coordinates in the special creation case. Native pointers, names' heap
addresses and allocation headers never occur on disk.

## 9. Snapshot waypoints

Common 8-byte prefix: i16 x at 0, i16 y at 2, i16 target id at 4, u16 flags at 6.
Flags: task 0–3, warp 4–7, object class 8–11, valid-task 12, no-auto-track 13,
unused 14–15. Object classes are none=0, planet=1, fleet=2, other/free-space=4,
thing=8; target IDs must be interpreted by that discriminator.

`rtOrderB` is exactly 8 bytes and writer uses it for task 0. `rtOrderA` is
18 bytes and adds ten union bytes. Task codes and union meanings:

| Task | Payload starting at 8 |
| --- | --- |
| 0 none | No semantic union payload. |
| 1 transport | Five u16 ITEMACTION, each quantity bits 0–11, action bits 12–15; cargo order as fleets. |
| 2 colonize, 3 remote mine, 4 merge, 5 scrap | No task-specific saved parameters read by the current comparison contract. |
| 6 lay mines | u16 cTime; u16 cTimeOld is write-only bookkeeping; remaining bytes unused. |
| 7 patrol | u16 iWarp; u16 iDist; remaining bytes unused. |
| 8 auto route | No task-specific union words read by the current comparison contract. |
| 9 give | u16 destination player; remaining bytes unused. |

Transport action codes are 0 none, 1 load all, 2 unload all, 3 load exact,
4 unload exact, 5 fill percent, 6 wait percent, 7 load dunnage, 8 set amount,
9 set waypoint. Optimal fuel uses the dunnage/fuel path, not a separate stored
action. Codes 10–15 have no declared variant in [enums.h](../src/core/enums.h).

All 18 bytes may be written even where the task does not use the whole union.
The regression comparator reports inactive union bytes, fUnused and the cleared
fNoAutoTrack bit as unused-storage differences. The logical model should retain
relevant typed task parameters and distinguish any opaque-byte preservation
mode; it must not interpret stale union bytes as additional orders.

## 10. Battle plans

Normal `rtBtlPlan`: u16 at 0 containing player 0–3, plan 4–7, tactic 8–11,
unused 12–13, delete 14, dump-cargo 15. U16 at 2 contains target1 0–3,
target2 4–7, attack-player 8–12, unused 13–15. Encoded name starts at 4.
The snapshot loader assigns the sequence index as `iplan` during unpacking;
log replay uses the encoded plan index. A log deletion contains only the first
two bytes with fDelete set. A normal full-plan record is not fixed-size 36:
its name is variable encoded text.

Declared tactics are 0 disengage, 1 disengage if challenged, 2 minimize self
damage, 3 maximize net damage, 4 maximize damage ratio, 5 maximize damage.
Targets are 0 none/disengage, 1 any, 2 starbase, 3 armed ships, 4 bombers or
freighters, 5 unarmed ships, 6 fuel transports, 7 freighters. These are the
[enum domains](../src/core/enums.h); `FRunLogRecord` currently permits tactic 6
and target 8 as well, so its upper-bound checks are not exact enum validation.

## 11. Messages

### Catalog messages (`rtMsg`)

Evidence: [msg.c](../src/core/msg.c) `WritePlayerMessages`, `ReadPlayerMessages`.
A payload concatenates message entries. Each begins u16 `iMsg` bits 0–8 and
`grWord` bits 9–15, followed by u16 `wGoto`. The number of parameters is
`rgcMsgArgs[iMsg]` from the generated message catalog. For each parameter in
order, one set bit of `grWord` means u16, otherwise u8. There are at most seven
parameter slots. Message IDs select catalog text and formatting semantics;
without that versioned S03 catalog this is not self-describing text.

The transient MSGTURN player/parameter-length byte is omitted. Only the target
player's non-deleted messages (`iMsg !=511`) are emitted. Writer flushes its
chunk before the accumulated byte count plus 20 reaches 1024. Preserve entry
boundaries when encoding; malformed message counts/IDs are not exhaustively
validated by the current reader. `wGoto` is the original tagged navigation word;
its planet/fleet/special-target interpretation belongs to M15, not a pointer.

### Player messages (`rtPlrMsg`)

Fixed 12-byte prefix: u32 legacy next-link at 0, i16 sender at 4, i16 recipient
at 6, i16 reply reference `iInRe` at 8, i16 length at 10. Current writer always
zeros the legacy link; reader ignores it and creates a native linked list.
Recipient 0 broadcasts to other players; otherwise recipient-1 is the player
index. Sender is zero-based.

For `cLen >=0`, exactly cLen compressed bytes follow (same nibble codec, without
a u8 length prefix). For `cLen <0`, `-cLen` raw bytes include a NUL-terminated
UTF-8 string. Maximum decoded text is 1000 bytes. The payload is `12+abs(cLen)`
in current output, at most 1013 for 1000 raw bytes plus terminator. Reader checks
header/text bounds and UTF-8, rejects malformed messages by returning NULL, and
message-list readers skip them rather than necessarily aborting the whole game.
Extra payload bytes can be copied even when the length declares fewer bytes;
a strict new writer should emit exactly the declared extent.

## 12. Battles

`rtBtlData` followed by zero or more `rtContinue` records concatenates a battle
blob of `cbData` bytes. `WriteBattles` emits only battles containing the player
in `grfPlr`; host output omits them. Header BTLDATA, 14 bytes:
u16 battle id at 0; u8 player count at 2; u8 token count at 3; u16 player mask
at 4; u16 total byte count at 6; u16 planet id at 8; i16 x,y at 10,12.
Then come `ctok` TOK entries of 29 bytes, then events to the cbData boundary.

TOK fields (offsets relative to the token):

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | u16 | Object id. |
| 2–14 | u8 each | player, object class, design index, board coordinate `brc`, initBase, initMin, initMac, target token, cloak, jam, battle-computer percentage, capacitor percentage, beam-defense percentage. |
| 15,17,19,21 | u16 each | mass, shields, ship count, DV damage word. |
| 23 | u16 | target1, target2, tactic, target0: four successive nibbles. |
| 25 | u16 | dxyLim, dxyMax, speed, target count: four successive nibbles. |
| 27 | u16 | active bit 0, detector 1, torpedo 2, regeneration 3, moved 4, disengagement 5–9, dwt 10–13, moves-left 14–15. |

Each current BTLREC event: u8 token at 0, u8 destination `brcDest` at 1, i16
kill-entry count `ctok` at 2, u16 round bits 0–3 / disengagement 4–7 / attacking
token 8–15 at 4, then `ctok` eight-byte KILL entries. KILL is u8 target token,
u8 weapon flags, u16 ships killed, u16 shields, u16 DV. Event size is
`6+8*ctok`. Weapon flags are beam-low `0x01`, beam-high `0x02`, torpedo
`0x04`, missile `0x08`, no-hit `0x40`, deflected `0x80`; bits 4–5 have no
declared variant. Battle replay meaning/legality belongs to M14.

For blobs <1024 bytes the writer emits one record. Larger blobs split header
and tokens: first at most 34 tokens plus header (1000 bytes), then at most 35
tokens per continuation (1015 bytes); remaining events are grouped below 1024,
or split into 1023-byte chunks when one event is too large. Continuations need
not start with a complete event. A decoder concatenates before traversing.
`0xffff` battle sentinels and Win16 heap block rollover are in-memory transport
mechanics, not serialized native pointers. The pre-2.80 event layout is not a
current input format; the offline tutorial conversion upgrades it explicitly.

## 13. Scores and things

SCOREX (24 bytes): u16 flags at 0 (player bits 0–4, valid 5, victory bits 6–13,
winner 14, history 15); i16 rank at 2, overlaid with u16 turn when history;
i32 score at 4; i32 resources at 8; i16 planets at 12; i16 starbases at 14;
three u16 ship-category counts at 16,18,20; i16 total technology levels at 22.
Non-history records acquire current game turn for historical storage. Score
calculation and victory bit meanings are linked to M03/M15/M18.

A snapshot thing block begins `rtThing(2)` with i16 count, followed by count
`rtThing(18)` entities. Every entity begins u16 idFull: id bits 0–8, owner bits
9–12, type bits 13–15. I16 x,y at 2,4; ten union bytes at 6; u16 observation
turn at 16. Owner bits here are unsigned four-bit values, not PLAYER's signed
five-bit -1 sentinel. The current snapshot loader stamps the current turn.

Union layouts, offsets relative to byte 6:

| Type | Fields |
| --- | --- |
| 0 minefield | i32 cMines at 0; u16 known-player mask at 4; u8 mine type at 6; u8 detonate at 7; u16 current-visible mask at 8. |
| 1 mineral packet / salvage representation | u16 destination bits 0–9, warp 10–13, moved 14, include 15 at 0; three i16 mineral quantities at 2,4,6; u16 max weight bits 0–13 and decay rate 14–15 at 8. |
| 2 wormhole | u16 stability 0–1, last-move count 2–11, destination-known 12, include 13 (remaining bits unused) at 0; u16 known mask at 2, traversed mask at 4, partner id at 6; two trailing union bytes unused. |
| 3 Mystery Trader | i16 destination x,y at 0,2; u16 warp 0–3, include 4, unused 5–15 at 4; u16 known mask at 6; u16 reward bits at 8. |

Types 4–7 have no current declared variant. Host writes complete thing data;
player copies retain only the recipient's bit in minefield/wormhole/trader
knowledge masks and zero the trader's reward. Packets/traders/wormholes are
included by their visibility flags; minefields by ownership or current
visibility. Catalog/rule owners define mine types/rewards and packet behavior.
Tag 43 in an orders file is the four-byte detonation command, never this block.

## 14. AI history

`FWriteHistFile` concatenates the blob whose first u16 is cbAiHist, including
that length word; values <=2 produce no AI_DATA. Chunks are 1023 bytes until the
remainder fits. `FLoadGame` concatenates AI_DATA only when the owning player is
AI, otherwise skips it. The schema is selected by the owner's AI identity,
not by a subtype byte inside each record.

- Robotoid, Turindrone, Automitron and Rototill use AIHIST: u16 length at 0,
  i16 starbase count at 2, then 20-byte AISTARBASE entries: i16 planet, i16
  freighter count, eight i16 fleet IDs. Capacity is 64 entries; active length
  is `4+20*cStarbase`. Slots beyond each freighter count are unused storage.
  `ValidateStarbaseHistory` in [aiutil.c](../src/core/aiutil.c) repairs/rebuilds
  history from current ownership rather than treating every historical entry
  as authoritative.
- Cybertron uses u16 length followed by one u16 CYBERINFO per universe planet.
  Length is `2+2*game.cPlanMax`. Bits: last packet direction 0–2, built colony
  3, launched packet 4, packet target 5–6, needs scan packet 7, unused 8–15.
  Temporary CYBERINFOTEMP and later scratch regions in its allocation are not
  included. Evidence: `DoCyberAiTurn` in [ai4.c](../src/core/ai4.c).
- Macinti/Maid do not introduce a third emitted structured history layout in
  the inspected AI dispatch. Initial two-byte empty memory is omitted.

These bytes are AI decision memory, not a complete copy of the host or RNG.
The save comparator uses a companion `.mN` to select an AI schema; without it,
it can report payload differences without assigning the correct logical type.
The game's history loader does not bound the aggregate chunks as strictly as
a new decoder should; see [validation gaps](S01-game-file-validation.md).
