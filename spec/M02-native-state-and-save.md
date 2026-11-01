# M02 — Native race comparison, version reading and save outcomes

Observed source inspected 2026-10-27 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working collection.
This companion specializes [M02-US02](M02-race-library.md#m02-us02) and
[M02-US03](M02-race-library.md#m02-us03): finish a reusable design, choose the
correct saved version, and recover from cancellation or failure. Requirements
remain at the [M02 entry](M02-race-library.md#4-acceptance-and-traceability).
M04 owns wizard/design validity, M03 roster binding, S01 records/checksum/parser,
S04 text/path/I/O, and S06 physical state and caller lifetimes. No runtime code,
record layout, file format, catalog, baseline or legacy policy is changed.

Canonical models:
[M02NativeRaceState](dafny/M02-native-race-state.dfy),
[M02VersionReader](dafny/M02-version-reader.dfy),
[M02RaceSave](dafny/M02-race-save.dfy). They reuse existing
[M02Library](dafny/M02-library.dfy) decisions,
[S04Utf8Prefix](dafny/S04-utf8-prefix.dfy) and
[S01Codecs](dafny/S01-codecs.dfy), rather than introducing another comparator,
UTF-8 decoder, checksum or record header. The
[collection record](README.md#m02-native-state-version-and-save-continuation)
distinguishes executed checks from remaining qualification.

Sources: [libraryui.c](../src/ui/libraryui.c) `FSameLibraryRace`, `SaveRaceState`,
`RestoreRaceState`, `FReadManagedRace`, `FindRaceVersion`,
`FSaveCurrentLibraryRace`, `FSaveLibraryRace`, `FCreateRaceStagePath`;
[raceui.c](../src/ui/raceui.c) `SetSzRaceFile`, the initial suggestion choice in
`FSaveRace`; [save.c](../src/core/save.c) `WriteRtPlr`, `WriteRt`, `RgToStream`,
`FCreateFile`; [file.c](../src/core/file.c) `StreamClose`;
[race.c](../src/core/race.c) `IRaceChecksum`, `FWasRaceFile`;
[encoding.c](../src/platform/encoding.c) `CchUtf8Prefix`;
[structs.h](../src/core/structs.h) `PLAYER` and
[native.c](../src/platform/native.c) `CloseFile`.

## Comparison versus captured memory

`PLAYER` occupies 192 native bytes. The comparison copies both records, clears
`fLearned` and compares bytes 0–127, followed by case-sensitive singular/plural
strings through their first NUL. Case-insensitive stored-name matching used by
version discovery is a separate operation. Unused bytes after the two name
terminators do not participate; all other prefix bytes do, including reserved
bits and runtime fields. This is the exact memory comparison, not equality of
arbitrary encoded files or an alternative reusable-race schema.

| Native byte range | Contents included in the comparison |
| --- | --- |
| 0–15 | Player/design/planet/fleet counts; detail/portrait/control word; home planet, score and credential salt. |
| 16–31 | Current/minimum/maximum environments, growth and six technology levels. |
| 32–61 | Research-spending array, research percentage/policy and last-year resources. |
| 62–85 | Sixteen race attributes, trait bits, trader bits and flags. Only bit 3 of byte 84 (`fLearned`) is cleared. |
| 86–127 | Complete production template and sixteen relation bytes. |
| 128–159 / 160–191 | Singular/plural string bytes before the first NUL; the remaining field storage is ignored. |

`ComparedPrefix`, `Project` and `Equal` bind this projection to canonical
`M02Library.Design`/`FSame`. Comparisons here require live readable input objects
with both names terminated within their 32-byte fields. Outside that bounded
name domain the model makes no native FALSE/error-return claim. Some native
short-circuit cases can return before a later string read; a complete malformed
object read-domain analysis remains S06/M02 qualification.

Checksum equality is weaker than comparison: two changed prefix words can cancel
in XOR. Conversely, comparison ignores `fLearned`, while `IRaceChecksum` includes
it. S01 owns name-tail cleaning and its checksum/loader implications. Normal
race admission and full/partial player serialization must be established before
claiming a stored race round trip from a raw `PLAYER`.

`Capture` instead retains all 192 player bytes, sixteen password bytes, sixteen
legacy filename-cache bytes, 512 library-path bytes and the signed 16-bit new flag.
`Restore` selects the race/password/cache group and path/new-flag group separately
using C truth semantics. It restores unused bytes and raw flag values too.

The 256-byte UI filename suggestion is outside `RACESTATE`. `RestoreOwned` proves
restoration of captured fields with the **current** suggestion retained. Thus
snapshot restoration is not restoration of every race-related UI global.
`Logical` supplies the older logical `RaceState` projection when names are in
its comparison domain; it does not replace the complete captured-byte state.

## Filename binding and the next save suggestion

`SetFile` uses the existing S04 prefix counter: at most fifteen complete UTF-8
bytes enter the legacy cache and 255 enter the UI suggestion. Malformed input
stops at the first invalid sequence; embedded NUL ends the C string. Copy the
prefix and one terminator, leaving subsequent buffer bytes unchanged.
`Bind` also sets the full library path and clears the new flag after publication.
Its path/name inputs denote decoded C-string text before the terminator; binding
those values and alias flags to native buffers remains an adapter obligation.
Portable library names can be much longer than the legacy cache.

`Suggested` represents the initial `FSaveRace` choice: when the requested pointer
is the legacy cache, both strings are nonempty and the cache equals the beginning
of the full suggestion, use that full suggestion. Equal text in a different
buffer does not establish pointer identity. Its alias input must be bound to the
actual pointer relationship, as the model precondition requires.

A restored fifteen-byte cache can match a newer suggestion left by an intervening
edit/read. The source fixture demonstrates `ABCDEFGHIJKLMNO-old.r1` followed by
`ABCDEFGHIJKLMNO-new.r1`: restoring the snapshot retains the newer full suggestion,
and the extracted selection expression chooses it when passed the cache pointer.
GAP-21 records this observed state boundary; whether cancellation should also restore the UI
suggestion remains an M02/M04 decision and end-to-end UI qualification task.

## Silent version inspection and ordered discovery

`M02VersionReader.Read` preserves source order: path info; reject directory/link;
open; read/check the current race BOF; close that stdio stream; then capture state,
set silent mode, call `FWasRaceFile(path,FALSE)`, copy successful output, restore
captured fields, and restore silent/version words. Header-close failure is ignored.
Only a parser result greater than zero writes the caller's output record.

The parser's full suggestion and global stream handle remain at their resulting
values. The output object must be separate from the restored global `vplr`;
output/global alias cases require separate S06 contracts. `WrapperRestoresOwned`
proves the captured fields and silent/version restoration, plus untouched output
on FALSE. It does not assert rollback of all parser, cipher, stream or UI state.

The BOF check and parser use separate opens. The current-2.85 gate describes the
initial read; stability of the path/content until parsing is an S04 obligation.
It cannot certify the second open under concurrent replacement or resolve the
protected-legacy policy disagreement (GAP-08).

`Fold` applies canonical `M02Library.Scan` in enumeration order with supplied
native comparison results. Portable derived-stem occupancy contributes to the
maximum even when decoding fails. Readable same-stored-name entries contribute
regardless of filename stem. Latest ties use native case-insensitive filename
ordering; equal-version identical reuse retains the first encountered candidate.
`Existing` derives exact design equality and credential presence from native bytes
for the existing-target `Plan` input. An unreadable selected target fails before
reuse of another identical entry. New-version selection uses maximum plus one,
not an earlier hole; maximum 999 fails except when reuse is available.

The `FSaveCurrentLibraryRace` fixtures compose actual scan/read/comparison bodies
with scripted dialog/authentication/save services, for both global and separate
candidate pointers. Authentication restoration keeps the completed candidate;
it can still leave the authentication helper's full suggestion. A failed
native enumeration can likewise leave a reader-produced suggestion even though
the save controller returns FALSE. Native Unicode comparison beyond the supplied
ASCII keys remains an explicit S04 adapter obligation.

## Staged save, input mutation and escaping writes

`Save` models `FSaveLibraryRace` with disjoint target/stage roles and a live,
bounded-name candidate. Its projected files include target/stage bytes, directory
presence and whether the global `hf` is open. No concurrent external mutation
or path/handle alias outside the declared candidate alias is included. Allocation is an external helper
receipt: failure can leave a directory or collision artifact, and the caller
returns without additional cleanup. Complete allocator loop/counter/path-string
refinement remains M02/S04 work.

After successful allocation, `FCreateFile` failure attempts stage and directory
removal. Its caught creation/BOF-write failure can leave `hf` open; cleanup does
not call `StreamClose`. Receipts retain that state rather than synthesizing a
closed stream. Successful creation requires a present stage and open `hf`.

`BeforeWrite` captures an actual caller mutation: when byte 84's dead bit is set,
`WriteRtPlr` sets the low three detail bits in byte 6 to `detAll=7` before copying
and encoding. Other input bytes remain. This also changes global `vplr` when the
candidate pointer aliases it. The alias must be consistent with the input bytes.
It is not undone by an escaping write or a later failed replacement.

| Boundary | Native controller outcome |
| --- | --- |
| Player write escapes | Partial stage and open `hf` can remain. No checksum, EOF, close, replacement or cleanup follows in this controller. |
| EOF write escapes | Checksum has already been computed; partial EOF and open `hf` can remain. No close/replacement/cleanup follows. |
| Close reports an underlying error | `CloseFile` returns void and `StreamClose` sets `hf=-1`; no success test blocks replacement. Descriptor/resource/durability consequences remain native qualification. |
| Replacement fails normally | Attempt file/directory cleanup; alert only when `IsWindow` is true; return FALSE. Preserve prior target under the stated disjoint/native-result contract. |
| Replacement succeeds | Remove stage-directory remnants as far as cleanup succeeds, bind path/filename, clear new flag and return TRUE. Failed directory cleanup can accompany TRUE. |

`Transfer` validates complete or prefix write receipts. `Escaped` identifies a
nonlocal control transfer, never a new native FALSE return. `InvalidEvidence`
identifies contradictory receipts, including impossible modeled cleanup or
success without a created/open stage; it is not an implementation error code.
EOF bytes use the existing S01 header and checksum functions after `BeforeWrite`;
player-record encoding, encryption and BOF bytes remain S01 helper obligations.
The controller does not re-read the final race file. TRUE establishes the observed
publication steps, not arbitrary raw-input validity, completed close, a durable
flush or a crash transaction.

`SuccessMilestones`, `FailureTargetFrame`, `CloseFailureIgnored` and
`EscapeSkipsPublication` prove their named conditional controller properties.
`CloseWarningVector` checks a successful publication with an ignored close error.
`FoldMaximum` proves scan-maximum monotonicity; existing M02 selection/version
properties remain canonical.

## Interaction, quality and evidence

M02-IR01/02/04 own save choice, credential prompts and useful failure feedback;
M04 owns the next file suggestion. This technical companion adds no screen or
keyboard flow. Keyboard, focus and assistive-technology qualification remains
M02/M04/S05 work. M02-QR01–03 own exact compared/captured fields, text byte limits,
version bounds and partial-save state; QR04/05 retain unagreed performance and
viewport targets. No timing, durable-write or UI conformance claim is made.

[Comparison/state/prefix fixture](../tests/spec/check_race_workflows_source.py),
[version fixture](../tests/spec/check_race_versions_source.py) and
[staged-save fixture](../tests/spec/check_race_save_source.py) compile extracted
source with native headers, warnings as errors and address/undefined-behavior
sanitizers. Layout/flag constants are bound to fresh native compilation. The save
fixture uses actual `WriteRtPlr`, checksum and `StreamClose` bodies, scripted
uncompressed encoding/I/O, and a live jump buffer. An independent byte/decision
oracle checks final files, input/global/binding frames and call order. See the
collection record for commands, counts, actual failures and final verification.

[Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify
mouse/cursor/tracking and color/erase; native event/window/capture qualification
remains with M04/S05/S06.

The [allocation companion](M02-race-file-allocation.md) now specifies the stage/
export-name loops, copy receipts and numbered suggestions, including the checked
successful-allocation projection into this save controller.
[Ordinary Duplicate/Export/Delete](M02-race-file-actions.md) now have composed
contracts and source fixtures. [Import/name-choice/load/edit wrappers](M02-race-import.md)
now have composed contracts too. [Page commands](M04-page-commands.md) specify
all six WM_COMMAND arms and partial/successful commit; [initialization](M04-page-initialization.md)
now specifies all six entry arms and habitat layout. Remaining page scope is
full paint/helper producers, complete drawing/pricing and native lifecycle
extraction.
[Shared naming](M01-library-name-dialog.md) and [wizard dispatch](M04-wizard-controller.md)
now have explicit control contracts. Qualification still needs full S01
parser/codec/admitted-state extraction; S06 pointer/lifetime/alias frames; S04
native path/Unicode/OS faults, concurrent writers and crash evidence; and native
UI evidence. GAP-08 remains Open.
