"""Audit actual compilation, owned declarations and explicit C-reference exceptions."""
from pathlib import Path
import json
import re
import sys

root = Path(sys.argv[1]).resolve()
manifest = Path(sys.argv[2]).resolve()
rows = json.loads(manifest.read_text(encoding="utf-8"))
generated = (manifest.parent / "generated", manifest.parent / "compat/win32/generated")
sdk_shared = {Path(name).with_suffix(".cpp").as_posix() for name in re.findall(r"tests/compat/[A-Za-z0-9_]+\.inc",
                           (root / "cmake/win32-reference-tests.cmake").read_text(encoding="utf-8"))}
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
    assert not re.search(r"(?:-D|/D)(?:STARS_TEST_(?:IMPL|ALLOC|CALL|REAL|STATE_DOUBLE)|[A-Za-z_]\w*=StarsTest_)", command), f"Test symbol substitution in compiler command: {path}"
    assert not re.search(r"(?:stars_core|stars-ui-compat-check|stars_win32|stars_filedialog_windows)-test-", command), f"Private production test clone in compiler command: {path}"
    cpp = "-x c++" in command or "-x objective-c++" in command or path.suffix in (".cpp", ".mm")
    if name.startswith(("src/", "compat/win32/src/")) or any(path.is_relative_to(directory) for directory in generated):
        assert path.suffix in (".cpp", ".mm"), f"Owned production source has a migration extension: {path}"
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
            assert path.suffix in (".cpp", ".mm"), f"Ordinary test source has a migration extension: {path}"
            assert cpp, f"Ordinary test source compiled as C/Objective-C: {path}"
    elif "/sdk-reference/" in path.as_posix():
        assert not cpp, f"SDK reference source must remain C: {path}"
        references.add(path.name)

# A renamed/generated source with an unknown extension can disappear from the
# compiler manifest without failing the build. Account for both header orders.
header_directories = [root / "src/core", root / "src/platform"]
if any(path.is_relative_to(root / "compat/win32/src") for path in production):
    header_directories += [root / "src/ui", root / "compat/win32/include"]
    private_headers = [root / "compat/win32/src" / name
                       for name in ("platform.h", "cleanup.h", "file_read.h")] + sorted((root / "compat/win32/src").glob("*_io.h"))
else:
    private_headers = []
headers = [path for directory in header_directories for path in directory.glob("*.h")] + private_headers
expected_headers = {str(manifest.parent / "header-checks" /
                        (path.relative_to(root).as_posix().replace("/", "_") + "_" + order + ".cpp"))
                    for path in headers for order in ("before", "after")}
actual_headers = {str(Path(row["file"]).resolve()) for row in rows
                  if Path(row["file"]).resolve().parent == manifest.parent / "header-checks"}
assert actual_headers == expected_headers, f"Standalone header compile inventory differs: missing={sorted(expected_headers - actual_headers)}, unexpected={sorted(actual_headers - expected_headers)}"

for directory in ("src", "compat/win32/include", "compat/win32/src"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in (".h", ".c", ".cpp", ".m", ".mm"):
            continue
        text = path.read_text(encoding="utf-8")
        assert not re.search(r"STARS_TEST_(?:IMPL|STATE_DOUBLE|ALLOC_)|stars_test_redirect|StarsTestReal_", text), f"Transitional test routing remains: {path}"
        assert not re.search(r'extern\s+"C"|__cplusplus', text), f"Owned C interface remains: {path}"
        assert not re.search(r"#\s*define\s+(?:min|max)\s*\(", text), f"Owned min/max macro remains: {path}"
        assert not re.search(r'\(\s*(?:char\s*\*|LPSTR|PSTR)\s*\)\s*"', text), f"Mutable string-literal cast remains: {path}"
        assert not re.search(r'const_cast\s*<\s*char\s*\*\s*>\s*\(\s*"', text), f"Mutable string-literal cast remains: {path}"
print(f"Owned C++20 sources: {len(production)}; compiled variants: {count}; independent C references: {len(references)}; standalone headers: {len(headers)} in both orders")
