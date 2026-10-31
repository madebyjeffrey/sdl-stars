# M08 — Terraforming availability, selection and native restoration

Observed behavior inspected 2026-10-26 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working specification
collection. This companion specializes [M08-US03](M08-planet-and-colony-management.md#m08-us03)
and the shared habitat inputs of [M08-US02](M08-planet-and-colony-management.md#m08-us02).
A player assesses possible environment changes; the host selects and applies
queued work under [M19-US01](M19-game-rules-and-resolution.md#m19-us01).
M08 owns capability/bounds/selection; M11 availability; S03 catalog rows; M04
safe race habitat rules; S04 binary64; S06 live storage/alias/lifetime; M19
construction and remote helper use; M09 charging/queues and M18 annual ordering.
The split is specification ownership, without an approved runtime refactor.

Canonical definitions:
[M08TerraformSelection](dafny/M08-terraform-selection.dfy),
[M08NativeHabitat](dafny/M08-native-habitat.dfy) and
[M19ConstructionEffects](dafny/M19-construction-effects.dfy) `SelectTerraform`/
`RunTerraform`. `BoundsRefine` connects native bounds to existing M08 `Terraform`;
native habitat `AxisRefines` proves agreement with M04's safe axis formula and
preserves widths around its canonical `DesireExpr` and S04 evaluation.
Stories/criteria remain at the module entries. No runtime, format,
catalog, saved-design or baseline change is made.

Sources: [planet.cpp](../src/core/planet.cpp) `FCanTerraformLppl`, `IBestTerraform`,
`PctPlanetDesirability`; [turn2.cpp](../src/core/turn2.cpp) `IBestRemoteTerra`,
`FBuildObject`; [parts.cpp](../src/core/parts.cpp) `FLookupPart`, `TechStatus`;
[research.cpp](../src/core/research.cpp) hidden-part rules;
[race.cpp](../src/core/race.cpp) getters;
[structs.h](../src/core/structs.h) PLANET/PLAYER/PART/TERRA;
[terraformers](../data/parts/terraformers.yaml). The
[collection evidence](README.md#terraforming-selection-continuation) records actual checks.

## Inputs, outputs and native domain

`Planet` projects signed-word owner and three signed-byte current/original axes.
`Player` couples the existing M11 availability context to three signed-byte
preferred/minimum/maximum habitat vectors. Sixteen physical player entries are
distinct from the active-game count. TERRA rows contain signed-word ability and
unsigned-word resource cost. The twenty metadata rows align by index with M11's
terraform catalog category; the native fixture exports fresh requirements,
ability and cost, rather than assuming model literals equal generated data.

Caller arrays have three signed-word cells each. `Option<I16>` distinguishes
known incoming values from uninitialized/unobserved cells; neither is an implicit
zero. Arrays and live planet/player/catalog storage are disjoint, aligned and
large enough. Aliases can affect later source reads because costs and bounds are
written during the call; S06 retains general alias/lifetime refinement. The
ordinary callers use distinct stack arrays, but this is not universal native
memory verification or proof of every file/UI state's reachability.

Done denotes an ordinary return, Undefined a reached native arithmetic/read
boundary. Pending means a required admissible S04 floating witness has not been
supplied; it is not a native error return or rollback. Neither status allows the
model to invent a choice. Preserved prefixes identify current global identity,
temporary environment and, for remote calls, the substituted player projection.

## Ordered catalog and partial-output workflow

`Can` saves `idPlayer`; only a headless saved value -1 is replaced with the planet
owner. A regular viewer therefore queries its own technology/habitat, even for
another owner's planet. `IBestTerraform` separately forces the owner before
calling Can. Neither fact is a generic ownership validator.

`Find` scans available status **1** in four separate descending groups:
7..0 for all axes, then 11..8, 15..12 and 19..16 for individual axes. Each group
stops at its first available item. A higher-ability lower-index item is not
searched after success. `Replies` derives statuses and per-lookup query traces
from the actual canonical M11 lookup under the selected context; it does not
assume a Boolean capability provider. Mathematical replies for uncalled rows do
not add native calls. The local PART's unobserved initial quantity/prior pointer
is not asserted to contain initialized numeric storage.

An all-axis success initializes all three abilities and costs, even for a raw
zero/negative ability. An axis-specific success overrides only when its ability
is **strictly greater** than the current axis ability. Equal ability retains
the earlier cost. Unsigned resource cost is assigned through a signed-word cell,
so 65535 becomes -1. No all-axis success leaves initial abilities zero and costs
untouched except for stronger individual overrides.

If nothing sets the preliminary found flag, FALSE returns before writing any
bound cells. If found, all three axes are processed and bound cells are written,
even when the final outcome is FALSE because no endpoint survives. These FALSE
outcomes must not be conflated. Unknown cost cells on unmodified axes remain
unknown; callers must not infer initialization from a TRUE return on another axis.
`Find`, `Upgrade`, `Prepare`, `Unavailable` and `BoundsLoop` own these rules.

The first lookup still reads a non-headless selected player before row validation
through M11. A headless selected value -1 may complete the catalog scan, then
reach invalid habitat storage at the first nonzero-ability axis. Zero abilities
skip that habitat read. Written costs and earlier bound cells remain prefix
effects; no restoration after a native undefined read is claimed. Ordinary
returns restore the saved global identity.

## Endpoints and habitat evaluation

`NativeBounds` first narrows original environment minus/plus signed ability to
signed-word storage, tests those raw endpoints against the current byte value,
then clamps surviving low/high to 1/99. A zero ability or minimum marker
`envImmune == -1` disables that axis. Helpful movement keeps the preferred side
and stops at the preferred value. Harmful movement chooses the farther endpoint;
ties prefer the low one, while no farther endpoint disables both. Sentinels are
native -1 values, not arbitrary logical endpoints. Raw byte/ability inputs can
therefore differ from the normal-domain scalar rule. `BoundsRefine` proves
agreement for current/original 1..99, ideal 0..100 and ability 0..30.

Native habitat deliberately tests **maximum < 0** for immunity; the capability
routine tests **minimum == -1**. These coincide for ordinary immune race designs,
but the specification preserves their distinct raw-field predicates.
`M08NativeHabitat.Axis`/`Accumulate` preserve integer accumulation, signed-word
percentage/penalty temporaries, unsigned positive accumulation and signed
modifier multiplication/division boundaries. A reached zero width or overflowing
signed intermediate is Undefined, without a proposed safe-clamping policy.
Negative habitat returns the narrowed negative penalty. Positive habitat uses
the existing M04 sqrt/bias expression, requires an admissible S04 binary64 trace
and int32 conversion, then applies the modifier/product/division and word return.
A mathematical square root does not establish native binary64 refinement.

## Best-axis selection and temporary state

`Best` returns zero immediately for owner -1, preserving the saved identity and
making no catalog/desirability call. Otherwise it selects the owner, runs Can,
and restores the saved identity on ordinary FALSE. TRUE evaluates current
desirability once, then processes axes in gravity/temperature/radiation order.
For each candidate, prefer the low endpoint if present, save the original byte,
assign the endpoint through signed-byte storage, evaluate desirability under the
owner and compute its score. An axis without a candidate receives score zero.

`Score` narrows the desirability difference to int16, applies the native signed
absolute assignment, narrows **100 times that value before division**, divides
by the absolute difference between the saved byte and the original signed-word
endpoint, adds one and stores a signed-word score. For a supplied helper delta
400 and distance ten, this yields -2552 rather than 4001. Such provider cases
test controller arithmetic; they are not claimed outputs of ordinary habitat.
A zero distance is a native division boundary after the temporary environment
assignment and helper call. Ordinary per-axis completion restores that byte.

`Winner` uses strict greater-than updates, preserving the first axis on ties.
The selected low endpoint returns -(axis+1); otherwise return axis+1. An
ordinary Best return restores all environment bytes and saved global identity.
Failed/Pending evaluation or score division retains the selected owner and
current trial byte as an observed prefix, without asserting a C post-state past
undefined behavior. Missing float evidence is not reported as successful restoration.

## Optimum preview and viewpoint

`Optimum` calls Can with helpful TRUE under the saved global viewer, letting
Can select the owner only for headless -1. It does not force the requested
player's technology. On FALSE it directly evaluates the unchanged planet for
the requested player, with no temporary environment writes.

On TRUE it saves all three current bytes. For each axis, the requested player's
minimum marker determines immunity and its preferred value determines direction.
Only a Can endpoint that lies farther along that direction is used; choose the
endpoint or preferred value, whichever is closer to the starting current value,
then assign through signed-byte storage. `OptAxes` owns the ordered updates.
After all axes, evaluate desirability using the saved global `idPlayer`, then
restore all saved environment bytes on ordinary return. The technology viewer,
requested preference player and evaluation player are separate inputs.

With a headless saved identity and TRUE Can, that final player index is still
-1; its native habitat read is an explicit boundary after the temporary changes,
not a safe evaluation for the requested player. A nonordinary habitat evaluation
does not imply restoration. This is an observed call-domain distinction, not a
claim that a supported UI path passes an invalid player. Native asymmetric
multi-player fixtures compare the actual evaluation identity/environment and
complete restored state for defined cases.

## Remote substitution and construction handoffs

`IBestRemoteTerra` snapshots the target owner's complete PLAYER, copies the
acting player's PLAYER into that slot, restores the owner's three habitat vectors
inside the copy, calls Best and restores the complete snapshot on ordinary return.
Thus technology/traits come from the actor and habitat preferences from the
owner. Owner snapshot and actor copy are reached array reads before Best's
unowned early-return guard; this caller cannot safely use owner -1 as that guard.
`Remote` captures the projected substitution/restoration and leaves the injected
projection on a nonordinary prefix. Native frame comparisons check the complete
PLAYER bytes for the exercised ordinary cases; broader alias/escape refinement
remains explicit.
The helper does not validate actor permission, relations, range or remote-fleet
eligibility; those remain with the owning M19/M10 task workflow.

Construction always asks for helpful terraforming. `SelectTerraform` feeds the
actual Best result into existing `TerraformStep`, which applies one signed
increment/decrement clamped to 1..99 and preserves zero choice as an iteration
without an environment change. `RunTerraform` repeats selection against updated
environment for the existing unsigned count projection; it does not stop early
when a choice is zero. On ordinary completion the exact requested iteration
count has been consumed. Its call/remaining conservation includes a started
nonordinary iteration without calling it completed work. Returning message/helper
state preservation is a declared integration condition; M09 paid work, M15
delivery and M18 whole-year effects are separate.

## Interaction, quality and evidence limits

These synchronous core helpers have no local cancellation point or new UI.
Planning controls, focus/keyboard/feedback, accessibility, text and display
qualification stay with M08/M09/M15/S05. No latency, memory or accessibility target
is invented. Catalog selection has at most twenty returning lookups and ranking
at most four desirability evaluations per iteration; these are structural call
bounds, not measured performance guarantees or helper liveness proofs.

The source fixture compiles actual bodies with native common.h layouts, fresh
catalogs, warnings-as-errors and ASan/UBSan. It combines actual lookup/technology/
race/habitat with Best, Remote and FBuildObject; independent scripted modes
exercise all availability subsets and raw ranking values. The observer supplies
initialized local output buffers before Can so untouched cells can be inspected;
Best ignores costs and never reads those arrays after FALSE. It compares outputs,
catalog/desirability call traces, whole PLAYER/PLANET/GAME byte frames and RNG-call
absence. This observation setup does not prove arbitrary output aliases or every
uninitialized native stack state.

Remaining work: S04 complete native float-trace production, S06 pointer/alias/
lifetime and caller frames, S01/M04 admitted race-state mapping, M08 optimum-value
caller forecast/copy integration, M19 full remote
fleets/annual effects and M09 full budget/application publication. The optimum
controller and its viewpoint distinction are specified above. Fixture domain exclusions are not new
runtime validation. Proposed stronger validation, policy and qualification
decisions retain their existing owners.


The subsequent [capability companion](M08-colony-capabilities.md) now specifies
`IpctCanTerraformLppl` signed aggregation and its actual Produce validation/
builder/application handoff. Can/bounds remain canonical here. That closes this
named scalar/controller gap without closing whole M09/M19 annual integration.
