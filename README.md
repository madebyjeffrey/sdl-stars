# Stars!

![meme](docs/images/windows10stars.jpg)

Stars! 4X game rebuilt from decompiled C sources using a custom [stars-asm](https://github.com/sirgwain/stars-asm/tree/main) win16 disassembler the decompiler.

The original 2.6jrc3 stars.exe included ~1MB of debug symbols with function names, variable names, symbol definitions, and even line numbers. The stars-asm project used that information to rebuild the Stars! source to be as close to the original as possible.

## Branches

- `2.6j` (tag `2.6jrc3`): the faithful reconstruction, original bugs
  included. It matches the original game turn for turn in the fixed-seed
  regression.
- `2.8` (tag `v2.8.0`): the released 2.8 line, bug fixes only. It fixes
  original bugs and replaces the Win16 shims with native code. It reads
  2.6j files and writes the same formats, marked as version 2.84 so that
  2.6j doesn't load them.
- `main`: the 2.9 line, splitting the game into core, UI and host builds so
  the host (`stars-host`) runs on Linux and macOS.

## Documentation

- [Reconstruction](docs/RECONSTRUCTION.md): how the source was rebuilt from the
  decompiler's output, and the conventions it follows.
- [Native port](docs/NATIVE-PORT.md) and [Win16 parity](docs/WIN16-PARITY.md):
  what the Win32/Win64 build changes, and the original behavior it reproduces.
- [Known bugs](docs/KNOWN-BUGS.md): the original release bug list, mapped to
  source.
- [Roadmap](docs/ROADMAP.md): the 2.9 split and the bug fixes still open.
- [Changelog](CHANGELOG.md) and [versioning](docs/VERSIONING.md): what 2.8
  changes from 2.6j, and how versions and build numbers are assigned.

## Source layout

- `src/core/`: game rules, AI, turn generation, save layouts and the `ui.h` boundary.
- `src/ui/`: shared SDL-backed desktop UI, including its original tutorial code.
- `src/host/`: command-line entry point and host UI adapter.
- `src/platform/`: native files, clock, formatting and shared encoding.
- `compat/win32/`: SDL-backed Win32 API wrapper and isolated platform bridges.

Original source filenames and debug-symbol names are preserved. `data/`, `text/`
and `res/` contain authored inputs; generated sources, builds and runtime installs
belong under `dist/`. Tests, build modules, packaging and tools remain separate.

## Build

The desktop client uses SDL3 through the project's Win32 wrapper on Windows,
Linux and macOS. There is no separate native Win32 desktop build. The command-line
host remains independent of SDL.

On Windows, use MSYS2 UCRT64; see [Windows SDL3 build and packaging](docs/WINDOWS-SDL3.md):

```sh
cmake --preset windows-sdl-debug
cmake --build --preset windows-sdl-debug
ctest --preset windows-sdl-headless
```

On Linux, use `sdl-debug` / `sdl-release`; on macOS, use `macos-sdl-debug` /
`macos-sdl-release`. See [SDL development](docs/SDL3-DEVELOPMENT.md) for dependencies
and platform qualification. Windows `mingw-debug` / `mingw-release` are legacy
aliases for the SDL builds, using the native UCRT64 compiler.

The executable is `<build>/bin/stars-sdl` (`stars-sdl.exe` on Windows). CMake builds
text and data tables from `text/*.txt` and `data/**/*.yaml` and generates the
wrapper's resources from `res/`. Open/save uses native platform file pickers;
`STARS_FILE_DIALOG_BACKEND=compat` selects the deterministic SDL picker for tests.

Set `STARS_BUILD_SDL_CLIENT=OFF` for a host-only configuration. Independent Windows
SDK contract tests are optional (`STARS_BUILD_WIN32_REFERENCE_TESTS=ON`); they do
not build a second desktop client.

### stars-host

`stars-host` is the game's host without its windows: it creates universes,
generates turns and writes the dumps from the same command line as
`stars.exe` (`-a game.def`, `-g[n] game.hst`, `-v game.hst`, `-b`, `-t`,
`-s<seed>`, `-dm`/`-dp`/`-df game.mN`). `--ini stars.ini` reads the
`stars.ini` settings a host uses (see below). It builds
with the native compiler on Linux and macOS, with only CMake and Ninja:

```sh
make host               # or: cmake --preset host-release && cmake --build --preset host-release
dist/host-release/bin/stars-host -g1 /path/to/game.hst
```

Turn generation uses binary64 `double`. On Apple silicon, `make host` builds
the native `macos-host-release` preset (`dist/macos-host-release/bin/stars-host`).
The migration intentionally changes some host results; see
[DOUBLE-PRECISION.md](docs/DOUBLE-PRECISION.md). ARM execution remains unqualified. `stars-host --version`
prints the version, and `make test-host` runs the unit tests natively.

Where `stars-host` differs from `stars.exe`:

- It reads `stars.ini` only when given `--ini`, and then only the settings
  a host uses: `[Files] Logging` and `[Misc] DefaultPassword`,
  `NewReports` (per-player `-d` files such as `game.p1`), `NoHostNames`
  and `Backups`. Without it they keep their defaults.
- It doesn't wait for turns (`-w`).
- On Linux and macOS it can't tell that another program has a game file
  open, as Windows file sharing does.
- Questions get the cautious answer (No, Cancel) and messages go to stderr.

## GitHub Actions

Every push to `main` builds the Windows SDL client, runs its headless checks,
and packages the runtime DLLs and offline help in `stars-sdl-windows-x64.zip`.
Extract the archive and run `bin/stars-sdl.exe`. The rolling
[`latest` prerelease](https://github.com/sirgwain/stars/releases/tag/latest) also
includes `stars-host-linux-x64.tar.gz`, built and unit-tested on Linux.
Version tags build their tagged source with that revision's workflow.

Pull requests run SDL unit/wrapper tests on Linux and Windows, host unit tests,
and full SDL-client and host regressions through turn 150 against the unchanged
native baseline. The tutorial workflow runs bounded SDL checkpoints and retains
diagnostics; full 80-page SDL tutorial coverage is still pending.

All workflows can also be run manually; the release workflow accepts `main`
or a tag. Publishing uses the built-in `GITHUB_TOKEN` with `contents: write`;
the tutorial uses read-only permissions.

## Regression save tooling

The standalone go module in `tests/savecli/` supplies the save
comparison and AI update commands used by checkpoint testing:

```sh
make regression        # every baseline scenario through turn 150
make regression-quick  # smallai4 through turn 10
make regression-export # after a full run, replace the baseline
make regression-host   # the same run through stars-host, without Wine
```

`make regression` builds the fixed-seed release SDL client and
`dist/stars-save`, runs a fresh `dist/scaffold/regression/native`, and
compares it with the checked-in baseline in
`tests/scaffold/fixtures/regression/native/`; any difference fails.
`SCENARIOS="noai smallai4"` and `THROUGH=10` limit a run. See
[the save CLI README](tests/savecli/README.md) and
[regression instructions](tests/scaffold/REGRESSION.md).
