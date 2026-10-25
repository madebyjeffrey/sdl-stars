"""Build navigation from authored trace rows; never infer test success or coverage."""
from collections import defaultdict
from hashlib import sha256
from html import escape
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

ANCHOR = re.compile(r'<a\s+id=["\']([ms]\d{2}-(?:[a-z]+-)?(us|ac|ir|qr)\d+)["\'][^>]*>\s*</a>')
STORY = re.compile(r'(?<![\w-])(?:(M\d{2}|S\d{2})-)?US(\d+)\b')
FENCE = re.compile(r"^(`{3,}|~{3,})[^\n]*\n.*?^\1[^\n]*$", re.M | re.S)
LINK = re.compile(r"\[[^\[\]]*\]\(([^\s)]+)\)")
CODE = re.compile(r"`+([^`\n]+)`+")


def without_fences(text):
    return FENCE.sub(lambda m: "\n" * m[0].count("\n"), text)


def plain(text):
    text = re.sub(r"\[([^]]+)\]\([^)]+\)", r"\1", text)
    return re.sub(r"\s+", " ", re.sub(r"<[^>]*>|[*`#]", "", text)).strip()


def links(text):
    spans = [(m.start(), m.end()) for m in CODE.finditer(text)]
    return [m[1] for m in LINK.finditer(text) if not any(a <= m.start() < b for a, b in spans)]


def first_paragraph(text):
    return re.split(r"\n\s*\n|\n#{1,6} ", text.strip(), maxsplit=1)[0].strip()


def mask_noncode(text):
    """Preserve offsets while hiding strings and nested Dafny comments."""
    result = list(text)
    i = 0
    while i < len(text):
        start = i
        if text.startswith("//", i):
            i = text.find("\n", i)
            if i < 0:
                i = len(text)
        elif text.startswith("/*", i):
            depth, i = 1, i + 2
            while i < len(text) and depth:
                if text.startswith("/*", i):
                    depth, i = depth + 1, i + 2
                elif text.startswith("*/", i):
                    depth, i = depth - 1, i + 2
                else:
                    i += 1
        elif text[i] == '"':
            verbatim = i > 0 and text[i - 1] == "@"
            i += 1
            while i < len(text):
                if not verbatim and text[i] == "\\":
                    i += 2
                elif verbatim and text.startswith('""', i):
                    i += 2
                elif text[i] == '"':
                    i += 1
                    break
                else:
                    i += 1
        else:
            i += 1
            continue
        for j in range(start, min(i, len(text))):
            if text[j] != "\n":
                result[j] = " "
    return "".join(result)


def declarations(text):
    """Lexical declaration navigation, not a Dafny parser or proof result."""
    masked = mask_noncode(text)
    modules = list(re.finditer(r"\bmodule\s+(\w+)", masked))
    pattern = re.compile(r"^\s*(?:(?:ghost|static|opaque|twostate)\s+)*(function|predicate|lemma|method|datatype|type|newtype|const)(?:\s+method)?(?:\s+\{:[^}]*\})*\s+(\w+)", re.M)
    result = []
    used = set()
    for match in pattern.finditer(masked):
        module = next((m[1] for m in reversed(modules) if m.start() < match.start(2)), "")
        qualified = (module + "." if module else "") + match[2]
        anchor = "def-" + qualified.lower().replace(".", "-")
        if anchor in used:
            raise ValueError(f"Ambiguous declaration anchor: {qualified}")
        used.add(anchor)
        # Whitespace in the regex may include preceding blank lines.
        name_start = match.start(2)
        result.append({"name": match[2], "qualified": qualified, "kind": match[1],
                       "anchor": anchor, "line": masked.count("\n", 0, name_start) + 1})
    return result


