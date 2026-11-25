# Linux SDL3 Win32 compatibility implementation plan

This is the historical implementation plan. Current SDL-only build policy,
source layout, native Windows dialogs and remaining acceptance gaps are tracked
in [ROADMAP.md](ROADMAP.md). Historical native Win32 build commands below are
retained for context and no longer describe the maintained product.

Date: October 10, 2026.

Build a native Linux Stars client using the existing C game and UI sources, a source-level implementation of the Windows APIs they use, and SDL3 for desktop windows, input and accelerated presentation. Add a `stars-sdl` CMake executable target and Debug/Release presets that CLion can import. Keep the current Windows client and command-line host available for comparison.

This report specifies the full implementation plan. SDL3 dependency discovery/fetching, `sdl-debug`/`sdl-release` presets and `stars-compat-smoke` are available; see [SDL3 development](SDL3-DEVELOPMENT.md). The source wrapper now implements **178/178 inventoried Win32 APIs (100%)**, with documented limits; see [coverage](FREE-API-ADOPTION.md). Compatibility tests, the compile-only UI target and the linked native `stars-sdl` client are available; see [client status](WIN32-CLIENT.md). Default-printer PDF/CUPS output now works; see [printing](WIN32-PRINTING.md). Modal open/save file pickers now work; see [file dialogs](WIN32-FILE-DIALOGS.md). Interactive message boxes and asynchronous audio alerts now work; see [message boxes](WIN32-MESSAGE-BOXES.md). All 34 canonical dialog templates now create; see [combo boxes](WIN32-COMBOS.md). Full-client build/run commands below are available; full qualification remains planned. The implemented font path uses FreeType/fontconfig, as documented in [fonts](WIN32-FONTS.md), rather than the SDL_ttf route originally proposed here.

## 1. Scope and design decisions

The [API inventory](WIN32-APIS.md) identifies 178 Windows function spellings, 5 legacy macros and 6 Microsoft CRT functions. Its [call locations](WIN32-API-CALLS.csv), [UI surface](WIN32-UI-SURFACE.csv), [test calls](WIN32-TEST-APIS.csv) and [imports](WIN32-IMPORTS.csv) are the implementation checklist. DLL startup and compiler-support imports are not part of the source wrapper.

Use these decisions for the first implementation:

- Build native GNU C11 with Linux GCC. Do not define `_WIN32` for this target.
- Preserve application functions, debug-symbol names, UI procedures and resource IDs. Limit source changes to platform selection and actual ABI or operating-system boundaries.
- Use one UI thread, one SDL event pump, and a logical Windows window/control hierarchy.
- Create an SDL desktop window for each visible top-level frame or dialog. Draw child windows and controls inside their top-level window rather than creating an SDL window for every control.
- Implement GDI drawing into persistent CPU bitmap surfaces first. Upload changed regions and present through SDL's GPU renderer. This is accelerated presentation; drawing itself is initially software.
- Keep a software presentation fallback. Later GPU implementations must match the CPU reference before replacing it.
- Start desktop validation on X11, then qualify Wayland separately. Do not promise arbitrary positioning or cross-window screen coordinates on Wayland.
- Use bundled, redistributable fonts with a fixed mapping from requested Windows font names. Record intentional visual differences.

SDL supplies platform windows and rendering, not Windows controls or message semantics. The controls, dialogs, menus and GDI state machine are substantial implementation work.

## 2. Install development dependencies

The observed system is Pop!_OS 24.04 LTS, based on Ubuntu Noble, on x86-64. CMake, Ninja, GCC, Make, Python/PyYAML, MinGW-w64, Wine and Go have already been exercised. The following command is safe to repeat and installs the additional development headers and diagnostic tools. It is a desktop-focused subset of SDL's published Linux dependencies, with font and Vulkan tools added. [SDL Linux build dependencies](https://wiki.libsdl.org/SDL3/README-linux).

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build git pkg-config gdb \
  python3 python3-pip python3-venv python3-yaml \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev \
  libxfixes-dev libxi-dev libxss-dev libxtst-dev \
  libxkbcommon-dev libwayland-dev wayland-protocols libdecor-0-dev \
  libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev \
  libvulkan-dev vulkan-tools \
  libdbus-1-dev libibus-1.0-dev libudev-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev \
  libfreetype-dev libharfbuzz-dev libcairo2-dev zlib1g-dev fonts-liberation \
  xvfb xauth poppler-utils cups-client libwmf-bin
