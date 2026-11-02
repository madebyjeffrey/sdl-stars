# Milestone D: a C++ file-selection interface behind the SDL Win32 wrapper

Status: D01–D02 complete on `develop`; D03–D06 remain open.
Recorded: 2026-10-31. D01 revalidated: 2026-11-01. Tracking: [#71](https://github.com/madebyjeffrey/sdl-stars/issues/71).

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

At the inspected post-B tree, `filedialog.cpp` owns the `OPENFILENAMEA` validation,
filter parsing, initial directory, shared file/extension/overwrite policy,
owner/focus cleanup, current-directory change and output-field commit. Its
`ICompatChooseFile` backend interface takes a long parameter list plus mutable
filter index and path buffers. The compatibility browser and platform backends
implement the selection; the wrapper retries invalid selections. `nativeui.cpp`
forwards UI open/save calls to the Win32-shaped functions. D01 confirms the
race-library import/export candidates and their current coverage below.

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

## D01 — post-C source and test inventory

The Milestone C merge is `a65e653a` (PR #68), on `develop`. The D01 source
inventory was revalidated at `f3b89311` after the current L05 cleanup. Public
Win32-shaped entry points remain necessary for SDK compatibility and reference
tests; the application call sites below are candidates for the new typed
operation.

| Layer | Current implementation | Responsibility or remaining coupling |
| --- | --- | --- |
| Application to wrapper | `src/ui/nativeui.cpp`: `FNativeOpenFileName` / `FNativeSaveFileName`; `compat/win32/src/calls.cpp`: `ui::GetOpenFileName` / `ui::GetSaveFileName` | The app uses `DesktopChoiceIO` function pointers. `ui::` forwards to `win32::` and then delivers pending callback errors. The caller still constructs `OPENFILENAMEA`. |
| Shared operation | `compat/win32/src/filedialog.cpp`: `FFileDialog`, `GetOpenFileNameA`, `GetSaveFileNameA` | Validates the ABI record and supported flags, parses filters, owns retry/overwrite and selected-path validation, handles CWD and commits output fields. One implementation serves both native and compatibility backends. |
| Compatibility picker | `FFileDialog` plus `LayoutFileDialog`, `FileDialogProc`, directory/accept helpers in `filedialog.cpp` | Owns the constructed browser and controls when compatibility mode is selected. |
| Backend seam | `compat/win32/src/filedialog_backend.h`: `ICompatChooseFile` | Positional arguments, borrowed label/folder/name/filter pointers, mutable selected-filter pointer, writable path plus byte capacity; returns selected/cancelled/failed as `1/0/<0`. This is the principal untyped handoff D04 replaces. |
| Linux | `filedialog_portal.cpp` | Portal request/response lifecycle and selected URI/path conversion; native method errors are distinct from cancellation. Private D-Bus and X11 coverage is configured on Linux. |
| macOS | `filedialog_cocoa.cpp` | AppKit open/save panels and callback boundary; adapter test covers labels, paths and cancellation. |
| Windows | `filedialog_windows.cpp`, `filedialog_windows_sdk.cpp`, `filedialog_windows_bridge.h` | SDK Unicode adapter and test seam; SDK records remain isolated here. Native adapter and flow tests are Windows-only. |
| Other picker | `CompatFolderIO` callbacks in `libraryui.cpp` | Folder selection is already a separate asynchronous callback interface; it is not an `OPENFILENAMEA` call and remains outside D. |

Application `OPENFILENAMEA` call sites at the D01 parent:

| Function | Mode and purpose | Caller currently provides |
| --- | --- | --- |
| `src/ui/mdi.cpp` — `FOpenGame` | Open a game or race file | 256-byte path/title buffers, game/race filter, initial directory and `HIDEREADONLY`, `PATHMUSTEXIST`, `FILEMUSTEXIST`. |
| `src/ui/raceui.cpp` — `FSaveRace` | Save a race design | 256-byte path/title buffers, `.r1` default extension, overwrite and writable-destination policy. |
| `src/ui/libraryui.cpp` — `FExportLibraryRace` | Save a race-library export | 512-byte path, race filter, overwrite prompt. |
| `src/ui/libraryui.cpp` — `FExportLibraryOrders` | Save exported player orders | 512-byte path, per-player filter and `xN` default extension; a separate replacement prompt applies to the corrected final path. |
| `src/ui/libraryui.cpp` — `FImportLibraryRace` | Open an external race design | 512-byte path, race filter, existing-file requirement. |
| `src/ui/libraryui.cpp` — `FExportLibraryGame` | Save a library game as ZIP | 512-byte path, ZIP filter and default extension. Folder export is a separate folder callback. |
| `src/ui/libraryui.cpp` — `FChooseLibraryImportFile` | Choose a game/race/ZIP/universe import source | Caller-provided path span (current use is 512 bytes), file-title output and import filter. |
| `src/ui/libraryui.cpp` — `FImportLibraryOrders` | Open an external orders file | 512-byte path, player-orders filter and existing-file requirement. |
| `compat/win32/src/print.cpp` | Print-to-file destination | Calls `GetSaveFileNameA` internally; printing is not a selected D05 caller. |

Existing contract and fault-path evidence is split across `tests/compat/`:
`win32_filedialog.inc` exercises modal controls, outputs, cancel, filter,
extension, CWD, errors and owner destruction; `win32_filedialog_windows.cpp`
and `win32_filedialog_windows_flow.cpp` exercise the SDK adapter and Windows
flow; `win32_fileportal.cpp` and `run_fileportal.py` exercise private-bus
responses/retries/errors; `win32_filedialog_cocoa.cpp` covers the AppKit
adapter; `modal_ownership.cpp` and `callback_context` cover nested callbacks,
cleanup faults and pending-error delivery; `win32_fileworkflow.cpp` and its
runners exercise real game/race/library workflows. CMake gates portal, X11,
Cocoa and Windows-native cases by platform/backend. D01 is complete.

D02 selects the request/result form from the application needs and
contracts above. The pilot remains `FImportLibraryRace` and
`FExportLibraryRace`; other application callers keep compatibility entry points
until the shared operation and backend handoff are proven.

## Interface design

`compat/win32/include/file_selection.h` defines the focused C++ interface,
separate from `wincompat_api.h` and the large wrapper `internal.h`. It uses
owned strings and filters, a typed mode/policy, an optional caller path budget,
and a selected/cancelled/failed variant. This is the D02 decision against the
post-C source and call-site inventory.

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
or separate policy implementations for new and legacy callers. Use one
synchronous free function, `win32::files::SelectFile`, rather than an
operation-owner class. The modal operation does not escape the call: its current
dialog state and cleanup are stack-scoped, and returning a class would add a
lifecycle surface without hiding state that must persist. The request is passed
by value and owns every string and ordered filter across the modal pump. `HWND`
is a separate borrowed argument, revalidated after callbacks. The selected
result owns the complete path, returns a zero-based optional filter index and
extension-difference flag, and never exposes `OPENFILENAMEA` output pointers.
An optional byte-capacity limit includes NUL; `bufferTooSmall` reports the full
required byte count so the legacy adapter can preserve its two-byte required
size behavior. Typed failure categories let the adapter map dialog and
thread-local error effects back to the existing codes. The interface is
declared in D02; D03 supplies the shared implementation.

## Checkpoints and order

The D IDs are stable deliverables, not a required number of PRs. Each batch
stays buildable and is qualified under the verification gate below.

| ID | Deliverable | Depends on | Completion evidence |
| --- | --- | --- | --- |
| **D01 — post-C inventory** | Pin the merged C parent; list every chooser implementation and caller, active flags/output fields, tests, callbacks and fault paths. Record current open/save/cancel/error behavior and gaps. | C complete | The source, test and contract map covers the actual C tree; no C obligation is relabelled D. |
| **D02 — interface decision** | Compare free-function and operation-owner forms; fix request/result ownership, effects and error mapping in a small header. | D01 | Complete: `file_selection.h` declares the owning request, explicit outcomes/error categories, borrowed owner lifetime and path-limit contract. D03 must prove the existing callers and adapters fit this seam. |
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
