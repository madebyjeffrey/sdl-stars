# M08 — Native mass-driver capability and construction handoff

Observed source behavior inspected 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the existing working collection.
This companion specializes [M08-US04](M08-planet-and-colony-management.md#m08-us04):
a player needs the selected colony's actual orbital capability to plan packets.
It also supplies [M19-US01](M19-game-rules-and-resolution.md#m19-us01) construction
and packet application. M08 owns capability calculation, M11 design availability,
M19 application, M09 charging/queues, S03 identities, S06 live state and S01
admission. It introduces no screen, runtime validation, behavior or format change.

Canonical [M08MassDriver](dafny/M08-mass-driver-capability.dfy) specifies the
complete `IWarpMAFromLppl` controller and slot scan. `Scalar` proves refinement
to the existing M08 `Drivers` aggregation after `Occupied` projects eligible
slots. [M19ConstructionEffects.DesignFling](dafny/M19-construction-effects.dfy)
composes actual capability outcomes with the existing `BaseFling`; it does not
replace that calculation with a second formula. Acceptance criteria stay at
the [M08 entry](M08-planet-and-colony-management.md#4-acceptance-criteria-and-traceability)
and [M19 entry](M19-game-rules-and-resolution.md#4-acceptance-and-traceability).
[Collection evidence](README.md#mass-driver-capability-continuation) records checks.

Source: [planet.c](../src/core/planet.c) `IWarpMAFromLppl`;
[turn2.c](../src/core/turn2.c) `FBuildObject`, `Produce`;
[structs.h](../src/core/structs.h) PLANET/SHDEF/HUL/HS;
[enums.h](../src/core/enums.h) detail/category constants;
[generated catalog inputs](../data/parts/starbase-specials.yaml) driver identities.
The helper makes no catalog lookup, availability query, race query or RNG call.
Other consumers include `produce.c`, `turn.c`, `file.c`, `ai3.c`, `ai4.c` and
`aiutil.c`; their complete surrounding algorithms remain their owners' work.

## Inputs, state and domain

`Lookup` takes the planet's signed-word owner, base flag and four-bit base slot,
the signed-word global `idPlayer`, sixteen roster-pointer projections and an
output-pointer-presence flag. Each present roster projects ten SHDEFs; a design
has three-bit detail, byte slot count and sixteen physical slots. Reuse S06's
category/item/quantity widths; `Shape` requires all sixteen physical slots even
when the active count is smaller. The model does not prove pointer liveness,
active-game membership, catalog compatibility or decoder/writer reachability.

The planet pointer is live/readable. The optional `pfTwo` points to writable
signed-word storage disjoint from the planet, designs, pointer arrays and global
identity. Null omits both writes. Aliasing is a separate S06 caller obligation:
the initial write occurs before reading planet state, so an aliased output can
change which branch or design is read. This model does not generalize its
nonmutation frame to arbitrary aliases. An invalid output pointer would fail
before the modeled body; ordinary input pointers also require C lifetime and
alignment. No native validation of those obligations is asserted.

`Outcome` distinguishes ordinary Done from Undefined read-domain markers and
records the computed capability prefix, optional output value and ordered logical
access events (controller stages, not a count of every repeated C load). Undefined
is not a native return, exception, repair or rollback. A capability prefix after
an invalid read is not a valid completed answer.

## Entry, visibility, scan and return

1. Initialize the local maximum and duplicate flag to zero; when output exists,
   write FALSE immediately. An unowned planet (`iplrNone == -1`) or absent base
   then returns zero without reading owner rosters. Other negative owners only
   avoid an invalid owner read when the absent-base guard applies.
2. For a base, reach the owner's roster and selected base. Missing/out-of-range
   owner, pointer or slot are distinct boundaries. There are sixteen possible
   packed base-slot values but only ten physical designs.
3. A foreign viewer (`owner != idPlayer` and `idPlayer != -1`) first reads detail.
   Anything other than `detAll == 7` returns zero; no hull slots are read. Owner
   view and headless `idPlayer == -1` bypass this detail test. They do not bypass
   roster/design access. Arbitrary other `idPlayer` values are compared without
   indexing a player array in this helper.
4. Scan exactly `hul.chs` entries in order. A candidate must have category
   **equal** to `hstSpecialSB == 512`, positive byte quantity, and item 7..15
   inclusive. Combined category masks do not qualify. Candidate warp is item-2,
   hence 5..13; other slots contribute nothing. There is no availability or
   supported-design check here.
5. A larger candidate replaces the maximum and clears the duplicate flag;
   an equal candidate sets it. Smaller candidates preserve it. Two components
   in one slot are one candidate. Two slots matching a lower maximum cease to
   earn the bonus when a later larger singleton replaces them.
6. After an ordinary full scan, write the final flag if output exists and return
   maximum warp. With zero slots, this includes a second FALSE write. The hidden
   and early-return paths only perform the initial output write. If count exceeds
   sixteen on a reached scan, the first out-of-array access is slot sixteen;
   output remains at its initial FALSE before that boundary even if local
   duplicate state has become TRUE. No final output store is modeled there.

`Fold` proves the returned warp is the eligible maximum and the duplicate flag
is true exactly when that positive maximum occurs at least twice, including an
incoming accumulator's contribution. `Visible` proves the zero-initialized result
for a complete visible physical design; `Early` proves guard order. General
postconditions bound capability to zero or 5..13 and require a positive maximum
for a true duplicate flag. Vectors cover quantity-versus-slots, reset-on-larger,
nonmatching masks, zero quantity and item bounds.

## Construction and packet workflows

The base branch of `FBuildObject` first completes eligibility/notice/downgrade
handling. It queries the old planet, updates the base flag/slot and queries the
new design **only if the old warp is zero**. A positive old capability preserves
the existing packet warp/destination, even when the new base has no driver. This
is the inspected source behavior; it is not a promise that the preserved setting
is usable by the replacement base. A zero old capability uses new warp plus its
duplicate bit minus four; a zero new capability clears warp/destination and
invalidates queued packets. Same-slot replacement and no-prior-base paths retain
their actual read order.

`DesignFling` returns before/optional-after capability outcomes and an optional
existing `Fling` decision. A reached undefined read produces no completed fling
decision. This handoff assumes ordinary returning intervening helpers preserve
the projected owner/viewer/designs; actual messages, callbacks, aliasing and
lifetime can require a larger state model. It does not claim to specify every
construction effect or commit/rollback boundary.

Packet creation likewise consumes the actual maximum and duplicate bit through
the existing `PlanPacket` rules. Zero capability and missing destination reject
at separate steps. Source fixtures cover actual packet formation and six
post-charge `Produce` base applications; no refund or annual-state guarantee is
derived from these cases.

## Interaction, quality and remaining qualification

Caller inputs/results are synchronous core values; keyboard/focus, destination
gestures, feedback and accessibility remain M08/M07/M15/S05. The helper has no
internal cancellation point; planning cancellation stays with the consuming
workflow. Determinism is checked through all category words,
all owner/viewer words in declared contexts, byte item/quantity/count domains,
slot positions, order/ties, null output and generated constant bindings. The
native fixture compares complete projected PLANET/SHDEF/PLAYER/GAME byte frames,
global identity and RNG-call absence under ASan/UBSan. It does not prove all
possible slot combinations or native memory refinement.

The normal loop has at most sixteen physical reads in the defined domain; this
is a source/structural bound, not a measured latency target. No performance,
display or assistive-technology conformance target is newly agreed. S06 owns
alias/lifetime and whole-caller frames; S01/M11 admission and real-game reachability;
M19/M18 full packet/base/annual integration; M20 the AI callers. E08-3 still owns
stargate/scanner extraction and destination eligibility, and E08-2 terraforming
selection remains open. The integration fixture uses actual eligibility, lookup, technology, hidden-part,
race-getter and hull-helper bodies with fresh catalog tables. Returning controlled
economy/message/heap/registration/routing providers do not close those separate
contracts.
