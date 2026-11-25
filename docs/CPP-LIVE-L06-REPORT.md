# L06: Buffer, length, result, and chooser cleanup

**Initial snapshot:** `develop` at `29a731f9` (2026-11-03), when the L06 ledger had 155 completed substeps and no `pending_substeps` despite still-open interface-audit work. The ledger now has 207 completed L06 substeps and two pending focused conversions.

**Current audit progress:** The initial candidate register and classification
pass are recorded in
[`CPP-LIVE-L06-INTERFACE-AUDIT.md`](CPP-LIVE-L06-INTERFACE-AUDIT.md), source
baseline `develop` at `4ac98d09` (2026-11-03). Fifty behavior-neutral
conversion batches are verified in the uncommitted working tree and recorded
in the audit register. The re-scan of the original register checked all 29
replaceable candidates and six required-boundary families and the input-only comparator callbacks against current
definitions, callers, and provider or ABI seams; all 29 conversions remain
complete. The full source inventory then covered 324 application-owned source
and header files; the current supplemental pointer-to-pointer pass traced one
additional private list-head candidate, `CreateSalvage`; the current-tree
fixed-array continuation found `FAddLibraryGameListItem`, whose sole caller
supplies 16 player-turn values. The first sweep found 13 additional private candidates; the
current-tree follow-up found nine more in display, toolbar, menu, dialog,
profile, scanner, fleet, and score helpers. A continued source scan found five
more private candidates in menu lookup, legacy race decoding, battle-plan
decoding, and trader/technology results. A supplemental current-tree scan of
UI point adapters and output-bearing helpers found three more private
candidates: `MapWindowPoints16`, `HcrsFromFrameWindowPt`, and the three
`nativeui.h` point conversion helpers. A later fixed-array/output review found
eight more private candidates in AI/production rows, decoded library results,
race loading/snapshots, UI window values, and part lookup. The audit also
records one additional object-representation boundary and more Win32/GDI
boundary evidence. All candidates were traced to callers and provider seams or
documented as having no provider. Batch 32 converted the optional message-decode result, and batch 33 converted
the menu/toolbar results. Batch 40 converted `CreateSalvage`; batch 41 converted
`FAddLibraryGameListItem` to a fixed 16-turn span; batch 42 converted the
planetary-scanner provider to return `PART` values. Thirty-three of the 40 additional
candidates from the earlier scans had verified conversions at batch 42, leaving seven. Batch 43 converted
`GetWindowRc` to return its rectangle by value, and batch 44 changed the library
record readers to optional results. Thirty-five of the 40 additional candidates had
verified conversions at batch 44, leaving five. Batch 45 converts `FReadManagedRace` to an
optional `PLAYER`, bringing the count to 36 of 40 and leaving four at batch 45. Batch 46
changes `GetIniWinRc` to return `WN`, bringing the count to 37 of 40 and leaving
three at batch 46. Batch 34 converted
the display rectangle results and passed the focused `win32_services` test;
batch 35 converted profile/dialog results and passed four focused compatibility tests. The scan also corrected the
`FDialogUnits` caller count to two. Batch 21 records the scanner-range definitions and all core/UI
callers; batch 22 records the library-file inspection definition, all
production callers, and its injected publication provider; batch 23 records
the mouse helper definitions and all nine UI loops; batch 24 converts the
runtime object targets after tracing their definitions, callers, and lack of a
provider or serialization seam. At this batch-24 checkpoint, the broader buffer/length review and
current-source Windows/Linux qualification remained open.

Batch 47 converts the `SaveRaceState` snapshot helper to return `RACESTATE` by
value. Its nine callers still capture at the same operation boundaries, and
`RestoreRaceState` retains its existing selective restoration behavior. Full
macOS SDL Debug, desktop and host units, both regressions, and tutorial smoke
pass. Batch 48 carries the AI queue through fixed 64-entry spans; batch 49
converts `InitProduction` and its provider to optional fixed spans, retaining
`std::nullopt` as the global-array fallback. Both batches preserve queue counts
and ordering, and the full required verification passes.

Batch 48 converts the AI production queue to `std::span<PROD, 64>` through
all seven AI implementations and shared helpers. Batch 49 carries the same
fixed extent through `InitProduction` and `ProductionIO::pfnInitialize`; both
preserve active counts, ordering, and the null/global-array fallback. Batch 50
converts the newly found `StartPrint` title pointer/count helper to an optional
`std::string_view`. Its only caller, the Win32 `IEscapePrinter` adapter,
retains its SDK signature and validates the byte count and pointer before
constructing the view. All three batches pass the macOS SDL Debug build, 18
desktop and 29 host unit tests, both 519-checkpoint regressions with zero
differences, and tutorial smoke.