```

Vulkan also needs a functioning driver for the laptop's GPU. Keep the existing vendor driver, especially on a System76/NVIDIA setup. For a Mesa-managed Intel or AMD installation, the relevant runtime package is `mesa-vulkan-drivers`; do not replace a working proprietary driver just to build this project. Check the existing installation:

```bash
vulkaninfo --summary
printf 'Desktop session: %s\n' "$XDG_SESSION_TYPE"
```

A Vulkan failure must not prevent using the software fallback. OpenGL development packages support the alternative SDL renderer route. No shader compiler is needed for the initial GPU-renderer presentation path; custom shaders are a later milestone.

For existing Windows comparisons, retain:

```bash
sudo apt install -y mingw-w64 binutils-mingw-w64-x86-64 wine
```

Use the existing Go installation instead of installing a second copy. This shell needed `.profile` loaded explicitly:

```bash
source "$HOME/.profile"
go version
cmake --version
ninja --version
x86_64-w64-mingw32-gcc --version
```

CLion launched from the desktop may have a different environment from a login terminal. Give its build environment the Go PATH when running Make targets, or use `/usr/local/go/bin/go` through the existing `GO` Make variable. The native CMake client target does not need Go; regression comparisons do.

## 3. Install pinned SDL3 and SDL3_ttf

Use a user-owned prefix rather than depending on the distribution shipping a particular SDL3 version. The selected releases are SDL 3.4.18 and SDL_ttf 3.2.2. Their official release pages were checked for this report. Pin these tags for repeatable setup; future updates should be deliberate. [SDL release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18), [SDL_ttf release](https://github.com/libsdl-org/SDL_ttf/releases/tag/release-3.2.2).

Run this in Bash. The clone destinations must not already contain an unrelated checkout. For a repeat setup, reuse the existing clones and verify their checked-out tags instead of cloning over them.

```bash
set -e
STARS_SDL_PREFIX="$HOME/.local/opt/stars-sdl3"
STARS_SDL_SOURCES="$HOME/.local/src/stars-sdl3"
mkdir -p "$STARS_SDL_PREFIX" "$STARS_SDL_SOURCES"

git clone --depth 1 --branch release-3.4.18 \
  https://github.com/libsdl-org/SDL.git "$STARS_SDL_SOURCES/SDL"
cmake -S "$STARS_SDL_SOURCES/SDL" \
  -B "$STARS_SDL_SOURCES/SDL/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$STARS_SDL_PREFIX" \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DSDL_SHARED=ON -DSDL_STATIC=OFF \
  -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF
cmake --build "$STARS_SDL_SOURCES/SDL/build" --parallel
cmake --install "$STARS_SDL_SOURCES/SDL/build"

git clone --depth 1 --branch release-3.2.2 \
  https://github.com/libsdl-org/SDL_ttf.git "$STARS_SDL_SOURCES/SDL_ttf"
cmake -S "$STARS_SDL_SOURCES/SDL_ttf" \
  -B "$STARS_SDL_SOURCES/SDL_ttf/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$STARS_SDL_PREFIX" \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DCMAKE_PREFIX_PATH="$STARS_SDL_PREFIX" \
  -DBUILD_SHARED_LIBS=ON \
  -DSDLTTF_VENDORED=OFF -DSDLTTF_HARFBUZZ=ON \
  -DSDLTTF_PLUTOSVG=OFF -DSDLTTF_SAMPLES=OFF
cmake --build "$STARS_SDL_SOURCES/SDL_ttf/build" --parallel
cmake --install "$STARS_SDL_SOURCES/SDL_ttf/build"

PKG_CONFIG_PATH="$STARS_SDL_PREFIX/lib/pkgconfig" \
  pkg-config --modversion sdl3 SDL3_ttf
