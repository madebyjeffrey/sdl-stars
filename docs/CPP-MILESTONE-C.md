# Milestone C: finish the C++ conversion

Status: implementation active; C01 complete; C02 source renames starting.
Evidence: [CPP-C-IMPLEMENTATION.md](CPP-C-IMPLEMENTATION.md).
Tracking: [#67](https://github.com/madebyjeffrey/sdl-stars/issues/67).
Recorded: 2026-10-29; refreshed against merged B on 2026-10-31.
Requested outcome: rename owned implementation files and remove all transitional
scaffolding, without introducing an object-oriented redesign.

## Purpose and prerequisites

Milestone A converted owned application interfaces and consumers to C++20.
Milestone B covers ownership, callback lifetimes, const-correct text, explicit
wire codecs, typed arenas, application/session ownership and strict C++20.
Milestone C makes the resulting application an ordinary C++ codebase whose
filenames, interfaces, tests and build policy no longer depend on migration
machinery.

The consuming developer must be able to navigate, compile and test the actual
production implementation without language overrides, alternate symbol names,
rewritten copies of function bodies or macros hiding state dependencies. The
player and host operator must see the same behavior and file results as after B.

The [completion analysis](CPP-CONVERSION-COMPLETION-ANALYSIS.md) supplies the
historical inventory from active B at
`5fd94fb81bb58840ecada0f9295d992f712f7466`. The post-B planning inspection below
supersedes its pending-B status and revalidates the remaining mechanisms.
The merged `docs/CPP-MIGRATION.md`, `docs/CPP-B-IMPLEMENTATION.md` and
`docs/CPP-CONSUMERS.md` remain the A/B plan and evidence.

Start C from `develop` at `62bb854d143a618419f645abdc4da37d53cb5837`, the merge of
[B PR #66](https://github.com/madebyjeffrey/sdl-stars/pull/66). Its tree matches
B's final receipt commit `d17f0048ed7608cff140014c01fb5ce0f87ab50b` exactly.
Record fresh parent checks in C01. If `develop` advances before C starts,
record the new parent and inspect the intervening changes. C cannot silently
absorb unfinished B work. If B already removes a mechanism listed below,
record its replacement and evidence rather than implementing it again.

## Post-B planning evidence

The final B receipt records all B01–B14 complete, with Linux/Windows CI and local
macOS qualification. It reports 119 desktop tests, 26 SDL-free host tests,
43 sanitizer cases, all 27 source witnesses, 112 standalone public headers and
116 production sources across 1,358 strict compiled variants. Both regressions
match all 519 files through turn 150, with 25 accepted unused-storage warnings
and zero differences, errors or skipped inputs. These are B's recorded results,
not new runs performed for this plan. The available tutorial checkpoints pass;
the full 80-page tutorial remains unfinished and the archived observer is
compile-only checked.

Static inspection of the exact merge tree confirms:

| Mechanism | Remaining work | Checkpoint |
| --- | --- | --- |
| Source extensions and language overrides | `src/` and `compat/win32/src/` contain 105 `.c`, four `.cpp` and one `.m` implementation files. Root/wrapper CMake still overrides languages. Generated sources account for additional compiled production inputs; the tracked count is not the compiler manifest count. | C02 |
| Callback call routing | `src/ui/win_calls.h` still intercepts 23 APIs through `STARS_CPP_CALL`. B's guards and pending-error protocol are the behavior to preserve. | C03 |
| Test substitution | `cmake/test-redirect.cmake` still builds private library clones and rewrites calls/implementation names; it also supplies allocation and SDL fault hooks. | C04 |
| Extracted witnesses | CI discovers 27 `tests/spec/check_*_source.py` programs. They retain source extraction, textual rewriting and/or test compilation machinery that must be classified individually. | C05 |
| Wrapper symbol routing | Wrapper CMake still generates `wincompat_namespace.h` with Windows-only `API` to `StarsCompat_API` macros. | C06 |
| Temporary state access | `APPSESSION`/`APPGAMESTATE` and `APPUISTATE` own state, but `globals.c`/`globalsui.c` publish reference aliases. `globals.h` also has a `STARS_TEST_STATE_DOUBLE` branch for isolated witnesses. | C05, C08 |
| Strict policy and independent evidence | B14 already enforces ISO C++20 with extensions off. Frozen C oracles, SDK reference inputs and private runtime geometry constraints have specific purposes; inventory each before removing adapters or exceptions. | C07, C09 |

This inspection establishes planning scope. It does not complete C01's exhaustive
consumer inventory, compile-command audit or fresh parent qualification.
GitHub issue #65 was still open with a completed B receipt when inspected; #67's
published blocker text/dependency needs housekeeping when tracking is updated.
The implementation prerequisite is satisfied by the qualified merge.

## Scope and invariants

C uses free functions, namespaces, explicit context parameters, value types and
the small ownership/guard types established by B. It introduces no class
hierarchy, virtual service architecture, entity redesign, C++ modules migration,
multi-game feature or concurrency feature. Existing debug-symbol names and
implementation basenames stay; namespace qualification and filename extensions
may change deliberately. Keep owned headers as `.h` in this milestone.

All checkpoints are behavior-neutral:

- Preserve format 2.85, serialized widths/record sizes, UTF-8 byte budgets and
  complete-code-point truncation. Preserve current reader domains and partial
  effects on cancellation or failure.
- Preserve catalog/text IDs, resource bytes and compressed/encrypted payloads.
- Preserve RNG draws, `qsort16` tie order, binary64 operation boundaries and
  integer conversions, including signedness and truncation.
- Preserve stable addresses, handle validation, borrowed lifetimes, reset order,
  callback order, modal cleanup and allocation/fault behavior established by B.
- Keep the core and command-line host independent of SDL; retain the existing
  SDL-backed desktop functionality and isolated native SDK adapters.
- Preserve frozen original fixtures and independent test expectations. Do not
  regenerate a baseline to make a C refactor pass. A newly found bug or desired
  behavior change belongs in a separate change with the repository's changelog,
  specification and baseline requirements.

Behavior remains owned by [S01 persistence](../spec/S01-game-file-formats.md),
[S03 catalogs](../spec/S03-static-game-data-and-text.md),
[S04 platform services](../spec/S04-platform-services.md),
[S05 desktop](../spec/S05-sdl-desktop-implementation.md) and
[S06 shared state](../spec/S06-shared-game-state.md), including their canonical
Dafny models and stated evidence limits. C changes engineering structure, not
those gameplay rules. Moving a witness does not prove whole-program refinement.

## Checkpoints and dependency order

The C IDs are stable implementation/evidence identifiers, not a required number
of PRs. Split large families into focused commits and reviewable PRs. Use
`codex/` branches targeting `develop`; a dependent unmerged PR may target its
predecessor and must be retargeted after that predecessor lands.

| ID | Deliverable | Depends on | Completion evidence |
| --- | --- | --- | --- |
| **C01 — post-B inventory** | Pin the B parent; inventory every migration mechanism, consumer and retained exception. | Merged B | Every item has a removal checkpoint or a specific permanent purpose; no unfinished B work is relabelled C. |
| **C02 — source renames** | Rename owned `.c` → `.cpp` and `.m` → `.mm`; update every consumer and delete redundant language overrides. | C01 | Fresh desktop/host builds, source-reader/tool coverage and compiler manifest prove that all owned sources are found and compiled correctly. |
| **C03 — explicit callback boundaries** | Replace macro-intercepted calls with ordinary C++ functions and the post-B cleanup/error protocol. | C02 | Callback faults, nesting, deferred destruction and modal pumps preserve cleanup-before-delivery. |
| **C04 — typed test seams** | Replace implementation-name macros, call substitution and private library clones with narrow typed effect seams. | C03 | Fault/RNG/UI tests exercise actual production functions, retain real delegation and restore overrides on every exit. |
| **C05 — linked witnesses** | Replace all extracted or rewritten production-body witnesses with linked implementation tests. | C04 | Case/domain/exclusion/traceability inventory accounts for every old witness; independent expectations survive and no production body is copied or rewritten for testing. |
| **C06 — real wrapper namespaces** | Replace generated API-renaming macros with C++ namespaces and qualified calls. | C03, C04 | Linux/macOS/Windows link and runtime checks preserve wrapper/SDK isolation and foreign callbacks. |
| **C07 — remaining compatibility macros** | Remove obsolete shims and replace suitable helpers/constants with typed declarations. | C05, C06 | Evaluation/conversion, resource parsing and numeric-contract evidence match; each retained macro has a named purpose. |
| **C08 — explicit state access** | Finish B's caller migration and delete temporary global-to-context aliases/accessors. | C05, C06 | One state owner, visible dependencies and unchanged initialization/reset/shutdown; observers and tests migrate with callers. |
| **C09 — permanent build and contract policy** | Remove remaining migration-only build/reference adapters and turn audits into maintained C++ policy. | C02–C08 | Fresh configure matrix, strict diagnostics, self-contained headers and independent references pass with a precise exception inventory. |
| **C10 — final removal and qualification** | Audit the whole tree, remove dead migration material and qualify the final revision. | C01–C09 | All exit conditions and verification below pass; no transitional entry or required check remains pending. |

Implement in this order by default. Families may be interleaved within the same
reviewable change when a dependency requires it; preserve buildable, testable
boundaries and record which checkpoints that change satisfies.

## Proposed delivery batches

Keep C01–C10 as the stable completion IDs. The following batches describe
reviewable work; split the witness and state families further when needed.
Each implementation PR targets `develop` and carries focused commits plus its
actual qualification receipt. This grouping is a proposal, not a fixed PR count.

| Batch | Work | Required result before dependent work |
| --- | --- | --- |
| 1 | C01 inventory and C02 mechanical renames | Exact source/consumer/exception lists; fresh desktop and SDL-free host configurations find every source; all 27 witnesses still run after path updates. Keep semantic edits out of rename commits. |
| 2 | C03 explicit callback delivery | Replace all 23 intercepted calls and remove `win_calls.h` routing. Nested modal, unowned callback, teardown and primary/secondary failure tests retain cleanup-before-delivery. |
| 3 | C04 typed effect seams | Migrate file/publication/UI decisions, RNG and SDL/allocation faults in bounded families. Real production delegation and scoped override restoration pass before each clone/hook is deleted. |
| 4 | C05 linked witness families | Migrate settings/platform first, library/file workflows next, then production/research/colony/construction and remaining UI/game cases. Every old program maps to linked tests, its original input domain, exclusions and independent oracle. |
| 5 | C06 wrapper namespaces | Replace generated name macros across declarations, definitions and consumers; qualify native SDK boundaries explicitly. Windows SDL/SDK linking and callback tests are mandatory. |
| 6 | C07 compatibility helpers | Replace obsolete shims and suitable macros after their callers move; prove numeric conversions/evaluation behavior and unchanged generated IDs/resource bytes. |
| 7 | C08 explicit state consumers | Migrate core/host call chains, then UI/callback/observer consumers in bounded families. Delete aliases and witness state doubles as their last consumers move; preserve initialization/reset/close behavior. |
| 8 | C09 permanent policy and C10 final qualification | Remove dead adapters/scripts, enforce exact permanent exceptions and qualify the final revision on Linux, macOS and Windows against B and the native baseline. |

Use one small linked witness as the first C04/C05 integration check before
expanding the effect interface. Supply only the dependencies that its production
function actually consumes. Test effects may delegate to their saved real
provider; they must not recurse through an override or implement a second game
algorithm. Preserve independent expected results even when the test fixture
changes shape.

For C08, keep B's owner and propagate explicit state references through existing
function families. Resolve foreign callback entry through the existing logical
target/session lifetime, then pass state into owned calls. Review a concrete
core/host family and a reentrant UI family before spreading the pattern. A
per-function lookup of the global session would retain the hidden dependency
that C08 is intended to remove. Domain-owner decomposition remains prospective
D work.

The two largest planning uncertainties are witness dependency breadth and the
number of state consumers. C01 records these inventories before assigning batch
size. Failure to preserve an oracle/domain or callback cleanup path leaves the
owning checkpoint pending; it is not a reason to weaken the exit gate.

Planning produces no new gameplay rule or formal definition. Existing canonical
Dafny rules remain authoritative. Implementation changes to evidence paths,
test IDs or source signatures must update traceability with the corresponding
family and run the checks required by `spec/AGENTS.md`.

## C01: inventory and evidence ledger

Create `docs/CPP-C-IMPLEMENTATION.md` when implementation begins. Record the B
merge SHA, platform/toolchain versions, parent regression results and the C
checkpoint statuses. Link C from the existing migration plan and consumer ledger
when this document joins the migration branch.

For every mechanism record: location, purpose, all known consumers, replacement,
deletion checkpoint and verification. Classify each as removed by B, to remove
in C, or permanent with a specific non-migration purpose. Include language/source
overrides, generated symbol aliases, test substitution, source extraction,
temporary state access, suppressions, generated adapters and one-off scripts.
Inspect tracked sources and actual compile commands; searches alone do not prove
that generated or platform-only consumers are accounted for.

The post-B planning count confirms 105 production `.c` files and one `.m`, plus
ordinary tests, and 27 CI-discovered witness programs. C01 expands those counts
into explicit path/target/consumer lists, including generated and platform-only
inputs. Recompute if the parent changes; additions must not be lost.

## C02: rename files and all consumers

Keep directory layout and basenames. Rename owned production, ordinary harness
and native-adapter implementations to `.cpp`/`.mm`; generated implementations
must have the same honest extensions. Keep intentional independent C references
and historical snapshots classified separately.

Update CMake source lists/globs, CI, packaging, private adapter inputs, source
evidence links and documentation together. Specifically inspect:

- Root/wrapper CMake `LANGUAGE CXX` and `LANGUAGE OBJCXX` lists.
- `tests/spec` hardcoded implementation paths and temporary witness filenames.
- `tests/scaffold/precision.py` source discovery and `precision_turns.py` copy
  filters; preserve intentionally pinned historical inputs.
- `tools/build_flatpak.py` untracked-source validation, including `.cpp`/`.mm`.
- The archived tutorial parser's `tutor.c` path and literal function boundaries,
  observer declarations and active tutorial source readers.
- SDK shared test-body inputs, generated resources and documentation publishing.

Delete redundant language overrides in the rename change. Preserve still-needed
source-specific options such as AppKit ARC/blocks. Keep C++ compilation policy
strict and leave historical evidence paths alone when they refer to old commits.
Validate from fresh build trees so cached source properties cannot hide omissions.

## C03–C04: explicit calls and typed effects

After reviewing B02's result, eliminate `win_calls.h`'s `STARS_CPP_CALL`, API
`#undef`/`#define` routing and equivalent migration wrappers. Put pending-error
delivery at named owned C++ boundaries. Propagate directly only where every
intervening frame and cleanup path permits it. Preserve containment at genuine
foreign callbacks and preserve the primary failure during unwinding.

Replace `STARS_TEST_IMPL`, alternate implementation declarations and
`stars_test_redirect`/private-clone machinery through narrow seams for effects
such as random draws, file operations, publication and UI choices. Use existing
module boundaries where practical; typed function providers or explicit context
parameters suffice. Production and tests must compile the same implementation.
Tests may supply effects and observe receipts; they must not substitute a second
algorithm. Scoped overrides must restore correctly through nesting, callbacks
and exceptions and must not become a global mock registry.

Delete each macro, clone variant and alternate declaration as its last consumer
moves. Preserve real-function delegation, deterministic RNG tests, allocation
faults and partial-effect checks. Callback coverage must include nested modal
owners, unowned timer/window failures, deferred destruction, restart, no commands
after failure and no hung pumps. A renamed wrapper around the same textual
substitution does not satisfy C03 or C04.

## C05: migrate witnesses without losing independent evidence

For each witness record its old cases, input domain, exclusions, expected-result
oracle, canonical Dafny links, source seam and replacement executable/test ID.
Keep the independent oracle and vectors; link the actual production implementation
through C04's seams. Small internal free-function seams are acceptable when they
remove extraction without broadening the public game API.

Remove signature/brace slicing, reconstructed substitute implementation types,
body rewriting such as native-file-to-stdio replacement, forced migration
includes and obsolete `-x c++` workarounds. A newly written copy of the production
algorithm is not a replacement test. Test-side fixtures, independent reference
types and Python orchestration may remain for documented purposes.

Migrate in bounded families: settings/platform, file/library/race workflows,
production/research/colony/construction, and remaining game/UI witnesses. Update
test registration, specification evidence links and verification entry points
alongside each family. No old case or exclusion disappears without an explicit
coverage accounting; mark any genuine gap as incomplete C work.

## C06–C08: namespaces, macros and state

Put owned wrapper APIs in a real C++ namespace, using explicit qualification at
ambiguous SDK/foreign seams. Remove generated `API → StarsCompat_API` renames and
their special cases in tests/build scripts. Preserve function basenames and
keep SDK adapters isolated; do not mix wrapper handles with native SDK handles
or rely solely on mangled symbols to establish type isolation.

Audit all remaining owned compatibility macros. Replace byte/word/message/color
helpers and ordinary forwarding aliases with typed functions or constants where
appropriate. Preserve evaluation count, integer/pointer conversion, signedness,
truncation and accepted arguments. Remove obsolete `MakeProcInstance`,
`FreeProcInstance`, no-op shims and dead callers, checking argument effects.
Do not widen stored flags to `bool` or redesign enums for stylistic consistency.

Resource IDs and constants read by `.rc` inputs or generator parsers need a real
replacement before macro deletion. Generate required resource/C++ forms from one
source where useful; preserve IDs and output bytes. Pointer-tagging helpers need
explicit treatment rather than a promise that every operation is `constexpr`.

Finish B13's state transition with explicit references/parameters within owned
code. Keep a narrow, documented application entry boundary where necessary, but
remove global-name macros, reference aliases and forwarding accessors retained
solely to avoid migrating callers. Do not introduce a generic service locator
that conceals the same dependencies. Preserve one owner and teardown ordering;
update observers/parsers in the same changes as their inputs.

## C09: retained mechanisms and permanent policy

The final ledger must justify individual retained mechanisms. Allowed purposes
are not a blanket exception for anything still named compatibility or test code.

| Retained mechanism | Required justification |
| --- | --- |
| Foreign headers, entry/callback linkage and native SDK adapters | A named external ABI or platform obligation; no owned parallel C interface. |
| Objective-C++ AppKit glue | Actual native platform integration, compiled as `.mm`. |
| Frozen C inputs, SDK references and original fixtures | Independent historical/wire/foreign-contract evidence, isolated from application C++ declarations. |
| Header guards, platform selection, resource and test assertion macros | A concrete preprocessing need; not application symbol/state routing. |
| File/UTF-8/sort boundaries, RAII owners and typed helpers | A continuing behavior, lifetime or conversion contract. |
| Python/Go tools and Dafny models | Maintained generators, orchestration, independent analysis or canonical rules; no transitional rewriting of production bodies. |
| SDL-backed Win32 module | Current desktop functionality; its existence does not permit retaining migration macros. |

Retire SDK include wrappers created solely to bypass temporary source-language
properties. Intentional C tests can have ordinary `.c` entry points and shared
test inputs where justified; other SDK tests may compile as C++. Preserve the
independence of reference expectations and never convert frozen evidence merely
to eliminate the appearance of C.

Simplify compiler audits to verify actual C++20/Objective-C++20 commands,
extensions disabled and strict diagnostics on owned targets. Recognize `.mm`
and generated sources; exercise host-only and platform-specific builds. Delete
dead C-only options and migration suppressions; any remaining compiler exception
must be narrow, explained and covered by a contract check. Enable C only where
remaining reference targets/dependencies require it.

Keep standalone header checks and enum/constant/wire/catalog/resource contracts.
As B separates runtime layout from wire layout, replace obsolete comparisons
with equivalent independent codec coverage rather than silently reducing tests.
Use a precise exception list and negative audit cases so new owned `.c`/`.m`
sources, symbol rewrites or accidental C interfaces cannot slip back in.

## Verification, exit gate and delivery

Every implementation batch follows the repository verification rules. Build the
platform SDL Debug preset with no new warnings; run `make regression`,
`make test-unit` and `make tutorial`. Shared/game changes also run
`make regression-host` and `make test-host`. Run relevant Release builds and
the full CTest contract/workflow suite beyond unit-only entry points. Renames
and final policy changes require the complete desktop and host matrix.

Qualify Linux, macOS and Windows, with relevant callback/allocation/reentrancy
faults, sanitizers and native leak checks. Exercise ordinary tests, SDK tests,
both and neither where supported, including an SDL-free host configuration.
Run every migrated witness and preserve canonical Dafny verification; if formal
files or specification traceability change, follow `spec/AGENTS.md`, verify/audit
the affected include closure and run the collection verifier.

Compare against both the exact B parent and the unchanged native baseline.
The analysis records 519 files across nine scenarios through turn 150 and 25
accepted unused-storage warnings on its parent/platform; C01 must establish the
actual post-B results. Record actual comparisons, warnings, skipped inputs and
the first unexpected diff. Missing tooling/platform evidence leaves the relevant
checkpoint pending rather than qualifying it by inference.

C is complete only when:

1. Every checkpoint and every transitional inventory item has recorded removal
   evidence. The only remaining exceptions have specific permanent purposes.
2. Owned production and ordinary test implementations, including generated and
   platform-only sources, use `.cpp`/`.mm` with no migration language overrides.
3. There is no owned C/C++ dual interface, generated macro namespace, callback
   call interception, implementation-symbol substitution or private test clone
   machinery left from the conversion.
4. Tests exercise linked production functions; extracted/rewritten substitute
   implementations and their migration compiler workarounds are gone. The full
   witness/oracle/domain and Dafny traceability inventory is accounted for.
5. Temporary state aliases and obsolete compatibility shims are gone; dependencies
   and genuine foreign boundaries are explicit.
6. Strict C++20, standalone headers, independent contracts and the full platform
   verification matrix pass on the final revision with no unexpected regression
   changes or unreported coverage gaps.
7. Active build/agent guidance, roadmap, migration/consumer inventories, source
   links and relevant native-port/parity documentation describe the final code.
   Historical plans remain clearly historical; one-off migration scripts and
   dead configuration are removed after their consumers are gone.

Keep focused commits and publish actual scope, evidence, limits and rollback in
each implementation PR. Revert dependent changes in reverse order if necessary;
no irreversible file conversion is part of C. Open tracking issues when work is
scheduled under `docs/agents/issue-tracker.md`; this plan does not publish tickets
or begin implementation.

This document records planned work and completion obligations only. No C code
changes, new behavior, completed checkpoint, new proof or runtime qualification
is claimed by publishing the plan.
