# SDL3 compatibility development

For native Windows x64 builds and portable runtime packaging, see
[Windows SDL3](WINDOWS-SDL3.md). WSL is not required.

The native `stars-sdl` client now links the shared game and UI. Select
**stars-sdl** in CLion under `sdl-debug`, or run
`./dist/sdl-debug/bin/stars-sdl`. See [client status and testing](WIN32-CLIENT.md).


DIB conversion and all 38 bitmap resources are now implemented.
`stars-compat-dib` runs shared Windows/Linux pixel and readback checks;
`stars-compat-paint` draws the real icon atlas in two windows. See the
[DIB milestone](WIN32-DIBS.md).

Memory DCs and bitmap blits are now implemented. `stars-compat-paint` draws
through offscreen bitmaps; `stars-compat-bitmap` runs focused ownership/pixel
checks shared with Windows. See [bitmap milestone](WIN32-BITMAPS.md).

The painting milestone adds **stars-compat-paint**: select this CLion target
for two Win32-created windows with independent drawings and text. Resizing
and key presses repaint; closing one keeps the other working. See
[Win32 painting](WIN32-PAINTING.md) for APIs, limitations and tests.

The window/message milestone adds **stars-compat-windows**: select this target
in CLion for two windows created through the Win32 API. Run without arguments
for an interactive test, or use `--self-test` for automated checks. See
[Win32 windows and dispatch](WIN32-WINDOWS.md) for scope and debugging.

The SDL presets also build `stars-compat-abi`, `stars-ui-compat-check` and
`stars-compat-runtime`. The runtime target tests the initial C11 implementation
adapted from Free API; see [adoption and coverage](FREE-API-ADOPTION.md).
The former validates the Win32 source ABI; the latter compiles the existing
UI against native compatibility declarations without linking it. See
[compatibility headers](../compat/win32/README.md) for scope, the Windows
reference test and CLion targets. Run both ABI and smoke tests with
`ctest --test-dir dist/sdl-debug -L compat --output-on-failure`.
Both SDL build presets also build `stars-host` and the `stars-host-unit-tests`
aggregate target. A complete `ctest --test-dir dist/sdl-debug --output-on-failure`
run therefore has all registered host and compatibility test executables after
building the preset; use Xvfb for the X11 tests outside a desktop session.

The first SDL milestone builds `stars-compat-smoke`, a native Linux executable with two independent desktop windows. It exercises SDL window events, persistent CPU surfaces, GPU texture presentation and software fallback. It does not yet implement Win32 APIs or run the Stars UI. The existing host and MinGW targets retain their normal configuration.

## Sanitizer configuration

The separate `sdl-sanitize` preset instruments project C code with
AddressSanitizer and UndefinedBehaviorSanitizer, preserves frame pointers and
stops at the first undefined-behavior report in each process. SDL and system
libraries retain their own normal builds. Select this profile in CLion for
debugging, or run:

```bash
cmake --preset sdl-sanitize
cmake --build --preset sdl-sanitize
xvfb-run -a ctest --test-dir dist/sdl-sanitize --output-on-failure
```

GCC supplies the sanitizer runtimes with the existing build dependencies; no
extra desktop service is needed. The initial run exposed signed bit-31 masks
in `GetRaceGrbit`/`SetRaceGrbit`, now unsigned. Further sanitizer qualification
is open: pointer arrays from the legacy two-byte-aligned heap trigger an
alignment report at `LpflNew` (`util.c`). No sanitizer finding is suppressed.

## Dependencies

The SDL presets first search for installed SDL3 3.4 or newer, including `$HOME/.local/opt/stars-sdl3`. If none is found, `STARS_SDL_FETCH=ON` downloads SDL 3.4.18 into the build tree and checks its SHA-256. The first configure requires network access; subsequent builds reuse it. Host and MinGW presets do not fetch SDL. Turn fetching off with `-DSTARS_SDL_FETCH=OFF` when using an installed package.

For Pop!_OS/Ubuntu 24.04, install the desktop build dependencies:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config \
  python3 python3-yaml gdb libfreetype-dev libfontconfig1-dev libcairo2-dev zlib1g-dev poppler-utils cups-client libwmf-bin \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
  libxi-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols libdecor-0-dev \
  libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev \
  libvulkan-dev vulkan-tools libdbus-1-dev libibus-1.0-dev libudev-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev xvfb xauth