```

SDL_ttf uses system FreeType and HarfBuzz here; SVG color emoji support is disabled because it is unnecessary for this UI. Its pinned CMake source defines these options and the `SDL3_ttf::SDL3_ttf` target. [SDL_ttf build configuration](https://raw.githubusercontent.com/libsdl-org/SDL_ttf/release-3.2.2/CMakeLists.txt).

Inspect SDL's configure summary: desktop builds must actually enable X11 and/or Wayland, and the chosen accelerated renderer. A successful build with no usable desktop video backend is not sufficient. SDL publishes its installation procedure and exported CMake targets. [SDL CMake integration](https://wiki.libsdl.org/SDL3/README-cmake).

## 4. Proposed repository layout

Keep the wrapper implementation independent of the game's rules and globals:

```text
compat/win32/include/windows.h       Supported Windows declarations
compat/win32/include/direct.h        UI CRT compatibility declarations
compat/win32/include/io.h            Only required CRT compatibility declarations
compat/win32/internal.h              Private wrapper structures
compat/win32/window.c                Handles, classes and window hierarchy
compat/win32/message.c               Queue, dispatch, timers and modal loops
compat/win32/controls.c              Standard controls and notifications
compat/win32/menu.c                  Menus and accelerators
compat/win32/dialog.c                Dialog construction and navigation
compat/win32/gdi.c                   DC state and CPU reference drawing
compat/win32/font.c                  Font lookup, metrics and rasterization
compat/win32/resource.c              Resource lookup and decoders
compat/win32/platform.c              INI, CRT, common dialogs and services
compat/win32/sdl.c                   SDL windows, events and presentation
compat/win32/entry.c                 Linux main and WinMain bridge
compat/win32/CMakeLists.txt          stars_win32_sdl library
res/resourcegen.py                  Windows resource blob generation
cmake/sdl-client.cmake              Native client and resource target wiring
cmake/dependencies/                 Dependency version documentation
resources/fonts/                   Approved font files and licenses
tests/compat/                      Wrapper semantic and pixel tests
```

New identifiers follow the repository's Hungarian naming conventions. Keep disk structs and core headers independent of SDL.

## 5. Add the CMake target before full UI integration

### 5.1 Restructure the current platform branches

`CMakeLists.txt` currently returns from the non-Windows branch after setting up the host and native tests. Simply appending a client target at the bottom will never build it on Linux.

1. Add `option(STARS_BUILD_SDL_CLIENT "Build the SDL3 desktop client" OFF)` before that branch.
2. Keep core creation, generated data/text dependencies and host creation shared.
3. Keep the existing native host tests using `hostui.c`; add separate client-linked test executables with distinct names and CTest labels.
4. Before the non-Windows `return()`, include `cmake/sdl-client.cmake` when the option is enabled.
5. Leave the existing Windows-only targets, `.rc` compilation and DLL linkage in the Windows branch.
6. Initially expose a two-window `stars-compat-smoke` executable. Add `stars-sdl` once declarations, resources and linkable implementations exist. Do not fill missing APIs with success-returning stubs to claim a working client.

The final target arrangement should be:

| Target | Purpose | UI implementation |
| --- | --- | --- |
| `stars_core` | Existing portable game object library | None |
| `stars-host` | Existing native host executable | `hostui.c` |
| `stars_win32_sdl` | New static compatibility library | SDL3-backed Windows subset |
| `stars_ui_sdl` | Existing UI sources compiled for Linux | Wrapper headers and functions |
| `stars-sdl` | New Linux client executable | `stars_ui_sdl` and wrapper |
| `stars-compat-smoke` | Early window/render integration test | Wrapper |

Never link `hostui.c` into `stars-sdl`: it would supply competing implementations of the game's `ui.h` interface.

### 5.2 Target wiring

This is a proposed integration skeleton, not a patch to apply before the named files and generated resources exist:

```cmake
find_package(SDL3 3.4 CONFIG REQUIRED)
find_package(SDL3_ttf 3.2 CONFIG REQUIRED)
add_subdirectory(compat/win32)

add_library(stars_ui_sdl OBJECT
    ${STARS_UI_SOURCES}
    "${STARS_GENERATED_DIR}/text_tutorial.c"
)
target_link_libraries(stars_ui_sdl PUBLIC stars_options stars_win32_sdl)
target_compile_definitions(stars_ui_sdl PRIVATE STARS_UI_SDL=1)
add_dependencies(stars_ui_sdl stars_version stars_sdl_resources)

