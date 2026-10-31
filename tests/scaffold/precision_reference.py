"""Immutable extended-arithmetic reference for the binary64 migration tests."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
REF = "3c67394973aacf6b0bf24d0a302f42b50a87bfa0"


def source_names():
    return set(subprocess.check_output(["git", "-C", str(ROOT), "ls-tree", "--name-only", REF], text=True).splitlines())


def read_source(name):
    # Current source extensions do not change the immutable C reference paths.
    name = str(Path(name).with_suffix(".c")) if name.endswith(".cpp") else name
    return subprocess.check_output(["git", "-C", str(ROOT), "show", f"{REF}:{name}"], text=True)