```

For automated fullscreen and dialog-stacking tests, also install:

```bash
sudo apt install -y openbox x11-utils xvfb xauth
```

Openbox runs only inside the private Xvfb test display. It is not a runtime
dependency and does not change your desktop session. The startup test creates
its own display; other X11 tests still use the outer `xvfb-run -a ctest` display.
These tests exercise X11, not the laptop’s Wayland compositor.

Both SDL build presets also build `stars-help-guide`, which generates the
original guide under `dist/sdl-debug/help` or `dist/sdl-release/help`. Select
this CMake target in CLion to rebuild only the guide, or use
`cmake --build --preset sdl-debug --target stars-help-guide`. Its WMF conversion
requires `libwmf-bin`. See [help content and qualification](WIN32-HELP.md).

Use the laptop's existing GPU driver. `vulkaninfo --summary` reports Vulkan support. A software Vulkan device can also run the GPU API, so a passing GPU check alone does not prove hardware acceleration; check the reported Vulkan device. The compatibility text runtime uses FreeType and Fontconfig with bundled Liberation fonts; see [fonts/text](WIN32-FONTS.md). The direct SDL smoke target retains SDL's debug text. SDL_ttf and custom shader tools are not required.

For an offline configure, extract the checksum-verified SDL archive and set `FETCHCONTENT_SOURCE_DIR_SDL3` to that source directory. This explicit CMake override bypasses downloading; verify its source version yourself. Ordinary CLion use needs neither this override nor a dependency installed system-wide.

## Build and run

From the repository root:

```bash
cmake --preset sdl-debug
cmake --build --preset sdl-debug
./dist/sdl-debug/bin/stars-compat-smoke
```

Move and resize both windows. Space pauses only the window receiving the key. Close either window: the other keeps running. Closing both exits. The console reports the video driver, renderer and shared GPU backend. `auto` attempts SDL's GPU renderer and falls back to software if initialization fails. Choose explicitly:

```bash
STARS_SDL_RENDERER=gpu ./dist/sdl-debug/bin/stars-compat-smoke
STARS_SDL_RENDERER=software ./dist/sdl-debug/bin/stars-compat-smoke
SDL_VIDEODRIVER=x11 ./dist/sdl-debug/bin/stars-compat-smoke
```

`gpu` fails rather than silently falling back. Wayland can be selected with `SDL_VIDEODRIVER=wayland` when the current desktop provides a Wayland connection. Window placement is left to the desktop compositor. This applies to the direct
SDL smoke target and now to compatibility top-level windows as well. Native
Wayland popups use parent-relative placement and controls use logical units.
X11/Xwayland remain supported. See [desktop qualification](DESKTOP-QUALIFICATION.md)
for private Wayland tests, macOS presets and remaining platform limits.
The Mesa runtime-assembler leak now has a pinned private dependency repair and
fatal sanitizer qualification; see [build and test instructions](MESA-GRAPHICS-LEAK.md).
This opt-in software-driver build leaves the installed desktop driver unchanged.

`cmake --build --preset sdl-release` builds the smoke executable in `dist/sdl-release/bin` after `cmake --preset sdl-release`. The restricted build presets build the smoke, ABI, runtime and window tests,
UI compile check and SDL dependency; they do not build the host or all unit tests. To build every native target, use `cmake --build dist/sdl-debug`.

## Automated smoke checks

CTest registers a display-independent software test when `STARS_BUILD_TESTS` is enabled:

```bash
ctest --test-dir dist/sdl-debug -R '^sdl_smoke_software$' --output-on-failure
```

It uses SDL's dummy video driver with software rendering. It verifies distinct pixel readbacks for both surfaces, queued keyboard routing, resizing, closure of one window and rendering in the survivor. It exits automatically. Real desktop/backend checks use the same test:

```bash
STARS_SDL_RENDERER=gpu ./dist/sdl-debug/bin/stars-compat-smoke --self-test
STARS_SDL_RENDERER=software ./dist/sdl-debug/bin/stars-compat-smoke --self-test
xvfb-run -a env SDL_VIDEODRIVER=x11 STARS_SDL_RENDERER=software \
  ./dist/sdl-debug/bin/stars-compat-smoke --self-test
