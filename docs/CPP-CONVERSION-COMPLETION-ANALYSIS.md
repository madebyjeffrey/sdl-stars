# Completing the C++ conversion after Milestone B

Analysis recorded 2026-10-29. This is an engineering assessment, not an
implementation change or an amendment to the approved migration scope.

The follow-on work is now organized in the [Milestone C plan](CPP-MILESTONE-C.md),
to be implemented after B. This analysis remains the historical source inventory;
C starts by revalidating it against the merged B result.

## Assessment and inspected baseline

The application can become an ordinary C++20 codebase, including honest source
extensions and removal of migration-specific build and preprocessor machinery,
without introducing an object-oriented game design. Free functions, namespaces,
explicit state parameters, value types and small resource owners suffice.

The important distinction is between completing B and removing all scaffolding
around that conversion. B already covers most difficult semantic work. It does
not explicitly finish filenames, test substitution, source extraction, wrapper
API macros or its own temporary state accessors. Those need a separate completion
stage with explicit deletion conditions.

The chat's checkout is `codex/traceability-views` at `e3222eb`, which predates
the migration. The substantive inspection used branch
`codex/cpp-application-milestone-b` at
`5fd94fb81bb58840ecada0f9295d992f712f7466`. Links below are pinned to
that inspected revision. Findings describe that revision, not the eventual result
of B.

At inspection, the B ledger has B01 active and B02–B14 pending. Existing passing
checks in that ledger qualify particular ownership batches, not completion of B.
Milestone A has already converted owned production, generators and ordinary
consumers to C++20/Objective-C++20 and removed owned C linkage/fallback branches.
Repeating A is unnecessary.

Sources: [migration plan](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/docs/CPP-MIGRATION.md),
[B ledger](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/docs/CPP-B-IMPLEMENTATION.md),
[consumer inventory](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/docs/CPP-CONSUMERS.md).

## What B already promises

| Work | Existing checkpoint | Why it matters to completion |
| --- | --- | --- |
| Resource owners and safe dispatch cleanup | B01–B02 | C++ destruction must preserve borrowed resources, reentrancy and failure delivery. |
| Const-correct text and bounded views | B03 | Removes literal casts and separates text from writable/binary buffers. Views cannot outlive scratch storage or heap resets. |
| Explicit wire/resource codecs and variable payloads | B04–B11 | Removes reliance on packed structs, zero-length tails and anonymous aggregate overlays. |
| Stable typed arenas | B12 | Starts and ends object lifetimes while retaining addresses, capacities and reset semantics. |
| One application/session state owner | B13 | Makes initialization and teardown explicit; the plan permits temporary accessors. |
| ISO C++20 with extensions disabled | B14 | Removes migration-only suppressions and extension dependencies on Linux, macOS and Windows. |

These are substantial changes across game and wrapper code. In the inspected
tree, `structs.h` still has packing directives, zero-length tails and a Clang
diagnostic suppression. Disabling GNU extensions or replacing a tail with a
container before its direct consumers and wire representation migrate would
not complete this safely. A successful strict compile also cannot establish
that all aliasing, lifetime and serialized-byte contracts are correct.

Preserve the existing boundaries: game core independent of SDL, UTF-8 byte
budgets, format 2.85, stable IDs, fixed wire widths, RNG consumption, `qsort16`
tie ordering and binary64 operation boundaries. Runtime types may change once
their serialized representation is separated. Fixed wire contracts must remain.

