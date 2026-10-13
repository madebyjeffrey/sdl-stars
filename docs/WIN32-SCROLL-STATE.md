# Scroll positions, ranges and control messages

Coverage is **143/178 inventoried Win32 APIs (80.3%)**, up from 140/178
(78.7%). This adds `GetScrollPos`, `SetScrollPos` and `SetScrollRange`.
`GetScrollRange` is also available outside that inventory. Six CRT helpers
remain separate. The full SDL game client is still unlinked.

## Contracts

Windows have independent horizontal and vertical state, initially position zero
with a range of 0 through 100. The built-in `SCROLLBAR` class now creates child
HWNDs with a separate, initially empty range. Reports use those with `SB_CTL`;
the scanner uses `SB_HORZ`/`SB_VERT`. Standard range changes set or clear the
corresponding scroll style bit for a nonempty or empty range.

Position writes return the previous position and clamp the new value. Range
changes clamp the existing position too. Reversed ranges normalize to 0 through
0; equal endpoints retain their position. Positions above 65535 keep their full
32-bit value. Native ranges enforce Microsoft's documented maximum span of
`MAXLONG`, rejecting overflow without changing state. Wine accepts larger spans;
that boundary is tested natively. See Microsoft's
[SetScrollRange](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setscrollrange)
and [SetScrollPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setscrollpos)
contracts.

`SB_CTL` operations dispatch `SBM_GETPOS`, `SBM_SETPOS`, `SBM_SETRANGE`,
`SBM_SETRANGEREDRAW` and `SBM_GETRANGE`. The built-in control handles those
messages. Custom procedures can implement them too; custom range messages
must return nonzero to indicate success through the range APIs. Built-in
range messages retain their previous-position return convention. Native tests
check signed custom setter parameters and destruction inside callbacks;
the shared SDK test also checks a custom position getter.

## Remaining work

The subsequent [scroll control milestone](WIN32-SCROLL-CONTROLS.md) adds drawing,
mouse dragging, keyboard requests and parent color brushes. Repeat timers,
nonclient standard bars and scrollbar-dependent client-area reflow remain
unfinished. Redraw requests invalidate the window for subsequent painting.
Size-box/alignment styles remain unfinished. The subsequent
[content-scrolling milestone](WIN32-SCROLL-WINDOW.md) adds `ScrollWindow` pixel
movement and child relocation.
API coverage describes tested basic implementations, not complete Win32
semantics or full-client readiness.

## CLion and tests

Select `stars-compat-scroll` in the SDL Debug profile; no new dependency is
needed. Its SDK counterpart is `test_win32_scroll_reference`. Shared tests cover
defaults, signed bounds, clamping, equal/reversed ranges, full-width positions
and independent axes/windows/controls. Native tests also check invalid inputs,
atomic overflow rejection, custom messages and callback destruction.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_scroll --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_scroll_reference --output-on-failure
```

## Verification

SDL Debug and Release pass all 30 compatibility tests in each build, including
X11 integration. Windows SDK/game units pass all 38 tests under Wine. Linux
host units pass all 12. Scroll contracts pass fatal AddressSanitizer,
UndefinedBehaviorSanitizer and leak checks. Full native regression returns
519 matches (22 existing warnings), zero differences/errors and zero skipped
reference inputs. The fresh Wine tutorial failed at year 2406, page 21,
instruction 166, after 101 actions: `Instruction did not advance after two
attempts` for `Now shift click back on Stove Top.` Artifacts:
`dist/scaffold/tutorial/20261010T215900.106774Z`. The Windows/Wine automation
failure remains unresolved; that tutorial does not link the SDL runtime.
Game logic, persisted layouts and baseline fixtures are unchanged; Windows and
Linux host builds do not link this runtime.
