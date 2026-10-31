# Milestone C implementation evidence

Tracking: [#67](https://github.com/madebyjeffrey/sdl-stars/issues/67).
Started: 2026-10-31. Branch: `codex/cpp-milestone-c`, targeting `develop`.
Plan: [CPP-MILESTONE-C.md](CPP-MILESTONE-C.md).

## Parent and scope

Parent: `62bb854d143a618419f645abdc4da37d53cb5837`, the qualified B merge
[PR #66](https://github.com/madebyjeffrey/sdl-stars/pull/66). Its tree matches
B receipt `d17f0048ed7608cff140014c01fb5ce0f87ab50b`. C preserves B's behavior,
wire format, independent expectations and regression baselines.

The [parent inventory](CPP-C-INVENTORY.json) is a revision-pinned snapshot,
not a generated view of later C commits. Paths and line numbers identify the
parent tree. It includes every tracked production/ordinary test implementation,
the shared SDK vector hashes, planned renames, explicit path consumers, macro
definitions, state aliases, source-witness programs and actual production
compile-variant counts. Search results are inspection leads; replacement and
verification receipts below decide completion.

## Checkpoint status

| ID | Status | Evidence / remaining work |
| --- | --- | --- |
| C01 | Complete | Parent pinned; source/mechanism/consumer inventory and fresh local parent matrix pass. Cross-platform B receipts remain historical. |
| C02 | Pending | Apply the recorded rename map, migrate every live consumer and qualify fresh configurations. |
| C03 | Pending | Remove 23 API interceptions with equivalent pending-error delivery. |
| C04 | Pending | Introduce typed effect providers and remove test implementation routing/library clones together. |
| C05 | Pending | Migrate all 27 current-source witness programs without losing their independent domains/oracles. |
| C06 | Pending | Replace generated wrapper names with namespaces and qualify Windows SDK isolation. |
| C07 | Pending | Audit all 2,101 parent macro definitions; replace compatibility helpers and record specific retained purposes. |
| C08 | Pending | Remove 156 game-state and 346 UI-state aliases through explicit caller dependencies. |
| C09 | Pending | Finalize compiler/reference policy and remove dead migration adapters. |
| C10 | Pending | Full final-tree audit and Linux/macOS/Windows qualification. |

## Removal and retention inventory

Every occurrence/consumer is listed by category in the JSON inventory. Items
removed by B are historical evidence; items retained by C require the named
continuing purpose. A macro with a permanent purpose can still contain an
obsolete alias; C07 checks its definition and callers before retaining it.

| Family | Parent location and consumers | Disposition / replacement | Completion evidence |
| --- | --- | --- | --- |
| Source language overrides | Root/wrapper CMake; ordinary test globs; AppKit adapters; clone source-property copying | C02: honest `.cpp`/`.mm` implementations; preserve AppKit ARC/blocks | Fresh manifests, desktop/host builds and source-reader checks |
| SDK shared source language | 38 input bodies discovered by `win32-reference-tests.cmake`; ordinary wrapper tests, SDK wrappers and manifest suppressions | C02: unchanged shared `.inc` vectors with ordinary C++ entries; independent SDK C entries remain distinct | Parent hashes identical; real SDK and application contracts both pass |
| Callback call interception | `src/ui/win_calls.h`; `win.h` and intercepted API consumers | C03: ordinary named calls with cleanup-before-error-delivery | Callback fault, nested modal, deferred destruction and primary-failure tests |
| Implementation/call substitution | `src/platform/test_impl.h`, annotated declarations/definitions, `cmake/test-redirect.cmake` and all hook targets | C04: scoped typed effects that can delegate to the real provider | Existing fault, allocation, RNG and workflow outcomes; override restoration |
| Extracted current-source bodies | 27 `tests/spec/check_*_source.py`, their imports and `cpp_production_storage.py` | C05: link production, retain independent interpreters/vectors; delete slicing, surrogate storage and rewriting | Per-witness case/domain/exclusion/oracle/traceability receipt; all 27 accounted for |
| Generated wrapper namespace | `compat/win32/CMakeLists.txt`, `wincompat_namespace.h`, special Windows hook names | C06: actual C++ namespace with explicit SDK qualification | Linux/macOS/Windows link and callback contracts |
| Legacy compatibility helpers | All owned macro definitions, including no-op thunk shims, scalar/message/color helpers and forwarding aliases | C07: typed helper/constants or deletion after checking argument effects | Evaluation count, conversion, numeric/resource contracts |
| Temporary state borrowing | `globals.h`/`globals.c`, UI globals, owner getters, witness state doubles and archived observer | C05/C08: explicit state at owned call chains; narrow foreign entry binding | One owner, stable addresses, unchanged reset/close order and reentrancy outcomes |
| Migration-only build/test tooling | Language flags, clone fixture, compiler audits and temporary SDK compilation adapters | C09: retained strict-policy audits, precise reference exceptions; remove dead adapters | Fresh configure combinations, strict manifests and negative audit cases |
| Transitional documentation | A/B migration/consumer records, active source links, repository instructions, native-port/parity and roadmap | C02 updates live paths; C09/C10 mark historical receipts and document final contracts | Links, source readers, collection verifier and final consumer search |
| Owned C interface / GNU production allowances | Removed by A/B; strict gate already active | Preserve removal; no new C interface or extension allowance | Standalone headers, actual compile commands, independent C boundaries |
| Packed wire overlays / raw ownership | B codecs, typed views/arenas, RAII/session and callback guards | Preserve B's replacement; private runtime geometry is a separate physical-layout requirement | Codec vectors, storage/lifetime tests, regressions and leak/sanitizer coverage |
| Foreign linkage / platform adapters | SDK, SDL, AppKit, Cairo/FreeType and native entry/callback conventions | Permanent named external obligations; `.mm` remains native AppKit integration | Native link, callback containment, picker and resource contracts |
| Independent C / historical numeric evidence | Frozen `tests/reference/cpp-a`; two C layout/payload oracles; `precision_probe.c`/`precision_race.c` and `precision_reference.py` pinned historical revision; independent Linux Mesa allocator C harness | Permanent independent reference inputs and foreign Mesa C ABI/allocator contract; do not rename historical Git paths or convert their expected values | Frozen-byte hashes, C compilation and precision tool unit checks |
| Resource/preprocessing/test instrumentation | Header guards; platform selection; RC/parser IDs; assertion/stringification and `STARS_TEST_*` instrumentation | Permanent preprocessing roles subject to C07's individual audit; numeric aliases still need typed replacements where possible | Resource IDs/bytes, compiler/preprocessor consumers and contract assertions |
| Maintained generators and formal rules | Python/Go generators, resource codecs and canonical Dafny models | Permanent generation/orchestration and independent rules; no current-production body rewriting | Generated-byte contracts, source links and formal verifier when affected |

## Parent qualification

Local platform: macOS arm64; Apple Clang 21.0.0, CMake 4.4.4, Ninja 1.13.2,
Go 1.27.2. CMake selects the required Python >= 3.12 and a build-local environment.
The shell's default Python is 3.9.6, so source witnesses use the configured Python.

Fresh `macos-sdl-debug` configure/build passes with zero diagnostics. Its actual
manifest contains 116 production sources / 1,358 compiled production variants,
including nine generated sources. The source inventory has 110 tracked production
implementations: 105 `.c`, four `.cpp`, one `.m`. Header checks compile 112 headers
in both standard-library include orders. Fresh parent commands/results:

| Command | Result |
| --- | --- |
| `cmake --preset macos-sdl-debug` and matching build | Pass; zero compiler diagnostics |
| `ctest --test-dir dist/macos-sdl-debug --output-on-failure` | 119/119 pass, including all 29 native leaks checks, 17 game unit programs and available tutorial smoke checkpoint |
| `cmake --preset host-release`, build and full CTest | 26/26 pass; SDL-free |
| All `tests/spec/check_*_source.py` with configured Python 3.14 | 27/27 pass; full assertion/exclusion receipts retained |
| `make regression` | 519 matches, 25 accepted unused-storage warnings; zero differences/errors/skipped references through turn 150 |
| `make regression-host` | Same 519/25/0 result through turn 150 |

The full desktop suite takes 330.27 seconds; the host suite takes 6.81 seconds.
No baseline or frozen input changed.

Parent logs and complete comparison/witness receipts are retained under
`dist/verification/c01-parent/` after completion. Linux/Windows B qualification
remains linked from `CPP-B-IMPLEMENTATION.md`; no new C qualification is inferred
from those historical runs.

## C02 implementation and qualification

Pending. Mechanical renames preserve implementation basenames and debug-symbol
names. Shared SDK input contents and independent/frozen references remain exact.
Source body changes, callback protocols, effect seams and state migration belong
to their subsequent checkpoints.
