# Cross-module player and host workflows

Follow a message from the actor's request through the module that owns the next
decision. The sequences below live in the owning `spec/` documents beside the
rules, canonical Dafny contracts, source evidence and coverage limits they explain.
This index is regenerated from those sections on every build.

Start with race design and new-game creation, then opening/planning/exporting a
player turn and importing its successor. The host sequences distinguish desktop
and CLI entry policy before following the shared resolution/publication engine.
File delivery between people is external; a saved or exported orders file does
not establish host receipt.

Lifelines represent **logical ownership**. A message can group several synchronous
C calls and shared-global effects; it does not imply a network service, a new API
or a transaction. `alt` marks alternatives, `opt` an optional step, and `loop`
repeated work. Read the scope notes for omitted native branches.

Cancellation remains entry-specific. A successful local step can survive a later
failure; host generation has no generic mid-phase Cancel or all-file rollback.
The diagrams preserve these observed limits and existing Open decisions. They do
not add rules or prove that C refines the models.

Use **Fit to width** to see a whole exchange and **Actual size** to read its labels.
Wide sequences scroll horizontally with touch or keyboard focus. For local module
decisions and recovery states, use the [module diagram atlas](diagrams.md).