class Catalog:
    def __init__(self, root, paths, test_paths=None):
        self.root = root
        self.documents = {}
        self.requirements = {}
        self.models = {p: declarations(p.read_text(encoding="utf-8")) for p in paths if p.suffix == ".dfy"}
        self.test_names = defaultdict(list)
        # The builder supplies tracked tests. Do not discover private/ignored
        # workspace paths from a generic filename mentioned in a public spec.
        for path in paths if test_paths is None else test_paths:
            if path.is_relative_to(root / "tests") and path.is_file() and not any(part.startswith(".") for part in path.relative_to(root).parts):
                self.test_names[path.name].append(path)
        for path in sorted(paths):
            if path.suffix == ".md" and path.parent == root / "spec" and re.match(r"[MS]\d{2}-", path.name):
                self.read_document(path)
        self.stories = {k: v for k, v in self.requirements.items() if v["kind"] == "us"}
        self.criteria = {k: v for k, v in self.requirements.items() if v["kind"] == "ac"}
        for identity, criterion in self.criteria.items():
            if not criterion["stories"] or any(s not in self.stories for s in criterion["stories"]):
                raise ValueError(f"{identity}: missing or unknown owning story")
            for story in criterion["stories"]:
                self.stories[story]["criteria"].append(identity)
            self.connect(criterion)

    def read_document(self, path):
        text = without_fences(path.read_text(encoding="utf-8"))
        # Presentation diagrams are not additional formal/evidence mappings.
        context = text.split("\n## Workflow and state diagrams", 1)[0].split("\n## Data-model and relationship diagrams", 1)[0]
        direct_models, tests = self.references(context, path)
        self.documents[path] = {"models": direct_models, "closure": self.closure(direct_models), "tests": tests}
        matches = list(ANCHOR.finditer(text))
        for i, match in enumerate(matches):
            identity, kind = match.groups()
            if identity in self.requirements or identity[:3] != path.name[:3].lower():
                raise ValueError(f"Duplicate or incorrectly owned requirement: {identity}")
            line_start = text.rfind("\n", 0, match.start()) + 1
            line_end = text.find("\n", match.end())
            if line_end < 0:
                line_end = len(text)
            row = text[line_start:line_end]
            end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
            if row.startswith("|"):
                cells = [re.sub(r'<a\s+(?:id|name)=[^>]*>\s*</a>', "", c).strip().replace(r"\|", "|")
                         for c in re.split(r"(?<!\\)\|", row.strip())[1:-1]]
                if kind == "ac" and len(cells) not in {3, 4}:
                    raise ValueError(f"{identity}: unsupported acceptance row; retain explicit criterion/rule/evidence columns")
                block = "\n".join(cells)
                label = cells[0]
            else:
                block = first_paragraph(text[match.end():end])
                cells, label = [], block
            owner_stories = sorted({(owner.lower() if owner else identity[:3]) + "-us" + number
                                    for owner, number in STORY.findall(label)}) if kind == "ac" else []
            title = re.search(r"\*\*([^*]+)\*\*", block)
            self.requirements[identity] = {"id": identity, "kind": kind, "source": path,
                "block": block, "cells": cells, "title": plain(title[1] if title else label),
                "stories": owner_stories, "criteria": []}

    def closure(self, initial):
        result = set()
        pending = list(initial)
        while pending:
            path = pending.pop()
            if path in result:
                continue
            if path not in self.models:
                raise ValueError(f"Trace references an unpublished model: {path}")
            result.add(path)
            pending += [(path.parent / name).resolve() for name in re.findall(
                r'^include\s+"([^"]+)"', path.read_text(encoding="utf-8"), re.M)]
        return result

    def references(self, text, source):
        models, tests = set(), {}
        for target in links(text):
            parts = urlsplit(target)
            if parts.scheme or parts.netloc or not parts.path:
                continue
            path = (source.parent / unquote(parts.path)).resolve()
            if path in self.models:
                models.add(path)
            if path.is_relative_to(self.root / "tests") and path.exists():
                tests[path] = "linked resource"
        for match in CODE.finditer(text):
            token = match[1]
            if not re.fullmatch(r"[\w./-]+\.[\w]+", token):
                continue
            candidates = self.test_names.get(Path(token).name, [])
            explicit = next((p for p in ((self.root / token).resolve(), (source.parent / token).resolve())
                             if p.is_relative_to(self.root / "tests") and p.is_file()), None)
            if explicit is not None:
                tests.setdefault(explicit, "named test file")
            elif "/" not in token and len(candidates) == 1:
                tests.setdefault(candidates[0], "uniquely named test file")
        return models, tests

    def connect(self, criterion):
        models, tests = self.references(criterion["block"], criterion["source"])
        edges = [{"path": p, "kind": "criterion file link", "symbol": None} for p in sorted(models)]
        ambiguous = []
        context = self.documents[criterion["source"]]["closure"]
        for token in sorted(set(CODE.findall(criterion["block"]))):
            token = token.removesuffix("()")
            if not re.fullmatch(r"\w+(?:\.\w+)*", token):
                continue
            candidates = [(p, symbol) for p in sorted(context) for symbol in self.models[p]
                          if token == (symbol["qualified"] if "." in token else symbol["name"])]
            if len(candidates) == 1:
                path, symbol = candidates[0]
                edges.append({"path": path, "kind": "exact declaration name", "symbol": symbol})
            elif len(candidates) > 1:
                ambiguous.append(token)
        criterion.update(models=edges, tests=tests, ambiguous=ambiguous)

    def counts(self):
        return {"requirements": len(self.requirements), "stories": len(self.stories), "criteria": len(self.criteria),
                "models": len(self.models), "declarations": sum(map(len, self.models.values())),
                "story_edges": sum(len(c["stories"]) for c in self.criteria.values()),
                "criteria_with_model_navigation": sum(bool(c["models"]) for c in self.criteria.values()),
                "criteria_with_test_navigation": sum(bool(c["tests"]) for c in self.criteria.values())}

    def export(self, revision):
        relative = lambda p: p.relative_to(self.root).as_posix()
        return {"revision": revision, "scope": "Navigation from authored references and exact declaration names; not proof/test coverage or execution results.",
                "counts": self.counts(),
                "stories": [{"id": k, "source": relative(v["source"]), "title": v["title"], "source_text": v["block"], "criteria": v["criteria"]} for k, v in self.stories.items()],
                "models": [{"path": relative(p), "declarations": symbols} for p, symbols in self.models.items()],
                "criteria": [{"id": k, "source": relative(v["source"]), "stories": v["stories"],
                              "source_text": v["block"], "ambiguous_names": v["ambiguous"],
                              "models": [dict(edge, path=relative(edge["path"])) for edge in v["models"]],
                              "tests": [{"path": relative(p), "kind": kind} for p, kind in sorted(v["tests"].items())]}
                             for k, v in self.criteria.items()],
                "documents": [{"source": relative(p), "linked_models": sorted(map(relative, d["models"])),
                               "model_include_closure": sorted(map(relative, d["closure"])),
                               "test_context": sorted(map(relative, d["tests"]))} for p, d in self.documents.items()]}


