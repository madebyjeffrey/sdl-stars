# S02 companion: Profile grammar, preference keys and layout restoration

Status: source-derived contracts, inspected at
`e2eae18cff0f990db20415c1c2262aed2022f95b`, 2026-10-24.
Stories and interaction/quality requirements remain in
[S02](S02-application-preferences-and-window-layout.md).
Canonical definitions: [profile grammar](dafny/S02-profile.dfy),
[profile I/O](dafny/S02-profile-io.dfy), [host grammar](dafny/S02-host-profile.dfy),
[shared UTF-8 prefix](dafny/S04-utf8-prefix.dfy),
[layout and scalar normalization](dafny/S02-layout.dfy),
[shared preference rules](dafny/S02-preferences.dfy),
[selection and recent-file state](dafny/S02-state.dfy), and
[preset codecs](dafny/S02-presets.dfy).

## Boundary and evidence

This inventory covers all application calls to `GetPrivateProfile*` and
`WritePrivateProfile*` in `src/`, plus `FReadHostIni`. Key spelling comes from
[text/strings.txt](../text/strings.txt); it is not inferred from labels on screen.
The current bare profile filename is **StarsUtf8.ini**. Unused string IDs such as
SoundFX, Music, Tracks and GlobalSettings do not establish live preferences.

Sources: [init.c](../src/ui/init.c) `ReadIniSettings`, `GetIniWinRc`,
`ReadIniTileSettings`; [mdi.c](../src/ui/mdi.c) `WriteIniSettings`,
`SetWindowIniString`; [fileui.c](../src/ui/fileui.c) `AddMRUFile`;
[libraryui.c](../src/ui/libraryui.c) recent-file deletion and Continue;
[host.c](../src/host/host.c) `FIniValue`, `LIniInt`, `FReadHostIni`;
[profile.c](../compat/win32/src/profile.c); [field widths](../src/core/structs.h).

Formal strings denote C string contents with ASCII syntax. For a byte buffer,
stop at its first NUL before invoking the string model. The source uses C locale
classification/case comparison; no `setlocale` call was found in the application
or wrapper. Non-ASCII locale-dependent equivalence is not invented by `Fold`.
UTF-8 validation and byte-prefix truncation remain S01/S04 adapters; Dafny string
length must not be substituted for UTF-8 byte length. Parsing models receive the
already bounded value. Allocation, errors, enumeration buffers, filesystem effects
and the host's distinct parser have the explicit boundaries below.

## Desktop profile grammar and ordered updates

`Parse` implements `LpProfileLine`: trim C whitespace; a leading `[` followed by
a closing `]` creates a section whose interior is trimmed, ignoring text after
`]`. Otherwise a nonempty line not beginning with `;` and containing `=` is a key
split at its first `=`. Thus `#key=value` is a key, and `[bad=x` can be a key;
there is no universal hash-comment or malformed-section rejection rule.
Unrelated raw lines are retained separately from the parsed view.

`Lookup` finds the **first** case-insensitive matching section and the first
matching key before the next section. A later duplicate section is not searched
when the first lacks the key. `Unquote` removes exactly one matching pair of
single or double outer quotes after trimming, preserving spaces inside quotes.
Missing-key defaults lose trailing ASCII spaces only (`DefaultText`). Returned
values are copied to the longest whole UTF-8 prefix fitting capacity minus one.
Section/key enumeration uses a double-NUL list and its own nSize-2 accounting;
that buffer algorithm is modeled separately by `List`/`ListLoop`.

`Integer` models `GetPrivateProfileIntA` after its 128-byte value-buffer read.
An empty returned string uses the default, reduced to uint32. Otherwise skip
leading whitespace, accept one sign and optional `0x`/`0X`, then consume decimal
or hexadecimal digits until the first invalid digit. Accumulation and negation
wrap modulo 2^32. Leading zero alone does not select octal. Nonempty nonsense,
a sign alone and `0x` alone produce zero, not the default. Whitespace preserved
inside quotes can therefore differ from a truly empty value.

`Change`/`ChangeKey` model ordered list edits after valid API arguments and a
successful load: replace/delete only the first matching key; insert a missing
key before the next section; append a missing section; delete the first section
and all following non-section lines in that block. Deleting an absent item is a
successful no-op without saving. Duplicate later keys can become visible after
deleting the first. New raw rows are reparsed, so whitespace/quote syntax still
applies. The API rejects invalid UTF-8, empty section, section `]`, forbidden
newlines and key `=`; it does not promise arbitrary strings round-trip literally.
A null key deletes a section; a null value deletes a key; three nulls do not flush
a cache and return false.

