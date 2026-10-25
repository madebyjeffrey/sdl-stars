# Win32 input state over SDL3

The input milestone reaches **124/178 inventoried Win32 APIs (69.7%)**, up from
120/178 (67.4%). It adds `GetKeyState`, `GetAsyncKeyState`, `GetCursorPos` and
`WindowFromPoint`, plus input-message points, CP1252 conversion, control
characters, wheel delivery and double-click handling. Six CRT helpers are counted
separately. The full Linux client remains unlinked; this percentage counts tested
API bodies with the limits below.

## Keyboard queries and message timing

`GetKeyState` reflects an input snapshot saved when the SDL event is translated.
Retrieving input with `PM_REMOVE` installs that snapshot as the thread's keyboard
state. `PM_NOREMOVE` leaves it unchanged. Mouse input snapshots include the
modifier state from preceding keyboard events, so typing ahead does not erase
Shift/Ctrl/Alt before an earlier mouse message is processed. Generic
`PostMessage(WM_KEYDOWN)` does not change keyboard state.

Left/right Shift, Ctrl and Alt have distinct state entries and aggregate virtual
keys. Releasing one side preserves the aggregate if the other is down. Caps,
Num and Scroll Lock expose SDL's toggle state in the low bit. Focus-loss input
clears held-key state, and the queued focus messages carry that cleared snapshot.
Shutdown clears the compatibility arrays. On macOS, the two Command keys use
`VK_LWIN`/`VK_RWIN`, including their queued modifier snapshots and PC/AT scan
fields. Text controls use these snapshots for Command+Left/Right navigation;
Command is not remapped to Control. In focused macOS text fields, Option and
Left/Right use regular editing-key messages. This enables Option word navigation
and prevents its modifier press/release from activating Alt menus.

`GetAsyncKeyState` pumps SDL events without dispatching application procedures,
then reads the latest SDL keyboard/modifier state and mouse button state. Its
high bit indicates a held key. The low bit records observed presses/edges and is
consumed separately for each queried key; it is best effort, as on Windows. A
press/release entirely outside SDL observation may be missed. Unlike
`GetKeyState`, it does not advance the thread's removed-input snapshot.

The maps cover letters, digits, F1–F12, navigation, editing, modifiers, lock keys
and common US-layout OEM punctuation, including the comma, period and bracket
shortcuts used by Stars. PC/AT scan fields, extended-key flags, repeats and Alt
context bits accompany keyboard messages. Arbitrary keyboard layouts, the
numeric keypad, IME/dead-key composition and Unicode Windows APIs remain to do.
These queries run on the SDL UI thread; invalid virtual-key indices return zero.
SDL keyboard state is constrained by backend focus/input delivery, and is not a
desktop-wide keyboard hook.