The 2026-11-04 post-conversion inventory rechecked 323 current owned
C/C++/Objective-C++ source/header files plus `compat/win32/src/installed_data.h.in`,
excluding 60 upstream `free-api` files. It found `StartPrint` as one additional
replaceable private candidate, then found no further unregistered candidate
after batch 50. The follow-up broad interface scan checked pointer/count and
output-count shapes across owned declarations and implementations. It registered
two further private interfaces for conversion: `FEncodeRtFleet` returns a
success flag and written-byte output that form one result, and `PopupMenu`
repeats its active item count alongside optional spans. The erased-pointer token
scan counted 112 `void *`/`LPVOID`/`PVOID` occurrences; retained forms map to
the SDK, callback, heap, wire-view, CRT, and object-representation families.
The broad scan is complete; those two conversions and Windows/Linux qualification
remain open.

The candidate-by-candidate register and broad current-tree interface scan are
complete. The L06 ledger now tracks the two resulting focused conversions:

- `fleet-record-encoder-optional-written-count-result`
- `popup-menu-count-derived-from-exact-spans`

## Current evidence

Recent completed API cleanups include:

- typed variants for cargo and object lookup targets, removing `void *` recovery casts;
- bounded byte spans for DIB palette/header helpers;
- fixed-extent spans for AI four-entry and sixteen-entry status arrays;
- a physical payload span for `FRunLogRecord`, while retaining its signed logical record length to preserve replay behavior;
- `MESSAGETEXTRESULT` propagation through `PszFormatString`, `PszFormatIds`, and `TEXTIO::pfnFormat`.

The broad signature-shaped scan found two additional replaceable interfaces. It screened the other span/count and output-count matches as domain counts, in/out cursor state, wire-record lengths, allocator extents, or required Win32/SDK/third-party callback shapes. The 58 production `memcpy` calls remain classified as wire/binary storage, object-representation, SDK, or callback-boundary movement; there are zero replaceable raw `memcpy` calls, zero owned C string-copy calls, and three incoming `strlen` boundary calls.

## Interface audit actions

- [x] Run a first-pass lexical scan of application-owned headers and implementations for pointer-plus-capacity, pointer-plus-count, `void *` targets, and output parameters; record concrete candidates and locations in the audit register.
- [x] Complete the candidate-by-candidate re-scan for the original registered candidates: inspect definitions, callers, and provider seams, then classify each as a replaceable private interface or required Win32/SDK callback, wire-record view, allocator/heap boundary, or third-party ABI. Completed 2026-11-03: 29 replaceable candidate rows, six required-boundary families, and the input-only comparator callbacks checked.
- [x] Add additional fixed arrays, grouped outputs, and runtime object targets found while tracing declarations and callers.
- [x] Convert the initial confirmed private pointer/count candidates and grouped logical outputs in focused batches; retain independent optional outputs where their semantics do not form one result. Thirty-one batches are verified.
- [x] Convert the remaining candidates from the original register in focused batches; all 29 original replaceable candidates have a completed conversion in 24 verified batches.
- [x] Confirm and record the raw-storage, wire-view, and callback-context `void *` boundaries in the audit register.
- [x] Continue the full source inventory for candidates outside this reviewed register; the 2026-11-04 post-conversion sweep across 323 owned source/header files plus `installed_data.h.in` is recorded in the audit register. It found and traced `StartPrint`, then found no further unregistered private candidate after conversion.
- [x] Complete the fixed-array/output continuation re-scan; all eight new candidate definitions, callers, provider seams, and screened lookup/duplication cases are recorded.
- [x] Convert the three core results from the supplemental inventory; batch 36 is verified; batches 46, 49, and 50 bring the additional-candidate conversion count to 41 of 41.
- [x] Convert or retire the 41 private candidates found outside the original register; all 41 have verified conversions, including `InitProduction` and its provider seam in batch 49 and `StartPrint` in batch 50. The post-conversion inventory found no additional private candidates.
- [x] Preserve record layouts, element widths, Win16 wire sizes, behavior, and regression baselines for the completed batches; both regression suites match all 519 checkpoints. Update `docs/CPP-LIVE-CLEANUP.json` after each verified batch.
- [x] Complete the broader current-tree buffer/length and source-interface scan; classify all signature-shaped matches and record the two additional private candidates below. The remaining work is their focused conversion and platform qualification.