`FLoadProfile` rejects invalid UTF-8 or files exceeding 8 MiB and removes a BOM
only from the first line. Missing file is an empty profile; other load failures
also leave no loaded rows. Crucially, the public **reader ignores the load
boolean**, returns fallback content, and retains the load error. The public
writer checks it and refuses to overwrite malformed content. Startup callers do
not inspect each last-error result. Rejection by the loader does not imply a
visible startup error or that no default is returned.

For an actual write, create directories only under the source's bare-name test,
then use a same-path temporary file; emit a UTF-8 BOM and raw rows, inserting a
newline between adjacent rows when needed; flush, fsync, close, rename. Failure
removes the temporary path on the cleanup attempt. POSIX preserves existing mode
bits. The process-local spin lock covers each read/write, not the entire sequence
of settings operations or another process. No directory-fsync/crash-durability
claim is made. Allocation/syscall outcomes and concurrency still require native evidence;
the save control and serialization contracts below do not make `Change` an
atomic filesystem proof.

## Profile buffers, validation and save outcomes

The [I/O definitions](dafny/S02-profile-io.dfy) use the shared
[S04 UTF-8 prefix contract](dafny/S04-utf8-prefix.dfy). `Count`/`Prefix` implement
`CchUtf8Prefix` on bytes, stopping before the first malformed sequence, NUL or
code point that would exceed the capacity. `PrefixValid` proves the returned
prefix is valid non-NUL UTF-8 even when the suffix is malformed. `BytesOf` and
`StringOf` map **byte values** to mathematical characters and back; they do not
decode UTF-8 into Unicode scalars or make character length a byte budget.

`Scalar` models the caller's entire output buffer, including untouched bytes.
A null output pointer or zero capacity returns zero without modification. A
nonempty buffer first gets a NUL at byte zero; a path-resolution failure returns
that initialized buffer. Otherwise copy at most capacity-1 complete UTF-8 bytes
and terminate; the remaining bytes retain their previous contents. Selected
value/default text and load/error status come from the profile contract.

`SectionNames`, `KeyNames` and `KeysInBlock` preserve duplicate names, ignore
empty names and stop key enumeration at the next section of the first matching
block. Their byte encodings feed `List`/`ListLoop`. The list starts with two NULs
where capacity permits. Each name reserves two bytes, copies a complete prefix,
then writes its terminator and the following sentinel. A truncated name clears
the remaining buffer. **The reported count is capacity-2 on truncation or exact
fill, even when UTF-8 alignment leaves fewer non-NUL bytes.** For a three-byte
buffer and a two-byte name é, the output is three NULs and the return count is 1.
For no names and capacity three, the third byte is untouched. Consumers must not
interpret the return count as a count of Unicode characters or non-NUL bytes.

`WriteArguments` represents the actual section/key/value checks on C-string
contents: validate non-NUL UTF-8, require a nonempty section, exclude CR/LF and
section `]`, exclude CR/LF/`=` from a non-null key, and exclude CR/LF from a
non-null value. Empty key and value are allowed; `[` in section and `=` in value
are not rejected. Filename/path validation is a separate S04 boundary.
`CheckRequest` distinguishes the all-null cache-flush request (false without a
new error) from invalid arguments (false with ERROR_INVALID_PARAMETER) and valid
arguments. A successful return does not clear an inherited last-error value.

`Serialized`/`Rows` define the complete byte stream for successfully emitted
rows: a single initial UTF-8 BOM followed by raw rows, inserting LF only between
rows when the preceding nonempty raw row lacks LF. No extra final LF is required.
Loading removes the original initial BOM before creating these raw rows.

`Stages`/`Execute` specify FSaveProfile's short-circuit order: temporary-name
allocation, temporary file creation, optional POSIX mode preservation when stat
succeeds, stream conversion, BOM, raw rows/newlines, flush, fsync, close, rename.
The `Stages` parameter `targetExists` denotes a successful stat result, not a
separate existence precheck: an unsuccessful stat skips mode copying. The row
stage aggregates actual row/newline calls; with no rows its supplied result is
success. These are input-mapping obligations for the native adapter.
The first failed stage prevents later stages; cleanup close/unlink calls are
separate from successful progress. `Execute` accepts the external operation
results and proves success exactly when every executed stage succeeds. `Save`
is the corresponding first-failure summary. It does not synthesize syscall
results or prove those results reflect a real disk.

