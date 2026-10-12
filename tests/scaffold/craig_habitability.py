#!/usr/bin/env python3
"""Compare extracted Craig-stars Go habitability code with both C variants.

Only temporary copies are compiled; neither repository is modified. Go methods
are copied verbatim, apart from a read-only accumulator trace. The adapter maps
the C race fields and differently ordered trait bits by name.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

import precision
import race_precision

ROOT = precision.ROOT
HERE = precision.HERE
METHODS = ("IsImmune", "HasLRT", "HabCenter", "GetPlanetHabitability", "ComputeRacePoints",
           "getHabRangePoints", "getPlanetHabForHabIndex")
PLANETS = (
    ("witness", (31, 61, 9), (71, 97, 73)),
    ("humanoid", (15, 15, 15), (85, 85, 85)),
    ("full", (0, 0, 0), (100, 100, 100)),
    ("narrow", (40, 40, 40), (60, 60, 60)),
    ("clipped", (0, 80, 0), (20, 100, 100)),
    ("odd-width", (11, 32, 77), (36, 73, 100)),
)

GO_DRIVER = r'''
var storedHab float64

type inputCase struct {
    Input []int64 `json:"input"`
    Extended []float64 `json:"extended"`
    Double []float64 `json:"double"`
    Go []float64 `json:"go"`
}

func adapt(rg []int64) Race {
    r := Race{}
    for i := 0; i < 3; i++ {
        r.HabLow.Set(HabType(i), int(rg[i*3]))
        r.HabHigh.Set(HabType(i), int(rg[i*3+2]))
    }
    r.ImmuneGrav, r.ImmuneTemp, r.ImmuneRad = rg[0] < 0, rg[3] < 0, rg[6] < 0
    r.GrowthRate = int(rg[9])
    flags := uint32(rg[10])
    // Original C bit order differs from Craig-stars after Ultimate Recycling.
    traits := []LRT{IFE, TT, ARM, ISB, GR, UR, MA, NRSE, CE, OBRM, NAS, LSP, BET, RS}
    for i, lrt := range traits { if flags & (1 << i) != 0 { r.LRTs |= Bitmask(lrt) } }
    r.FactoriesCostLess = flags & (1 << 31) != 0
    r.TechsStartHigh = flags & (1 << 29) != 0
    r.PopEfficiency = int(rg[11])
    r.FactoryOutput, r.FactoryCost, r.NumFactories = int(rg[12]), int(rg[13]), int(rg[14])
    r.MineOutput, r.MineCost, r.NumMines = int(rg[15]), int(rg[16]), int(rg[17])
    levels := []ResearchCostLevel{ResearchCostExtra, ResearchCostStandard, ResearchCostLess}
    r.ResearchCost = ResearchCost{levels[rg[19]], levels[rg[20]], levels[rg[21]],
                                 levels[rg[22]], levels[rg[23]], levels[rg[24]]}
    r.PRT = PRTs[rg[25]]
    return r
}

type planetConfig struct { Name string; Low, High [3]int }
type planetStats struct {
    Name string `json:"name"`
    Evaluated uint64 `json:"evaluated"`
    Different uint64 `json:"different"`
    MaxAbs int `json:"max_abs"`
    First any `json:"first"`
}

func comparePlanets() {
    configs := []planetConfig{CONFIGS}
    stats := []planetStats{{Name:"Go vs extended C"}, {Name:"Go vs double C"}}
    reader := bufio.NewReaderSize(os.Stdin, 1048576)
    if _, err := reader.ReadString('\n'); err != nil { panic(err) } // C oracle environment record
    var pair [4]byte
    for _, config := range configs {
        for mask := 0; mask < 8; mask++ {
            r := Race{HabLow: Hab{config.Low[0],config.Low[1],config.Low[2]},
                      HabHigh: Hab{config.High[0],config.High[1],config.High[2]},
                      ImmuneGrav:mask&1 != 0, ImmuneTemp:mask&2 != 0, ImmuneRad:mask&4 != 0}
            for grav := 0; grav <= 100; grav++ {
                for temp := 0; temp <= 100; temp++ {
                    for rad := 0; rad <= 100; rad++ {
                        if _, err := io.ReadFull(reader, pair[:]); err != nil { panic(err) }
                        values := [2]int{int(int16(binary.LittleEndian.Uint16(pair[:2]))),
                                         int(int16(binary.LittleEndian.Uint16(pair[2:])))}
                        got := r.GetPlanetHabitability(Hab{grav,temp,rad})
                        for i, value := range values {
                            s := &stats[i]
                            s.Evaluated++
                            if got != value {
                                s.Different++
                                delta := got-value; if delta < 0 { delta = -delta }
                                if delta > s.MaxAbs { s.MaxAbs = delta }
                                if s.First == nil { s.First = map[string]any{"configuration":config.Name,
                                    "immunity_mask":mask,"planet":[]int{grav,temp,rad},"c":value,"go":got} }
                            }
                        }
                    }
                }
            }
        }
    }
    if _, err := reader.ReadByte(); err != io.EOF { panic("Unexpected extra planet records") }
    if err := json.NewEncoder(os.Stdout).Encode(stats); err != nil { panic(err) }
}

func main() {
    if len(os.Args)>1 && os.Args[1]=="planets" { comparePlanets(); return }
    scanner := bufio.NewScanner(os.Stdin)
    writer := bufio.NewWriterSize(os.Stdout, 1048576)
    defer writer.Flush()
    encoder := json.NewEncoder(writer)
    for scanner.Scan() {
        var c inputCase
        if err := json.Unmarshal(scanner.Bytes(), &c); err != nil { panic(err) }
        if c.Input == nil { continue }
        if len(c.Input) != 27 { panic("Bad race input width") }
        r := adapt(c.Input)
        hab := r.getHabRangePoints()
        sum := storedHab
        points := r.ComputeRacePoints(1650)
        c.Go = []float64{sum,float64(hab),float64(hab/2000),float64(points)}
        if err := encoder.Encode(c); err != nil { panic(err) }
    }
    if err := scanner.Err(); err != nil { panic(err) }
}
'''

PLANET_DRIVER = r'''
#include "common.h"
#include "precision_probe.c"
PLAYER rgplr[16];
int16_t ext_PctPlanetDesirability(PLANET *, int16_t);
int16_t dbl_PctPlanetDesirability(PLANET *, int16_t);

int main(void) {
    int rgLow[][3] = {LOWS};
    int rgHigh[][3] = {HIGHS};
    int iConfig, iMask, i, x, y, z;
    PLANET pl = {0};
    int16_t rgResult[2];

    if (!FCheckPrecision()) return 2;
    for (iConfig = 0; iConfig < CONFIG_COUNT; iConfig++) {
        for (iMask = 0; iMask < 8; iMask++) {
            for (i = 0; i < 3; i++) {
                rgplr[0].rgEnvVarMin[i] = iMask & (1 << i) ? envImmune : rgLow[iConfig][i];
                rgplr[0].rgEnvVarMax[i] = iMask & (1 << i) ? envImmune : rgHigh[iConfig][i];
                rgplr[0].rgEnvVar[i] = iMask & (1 << i) ? envImmune : (rgLow[iConfig][i] + rgHigh[iConfig][i]) / 2;
            }
            for (x = 0; x <= 100; x++) {
                for (y = 0; y <= 100; y++) {
                    for (z = 0; z <= 100; z++) {
                        pl.rgEnvVar[0] = x; pl.rgEnvVar[1] = y; pl.rgEnvVar[2] = z;
                        rgResult[0] = ext_PctPlanetDesirability(&pl, 0);
                        rgResult[1] = dbl_PctPlanetDesirability(&pl, 0);
                        if (fwrite(rgResult, sizeof(rgResult), 1, stdout) != 1) return 3;
                    }
                }
            }
        }
    }
    return 0;
}
'''


def declaration(source, pattern):
    match = re.search(pattern, source)
    if not match:
        raise ValueError("Craig-stars declaration changed: " + pattern)
    masked = precision.mask_c(source)
    start = masked.index("{", match.start())
    depth = 0
    for end in range(start, len(masked)):
        if masked[end] == "{":
            depth += 1
        elif masked[end] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():end+1]
    raise ValueError("Unbalanced Go declaration")


def go_source(craig):
    race = (craig / "cs/race.go").read_text()
    hab = (craig / "cs/hab.go").read_text()
    math_source = (craig / "cs/math.go").read_text()
    chunks = ['package main\nimport ("fmt";"math";"os";"bufio";"encoding/json";"encoding/binary";"io")',
              'type DBObject struct { ID int64 }; type RaceSpec struct {}; type Bitmask uint32',
              declaration(race, r"type Race struct"),
              race[race.index("type ResearchCostLevel"):race.index("func (rc ResearchCost) Get")],
              race[race.index("type PRT string"):race.index("func (r *Race) IsImmune")],
              hab[hab.index("type Hab struct"):]]
    for name in METHODS:
        method = declaration(race, r"func \([^)]*\) " + name + r"\(")
        if name == "getHabRangePoints":
            old = "return int64(points/10.0 + 0.5)"
            if method.count(old) != 1:
                raise ValueError("Craig-stars accumulator instrumentation changed")
            method = method.replace(old, "storedHab = points\n\t" + old)
        chunks.append(method)
    # The extracted methods use these generic helpers only with ints. Narrow the
    # external constraints declaration, preserving the actual helper bodies.
    chunks.append(declaration(math_source, r"func Clamp\[").replace("constraints.Ordered", "interface{~int}"))
    chunks.append(declaration(math_source, r"func Abs\[").replace("constraints.Integer | constraints.Float", "interface{~int}"))
    configs = ",".join('{"%s",[3]int{%s},[3]int{%s}}' % (name, ",".join(map(str,lo)), ",".join(map(str,hi)))
                       for name, lo, hi in PLANETS)
    chunks.append(GO_DRIVER.replace("CONFIGS", configs))
    return "\n\n".join(chunks)


def corpus_source():
    source = (HERE / "precision_race.c").read_text()
    needle = "    Record(&rgStats[4], 0, cBytes, rg, 27, 0);"
    if source.count(needle) != 1:
        raise ValueError("C corpus instrumentation changed")
    trace = r'''
    printf("{\"input\":[");
    for (i = 0; i < 27; i++) printf("%s%.0f", i != 0 ? "," : "", rg[i]);
    printf("],\"extended\":[%.17g,%d,%d,%d],\"double\":[%.17g,%d,%d,%d]}\n",
           dSumA, lA, lA / 2000, cPtsA, dSumB, lB, lB / 2000, cPtsB);
'''
    return source.replace(needle, needle + trace)


def key(value):
    bits = struct.unpack(">Q", struct.pack(">d", value))[0]
    return ((~bits + 1) & ((1 << 64) - 1)) if bits >> 63 else bits | (1 << 63)


def measure(rows):
    results = []
    for reference in ("extended", "double"):
        for i, name in enumerate(("stored habitability accumulator", "integer innate habitability",
                                  "habitability / 2000", "returned advantage points")):
            results.append({"reference": reference, "output": name, "evaluated": 0, "different": 0,
                            "max_abs": 0, "max_ulps": 0, "first": None, "largest": None})
    with rows.open() as file:
        for line in file:
            case = json.loads(line)
            for index, result in enumerate(results):
                output = index % 4
                a, b = case[result["reference"]][output], case["go"][output]
                result["evaluated"] += 1
                if a == b:
                    continue
                result["different"] += 1
                witness = {"input": case["input"], "c": a, "go": b,
                           "c_hex": float(a).hex(), "go_hex": float(b).hex()}
                if result["first"] is None:
                    result["first"] = witness
                if abs(a-b) > result["max_abs"]:
                    result["max_abs"] = abs(a-b)
                    result["largest"] = witness
                if output == 0:
                    result["max_ulps"] = max(result["max_ulps"], abs(key(a)-key(b)))
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--craig", type=Path, default=ROOT.parent / "craig-stars")
    parser.add_argument("--samples", type=int, default=100000)
    parser.add_argument("--odd-widths", action="store_true")
    parser.add_argument("--skip-planets", action="store_true")
    parser.add_argument("--no-axis-sweep", action="store_true")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--go", default="go")
    parser.add_argument("--generated-dir", type=Path, default=ROOT / "dist/host-release/generated")
    parser.add_argument("--report", type=Path, default=ROOT / "dist/scaffold/precision/craig-habitability.json")
    args = parser.parse_args()
    if not 0 <= args.samples <= 100000000:
        parser.error("Invalid sample count")
    craig = args.craig.resolve()
    paths = [craig / ("cs/" + name) for name in ("race.go", "hab.go", "math.go", "bitmask.go")]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    report = {"c_reference_commit": race_precision.precision_reference.REF, "craig_source_sha256": hashes,
              "craig_commit": subprocess.check_output(["git", "-C", str(craig), "rev-parse", "HEAD"], text=True).strip(),
              "go_version": subprocess.check_output([args.go, "version"], text=True).strip(),
              "cc_version": subprocess.check_output([args.cc, "--version"], text=True).splitlines()[0],
              "samples": args.samples, "seed": 0x123456789abcdef, "go_amd64": "v1",
              "axis_sweep": not args.no_axis_sweep, "odd_widths": args.odd_widths,
              "mapping": "C LRT bits mapped by trait name; research 0=Extra/1=Standard/2=Less; PRT indices 0..9; 1650 starting points",
              "limitations": "Valid centered habitats only. Race corpus is finite; planet coordinates are exhaustive only for the six selected range triples and eight immunity masks."}
    with tempfile.TemporaryDirectory(prefix="stars-craig-hab-") as directory:
        work = Path(directory)
        source = race_precision.sources()
        report["c_source_sha256"] = hashlib.sha256(source.encode()).hexdigest()
        for mode in ("ext", "dbl"):
            (work / (mode + ".c")).write_text(race_precision.variant(source, mode))
        (work / "corpus.c").write_text(corpus_source())
        (work / "main.go").write_text(go_source(craig))
        flags = ["-O2", "-std=gnu11", "-fsigned-char", "-fno-fast-math", "-ffp-contract=off",
                 "-iquote", str(ROOT / "src/core"), "-iquote", str(ROOT / "src/platform"), "-iquote", str(HERE), "-iquote", str(args.generated_dir.resolve())]
        objects = [str(work / "ext.c"), str(work / "dbl.c")]
        print("Compiling extracted C and Go calculators", flush=True)
        subprocess.run([args.cc, *flags, *objects, str(work / "corpus.c"), "-lm", "-o", str(work / "corpus")], check=True)
        env = dict(os.environ, GO111MODULE="off", GOTOOLCHAIN="local", GOPROXY="off", GOAMD64="v1", GOCACHE=str(work / "go-cache"))
        subprocess.run([args.go, "build", "-o", str(work / "craig"), str(work / "main.go")], env=env, check=True)
        print("Generating the identical C race corpus", flush=True)
        with (work / "corpus.jsonl").open("w") as output:
            subprocess.run([str(work / "corpus"), str(args.samples), str(int(args.odd_widths)), "0", "0",
                            str(0x123456789abcdef), str(int(not args.no_axis_sweep))], stdout=output, check=True)
        with (work / "corpus.jsonl").open() as file:
            report["oracle"] = json.loads(file.readline())["environment"]
        print("Evaluating Craig-stars on the same corpus", flush=True)
        with (work / "corpus.jsonl").open() as inputs, (work / "results.jsonl").open("w") as output:
            subprocess.run([str(work / "craig")], stdin=inputs, stdout=output, check=True)
        report["races"] = measure(work / "results.jsonl")
        for result in report["races"]:
            print(f"Go vs {result['reference']} C: {result['output']}: {result['different']}/{result['evaluated']} differ; max={result['max_abs']}", flush=True)
        if not args.skip_planets:
            planet = PLANET_DRIVER.replace("CONFIG_COUNT", str(len(PLANETS)))
            for name, position in (("LOWS", 1), ("HIGHS", 2)):
                planet = planet.replace(name, ",".join("{" + ",".join(map(str,config[position])) + "}" for config in PLANETS))
            (work / "planets.c").write_text(planet)
            subprocess.run([args.cc, *flags, *objects, str(work / "planets.c"), "-lm", "-o", str(work / "planets")], check=True)
            print("Exhausting 0..100 on all three planet axes, six ranges, eight immunity masks", flush=True)
            with (work / "planets.bin").open("wb") as output:
                subprocess.run([str(work / "planets")], stdout=output, check=True)
            with (work / "planets.bin").open("rb") as inputs:
                result = subprocess.run([str(work / "craig"), "planets"], stdin=inputs, capture_output=True, text=True, check=True)
            report["planets"] = json.loads(result.stdout)
            report["planet_configs"] = PLANETS
            for result in report["planets"]:
                print(f"{result['name']}: {result['different']}/{result['evaluated']} planets differ", flush=True)
    if any(hashlib.sha256(p.read_bytes()).hexdigest() != hashes[str(p)] for p in paths):
        raise RuntimeError("Craig-stars sources changed during the comparison")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# Craig-stars habitability comparison", "", f"Craig-stars commit: `{report['craig_commit']}`", "",
             "| Reference | Output | Evaluated | Different | Maximum absolute difference |",
             "| --- | --- | ---: | ---: | ---: |"]
    for result in report["races"]:
        lines.append(f"| {result['reference']} C | {result['output']} | {result['evaluated']:,} | {result['different']:,} | {result['max_abs']:.10g} |")
    for result in report.get("planets", []):
        lines.append(f"| {result['name']} | planet habitability | {result['evaluated']:,} | {result['different']:,} | {result['max_abs']} |")
    lines += ["", report["limitations"], "", "Full source hashes, mapping and counterexamples are in the JSON report.", ""]
    args.report.with_suffix(".md").write_text("\n".join(lines))
    print("Report: " + str(args.report.resolve()), flush=True)


if __name__ == "__main__":
    main()
