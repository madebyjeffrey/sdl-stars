# Binary64 arithmetic migration

Production uses binary64 `double` for the 178 former extended-precision casts
in 38 functions across 18 C files. There were no persistent `long double`
fields. Stored `float` values, integer narrowing, expression order, file record
sizes and file versions are unchanged. Existing saves remain readable, but
generating subsequent turns can produce different game results.

The reference implementation is commit
`3c67394973aacf6b0bf24d0a302f42b50a87bfa0`. The comparison harness extracts that
source and builds separate extended and double variants. See
[the precision guide](../tests/scaffold/PRECISION.md) for reproducible commands,
domain definitions, witnesses and exhaustive versus sampled coverage.

## Measured effects

- 9,382,918 expression inputs were evaluated. 37 of 122 expression groups had
  differences; 57 groups were tested over their complete configured grids.
  These counts describe expression evaluations, not unique game states.
- 140,665 whole race configurations produced 54 innate-habitability differences,
  each of one integer unit. The known witness changes from 1,470,286 to 1,470,287;
  both versions price it at 541 advantage points. All tested point balances and
  normalized race records match. The accumulated floating result differs in
  21,380 cases, by at most 1.4901161193847656e-8 (two ULPs).
- The actual Craig Stars Go calculator at commit
  `85b83c422f14529b61ec45716261c53d1ad515ce` matches the double variant for all
  140,665 race configurations, including accumulated habitability. Planet
  habitability matches both variants on 49,454,448 inputs: six habitat range
  triples, all eight immunity masks and every 0..100 planet coordinate triple.
- All 1,000,001 tested minefield growth inputs and all 30,001 tested integer
  distance inputs match. This does not prove every other floating calculation.
- Two independently built hosts, using fixed seeds and all nine regression
  scenarios through turn 150, produced 393 matching and 126 differing checkpoint
  files. Only `oneai5` and `smallai6` diverged, both from turn 1. The first
  differences are Cybertron AI scanner destination choices in `rtAiData`.
  `IdGetBestScannerDest` truncates `1200 * 0.3` to 359 with the extended
  intermediate and to 360 with double. Subsequent orders, RNG consumption and
  turns can consequently diverge. The native baseline is updated for these
  two scenarios only; frozen original-game checkpoints are untouched.

## Possible impacts outside the corpus

Expressions in AI selection, fleet distances and headings, fuel, bombing,
defenses, cargo, scanning/cloaking and report formatting can cross integer or
display rounding boundaries. The synthetic comparisons include one-unit
integer differences and small floating differences. A small local difference
can become a much larger gameplay difference when it changes a decision or RNG
sequence. Some large synthetic scanner-power operands yield large absolute
floating differences; the harness does not establish that all such operands
are reachable in a real game.

The race corpus is not every possible race. A one-unit innate difference near
a division-by-2000 boundary could change wizard pricing for an untested race.
The planet sweep exhausts coordinates for its six range triples, not every
possible habitat range. Regression success on the remaining seven scenarios
does not guarantee identical turns for other seeds, races or universes.

## Platform contract

CMake requires a 53-bit `double` significand, binary64 exponent range and
`FLT_EVAL_METHOD == 0`. Production disables fast-math and floating contraction
so an FMA cannot silently remove an operation's rounding boundary. Explicit
casts and stored float conversions remain intentional. Historical comparison
tests additionally require an x87-compatible extended type and run on x86.

macOS host presets now select the native architecture instead of requiring
Rosetta for extended arithmetic. SDL macOS presets retain their Intel dependency
environment. Linux x86-64 and MinGW x86-64 are tested here; native ARM and macOS
execution are not qualified by these results. Library implementations of
`pow`, `sqrt` and trigonometric functions may still vary across platforms.

## Production verification

The MinGW Debug build, all 55 Windows unit executables, 15 native host unit
executables and six differential harness tests pass. After exporting only the two changed native scenarios,
both MinGW Release and Linux host Release match all 519 checkpoint files
through turn 150 (55 and 69 unused-storage warnings respectively). The first
pre-migration differences in both builds are `oneai5` turn 1 `game.h2`, record
5: planets 107/122 exchange scanner target codes 0/3; and `smallai6` turn 1
`game.h6`, record 5: planets 1/3 and 108/123 exchange the same codes.

The initial Windows Debug tutorial attempt stopped at year 2403/page 15,
instruction 114 (QuikDrop popup menu): Stars exited and Wine logged
`free(): invalid next size (fast)`. This is a failed tutorial run, not a full
pass. A Release retry and native SDL qualification are recorded separately in
[the native tutorial guide](NATIVE-TUTORIAL-RUNNER.md).
