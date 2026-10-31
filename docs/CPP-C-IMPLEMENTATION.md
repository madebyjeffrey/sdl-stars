# Milestone C implementation evidence

Tracking: [#67](https://github.com/madebyjeffrey/sdl-stars/issues/67).
Started: 2026-10-31. Initial branch: `codex/cpp-milestone-c`, targeting `develop`.
Current continuation: `codex/cpp-c09-build-policy` from C08 PR #75
(`381d61a`); the dependent PR chain starts at C04/C05 PR #72 against `develop`.
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
| C02 | Complete under current CI policy | PR #68 merged at a65e653; all 15 enabled CI jobs pass. The full Linux desktop and Windows SDL jobs remain explicitly deferred. |
| C03 | Complete under current CI policy | PR #69 merged at ab62fef; all enabled CI passed. Local 119 desktop / 26 host tests, callback ASan/UBSan faults, regressions and 27 unchanged witness receipts pass. |
| C04 | Complete under current CI policy | Every implementation/call redirect, allocator hook and private library clone is replaced by typed module effects. Desktop/host workflows, regressions and full formal verification/audit pass. |
| C05 | Complete under current CI policy | All 27 commands use normal linked production targets; every parent case/assertion/exclusion receipt matches. The 67 retained independent Python definitions have identical parent ASTs. See the [per-program inventory](CPP-C05-WITNESSES.md). |
| C06 | Complete under current CI policy | All 23 C03 names use `ui::` directly; raw operations and actual type tags use `win32::`. Generated names are deleted; [boundary/vector receipts](CPP-C06-BOUNDARIES.md) account for every entry and all 38 shared SDK inputs. |
| C07 | Complete under current CI policy | All 2,101 parent macros have individual dispositions; typed helpers/constants, direct calls and shim removal pass full desktop/host/regression/sanitizer checks. |
| C08 | All enabled CI passes; manual platform run pending | All 502 inventoried aliases plus two earlier XOR-seed aliases and their accessors are removed; owned calls receive explicit references, with target-bound foreign callbacks. See [state receipts](CPP-C08-STATE.md). |
| C09 | Local qualification active | Checked-in SDK C entries replace generated adapters; permanent strict/reference policy, negative cases and fresh configuration registries pass. |
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
| C03 prefixed entry functions | `compat/win32/include/wincompat_calls.h`, `compat/win32/src/calls.cpp` and their consumers; introduced after the pinned parent inventory | C06: qualified `ui::` operation names, consolidated implementations and deletion of redundant forwarders; no old-name aliases | All 23 entries accounted for; no active old API consumers; exact signatures and cleanup-before-error-delivery preserved |
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
`madebyjeffrey/sdl-stars`; [draft PR #68](https://github.com/madebyjeffrey/sdl-stars/pull/68)
targets `develop`. All local gates are complete.

On 2026-10-31 the user clarified that only the longest CI jobs should be deferred.
The initial broad workflow pause was corrected: only the full Linux desktop
`unit` job and `windows-sdl` are opt-in. Select `run_slow_platform_jobs` on the
unit workflow's manual dispatch to run them. Host unit tests, source witnesses,
sanitizers, all regressions and tutorial checks retain automatic triggers;
documentation and release publishing also retain their triggers.

The two latest successful B runs measured Linux desktop at 19m11s–19m23s and
Windows SDL at 25m54s–26m58s. Host units took 1m39s–2m01s, source witnesses
1m58s–3m04s, regressions 1m25s–3m59s, sanitizers 4m57s–7m46s and tutorial
12m26s–12m45s. Sources: unit runs
[41154569047](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41154569047),
[41132249095](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41132249095),
plus matching regression/tutorial runs recorded in B's qualification receipts.

Local verification remains required. Evidence from the two deferred jobs is not
claimed as passing and does not block the next C checkpoint during this pause.
The planned final platform qualification remains recorded for when those full
desktop suites are prioritized again.

Complete local receipts are kept in `dist/verification/c02/`. C03–C10 remain
pending. This change removes filename/language machinery only; the inventoried
callback, effect, witness-body, namespace and state migrations are subsequent
work.

## C03: explicit callback boundaries

Parent: `a65e653afef2efb70f69ecf1dd3d12de41664ac7`, the merge of
[PR #68](https://github.com/madebyjeffrey/sdl-stars/pull/68). All 15 enabled
C02 jobs passed; the two full desktop jobs were skipped under the recorded
user priority. C03 uses `codex/cpp-c03-callback-boundaries` and targets `develop`.

`compat/win32/include/wincompat_calls.h` declares 23 ordinary C++ functions,
implemented in `compat/win32/src/calls.cpp`. Each calls its underlying raw API,
then delivers the pending exception after that operation returns. Compile-time
signature assertions retain exact return types, arguments and pointer widths.
The dialog functions accept the existing zero initialization argument by default;
former `CreateWindow` calls pass their original zero extended style explicitly.
No owned dispatch frame or callback cleanup is bypassed.

| Boundary family | Explicit entry examples |
| --- | --- |
| Window/dialog creation and modal lifetime | `HwndUiCreateWindowEx`, `HwndUiCreateDialog`, `LUiDialogBox`, `FUiDestroyWindow` |
| Dispatch, messages and translation | `LUiCallWindowProc`, `LUiDispatchMessage`, `FUiGetMessage`, `FUiPeekMessage`, `LUiSendMessage`, `LUiSendDlgItemMessage`, `FUiIsDialogMessage`, `IUiTranslateAccelerator` |
| Window/control effects | `FUiShowWindow`, `FUiEnableWindow`, `HwndUiSetFocus`, `FUiSetWindowPos`, `FUiMoveWindow`, `FUiUpdateWindow`, `FUiSetWindowText`, `LUiDefWindowProc` |
| Popup and native file selection | `FUiTrackPopupMenu`, `FUiGetOpenFileName`, `FUiGetSaveFileName` |

All 931 production UI API references now name the checked functions; 1,329
references moved in total including ordinary units and UI harnesses. Strings,
comments, debug-symbol function/field names and calculation bodies are preserved.
Boolean message-pump truth tests follow the repository's bare `F…()` convention.
Five source-witness programs rename their existing UI effect doubles to match;
their independent algorithms, cases, domains and exclusions are unchanged.
All 27 complete witness receipts are byte-identical to C02.

`src/ui/win_calls.h`, `STARS_CPP_CALL`, `StarsCFunction`, `StarsCall_*` and their
include-dependent alternate-declaration guards are removed. There is no new
source-call redirection. The new translation unit participates in the existing
wrapper variants without adding hooks or clone kinds. C04 still owns removal
of those test variants, and C06 still owns the generated Windows symbol aliases.
The user-approved C06 plan also replaces all 23 prefixed names with `ui::`
operation names and removes unnecessary forwarding functions. The names above
describe C03's current implementation, not the final public API.

Runtime internals retain their raw calls. Their guards can notify or retire
windows while unwinding; delivering a captured error inside those frames would
run before the outer cleanup completes. Existing foreign callback adapters,
pending-error state, primary-error preservation and shutdown/reset policy are
unchanged. The raw function pointers in the callback fixture are intentional
coverage of containment before the owned checked boundary delivers the failure.

The real callback fixture retains its nested/modal/modeless, allocation, timer,
unowned window and no-post-failure-command cases. Three additional faults throw
from text, position and enable notifications, proving that the new checked calls
deliver the error after callback destruction and preserve a usable owner. The
fixture restores an explicitly disabled owner after the test; it does not assert
or introduce transactional rollback of existing partial window effects.

Local qualification:

- SDL Debug and Release builds: zero compiler diagnostics. The actual Debug
  manifest has 117 production sources / 1,376 variants and 112 headers in both
  include orders; the removed private header is replaced by the public one.
- Full desktop suite: 119/119 pass in 320.12 seconds, including all 29 native
  leak checks, 17 game unit programs and available tutorial smoke coverage.
  The real callback fixture also passes under ASan/UBSan with no diagnostics.
- `make test-host`: 26/26 pass. Both full desktop/host regressions match all
  519 baseline files through turn 150, with the same 25 accepted unused-storage
  warnings and zero differences/errors/skipped inputs.
- Direct comparison against C02 desktop/host outputs also matches all 519 files
  in each suite, with zero differences/errors/skipped inputs.
- All 27 witnesses pass, with byte-identical complete assertion/exclusion
  receipts compared with C02.
- Local MinGW/GCC syntax checks pass all 23 exact signatures, the existing
  generated Windows API isolation and the popup test-provider variant.
- Specification links: 2,812 links / 700 IDs, zero errors. Formal inputs are
  byte-identical to C02; its proof receipts remain historical evidence.
- Core/host/platform-service sources, wire layouts, data/text/resources, all
  canonical formal files and frozen reference inputs are unchanged. No new
  gameplay behavior, proof or whole-program refinement is claimed.

Complete local receipts are saved under `dist/verification/c03/`.
[PR #69](https://github.com/madebyjeffrey/sdl-stars/pull/69) merged at
`ab62fef99f86348f3f666443f518e47ad72dca45`; all enabled CI passed. The two full
platform jobs retain the user's opt-in policy. C04–C10 remain subsequent work;
no baseline was regenerated for this refactor.

## C04: first effect family and C05 integration

Parent: `ab62fef99f86348f3f666443f518e47ad72dca45`, the qualified C03 merge.
Branch: `codex/cpp-c04-typed-effects`, targeting `develop`. This is the plan's
first bounded family and linked-witness integration, not completion of C04/C05.

`FSaveProfile` moves without algorithm changes to the private
`compat/win32/src/profile_save.cpp` translation unit. Its explicit `PROFILEIO`
argument supplies native allocation/publication effects and defaults to the
profile module's current provider. `COMPATPROFILE` keeps its actual layout in
the private `profile_io.h`; tests consume that type instead of reconstructing
it. The unchanged error mapper and thread-local `GetLastError`/`SetLastError`
storage are separately linkable production code, not fixture replacements.

`PROFILEIOSCOPE` owns a provider snapshot and restores the previous provider on
the same thread through nesting and exceptions. Profile loading binds its line reader
at the operation boundary. `COMPATFILEREADSCOPE` changes only the native read
effect used while the legacy file adapter holds its lock. These scopes are
confined to their modules; they provide no symbol registry, key lookup or
algorithm substitution. Real-service function pointers remain available for
delegation. Cleanup effects must not throw and allocations remain `free`-compatible.

The existing guard fixture now binds typed read/line/write providers and links
the normal `stars_win32` library. Its `CbReadNativeFile`, `CbGetNativeLine` and
`CWriteNative` CMake redirects and the associated private wrapper clone are
gone. Its file-lock retry also checks restoration of an override nested inside
a throwing provider. Other hook sets and clone machinery remain pending their
own family migrations.

| Witness accounting | C03 receipt | Replacement / preserved evidence |
| --- | --- | --- |
| Save control (`check_s02_save_source.py`) | 42 assertions over all 11 stages from success through rename, plus failed sync with failed cleanup | `tests/compat/profile_save.cpp`, CTest `win32_profile_save`; same literal bytes, counters, expected errors and assertions |
| Source seam | Extracted `FSaveProfile`, reconstructed profile/file types, macro-rewritten I/O and a substitute error mapper | Linked production serializer, real private header, error mapper, error state and native file services; typed I/O effects only |
| Canonical rules | `S02-profile-io.dfy`: `Serialized`, `Stages`, `Execute`, `FailedDoesNotPublish`; S02-CONTRACT-AC09 | Rule definitions and traceability unchanged; no new proof or whole-program refinement claimed |
| Exclusions | Real syscall failure and crash durability | Unchanged; fixture tokens and stage receipts do not qualify OS failure/crash recovery |
| Additional runtime evidence | Guard fault/retry and profile registry cleanup | Retained; new checks cover primary error preservation despite a different cleanup errno, owned snapshots, nested bindings, callback unwinding, successful retry and thread isolation |

The Python entry remains an orchestration command for the existing witness job.
It compiles and links unmodified production translation units with ISO C++20
and strict diagnostics; it no longer slices source, reconstructs production
types, substitutes an error algorithm or rewrites function bodies. The normal
desktop target uses the same implementation from `stars_win32`. The new target
also joins automatic sanitizer CI; the opt-in platform policy is unchanged.

Local qualification:

- Fresh SDL Debug/Release builds pass without compiler diagnostics. The actual
  manifest has 119 production sources / 1,368 variants and 114 standalone
  headers in both include orders. The removed guard redirects appear in zero
  compile rows, down from 42 on C03.
- Full desktop suite: 120/120 pass in 315.69 seconds, including all 29 native
  leak checks, 17 game unit programs and the available tutorial smoke test.
  The final profile/guard fixtures also pass targeted rebuilds.
- Focused profile, guard and real callback tests pass ASan/UBSan; final added
  error-preservation and outer-scope restoration checks also pass sanitizers.
- Full native regression: 519 matches through turn 150, the same 25 accepted
  unused-storage warnings, zero differences/errors/skipped inputs. Direct
  comparison against C03's outputs also matches all 519 files.
- All 27 witness programs pass. Complete receipts match C03 byte-for-byte after
  removing runner labels, mapping the migrated save-test banner and excluding
  the new provider-check banner. Original counts, domains and exclusions remain
  unchanged; all 42 save-control assertions are retained.
- MinGW/GCC strict C++20 syntax checks cover the serializer/provider types,
  error state and fixture with generated wrapper aliases; native file services
  compile separately against real SDK headers. Full Windows runtime remains
  deferred under the existing policy.
- Documentation: 26/26 tests and 2,818 links / 700 requirement IDs pass.
  Canonical formal inputs are unchanged; no new proof run is claimed.

The initial sandboxed desktop run timed out at macOS leak inspection with
`Couldn't get task port` and was interrupted. Its separate receipt is retained;
the complete rerun with process-inspection access passes all 120 tests.

Receipts are retained in `dist/verification/c04/profile/`; enabled CI is pending.
Those families remained after PR #70; the combined continuation below replaces
them and all 26 remaining extracted programs.


## C04/C05 combined continuation

Scope: finish both checkpoints together on `codex/cpp-c04-c05-linked-tests`.
The original `STARS_TEST_IMPL` implementation-name markers, alternate declarations,
`stars_test_redirect`, allocation call macros, private library clones and their
contract fixture are deleted. Test callback names remain ordinary callback
functions. Typed module providers default to original functions; scopes own
snapshots, restore through nesting/unwinding and isolate threads. Worker-message
fixtures explicitly transfer their provider to worker scopes.

Effects cover native publication; record streams/writes/create/dirty state;
alerts, password/wizard/startup choices; RNG/capacity observation; SDL render,
window, display, input, paths, help/external, audio, ticks and folder selection;
message allocation and both chooser backends. SDK chooser types remain isolated
from wrapper headers. The publication fixture checks exact native flags/results,
snapshots, nesting, failure/retry, thread isolation and real UTF-8 filesystem
operations. POSIX rename behavior is preserved; no new no-replace guarantee is
claimed.

The [C05 inventory](CPP-C05-WITNESSES.md) and its JSON sidecar account for every
original witness, domain, exclusion, assertion, independent oracle and canonical
specification owner. CMake adds 26 witness tests and reuses `win32_profile_save`;
`stars-linked-witnesses` builds all normal targets plus the copy companion.
All 27 program receipts match the parent after presentation-label normalization;
67 retained independent Python functions/classes have identical ASTs. List/heap
lifetime helpers, real owned state and actual record types are linked. Small
private blocks move once to preference, transport, production-preset, restoration
and library-publication helpers called by the real workflows. There is no copied
production algorithm, reconstructed implementation record or stdio rewrite.
`cpp_production_storage.py`, extraction utilities and `STARS_TEST_STATE_DOUBLE`
branches are removed. Binary64 conversion observations retain the original
expression grouping and representability domain, with no replacement result.
Canonical Dafny files, baselines, frozen inputs and `structs.h` are unchanged.

The manifest rejects test symbol routing, state-double compiler flags and cloned
production targets; negative cases exercise those rejections. It accounts for
156 production sources / 157 compiled variants and 156 standalone headers in
both include orders. Assertion macros stay active in Release witness fixtures.
The automatic linked-witness job installs the normal SDL dependencies, builds the
normal sanitized targets once and runs the original 27 commands with ASan/UBSan/
LSan. Only the two previously deferred full platform jobs remain opt-in.

Qualification artifacts are in local `dist/verification/c04-c05/`:

- Fresh macOS Debug build and standalone interfaces pass without compiler diagnostics.
  The complete desktop suite passes 147/147, including 29 native leak checks,
  17 game unit programs and the available tutorial smoke checkpoint. The final
  portable CRT pointer declarations pass a rebuild and 29 affected checks.
- Fresh SDL-free Release host passes 27/27 tests, including the routing policy.
- Native and host regressions each match all 519 unchanged baseline files and
  exact PR #70 outputs through turn 150: 25 accepted unused-storage warnings,
  zero differences/errors and zero skipped inputs.
- All 26 new witness CTests, save/profile, publication and selected callback/
  allocation guards pass 32 ASan/UBSan checks. A broader native selection passes
  45 unique sanitizer checks across the selection and a focused font retry. The
  initial selection reported that font test Not Run until its target was built.
  macOS uses native `leaks` for leak qualification; Linux CI enables LSan.
- Choosers disabled plus optional real RNG/capacity trace builds and links. A
  `smallai4` run through turn 10 matches all 31 files, with 773 trace receipts.
- MinGW strict C++20 syntax checks pass the new provider surfaces and representative
  real UI witnesses. The chooser SDK adapter, native publication and native file
  service separately compile against genuine Windows SDK headers. This is compile
  evidence; full Windows runtime checks remain under the existing opt-in policy.
- Documentation checks pass 26 tests and 2,819 spec links / 700 requirement IDs.
  Dafny 4.11.0 collection verification passes with 12,000 verified / zero errors;
  audit reports zero findings. The unchanged `AxisRefines` proof first reached the
  60-second limit; the complete collection passes the documented 120-second retry.
  A focused habitat include closure also passes 986 verified / zero errors.

Qualification caught two missed effects hidden by the old compiler substitutions:
the shared export header's folder-choice callback and a dialog identity comparison.
The former now binds the folder service; the latter compares the active typed
callback. The complete export/save workflow passes after both corrections.
All previous callback names were audited for a typed binding reference. Moved-only
unused locals were removed; existing unrelated warnings remain unchanged.

The first enabled Linux CI run passes all native/host regression jobs, host
units and resource sanitizers. The witness/tutorial builds both reject the same
fixture-only `snprintf` truncation warning under GCC. The fixture now formats
into a full-size temporary and explicitly retains the original 14-byte text
limit; its linked witness and strict GCC syntax check pass. The corrected CI
run passes all 15 enabled jobs at `c6940721`; no production behavior or independent
expectation changes. The two slow full platform jobs retain their opt-in policy.

An additional historical precision experiment exposed a pre-existing reference
adapter issue: its frozen C bodies include current C++ headers. The arm64 run
also requires an x86 extended-precision oracle. Source discovery passes; the
whole differential experiment does not. C09 will materialize matching immutable
headers/data/generators at `precision_reference.REF`, preserving the independent
C reference and its expected values rather than changing application headers.

The completed C06 checkpoint implements the approved contract: use `ui::` operation basenames, replace
all 23 C03 prefixed APIs, remove redundant forwarding functions and preserve the
cleanup-before-error-delivery and SDK isolation contracts. C07–C10 follow in
focused, dependent PRs for review; no PR is merged by the agent.


## C06: actual wrapper namespaces

The [boundary inventory](CPP-C06-BOUNDARIES.md) and JSON sidecar account for all
23 checked entries, 223 raw API declarations plus common-control initialization,
38 independent shared SDK vectors and the unchanged moved Windows process bodies.
`ui::` names are the implementations themselves; old prefixed names and generated
Windows aliases are deleted. Every checked body is identical after qualification.
Their raw/checked distinction retains cleanup-before-error-delivery. Wrapper tags
are actual namespaced types, with one core/UI geometry definition; `win32::MSG`
coexists with SDL's native SDK hook. Windows process services are isolated from
application records and wrapper headers. The archived observer remains C-compatible
and uses explicit narrow SDK calls in its current C++ type-check entry.

The corrected head passes a clean full 149/149 desktop run. A preceding collection
timed out four workflows during confirmed system sleep; unchanged-limit isolated
retries and then the clean full rerun pass under process-scoped sleep assertions.
The prior 148-check full run also passes. All 29 native leak checks, the available tutorial smoke checkpoint,
34 final sanitizer checks and fresh SDL-free host tests (27/27) pass. Native/host
regressions each match 519 baseline files through turn 150 with 25 accepted
unused-storage warnings. Exact C04/C05 output comparisons match 519 files with
25 native / 23 host accepted warnings; no differences/errors or skipped inputs.
The manifest accounts for 156 production sources / 157 variants and 158 standalone
headers in both include orders. All 38 SDK C inputs and 29 representative C++
application/provider/isolated SDK units compile; paired wrapper/SDK identity
contracts pass compilation. Documentation checks pass 26 tests and 2,819 links /
700 requirement IDs. No model, baseline, frozen C input, record-layout or catalog
change is made. The corrected manual Windows/Linux runtime run is recorded below;
automatic CI policy is unchanged. Receipts: `dist/verification/c06/`.


C06's first manual Windows build identifies a `WINAPI` redefinition after SDL
intrinsics. Empty owned C++ calling-convention annotations (`WINAPI`, `WINAPIV`,
`APIENTRY`) are removed rather than masked; genuine SDK conventions stay native.
The SDL-first type contract and a strict MinGW check with an inherited
`WINAPI=__stdcall` now compile. Local builds and the full native regression
(519 matches / 25 accepted warnings / zero differences/errors/skips) pass again;
a clean full desktop rerun passes 149/149. All 14 enabled PR jobs and all five
corrected [manual platform jobs](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41206840795)
pass at `375538b0`: Windows probe 2/2, selected game units 13/13, platform
checks 88/88, and Linux desktop 153/153. Independent SDK configuration combinations
remain C09/C10 work. No automatic platform policy or baseline changes.


## C07: typed compatibility values and direct operations

The [macro audit](CPP-C07-MACROS.md) and per-definition JSON ledger account for
all 2,101 original definitions, their exact parent expressions and concrete
retention/removal purposes. Owned numeric values and conversion helpers are typed;
A/W aliases, Win16 procedure instances and no-op shims are deleted. Generated UI
resource payloads and manifests match C06 byte-for-byte. Local checks pass 150/150
desktop, 27/27 host and 35/35 selected sanitizer tests. Native/host baseline and
exact C06 comparisons each match 519 files (25 accepted warnings, zero differences/
errors/skips). The 27 witness receipts and 67 independent Python ASTs match.
The strict manifest covers 156 production sources / 157 variants / 160 headers;
38 real SDK C and 29 representative C++ units compile. Documentation checks pass
26 tests and 2,819 links / 700 IDs. All 14 enabled PR jobs and all five manual platform jobs pass at
`bcdd77bf`; Linux desktop passes 153/153, Windows probe 2/2, game units
13/13 and platform checks 88/88. C08–C10 remain in progress.


## C08: explicit state consumers

The [state ledger](CPP-C08-STATE.md) and JSON sidecar account for all 504 removed
references: 156 C01 game globals, two earlier file-XOR seeds, and 346 UI globals.
Actual owners retain their pre-main lifetime; owned callers pass `APPSESSION &`.
Foreign callbacks bind their target/harness context, and core remains SDL-free.
Local desktop/host/sanitizer collections pass 151/27/37 checks respectively.
Native and host regressions plus exact C07 comparisons each match 519 files
through turn 150 with 25 accepted warnings and zero differences/errors/skips.
The 27 witness receipts and 67 Python ASTs match. The trace/chooser-off build
matches 31 files and 773 normalized trace events. Compiler/header/docs checks
pass; remote CI and C09/C10 remain pending.


## C09: maintained build/reference policy

The [permanent policy](CPP-POLICY.md) and [path/hash inventory](CPP-C09-REFERENCES.json)
classify the precise independent C/SDK, AppKit, observer and preprocessing roles.
All 38 SDK C entries are checked in; their shared vectors and expectations remain
unchanged. No generated source-language wrapper remains. The manifest rejects
owned C/Objective-C, GNU mode, language forcing, weakened diagnostics, test symbol
substitution, private clones and generated/converted SDK entries. The native
Objective-C++ driver spelling is recognized only for `.mm`. C is enabled for
independent references/dependencies; dead C-only diagnostics are deleted, and the
ordinary native-point poison probe now tests the actual C++ compiler.

Fresh ordinary/neither macOS configurations have 151/0 tests, with no SDK entries;
SDK-only and both correctly reject the unsupported toolchain. Both eligible
Windows configurations will build and run all 38 SDK vectors plus type isolation
in distinct caches; the manual job also runs the entire ordinary registry.
Automatic job selection remains unchanged. Thirty-eight checked-in SDK C entries
compile with MinGW. All six unchanged precision experiment tests pass, using
matching headers/data/generators from the immutable revision; its two pinned host
builds match 31 turn-10 smoke files. Native/host regressions each match 519 baseline
files with 25 accepted warnings and zero differences/errors/skips. Exact merged-B
comparisons also match 519 files for each backend. The fresh desktop suite passes 151/151, including 29 native leak checks and
the available tutorial smoke checkpoint; the host passes 27/27. The final policy
checks and native-point contract also pass after the compiler cleanup. The updated
Windows matrix and C09 remote CI remain pending. Receipts: `dist/verification/c09/`.

C09's expanded SDK-only execution at `6e547f05` passes 20/39 checks and finds
19 failures in unchanged C07 probe inputs. [Exact first failures](CPP-C09-SDK-FINDINGS.md)
and retained logs distinguish observations from diagnoses. Existing ABI/native-point
checks and SDK isolation pass. No expectations or application behavior are changed;
the 19 exact hash-pinned behavior/automation probes remain diagnostics tracked in
issue #78, while the other 20 checks remain gates. Four policy tests reject hidden
boundary failures or incomplete execution. Matrix results now retain
both configuration results and do not prevent ordinary Windows qualification.

The first full ordinary Windows registry passes 128/131 checks and exposes three
linked-fixture portability gaps. Terraform/capability streams now explicitly use
binary stdin under the Windows CRT (preserving byte 0x1a and CR/LF records); the
publication fixture compares virtual path roles across native slash separators.
All original cases, independent expected values and assertion receipts remain
unchanged. Strict MinGW syntax checks pass for all three corrected fixtures;
corrected Windows execution remains pending. Both fresh SDK configurations already
record 20 passing contract checks and the same 19 diagnostics, with zero skipped
checks or unexpected contract failures; registries are ordinary 131, SDK 41, both
170 and neither zero.
