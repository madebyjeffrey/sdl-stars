# Milestone D: a C++ file-selection interface behind the SDL Win32 wrapper

Status: planned; implementation starts after Milestone C is complete and merged.
Recorded: 2026-10-31. Tracking: [#71](https://github.com/madebyjeffrey/sdl-stars/issues/71).

## Outcome and scope

Give the desktop's open/save file selection one owned C++ interface with a typed
request and a selected, cancelled or failed result. Keep `GetOpenFileNameA` and
`GetSaveFileNameA` as compatibility entry points, then move one real open caller
and one real save caller to the same operation. The compatibility browser and
Linux portal, macOS AppKit and Windows SDK pickers remain actual backends. Their
different implementations justify a shared seam; the C++ operation must own
validation, selection policy and lifecycle rather than pass arguments through.

This milestone changes the desktop module's internal interface, not the file
format or the player's open/save behavior. The command-line host remains SDL-free.
It does not redesign the visible picker, replace the native backends, convert the
game core, or start the broader domain-encapsulation work considered in
[the prospective D analysis](CPP-MILESTONE-D-ANALYSIS.md). That earlier analysis
remains a source of future candidates; this plan chooses the wrapper pilot first.

Milestones [A/B](CPP-CONVERSION-COMPLETION-ANALYSIS.md) already compile the owned
wrapper as C++20 and give it typed owners. [C](CPP-MILESTONE-C.md) removes the
remaining callback, test, source-language and symbol-routing scaffolding and
gives wrapper APIs a real namespace. D begins from C's qualified `develop` merge,
not from the older C11 checkout or an unfinished C worktree. D01 records the
actual parent revision and revalidates every source path below after C's renames.

## Current seam and behavior to preserve

At the inspected post-B tree, `filedialog.c` owns the `OPENFILENAMEA` validation,
filter parsing, initial directory, shared file/extension/overwrite policy,
owner/focus cleanup, current-directory change and output-field commit. Its
`ICompatChooseFile` backend interface takes a long parameter list plus mutable
filter index and path buffers. The compatibility browser and platform backends
implement the selection; the wrapper retries invalid selections. `nativeui.c`
forwards UI open/save calls to the Win32-shaped functions. Race-library import
and export in `libraryui.c` are candidate real callers for the first vertical
slice; D01 must confirm their names and coverage on the C parent.

Preserve the documented [file-dialog contracts](WIN32-FILE-DIALOGS.md), including:

- Ordered wildcard filters, `*.*`, selected one-based filter index, suggested
  filename, default extension and the `OFN_EXTENSIONDIFFERENT` result.
- Complete UTF-8 path bytes, fixed caller-buffer limits, basename/title and
  byte offsets. An insufficient `OPENFILENAMEA` buffer retains the required-size
  and `FNERR_BUFFERTOOSMALL` behavior; no output path is truncated.
- Distinct selected, cancelled and failed outcomes. Cancellation leaves the
  caller's filename and CWD unchanged and gives zero `CommDlgExtendedError`.
  Invalid input, unsupported flags, backend failure and allocation failure retain
  their existing error codes and `SetLastError` effects.
- `OFN_FILEMUSTEXIST`, `OFN_NOREADONLYRETURN`, overwrite confirmation and the
  current-directory policy. Choosing a save destination does not create a file.
- Main-thread use, live-owner checks after modal pumps, nested prompts, timer
  delivery, quit reposting, owner enable/focus restoration and cleanup before a
  pending callback error is delivered.

The canonical desktop behavior and evidence limits are in
[S05](../spec/S05-sdl-desktop-implementation.md); UTF-8 and native path rules
are in [S04](../spec/S04-platform-services.md). D does not strengthen these
contracts by assuming atomic rollback or full Win32 semantics.

## Interface design

Create a focused owned header for the file-selection operation, separate from
`wincompat_api.h` and the large wrapper `internal.h`. Use C++20 value types for
mode, title, initial folder/name, ordered filters, selected filter and the
currently supported selection policy. Return a tagged outcome with the selected
UTF-8 path and filter, or an explicit cancellation or error. The final names and
field types are decided in D02 against C's actual namespace and call sites.

The interface must state who owns each string and filter while a modal chooser
pumps messages; a borrowed view cannot outlive its source through that pump. It
must state which side owns the owner-window reference, how a destroyed owner is
reported, and whether a successful path changes CWD. A backend chooses a path;
the shared operation validates it, handles retry/overwrite policy, and commits
the CWD and result only under the existing success conditions. Keep native SDK
handles and AppKit/portal callbacks in their isolated adapters. No C++ exception
may cross an SDL, OS or Objective-C callback or a C/SDK reference boundary.

`GetOpenFileNameA` and `GetSaveFileNameA` translate supported Win32 inputs into
the request and translate the outcome back into the exact legacy output fields
and thread-local error state. They remain available for SDK comparisons and
callers that have not moved. Avoid a second generic interface that merely
duplicates `OPENFILENAMEA`, a virtual picker hierarchy without a need for it,
or separate policy implementations for new and legacy callers. Compare a plain
request/result free function with a concrete operation owner in D02; use an
owner only if it hides real modal state or cleanup obligations.

## Checkpoints and order

The D IDs are stable deliverables, not a required number of PRs. Each batch
stays buildable and is qualified under the verification gate below.

| ID | Deliverable | Depends on | Completion evidence |
| --- | --- | --- | --- |
| **D01 — post-C inventory** | Pin the merged C parent; list every chooser implementation and caller, active flags/output fields, tests, callbacks and fault paths. Record current open/save/cancel/error behavior and gaps. | C complete | The source, test and contract map covers the actual C tree; no C obligation is relabelled D. |
| **D02 — interface decision** | Compare free-function and operation-owner forms; fix request/result ownership, effects and error mapping in a small header. | D01 | A real caller and each backend can use the interface without learning `OPENFILENAMEA` output mechanics; modal lifetime and cancellation are explicit. |
| **D03 — shared operation and Win32 adapter** | Move shared validation, retry, output and lifecycle policy into one implementation. Keep both Win32 entry points as adapters. | D02 | Existing picker and SDK contract cases exercise the new operation through the adapter with unchanged observable results. |
| **D04 — backend handoff** | Replace the mutable argument/buffer backend handoff with the typed request/result across compatibility, portal, AppKit and Windows adapters. | D03 | Each backend remains isolated, preserves its platform behavior, and routes success/cancel/failure through the shared policy and callback cleanup. |
| **D05 — real UI callers** | Migrate race-library import and export, or the equivalent C-parent open/save pair, to the operation; retire their `OPENFILENAMEA` construction where no longer needed. | D04 | Actual open and save workflows pass through the same operation and retain filename, CWD, cancellation, error and publication behavior. |
| **D06 — qualification and expansion decision** | Qualify the final revision, update active documentation, compare caller knowledge and duplicate policy before/after, and choose the next wrapper family only if the pilot improved locality. | D01–D05 | All gates below pass; remaining Win32 callers and any deliberate exception are listed. The next slice is a separate scoped plan. |

Prefer focused commits for the interface, shared operation, backend migration and
caller migration. Do not leave a permanent old/new policy split after D05. If
D01 shows that C already supplied a suitable interface or changed these callers,
record it and revise the smallest remaining pilot instead of duplicating it.

## Verification and exit gate

After every implementation batch, build the platform SDL Debug preset with no
new warnings and run `make regression`, `make test-unit` and `make tutorial`.
Run the full relevant CTest desktop/compatibility suite, including
`win32_filedialog`, file workflow and callback/fault cases, plus a relevant
Release build. If a batch changes game or shared code, also run
`make regression-host` and `make test-host`; the final gate runs both regardless
to confirm SDL-free host behavior. Preserve the unchanged native regression
baseline; report any unexpected first diff rather than exporting a new one.

Qualify Linux, macOS and Windows backends on the final code, including the
compatibility picker on each platform, SDK/Wine comparisons where available,
UTF-8 paths, short buffers, invalid filters/flags, overwrite refusal, owner
destruction, nested/modal callback faults, cancellation, allocation failure and
shutdown/restart. Retain independent reference expectations. Run sanitizer and
native leak checks where supported. Record exact commands, counts, skipped
cases, warnings, environment and revision in a D implementation ledger. An
unrun platform or missing test remains an explicit open qualification item.

D is complete when the Win32 adapter and migrated UI callers share one file
selection policy, the four backend paths use the typed handoff, all documented
contracts and tests remain accounted for, and the final verification gate has
no unexplained differences. Update `WIN32-FILE-DIALOGS.md`, the roadmap and any
affected source/spec links with the implementation. If a specification or Dafny
rule changes, follow `spec/AGENTS.md` and its verification requirements. No
behavior change is part of this milestone; a desired picker improvement or bug
fix takes its own commit, changelog entry and baseline decision under the root
repository rules.

This plan records future work. Publishing it does not claim an implementation,
runtime result or behavior proof.