add_executable(stars-sdl
    compat/win32/entry.c
    "${STARS_GENERATED_DIR}/sdl_resources.c"
)
target_link_libraries(stars-sdl PRIVATE
    stars_core stars_ui_sdl stars_win32_sdl
)
target_compile_definitions(stars-sdl PRIVATE STARS_UI_SDL=1)
add_dependencies(stars-sdl stars_sdl_resources)
set_target_properties(stars-sdl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
```

`compat/win32/CMakeLists.txt` must create `stars_win32_sdl`, link `SDL3::SDL3` and `SDL3_ttf::SDL3_ttf`, and publish its compatibility include directory with `BEFORE`. That directory supplies `<windows.h>`, `<direct.h>` and `<io.h>` only to wrapper consumers. Do not add it globally or to `stars_core`.

Use `STARS_UI_SDL` on UI and wrapper targets, not on the core. Any wrapper code that calls portable application helpers also needs the existing `stars_options` quote include paths and generated-header dependencies; expose only the necessary portable declarations. Keep `_WIN32` undefined so `native.c` selects its existing POSIX implementation. Reuse current `-iquote` include handling to prevent the project's `strings.h` from shadowing the system header.

The resource target must generate `sdl_resources.c` through an `add_custom_command(OUTPUT ...)`, list all resource files and `version.h` as dependencies, and use `VERBATIM`. Make the blob table API independent of Stars by passing the generated table to wrapper initialization from `entry.c`.

### 5.3 Add presets

Append these objects to the existing `configurePresets` and `buildPresets` arrays. Preserve the current schema version, minimum CMake version and all existing presets:

```json
{
  "name": "sdl-debug",
  "displayName": "Linux SDL3 client Debug",
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/dist/sdl-debug",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_PREFIX_PATH": "$env{HOME}/.local/opt/stars-sdl3",
    "STARS_BUILD_SDL_CLIENT": "ON",
    "STARS_BUILD_TESTS": "ON"
  }
}
```

```json
{
  "name": "sdl-release",
  "inherits": "sdl-debug",
  "displayName": "Linux SDL3 client Release",
  "binaryDir": "${sourceDir}/dist/sdl-release",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Release"
  }
}
```

```json
{
  "name": "sdl-debug",
  "configurePreset": "sdl-debug",
  "targets": ["stars-sdl", "stars-compat-smoke"]
}
```

```json
{
  "name": "sdl-release",
  "configurePreset": "sdl-release",
  "targets": ["stars-sdl"]
}
```

Add a separate build preset for all compatibility/native test executables, or build without `--preset` when running CTest. A preset restricted to `stars-sdl` does not build those test executables.

Once implemented:

```bash
cmake --preset sdl-debug
cmake --build --preset sdl-debug
./dist/sdl-debug/bin/stars-sdl

