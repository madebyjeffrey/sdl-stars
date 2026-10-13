# Resume Stars development on macOS and Windows

This is the handoff from the Linux `win32_adapter` branch after the PDF printing
and offline-help milestones. Start from a checkout containing **2afbf63** and
this guide. The source and Git history are portable; build directories are not.
These are setup and acceptance instructions, not claims of successful Mac or
real Windows execution. Those desktops are the next qualification environments.

## 1. Current state and scope

- `6afa88b`: Print Map starts one document, checks page/document errors, saves
  PDFs atomically and opens completed output in the platform viewer. Cancellation
  is quiet; viewer failure retains the PDF and reports its path.
- `2afbf63`: SDL WinHelp exports a self-contained HTML guide with all 414 topics,
  413 contexts, fonts, diagrams, navigation and popups. Initial context survives
  a Linux OpenFile portal export. No content server is needed.
- Linux qualification: 124/124 non-tutorial fatal sanitizer cases, 55/55 Windows
  unit/reference tests under Wine and 519 native regression comparisons with
  zero differences/errors, no skipped inputs and 87 existing warnings. Later
  test-only fixture preparation and error-edge cases passed focused reruns.
- Relocated CMake and installed Flatpak products both passed actual Print Map
  and document handoff. Both exported guides passed a full headless Firefox walk.
- API-body coverage is 178/178 (100%). It does **not** mean every platform,
  renderer, control behavior or product acceptance requirement is complete.
- The broader saved goal and tutorial expansion are paused. Prioritize product
  qualification. Keep tests sequential; do not run a test client alongside a
  manual client. Further Linux desktop feedback remains queued for later.
- Commit completed work locally, with one bug/area per commit. **Do not push.**
  Keep game rules, disk layouts and regression baselines unchanged unless an
  intentional behavior change is explicitly agreed and documented.

Read [AGENTS.md](../AGENTS.md), then [current milestone status](LINUX-CLIENT-GOAL.md)
and [desktop qualification](DESKTOP-QUALIFICATION.md). Read `AGENTS-private.md`
only if it exists locally; do not transfer or publish private reference material.

| Desktop | Product target | Build path |
| --- | --- | --- |
| macOS | `stars-sdl`, native Cocoa picker, PDF/browser handoff | `macos-sdl-debug` / `macos-sdl-release`, arm64 |
| Windows | `stars.exe`, native Win32 UI/dialogs/printer | `mingw-debug` / `mingw-release`, MinGW-w64 |
| Linux | `stars-sdl`, portal picker, PDF/browser handoff | Existing SDL and Flatpak presets |

The SDL compatibility target currently accepts only Linux and Darwin. Do not
select `sdl-debug` on Windows or assume its new browser/PDF backend replaces
Windows' existing WinHelp/default-printer implementations.

## 2. Transfer without pushing

A self-contained bundle can carry the committed branch and tags on a USB drive,
shared folder or other file transfer. From the Linux repository, after committing
this guide:

```sh
mkdir -p dist/transfer
git bundle create dist/transfer/stars-desktop-handoff.bundle win32_adapter --tags
git bundle verify dist/transfer/stars-desktop-handoff.bundle
```

Copy that bundle to each desktop. It contains the development branch and tags, without unrelated internal Git
checkpoint refs. It excludes ignored build output, local settings, games, dependency caches or untracked private files.
Copy any games you want to test **separately**, and use disposable copies.
The existing `dist/` logs and screenshot evidence also stay on Linux unless you
copy them separately; the qualification summaries above are committed.

For a fresh Mac checkout:

```sh
mkdir -p "$HOME/Projects"
cd "$HOME/Projects"
git clone --branch win32_adapter /path/to/stars-desktop-handoff.bundle stars
cd stars
git switch -c codex/macos-qualification
```

For a fresh Windows checkout, run in the MSYS2 MINGW64 shell described below:

