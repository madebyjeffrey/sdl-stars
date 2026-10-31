"""Audit actual compilation, owned declarations and explicit C-reference exceptions."""
from pathlib import Path
import json
import re
import sys

root = Path(sys.argv[1]).resolve()
manifest = Path(sys.argv[2]).resolve()
rows = json.loads(manifest.read_text(encoding="utf-8"))
generated = (manifest.parent / "generated", manifest.parent / "compat/win32/generated")
sdk_shared = set(re.findall(r"tests/compat/[A-Za-z0-9_]+\.c",
                           (root / "cmake/win32-reference-tests.cmake").read_text(encoding="utf-8")))
count = 0
production = set()
references = set()
for row in rows:
    path = Path(row["file"]).resolve()
    try:
        name = path.relative_to(root).as_posix()
    except ValueError:
        name = ""
    command = row.get("command", " ".join(row.get("arguments", [])))
    cpp = "-x c++" in command or "-x objective-c++" in command or path.suffix in (".cpp", ".mm")
    if name.startswith(("src/", "compat/win32/src/")) or any(path.is_relative_to(directory) for directory in generated):
        assert cpp, f"Owned production source compiled as C/Objective-C: {path}"
        standards = re.findall(r"-std=(\S+)", command)
        assert standards and standards[-1] in ("c++20", "c++2a"), f"Owned production source lacks ISO C++20: {path}"
        assert "-Wpedantic" in command and "-Werror" in command, f"Owned production source lacks strict diagnostics: {path}"
        assert not re.search(r"-fpermissive|-Wno-(?:missing-field-initializers|missing-braces|pedantic)|-Wno-error(?:=pedantic)?(?:\s|$)", command), f"Owned production source disables strict diagnostics: {path}"
        count += 1
        production.add(path)
    elif name.startswith("tests/"):
        if name not in sdk_shared:
            assert "-Wno-missing-field-initializers" not in command and "-Wno-missing-braces" not in command, f"Ordinary C++ consumer retains a migration suppression: {path}"
        if name in ("tests/compat/record_layout.c", "tests/compat/popup_payload_reference.c"):
            assert not cpp, "Independent frozen-declaration reference must remain C"
            references.add(name)
        else:
            assert cpp, f"Ordinary test source compiled as C/Objective-C: {path}"
    elif "/sdk-reference/" in path.as_posix():
        assert not cpp, f"SDK reference source must remain C: {path}"
        references.add(path.name)

for directory in ("src", "compat/win32/include", "compat/win32/src"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in (".h", ".c", ".cpp", ".m"):
            continue
        text = path.read_text(encoding="utf-8")
        assert not re.search(r'extern\s+"C"|__cplusplus', text), f"Owned C interface remains: {path}"
        assert not re.search(r"#\s*define\s+(?:min|max)\s*\(", text), f"Owned min/max macro remains: {path}"
        assert not re.search(r'\(\s*(?:char\s*\*|LPSTR|PSTR)\s*\)\s*"', text), f"Mutable string-literal cast remains: {path}"
        assert not re.search(r'const_cast\s*<\s*char\s*\*\s*>\s*\(\s*"', text), f"Mutable string-literal cast remains: {path}"
print(f"Owned C++20 sources: {len(production)}; compiled variants: {count}; independent C references: {len(references)}")