cmake --build dist/sdl-debug --parallel
ctest --test-dir dist/sdl-debug --output-on-failure
```

CMake normally provides development build RPATHs for imported shared libraries. Confirm with `ldd dist/sdl-debug/bin/stars-sdl`; if local SDL libraries are unresolved, correct the target build RPATH rather than relying on a terminal-only `LD_LIBRARY_PATH` setting. Packaging needs a separate install RPATH policy.

## 6. Configure CLion

1. Open the existing repository root as a CMake project.
2. In **Settings → Build, Execution, Deployment → Toolchains**, select a native Linux toolchain with GCC, GDB, CMake and Ninja. The SDL presets must not use the MinGW toolchain file.
3. Reload CMake presets. Use **Find Action → Load CMake Presets** if they have not been imported.
4. Enable the `sdl-debug` profile under CMake settings if it is disabled. Confirm its build directory and dependency prefix.
5. Select the `stars-sdl` CMake Application run/debug configuration, using the SDL Debug profile. Set the working directory to the repository root initially, matching the resource/INI policy established during integration.
6. Debug with breakpoints in `WinMain`, `InitInstance`, `FrameWndProcDeferred`, message dispatch and the wrapper's SDL event conversion.
7. Add CTest configurations for compatibility tests. Add a separate X11 run configuration with `SDL_VIDEODRIVER=x11`, and a Wayland configuration where the desktop exposes Wayland. Add `STARS_SDL_RENDERER=software` for the proposed fallback configuration; this environment variable is a wrapper feature to implement, not an existing SDL setting.

CLion imports configure/build presets into CMake profiles; local compiler/debugger selection belongs to its toolchain settings. Avoid checking in machine-specific `.idea` settings. [CLion preset support](https://www.jetbrains.com/help/clion/cmake-presets.html).

## 7. Implement the wrapper in dependency order

Each milestone should be a focused commit or small group of behavior-neutral commits. Add tests of externally observable semantics, rather than tests that merely duplicate helper implementations.

### Milestone 1: Windows declarations and Linux ABI

Implement only the declarations used by the inventory, expanding the list when compilation exposes additional SDK types or macros. Keep `LONG` and `DWORD` 32-bit, `WORD` 16-bit, and `WPARAM`, `LPARAM`, pointer fields and procedure results pointer-sized. Provide compatible struct tags for `HWND__` and `HDC__`, which `native.h` already forward-declares. Define the rectangle guard expected by `native.h` to avoid duplicate `RECT` declarations, and add static assertions for public structures and scalar widths.

Provide the existing inline point adapters with compatible `POINT` types. Preserve `POINT16` and every disk-facing layout. Audit wrapper header packing separately from on-disk structs; copying all MinGW packing pragmas globally would be unsafe.

Implement `wsprintf`/`wsprintfA` with the existing 32-bit formatting behavior of `CchSprintf`, and `MulDiv` through `LMulDiv`. Do not forward Windows `%ld` formats to Linux `vsprintf`: Linux x86-64 `long` is 64-bit. Cover signed, unsigned, width, negative and rounding boundaries. Map `_stricmp`, `_strnicmp`, `_strdate` and `_strtime` deliberately; use narrow-character behavior compatible with the existing game text.

Implement `GetTickCount` as the same wrapping 32-bit monotonic millisecond clock as `DwTickCount`. Preserve explicit legacy macro behavior where appropriate. Do not emulate desktop logoff when Stars calls `ExitWindows`: return a documented unsupported-operation failure or route the feature through a deliberate application policy, and report the difference.

Exit criterion: all UI sources compile against wrapper declarations; unsupported runtime functions remain explicitly tracked.

### Milestone 2: Handles, windows, message dispatch and multiple windows

Implement validated handles with object kinds and explicit ownership. Store logical parent and owner relationships separately, sibling order, visibility, enabled state, window procedures, styles, control IDs, user data and invalid regions. Destruction must cancel timers and queued references and safely handle procedures that destroy windows during a call.

Implement `RegisterClass`, `CreateWindow`, destruction, window data, subclassing and `CallWindowProc`. Handle creation/destruction notifications and default behavior before adding controls. `SendMessage` dispatches synchronously and supports reentrancy. `PostMessage` queues messages. Implement `GetMessage`, `PeekMessage`, quit propagation, translation and dispatch with one integrated SDL event pump.

Map SDL window IDs to root logical windows. Implement per-root focus, capture, hit testing, keyboard state, character input, wheel deltas and timer delivery. Use SDL text input for text characters, distinct from key events. Coordinate conversions must include parent offsets and a clear logical-pixel/display-scale policy.

Test two independently resizable top-level windows; redraw either without corrupting the other, close one without quitting prematurely, and exercise a modal child of each. Keep rendering dirty windows independent; do not unnecessarily wait on a hidden or minimized window's presentation.

Wayland does not provide arbitrary global positioning like classic Windows. Keep child/control coordinates in the wrapper and avoid depending on desktop-wide positions. Validate menus, capture and cross-window dragging on both X11 and Wayland; document any remaining restrictions. [SDL Wayland notes](https://wiki.libsdl.org/SDL3/README-wayland).

Exit criterion: two-window smoke application with child hit testing, synchronous/queued messages, subclassing, capture and timers.

### Milestone 3: GDI reference rendering and acceleration

Basic painting is now implemented in `compat/win32/src/paint.cpp`: window DCs,
paint lifecycle, solid/null objects, lines/rectangles, fixed ASCII text, and
per-window SDL renderer presentation. `stars-compat-paint` verifies two-window
painting and resize/close behavior. The subsequent [bitmap milestone](WIN32-BITMAPS.md) adds memory DCs,
compatible bitmaps and ROP3 blits. This milestone remains partial: regions,
DIB/resource loading, transforms and full fonts are still pending. See
[implemented painting subset](WIN32-PAINTING.md).

Represent each DC's selected objects, origin, clip region, colors, background mode and raster operation. Own persistent CPU surfaces for drawable windows and memory bitmaps. Implement rectangles, lines, ellipses, patterns, text and bitmap copies; respect source/destination clipping and overlapping copies. Implement the actually used ROP2 and BitBlt/PatBlt operations exactly, including XOR. Translate indexed DIBs and palettes without changing resource colors or table indices.

`BeginPaint` consumes the appropriate invalid region; `EndPaint` marks changed pixels for presentation. `GetDC` drawing outside paint must persist and become visible. Implement selection/deletion lifetimes, stock objects, memory DCs and the region operations needed by scanner drawing.

Initially use `SDL_CreateGPURenderer` with a shared GPU device and one renderer per top-level SDL window; share CPU images and create renderer-specific textures. Let SDL's renderer manage its window presentation. Do not separately claim the same windows for a competing raw-GPU presentation path. SDL's GPU renderer accepts a caller-provided device. [SDL GPU renderer](https://wiki.libsdl.org/SDL3/SDL_CreateGPURenderer).

If initialization fails, use `SDL_CreateRenderer(window, "software")` for presentation and report the active backend. Implement an explicit renderer selection option. For a later raw SDL GPU backend, let one device claim multiple windows, each with its own swapchain; provide pinned shader artifacts and synchronization before replacing the renderer path. [SDL GPU window ownership](https://wiki.libsdl.org/SDL3/SDL_ClaimWindowForGPUDevice).

Exit criterion: deterministic pixel fixtures for clipping, copy overlap, XOR, patterns, palettes and text, plus accelerated updates in two windows and a working software fallback.

### Milestone 4: Fonts and resources

Select fixed fonts and preserve their licenses. Liberation fonts installed above are a development fallback, not automatically a guarantee of matching Windows metrics. Resolve requested face, weight, size and rotation; keep text measurements and rendered glyph placement consistent. Decode application ANSI/CP1252 strings to Unicode at the display boundary while preserving original bytes for saves and game strings. Check numeric labels, ellipses, extended characters and rotated text.

Avoid implementing a general RC-language parser initially. Compile `res/stars.rc` with `x86_64-w64-mingw32-windres -O res` into a Windows resource stream, then have `resourcegen.py` validate and embed the resource records in generated C tables. This is a build-time cross-tool use; the resulting executable is native Linux and does not link Windows DLLs.

An initial resource-compiler command, after the SDL configure has generated `version.h`, is:

```bash
mkdir -p dist/sdl-debug/generated
(
  cd res
  x86_64-w64-mingw32-windres -O res \
    -I ../dist/sdl-debug/generated \
    -i stars.rc -o ../dist/sdl-debug/generated/stars.res
)
```

The production CMake custom command must set the equivalent resource working directory, then invoke the new Python generator on this `.res` file. The `sdl_resources.c` output is compiled as ordinary Linux C; do not attempt to link a COFF resource object into ELF.

Preserve numeric and named resource types/IDs. Parse standard and extended dialog templates and menu formats as required by the emitted stream, as well as accelerator tables, bitmap/DIB data, grouped icons/cursors and embedded tutorial bytes. Decode resource strings according to their actual encoded format; the RC source's CP1252 declaration must survive compilation correctly. Resolve group icon/cursor records to their referenced image entries. Inspect the generated formats before deciding which parsers can be omitted.

Model Win32 resource and memory handles separately, preserve lock/unlock semantics, and validate sizes before reading. Treat version resources explicitly even if only the generated `version.h` supplies application display text.

Exit criterion: all resource IDs used by Stars resolve, one real dialog is instantiated with correct IDs and bounds, images/cursors load and embedded tutorial files match their original bytes.

### Milestone 5: Standard controls, dialogs, menus and accelerators

Implement the exact messages and notifications in `WIN32-UI-SURFACE.csv`: buttons/check/radio state; edit text, selection and limits; list and combo contents/selections; scrollbars; owner drawing and measurements. Maintain notification timing, focus behavior and subclass forwarding. Support drop-down popups without assuming global screen positioning is available.

Implement dialog initialization, control lookup, default/cancel buttons, Tab traversal, mnemonics, enable state and dialog return values. A modal loop must continue pumping messages for other windows while restricting input to the owner. Modeless dialogs must coexist with the normal loop.

Build menus and accelerators from resources, including command IDs, enabled/check state, popup dismissal and keyboard handling. Implement the default window procedure's meaningful behavior; it cannot simply return zero for every message.

Exit criterion: resource-based dialog tests, edited text, list/combo choices, keyboard navigation, nested notifications and modal lifetime checks pass.

### Milestone 6: Platform services and application entry

Implement INI/profile functions, system metrics/colors, message boxes, resource memory, timing and CRT calls still used directly in UI files. Map host/service calls to existing POSIX abstractions where possible. Preserve profile integer/string defaults, buffer limits, section/key handling and writes. Decide where user settings live and how command-line overrides find them; preserve portable game file paths.

`GetOpenFileName`/`GetSaveFileName` now use the wrapper's modal controls and filesystem browser; see [file-dialog contracts](WIN32-FILE-DIALOGS.md). SDL's extension-only filter interface cannot express Stars' `*.R*` and `*.M*` patterns directly, so the implementation preserves those patterns locally. Selection, cancellation, buffers, filename/extension fields and owner lifetime have native and SDK tests. If a future native-dialog adapter replaces this browser, preserve the same filters and contracts, keep callback state alive, and marshal asynchronous callback delivery to the UI thread. [SDL file dialog API](https://wiki.libsdl.org/SDL3/SDL_ShowOpenFileDialog).

Implement `main` in `entry.c`: initialize wrapper/SDL/fonts/resources, establish a compatible module handle, construct the command-line tail expected by `WinMain`, call it and release resources. Test quoting and filenames with spaces rather than joining `argv` with plain spaces. Keep Stars' host-generation options usable through this executable for regression testing.

The default-printer `PrintDlg`/`Escape` path now has a tested Cairo PDF backend and CUPS submission adapter; see [printing contracts](WIN32-PRINTING.md). Interactive print settings and physical-printer/full-map qualification remain. `MessageBeep` has an asynchronous SDL implementation. The `WinHelp` path now opens the original help content and mapped context pages. The checked native archive reader establishes the original title index, context map and complete topic-record chain; all 413 numeric contexts resolve. Paragraph/table commands, 32 font descriptors, 125 bitmap images, eight WMFs and 28 image hotspots now decode; see [help content](WIN32-HELP.md). The `stars-help-guide` CMake target now renders 414 topics with clickable links, popups and a filterable index. The WinHelp adapter now has tests for all 413 contexts; the user confirmed offline browser launch, with offline content checks still remaining. Until unavailable commands have real implementations and the full client is qualified, label the build a preview. Track every remaining inventory item as implemented, equivalent adapter, documented platform difference or unavailable. No unavailable path should report success.

Exit criterion: Linux client starts, loads/saves an existing game, runs dialogs, changes settings and generates host turns using native file I/O.

### Milestone 7: Bring up Stars and qualify the Linux client

Integrate in this order: title window; new/open game; frame and scanner; planet/mine/message panes; production/research/ship dialogs; report windows; battle playback; race wizard; tutorial. At each stage compare the same resource/state with Windows and investigate differences before broadening scope.

Maintain one implementation status row per inventory symbol. API availability is only the first check: record tested styles/messages, ownership rules and remaining behavior limitations. Keep compiler warnings and intentional behavior changes separate from mechanical wrapper work.

## 8. Verification commands and acceptance criteria

The existing baseline is the reference. Do not regenerate it for the compatibility layer. The current Windows regression passed all 9 scenarios through turn 150, with 519 matches and 22 unused-storage warnings. All 13 Windows unit tests passed. The existing Windows tutorial stalled at year 2406, page 21, instruction 166; preserve that evidence when evaluating the new UI and do not treat it as a newly introduced Linux regression without reproduction.

For each completed implementation batch, run the repository checks:

```bash
source "$HOME/.profile"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
make regression
make test-unit
make tutorial
make regression-host
make test-host
```

Host checks are mandatory when core code changes and useful at the integration gate. Add native client verification once the new targets exist:

```bash
cmake --preset sdl-debug
cmake --build dist/sdl-debug --parallel
ctest --test-dir dist/sdl-debug --output-on-failure
SDL_VIDEODRIVER=x11 ./dist/sdl-debug/bin/stars-compat-smoke
STARS_SDL_RENDERER=software ./dist/sdl-debug/bin/stars-compat-smoke
```

Run a Wayland configuration only in a session with the required compositor/socket. `xvfb-run` can support X11/software tests, but it does not prove real GPU or Wayland behavior. Add ASan/UBSan as a separate native development preset; investigate failures around alignment, arithmetic and object lifetimes rather than immediately suppressing them.

Add client-linked versions of the relevant existing unit tests. The UI cases in `test_battle.c`, `test_race.c` and the initialization helpers currently select Windows behavior with `_WIN32`. Select those UI paths with `defined(_WIN32) || defined(STARS_UI_SDL)` where appropriate; leave genuine OS-specific file enumeration and `getcwd` branches using the native Linux implementation. Update `stars_test.h` to include `win.h` for SDL UI tests, initialize wrapper resources before `FCreateStuff`, and use portable private C adapters for `AlertSz` interception on client test executables. Do not broaden every `_WIN32` guard indiscriminately or link the host's `IdAlertBox` stub alongside the UI implementation.

Use the existing regression runner against the Linux client after its host options work. Keep its work directory separate from the existing native and host runs:

```bash
cmake --preset sdl-release
cmake --build --preset sdl-release
make save-cli
python3 tests/scaffold/regression.py prepare \
  --seed 12345 --exe dist/sdl-release/bin/stars-sdl \
  --work dist/scaffold/regression/sdl
