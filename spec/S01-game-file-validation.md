# S01 validation, acceptance and evidence

Part of [S01](S01-game-file-formats.md). This document separates the well-formed
format, currently implemented checks, proposed stricter validation, and tests
actually run. A complete format description does not imply that every malformed
record is safely handled by the present implementation.

## Story-linked requirements

This companion is the evidence/acceptance owner for
[S01-US01](S01-game-file-formats.md#s01-us01) through
[S01-US04](S01-game-file-formats.md#s01-us04). The direct actor is a developer or
tester deciding what a file adapter actually guarantees. Inputs are the chosen
entry point, file/context, expected behavior status and a reproducible test case.

Workflow: identify the current reader/writer contract, select the applicable
existing `S01-A01`…`S01-A30` vector or a specific new case, execute/model it, then
record source/proof/test evidence separately. A missing test remains a gap; a
proposed strict rejection is not relabeled as current behavior. Failure requires
the actual first discrepancy and partial-state outcome rather than a generic
“invalid file” conclusion.

| ID / parent story / status | Acceptance criterion | Rule, evidence and coverage |
| --- | --- | --- |
| <a id="s01-val-ac01"></a>S01-VAL-AC01 / US01–US03 / Observed evidence distinction | Given the same payload reaches different entry points, when assessing acceptance, then use each entry's actual checks and limits rather than transferring a stronger check from another reader. | [Entry matrix](#1-validation-by-entry-point), source map and existing [S01-A vectors](#3-concrete-acceptance-vectors). This is an evidence-review criterion, not a new executable reader rule. |
| <a id="s01-val-ac02"></a>S01-VAL-AC02 / US04 / Proposed | Given malformed length/domain/reference input, when a future bounded decoder validates its candidate, then reject before publishing it and report any compatibility repair explicitly. | [Strict contract](#2-proposed-strict-decoder-contract); S01 `ReadRecord`, `PublishDecoded`, kind-specific predicates; S06 validated boundary. Complete strict adapter/C-refinement proof is missing. |
| <a id="s01-val-ac03"></a>S01-VAL-AC03 / US01–US04 / Observed evidence distinction | Given a named test or lemma, when reporting coverage, then identify the property/input scope and dated run result or state that no new run occurred; a passed proof is not certification of the C decoder. | [Evidence limits](#4-existing-evidence-and-its-limits), [verification history](#5-verification-for-this-specification-change); collection verification workflow. Evidence metadata is documentary, not an invented game-state transition. |

| ID / status | Interaction or quality requirement and check | Owner / gap |
| --- | --- | --- |
| <a id="s01-val-ir01"></a>S01-VAL-IR01 / Proposed process | Diagnostics identify entry point, expected status, actual outcome and relevant partial-state effects, so a developer can reproduce the discrepancy. User-facing wording/focus/accessibility remain workflow-owner obligations. | S01 test/decoder maintainers; this review document itself has no in-game controls or viewport. |
| <a id="s01-val-qr01"></a>S01-VAL-QR01 / Observed process | Keep the existing acceptance-vector IDs stable and maintain links to canonical models/source/test evidence. Re-run collection proof/audit/link checks after changes and report failures. | Spec verification workflow; no schema predicate is promoted to runtime enforcement. |
| <a id="s01-val-qr02"></a>S01-VAL-QR02 / Open | Add the missing malformed-input, restoration and aggregate resource cases identified in §3/§6; record corpus/platform and expected oracle before measuring cost or declaring coverage complete. | S01/S04 and [S01-QR03](S01-game-file-formats.md#s01-qr03); coverage/budgets and unresolved compatibility policies remain explicit. |

This section indexes the existing vectors rather than replacing them with a
second list. [Open decisions](#6-open-decisions-and-follow-up-boundaries) retain
the protected-race, unknown-record, bounded-reader and publication policy gaps.

Checks for this story expansion are recorded in the
[collection verification](README.md#story-expansion-verification).

## 1. Validation by entry point

| Layer / entry point | Checks observed | Important limits |
| --- | --- | --- |
| `RgFromStream` | Physical short reads fail and long-jump; `ReadRt` requires BOF payload size 16. | The tutorial memory-stream path copies without a supplied end bound. Non-BOF lengths are not globally checked against their schema. |
| `FOpenFile` | BOF tag; major/minor 2.85 with distinct old/new errors; player; game identity when established; turn; orders generation; submitted flag while generating without force. | Does not explicitly check magic/dt. History turn may differ. At game.turn=0, adopts snapshot turn/generation. Multi-footer check uses `rt!=EOF && cb!=2`, weaker than requiring both correct. |
| `FMarkFile` | Version and game identity before changing status/AI marker. | No comprehensive file parse; in-place header/record update, not transactional rewrite. |
| `FLoadGame` universe | GAME tag/64-byte size, NUL/UTF-8 title, upper coordinate/name-ID bounds, EOF. | Counts, all scalar domains, lower coordinate bounds and trailing bytes do not have one comprehensive validation pass. |
| `ReadRtPlr`, `FReadUserString` | Prefix/name bounds, bounded relation copy, valid UTF-8 and complete string encodings. | Modern extra relations can be skipped; trailing bytes need not be exhausted; player identity/counts require caller discipline. |
| `FReadShDef`, `UnpackBattlePlan` | Design prefix/slot/name bounds, hull upper bound; plan prefix/name. | Some values are repaired. Whole rosters, part IDs and references are not comprehensively validated by these helpers alone. |
| `FReadPlanet`, `FReadFleet` | Selected domain/tag/reference checks, environment <=100, mineral-level width restrictions, damage clamping, waypoint/name parsing. | Variable field cursor is not uniformly bounded by hdrCur.cb; masks/counts/negative IDs can reach assumptions. These are not general hostile-input-safe parsers. |
| `LpmsgplrFromRt` | Header/declared-text length, decompression bounds, NUL and UTF-8. | Malformed player messages are skipped. Catalog messages use catalog-derived counts without an equivalent full bounded validation layer. |
| `FWasRaceFile` | BOF major 2/minor49–85, race kind, PLAYER then 2-byte checksum EOF; bounded legacy strings/relation count and exact legacy payload consumption. | Does not independently require magic/player or physical EOF after trailer; modern strings can leave trailing bytes. Integrity is XOR checksum, not authentication or race-rule legality. |
| `FLoadLogFile` / `FCheckLogFile` | Game/turn/generation envelope and eventual EOF; host check via FOpenFile. | Direct loader can ignore incompatible/stale/missing orders successfully; not every header tag, cbLog boundary or command length is checked. Readiness is not successful replay. |
| `FRunLogRecord` | Command-specific ownership/domain checks and rule-level repairs/refunds. | Checks differ by command; no general length validation, rollback, idempotence or uniform ownership validation. Unknown tags succeed without action. |
| Library `FInspectImportFile` | Signature, kind/player/version, nonzero game ID, whole-file framing and final EOF; XY structural size. | Does not validate decrypted content, cbLog semantics or all companion years. Rejects ordinary appended multi-segment turns. |
| `FCompleteGeneratedFile` | Expected identity/type/player, whole-file framing/length and ending. | No semantic reload; its header check does not independently enforce format version or decode GAME. |
| ZIP extraction | Entry/name/path/method/CRC/length checks and size budgets. | Packaging checks do not prove that contained Stars! payloads decode or replay. |

Sources: [file.c](../src/core/file.c), [save.c](../src/core/save.c),
[log.c](../src/core/log.c), [race.c](../src/core/race.c),
[msg.c](../src/core/msg.c), [libraryui.c](../src/ui/libraryui.c),
[libraryzip.c](../src/ui/libraryzip.c). These are source observations, not
claims that exploitability has been tested or a security audit completed.

## 2. Proposed strict decoder contract

These are acceptance requirements for a future bounded adapter, **not changes
made by this specification**. Preserve canonical writer output while treating
stricter malformed-input behavior as an explicit implementation decision.

1. Decode HDR and payload from bounded slices, check all required minimum/exact
   sizes, reject physical truncation and arithmetic overflow, and never read
   unused bytes from the previous record buffer.
2. Validate magic, file kind, supported version, player range, nonzero game
   identity, and expected companion identities before mutating shared state.
   Check every appended BOF, not only the first.
3. Validate universe count <=999, player count 1–16, geometry, title and
   catalog IDs. Require star-pack byte count to equal GAME.cPlanMax.
4. Validate entity IDs, ownership, design/plan indices and cross-references,
   record-group counts, masks and list capacities. Bound aggregate battle/AI
   chunks, message traversal and history allocations before allocating/copying.
5. Reconstruct variable records using their encoded discriminator/flags; require
   well-formed length and explicitly handle any accepted trailing bytes. Preserve
   unknown information separately from zero, unowned, absent and stale.
6. Require exact cbLog exhaustion and per-command lengths, handle short waypoint
   prefixes explicitly, and establish ownership/range checks before replay.
   Separate decoding errors from legal commands that fail against current state.
7. State a duplicate/unknown-record policy by file context. Canonical writer
   output is ordered; it is not a map with implicit last-write-wins semantics.
   Existing score/history merging is a specific exception, not a global rule.
8. Validate original legacy race/checksum before encoding conversion and follow
   the agreed policy on protected designs. Reject unsupported format variants
   rather than infer an encoding from plausible bytes.
9. Publish temporary decoded state only after validation. If preserving legacy
   repair behavior, return explicit repairs. Define multi-file staging/recovery
   separately from record-level correctness.

## 3. Concrete acceptance vectors

These vectors are source-derived requirements/examples. A row is not claimed
as an executable test unless the evidence column identifies existing coverage;
§5 records commands actually run for this documentation change.

| ID | Input / action | Expected result | Evidence / coverage |
| --- | --- | --- | --- |
| S01-A01 | Pack tag 8, length 16 | HDR bytes `10 20`; current version word `a0 2a`; 16-byte BOF. | `WriteBOF`; `test_save.c` version test; Dafny `FramingVectors` checks the HDR bytes. |
| S01-A02 | BOF length 15 or version 2.84/2.86 | Corruption / older / newer error respectively through FOpenFile. | `test_FOpenFile_reports_unsupported_versions_and_corruption`. |
| S01-A03 | XOR payload lengths 5 then 1 | Consume 2 words then 1, discard unused word bytes at each record boundary; BOF resets stream. | utilgen codec; save CLI encrypted update/comparison tests; dedicated cross-language vector remains to add. |
| S01-A04 | Two planets with dx=5,7 and y=1100,1200 | x=1005,1012; universe size 96 bytes; array indices 0,1 are planet IDs. | STARPACK grammar; comparator coordinate/truncation tests cover the boundary. |
| S01-A05 | User string `00 41 00`, `01 b0`, `01 1f` | Decode `A`, `A`, `a`; lengths consumed 3,2,2. | Codec/source; `test_encoding`/`test_save` cover UTF-8 and string bounds. |
| S01-A06 | 31-byte UTF-8 name vs 32 bytes; truncated raw NUL/escape or malformed UTF-8 | 31-byte name round-trips; excess/malformed input rejected by bounded readers; no split code point. | `test_save`, `test_race`, `test_encoding`; Dafny `TextVectors` checks selected UTF-8/truncation cases. |
| S01-A07 | Full PLAYER with relations `[2,0,0,...]` | At offset112 write count1 and byte2, then names; decode remaining15 relations zero. Partial PLAYER puts names at8. | `WriteRtPlr`/`ReadRtPlr`; focused byte-vector coverage still desirable. |
| S01-A08 | Rename race from longer name to shorter name leaving stale buffer tail | Checksum equals the same design with zero tail; successful load. | `test_IRaceChecksum_ignores_stale_name_bytes`. |
| S01-A09 | Valid legacy 2.49 singular-only / 2.55 plural race | Verify raw-name checksum first; convert CP1252; derive plural for pre2.55; re-save as2.85. | `test_race` legacy-import cases. Protected case exercises current code but conflicts with root policy. |
| S01-A10 | Sparse planet quantities 0,255,256,65536 | Width codes 0,1,2,3; packed values only for present fields; full population allowed only at full detail. | `WritePlanet`/`FReadPlanet`; no dedicated exhaustive width-boundary test identified. |
| S01-A11 | Ship counts 1 and255 vs one count256 | First encoding uses bytes for every present slot; second uses words for every present slot. | Fleet codec; regression fixtures exercise fleets, not all asserted width boundaries. |
| S01-A12 | Partial hostile fleet with known mineral cargo | No private orders/custom name/damage; no population/fuel cargo in writer projection; retains reported total mass/direction. | `WriteFleet`, visibility setup; explicit privacy tests currently focus on players/things. |
| S01-A13 | Claim Adjuster reads another live player; player receives things | Habitat retained, unrelated full-player fields cleared; other players' masks and trader reward removed. | `test_FWriteDataFile_claim_adjuster_sees_only_habitat`, `test_FWriteDataFile_masks_other_players_things`. |
| S01-A14 | Design replacement with queued partial work | Clear progress for queued references; do not reuse old cheap-design completion credit. | `test_FRunLogRecord_design_change_clears_progress`. |
| S01-A15 | Cargo transfer cannot deliver entire amount | Applied quantities obey gameplay bounds; undelivered quantity refunded. | `test_FRunLogRecord_refunds_undelivered_cargo`. |
| S01-A16 | Invalid UTF-8 fleet-rename command | Reject without replacing current fleet name. | `test_FRunLogRecord_rejects_invalid_utf8_name`. |
| S01-A17 | Missing/stale `.xN` vs failed counted command | Missing/stale can load as no orders; replay failure aggregates false after other commands have run. | `FLoadLogFile`/`FRunLogFile`; missing exhaustive malformed/replay rollback tests are explicit gaps. |
| S01-A18 | Two full turn segments concatenated, proper BOFs/EOFs | Core loads both with history overlays; library inspector currently rejects at intermediate EOF. | Source audit; no new successful library multi-year test claimed. |
| S01-A19 | Historical planet not in current observations | Historical information retained with observation age; current own ownership is not inferred solely from old history. | `FLoadGame` merge path; full history-order permutation matrix not identified. |
| S01-A20 | Malformed/truncated player message or nonzero legacy next-link | Invalid text skipped; link ignored; new writes always zero four link bytes. | `test_native_ports` message serialization/readers/bounds. |
| S01-A21 | Battle >1023 bytes / large event | Concatenate continuation chunks before token/event traversal; no native pointer/heap-header bytes. | `WriteBattles`; battle heap rollover unit coverage is partial, not every split case. |
| S01-A22 | AIHIST inactive freighter slot changes vs active count change | Inactive storage warning; active quantity/reference change is meaningful. | save CLI `TestCompareRecordsSeparatesUnusedFreighterSlots`. |
| S01-A23 | New game generation claims success but a companion is missing/truncated | Desktop library finalization fails and removes pending game; batch success alone does not prove completeness. | `FFinishLibraryGame`; `run_newgame.py` injects universe/host write failures, not every truncated-companion variant. |
| S01-A24 | Import later same-game/player turn, other player's year differs | Advance only target; retire observed prior orders after publication; preserve histories/backups/other player. FALSE orders query can retain orders despite TRUE ([M01-AC16](M01-game-library.md#m01-ac16)). | `run_newgame.py` update/conflict/failure cases and source; power-loss durability is not covered. |
| S01-A25 | Import host orders with wrong player/year/wGen or identity | Reject before replace; identical bytes no-op; different valid orders require replacement choice. | `run_hostgames.py` and its client-startup order variants; command semantics still validated later. |
| S01-A26 | Metadata v1/v2 round-trip and failed replacement | v1 exchange defaults to name; v2 preserves exchange; temporary publication preserves prior metadata on ordinary failure. | Library code; native replacement unit test. |
| S01-A27 | ZIP traversal, bad CRC, conflicting local filename, non-UTF8 name | Reject before managed publication; UTF8-name error is distinct; remove extraction stage. | `run_game_export.py` covers traversal, truncation and local-header mismatches; CRC/non-UTF8 cases are source-derived requirements without focused tests identified here. |
| S01-A28 | Batch title with BOM, UTF8 path, invalid UTF8 or oversized file | BOM accepted; valid paths preserved; invalid input rejected; <16000-byte bound. | `test_encoding` invalid-definition test plus creation/regression fixtures. |
| S01-A29 | Dump known universe/planet/fleet state | Exact tab-separated headers/CRLF and existing golden output; unknown fields remain blank/estimated as applicable. | `test_report` golden comparisons. |
| S01-A30 | Same gameplay with different BOF salt/game ID | Raw hashes differ; normalized record comparison matches; version/ordering/coordinate changes remain differences. | Save CLI normalization tests and full native regression. |

## 4. Existing evidence and its limits

### Source and test map

| Area | Tests / tools inspected |
| --- | --- |
| Save/version/visibility | [test_save.c](../tests/unit/test_save.c). |
| Race framing/checksum/conversion | [test_race.c](../tests/unit/test_race.c). |
| Order effects | [test_log.c](../tests/unit/test_log.c). |
| Filesystem / UTF-8 | [test_native_files.c](../tests/unit/test_native_files.c), [test_encoding.c](../tests/unit/test_encoding.c). |
| Message layout, text bounds, heap rollover | [test_native_ports.c](../tests/unit/test_native_ports.c). |
| Report bytes | [test_report.c](../tests/unit/test_report.c), [goldens](../tests/unit/golden). |
| Creation, import and host orders | [run_newgame.py](../tests/compat/run_newgame.py), [run_hostgames.py](../tests/compat/run_hostgames.py), [client harness](../tests/compat/win32_client_startup.c). |
| Export and ZIP boundaries | [run_game_export.py](../tests/compat/run_game_export.py). |
| Library rename/delete/races | [run_game_library_safety.py](../tests/compat/run_game_library_safety.py), [run_game_library_delete.py](../tests/compat/run_game_library_delete.py), [run_racelibrary.py](../tests/compat/run_racelibrary.py). |
| Independent record tooling | [save CLI](../tests/savecli/README.md), [savefile.go](../tests/savecli/savefile/savefile.go), its comparison/update unit tests. |
| Full generated families | [native regression fixtures](../tests/scaffold/fixtures/regression/native/run.json), [regression procedure](../tests/scaffold/REGRESSION.md). |
| Tutorial assets | [offline converter](../tests/savecli/cmd/utf8-tutorial/main.go); SDL checkpoints remain partial. |

The native suite covers nine scenarios (`noai`, `oneai1`–`oneai6`, `smallai4`,
`smallai6`) and checkpoint years 0,1,10,25,50,80,100,150: 519 compared files in
its current manifest. Regression compares decrypted records, ignores only the
clock-derived game IDs and BOF salt, and explicitly reports selected unused
AI/order storage as warnings. It retains meaningful field changes, version,
record order and coordinates. This is broader than a few format fixtures but
is not an exhaustive parser, malicious-input or round-trip proof.

Frozen `original/` fixtures remain untouched. Legacy ASCII races used for modern
regression are retagged only in generated working copies by the harness;
that controlled test-data preparation does not authorize user-game conversion.
The standalone save CLI can inspect historical data that product readers reject;
it is test tooling, not the supported product-version policy.

No new fuzzing, crash injection, historical binary comparison, full 80-page
SDL walkthrough, or cross-platform execution is claimed for this spec. Relevant
historical documentation can name older registration/coverage counts; the
actual commands below supersede those counts for this change only.

## 5. Verification for this specification change

### 5.1 Initial source-derived specification

Verification used fresh build directories because the existing SDL cache named
an older checkout path. The pinned PyYAML 6.0.3 dependency was installed into the
fresh build's virtual environment; Go used a workspace-local cache.

| Check | Command / result |
| --- | --- |
| SDL Debug build | `cmake --preset macos-sdl-debug -B dist/s01-debug`, then `cmake --build dist/s01-debug -j 8`: passed. |
| Native regression | `make regression DIST_DIR=dist/s01-verification` with local dependency/cache paths: all nine scenarios through turn150, 519 matches, 21 with unused-storage warnings, 0 differences/errors, 0 skipped reference inputs. |
| SDL native unit suite | Dummy-video/audio `ctest --test-dir dist/s01-debug --output-on-failure --timeout 300 -R '^sdl_test_'`: all 17 passed. |
| SDL tutorial | `ctest --test-dir dist/s01-debug --output-on-failure -R 'win32_native_tutorial_.*dummy$'`: registered macOS smoke checkpoint passed (1/1); not the full tutorial. |
| Library/file workflows | `ctest --test-dir dist/s01-debug --output-on-failure --timeout 300 -R 'win32_client_(newgame\|game_export\|game_library_safety\|game_library_delete\|race_library\|host_library\|fileworkflow)_dummy$'`: all seven passed, covering creation, export/ZIP, rename/duplicate safety, deletion, races, host orders and file workflow. |
| Independent save tooling | `go test ./...` in `tests/savecli` with local Go cache: passed. |

Documentation checks: all 221 local links/anchors resolve; `git diff --check`
and new-file trailing-whitespace checks pass. The fresh Debug build reported no
compiler warnings.

Runtime sources, catalog data, record widths and baselines are unchanged.
Host-specific verification is not required by the root rule for this
documentation-only change.

### 5.2 Dafny data and conditions — 2026-10-24

The initial [formal companion](S01-game-file-contracts.dfy) supplied the types and
conditions mapped in [S01 §1.2](S01-game-file-formats.md#12-what-the-formal-model-establishes).
It was checked with the official **Dafny 4.11.0** arm64 macOS release. The
archive SHA-256 matched the digest published with that release; the tool was
unpacked into temporary storage, with no project runtime dependency added.

```sh
dafny verify spec/S01-game-file-contracts.dfy --verification-time-limit 30
dafny audit spec/S01-game-file-contracts.dfy
```

Verification reports **36 verified, 0 errors**. The audit reports **0 findings**;
audit is an assumption check, not a substitute for verification. Every function
and lemma has a body; no `assume`, axioms, verification disabling, or external
function contracts supply the proof. Conditional input obligations still apply,
including the decoder-supplied checksum result and valid-candidate designation.

| Evidence | What it establishes |
| --- | --- |
| `Header`, `WordBytes`, `ReadRecord`, `FrameRoundTrip` | Header packing, little-endian word round-trip, bounded record extraction and framing round-trip for all bounded record payloads. |
| `FramingVectors`, `TextVectors`, `CompatibilityVectors` | Specific successful and rejected framing/text inputs, old-history acceptance, stale-order rejection and root-supported race-policy examples. |
| `PublishLaterTurn` | Completed logical effect: preserve managed-game identity, replace the selected turn, retire only its orders, preserve histories/backups/other players. [Native M01 outcomes](M01-import-publication.md) require separate query/retirement premises; this theorem does not classify every C TRUE return. |
| `PublishDecoded` | Invalid candidates leave the prior logical value unchanged in the proposed adapter. |
| Predicate well-formedness | Field access, indexing, recursive termination and subset constraints are safe within the mathematical model; it is not proof that C satisfies those predicates. |

Those initial proofs did not verify C codecs, file I/O, replay, or a complete
end-to-end parser. Section 5.3 extends the formal model across the previously
unmodeled S01 areas; C refinement remains a separate obligation. The accepted and rejected examples supplement the C tests;
they do not replace them or close S01-D01 through S01-D08.

The incremental SDL Debug build passed with no warnings. The 17 SDL unit tests
and the registered tutorial smoke test passed again (18/18 together). Full
native regression again produced **519 matches (21 unused-storage warnings),
0 differences/errors, 0 skipped inputs**. The commands and local cache/build
paths were those in §5.1; no runtime code or baseline changed. Library workflow
and save CLI results in §5.1 belong to the initial specification run, not a new
run for this Dafny addition.

Documentation links and embedded Dafny excerpts were checked against their
actual targets and the verified companion; whitespace checks passed.

### 5.3 Remaining S01 data and conditions — 2026-10-24

[S01-game-file-specification.dfy](S01-game-file-specification.dfy) is now the full-model
entry point. It includes the foundation and the codecs, records, orders, file
families, external formats and witness modules under `spec/dafny/S01-*.dfy`.
The unrelated S06 model is not part of this verification command.

```sh
dafny verify spec/S01-game-file-specification.dfy --verify-included-files --verification-time-limit 30 --cores 2
dafny audit spec/S01-game-file-contracts.dfy spec/dafny/S01-*.dfy
```

The include flag is essential: an include-only entry point can otherwise report
success without checking its dependencies. Each module was also checked while
being developed. The final aggregate run with Dafny 4.11.0 reports **271 verified,
0 errors**. The assumption audit reports **0 findings** and takes the foundation
and every S01 module as explicit input files, so its scope does not depend on
include traversal defaults. All 250 relative documentation links, formal
coverage identifiers and whitespace checks passed.

| Coverage added | Evidence and limits |
| --- | --- |
| XOR and byte codecs | Explicit signed division/wrapping, prime table, seed initialization, per-record transform and a general consumption lemma. Byte XOR involution is proved; a universal end-to-end encrypted-file round trip is not claimed. |
| User text and races | Both name encodings, literal nibble escapes/padding, raw fallback, CP1252 table, checksum reconstruction and password hash. Legacy conversion relates the original checksum, converted/truncated names, consent and replacement credential. |
| All entity families | Field/array schemas, variable extent and discriminator conditions, observed PLAYER tolerance, design repairs, battle continuation construction, visibility masking and AI layouts. These are formal specifications, not a proof of all C reader branches. |
| File groups and commands | Snapshot/history/order/race/universe assembly, design owner grouping, appended turns, every command payload shape, contextual guards, simple command effects, queue-progress consumption and replay failure aggregation. |
| External inputs and persistence | Decoded batch and metadata values, filename families, report bytes, ZIP structure/name/CRC/size rules, publication and rollback relations, direct partial writes and import-choice effects. Deflate and actual OS operations remain explicit external dependencies. |
| Concrete witnesses | Accepted/truncated player, planet, design, waypoint, plan, AI, message and battle payloads; single-use progress; standard CRC32 input `123456789`; malformed ZIP names/attributes/CRC; successful restoration after publication failure. |

No `assume`, bodyless axioms, external proof contracts or disabled verification
were introduced. Opaque functions still have checked bodies; proof lemmas reveal
them where required. Witness proofs use intermediate facts to keep solver work
bounded, without changing the expected results.

Source review refined two external-format descriptions: directory entries count
toward ZIP size budgets and undergo local-header matching before their special
CRC-free branch; the path filter rejects bytes below 0x20 (DEL is not explicitly
rejected by that function). These are documentation/model corrections, with no
runtime behavior change.

The incremental SDL Debug build completed without warnings; all 17 unit tests
and the tutorial smoke test passed (18/18 combined). Full native regression again
matched **519 files (21 unused-storage warnings), with 0 differences/errors and
0 skipped reference inputs**. Commands and local cache/build paths follow §5.1.
No native baseline, runtime source or S06 file was modified by this extension.

## 6. Open decisions and follow-up boundaries

The emitted formats and current read/write behavior are specified above. These
remaining items are policy or implementation decisions, not silently assumed
features:

| ID | Decision / gap | Owner and consequence |
| --- | --- | --- |
| S01-D01 | Protected legacy race support disagrees between root policy and accepted ADR/current code/tests. | M02/S01 and repository policy: reconcile explicitly. Until then this spec separates supported root requirement from observed replacement-password behavior. |
| S01-D02 | Core appended-turn support versus library single-EOF validation. | M01/S01: choose/test a full-segment import validator; do not claim current multi-year library import works. |
| S01-D03 | Bounded decoding and uniform semantic validation are incomplete. | S01 plus entity owners: implement strict per-record/count/reference checks, with explicit compatibility choices and malformed-input tests. No broad parser hardening was performed here. |
| S01-D04 | Direct multi-file save/generation is sequential and ignores some failures. | M05/M06/M17/S01: define publication/recovery policy; add failure injection. Staged library operations only cover their documented boundary. |
| S01-D05 | Replay continues after errors and is not transactional/idempotent. | M06/M17/M19: preserve known behavior or deliberately introduce decode-before-apply/rollback; command ordering and transfers need regression evidence. |
| S01-D06 | Conflict turn replacement preserves copied orders/history, unlike later-turn advancement. | M01/M06: decide whether this is intended, then test corresponding stale-order handling. |
| S01-D07 | Reader-only/unassigned tags lack a current emitted schema. | S01: no extension semantics inferred. Historical support would need its own version-specific specification. |
| S01-D08 | S06 logical-state and S03 catalog contracts are separately owned specifications. | Shared model/catalog owners: adopt the explicit mappings and immutable catalog IDs here; do not reinterpret C layout as a new wire scheme. |

This work neither closes GitHub issues nor publishes the specifications outside
the repository. It records findings without claiming bug fixes or changing
`CHANGELOG.md`, known-bug status or regression baselines.
