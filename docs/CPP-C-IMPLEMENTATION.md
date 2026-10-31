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
| C02 | Qualification active | Rename map/live consumers implemented; full local desktop/host/regression/witness gates pass. Collection verification passes; Linux/Windows CI pending. |
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

The filename migration is implemented. All 201 moved bodies are byte-identical
to B: 105 production `.c` to `.cpp`, one production `.m` to `.mm`, 57 ordinary
harness/adaptor inputs to `.cpp`/`.mm`, and 38 shared SDK bodies to `.inc`.
The latter have one-line `.cpp` application entries and independent C11 SDK
entries that include the same vectors. Their 38 parent SHA-256 hashes match.
The five independent C/foreign/historical harness exceptions in C01 remain C.

Root/wrapper language overrides are removed; native AppKit ARC/blocks options
remain. Source lists, dynamic harness paths, unit globs, packaging, wrapper API
auditing, witness paths/temp filenames, historical precision discovery, archived
tutorial parsing and live source/evidence links follow the new extensions.
Historical migration receipts and immutable Git reference paths retain their
original names. Current source-line evidence for shared SDK vectors points to
`.inc`, where the assertions actually reside.

The compiler audit requires `.cpp`/`.mm` for owned implementations and verifies
that every public header has both standalone compile commands. It includes `.mm`
in owned-interface inspection and normalizes SDK exception names for Windows.
Controlled negative manifests prove that missing header compilations and a C
extension on either production or an ordinary test are rejected. The existing
C/C++ redirect fixture now uses honest suffixes for each language; its removal
remains C04/C09 work.

Fresh configuration first caught five dynamically constructed `.c` test path
templates. Comparing parent/current manifests then exposed a generated header
filename typo that omitted 224 header-check compiles. Both are corrected; the
new negative inventory check prevents silent header coverage loss.

Local results so far:

| Check | Actual result |
| --- | --- |
| Fresh `macos-sdl-debug` at `dist/c02-debug` | Build passes without compiler diagnostics; 116 production sources / 1,358 variants; 112 headers in both orders |
| Fresh SDL-free Release at `dist/c02-host` | Build and 26/26 tests pass |
| Full desktop CTest | 119/119 pass, including 29 native leaks checks, 17 game unit programs and available tutorial smoke coverage; 352.24 seconds |
| Full desktop and host regressions | Each matches all 519 baseline files and the exact B parent outputs through turn 150; same 25 accepted unused-storage warnings and zero differences/errors/skipped references |
| All source witnesses | 27/27 pass; complete output, assertion counts and exclusions are byte-identical to the parent receipts |
| Specification links / requirements | 2,812 local links and 700 requirement IDs pass |
| Handbook / traceability unit tests | 26/26 pass |
| Historical precision discovery | Existing `SourceCoverage` test passes; immutable historical C source names retained. The full extended-vs-double experiment is not rerun on arm64. |
| Archived tutorial parser | 640 fragments / 80 pages; highlighted actions accounted for; compile-only observer also builds |
| Flatpak source preparation | Staged export passes; untracked `.cpp`, `.mm` and `.inc` negative probes are rejected |
| Windows SDK / GCC syntax | 38 shared bodies compile independently as C11 against MinGW SDK; all 30 core sources pass GCC 16.2.0 ISO C++20 with the actual production strict-diagnostic policy |
| Wrapper API discovery | 178/178 local API definitions and 6/6 CRT helpers found; source positions regenerated |
| Body / input integrity | All 201 bodies identical to B; frozen references, original/native baselines, data, text, resources and canonical Dafny files unchanged |

The full Dafny collection at the existing 120-second limit finishes with
11,998 verified, zero errors and two timeouts in
unchanged `S02Profile.ParsingVectors` (`spec/dafny/S02-profile.dfy:122`) and
`M08NativeHabitat.AxisRefines` (`spec/dafny/M08-native-habitat.dfy:105`). Focused
300-second retries of their complete include closures pass: 346 verified / zero
errors for S02 and 986 verified / zero errors for M08. Audit of all 111 formal
files completes with zero findings. The full 300-second collection retry then
passes: 12,000 verified, zero errors, zero timeouts; collection audit zero findings.
The earlier 120-second run remains a failure receipt. Canonical formal files
remain unchanged.

The first hand-written MinGW core probe enabled `-Wall -Wextra`, which the core's
production policy does not enable; it stopped on existing unused-variable and
parameter warnings. Repeating the probe with the actual ISO C++20 / `-Wpedantic
-Werror` production policy passes all 30 core files, without changing the code
or its build flags. The failed extra-warning probe is retained separately.

The user approved publication of `codex/cpp-milestone-c` to
`madebyjeffrey/sdl-stars`, with a draft PR targeting `develop`. All local gates
are complete. Linux/Windows CI remains pending; C02 stays in qualification until
its platform results are recorded.

Complete local receipts are kept in `dist/verification/c02/`. C03–C10 remain
pending. This change removes filename/language machinery only; the inventoried
callback, effect, witness-body, namespace and state migrations are subsequent
work.
