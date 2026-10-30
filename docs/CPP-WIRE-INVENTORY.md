# C++ wire and storage inventory

B04 introduces checked, little-endian byte operations and migrates `RTHISTHDR`.
B05 migrates production queue storage and its file/log byte boundaries.
B06 migrates order arrays, task-word views and full/partial waypoint records.
B07 migrates full/partial ship designs, hull slots and design-change headers.
B08 migrates fleet snapshots, ID sequences and signed ship-transfer logs.
B09 battle producers, loaders, continuations and UI replay use explicit byte views.
AI history and persistent/temporary Cybertron words use the same scoped byte
window primitives, with live mutation and signed domains preserved. Battle-plan
prefix codecs retain the two-byte deletion variant and four-byte full header.
The canonical record fields, bit positions, optional fields and malformed-input
observations remain in [S01 records](../spec/S01-game-file-records.md) and
[S01 orders](../spec/S01-game-file-orders.md). This document assigns migration
work; it does not define a competing file schema or stricter admission policy.

## Reproducible source inventory

`test_wire_inventory` emits `wire-inventory.json` in each test build directory.
It scans all owned core, UI, host, platform and wrapper source/headers (excluding
vendored upstream/reference inputs). Each entry carries its path, line and source
expression. It includes declarations/definitions alongside call sites, so counts
must not be reported as executable call counts. The report records:

- Every direct record/stream/native file operation and resource acquisition.
- Every byte-copy/comparison/initialization and size/offset expression. These
  include runtime copies, because names alone cannot distinguish wire storage.
- Every C-style/C++ pointer cast candidate, including LP-style pointer aliases.
  Opaque handles, callbacks, owned runtime objects and actual byte overlays all
  appear; a candidate is not evidence that it serializes an object.
- Every packing/alignment directive, bitfield declaration, anonymous aggregate
  and bit-operation source line. Logical/runtime bit operations are retained as
  candidates rather than silently filtered out.
- All 46 `RecordType` names, their numeric values and responsible checkpoints;
  all twelve historical variable-tail dispositions below (one active zero-array
  declaration and eleven migrated families);
  the unused zero-storage `_ctype` declaration/definition (B14).
- All 259 independent historical layout/prefix measurements. Those frozen C
  expectations remain unchanged while codecs replace native layout consumers.

The checker rejects an unassigned record name or zero-tail declaration. It is a
conservative source index, not a C++ alias-analysis or lifetime proof. Each later
family must migrate its direct consumers and review the indexed casts, copies,
bit operations and prefix expressions before retiring its old layout checks.
The report is generated, rather than checking in thousands of duplicated source
expressions that would become stale when unrelated lines move.

## Family ownership and remaining consumers

| Family / source boundary | Responsibility | Checkpoint |
| --- | --- | --- |
| History header read in `file.c`, write in `log.c` | Explicit two-signed-16 codec; exact four bytes; complete scratch backing preserves existing short-declared-payload behavior | B04 migrated |
| Production queues and queue log edits | PROD scalar bits, PLPROD runtime storage, RTCHGPRODQ byte prefix; allocation/capacity and forecast lifetimes | B05 migrated |
| Orders and order logs | ORDER/PLORD runtime items, 18/8-byte full/partial encoding, transport/edit/replay operations | B06 migrated |
| Hulls and designs | Four-byte HS codecs, explicit full/partial SHDEF prefixes and design-log words; fixed runtime slots and unchanged catalog IDs | B07 migrated |
| Fleets and ID lists | Explicit snapshot/count/cargo/damage codecs, zero-prefix byte ID views and signed ship-transfer logs; original split/merge traversal | B08 migrated |
| Battle and AI | Explicit TOK/KILL/BTLDATA/event codecs and scoped byte views; AI history/word views preserve live traversal and original scratch/opaque bytes | B09 migrated, full qualification complete |
| Messages and logs | Player/generated messages and log/control prefixes use explicit codecs; strings/filters retain existing byte encoding. Cargo transfer quantities use explicit 8/16/32-bit byte views with original lookup/read ordering | B10 migrated, full qualification complete |
| Remaining fixed game records | GAME/PLAYER/PLANET/THING/SCOREX and preferences; unknown tags and reserved bits | B11 |
| Record stream boundary in `file.c` / `save.c` | Explicit HDR/BOF words, XOR and EOF scalars; partial read/write and callback effects preserved | B11 stream families locally qualified; entity/resource work remains |
| Wrapper resources in `resource.c`, `dialog.c`, `accelerator.c`, `image.c`, `dib.c` | Menu/dialog/DIB/icon/cursor/accelerator bytes; resource ownership stays module-scoped | B11 |
| Remaining runtime copies/casts and packed storage | Generic PL, heaps/pools, UI/control payloads, pointer/stride identities and aliases; no wire assumption | B12, with family prerequisites |
| FreeType/Fontconfig/Cairo/zlib/SDL and actual OS SDK layouts | Genuine foreign seams; external decoders remain in their adapters; owned byte parsing still belongs above | B11 adapter review / B14 isolation |
| Global stream/game/RNG/UI roots | One session owner, deterministic reset/shutdown and callback stop order | B13 |

