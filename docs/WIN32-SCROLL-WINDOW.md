# Client scrolling and exact update regions

Coverage is **144/178 inventoried Win32 APIs (80.9%)**, up from 143/178
(80.3%). This adds `ScrollWindow`; the six CRT helpers remain separate. The
complete SDL client is still unlinked.

## Contracts

`ScrollWindow` copies overlapping client pixels safely in either direction,
within the scroll and clip rectangles. Pixels outside the clip are untouched;
newly exposed areas become pending damage requiring background erasure. Large
and extreme offsets expose the full affected rectangle without overflowing
coordinates. Zero offsets are a successful no-op. Drawing is presented through
the existing SDL texture path; no game code or persisted layouts change.

With a null scroll rectangle, pending damage moves with the pixels and direct
child HWNDs move by the offset, even with an empty clipping rectangle. With an explicit scroll rectangle, pending
damage and child positions stay in place. Child movement uses the existing
positioning callbacks, and parent destruction during a callback terminates
safely. Offsets that overflow child coordinates fail before scrolling.

Window update regions now retain a union of rectangles rather than only their
bounding box. `GetUpdateRect` and `PAINTSTRUCT.rcPaint` report the bounds;
`BeginPaint` drawing clips to the exact damage, preserving holes and disjoint
areas. `ValidateRect` subtracts a partial rectangle. Application DC clipping is
independent of that paint clip: clearing or replacing an application clip cannot
paint pixels outside the pending damage. Solid fills use clipped spans without
allocating a mask for every screen pixel. Empty regions and destruction release
their allocations.

These contracts follow Microsoft's
[ScrollWindow documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-scrollwindow).
The SDK test under Wine 9 retains both original and shifted pending damage for
both null and explicit scroll rectangles. Native tests enforce Microsoft's
specified distinction; shared checks accept the two observed bounding outcomes
while enforcing pixel movement, clipping, child movement, exposed damage and
paint holes on both builds. No Wine source was copied.

## Build and debug in CLion

Select `stars-compat-scrollwindow` in the SDL Debug profile. Its Windows SDK
counterpart is `test_win32_scrollwindow_reference`. No new libraries are needed.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R 'win32_scrollwindow|win32_paint' --output-on-failure
cmake --preset mingw-debug
cmake --build --preset mingw-debug
xvfb-run -a env WINEDEBUG=-all ctest --test-dir dist/mingw-debug \
  -R test_win32_scrollwindow_reference --output-on-failure
```

## Remaining scope

The application uses device-coordinate rectangular scrolling. Persistent
`CS_OWNDC`/`CS_CLASSDC` mapping and a system caret are not implemented; existing
edit controls draw their own cursor. `ScrollWindowEx` and `ScrollDC` are outside
the inventoried subset. Standard nonclient scrollbar rendering, scrollbar repeat
and page sizing, full dialogs/menus/resources, printing and the complete SDL
client remain unfinished. The percentage measures tested basic API bodies,
not full Win32 semantics or full-client readiness.

## Verification

SDL Debug and Release pass all 31 compatibility tests each, including X11
integration. Windows units pass all 39; the final SDK ScrollWindow test also
passes in an isolated Wine prefix after adding empty-clip relocation. Linux
host units pass all 12. The ScrollWindow and broader paint suites pass fatal
AddressSanitizer, UndefinedBehaviorSanitizer and leak checks. Full native
regression returns 519 matches (22 existing warnings), zero differences/errors
and zero skipped reference inputs. No baseline was changed.

The fresh tutorial failed at year 2406, page 21, instruction 166 after 102
actions: `Instruction did not advance after two attempts` for the Shift-click
back on Stove Top. Artifacts: `dist/scaffold/tutorial/20261010T222703.656939Z`.
The Windows tutorial does not link the SDL runtime. The earlier instruction-203
artifact showed incorrect button-availability observations: the observer exposed
`fDisabled`, while the actual transfer buttons use `bt & 4`. The direction in
`TransferShip` was correct. These Windows automation issues are being addressed
in a subsequent harness batch.

## Performance follow-up

Overlapping canvas movement now uses direction-aware RGBA row copies. Shared
native/SDK tests check clipped diagonal movement in all four directions and
preserve pixels outside the clip. Damage and child relocation contracts remain
unchanged. See [drag smoothness](WIN32-SCROLL-CONTROLS.md#drag-smoothness-follow-up)
for loaded-client measurement and current coverage (178/178 basic APIs).
