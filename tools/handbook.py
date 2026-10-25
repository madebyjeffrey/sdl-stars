#!/usr/bin/env python3
"""Assemble the specification handbook without modifying its source documents."""
import argparse
import hashlib
import importlib.util
from html import escape
from html.parser import HTMLParser
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import tomllib
from urllib.parse import quote, unquote, urlsplit
import zipfile

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "dist/handbook"
SOURCE = WORK / "source"
SITE = WORK / "site"
REPO = "https://github.com/madebyjeffrey/sdl-stars"
CONFIG = WORK / "zensical.toml"
REQUIREMENT = re.compile(r'<a\s+id=["\']([ms]\d{2}-(?:[a-z]+-)?(us|ac|ir|qr)\d+)["\']')
# Keep code examples untouched, including examples containing Markdown links.
CODE = re.compile(r"^(`{3,}|~{3,})[^\n]*\n.*?^\1[^\n]*$", re.M | re.S)
INLINE_CODE = re.compile(r"`+[^`\n]*`+")
LINK = re.compile(r"(!?\[[^\[\]]*\]\()([^\s)]+)(\))")
TRACE_SPEC = importlib.util.spec_from_file_location("handbook_traceability", Path(__file__).with_name("traceability.py"))
traceability = importlib.util.module_from_spec(TRACE_SPEC)
TRACE_SPEC.loader.exec_module(traceability)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def toml_value(value):
    if isinstance(value, dict):
        return "{ " + ", ".join(f"{json.dumps(k)} = {toml_value(v)}" for k, v in value.items()) + " }"
    if isinstance(value, list):
        return "[" + ", ".join(map(toml_value, value)) + "]"
    return json.dumps(value, ensure_ascii=False)


def module_map():
    """Use the first-level map's grouping, names and entry paths as navigation."""
    text = (ROOT / "spec/README.md").read_text(encoding="utf-8")
    text = text.split("## Proposed first-level module map\n", 1)[1].split("## Shared data contracts", 1)[0]
    groups = []
    for line in text.splitlines():
        if line.startswith("### "):
            groups.append((line[4:], []))
        match = re.match(r"\| ([MS]\d{2}) \| \[([^]]+)\]\(([^)]+)\)", line)
        if match:
            identity, title, filename = match.groups()
            groups[-1][1].append((identity, title, Path("spec") / filename))
    entries = [entry for _, group in groups for entry in group]
    if not entries or len({entry[0] for entry in entries}) != len(entries):
        raise ValueError("The specification module map is empty or has duplicate module IDs")
    return groups


def mermaid_blocks(text):
    """Return diagram bodies, ignoring optional Mermaid YAML frontmatter."""
    blocks = re.findall(r"^```mermaid\s*\n(.*?)^```\s*$", text, re.M | re.S)
    return [re.sub(r"\A---\n.*?\n---\n", "", block, count=1, flags=re.S) for block in blocks]


def diagram_entries(paths, heading, kinds):
    """Discover named diagram sections in canonical specs for navigation only."""
    entries = []
    for path in sorted(paths):
        if path.suffix != ".md" or path.parent != ROOT / "spec":
            continue
        text = path.read_text(encoding="utf-8")
        section = text.partition(f"\n## {heading}\n")[2]
        section = re.split(r"^## ", section, maxsplit=1, flags=re.M)[0]
        for part in re.split(r'(?=^<a id="[^"\n]+"></a>\s*\n### )', section, flags=re.M):
            blocks = mermaid_blocks(part)
            if not any(block.split(maxsplit=1)[0] in kinds for block in blocks if block.strip()):
                continue
            match = re.match(r'<a id="([^"\n]+)"></a>\s*\n### ([^\n]+)', part)
            if not match:
                raise ValueError(f"{path}: diagram needs a stable anchor and heading")
            entries.append((path, match[1], match[2]))
    return entries


def sequence_entries(paths):
    return diagram_entries(paths, "Cross-module sequences", {"sequenceDiagram"})


def data_entries(paths):
    return diagram_entries(paths, "Data-model and relationship diagrams", {"classDiagram", "erDiagram"})


