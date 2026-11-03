# L05: Text and scratch ownership cleanup

**Snapshot:** `develop` at `d9ce84ab` (2026-11-03).

The destination classification and this pass's caller audit are recorded in [L05 destination scan](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-DESTINATION-SCAN.md>) and its [refreshed formatter inventory](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-FORMATTER-DESTINATIONS.csv>).

## Status

L05 has 217 recorded completed substeps. The post-migration scratch and borrowed-return audit is closed.

## Current evidence

- The current owned-source scan finds no calls to `strcpy`, `strcat`, `strncpy`, or `strncat`; the maintained text-copy manifest records 306 migrations.
- The cleanup inventory records 116 redundant UI and compatibility text/path scans removed.
- Three production `std::strlen` calls remain, each at an incoming C-string boundary: compatibility list search, edit replacement input, and D-Bus unique-name input. No owned/fixed text-length reads are recorded.
- The broader scratch inventory records eleven library path/name owners moved into `APPUISTATE` or operation-local storage. It finds no shared library path/name arrays remaining in `libraryui.cpp`; the producer-to-consumer and modal/teardown trace is now closed.
- The completed producer-to-consumer trace records backing owner and lifetime for production returned views/spans, session scratch, enumeration callbacks, popup labels, chooser state, and report results in [the scratch lifetime audit](CPP-LIVE-L05-SCRATCH-LIFETIMES.md). No dangling view or teardown/modal retention defect was found.
- Report label producers copy catalog, game-field, local-buffer, and `APPSESSION` scratch inputs into `REPORTTEXTRESULT` before returning. Result `.view()` borrows its own result array only.
- `InitBtnTrack` retains a `const char *` because `BTNT` stores the borrowed pointer across events and its record layout is fixed. C-string format arguments at native CRT `%s`/`%p` boundaries are also retained.
- `PszFormatString`, `PszFormatIds`, and the `TEXTIO` formatter callback now return `MESSAGETEXTRESULT`, carrying the already-computed UTF-8 byte count. This batch passed 18 desktop tests, 29 host tests, both 519-checkpoint regressions with zero differences/errors, and the tutorial smoke test.

## Actions to close L05

- [x] Scan and classify the remaining text destinations as bounded spans/views, fixed-format fields, or deliberate C ABI boundaries. Record file, function, capacity, and failure/truncation behavior in the scan and formatter CSV. No owned `strcpy`/`strcat` family calls remain.
- [x] Finish caller-return review for the 148 remaining snprintf-style production callsites. Of the original 186 candidates, 38 now use `CchSprintf` or UTF-8-aware copy helpers. All 148 remaining calls capture or conditionally consume the result and check it before using the destination. Strict path, filename, and serialization outputs reject formatter failure or a required byte count at or above capacity. Bounded numeric/config values have fit proofs at their source bounds. Display labels, prompts, and diagnostics use complete-prefix `CchSprintf` formatting. The inventory records the current file, function, destination capacity, and behavior for each call.
- [x] Audit production scratch arrays and returned `string_view`/`span` values from producer to final consumer. Record owners and lifetimes across callbacks, modal loops, and app/session teardown in the scratch lifetime audit. Report-label helpers copy borrowed inputs into result-owned arrays.
- [x] Reconcile `ui_scratch_storage` and `borrowed_return_audit` in `docs/CPP-LIVE-CLEANUP.json`; no lifetime defect required a behavior fix.
- [x] Update the cleanup ledger and L05 reports with the source revision and audit outcome.

## Verification to record for any source batch

- `make test-unit`
- `make test-host` when core or shared APIs change
- `make regression` and `make regression-host` for game/core changes; behavior-neutral changes must match all 519 checkpoints without baseline changes
- `make tutorial`
- `git diff --check`

The formatter caller-return audit is closed by static source review at `8da637d3`. The scratch and borrowed-view audit is closed by static review at `d9ce84ab`; `git diff --check` passes. Builds, unit tests, tutorial checks, and regressions have not been run for this audit batch.

Do not widen saved-field or record storage to solve a destination issue. Preserve UTF-8 byte limits and the existing whole-string versus display-prefix contracts.
