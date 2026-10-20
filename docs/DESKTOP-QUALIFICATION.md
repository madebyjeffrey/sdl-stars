# Desktop and platform qualification

For transferring the committed work and resuming on a Mac or Windows desktop,
use [the desktop handoff guide](DESKTOP-HANDOFF.md): platform dependencies,
CLion/build commands, acceptance order and a ready-to-use resume request.

Native Wayland startup no longer fails when SDL rejects desktop positioning.
Top-level placement belongs to the compositor. Child controls and native popups
use owner-local coordinates; global physical desktop coordinates and arbitrary
stacking are unavailable on Wayland. Windowed intro startup and the race wizard
are exercised on a private compositor, without touching the user's desktop.

The UI canvas and event coordinates stay in logical units. SDL high-pixel-density
windows and nearest-neighbor texture scaling present that canvas at the output's
pixel density. The `STARS_SDL_RENDERER` setting accepts `auto`, `software` or
`opengl`; an explicit renderer request fails if unavailable rather than falling
back silently. Tests check the actual selected renderer.

## Linux automated checks

Install the invisible Wayland test compositor with `sudo apt install weston`.
CMake detects it and includes six sequential tests: native startup, race wizard,
and coordinate/popup/capture/presentation checks at two scales with two renderers.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R wayland --output-on-failure -j1
```

`tests/compat/run_wayland.py` creates a private runtime directory, settings folder
and Weston socket. It removes `DISPLAY` and uses Mesa software rendering through
the OpenGL driver, so no test surface appears on the user's desktop. The headless
compositor disables libdecor: native title-bar appearance on GNOME/COSMIC is a
separate real-desktop check. X11 tests continue to use private Xvfb/Openbox.

## macOS build path

The product target and existing Cocoa native-file-picker adapter are enabled on
Darwin. The Linux-only X11 and DBus harness is excluded; test adapters use ordinary C symbols.
The ABI executable and direct SDL two-window smoke target are also available
with portable C test adapters. The platform thread ID uses `pthread_threadid_np` instead of Linux's `gettid`.
A Mac is required to establish successful compilation and native execution.

```sh
brew install cmake ninja pkg-config sdl3 freetype fontconfig cairo libwmf python
cmake --preset macos-sdl-debug
cmake --build --preset macos-sdl-debug
```

Use Python 3.14 or newer with pip on `PATH`, or pass its path as
`-DPython3_EXECUTABLE=...`. CMake creates the build-local environment and
installs the pinned Python requirements.

The SDL presets target Apple silicon (`arm64`) with Homebrew dependencies under
`/opt/homebrew`. Host presets use the native architecture; both use binary64
arithmetic. The commands above run in a native Apple silicon shell. ARM-native turn
parity remains separate work and is not claimed here. The generated offline help
requires `wmf2svg`, supplied by libwmf. These commands are qualification instructions,
not a claim that this Linux machine tested a macOS build.

## Remaining hardware and desktop acceptance

- Verify windowed intro decorations and title-owner visibility through native
  Open/Save on the actual Linux compositor.
- Check focus/capture and menu/combo/tooltip placement on GNOME/COSMIC and KDE.
- Check physical GPU drivers, fractional scaling, multiple monitors, moving a
  window between different scales, and display hotplug. Headless integer scaling
  and Mesa tests do not prove these hardware behaviors.
- Build and exercise the client and native picker on macOS; exercise the Windows
  build on a real Windows desktop. Wine/SDK reference tests remain a separate gate.

These checks are saved for later manual qualification, as requested. File formats,
record sizes, game rules and native regression baselines are unchanged.

## Validation results and dependency leak

The allocation owner is now identified and a local Mesa dependency patch fixes
the cleanup; see [Mesa executable-heap repair](MESA-GRAPHICS-LEAK.md). All six
Stars Wayland sanitizer cases pass with that explicit private dependency. The
same build with only the patch reverted reproduces exactly 112 bytes in the
SDL-only smoke. Installed system/Flatpak drivers are not changed by this repair,
so their leak status must be reported separately. The original findings below
record the system-driver failure rather than a failure of the patched build.

All six new Wayland cases pass in Debug, including both scales and explicit
OpenGL. The final 11 startup/race/file-workflow/Wayland checks pass, including
transient-parent assertions on windowed dialog owners in private X11 and Wayland. The broad fatal sanitizer run passed the 102 existing non-tutorial
cases; all six new Wayland cases completed their assertions but failed LSan
with the same 112-byte/two-allocation report in the OpenGL presentation stack.
This report reproduces in `stars-compat-smoke --self-test`, which links SDL only
and exercises no Win32 compatibility code. No suppression or disabled leak
check was used. Explicit EGL thread release and Mesa softpipe did not resolve it;
those exploratory changes are not part of the implementation. Recording loaded
module mappings subsequently identified Mesa's runtime-assembler heap as the
allocation owner, and the local dependency patch repairs its last-free cleanup.

The system-driver Wayland ASan/UBSan/LSan gate still fails until that driver is
repaired. The explicit patched software-driver gate passes; it does not qualify
the installed hardware driver or real compositor. Real desktop/hardware and
macOS checks above remain outstanding.

SDL documents the platform positioning and cursor restrictions in its
[Wayland backend guide](https://wiki.libsdl.org/SDL3/README-wayland).

The Windows-native regression matches all 519 checkpoints, with 87 existing
unused-storage warnings and no differences/errors or skipped inputs.

The four-path Continue workflow also passes on native Wayland (relative/absolute
and ASCII/UTF-8 fixture paths), including toolbar/menu/combo/tooltip placement,
map inspection, report bars and gameplay dialogs. Its injected pointer events
are surface-relative; it does not attempt unsupported global pointer warping.
Pointer hit testing is scoped to the mouse-focus/event surface on Wayland so an
unrelated focused window does not receive another surface's wheel input.
All 52 Windows unit/reference tests pass under Wine.

The final input/windowed-dialog batch passes 11/11 fatal ASan/UBSan/LSan
focused tests on dummy/private X11, and 13/13 focused Debug checks.
