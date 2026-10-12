# Game test scaffolding

The scaffolding covers new-game generation, fixed-seed regression comparisons
against the checked-in native baseline, and the tutorial UI under Wine.

- [Regression guide](REGRESSION.md): scenarios `noai`, `oneai1`–`oneai6`,
  `smallai4`, and `smallai6`, with checkpoints at turns 0, 1, 10, 25, 50, 80,
  100, and 150.
- [Tutorial guide](tutorial/README.md): the full walkthrough and premature
  Generate rejection, driven by AutoHotkey v2.0.28.
- [Save CLI](../savecli/README.md): the standalone Go/Cobra save comparison and
  AI update commands.
- [Precision experiments](PRECISION.md): source-extracted extended-versus-double
  operand sweeps and whole race-calculator comparisons (`make precision`).

## Layout

```text
tests/scaffold/
  newgame.sh                     New-game smoke test runner
  regression.py                  Checkpoint generation, comparison, and tracing
  regression_trace.c             Optional native trace wrappers
  fixtures/newgame/tiny/         Single-player game.def and humanoid.r1
  fixtures/regression/           Fixed-seed AI scenario definitions and report
  tutorial/                     AutoHotkey runner, scripts, and observer

tests/savecli/                  Standalone save CLI module

dist/                          Generated builds, tools, and run artifacts (ignored)
  stars-save                    Save CLI executable
  scaffold/newgame/run/         Recreated by the default smoke-test run
  scaffold/tutorial/            Retained tutorial results by timestamp
```

Keep fixture inputs in `fixtures/`. The Makefile's regression runs in
`dist/scaffold/regression/native`. The original game's checkpoints in
`fixtures/regression/original/` are frozen: the DOSBox setup that made them
is no longer kept (see [REGRESSION.md](REGRESSION.md)).

## Regression and tutorial commands

Run from the repository root:

```sh
make compile
make regression
make regression-quick
make tutorial
make tutorial-reject
```

`make regression` builds the fixed-seed release `stars.exe` and
`dist/stars-save`, deletes and restages `dist/scaffold/regression/native`,
runs every scenario in the native baseline through turn 150, and compares the
run with `fixtures/regression/native/`. Any difference exits nonzero; the
report is `dist/scaffold/regression/comparison.json`. `SCENARIOS="noai
smallai4"` and `THROUGH=10` limit the run, and `make regression-quick` is
smallai4 through turn 10. After a full run that moves the baseline on
purpose, `make regression-export` replaces the baseline with it.

The tutorial targets build with `STARS_TEST_TUTORIAL=ON` and use an isolated Wine
prefix.
See the tutorial guide for retained results, and runtime requirements.

## New-game smoke test

Build the executable, then invoke the script directly:

```sh
make compile
tests/scaffold/newgame.sh
```

There is no `make newgame` target. The script requires Bash, Perl, and Wine on
`PATH`. It uses Wine's default prefix (or `WINEPREFIX` if set). 

To select a fixture, executable, timeout, or output directory:

```sh
tests/scaffold/newgame.sh --exe dist/mingw-debug/bin/stars.exe \
  --timeout 60 --work dist/scaffold/newgame/custom \
  tests/scaffold/fixtures/newgame/tiny/game.def
```

The work directory is deleted and recreated on every run. Defaults resolve from
the repository location; explicit relative paths resolve from the current working
directory. Use a dedicated scratch directory.

Each fixture has a `game.def` and its race files. Race paths are relative to the
fixture directory, and output paths are relative to the run directory. Use
forward slashes and relative paths; the runner converts separators for Stars!.

The smoke test passes when the executable exits successfully within the timeout
and writes the `.xy` universe and `.hst` host files named by the definition.
It retains staged inputs, generated saves, and `wine.log` for diagnosis. An error
dialog can block until the timeout. This smoke test checks file generation;
content comparison and turn processing belong to the regression harness.