python3 tests/scaffold/regression.py run \
  --cli dist/stars-save --work dist/scaffold/regression/sdl --through 150
python3 tests/scaffold/regression.py compare \
  tests/scaffold/fixtures/regression/native \
  dist/scaffold/regression/sdl --through 150 \
  --report dist/scaffold/regression/sdl-comparison.json
```

The current runner selects native invocation when the executable does not have an `.exe` suffix and defaults to its full `SCENARIOS` list when `--scenario` is omitted. Preserve its expected exit codes: creation returns 0 and successful turn generation returns 1. Show the first diff if results diverge unexpectedly.

Update the tutorial observer and parser together when their assumptions change. Linux-native automation cannot drive SDL controls through Windows handles; add a test observer/input interface exposing logical control rectangles, messages and state, and a runner using it. Restrict automation hooks to `STARS_TEST_*` builds. Test focus/capture and actual SDL event translation separately so direct message injection does not hide input bugs.

Acceptance requires:

1. Native `stars-sdl` configures/builds from presets and debugs in CLion without Wine.
2. Real GPU and software presentation work; at least two top-level windows can redraw, resize, focus and close independently.
3. Modal/modeless dialogs, menus, edit/list/combo controls and scanner capture/scrolling behave as specified.
4. Save/turn/race/history formats and host results match the current baseline.
5. Wrapper ownership, clipping, raster operations, fonts and resources have meaningful tests.
6. Every inventory item has a reviewed status; unavailable features are visible and documented.
7. The Windows build and existing host remain usable, with no new unexplained warnings or baseline changes.

## 9. Packaging and completion

Add an install rule for `stars-sdl`, its SDL/SDL_ttf libraries if bundled, licensed fonts and any external resources. Use a relocatable layout such as `bin/`, `lib/` and `share/stars/`, with executable-relative resource lookup and `$ORIGIN/../lib` install RPATH where appropriate. Do not ship the developer's absolute dependency prefix or rely on CLion environment variables.

Test an installed copy from outside the repository on a clean x86-64 Linux system. Record required system runtime libraries and GPU drivers. Keep the current product-version generation; no save-file version change is warranted by this UI port.

Update `docs/ROADMAP.md` when implementation milestones are actually completed, and `docs/NATIVE-PORT.md`/`docs/WIN16-PARITY.md` when existing adapters change. Record intentional behavior changes separately in `CHANGELOG.md`, with their required regression evidence. This implementation plan itself does not mark any roadmap work complete.

## 10. Suggested commit sequence

| Order | Deliverable | Demonstrable result |
| --- | --- | --- |
| 1 | Dependency pins, CMake options and smoke target | CLion configures a native SDL project |
| 2 | Public declarations and ABI tests | Compatible types and formatting |
| 3 | Handles, message loop and SDL windows | Two-window dispatch/capture/timer test |
| 4 | CPU GDI and GPU/software presentation | Pixel-tested drawing in both windows |
| 5 | Resource generation and fonts | Real resources and stable measurements |
| 6 | Controls, dialogs, menus and accelerators | Complete interactive test dialog |
| 7 | Entry point and platform services | Linkable Linux Stars client |
| 8 | Stars UI integration in small batches | Playable preview with tracked limitations |
| 9 | Client regression and native UI automation | Baseline and UI behavior evidence |
| 10 | Remaining features, packaging and documentation | Installable qualified Linux client |

Build the smoke target first. It exposes the architectural risks—multiple windows, message semantics, persistent drawing and font/resource handling—before the whole application depends on them.
