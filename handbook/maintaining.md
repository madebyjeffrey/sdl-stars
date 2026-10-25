# Maintaining the handbook

The repository's `spec/` Markdown and Dafny files are the source of truth.
`handbook/` supplies introductions, navigation guidance, and overview diagrams.
Generated copies and HTML live under ignored `dist/handbook/`.

## Install and preview

Use Python 3.12 or newer, from the repository root:

```sh
python3 -m venv dist/docs-venv
dist/docs-venv/bin/python -m pip install -r requirements-docs.txt
make docs-serve DOCS_PYTHON=dist/docs-venv/bin/python
```

On Windows, use `dist/docs-venv/Scripts/python.exe` for both commands and
`DOCS_PYTHON`. The preview prints its local URL. It watches the original specs,
formal files, glossary, decisions, handbook pages, and configuration. Editing
those sources regenerates the staged pages; Zensical reloads the browser.

For an installed Zensical environment, `make docs DOCS_PYTHON=python3` builds
and validates the complete static site at `dist/handbook/site/`.
`make docs-check` rechecks existing HTML; `make test-docs` checks publication
and link-resolution boundaries. All three accept `DOCS_PYTHON`.

## Update content once

| Change | Edit |
| --- | --- |
| Behavior, stories, criteria, evidence, or gaps | The owning `spec/*.md` document, following its repository instructions |
| Canonical definitions and proofs | The owning `spec/*.dfy` or `spec/dafny/*.dfy` file; run formal verification separately |
| A module's title, entry document, or grouping | The first-level module map in `spec/README.md` |
| A technical companion | Add its module-prefixed Markdown file under `spec/`; navigation discovers it automatically |
| Orientation and diagrams | The appropriate `handbook/*.md` page, with links to its authoritative specification |
| Theme or site URL | `zensical.toml` and `handbook/assets/` |

The builder discovers module entries from the first-level map, companions by
module prefix, requirements from explicit ID anchors, and formal references
from all Dafny files in `spec/`. Existing source anchors are retained. The
rendered-link check catches renderer differences in heading anchors.

Each build assembles the current source collection and removes stale generated
files, so renamed or removed documents do not linger. Preview updates replace
changed files individually without emptying the live source directory.
Do not edit generated files.
Preserve stable requirement IDs and existing links when updating the specs.

## Links and publication boundaries

Specification links to included pages become local site links. Links to Dafny
files open readable reference pages with unchanged raw downloads; `models.zip`
retains include paths for the whole collection. Other repository links point
to the build commit on GitHub. Every authored page includes source/edit links.

The published inputs are module specifications, the collection README, formal
files, the glossary, `docs/adr/*.md`, and the handbook. Agent instructions,
private material, build logs, and arbitrary repository files are not copied
into the site. Adding another publication source requires changing the builder's
explicit selection.

The build provenance page records the repository revision, source fingerprint,
counts, and whether the checkout contains unpublished changes. Local new files
may not yet exist at their GitHub source URLs. Published specifications retain
their own behavior-inspection revisions and historical verification dates.

## GitHub Pages

The `Specification handbook` workflow builds and validates on pull requests,
pushes to `develop` and `main`, and manual dispatch. It uploads a downloadable
preview artifact for every successful build. Only a push or manual run on
`main` deploys the Pages artifact; pull requests cannot deploy.

Commit the specification collection along with the site before publishing:
local previews include untracked specs, while CI only receives committed files.
Follow the repository's promotion path through `develop` to `main`.

To enable publishing, select **GitHub Actions** under the repository's
**Settings → Pages → Build and deployment → Source**. The deployment uses
the `github-pages` environment. Configure its permitted deployment branch as
`main` if repository policy requires it.

The default project URL is `https://madebyjeffrey.github.io/sdl-stars/`.
Set `HANDBOOK_SITE_URL` when building for a different host or project path.
The workflow uses the URL reported by GitHub Pages for deployment builds.

To repeat the browser checks locally, install the optional requirements and
Chromium, then run the script:

```sh
dist/docs-venv/bin/python -m pip install -r requirements-docs-browser.txt
dist/docs-venv/bin/python -m playwright install chromium
dist/docs-venv/bin/python tests/docs/browser.py
```

It serves the built site on a temporary loopback port under its project path.
Screenshots are saved to `dist/handbook/screenshots/`. Mermaid's runtime is
loaded by Zensical from its CDN, so the rendering check needs internet access.

## Verification scope

`make docs` runs the existing specification link/requirement checker, builds
with pinned Zensical, and validates rendered local links, fragments, and assets.
The workflow also renders every Mermaid block in a browser and exercises the
site under its project path. The generated counts do not represent proof or
runtime coverage.

Run `python3 tests/spec/verify.py --time-limit 120` for Dafny verification/audit.
Runtime changes still require the repository's SDL build, regression, unit,
tutorial, and applicable host checks. Keep runtime changes separate from
handbook publication changes.
