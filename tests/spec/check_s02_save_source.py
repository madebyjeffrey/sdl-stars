#!/usr/bin/env python3
"""Link S02 save-stage vectors to production code; no source extraction/rewriting.

The provider fixture retains all 11 stages and 42 original assertions. Real
syscall/crash durability remains outside this bounded effect test.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="s02-save-linked-") as work:
        binary = Path(work) / ("witness.exe" if os.name == "nt" else "witness")
        sources = ["tests/compat/profile_save.cpp",
                   "compat/win32/src/profile_save.cpp", "compat/win32/src/last_error.cpp",
                   "src/platform/fileio.cpp", "src/platform/encoding.cpp"]
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        objects = []
        for index, source in enumerate(sources):
            target = Path(work) / f"{index}.o"
            includes = ["-I" + str(ROOT / "src/platform")]
            if source.startswith(("tests/compat/", "compat/win32/")):
                includes += ["-I" + str(ROOT / "compat/win32/include"),
                             "-I" + str(ROOT / "compat/win32/src")]
            # Native file services consume SDK headers on Windows; wrapper
            # sources consume their own headers, exactly as in the product.
            subprocess.run(compiler + ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                                      "-Werror", "-pthread"] + includes +
                           ["-c", str(ROOT / source), "-o", str(target)], check=True)
            objects.append(str(target))
        subprocess.run(compiler + objects + ["-pthread", "-o", str(binary)] +
                       (["-lbcrypt"] if os.name == "nt" else []), check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
