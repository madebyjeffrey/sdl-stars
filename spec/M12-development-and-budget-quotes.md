# M12 — Native development quotes and research budget preview

Status: observed native behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion specializes [M12-US02](M12-research-and-technology.md#m12-us02)
and [M12-US03](M12-research-and-technology.md#m12-us03), and the production
handoff [M09-US02](M09-production-management.md#m09-us02). Entry documents own
stories and acceptance IDs. A quote estimates investment; it does not grant a
technology, advance the host, save orders or confirm equipment availability.

Canonical native models: [development quotes](dafny/M12-development-quotes.dfy),
[budget preview](dafny/M12-budget-preview.dfy). The former reuses
[M03 `TechCosts`/`ResearchCost`](dafny/M03-scoring.dfy); the latter consumes the
existing [M08 resource word](M08-colony-economy.md) and
[M09 production forecast](M09-production-forecast.md). S01 codecs retain native
signed reinterpretation; S06 retains actual player/planet storage and lifetimes.

Source: [research.c](../src/core/research.c) `GetTechLevelCost`,
`CostOfDevelopingItem`, `ProjectedResearchSpending`;
[race.c](../src/core/race.c) `GetRaceStat`;
[structs.h](../src/core/structs.h) `PLAYER`, `PLANET`, `PLPROD`;
[researchui.c](../src/ui/researchui.c) budget refresh and component requirements;
[thing.c](../src/core/thing.c) trader quote use. The
[collection record](README.md#research-quote-and-preview-continuation) records
actual verification and remaining scope.

## Interface, actors and ordinary/error flows

Players inspect costs through the research dialog/component browser. Trader
resolution also requests a development quote after setting the global player to
the recipient. These headless routines have no independent controls, focus,
cancellation or error dialog. M12/S05 own browser feedback, keyboard/accessibility,
text/layout/display qualification and unagreed latency targets; M06/S01 own
persisting a chosen policy. Ordinary quote completion returns a number and
restores specified temporary fields. It is not a global transaction.

Player technology and research-cost choices are signed bytes; stored spending is
**unsigned-32**. The cost function's level/player arguments are signed-16 and its
field selects the corresponding race attribute (six normal technology fields).
`Development` consumes six target bytes, six level bytes, six spending words and
six choice bytes. The current supported native targets read the source's plain
`char` technology buffers with the signed-byte interpretation checked by the
native fixtures; other compiler char conventions require separate qualification.
Raw typed values are wider than supported game settings.

`Done` is ordinary completion; `Unreachable` is the source's returned -1 when
any target exceeds 26. `Undefined(reason)` is a model boundary for invalid
indexing, missing adapters or signed arithmetic; it is not a newly implemented
error return. State/events expose the preceding numeric prefix without claiming
a deterministic C post-state after undefined behavior. Native memory/alias and
escaping-helper frames are separate obligations. Signed reinterpretation follows
S01's supported two's complement targets, not every possible C implementation.

## Native single-level technology cost

`TechCost` first sums all six current signed level bytes into the source's signed
16-bit total. That sum is -768..762, so the source's narrowed ten-times surcharge
is exact on this typed domain. Add the canonical table entry for level 0..26;
indices outside the table have no fabricated price.

Read the signed cost choice and subtract one in signed-16 storage. A negative
result (original choice below one) selects expensive cost `2*base-(base>>2)`;
zero (choice one) leaves base unchanged; positive selects signed truncation /2.
The right shift uses native arithmetic shift, equivalent to floor /4 including
negative bases. Slow tech doubles the final price after that branch. Mathematical
truncation is not substituted for a negative arithmetic right shift.

`CostDefined` proves a defined signed-32 price in -26880..323120 across the full
six-byte level/choice domain and valid table indices. It does not make negative
levels or arbitrary choices valid game state. `CostProjection` proves agreement
with canonical M03 `ResearchCost` for nonnegative levels and choices 0..2; the
shared table and normal pricing formula are not copied into a second owner.

## Ordered development quote and restoration

`Development` preserves the entire source algorithm:

1. Scan all six target bytes before changing levels. Any target above 26 returns
   -1, even if that field would not otherwise need an increase. No level quotes
   occur. Negative targets are not clamped or rejected by this upper-bound check.
2. Process field indices 0..5. Save each original level before examining that
   field. If its target is not higher, skip spending and price calculations.
3. Otherwise reinterpret its uint32 stored spending as signed-32. Double it for
   slow tech with a signed-overflow boundary, then negate it with a separate
   INT32_MIN boundary to initialize the field's current cost.
4. While target > current level, quote the next level against the **currently
   modified six-field state**, add the quote with a signed-overflow guard, and
   only then increment the signed byte. A failed addition precedes that increment.
5. Add max(0,fieldCurrentCost) to the signed total. Earlier fields stay raised
   while later fields are quoted. After all fields return, restore all original
   levels and return the total; stored spending and choices were read, not spent.

At zero starting levels, normal cost choices and no prior spending, raising the
first two fields to one costs 50+60=110. Two independent next-level quotes from
the original state would give 100 and do not reproduce this algorithm.
Previously spent research is deducted per field only when that field needs work;
there is no cross-field refund of a negative field remainder.

`RaiseTo` terminates as the target/current gap decreases, and `Fields` visits six
fields in order. Inner loops can return only Done or Undefined; only the initial
full-target scan produces Unreachable. Postconditions prove unchanged unrelated
levels during a field, nondecreasing accumulated total, restoration on Done,
nonnegative completed quote, and untouched levels/empty quote trace on the -1
path. Undefined arithmetic retains earlier modeled raises without promising the
later restoration. Caller memory aliases and the actual quote-to-trader/browser
handoff still need their native state adapters.

## Budget preview: allocation, queries and captured bounds

`ProjectedResearchSpending` saves the current global player's signed allocation
byte and assigns its signed-32 requested percentage to that byte. `Byte` preserves
low-eight-bit narrowing: 255 becomes -1, 256 becomes zero. This routine does not
clamp the request to 0..100. The UI's supported control range is a separate caller
contract.

The routine captures the planet array pointer and end bound once. `Preview` uses
a valid captured array of 0..32767 planets; callback changes to the global count
do not shorten this traversal. Replacing native storage or creating aliases is
not implied safe by this logical captured-array projection.

For each captured planet, compare its current owner to the current `idPlayer`.
For an owned planet **always** query `CResourcesAtPlanet` first, sign-extending its
signed-16 result. After that query returns, inspect the planet's current queue:

- Null queue or allocated count zero: add the queried gross word directly.
- Nonempty queue: call `EstimateItemProdSched` with null temporary queue,
  research query item 65535, and separate first/bogus-last output locations. Add
  its signed-16 first result; the earlier gross result is discarded numerically.

There is therefore a gross query even when an estimator will query production
resources again. Helpers can have effects beyond their numeric return. Queue
branching reads the post-resource state; it is not captured before the call.
The numeric total adds one signed word per retained planet. `BoundedAddition`
and `Loop` prove bounds -32768*count..32767*count, within signed-32 for the native
count range. Missing/escaping helpers do not produce a completed budget.

## Helper traces, failure prefixes and restoration target

`World` carries allocations, the captured planet/queue projection and current
global player. `Reply` identifies the exact call and its returning word, complete
post-helper projection and whether it returned. `Receive` checks call order and
adapter shape. Missing trace is Pending; supplied nonreturn is Escaped; mismatch
or invalid storage projection is an Undefined adapter boundary. These are
specification outcomes, not source catch/rollback branches.

`ResourceReply` connects a defined M08 signed word to the resource call; a missing
numeric result does not become a successful zero query. M09 still owns computing
the production forecast. The tests use explicit callbacks to exercise controller
frames; they do not prove every real helper can reach each supplied post-state.

On ordinary return, source writes the saved byte through **current** `idPlayer`,
then returns total. `Loop`/`Preview` prove that current player's allocation equals
the saved entry byte. Restoring the original player's allocation additionally
requires that global player identity remained the same. If a declared callback
changes it, the original requested byte can remain at the entry player while
the saved byte is written to the other player. This is a caller-context boundary,
not evidence that the real resource/estimator pipeline necessarily changes it.
There is no restoration after an escaping helper or an undefined final index.

A copied production queue/planet also does not establish global purity. The
[M09 helper-effect contract](M09-production-forecast.md#local-copying-does-not-establish-global-purity)
retains inherited generation/RNG, live AR fleet flags and generation-time
messages. Preview success is neither a host breakthrough nor a future event
prediction.

## Evidence and remaining work

[Native fixtures](../tests/spec/check_research_source.py) compile actual common.h
records, fresh generated IDs, the actual cost table, race getter and the three
research bodies against an independent width-aware interpreter. Cost traces
check field, requested level, six-field total and returned price, as well as
restored levels and unchanged spending/choice/player fields. The corpus covers
all signed-byte cost choices, every table index at declared level contexts,
ordered multi-field quotes, raw unsigned spending and slow/negation/addition
boundaries. Undefined candidates are filtered before C execution.

Preview fixtures explicitly supply resource/estimator post-state callbacks,
including a controlled longjmp escape caught by the harness. They compare
allocation, player/queue/owner/count effects and query ordering; a nonreturning
call has no asserted function return value. The maximum native count is exercised
with signed-word extrema. Actual helper algorithms have their own M08/M09/M19
contracts/fixtures; end-to-end native refinement remains open. C11 warnings,
ASan/UBSan are fatal except the project's existing pointer-sign and unused-local
warnings in the extracted unchanged source. Draft fixture prototypes were corrected
to match native declarations; no production C changed.

Remaining E12-1 obligations are native memory/alias/storage/helper/caller-state
refinement, actual browser/trader contexts, escaping cleanup and cross-platform
qualification. E12-2 still owns browser/hidden-part/catalog interaction; E12-3
now has [complete host/equipment operational transitions](M12-host-research-resolution.md);
its total termination, native helper/state/caller integration and qualification
remain open. Passing these quote/controller proofs
does not close the remaining liveness/refinement obligations, establish UI accessibility/performance targets
or authorize a gameplay change.
