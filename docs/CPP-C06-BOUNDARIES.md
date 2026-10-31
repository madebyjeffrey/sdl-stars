# C06: runtime namespaces and error boundaries

Parent: `c6940721a3ec258fc182b6d5cfa3192c52b08e6c` (C04/C05 PR #72). Qualification is active on
`codex/cpp-c06-namespaces`.

## Operations

All 23 public names move directly to `ui::` operation basenames. No old-name
aliases or second forwarding layer remain. Each implementation has a distinct
contract: raw dispatch/modal/window work returns and finishes cleanup before
`StarsRethrowPendingError` delivers a captured exception. Runtime internals use
`win32::`, preserving foreign-callback containment. Signature assertions compare
every checked operation with its raw counterpart. Default dialog arguments stay
zero. All 23 bodies have identical normalized tokens after namespace qualification;
the three moved native bodies are token-identical. Popup selection retains its
typed choice effect and real raw default.

| Removed name | Public operation | Raw operation |
| --- | --- | --- |
| `LUiCallWindowProc` | `ui::CallWindowProc` | `win32::CallWindowProcA` |
| `HwndUiCreateWindowEx` | `ui::CreateWindowEx` | `win32::CreateWindowExA` |
| `HwndUiCreateDialog` | `ui::CreateDialog` | `win32::CreateDialogParamA` |
| `LUiDialogBox` | `ui::DialogBox` | `win32::DialogBoxParamA` |
| `FUiDestroyWindow` | `ui::DestroyWindow` | `win32::DestroyWindow` |
| `LUiDispatchMessage` | `ui::DispatchMessage` | `win32::DispatchMessageA` |
| `FUiGetMessage` | `ui::GetMessage` | `win32::GetMessageA` |
| `FUiPeekMessage` | `ui::PeekMessage` | `win32::PeekMessageA` |
| `LUiSendMessage` | `ui::SendMessage` | `win32::SendMessageA` |
| `LUiSendDlgItemMessage` | `ui::SendDlgItemMessage` | `win32::SendDlgItemMessageA` |
| `FUiShowWindow` | `ui::ShowWindow` | `win32::ShowWindow` |
| `FUiEnableWindow` | `ui::EnableWindow` | `win32::EnableWindow` |
| `HwndUiSetFocus` | `ui::SetFocus` | `win32::SetFocus` |
| `FUiSetWindowPos` | `ui::SetWindowPos` | `win32::SetWindowPos` |
| `FUiMoveWindow` | `ui::MoveWindow` | `win32::MoveWindow` |
| `FUiUpdateWindow` | `ui::UpdateWindow` | `win32::UpdateWindow` |
| `FUiSetWindowText` | `ui::SetWindowText` | `win32::SetWindowTextA` |
| `FUiIsDialogMessage` | `ui::IsDialogMessage` | `win32::IsDialogMessageA` |
| `IUiTranslateAccelerator` | `ui::TranslateAccelerator` | `win32::TranslateAcceleratorA` |
| `FUiTrackPopupMenu` | `ui::TrackPopupMenu` | `win32::TrackPopupMenu` |
| `LUiDefWindowProc` | `ui::DefWindowProc` | `win32::DefWindowProcA` |
| `FUiGetOpenFileName` | `ui::GetOpenFileName` | `win32::GetOpenFileNameA` |
| `FUiGetSaveFileName` | `ui::GetSaveFileName` | `win32::GetSaveFileNameA` |

The raw API header declares 223 operations in the actual `win32` namespace;
`win32::InitCommonControlsEx` also moves from the common-control header.
Generated Windows symbol macros and their include directory are deleted.
`ui::WinMain` remains an owned C++ game entry; SDL's process entry stays global.
Narrow A/W forwarding macros that intercepted checked basenames and unused
dialog-creation macros are deleted. Other compatibility helpers remain C07 work.
No spelling-only forwarding function is introduced; required SDK-shaped exports
and cleanup/error boundaries retain their separate contracts.

## Type and SDK isolation

Wrapper handle and record tags belong to `win32`. Original application width/type
names remain aliases of the actual types. Core/UI handles and frame geometry have
one SDK-independent definition in `src/platform/windowtypes.h`. Rectangle fields
remain signed 32-bit. `win32::MSG` has no global owned alias: SDL's Windows system
header reserves global `MSG` for a native SDK hook. Shared test entries choose
`APPMSG` as the actual wrapper or SDK type explicitly.

Paired ordinary and SDK compile contracts check matching wrapper identities and
different SDK identities for 13 handles and six records. The chooser, filesystem
and process services consume real SDK declarations in isolated translation units.
Windows command-line/tick services move unchanged to `native_windows.cpp` with
scalar/UTF-8 declarations in `native_process.h`; that SDK file includes no game
records or wrapper headers.

All 38 independent SDK C inputs compile. Their normalized token receipts in the
JSON sidecar preserve every constant, argument and assertion. Only removed API
aliases/type spellings change. Cursor main vectors retain the same positive and
negative cases through an explicit typed callback; the frozen C reference keeps
its historical injection boundary. The archived observer keeps C-compatible SDK
calls; its current C++ type-check entry imports the raw namespace explicitly.

## Qualification

The corrected head passes a clean full 149/149 desktop run. An earlier full
collection passed 145 checks and timed out four workflows during logged macOS
idle/maintenance sleep; all four isolated retries passed unchanged limits with
a temporary process-scoped sleep assertion. The preceding 148-check run passed.
All 29 native leak checks and the available tutorial smoke checkpoint pass.
The final 34-check ASan/UBSan selection passes all linked witnesses, save/profile,
callback guards and the cursor/type contracts; macOS uses native leak inspection.

Fresh SDL-free Release host tests pass 27/27. Final native and host regressions
match all 519 baseline files through turn 150, each with 25 accepted unused-storage
warnings. Exact C04/C05 comparisons match 519 files with 25 native / 23 host
accepted warnings. All comparisons have zero differences/errors and skipped
inputs. The strict manifest accounts for 156 production sources / 157 variants
and 158 standalone headers in both include orders. Documentation checks pass 26
tests and 2,819 links / 700 requirement IDs.

All 38 independent SDK C entries and 29 representative C++ application/provider/
isolated SDK translation units compile, including ordinary wrapper ABI assertions;
the paired SDK identity contract separately compiles. This is compile evidence.
The corrected head passes all five jobs in the manually enabled platform
workflow: Linux desktop 153/153, Windows probe 2/2, selected game units 13/13
and platform checks 88/88. Automatic CI defaults stay unchanged. Canonical
model files, record layouts, catalog IDs, frozen C inputs and baselines are
unchanged. C05's full unchanged model receipt remains 12,000 verified / zero
errors and zero audit findings; no new proof run or whole-program refinement
is claimed here. Local logs and
comparison reports are retained in `dist/verification/c06/`.


The first manual Windows build catches `WINAPI` redefinition when SDL intrinsics
precede the wrapper. Owned C++ declarations/definitions now omit their empty
`WINAPI`/`WINAPIV` annotations, and the unused `APIENTRY` alias is deleted. Native
SDK declarations and callback annotations retain their real conventions. The
C++ ABI assertion explicitly uses the same native C++ function type; its SDK C
branch is unchanged. The type contract includes SDL first, checks native SDK
`MSG` separation, and compiles with an inherited `WINAPI=__stdcall` definition.
The corrected local build and strict MinGW regression compile checks pass.
The clean desktop run and corrected platform workflow above complete the reruns.
See the [manual workflow](https://github.com/madebyjeffrey/sdl-stars/actions/runs/41206840795).
