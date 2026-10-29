# SDL-backed Win32 windows and dispatch

This milestone adds independent top-level Win32 windows over SDL3, with an
interactive and automated two-window executable, `stars-compat-windows`.
It builds in the existing SDL presets and does not change the Windows game,
the Linux host, or disk formats. No new packages are required.

## Build and debug in CLion

Reload the CMake project after these changes. In the enabled `sdl-debug`
profile, select the new **stars-compat-windows** application target. Run it
without arguments for two desktop windows. Type in either window: its title
shows its own key count. Close either window and keep using the survivor;
closing the final window ends the application.

The windows are intentionally blank inside: this milestone tests window
ownership and dispatch, with titles providing visible feedback. GDI drawing
and accelerated/software presentation through the compatibility layer are now
covered by the separate [painting milestone](WIN32-PAINTING.md) and
`stars-compat-paint` target.
The existing `stars-compat-smoke` still tests SDL GPU/software rendering.

Useful breakpoints:

- `RegisterClassA`, `CreateWindowExA`, `DestroyCompatWindow` in
  `compat/win32/src/window.c`.
- `PeekMessageA`, `SendMessageA`, `DispatchMessageA` in
  `compat/win32/src/message.c`.
- `CompatRouteEvent` in `compat/win32/src/input.c`.
- `TestWndProc` in `tests/compat/win32_windows.c`.

Use program arguments `--self-test` for the automated test. For a headless
CLion run, set environment variable `SDL_VIDEODRIVER=dummy` in that run
configuration. Leave it unset for desktop windows.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure

# Interactive desktop test:
./dist/sdl-debug/bin/stars-compat-windows

# Native X11 backend in a virtual desktop, if xvfb is installed:
xvfb-run -a env SDL_VIDEODRIVER=x11 \
  ./dist/sdl-debug/bin/stars-compat-windows --self-test