## Chooser qualification (L06 Milestone D / L08)

The typed picker migration records eight UI callers and four backend paths: compatibility browser, Linux portal, macOS AppKit, and Windows SDK. The remaining caller workflows now have focused macOS dummy-client coverage:

- [x] ZIP export — `win32_client_game_export_dummy` passed; it selects the ZIP destination through the folder callback and verifies archive contents, cancellation, and blocked destinations.
- [x] Orders import — `win32_client_host_library_dummy` passed; it selects an incoming order file through the typed chooser and verifies the managed orders are updated.
- [x] Active game/race import-source chooser — `win32_client_newgame_dummy` and `win32_client_race_library_dummy` passed; they exercise external game and race selection, import, and the race edit-modal cancel path.

The broader `win32_client_fileworkflow_dummy` test timed out after 75 seconds in this run; it is not counted as passing evidence for these items. The focused tests above exercise each listed caller workflow independently.

Then qualify the current source on both target platforms. The first Linux Actions
run exposed misaligned `std::variant` construction in the pack(1) `PART` carrier,
along with related Linux regression and sanitizer failures. Follow-up linked
witnesses then found that the terraform result also needed to preserve an absent
result and individually unwritten cost entries. The current correction keeps
packed-safe pointer/tag storage in `OBJ` and `PART`, uses the typed variant
snapshot only in `NATIVEPARTCOMMONVIEW`, and models terraform outputs with an
optional result and optional cost entries. The corrected source still needs fresh
Linux CI evidence.

The latest macOS SDL Debug run passes after the terraform correction: desktop
units 18/18, host units 29/29, native and host regressions each 519 matches with
zero differences/errors/skips, tutorial smoke 1/1, and the linked terraform
witnesses pass 1,423,925 cases / 303,296,026 assertions and 1,756,689 cases /
497,240,245 assertions. This is local macOS evidence; it does not qualify Linux
or Windows.

- [ ] **Linux:** compile with DBus development headers; run the portal chooser suite under `dbus-run-session`; cover cancellation, selection, filters, and caller integration. Re-run the host and SDL regressions plus the sanitizer/resource-ownership suite to verify the packed-carrier correction. The macOS checkout does not have the DBus development headers or Linux sanitizer preset, so this needs Linux CI or a suitable Linux environment.
- [ ] **Windows:** build the Windows SDL client and SDK chooser adapter with the strict compiler policy; run the relevant chooser/client tests on Windows.
- [ ] Record exact commit SHA, test/workflow links, results, first diff on any regression mismatch, and whether the native baseline remained unchanged.

## Final verification and closure

All fifty conversion batches passed the macOS SDL Debug build, desktop and
host unit suites, native and host regressions, and tutorial smoke. The focused DIB
resource check passed for 38 assets and the focused paint batch passed 1/1; its separate leak wrapper timed out because
the macOS leaks tool could not obtain a task port, though the DIB subcheck printed PASS.
Detailed batch entries are in [`CPP-LIVE-L06-INTERFACE-AUDIT.md`](CPP-LIVE-L06-INTERFACE-AUDIT.md).

- [x] `make test-unit` (18/18)
- [x] `make test-host` (29/29)
- [x] `make regression` (519 matches; zero differences/errors/skips)
- [x] `make regression-host` (519 matches; zero differences/errors/skips)
- [x] `make tutorial` (1/1 smoke checkpoint)
- [ ] Current-source Windows and Linux qualification, including the chooser cases above
- [ ] Convert `FEncodeRtFleet` status plus written count to one optional result and remove `PopupMenu`'s duplicate active count by passing exact spans through `CONTROLIO` and its providers.
- [x] Convert the three candidates still open after batch 46 (`SaveRaceState`, the AI production queue, and `InitProduction`) and re-run the source/macro inventory; batches 47–49 verified the conversions, batch 50 converted the later-discovered `StartPrint` candidate, and the post-conversion inventory found no further private candidate. The broader interface scan is now complete and has two additional conversions registered below.

L06 is complete only when the two registered conversions are verified and the chooser/platform evidence applies to the final source revision. Earlier CI runs on stale revisions do not qualify the final head.
