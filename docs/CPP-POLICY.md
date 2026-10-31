# Maintained C++ and independent-reference policy

Owned production uses ISO C++20 (`.cpp`) or Objective-C++20 (`.mm`), extensions
off, with `-Wpedantic -Werror` in every actual compile variant. Native AppKit
sources retain ARC/blocks and CMake's `-x objective-c++` driver spelling for `.mm`;
that is not permission to force a C source into C++. Ordinary tests consume the
same application headers and linked production targets. No function redirection,
private production clone, surrogate state definition or session locator is allowed.

`cmake/strict-cpp.cmake` applies production policy; the actual compilation manifest
and standalone-header target enforce it. Negative manifest cases reject GNU mode,
language forcing, weakened diagnostics, symbol substitution, state doubles and
private production clones. Application headers compile independently before and
after standard-library headers. The SDL-free host checks core/platform headers;
the desktop additionally checks UI/wrapper headers. Core uses only a forward UI
state type and receives the application owner explicitly.

## Precise continuing exceptions

The [path/hash inventory](CPP-C09-REFERENCES.json) records the reference inputs.
These roles do not exempt other sources in their directories.

| Inputs | Continuing purpose and restriction |
| --- | --- |
| `tests/reference/sdk/*.c` (38 enumerated entries) and matching `tests/compat/*.inc` | Actual C entry files compile unchanged shared vectors against native SDK headers. Each entry has exactly the four recorded include/type lines; no generated language adapter or application wrapper include path. |
| The matching 38 ordinary `.cpp` entries | Native C++ wrapper consumers of those same C11 vectors. Only these entries suppress missing-field/braces warnings for the shared `{0}` initialization spelling; production and other ordinary tests remain strict. |
| `tests/compat/record_layout.c`, `popup_payload_reference.c` | Independent C declarations from frozen CPP-A inputs check physical records and popup payload values. They cannot include current C++ declarations. |
| `tests/reference/cpp-a/*` | Immutable pre-C++ declarations, codecs and vectors; historical paths/bytes stay frozen. |
| `precision_probe.c`, `precision_race.c` and `precision_reference.py` | Historical C arithmetic at commit `3c67394973aacf6b0bf24d0a302f42b50a87bfa0`, with matching headers, data and generators materialized from that commit. Never mix the old bodies with current headers. x87 format/control-state validation remains mandatory; Darwin arm64 uses an x86 build through Rosetta. |
| `tests/compat/mesa_rtasm_lifetime.c` | Independent Linux allocator/foreign Mesa C ABI experiment; it is not an application implementation. |
| `tests/reference/win32-tutorial/observer.c` and `tests/compat/archived_tutorial_types.cpp` | Maintained archived native observer and current C++ type projection. The observer keeps its C ABI; one explicit arity macro and local field borrows adapt current owned calls. The projection names retired native APIs solely to compile observation fields. This is type checking, not SDL tutorial execution. |
| `compat/win32/src/filedialog_cocoa.mm` and the two Cocoa client test `.mm` files | Native AppKit integration, ARC and foreign platform callbacks. |
| Resource/SDK/platform selection, assertion inventories, build instrumentation | Specific preprocessing roles recorded in the C07 per-definition ledger; no blanket compatibility/test exception. `shared_sdk.h` only reconciles the shared vectors' callback spelling with the SDK C ABI. |

Foreign window/dialog/timer callbacks retain their fixed ABI and borrow the target
owner through the callback-context service. The C08 per-callback ledger lists the
entries; owned call chains pass references. Raw SDK bridges remain isolated from
application records. Third-party dependencies keep their own source languages and
compiler policies; none supplies an owned C interface.

## Fresh configuration matrix

Run `tests/cmake/configure_matrix.py --preset <platform-debug> --work <fresh-dir>`.
It checks ordinary-only, SDK-only, both and neither configurations with distinct
CMake caches and verifies exact registry membership (38 SDK vector tests plus SDK
type isolation). In SDK-only and combined builds it audits the compile manifest:
each of the 38 C entry files must appear once, use CMake's C compiler and avoid
C++ language overrides. Non-Windows toolchains must reject SDK configurations explicitly.
On native Windows add `--run-sdk` to build and run all SDK vectors in both eligible
configurations. The [native SDK findings](CPP-C09-SDK-FINDINGS.md) record 19
individually named, hash-pinned pre-existing behavior/automation probes as
diagnostics. All original assertions execute, with raw results/JUnit retained;
the other 20 SDK checks remain gates. Skipped/missing checks, modified diagnostic
inputs or unexpected failures remain errors. This classification does not weaken
the ordinary SDL counterparts' assertions. The manual Windows job then builds and runs the entire ordinary
registry, including the standalone headers, owner/callback checks and witnesses.
The Linux and Windows full platform jobs remain opt-in; other automatic CI stays
active. Matrix logs/manifests are uploaded by the manual Windows job.
