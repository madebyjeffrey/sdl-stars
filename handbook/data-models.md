# Shared state and file relationships

Read the shared-state diagrams for logical entities, owner-scoped keys, tagged
variants, observations and recorded results. Then follow the file diagrams for
companion files, record groups, byte framing, orders and library storage.

Every diagram lives in its owning `spec/` document beside its canonical Dafny
contracts, named source evidence and scope notes. This index discovers those
sections on every build. Edit the owning spec once to update the diagram and
its navigation; no second diagram copy is maintained here.

## Reading the relationships

The entity diagram describes selected stable **full-host** relationships.
At a line end, `||` means one, `o|` zero or one, `o{` zero or many and `|{` one
or many. Additional count, ordering and validity constraints remain in the spec.
These marks do not imply a SQL schema, permanent ID uniqueness or validation by
every native reader.

Class-style boxes select existing logical field groups or tagged alternatives.
A solid arrow names an association, a dotted arrow a contextual dependency or
key reference, and a diamond a grouping of parts. Read their labels and scope
notes: a grouping is not a new C allocation/layout, and a reference can be stale.
Boxes listing variants do not hold all alternatives simultaneously.

Player observations can omit entities or retain stale knowledge. Full-host
reference closure must not be imposed on a filtered view. File records encode
selected fields and implicit ordering; they do not serialize native pointers,
allocation capacity or the complete execution context. The diagrams retain
existing Proposed boundaries, reader gaps and Open decisions.

Use **Fit to width** for the whole model and **Actual size** for labels, with
horizontal scrolling by touch or keyboard. For control flow instead, use the
[module atlas](diagrams.md) or [player and host sequences](sequences.md).
