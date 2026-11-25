# Floating-point differential tests

The extended source is now pinned to pre-migration commit
`3c67394973aacf6b0bf24d0a302f42b50a87bfa0` through `precision_reference.py`.
The production game uses binary64 intermediates; these historical
comparisons remain available and require an x86 extended-precision oracle.
References to “current source” below describe the original experiments before
the migration. The immutable reference keeps their casts, operation order and
witnesses reproducible after production casts are replaced. Headers, catalogs and
generators are materialized from that same revision, including full source trees
for `precision-turns`; no current C++ headers or build files enter the C oracle.
`--generated-dir` is a deprecated compatibility argument and is ignored. Darwin
arm64 compiles x86 executables through Rosetta; the original format/control-state
check still rejects an unsuitable oracle. Other platforms require an x86 compiler.

`make precision` compares every explicit `long double` intermediate in the
production C files against the same expression with `double` intermediates.
The runner extracts current source, retains integer operations and explicit
result conversions, and compiles two separate functions for each expression.
It also extracts the real race calculator and planet-desirability functions
using the existing `race_precision.py`, and compares their complete results.
Production sources, saves, and regression baselines are never changed.

```sh
make precision
make precision PRECISION_ARGS='--sanitize --race-samples 100000'
make test-precision
```

The default report is `dist/scaffold/precision/report.json`, with a readable
table in `report.md`. Differences are successful experimental results: exit
zero means the experiment ran. `--fail-on-difference` exits 1 when any compared
output differs, after saving the reports. Compiler/probe failures exit nonzero
without writing a successful report. Reports include compiler flags, source
hashes, floating-point format/control state, operand domains, evaluated and
skipped counts, difference counts, maximum absolute/relative errors, mean
absolute error across evaluated cases, ULP distances, and first/largest
counterexamples with decimal and hexadecimal outputs and their inputs.
Integer results are compared exactly, with no tolerance. ULP counts apply only
to floating results; the absolute difference is the relevant integer measure.
Relative error divides by the absolute extended result; for a zero reference it
uses the absolute error. Both signed zeros compare equal.

## Coverage and exhaustive searches

Every cast is associated with an extracted expression and typed scalar
operands. The current inventory is 178 casts in 38 functions across 18 files.
Repeated expressions within one function share a test and retain every source
line in the report. Narrowing to a local destination type is also retained.
Unrecognized operands fail the runner, requiring an explicit domain/binding
when the source changes. `test_precision.py` checks extraction coverage, pins
the known AI/race counterexamples, exhausts the planet's 30001 root inputs,
and checks deterministic reports and the difference exit status.

Default operand domains include all five universe sizes, all defense counts
0..100 with the five defense abilities read from `data/parts/planetary.yaml`,
smart-bomb abilities read from `data/parts/bombs.yaml`, resource/energy/habitability settings,
warp values, signed coordinates/deltas, integer counts, and representative
floating-point grids with adjacent IEEE values. Probability grids also include
defense survival powers; distances include integer-rounding thresholds and
Euclidean distances from integer coordinates. The exact domain sizes,
descriptions, and hashes of the part-data inputs are saved.
These are stated bounds and stress grids, **not a claim that every combination
is reachable**. In particular, scanner powers, slopes, trigonometric values,
coordinates, and weights are independent operands in isolated tests. Large
scanner-power errors can describe synthetic values outside normal game state;
inspect the witness and domain before assigning a gameplay implication.

Cartesian products with at most 250000 inputs are exhausted. Larger products
use a Cartesian grid of representative/end values (capped at 250000 cases),
then 100000 deterministic samples from the declared operand grids. The seed
is fixed, and each expression gets its own stream. A zero difference count
means only the evaluated inputs matched. Samples do not exhaust the spaces
between grid values, and matching expression tests do not prove downstream
behavior remains identical when several replacements propagate together.

Zero divisors, nonfinite outputs, inverse packet-attenuation ratios above eight,
and outputs outside the safe `int32_t` conversion range are skipped and counted.
`--sanitize` instruments undefined arithmetic and float-to-integer overflow;
it must pass before interpreting a new domain. This is not an invalid-state
compatibility oracle. The declared integer grids fit the chosen scalar widths;
domain overrides must also do so to avoid narrowing/repeated input values.

Restrict functions, adjust domains, or raise the exhaustive limit:

```sh
python3 tests/scaffold/precision.py --function IdGetBestScannerDest --skip-race
python3 tests/scaffold/precision.py --function IncreaseAIMinefieldSizes \
  --domain mines=0:1000000 --exhaustive-limit 1000001 --skip-race
python3 tests/scaffold/precision.py --function CResourcesAtPlanet \
  --domain count=0:10000 --exhaustive-limit 20000000 --skip-race
```

`--exhaustive-limit 0` exhausts every declared Cartesian operand grid. Use it
with restricted functions/domains: products can be enormous. The runner rejects
products exceeding `uint64_t` rather than silently truncating enumeration.
`--all-float-probabilities` changes the binary32 probability domain to **every
nonnegative IEEE binary32 encoding from +0 to 1**, 1065353217 values. For example:

```sh
python3 tests/scaffold/precision.py --function DrawPlanetStats --skip-race \
  --all-float-probabilities --exhaustive-limit 0
```

That is a long run. It does not exhaust binary64 probabilities, signed zero,
or all game states. There is no tractable claim of exhaustive coverage for all
64-bit floating-point inputs and every combination of fleet/planet state.

## Whole race calculations

The race experiment compares the stored habitability accumulator, integer
innate result, integer `/2000` result, final advantage points, and normalized
`PLAYER` bytes. It checks that `rgplr[0]` is restored and pins the known witness:
extended innate 1470286, double innate 1470287, advantage points 541 in both.
Counterexamples contain all habitat endpoints/centers, growth, trait bits, and
all sixteen race attributes in the input order recorded in JSON.

