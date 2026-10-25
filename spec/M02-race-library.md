# M02 — Race Library

Status: source-derived specification with explicit policy, adapter and UI gaps.
Inspected: 2026-10-24, `e2eae18cff0f990db20415c1c2262aed2022f95b`, with the
existing local specification collection. Runtime sources are unchanged.
Canonical definitions: [entry point](M02-library-specification.dfy),
[M02Library](dafny/M02-library.dfy), [shared library names](dafny/S01-library-names.dfy).
Verification: [collection run](README.md#m01-and-m02-verification).

## 1. Purpose, scope and ownership

Manage reusable race designs independently of player turns: discover designs,
create/edit them, retain versions, import/export, duplicate and delete selected
entries. Follow the [glossary](../GLOSSARY.md) and
[accepted library decision](../docs/adr/0001-desktop-managed-libraries.md).
A numbered `.rN` identifies a race version, not a game player slot. Stored race
name, current library filename and sanitized filename stem are distinct values.

The implementation lives chiefly in [libraryui.c](../src/ui/libraryui.c), with
race loading/conversion in [race.c](../src/core/race.c), wizard/password UI in
[raceui.c](../src/ui/raceui.c), roster handoffs in
[createui.c](../src/ui/createui.c) and title dispatch in
[mdi.c](../src/ui/mdi.c). This specification does not require a source refactor.

| Owner | Handoff |
| --- | --- |
| M02 | Library location/list, current destination binding, version matching/choice, staged save and exchange/removal. |
| [M04](M04-race-design.md) | Race fields, controls, normalization, credentials and advantage pricing. M02 does not redefine design validity or affordability. |
| [M03](M03-new-game-wizard.md) | Selected race filename and roster customization context; successful customization returns the saved filename/design. |
| [M01](M01-game-library.md) | External-open dispatch and the separate game library. |
| [S01](S01-game-file-formats.md) | Race BOF/records/checksum, UTF-8/legacy conversion, bounded parsing and file publication. |
| [S06](S06-shared-game-state.md), S02/S04/S05 | Player-state distinction, settings, native paths/files and controls/pickers. |

No live player progress is versioned here. Rust/SQL race storage in
[M04's data companion](M04-race-data-model.md) remains a proposal; current
versions are ordinary files. Protected legacy race import has an unresolved
root-instruction/ADR conflict, recorded in §3.5 and §7.

## User stories and requirements

Actors are a player building a reusable collection and a player choosing a race
for New Game. Entry requires accessible per-user storage; operations on an
existing design require a selected filename and, where applicable, credentials.
Goals are source/ADR-derived. [Behavior status and evidence](README.md#requirement-ids-and-status)
are separate; Observed does not imply complete UI coverage.

<a id="m02-us01"></a>
**M02-US01 — Find a reusable design (Observed).** As a player, I want to browse
saved race files so that I can choose a design to edit or use in New Game.
Refresh Race Designs, select an entry and load it. Listing establishes filename/
nonempty-file eligibility; loading can still reject malformed content or fail
authentication. Roster selection remains M03.

<a id="m02-us02"></a>
**M02-US02 — Create or customize a race (Observed).** As a player, I want New
or roster customization to open the race wizard directly so that I can design
before choosing storage details. Edit through M04 and Finish; derive the library
filename from the completed singular name. Cancel restores the wrapper's saved
race/destination context. Saved designs remain separate from setup cancellation.

<a id="m02-us03"></a>
**M02-US03 — Reuse, update or retain a version (Observed).** As a player, I want
unchanged designs reused and changed designs offered Update Existing, Save New
Version or Cancel so that I control earlier revisions. Scan versions by stored
race name, identify the actual target, then save through staging. A selected edit
can update its original filename after a name change; a new version uses the
completed name. Authentication, version exhaustion and publication failure have
distinct outcomes.

<a id="m02-us04"></a>
**M02-US04 — Import a design without changing its source (Observed, with Open legacy policy).**
As a player, I want an external race copied or converted into the library so
that I can reuse it independently. Load/validate, choose a unique filename and
stage/import; external Open then offers editing. Cancelling that edit leaves the
successful import intact. Legacy conversion can require name confirmation and
replacement credentials; the policy conflict is not resolved by this story.

<a id="m02-us05"></a>
**M02-US05 — Make an independent duplicate (Observed).** As a player, I want
Duplicate to create another saved file so that I can work from a copy. Authenticate
and validate the source, choose the first available numbered suggestion, copy
and revalidate. This operation intentionally bypasses identical-design reuse.
Failure can leave different artifacts depending on its stage.

<a id="m02-us06"></a>
**M02-US06 — Export a race (Observed).** As a player, I want an ordinary race
file outside the library so that I can share or archive it. Select destination,
authenticate, stage a copy, validate and replace the target. Cancel or failure
preserves the managed source; export retains its stored credential behavior.
It is not a ZIP or an unlocked race conversion.

<a id="m02-us07"></a>
**M02-US07 — Delete the selected saved version (Observed).** As a player, I want
to remove one named race file after confirmation so that other revisions remain
available. Select Delete, confirm or cancel, then refresh. A failed removal
reports failure; successful removal clears the matching current-path binding.
It does not cascade through other versions or existing games.

## 2. Inputs, outputs and state

| State | Meaning / ownership |
| --- | --- |
| Root | `STARS_LIBRARY_DIR`, else `STARS_SETTINGS_DIR`, else SDL per-user preference root, with `Race Designs` appended. Separate from `Games`. |
| `vplr` / race password / race filename | Working design and credential context consumed by M04 and `FWasRaceFile`. Running-player fields are not a new reusable-race schema. |
| Current library path / new flag | Selected edit destination versus a new race lacking a destination. `FHasCurrentLibraryRace` accepts either state. |
| `RACESTATE` | Snapshot of design, password, filename, current path and new flag used by wrappers. Restoration choices differ after success and cancellation. |
| Version scan | Maximum occupied/relevant version, latest same-name entry, highest-version identical entry. Enumeration is native and can fail. |
| Staging path | Unique temporary directory plus `design.r1` for save/import; export uses an exclusive sibling temporary file at its destination. |
| Roster filename | M03 owns the filename for each selected library race. Customization must bind that slot, not a previously imported global destination. |

M04 owns the [logical race fields](M04-race-data-model.md). `M02Library.Design`
is a comparison projection: the persisted `PLAYER` prefix before `szName`, with
`fLearned` cleared, plus singular/plural strings through their terminators.
The adapter must retain every compared prefix field, including credentials.
It is not an alternate serialization layout or a checksum-only identity.

## 3. Workflows and rules

### 3.1 Enumerate, load and bind an editor

Sources: `FInitRaceLibrary`, `FEnumerateLibraryRaces`, `EnumLibraryRace`,
`FLoadLibraryRace`, `FReadManagedRace`, `SaveRaceState`, `RestoreRaceState`.
Models: `FListed`, `FVersionReadable`, `RaceState`, `Restore`, `WizardExit`.

List entries need a portable race filename and a positive native file size.
Enumeration does not fully parse each design. Version matching uses the stricter
`FReadManagedRace`: reject directories/links, require a current 2.85 race BOF,
then decode silently without password prompts. It restores saved race context,
file-error mode and file-version state after inspection. This prevents browsing
versions from triggering legacy conversion or replacement-password dialogs.

Explicit load uses `FWasRaceFile(..., TRUE)`. Failure restores the wrapper snapshot
and reports inability to open; success binds the current path, filename and
non-new state. Existing protected designs authenticate before editing. The full
race parser and checksum/credential rules remain S01/M04 obligations.

`FBeginLibraryRace` starts from the first predefined race with default singular/
plural labels, clears password/filename and calls M04 without an earlier filename
prompt. `FEditLibraryRace` first loads the selected entry. Both restore prior
binding after wizard exit; on failure they also restore the saved design,
password and filename. On success, the completed design remains in working state
while the outer binding is restored. This is wrapper-state restoration, not a
promise to undo every previously saved file.

`FCustomizeLibraryRace` binds the roster slot's filename, or new-race state for
a predefined choice, then calls the wizard. On success it returns the saved
filename to that slot and retains the completed design; on failure it restores
the snapshot. In both cases it restores the prior outer path/new flag. Thus an
unrelated prior import cannot silently become that slot's save target.

### 3.2 Portable filenames and suggestions

Sources: `FPortableRaceFilename`, `GetRaceLibraryStem`, `SuggestRaceFilename`,
`FChooseLibraryRaceName`, `LibraryNameDlg`.
Models: shared `FRaceName`, `FComponent`, `FReserved`, `FAvailable`, `CleanStem`,
`DropSuffix`, `FUtf8Cut`; M02 `DuplicateStem`, `SuggestedVersion`.

A filename is a nonempty stem followed by case-insensitive `.r` and 1–3 decimal
digits, first digit nonzero: `.r1`…`.r999`. Whole length is below 240 bytes,
`rootBytes + filenameBytes + 2 < sizeof(szBase)`, and portable component checks
reject edge space/period, controls/DEL, `<>:"/\|?*` and reserved Windows stems.
The shared model writes lowercase `r`; native parsing also accepts uppercase `R`.

For wizard saving, derive the stem from the completed singular name: replace
forbidden bytes with `_`, trim edge spaces/periods, substitute the default when
empty and prefix `_` when the prospective filename fails portability. The stored
race name is unchanged. Final validation can still reject path-capacity failures;
prefixing is not a guarantee that an overlong path becomes valid. Unlike game
title suggestions, internal periods in race stems are retained.

Import naming keeps a unique-name dialog. The model uses `SuggestedVersion ==
1000` for exhaustion; the source returns an empty suggestion instead. `SuggestRaceFilename` strips the last
extension from its input and tries the first available `.r1`…`.r999`; availability
includes occupied filenames even if their contents are invalid. Suggestions and
native path conversions must retain complete UTF-8 code points. Native
case-insensitive comparison defines collision/name grouping; no normalization
policy is added. The formal key adapter constrains ASCII folding and leaves
non-ASCII native comparison explicit; the tests include `Élan`/`éLAN` cases.

An observed duplicate edge case is retained: `FDuplicateLibraryRace` strips
`.rN` before calling the helper, which strips one more dotted suffix. Duplicating
`A.B.r1` therefore suggests an `A.rN` stem, not `A.B.rN`. The model records this
behavior; changing it is separate work.

### 3.3 Find versions and choose a save target

Sources: `FindRaceVersion`, `FSameLibraryRace`, `FSaveCurrentLibraryRace`,
`LibraryRaceSaveDlg`. Models: `Candidate`, `Versions`, `Scan`, `Target`, `Plan`,
`FSame`, `OccupiedStemIsNotRace`, `VersionAboveOccupied`.

For each portable filename, count its numeric suffix toward the maximum if its
stem matches the newly derived stem, even if the file cannot be decoded. Then
attempt a silent current-format read. Only a matching stored singular race name
(case-insensitive) joins the version family; this includes unrelated filenames
such as `Imported.r8` holding a race named `Legacy`.

Within matching races, count the suffix toward the maximum. Latest is the
highest numeric version, breaking equal-version ties with native case-insensitive
filename ordering. Identical reuse takes the highest numeric identical version;
equal identical-version ties retain the first encountered entry. Complete design
comparison clears `fLearned`, compares the prefix and both terminated names;
unused bytes after the terminators and the checksum alone do not define equality.
Names and credentials therefore matter even if pricing is unchanged.

Save choice proceeds in this exact order:

1. An existing edit uses its selected current filename, even if the completed
   race name changed. New uses the latest matching stored-name entry.
2. If that target exists but cannot be read, fail before trying another reuse.
   If it is identical, reuse it immediately, without a save-choice dialog.
3. Otherwise reuse an identical matching-family entry if found, also without
   writing. This precedes asking Update/New Version/Cancel.
4. If no existing target remains, choose New Version directly. Otherwise show
   the target filename with Update Existing, Save New Version and Cancel.
5. New Version uses maximum-plus-one, considering both same-name versions and
   occupied derived-stem filenames. Maximum 999, invalid final filename or
   unavailable destination fails. This is not a first-hole search.
6. Update uses the selected/latest filename. A New workflow updating a protected
   existing entry loads/authenticates that target while preserving the completed
   design. An existing edit authenticated when opened.
7. Save through `FSaveLibraryRace`; reuse or successful save updates current
   path/filename and clears new state. Failure/cancel returns to the M04 workflow.

Distinct race names that sanitize to one stem affect filename occupancy but do
not become each other's update target. Explicit Duplicate uses a separate path
and can deliberately create identical copies.

### 3.4 Staged save, cancellation and failure

Source: `FCreateRaceStagePath`, `FSaveLibraryRace`; models `StageAttempts`,
`PublishedBytes` and
S01's publication/native-file boundary. The stage allocator tries at most 1000
unique tick/counter directory names, writes a race BOF/player/checksum EOF to
`design.r1`, closes and replaces the selected destination. Failed creation or
replacement attempts removal of temporary artifacts; only successful replacement
updates the current path/filename/new flag.

The source does not re-read the saved candidate before replacement and does not
expose separate checks for every `WriteRtPlr`/`WriteRt`/`StreamClose` result here.
Consequently `PublishedBytes` is conditional on supplied candidate bytes and the
native replacement result; it does not prove completed-write durability, correct
flush error handling or a transaction across all files. Existing injected save
failures test a narrower boundary. Cleanup itself can fail.

Cancel before publication leaves existing saved versions untouched. Wizard
wrappers restore their owned context as §3.1 specifies; M04 retains its own
working-record/normalization behavior. Cancelling New Game later does not remove
a race already independently saved by customization.

### 3.5 Import and legacy conversion

Sources: `FImportLibraryRace`, `FImportLibraryRaceAs`,
`FImportLibraryRaceAndEdit`, `FWasRaceFile`, `FConvertLegacyRaceNames`,
`FPromptLegacyRacePassword`. Models: S01 race validation/conversion predicates,
M02 `ImportAndEdit`; `LegacyProtected` makes the two policies explicit.

Choose a source, load/validate with credentials as applicable, derive an initial
filename suggestion from its source stem and obtain a unique portable library
name. A caller-supplied import name must pass the same validity/availability
checks. Cancel restores saved race context.

For current 2.85 input, copy into a race stage and replace the chosen unique
library destination, then validate the published destination again. Failed
post-publication validation attempts to remove it and restores the snapshot;
that path is not a pre-publication validation guarantee. Successful import binds
the imported filename/path. For legacy input, conversion occurs in memory and
`FSaveLibraryRace` writes current 2.85 content; it does not preserve legacy bytes
or the old protected hash. Original source files are retained.

External Open routes a race to Import and then Edit. Successful import returns
success even if the subsequent edit is cancelled. The imported file remains;
`ImportAndEdit` formalizes that distinction.

**Policy conflict remains Open:** root [AGENTS.md](../AGENTS.md) permits only
unprotected legacy races, while accepted
[ADR 0002](../docs/adr/0002-utf8-application-text.md) and inspected source support
protected legacy races after original checksum/record validation and confirmation
of a replacement UTF-8 password. The old password is not authenticated or
recovered. Source also requests confirmation when converted names need shortening.
`LegacyProtected` has separate policy inputs; its conversion branch is an observed
source/ADR description, not authorization to override the root instruction.
This documentation changes neither policy nor runtime.

### 3.6 Duplicate, export and delete

Sources: `FDuplicateLibraryRace`, `FCopyRaceExport`, `FExportLibraryRace`,
`FDeleteLibraryRace`. Models: `DuplicateStem`, shared availability, `Restore`,
`PublishedBytes`; actual file operations are external outcomes.

Duplicate authenticates/validates, computes a first-free numbered suggestion,
copies bytes directly to the chosen destination and revalidates. It restores
outer race state on success and failure. Failed revalidation removes the copied
file; direct copy failure returns without an explicit destination cleanup in
this wrapper, so a partial file can remain. It is not the staged Save New Version
workflow. Successful duplication preserves source bytes, including protection.

Export offers a race-file picker with overwrite confirmation, then authenticates
and rejects a case-insensitive identical source/target path. It exclusively copies
to a temporary sibling, validates that copy and replaces the destination. Failure
cleans the temporary file where possible; wrapper state is restored. A second
validation can exercise credential behavior too. Native path-alias/concurrent
writer behavior is not established by the lexical same-path check.

Delete checks a portable name and nonempty file, asks a named Yes/No confirmation,
removes that one file and clears an exactly matching current-path binding. It
does not authenticate the race or delete all matching-name versions. Failure to
remove produces an error; cancellation leaves the selected file. Existing game
players contain their own data and are not erased with the reusable definition.

## 4. Acceptance and traceability

Observed rows describe current code; policy conflict is explicitly Open.

| ID / story / status | Given / when / then | Rule and evidence | Test or gap |
| --- | --- | --- | --- |
| <a id="m02-ac01"></a>M02-AC01 / US01 / Observed | Given a nonempty portable `.rN`, list it; listing alone does not certify the race. Version scanning rejects links and non-current BOFs without conversion prompts. | `FListed`, `FVersionReadable`; §3.1. | Race-library fixtures; malformed-listed-file combinations need broader coverage. |
| <a id="m02-ac02"></a>M02-AC02 / US02 / Observed | Given New, open M04 directly without a filename prompt; derive a portable filename at Finish while preserving the in-game singular name. | Shared `CleanStem`, `FRaceName`; New/save source. | `FCheckRaceVersionWorkflow` explicitly rejects an early naming dialog. |
| <a id="m02-ac03"></a>M02-AC03 / US02 / Observed | Given failed/cancelled wizard completion, restore the wrapper snapshot; on success retain completed design but restore the outer destination binding. | `WizardExit`, `CancelRestores`; §3.1. | Race-cancellation and roster workflows. |
| <a id="m02-ac04"></a>M02-AC04 / US03 / Observed | Given identical selected design, reuse it before prompting/writing; credentials and names participate in equality while `fLearned` and trailing name storage do not. | `FSame`, `Plan`; `FSameLibraryRace`. | Version reuse tests; prefix-extraction conformance is S01/native evidence. |
| <a id="m02-ac05"></a>M02-AC05 / US03 / Observed | Given `Imported.r8` contains stored name `Legacy`, a changed New `Legacy` can target that entry or create `Legacy.r9`. | `Scan`, `Target`, `Plan`; §3.3. | Imported-version fixture and actual target prompt assertion. |
| <a id="m02-ac06"></a>M02-AC06 / US03 / Observed | Given a different race with the same sanitized stem, count its occupied version but never choose it as the update target. | `OccupiedStemIsNotRace`; `FindRaceVersion`. | General model property; dedicated collision matrix remains partial. |
| <a id="m02-ac07"></a>M02-AC07 / US03 / Observed | Given max version 7, New Version selects 8; given 999, fail; an available earlier hole is not used. Identical reuse can still succeed. | `VersionAboveOccupied`, `Plan`; naming source. | Model witnesses; runtime 999-exhaustion fixture remains missing. |
| <a id="m02-ac08"></a>M02-AC08 / US03 / Observed | Given an edit whose race name changes, Update retains the selected filename; New Version uses the completed name. Cancel/failing replacement preserves earlier versions at the tested boundary. | `Target`, `Plan`, `PublishedBytes`; §3.3–3.4. | Version/failure tests; write-flush/crash durability not proved. |
| <a id="m02-ac09"></a>M02-AC09 / US03 / Observed | Given New updating a protected existing target, require its existing credentials; restoring the authentication snapshot retains the completed candidate. | `Plan`, `Restore`; `FSaveCurrentLibraryRace`. | Source/formal branch; broad protected-update GUI coverage remains a gap. |
| <a id="m02-ac10"></a>M02-AC10 / US04 / Observed | Given successful import followed by cancelled Edit, retain the imported copy and report import success; external source stays unchanged. | `ImportAndEdit`, `ImportSurvivesEditCancel`; import source. | Race-library external-open and source snapshot tests. |
| <a id="m02-ac11"></a>M02-AC11 / US04 / Open policy | Given a protected legacy race, root policy rejects support; source/ADR conversion requires validated input and accepted replacement credentials. Do not merge these into one approved requirement. | `LegacyProtected`; S01 legacy contracts and ADR 0002. | Existing legacy conversion harness is observed implementation evidence, not policy resolution. |
| <a id="m02-ac12"></a>M02-AC12 / US05 / Observed | Given explicit Duplicate, produce an independent byte copy despite identical-version reuse; restore outer working state. A dotted stem follows the documented double-strip behavior. | `DuplicateStem`, `Restore`; §3.6. | Race duplication/source snapshots; dotted-stem behavior modeled, focused native fixture missing. |
| <a id="m02-ac13"></a>M02-AC13 / US06 / Observed | Given failed export replacement, preserve the previous destination at the native replacement boundary; keep the managed source and stored protection. | `PublishedBytes`; staged copy/validation source. | Export fault injection and byte snapshots; aliases/durability external. |
| <a id="m02-ac14"></a>M02-AC14 / US07 / Observed | Given cancelled deletion, retain selected file; successful deletion removes only that version and leaves siblings/source exports. | §3.6; native single-file removal outcome. | Race-library deletion and siblings checks; no version cascade exists. |

## 5. Interaction and quality requirements

| ID / status | Requirement and check / target | Owner and remaining evidence |
| --- | --- | --- |
| <a id="m02-ir01"></a>M02-IR01 / Observed | Expose New, Edit, Duplicate, Import, Export and Delete on Race Designs; save choices name the actual Update target and distinguish Update/New Version/Cancel. | M02/S05; title controls and version prompt tests. |
| <a id="m02-ir02"></a>M02-IR02 / Observed | Keep import filename choice separate from wizard-derived New naming, and credential prompts separate from point affordability. Verify cancelled import/edit/save produces the documented state. | M02/M04; §3 and race workflows. |
| <a id="m02-ir03"></a>M02-IR03 / Proposed | All library and save-version choices support keyboard operation, visible focus and readable labels independent of color; error return leaves a useful correction target. | M02/S05/M04; full accessibility, picker and focus audit is incomplete. |
| <a id="m02-ir04"></a>M02-IR04 / Proposed | Explain stored race name versus filename/version, identical reuse and source-preserving import/export in consistent feedback; identify partial-copy/recovery artifacts when cleanup fails. | M02/S05; a uniform diagnostic/result interface is not implemented. |
| <a id="m02-qr01"></a>M02-QR01 / Observed | Preserve complete compared design fields, credentials, fixed record widths and UTF-8 byte limits. Test byte snapshots plus decoded names/credentials, not checksum alone. | S01/M04/M02; existing equality/version tests and formal projection. |
| <a id="m02-qr02"></a>M02-QR02 / Observed | Preserve explicit `.r1`…`.r999` bounds, occupied-stem accounting, path budgets and stage attempt limits. Test invalid digits, reserved stems, UTF-8 and full-version boundaries. | M02/S01/S04; model/source; not all runtime exhaustion cases tested. |
| <a id="m02-qr03"></a>M02-QR03 / Observed | Retain earlier versions on Cancel and at checked save-replacement failures; report duplicate direct-copy and post-publication import limitations separately. | M02/S01/S04; fault injection supports selected stages, not all write/close failures or crash recovery. |
| <a id="m02-qr04"></a>M02-QR04 / Open | Measure enumerate/scan/save/import/export latency, file opens and memory for 0/1/999 versions plus unrelated names on defined storage/platform; report median/p95 and failed reads. | M02/S04/S05; agree budgets and total collection limits. No benchmark result is claimed. |
| <a id="m02-qr05"></a>M02-QR05 / Open | Define supported viewport/DPI/localization profiles and test long filenames, Unicode labels, password and save dialogs for readable/reachable controls. | S05/M02/M04; minimum display and assistive-technology acceptance remain unagreed. |

## 6. Formal coverage and boundaries

| Rule area | Canonical definitions / external obligation |
| --- | --- |
| Race contents, affordability, credentials | Existing M04 `Race`, `Finish`, pricing and credential definitions; S01 packed race records/checksum/legacy contracts. No second calculator or binary layout. |
| Name/number/path validation | Shared `FRaceName`, `FComponent`, `FReserved`, `FAvailable`, `CleanStem`, `FUtf8Cut`; input slicing, localized fallback and native Unicode comparison are adapter obligations. |
| Listing/read levels | `FListed`, `FVersionReadable`; actual path kind, BOF and decode results come from native/S01 inspection. Listing is deliberately weaker. |
| Version scan | `Candidate`, `Versions`, `Scan`, `FSame`; a candidate version is 1–999. Apply `Scan` in enumeration order with the native filename comparison result; complete read/error effects are external. |
| Save decision/authentication | `Target`, `Plan`, selected equality/version lemmas. Native authentication outcome is input, not a cryptographic security proof. |
| Context and roster isolation | `RaceState`, `Restore`, `WizardExit`; actual UI/global aliasing and M03 call order remain native obligations. |
| Save/import/export | `PublishedBytes`, `ImportAndEdit`, S01 publication conditions; actual bytes, close/flush, replacement, cleanup, cancellation callbacks and concurrent filesystem changes are external. |
| Duplicate/delete | `DuplicateStem`, context restore and single-file native removal described in §3.6. Partial destination artifacts and path aliases are not abstracted into an all-or-nothing guarantee. |
| Legacy policy | `LegacyProtected` records separate root and accepted-ADR outcomes; it does not resolve precedence or authorize a behavior change. |

The formal `Design` projection must be extracted exactly as `FSameLibraryRace`
describes. Equality of two projected values proves that comparison's result,
not equality of arbitrary `PLAYER` memory or native files. Race-version metadata
is not race pricing input. These checks establish selected model properties;
C refinement, end-to-end filesystem guarantees and accessibility remain unproved.

## 7. Evidence, verification and open decisions

Relevant test sources are the [race-library runner](../tests/compat/run_racelibrary.py),
[version workflow](../tests/compat/win32_raceversion_workflow.h),
[roster workflow](../tests/compat/win32_raceroster_workflow.h),
[startup harness](../tests/compat/win32_client_startup.c) and
[race unit tests](../tests/unit/test_race.c).
See the [shared verification record](README.md#m01-and-m02-verification) for this
batch's actual commands/results; named harness coverage is not an assertion that
every acceptance row is exercised.

| Decision / gap | Owner / consequence |
| --- | --- |
| Reconcile protected legacy race support in root rules, accepted ADR 0002, S01/M04 and current source. | Repository owner/S01/M02; policy remains Open and no rule is silently overridden. |
| Decide whether dotted-stem duplication should retain the full stem; add a native case before changing behavior. | M02; current double-strip behavior is recorded, not fixed. |
| Verify write/close failure handling and cleanup artifacts across Save, Duplicate, Import and Export. | M02/S01/S04; currently different publication/cleanup guarantees. |
| Define native non-ASCII comparison, alias and concurrent-update expectations across platforms. | S04/M02; ASCII model and selected UTF-8 tests are not full equivalence coverage. |
| Decide whether the list should validate/show corrupt/legacy entries rather than merely failing at load. | M02/S05/S01; changing enumeration is a separate UX/behavior decision. |
| Agree performance/display/accessibility targets and complete protected-update, version-exhaustion and focused failure tests. | M02/S05/M04; §4–5 explicitly retain their gaps. |