| First failure | Return / successful publication | Cleanup and last-error behavior |
| --- | --- | --- |
| Temporary-name allocation | false / none | No unlink attempt; set out-of-memory. |
| mkstemp | false / none | Attempt unlink of temporary path; no newly created file is assumed; assign mapped error. |
| POSIX mode copy or fdopen | false / none | Close descriptor, then attempt unlink; assign mapped error. |
| BOM write | false / none | Close stream and attempt unlink; **no explicit SetLastError** on this branch. |
| Row/newline, flush or fsync | false / none | Close stream, then attempt unlink; assign mapped error. A row failure skips flush/fsync, and a flush failure skips fsync. |
| fclose | false / none | Skip rename, attempt unlink, assign mapped error. |
| rename | false / none | Stream already closed; attempt unlink, assign mapped error. |
| All stages succeed | true / successful rename | No unlink; no explicit clearing of last error. |

`temporaryMayRemain` distinguishes failed unlink from an attempted cleanup;
`published` means a successful rename return, not crash durability. On a failed
stage a partial temporary stream is possible. No directory fsync, cross-process
lock, competing-writer isolation or rollback of earlier settings calls is
implied. The public writer's load, edit-allocation, directory-creation and
absent-delete branches remain explicit outer-call adapters; a successful absent
delete returns before this save machine. Test doubles can validate the control
order but cannot qualify real syscall failures or a crash/restart sequence.

## Headless profile grammar

The [host definitions](dafny/S02-host-profile.dfy) are canonical for
`FIniValue`/`LIniInt`; M17 consumes these contracts instead of assuming desktop
parser equivalence. `ReadLine` specifies a bounded line; `Lookup` traverses original LF positions,
with checked `LastLine`/`HeadLine` composition facts, then takes at most
255 complete UTF-8 bytes of each line. It trims CR/space/tab from the right only.
A section marker must begin at byte zero, and its interior is not trimmed. A
leading `[` with no closing `]` is skipped **without changing the previous
section state**. Duplicate matching sections can be searched until a key is
found. Keys are right-trimmed only; values lose leading spaces/tabs and keep
quotes. `capacity` includes the output NUL; a found value is independently
bounded to capacity-1 complete UTF-8 bytes. Truncating a long line does not turn
its unprocessed suffix into another line.

`Decimal` follows the leading decimal/sign/whitespace behavior used by `atol`,
without desktop hexadecimal recognition or quote removal. `Integer` uses the
default only for a missing key; a found empty string gives zero. It classifies
values beyond the target platform's signed long range as `OutsideLongDomain`,
an **unspecified C-input domain**, not a runtime error return. Within that range,
the source return narrows to int32; the model uses the existing signed-width
convention of the supported compilers. Out-of-range signed narrowing is an
implementation-defined C conversion, not a portable arithmetic guarantee. `long64` identifies the native long domain (LP64 versus LLP64/32-bit).
The 32-byte temporary value buffer in `LIniInt` bounds input before conversion.

`RemoveBom` models the one leading BOM removed after whole-file UTF-8 validation.
Unlike the desktop loader, the host has no 8 MiB guard and attempts one allocation
from file length; seek/tell/allocation/read failures and the exact loader/native
error contract remain explicit adapters. `FReadHostIni` reads only Logging,
DefaultPassword, NewReports, NoHostNames and Backups. Its booleans use `!=0`, so
host Logging=2 is true while the desktop's one-bit assignment makes it false.

## Active key inventory

Notation: **I16/U16** means narrow before later checks; **bit** means low bit,
not `value != 0`; **NZ16** means nonzero after 16-bit narrowing. Numeric desktop
input first follows `Integer`. “Always” means `WriteIniSettings` attempts the
write; that function ignores individual write results. A dash means it has no
write in that function, not that a file cannot be edited externally.

