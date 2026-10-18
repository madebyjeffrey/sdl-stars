# Dialog creation and modal lifecycle

`CreateDialog`, `DialogBox` and `EndDialog` now have tested basic implementations,
bringing inventoried API coverage to **167/178 (93.8%)**, with **11 remaining**.
The six CRT helpers and supporting `MapDialogRect`/`IsDialogMessage` APIs are
counted separately. The full SDL game client remains unlinked.

## Implemented behavior

Dialogs use the bounded [template parser](WIN32-DIALOG-TEMPLATES.md), existing
window records and standard control implementations. Template fonts determine
dialog units from the alphabet width and text height. Controls retain their
IDs, text, styles and independent window state. The dialog owns its font and
releases it after child teardown. Static icon controls draw through the existing
resource image path.

The runtime creates controls before `WM_INITDIALOG`, passes the initialization
parameter and proposed focus control, and honors the callback's focus return.
`DWLP_USER`, `DWLP_DLGPROC` and `DWLP_MSGRESULT` use native pointer widths.
Modeless dialogs have independent lifetimes and use `DestroyWindow`.

Ordinary native owned windows detach before destruction: Cocoa otherwise can
expose an unrelated hidden gameplay frame when a library dialog closes. Native
popup/tooltip windows retain their required parent through destruction.

Modal dialogs disable their root owner, pump the existing message queue and
restore owner/focus state on exit, including native activation once the owner
is re-enabled. Hidden/disabled owners and pending application quit do not
activate; a close callback selecting another window keeps its focus. `EndDialog` stores a result and defers
destruction until the callback returns. Ending during initialization, posted
completion and nested modal dialogs are tested. Pending `WM_QUIT` is reposted
when a modal loop exits. Owner destruction and missing resources cannot leave
a live modal loop behind.

Basic dialog keyboard routing supports Tab/Shift-Tab and Return/Escape. Tab
visits enabled, visible direct child tab stops; Return selects the template's
default pushbutton. `WM_GETDLGCODE` requests for all keys or Tab are respected.

## Scope and remaining work

The canonical resources are embedded, and a native creation sweep now succeeds
for **34/34** templates using Button, Static, Edit, ListBox, ScrollBar and
ComboBox classes. See [combo-box contracts](WIN32-COMBOS.md) for editable and
fixed-height owner-drawn support and remaining limits. Custom dialog classes/menus, nonempty creation data,
extended templates, deeper keyboard/group/mnemonic navigation and full dialog
default-button semantics remain unqualified. `EndDialog` accepts modal dialogs;
modeless callers must use `DestroyWindow`.

The wrapper preserves an initially disabled modal owner as described in the
[DialogBoxParam contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dialogboxparama).
The SDK test under this Wine installation instead re-enables that owner when
the dialog ends during initialization. The test prints that observed state
explicitly and checks the documented preservation rule in the native runtime.
This platform difference is not treated as identical behavior.

Native window behavior is tested on X11; Wayland remains unqualified.

## CLion and verification

Select `stars-compat-dialog` under `sdl-debug` or `sdl-release`. It is a finite
test executable with breakpoints available in creation, callbacks and modal
dispatch. No additional libraries are needed.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
xvfb-run -a ctest --test-dir dist/sdl-debug -R win32_dialog --output-on-failure
```

The managed-race client test also checks filename-dialog centering after moving
the library, plus native window visibility after filename/wizard cancellation.
To run just these checks on a macOS desktop:

```sh
SDL_VIDEODRIVER=cocoa python3 tests/compat/run_racelibrary.py \
  dist/macos-sdl-debug/bin/stars-compat-client-startup --dialogs-only
```

Shared SDK/native tests cover independent modeless state, font-based geometry,
initialization focus, tab routing, destruction notification order, immediate
and posted modal completion, nesting and owner restoration. Native tests also
verify dialog font release and stale-handle rejection. Full-runtime address,
undefined-behavior and leak sanitizer checks pass.

SDL Debug and Release suites pass **41/41** each; the Windows SDK/unit suite
passes **45/45**. The final About/default-module and lifecycle checks also pass
against the SDK and both native backends.

The fresh tutorial failed at year 2407, page 24, instruction 186 after 94
actions with an observer timeout; its Wine log contains
`free(): invalid next size (fast)`. Artifacts:
`dist/scaffold/tutorial/20261011T002839.658550Z`. The Windows tutorial does
not link this SDL runtime; the failure origin remains unresolved.
Native regression matched all **519** checkpoints, with 22 existing warnings
and zero differences/errors or skipped reference inputs. The Linux host build
and **12/12** unit tests pass. No game behavior or regression baseline changed.

## Modal result preservation

After an application calls `EndDialog`, default handling no longer replaces the
result when the callback returns false. Destruction is deferred until the modal
loop returns, so checking only `IsWindow` was insufficient. This caused Stars'
production Cancel to commit its temporary queue. Shared SDK/native tests cover
zero and custom results with false callbacks; loaded-game workflow tests check
production cancel, commit and reopen through inspected controls and SDL clicks.
The [EndDialog contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enddialog)
requires returning the application's result after deferred destruction.

## OK/Cancel commands inside edit modes

Default `WM_COMMAND` handling leaves modal termination to the application.
It must not call `EndDialog` automatically for `IDOK` or `IDCANCEL`: Stars'
ship designer uses those commands to accept/cancel an edit and return to its
main controls within the same modal. Shared SDK/native tests post an unhandled
OK/Cancel command before an explicit completion message and require the latter's
result. The SDK reference passes; the old local default handler fails.
This follows the application-owned termination described in Microsoft's
[dialog programming guidance](https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgbox-programming-considerations).
