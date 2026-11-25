# C10: final Milestone C audit and qualification

Status: complete; PRs #72–#77 and follow-up fixes #79–#80 are integrated into
`develop` at `58c5e707`. Qualified source/build/test revision: `ee978a44`,
including the C08 Windows filename-fallback and C09 witness portability corrections. C10 changes
documentation/audit files only; its source/build/test/data/model trees match that
qualified revision exactly. The integration merge has the final stack's file tree.

The [final inventory](CPP-C10-AUDIT.json) lists all 152 tracked owned production
implementations (151 `.cpp`, one AppKit `.mm`) and every live owned production,
ordinary-test and resource macro. All 657 definitions have an individual
preprocessing purpose. These counts include platform-specific sources; the
macOS compile manifest covers 157 sources / 158 variants including generated
implementations. Its 160 standalone headers compile in both include orders.

## Removal and retention

| Mechanism | Final disposition |
| --- | --- |
| C/Objective-C implementation extensions, forced language and owned dual C interface | Removed; strict ISO C++20 / Objective-C++20 policy is permanent. |
| Generated wrapper namespace, A/W aliases and C03-prefixed UI entry names | Removed; actual `win32::` types/operations and direct `ui::` implementations. |
| Callback/implementation call interception, private production test clones and allocation macros | Removed; typed scoped module effects call the real providers by default. |
| Twenty-seven extracted current-source witnesses and surrogate state | Removed; ordinary linked targets preserve all original case/domain/exclusion receipts and 67 independent Python definitions. |
| Five hundred and four global state aliases and session getters | Removed; owned calls receive references, fixed foreign callbacks bind their target owner. Pre-main owner lifetime and reset/close order remain. |
| Win16 thunk/no-op shims and suitable conversion/constant macros | Removed or typed; retained preprocessing definitions each have a specific purpose. |
| Generated SDK C language entries and dead C-only diagnostics | Removed; 38 checked-in independent C entries, actual C++ compiler probes and negative audit cases. |
| Independent C, native SDK/AppKit, archived observer, resources and assertions | Kept for the exact roles in [permanent policy](CPP-POLICY.md), the C07/C08 ledgers and the C09 path/hash inventory. |
| A/B plans and source receipts | Historical; inspection links now pin their inspected commits. Current build/interface rules live in permanent policy. |

Frozen CPP-A inputs, original/native regression fixtures, catalog YAML, text rows
and canonical Dafny models match the merged B parent. Record/layout, enum storage,
codec, catalog and resource checks remain registered. No gameplay, file-format,
record-size, ID or expectation changes are made. One-off migration scripts are
not tracked in the repository.

## Final evidence

| Check | Result |
| --- | --- |
| macOS Debug, full desktop collection | 151/151, including 29 native leak checks and the available tutorial smoke checkpoint |
| SDL-free host collection | 27/27 |
| ASan/UBSan selection | 37/37, including linked witnesses, callback owners/failures, memory/type/ABI and resource guards |
| Native and host baseline comparisons | Each 519 matches through turn 150; 25 accepted unused-storage warnings, zero differences/errors/skips |
| Exact merged-B comparisons | Each backend matches all 519 files, with the same 25 accepted warnings and no differences/errors/skips |
| Windows compilation | All 87 core/UI/host translation units pass strict MinGW syntax; 38 native SDK C entries and isolated SDK adapters compile |
| Historical precision experiment | All six unchanged tests pass with original counterexamples and deterministic reports; two pinned host builds match 31 turn-10 files |
| Trace / native chooser disabled | C08's unchanged source path matches 31 baseline files and 773 normalized trace events |
| Documentation | 26 tests, 2,819 links / 700 IDs; rendered handbook: 207 pages / 38,601 local links and assets |
| Full Dafny verification/audit | 12,000 verified, zero errors; auditor zero findings; no canonical model changes |
| Linux desktop | 154/154 in the final manual run |
| Windows ordinary registry | 131/131, including all linked witnesses and callback/type/ABI contracts |
| Fresh Windows registry matrix | Ordinary 131, SDK-only 41, both 170, neither zero; SDK-only/both execute all 39 selected SDK checks: 20 passing gates and 19 named diagnostics each, zero skipped/unexpected failures |

Local receipts are in `dist/verification/c10/`, with preceding checkpoint receipts
kept separately. This includes actual comparison reports rather than inferred
platform results. The first C08 Windows run failed on two missed `_WIN32` state
reads; the corrected unit is compiled, and corrected full runs qualify it.
The [final platform run](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41224660401)
passes all five jobs. C08's corrected run also passes all five jobs and its 14
enabled PR jobs. C09 passes all 15 enabled PR jobs; C10's preceding documentation
head passes all 14. The automatic full Linux/Windows jobs remain opt-in.

At C10 qualification, 19 native SDK behavior/automation probes were diagnostics in
[issue #78](https://github.com/madebyjeffrey/sdl-stars/issues/78), with every original
assertion retained and raw results/JUnit published. The other 20 SDK checks,
missing/skipped execution, changed diagnostic inputs and unexpected failures are
gates. Later corrections and the current classification are recorded in the
[SDK findings](CPP-C09-SDK-FINDINGS.md). This is not a claim that all 39 native
probes pass. The first full ordinary Windows run passed 128/131; binary stdin
mode and native path-role recognition
corrected the three fixture portability gaps without changing cases/expectations.

The full 80-page SDL tutorial remains an existing coverage limit; this milestone
uses the available bounded tutorial checks without claiming complete walkthrough
coverage.

## Review and rollback

PRs #72 (C04/C05), #73 (C06), #74 (C07), #75 (C08), #76 (C09) and #77 (C10)
merged into their dependent branches in order. The final stack was merged into
`develop` at `58c5e707`; roll back dependents first if necessary. No irreversible
file conversion is part of C.
