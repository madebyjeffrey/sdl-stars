"""Immutable extended-arithmetic reference for the binary64 migration tests."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
REF = "3c67394973aacf6b0bf24d0a302f42b50a87bfa0"


def read_source(name):
    return subprocess.check_output(["git", "-C", str(ROOT), "show", f"{REF}:{name}"], text=True)
