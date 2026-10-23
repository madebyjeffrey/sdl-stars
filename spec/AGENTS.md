# Working on SDL-Stars specifications

This folder contains the repository-local specification work requested by the
user. The root AGENTS.md still applies.

## Start here

Before extending or decomposing a module, read [README.md](README.md). It holds
the first-level module map, evidence, unresolved questions, and specification
outline. Keep that map as the entry point as detailed documents are added.

Follow [domain guidance](../docs/agents/domain.md) for vocabulary and decisions.
Before publishing specs or creating tracking issues, read the
[issue-tracker guidance](../docs/agents/issue-tracker.md). This folder is the
requested working document destination; publication to GitHub is separate work.

## Scope and evidence

- Develop the specifications from high-level modules toward smaller workflows
  and rules. Expand the module requested in the current task; keep other modules
  at their existing level of detail.
- Distinguish observed behavior, proposed module ownership, and open questions.
  A candidate module is not evidence of an isolated implementation or an approved
  refactor. Specification work alone does not authorize gameplay changes.
- Verify behavior against current source and relevant tests. Link the source
  files and name the functions supporting a claim. State the inspected revision
  and identify historical documents whose details need revalidation.
- Describe a module through its purpose, state, interface, rules, and observable
  outcomes. Use source filenames as evidence rather than as the module taxonomy.
- Keep shared rules in one place and link to them. Record ownership questions
  explicitly when workflows, UI code, and turn resolution overlap.
- Preserve glossary terminology. Surface conflicts with accepted ADRs or root
  instructions as unresolved questions; resolving them requires a separate
  decision, not an assumption embedded in a draft.

## Extending the map

Use the specification outline in README.md. Create a focused Markdown document
when a module needs a detailed breakdown, then link it from its existing map row.
Keep its stable module ID when refining its name; record splits or mergers so
older references remain understandable.

Before finishing, check every relative link, review the diff for unsupported
behavior claims, and ensure each new detail is either sourced, proposed, or
explicitly unresolved. Report verification actually performed. Follow the root
verification rules for repository edits; keep runtime changes separate from
specification-only changes.
