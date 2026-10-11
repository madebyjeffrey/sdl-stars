# Interactive scrollbar controls

Coverage remains **143/178 inventoried Win32 APIs (80.3%)**. This extends the
existing `SCROLLBAR` implementation without adding inventory entries. The six CRT
helpers are separate, and the complete SDL game client is still unlinked.

## Behavior

Horizontal and vertical child scrollbars draw a shaft, arrow buttons and a
fixed-size thumb. They borrow the parent's `WM_CTLCOLORSCROLLBAR` brush without
owning it, and fall back to system colors. Disabled or empty-range controls
suppress requests and draw disabled arrows. Focus has a simple thumb outline.
This is basic classic rendering; it does not reproduce Windows themes.

Arrow and shaft clicks send line/page requests to the parent. Thumb dragging
captures the mouse, paints a position preview and sends `SB_THUMBTRACK`, followed
by `SB_THUMBPOSITION` and `SB_ENDSCROLL` on release. Cancellation or capture
transfer ends the operation without releasing another window's capture.
Notifications carry the control HWND and use `WM_HSCROLL` or `WM_VSCROLL` according
to orientation. Thumb positions in messages have Win32's 16-bit high-word limit;
stored positions remain full-width.

Arrow, Page Up/Down and Home/End keys send the corresponding request. Key release
adds no end-scroll request, matching the SDK test under Wine. The parent must
explicitly update the stored scroll position after a request; mouse and keyboard
input do not change it automatically. These responsibilities follow Microsoft's
[scrollbar contracts](https://learn.microsoft.com/en-us/windows/win32/controls/about-scroll-bars).

## Tests and CLion

The existing `stars-compat-scroll` target is available in the SDL Debug profile.
No additional libraries are required. Shared tests check both keyboard axes and
parent-controlled positions against `test_win32_scroll_reference`. Native tests
also route SDL mouse events through the window queue and check pixels, brush ownership, arrow/page requests, thumb tracking/final
notifications, capture cancellation/transfer, disabled input and destruction
inside parent notification and color callbacks.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_scroll --output-on-failure
```

## October 11 follow-up

Held arrows/pages repeat after 400 ms, then every 50 ms. Repeat pauses outside
the pressed region; a page request stops when the parent advances the thumb
under the pointer. Release/cancel/capture transfer removes the internal timer.
A native test waits for a real timer message and checks repeat, outside-pointer
pause and release cleanup. See [current qualification](WIN32-CONTROL-QUALIFICATION.md).
Coverage is now 178/178 and the native client is linked; the earlier figures
record the initial scrollbar milestone.

## Further behavior and limits

`SCROLLINFO`, `SetScrollInfo`, `GetScrollInfo` and `ShowScrollBar` now support
page-aware thumb sizing, 32-bit track positions, standard nonclient bars and
client-area reflow. Internal bars send owner requests with a zero control handle;
separate scrollbar controls retain their handle. List/edit controls synchronize
content ranges and pages. Scanner and report UI code now includes the visible
map span or row/column count in `SCROLLINFO`; existing pan/scroll limits stay the
same. The thumb occupies the visible fraction of the track, with an 8-pixel
minimum for interaction. Captured thumb movement repaints immediately rather
than waiting for low-priority paint delivery. Consecutive native mouse motions
retain the latest position so map redraws do not accumulate stale drag positions;
button/key boundaries and explicitly posted messages are preserved. See
[Microsoft’s message-generation explanation](https://devblogs.microsoft.com/oldnewthing/20031001-00/?p=42343).
Pixel tests compare 20% and 50% thumb lengths; loaded
client tests verify scanner page sizes and unchanged maximum positions. Closed combo fields do not acquire nonclient bars from
their dropdown scroll styles.

Drag snap-back thresholds, full size-box/alignment styles and individual arrow
disabling remain unfinished. Native Wayland qualification remains outstanding.
Original inventory coverage stays 178/178; these added APIs do not change that
denominator. API coverage does not measure complete Windows behavior.

## Verification

SDL Debug and Release pass all 30 compatibility tests each, including X11
integration. Linux host units pass all 12. The scrollbar test passes fatal
AddressSanitizer, UndefinedBehaviorSanitizer and leak checks. The shared SDK
keyboard test passes under Wine after correcting an extra key-release
notification in the native implementation. All 38 Windows units pass. Full native regression returns 519 matches
(22 existing warnings), zero differences/errors and zero skipped reference
inputs. No baseline was changed.

The fresh Wine tutorial failed at year 2408, page 26, instruction 203 after 112
actions: `No ship transfer arrow is available`, while moving a Santa Maria to
Fleet #10. Artifacts: `dist/scaffold/tutorial/20261010T221018.430793Z`. This
unresolved Windows automation failure occurs in a build that does not link the
SDL runtime. Game code, persisted layouts and baseline fixtures are unchanged.

## Drag smoothness follow-up

Captured scrollbar motion now defers root presentation while the owner redraws
and the thumb paints, then presents the completed frame once. The depth counter
preserves existing wizard presentation deferral and is balanced if a paint
callback destroys the bar. Loaded-game tests count one presentation per drag
update; timing is diagnostic rather than a pass/fail threshold.

Standard `SetScrollPos` updates only its bar and honors the redraw flag; it no
longer dirties the owner's client. That follows the
[documented redraw scope](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setscrollpos)
and keeps `ScrollScanner` on its existing pixel-scrolling path. Window pixel
movement uses overlap-safe RGBA row copies, retaining the existing update-region
and child-relocation rules. Shared native/SDK tests check clipped diagonal copies
in all four directions, unchanged outside pixels, damage and child relocation.

For ten small horizontal scanner drag updates on the frozen Continue fixture,
headless Debug measured roughly 330–350 ms before and 33–35 ms after, with full
presentations reduced from 20 to 10. This measures the dummy/software test path;
it does not certify desktop compositor frame pacing or eliminate all expensive
map redraws at larger jumps/zoom levels. Basic API coverage remains 178/178.

The corner shared by horizontal and vertical bars is composed in button-face
gray. Pixel checks cover both top-level and nested windows against black client
contents; application drawing remains clipped to the client.

The smoothness and corner batch passes all 96 fatal ASan/UBSan/LSan tests and
all 51 Windows units. The full native regression returns 519 matches,
87 unused-storage warnings, zero differences/errors and zero skipped inputs;
no baseline changed. Tests run sequentially with desktop cases on private
virtual displays. Tutorial work remains deferred.

The final headless Debug profile passes 81/81; focused Release input, capture,
scroll, pixel scrolling and loaded-game Continue checks pass 5/5. Debug, Release,
sanitizer and MinGW Debug builds introduce no warnings in changed files.


A bar resized to zero width or height can retain an old update region. The
scrollbar paint handler now begins/ends painting even without drawable geometry,
validating that region. Previously it kept generating `WM_PAINT`, starving
timers in the Merge dialog after waypoint task changes. Shared SDK/native
assertions cover both zero dimensions and painting after restoring normal size;
the SDK passes while the old local handler fails. Repaired focused Debug
scroll/fleet checks pass headlessly and the combined fleet profile passes
private X11. Full qualification is recorded in the
[goal tracker](LINUX-CLIENT-GOAL.md).

Final cargo/task/empty-scrollbar qualification passes 99/99 fatal
ASan/UBSan/LSan tests, 51/51 Windows unit/reference tests and all three focused
Release checks (scrollbar, headless fleet and private-X11 fleet). Full native
regression returns 519 matches, 87 existing unused-storage warnings, zero
differences/errors and no skipped inputs. Debug, sanitizer, Release and MinGW
Debug builds pass without new warnings in touched files. UI suites ran
sequentially on private displays. No core game logic, saved layout or baseline
changed. The legacy Wine tutorial remains deferred to the native runner; route
creation/drag/deletion, transport presets and broader cargo workflows remain
outstanding. Basic API coverage stays 178/178 (100%).
