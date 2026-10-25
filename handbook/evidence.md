# Evidence and remaining coverage

Document presence is complete at the first module level. Rule-level
formalization and implementation qualification are still in progress. The
[completion register](../spec/README.md#completeness-review-and-completion-backlog)
is the authoritative inventory of those gaps.

| Follow this question | Destination |
| --- | --- |
| Which goal or criterion am I looking for? | [Generated requirement index](requirements.md) |
| Which model contains the definition? | [Generated Dafny reference](models.md) |
| What work remains in each module? | [Module completion inventory](../spec/README.md#module-by-module-completion-inventory) |
| How do I reproduce formal checks? | [Verification commands](../spec/README.md#reproducing-the-checks) |
| What was checked in a previous batch? | [Dated verification records](../spec/README.md#dafny-coverage-and-verification) |
| Which source revision produced this site? | [Build provenance](build.md) |

## What a site build establishes

The handbook build checks local specification links and requirement references,
then checks the rendered site's local links, fragments, and assets. Its indexes
are generated from the current files. Rendering documentation does not run
Dafny, C tests, game regressions, or desktop qualification.

Run `python3 tests/spec/verify.py --time-limit 120` separately to verify and audit
the formal collection. Existing verification records retain their own dates,
revisions, assumptions, and limits. A newer site build does not renew those results.
