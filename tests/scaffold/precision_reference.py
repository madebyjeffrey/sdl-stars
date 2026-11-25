"""Immutable extended-arithmetic reference for the binary64 migration tests."""

from pathlib import Path
import subprocess
import io
import platform
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[2]
REF = "3c67394973aacf6b0bf24d0a302f42b50a87bfa0"


def source_names():
    return set(subprocess.check_output(["git", "-C", str(ROOT), "ls-tree", "--name-only", REF], text=True).splitlines())


def read_source(name):
    return subprocess.check_output(["git", "-C", str(ROOT), "show", f"{REF}:{name}"], text=True)


def snapshot(destination):
    """Materialize one immutable tree; never combine old C with current headers."""
    destination.mkdir()
    archive = subprocess.check_output(["git", "-C", str(ROOT), "archive", REF])
    with tarfile.open(fileobj=io.BytesIO(archive)) as source:
        source.extractall(destination, filter="data")
    return destination


def include_flags(work):
    reference = snapshot(work / "reference")
    generated = reference / "generated"
    for name in ("text/textgen.py", "data/datagen.py"):
        subprocess.run([sys.executable, str(reference / name), str(generated)],
                       check=True, stdout=subprocess.DEVNULL)
    return ["-iquote", str(reference), "-iquote", str(generated)]


def oracle_flags():
    # Darwin arm64 has binary64 long double; the existing oracle needs x87.
    # Rosetta runs the x86 executable, and FCheckPrecision still validates it.
    return ["-arch", "x86_64"] if platform.system() == "Darwin" and platform.machine() == "arm64" else []
