# C08: explicit application state

C08 is qualified in draft PR #75 at `14b4bc39`. All 502 C01-inventoried aliases and the two older
file-XOR seed aliases are removed. No `GetAppSession`, `GetAppUiState` or
`UiSession` accessor remains. The [per-alias ledger](CPP-C08-STATE.json) records
all 158 game-state and 346 UI-state references and their replacements.

## Explicit owners and callback entries

The host references its actual owner at `main` and passes `APPSESSION &app` to
`FReadHostIni`. That function reads/writes the same settings through `app.state`;
its parsing, limits, return values and cleanup order are unchanged.

`host::app` and `ui::app` are the actual backend-owned values, constructed before
`main` and SDL as before. Only application/test entry points reference them;
owned calls receive `APPSESSION &app`. There is no replacement session locator.
Core fields use `app.state`; UI fields use `app.lpui`, a fixed non-owning pointer
to the existing derived owner's UI value. The core needs only a forward UI type,
so the host still builds without SDL/Windows headers. Reset assigns fields in
place and keeps addresses, heap roots and the UI borrow stable.

The desktop binds its owner at the application entry. Windows borrow
the context of their parent/owner, or the creating entry for a root window.
Dispatch binds the target window's context and restores the caller's context
through nested dispatch and failure capture. Thread timers capture the registering
context; queued callback messages retain it after dequeue and `KillTimer` until
dispatch. The context is non-owning, and its existing application owner outlives
its windows and timers. Public Win32 message/record layouts do not change.

The portable callback-binding service is independent of SDL. Its purpose is the
fixed-signature callback boundary; owned call chains receive references
explicitly. Tests of context-free module ports can bind their observation owner
at the harness boundary without adding game dependencies to the platform port.

`About` is the first real callback using its target's owner directly. Its
signature, timer behavior, drawing and original field names remain unchanged.
The existing derived UI owner is visible in `uistate.h`; its fields and lifecycle
are unchanged. All owned call chains and their linked fixtures now pass the owner explicitly.
Context-free platform/module ports keep their signatures; their test providers
borrow the harness observation context at that fixed callback boundary. The
private heap owner retains its application reference through idempotent teardown.
Owned AI/directory/log/tile callbacks carry references explicitly. The native
`bsearch` comparator remains two-argument `QSORTCOMPARE`; `APPQSORTCOMPARE` carries
the owner through the unchanged Win16 sorting loop. No comparator calls, integer
conversion or tie-order boundaries are reordered.

The maintained archived C observer keeps its native ABI. Its C++ type projection
binds observation fields locally at the fixed timer entry and uses one explicit
arity macro for calls into current owned functions. This is confined to that
foreign reference input; it adds no application symbol or global-state routing.
The tutorial parser accepts the explicit owner argument in `FTutorTaskDone`;
original names, instruction IDs and observation fields stay unchanged.

## Qualification evidence

- The full desktop collection passes 151/151, including the available tutorial
  smoke checkpoint and 29 native leak checks. The independent host passes 27/27.
- Native/host baseline and exact C07 comparisons each match 519 files through
  turn 150, with 25 accepted unused-storage warnings and zero differences/errors/
  skipped inputs.
- The selected ASan/UBSan collection passes 37/37, including every linked witness,
  callback/ownership guards, memory, cursor/type and ABI contracts. The new game test creates two temporary owners
  and real About dialogs: a message updates the target's fields and restores the
  caller's binding. The generic test also covers nested windows, captured
  failure, queued thread-timer removal and destruction.
- All 27 witness receipts preserve every original case/count/exclusion line; all
  67 independent Python definitions retain identical AST hashes. Strict manifests
  cover 157 production sources / 158 variants and 160 standalone headers in both
  include orders. Thirty representative MinGW C++ units and all 38 genuine SDK C
  vector entries compile. This is compile evidence, not the remaining SDK matrix.
- The trace-enabled, native-chooser-disabled build passes 31 smoke comparisons.
  All 773 trace records match the parent arguments/results/prior seeds after
  excluding executable return addresses. Documentation checks pass 26 tests and
  2,819 links / 700 requirement IDs. No canonical model or baseline changes.
- The first reviewed entry pattern is commit `006a7eb`; its receipts remain in
  `dist/verification/c08-entry/`. Current receipts are in `dist/verification/c08/`.
  All 14 enabled PR jobs and all five corrected manual platform jobs pass.
  The final C09/C10 matrix and precise SDK diagnostic policy are recorded in the
  [final qualification ledger](CPP-C10-QUALIFICATION.md).

The first manual Windows build found two filename-fallback reads behind `_WIN32`
that still used the removed `szBase` alias. Both now use the same passed owner's
`app.state.szBase`; slash/drive fallback order and bytes are unchanged. The full
`mdi.cpp` translation unit passes strict MinGW syntax compilation. Corrected
platform qualification passes at `14b4bc39`: Linux 154/154, Windows SDK probes 2/2,
game units 13/13 and platform checks 88/88. The first run is not reported as passing.
