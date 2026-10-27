# M09 — Construction payment, partials and alchemy

Status: observed `CBuildProdItem` behavior, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion supports [M09-US02](M09-production-management.md#m09-us02) and
[M09-US05](M09-production-management.md#m09-us05); criteria remain in M09.
[Canonical contracts](dafny/M09-builder.dfy) specify the numeric/control trace.
Sources are [turn2.c](../src/core/turn2.c) `CBuildProdItem` and `Produce`,
[structs.h](../src/core/structs.h) `PROD`/`PLANET`, and
[enums.h](../src/core/enums.h) `ProdItemType`/`mdProdStat`.

The caller first obtains four unit costs from the [cost owner](M09-production-costs.md),
then computes quantities and payments. `CBuildProdItem` does not install objects:
`Produce` subsequently calls `FBuildObject`. Failed object application can follow
successful charges. The [forecast controller](M09-production-forecast.md) calls
this same builder and applies only its own post-build local effects. Full host
queue mutation, application and phase effects remain M09/M18/M19 obligations.

## Interface, storage and execution domain

Inputs are a valid live planet/player, packed source row, four defined signed-32
cost words, four signed-32 available amounts, optional independent partial/status
storage, and nonzero/zero alchemy/preview/generation flags. The cost buffer is
filled through a routine that writes unsigned words; the builder interprets the
same bits as signed-32 values. `Start` begins after a successful, defined quote;
invalid design/catalog pointers and an unassigned quote buffer have no invented
values. Cost/capacity/race/planet helpers must finish normally. Pointer validity,
aliasing with input buffers and helper effects require native adapters.

`Row`/`Fits` reuse the canonical S01/M09 packed representation: count 10 bits,
item 7, category 3, progress 7 and unused 5. Progress 101..127 is representable
even though normal game progress is smaller. Installation fields have 12 bits;
helper capacities/terraform/desirability/warp are signed 16-bit returns. Growth
is signed 32-bit. A partial slot can be absent, count-only zero, or fully defined;
the algorithm does not read the uninitialized remainder of a count-only slot.

`State` retains caller and local rows, partial, fixed quote, four paid/remaining
amounts, built count, signed-16 conversion count, automatic mode, manual-item
mapping, initial progress, limiting percentage, possibly uninitialized deficit,
accumulated blocking flags, cursor and phase. `Step` follows each native phase;
`Run` executes a supplied finite transition budget and explicitly returns an
unfinished state if exhausted. `NoAlchemyStepProgress` proves a strictly
decreasing rank when alchemy is disabled, and `FinishNoAlchemy` reaches either
ordinary completion or a named undefined boundary. Retrying alchemy remains
outside that termination theorem; a finite trace alone is not its proof.

`Undefined(reason)` marks the first signed overflow or uninitialized deficit
read. It is a formal boundary, not a C error return. Defined state changes before
that boundary remain visible. Signed additions, subtractions and products are
checked before casts. Initial paid amounts use signed multiplication followed
by unsigned-32 reinterpretation and division by 100. Alchemy's resource compound
subtraction is unsigned-32 and wraps before signed storage. Progress/count writes
retain low 7/10 bits; conversion totals retain signed low 16 bits. These domains
must not be replaced with unbounded nonnegative formulas for malformed inputs.

## Automatic cap and helper order

Cost quotation precedes cap helpers. A row is automatic exactly when category
is planet (1) and item is below 7. `Prepare` uses actual helper return snapshots;
`Calls` specifies their lazy query order after quotation:

| Item | Cap and manual partial mapping |
| --- | --- |
| Mine 0 / factory 1 / defense 2 | Signed-16 narrowing of operable minus installed, then clamp negative to zero; manual IDs 8 / 7 / 9. |
| Alchemy 3 | Cap 1000, forced even if original count is smaller; manual ID 11. |
| Minimum terraform 4 | Terraformable count; when positive, query growth, then desirability only if growth is nonnegative. Nonnegative growth and positive desirability force cap zero; manual ID 12. |
| Maximum terraform 5 | Terraformable count, without those growth/desirability calls; manual ID 12. |
| Packet 6 | Cap 1000 only if warp and destination are nonzero, otherwise zero; manual ID 17. |

Replace count when above cap or for automatic alchemy; assign the cap through the
10-bit count field. Otherwise retain count. For example, operable -32768 minus
one installed unit narrows to 32767 before clamping; it does not become zero. The caller's automatic row remains
unchanged even if local count/progress changes. Helper implementations and their
call-time identity/state mappings remain with M08/M19/S06; supplied values do
not assert purity or verify their full formulas.

## Payment and retry workflow

1. Initialize paid amounts from the capped local row's progress. Inspect remaining
   cost in dimension order (Ironium, Boranium, Germanium, resources), stopping at
   the first unaffordable dimension.
2. If all fit, increment built count, decrement local quantity and reset local
   progress before subtracting each remaining payment and clearing its paid
   amount. Repeat for the next item using the same quote.
3. Otherwise start limiting progress at 100 and scan positive cost dimensions.
   If available is at least unit cost, that dimension contributes 100. Otherwise
   compare truncating `(available+paid)*100/cost` and
   `(available+paid+1)*100/cost-1`. Only a strict decrease replaces the limit and
   deficit. Mineral/resource blocking flags accumulate; a later minimum does
   not clear an earlier flag.
4. An automatic mineral block bypasses charging. Without alchemy it sets mode 2
   and finishes. With alchemy it jumps directly to conversion. Other blocks
   charge every dimension's new integer paid amount minus old paid amount,
   then write low-7-bit progress. Preview mode still performs these payments.
   No-alchemy or any accumulated resource block ends the loop.
5. Conversion price is 25 for Mineral Alchemy, otherwise 100. Convert the lesser
   of truncating remaining resources/price and the last limiting deficit. A
   positive conversion adds one of each mineral per unit, deducts resources
   using unsigned compound subtraction and updates the signed-16 counter.
   Exact deficit satisfaction retries affordability with retained paid amounts.
   The deficit variable is retained across retries, not reset on each scan.
6. If the deficit cannot be met and positive resources/partial storage remain,
   clear the entire partial, create a one-item manual-alchemy row, compute its
   progress with the same one-unit rounding allowance and charge that progress.
   This happens even in preview mode and can overwrite a previously populated
   partial. Otherwise finish without manufacturing one.

`PaidChargeBalance` checks that each defined partial charge preserves available
plus paid for that dimension. It does not claim that all four changes are atomic
or that percentages monotonically increase over every malformed signed input.
The built-plus-local-count bound is preserved at every defined transition.

## Completion, caller writes and observable results

Built planetary alchemy (IDs 3/11) adds built units to each mineral and the
conversion counter. Nonzero conversions emit a message only outside preview
and while generation is active. Shared `AlchemyMessage`, `Result` and
`StatusWord` remain the canonical predicates/encoding; status declaration order
is not its native numeric value. Optional status storage is written before
caller/partial publication.

Ordinary rows copy the complete local row back only outside preview. Automatic
rows retain their original caller row. If automatic progress is nonzero and
partial storage is present with count zero, copy the local row, change count to
one and map to the manual item; retain unused bits. An existing nonzero partial
is retained by this final publication branch. Earlier partial-alchemy creation
has its own unconditional overwrite rule. There is no user cancellation inside
this numeric routine; cancellation and recovery belong to the owning editor or
outer host invocation. Interaction, focus, display and performance targets stay
in M09/S05 rather than defining a new interface here.

## Evidence and remaining boundaries

[Source fixtures](../tests/spec/check_production_builder_source.py) extract the
actual unchanged C builder, native packed structs and enum values. Quote,
capacity, growth/desirability, race and message services are explicit providers.
A separate Python interpreter computes expected row/partial words, all four
remaining amounts, status, quantity, message payload and helper call order.
It checks signed intermediates before execution; undefined vectors are excluded
before calling C, rather than treating sanitizer failure as a passing result.
The corpus includes all packed percentages, all eight statuses, all automatic
cap branches, 16 preview/alchemy/trait/generation combinations, optional outputs,
unused bits, signed capacity narrowing, large-deficit signed-16 conversion
counter wrapping, malformed numeric inputs and deterministic random vectors. C11 warnings, ASan and UBSan findings are fatal.

These comparisons do not prove the interpreter, Dafny model and C body equivalent
for every input. Termination/progress for retrying alchemy over a declared valid-game domain,
helper/pointer/frame refinement, quote derivation, escaping message/helper calls,
alias cases and the outer `Produce`/`FBuildObject` queue/application/failure state
remain explicit work. The full verification/evidence counts are recorded in the
[collection continuation](README.md#production-builder-continuation).
