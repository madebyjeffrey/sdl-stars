# M12 — Host research advancement and equipment announcements

Status: observed native behavior, reviewed 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with existing specification work.
This companion specializes [M12-US04](M12-research-and-technology.md#m12-us04),
[M12-US01](M12-research-and-technology.md#m12-us01) and
[M19-US01](M19-game-rules-and-resolution.md#m19-us01)/[M19-US04](M19-game-rules-and-resolution.md#m19-us04).
M12/M19 entries retain stories and criteria. These are headless host outcomes;
M12/M15/S05 own research/messages UI, focus/keyboard/accessibility, text/display
qualification and unagreed performance targets. M18 owns when research runs;
S01/S06 own persistence and native player/global lifetimes.

Canonical definitions: [host transition machine](dafny/M12-host-research.dfy),
[equipment announcement scan](dafny/M12-research-announcements.dfy),
[catalog termination](dafny/M12-catalog-termination.dfy) and
[host control progress](dafny/M12-host-termination.dfy). The host uses
[M12 native cost](M12-development-and-budget-quotes.md), retaining M03's canonical
cost table/formula. Native eligibility, pool and lowest-field refinements reuse
[M12 scalar rules](dafny/M12-research.dfy); source widths/order are not replaced
with those rules outside their domains.

Sources: [turn2.c](../src/core/turn2.c) `UpdateResearchStatus`,
[research.c](../src/core/research.c) `GetTechLevelCost`,
[race.c](../src/core/race.c) `GetRaceStat`, `GetRaceGrbit`,
[parts.c](../src/core/parts.c) `FLookupPart`,
[msg.c](../src/core/msg.c) `FSendPlrMsg`, `FSendPlrMsg2`, `PackageUpMsg`,
[structs.h](../src/core/structs.h) `PLAYER` and `PART`,
[enums.h](../src/core/enums.h) and generated S03 text/data IDs.
[Verification](README.md#host-research-resolution-continuation) records actual checks.

## Interface, state and outcome

`UpdateResearchStatus(fUsePool)` returns void. Any nonzero signed flag means the
annual-pool pass; zero processes existing progress only. It iterates the current
game-player extent. Nonpositive native counts visit no players and finish with
`idPlayer=-1`; positive extents must fit the sixteen-player storage. The model's
player sequence is this iterated projection, not a new allocation/layout.

`Player` records six signed-byte levels/choices, six unsigned-32 progress words,
unsigned four-bit current field, signed four-bit next policy (-8..7), signed-32
annual pool, traits and dead/crippled flags. Supported next policies are fields
0..5, same-field 6 and lowest 7; raw negative nibbles are not silently validated
or promoted to supported orders.

`State` includes local current/next policy, current field and working spending,
Generalized Research state 0/1/2, use-pool/redo latches, per-field shared totals,
alive count, global player, phases, helper trace/cursor and ordered effect events.
`Shape` checks native/projection/control bounds. Initial shared totals are zero
for the pooled path. On nonpool paths they are unread ghost storage; the model
is not asserting that the source initializes its unused stack array.

`Step` specifies every numeric/control branch. `Run` executes a finite prefix;
Running after exhausted fuel is unfinished execution, not success. A missing
lookup result is Pending; an explicitly nonreturning lookup is Escaped; an
invalid integer/index/adapter is Undefined. These markers do not add source
error returns, rollback, recovery or asynchronous behavior. Prefix writes/events
preceding a boundary remain visible in the model without defining C after UB.

The service domain assumes ordinary-returning message calls and lookup results
that preserve the research fields, game configuration and global identity.
Availability/part pointers come from explicit lookup traces. Actual native
message/heap/pointer/catalog frames and escaping message calls remain named
qualification obligations; a Message event means a call attempt, not delivery.

## Player, field and pass order

The source saves the original pool flag, then processes players in index order.
For each player, query Generalized Research, copy the packed current/next fields
to locals, set global `idPlayer` to that player, and increment alive count only
if not dead. **Dead players are still processed.** Each player gets its own copy
of the original pool flag; one player's redirection does not suppress another's
annual allocation.

A pass clears redo, visits fields 0..5 and loads each uint32 progress word by
signed reinterpretation. Slow tech doubles that signed value before adding any
pool, with a signed-overflow boundary. Load also resets the change-current flag.
At the end of six fields, redo restarts that player's field scan with the current
local policy/use-pool/general state. Otherwise advance to the next player.
Only after all players finish does source set global player to -1 and consider
shared research. There is no user cancellation inside this routine.

## Annual pool, Generalized Research and stored units

Only the locally selected field can take annual resources, and only while
use-pool is true and the local general state is below two.

- Ordinary research adds the complete signed annual pool to working spending,
  then to that field's signed shared total. These are separate guarded additions.
- Generalized Research sets redo and local general state to two **before** its
  arithmetic. It adds signed `(pool+1)/2` to working spending and shared total,
  retaining the native +1 boundary.
- For each other field in index order, multiply pool*3 as signed-32, add 19 as
  unsigned-32, reinterpret as signed-32, then divide with signed truncation /20.
  Credit that value to the uint32 stored field (truncating /2 first for slow tech),
  then add the unhalved value to its signed shared total. A later total overflow
  does not erase the preceding unsigned credit.

Unsigned compound progress additions retain low 32 bits. Signed working/total
arithmetic is guarded separately; unsigned assignment cannot repair an already
undefined signed multiplication. `PrimaryProjection`/`OtherProjection` establish
the existing rounded-half/15% formulas only on their declared nonnegative,
nonwrapping domains. Pool 101 gives primary 51 and other fields 16; slow storage
may discard half-unit remainders before a later reload doubles them again.

General state two prevents reapplying the annual pool during redo. Other fields
below the current index see their new credit on the next pass; higher indices
can see it during the current pass. This ordering is not simultaneous allocation.
The routine reads the annual pool without consuming/zeroing its stored field.

## Breakthroughs, caps and temporary working spending

`Eligible` reuses canonical M12 `CanAdvance` for nonnegative levels. A field at
26, or a crippled player's field at/above 10, skips the cost/store branch. It
does not clamp an existing larger level or necessarily write working spending
back. For example, an already-capped selected field can contribute its annual
pool to shared totals while its local working addition is not published.

For an eligible field, quote the next level using the current six-field state.
If the price exceeds working spending, store the latter and finish the field.
Otherwise subtract the signed price, increment the signed-byte level, compute
next-field notice state, attempt a breakthrough message, then enumerate equipment
before deciding whether to redirect. Without a redirect, check again using the
same local working remainder. Several breakthroughs can occur in one field.

Slow storing/redirecting computes `(working+1)>>1` with a signed +1 guard and
arithmetic shift. This differs from the /2 truncation used for Generalized
Research's other-field credits. Normal storage assigns the resulting bits to a
uint32 field. `RunNormal` proves levels that start in 0..26 remain there across
any represented prefix. It does not normalize malformed initial levels, clamp
crippled players already above 10, or establish total termination.

## Local next policy, lowest comparison and redirection

Any breakthrough producing level 26 changes local next=6 to local next=7. That
local assignment is not a direct write of packed `PLAYER.iTechNext`.
For a breakthrough in the selected field with next!=6, select the explicit next
value or compute lowest for the notice and set the change-current latch.

Native lowest compares each signed level byte against the **unsigned low byte**
of the current best. With normal nonnegative levels it keeps the first minimum;
`LowestProjection` proves agreement with canonical M12 `Lowest`. With raw levels
[-1,0,0,0,0,0], it picks index one, so that property is not universal over signed
malformed state.

After all equipment lookups/messages, redirect when `(usePool || change ||
next==7) && next!=6 && field==current`:

1. Lowest recomputes the current minimum, writes packed current field, clears
   old stored progress, and updates local current/next (local next stays seven).
   It leaves packed next unchanged.
2. Explicit next writes packed next=6 and packed current to the low four bits of
   the requested value, clears old progress, then sets local current to the raw
   requested value and local next=6.
3. Apply slow rounding, then add remaining spending to the new field as uint32.
   Invalid raw next indices are boundaries **after** the preceding policy/progress
   prefix, not source rejection before the message or clear.
4. Disable use-pool and set redo. Finish this field; revisit fields as the scan
   and redo require.

The distinction between local seven and persisted next is recorded in the
[collection gap register](README.md#gap-18). No new gameplay policy is selected.

## Full equipment lookup and announcement algorithm

For every breakthrough, start slot mask at Engine (1) and scan all sixteen bits
through Planetary (32768), ending on native uint16 wrap to zero. For each type,
start signed-16 item iterator at zero, copy its low byte into `PART.hs.iItem`,
and call `FLookupPart`. **Only availability zero stops that type.** Restricted
-1, next-tech 2 and other-tech 99 continue scanning without announcing.

Available=1 requires a readable common-part pointer. Announce only when the new
field level equals the unsigned low byte of that part's corresponding required
technology. Other technology/trait eligibility comes from the returning helper;
the controller does not independently recalculate availability.

Starbase hulls use the hull-design notice and ordinary hulls the hull-type notice,
both with goto -3. Matching terraforming entries query TT; when TT and raw item
8/12/16, suppress the notice but continue. Planetary raw items 9..13 and 0..8 use
the two build notices; all other matching entries use the benefit notice. Part
goto is native signed-16 `(typeIndex<<8)|0xc000|rawItem`. Message parameters retain
the signed slot-mask word and raw signed item iterator.

The iterator is incremented as signed-16 storage; it can wrap, while lookup sees
only a byte. `PartTarget` preserves the bitwise OR even for raw iterators above
255 or negative. A supplied finite trace does not invent an Invalid terminator.
`Scan` terminates as a trace consumer, returning Pending when exhausted; a total
native scan needs a source-backed terminating catalog contract. Message return
values are ignored by the source. Its numeric progress is not undone when a
message cannot be packaged/delivered.

## Shared research and recursive nonpool pass

After global player resets to -1, the original pool flag and alive>1 gate shared
research. Visit all player records in index order, query primary trait once per
player, and for Stealth (1) visit positive field totals in field order. Dead flags
do not skip recipients or contributions. Compute `(fieldTotal/alive)/2`, preserving
both integer divisions. Amounts at most one do nothing.

For a larger amount, set redo, attempt the shared notice with low-16 amount,
apply slow round-half if needed, then add it to uint32 progress. Once all grants
finish, redo invokes `UpdateResearchStatus(FALSE)` for **all** players. That pass
reloads progress and can grant more levels/equipment; it cannot distribute another
annual pool or trigger another shared-grant recursion. Complete actual phase,
message and native array/caller-state refinement remains M18/M15/S06 work.

## Catalog bound and host control progress

[S03 research limits](dafny/S03-research-catalog.dfy) records the sixteen native
counts in ascending slot-mask order. `Limits` is bound by the native fixture to
fresh generated enum counts, with ship hulls ending at `ihuldefOrbitalFort` and
base hulls at `isbhullCount`. Every count is below 256. `CurrentCalls` sums each
count plus its terminating lookup: **255 calls per complete equipment scan**.
This is a call bound, not a time or memory budget.

`M12CatalogTermination.Complete` describes a returning trace in which each type
has non-Invalid results below its count and Invalid exactly at that count;
availability one also needs its requirement pointer. Other nonzero status values
continue. `ScanCompletes` proves canonical `Scan` reaches Done and consumes the
remaining calls; `CurrentBound` specializes that result to 255. `AfterLookup`
is the canonical announcement calculation reused by both scan and proof. These
conditions still need refinement from actual player/catalog/helper frames; a
finite trace supplied without its required replies can remain Pending.

`M12HostTermination` uses remaining normal-level capacity, original pool flag,
main/shared phase, remaining players, unused general distribution, redo credit,
remaining fields and within-field work as a natural rank. Each successful Check
raises the six-field level sum by exactly one, decreasing remaining capacity.
Generalized distribution consumes its one-time latch; a redirection follows a
new level and grants one redo. Field/other-field/player traversal decreases
within those phases. The shared recursion consumes the original pool flag and
cannot recur again. Control invariants in `Shape` are preserved from `Begin`;
they describe the phase, not new C input validation. `Fail` permits the source's
intermediate general-state write before a boundary by checking the terminal
state's shape, without asserting that transient state is a running loop head.

`StepDecreases` states strict rank descent for every Running shaped state with
levels 0..26. `Finish` recurses on that rank without caller-provided fuel;
`SufficientFuel` connects finite-prefix `Run` to `Finish` once fuel exceeds the
derived rank. This establishes termination of represented host control at Done,
Pending, Escaped or Undefined. Successful native completion additionally needs
actual arithmetic/index/service/frame conditions excluding the other boundaries;
normal levels alone do not establish those conditions. The rank is not a measured
execution-cost estimate, a newly approved pool limit or a recovery guarantee.

### Lookup-derived scan completion

[M11 native availability](M11-part-availability.md) now specifies the complete
category/trait/trader/status decisions, source call order and pointer effects.
[M12EquipmentLookup](dafny/M12-equipment-lookup.dfy) supplies `AppendTrace` and
`ScanCatalog`: a shaped sixteen-category catalog and selected context produce
the required returning replies and a proved Done result after 255 calls. The
reply prefix is preserved. This adapter derives Complete from the lookup rules;
it retains canonical announcement Scan and AfterLookup effects.

The host machine still consumes an explicit trace. Integrating the adapter must
map the current PLAYER and all flags/trader/row state into Context at each
breakthrough, with the declared helper/storage frames. That complete native
mapping and the host arithmetic/successful-Done domain remain open. The broader
native lookup fixture checks current generated rows and finite frames, without
claiming every host/browser/design caller is already qualified.

## Evidence, liveness and remaining work

[Native source fixtures](../tests/spec/check_host_research_source.py) compile
actual common.h records, fresh text/data IDs, the complete host body, actual race
and cost bodies, and the actual two-argument message forwarding wrapper. Part
availability/requirements and message returns are explicit providers. An
independent width-aware interpreter checks levels, all progress words, packed
policies, annual pools, global identity, getter/query/message/cost/entry counts
and ordered 32-bit trace fingerprints. Entire remaining PLAYER fields and GAME
records are compared unchanged; fingerprints are finite diagnostics, not a
universal trace/refinement proof.

Cases cover all 256 packed policies, normal/raw byte levels and uint32 spending,
ordinary/general/slow/nonpool paths, dead/crippled flags, multiple/zero/negative
player counts, shared recipients and recursion, all notice groups, TT suppression,
availability codes, byte and signed-16 iterator wrapping, false message returns
and controlled lookup longjmp prefixes. A stateful width-test provider can delay
Invalid beyond real catalog extents; those cases do not establish real-game
reachability. Invalid arithmetic/index/pointer candidates are filtered before C;
this is not implemented source error handling.

A separate native probe compiles actual `FLookupPart`, `TechStatus`, hidden-part
logic and fresh generated data tables. It checks all 256 byte IDs for each type
in headless context, all out-of-range IDs for all signed-byte primary traits,
and that all sixteen actual terminators occur before byte wrap. It also checks
every formal limit against the corresponding freshly generated enum. Restricted
headless entries are non-Invalid and do not stop the scan. This supplies current
native catalog-bound evidence; it does not prove the complete availability table
or bind every dynamic lookup trace in the formal host model.

Proofs cover shape/control and normal-level preservation, nonnegative-domain
pool/lowest refinements, terminal stability, catalog-bounded scan completion and
host rank descent. Dated collection verification records which definitions and
obligations passed. Native finite convergence cases independently compare the
source and interpreter; they do not prove universal C refinement.

The remaining liveness work is successful Done completion under native-derived
arithmetic/index/policy/storage/service conditions, followed by actual helper,
message/heap/pointer/global/caller frames and annual phase integration. Catalog
counts and named lookup IDs are bound to fresh generated enums; the M11 decision
model and M12 adapter derive complete scans from shaped rows/context. Their full
native host state/address/lifetime extraction remains open. Saved-game/GUI/AI/platform qualification
also remains open. An arbitrary convenient fuel or pool bound is not a substitute
for the actual supported domain. No runtime, file-format, baseline or approved
behavior changes are introduced.


### Remaining completion acceptance work

| Obligation / owner | Authoritative inputs and acceptance evidence still required |
| --- | --- |
| Entry arithmetic and spending domain / M12/M09/M19/S06 | Bind annual `Player.pool` to `PLAYER.lResLastYear` produced by `Produce`, including its initial zeroing, empty-queue resources, allocation and queue-tail additions. Bind uint32 `rgResSpent` to loaded state and every earlier same-year writer. Prove that resulting values preserve each actual signed multiplication/addition/subtraction/rounding domain through all redo and grant passes, or identify the reachable source boundary. Use actual producers rather than selecting a convenient numeric pool bound. |
| Policy and level reachability / M12/S01/M18 | Bind normal levels and local/packed policy to the accepted loaded state and preceding annual phases. [Research order replay](S01-game-file-orders.md) validates unsigned current/next nibbles before copying the packed byte into PLAYER's signed-next overlay; `rtLogResearch` in [log.c](../src/core/log.c) and all other writers must establish the host preconditions. Raw signed-policy fixture coverage alone does not prove accepted-game reachability. Include pooled `Produce` and both nonpool `DoOrders` call contexts. |
| Actual catalog and helper frame / M11/S03/M12/M15/S06 | The M11 model now specifies all three decision helpers and the M12 adapter derives Complete from shaped rows/context. Bind that adapter to actual host PLAYER flags/trader state, catalog addresses and lifetimes at every breakthrough; native source fixtures supply current finite pointer/frame evidence. Establish message/race/cost return and frame conditions, including global identity, research fields and storage lifetimes; separately qualify message packaging/allocation/delivery and escaping calls. |
| Successful completion and annual integration / M12/M18/M19 | Combine the derived input/frame invariants with scan completion and rank descent to show Finish is Done, with the specified research/global post-state. Compare actual complete annual entry/exit state and saved-turn outputs, including general/slow/dead/crippled/shared-grant cases; preserve current undefined-input and partial-effect boundaries. A terminal model marker or passing standalone provider fixture does not satisfy this criterion. |
