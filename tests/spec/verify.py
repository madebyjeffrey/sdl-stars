#!/usr/bin/env python3
"""Check spec links and requirement traces, verify Dafny, then audit trust gaps."""
import argparse
import re
import subprocess
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
SPEC = ROOT / "spec"


def exact_path(path):
    path = Path(path.absolute())
    parts = path.parts
    current = Path(parts[0])
    for part in parts[1:]:
        if part == "..":
            current = current.parent
        elif part != ".":
            if not current.is_dir() or part not in {p.name for p in current.iterdir()}:
                return False
            current = current / part
    return current.exists()


def anchors(path):
    text = re.sub(r"```.*?```", "", path.read_text(), flags=re.S)
    result = set(re.findall(r'<a\s+(?:id|name)=["\']([^"\']+)', text))
    counts = {}
    for title in re.findall(r"^#{1,6}\s+(.+?)\s*#*\s*$", text, flags=re.M):
        title = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", title)
        slug = re.sub(r"[^\w\- ]", "", title.lower()).replace(" ", "-")
        n = counts.get(slug, 0)
        counts[slug] = n + 1
        result.add(slug if n == 0 else f"{slug}-{n}")
    return result


def links():
    errors = []
    count = 0
    for source in sorted(SPEC.rglob("*")):
        if not source.is_file() or source.suffix not in {".md", ".dfy"}:
            continue
        if source.name not in {"README.md", "AGENTS.md"} and not re.match(r"[MS]\d{2}-", source.name):
            errors.append(f"{source}: missing module prefix")
        text = source.read_text()
        if any(line.rstrip() != line for line in text.splitlines()):
            errors.append(f"{source}: trailing whitespace")
        if source.suffix == ".dfy":
            targets = re.findall(r'^include\s+"([^"]+)"', text, flags=re.M)
        else:
            text = re.sub(r"```.*?```", "", text, flags=re.S)
            targets = re.findall(r"\[[^\]]*\]\(([^)]+)\)", text)
        for target in targets:
            target = target.strip().strip("<>")
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc:
                continue
            path = source.parent / unquote(parsed.path) if parsed.path else source
            count += 1
            if not exact_path(path):
                errors.append(f"{source.relative_to(ROOT)}: missing or incorrectly cased {target}")
            elif parsed.fragment and path.suffix == ".md" and unquote(parsed.fragment) not in anchors(path):
                errors.append(f"{source.relative_to(ROOT)}: missing anchor {target}")
    for error in errors:
        print(error)
    print(f"{count} local links/includes checked; {len(errors)} errors", flush=True)
    return not errors


REQUIREMENT = re.compile(r'<a\s+id=["\']([ms]\d{2}-(?:[a-z]+-)?(us|ac|ir|qr)\d+)["\'][^>]*>')
STORY_REFERENCE = re.compile(r'(?:(M\d{2}|S\d{2})-)?US(\d+)')


def requirement_errors(documents):
    """Check explicit IDs and criterion-to-story edges, not semantic coverage.

    documents maps names to Markdown. A criterion block ends at the next
    requirement anchor. Existing tables use both qualified and local US IDs.
    """
    definitions = {}
    edges = {}
    counts = dict.fromkeys(("us", "ac", "ir", "qr"), 0)
    errors = []
    for name, text in documents.items():
        text = re.sub(r"```.*?```", "", text, flags=re.S)
        matches = list(REQUIREMENT.finditer(text))
        for i, match in enumerate(matches):
            identity, kind = match.groups()
            counts[kind] += 1
            if identity in definitions:
                errors.append(f"{name}: duplicate requirement {identity} (also {definitions[identity]})")
            definitions[identity] = name
            if name[:3].lower() != identity[:3]:
                errors.append(f"{name}: requirement {identity} belongs to another module")
            if kind == "ac":
                end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
                block = text[match.end():end]
                edges[identity] = {
                    (owner.lower() if owner else identity[:3]) + "-us" + number
                    for owner, number in STORY_REFERENCE.findall(block)
                }
                if not edges[identity]:
                    errors.append(f"{name}: {identity} has no story reference")
    referenced = set().union(*edges.values()) if edges else set()
    for criterion, stories in edges.items():
        for story in sorted(stories):
            if story not in definitions:
                errors.append(f"{definitions[criterion]}: {criterion} references undefined story {story}")
    for identity, name in definitions.items():
        if re.search(r"-us\d+$", identity) and identity not in referenced:
            errors.append(f"{name}: {identity} has no acceptance criterion")
    return counts, errors


def requirements():
    documents = {p.name: p.read_text() for p in sorted(SPEC.glob("*.md"))}
    counts, errors = requirement_errors(documents)
    for error in errors:
        print(error)
    print(f"{sum(counts.values())} requirement IDs checked ({counts}); {len(errors)} errors", flush=True)
    return not errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--links-only", action="store_true")
    parser.add_argument("--cores", type=int, default=4)
    parser.add_argument("--time-limit", type=int, default=60)
    args = parser.parse_args()
    links_ok = links()
    requirements_ok = requirements()
    if not links_ok or not requirements_ok:
        return 1
    if args.links_only:
        return 0
    sources = [str(p.relative_to(ROOT)) for p in sorted(SPEC.rglob("*.dfy"))]
    subprocess.run(["dafny", "--version"], cwd=ROOT, check=True)
    subprocess.run(["dafny", "verify", *sources, "--verify-included-files",
                    "--cores", str(args.cores), "--verification-time-limit", str(args.time_limit)], cwd=ROOT, check=True)
    subprocess.run(["dafny", "audit", *sources], cwd=ROOT, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