| Section / key(s) | Default and destination / normalization | Persistence |
| --- | --- | --- |
| Windows / Main | `X`, 20-byte buffer; `ReadPlacement`; frame values are x,y,width,height. Invalid uses default-position sentinels, maximized=true. | Always; M/I/R plus four `%04d` values from normal placement. |
| Windows / ReportFleetWin, ReportEFleetWin, ReportBtlWin, ReportPlanWin | `X`, 20-byte buffer each; valid four values are left,top,right,bottom; assign position and right-left/bottom-top. Invalid leaves prior report position/size. | Always; M plus position and position+size. |
| Windows / Resolution | 0 → I16; zero plus <=4 colors or smallest screen mode causes advisory. | Always; low bits encode current color/screen limitation. |
| Windows / Layout | 1 → U16, compared as I16; `Layout`, 0–2. | Always. |
| Windows / Style1Width, Style2Width | 396 → I16 pane width; `PaneSize`, 10–2000. | Always. |
| Windows / Style1Height, Style2Height | 110 → I16 message height; `PaneSize`. | Always. |
| Windows / Style1Height2, Style2Height2 | 192 → I16 minimum pane height; `PaneSize`. | Always. |
| Windows / Toolbar | 1 → I16 then nonzero. | Always, current flag. |
| Windows / PlanetTiles, ShipTiles | `X`, 20-byte buffers, six/seven existing tiles; `ReadTiles`. | Always, `WriteTiles` on each current list. |
| Windows / Selection | `N`, 20-byte buffer. Under 3 bytes disables selection. Leading P/S/E maps to 1/2/4, other to none. Second byte B–Q sets player 0–15; invalid disables selection. Only non-none parses suffix with `atoi` into I16 object ID. | Always from current kind, player+'B', decimal object ID. Loading does not validate object existence; M07 owns that lookup. |
| Windows / Message | 0 → I16 `ini.iMsg`. | Always from `iMsgCur`. |
| Windows / GameID | `0`, 10-byte buffer. Start zero, multiply by 16 per byte; only digits/lowercase a–f contribute. Other bytes still shift. Signed int32 overflow is outside the defined C arithmetic domain. | Always, lowercase hex `game.lid`. |
| Windows / ScanZoom | 4 → U16; 1–9 maps to -4..4; outside range retains prior zoom (`Zoom`). | Always, zoom+'5' as one character. |
| Windows / ScanFilterV25, ScanEFilterV25 | 0 → U16 ship/enemy masks; the ship mask may subsequently clear on invalid ScanModeV25. | Only `gd.fChgScanner`. |
| Windows / ScanMines | 15 → U16 low four bits. | Only `gd.fChgScanner`. |
| Windows / ScanRadar | 100 → U16; above 100 becomes 100 (`Radar`); negative raw numbers wrap before check. | Only `gd.fChgScanner`. |
| Windows / ScanModeV25 | 224 → U16; if `(w & 0xc00f) >= 6`, clear mode and own-ship filter; else retain w (`Scan`, `ShipFilter`). Enemy filter unchanged. | Only `gd.fChgScanner`. |
| Windows / MineralScale | Existing `cMinGrafMax` → I16; outside 100–30000 becomes 5000 (`MineralScale`). | Always. |
| Files / Logging | 0 assigned directly to a one-bit field (`BitFlag`); desktop 2 therefore disables it. | — |
| Files / Wait | 0 assigned directly to one-bit `ini.fWait`. | Active player only; first byte set from timer presence. Source does not explicitly terminate the buffer at byte 1 here; suffix is retained from preceding MineralScale formatting. `WaitText` preserves this behavior: prior 5000 and active timer produce 1000, whose low-bit reload is false. |
| Files / File1..File9 | `.`, 256-byte buffers; fewer than 4 returned bytes clears entry, then compact in order. File1 separately sets startup flag/base before compaction. | `AddMRUFile` writes all nine; shutdown writes File1 for active player; library deletion writes affected list with its Continue guard. |
| Files / Turn | `game.turn` → U16 `ini.turn`. | Active player and `gd.fWriteTurnNum`; clear flag after attempted write regardless of result. |
| Misc / DefaultPassword | Empty, 16-byte buffer; whole UTF-8 prefix <=15 bytes. | —; never include its contents in diagnostics. |
| Misc / Progress, NewReports, NoHostNames | 0 → U16 then nonzero (`Nonzero16`). | — |
| Misc / Backups | 1 → I16, 1–999 else 1 (`ConfiguredBackups`). | — |
| Misc / ReportPlanFld, ReportFleetFld, ReportEFleetFld, ReportBtlFld | -1 → uint32 visible mask. | Only `gd.fChgReports`, writing LOWORD; upper 16 bits are not preserved by this path. |
| Misc / ReportPlanSort, ReportFleetSort, ReportEFltSort, ReportBtlSort | 0 → I16; column low byte, ascending bit 8. No column-range validation here. | Only `gd.fChgReports`, column OR ascending bit. |
| Misc / ReportDefGraph | 7 assigned to a **four-bit** field, then values >7 become 7 (`Graph`); raw 16 becomes 0. | Only `gd.fChgReports`. |
| Misc / VCRSpeed | 1 → I16 without range clamp. | Always. |
| Misc / HistoryInfo | -1 → U16 installation date. If current encoded date is smaller, replace stored date; compare current against U16(stored+21) (`Installed`, `TrialOver`). Current date is month*31+day+two-digit-year*372, narrowed U16. | Always, current stored date. Clock/date formatting belongs to S04. |
| Fonts / Arial, ArialBold, ArialItalic, ArialBoldItalic | Empty, 80-byte buffer each. Only returned lengths 5–31 bytes replace existing font name; others retain it. | — |
| ZipOrders / ZipOrders1..ZipOrders4 | Empty, 80-byte buffer each; clear presets, then accept 20–32 bytes with first 20 all a–p. Five groups: action nibble then three little-endian quantity nibbles; remaining <=12 bytes are name. | Only `gd.fChgZipOrd`; valid serializes action/quantity/name, invalid writes empty. M10 owns application semantics. |
| ZipOrders / ZipOrdersP1..ZipOrdersP5 | Empty, 80-byte buffer; clear presets, then accept 3–64 bytes, count byte-'a' in 0–12, first 2+4*count bytes all a–p, remainder name <=12 bytes. First byte !=a disables research. Four little-endian nibbles make each PRODQ1; quantity >1020 becomes 1, item >=7 becomes mine(0). Finally force P1 name to localized Default and valid=true even after rejection. | Only `gd.fChgZipProd`; valid encodes flag/count/packed rows/name, invalid empty. M09 owns queue use. |
| Library / ContinueDeleted | 0; any nonzero uint32 read disables Continue in `FFindContinueLibraryGame` path. | Delete marks 1; successful player launch deletes key. This guard prevents MRU compaction from resuming an unrelated game. M01 owns workflow. |

