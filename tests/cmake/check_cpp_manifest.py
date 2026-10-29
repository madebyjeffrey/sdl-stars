"""Audit actual compilation, owned declarations and explicit C-reference exceptions."""
from pathlib import Path
import json
import re
import sys

root = Path(sys.argv[1]).resolve()
manifest = Path(sys.argv[2]).resolve()
rows = json.loads(manifest.read_text(encoding="utf-8"))
generated = (manifest.parent / "generated", manifest.parent / "compat/win32/generated")
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
        assert re.search(r"-std=(?:gnu|c)\+\+(?:20|2a)(?:\s|$)", command), f"Owned production source lacks C++20: {path}"
        count += 1
        production.add(path)
    elif name.startswith("tests/"):
        if name == "tests/compat/record_layout.c":
            assert not cpp, "Frozen layout reference must remain C"
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
print(f"Owned C++20 sources: {len(production)}; compiled variants: {count}; independent C references: {len(references)}")
