"""Fail if compiling shared storage as C++ changes its C layout."""
import difflib
import subprocess
import sys

before = subprocess.check_output([sys.argv[1]], text=True).splitlines(keepends=True)
after = subprocess.check_output([sys.argv[2]], text=True).splitlines(keepends=True)
if before != after:
    sys.stderr.writelines(difflib.unified_diff(before, after, fromfile="C11", tofile="C++17"))
    sys.exit(1)
print(f"C/C++ layouts match: {len(before)} types, aliases and record prefixes")