def ref_anchor(prefix, path):
    return prefix + "-" + sha256(Path(path).as_posix().encode()).hexdigest()[:12]


def source_block(text):
    return re.sub(r'<a\s+(?:id|name)=[^>]*>\s*</a>', "", text)


def module_page(catalog, identity, title, link, rewrite):
    stories = [v for k, v in catalog.stories.items() if k.startswith(identity.lower() + "-")]
    criteria = [v for k, v in catalog.criteria.items() if k.startswith(identity.lower() + "-")]
    body = f"# {identity} · {title} · Traceability\n\n"
    body += "[All modules](../traceability.md) · [Model backlinks](../trace-models.md) · [Test/evidence backlinks](../trace-evidence.md)\n\n"
    body += "Generated from the current specs. Evidence text below is quoted from its owning criterion. Exact declaration matches provide navigation; they do not prove implementation refinement. Source documents retain behavior status, inspected revisions, dated runs and coverage limits.\n\n"
    body += '<div class="trace-controls" data-trace-controls hidden>\n<div class="trace-field"><label for="trace-story">Story</label><select id="trace-story" data-trace-story><option value="">All stories</option>'
    body += "".join(f'<option value="{s["id"]}">{escape(s["id"].upper())}</option>' for s in stories)
    body += '</select></div>\n<div class="trace-field"><label for="trace-query">Search this trace</label><input id="trace-query" type="search" data-trace-query placeholder="ID, rule, model, or evidence"></div>\n<button type="button" data-trace-reset>Reset</button>\n<p role="status" aria-live="polite" data-trace-count></p>\n</div>\n\n'
    body += '<div data-trace-view markdown="1">\n\n## Stories\n\n'
    for story in stories:
        body += f'<section class="trace-story" id="trace-{story["id"]}" data-story="{story["id"]}" markdown="1">\n\n'
        body += f'### {story["title"]}\n\n'
        content = re.sub(r"^\*\*[^*]+\*\*\s*", "", story["block"])
        body += rewrite(content, story["source"]) + "\n\n"
        body += f'[Source story]({link(story["source"], story["id"])}) · Criteria: '
        body += ", ".join(f'[{c.upper()}](#trace-{c})' for c in story["criteria"]) or "No criterion reference recorded"
        body += "\n\n</section>\n\n"
    body += "## Acceptance criteria\n\n"
    for c in criteria:
        body += f'<article class="trace-criterion" id="trace-{c["id"]}" data-stories="{" ".join(c["stories"])}" markdown="1">\n\n'
        body += f'### {c["id"].upper()}\n\n[Source criterion]({link(c["source"], c["id"])}) · Stories: '
        body += ", ".join(f'[{s.upper()}](#trace-{s})' for s in c["stories"]) + "\n\n"
        if c["cells"]:
            body += "**Source label / status:** " + rewrite(c["cells"][0], c["source"]) + "\n\n"
            labels = ["Acceptance statement", "Rule / source evidence"] if len(c["cells"]) == 4 else ["Acceptance statement", "Rule, evidence and limits (combined in source)"]
            if len(c["cells"]) == 4:
                labels.append("Test/proof evidence and limits")
            for label, content in zip(labels, c["cells"][1:]):
                body += f"**{label}:** " + rewrite(content, c["source"]) + "\n\n"
        else:
            body += rewrite(c["block"], c["source"]) + "\n\n"
        body += "**Model navigation:** "
        if c["models"]:
            body += "; ".join(f'[{edge["symbol"]["qualified"] if edge["symbol"] else edge["path"].name}]({link(edge["path"], edge["symbol"]["anchor"] if edge["symbol"] else "")}) ({edge["kind"]})' for edge in c["models"])
        else:
            body += "No file link or unambiguous declaration name resolved in this criterion."
        if c["ambiguous"]:
            body += " Multiple declaration matches left unresolved: " + ", ".join(f"`{n}`" for n in c["ambiguous"]) + "."
        body += "\n\n**Test/resource navigation:** "
        body += "; ".join(f'[{p.relative_to(catalog.root)}]({link(p)}) ({kind})' for p, kind in sorted(c["tests"].items())) or "No file-level reference resolved here; the authored evidence and limits above still apply."
        body += f'\n\n[Source-document model/test context](#context-{c["source"].stem.lower()}) (not a criterion-level mapping).\n\n</article>\n\n'
    body += "</div>\n\n## Source-document context\n\nThese are document links and include dependencies, not additional criterion associations or passing test results.\n\n"
    for path, context in catalog.documents.items():
        if path.name[:3] != identity:
            continue
        body += f'<a id="context-{path.stem.lower()}"></a>\n\n### {path.stem}\n\n[Original status, evidence and dated checks]({link(path)})\n\n'
        for label, paths in (("Linked models", context["models"]), ("Test/evidence resources", context["tests"])):
            body += f"**{label}:** " + (", ".join(f'[{p.name}]({link(p)})' for p in sorted(paths)) or "No file links recorded.") + "\n\n"
    return body


