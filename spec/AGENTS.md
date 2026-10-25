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

## Stories, requirements and traceability

Use [the module outline](README.md#outline-for-each-detailed-module-specification)
and its ID/status conventions for entry documents and technical companions.

1. Identify actors, goals, prerequisites and the requested module boundary.
   Write stable story IDs for user goals; for support modules name the consuming
   developer/operator and the player/host outcome enabled. A source-derived
   story explains intent; it is not evidence of user research.
2. Connect each story to a main workflow and relevant alternatives, cancellation,
   failures and recovery. State the resulting observable state and handoffs.
   Preserve known limitations when describing current behavior.
3. Give each story testable acceptance criteria with stable IDs. Trace each
   criterion to its authoritative rule/model or a specific external obligation,
   source evidence, and test/proof evidence or an explicit coverage gap.
   Keep behavior status separate from whether an implementation or test exists.
4. Record interaction and quality requirements using the outline's fields.
   Include keyboard, focus, feedback, accessibility, text, performance and
   supported-display concerns where applicable. For technical companions,
   explain the caller interface and link the consuming UI instead of inventing
   a screen. Mark unagreed targets and unmeasured properties Open.
5. Keep stories at their owning module entry. Companions link those story IDs
   and add focused criteria/traceability for their own rules. Preserve existing
   IDs and anchors; retain retired IDs with a successor link. Historical
   references point to current owners and keep their historical status.
6. Before completion, account for every story, criterion and applicable quality
   concern. Validate links/IDs and the formal closure. Record actual checks,
   unresolved decisions and test gaps; a structural documentation pass does
   not make proposed behavior implemented or an untested workflow verified.

## Formal specification workflow

Use Dafny as the canonical notation for specified data, constraints, calculations,
state transitions and acceptance conditions. Apply this when creating a spec,
extending an existing one, or changing a rule. Keep Markdown for purpose,
explanation, source evidence, ownership, verification results and open decisions.

1. Inventory every rule in the requested scope. Reuse existing shared models;
   give each rule a named Dafny definition or a specific documented boundary
   explaining why it cannot yet be represented faithfully.
2. Prefix module files with the uppercase module ID (`M05-`, `S01-`, etc.).
   Link Markdown explanations to the authoritative definitions. Mark proposed
   validation separately from observed runtime behavior and undefined inputs.
3. Preserve units, integer widths, signed division, floating rounding boundaries,
   RNG consumption and operation order. Model external results explicitly with
   checked conditions; mathematical reals alone do not reproduce binary64.
4. Add useful positive/negative witnesses and general properties for the rules
   migrated. Give functions checked bodies and prove claimed postconditions;
   do not replace a missing proof with assumptions, axioms, disabled verification
   or an unrelated weaker property.
5. Verify the full include closure with `dafny verify --verify-included-files`,
   audit every relevant file with `dafny audit`, and check links/includes with
   exact filename casing. Run `python3 tests/spec/verify.py` from the repository
   root for the collection-wide checks; use its time-limit option if needed.
   Record commands, version, counts and actual failures.
6. Finish only when the coverage inventory accounts for every in-scope rule,
   proofs and checks pass, and Markdown distinguishes definitions, proved
   properties, external obligations and remaining limits. A verified model is
   not evidence that the C implementation refines it.

Before finishing, check every relative link, review the diff for unsupported
behavior claims, and ensure each new detail is either sourced, proposed, or
explicitly unresolved. Report verification actually performed. Follow the root
verification rules for repository edits; keep runtime changes separate from
specification-only changes.