def sequence_index(paths):
    entries = sequence_entries(paths)
    rows = ["| Workflow | Owning specification |\n| --- | --- |\n"]
    for path, anchor, title in entries:
        relative = path.relative_to(ROOT).as_posix()
        rows.append(f"| [{title}](../{relative}#{anchor}) | [{path.name[:3]}](../{relative}) |\n")
    return f"**{len(entries)} cross-module sequences**, discovered from the current specs.\n\n" + "".join(rows)


def data_index(paths):
    entries = data_entries(paths)
    rows = ["| Data model / relationship | Owning specification |\n| --- | --- |\n"]
    for path, anchor, title in entries:
        relative = path.relative_to(ROOT).as_posix()
        rows.append(f"| [{title}](../{relative}#{anchor}) | [{path.stem}](../{relative}) |\n")
    return f"**{len(entries)} data-model and relationship diagrams**, discovered from the current specs.\n\n" + "".join(rows)


def diagram_index(groups):
    """Discover module diagrams in their canonical Markdown, without copying them."""
    rows = []
    count = 0
    for group_title, group in groups:
        rows.append(f"## {group_title}\n\n| Module | Workflow | State and recovery | Sequences | Data models |\n| --- | --- | --- | --- | --- |\n")
        for identity, title, entry in group:
            text = (ROOT / entry).read_text(encoding="utf-8")
            blocks = mermaid_blocks(text)
            flow = sum(block.startswith("flowchart") for block in blocks)
            state = sum(block.startswith("stateDiagram-v2") for block in blocks)
            sequence = sum(block.startswith("sequenceDiagram") for block in blocks)
            data = sum(block.startswith(("classDiagram", "erDiagram")) for block in blocks)
            count += len(blocks)
            path = "../" + entry.as_posix()
            workflow = f"[View ({flow})]({path}#workflow)" if flow else "Not yet diagrammed"
            states = f"[View ({state})]({path}#state-and-recovery)" if state else "Not yet diagrammed"
            sequences = f"[View ({sequence})]({path}#cross-module-sequences)" if sequence else "—"
            models = f"[View ({data})]({path}#data-model-and-relationship-diagrams)" if data else "—"
            rows.append(f"| [{identity} · {title}]({path}) | {workflow} | {states} | {sequences} | {models} |\n")
        rows.append("\n")
    return f"**{count} module diagrams**, discovered from the current source files.\n\n" + "".join(rows)


def sources():
    # Explicit publication boundary: never sweep the repository or agent guidance.
    paths = [ROOT / "spec/README.md", ROOT / "GLOSSARY.md"]
    paths += sorted((ROOT / "spec").glob("[MS][0-9][0-9]-*.md"))
    paths += sorted((ROOT / "spec").rglob("*.dfy"))
    paths += sorted((ROOT / "docs/adr").glob("*.md"))
    paths += sorted((ROOT / "handbook").rglob("*"))
    return [p for p in paths if p.is_file() and not p.name.startswith("AGENTS")
            and p.name != "CLAUDE.md" and not any(part.startswith(".") for part in p.relative_to(ROOT).parts)]


def destination(path):
    relative = path.relative_to(ROOT)
    if relative.parts[0] == "handbook":
        return Path(*relative.parts[1:])
    if path.suffix == ".dfy":
        return Path("models") / path.relative_to(ROOT / "spec").with_suffix(".md")
    return relative


def resolve_link(target, source, destinations, revision, output=None):
    parts = urlsplit(target)
    if parts.scheme or parts.netloc or (not parts.path and output is None):
        return target
    if parts.path.startswith("/"):
        raise ValueError(f"{source}: root-relative link bypasses the project URL: {target}")
    path = (source.parent / unquote(parts.path)).resolve() if parts.path else source
    suffix = ("?" + parts.query if parts.query else "") + ("#" + parts.fragment if parts.fragment else "")
    if path in destinations:
        local = os.path.relpath(destinations[path], (output or destinations[source]).parent)
        return quote(Path(local).as_posix(), safe="/.-") + suffix
    if not path.is_relative_to(ROOT) or not path.exists():
        raise ValueError(f"{source.relative_to(ROOT)}: missing or unpublished local target {target}")
    kind = "tree" if path.is_dir() else "blob"
    return f"{REPO}/{kind}/{revision}/{quote(path.relative_to(ROOT).as_posix())}{suffix}"