The timing contracts follow Microsoft's [GetKeyState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeystate),
[GetAsyncKeyState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate)
and [posted-key explanation](https://devblogs.microsoft.com/oldnewthing/20050530-11/?p=35513).

## Text and control characters

SDL UTF-8 text events are decoded to the CP1252 bytes used by the narrow Stars UI
and its font path. Latin-1 letters, the euro symbol and punctuation share the
font renderer's CP1252 table. Valid Unicode characters absent from CP1252 produce
`?`. Malformed UTF-8, overlong encodings, surrogates and code points beyond
U+10FFFF reject the entire event, without posting an already-decoded prefix.

SDL supplies printable character events. `TranslateMessage` supplies the missing
Backspace, Tab, Enter and Escape control characters on key-down messages, using
`WM_CHAR` or `WM_SYSCHAR`. Printable keys are not synthesized a second time.
Full `ToUnicode`/keyboard-layout translation and Ctrl-letter character synthesis
remain outside this implementation.

## Pointer and wheel routing

`GetCursorPos` queries global coordinates on Windows, X11 and Cocoa through
[SDL_GetGlobalMouseState](https://wiki.libsdl.org/SDL3/SDL_GetGlobalMouseState).
The Windows driver was previously rejected; a mocked-driver regression reproduces
that failure and verifies desktop coordinates, non-finite rejection and the
retained unsupported-driver error. The opt-in `stars-compat-cursor-desktop`
check also compares a real Windows SDL query with the SDK cursor without moving
it; the desktop check and full 44-test headless suite pass. Native Wayland
uses the SDL mouse-focus window and translates its surface-relative point into
the owner-local coordinate system. During queued/captured input it can retain
the last delivered point while its source window still exists. It cannot query
the pointer over another application. The dummy backend likewise returns the
most recently routed point and fails before an event. Null pointers fail.
Wayland asynchronous mouse buttons use SDL's local mouse state rather than its
global fallback; see [positioning](WIN32-POSITIONING.md) for coordinate limits.

`WindowFromPoint` searches the application's logical root order and visible,
enabled descendants, returning the deepest child at a screen point. It respects
parent clipping and excludes hidden/disabled windows. It does not manufacture
handles for other applications or the desktop. The subsequent [control milestone](WIN32-CONTROLS.md) adds text-label and
group-box transparency. Native nonclient hit testing and exact stacking against
other applications remain unfinished.

Pointer input records screen coordinates in `MSG.pt`. Wheel `lParam` is also in
screen coordinates. Wheels target the local focus window, with a root fallback;
`DefWindowProc` propagates unhandled wheel messages to the parent. This supplies
the points and hit testing used by `nativeui.c`'s existing wheel rerouting.
Generic `PostMessage` messages still leave `MSG.pt` zero.

Vertical/horizontal SDL wheel values scale to signed Windows units of 120 per
notch. Sub-unit fractions accumulate per root; flipped direction is honored.
Large deltas split into signed 16-bit messages without losing the total.
Double-click-enabled classes receive left/right/middle double-click messages in
place of the corresponding second down message. SDL determines time/distance;
logical target changes reset the click sequence, and capture routing is applied.
Mouse buttons remain logical SDL buttons; platform-specific swapped-button
configuration and extra mouse buttons are not implemented.

These contracts follow [GetCursorPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getcursorpos),
[WindowFromPoint](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-windowfrompoint),
[WM_MOUSEWHEEL](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel)
and [double-click messages](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondblclk).

## CLion and tests

Build/debug `stars-compat-input` with the SDL debug preset. Its normal headless
mode uses `SDL_VIDEODRIVER=dummy;STARS_SDL_RENDERER=software`; an X11 run uses
`SDL_VIDEODRIVER=x11`. The test checks real X11 cursor coordinates and uses mocked
SDL state for controlled asynchronous-query cases.

An additional `stars-compat-input-x11` target is available when X11/XTest and
`xvfb-run` are found. Its CTest wrapper creates a private Xvfb display, sends real
X11 Shift presses/releases, and checks asynchronous state versus queued state.
Build presets pull this target in through `stars-compat-input`. Run it through
CTest to keep synthetic input isolated from the user's desktop:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R win32_input --output-on-failure
```

The existing dependency plan already includes the integration-test packages. If
they are absent, install them with:

```sh
sudo apt install libx11-dev libxtst-dev xvfb xauth
```

The shared SDK test uses `SendInput` to verify queue timing rather than pretending
posted keyboard messages are physical input. It also checks control characters,
point hit testing and default wheel propagation. Native tests cover typed-ahead
modifiers, simultaneous modifier sides, Alt flags, CP1252/replacement conversion,
malformed UTF-8, wheel fractions/signs/large deltas, message points and double
clicks. The game core, disk layouts and baseline fixtures are unchanged; Windows
and host builds do not link this SDL runtime.

Verified on October 10, 2026:

- SDL Debug and Release: 22/22 compatibility tests pass in each build, including
  real X11 keyboard delivery under private Xvfb displays.
- Windows SDK/game units under Wine: 31/31 pass, including `SendInput` queue
  timing, control-character translation, point hit testing and wheel propagation.
- Linux host units: 12/12 pass.
- The expanded input test passes on SDL X11, including real global cursor
  coordinates and wheel/click routing.
- Fatal address, undefined-behavior and leak sanitizer checks pass.
- Full native regression: 519 matches (22 with existing warnings), zero
  differences/errors and zero skipped reference inputs.
- MinGW and SDL builds have no new compiler warnings; `git diff --check` passes.
- The required tutorial run fails at year 2402, page 9, instruction 68:
  `Timed out waiting for Shift held before mouse gesture`, after 43 actions.
  Artifacts are in `dist/scaffold/tutorial/20261010T200751.350362Z`. This differs
  from the preceding run's instruction-203 transfer failure. The Windows
  tutorial does not link the SDL runtime; its automation remains a failing gate.
