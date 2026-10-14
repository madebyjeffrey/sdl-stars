# PR #22 review follow-up for issue #30

This note records the non-UTF-8 review follow-up for managed games so it can be
checked when evaluating [issue #30](https://github.com/madebyjeffrey/sdl-stars/issues/30).

## Before the #23 integration

PR #22 keeps desktop root resolution in the SDL-backed UI layer and routes
directory creation, enumeration, removal, and file metadata checks through
`src/platform/native.c`. Its real-client workflow exercises the default
`SDL_GetPrefPath` path with an isolated test root, without `STARS_LIBRARY_DIR`
or `STARS_SETTINGS_DIR`, then creates and reopens a managed game from that
root. The workflow test supplies its temporary root through the
`SDL_GetPrefPath` test hook so it cannot touch the user's real library. SDL
supplies the native Application Support, XDG data, and Windows application-data
conventions.

## Deferred to issue #30

Issue #30 owns reconciliation after PR #23 merges. It must resolve the obsolete
UTF-8 control opt-in against the migration's uniformly UTF-8 controls, then
validate Unicode names and paths through race import, managed-game creation,
rename, restart/reopen, game export, and orders export. Verify protected-race
password replacement and expanded-name confirmation through the combined
New Game and library workflows. The independent green checks on PR #22 and PR
#23 do not establish combined compatibility.

Keep platform desktop qualification, full regressions, unit tests, and tutorial
checkpoints as issue #30 acceptance work. Real-desktop manual qualification is
future work, as issue #30 specifies.
