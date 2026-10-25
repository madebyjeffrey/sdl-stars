# Follow a story to its rules and evidence

Choose a module below, open a story, and follow its acceptance criteria to the
authored rule/evidence text, Dafny declarations and test resources. Each module
view supports story and text filters; links to a criterion remain shareable.
The full trace stays readable when JavaScript is disabled.

[Dafny model backlinks](trace-models.md) start from a formal file.
[Test and evidence-resource backlinks](trace-evidence.md) start from a resource.
[Download the trace data](traceability.json) for the same relationships as JSON.
The [requirement index](requirements.md) retains interaction and quality IDs too.

## What each link means

| Link type | Meaning |
| --- | --- |
| Story → criterion | The criterion's authored parent-story reference; companions use their module's story IDs. |
| Criterion file link | A model or resource linked in that criterion's row/paragraph. |
| Exact declaration name | A code-formatted name with one declaration match in models linked by the source document and their include closure. The declaration page also links to the exact source line. |
| Named test file | A code-formatted test path, or a filename with one match under `tests/`. This resolves a resource, not the scope or result of a test run. |
| Source-document context | Models, include dependencies and evidence resources mentioned elsewhere in that document. These are kept separate from criterion-level associations. |

The module views preserve the authored acceptance, rule, proof/test and gap text.
Where a table combines evidence and limits in one cell, that combination remains
visible. Ambiguous declaration names stay unresolved; unnamed test descriptions
remain prose. A missing file-level link does not establish missing test coverage.

Behavior status and evidence are independent. **Observed**, **Proposed** and
**Open** retain the meanings in the [spec conventions](../spec/README.md#requirement-ids-and-status).
Historical runs retain their source revision, date, assumptions and environment.
A new handbook build does not renew a test run, prove C/model refinement, or
close an external obligation. Read the [coverage register](evidence.md) alongside
these views; counts describe trace structure.

The specs and canonical Dafny files are the source of truth. All views regenerate
from their current anchors, trace rows, links, names and includes. To change a
mapping or its evidence, edit the owning spec and its linked contract/evidence,
following [the maintenance guide](maintaining.md#maintain-traceability).