def overview(catalog, groups):
    counts = catalog.counts()
    body = f'\n**{counts["stories"]} stories · {counts["criteria"]} acceptance criteria · {counts["models"]} formal files.** Counts describe navigation structure, not coverage or current test results.\n\n'
    for group_title, group in groups:
        body += f"## {group_title}\n\n| Module | Stories | Criteria | Trace view |\n| --- | ---: | ---: | --- |\n"
        for identity, title, _ in group:
            stories = sum(k.startswith(identity.lower() + "-") for k in catalog.stories)
            criteria = sum(k.startswith(identity.lower() + "-") for k in catalog.criteria)
            body += f"| {identity} · {title} | {stories} | {criteria} | [Follow traces](trace/{identity}.md) |\n"
        body += "\n"
    return body


def reverse_page(catalog, kind, link):
    model = kind == "models"
    title = "Dafny model backlinks" if model else "Test and evidence-resource backlinks"
    body = f"# {title}\n\n[Traceability overview](traceability.md)\n\n"
    body += "Criterion references below distinguish authored file links from exact declaration/filename matches. Document context is listed separately and does not assign its models or tests to every criterion. These are navigation links, not proof/test execution results.\n\n"
    paths = sorted(catalog.models) if model else sorted({p for d in catalog.documents.values() for p in d["tests"]} | {p for c in catalog.criteria.values() for p in c["tests"]})
    for path in paths:
        body += f'<a id="{ref_anchor(kind, path.relative_to(catalog.root))}"></a>\n\n## {path.relative_to(catalog.root)}\n\n[Open resource]({link(path)})\n\n'
        body += '<details markdown="1"><summary>Criterion references and document context</summary>\n\n'
        matching = []
        for c in catalog.criteria.values():
            edges = [edge for edge in c["models"] if edge["path"] == path] if model else ([c["tests"][path]] if path in c["tests"] else [])
            if edges:
                kinds = sorted({edge["kind"] for edge in edges}) if model else edges
                matching.append(f'- [{c["id"].upper()}](trace/{c["id"][:3].upper()}.md#trace-{c["id"]}) · {", ".join(kinds)}\n')
        body += "**Criterion-level navigation:**\n\n" + ("".join(matching) or "No criterion-level reference resolved.\n")
        body += "\n**Source-document context:**\n\n"
        context_rows = []
        for source, d in catalog.documents.items():
            if path in (d["closure"] if model else d["tests"]):
                scope = "linked in document" if not model or path in d["models"] else "via model include"
                context_rows.append(f'- [{source.name}]({link(source)}) · {scope}\n')
        body += "".join(context_rows) or "No source-document context recorded.\n"
        body += "\n</details>\n\n"
    return body
