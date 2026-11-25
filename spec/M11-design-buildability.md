# M11 — Design buildability and temporary player context

Status: observed source behavior reviewed 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the working specification collection.
This companion specializes [M11-US01](M11-ship-and-starbase-design.md#m11-us01),
[M11-US03](M11-ship-and-starbase-design.md#m11-us03) and
[M11-US06](M11-ship-and-starbase-design.md#m11-us06). It supplies the availability
answer consumed by host construction and annual design checks. M11 retains its
stories and acceptance criteria; M09/M19 own construction, M18 annual sequencing,
S01 accepted serialized designs and S06 native storage/identity/lifetimes.

Canonical definitions: [M11DesignBuildability](dafny/M11-design-buildability.dfy),
reusing [native part availability](M11-part-availability.md),
[M11's existing CanBuild predicate](dafny/M11-design.dfy),
[S06 HullSlot](dafny/S06-shared-state.dfy) and current S03 masks/catalog IDs.
[Collection evidence](README.md#design-buildability-continuation) records checks.
No gameplay, import policy, record layout or runtime validation change is proposed.

Sources: [util.cpp](../src/core/util.cpp) `FCanBuildShdef`;
[parts.cpp](../src/core/parts.cpp) `FLookupPart`, `TechStatus`;
[research.cpp](../src/core/research.cpp) `FShouldPartBeHidden`;
[race.cpp](../src/core/race.cpp) the actual primary/bit getters;
[structs.h](../src/core/structs.h) `SHDEF`, `HUL`, `HS`, `PART`, `PLAYER`.
Call sites include [turn2.cpp](../src/core/turn2.cpp) `FBuildObject` and
[turn.cpp](../src/core/turn.cpp) annual design checks. UI browse/copy/edit paths retain
their own M11 context/knowledge/focus/keyboard/accessibility qualification. The
helper is synchronous, has no cancellation or user prompt, and has no agreed
performance budget beyond the modeled traversal bound.

## Interface and source domain

`FCanBuildShdef(SHDEF *lpshdef, int16_t iplr)` returns TRUE or FALSE on ordinary
completion. It saves the current signed-word global `idPlayer`, chooses a hull
lookup, sets global player to the requested signed target, then invokes the
part lookup service. It restores the saved global before each ordinary return.
The old saved value is copied without being used to index a player here; it can
be any signed word. Target `iplrNone` (-1) uses the lookup's headless behavior;
other targets require a live entry in the sixteen-player array.

`Design` projects the hull's unsigned-16 catalog field, unsigned-byte `chs` and
sixteen physical `HullSlot` entries. Slot category is uint16 and item/count are
bytes. `Shape` covers the physical storage and field widths, not the source's
absence of an up-front `chs<=16` guard. The structural normal-call theorem uses
`chs<=16` explicitly. Live correctly typed SHDEF/player/catalog storage is a
native caller condition; the model is not a pointer validator.

`PlayersFit` projects all sixteen physical PLAYER entries into the already
specified availability Context. `Selected` chooses the requested player or a
headless context. Its headless placeholders describe fields the helper does not
read, rather than asserting zeroed native PLAYER contents.

## Hull request and native narrowing

`HullRequest` preserves the actual order:

1. Interpret the unsigned hull word through the native signed-16 comparison.
2. At signed value >=`ihuldefOrbitalFort` (32), choose the base-hull mask and
   subtract 32; otherwise choose the ship-hull mask and retain the raw word.
3. Assign the resulting item through the eight-bit `HS.iItem` field.

Normal IDs therefore use ship 0..31 and base item `hull-32`. Raw values are not
silently clamped or rejected: hull 288 selects base item zero after byte
narrowing, while raw 32768 takes the signed-negative branch and selects ship
item zero. `HullVectors` records these cases. They are raw-width witnesses,
not supported design IDs or proof that a normal load/UI path creates them.

The local hull PART initializes its mask and item but leaves quantity and prior
union-pointer storage uninitialized. `Quantity.Uninitialized` makes that visible;
no numeric zero is invented. Actual lookup does not inspect quantity, and the
controller only consumes its returned status. The native observer also avoids
reading that uninitialized hull quantity/pointer. Component requests later copy
complete initialized HS entries. Logical lookup pointer effects remain owned by
M11 native availability; this routine does not expose its local PART afterward.

## Ordered workflow, rejection and restoration

`Check` queries the hull first. Any result other than exactly Available (1)
returns FALSE and restores the old global; positive future-status codes also
reject. No slot is visited after hull rejection, even if the declared count is
larger than physical storage. `RejectBeforeExtent` proves this early rejection
without assuming a small slot count.

After an available hull, `Traverse` visits declared slots in order. Zero count
skips lookup even when the unused category/item bytes are garbage. Every positive
quantity makes one lookup, preserving its byte count in the copied request.
There is no per-unit lookup loop. The first non-1 result returns FALSE; otherwise
completion returns TRUE. `Finish` restores the exact saved signed-word identity
on both ordinary outcomes. At most seventeen part lookups occur in the defined
physical traversal: one hull plus up to sixteen occupied slots.

`Requirements` is a mathematical sequence of the occupied-slot availability
requirements under stable context/catalog inputs. It does not add native calls
after rejection. `Decode` selects a semantic status representative with the same
numeric code; it retains the existing 99/Later encoding ambiguity. The proved
Check postcondition agrees with canonical `M11Design.CanBuild` on structurally
bounded calls. `HullOnly` shows zero declared slots leave only the hull test;
this is an availability check rather than an engine or complete design validator.

The routine does not inspect free/gift metadata, slot compatibility/capacity,
engine presence, budget, fleet/roster limits or whether the hull fits the caller's
roster kind. Callers and other M11/M09/M19 rules own those checks. In particular,
FBuildObject short-circuits free designs before this helper. This specification
does not turn the helper's availability TRUE into a general acceptance guarantee.

## Undefined domains and failure prefixes

`Undefined` records a first invalid native read boundary, with the known preceding
context change and probe prefix. It is not a FALSE result, rollback, recovery,
or a defined C post-state after undefined behavior.

- A target other than -1 or 0..15 reaches a player access inside the first lookup;
  player context has already been selected. It is not rejected by this controller.
- With an available hull, a count above sixteen can reach `rghs[16].cItem`.
  Earlier hull/component rejection can return normally first. `EmptyOverrun`
  proves that an oversized all-empty slot sequence reaches the read boundary
  after just the hull lookup, leaving the modeled known prefix at the target.

Ordinary restoration depends on returning helpers and valid native read/lifetime
conditions. The actual lookup/race/technology bodies have finite checked decision
models under those conditions. A nonreturning access/helper or external escape
has no general restoration guarantee from this source. The model makes no new
exception handling or asynchronous result convention.

## Caller handoffs and remaining integration

[M19 construction effects](M19-construction-effects.md) and
[M09 execution](M09-production-execution.md) consume buildability separately from
costs, resources, base prerequisites and free-design guards. Check now supplies
a canonical producer for the formerly abstract `canBuild` helper answer. The
original whole-construction fixture retains its explicit availability provider.
The [M19 eligibility integration](M19-construction-buildability.md) now adds a
checked exact-outcome/guard/roster handoff and actual checker/catalog composition
inside FBuildObject and a full Produce/fleet-limit failure case. Complete native
lifetime/alias and annual/UI qualification remain open.

Full integration must bind each chosen roster/design pointer and planet owner to
these inputs at the call, preserve short-circuit guard order, and establish
storage lifetimes. Annual design/knowledge state and gift marking are M18/M11
caller work. UI browse/copy selection and temporary contexts on other paths
retain their own qualification. S01 owns which raw counts/hull/policy values
accepted files can produce; the width fixtures do not approve malformed designs.

## Evidence and qualification limits

[Native controller fixture](../tests/spec/check_design_buildability_source.py)
compiles the actual controller together with actual lookup, race, hidden-part
and technology bodies and fresh generated catalog tables. It reuses the
[availability fixture's native source builder](../tests/spec/check_part_availability_source.py),
whose complete existing matrix is rerun to validate that extraction. The
independent reference uses the actual typed-array export of all 239 catalog rows.

Coverage includes every unsigned hull word, every saved signed global word,
normal target/headless contexts, raw target-domain candidates, all byte slot
counts with early-rejection alternatives, all 65536 slot-occupancy masks, every
catalog row/terminator in every slot across primary/level contexts, varying
quantities, free/gift flags and seeded raw states. Exact lookup requests, helper
query order and ordinary global restoration are compared. Complete SHDEF,
PLAYER and GAME byte frames are checked; the uninitialized local hull quantity
is deliberately not inspected.

Undefined player/slot-read candidates are classified and excluded before C
execution. That filtering is not native validation or recovery. ASan/UBSan and
compiler warnings are fatal, with the existing native pointer-sign declaration
exclusion. This is finite evidence at the tested native widths/char conventions;
full C memory/alias/lifetime and real caller/annual/UI/platform refinement remain
explicit obligations. The completion goal covers those remaining gaps separately.
