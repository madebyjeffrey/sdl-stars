# PR #22 and PR #23 integration for issue #30

PR #22 was merged into `develop` at `b6d6cf8` before the UTF-8 branch was
reconciled. Merging the updated `develop` into `codex/utf8-application-text`
exposed conflicts in `CHANGELOG.md`, `cmake/macos-client-tests.cmake`,
`src/platform/native.c`, `tests/compat/win32_utf8.c`, and `text/strings.txt`.
The combined macOS test configuration also included the same test file twice,
which was not reported as a textual merge conflict.

## Resolutions

- Removed the per-control UTF-8 opt-in API and its call from the library-name
dialog. The UTF-8 branch makes all wrapper controls UTF-8. The compatibility
test now confirms an ordinary edit control round-trips a Unicode string without
an opt-in.
- Merged the managed-game resource IDs into `0x058b`–`0x058f`, retaining the
UTF-8 messages at their existing IDs.
- Kept both branches' changelog entries and combined their macOS startup test
redirects. Removed the duplicate Apple test include and the obsolete save-picker
redirect; the managed-game workflow now uses `SDL_GetPrefPath` and `FCreateFile`.
- Restored the managed-game native filesystem helpers on both platforms. POSIX
helpers now validate input paths and reject invalid UTF-8 directory entry names;
the regression covers Unicode directories, entries, metadata replacement, and
invalid paths.

## Combined-branch verification

On macOS:

- `macos-sdl-debug` configure and build passed.
- All 39 registered Debug CTest tests passed, including Unicode managed-game
  creation/reopening, startup/race workflows, UTF-8 controls, and tutorial smoke.
- `make test-unit`: 17/17 passed. `make test-host`: 16/16 passed.
- `make regression` and `make regression-host`: 519 matches each, 0 differences
  or errors, 0 skipped inputs. The 21 native regression warnings remain.
- `make tutorial`: 1/1 available tutorial smoke checkpoint passed. The full
  80-page SDL tutorial remains unfinished.

Windows/Linux execution and real-desktop manual qualification were not run.
The existing tests cover Unicode library-name creation and reopening, and unit
tests cover legacy race import, replacement passwords, and shortened-name
confirmation. Managed-library rename and game/orders export were not exercised
by this integration change and remain open for issue #30 qualification.