Direct `ReadRt`/`WriteRt` and raw `RgFromStream`/`RgToStream` consumers remain
indexed until all families have moved. `RawLoad16/32` and `RawStore16/32` still
serve both native heap metadata and file bytes; replacing them globally would
conflate those contracts. Raw typed aliases in readers, log replay and variable
storage must move with their family, rather than being hidden by a generic
`Read<T>` or a `span<T>` over unstarted/unaligned objects.

## Variable tails

Counts below describe current storage, not proposed new validation. Unused
capacity, arithmetic widths, truncation and failure receipts must remain as they
are while the runtime representation changes.

| Type.member | Prefix / length / lifetime | Checkpoint |
| --- | --- | --- |
| BTLREC.rgkill | Six-byte wire prefix + ctok × eight-byte KILL; scoped arena byte views | B09 battle migrated |
| BTLREC26.rgkill | Six-byte legacy prefix + ctok × eight-byte KILL; explicit legacy conversion | B09 battle migrated |
| MSGPLR.rgbMsg | Typed runtime header + byte array over physical slot capacity; native next pointer stays runtime-only | B10 player family migrated |
| PL.rgb | Four-byte runtime prefix + iMax × cbItem; iMac live items; pooled/reset lifetime | B12 |
| PLPROD.rgprod | Four-byte runtime prefix + iprodMax × four-byte PROD; iprodMac live items; rtProdQ wire prefix is zero | B05 migrated |
| RTCHGPRODQ.rgprod | Two-byte ID prefix + payload-derived four-byte PROD items | B05/B06 |
| RTSHDEF.rghs | Seventeen-byte wire prefix + chs × four-byte HS | B07 |
| RTFLEET.rgb | Fourteen-byte prefix followed by conditional counts, cargo and damage fields | B08 |
| RTFLEETIDS.rgid | Zero-byte prefix + payload-derived signed-16 IDs; never sizeof an empty C++ object | B08 |
| RTPLRMSG.rgbMsg | Explicit twelve-byte wire prefix + abs(cLen); native pointer is encoded as the existing zero marker | B10 player family migrated |
| PLORD.rgord | Four-byte runtime prefix + iordMax × sizeof(ORDER); iordMac live items; wire items are 18/8 bytes | B06 migrated |
| BTLDATA.rgtok | Fourteen-byte prefix + ctok × 29-byte TOK, then variable BTLREC events through cbData; scoped arena byte views | B09 battle migrated |

## First codec and compatibility evidence

`wire.h` accepts byte spans and exposes explicit unsigned/signed little-endian
primitives. A failed primitive leaves its cursor and output unchanged. Bulk
copies allow overlap and empty buffers; scalar reads snapshot before publishing
an overlapping output. No native struct dump, padding or pointer enters a codec.
`recordcodec.h` decodes the history header only after both fields are available
and encodes a by-value snapshot after checking the four-byte output capacity.

The history caller intentionally supplies its complete reusable scratch backing,
not the declared `hdrCur.cb`: the old reader consumed that prefix even for a
short declared record. Tightening admission would be a separate behavior change.
Independent little-endian vectors, all 65,536 paired signed-16 values, unaligned
slices, truncation, trailing bytes, sentinels, empty spans and input/output alias
cases exercise the codec without deriving expected bytes from native layout.

The B05 production migration retains the independent four-byte PLPROD and
two-byte RTCHGPRODQ measurements: their sizes are unchanged, and native PLPROD
pointer arithmetic still has its old stride. Their zero-array members are gone.
The report keeps their historical dispositions under `migrated_tails` and rejects
a reintroduced zero-array declaration. PROD's wire fields, reserved bits and
incomplete-row behavior are covered by independent codec vectors and real log
replay tests rather than a native struct dump. Runtime queue copies retain their
existing four-byte element storage and pooled lifetimes.

## Order and task-word migration

ITEMACTION now stores a uint16 word with explicit 12/4-bit masks. ORDER owns one
real TASKXPORT array; mine-laying, patrol and player-transfer aliases are scoped
references to those actual words. Value assignment copies the selected words,
including const-to-mutable copies, while leaving the remaining payload unchanged.
The PLORD accessor borrows a real ORDER array started over its existing physical
pool slot. Counter widths, rounding, address stability/reallocation and 18-byte
item stride remain unchanged.

