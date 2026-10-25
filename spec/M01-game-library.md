# M01 — Game Library

Status: source-derived specification; logical module ownership and future UI
requirements are distinguished from implemented behavior.
Inspected: 2026-10-24, `e2eae18cff0f990db20415c1c2262aed2022f95b`, with the
existing local specification collection. Runtime sources are unchanged.
Canonical definitions: [entry point](M01-library-specification.dfy),
[M01Library](dafny/M01-library.dfy), [shared library names](dafny/S01-library-names.dfy).
Verification: [collection run](README.md#m01-and-m02-verification).

## 1. Purpose, scope and ownership

Help players and desktop hosts find, open, copy, exchange and remove managed
games without confusing a library copy, a player turn and a host session.
Follow the [glossary](../GLOSSARY.md) and
[accepted library decision](../docs/adr/0001-desktop-managed-libraries.md).
A game with both player and host files can appear in both lists, backed by the
same folder. Identical internal game IDs can belong to independent library copies.

This boundary spans [libraryui.c](../src/ui/libraryui.c), title/action dispatch
in [mdi.c](../src/ui/mdi.c), [fileui.c](../src/ui/fileui.c) and
[libraryzip.c](../src/ui/libraryzip.c); it is not an isolated service today.

| Owner | Handoff |
| --- | --- |
| M01 | Library folders, names, listing, destination choice, copy/rename/delete and exchange coordination. |
| [M02](M02-race-library.md) | Reusable race designs; external-open dispatch routes race files there. |
| [M03](M03-new-game-wizard.md), [M05](M05-universe-creation.md) | Setup and generation; M01 reserves the managed destination and publishes metadata after completeness checks. |
| [M06](M06-player-session-and-turn-orders.md) | Authentication, loading, pending-order prompts, saving, submission intent, active-session replacement and Orders Export. Importing a disk turn does not itself reload the session. |
| M17/M18 | Host management and generation after selecting `.hst`; M01's Import Orders validates/stages input but does not execute it. |
| [S01](S01-game-file-formats.md), [S06](S06-shared-game-state.md) | Wire/metadata/ZIP contracts and scoped logical state; library identity is separate from internal game identity. |
| S02/S04/S05 | Settings, filesystem/path operations, picker dispatch, controls and rendering. |

The command-line host continues to accept explicit paths. Startup does not
migrate recent external files automatically. Library import is explicit.

## User stories and requirements

Primary actors are a player managing local games and a desktop host managing
host files. Prerequisites are accessible per-user storage and, where relevant,
a selected library entry or external source. Goals are inferred from source and
ADR 0001, not user research. Use the [collection status conventions](README.md#requirement-ids-and-status).

<a id="m01-us01"></a>
**M01-US01 — Find and resume a player turn (Observed).** As a player, I want to
see available players and their individual years so that I open the intended
turn. Refresh Games, choose a game/player, then Open; Continue uses remembered
play metadata. Missing remembered-player data requires an explicit new choice.
Loading and password failure remain M06 outcomes.

<a id="m01-us02"></a>
**M01-US02 — Start a managed game (Observed).** As a player, I want completed
setup to create a library entry so that I can return to it later. Enter M03,
choose a valid unique library name, generate through M05 and publish only after
required files pass checks. Cancel or failure cleans the pending destination as
far as the documented filesystem operations succeed.

<a id="m01-us03"></a>
**M01-US03 — Import an independent game (Observed).** As a player or host, I
want to import a folder, file family or ZIP so that I can manage a local copy.
Inspect the source, choose a unique name, stage files and publish metadata.
Incomplete families, ambiguous universe discovery, invalid archives and cancelled
naming stop the operation; original source files are not the managed destination.

<a id="m01-us04"></a>
**M01-US04 — Receive a player turn (Observed).** As a player, I want an incoming
turn applied to the correct library copy so that other players' work survives.
Match internal game identity, choose among matching copies and classify the turn.
Later turns update without a conflict prompt; changed same/older turns offer
Cancel, Replace or Separate Copy. Partial publication and restoration have
explicit outcomes below.

<a id="m01-us05"></a>
**M01-US05 — Organize independent copies (Observed).** As a player or host, I
want Duplicate and Rename so that I can distinguish local games while retaining
stored identity and exchange naming. Choose an available portable name, stage
the tree and publish. Invalid names, links or target collisions stop staging;
rename publication failure attempts to restore the held original.

<a id="m01-us06"></a>
**M01-US06 — Export a game (Observed).** As a player or host, I want a folder or
ZIP containing the supported game family so that I can use it elsewhere. Choose
a destination, resolve relevant pending work through the existing save path,
copy to staging and finish export. Picker cancellation, failed copying and a
completed local save followed by failed export are separate outcomes.

<a id="m01-us07"></a>
**M01-US07 — Remove a managed game (Observed).** As a player or host, I want to
delete the selected managed copy with a clear scope so that I can reclaim its
storage. Confirm the named game, including host files and backups, or cancel
and export first. Failure may leave a partially removed tree; successful deletion
also clears relevant recent/Continue state.

<a id="m01-us08"></a>
**M01-US08 — Select host work and receive orders (Observed).** As a desktop
host, I want a separate Host Games entry and validated orders import so that I
work on the selected host's game and year. Select the host, inspect incoming
`.xN`, confirm replacement if different, stage/recheck and publish. Wrong game,
player, year or generation is rejected; host readiness and execution remain M17.

## 2. Inputs, outputs and state

| State | Meaning and lifetime |
| --- | --- |
| Root | `STARS_LIBRARY_DIR`, else `STARS_SETTINGS_DIR`, else `SDL_GetPrefPath("", "Stars!")`; append `Games`. `FInitLibrary` creates/validates the directory. |
| Managed folder and family stem | Unique library name. `FPortableLibraryName` and native case-insensitive enumeration govern availability. This is not a globally unique game ID. |
| Game title / internal ID | Stored game content; rename/duplicate preserve them. Copies can match an incoming turn by the same internal ID. |
| Exchange basename | Original imported stem, or initial library name for a new game. Retained across rename/duplicate; M06 uses it to suggest Orders Export filenames. |
| `.library` | S01 version 2 metadata: name, remembered zero-based player (`-1` permitted), last-played time and exchange basename. Version 1 reads use name as exchange basename. |
| Pending path/name | Destination reserved during setup/import, distinct from completed metadata publication. Cleared after finish/failure handling. |
| List cache | Name, remembered player, 16-bit available-player mask and 16 separate turn values. Rebuilt on refresh; list item data indexes this allocation. |
| Continue state | Most recent eligible metadata in enumeration order, plus remembered-turn availability. `Library/ContinueDeleted` can suppress Continue until a successful remember operation clears it. |
| Staging / held original | Operation-local paths for copy, import, export and rename. A recovery path can survive failure; it is not a second active session. |

Metadata bytes and file-family identity stay in
[S01 external formats](S01-game-file-external.md#3-library-metadata-and-names).
The `Entry` datatype in M01 is a logical projection, not a replacement metadata
schema. Time input and all native filesystem outcomes are external inputs.

## 3. Workflows and rules

### 3.1 Browse, choose and Continue

Sources: `ListLibraryGame`, `ListLibraryHostGame`, `RefreshLibraryPlayers`,
`RefreshLibraryGames`, `FindContinueLibraryGame`, `FContinueLibraryGame`,
`FSelectLibraryGame`, `FSelectLibraryHostGame`, `RememberLibraryGame`.
Models: `FGamesRow`, `FHostRow`, `Recent`, `Consider`, `Latest`, `Continue`,
`PlayerYear`; M06 `FSelectPlayer` and `Continue`.

Games requires a portable name, readable metadata, a nonempty `.xy` and at least
one nonempty `.mN` that passes `FInspectImportFile`. It does **not** revalidate the
universe identity against every turn during listing. Host Games requires metadata
to exist, inspected `.xy`/`.hst` and matching game IDs; that listing does not
parse all metadata fields. Listing is not full semantic validation.

The player selector iterates slots 0–15 and displays player number `slot+1` with
`2400+turn`; years can differ within one folder. Ordinary selection chooses the
remembered available player or first available player. Refresh attempts to retain
an explicitly selected available player. Open checks the cached mask and hands
off the selected `.mN` path; host Open selects `.hst` directly.

Continue scans metadata separately. It accepts versions 1/2, a matching metadata
name and remembered slot 0–15, keeping the greatest last-played value. Equal
values retain the first encountered candidate; this is not a stable filename
sort. A more recent candidate with a missing turn is not replaced by an older
available game. Continue then selects Games, clears the player selection,
disables Open and focuses the player control if that game remains listed.
If it is absent, Continue cannot proceed. Its metadata scan is narrower than
`FReadLibraryMetadata` and does not prove every metadata field valid.

Remembering a successfully opened managed game rewrites metadata with current
player/time while retaining exchange basename; only a successful write clears
the deletion-suppression setting. M06 owns when loading succeeds and recording
occurs. A visible game list is name-sorted; the implementation does not provide
a last-played column despite the broader ADR wording.

### 3.2 Names and new-game publication

Sources: `FPortableLibraryName`, `GetGameLibraryName`, `SuggestLibraryName`,
`LibraryNameDlg`, `FChooseLibraryGameName`, `FFinishLibraryGame`.
Models: shared `FComponent`, `FReserved`, `FGameName`, `CleanStem`, `FUtf8Cut`,
`FAvailable`; M05 `FPublished` and its completeness conditions.

Names reject empty strings, leading/trailing space or period, controls/DEL,
`<>:"/\|?*`, and Windows reserved stems (including before an extension).
The byte/path inequality is `rootBytes + 2*nameBytes + 18 < sizeof(szBase)`;
name length must also be below 256. Availability enumerates all entries, compares
using the native case-insensitive routine and fails closed if enumeration fails.
No Unicode normalization or new equivalence rule is introduced.

Title-derived suggestions replace forbidden bytes with `_`, trim edge spaces/
periods, replace internal periods, use a default for empty output and prefix
reserved names. Byte truncation preserves a UTF-8 boundary. Collision suffixes
try 2 through 9999, subject to path budgets; the dialog still validates the final
choice. The portable predicate itself allows internal periods in manually entered
names. Their interaction with the loader's last-period parsing remains an audit
boundary, not an assertion that all such names round-trip.

Direct rename permits the source comparison key through its availability-except
check, while the generic naming dialog still requires an unoccupied name. A
case-only rename is therefore not promised as an exposed UI workflow.

New-game setup reserves the named folder. After M05 generation, M01 checks `.xy`,
`.hst` and every player `.mN` for completeness and writes metadata. It removes
pending files/folder on failure and clears pending state. Generation returning
success alone does not publish a complete game. See M03/M05 for cancellation,
previous-session destruction, file checks and their existing formal definitions.

### 3.3 Import a new family

Sources: `FImportDesktopFile`, `FImportLibraryGame`, `FindImportUniverse`,
`CopyImportEntry`, `CopyBackupDirectory`, `FInspectImportFile`.
Models: `FNewFamily`, `FBackupDirectory`, shared S01 file/ZIP predicates and
publication relation.

1. Select an existing nonempty file or directory. BOF race dispatch goes to M02;
   ZIP input extracts into a temporary stage, then runs family import.
2. A standalone `.m1`…`.m16` first attempts an existing-library update (§3.4).
   No matching library copy falls back to new-family discovery.
3. A directory must yield exactly one top-level `.xy` stem. File selection uses
   that filename's stem and neighboring recognized family members.
4. Inspect all recognized files before copying: current 2.85 BOF/type/player,
   framing/EOF, nonzero matching game ID and distinct player-turn slots. Require
   `.xy` plus at least one player turn **or** host state. Malformed companion-like
   names fail the family; unrelated files are not automatically imported.
5. Choose an available name, remember the source stem for exchange, copy family
   and recognized backups into staging, write metadata and publish the folder.

The metadata remembered slot is the lowest present player, or `-1` for host-only;
its initial timestamp is the current native time. First-pass inspection is not
complete decoding of every record or proof of every gameplay invariant. Ordinary
import leaves source files alone. Changes to source files between inspection and
copying, aliases and concurrent writers are external filesystem obligations.

Recognized backup directories are `backup`, `backup` plus one/two digits, or
`backup.` plus three digits. Only recognized family members within them are
copied. This differs from recursive duplicate/rename below. S01 owns archive
methods, CRC/name/size checks and unsupported-name errors.

### 3.4 Update, conflict or separate copy

Sources: `FindLibraryTurnMatch`, `FImportSubsequentTurn`,
`FReplaceConflictingLibraryTurn`, `FImportConflictingTurnAsCopy`.
Models: `ImportTarget`, M06 `ClassifyImport`, S01 `FImportEffect`,
`PublishLaterTurn`, `FTransition`; `BackupSlot` selects the first free backup,
and `ConflictCopy` describes copy metadata.

Match an inspected incoming turn by internal game ID against stored universes.
A single match is selected directly; 2–64 require explicit target choice; more
than 64 cannot be selected by the current bounded dialog. The incoming filename
need not use the library stem. Destination metadata presence is not a full
metadata parse in this match scan.

- Identical bytes return success without rewriting.
- A first turn for that player checks the stored universe's player count and
  rejects dangling/unreadable destination entries. It retains existing orders
  and history because no previous player turn advanced.
- A strictly later turn stages/rechecks, preserves a temporary previous turn,
  publishes and then retires only that player's old orders. History and other
  players remain. Retirement failure attempts restoration; restoration failure
  reports the recovery path. Skipped years do not become a conflict.
- Changed same-year or older input prompts Cancel, Replace or Separate Copy.
  Replace stages a full copy, backs up that player's `.mN`, optional `.xN` and
  `.hN` into the first free `backup.000`…`backup.999`, installs the turn and swaps
  directories. Existing orders/history are retained in this conflict path.
  Backup exhaustion fails (`BackupSlot == 1000` in the model); this is not the
  later-turn retirement path.
- Separate Copy leaves the original unchanged, stages under a new unique name,
  installs the incoming turn and records its player with played time zero.
  Existing copied orders/history are retained; later loading applies S01/M06 rules.

Failed staging, failed swap, failed restoration and successful publication with
failed old-tree cleanup are distinct states. Cleanup warnings can accompany a
true return. Disk import does not update live memory by itself.

### 3.5 Duplicate, rename and delete

Sources: `FValidManagedLibraryGame`, `FCreateLibraryGameStage`,
`FCopyLibraryGameEntry`, `FDuplicateLibraryGame`, `FRenameLibraryGame`,
`RemoveLibraryGameTree`, `FDeleteLibraryGame`, `FDeleteLibraryGameRecentPaths`.
Models: `Renamed`, `Duplicated`, `CopiedLeaf`, `FCopyTargets`, `RenameOutcome`,
`DeleteOutcome`, `RecurseDelete`, `StageAttempts`; S01 native publication obligations.

Duplicate/rename require a valid non-link directory and matching parsed metadata.
Recursively retain directories and unrelated sidecars; rename recognized family
leaves to the new stem even inside backups. Skip `.library`/`.library.tmp` copies
and write new metadata. Reject links, insufficient path capacity and
case-insensitive collisions between renamed family leaves and sidecars. These
operations check a nonempty source universe but do not re-decode the whole game.

Duplicate preserves game title, internal ID and exchange basename, but sets
remembered player `-1` and played time `0`. Rename preserves those metadata
values as well as the game content. Rename holds the original tree before
publishing; failed publication attempts restoration (including a retry during
cleanup). If restoration fails, report where the original remains. Successful
publication with failed hold cleanup is success plus a warning.

Delete requires readable matching metadata and an explicit named confirmation
covering the whole managed game and backups, with export-first guidance. It
removes children recursively, removing links themselves without following their
targets. Deletion is not transactional: an error can follow earlier removals.
After success it removes corresponding recent paths. Deleting the Continue
target suppresses automatic promotion of another game; recent-settings failure
is a warning after the game was removed. Imported originals are separate copies.

[S02's recent-file contract](S02-preference-contracts.md#selection-identity-and-recent-file-state)
specifies the cleanup's exact partial states: each memory row changes before its
key write, failure stops later rows, and deleting File1 keeps holes rather than
promoting a different game. The extra final File1 clear can itself fail after
all nine rows were written. These File keys are distinct from the metadata
enumeration used by `Latest`/Continue and from the `ContinueDeleted` guard.

### 3.6 Export and host orders

Sources: `FExportLibraryGame`, `CopyGameExportEntry`, `FExportSelectedLibraryGame`,
`FImportLibraryOrders`. Models: `FExportFamily`, `ExportResult`, `ExportReturn`,
`FHostOrders`, `OrdersAction`; S01 archive/publication and M06 save contracts.

Game export offers ZIP or folder selection. Starting the asynchronous folder
picker returns true before copying finishes; it is not completion evidence.
Destination rejection includes export inside the managed source and an existing
folder destination. Copy recognized top-level family files and recognized backup
family members, require universe plus playable turn/host, then publish. Unlike
Duplicate, arbitrary sidecars, metadata and arbitrary nested directories are not
included. “Complete game export” means this supported family, not a byte-for-byte
archive of every directory entry. Malformed companion-like names fail export.

Once a concrete destination exists, exporting the active dirty managed game
calls legacy `PromptSaveGame`; it does not use the stronger guarded M06 departure
contract. Export does not set submission intent. Copy/ZIP failure can follow
successful local saving. S01 owns the archive bounds and filesystem staging.

Host Games hides the player selector and shows Import Orders. For incoming `.xN`,
inspect the source, selected `.hst` and `.xy`, require matching game ID, host turn,
generation and an existing player. Identical existing orders succeed unchanged;
different existing orders require confirmation. Copy exclusively to staging,
reinspect, then replace. No host generation or readiness guarantee follows from
successful import. Orders Export remains the distinct M06 player workflow.

## 4. Acceptance and traceability

Each row uses Observed behavior; Proposed UI/quality outcomes are separate in §5.
Named tests are evidence of scope; the dated run in §7 establishes execution.

| ID / story | Given / when / then | Rule and evidence | Test or gap |
| --- | --- | --- | --- |
| <a id="m01-ac01"></a>M01-AC01 / US01 | Given two players at different years, when Games is refreshed, show each available slot's own year and open the selected `.mN`. | §3.1; `FGamesRow`, `PlayerYear`, M06 `FSelectPlayer`. | Startup multi-player/import cases; not full payload validation. |
| <a id="m01-ac02"></a>M01-AC02 / US01 | Given newest eligible metadata has a missing remembered turn, Continue requests that game's player choice; it does not silently open an older candidate. Equal timestamps retain first enumeration candidate. | `Latest`, `Continue`, `ContinueTieKeepsFirst`; Continue source. | Continue workflow tests; native directory tie order is external. |
| <a id="m01-ac03"></a>M01-AC03 / US02 | Given generated files are incomplete, Finish fails library publication even if generation reported success. | M05 `FPublished`; `FFinishLibraryGame`. | Existing new-game/failure harness; no crash-durability proof. |
| <a id="m01-ac04"></a>M01-AC04 / US02, US05 | Given reserved `CON` or an occupied native comparison key, naming fails; multibyte suggestions remain complete UTF-8 prefixes. | Shared `FReserved`, `FGameName`, `FAvailable`, `FUtf8Cut`. | Name/collision/path tests; Unicode comparison adapter separately required. |
| <a id="m01-ac05"></a>M01-AC05 / US03 | Given `.xy` and valid host state but no player turns, import can succeed and appear in Host Games; universe-only input fails. | `FNewFamily`, `FHostRow`; S01 family inspection. | Host-library and new-game import cases. |
| <a id="m01-ac06"></a>M01-AC06 / US03 | Given two top-level universes or an invalid ZIP entry, import fails before publishing a managed game. | §3.3; S01 `FArchive`, inspection boundary. | Export/import runner; exhaustive archive cases not covered. |
| <a id="m01-ac07"></a>M01-AC07 / US04 | Given 2–64 matching copies, require target selection; over 64 fails. Identical bytes do not trigger replacement. | `ImportTarget`, `TurnAction`; match/conflict source. | Multi-copy startup tests; 64/65 boundary model, native boundary test gap. |
| <a id="m01-ac08"></a>M01-AC08 / US04 | Given a later Player 2 turn, retire only Player 2 orders after publication; on first Player 2 import retain orders/history. | S01 `FImportEffect`/`PublishLaterTurn`, M06 classification. | Update/failure harness; restoration failure remains explicit. |
| <a id="m01-ac09"></a>M01-AC09 / US04 | Given changed same/older input, Cancel preserves destination; Replace backs up player files; Separate Copy preserves the original. | §3.4; S01 effects, `ConflictCopy`. | Conflict runner; backup exhaustion needs focused runtime coverage. |
| <a id="m01-ac10"></a>M01-AC10 / US05 | Given Rename or Duplicate, preserve internal ID, title and exchange basename; only Duplicate resets remembered player/time. | `RenamePreservesIdentity`, `DuplicateNotPlayed`. | Library safety runner, metadata/file checks. |
| <a id="m01-ac11"></a>M01-AC11 / US05 | Given sidecar/family target collision or a link, staging fails without truncating an earlier staged file; failed rename publication reports restoration outcome. | `FCopyTargets`, `RenameOutcome`; copy source. | Library safety fault injection; native races remain external. |
| <a id="m01-ac12"></a>M01-AC12 / US06 | Given folder export, copy supported family/backups, omit arbitrary sidecars, reject destination collision; picker-start success alone is not completed export. | `FExportFamily`, `ExportReturn`; §3.6. | Export runner; sidecar scope source-backed, needs focused golden coverage. |
| <a id="m01-ac13"></a>M01-AC13 / US07 | Given cancelled deletion, remove nothing. Confirmed deletion never traverses a link target; partial failure does not claim restoration. | `DeleteOutcome`, `NoLinkTraversal`; removal source. | Delete runner cancellation/failure/link checks. |
| <a id="m01-ac14"></a>M01-AC14 / US07 | Given deletion of the Continue target, suppress automatic promotion; failed recent cleanup warns after deletion. | §3.5; Continue suppression, `DeletedRecentWarning`. | Delete/Continue tests; settings-write failure scope limited. |
| <a id="m01-ac15"></a>M01-AC15 / US08 | Given wrong game/player/turn/generation orders, reject; identical bytes succeed unchanged; different existing orders require confirmation. | `FHostOrders`, `OrdersAction`; S01 `FOrdersFor`. | Host-library cases and model conditions; no execution claim. |

## 5. Interaction and quality requirements

| ID / status | Requirement and check / target | Owner and remaining evidence |
| --- | --- | --- |
| <a id="m01-ir01"></a>M01-IR01 / Observed | Games and Host Games share folder identity but expose different selection/actions. Verify Open enablement, sorted names, player/year labels and explicit missing-player focus. | M01/S05; startup/Continue tests and §3.1. Last-played column is absent. |
| <a id="m01-ir02"></a>M01-IR02 / Observed | Name/conflict/deletion prompts identify the chosen target and available choices; invalid/colliding names keep the dialog open with feedback/suggestion. Verify Cancel and failed publication separately. | M01; `LibraryNameDlg` and turn/confirmation dialogs. |
| <a id="m01-ir03"></a>M01-IR03 / Proposed | Complete browse/import/copy/rename/export/delete and host selection with keyboard-only input, visible focus and text cues independent of color; retain useful focus after errors. | M01/S05; full control, picker and assistive-technology audit remains open. |
| <a id="m01-ir04"></a>M01-IR04 / Proposed | Distinguish picker launched, staged, published, partially removed and recovery-required outcomes in consistent user-facing feedback. | M01/M06/S05; current messages exist, but a uniform result interface is not implemented. |
| <a id="m01-qr01"></a>M01-QR01 / Observed | Preserve stored bytes, identity/title/exchange and documented unrelated-player state for each operation. Compare source snapshots, decoded metadata and failure artifacts. | M01/S01; regression plus library fixtures. No family-wide crash transaction. |
| <a id="m01-qr02"></a>M01-QR02 / Observed | Enforce existing path-byte budgets, reserved names, native case collision checks and UTF-8-safe suggestion truncation. Test boundary and multibyte paths. | S01/S04/M01; name model and native tests; normalization is not added. |
| <a id="m01-qr03"></a>M01-QR03 / Observed | Retain explicit 64-match selector limit, 1000 staging/backup attempts and failure on exhausted allocation/path capacity. | M01/S04; model/source. These bounds do not establish a total library-size or memory guarantee. |
| <a id="m01-qr04"></a>M01-QR04 / Open | Measure startup/refresh, player change, import/export/copy latency and peak memory/disk on 0/1/64/1000-entry corpora with declared file sizes, platform and storage; report median/p95 and failures. | M01/S04/S05 must agree budgets and filesystem scenarios. No timing measurement performed here. |
| <a id="m01-qr05"></a>M01-QR05 / Open | Define supported viewport/DPI/localization profiles; verify all action controls, long UTF-8 labels and picker/error focus remain reachable/readable. | S05/M01; current position formulas do not establish a minimum usable width or accessibility conformance. |

## 6. Formal coverage and boundaries

| Rule area | Canonical model / explicit external obligation |
| --- | --- |
| Names, path limits, sanitization core and collisions | Shared S01 library-name definitions; source-derived default text, suffix formatting, native Unicode comparison and maximal UTF-8 cut extraction remain adapter obligations. `FUtf8Cut` checks a supplied safe prefix, not its maximality. |
| Lists, per-player years, Continue ranking/action | `FGamesRow`, `FHostRow`, `Latest`, `Continue`, `PlayerYear`; inputs already reflect the distinct inspection levels described above. Enumeration, listbox sorting/focus and I/O scheduling are native obligations. |
| Setup and publication | Existing M03/M05 rules and S01 metadata; no duplicate generation algorithm. |
| Family/ZIP import and turn update | `FNewFamily`, `ImportTarget`, `TurnAction`; existing S01 framing/archive/effects and M06 classification. Full record semantics remain S01/gameplay concerns. |
| Host orders | `FHostOrders`, `OrdersAction`, shared S01 identity match; native player-count extraction and staged bytes are checked by source adapters. |
| Copy/rename | `Renamed`, `Duplicated`, `ConflictCopy`, `CopiedLeaf`, `FCopyTargets`, `RenameOutcome`; actual recursive enumeration, case comparison, exclusive copy and native replacement are external results. |
| Deletion and export | `DeleteOutcome`, `RecurseDelete`, `FExportFamily`, `ExportReturn`; tree side effects, settings writes, ZIP codec and callbacks remain separately tested obligations. |

The model proves selected metadata-preservation and decision properties. It does
not verify C refinement, actual filesystem atomicity, symlink-race resistance,
crash recovery, concurrency or accessible rendering. S01's generic publication
relation describes milestones; each workflow's source-specific return/cleanup
rules above remain authoritative. No new filesystem or gameplay implementation
is introduced.

## 7. Evidence, verification and open decisions

Source functions are named in §3; relevant test sources are the
[startup harness](../tests/compat/win32_client_startup.c),
[game export runner](../tests/compat/run_game_export.py),
[library safety runner](../tests/compat/run_game_library_safety.py) and
[delete runner](../tests/compat/run_game_library_delete.py).
Actual commands, counts and failures for this batch are in the
[shared verification record](README.md#m01-and-m02-verification).

| Decision / gap | Owner / consequence |
| --- | --- |
| Align ADR “complete export” with supported-family export versus arbitrary sidecars; decide whether broader copying is desired. | M01/S01 product owner; no silent expansion of export contents. |
| ADR list includes last-played display; current list shows names with player/year selector and Continue details. | M01/S05; choose/display the missing metadata before claiming ADR UI completion. |
| Internal-period manual names, source mutation between inspection/copy, and native path aliases need end-to-end coverage. | M01/S04; do not infer universal safe round trips from lexical checks. |
| Unify list/Continue/action validation and failure interfaces only if deliberately approved. | M01/S01/M06; current validation levels and legacy save semantics differ. |
| Define crash recovery, simultaneous host/client operations and cleanup discoverability. | S04/M01/M17; passing injected failures does not supply durability or locking. |
| Agree timing/display/accessibility targets and complete the missing boundary cases in §4–5. | M01/S05; these remain Open or Proposed, not delivered guarantees. |
