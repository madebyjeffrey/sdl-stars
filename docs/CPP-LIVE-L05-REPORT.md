# L05: Text and scratch ownership cleanup

**Snapshot:** `develop` at `8da637d3` (2026-11-03).

The destination classification and this pass's caller audit are recorded in [L05 destination scan](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-DESTINATION-SCAN.md>) and its [refreshed formatter inventory](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-FORMATTER-DESTINATIONS.csv>).

## Status

L05 has 216 recorded completed substeps and one open audit item:

1. `post-migration-scratch-storage-and-borrowed-return-audit` — trace scratch owners and returned views through consumers, callbacks, modal loops, and teardown.

## Current evidence

- The current owned-source scan finds no calls to `strcpy`, `strcat`, `strncpy`, or `strncat`; the maintained text-copy manifest records 306 migrations.
- The cleanup inventory records 116 redundant UI and compatibility text/path scans removed.
- Three production `std::strlen` calls remain, each at an incoming C-string boundary: compatibility list search, edit replacement input, and D-Bus unique-name input. No owned/fixed text-length reads are recorded.
- The broader scratch inventory records eleven library path/name owners moved into `APPUISTATE` or operation-local storage. It finds no shared library path/name arrays remaining in `libraryui.cpp`, but explicitly leaves the post-migration scratch/borrowed-return audit open.
- `InitBtnTrack` retains a `const char *` because `BTNT` stores the borrowed pointer across events and its record layout is fixed. C-string format arguments at native CRT `%s`/`%p` boundaries are also retained.
- `PszFormatString`, `PszFormatIds`, and the `TEXTIO` formatter callback now return `MESSAGETEXTRESULT`, carrying the already-computed UTF-8 byte count. This batch passed 18 desktop tests, 29 host tests, both 519-checkpoint regressions with zero differences/errors, and the tutorial smoke test.

## Actions to close L05

- [x] Scan and classify the remaining text destinations as bounded spans/views, fixed-format fields, or deliberate C ABI boundaries. Record file, function, capacity, and failure/truncation behavior in the scan and formatter CSV. No owned `strcpy`/`strcat` family calls remain.
- [x] Finish caller-return review for the 148 remaining snprintf-style production callsites. Of the original 186 candidates, 38 now use `CchSprintf` or UTF-8-aware copy helpers. All 148 remaining calls capture or conditionally consume the result and check it before using the destination. Strict path, filename, and serialization outputs reject formatter failure or a required byte count at or above capacity. Bounded numeric/config values have fit proofs at their source bounds. Display labels, prompts, and diagnostics use complete-prefix `CchSprintf` formatting. The inventory records the current file, function, destination capacity, and behavior for each call.
- [ ] Audit production scratch arrays and returned `string_view`/`span` values from producer to final consumer. For each view, record the owner and lifetime across callbacks, modal loops, and app/session teardown. Confirm report-label helpers copy where a returned view cannot outlive the source.
- [ ] Reconcile `ui_scratch_storage` and `borrowed_return_audit` in `docs/CPP-LIVE-CLEANUP.json` with current source. Either make a focused fix for each confirmed defect or record why each remaining buffer/view is correctly owned.
- [x] Update the cleanup ledger and L05 reports with the source revision and audit outcome.

## Verification to record for any source batch

- `make test-unit`
- `make test-host` when core or shared APIs change
- `make regression` and `make regression-host` for game/core changes; behavior-neutral changes must match all 519 checkpoints without baseline changes
- `make tutorial`
- `git diff --check`

The formatter caller-return audit is closed by static source review at `8da637d3`. `git diff --check` passes. Builds, unit tests, tutorial checks, and regressions have not been run for this audit batch.

Do not widen saved-field or record storage to solve a destination issue. Preserve UTF-8 byte limits and the existing whole-string versus display-prefix contracts.
