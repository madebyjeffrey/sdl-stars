# M02 — Duplicate, Export and Delete outcomes

Observed source inspected 2026-10-27 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working collection.
This companion specializes [M02-US05](M02-race-library.md#m02-us05),
[US06](M02-race-library.md#m02-us06) and [US07](M02-race-library.md#m02-us07):
make an independent copy, export a reusable design and delete one version.
Stories and [M02-AC30–36](M02-race-library.md#m02-ac30) remain at the entry.
[Import and ordinary naming/load/edit wrappers](M02-race-import.md) now have
composed contracts. [Shared naming](M01-library-name-dialog.md) and
[top-level wizard dispatch](M04-wizard-controller.md) now have control contracts;
individual wizard pages and native modal qualification remain separate work.
Runtime C, file formats, baseline fixtures and legacy policy are unchanged.

Canonical model: [M02RaceActions](dafny/M02-race-actions.dfy), reusing
[allocation/copy/suggestions](M02-race-file-allocation.md),
[native captured-byte state](M02-native-state-and-save.md), existing
[M02Library](dafny/M02-library.dfy) and shared
[S01 name/comparison boundaries](dafny/S01-library-names.dfy).
Source: [libraryui.c](../src/ui/libraryui.c) `FDuplicateLibraryRace`,
`FExportLibraryRace`, `FCopyRaceExport`, `FDeleteLibraryRace`, `SaveRaceState`,
`RestoreRaceState`, `FPortableRaceFilename`, `SuggestRaceFilename`;
[race.c](../src/core/race.c) `FWasRaceFile`; [raceui.c](../src/ui/raceui.c)
`SetSzRaceFile`. Executed checks are in the
[collection record](README.md#m02-race-actions-continuation).

## Inputs, owned state and qualification domain

`State` carries the full captured native race/password/cache/path/new-flag arrays,
the uncaptured full filename suggestion, current unsigned version word, literal
file/directory store and shared stage counter. S06 owns native object/lifetime
extraction; this is a focused projection, not every global, stream or OS handle.
The store projects relevant action paths, not all filesystem metadata. Root
selection/creation inside `FInitRaceLibrary` and picker/modal side effects stay
with S04/S05; an unchanged projected store does not imply no root-directory or
UI effect. The supplied root is the initializer's resulting logical path.
Paths are C-string bytes before NUL. Callers supply valid string objects, native
separator and buffers with the source's capacities and lifetime/overlap rules.
`Filename` reuses S01's complete portable race-name predicate and admits either
`r` or `R` in the extension. It introduces no broader Unicode case folding.
Logical input paths exclude embedded NUL bytes. Native array `Shape` alone does
not prove termination: a reached Delete `strcmp` additionally requires the
current binding's `N.Text` length below 512. The total mathematical `Text`
projection does not assign defined C behavior to an unsafe native string read.

`Read` is an explicit returning `FWasRaceFile(..., TRUE)` receipt: signed 16-bit
result, native state and version. `ReadFrame` checks native array shapes and the
unchanged full path/new flag. Parser design/password/cache/suggestion/version
effects remain visible before restoration. S01 owns decoder, credential and
admission refinement; S05 owns modal callbacks. Receipts do not prove those
callees accept arbitrary bytes in the modeled store.

The models cover ordinary returning services without modal reentrancy or
unrepresented external mutation. Availability, parser, picker and I/O replies
are supplied explicitly. Their complete correlation with real files, native
enumeration and OS error delivery remains an adapter obligation. The source
fixture deliberately tests caller dependence on such replies, including a stale
availability reply selecting the source path. It is not evidence that a correct
quiescent enumeration returns that reply. Literal path equality is modeled;
nonliteral case, symlink and hard-link aliases require S04 qualification.

`Returned(false)` denotes native FALSE. `Pending` denotes a finite allocation
receipt trace ending before completion; `InvalidEvidence` rejects a violated
checked receipt/frame. These are diagnostics, not extra C returns. Nonreturning
or reentrant external services and complete stream/error state remain outside
this ordinary-return contract. `Event` records reached controller calls, nesting
the allocator's own event sequence instead of flattening it into another loop.

## Duplicate

`Duplicate` initializes storage and validates the source filename before saving
the outer snapshot. It formats the source path, then validates/authenticates the
source. A nonpositive source result restores the snapshot and returns FALSE
without the wrapper's duplicate-failed alert.

After source success, it removes one filename suffix, then invokes canonical
`Suggest` (which removes another suffix). Exhaustion or a nonpositive target
format result restores state, calls `IsWindow` and alerts only for a valid window.
There is no second portable-name/path-budget guard: GAP-22 remains observed.
The target buffer is 512 bytes; the guard checks positivity rather than a second
capacity comparison. On admitted portable source/root and bounded suggestion
lengths this target formatting fits; formatter failure remains explicit.

The direct, nonexclusive copy uses existing `ValidCopy`/`Copied`. A FALSE result
returns after restoring the outer snapshot and conditional alert, without an
additional wrapper removal. The copy helper may already have attempted cleanup;
its residual file result remains authoritative. A successful copy is parsed and
authenticated again. Nonpositive target validation attempts target removal,
ignores its result, restores state and conditionally alerts. Positive validation
restores state and returns TRUE. Explicit copying remains independent of
identical-design version reuse.

`DuplicateRestores` proves full captured-state restoration on every modeled
ordinary return. `DoneRestoresOwned` specifies the exact restored groups: current
suggestion, version and counter are not rolled back. It does not promise native
descriptor closure, source separation under stale availability, nonliteral alias
independence or cleanup success. The standalone real-stdio copy evidence remains
at the allocation owner.

## Export

`Export` validates initialization/source filename, then offers the overwrite-
confirming race-file picker only when no explicit target was passed. Picker
cancellation occurs before snapshot capture or source parsing. The returning
picker's selected C string must fit its 512-byte output buffer; full picker ABI,
interaction, extensions and platform behavior remain S05/S04 qualification.

The source is parsed **before** the case-insensitive lexical source/target
comparison. A source failure short-circuits comparison. `FComparisonKey` checks
ASCII lowercase keys and leaves non-ASCII native comparison extraction explicit.
Source failure or equal keys restore state and conditionally alert. Equality is
lexical; it is not a native alias/ownership check.

The controller composes canonical `A.Export` using current files/counter and one
tick. A helper FALSE returns with that helper's residual store and advanced
counter, restoring captured state and conditionally alerting. It adds no wrapper
temporary removal. A successful helper produces a temporary path to revalidate;
only positive validation reaches replacement. Validation or replacement failure
attempts temporary removal, ignores its result, restores state and conditionally
alerts. Success restores state and returns TRUE without changing the outer
binding. `ExportRestores` proves captured-state restoration on ordinary returns;
the most recent parser version/suggestion and allocation counter remain current.

`ReplaceFrame`/`Replaced` model the admitted file-only native replacement boundary:
failure leaves that boundary's store unchanged, success moves source bytes to
target. `ReplacementFailureFrame` is local to that call. It must not be promoted
to preservation of the destination since entry.

In particular, an explicit target can equal the next generated temporary name.
If that target was absent, exclusive creation writes the destination itself
before second validation. Failed validation plus failed removal can return FALSE
with that file still present. `ReplacementSelf` handles successful source/target
identity, and `FailedAliasValidationRetainsFile` proves the retained-file outcome
at the post-copy stage. Native controller vectors exercise the whole outer path.
[GAP-23](README.md#gap-23) records this exception to any proposed universal
pre-publication validation or unchanged-destination-on-failure guarantee. Normal
picker reachability and intended behavior remain decisions; no guard is added.

## Delete

`Delete` initializes storage and validates the filename, then requires a successful
regular-file size read with positive size. `sizeRead` receipts must match the
modeled file and byte length; failed size reads imply no absence. No race parser,
password check or snapshot is involved. A named Yes/No confirmation permits
removal only for IDYES (6); other results return FALSE without mutation.
The mathematical size/answer inputs represent native signed 64-bit file sizes
and signed 32-bit dialog results. Values outside those native widths are not
admitted C inputs; no arithmetic narrowing or overflow rule is introduced.

Removal failure invokes the delete-failed alert unconditionally, without an
`IsWindow` check. Successful removal affects exactly that file. Only an exact,
case-sensitive `strcmp` match with the current binding clears its first byte.
Later path storage, design/password/cache/suggestion, raw new flag, version and
stage counter remain unchanged. `DeleteFrames` proves these memory/file frames,
and `DeclinePreserves` proves state preservation when the answer is not IDYES.
Other versions, exports and existing game-player definitions are separate data;
there is no family cascade in this controller.

## Interaction, quality and handoffs

These are technical caller contracts, not new screens. Existing M02-IR01–04 and
QR01–05 apply. M02 owns action choice, named confirmation and recovery feedback;
S05 owns keyboard/focus/readability, native dialog/callback lifecycle and
accessibility. The fixture checks the deletion prompt's selected name, picker
flags, early cancellation and different alert gates. It does not qualify visual
presentation, native dialog ABI or assistive technology.

Deterministic limits, byte budgets and counter wrap reuse the allocation owner;
parser input/output rules remain S01/M04. Cleanup is best effort per branch.
S04 owns actual open/replace/remove modes, path round trips, comparison and
concurrent writers. No uniform latency, crash durability, cross-instance lock,
no-artifact-on-FALSE or full Unicode equivalence guarantee is claimed. Performance
and display targets remain Open. GAP-08 legacy policy and GAP-21 uncaptured
suggestion state retain their existing owners.

## Verification and remaining work

[Source fixture](../tests/spec/check_race_actions_source.py): actual outer
controllers, export allocation, portable/suggestion/UTF-8 bodies and native
snapshot/filename functions are compiled with native headers, fresh generated
IDs, warnings as errors and address/undefined-behavior sanitizers. An independent
Python state oracle compares all captured bytes and suggestion tails, version,
counter, residual file bytes and exact provider call traces in 4354 cases.
Cases include every ordinary failure branch, early picker cancellation,
source-before-comparison order, 999/1000 attempts, wrap, copy remnants, failed
cleanup, dotted/uppercase/long names, destination/temp equality, confirmation
decline and exact/case-different binding behavior. Initial compile/link failures
were fixture forward/global declarations; the first runtime mismatch exposed a
copy-provider self-alias error, corrected to match independently tested stdio.
No runtime source was edited to make these tests pass.

[Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify
mouse/cursor/tracking and color/erase; native event/window/capture qualification
remains with M04/S05/S06.

The collection record reports full verification and repository checks. Remaining
M02/M04 specification work includes full paint/helper producers
and native modal lifecycle extraction; [import and ordinary wrappers](M02-race-import.md)
now have composed contracts. [Shared naming](M01-library-name-dialog.md) and
[top-level wizard control](M04-wizard-controller.md) now cover callback/dispatch/
retry behavior. [Page commands](M04-page-commands.md) now specify all six command
arms and partial commit; [initialization](M04-page-initialization.md) now supplies
all six entry arms and habitat layout. Complete native page/modal extraction remains open.
Native callee, pointer/alias/lifetime, parser/credential, UI/platform/concurrency
and crash qualification remain explicit A/V obligations. Passing supplied
receipts does not establish complete C or application refinement.
