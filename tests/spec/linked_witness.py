"""Locate/build normal production-linked witness targets; no source extraction."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def binary(name):
    return target("stars-witness-" + name)


def target(name):
    supplied = os.environ.get("STARS_WITNESS_EXE")
    if supplied:
        return Path(supplied).with_name(name + (".exe" if sys.platform == "win32" else ""))
    build = Path(os.environ.get("STARS_WITNESS_BUILD", ROOT / "dist/linked-witnesses"))
    if not build.is_absolute():
        build = ROOT / build
    preset = "windows-sdl-debug" if sys.platform == "win32" else "macos-sdl-debug" if sys.platform == "darwin" else "sdl-debug"
    commands = []
    if not (build / "CMakeCache.txt").exists():
        commands.append(["cmake", "--preset", preset, "-B", str(build), "-DSTARS_ENABLE_SANITIZERS=ON"])
    commands.append(["cmake", "--build", str(build), "--target", name, "--parallel", "2"])
    for command in commands:
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            sys.stderr.write(result.stdout)
            raise subprocess.CalledProcessError(result.returncode, command)
    return build / (name + (".exe" if sys.platform == "win32" else ""))
