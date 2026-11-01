# S03: Player labels and M04 race titles

Status: Observed helper behavior, with explicit native input domains.
Reviewed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
This technical companion specializes [S03-US03](S03-static-game-data-and-text.md#s03-us03),
[S03-US04](S03-static-game-data-and-text.md#s03-us04),
[M04-US01](M04-race-design.md#m04-us01),
[M04-US03](M04-race-design.md#m04-us03) and
[S04-US03](S04-platform-services.md#s04-us03). Stories remain at those entries.

Canonical definitions: [player labels](dafny/S03-player-labels.dfy),
[integer-format subset](dafny/S04-integer-format.dfy) and
[M04 producer bindings](dafny/M04-text-producers.dfy). Evidence comes from
[util.c](../src/core/util.c) `PszPlayerName`,
[utilgen.c](../src/core/utilgen.c) `CchGetString`,
[strings.c](../src/core/strings.c) `PszGetCompressedString`,
[raceui.c](../src/ui/raceui.c) `SetRCWTitle` and the preset command,
[native.c](../src/platform/native.c) `CchSprintf`,
[SDL services](../compat/win32/src/services.c) `wsprintfA`, and the generated
[text source](../text/strings.txt). No game behavior or text is changed.

## Caller workflow and owned rules

The consuming player sees a generated race/player label or the current wizard
step. A caller supplies either an explicit PLAYER or a player-array index,
signed-16 flags, and a grammar selector. `Select` follows the explicit pointer
first: an explicit PLAYER permits any signed-16 index for fallback numbering;
otherwise the index must address one of the 16 native player slots. `Shape`
retains both 32-byte fields. Other PLAYER bytes are unchanged by this helper.

For a nonempty singular field, `Name` constructs an optional `the ` article,
capitalizing only its `T` when requested. Capitalization does not change the
race name. Nonzero plural mode selects an existing nonempty plural field;
otherwise use the singular. Trim trailing ASCII spaces from the resulting
article/name combination. In automatic plural mode, append lowercase `s`
unless the last byte is lowercase `s` or the final two bytes are `se`.
Consequently `Mouse` stays `Mouse`, `RACE` becomes `RACEs`, and an explicitly
provided plural is not inflected. This is the observed byte rule, not an English
language rule or Unicode case conversion.

Named labels append ` have` for grammar selector exactly 1 or ` are` for exactly
2, independently of plural mode. Every other selector appends nothing. If the
singular's first byte is zero, do not read the plural as a string: produce
`Player ` followed by the **promoted** index plus one. Index 32767 therefore
produces 32768. Append `'s` only when plural mode is zero, then ` has` / ` is`
for selectors 1 / 2. Article and capitalization flags do not affect this branch.

`Catalog` pins the eight title/grammar/fallback rows and seven default race names
used by these workflows. The fixture checks all fifteen against fresh generated
tables and actual decoding. This is versioned English content; translating or
changing its grammar is a separate content/product decision.

## Reached strings, scratch state and outcomes

The pure model distinguishes a produced `Label` from missing extraction evidence
and undefined native inputs. Those diagnostic constructors are specification
results, not error returns or validations performed by the C helper.

- Only the selected field requires a reached NUL in its 32 bytes. An unused
  plural may be unterminated. A nonempty explicit plural can also make the
  singular's later bytes irrelevant after its first-byte test.
- The source tests the trailing character before its pointer bound. A selected
  all-space name without an article has no defined normal result; the later
  `No Name` fallback must not be advertised as recovery from that read. With an
  article, the article itself remains after trimming: `The ` plus spaces can
  become automatic plural `Thes`.
- A single-byte `e` in automatic plural mode without an article reaches a
  preceding-byte read outside the local buffer. The model records that domain
  separately rather than inventing its suffix. These are ordinary correctness
  boundaries; repairs and their chosen replacement behavior remain separate work.
- Every reached intermediate append must fit the local 50-byte array, including
  NUL. Current 32-byte fields and pinned grammar keep defined named outputs below
  that limit. The final copy replaces only result bytes and NUL in `szWork[360]`;
  `Work` preserves the rest of its previous bytes. The returned pointer is that
  shared buffer, so a caller needing a lasting value must copy it before reuse.

Text lookup changes the separate `szLastStrGet[256]` prefix and terminator,
preserving its later bytes (`LastScratch`). `CchGetString` copies that decoded
text to the caller's destination. `PszGetCompressedString` does not write
`szWork`; the name helper writes it only on the final copy. At this revision the
decoder does not update `iLastStrGet`. The fifteen pinned IDs exceed the native
one-byte cache range, so their lookups always decode and do not take its
cache-hit branch. General cache/table/pointer qualification remains S03/S06.

No UTF-8 validation or character normalization is performed by `PszPlayerName`.
The supported application text contract still requires valid UTF-8 at its input
boundary. Byte preservation tests with other bytes do not expand supported text.

## M04 preset plural and title handoffs

The [preset command](M04-page-commands.md#identity-page-preset-selection-versus-form-commit)
zeroes the plural field after writing the selected default singular, then calls
`PszPlayerName(0, TRUE, TRUE, FALSE, 0, &vplr)`. `DefaultPlural` specifies that
producer for all seven current defaults. `CurrentDefaultReceipt` binds the
command model's supplied default-name/plural values to it. `PluralReceipt`
also describes the general native 192-byte record projection. Copying the
returned plural into the native 32-byte field has its own capacity condition;
the current generated default plurals fit. The preset click's remaining state
and form effects stay with the existing command contract.

`SetRCWTitle` adds the raw signed-16 read-only flag to ID 270 and narrows the
sum to unsigned StringId. It copies that catalog row into a 50-byte local,
formats it into the shared 360-byte work buffer with the supplied signed-16
step, and requests `SetWindowText`. The step need not equal the callback number.
Modes 0 and 1 select the two ordinary title templates; other raw flags may select
different existing rows. `Title` resolves the fifteen pinned rows and names a
`CatalogBoundary` for other IDs. That boundary does not mean native lookup
returns an empty string or fails safely. The complete runtime table/index domain
is still S03/S05 work.

The title format call is SDL `wsprintfA`; fallback player numbering uses
`CchSprintf`. `S04IntegerFormat` specifies their common **untruncated literal,
`%d` and `%%` subset with one promoted 32-bit integer argument**. `CallDomain` limits the
format to fewer than 256 bytes, keeping CchSprintf on its local format buffer. A second `%d`
would require an absent argument; another conversion is outside this subset.
This does not specify all flags, widths, lengths, conversions, allocation
failures or the 1024-byte truncation paths of either function. `Fits` checks
the actual caller capacity as well as the formatter limit. All pinned reachable
outputs here fit without truncation. The two ordinary title templates and any
signed-16 step fit both title buffers.

The title setter ignores its formatting counts and the text API's return value.
An accepted request does not prove visible text, focus, native conversion or
rendering success. A scripted failed text API still receives the same request
in the source fixture. There is no helper-level cancellation or rollback;
the caller owns wizard cancellation and M02 snapshot restoration.

## Acceptance, interaction, quality and evidence

The new criteria live at [S03-AC11–14](S03-static-game-data-and-text.md#s03-ac11),
[S04-AC08](S04-platform-services.md#s04-ac08) and
[M04-AC48–49](M04-race-design.md#m04-ac48). They link stories to these rules and
the [source fixture](../tests/spec/check_player_labels_source.py).
General properties cover trimming idempotence, literal templates, nonempty
decimal output, all signed-16 ordinary titles, unused-plural independence and
prefix/NUL/tail frames. Witnesses
exercise suffix, space, short-name, raw-flag, fallback and title boundaries.
The model does not prove C refinement for every native input.

The fixture compiles the actual name, title, decoding, string-copy, UTF-8 prefix
and both format bodies, with freshly generated text/enums and native PLAYER
layout. Its independent oracle compares 1,033 defined cases, all PLAYER bytes,
lookup order, full shared scratch buffers and preserved tails; 39 undefined-domain
vectors are classified before execution and are not counted as C failures or
normal returns. It uses warnings as errors and address/undefined-behavior checks;
the title's pre-existing assigned-but-unused count has a local diagnostic
suppression. Only SetWindowText and last-error storage are scripted. See the
[dated verification record](README.md#player-label-and-race-title-continuation)
for actual formal and repository check results.

This companion reuses S03-IR01/02, S03-QR01–03, S04-IR01/02, S04-QR01–03 and
M04-IR01/02, M04-QR01–03. The helper has no focus, gestures or viewport of its
own; M04/S05 own those interactions. Exact text/byte/determinism checks are
provided here. Localization, native text API/reentrancy/lifetime, display and
accessible title announcement remain Open with S03/S05/S06 and the UI owner.
No latency or accessibility target is inferred from these fixtures.
