"""Compare C++ catalogs with bytes captured from develop's C generators."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

expected = json.loads((Path(__file__).with_name("catalog_golden.json")).read_text())
actual = {}
for line in subprocess.check_output([sys.argv[1]], text=True).splitlines():
    name, size, data = line.split()
    actual[name] = {"bytes": int(size), "sha256": hashlib.sha256(bytes.fromhex(data)).hexdigest()}
assert actual == expected, "Catalog bytes differ: " + repr([name for name in actual if actual[name] != expected.get(name)])
print(f"Catalog bytes unchanged: {len(actual)} arrays, {sum(v['bytes'] for v in actual.values())} bytes")
