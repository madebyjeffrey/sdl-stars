# M02 — Import, naming and customization wrappers

Historical evidence: inspected C11 source at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The retained fixtures run only against the frozen snapshot; current C++ refinement remains open.

Observed source inspected 2026-10-27 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working collection.
This companion specializes [M02-US04](M02-race-library.md#m02-us04) (import a
reusable race) and the [US02](M02-race-library.md#m02-us02) editor handoff.
Stories and [M02-AC37–45](M02-race-library.md#m02-ac37) stay at the module entry.
It specifies import/current-versus-legacy publication, the ordinary name-choice,
load/edit wrapper controllers and ImportAs/ImportAndEdit. S01 owns decoding,
credentials and serialized bytes; M04/S05 own the full wizard/dialog interactions;
S04 owns native paths/I/O; S06 owns physical state/lifetime extraction.
No runtime source, file format, baseline, catalog or legacy policy changes here.

Canonical model: [M02RaceImport](dafny/M02-race-import.dfy), composing
[M02RaceActions](dafny/M02-race-actions.dfy),
[M02RaceAllocation](dafny/M02-race-allocation.dfy),
[M02RaceSave](dafny/M02-race-save.dfy),
[native captured state](M02-native-state-and-save.md) and existing S01/S04 name,
UTF-8, record-header and checksum definitions. Source:
[libraryui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/libraryui.c) `FImportLibraryRace`, `FImportLibraryRaceAs`,
`FImportLibraryRaceAndEdit`, `FChooseLibraryRaceName`, `FLoadLibraryRace`,
`FEditLibraryRace`, `FSaveLibraryRace`, snapshot/naming helpers;
[race.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/race.c) `FWasRaceFile`;
[raceui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/raceui.c) `SetSzRaceFile`, `RaceCreationWizard`;
[save.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/save.c) `FCreateFile`, `WriteBOF`, `WriteRtPlr`;
[file.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/file.c) `StreamClose`.
The [collection record](README.md#m02-race-import-continuation) reports actual
checks and distinguishes supplied service receipts from native qualification.

## State and entry domain

`State` extends the action context with the entire 256-byte global library-name
buffer, optional name override, raw signed 16-bit name-dialog flag and logical
save-stream-open state. The action context contains full native captured groups,
uncaptured filename suggestion, version word, projected file/directory store and
stage counter. A NULL override differs from a non-NULL empty string. Native
pointer identity/lifetime is an S06 boundary, not another filename comparison.

Logical input strings are bytes before NUL, without embedded NUL; native string
objects, buffers, path round trips and any reached global binding read must be
valid for their native capacities. Array `Shape` is not a proof of termination or
all native object/alias invariants. The store projects action files and private
directories, not all metadata/root-creation or UI effects. Root selection/creation
and modal/reentrant/concurrent effects remain S04/S05 obligations. Ordinary parser
receipts reuse `ReadFrame`; both native returning parser paths call `StreamClose`,
so `ReadState` normalizes logical hf to closed. This does not prove successful
physical closure or account for every native stream/global.

`Returned(false)` is native FALSE. `Escaped` represents a nonlocal exit rather
than a failed boolean return, notably a legacy player/EOF write. `Pending` means
the finite allocator reply trace is incomplete; `InvalidEvidence` rejects an
inconsistent frame/receipt. Neither diagnostic invents a new C return. Nested
`Save` events retain allocator events at the existing save controller's Allocate
operation; the allocation and write algorithms are not duplicated in prose.

## Source selection, classification and naming hint

`Import` offers the race-file picker only when no explicit source was passed.
The picker has a 512-byte output buffer and file/path-existence flags. Cancellation
returns before source length checking, capture or parsing. An explicit source
whose byte length is at least 512 returns FALSE before capture/parsing. The model
does not reinterpret an out-of-bounds native picker write as valid input.

After capture, a nonpositive authenticated parser result restores the captured
groups and returns FALSE. Source readability, credentials, shortened converted
names and legacy password-replacement interactions remain S01/M04 receipts.
`Minor` extracts version bits 5–11. The `<85` legacy choice is latched immediately
after this read, before naming; it is not recomputed after later calls.

`Hint` chooses the basename after the later of slash or backslash, takes the
first 255 bytes, removes the final dotted suffix, appends `.r1` and again takes
255 bytes. These two source `snprintf` cuts are raw byte truncation and can split
a multibyte character. They are a temporary suggestion, not the final library
filename. The subsequent canonical `Suggest` uses its existing 239-byte valid
UTF-8 prefix and first-free suffix search. This is separate from the saved
version maximum-plus-one algorithm.

## Override and ordinary name choice

A present override initializes the library, checks the shared portable filename
predicate, then checks availability in that order. Failure restores captured
race state. Success copies the name into the global buffer and builds the target
path. Override selection skips the naming dialog. A non-NULL empty override is
rejected; it does not select interactive naming.

Without an override, `Choose` models `FChooseLibraryRaceName`: initialize, copy
the hint into the global name buffer, apply first-free suggestions, and fail
before opening a dialog when all 999 queries fail. `NameWrites`/`Format` preserve
actual buffer tails through the successive formats; exhausted search clears only
byte zero after the `.r999` write. Import's snapshot does not contain this buffer.

Only when a suggestion exists does the helper set the raw dialog flag to TRUE.
Every ordinary dialog return sets it to FALSE, rather than restoring its previous
value. A result other than exactly TRUE (1) fails. A normal successful race-name
dialog receipt supplies a portable name: the shared `LibraryNameDlg` validates
name/availability before copying into that global output. Invalid UI input keeps
the modal dialog active; it is not represented as an ordinary FALSE return from
`Choose`. [Shared naming](M01-library-name-dialog.md) now specifies control/edit/
invalid-input retry and conditional geometry; native text/focus/lifetime extraction
remains M01/M02/S05/S06 work.

After dialog success the helper formats the target and rechecks availability.
A nonpositive format result short-circuits that query. Either failure reports
the storage message and returns FALSE; the accepted name and cleared dialog flag
remain current. Cancellation/error similarly leaves the global suggestion buffer
current. Restoring RACESTATE does not roll these globals back.

## Current-format publication and recovery

`NamedImport` composes canonical stage allocation. Allocation FALSE can leave
its documented directory artifact and advanced counter, then restores captured
race state. Successful allocation supplies a fresh directory and absent stage
file. `CopyCurrent` performs a nonexclusive source-to-stage copy; only copy TRUE
reaches replacement of stage by final target. Copy/replacement FALSE attempts
stage-file removal and then directory removal, ignores cleanup results, and
restores the captured groups. Copy receipt effects remain authoritative.

Successful replacement has already published the target. The helper attempts
directory removal **before** `ValidatePublished` parses/authenticates that target.
A nonpositive final result attempts target removal and restores captured state.
Failed target removal can leave the published file even though Import returns
FALSE; `PublishedRejectionRetainsFile` proves that outcome at this stage. A FALSE
validation may be I/O/authentication/cancellation, not proof of corrupt bytes.
There is no extra uniform wrapper alert on these failure branches.

Successful final validation copies the target binding and calls `SetSzRaceFile`.
It retains the parsed design/password, current suggestion/version and the raw
new-race flag. `NewFlagOnCurrentSuccess` proves that the flag is unchanged from
the current-import branch's entry. The full-entry `ImportFalseRestores` contract
proves captured-state restoration on every ordinary FALSE, including failures
after publication; it does not restore files, counter, full suggestion, global
library name or dialog flag. The preservation is not an all-or-nothing filesystem
guarantee.

## Legacy save composition and nonlocal exits

`LegacySave` consumes the converted in-memory global design and composes
canonical stage allocation with `F.Save`, forcing the actual `&vplr` candidate
alias. It projects distinct stage/target paths, merges resulting files/directory
and counter effects back into the store, and retains logical open state. It does
not replace the helper with an unconstrained success boolean. Initialization and
filename checks precede allocation; failed allocation preserves its actual
residual store. S01 owns BOF/encoded player payloads. Current `WriteBOF` writes
2.85; it does not assign `wVersFile`, so the last parsed legacy version word can
remain current after a successful conversion save.

Player writing may mutate the global candidate's dead/detail field before its
checksum is computed. Returning save FALSE restores the outer import snapshot;
successful save binds the target, clears the new flag and returns TRUE. Underlying
close failure and failed empty-directory cleanup can accompany success, as
already specified by the save owner. An escaping player/EOF write bypasses
Import's LRestore, retaining partial stage bytes/open logical stream and current
converted/global candidate state. `Escaped` is not native FALSE.

Current import's retained raw new flag differs from legacy save's cleared flag.
[GAP-24](README.md#gap-24) records the observed state distinction; M02/M04/S05
must qualify normal caller reachability and intended later-save behavior before
any runtime change. The model adds no new guard or flag reset.

**GAP-08 remains Open:** root policy permits only unprotected legacy races, while
accepted ADR 0002/current source describe protected conversion with accepted
replacement credentials. This source-derived branch does not resolve that
conflict or authorize broader legacy support.

## ImportAs, Load/Edit and ImportAndEdit

`As` sets the supplied override before Import, replacing any previous pointer.
On every ordinary return, TRUE or FALSE, it clears the override to NULL; it does
not restore the previous value. Escaping save exits bypass that clear.
`AsClearsOrdinary` proves the ordinary-return behavior. Supplied NULL selects
interactive naming even when an earlier override was present.

`Load` models the ordinary load controller: initialize/name check, capture and
parse; nonpositive results restore captured groups and conditionally alert.
Success binds the selected name/path and clears the new flag. `Edit` captures
the outer state, invokes Load, and calls the existing wizard only after successful
load. On ordinary wizard return it always restores outer binding/new flag; a
FALSE wizard result also restores design/password/cache, while TRUE retains the
completed groups. Full suggestion/version and independently persisted wizard
effects are not rolled back. `EditKeepsBinding` proves the ordinary binding frame.
The wizard outcome includes explicit state/effects and escape status, with its
inner page/control/save/validation rules still owned by M04/S05.

`AndEdit` calls Import and only proceeds on TRUE. It takes the name after the last
native separator in the current binding and calls Edit. Every ordinary Edit
result, including FALSE/load failure/cancel, is ignored for the wrapper's return:
ImportAndEdit returns TRUE and performs no rollback of the imported file.
`EditReturnDoesNotUndoImport` proves this for the resulting state, including
persisted wizard effects. An escaping edit is not an ordinary successful return.

## Interaction, quality and evidence

These are internal caller contracts; existing M02-IR01–04 and QR01–05 remain the
interaction/quality owners. M02 owns name/source choices, cancellation feedback
and recovery. S05 owns picker/dialog/widget keyboard/focus, accessibility and
reentrancy. Native dialog ABI, visual presentation and actual OS error delivery
are not qualified by scripted UI replies. Performance/display targets remain
unagreed/unmeasured. S04/S06 own path identity, concurrency, pointer/lifetime and
native extraction; stronger crash durability/publication remains an explicit
policy obligation. No second credential/hash/codec implementation is introduced.

[Source fixture](../tests/spec/check_race_import_source.py) compiles actual
import, naming, allocation, save, load/edit and wrapper bodies with native headers,
fresh generated IDs, warnings as errors and address/undefined-behavior sanitizers.
It uses actual snapshot/prefix/player-writer/checksum/StreamClose bodies, scripted
parser/dialog/BOF/file services and a live nonlocal-jump target. An independent
Python oracle checks full native/name-buffer tails, dialog/override state,
version/counter, residual files/directories, logical/physical close distinction
and exact provider calls in 3202 cases. A first oracle mismatch was an extra
logged path-format event on binding-only copies; the oracle was corrected to the
source call classification. Model composition initially timed out on a partial
status destructor; checked exhaustive pattern matching removed that proof
ambiguity without changing the controller's cases.

[Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify
mouse/cursor/tracking and color/erase; native event/window/capture qualification
remains with M04/S05/S06.

The collection record reports full closure/audit and repository checks.
[Shared name-dialog control](M01-library-name-dialog.md) and [top-level wizard
control](M04-wizard-controller.md) now have contracts/source fixtures.
[Page commands](M04-page-commands.md) now specify all six command arms and
partial commit; [initialization](M04-page-initialization.md) now supplies all six
entry arms and habitat layout. Remaining work includes full paint/helper producers,
complete drawing/pricing mutation and native callee/
codec/credential/admission extraction, pointer/alias/lifetime,
path/Unicode/OS/concurrency/crash and actual UI qualification. Passing supplied
service receipts is not proof of complete C/application refinement.
