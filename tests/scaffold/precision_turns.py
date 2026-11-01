#!/usr/bin/env python3
"""Compare real turns from isolated extended and double-only native host builds."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import regression
import precision_reference

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def run(command, log, env=None):
    print("Running: " + " ".join(map(str, command)), flush=True)
    with log.open("w") as output:
        subprocess.run(list(map(str, command)), check=True, stdout=output, stderr=subprocess.STDOUT, env=env)


def snapshot(destination, fDouble, core):
    precision_reference.snapshot(destination)
    hashes = {}
    cCasts = 0
    for name in sorted(precision_reference.source_names()):
        if not name.endswith(".c"):
            continue
        path = destination / name
        source = path.read_text()
        hashes[name] = hashlib.sha256(source.encode()).hexdigest()
        if name in core:
            cCasts += source.count("long double")
            if fDouble:
                path.write_text(source.replace("long double", "double"))
    return hashes, cCasts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, default=ROOT / "dist/scaffold/precision/turns")
    parser.add_argument("--through", type=int, choices=regression.CHECKPOINTS[1:], default=150)
    parser.add_argument("--scenario", choices=regression.SCENARIOS, action="append")
    parser.add_argument("--seed", type=int, default=12345)
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--cli", type=Path, default=ROOT / "dist/stars-save")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--fail-on-difference", action="store_true")
    args = parser.parse_args()
    work = args.work.resolve()
    if work.exists():
        parser.error("Use a fresh --work directory; existing results are never deleted")
    if not args.cli.is_file():
        parser.error("Build the save comparator first with make save-cli")
    if not 0 <= args.seed <= 0xffffffff or args.jobs <= 0:
        parser.error("Invalid seed or jobs")
    if any(c.isspace() for c in str(work)):
        parser.error("Work path must not contain whitespace (game filenames cannot be quoted)")
    work.mkdir(parents=True)
    # Reject a double-only 'extended' oracle before attempting turn comparisons.
    run([sys.executable, HERE / "precision.py", "--cc", args.cc, "--function", "IdGetBestScannerDest",
         "--skip-race", "--report", work / "environment.json"], work / "environment.log")
    cmake = precision_reference.read_source("CMakeLists.txt")
    core = re.search(r"set\(STARS_CORE_SOURCES\s+(.*?)\)", cmake, re.S).group(1).split()
    source_hashes = None
    for mode in ("ext", "dbl"):
        hashes, cCasts = snapshot(work / (mode + "-source"), mode == "dbl", core)
        if source_hashes is not None and hashes != source_hashes:
            raise ValueError("Production sources changed between snapshots")
        source_hashes = hashes
    # Both source snapshots share a fixed product version so build metadata cannot
    # masquerade as a precision difference. File-format versions remain in source.
    env = dict(os.environ, STARS_VERSION="2.9.0-precision-test")
    scenarios = args.scenario or regression.SCENARIOS
    common = [item for name in scenarios for item in ("--scenario", name)] + ["--through", str(args.through)]
    for mode in ("ext", "dbl"):
        build = work / (mode + "-build")
        run(["cmake", "-S", work / (mode + "-source"), "-B", build, "-G", "Ninja",
             "-DCMAKE_C_COMPILER=" + args.cc,
             *(["-DCMAKE_OSX_ARCHITECTURES=x86_64"] if precision_reference.oracle_flags() else []),
             "-DPython3_EXECUTABLE=" + sys.executable, "-DCMAKE_BUILD_TYPE=Release", "-DSTARS_BUILD_TESTS=OFF", "-DSTARS_BUILD_SDL_CLIENT=OFF"],
            work / (mode + "-configure.log"), env)
        run(["cmake", "--build", build, "--target", "stars-host", "--parallel", args.jobs],
            work / (mode + "-build.log"), env)
        run([sys.executable, HERE / "regression.py", "prepare", "--exe", build / "bin/stars-host",
             "--seed", args.seed, "--work", work / mode], work / (mode + "-prepare.log"))
        run([sys.executable, HERE / "regression.py", "run", "--work", work / mode,
             "--cli", args.cli.resolve(), *common], work / (mode + "-run.log"))
    comparison = work / "comparison.json"
    command = [sys.executable, HERE / "regression.py", "compare", work / "ext", work / "dbl",
               "--cli", args.cli.resolve(), "--report", comparison, *common]
    with (work / "compare.log").open("w") as output:
        proc = subprocess.run(list(map(str, command)), stdout=output, stderr=subprocess.STDOUT)
    if proc.returncode not in (0, 1) or not comparison.is_file():
        raise RuntimeError("Save comparison failed; see compare.log")
    records = json.loads(comparison.read_text())
    failures = [r for r in records if not r.get("match", False)]
    if any("error" in r for r in failures):
        raise RuntimeError("Missing/error checkpoint; see comparison.json")
    first = {name: next((r for r in failures if r["scenario"] == name), None) for name in scenarios}
    summary = {"reference_commit": precision_reference.REF, "source_sha256": source_hashes, "replaced_core_casts": cCasts, "seed": args.seed,
               "through": args.through, "scenarios": scenarios, "matching_files": len(records) - len(failures),
               "differing_files": len(failures), "first_checkpoint_differences": first,
               "comparison": str(comparison), "oracle": json.loads((work / "environment.json").read_text())["environment"]}
    (work / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print((work / "compare.log").read_text(), end="")
    for name, record in first.items():
        if record:
            print(f"{name}: first differing checkpoint {record['turn']}, {record['file']}\n{record['detail']}")
    print(f"Summary: {work / 'summary.json'}")
    if args.fail_on_difference and failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
