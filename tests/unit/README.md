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
`--wrap=AlertSz`; host tests keep `hostui.c`. OS-specific directory enumeration
and `getcwd` retain their Linux implementations.

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
instead of shown (Yes/No boxes answer Yes): on Windows every test links with
`--wrap=AlertSz`, and natively `stars_test.c` supplies `IdAlertBox`. `test_native_ports.c` also wraps file I/O (see
`CMakeLists.txt`); give other tests that need wraps the same treatment.
