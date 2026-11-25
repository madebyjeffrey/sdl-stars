# Stars!

Stars! is a turn-based 4X space strategy game, rebuilt in C from the original
Win16 executable. This repository develops a portable SDL3 desktop client and
an independent command-line host for Windows, Linux and macOS.

**The current desktop client is alpha.** This README primarily helps developers
build, test and contribute. Automated game regressions pass on Windows, Linux
and Apple-silicon macOS, but full tutorial coverage and desktop acceptance remain
unfinished. See [desktop qualification](docs/DESKTOP-QUALIFICATION.md) and
[macOS qualification results](docs/MACOS-BUNDLE.md#deployment-and-verification)
for the scope of the checks.

The [specification handbook sources](handbook/index.md) connect player and host
workflows with the [module specifications](spec/README.md), formal models, and
coverage gaps. See [previewing and updating the handbook](handbook/maintaining.md)
for the Zensical build and GitHub Pages publishing workflow.

## Build and run

Run the commands below from the repository root. All builds need C11 and C++20 compilers,
CMake 3.23 or newer, Ninja, and Python 3.12 or newer with pip and venv support.
CMake creates a build-local Python environment and installs the pinned Python
dependencies on the first configure. Desktop builds also need
SDL3, FreeType, Fontconfig, Cairo and libwmf for fonts, printing and offline help.
The game, desktop UI, host, SDL wrapper, generated tables and ordinary tests
compile as C++20; AppKit adapters use Objective-C++20. Independent storage and
Windows SDK reference tests retain C11. See the [migration plan](docs/CPP-MIGRATION.md).
The SDL presets use an installed SDL3 3.4 or newer, or download the pinned SDL
source on the first configure. Host-only builds do not need SDL or desktop
libraries.

### Windows

Install [MSYS2](https://www.msys2.org/), open its **UCRT64** terminal, and update
with `pacman -Syu` (reopen the terminal and repeat if requested). Install the
native Windows dependencies:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-pip \
  mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-freetype \
  mingw-w64-ucrt-x86_64-fontconfig mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-libwmf
cmake --preset windows-sdl-debug -DPython3_EXECUTABLE=C:/msys64/ucrt64/bin/python.exe
cmake --build --preset windows-sdl-debug
./dist/windows-sdl-debug/bin/stars-sdl.exe
```

Adjust the Python path if MSYS2 is installed elsewhere. Use UCRT64 tools
consistently; its Python must be 3.12 or newer and include pip. Development
executables need the UCRT64 DLLs on `PATH`, which this terminal supplies.
See [Windows build and packaging](docs/WINDOWS-SDL3.md) for PowerShell scripts
and portable installs. `mingw-debug` / `mingw-release` are legacy SDL preset aliases.

### Linux

On Ubuntu 24.04 / Pop!_OS, install the build and test dependencies:

Use Python 3.12 or newer with pip and venv support. Ubuntu 24.04's system
Python meets the version requirement; install `python3-pip` and `python3-venv`
along with the packages below. CMake creates the build venv and installs
PyYAML automatically.

```sh
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config \
  python3 python3-pip python3-venv libfreetype-dev libfontconfig1-dev libcairo2-dev \
  zlib1g-dev poppler-utils cups-client libwmf-bin \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
  libxi-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols libdecor-0-dev \
  libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev \
  libvulkan-dev vulkan-tools libdbus-1-dev libibus-1.0-dev libudev-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev xvfb xauth openbox x11-utils
cmake --preset sdl-debug
cmake --build --preset sdl-debug
./dist/sdl-debug/bin/stars-sdl
```

Native file selection uses the desktop's `xdg-desktop-portal` FileChooser
backend. See [SDL development](docs/SDL3-DEVELOPMENT.md) for portal setup,
other test dependencies, sanitizers and troubleshooting, and
[Linux packaging](docs/LINUX-PACKAGING.md) for runtime installs and Flatpak.

### macOS

Use an Apple-silicon Mac with Xcode command-line tools and Homebrew installed.
The desktop presets target `arm64` and Homebrew under `/opt/homebrew`:

```sh
brew install cmake ninja pkg-config sdl3 freetype fontconfig cairo libwmf python
cmake --preset macos-sdl-debug
cmake --build --preset macos-sdl-debug
./dist/macos-sdl-debug/bin/stars-sdl
```

Select a Python 3.12 or newer interpreter with pip explicitly when configuring
if it is not the first `python3` on `PATH`. See
[macOS build details](docs/DESKTOP-QUALIFICATION.md#macos-build-path) and
[app bundle packaging](docs/MACOS-BUNDLE.md). Client, host and signed app
regressions have passed on Apple silicon; older macOS execution and public
notarized distribution remain unqualified.

Each platform has a corresponding Release preset: `windows-sdl-release`,
`sdl-release` or `macos-sdl-release`. With GNU Make installed, `make compile`
selects the platform's Debug preset automatically.

### Command-line host

`stars-host` creates universes, generates turns and writes dumps using the
original `stars.exe` host command line (`-a game.def`, `-g[n] game.hst`,
`-v game.hst`, `-b`, `-t`, `-s<seed>`, `-dm`/`-dp`/`-df game.mN`).
For a host-only build, use the same compiler, CMake, Ninja and Python 3.12 or newer:

```sh
cmake --preset host-release
cmake --build --preset host-release
./dist/host-release/bin/stars-host -g1 /path/to/game.hst
```

On Windows, select UCRT64 Python as above and run `stars-host.exe`.
`make host` defaults to `host-release` on every platform. To use the separate
native macOS preset, run `make host HOST_PRESET=macos-host-release`; its output
is `dist/macos-host-release/bin/stars-host`. `stars-host --version` prints the
build version.

The host differs from the original in these ways:

- It reads `stars.ini` only with `--ini stars.ini`, and only the host settings:
  `[Files] Logging` and `[Misc] DefaultPassword`, `NewReports` (per-player `-d`
  files such as `game.p1`), `NoHostNames` and `Backups`. Otherwise defaults apply.
- It doesn't wait for turns (`-w`).
- On Linux and macOS it can't detect another program holding a game file open
  through Windows file-sharing rules.
- Questions receive the cautious answer (No, Cancel); messages go to stderr.

Turn generation uses binary64 `double`, with intentional differences from the
original documented in [double precision](docs/DOUBLE-PRECISION.md).

## Verify changes

The Make targets below require GNU Make; regression tooling also needs Go
for the save comparison CLI in `tests/savecli/`. Run a quick setup check with:

```sh
make test-unit
make regression-quick  # smallai4 through turn 10
```

For broader headless wrapper/client checks, run `ctest --preset sdl-headless`
on Linux or `ctest --preset windows-sdl-headless` on Windows after building Debug.

Before submitting changes, follow the full verification in [AGENTS.md](AGENTS.md):

```sh
make compile     # platform SDL Debug build
make regression  # all baseline scenarios through turn 150
make test-unit
make tutorial    # available SDL tutorial checkpoints
```

For game-code changes, also run `make regression-host` and `make test-host`.
Tutorial checks provide partial coverage; the full 80-page SDL walkthrough is
unfinished, and the macOS tutorial target currently registers no tests.

`make regression` builds the Release SDL client and `dist/stars-save`, runs a
fresh game suite under `dist/scaffold/regression/native`, and compares against
`tests/scaffold/fixtures/regression/native/`. Any difference fails.
`SCENARIOS="noai smallai4"` and `THROUGH=10` limit a run. Report unexpected
changes starting with the first diff. Behavior-neutral changes must preserve
the baseline; intentional behavior changes update it in the same commit with
`make regression-export` after a full run. Never change the frozen original
2.6j fixtures. See [regression instructions](tests/scaffold/REGRESSION.md),
[unit tests](tests/unit/README.md) and [save CLI](tests/savecli/README.md).

## Architecture and contributing

The desktop client uses SDL3 through the project's Win32 API wrapper on all
three platforms. The shared game core also builds into `stars-host`, which is
independent of SDL. Game code reaches the UI through `src/core/ui.h`.

| Directory | Purpose |
| --- | --- |
| `src/core/` | Game rules, AI, turn generation, save layouts and the UI contract |
| `src/ui/` | Shared desktop dialogs, drawing, window state and original tutorial |
| `src/host/` | Command-line entry point and host UI adapter |
| `src/platform/` | Native files, clock, formatting and shared encoding |
| `compat/win32/` | SDL-backed Win32 API wrapper and isolated platform bridges |
| `data/`, `text/`, `res/` | Authored game data, text and resources |
| `dist/` | Generated sources, builds, test output and runtime installs |

CMake generates text and data tables and wrapper resources from the authored
inputs. Original source filenames and debug-symbol names are preserved.
Open/save uses native platform pickers; `STARS_FILE_DIALOG_BACKEND=compat`
selects the deterministic SDL picker for tests.

Read [AGENTS.md](AGENTS.md) before editing. In particular:

- Preserve compatibility with original save, turn, history and race files;
  on-disk layouts and record sizes must not change.
- Preserve original symbol names and follow the surrounding C style.
- Keep commits focused. Each intentional behavior change needs its own commit,
  a changelog entry and an updated native regression baseline.
- Keep completed work and known limitations reflected in the relevant docs.

Start with the [roadmap](docs/ROADMAP.md) and [known bugs](docs/KNOWN-BUGS.md)
for work to tackle. Desktop acceptance and tutorial automation remain open.

## Alpha builds for playing

The rolling [latest prerelease](https://github.com/sirgwain/stars/releases/tag/latest)
provides `stars-sdl-windows-x64.zip`: extract the whole archive and run
`bin/stars-sdl.exe`. Keep its libraries and help files alongside the executable.
It also provides `stars-host-linux-x64.tar.gz` for hosting on Linux.

For Linux desktop installs and macOS app bundles, follow the
[Linux packaging](docs/LINUX-PACKAGING.md) and [macOS bundle](docs/MACOS-BUNDLE.md)
guides. These are alpha builds with the qualification limits described above.

## Project history and branches

![Stars! on modern Windows](docs/images/windows10stars.jpg)

The original Stars! 2.6jrc3 executable included roughly 1 MB of debug information:
function and variable names, symbol definitions and line numbers. The custom
[stars-asm](https://github.com/sirgwain/stars-asm/tree/main) Win16 disassembler and
decompiler used that information to reconstruct C sources close to the original.

- `2.6j` (tag `2.6jrc3`): the finished faithful reconstruction, original bugs
  included. It matches the original turn for turn in the fixed-seed regression.
- `2.8` (tag `v2.8.0`): the released native port line, taking bug fixes only.
  It reads 2.6j files and writes the same formats marked as version 2.84, which
  2.6j/2.7 refuse to load.
- `main`: the 2.9 development line. The core/UI/host split is complete, and the
  desktop client now uses SDL3 on every platform. See the changelog and roadmap
  for current work and the remaining alpha qualification.

## Documentation and CI

- [Reconstruction](docs/RECONSTRUCTION.md): how the sources were recovered.
- [Native port](docs/NATIVE-PORT.md) and [Win16 parity](docs/WIN16-PARITY.md):
  native boundaries and original behavior.
- [Changelog](CHANGELOG.md) and [versioning](docs/VERSIONING.md): intentional
  changes, release tags and generated build numbers. Product versions are
  separate from the save-file format version, 2.84.

Pushes to `main` build and check the Windows SDL client, package its runtime
DLLs and offline help, and publish the rolling prerelease with the Linux host.
Version tags build with that revision's workflow. Pull requests run Linux and
Windows SDL checks, host unit tests, and full client/host regressions through
turn 150. The tutorial workflow retains bounded-checkpoint diagnostics; it does
not establish full tutorial coverage. See [workflows](.github/workflows) for
manual runs and configuration.