def rewrite_links(text, source, destinations, revision, output=None):
    def rewrite(block):
        spans = [(m.start(), m.end()) for m in INLINE_CODE.finditer(block)]
        def replace(match):
            if any(start <= match.start() < end for start, end in spans):
                return match[0]
            return match[1] + resolve_link(match[2], source, destinations, revision, output) + match[3]
        block = LINK.sub(replace, block)
        return re.sub(r'^(\[[^]\n]+\]:\s*)(\S+)',
                      lambda m: m[1] + resolve_link(m[2], source, destinations, revision, output), block, flags=re.M)
    pieces = []
    end = 0
    for match in CODE.finditer(text):
        pieces.extend([rewrite(text[end:match.start()]), match[0]])
        end = match.end()
    pieces.append(rewrite(text[end:]))
    return "".join(pieces)


def readable_diagrams(text):
    """Keep diagram text at its natural size inside the theme's scroll container."""
    config = "---\nconfig:\n" + "".join(f"  {kind}:\n    useMaxWidth: false\n" for kind in ("flowchart", "state", "sequence", "class", "er")) + "---\n"
    return re.sub(r"(^```mermaid[^\n]*\n)(?=(?:flowchart|stateDiagram-v2|sequenceDiagram|classDiagram|erDiagram)\b)",
                  lambda match: match[1] + config, text, flags=re.M)


def provenance(source, revision, branch):
    path = quote(source.relative_to(ROOT).as_posix())
    return (f'\n\n<p class="handbook-source">Built from '
            f'<a href="{REPO}/blob/{revision}/{path}">{escape(source.relative_to(ROOT).as_posix())}</a>'
            f' · <a href="{REPO}/edit/{quote(branch, safe="")}/{path}">Edit source</a>'
            f' · Build revision <code>{revision[:12]}</code>. '
            'Behavior inspection and verification dates remain those recorded above.</p>\n')


