# Game test scaffolding

The scaffolding covers new-game generation, fixed-seed regression comparisons
against the checked-in native baseline. The SDL tutorial and UI workflow runners
live in `tests/compat`; retired native Win32 scripts live in `tests/reference`.

- [Regression guide](REGRESSION.md): scenarios `noai`, `oneai1`–`oneai6`,
  `smallai4`, and `smallai6`, with checkpoints at turns 0, 1, 10, 25, 50, 80,
  100, and 150.
- [SDL tutorial guide](../../docs/NATIVE-TUTORIAL-RUNNER.md): bounded normal-input
  scenarios, including premature Generate rejection.
- [Archived native tutorial](../reference/win32-tutorial/README.md): original
  AutoHotkey gestures, observer and historical qualification records.
- [Save CLI](../savecli/README.md): the standalone Go/Cobra save comparison and
  AI update commands.
- [Precision experiments](PRECISION.md): source-extracted extended-versus-double
  operand sweeps and whole race-calculator comparisons (`make precision`).

## Layout

```text
tests/scaffold/
  regression.py                  Checkpoint generation, comparison, and tracing
  regression_trace.c             Optional native trace wrappers
  fixtures/newgame/tiny/         Single-player game.def and humanoid.r1
  fixtures/regression/           Fixed-seed AI scenario definitions and report

tests/compat/                   SDL wrapper, product workflow and tutorial tests
tests/reference/win32-tutorial/ Retired native harness and gesture references

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

`make regression` builds the fixed-seed SDL release client and
`dist/stars-save`, deletes and restages `dist/scaffold/regression/native`,
runs every scenario in the native baseline through turn 150, and compares the
run with `fixtures/regression/native/`. Any difference exits nonzero; the
report is `dist/scaffold/regression/comparison.json`. `SCENARIOS="noai
smallai4"` and `THROUGH=10` limit the run, and `make regression-quick` is
smallai4 through turn 10. After a full run that moves the baseline on
purpose, `make regression-export` replaces the baseline with it.

The tutorial targets run the bounded SDL checkpoints registered by CTest.
The SDL runner guide describes exact coverage and retained results.

## New-game workflow checks

The SDL CTest suite runs `run_newgame.py` through `stars-compat-client-startup`.
It checks the actual title screen, Basic/Advanced creation and picker cancellation,
including Unicode output paths. See [gameplay qualification](../../docs/GAMEPLAY-QUALIFICATION.md).
The previous Wine-only `newgame.sh` is archived with the native tutorial harness.
