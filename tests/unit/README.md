# Unit tests

Each `test_<name>.c` is an Acutest program linked with the game core and SDL
wrapper UI. `make test-unit` builds the platform SDL preset and runs the game
unit programs; `CLIENT_PRESET` can override the preset. Use CTest's `-R` filter
for a subset. Executables are in `<build>/tests-client/sdl_test_<name>`.

Host presets instead link the core and `hostui.c`; `make test-host` runs these
without SDL. UI cases select `STARS_UI_SDL`, independently of the operating system.
Host builds omit the native-port and native-file-UI programs that need the client.

On Windows SDL, Acutest's SDK process/timer implementation is disabled because
its system API types conflict with wrapper types. `run_isolated.py` enumerates
cases and launches each in a fresh process, preserving the same isolation as
Acutest on Linux and host-only Windows builds.

Acutest runs every test in its own process, so each test starts from the
game's initial globals. Client tests run in `<build>/tests-client` (host tests in `<build>/tests`), where CMake copies the
fixture race to `data/humanoid.r1`.

## SDL client tests

The SDL presets also build `stars-sdl-unit-tests`: all game unit-test programs
linked with the shared UI and compatibility runtime, including Battle Plans,
race-wizard choices, habitat painting/immunity redraw and native-port cases.
`test_memory` checks aligned mixed allocations, free-list reuse/coalescing,
reset, realloc growth, dedicated heap growth and the 16-bit capacity guard.
They use the dummy backend, a separate `tests-client` fixture directory and
compile-time `AlertSz` substitution; host tests keep `hostui.c`. Directory
cleanup uses the Windows CRT on Windows and POSIX enumeration elsewhere.
The macOS Debug preset now enables these tests by default.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L client --output-on-failure
```

The `client` label also runs the real startup/race-wizard input tests on dummy
and private X11/Openbox displays. See [client testing](../../docs/WIN32-CLIENT.md).

## Two kinds of test

- **Function tests** call one game function with crafted inputs, such as
  `FIntersectCircleLine` in `test_utilgen.c`.
- **Turn tests** build a game and generate turns with the helpers in
  `stars_test.h`: `FStarsTestInit`, `FStarsTestDir` (a fresh
  `work/<name>` directory), `FStarsTestNewGame` (a tiny universe with the
  test race and any AI players), `FStarsTestLoadHost` and
  `FStarsTestGenerate`. Between loading the host and generating, a test can
  change the loaded game (`LpplStarsTestHomeworld`, `LpflStarsTestAddFleet`,
  or directly) and save it with `FStarsTestSaveHost`. AI players give their
  orders while one turn generates and the next turn carries them out, so AI
  behavior needs two generations. `test_turn.c` shows the plumbing and
  `test_ai4.c` a full example.

## Golden files

`test_report.c` checks the universe, planet and fleet dumps against
`tests/unit/golden`, so the Windows build and `stars-host` keep writing
the same files. If a change to the dumps is intended, regenerate them from
the Windows build and commit them with the change:

```sh
cd dist/mingw-debug/tests && STARS_UPDATE_GOLDEN=1 wine ./test_report.exe
```

## Bug fixes

A bug fix in [docs/ROADMAP.md](../../docs/ROADMAP.md) step 5 adds a test that
fails on the code before the fix. Name the test after the behavior it
checks and put it in the file for the source file that holds the fix.
The game's message boxes are recorded in `cStarsTestAlert`/`szStarsTestAlert`
instead of shown (Yes/No boxes answer Yes). SDL test builds redirect `AlertSz`
to `StarsTest_AlertSz`; host builds supply `IdAlertBox`. `test_native_ports.c`
also substitutes record/file operations.

## Portable test adapters

`cmake/test-redirect.cmake` registers substitutions with
`stars_test_redirect(target Function ...)`. It builds private library copies,
using compile definitions to route calls to `StarsTest_Function`. Definitions
of game/wrapper functions marked `STARS_TEST_IMPL(Function)` retain their real
implementation as `StarsTestReal_Function` in those copies. A test adapter can
call that implementation; adapters for external SDL functions call SDL directly.
Production libraries retain the original function names and code. No special
linker, weak symbols, dynamic interposition or source rewriting is required.
Private copies share completed platform settings and generated resources;
identical hook sets reuse the same copies.

When adding an internal adapter, add its default definition in
`src/platform/test_impl.h` and its name to the implementation-hook list in the
CMake helper. Give its test function the exact original signature. Register all
substitutions before the directory finishes configuring. The mechanism also
routes calls within a translation unit, unlike the old linker interception.

`test_redirect_contract` builds a small independent C project that verifies
production isolation, real delegation, multiple hook sets and late build
settings. It can also be run directly with a chosen CMake generator/compiler:

```sh
python3 tests/cmake/test_redirect.py
```

The adapter mechanism uses standard C preprocessing and ordinary C symbols,
including for MSVC. This does not complete the repository's broader MSVC port:
the product currently selects MinGW on Windows and retains other toolchain
flags/dependencies that still need qualification. MSVC execution has not been
verified on the Mac.

On October 12, 2026, Apple-silicon macOS passed all 28 registered desktop/build
checks, including all 17 game-unit programs and the private adapter contract.
All 15 host-unit programs plus the adapter contract passed. Desktop and host
full regressions each matched 519 files through turn 150, with 67 accepted
unused-storage warnings, zero differences/errors and zero skipped inputs.
A traced first turn matched the host save baseline, and the native Retina
renderer check passed using the portable adapter. The Mac tutorial target
still registers no tests; MSVC and Linux/Windows desktop execution were not
repeated in this environment.
