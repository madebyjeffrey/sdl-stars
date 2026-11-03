# L06: Buffer, length, result, and chooser cleanup

**Snapshot:** `develop` at `29a731f9` (2026-11-03). The ledger has 155 completed L06 substeps. Its L06 `pending_substeps` list was empty despite the top-level inventory and `remaining_work` naming the general buffer/length audit as open; that inconsistency should be corrected when updating the ledger.

## Current evidence

Recent completed API cleanups include:

- typed variants for cargo and object lookup targets, removing `void *` recovery casts;
- bounded byte spans for DIB palette/header helpers;
- fixed-extent spans for AI four-entry and sixteen-entry status arrays;
- a physical payload span for `FRunLogRecord`, while retaining its signed logical record length to preserve replay behavior;
- `MESSAGETEXTRESULT` propagation through `PszFormatString`, `PszFormatIds`, and `TEXTIO::pfnFormat`.

The current inventory reports zero replaceable raw `memcpy` operations; its 58 production calls are classified as wire/binary storage, object-representation, SDK, or callback-boundary movement. It reports zero owned C string-copy calls and three incoming `strlen` boundary calls. There is not yet a line-by-line, current-head count of all general buffer/length interfaces that might still benefit from a span/result type.

## Interface audit actions

- [ ] Re-scan application-owned headers and implementations for pointer-plus-capacity, pointer-plus-count, `void *` object targets, and output parameters whose values form one logical result.
- [ ] For every candidate, classify it as a replaceable private API or a required Win32/SDK callback, wire-record view, allocator/heap boundary, or third-party ABI. Record the reason and source location in the inventory.
- [ ] Convert confirmed private candidates in focused batches to spans, string views, typed variants, optionals, or result structs. Keep logical protocol lengths separate where legacy replay semantics require that distinction.
- [ ] Preserve record layouts, element widths, Win16 wire sizes, behavior, and regression baselines. Update `docs/CPP-LIVE-CLEANUP.json` after each verified batch.
- [ ] Correct the L06 `pending_substeps` entry so the ledger agrees with its own top-level `remaining_work` and current inventory.

## Chooser qualification (L06 Milestone D / L08)

The typed picker migration records eight UI callers and four backend paths: compatibility browser, Linux portal, macOS AppKit, and Windows SDK. MacOS focused workflows already cover new-game, race-library, host-games, main-open, and race-save selection. The checkpoint names three workflows without independent end-to-end chooser coverage:

- [ ] ZIP export
- [ ] Orders import
- [ ] Active game/race import-source chooser

Then qualify the current source on both target platforms:

- [ ] **Linux:** compile with DBus development headers; run the portal chooser suite under `dbus-run-session`; cover cancellation, selection, filters, and caller integration. The macOS checkout does not have the DBus development headers, so this needs Linux CI or a suitable Linux environment.
- [ ] **Windows:** build the Windows SDL client and SDK chooser adapter with the strict compiler policy; run the relevant chooser/client tests on Windows.
- [ ] Record exact commit SHA, test/workflow links, results, first diff on any regression mismatch, and whether the native baseline remained unchanged.

## Final verification and closure

- [ ] `make test-unit`
- [ ] `make test-host`
- [ ] `make regression`
- [ ] `make regression-host`
- [ ] `make tutorial`
- [ ] Current-source Windows and Linux qualification, including the chooser cases above
- [ ] Refresh source/macro inventories, close or explain every remaining candidate, and reconcile L06/L08 status in the ledger

L06 is complete only when the general interface audit is explicitly closed and the chooser/platform evidence applies to the final source revision. Earlier CI runs on stale revisions do not qualify the final head.
