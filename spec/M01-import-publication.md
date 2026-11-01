# M01 — Import publication and recovery

Observed source behavior inspected 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working collection.
This companion specializes [M01-US03](M01-game-library.md#m01-us03),
[M01-US04](M01-game-library.md#m01-us04) and
[M01-US05](M01-game-library.md#m01-us05): import an independent managed game,
receive a player turn, and retain independent copies. Criteria remain at the
[M01 entry](M01-game-library.md#4-acceptance-and-traceability).
The [accepted library decision](../docs/adr/0001-desktop-managed-libraries.md)
provides intent; the source outcomes below describe current implementation,
including failures that do not satisfy a stronger publication contract.

Canonical [M01ImportPublication](dafny/M01-import-publication.dfy) describes
ordinary control flow after admission and choice. Its `Later`, `ConflictReplace`,
`SeparateCopy` and `FamilyPublish` programs consume explicit filesystem/helper
replies. [Collection evidence](README.md#m01-import-publication-and-recovery)
records executed checks. No game code, format, catalog or baseline is changed.

Sources: [libraryui.c](../src/ui/libraryui.c) `FImportSubsequentTurn`,
`FReplaceConflictingLibraryTurn`, `FImportConflictingTurnAsCopy`,
`FImportLibraryGame`, `FCopyNativeFileExclusive`, `FCopyNativeFileContents`,
`FInspectImportFile`, `FCreateLibraryGameStage`, `FStageConflictingTurn`,
`FBackupConflictingPlayerFiles`, `RemoveImportFile`, `RemoveLibraryGameTree`.
S04 owns native path/I/O outcomes; S01 owns file framing and metadata bytes;
M06 owns active-session state, save/submission and reloading. An import return
alone does not reload memory or establish host receipt.

## Scope, inputs and projected state

`Later` begins after destination selection, incoming inspection, existing-file
comparison, and conflict routing. It covers the orders/stage/previous path
format guards onward, for first-player and strictly later existing turns. The earlier no-match -1 return
and fallback to family discovery are outside this tail; they do not become a
new return from `Later`.
`ConflictReplace` and `SeparateCopy` cover their complete controller functions,
with called validation, cloning, backup and incoming-file helpers supplied as
external results. `FamilyPublish` begins after successful family admission,
name reservation and exchange-basename assignment. The earlier writes are
already present; failure here does not restore the old UI name or exchange name.

A `Path` is an abstract role key, not a native path string. An `Artifact` is a
file or directory with indexed byte cells. A file occupies cell zero; directory
cells project relative child-file contents. Source, target, orders, stage,
previous, directory, hold and pending roles must be bound to the actual paths.
A projection must avoid overlapping parent/child objects when asserting frames.
Path aliasing, case comparison, symlinks and live concurrent modification need
separate S04 evidence; distinct role numbers alone do not prove distinct paths.

`Reply` records the combined success guard, an optional partial artifact and
explicit environmental additions/replacements before a call. Failed exclusive
copy may leave a byte prefix only at an initially absent target. Failed cleanup
may retain a subset of unchanged cells. `Build` supplies the resulting stage
artifact on either return; this is a stage-only helper boundary, not a model of
each recursive copy or metadata write. `Replace` moves an existing source only
on reported success. No OS crash-atomicity or durable-flush guarantee follows.

The tag-to-source interface is explicit; a FALSE probe does not generally mean
that its path is absent:

| Operation | Source guard or helper |
| --- | --- |
| `Format(1..3)` | Checked orders, tick-stage and previous-path formatting. |
| `Format(4..5)` | Checked managed-directory and separate-copy target formatting. |
| `Probe(1)` | Orders `FNativeFileSize` result. |
| `Probe(2)` | Stage inspection AND matching admitted game ID and turn; generation output is NULL. |
| `Probe(3..5)` | Stage-directory existence, hold-path selection, managed-directory validation. |
| `Probe(6..8)` | Separate-copy name choice, availability and byte-budget guard. |
| `Build(1..4)` | Conflict clone, player-file backup, staged incoming turn, separate-copy clone. |
| `Build(5..7)` | Family enumeration AND no bad flag, backup enumeration AND no bad flag, metadata write. |
| `Allocate(1..2)` | Unique stage-path allocation; time-derived family-stage mkdir. |

`Pending` means evidence ended before the next helper reply; `InvalidEvidence`
means a receipt violates its modeled contract. Neither is a native error return.
`Returned(0)` and `Returned(1)` are ordinary FALSE/TRUE controller returns.
`RunFrame` proves preservation outside the called write footprint only when
`Quiet` excludes environmental changes. It is a projected-store theorem, not a
universal C memory, callback or filesystem refinement theorem.

## First-player and later-turn publication

`Later` formats orders, stage and previous paths in that order. The latter two
use the current tick. Each checked format failure returns FALSE before copying.
Exclusive copy to the stage must succeed, and stage inspection must match the
admitted game ID and turn. The controller does not compare the staged generation
number. Short-circuiting stops before inspection if copying fails.

| Branch | Returned outcome and surviving state |
| --- | --- |
| Stage copy/inspection/ID/turn guard fails | Attempt to remove the stage, ignore that cleanup result, report stage failure, return FALSE. A pre-existing stage that made exclusive creation fail is also subject to this removal attempt. |
| First player; replacement succeeds | Incoming stage becomes the player's turn; return TRUE. Existing orders/history remain because there was no previous turn to advance. |
| First player; replacement fails | Attempt stage removal, report install failure, return FALSE. |
| Existing turn; orders query returns FALSE | Continue with `ordersSeen=false`, even if the query failed for a reason other than absence. Successful publication can retain an existing orders file. FALSE query is not proof of absence. |
| Preservation copy fails | Remove stage as far as cleanup permits; report preservation failure; return FALSE. A partially created or pre-existing previous path is not removed in this branch. |
| Install fails after preservation | Attempt both stage and previous removal, ignoring cleanup results; report install failure; return FALSE. |
| Install succeeds; orders not observed or retirement succeeds | Attempt previous removal, ignoring cleanup result; return TRUE. A previous-file remnant can survive success. |
| Orders retirement fails; restoration succeeds | Move previous over target, attempt previous removal, report retirement failure; return FALSE with the prior turn restored. Orders removal already returned failure; this is not a whole-directory rollback guarantee. |
| Orders retirement and restoration both fail | Return FALSE, report the previous recovery path. Incoming target and previous recovery copy can both survive. |

`Vectors` and `CollisionCleanup` prove complete first/later-program examples;
`RetirementOutcomes` proves both outcomes at the post-retirement-failure checkpoint. `CopySuccess`/`RestoreSuccess` specify projected successful
operations. `FirstDoesNotRetire` preserves the separate first-player branch.
An unchanged-input success occurs before this program and does no publication.

S01 `PublishLaterTurn` and `FImportEffect` describe the completed logical
retirement effect. They must not be applied to every native TRUE return without
establishing the orders-query/retirement premises. The observed query failure
boundary is tracked at M01-AC16 and GAP-09; changing that behavior requires a
separate intentional runtime fix and normal regression evidence.

## Changed same-year or older turn: Replace

After checked directory formatting, managed-game validation and stage allocation,
`ConflictReplace` requests a clone preserving remembered player, played time and
exchange basename; player-file backup; staged incoming turn; then a hold path.
The backup helper considers optional `.mN`, `.xN`, `.hN` and first available
`backup.000` through `backup.999`. Its partial backup, native enumeration and
exclusive-copy details remain helper qualification, rather than silently being
promoted to transactional behavior.

Move the old directory to hold, then publish the stage at the managed directory.
If publication succeeds, attempt hold-tree removal. Failure can partially remove
the hold and produces a warning while returning TRUE. `PublishedWithRemnant`
proves a success-with-remnant example from the held-original publication checkpoint.

If publication fails, attempt hold-to-directory restoration immediately. At
cleanup, query stage existence and remove it if observed; ignore cleanup failure.
If the original remains held, retry restoration once. A successful retry clears
the restoration-failure flag and reports ordinary replacement failure. Two failed
restores report the recovery hold path. `RetriedRestore` checks this distinction from that same held-original checkpoint;
FALSE does not mean the original is already back at its original pathname.

## Import as a separate copy

`SeparateCopy` validates the source managed directory, asks for a name, formats
the target, checks availability and the existing byte-budget inequality, then
allocates a stage. It clones under the selected name with remembered player set
to the incoming player and last-played time zero, retaining exchange basename.
After installing the incoming turn into the stage, it moves stage to target.

Preparation/publication failure attempts stage cleanup only when stage existence
is observed, reports separate-copy failure and returns FALSE. Original-directory
preservation depends on the declared stage-only helper footprints and disjoint
native paths. Naming can already have changed UI globals on a later failure.
No pending-directory reservation is used by this controller.

## Publish a newly admitted family

`FamilyPublish` allocates its time-derived stage, copies current family members,
copies recognized backup members, writes metadata, removes the reserved empty
pending directory, then renames stage to pending. Each short-circuit failure
prevents later publication steps. Allocation failure attempts pending removal
and clears the pending-path string, without the generic import-failed alert.

Other failures enumerate stage children through `RemoveImportFile`, attempt stage
removal, attempt pending removal, clear the pending-path string and report import
failure. This cleanup is shallow for child directories; nested remnants and
failed removals can remain. The model's `Remove(stage,true)` groups these cleanup
calls and permits partial residue; it is not the recursive conflict-cleanup
helper. Clearing pending memory does not certify deletion of the reserved path.
`FamilyClearsPending` proves clearing on every modeled ordinary return; it makes
no clearing claim about a missing reply or invalid receipt.

This source path formats the time-derived stage into 256 bytes without checking
the formatting return. Family inspection and the copying pass are separate;
copying does not repeat every first-pass inspection. Source mutation, overflow
of copied-entry counters and malformed/colliding member enumeration remain
explicit admission/helper work, not assumptions of stronger revalidation here.

## Interaction, quality and evidence boundaries

M01-IR02/IR04 own conflict choice and distinguish failure, restoration failure and
cleanup warning. This companion adds no screen or keyboard flow. Exact localized
message text, picker/focus behavior and assistive-technology qualification remain
M01/S05 work. Model notice numbers normalize existing message IDs, not new text.

M01-QR01/QR02 own surviving-file integrity and byte/path budgets; S04 owns native
failure semantics and OS qualification. No responsiveness, durability, locking or
recovery-discovery target has been agreed here. M01-QR04/QR05 retain measurement
and viewport obligations. More forceful cleanup or stricter query handling is a
proposed behavior decision until separately approved and implemented.

[Source-controller fixture](../tests/spec/check_library_publication_source.py)
extracts current C bodies (publication tails for later/family; whole conflict
controllers) and compiles them with warnings as errors. An independent
interpreter checks operation order, every projected post-operation store, return,
notice and pending-clear events over exhaustive bounded Boolean reply masks.
Partial copy/cleanup and stage/previous collision profiles are included. Some
receipt combinations are conservative abstractions that a particular native
backend may not produce.
Validation, recursive copy/backup/cleanup and native I/O are scripted providers;
these cases establish controller behavior under their stated receipts, not full
filesystem integration or C refinement of the complete helper family. Separately,
48 additional cases execute extracted `FCopyNativeFileContents` and
`FCopyNativeFileExclusive` over empty, one-byte and 8192-byte chunk boundaries,
existing-target preservation with EEXIST, and missing sources. The first 15
use ordinary stdio. Another 33 inject read/error/write/close/removal results into
the actual copy loop, checking retained byte prefixes, full-copy remnants,
source preservation and errno restoration. These are scripted I/O failures;
actual OS fault delivery and other native platforms remain qualification gaps.

Remaining owners: M01/S01 complete family lexing, duplicate-member handling,
enumeration/admission, BOF/record parsing and byte-to-inspection/probe-field mapping;
M01 full host-orders, duplicate/rename/delete/export controllers and helper composition; S04
native path aliases, copy/write/close/delete/rename failures and concurrent/crash
qualification; M06 pending-session reloading and save handoffs; M01/S05 native
UI and recovery-path discoverability. GAP-09 retains the wider M02/M05/M06/M17/M18
publication/submission/receipt outcome matrix. Stronger guarantees remain Open.