def prepare():
    revision = git("rev-parse", "HEAD")
    branch = os.environ.get("GITHUB_HEAD_REF") or git("branch", "--show-current") or "develop"
    paths = sources()
    virtual = {ROOT / "handbook" / f"{name}.md": Path(f"{name}.md")
               for name in ("requirements", "models", "build", "trace-models", "trace-evidence")}
    virtual[ROOT / "handbook/traceability.json"] = Path("traceability.json")
    groups = module_map()
    virtual |= {ROOT / "handbook/trace" / f"{identity}.md": Path(f"trace/{identity}.md")
                for _, group in groups for identity, _, _ in group}
    destinations = {path: destination(path) for path in paths} | virtual
    test_paths = [ROOT / name for name in git("ls-files", "-z", "--", "tests").split("\0") if name]
    traces = traceability.Catalog(ROOT, paths, test_paths)
    config = tomllib.loads((ROOT / "zensical.toml").read_text(encoding="utf-8"))
    project = config["project"]
    project["docs_dir"], project["site_dir"] = "source", "site"
    project["site_url"] = os.environ.get("HANDBOOK_SITE_URL", project["site_url"]).rstrip("/") + "/"
    nav = [{"Start here": [{"Welcome": "index.md"}, {"Reading a specification": "reading.md"},
                            {"System overview": "overview.md"}, {"Cross-module sequences": "sequences.md"},
                            {"Data models and files": "data-models.md"}, {"Module diagrams": "diagrams.md"},
                            {"Glossary": "GLOSSARY.md"}]}]
    entries = {entry[2] for _, group in groups for entry in group}
    for group_title, group in groups:
        items = []
        for identity, title, entry in group:
            if ROOT / entry not in destinations:
                raise ValueError(f"Module map entry has no specification: {entry}")
            companions = [p for p in paths if p.suffix == ".md" and p.name.startswith(identity + "-")
                          and p.relative_to(ROOT) not in entries]
            children = [{"Overview": entry.as_posix()}, {"Traceability": f"trace/{identity}.md"}]
            for path in companions:
                heading = re.search(r"^# (.+)$", path.read_text(encoding="utf-8"), re.M)
                children.append({heading[1] if heading else path.stem: destination(path).as_posix()})
            items.append({f"{identity} · {title}": children})
        nav.append({group_title: items})
    nav.append({"Evidence": [{"Traceability views": "traceability.md"}, {"Model backlinks": "trace-models.md"},
                             {"Test and evidence backlinks": "trace-evidence.md"},
                             {"Coverage guide": "evidence.md"}, {"Requirement index": "requirements.md"},
                             {"Dafny reference": "models.md"}, {"Source module map and verification": "spec/README.md"},
                             {"Build provenance": "build.md"}, {"Maintaining the handbook": "maintaining.md"},
                             {"Decisions": [{p.stem: p.relative_to(ROOT).as_posix()} for p in paths if p.parent == ROOT / "docs/adr"]}]})
    project["nav"] = nav
    # Assemble off to the side so a live reader never sees an emptied source tree.
    stage = WORK / "next-source"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    requirements = []
    models = []
    for path in paths:
        target = stage / destinations[path]
        if path.suffix == ".md":
            text = path.read_text(encoding="utf-8")
            if path == ROOT / "handbook/diagrams.md":
                text += "\n" + diagram_index(groups)
            if path == ROOT / "handbook/sequences.md":
                text += "\n" + sequence_index(paths)
            if path == ROOT / "handbook/data-models.md":
                text += "\n" + data_index(paths)
            if path == ROOT / "handbook/traceability.md":
                text += "\n" + traceability.overview(traces, groups)
            for match in REQUIREMENT.finditer(CODE.sub("", text)):
                requirements.append((match[1], match[2], path))
            write(target, readable_diagrams(rewrite_links(text, path, destinations, revision)) + provenance(path, revision, branch))
        elif path.suffix == ".dfy":
            text = path.read_text(encoding="utf-8")
            raw = target.with_suffix(".dfy")
            write(raw, text)
            includes = re.findall(r'^include\s+"([^"]+)"', text, re.M)
            links = [f"[{name}]({resolve_link(name, path, destinations, revision)})" for name in includes]
            body = f"# {path.name}\n\nGenerated from the canonical Dafny source. "
            body += f"[Download this file]({quote(raw.name)}).\n\n"
            backlink = Path(os.path.relpath(Path("trace-models.md"), destinations[path].parent)).as_posix()
            body += f'[Trace references]({backlink}#{traceability.ref_anchor("models", path.relative_to(ROOT))}).\n\n'
            if links:
                body += "Includes: " + ", ".join(links) + ".\n\n"
            body += "Definitions and proofs apply within their stated assumptions. Rendering this page does not verify the model or the C implementation.\n\n"
            body += "## Definitions\n\nDeclaration navigation is generated lexically from the canonical file; source-line links open the exact build revision.\n\n"
            for symbol in traces.models[path]:
                body += f'<a id="{symbol["anchor"]}"></a>\n\n- {symbol["kind"]} **`{symbol["qualified"]}`** · [source line {symbol["line"]}]({REPO}/blob/{revision}/{path.relative_to(ROOT).as_posix()}#L{symbol["line"]})\n\n'
            body += "## Canonical source\n\n"
            fence = "`" * max(3, max((len(m[0]) + 1 for m in re.finditer(r"`+", text)), default=3))
            body += f"{fence}dafny\n{text}\n{fence}\n" + provenance(path, revision, branch)
            write(target, body)
            models.append(path)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
    req_page = "# Requirement index\n\nGenerated from explicit anchors in the current specifications. Counts describe trace structure, not semantic completeness or verification. Use the Trace column or [traceability overview](traceability.md) to follow stories, criteria, models and evidence.\n\n"
    req_page += f"**{len(entries)} modules · {len(requirements)} requirements · {len(models)} formal files.**\n\n"
    kinds = {"us": "Story", "ac": "Acceptance", "ir": "Interaction", "qr": "Quality"}
    for _, group in groups:
        for identity, title, _ in group:
            req_page += f"## {identity} · {title}\n\n| ID | Kind | Source | Trace |\n| --- | --- | --- | --- |\n"
            for anchor, kind, path in requirements:
                if anchor[:3] == identity.lower():
                    url = path.relative_to(ROOT).as_posix() + "#" + anchor
                    trace_link = f"[Follow](trace/{identity}.md#trace-{anchor})" if kind in {"us", "ac"} else "—"
                    req_page += f"| [{anchor.upper()}]({url}) | {kinds[kind]} | {path.stem} | {trace_link} |\n"
            req_page += "\n"
    write(stage / "requirements.md", req_page)
    model_page = "# Dafny reference\n\nReadable views and unchanged raw downloads are generated from the canonical files. [Download all formal files](models.zip), retaining relative include paths. Run verification separately; see the [coverage guide](evidence.md).\n\n"
    for _, group in groups:
        for identity, title, _ in group:
            model_page += f"## {identity} · {title}\n\n"
            for path in models:
                if path.name.startswith(identity + "-"):
                    model_page += f"- [{path.relative_to(ROOT / 'spec')}]({destination(path).as_posix()})\n"
            model_page += "\n"
    write(stage / "models.md", model_page)
    def trace_link(path, output, anchor=""):
        suffix = "#" + anchor if anchor else ""
        if path in destinations:
            return quote(Path(os.path.relpath(destinations[path], output.parent)).as_posix(), safe="/.-") + suffix
        if not path.is_relative_to(ROOT) or not path.exists():
            raise ValueError(f"Missing trace resource: {path}")
        kind = "tree" if path.is_dir() else "blob"
        return f"{REPO}/{kind}/{revision}/{quote(path.relative_to(ROOT).as_posix())}{suffix}"
    for _, group in groups:
        for identity, title, _ in group:
            output = Path(f"trace/{identity}.md")
            body = traceability.module_page(traces, identity, title,
                lambda p, a="": trace_link(p, output, a),
                lambda text, source: rewrite_links(traceability.source_block(text), source, destinations, revision, output))
            write(stage / output, body)
    for kind in ("models", "evidence"):
        output = Path(f"trace-{kind}.md")
        write(stage / output, traceability.reverse_page(traces, kind, lambda p, a="": trace_link(p, output, a)))
    with zipfile.ZipFile(stage / "models.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in models:
            archive.write(path, path.relative_to(ROOT / "spec"))
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(ROOT).as_posix().encode())
        digest.update(path.read_bytes())
    dirty = bool(git("status", "--porcelain", "--untracked-files=all"))
    manifest = {"revision": revision, "working_tree_changes": dirty, "source_sha256": digest.hexdigest(),
                "modules": len(entries), "requirements": len(requirements), "formal_files": len(models),
                "traceability": traces.counts()}
    trace_data = traces.export(revision) | {"working_tree_changes": dirty, "source_sha256": digest.hexdigest()}
    write(stage / "traceability.json", json.dumps(trace_data, ensure_ascii=False, indent=2) + "\n")
    write(stage / "build.json", json.dumps(manifest, indent=2) + "\n")
    build = f"# Build provenance\n\n- Repository revision: [`{revision}`]({REPO}/commit/{revision}).\n"
    build += f"- Source fingerprint: `{digest.hexdigest()}`.\n- Modules: {len(entries)}; requirement IDs: {len(requirements)}; formal files: {len(models)}.\n"
    build += f"- Working tree contains unpublished changes: **{'yes' if dirty else 'no'}**.\n\n"
    build += "Source links point to the build revision. In a local working copy, unpublished edits and new files may differ from those links or not yet exist on GitHub. Each specification retains its own inspected revision and dated evidence.\n\n[Machine-readable provenance](build.json).\n"
    write(stage / "build.md", build)
    prepared = {p.relative_to(stage) for p in stage.rglob("*") if p.is_file()}
    # Publish new targets before updating existing references; skip unchanged files
    # so preview builds only see meaningful edits. Replace each file atomically.
    for relative in sorted(prepared, key=lambda p: ((SOURCE / p).exists(), str(p))):
        target = SOURCE / relative
        content = (stage / relative).read_bytes()
        if not target.exists() or target.read_bytes() != content:
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = target.with_name("." + target.name + ".tmp")
            temporary.write_bytes(content)
            temporary.replace(target)
    for path in SOURCE.rglob("*"):
        if path.is_file() and path.relative_to(SOURCE) not in prepared:
            path.unlink()
    config_text = "\n".join(f"{key} = {toml_value(value)}" for key, value in config.items()) + "\n"
    if not CONFIG.exists() or CONFIG.read_text(encoding="utf-8") != config_text:
        write(CONFIG, config_text)
    shutil.rmtree(stage)
    print(f"Prepared {len(entries)} modules, {len(requirements)} requirements, {len(models)} formal files", flush=True)