`AddMRUFile` compares case-insensitively with current first entry; otherwise it
searches entries 1–7, using slot 8 as the insertion/drop boundary, shifts entries
and writes all nine. Library deletion has different semantics: if File1 was
removed it does not promote another entry into Continue. Memory may already be
changed when a later per-key write fails; no all-nine transaction is implied.

## Selection, identity and recent-file state

`ReadSelection` consumes the profile's already bounded 19-byte value. A value
shorter than three bytes clears only `grobjSel`, retaining player and object.
For longer values, a valid B–Q player byte updates `idPlayer` even when the kind
is N/unrecognized. An invalid player byte clears the kind but leaves the old
player. The old object survives every no-selection branch. Otherwise `atoi`
parses the suffix (whitespace, optional sign, decimal digits, trailing text
ignored) and native I16 narrowing follows. `OutsideAtoiDomain` marks a result
outside 32-bit `int`; it is not an implemented error or a guaranteed unchanged
state. The already decoded kind/player may have changed before that conversion.

`ReadIdentity` consumes at most nine bytes. Every byte multiplies the accumulator
by 16; only 0–9 and lowercase a–f add a digit. Thus `1A` gives 16 and `1a` gives
26. `IdentityLoop` checks the multiplication and addition separately against
INT32_MAX. `80000000` crosses the defined signed-arithmetic domain; the model
does not invent unsigned wrapping. `WriteIniSettings` formats the game identity
as lowercase hex, but a universal read/write identity round trip is consequently
not claimed. Its formatting adapter remains open.

