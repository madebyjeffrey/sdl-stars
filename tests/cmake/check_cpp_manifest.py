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
sdk_entries = {"tests/reference/sdk/" + Path(name).stem + ".c" for name in sdk_shared}
for name in sdk_entries:
    path = root / name
    expected = ('#include <assert.h>\n#include <windows.h>\ntypedef MSG APPMSG;\n'
                + '#include "../../compat/' + path.stem + '.inc"\n')
    assert path.read_text(encoding="utf-8") == expected, f"SDK entry changes independent vector inputs: {path}"
assert {path.relative_to(root).as_posix() for path in (root / "tests/reference/sdk").glob("*.c")} == sdk_entries, "SDK entry inventory differs"
assert len(sdk_entries) == 38, "Shared SDK vector inventory differs"
count = 0
production = set()
references = set()
for row in rows:
    path = Path(row["file"]).resolve()
    try:
        name = path.relative_to(root).as_posix()
    except ValueError:
        name = ""
    assert "/sdk-reference/" not in path.as_posix(), f"Generated SDK language adapter remains: {path}"
    command = row.get("command", " ".join(row.get("arguments", [])))
    assert not re.search(r"(?:-D|/D)(?:STARS_TEST_(?:IMPL|ALLOC|CALL|REAL|STATE_DOUBLE)|[A-Za-z_]\w*=StarsTest_)", command), f"Test symbol substitution in compiler command: {path}"
    assert not re.search(r"(?:stars_core|stars-ui-compat-check|stars_win32|stars_filedialog_windows)-test-", command), f"Private production test clone in compiler command: {path}"
    cpp = "-x c++" in command or "-x objective-c++" in command or path.suffix in (".cpp", ".mm")
    if name.startswith(("src/", "compat/win32/src/")) or any(path.is_relative_to(directory) for directory in generated):
        assert path.suffix in (".cpp", ".mm"), f"Owned production source has a migration extension: {path}"
        assert cpp, f"Owned production source compiled as C/Objective-C: {path}"
        assert not re.search(r"(?:^|\s)-x\s", command.replace("-x objective-c++", "") if path.suffix == ".mm" else command), f"Owned production source has a language override: {path}"
        standards = re.findall(r"-std=(\S+)", command)
        assert standards and standards[-1] in ("c++20", "c++2a"), f"Owned production source lacks ISO C++20: {path}"
        assert "-Wpedantic" in command and "-Wold-style-cast" in command and "-Wzero-as-null-pointer-constant" in command and "-Werror" in command, f"Owned production source lacks strict diagnostics: {path}"
        assert not re.search(r"-fpermissive|-Wno-(?:missing-field-initializers|missing-braces|pedantic|old-style-cast|zero-as-null-pointer-constant)|-Wno-error(?:=pedantic|=old-style-cast|=zero-as-null-pointer-constant)?(?:\s|$)", command), f"Owned production source disables strict diagnostics: {path}"
        count += 1
        production.add(path)
    elif name.startswith("tests/"):
        if name not in sdk_shared:
            assert "-Wno-missing-field-initializers" not in command and "-Wno-missing-braces" not in command, f"Ordinary C++ consumer retains a migration suppression: {path}"
        if name in ("tests/compat/record_layout.c", "tests/compat/popup_payload_reference.c") or name in sdk_entries:
            assert not cpp, "Independent frozen-declaration reference must remain C"
            references.add(name)
        else:
            assert path.suffix in (".cpp", ".mm"), f"Ordinary test source has a migration extension: {path}"
            assert cpp, f"Ordinary test source compiled as C/Objective-C: {path}"

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
cache = manifest.parent / "CMakeCache.txt"
ordinary = True
if cache.exists():
    match = re.search(r"^STARS_BUILD_TESTS:BOOL=(ON|OFF)$", cache.read_text(encoding="utf-8"), re.MULTILINE)
    assert match, f"Missing STARS_BUILD_TESTS option in {cache}"
    ordinary = match.group(1) == "ON"
expected_headers = {str(manifest.parent / "header-checks" /
                        (path.relative_to(root).as_posix().replace("/", "_") + "_" + order + ".cpp"))
                    for path in headers for order in ("before", "after")} if ordinary else set()
actual_headers = {str(Path(row["file"]).resolve()) for row in rows
                  if Path(row["file"]).resolve().parent == manifest.parent / "header-checks"}
assert actual_headers == expected_headers, f"Standalone header compile inventory differs: missing={sorted(expected_headers - actual_headers)}, unexpected={sorted(actual_headers - expected_headers)}"