```

The self-test injects queued SDL key/close events; it is not a substitute for physical keyboard, desktop focus and window-manager checks. Xvfb/software tests do not establish hardware GPU or Wayland support.

## CLion

1. Open the repository as a CMake project and use the native Linux GCC/GDB toolchain.
2. Press `Ctrl+Shift+A`, run **Load CMake Presets**, select `sdl-debug` and press Enter. Open **Settings → Build, Execution, Deployment → CMake**, select the imported SDL Debug profile and tick **Enable profile**. CLion imports new presets disabled by default; they do not appear in the build/run switcher until enabled. The configure preset's display name is **Linux SDL3 compatibility Debug**. Apply the settings and reload CMake. The initial dependency build may take longer than the application build.
3. Select the `stars-compat-smoke` CMake Application target and `sdl-debug` profile. Use the repository root as the working directory.
4. Run or debug. Useful breakpoints are `main`, `DispatchSmokeEvent`, `FDrawSmokeWindow` and `CloseSmokeWindow` in `tests/compat/sdl_smoke.c`.
5. Add a second run configuration with `STARS_SDL_RENDERER=software`, and an automated configuration with program arguments `--self-test`. Environment variables belong in the run configuration, not CMake options.

The fetched shared SDL library is located through CMake's development RPATH; no `LD_LIBRARY_PATH` setting should be needed. The presets do not use `toolchains/mingw-w64.cmake`. Local `.idea` changes need not be committed.

If **Load CMake Presets** does not list the SDL preset, check that the project
root contains `CMakePresets.json`. In **Settings → Advanced Settings**, search
for **Automatically import CMake Presets** and permit configure and build
presets, then load the presets again. See [JetBrains' preset instructions](https://www.jetbrains.com/help/clion/cmake-presets.html).

As a fallback, add an enabled CMake profile named `sdl-debug`, select the
native Linux toolchain, use Debug and Ninja, and set its CMake options to:

```text
-DSTARS_BUILD_SDL_CLIENT=ON -DSTARS_SDL_FETCH=ON
```

Use `dist/clion-sdl-debug` as that manual profile's build directory so it has
its own CMake cache. Reload CMake, then select `stars-compat-runtime` or
`stars-compat-smoke` as the application target.

## Next milestone

[Child windows](WIN32-CHILDREN.md), fonts/layout and bitmap resources now have
local implementations and tests. Push/check/radio buttons and text labels now
work; see [controls](WIN32-CONTROLS.md). Basic edit fields also work; see
[edit fields](WIN32-EDIT.md). Fixed-height string and owner-drawn lists also work; see [list boxes](WIN32-LISTS.md).
Continue with remaining edit/list behavior and combo controls,
dialogs, menus,
full-client qualification, real-desktop Wayland/GPU checks and native tutorial automation. Keep the source inventory as the implementation checklist. The [implementation plan](LINUX-SDL3-WIN32-PLAN.md) describes the remaining qualification work. The linked `stars-sdl` client remains a preview.

## Verification on October 10, 2026

Debug and Release smoke builds passed without warnings in the smoke source.
The automated software check passed in both configurations. Debug also
passed X11/software under Xvfb and real-desktop X11/GPU on the Intel ADL
GT2 adapter through Vulkan. Wayland has not been exercised.

The SDL Debug build's 12 native unit executables and software smoke test
all passed. The existing MinGW build and all 13 Windows unit executables
passed. Full Windows regression through turn 150 returned 519 matches,
22 unused-storage warnings, and zero differences/errors. No baseline was
changed. The full native test build also reports existing format-truncation
warnings in untouched unit-test sources.

The tutorial check failed at year 2400, page 4, instruction 31 ("Hold down
the shift key and select Alexander"), after 12 actions. Its artifacts are
in `dist/scaffold/tutorial/20261010T080447.181541Z/`. This differs from the
previous page-21 stall; the tutorial is not claimed to pass. No game UI,
core behavior, save layout or tutorial observer was changed in this milestone.

The compatibility printer uses Cairo >= 1.16 with its PDF backend. Poppler's
`pdfinfo` and `pdftoppm` verify generated pages when tests are enabled. `cups-client`
supplies the optional system-printer `lp` command; PDF output does not require a
printer or CUPS server. See [printing setup and CLion target](WIN32-PRINTING.md).

## Invisible UI tests

Use `ctest --preset sdl-headless` after building SDL Debug; it runs sequentially
without a desktop display. `stars-sdl --headless` starts the real UI event loop
invisibly for an in-process driver or debugger. See [headless UI testing](SDL3-HEADLESS.md).

## Native file selection dependencies

Linux native file selection adds `libdbus-1-dev` at build time and requires
`xdg-desktop-portal` plus the desktop's FileChooser backend at runtime. This
laptop already has libdbus, the portal, GTK and COSMIC backends installed.
For a GTK desktop setup:

```sh
sudo apt install libdbus-1-dev xdg-desktop-portal xdg-desktop-portal-gtk
```

The isolated protocol test also needs `dbus-run-session`, `python3-dbus` and
`python3-gi`; these are installed here. See [native file dialogs](WIN32-FILE-DIALOGS.md#platform-native-file-selection).

Product installs and the `sdl-package` CLion preset are described in
[Linux packaging](LINUX-PACKAGING.md), including local Flatpak builds.
