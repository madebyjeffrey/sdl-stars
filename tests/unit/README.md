# Unit tests

Each `test_<name>.cpp` is an Acutest program linked with the game core and SDL
wrapper UI. `make test-unit` builds the platform SDL preset and runs the game
unit programs; `CLIENT_PRESET` can override the preset. Use CTest's `-R` filter
for a subset. Executables are in `<build>/tests-client/sdl_test_<name>`.

Host presets instead link the core and `hostui.cpp`; `make test-host` runs these
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
a scoped typed `ALERTIO` binding; host tests keep `hostui.cpp`. Directory
cleanup uses the Windows CRT on Windows and POSIX enumeration elsewhere.
The macOS Debug preset now enables these tests by default.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L client --output-on-failure
```

The `client` label also runs the real startup/race-wizard input tests on dummy
and private X11/Openbox displays. See [client testing](../../docs/WIN32-CLIENT.md).

Game and wrapper harnesses compile as C++20 from `.cpp` sources.
Shared `.inc` vectors also compile independently through Windows SDK C entries.
Allocation overflow, missing files and short reads exercise typed failure
handling and scoped cleanup. Independent frozen-layout and real Windows SDK
reference programs remain C. The separate `win32_exception_callbacks` test
injects failures through wrapper dispatchers and checks modal cleanup before
C++ delivery.

## Two kinds of test

- **Function tests** call one game function with crafted inputs, such as
  `FIntersectCircleLine` in `test_utilgen.cpp`.
- **Turn tests** build a game and generate turns with the helpers in
  `stars_test.h`: `FStarsTestInit`, `FStarsTestDir` (a fresh
  `work/<name>` directory), `FStarsTestNewGame` (a tiny universe with the
  test race and any AI players), `FStarsTestLoadHost` and
  `FStarsTestGenerate`. Between loading the host and generating, a test can
  change the loaded game (`LpplStarsTestHomeworld`, `LpflStarsTestAddFleet`,
  or directly) and save it with `FStarsTestSaveHost`. AI players give their
  orders while one turn generates and the next turn carries them out, so AI
  behavior needs two generations. `test_turn.cpp` shows the plumbing and
  `test_ai4.cpp` a full example.

## Golden files

`test_report.cpp` checks the universe, planet and fleet dumps against
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
instead of shown (Yes/No boxes answer Yes). SDL test fixtures bind `ALERTIO`
to `StarsTest_AlertSz`; host builds supply `IdAlertBox`. `test_native_ports.cpp`
binds the typed record/file services.

## Typed effects and linked witnesses

Tests link the same production libraries as the application. A module's typed
provider defaults to its original functions; its scope owns a provider snapshot,
restores the enclosing binding on every exit and keeps bindings thread-local.
An observer delegates directly to the original function, avoiding recursion
through its own override. For example, RNG observers bind `RANDOMIO` and call
`Random` to obtain the real draw before recording it.

Bind only the effects consumed by the operation under test. Keep scalar locals
at the top and construct the scope before the first affected operation. Worker
threads that need a fixture explicitly capture its provider and bind their own
scope. The message ownership tests cover this handoff, allocation failure and
rejection/teardown receipts. There is no shared mock registry, implementation
renaming, compile-definition routing or private production library clone.

The [linked witness inventory](../../docs/CPP-C05-WITNESSES.md) accounts for all
27 former extraction programs and their unchanged independent expectations,
domains and exclusions. `stars-linked-witnesses` builds their normal targets;
`ctest -L witness` runs the 26 new tests, and `win32_profile_save` covers the
remaining save program. Their original Python commands remain supported.

The compilation manifest and its negative policy checks reject symbol
substitution, state doubles and private production clones. See
[the implementation ledger](../../docs/CPP-C-IMPLEMENTATION.md) for the current
platform, regression, sanitizer, native-leak and tutorial qualification. The
full 80-page SDL tutorial remains unfinished.