# Ordinary state uses typed initialization. Exact byte redaction remains
# enumerated with its live technical reason, rather than a directory exception.
initialization = json.loads((root / "docs/CPP-LIVE-INITIALIZATION.json").read_text())
byte_resets = {(entry["path"], re.sub(r"\s+", "", entry["code"]))
               for entry in initialization["initialization_sites"]
               if entry["disposition"] == "retained-byte-range" and entry.get("reason")}

ownership = json.loads((root / "docs/CPP-LIVE-OWNERSHIP.json").read_text())
ownership_contracts = {entry["id"]: entry for entry in ownership["contracts"]}
heap_sites = {(entry["path"], entry["normalized_call"]) for entry in ownership["sites"]
              if ownership_contracts[entry["contract"]].get("owner") and ownership_contracts[entry["contract"]].get("reason")}

for directory in ("src", "compat/win32/include", "compat/win32/src"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in (".h", ".c", ".cpp", ".m", ".mm"):
            continue
        assert path.suffix not in (".c", ".m"), f"Owned source has a migration extension: {path}"
        text = path.read_text(encoding="utf-8")
        assert not re.search(r"STARS_TEST_(?:IMPL|STATE_DOUBLE|ALLOC_)|stars_test_redirect|StarsTestReal_", text), f"Transitional test routing remains: {path}"
        assert not re.search(r"GetAppSession|GetAppUiState|\bUiSession\s*\(", text), f"Implicit application state accessor remains: {path}"
        assert not re.search(r'extern\s+"C"|__cplusplus', text), f"Owned C interface remains: {path}"
        # These declarations are C++-only; comments and literals are not code.
        code = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|R"(?P<delimiter>[^ ()\\\t\r\n]*)\([\s\S]*?\)(?P=delimiter)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', ' ', text)
        assert not re.search(r"\bNULL\b", code), f"Owned null spelling must use nullptr: {path}"
        assert not re.search(r"\btypedef\b", code), f"Owned type aliases must use C++ declarations: {path}"
        assert not re.search(r"\b(?:FARPROC|MakeProcInstance|FreeProcInstance)\b", code), f"Use typed callbacks without Win16 instance wrappers: {path}"
        if path.is_relative_to(root / "src") and not path.is_relative_to(root / "src/platform"):
            assert not re.search(r"\bFreeResource\s*\(", code), f"Borrow module bytes; release owned heap copies with their allocator: {path}"
        assert not re.search(r"(?<![\w:.])(?:malloc|calloc|realloc|free)\s*\(", code), f"Owned CRT allocator calls must use std qualification: {path}"
        for allocation in re.finditer(r"\b(?:(?:std\s*::\s*)?(?:malloc|calloc|realloc|free)|strdup|SDL_malloc|SDL_calloc|SDL_realloc|SDL_free|SDL_strdup)\s*\(", code):
            end, depth = allocation.end(), 1
            while depth:
                if code[end] == "(": depth += 1
                elif code[end] == ")": depth -= 1
                end += 1
            call = re.sub(r"\s+", "", code[allocation.start():end])
            assert (path.relative_to(root).as_posix(), call) in heap_sites, f"New raw heap call needs a specific owner/lifetime contract in CPP-LIVE-OWNERSHIP.json: {path}"
        for reset in re.finditer(r"\b(?:std::)?memset\s*\(", code):
            statement = code[reset.start():].split(";", 1)[0]
            assert (path.relative_to(root).as_posix(), re.sub(r"\s+", "", statement)) in byte_resets, f"Uninventoried object byte reset; use typed initialization or record its exact byte contract: {path}"
        assert not re.search(r"#\s*include\s*<(?:assert|ctype|errno|fenv|float|inttypes|limits|locale|math|setjmp|signal|stdarg|stddef|stdint|stdio|stdlib|string|time|uchar|wchar|wctype)\.h>", text), f"Owned standard headers must use C++ spellings: {path}"
        assert not re.search(r"(?<![\w:])(?:u?int(?:8|16|32|64)_t|u?intptr_t|u?intmax_t|size_t|ptrdiff_t|max_align_t|FILE|fpos_t|va_list|time_t|clock_t)\b", code), f"Owned standard types must use std qualification: {path}"
        assert not re.search(r"#\s*define\s+(?:min|max)\s*\(", text), f"Owned min/max macro remains: {path}"
        assert not re.search(r'\(\s*(?:char\s*\*|LPSTR|PSTR)\s*\)\s*"', text), f"Mutable string-literal cast remains: {path}"
        assert not re.search(r'const_cast\s*<\s*char\s*\*\s*>\s*\(\s*"', text), f"Mutable string-literal cast remains: {path}"
print(f"Owned C++20 sources: {len(production)}; compiled variants: {count}; independent C references: {len(references)}; standalone headers: {len(headers) if ordinary else 0} in both orders")
