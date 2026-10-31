# C07: typed values and direct compatibility operations

Implementation parent: `375538b0` (C06, PR #73). The pinned milestone inventory
is `62bb854d143a618419f645abdc4da37d53cb5837`.

The [per-definition ledger](CPP-C07-MACROS.json) accounts for all 2,101 pinned
owned macro definitions. It retains the exact parent path, line and expression,
records the replacement and purpose, and inventories each live owned production,
ordinary-test and resource macro. It excludes immutable reference/third-party
inputs, including Acutest; it does not declare a blanket compatibility exception.

| Disposition | Original definitions |
| --- | ---: |
| Typed numeric/string/tag values in C07 | 1,835 |
| Typed conversion helpers in C07 | 18 |
| Deleted shims/aliases/keyword spellings in C07 | 61 |
| Deleted in C06 | 17 |
| Deleted test substitutions in C04 | 28 |
| Deleted call-routing macros in C03 | 25 |
| Retained specific preprocessing roles | 117 |
| Total | 2,101 |

## Values and conversion contracts

`value.h` provides the core's word/byte helpers. `wincompat_values.h` provides
message, resource-tag and color helpers in `win32::`. Arguments are passed by
value so existing packed bit-field expressions remain valid. Return types retain
16/8/32-bit results or the required native parameter width. Negative word pairs
in `MAKEWPARAM` and `MAKELPARAM` still zero-extend the combined 32-bit value;
wheel deltas retain their signed 16-bit conversion. Resource pointer tags are
explicit runtime casts; their declarations make no constexpr claim.

The shared ABI input retains its original vectors and adds literal boundary,
truncation, bit-field and independent operand evaluation-count cases. C++ also
checks scoped enums and exact result types. The same vectors compile against
the genuine Windows SDK. Production multi-argument helper calls were inspected:
the only nested operations are `CyEdit(hwnd)` with literal high word `1`, and
`WMouseModifiers(buttons)` with a local wheel delta. Neither shares effects with
its other operand; no RNG calls or interacting argument effects are reordered.

The constants retain original expressions and LLP64 casts. The duplicate
`PATINVERT` definitions denote the same value and are coalesced. `VOID` is the
C++ keyword `void`; `ENOENT` and `SEEK_*` use their actual CRT declarations.
`FSHIFT` has one explicit foreign POSIX macro collision cleanup, rather than an
owned macro definition. Opaque handle tags are declared explicitly.

The game's original `IDOK`, `IDCANCEL`, `IDHELP`, `IDC_HELP` and `IDC_HAND`
enumerators remain unchanged. In particular, dialog `IDC_HELP` (118) and resource
`IDC_HAND` (265) are distinct from the SDK cursor pointer tags with those names.
Raw runtime consumers use qualified values; game callers keep their game IDs.

`uigen.py` reads typed numeric declarations while `res/resource.h` remains the
single resource-compiler/parser ID input. The resource rule depends on the shared
SDL-independent values header. Generated `ui_resources.cpp` and `ui_manifest.h`
match C06 byte-for-byte. Catalog flag generation emits integer `1`/`0`, exactly
the old macro expansions, without widening stored flags or changing YAML/IDs.

## Deleted operations and retained boundaries

All 46 `MakeProcInstance` expressions become direct callback values. Their
instance argument was unevaluated by the original macro and remains unused.
All 38 removed `FreeProcInstance` statements consumed pure local identifiers;
the ledger records each site. The single `Yield` statement was a no-op.
Generic A/W function aliases become direct existing operations. `CreateWindow`
callers provide the same explicit extended-style zero to `CreateWindowExA`.
No spelling-only forwarding function is introduced.

The four `ExitWindows` consumers call `ui::RequestQuit` directly. This existing
implementation still reports unsupported logoff, sets the same error, queues the
application exit code and returns the original result. Its distinct behavior is
retained rather than silently dropping the diagnostic or quit notification.

The style inspector's existing `DecodeStyles` implementation infers its array
count from a reference parameter; token-pasting dispatch is deleted. Private UI,
paint, portal and chooser values are ordinary constants. Test fixtures use typed
CRT/platform helpers with their original path sizes, modes and delays.

Retained macros have concrete roles: repeated-inclusion guards; Windows SDK
`NOMINMAX`; the installed help-data build path; RC/parser resource IDs;
assertion/field stringification and the explicit enum/layout inventories;
compiler return-address instrumentation; the printer fixture output path; and
narrow shared SDK/historical observer adapters. The `CALLBACK` test annotation is
confined to shared SDK vector entry points; owned production callback signatures
use normal C++. The historical C cursor fault hook applies only to immutable
reference declarations; current C++ code takes a typed callback.

## Qualification

Local qualification passes the full 150/150 desktop collection, including all
29 native leak checks, the available tutorial smoke checkpoint and the newly
registered ABI fixture. The selected ASan/UBSan collection passes 35/35, including
all 27 witness programs, callback/ownership guards and cursor/type/ABI contracts.
All 27 original witness receipts retain every parent case/count/exclusion line;
all 67 independent Python definitions retain identical AST hashes.

The fresh SDL-free host passes 27/27 tests. Full native and host regressions, and
exact C06 output comparisons, each match 519 files through turn 150 with
25 accepted unused-storage warnings and zero differences/errors/skips. The
strict manifest accounts for 156 production sources / 157 compiled variants and
160 standalone headers in both include orders. Generated UI payloads/manifests
match C06 byte-for-byte. Documentation checks pass 26 tests and 2,819 links /
700 requirement IDs. The API audit finds 178/178 concrete API definitions and
6/6 separate CRT definitions; this lexical result is not conformance evidence.

All 38 SDK C inputs compile with their independent C reference policy; a broader
strict-warning experiment encountered the pre-existing frozen C flexible-array/
enum extensions and an unused platform parameter, and is not reported as passing.
The 29 representative application/provider/isolated SDK C++ units compile under
their normal target diagnostics. Remote CI and remaining platform configuration
qualification are recorded separately; automatic slow-job defaults are unchanged.
No baseline, game record layout, canonical model, catalog ID or immutable C
reference input changes. The maintained archived tutorial observer changes its
integer truth spellings only. Local receipts: `dist/verification/c07/`.

The first Linux CI build finds one remaining string-literal concatenation in
`filedialog_portal.cpp`. It now formats the typed `PORTAL_PATH` as `%s`, followed
by the same request/sender/token bytes. No portal path, argument or truncation
limit changes. Corrected Linux/platform reruns are pending.
