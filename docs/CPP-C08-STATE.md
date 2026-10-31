# C08: explicit application state

C08 is in progress. The 156 game-state and 346 UI-state aliases are still present
at the first pattern checkpoint; this document does not mark their removal done.

## Reviewed entry pattern

The host resolves its existing owner at `main` and passes `APPSESSION &app` to
`FReadHostIni`. That function reads/writes the same settings through `app.state`;
its parsing, limits, return values and cleanup order are unchanged.

The desktop binds its existing owner at the application entry. Windows borrow
the context of their parent/owner, or the creating entry for a root window.
Dispatch binds the target window's context and restores the caller's context
through nested dispatch and failure capture. Thread timers capture the registering
context; queued callback messages retain it after dequeue and `KillTimer` until
dispatch. The context is non-owning, and its existing application owner outlives
its windows and timers. Public Win32 message/record layouts do not change.

The portable callback-binding service is independent of SDL. Its purpose is the
fixed-signature callback boundary; owned call chains will receive references
explicitly. Tests of context-free module ports can bind their observation owner
at the harness boundary without adding game dependencies to the platform port.

`About` is the first real callback using its target's owner directly. Its
signature, timer behavior, drawing and original field names remain unchanged.
The existing derived UI owner is visible in `uistate.h`; its fields and lifecycle
are unchanged. The remaining caller migration will retain one owner, remove both
borrowed-global definition/declaration sets and retire their forwarding getters.

## Pattern evidence

- The full desktop collection passes 151/151, including the available tutorial
  smoke checkpoint and 29 native leak checks. The independent host passes 27/27.
- Native/host baseline and exact C07 comparisons each match 519 files through
  turn 150, with 25 accepted unused-storage warnings and zero differences/errors/
  skipped inputs.
- Focused ASan/UBSan checks pass the game memory suite, callback fault collection
  and target-context test (3/3). The new game test creates two temporary owners
  and real About dialogs: a message updates the target's fields and restores the
  caller's binding. The generic test also covers nested windows, captured
  failure, queued thread-timer removal and destruction.
- Receipts are in `dist/verification/c08-entry/`. Remaining alias removal and
  final-tree qualification are pending; these receipts cover the entry pattern.
