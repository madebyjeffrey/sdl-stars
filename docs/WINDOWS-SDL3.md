# Native Windows SDL3 build

`windows-sdl-debug` and `windows-sdl-release` build the same SDL3 Win32 source
wrapper used on Linux and macOS as native Windows x64 executables. Neither the
application nor its users need WSL, Wine or the MSYS runtime. MSYS2 UCRT64 is
the development toolchain; the portable install includes the runtime DLLs.
The original SDK-backed `stars.exe` and its reference/unit tests remain separate
targets, using the same core objects as `stars-sdl.exe`.

## Install development dependencies once

Install [MSYS2](https://www.msys2.org/) into `C:\msys64`. Open **MSYS2 UCRT64**
from the Start menu and run:

```sh
pacman -Syu
```

If instructed to close the terminal, reopen UCRT64 and repeat that command.
Then install the native Windows packages:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-python-yaml \
  mingw-w64-ucrt-x86_64-sdl3 \
  mingw-w64-ucrt-x86_64-freetype \
  mingw-w64-ucrt-x86_64-fontconfig \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-libwmf
```

Use the UCRT64 packages and tools consistently. The `/usr/bin` compiler, if
installed, targets the MSYS environment rather than a standalone Windows app.

## Build and package from PowerShell

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_windows.ps1
.\dist\windows-sdl-release-portable\bin\stars-sdl.exe
```

The execution-policy override applies only to this invocation. For Debug:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_windows.ps1 -Configuration Debug
```

For another MSYS2 installation, pass `-Msys2Root D:\msys64`. The script sets
PATH for its own process, uses MSYS2 Python with PyYAML, builds the targets and
installs the client, host, original help guide and dependent DLLs. Distribute
the entire `dist/windows-sdl-release-portable` directory, preserving `bin` and
`share`; copying just the executable loses its libraries and help files.

Equivalent commands in an MSYS2 UCRT64 terminal:

```sh
cd /c/Projects/stars
cmake --preset windows-sdl-release -DPython3_EXECUTABLE=C:/msys64/ucrt64/bin/python.exe
cmake --build --preset windows-sdl-release --parallel 4
cmake --install dist/windows-sdl-release \
  --prefix "$PWD/dist/windows-sdl-release-portable" --component StarsRuntime
```

The installed executable runs from ordinary PowerShell or Explorer. Development
executables in `dist/windows-sdl-release/bin` need `C:\msys64\ucrt64\bin` on
PATH to locate DLLs. `STARS_SDL_RENDERER=software` selects software rendering.
Windows settings default to `%LOCALAPPDATA%\stars\Stars.ini`;
`STARS_SETTINGS_DIR` overrides that directory for isolated runs.

## Verification

Build Debug and run the Windows SDL checks from UCRT64:

```sh
cmake --preset windows-sdl-debug -DPython3_EXECUTABLE=C:/msys64/ucrt64/bin/python.exe
cmake --build --preset windows-sdl-debug --parallel 4
ctest --preset windows-sdl-headless
```

The wrapper has native Windows tests for Unicode paths/settings, binary file
contents, distinct file identities, destination replacement and sparse files
larger than 4 GiB. Shared tests select the SDL backend by `STARS_UI_SDL`, not
by the operating system. Linux X11/D-Bus and macOS integration tests retain
their existing platform-specific harnesses.

The regression runner now launches Windows `.exe` files directly on Windows;
on other systems it continues using Wine. Go is needed to build its save CLI:

```sh
cd tests/savecli
go build -o ../../dist/stars-save.exe .
cd ../..
python tests/scaffold/regression.py prepare \
  --exe dist/windows-sdl-debug/bin/stars-host.exe \
  --work dist/scaffold/regression/windows-host-new
python tests/scaffold/regression.py run \
  --work dist/scaffold/regression/windows-host-new --cli dist/stars-save.exe
python tests/scaffold/regression.py compare \
  tests/scaffold/fixtures/regression/native \
  dist/scaffold/regression/windows-host-new --cli dist/stars-save.exe \
  --report dist/scaffold/regression/windows-host-new/comparison.json
```

Choose a new work directory each run. Substitute `stars.exe` to qualify the
original Windows client. The frozen original fixtures and native baseline do
not change for this port.

### Results on this checkout

- Native Windows x64 SDL Debug and Release builds succeed. The portable
  Release client loads with only Windows system directories on PATH and
  generates a turn from a copied game using `--headless`.
- The Windows SDL headless preset passes 25/25 checks, including actual
  New Game/save-picker coverage, startup,
  the race wizard, four saved-game workflows with Unicode paths and the
  bounded tutorial smoke scenario. This is not the complete tutorial walkthrough.
  The New Game test cancels the real picker, then saves Basic/Advanced games in
  ASCII and Unicode paths; it also exercises the native Windows video backend
  with the default renderer during local qualification.
- Full SDK-client and host regressions each return 519 matches (74 with
  unused-storage warnings), zero differences/errors, through turn 150.
- The separate SDK-backed MinGW suite passes 36/55 checks on this Windows
  desktop; 19 fail, including native dialog/control reference tests and the
  `test_native_fileui` caption/password assertions. The game unit executables
  pass 16/17. The edited `nativeui.c` preprocesses identically to HEAD for the
  SDK backend; these failures are not claimed fixed by the SDL port. The logs
  are `dist/mingw-tests.log` and `dist/windows-sdl-debug/final-sdl-tests.log`.

Compiler warnings were not introduced. Linux/macOS rebuilds and Windows GPU,
native chooser/printer and complete tutorial qualification remain separate work.

## Platform boundary and future Rust integration

The SDK-backed core filesystem implementation remains in `native.c`. SDL UI
sources include the wrapper's `windows.h`; `platform_windows.c` alone includes
real SDK declarations for filesystem services. CMake generates Windows wrapper
symbol aliases such as `StarsCompat_CreateWindowExA`, so wrapper handles cannot
accidentally reach identically named system APIs. Linux/macOS keep their current
symbols. The wrapper's calling convention remains C, including its callbacks;
it is independent of the SDK's historical Win32 calling convention.

A future Rust consumer can use the `stars_win32` C library through an
`extern "C"` interface with a matching compiler/runtime, or through a DLL facade
that keeps CRT-owned objects private. Bindings must use these wrapper declarations and opaque
wrapper handles, not the Windows crate's system handles. Fixed-width integer
fields and pointer-sized parameters already distinguish disk storage from the
process ABI. Keep allocations and frees on the owning side, and do not unwind
Rust panics through C callbacks. Rust bindings, a supported public game-core API,
an MSVC build and Windows ARM64/32-bit qualification are future work; this change
qualifies the UCRT64 x64 toolchain without claiming those targets work.

The Windows file chooser currently uses the shared SDL compatibility browser,
and printing uses the shared PDF backend. Native Windows chooser and printer
integration need separate SDK adapters; CUPS printer names are Linux-only.