The default suite includes the witness, all eight immunity masks with Total
Terraforming on/off and growth 1..20, **40344 one-axis interval cases** (1681
valid intervals on each axis, other axes using the witness or immunity, TT
both ways), and 10000 reproducible random races. Random races vary every
primary trait, all trait bits, bounded economy/research attributes, and growth.
`--odd-widths` uses all 4950 intervals of integer width 2..100, covering positive
odd-width habitats from files. Zero/one-width intervals and inconsistent
centers are outside this valid-input corpus because division/normalization
requires a separate invalid-input policy. `--no-axis-sweep` disables the axis
sweep for smoke checks; `--race-samples` tunes the random corpus.

Even-width habitats with immunity and TT have **9517173136 combinations**.
Full Cartesian searches can be split into disjoint shards:

```sh
python3 tests/scaffold/precision.py --function LInnateRaceHabitability \
  --no-axis-sweep --race-samples 0 --habitat-start 0 --habitat-count 100000
python3 tests/scaffold/precision.py --function LInnateRaceHabitability \
  --no-axis-sweep --race-samples 0 --habitat-start 100000 --habitat-count 100000 \
  --report dist/scaffold/precision/habitats-100000.json
```

The mixed-radix order is TT (fastest), gravity, temperature, radiation. Each
axis enumerates width then lower endpoint, followed by immunity. These Cartesian
shards fix non-habitat settings to the witness; even covering every shard would
not exhaust every economy/trait/growth combination. Per-shard results include
their exact start/count. See `docs/RACE-WIZARD-CALCULATIONS.md` for the broader
point-pricing and normalization cases required before a production migration.

## Oracle requirements and ordinary regression

### Craig-stars comparison

`craig_habitability.py` copies the actual Go planet habitability, habitability
integral, and race point methods from a local Craig-stars checkout into a
temporary standalone program. It also copies their habitat helpers and enum
definitions; a small adapter supplies race data. No Craig-stars files are
edited and no Go modules are downloaded. The sole method-body instrumentation
records the stored habitability accumulator before its return conversion.

```sh
python3 tests/scaffold/craig_habitability.py --craig ../craig-stars
python3 tests/scaffold/craig_habitability.py --craig ../craig-stars \
  --samples 100 --no-axis-sweep --skip-planets \
  --report dist/scaffold/precision/craig-smoke.json
```

The default race corpus is the same 140665 cases used by the larger C precision
run: witness, growth/immunity/TT combinations, 40344 axis-sweep cases, and
100000 seeded random races. It compares Go separately against extended C and
double-only C at the stored sum, innate integer, `/2000`, and final points.
The adapter maps lesser traits by **name**, since the projects' bit orders
differ, and maps research costs, primary traits, factory discount and starting
technology explicitly. Both point calculators start with 1650 points.

Planet comparisons exhaust all 101 cubed coordinate triples for six selected
range triples and all eight immunity masks: **49454448 planets**. The selected
races cover the witness, humanoid, full and narrow centered ranges, clipped
endpoints, and odd widths. This exhausts planet inputs for those races, not
all possible race habitats. `--skip-planets` disables that phase; `--odd-widths`
expands the integral corpus's interval catalog as in the C suite.

Reports are `dist/scaffold/precision/craig-habitability.json` and `.md`, with
the Craig-stars commit/source hashes, compiler versions, C oracle environment,
comparison counts, maximum errors, and first/largest race counterexamples.
Generic helper constraints are narrowed to integers in the temporary shell;
their bodies are unchanged. Game specs/database types unused by the extracted
methods are stubbed. This is a comparison of actual source calculations on
valid centered inputs, not an integration test of the Craig-stars server,
its input validation, or either project's complete race editor.

`make precision-turns` adds a whole-game test of propagated differences. It
builds two isolated native release hosts from source snapshots, replacing all
core `long double` tokens only in the candidate. Both builds use the same fixed
product-version metadata. It generates all nine regression scenarios through
turn 150 with identical seeds, then uses the existing save comparator to compare
their checkpoints. This includes real file records, RNG effects, and successive
turns; it excludes Windows drawing and is still a finite scenario corpus.

```sh
make precision-turns
make precision-turns PRECISION_TURN_ARGS='--through 10 --scenario smallai4 \
  --work dist/scaffold/precision/turns-smallai4'
```

The default work directory is `dist/scaffold/precision/turns`. Choose a fresh
`--work` for another run: this runner never deletes prior experiments. It keeps
both builds, all checkpoints, build/run logs, `comparison.json` with save-field
differences, and `summary.json` with the first differing checkpoint per scenario.
It rejects an unsupported floating-point oracle before building and reports
missing/failed runs as errors. `--fail-on-difference` makes a completed unequal
comparison exit 1. It never exports or regenerates the native/original baseline.

Run on an x86 native GCC-compatible compiler with binary64 `double`, x87
extended `long double` (64-bit significand, exponent limit 16384), nearest
rounding, `FLT_EVAL_METHOD == 0`, and FTZ/DAZ disabled. The runner rejects other
environments rather than comparing two double implementations. Storage padding
is reported separately. It disables fast math and contraction. `--cc` selects
the compiler and `--optimization O0|O2|O3` selects optimization; retain separate
reports when comparing these builds. On ARM, an independent software extended
oracle or an x86 execution environment is required.

Use `--generated-dir` when the race calculator headers are in another build
directory. `make precision` supplies the host preset's directory automatically.
The experiment does not replace `make regression`, unit tests, the tutorial,
or host regression. Those exercise production arithmetic and the existing
baseline; a production replacement needs its own behavior-change verification.
