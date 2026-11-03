# L05: Text and scratch ownership cleanup

**Snapshot:** `develop` at `5988b374` (2026-11-03).

The destination classification and this pass's caller audit are recorded in [L05 destination scan](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-DESTINATION-SCAN.md>) and its [refreshed formatter inventory](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-FORMATTER-DESTINATIONS.csv>).

## Status

L05 has 215 recorded completed substeps and two open audit items:

1. `formatter-caller-return-audit` — the formatter destinations are classified, but each snprintf-style caller still needs its return handling confirmed against its path, record, or display role.
2. `post-migration-scratch-storage-and-borrowed-return-audit` — trace scratch owners and returned views through consumers, callbacks, modal loops, and teardown.

## Current evidence

- The current owned-source scan finds no calls to `strcpy`, `strcat`, `strncpy`, or `strncat`; the maintained text-copy manifest records 306 migrations.
- The cleanup inventory records 116 redundant UI and compatibility text/path scans removed.
- Three production `std::strlen` calls remain, each at an incoming C-string boundary: compatibility list search, edit replacement input, and D-Bus unique-name input. No owned/fixed text-length reads are recorded.
- The broader scratch inventory records eleven library path/name owners moved into `APPUISTATE` or operation-local storage. It finds no shared library path/name arrays remaining in `libraryui.cpp`, but explicitly leaves the post-migration scratch/borrowed-return audit open.
- `InitBtnTrack` retains a `const char *` because `BTNT` stores the borrowed pointer across events and its record layout is fixed. C-string format arguments at native CRT `%s`/`%p` boundaries are also retained.
- `PszFormatString`, `PszFormatIds`, and the `TEXTIO` formatter callback now return `MESSAGETEXTRESULT`, carrying the already-computed UTF-8 byte count. This batch passed 18 desktop tests, 29 host tests, both 519-checkpoint regressions with zero differences/errors, and the tutorial smoke test.

## Actions to close L05

- [x] Scan and classify the remaining text destinations as bounded spans/views, fixed-format fields, or deliberate C ABI boundaries. Record file, function, capacity, and failure/truncation behavior in the scan and formatter CSV. No owned `strcpy`/`strcat` family calls remain.
- [ ] Finish caller-return review for the 186 snprintf-style production callsites. The inventory currently flags 88 ignored callsite returns and 98 captured/conditional returns for source-level contract review; prove safe-fit bounds or reject strict truncation, and use complete-prefix formatting for display text. This pass repaired strict library path joins and export/staging paths, and switched fixed display labels to UTF-8-prefix formatting.
- [ ] Audit production scratch arrays and returned `string_view`/`span` values from producer to final consumer. For each view, record the owner and lifetime across callbacks, modal loops, and app/session teardown. Confirm report-label helpers copy where a returned view cannot outlive the source.
- [ ] Reconcile `ui_scratch_storage` and `borrowed_return_audit` in `docs/CPP-LIVE-CLEANUP.json` with current source. Either make a focused fix for each confirmed defect or record why each remaining buffer/view is correctly owned.
- [x] Update the cleanup ledger and L05 reports with the source revision and audit outcome.

## Verification to record for any source batch

- `make test-unit`
- `make test-host` when core or shared APIs change
- `make regression` and `make regression-host` for game/core changes; behavior-neutral changes must match all 519 checkpoints without baseline changes
- `make tutorial`
- `git diff --check`

This report update records static source review only; build, tests, and regressions have not been run for the current audit changes.

Do not widen saved-field or record storage to solve a destination issue. Preserve UTF-8 byte limits and the existing whole-string versus display-prefix contracts.