These are persisted preference fields, not validated live-object references.
`RestoreSelection` in [mdi.c](../src/ui/mdi.c) checks game/player identity,
looks up fleets/owned planets, falls back to the home planet or another object,
and restores eligible messages/tutorial effects. M07/M06 own that consuming
selection lifecycle, M15/M16 its message/tutorial handoffs. The
[M07 controller](M07-galaxy-map-and-navigation.md#61-restoring-selection-after-opening-a-turn)
now specifies dispatch, fallback and post-callback continuation, using S06 lookup
algorithms and M15 bidirectional traversal. Complete selection/editor, title and
tutorial callback effects and selection formatting remain D/A work; passing
`ReadSelection` does not establish them.

`LoadRecent` takes the nine bounded File values and the separate, earlier
File1 read used for startup. It derives startup/base from that **earlier File1
result**, retaining the old base when it has fewer than four bytes. File1 is
read twice; the model permits those calls to return different results. It then clears short entries and packs remaining paths in order, padding
to nine empty/nonempty slots. A valid File2 can become the first MRU entry while
startup remains false. Allocation failure and caller buffer lifetimes remain
native adapter obligations.

`AddRows`/`Add` model `AddMRUFile`: no storage or a case-insensitive match at slot
zero causes no writes. Otherwise search slots 1–7, use slot 8 when none matches,
shift the prefix and put the new path first. Deeper duplicates are not globally
deduplicated. Memory changes before nine unconditional write attempts; failures
do not stop later writes and the void function does not report them. Its result
record's `success=true` denotes reaching that return, not successful persistence.
The supplied name is filename + dot + extension, fitting the 256-byte buffer;
unbounded source concatenation has no specified recovery for oversized input.

`Removed`/`DeleteWrites`/`Delete` model `FDeleteLibraryGameRecentPaths`. Native
`FPathUnderLibrary` results are supplied per slot. If slot zero was removed,
retain all remaining positions, including holes; otherwise compact and pad.
Each row enters live memory **before** its key write. A false write stops the
loop, leaving that memory row changed while the corresponding persisted row
retains its earlier value. After nine successful writes, clearing File1 is
attempted once more if the first slot was removed; failure of this redundant
write still returns false. `DeleteSuccessInstalls` proves successful completion
installs the planned suffix. Formatting failure before the write loop changes
neither list; absent MRU storage returns true immediately.

The modeled `disk` is the nine persisted key values under isolated, atomic
per-key success/failure outcomes from the profile contract. It is not a proof of
OS durability, path classification, multi-process isolation or a nine-key
transaction. M01 owns game deletion and `ContinueDeleted`; its metadata-based
Continue selection is distinct from this File1/MRU list. Source-extracted
[state witnesses](../tests/spec/check_s02_state_source.py) cover parsing,
compaction, insertion and every deletion-write failure stage with operation
doubles, including memory/disk disagreement after failure.

## Transport and production preset codecs

`ReadTransport` validates 20–32 bytes and the first twenty a–p characters,
then decodes five action/quantity groups and the remaining name. An action uses
the first nibble; a 12-bit quantity uses the next three, least significant first.
The source's action bits ORed into a quantity expression do not survive the
12-bit quantity assignment. This accepts all action nibbles 0–15; it does not
establish that M10 can execute every action. Rejection represents the cleared
slot (five zero actions, empty name, invalid). `WriteTransport` emits the inverse
or empty text when invalid; `TransportRoundTrip` proves the valid, bounded-name
round trip. [M10 command contracts](M10-fleet-operations.md#311-applying-transport-presets)
now specify apply/edit/cancel and bounded transport control refresh; full host
task eligibility/execution remains M10/M19 work.

`ReadProduction` validates 3–64 bytes, a count in 0–12, an a–p prefix of length
2+4*count and a name of at most twelve bytes. The first prefix character is a
boolean test against a, so b through p all disable research. Four low-to-high
nibbles decode each 16-bit `PRODQ1`. `Normalize` extracts the low-six-bit item and
high-ten-bit quantity: quantity above 1020 becomes 1, and item 7 or higher becomes
mine (0). Active rows are modeled; unused native rows remain zero after clearing.
`NormalizeIdempotent` proves repeat normalization stable. `ProductionRoundTrip`
proves writing and reading produces these normalized rows, provided the encoded
length is at least three. It does not promise an unnormalized row survives.

A valid empty-name, zero-row preset writes `aa`, which the loader rejects for
being too short. After every load, `LoadProduction` forces P1 valid and replaces
its name with localized Default, preserving other successfully parsed fields;
after rejection those fields remain cleared. The localized name is an S03
input that must fit the native 13-byte buffer. Storage reads/writes **five**
production slots; the [M09 UI](M09-production-management.md#32-templates-and-automatic-policies)
offers/snapshots the first **four**, including the default. The fifth stored
slot must not be described as a fifth UI choice. M09 now specifies preset
application, payload editing, nested rename and outer snapshot cancellation in
[its command contract](M09-production-management.md#32-templates-and-automatic-policies).
In particular, customization Cancel restores four slots but not the dirty flag;
the next preference write can therefore run even after rollback.

`WriteAttempts` records the four transport and five production writes gated by
their dirty flags. Individual failures are ignored; these writer blocks do not
clear their dirty flags. Actual per-key publication uses the profile contract.
Writer projections require bounded native state: five transport actions,
production count 0–12, boolean research flag and NUL-terminated names fitting
twelve bytes. They do not specify corrupted out-of-range globals.
The [source-extracted preset checks](../tests/spec/check_s02_presets_source.py)
exercise both source blocks and source-defined packed structures, including all
65,536 production words. Names are byte strings after profile UTF-8 truncation;
these codecs do not replace S04's byte-capacity contract with character counts.

## Layout state transitions

`ReadTiles` represents the entire `ReadIniTileSettings` loop and trailing pass.
Start with selected-prefix length zero and column zero. `*` changes column to one
and further stars do not increase it. ASCII letter value chooses ID 0–25; case
sets expanded/popped state, not whether the tile exists or is visible. Search only
the unused suffix. A match receives column/popped and **swaps** with the first
unused slot, then extends the selected prefix. Unknown IDs and repeated consumed
IDs do nothing. Thus `[A,B,C]` with format `C` becomes `[C,B,A]`, not `[C,A,B]`.
After parsing, raise every remaining tile's column to at least the final column.
Other tile fields move with the complete tile; `other` represents their unchanged
payload. The source column field admits 0–7, so a preexisting higher column is
not clamped by the trailing pass. Normal two-column defaults are a caller domain.
`PrefixPreserved` proves later parsing cannot alter the already selected prefix.

`WriteTiles` emits enough stars to reach each rising column, then a lowercase or
uppercase ID. Falling column numbers emit no reverse separator. A round trip
requires an ordered two-column list with unique IDs; arbitrary 0–7 bit patterns
or nonmonotonic columns do not have a lossless round trip.

`ReadPlacement` implements the exact 17-byte format: M/R/I then four four-byte
chunks containing only digits or `-`. A minus can appear anywhere or repeatedly
inside a chunk; it marks the accumulated digits negative and contributes no digit.
There is no ordering, positivity, monitor or minimum-size validation. The Main
caller interprets the final two values as dimensions, while report callers treat
them as right/bottom edges. `%04d` on write is a minimum width, not a four-character
bound: larger coordinates generate a string the next read can reject. S05 owns
OS placement, changed-monitor recovery and display qualification.

## Acceptance and evidence

| ID / owning story | Given / when / then | Definition and evidence |
| --- | --- | --- |
| <a id="s02-contract-ac01"></a>S02-CONTRACT-AC01 / S02-US03, S02-US04 | Given duplicate section A whose first block lacks k, lookup must not find k in a later A block; quoted values preserve interior spaces. | `Lookup`, `Unquote`, `LookupVectors`; profile.c. |
| <a id="s02-contract-ac02"></a>S02-CONTRACT-AC02 / S02-US04 | Empty returned integer text uses default; nonempty nonsense gives 0; -0x10 gives unsigned 4294967280; decimal accumulation wraps at 2^32. | `Integer`, `Digits`, `IntegerVectors`; existing profile adapter tests cover wrap/default/quotes. |
| <a id="s02-contract-ac03"></a>S02-CONTRACT-AC03 / S02-US02 | Deleting first duplicate k exposes the later duplicate; deleting section A removes its non-section rows and preserves following section B. | `Change`, `ChangeKey`, `LookupVectors`; native whole-file publication remains separate. |
| <a id="s02-contract-ac04"></a>S02-CONTRACT-AC04 / S02-US02 | Given tiles [A,B,C], reading C yields [C,B,A]; reading Cc does not undo C's expanded state; repeated stars cap newly assigned columns at 1 and raise omitted tiles. | `ReadTiles`, `PrefixPreserved`, `LayoutVectors`; [source-extracted C fixtures](../tests/spec/check_s02_source.py). |
| <a id="s02-contract-ac05"></a>S02-CONTRACT-AC05 / S02-US01, S02-US04 | Invalid placement yields sentinel/default flags. Valid four-byte chunk 1-2- decodes to -12; no strict sign-position rule is invented. | `ReadPlacement`, `Coord`, `LayoutVectors`; [source-extracted C fixtures](../tests/spec/check_s02_source.py). |
| <a id="s02-contract-ac06"></a>S02-CONTRACT-AC06 / S02-US04 | Loading raw logging 2 differs from NZ16 option 2; graph 16 becomes 0; invalid scan mode clears own-ship filter only. | `BitFlag`, `Nonzero16`, `Graph`, `Scan`, `ShipFilter`; structs.h and ReadIniSettings. |
| <a id="s02-contract-ac07"></a>S02-CONTRACT-AC07 / S02-US03, S02-US04 | For a two-byte name and capacity 3, enumeration writes three NULs and reports 1; empty list leaves bytes beyond its two sentinels untouched. Scalar copying never splits UTF-8. | `List`, `Scalar`, `BufferVectors`, `PrefixValid`; [native source cases](../tests/spec/check_s02_io_source.py). |
| <a id="s02-contract-ac08"></a>S02-CONTRACT-AC08 / S02-US04 | All-null write arguments return false without a new error; invalid section/key syntax differs from an allowed empty key/value. | `CheckRequest`, `WriteArguments`, `BufferVectors`; profile.c validation branch. |
| <a id="s02-contract-ac09"></a>S02-CONTRACT-AC09 / S02-US02, S02-US04 | Save emits one BOM and necessary between-row LF. Failure stops later stages, attempts appropriate cleanup, and does not claim successful rename; BOM failure leaves error assignment unchanged. | `Serialized`, `Stages`, `Execute`, `FailedDoesNotPublish`; [source failure injection](../tests/spec/check_s02_save_source.py), separate from real OS faults. |
| <a id="s02-contract-ac10"></a>S02-CONTRACT-AC10 / S02-US03, S02-US04 | The host can find a key in a later duplicate section, ignores a malformed leading section without resetting state, preserves quotes and bounds the returned value. | `S02HostProfile.Lookup`, `HostVectors`; [source cases](../tests/spec/check_s02_io_source.py). |
| <a id="s02-contract-ac11"></a>S02-CONTRACT-AC11 / S02-US04 | A missing host key uses default; an empty, quoted or 0x-prefixed value gives decimal zero. Conversion beyond native long range is outside the defined domain, not a guaranteed fallback. | `Decimal`, `Integer`, `HostVectors`; native `LIniInt` cases and explicit overflow boundary. |
| <a id="s02-contract-ac12"></a>S02-CONTRACT-AC12 / S02-US02, S02-US04 | Short selection clears only kind; NQx updates player to 15 while retaining object. A valid kind/player parses and narrows the object suffix. | `ReadSelection`, `ShortSelectionRetainsIdentity`, `StateVectors`; extracted ReadIniSettings cases. |
| <a id="s02-contract-ac13"></a>S02-CONTRACT-AC13 / S02-US04 | GameID 1A gives 16 while 1a gives 26; 7fffffff is defined and 80000000 exceeds signed arithmetic. | `ReadIdentity`, `IdentityLoop`, `StateVectors`; native cases exclude undefined-overflow inputs. |
| <a id="s02-contract-ac14"></a>S02-CONTRACT-AC14 / S02-US02 | A short startup File1 read and valid File2 yield no startup file, retain prior base, and pack File2 first. The separate second File1 read need not equal the first. Adding a new path attempts all nine writes even after one fails. | `LoadRecent`, `AddRows`, `Add`; source state cases. |
| <a id="s02-contract-ac15"></a>S02-CONTRACT-AC15 / S02-US02, S02-US04 | Removing File1 preserves later positions. A failed per-key deletion write leaves that row changed in memory and earlier writes published; a failed final redundant File1 write still returns false. | `Removed`, `DeleteWrites`, `DeleteSuccessInstalls`; source cases at all ten failure stages. M01 owns deletion outcome. |
| <a id="s02-contract-ac16"></a>S02-CONTRACT-AC16 / S02-US02, S02-US04 | Five valid transport action/quantity groups and a bounded name round-trip; invalid prefix or length clears the slot. | `ReadTransport`, `WriteTransport`, `TransportRoundTrip`; extracted native codec cases; M10 owns application. |
| <a id="s02-contract-ac17"></a>S02-CONTRACT-AC17 / S02-US02, S02-US04 | Packed production word 65535 normalizes to mine quantity 1; zero-row empty-name text is rejected, then P1 becomes a valid localized default. Five stored slots coexist with four UI choices. | `Normalize`, `ProductionRoundTrip`, `LoadProduction`, `PresetVectors`; exhaustive native packed-word cases and produceui.c control loops. |

## Remaining boundaries and verification

Closed here: desktop scalar parse/lookup/ordered list-edit algorithms, scalar and
enumeration buffers, write validation, save serialization/control, host grammar
and defined decimal-conversion domain, shared UTF-8 prefix validity, exact tile
permutation and coordinate parsing, complete active key/default/destination/write
inventory, selection/GameID decoding within defined arithmetic, MRU state and
partial-write transitions, transport/production codecs with normalization and
forced-default rules, and additional scalar width rules. Source-to-model byte/locale mapping
and actual API effects are still separate evidence obligations.

Remaining D/A work: full outer loader/writer effects and errno/path mapping,
byte-string/parsed-row refinement across all entry points, selection/identity
formatting and full selection/editor/title/tutorial callback effects (M07 now
specifies RestoreSelection dispatch and return boundaries), full M09/M10 editor
integration beyond their checked preset/quantity commands, and a proved
conditional tile round-trip theorem. The inventory above provides their source behavior; it does
not label these missing definitions as intrinsically unrepresentable. OS effects,
concurrent writers and display/accessibility qualification remain V/P boundaries.

Verification results are recorded in the [S02 state continuation entry](README.md#s02-saved-state-and-preset-continuation), following the earlier inventory and I/O batches. Proofs establish the definitions and
listed witnesses, not a full native refinement or display qualification.
