# Issue tracker: GitHub

Issues and specs live in GitHub Issues for madebyjeffrey/sdl-stars.
Use the gh CLI from this repository.

## Operations

- Publish a ticket or spec by creating a GitHub issue.
- Read tickets with gh issue view --comments, including body, labels, and comments.
- List tickets with gh issue list and appropriate state/label filters.
- Create, edit, and comment using --body-file for multiline content.
- Apply the labels defined in docs/agents/triage-labels.md.
- Link child tickets using GitHub sub-issues when supported.
  Otherwise, put "Part of #<parent>" in the child and add it to
  the parent's task list.
- Represent blockers using GitHub issue dependencies when supported.
  Otherwise, put "Blocked by: #<number>" in the dependent ticket.
- Close resolved tickets with a comment explaining the result.

## Pull requests as a triage surface

PRs as a request surface: no.

## Wayfinding

A map is an issue labelled wayfinder:map.
Child tickets use wayfinder:research, wayfinder:prototype,
wayfinder:grilling, or wayfinder:task.

Choose an open child with no open blockers or assignee, in map order.
Claim it by assigning the driving developer. Resolve it by recording
the answer, closing the ticket, and linking the result from the map.

## Branching Conventions
- Feature PRs target `develop`.
- After changes have been tested on `develop`, promote them to `main` with a PR
  from `develop` to `main`.
- Do not push commits directly to `main`.