The eight-byte order header, full eighteen-byte order, four-byte waypoint prefix
and signed ship/index words use explicit little-endian codecs. Header-only
encoding never consumes omitted task words. File reads keep the old zeroed
reusable-buffer prefix for short declared records, and log writers trim the same
trailing zeros after encoding. Replay keeps lookup/read predicate order, signed
index/task admission, shifts before copying, short-record zeroing, neighboring
row effects and final fNoAutoTrack clearing.

Legacy order log edits can address neighboring runtime bytes. Their adapter
borrows the owning allocation and uses aligned ORDER temporaries; it never
invents typed objects at those offsets. Physical copies outside that allocation
remain outside the original defined domain. No new logical index/nonnegative
task policy is introduced. Tests retain bounded negative-index and oversized
neighbor mutations, including restoring deliberately changed test metadata.

ITEMACTION/TASKXPORT/ORDER/PLORD/RTWAYPT/ZIPORDER now require native alignment two
so references to uint16 task words are valid. Their sizes remain 2/10/18/4/22/24
bytes. The frozen C inputs and all 259 size/prefix measurements remain intact;
the comparator explicitly checks these six new runtime alignments and keeps
every other alignment comparison unchanged. These types no longer supply file
bytes by native layout. Independent wire vectors check their byte contracts.


## B07 design storage and wire boundary

A full design is decoded from a seventeen-byte prefix plus four-byte HS slots
into the existing fixed sixteen-slot runtime HUL. A partial design places its
name marker at offset six; the old marker-to-hul.chs effect remains, without
applying full-slot validation to partial names. Full/partial name selection,
compression, bitmap/category/engine repairs and prior-knowledge retention remain
in their callers. The RTSHDEF metadata type has no trailing array and no wire
overlay consumers; neither its sizeof nor native flag layout defines file bytes.
Design change replay preserves mode/index admission and partial effects, including
failed replacements and unknown modes. Only formerly indeterminate output junk
bits acquire a defined zero value; incoming junk bits decode unchanged.


## B08 fleet storage and wire boundary

The fleet header has fourteen wire bytes. Its count presence mask selects
ascending byte/word counts; cargo and damage suffixes are decoded by numeric
width/mask, with existing detail and clamp rules. FReadFleet still consumes its
full reusable scratch backing, preserving short-declared records and the partial
state published before tag errors. No byte view survives into subsequent record
reads. Runtime IDs, damage/mass, power/movement and direction words have explicit
value accessors; no inactive union determines serialized bits.

RTFLEETIDS is now an aligned, call-scoped span of bytes. Its first member stays
at offset zero, matching the frozen prefix measurement. The runtime view's size
is unrelated to the explicitly zero-byte wire prefix. IDs are decoded into
aligned signed scalars; no integer span aliases packet bytes. Split/merge keeps
input order, duplicates and odd trailing bytes, and ship transfers retain their
seven-byte prefix, signed deltas, lookup/read order and two-pass mutation order.


## Historical minimum transfer records

The generated inventory separately records the four historical one-element
quantity fields. RTXFER/RTXFERX/RTXFERL retain their seven-/eight-/ten-byte
minimum metadata measurements; RTXFERF retains its nine-byte measurement.
Their actual wire prefixes are six/six/six/seven bytes, followed by mask-selected
signed quantities of width one/two/four/two. No production consumer traverses
these fields as variable arrays: explicit codecs/views read/write the bytes.
The first three families migrate in B10; ship transfer migrated in B08.
These declarations retain frozen measurements while B11 finishes packing/layout
retirement; they are not a new runtime variable-storage scheme.


B11 stream-header family: explicit two-byte codecs now serve file/memory logs,
tutorial record traversal and library inspectors. Header lengths/types use
numeric little-endian bits, independent of runtime HDR packing. Zero-/one-byte
read failures publish the merged old/new header before error callbacks; payload
partial publication, callback-mutated strides, unknown tags and length wrapping
remain. Remaining BOF/entity/resource boundaries still require migration.


B11 BOF family: explicit sixteen-byte codecs replace raw preamble dumps in
writers and library inspectors. Scoped byte fields preserve per-read scratch
publication and version/mark/import short circuits. Signed player/salt bits
retain their domains and encryption inputs. RTBOF version aliases now compute
the numeric word from its actual fields; VERS casts of wVersFile are removed.
The historical RTBOF/VERS size contracts remain independently measured.


B11 encryption words: XorFileBuf now uses explicit four-byte little-endian
reads/writes over its physical byte buffer. No native int32_t object is invented
at the byte address. Trailing-byte operations and exact XOR-seed consumption
remain unchanged. This retires the encryption loop's raw scalar alias only;
other RawLoad/RawStore sites remain assigned to their own wire/runtime family.


B11 EOF values: all owned two-byte EOF producers encode numeric words explicitly;
turn-date and race-checksum readers decode little-endian values from the same
scratch backing. Empty EOF records retain their existing calls. Native copies
used for THING counts remain assigned to the forthcoming THING record family.
