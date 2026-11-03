# L05: Text and scratch ownership cleanup

**Snapshot:** `develop` at `29a731f9` (2026-11-03). This report records the remaining L05 work after the formatter result change was committed.

The subsequent text-destination scan is recorded in [L05 destination scan](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-DESTINATION-SCAN.md>) with its [628-callsite formatter inventory](</Users/jdrake/Projects/sdl-stars/docs/CPP-LIVE-L05-FORMATTER-DESTINATIONS.csv>). The scan identifies unchecked, fixed-size display formatting as a follow-up to resolve before closing `bounded-text-destinations`.

## Status

L05 has 214 recorded completed substeps and two open audit items:

1. `bounded-text-destinations`
2. `post-migration-scratch-storage-and-borrowed-return-audit`

The exact number of edits is not known. The open work is a line-by-line contract audit; a clean audit may close an item without requiring code changes.

## Current evidence

- The current owned-source scan finds no calls to `strcpy`, `strcat`, `strncpy`, or `strncat`; the maintained text-copy manifest records 306 migrations.
- The cleanup inventory records 116 redundant UI and compatibility text/path scans removed.
- Three production `std::strlen` calls remain, each at an incoming C-string boundary: compatibility list search, edit replacement input, and D-Bus unique-name input. No owned/fixed text-length reads are recorded.
- The broader scratch inventory records eleven library path/name owners moved into `APPUISTATE` or operation-local storage. It finds no shared library path/name arrays remaining in `libraryui.cpp`, but explicitly leaves the post-migration scratch/borrowed-return audit open.
- `InitBtnTrack` retains a `const char *` because `BTNT` stores the borrowed pointer across events and its record layout is fixed. C-string format arguments at native CRT `%s`/`%p` boundaries are also retained.
- `PszFormatString`, `PszFormatIds`, and the `TEXTIO` formatter callback now return `MESSAGETEXTRESULT`, carrying the already-computed UTF-8 byte count. This batch passed 18 desktop tests, 29 host tests, both 519-checkpoint regressions with zero differences/errors, and the tutorial smoke test.

## Actions to close L05

- [ ] Re-scan every remaining text destination and classify it as a bounded span/view, a fixed-format/wire field with an explicit contract, or a deliberate C ABI boundary. Record each remaining candidate by file, function, destination capacity, and failure/truncation behavior.
- [ ] Reconcile that list with `docs/CPP-LIVE-TEXT-COPIES.json`. Confirm there are no unmanifested raw copies, no destination length reconstructed from a pointer, and no truncation that splits UTF-8 or changes strict path/record admission.
- [ ] Audit production scratch arrays and returned `string_view`/`span` values from producer to final consumer. For each view, record the owner and lifetime across callbacks, modal loops, and app/session teardown. Confirm report-label helpers copy where a returned view cannot outlive the source.
- [ ] Reconcile `ui_scratch_storage` and `borrowed_return_audit` in `docs/CPP-LIVE-CLEANUP.json` with current source. Either make a focused fix for each confirmed defect or record why each remaining buffer/view is correctly owned.
- [ ] Update the cleanup ledger and text/ownership manifests with the source revision and audit outcome.

## Verification to record for any source batch

- `make test-unit`
- `make test-host` when core or shared APIs change
- `make regression` and `make regression-host` for game/core changes; behavior-neutral changes must match all 519 checkpoints without baseline changes
- `make tutorial`
- `git diff --check`

Do not widen saved-field or record storage to solve a destination issue. Preserve UTF-8 byte limits and the existing whole-string versus display-prefix contracts.
