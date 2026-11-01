# M02 — Race staging, copy receipts and numbered suggestions

Observed source inspected 2026-10-27 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the working collection.
This technical companion supports [M02-US03](M02-race-library.md#m02-us03),
[US04](M02-race-library.md#m02-us04), [US05](M02-race-library.md#m02-us05) and
[US06](M02-race-library.md#m02-us06): save, import, duplicate and export a reusable
race design. Stories and acceptance criteria stay at the
[M02 entry](M02-race-library.md#4-acceptance-and-traceability).
This document completes the bounded temporary-name and suggestion algorithms;
[ordinary Duplicate/Export/Delete controllers](M02-race-file-actions.md) now
compose these rules. [Import/name-choice/wrapper contracts](M02-race-import.md)
now compose these rules too. [Shared naming](M01-library-name-dialog.md) and
[wizard dispatch](M04-wizard-controller.md) supply control contracts; complete
individual pages and native modal qualification remain separate work. No runtime
behavior or policy changes here.

Canonical definitions: [M02RaceAllocation](dafny/M02-race-allocation.dfy), reusing
[S01LibraryNames](dafny/S01-library-names.dfy),
[S04Utf8Prefix](dafny/S04-utf8-prefix.dfy),
[S01External](dafny/S01-external.dfy) and
[M02RaceSave](dafny/M02-race-save.dfy).
Evidence: [libraryui.c](../src/ui/libraryui.c) `FCreateRaceStagePath`,
`FCopyRaceExport`, `FCopyNativeFileContents`, `FCopyNativeFile`,
`FCopyNativeFileExclusive`, `SuggestRaceFilename`, `FPortableRaceFilename` and
`FDuplicateLibraryRace`; [encoding.c](../src/platform/encoding.c)
`CchUtf8Prefix`. The [verification record](README.md#m02-race-allocation-continuation)
separates model proofs, extracted C tests and outstanding platform qualification.

## Callers, state and observable results

The caller supplies UTF-8 root/source/target C strings, native separator, buffer
capacities, one sampled unsigned 32-bit tick and the shared unsigned 32-bit
`idRaceStage`. Paths in the model are logical bytes before the first NUL. `Store`
maps literal path bytes to file bytes and separately records directories. It
does not normalize case, separators, symlinks or hard links. S04 owns lossless
path conversion and actual operating-system results; S01 owns race-file decoding.

`Allocation` returns the result, final store/counter, consumed reply count,
logical directory/file slots and reached events. Events retain attempted names;
empty slots at retry exhaustion do not describe the native output buffer.
`Returned(false)` is an
ordinary native failure. `Pending` means the supplied finite reply trace stops
before the controller finishes; `InvalidEvidence` rejects inconsistent service
receipts. Neither diagnostic is a native return value. Event traces describe
checked service replies, not arbitrary reentrant callback or concurrent-writer
interleavings.

The model keeps full intended formatting bytes. With positive capacity, native
truncation leaves at most capacity minus one bytes and a NUL; zero capacity
writes no output bytes. Those bytes are not a usable allocation. The source
fixture checks the actual truncated
output separately. Unwritten later storage is not modeled as an initialized
string or as a new path. Native callers must supply valid, sufficiently sized
buffers and representable C-string inputs; formatting output must not overlap
strings it reads. Equal source/target path text remains represented.

The clock, format-error, mkdir/directory-query, copy and cleanup receipts are
explicit external inputs. Store transitions assume no unrepresented external
mutation during a helper call. A failed directory query does not prove a path
absent. A successful fresh directory admission requires no conflicting file,
directory or children in the model. Directory-query and cleanup replies are
qualified observations, not promises that the next operation succeeds.

## Stage-directory allocation

`Stage` samples the tick once and calls `StageLoop` with exactly 1000 attempts.
Each attempt formats `root<sep>.stars-race-stage-<tick>-<counter>` using unsigned
decimal text, then advances the counter modulo 2³² **before** checking formatting
success. A nonpositive formatter result or result at least the directory
capacity returns FALSE without a mkdir call.

When mkdir fails, the following directory query controls the outcome: a true
query retries with the next counter; a false query returns FALSE. The code does
not require mkdir's error to be EEXIST. Exhausting all 1000 attempts returns
FALSE. On successful mkdir, format `<directory><sep>design.r1`; file-name
formatting failure attempts directory removal and returns FALSE even when that
removal fails. A successful allocation creates an empty directory and reserves
no file descriptor or file content.

`Advance`, the loop postconditions and `Bounded` prove the exact counter advance
for the consumed attempts and the 1000-attempt upper bound. Stage allocation
preserves all file bytes. `StageAdmitsSave` and `SaveAllocation` project a
successful allocation into the existing save controller's fresh-stage inputs:
directory present, file absent. A failed allocation can retain a new directory;
it must not be projected into a universal unchanged-filesystem guarantee.

## Export temporary files and copy outcomes

`ExportName` keeps the target prefix through its last **native** separator.
The other slash character has no separator meaning in this helper. With no
native separator the temporary name is relative to the working directory.
The name is `<prefix>.stars-race-export-<tick>-<counter>.tmp`. A directory-prefix
length greater than INT_MAX returns FALSE before sampling the clock. Otherwise
the tick is sampled once, and each of 1000 possible attempts advances the shared
counter before its formatting check.

After a fitting name, the helper clears errno, calls the exclusive copy and
returns TRUE on success. On failure it retries only if errno **after the entire
copy helper** equals EEXIST. The copy helper preserves target-open errno around
source close, but does not preserve errno around content/close failure cleanup.
Consequently an EEXIST result is not proof that the attempted temporary path
was an untouched preexisting collision. `ExportLoop` carries every reached
copy's store result into a retry. It performs no extra cleanup on formatting,
copy or retry-limit failure.

`Copy`, `ValidCopy`, `Wanted` and `Copied` describe reached open phases and byte
effects. Source-open failure leaves the destination unchanged. Target-open
failure can leave the old destination, or an empty destination where the
creation/open operation admits that effect. Exclusive creation cannot replace
an already present target. A successful content copy stores the requested bytes;
a failed content/close phase followed by attempted removal can leave no file or
a prefix, including the full bytes when failure occurred at close. S01's existing
`FDirectFailure` supplies that prefix law. Actual native open/error/path mapping
remains an S04 obligation.

`FCopyNativeFileContents` uses 8192-byte reads, detects short writes/read errors,
and treats either close error as failure. The nonexclusive wrapper opens `rb`
then `wb`; literal source/target equality therefore truncates the file before
reading. `Wanted` models that outcome as an empty successful copy. Both wrappers
attempt removal after a content/close failure, and ignore its result. This
differs from `FSaveLibraryRace`, which ignores its underlying close failure.
`FileCopyFrame` proves that literal non-target paths retain their bytes, and
`ExclusiveSourceFrame` proves literal-source preservation for admitted exclusive
copy receipts. These properties require the stated path-identity domain; they
do not establish alias independence on a real filesystem.

## Numbered suggestions and the duplicate-name boundary

`SuggestStem` drops the final dotted suffix, then reuses the existing maximal
valid UTF-8 prefix at 239 bytes. `SuggestLoop` formats `.r1` through `.r999` and
returns the first candidate whose availability query succeeds. It does not
search for maximum-plus-one; that is the separate version-save algorithm.
The loop proves the candidate range, absence of an earlier available candidate,
exact query count and empty-name exhaustion. Availability reflects the actual
query result, including enumeration failure being unavailable.

The source checks no final portable-name or root/path budget here. A 239-byte
stem can grow to 244 bytes with `.r999`; it still fits the 256-byte suggestion
buffer but fails the portable-name limit. Duplicate removes its source's last
suffix before calling this helper, so a dotted source stem is stripped again.
For example, `A.B.r1` proposes `A.r1`, not `A.B.r1`.

An input containing 236 ASCII `A` bytes plus `.r1` has length 239 and passes
`FPortableRaceFilename` with a 14-byte root (14 + 239 + 2 < 256). If only `.r999`
is available, suggestion produces 241 bytes and fails both final name/path
budgets. `FDuplicateLibraryRace` does not recheck either budget before its direct
copy and revalidation. [GAP-22](README.md#gap-22) records that observed boundary.
If such a file is written, `EnumLibraryRace` rejects its nonportable name before
checking size or delivering it to the list callback; canonical `FListed` remains
false even for a nonempty file. The source fixture checks that admission gate.
The source fixture establishes the admitted original and rejected suggestion;
the [action fixture](../tests/spec/check_race_actions_source.py) also exercises
the ordinary Duplicate controller through successful copy/revalidation under
scripted replies. Callee refinement, normal UI reachability and the intended
fix remain open with M02/S04/S05. No failing validation is silently added to C.

## Interaction, quality and handoffs

These are synchronous internal helpers with no independent screen, focus or
keyboard state. M02 owns naming/copy failure feedback and the outer controller;
S05 owns dialogs, native pickers and accessibility. Existing
[interaction requirements](M02-race-library.md#5-interaction-and-quality-requirements)
apply. A FALSE result does not by itself identify which partial artifact needs
recovery; consistent user-facing diagnostics remain proposed M02-IR04 work.

M02-QR02's deterministic bounds are the 1000 temporary attempts, 999 suggestion
queries, unsigned wrap and byte capacities, not a latency guarantee. M02-QR01
uses S04's UTF-8 prefix, without Unicode normalization or broader case folding.
M02-QR03 retains the per-helper cleanup differences. M02-QR04 performance targets
and QR05 display/accessibility targets remain unagreed/unmeasured. No crash
durability, exclusive reservation until publication or concurrent-instance
coordination property follows from these helpers.

## Acceptance, verification and open work

Acceptance/traceability: [M02-AC24–29](M02-race-library.md#m02-ac24).
`python3 tests/spec/check_race_allocation_source.py` compiles actual allocator,
suggestion, portable-name/enumeration, UTF-8 prefix and copy bodies. An independent Python
oracle checks names, counters, raw truncated outputs and every reached call in
2376 cases: wrap, first/last/exhausted attempts, zero/small/exact capacities, forced
format failures, both cleanup replies, both slash forms, malformed and multibyte
text, reserved names and suggestion growth. The fixture also runs 48 exclusive
real-stdio cases from the existing M01 copy fixture and 16 nonexclusive real-stdio
cases, including literal self-alias truncation and open failures. Tests use
warnings as errors; the new allocator and direct-copy builds enable address and
undefined-behavior sanitizers. Exclusive-copy fault providers remain separately
scoped; this is not a proof that all C executions refine the model.

[Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify
mouse/cursor/tracking and color/erase; native event/window/capture qualification
remains with M04/S05/S06.

The collection verification record reports the full include/audit, links,
handbook and repository checks. [Import/wrappers](M02-race-import.md) now
compose these helpers. [Page commands](M04-page-commands.md) now specify all six
command arms and partial commit; [initialization](M04-page-initialization.md)
now supplies all six entry arms and habitat layout. Still owned work: M04
full paint/helper producers and
M02/M04/S05 native modal lifecycle extraction. [Shared naming](M01-library-name-dialog.md)
and [wizard dispatch](M04-wizard-controller.md) now have explicit control contracts.
Other obligations remain S01 complete decoder and credential refinement; S04 actual OS error mapping, nonliteral path
aliases, native comparison and concurrent mutation; S05 UI/recovery/accessibility
qualification. GAP-08's legacy policy conflict and GAP-21's uncaptured suggestion
state remain unchanged. GAP-22 needs an explicit behavior decision and native
callee/GUI qualification before any runtime fix; ordinary Duplicate controller
cases are now covered.