Sources: [B04–B14 and verification policy](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/docs/CPP-MIGRATION.md#L212),
[current storage declarations](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/core/structs.h#L127).

## Additional work needed for a complete conversion

### 1. Rename implementations and simplify the build

The tracked production tree contains 105 `.c` files, four `.cpp` files and one
`.m` file across `src/` and `compat/win32/src/`. The source bodies total about
86,000 lines; this is the scale of the application, not an estimate of lines
that need editing. Ordinary harness directories contain another 97 `.c` files
before independent-reference exceptions, plus two Cocoa `.m` test adapters.

Rename owned implementations `.c` → `.cpp` and AppKit adapters `.m` → `.mm`.
Keep original basenames and debug-symbol names. `.h` headers are perfectly
valid in C++; renaming owned headers to `.hpp` is an optional consistency choice
with additional include/tool churn. Foreign-shadow headers such as `windows.h`
have a deliberate lookup role and need separate consideration.

Then remove the large `LANGUAGE CXX`/`LANGUAGE OBJCXX` override lists in root
and wrapper CMake. CMake normally selects source language from extension, so
honest extensions make these overrides redundant. Remove C-only warnings and
dead mixed-language conditions from owned targets. Apply strict C++ diagnostics
directly; currently many `-Wpedantic` expressions apply only to C and therefore
do nothing for the converted sources. Audit test-wide warning suppressions too.

Update more than CMake source lists:

- All 27 source-witness scripts with hardcoded implementation paths.
- Test discovery globs, private test clones, SDK shared-body wrappers and CI.
- `precision.py`, which scans `*.c`, and `precision_turns.py`, which copies
  only `.c`/`.h` sources.
- Flatpak's untracked-source guard, which currently omits `.cpp` and `.mm`.
- The archived tutorial parser's `src/ui/tutor.c` path and its literal function
  boundaries, plus observer declarations if changed by B.
- Documentation, source evidence links and traceability tooling.

Do not mass-replace historical paths in frozen references: some tools deliberately
read an old Git revision. Generated game/resource outputs already use C++ per A;
verify them rather than assuming their old documentation describes current code.

Sources: [root language overrides](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/CMakeLists.txt#L55),
[wrapper overrides](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/CMakeLists.txt#L71),
[precision scan](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/scaffold/precision.py#L212),
[precision copies](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/scaffold/precision_turns.py#L33),
[Flatpak guard](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tools/build_flatpak.py#L27),
[tutorial parser](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/reference/win32-tutorial/run.py#L68),
[CMake LANGUAGE documentation](https://cmake.org/cmake/help/latest/prop_sf/LANGUAGE.html).

### 2. Replace callback-call macros with explicit boundaries

`src/ui/win_calls.h` intercepts 23 wrapper APIs. `STARS_CPP_CALL`, `#undef` and
`#define` capture an API and replace source calls with a template wrapper that
rethrows pending callback exceptions after dispatch cleanup. Its comment still
describes original C symbols although A has moved owned APIs to C++ linkage.

Replace this textual interception with ordinary named C++ entry functions.
Review which calls can propagate directly after B02's guards and which still
need a pending-error boundary. Direct propagation is appropriate only after
every intervening frame and cleanup path supports it. Keep containment where a
callback returns through SDL, AppKit, an OS API or another foreign frame.

Deletion requires equivalent fault evidence: nested modal owner restoration,
deferred destruction, unowned timer/window failures, no commands after failure,
and no second exception escaping a destructor. Merely deleting `win_calls.h`
would remove error delivery, not just scaffolding.

Sources: [macro routing](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/ui/win_calls.h#L7),
[callback containment](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/src/callback.cpp#L13),
[pending errors](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/platform/errors.cpp#L14).

### 3. Replace test symbol rewriting with typed test seams

`test_impl.h` changes function definition names; `test-redirect.cmake` clones
libraries and substitutes calls with preprocessor definitions. Some headers
then declare extra symbols using `decltype`. This is test infrastructure rather
than a C++ language requirement, but it obscures the functions being compiled
and duplicates build work.

Use ordinary implementation functions and narrow typed operation providers for
the external effects a test controls: random draws, file operations, dialogs,
publication and selected UI actions. Function-pointer tables or explicit
operation/context parameters are sufficient; no interface inheritance is needed.
For unchanged public debug names, a small facade can delegate to an implementation
helper. Prefer explicit provider parameters where practical. If a scoped override
is necessary, account for nesting, reentrancy and restoration on exceptions.

Migrate one seam at a time, preserving tests that both inject failures and
delegate to the real implementation. Remove each corresponding macro, alternate
declaration and clone variant as its last consumer moves. Avoid replacing the
current machinery with a permanent global mock registry.

Sources: [definition substitutions](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/platform/test_impl.h),
[private clone/substitution mechanism](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/cmake/test-redirect.cmake),
[alternate declarations](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/core/file.h#L30).

### 4. Retire source extraction and implementation rewriting

Compiling a generated witness as C++ does not make it an ordinary C++ test.
The witness infrastructure still extracts functions by literal signature and
the next closing brace, reproduces declarations and sometimes rewrites function
bodies. For example the library-publication witness replaces `NATIVEFILE` and
native file operations with stdio, writes `copy.c`, then forces C++ compilation.

Move these cases to tests linked against actual production modules and the typed
seams above. Preserve each independent arithmetic oracle, witness vector, excluded
domain and Dafny mapping. Test doubles for effects remain legitimate; silently
testing a rewritten copy of the algorithm is the part to remove. Linkable
internal helpers can expose a narrow seam without becoming public application APIs.

The target is deletion of textual extraction/rewrite code and redundant forced
includes/`-x c++` flags, while retaining Python orchestration if useful. This
is more work than updating extensions, because existing witnesses isolate many
dependencies with reconstructed local types and globals.

Sources: [extraction helper](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/spec/check_s02_source.py#L24),
[file abstraction rewriting](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/spec/check_library_publication_source.py#L379),
[representative reconstructed fixture](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/spec/check_production_cost_source.py).

### 5. Replace wrapper macro namespaces and obsolete aliases

On Windows, CMake generates macros mapping Win32-style API names to
`StarsCompat_*`. Use an actual C++ namespace for owned wrapper declarations,
definitions and calls. Qualify calls explicitly where SDK names could collide.
Keep SDK translation units isolated: C++ name mangling alone is an inadequate
reason to merge independently defined wrapper and native SDK types.

Audit the remaining function-like compatibility macros. Byte/word extraction,
message packing and color helpers can generally become small typed functions
or `constexpr` functions. Preserve signedness, pointer conversions, truncation
and operand evaluation. Resource-pointer tagging needs deliberate handling;
do not promise every pointer conversion can be a C++20 constant expression.
Replace creation aliases with ordinary forwarding functions or direct calls.
Remove `MakeProcInstance`/`FreeProcInstance` and other obsolete no-op call sites
after checking argument effects and their documentation entries.

Use typed constants for owned `TRUE`/`FALSE`, message IDs and flags where
appropriate, preserving existing numeric domains and storage widths. Do not
blindly replace stored `int16_t` flags with `bool`. The resource compiler and
`uigen.py` still consume macro definitions; retain necessary resource inputs or
generate both representations from one source before deleting those macros.

After B13, remove temporary globals-to-context macros/accessor aliases as well.
Give functions an explicit state/context reference or use a clearly named access
at a narrow boundary. One owning context with hundreds of permanent global-name
macros would leave the dependency structure hidden. B13 permits temporary
accessors, so this final removal must be tracked explicitly.

Sources: [generated API renames](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/CMakeLists.txt#L5),
[wrapper aliases](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/include/windows.h#L11),
[type macros](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/include/wincompat_types.h#L17),
[resource parsing](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/compat/win32/uigen.py#L15),
[temporary state accessors in B13](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/docs/CPP-MIGRATION.md#L234).

## What should remain, or be a separate project

| Mechanism | Recommended disposition |
| --- | --- |
| Standalone header compilation and actual compiler-command audit | Keep as permanent assurance. Simplify after renames; require strict C++/Objective-C++ and explicitly list genuine exceptions. Ensure the audit recognizes `.mm` too. |
| Enum widths, constant values, independent wire vectors and catalog/resource bytes | Keep. Replace obsolete runtime layout comparisons with codec evidence as B separates runtime and wire types. |
| Frozen C declarations and original regression fixtures | Preserve as independent evidence. They do not expose a C application API. Do not rewrite expectations to match changed implementation. |
| Shared C SDK test wrappers | Optional cleanup. Keep independent C tests when they test a C/ABI contract; otherwise C++ SDK tests can remove the extra generated `.c` include wrappers. Preserve independence from wrapper headers. |
| Foreign headers/linkage and callback conventions | Keep where actually required. Native C++ linkage is the default for owned declarations; foreign linkage is a real boundary, not a migration failure. |
| AppKit Objective-C++ | Keep as `.mm`; using Apple's native object model does not require rewriting the game around classes. |
| SDL-backed Win32 compatibility module | Keep its functionality. It implements the UI today. Removing it requires a separate UI port, although its macros and internals can become ordinary C++. |
| `native`/filesystem/file-service functions, UTF-8 validation, custom sorting | Keep useful contracts and names. Having a wrapper around a standard library or OS operation is not inherently transitional. |
| `StarsMin`/`StarsMax` and small RAII templates | Evaluate by semantics, not spelling. Their mixed-type conversion behavior or resource release policy can justify them permanently. |
| Header guards, platform feature selection, resource macros and assertion macros | Retain where they serve preprocessing needs. A fully C++ application need not have zero preprocessor directives. |
| Python generators, build/test drivers, Go save-analysis tooling, Dafny models | Keep their languages unless there is a separate reason to rewrite them. They are not application C translation units or inherently transitional tools. |
| C++ modules, broad enum-class migration, algorithm/container replacement, class hierarchies | Optional later design work. None is required to finish the language conversion, and some could change behavior or storage. |

Sources: [frozen reference purpose](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/reference/cpp-a/README.md),
[header checks](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/cmake/cpp-interface-checks.cmake),
[compiler audit](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/tests/cmake/check_cpp_manifest.py),
[mixed-type value helpers](https://github.com/madebyjeffrey/sdl-stars/blob/5fd94fb81bb58840ecada0f9295d992f712f7466/src/platform/value.h),
[C++ language linkage rules](https://eel.is/c++draft/dcl.link).

## Suggested delivery and completion evidence

Finish the agreed B scope first. Review its final result against this analysis;
B02/B14 may already remove some items listed here. Avoid concurrent renaming
while B is editing the same implementation files.

Then use focused, separately reviewable changes:

1. Rename owned implementation/test files and update every consumer. Delete
   redundant language overrides in that same mechanical change. Keep header
   extensions initially to limit churn.
2. Replace callback routing macros with explicit C++ boundaries, retaining the
   verified cleanup/error protocol. Coordinate this with B02 rather than
   duplicating its work.
3. Introduce typed test seams and migrate source witnesses in bounded families;
   remove substitution and extraction machinery as consumers disappear.
4. Replace wrapper API macro namespacing with real namespaces; retire obsolete
   API/no-op macros and move suitable constants into typed declarations.
5. Finish B13's state access migration and remove temporary aliases. Simplify
   build/audit policy and update active agent/build/migration documentation.

These are workstreams, not a reliable PR count or calendar estimate. Renames
have low semantic risk but broad tooling reach. Callback delivery, test seam
migration and state access have higher semantic risk. Storage/arena/codec work
in B remains the largest prerequisite. A credible time estimate requires the
final B diff and a complete remaining-consumer inventory.

Completion should demonstrate: owned production and ordinary test sources have
honest `.cpp`/`.mm` extensions; strict C++20 policy applies to all those targets;
no owned C/C++ dual interface remains; no migration-only symbol/language rewrite
or source-extracted substitute implementation remains; temporary state aliases
are gone; all retained macros, C sources and foreign adapters have named purposes.
Keep this inventory in maintained build policy rather than a one-time report.

Run the platform SDL Debug build, relevant Release build, full native regression,
unit and SDL tutorial checks; shared/game changes also require full host
regression and host units. Include all CTest contracts, the 27 migrated witnesses,
Linux/macOS/Windows qualification, and relevant ownership/fault/sanitizer checks.
The plan records 519 regression files across nine scenarios through turn 150;
use the actual parent/platform results and show the first unexpected difference.
Behavior-neutral work must preserve the native baseline. Keep original fixtures
frozen and do not update expected results merely to make cleanup pass.

This assessment changed no implementation and ran no build or regression suite.
Its evidence is source inspection, the recorded migration contracts and a tracked
file inventory. Existing B verification is cited as recorded evidence only.