```

## Implemented subset

The library continues to use our ABI-checked headers and C++20 adaptations of
Free API, with its MIT attribution retained. The unmodified upstream source
reference and SHA-256 records are unchanged.

- `RegisterClassA` owns copies of class and menu names, preserves `WNDCLASSA`
  metadata and allocates zeroed class extra storage. Duplicate registration
  fails. Creation accepts the registered name or returned atom, matched to
  its instance handle.
- `CreateWindowExA` allocates an opaque window record and zeroed window extra
  bytes, then synchronously delivers `WM_NCCREATE` and `WM_CREATE` with the
  creation payload. Rejected creation and destruction from inside `WM_CREATE`
  are cleaned up. The class procedure is copied per window for subclassing.
- `Get/SetWindowLongPtrA` support `GWLP_USERDATA`, `GWLP_WNDPROC` and
  pointer-sized reads/writes at valid window-extra offsets. Reads also support
  `GWLP_HINSTANCE`, `GWL_STYLE` and `GWL_EXSTYLE`; writes to those indices are
  not implemented.
- `DestroyWindow` marks teardown in progress, synchronously sends `WM_DESTROY`
  and `WM_NCDESTROY`, purges queued messages for that handle, and releases its
  SDL window and extra data. Reentrant or repeated destruction returns false.
  It does not post `WM_QUIT` or shut SDL down while another window is alive.
- `ShowWindow`, `IsWindowVisible`, `GetClientRect`, `MoveWindow`,
  `GetClassNameA`, `SetWindowTextA`, `GetWindowTextA` and `GetFocus` provide the
  basic window state used by the smoke test. SDL geometry is synchronized
  before a create/move call returns. Sizes are SDL client sizes in logical
  units; outer-frame/client conversions are not yet provided. `MoveWindow`'s
  repaint parameter now invalidates and synchronously updates the client.
- `SendMessageA` invokes the target procedure synchronously, including nested
  sends. `CallWindowProcA` supports per-window subclass chains.
- `PostMessageA` preserves pointer-sized parameters in a FIFO. A null HWND
  denotes a thread message. Invalid or destroyed HWNDs are rejected.
- `PeekMessageA` pumps SDL without sleeping, applies HWND and message-range
  filters, and honors `PM_NOREMOVE`/`PM_REMOVE`. A HWND of `(HWND)-1` selects
  thread messages. `WM_QUIT` bypasses filters. There is no fallback dispatch
  to a different window when a target is absent.
- `GetMessageA` blocks through SDL's event wait when no matching message is
  ready; it returns 1 for a message, 0 for quit and -1 for an invalid filter,
  null output pointer or failed SDL wait. `WaitMessage` leaves messages queued.
- `DispatchMessageA` ignores thread/quit messages and stale handles.
  `DefWindowProcA` accepts nonclient creation, implements close/title messages
  and does not quit on destruction. `PostQuitMessage` is the application's
  explicit last-window exit policy.

SDL close, resize, focus gain/loss, common keyboard keys, CP1252 text, mouse
motion and left/right/middle buttons are routed to the exact window ID.
Keyboard messages use Win32 virtual keys and PC/AT scan-code fields, including
extended keys, repeat/up bits and system-key messages. `TranslateMessage`
generates control characters; SDL text events supply printable `WM_CHAR`
messages directly so calling it does not duplicate them. Focus-loss messages are delivered,
unlike upstream's suppression policy.

## Boundaries and lifetime

All Win32 compatibility calls and shutdown run on the SDL main/UI thread.
[Child and owned windows](WIN32-CHILDREN.md) are now supported.
Cross-thread Win32 posts/sends, broadcast posts, menus,
extended window styles beyond `WS_EX_TOPMOST`, control classes, full activation
policy, IME and Unicode APIs remain to do. [Capture](WIN32-CAPTURE.md), key-state
queries, wheels, double-clicks and CP1252 input are now supported; see
[input contracts](WIN32-INPUT.md). Unknown SDL window IDs are dropped, never sent to the surviving window.
Message timestamps use the existing SDL-based clock. Routed input messages
carry their recorded screen point in `MSG.pt`; generic posted messages do not
yet populate that field. This is not a complete Win32 UI implementation.

Creation accepts child parent/ID pairs and top-level owners. It rejects a
top-level menu handle or extended styles beyond `WS_EX_TOPMOST`. Unsupported
or invalid operations report diagnostics through `SDL_GetError`; window operations do not yet consistently set Win32 `GetLastError`.

Small released handle records remain as tombstones until
`StarsCompatShutdown()` so a newly allocated window cannot acquire the same
pointer and make a stale HWND valid again. Heavy window/extra data is freed on
destruction. Shutdown destroys any live windows, clears queued messages and
class/handle records, and releases only the SDL video reference acquired by
this runtime. Call it after the message loop; shutdown from a window procedure
is rejected to prevent freeing a record still on the callback stack. All
handles and atoms must be discarded before starting a new runtime session.

## Verification on October 10, 2026

The automated test covers copied class names and atoms, creation payloads,
rejected/self-destroying creation, 64-bit user/extra data and message values,
per-window subclassing, nested sends, rejected callback-time shutdown,
visibility/titles, synchronous resize, filtered queue removal, thread messages,
focus transfer/loss, signed mouse coordinates, scan-code/up flags, text routing,
blocking waits, destroyed-ID rejection, queued-message purging, reentrant
teardown, and explicit quit filters. It closes the first window and confirms
the survivor keeps receiving input and messages before closing it too.

Debug and Release compatibility suites pass with the dummy video backend.
The test also passes with real X11 windows under Xvfb. Address, undefined
behavior and leak sanitizers pass for the dummy-backend self-test outside the
sandbox (LeakSanitizer cannot inspect threads inside the sandbox).

Current Windows/host and game regression checks still pass. See the
[bitmap verification report](WIN32-BITMAPS.md#verification)
for the latest full results, including the tutorial failures. The earlier
window milestone encountered the year-2406 tutorial stall; later painting
runs encountered Wine heap errors, and the bitmap run failed on production
queue validation. Neither is recorded as passing.

## Client fullscreen and final cleanup

Display-sized borderless popups now request SDL native fullscreen; native
parenting associates dialogs with a fullscreen owner. The dummy backend keeps
HWND ownership without unsupported SDL native parenting. The X11 startup test
uses its own Xvfb/Openbox display to verify fullscreen and modal ownership.
See [client testing](WIN32-CLIENT.md).

`StarsCompatProcessExit` releases the wrapper without window-procedure callbacks
after `WinMain` has released game state. `StarsCompatShutdown` retains normal
destroy notifications for applications that still own their state. Both must
run on the UI thread outside a window-procedure callback.
