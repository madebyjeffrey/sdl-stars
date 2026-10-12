# Headless native UI testing

Headless mode runs the same game UI, logical windows, dialogs, message loop and
software GDI painting through SDL's invisible `dummy` video/audio backends. It
needs no display server at runtime and does not create desktop windows. Logical
`IsWindowVisible` state still means a control participates in the game UI; it
does not mean an operating-system window is displayed in this mode.

## Run the automated UI suite

Use the normal build, then the headless CTest preset:

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --preset sdl-headless
```

The preset runs sequentially, clears DISPLAY/WAYLAND_DISPLAY, selects dummy
video/audio plus software rendering, and excludes the separate X11 and Wayland desktop
qualification tests. No new libraries are needed. Existing configure dependencies
remain those in [SDL development](SDL3-DEVELOPMENT.md); this preset does not run
Xvfb, Openbox or external mouse/keyboard automation.

For just the real game UI workflows:

```sh
ctest --preset sdl-headless -R '^win32_client_(startup|race|continue)_dummy$'
```

Those tests start the shared WinMain, inspect/render the title, use queued SDL
mouse/key events to open/cancel New Game, traverse all race-wizard pages with
Back/Next, and load a copied saved game. Continue also exercises the Technology
Browser close paths. Settings and game fixtures are isolated from the user's
files. Failures have bounded timeouts. Bitmap captures remain available under
`dist/sdl-debug/test-client/` for inspecting an invisible run.

Fatal sanitizer qualification uses the same invisible backend:

```sh
cmake --preset sdl-sanitize
cmake --build --preset sdl-sanitize
ctest --preset sdl-headless-sanitize
```

## Launch or debug the application invisibly

The native entry point consumes `--headless` before forwarding the remaining
arguments to the unchanged game parser:

```sh
./dist/sdl-debug/bin/stars-sdl --headless
```

In CLion, choose **stars-sdl** in the SDL Debug profile and put `--headless` in
Program arguments. This starts the ordinary UI event loop invisibly; it does
not supply actions or automatically terminate a run. An in-process driver can
use the [hierarchy API](WIN32-INSPECTION.md), resolve control IDs and invoke their
normal messages, or inject SDL input events. The complete tutorial driver and
semantic map-object adapter remain next steps.

For the already automated, bounded scenario in CLion, debug
**stars-compat-client-startup** with environment:

```text
SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;STARS_SDL_RENDERER=software;STARS_SETTINGS_DIR=/tmp/stars-headless-debug
```

Create that settings directory first (`mkdir -p /tmp/stars-headless-debug`).
Use the CTest workflows when you need their isolated fixture/capture setup.
The ordinary application's `--headless` launch otherwise uses its normal files
and settings.

Headless software tests qualify UI behavior and rendered surfaces. GPU drivers,
compositor stacking, native focus/decorations and the external browser/printing
integrations still have separate desktop/integration checks; the headless mode
covers the SDL application UI and does not virtualize other applications.

## Verification

The entry test checks flags at different argument positions, repeated flags,
legacy-parser limits, overriding incompatible driver settings, and a logically
visible window using the dummy backend. The Debug headless preset passes 80/80 tests with DISPLAY and WAYLAND_DISPLAY
cleared. Its selected commands/environments were also checked to contain no
X11 launchers. The fatal headless sanitizer preset also passes 80/80 without
suppressions. The final inspector background/pixel change was separately
rechecked in headless mode.

Native desktop file pickers normally use the platform backend. `--headless`
sets `STARS_FILE_DIALOG_BACKEND=compat`, as do the headless presets, keeping the
logical picker inside the invisible SDL UI. The portal wire test uses a private
D-Bus session and fake service; its child explicitly selects the portal backend
without displaying any platform dialog.
