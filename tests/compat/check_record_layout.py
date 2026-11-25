"""Compare frozen widths/prefixes and explicitly migrated runtime alignments."""
import difflib
import subprocess
import sys

before = subprocess.check_output([sys.argv[1]], text=True).splitlines(keepends=True)
after = subprocess.check_output([sys.argv[2]], text=True).splitlines(keepends=True)
# These families no longer use native layout for file bytes. Their widths and
# offsets still match the independent C fixture, while real task-word references
# require runtime alignment two. Every other alignment comparison stays exact.
migrated = {"ITEMACTION": 2, "TASKXPORT": 10, "ORDER": 18, "PLORD": 4, "RTWAYPT": 22, "ZIPORDER": 24}
runtime_variants = {"OBJ", "PART", "POPUPDATA"}
pointer_width = int(next(line.split()[1] for line in before if line.startswith("OBJ ")))
expected = []
seen = set()
for line in before:
    fields = line.split()
    if fields[0] in runtime_variants:
        if fields[0] == "POPUPDATA":
            assert tuple(map(int, fields[1:])) == (22, 1), "Frozen popup measurement changed"
        elif fields[0] == "PART":
            assert tuple(map(int, fields[1:])) == (4 + pointer_width, 1), "Frozen PART measurement changed"
        else:
            assert tuple(map(int, fields[1:])) == (pointer_width, 1), "Frozen OBJ measurement changed"
        expected.append(None)
    elif fields[0] in migrated:
        name = fields[0]
        assert tuple(map(int, fields[1:])) == (migrated[name], 1), f"Frozen contract changed: {line}"
        expected.append(f"{name} {migrated[name]} 2\n")
        seen.add(name)
    else:
        expected.append(line)
assert seen == set(migrated), "Missing migrated type in the independent reference"
assert len(expected) == len(after), "C++ runtime layout output changed its type inventory"
for index, line in enumerate(expected):
    if line is None:
        name, size, alignment = after[index].split()
        assert name in runtime_variants and int(size) > 0 and int(alignment) == 1, f"Invalid tagged runtime layout: {after[index]}"
        expected[index] = after[index]
if expected != after:
    sys.stderr.writelines(difflib.unified_diff(expected, after, fromfile="frozen widths/prefixes + declared runtime alignment", tofile="C++20 runtime"))
    sys.exit(1)
print(f"{len(before)} historical measurements checked; tagged OBJ/PART runtime variants and their POPUPDATA container may grow while retaining byte alignment; {len(migrated)} migrated runtime alignments verified at two; all other widths/alignments unchanged")