```sh
mkdir -p /c/src
cd /c/src
git clone --branch win32_adapter /c/path/to/stars-desktop-handoff.bundle stars
cd stars
git switch -c codex/windows-qualification
```

Replace the bundle path. Keep the checkout/game paths short and without spaces;
the legacy game command parser cannot quote filenames containing spaces.
The cloned `origin` points at the bundle, not a hosting service.

For an existing checkout, preserve its local changes first, then fetch into a
separate ref and create a new qualification branch; do not reset an existing
branch:

```sh
git bundle verify /path/to/stars-desktop-handoff.bundle
git fetch /path/to/stars-desktop-handoff.bundle win32_adapter:refs/remotes/handoff/win32_adapter
git switch -c codex/macos-qualification refs/remotes/handoff/win32_adapter
```

Use `codex/windows-qualification` on Windows. In either checkout verify:

```sh
git status --short
git merge-base --is-ancestor 2afbf63 HEAD
git log -3 --oneline
```

The ancestry command must succeed. Do not copy Linux CMake caches or `.idea`
profiles to the new machine. Reconfigure from source in the platform presets.

## 3. macOS setup

Install Xcode Command Line Tools if absent:

```sh
xcode-select --install
```

The SDL Mac presets build **arm64** for Apple silicon, using native Homebrew
dependencies under `/opt/homebrew`. Host presets use the native architecture
and binary64 arithmetic; see [DOUBLE-PRECISION.md](DOUBLE-PRECISION.md).
If Homebrew is absent, follow
[Homebrew's installation instructions](https://docs.brew.sh/Installation)
from a native Apple silicon shell.

```sh
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
/opt/homebrew/bin/brew install cmake ninja pkgconf freetype fontconfig cairo libwmf python
export PKG_CONFIG_LIBDIR="/opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig"
uname -m
/opt/homebrew/bin/brew --prefix
pkg-config --modversion freetype2 fontconfig cairo cairo-pdf
command -v wmf2svg
```

Expected architecture/prefix: `arm64` and `/opt/homebrew`. Do not mix ARM and Intel
library paths. [libwmf](https://formulae.brew.sh/formula/libwmf) supplies `wmf2svg`
for the original guide's WMF diagrams. SDL can be fetched by CMake from the pinned,
checksum-verified 3.4.18 archive; installing SDL separately is optional.

Use a repository-local Python environment under ignored `dist/`:

```sh
mkdir -p dist
/opt/homebrew/bin/python3 -m venv dist/dev-venv
dist/dev-venv/bin/python -m pip install pyyaml
cmake --preset macos-sdl-debug -DPython3_EXECUTABLE="$PWD/dist/dev-venv/bin/python"
cmake --build --preset macos-sdl-debug --parallel 4
./dist/macos-sdl-debug/bin/stars-sdl
```

The preset disables tests for the first product build. It is already wired to
AppKit native file dialogs. Start by fixing actual configure/compiler/linker
failures on this Mac; the Linux qualification did not prove a successful Mac
build. Preserve the architecture and file-format constraints while fixing them.

After a successful product build, optionally enable the Mac-supported host,
ABI and direct SDL smoke tests:

```sh
cmake --preset macos-sdl-debug -DPython3_EXECUTABLE="$PWD/dist/dev-venv/bin/python" -DSTARS_BUILD_TESTS=ON
cmake --build --preset macos-sdl-debug --parallel 4
./dist/macos-sdl-debug/bin/stars-compat-abi
./dist/macos-sdl-debug/bin/stars-compat-smoke --self-test
ctest --test-dir dist/macos-sdl-debug --output-on-failure -j 1
```

Linux-only private DBus, X11 and Weston harnesses are
excluded on Mac. Do not try to transplant `sdl-sanitize`: its sanitizer option
currently requires Linux. The standalone help viewer is:

```sh
./dist/macos-sdl-debug/bin/stars-compat-help-viewer --contents
```

For host regression, install Go (the save CLI requires Go 1.25 or newer):

```sh
/opt/homebrew/bin/brew install go
go version
make regression-host HOST_PRESET=macos-host-release PYTHON="$PWD/dist/dev-venv/bin/python"
```

Run this separately from manual UI testing. Compare the baseline; never export
it to hide a platform mismatch. Record the first unexpected diff. See
[native regression](../tests/scaffold/REGRESSION.md) for interpretation.

## 4. Windows setup

Install [MSYS2](https://www.msys2.org/), open its **MINGW64** shell, and update it:

```sh
pacman -Syu
```

If the update asks you to close the terminal, do so, reopen MINGW64 and rerun
`pacman -Syu`. Then install the matching native Windows tools:

```sh
pacman -S --needed git mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils \
  mingw-w64-x86_64-gdb mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-python mingw-w64-x86_64-python-yaml
command -v x86_64-w64-mingw32-gcc
command -v x86_64-w64-mingw32-windres
command -v cmake
python -c "import yaml; print(yaml.__version__)"
```

Use the MinGW CMake/Ninja/Python tools from `/mingw64/bin`, not MSYS POSIX tools
or an MSVC toolchain. This matches the repository's prefixed compiler names and
GNU extensions. See [MSYS2 CMake guidance](https://www.msys2.org/docs/cmake/) and
the [PyYAML package](https://packages.msys2.org/packages/mingw-w64-x86_64-python-yaml).
The Windows product needs neither SDL/Cairo nor Linux desktop services.

From the checkout root in that shell:

```sh
cmake --preset mingw-debug -DPython3_EXECUTABLE="$(cygpath -m /mingw64/bin/python.exe)"
cmake --build --preset mingw-debug --parallel 4
./dist/mingw-debug/bin/stars.exe
```

Close the manual client before running native unit/reference tests:

```sh
ctest --test-dir dist/mingw-debug --output-on-failure --timeout 300 -j 1
```

CTest can execute these Windows programs directly; Wine is not required on
Windows. A configure message that `run_in_wine` is unavailable is expected.
Some reference tests create native windows, so run them when you are not
interacting with another Stars instance. For Release:

```sh
cmake --preset mingw-release -DPython3_EXECUTABLE="$(cygpath -m /mingw64/bin/python.exe)"
cmake --build --preset mingw-release --parallel 4
```

Do **not** use `make regression`, `make regression-host` or the Python regression
runner as native-Windows acceptance commands yet: the current runner treats
all `.exe` files as Wine programs and constructs Wine `Z:` paths. Its native
Windows launch/path adapter is separate work. Keep full regression qualification
on Linux, and use the native host path above on Mac. Tutorial automation remains
paused; do not install or run AutoHotkey as part of this handoff.

## 5. CLion on each desktop

Open the repository root and load CMake presets. Enable the imported profile
under Settings/Preferences → Build, Execution, Deployment → CMake; disabled
profiles do not appear in the run selector. See [CLion preset support](https://www.jetbrains.com/help/clion/cmake-presets.html).

On Mac use the Xcode/Clang toolchain, `macos-sdl-debug`, the Apple silicon library paths
above and `-DPython3_EXECUTABLE=<absolute checkout>/dist/dev-venv/bin/python`.
Choose `stars-sdl`, with the checkout root as working directory. The preset
requests arm64; CLion's shell/tool environment must find dependencies under `/opt/homebrew`.
On Windows use a MinGW toolchain rooted at `C:\msys64\mingw64`, the
`mingw-debug` preset and the matching Python executable. Choose `stars` (the
output is `stars.exe`). Verify the configure log selects MinGW before building.
Use GDB on Windows and the supported macOS debugger on Mac. If an imported
profile is read-only and you need to edit its Python path or toolchain, duplicate
it as a local CLion profile and keep its architecture/options; use a separate
build directory for that copy. Do not commit machine-specific `.idea` state.

Do not inherit Linux-only `SDL_VIDEODRIVER`, `LD_LIBRARY_PATH`, Mesa-prefix,
`DISPLAY` or `WAYLAND_DISPLAY` settings. Start with the normal native renderer.
For a Mac rendering failure, retry with `STARS_SDL_RENDERER=software` in a
separate run configuration and record both outcomes. Always close one run
before starting another.

## 6. Desktop acceptance, in order

Use disposable copies of game/race files. Record OS/version, CPU architecture,
compiler and dependency versions, commit, renderer, display scale and monitors.

1. Build and launch. Intro must be a decorated desktop window with its image
   painted promptly. Continue a game: toolbar, scanner, expanded left panels
   and messages should appear immediately. Close Technology Browser with both
   Close and the caption X. Quit normally.
2. Native Open/Save: open a game and race, save/reload a race, cancel selections,
   and test Unicode names/paths. Confirm the title owner stays visible and the
   native picker retains modality. Use the picker for paths containing spaces;
   avoid passing those paths through the legacy command line.
3. Controls: navigate the race wizard through habitat bars and Back/Next; confirm
   one persistent desktop window. Check menus, toolbar split-arrow placement and
   pressed appearance, tooltips, scanner-range combo hover versus committed
   selection, keyboard focus, live proportional scrollbars and pane scrolling.
4. Mac PDF: File → Print Map, choose a destination, and confirm the saved map
   opens in the default PDF viewer. Try a tiled map, cancel the destination, and
   use a Unicode filename. Physical printing remains the viewer's responsibility.
   Windows retains its native default-printer path; if testing it, deliberately
   select a PDF virtual printer as the system default first and record results.
5. Mac help: Contents, contextual Research help, diagram hotspots, images/fonts,
   definition popup, topic-index filter and Back, with no content server and
   while offline. Close/reopen from the installed layout if you qualify one.
   Windows still uses native legacy WinHelp; modern Windows availability is
   unqualified. Record missing-viewer/help failures as a separate Windows product
   gap; the Linux/macOS self-contained adapter is not installed in that build.
6. Display/input: focus/capture, minimization/restore, resize, Retina/Windows scale
   settings, move between differently scaled monitors, and physical GPU versus
   software presentation on Mac. Record actual hardware/backend; a headless or
   software pass does not qualify the hardware driver.

Store results in a new Markdown file under `docs/`, with passed/failed/unverified
checks and reproducible steps. Keep temporary logs, screenshots and dumps under
`dist/desktop-acceptance/`. Include the first useful debugger stack for crashes.
Update [desktop qualification](DESKTOP-QUALIFICATION.md) and
[the milestone list](LINUX-CLIENT-GOAL.md) when evidence changes; do not relabel
Linux results as Mac or real Windows results.

The private Mesa leak repair in [MESA-GRAPHICS-LEAK.md](MESA-GRAPHICS-LEAK.md)
qualifies a specific Linux software-driver build. It is not a Mac/Windows driver
patch and is not included in Flatpak runtime drivers. Do not copy that prefix
or disable leak checking to declare a different graphics stack qualified.

## 7. First request to the next development session

Paste this, adapting the platform name:

> Read AGENTS.md and docs/DESKTOP-HANDOFF.md. Verify this checkout contains
> 2afbf63 and the handoff guide, and inspect local changes before editing. Resume
> product qualification on this macOS/Windows desktop: build with its documented
> preset, fix platform build/runtime blockers in focused commits, then execute
> the desktop acceptance checklist sequentially. Preserve game/file compatibility
> and the existing native regression baseline. Record platform evidence and
> remaining gaps, and update the milestone status. Tutorial expansion and the
> broader saved goal stay paused. Commit completed areas locally; do not push.

No global package installs, graphics-driver patches, published releases or
repository resets have been performed by this documentation handoff.