class Page(HTMLParser):
    def __init__(self, text):
        super().__init__(convert_charrefs=True)
        self.ids, self.links = set(), []
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        for key in ("href", "src"):
            if attrs.get(key):
                self.links.append(attrs[key])


def check_site(site=SITE, site_url=None):
    """Check actual rendered fragments and project-subpath URLs, not just Markdown."""
    site = site.resolve()
    if site_url is None:
        site_url = tomllib.loads(CONFIG.read_text(encoding="utf-8"))["project"]["site_url"]
    base = urlsplit(site_url)
    pages = {p.resolve(): Page(p.read_text(encoding="utf-8")) for p in site.rglob("*.html")}
    if not pages:
        raise ValueError(f"No rendered pages found in {site}")
    errors, count = [], 0
    for path, page in pages.items():
        for link in page.links:
            parts = urlsplit(link)
            if parts.scheme or parts.netloc:
                if parts.scheme not in ("http", "https") or parts.netloc != base.netloc:
                    continue
                url = unquote(parts.path)
                if not url.startswith(base.path):
                    continue
                target = site / url[len(base.path):]
            elif parts.path.startswith("/"):
                url = unquote(parts.path)
                if not url.startswith(base.path):
                    errors.append(f"{path.relative_to(site)}: URL escapes project path: {link}")
                    continue
                target = site / url[len(base.path):]
            else:
                target = path.parent / unquote(parts.path) if parts.path else path
            if parts.path.endswith("/") and not target.is_dir():
                errors.append(f"{path.relative_to(site)}: not a directory URL: {link}")
                continue
            if target.is_dir():
                target /= "index.html"
            target = target.resolve()
            count += 1
            if not target.is_relative_to(site.resolve()) or not target.is_file():
                errors.append(f"{path.relative_to(site)}: missing {link}")
            elif parts.fragment and target.suffix == ".html" and unquote(parts.fragment) not in pages[target].ids:
                errors.append(f"{path.relative_to(site)}: missing fragment {link}")
    if errors:
        raise ValueError("\n".join(errors))
    print(f"Checked {len(pages)} rendered pages and {count} local links/assets; 0 errors", flush=True)


def fingerprint():
    return [(str(p), p.stat().st_mtime_ns, p.stat().st_size) for p in sources() + [ROOT / "zensical.toml"]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("build", "serve", "check"))
    args = parser.parse_args()
    if args.command == "check":
        check_site()
        return
    subprocess.run([sys.executable, "tests/spec/verify.py", "--links-only"], cwd=ROOT, check=True)
    prepare()
    command = [sys.executable, "-m", "zensical", args.command, "--config-file", str(CONFIG)]
    if args.command == "build":
        subprocess.run(command + ["--clean"], cwd=ROOT, check=True)
        check_site()
    else:
        previous = fingerprint()
        process = subprocess.Popen(command, cwd=ROOT)
        try:
            while process.poll() is None:
                time.sleep(.5)
                current = fingerprint()
                if current != previous:
                    try:
                        subprocess.run([sys.executable, "tests/spec/verify.py", "--links-only"], cwd=ROOT, check=True)
                        prepare()
                        previous = current
                    except (OSError, ValueError, subprocess.CalledProcessError) as error:
                        print(f"Handbook update failed: {error}", file=sys.stderr)
                        previous = current
        finally:
            process.terminate()
            process.wait()


if __name__ == "__main__":
    try:
        main()
    except (ValueError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
