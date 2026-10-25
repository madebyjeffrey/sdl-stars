# S02: Application Preferences and Window Layout

Status: source-derived detailed specification; logical ownership is proposed.
Inspected: `e2eae18cff0f990db20415c1c2262aed2022f95b`, 2026-10-24.

Canonical entry: [S02-preferences-specification.dfy](S02-preferences-specification.dfy);
[rules and proofs](dafny/S02-preferences.dfy).
The [preference contracts companion](S02-preference-contracts.md) inventories
every active key and specifies desktop/host grammar, UTF-8 buffers, validation,
save control/serialization, widths/defaults/write conditions and tile/placement transitions. Behavior statuses and evidence follow
[the collection conventions](README.md#requirement-ids-and-status).

## 1. Purpose, actors and scope

Read, normalize and persist application preferences and presentation state.
Game rules/orders belong to M06/M08–M20, game files to S01, logical entities to
S06, native paths/encoding to S04 and actual controls/windows to S05. Preferences
include host defaults but are not authoritative game state. Tutorial temporary
presentation save/restore belongs to M16. This module has no independent settings
screen: the callers edit state through the existing desktop and INI interface.

Primary actors: A player restoring a usable desktop and a developer/operator selecting a settings location.

### User stories

These source-derived stories describe intent, not evidence of user research.
The prerequisites and triggers are the corresponding workflows below. All
stories describe Observed capabilities with the stated limitations.

<a id="s02-us01"></a>
**S02-US01 — Restore a usable layout.** As a player, I want reasonable restored window and pane settings so that I can resume using the desktop.

<a id="s02-us02"></a>
**S02-US02 — Keep my presentation choices.** As a player, I want tile, toolbar and report sorting choices retained so that repeated work stays familiar.

<a id="s02-us03"></a>
**S02-US03 — Locate settings predictably.** As an operator, I want explicit and per-user settings paths so that profiles can be isolated for normal use and tests.

<a id="s02-us04"></a>
**S02-US04 — Recover from invalid settings.** As a player, I want malformed or out-of-range preferences handled visibly and conservatively so that they do not become game-state changes.

### Story entry and completion conditions

| Story | Trigger and prerequisites | Completion / alternate outcome |
| --- | --- | --- |
| [S02-US01](#s02-us01) | Start desktop with stored placement or missing/default settings. | Normalize bounds and restore available layout; monitor recovery remains qualified separately. |
| [S02-US02](#s02-us02) | Change tiles/toolbar/report sort then write settings. | Persist current presentation per key; individual failures need caller handling. |
| [S02-US03](#s02-us03) | Open a profile using an explicit path or bare name with environment context. | Apply deterministic precedence then strict UTF-8/path handling. |
| [S02-US04](#s02-us04) | Read malformed/out-of-range/missing profile data. | Reject malformed data or apply source defaults/clamps; never reinterpret it as orders. |

## 2. Inputs, outputs and state

| Area | Contract |
| --- | --- |
| Storage | UTF-8 profile filename, section/key/value lines, application directory selection and unrelated preserved lines. |
| Presentation | Window rectangles, layout mode, toolbar, tile order/expansion/column, pane dimensions and selected object kind. |
| Defaults/history | Recent files, report sort choices, host backup count, generation options and other application defaults. |
| Lifetime | Loaded at startup and written from desktop shutdown/settings paths; per-dialog rollback is not implied by persistent defaults. |

Persistent records remain [S01](S01-game-file-formats.md); logical state and
identity remain [S06](S06-shared-game-state.md). A derived/working value must not
be treated as a separately committed record or as information known to every player.

## 3. Workflows, rules and handoffs

### Location and loading

[profile.c](../compat/win32/src/profile.c) `SzProfilePath` keeps explicit paths
literal. A bare name chooses `STARS_PROFILE_DIR`, then `STARS_SETTINGS_DIR`, then
the platform user location, then the literal working-directory name. Windows
uses `LOCALAPPDATA/stars`; POSIX uses absolute `XDG_CONFIG_HOME/stars`, else
absolute `HOME/.config/stars`. `Location` models precedence, given classified
availability; it does not resolve a path. Path construction must pass strict
UTF-8 validation. No profile contents or credentials should be copied into
specification diagnostics.

`FLoadProfile` reads UTF-8, accepts an initial BOM, rejects malformed text or
more than 8 MiB, preserves raw unrelated lines and matches section/key names
case-insensitively. Missing files permit defaults; other read failures retain an
error. Pure parsing, lookup and ordered key/section updates are specified in the
[companion](S02-preference-contracts.md#desktop-profile-grammar-and-ordered-updates).
The public reader returns defaults after a load failure while retaining its
error; the writer refuses the failed load. Persistence, buffer adapters and
process-local locking remain E02-1. Per-key writes are not a transaction spanning every setting.

### Normalization and layout

[init.c](../src/ui/init.c) `ReadIniSettings` initializes generation flags false,
reads placement rectangles and recognizes the unset-position sentinel -32768.
`ProfileI16` preserves the source’s 16-bit narrowing before comparison.
`Layout` then clamps arrangement to 0–2 and `PaneSize` clamps pane dimensions to
10–2000. These are not clamps applied directly to an unbounded INI integer: raw
32768 maps to layout 0, while raw 65636 maps to pane size 100.
Fallback dimensions are caller-specific (including 396/110/192), not one global
window size. Toolbar visibility and tile expansion are boolean choices. Recent-file slots
are byte-bounded; source entries shorter than four bytes are cleared, then the
remaining entries compact in order (`Compact`). `LoadRecent` preserves the
separate startup decision from original File1; `Add`/`DeleteWrites` capture
different failure handling and partial memory/persisted effects. Saved selection
and GameID decoding, plus preset codecs, are specified in the
[state/preset companion](S02-preference-contracts.md#selection-identity-and-recent-file-state).
S01/S04 own UTF-8/file paths; M07/M06 own live selection restoration and M09/M10
own applying/editing presets; [M09](M09-production-management.md#32-templates-and-automatic-policies) and [M10](M10-fleet-operations.md#311-applying-transport-presets) now provide checked command/cancellation contracts.

`ReadIniTileSettings` interprets tile letters as IDs, case as expansion and `*`
as the next column, capped at one by `Column`. It finds unused matching tiles
and swaps them into the configured prefix; duplicates already consumed do not
create another tile. Unspecified tiles remain after the prefix, possibly reordered by those swaps,
and their columns are raised to at least the final parsed column. `SortColumn` and `Ascending` decode the low-byte column and bit 8.
The range rule `Backups` accepts 1–999, otherwise 1. Both desktop and host first
store the parsed numeric value in signed 16-bit `vcBackupDirs`, so the complete
normalizer is `ConfiguredBackups`, reused by M17: parsed 65538 becomes 2.
Applying `Backups` directly to 65538 would incorrectly give 1. Parsing is a
separate boundary: the desktop accepts a hexadecimal prefix; host `LIniInt` uses
`atol`, so the literal text `0x2` does not give the same parsed value.

### Saving, cancellation and recovery

[mdi.c](../src/ui/mdi.c) `WriteIniSettings` serializes current preferences.
The profile implementation preserves unrelated lines on updates; a write can
fail and must not be called successful merely because the in-memory preference
changed. Dialog-specific Apply/Cancel semantics belong to the caller: closing
the application is not an undo of prior presentation changes. On restart, the
persisted values/defaults are authoritative. Invalid placement on a changed
monitor arrangement, multi-process concurrent writes and comprehensive recovery
from partial multi-key writes are E02-1/E02-2. Do not infer a whole-profile
transaction or universal offscreen recovery from individual clamping rules.

## 4. Acceptance and traceability

All criteria below have **Observed** behavior status except where a rule is
explicitly a caller convention or repository maintenance constraint. A named
model/lemma supplies mathematical evidence within its preconditions. An E-boundary
supplies source evidence and an explicit implementation-test/refinement gap,
not a proved runtime outcome. The dated collection run records actual checks.

| Criterion / story | Given / When / Then | Authoritative rule, source and evidence scope |
| --- | --- | --- |
| <a id="s02-ac01"></a>**S02-AC01** / [S02-US01](#s02-us01) | Given a parsed pane value, when loading it, then narrow to signed 16-bit before clamping to 10–2000; raw 65636 becomes 100. | `ProfileI16`, `PaneSize`, vectors; ReadIniSettings |
| <a id="s02-ac02"></a>**S02-AC02** / [S02-US01](#s02-us01) | Given a parsed layout value, when normalizing, then use its signed 16-bit interpretation before clamping to 0–2; raw 32768 becomes 0. | `Layout`, `LayoutBound`, vectors; ReadIniSettings |
| <a id="s02-ac03"></a>**S02-AC03** / [S02-US02](#s02-us02) | Given repeated column separators, when decoding layout, then do not create a third column. | `Column`, vectors; ReadIniTileSettings |
| <a id="s02-ac04"></a>**S02-AC04** / [S02-US02](#s02-us02) | Given stored sort bits, when decoding, then use the low byte for the column and bit 8 for ascending. | `SortColumn`, `Ascending`; E02-2 caller ranges |
| <a id="s02-ac05"></a>**S02-AC05** / [S02-US03](#s02-us03) | Given an explicit path plus configured profile directory, when choosing location, then preserve the explicit path. | `Location`, vectors; SzProfilePath |
| <a id="s02-ac06"></a>**S02-AC06** / [S02-US03](#s02-us03) | Given two profile directory overrides, when selecting a bare-name location, then prefer STARS_PROFILE_DIR. | `Location`; SzProfilePath |
| <a id="s02-ac07"></a>**S02-AC07** / [S02-US04](#s02-us04) | Given malformed UTF-8 profile text, when loading, then discard loaded rows; the public reader returns fallback values with ERROR_BAD_FORMAT, while writes refuse that load. | E02-1; FLoadProfile |
| <a id="s02-ac08"></a>**S02-AC08** / [S02-US04](#s02-us04) | Given empty recent-file entries between nonempty entries, when compacting, then preserve the surviving order. | `Compact`, vectors |
| <a id="s02-ac09"></a>**S02-AC09** / [S02-US04](#s02-us04) | Given parsed retention 65538, when loading preferences, then narrow to signed 16-bit value 2 and accept it; 1000 still falls back to 1. | `ConfiguredBackups`, vectors; ReadIniSettings |

## 5. Interaction and quality requirements

<a id="s02-ir01"></a>
**S02-IR01 — Caller and interaction requirement (Proposed).** Presentation preferences must remain reachable through the owning desktop controls. The current normalization rules are Observed; keyboard reordering, focus after restoring layout, large fonts, screen-reader labels and recovery on removed displays remain Open S05/S02 qualification.

<a id="s02-ir02"></a>
**S02-IR02 — Feedback and recovery (Proposed).**
Use the success, no-change, cancellation and failure distinctions in §3. The
owning caller must not imply that a forecast, accepted request or attempted write
is a confirmed final result. Section 3 records current outcomes and their limitations; this requirement does
not establish implementation. Complete keyboard/focus/error-announcement acceptance remains Open with the
caller and S05. No accessibility conformance level is asserted.

<a id="s02-qr01"></a>
**S02-QR01 — Integrity and compatibility (Observed constraints).**
Keep settings UTF-8 and separate from save-game records. Bound panes, layout and retention according to the named rules; do not promise whole-profile atomicity. Startup/save latency and multi-display behavior need measured targets; profile secrets are not diagnostic output.

Scenario/check: source/model boundary review and the native regression/build
checks below. Target: preserve the specified existing results and file/catalog
contracts; any intentional runtime change is separate work. Evidence: named
definitions, source pointers and dated checks; uncovered adapters are in §6.

<a id="s02-qr02"></a>
**S02-QR02 — Performance and resource budgets (Open).**
Scenario: the largest supported workload for these workflows on each supported
backend. Method: measure wall time, peak allocation and caller-visible response
time with controlled inputs. Target: no agreed numeric budget or new measurement
in this specification batch. Owner: S02 and the consuming module/platform owner;
absence of a budget does not establish responsiveness.

<a id="s02-qr03"></a>
**S02-QR03 — Text and presentation (Observed boundary / Open qualification).**
Use UTF-8 byte capacities and complete-code-point truncation from
[S04](S04-platform-services.md) and [Unicode boundaries](../docs/UNICODE-BOUNDARIES.md).
Native controls and rendering use [S05](S05-sdl-desktop-implementation.md);
headless/internal APIs have no independent viewport. Check non-ASCII boundary
inputs and the actual caller on supported displays. Target: valid text and the
stated capacities; complete font, large-text, IME and assistive-technology coverage
remains Open with S05 and the caller.

## 6. Coverage inventory and open decisions

The definitions named in §§3–4 are canonical for their stated projections.
The following inventory accounts for rules requiring native inputs/effects or
further decomposition; it is not a claim that their algorithms are proved.

| Boundary | Scope and limitation | Owner / next evidence or decision |
| --- | --- | --- |
| E02-1 | Pure parsing, list edits, enumeration/scalar buffers, argument validation, UTF-8 prefixes, host grammar and save serialization/control now have checked definitions. Outer loader/writer effects, errno/path mapping, native-operation refinement, locks and cross-call/concurrent writes remain open. Location is precedence, not filesystem refinement. | S02/S04/S05; close the companion D/A inventory and add failure fixtures. |
| E02-2 | Tile permutations, coordinate parsing, scalar widths, all active keys/defaults/write conditions, saved selection/GameID decoding, MRU transitions and preset codecs now have companion contracts. Remaining D/A: selection/identity formatting, full selection/editor/title/tutorial callback effects (M07 now models restoration dispatch), full M09/M10 editor integration beyond their checked preset/quantity commands, and the conditional tile round-trip proof. Report-column consumers, actual placement and monitors/DPI remain adapter/qualification work. | S02/S05/M06/M07/M09/M10/M15/M16; companion contracts and native fixtures, then supported-display/keyboard qualification. |

## 7. Evidence and verification

[settings reader](../src/ui/init.c), [settings writer](../src/ui/mdi.c), [profile implementation](../compat/win32/src/profile.c), [profile tests](../tests/compat/win32_profile.c), and [UTF-8 boundaries](../docs/UNICODE-BOUNDARIES.md). Per-key source defaults remain authoritative where not normalized by the named model.

[Collection verification](README.md#remaining-specifications-verification)
records exact commands and outcomes for this batch. Proofs establish only the
named model properties and witnesses, with their preconditions; they do not prove
that C refines the models or that every UI/platform workflow has been tested.
The boundary inventory and Open interaction/quality requirements remain visible
after a successful formal/build/regression run.
