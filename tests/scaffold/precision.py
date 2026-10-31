#!/usr/bin/env python3
"""Measure replacing every production long-double intermediate with double.

Extract expressions from the current source, retaining result casts and integer
operations. Domain sweeps are exhaustive only over the reported operand grids;
larger products get a boundary grid and deterministic samples. They do not prove
reachability or whole-game equivalence. No production source/baseline is edited.
"""

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import struct
import subprocess
import tempfile

import precision_reference

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
TYPES = r"(?:int16_t|int32_t|uint16_t|uint32_t|float|double)"
TRANSPARENT = {"LOWORD", "sqrt", "pow", "atan2", "sin", "cos"}
INPUT_NAME = r"(?:i|l|w|dw|pct|d|f)Input\d+"


@dataclass
class Domain:
    name: str
    low: int = 0
    high: int = 0
    step: int = 1
    values: tuple = ()
    description: str = ""
    float_bits: bool = False

    @property
    def count(self):
        return len(self.values) if self.values else (self.high - self.low) // self.step + 1

    def value(self, index):
        if self.float_bits:
            return struct.unpack("<f", struct.pack("<I", self.low + index * self.step))[0]
        return self.values[index] if self.values else self.low + index * self.step

    def edges(self):
        indices = {0, 1, self.count - 2, self.count - 1}
        indices.update((self.count - 1) * i // 8 for i in range(9))
        return sorted(i for i in indices if 0 <= i < self.count)


def float32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def neighbors32(value):
    value = float32(value)
    bits = struct.unpack("<I", struct.pack("<f", value))[0]
    if value >= 0:
        return [struct.unpack("<f", struct.pack("<I", b))[0]
                for b in (bits - 1, bits, bits + 1) if 0 <= b <= 0x3f800000]
    return [value]


def grid(name, low, high, steps, fFloat=False):
    values = set()
    for i in range(steps + 1):
        value = low + (high - low) * i / steps
        neighbors = neighbors32(value) if fFloat else (math.nextafter(value, -math.inf), value,
                                                       math.nextafter(value, math.inf))
        values.update(v for v in neighbors if low <= v <= high)
    return Domain(name, values=tuple(sorted(values)),
                  description=f"{steps + 1} equally spaced values in [{low}, {high}], with IEEE neighbors")


def domains():
    import yaml

    result = {}
    for name, low, high, step in (
        ("bool", 0, 1, 1), ("size", 400, 2000, 400), ("root", 0, 30000, 1),
        ("i16", -32768, 32767, 1), ("u16", 0, 65535, 1), ("coord", 0, 6400, 1),
        ("delta", -6400, 6400, 1), ("radius", 0, 6400, 1), ("square", 0, 81920000, 1),
        ("signed_square", -40960000, 40960000, 1),
        ("count", 0, 1000000, 1), ("large", 0, 1000000000, 1),
        ("mines", 0, 2147483647, 1), ("warp", 0, 16, 1),
        ("eff", 7, 25, 1), ("energy", 1, 26, 1), ("hab", 25, 100, 1),
        ("inc", 0, 100, 1), ("defenses", 0, 100, 1), ("defguess", 0, 15, 1),
        ("popguess", 0, 4095, 1), ("slots", 0, 255, 1), ("power", 0, 200, 1)):
        result[name] = Domain(name, low, high, step, description=f"every integer {low}..{high}, step {step}")
    planetary = yaml.safe_load((ROOT / "data/parts/planetary.yaml").read_text())
    names = [part["name"] for part in planetary]
    defenses = planetary[names.index("iplanetarySDI"):names.index("iplanetaryNeutronShield") + 1]
    bombs = yaml.safe_load((ROOT / "data/parts/bombs.yaml").read_text())
    abilities = tuple(sorted({part["grAbility"] for part in defenses}))
    smart_bombs = tuple(sorted({part["dDmgCol"] for part in bombs
                               if part.get("dDmgBldg", 0) == 0 and part.get("dDmgCol", 0) != 0}))
    for name, values in (("ability", abilities), ("round", (-0.5, 0.5)), ("bomb", smart_bombs)):
        result[name] = Domain(name, values=values, description="explicit values " + str(values))
    for args in (("prob32", 0, 1, 10000, True), ("prob64", 0, 1, 1000),
                 ("distance", 0, 6400, 6400), ("angle", -math.pi * 2, math.pi * 2, 4096),
                 ("trig", -1, 1, 4096), ("slope", -6400, 6400, 4096),
                 ("accum", 0, 1000000000, 4096), ("fourth", 0, 6400**4, 4096),
                 ("weighted", 0, 1000000000000, 4096), ("mod", 1 / 16000, 10, 4096)):
        domain = grid(*args)
        result[domain.name] = domain
    # Integer conversion boundaries are where otherwise tiny errors matter.
    values = set(result["distance"].values)
    for n in range(6401):
        for bias in (0.9999, 0.99999, 0.999, 0.5):
            value = n - bias
            values.update(v for v in (math.nextafter(value, -math.inf), value,
                                     math.nextafter(value, math.inf)) if 0 <= v <= 6400)
        for dy in (1, 10, 100, 1000, 6400):
            value = math.sqrt(n * n + dy * dy)
            if value <= 6400:
                values.add(value)
    result["distance"].values = tuple(sorted(values))
    result["distance"].description += "; rounding thresholds n-bias and IEEE neighbors for biases .9999/.99999/.999/.5; integer-coordinate Euclidean distances"
    # Actual defense survival values supplement the synthetic probability grid.
    values = set(result["prob32"].values)
    for ability in result["ability"].values:
        for count in range(101):
            for divisor in (1000, 2000):
                values.update(neighbors32(math.pow(1 - ability / divisor, count)))
    result["prob32"].values = tuple(sorted(values))
    result["prob32"].description += "; plus defense survival powers from data/parts/planetary.yaml, 0..100 defenses, two divisors"
    return result


def mask_c(source):
    """Mask comments/literals without changing offsets used in source links."""
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: re.sub(r"[^\n]", " ", m.group()), source, flags=re.S)


def pairs(source):
    stack = []
    result = []
    for index, char in enumerate(source):
        if char == "(":
            stack.append(index)
        elif char == ")":
            if not stack:
                raise ValueError("Unbalanced source parentheses")
            result.append((stack.pop(), index + 1))
    if stack:
        raise ValueError("Unbalanced source parentheses")
    return result


def expression_at(source, spans, cast):
    containers = sorted((a, b) for a, b in spans if a <= cast.start() and b >= cast.end())
    a, b = containers[0]
    preceding = re.search(r"(\w+)\s*$", source[:a])
    name = preceding.group(1) if preceding else None
    if name and name not in TRANSPARENT:
        if name != "if":
            depth = 0
            starts = [a + 1]
            for index in range(a + 1, b - 1):
                if source[index] in "([":
                    depth += 1
                elif source[index] in ")]":
                    depth -= 1
                elif source[index] == "," and depth == 0:
                    starts.append(index + 1)
            ends = [start - 1 for start in starts[1:]] + [b - 1]
            a, b = next((u, v) for u, v in zip(starts, ends) if u <= cast.start() < v)
        else:
            a, b = a + 1, b - 1
    elif name:
        a = preceding.start(1)
    outer = re.search(r"\((int16_t|int32_t|double|float)\)\s*$", source[:a])
    if outer:
        a = outer.start()
    return a, b


def functions(source):
    """Top-level function bodies, including multiline signatures."""
    depth = 0
    start = 0
    header = ""
    result = []
    for index, char in enumerate(source):
        if char == "{":
            if depth == 0:
                before = source[:index]
                match = re.search(r"^([A-Za-z_]\w*[^;{}]*?\b(\w+)\s*\([^;{}]*\))\s*\Z", before, re.M)
                if match:
                    start, header = match.start(), match.group(1)
                    name = match.group(2)
                else:
                    header = ""
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0 and header:
                result.append((start, index + 1, name, header))
    return result


def extract(root=ROOT):
    result = []
    cCasts = 0
    digest = hashlib.sha256()
    directory = root / "src" if (root / "src").is_dir() else root
    paths = sorted(p for p in directory.rglob("*") if p.suffix in (".c", ".cpp"))
    historical = precision_reference.source_names() if root == ROOT else None
    for path in paths:
        if historical is not None and path.with_suffix(".c").name not in historical:
            continue
        source = precision_reference.read_source(path.name) if root == ROOT else path.read_text()
        masked = mask_c(source)
        casts = list(re.finditer(r"\(long\s+double\)", masked))
        if not casts:
            continue
        digest.update(path.name.encode() + b"\0" + source.encode())
        cCasts += len(casts)
        spans = pairs(masked)
        funcs = functions(masked)
        groups = {}
        for cast in casts:
            a, b = expression_at(masked, spans, cast)
            fstart, fend, name, header = next(f for f in funcs if f[0] <= a < f[1])
            expr = re.sub(r"\s+", " ", source[a:b]).strip()
            key = (name, expr)
            if key not in groups:
                types = {name: ctype for ctype, name in re.findall(
                    r"\b(" + TYPES + r")\s+(\w+)\s*(?:[;,)=\[])", masked[fstart:fend])}
                groups[key] = {"file": path.name, "function": name, "expression": expr,
                               "lines": [], "casts": 0, "types": types}
                assignment = re.search(r"\b(\w+)\s*=\s*$", masked[fstart:a])
                if assignment and assignment[1] in types:
                    groups[key]["destination_type"] = types[assignment[1]]
            groups[key]["lines"].append(source.count("\n", 0, cast.start()) + 1)
            groups[key]["casts"] += 1
        result.extend(groups.values())
    if sum(case["casts"] for case in result) != cCasts:
        raise ValueError("Unaccounted extended-precision cast")
    return result, cCasts, digest.hexdigest()


# Member reads and pure function results become typed operands. Their producers
# are outside this expression test; the full race test separately checks chains.
BINDINGS = (
    (r"DGetDistance\([^)]*\)", "double", "distance"),
    (r"LDistance2\([^)]*\)", "int32_t", "square"),
    (r"RawLoad32\(plMinMax\)", "int32_t", "count"),
    (r"lpThings\[[^]]*\]\.thm\.cMines|lpth->thm\.cMines", "int32_t", "mines"),
    (r"lppl(?:Dst|Test)->uDefGuess", "uint16_t", "defguess"),
    (r"lppl(?:Dst|Test)->uPopGuess", "uint16_t", "popguess"),
    (r"pdc->rgrad\[i\]", "int16_t", "radius"),
    (r"part\.pplanetary->grAbility", "int16_t", "ability"),
    (r"part\.pbomb->dDmgCol", "int16_t", "bomb"),
    (r"lphs->cItem", "uint16_t", "slots"),
    (r"lpcdLook->cColonist", "int32_t", "count"),
    (r"rgpflNew\[i\]->rgwtMin\[[^]]*\]|lpfl->rgwtMin\[[^]]*\]", "int32_t", "count"),
    (r"rg(?:Fuel|Cargo)(?:CapLoss|Capacity)\[i\]", "int32_t", "count"),
    (r"rgInc\[[01]\]", "int16_t", "inc"),
    (r"\bpt(?:Dst|Src|End|Beg|C|L1)\.[xy]\b", "int16_t", "coord"),
    (r"\*pParams\b", "int16_t", "i16"),
)


def domain_for(name, ctype, function):
    if name.startswith("f"):
        return "bool"
    if name == "iSize":
        return "size"
    if name == "pctPos":
        return "root"
    if name.startswith("iWarp"):
        return "warp"
    if name in ("dxRound", "dyRound"):
        return "round"
    if name in ("d", "dbl", "dDistance", "dDistanceTgt"):
        return "distance"
    if name == "r":
        return "prob64"
    if name in ("pct", "pct1", "pct2", "pctSurvive", "pctSuccess", "pctSmart", "pctSuccessHalf", "dmgSmart", "dmgT"):
        return "prob32" if ctype == "float" else "prob64"
    if name in ("angle", "dAngle"):
        return "angle"
    if name in ("dcos", "dsin"):
        return "trig"
    if name == "m":
        return "slope"
    if name == "dMod":
        return "mod"
    if name == "iEnergy":
        return "energy"
    if name == "iEff":
        return "eff"
    if name == "pctVal":
        return "hab"
    if name == "cDefenses":
        return "defenses"
    if name in ("dx", "dy", "dxdy"):
        return "delta" if name != "dxdy" else "signed_square"
    if name in ("r2", "dx2", "dy2", "lWarp2") or (name == "l" and function == "DrawRadarCircle"):
        return "square"
    if name == "lPower":
        return "power"
    if name == "l" and function in ("DrawVCReport", "DrawScoreReport"):
        return "u16"
    if name in ("dxText", "dyText", "rad", "dRange", "dTravel"):
        return "radius"
    if ctype in ("double", "float"):
        if name in ("lBIR4", "lBIPR4", "lRange4", "lPlanRange4", "lT"):
            return "fourth"
        if name in ("dcPts", "dwtFleet"):
            return "weighted"
        return "accum"
    if name.startswith("dmg") or name in ("dpMain", "dp", "dpT"):
        return "large"
    return "count"


def bind(case):
    expr = case["expression"]
    inputs = []
    replacements = {}
    for pattern, ctype, domain in BINDINGS:
        for match in list(re.finditer(pattern, expr)):
            text = match.group()
            if text not in replacements:
                replacements[text] = f"v{len(inputs)}"
                inputs.append({"source": text, "type": ctype, "domain": domain})
        for text in sorted(replacements, key=len, reverse=True):
            expr = expr.replace(text, replacements[text])
    known = set(TRANSPARENT) | {"long", "double", "float", "int16_t", "int32_t", "uint16_t", "uint32_t"}
    known.update(f"v{i}" for i in range(len(inputs)))
    # Avoid exponent/hex suffixes when identifying free scalar operands.
    tokens = re.sub(r"\b\d+(?:\.\d*)?(?:[eE][+-]?\d+)?", " ", expr)
    names = list(dict.fromkeys(re.findall(r"\b[A-Za-z_]\w*\b", tokens)))
    for name in names:
        if name in known:
            continue
        if name not in case["types"]:
            raise ValueError(f"No scalar binding for {case['file']}:{case['lines'][0]}: {name}: {expr}")
        ctype = case["types"][name]
        inputs.append({"source": name, "type": ctype, "domain": domain_for(name, ctype, case["function"])})
        expr = re.sub(r"\b" + re.escape(name) + r"\b", f"v{len(inputs)-1}", expr)
    if len(inputs) > 12:
        raise ValueError("Too many expression operands")
    if "->" in expr or "[" in expr:
        raise ValueError(f"Unbound member/array read: {expr}")
    prefixes = {"int16_t": "i", "int32_t": "l", "uint16_t": "w", "uint32_t": "dw", "float": "pct", "double": "d"}
    for i, value in enumerate(inputs):
        prefix = "f" if value["domain"] == "bool" else prefixes[value["type"]]
        value["name"] = f"{prefix}Input{i}"
        expr = re.sub(r"\bv" + str(i) + r"\b", value["name"], expr)
    case["bound_expression"] = expr
    case["inputs"] = inputs
    case["lines"] = sorted(set(case["lines"]))
    case.pop("types")
    case["result"] = "float" if expr.startswith("(float)") else "double" if expr.startswith("(double)") else "integer"
    destination = case.get("destination_type")
    if destination:
        case["bound_expression"] = f"({destination})({expr})"
        case["result"] = destination if destination in ("float", "double") else "integer"
    return case


def raw_output(expr):
    while True:
        cast = re.match(r"^\((int16_t|int32_t|float|double)\)", expr)
        if not cast:
            break
        expr = expr[cast.end():].strip()
        if expr.startswith("(") and pairs(expr)[-1] == (0, len(expr)):
            expr = expr[1:-1].strip()
    if expr.startswith("LOWORD("):
        expr = expr[7:-1].strip()
    if expr.startswith("(int32_t)"):
        return expr[len("(int32_t)"):]
    return expr


def generate(cases, rgDomains, samples, limit, seed):
    code = [f'#include "{HERE / "precision_probe.c"}"']
    for name, domain in rgDomains.items():
        if domain.float_bits:
            code.append(f"static double D_{name}(uint64_t i) {{ uint32_t dw = (uint32_t)i; float pct; memcpy(&pct, &dw, sizeof(pct)); return pct; }}")
        elif domain.values:
            code.append(f"static const double rg_{name}[] = {{" + ",".join(float(v).hex() for v in domain.values) + "};")
            code.append(f"static double D_{name}(uint64_t i) {{ return rg_{name}[i]; }}")
        else:
            code.append(f"static double D_{name}(uint64_t i) {{ return {domain.low}.0 + (double)i * {domain.step}.0; }}")
    for iCase, case in enumerate(cases):
        expr = case["bound_expression"]
        inputs = case["inputs"]
        declarations = "\n".join(f"    {v['type']} {v['name']} = ({v['type']})rg[{i}];" for i, v in enumerate(inputs))
        guard = []
        for name in re.findall(r"/\s*(" + INPUT_NAME + r")\b", expr):
            guard.append(f"{name} != 0")
        if "pow(" in expr:
            # Inverse attenuation can exceed int32; keep a bounded, safe ratio.
            for match in re.finditer("(" + INPUT_NAME + ") / (" + INPUT_NAME + ")", expr):
                guard.append(f"{match[1]} / {match[2]} <= 8.0")
        raw = raw_output(expr) if case["result"] == "integer" and "int32_t" in expr else None
        code.append(f"static int F_{iCase}(const double *rg) {{\n{declarations}")
        if guard:
            code.append("    if (!(" + " && ".join(sorted(set(guard))) + ")) return 0;")
        if raw:
            code.append(f"    long double dExt = {raw};\n    long double dDbl = {raw.replace('long double', 'double')};")
            code.append("    if (!isfinite(dExt) || !isfinite(dDbl) || dExt < INT32_MIN || dExt > INT32_MAX || dDbl < INT32_MIN || dDbl > INT32_MAX) return 0;")
        code.append("    return 1;\n}")
        for mode in ("ext", "dbl"):
            candidate = expr if mode == "ext" else expr.replace("long double", "double")
            code.append(f"static __attribute__((noinline)) double D_{mode}_{iCase}(const double *rg) {{\n{declarations}\n    return {candidate};\n}}")
        product = math.prod(rgDomains[v["domain"]].count for v in inputs)
        exhaustive = limit == 0 or product <= limit
        case["domain_combinations"] = product
        case["coverage"] = "exhaustive operand grid" if exhaustive else "boundary Cartesian grid plus deterministic operand-grid samples"
        if exhaustive and product >= 2**64:
            raise ValueError("Exhaustive product exceeds uint64; restrict the function and operand domains")
        axes = [range(rgDomains[v["domain"]].count) if exhaustive else rgDomains[v["domain"]].edges() for v in inputs]
        if not exhaustive:
            while math.prod(map(len, axes)) > 250000:
                axis = max(axes, key=len)
                axis.pop(len(axis) // 2)
        # Exhaustive mixed-radix enumeration avoids embedding large products.
        counts = [len(axis) for axis in axes]
        nGrid = math.prod(counts)
        if not exhaustive:
            for i, axis in enumerate(axes):
                code.append(f"static const uint64_t rgIndex_{iCase}_{i}[] = {{" + ",".join(map(str, axis)) + "};")
        code.append(f"static void Run_{iCase}(void) {{\n    PRECISION_STATS ps = {{0}};\n    double rg[MAX_INPUTS];\n    double dExt, dDbl;\n    uint64_t i, q;")
        for i, v in enumerate(inputs):
            code.append(f"    uint64_t i{i};")
        code.append(f"    qwSeed = UINT64_C({seed}) ^ UINT64_C({iCase + 1});")
        code.append(f"    for (i = 0; i < UINT64_C({nGrid + (0 if exhaustive else samples)}); i++) {{\n        q = i;")
        for j, v in enumerate(inputs):
            domain = rgDomains[v["domain"]]
            if exhaustive:
                code.append(f"        i{j} = q % UINT64_C({domain.count}); q /= UINT64_C({domain.count});")
            else:
                code.append(f"        if (i < UINT64_C({nGrid})) {{ i{j} = rgIndex_{iCase}_{j}[q % {counts[j]}]; q /= {counts[j]}; }}\n"
                            f"        else i{j} = QwRand() % UINT64_C({domain.count});")
            code.append(f"        rg[{j}] = ({v['type']})D_{domain.name}(i{j});")
        grResult = {"integer": 0, "float": 1, "double": 2}[case["result"]]
        code.append(f"        if (!F_{iCase}(rg)) {{ ps.cSkipped++; continue; }}\n"
                    f"        dExt = D_ext_{iCase}(rg); dDbl = D_dbl_{iCase}(rg);\n"
                    "        if (!isfinite(dExt) || !isfinite(dDbl)) { ps.cSkipped++; continue; }\n"
                    f"        Record(&ps, dExt, dDbl, rg, {len(inputs)}, {grResult});\n    }}\n"
                    f"    WriteStats({iCase}, &ps, {len(inputs)}, {int(exhaustive)});\n}}")
    code.append("int main(void) {\n    if (!FCheckPrecision()) return 2;")
    code.extend(f"    Run_{i}();" for i in range(len(cases)))
    code.append("    return 0;\n}")
    return "\n".join(code) + "\n"


def run_race(args, flags, work):
    import race_precision

    for name in ("textids.h", "dataids.h"):
        if not (args.generated_dir / name).is_file():
            raise ValueError("Generate host headers first with make host, or pass --generated-dir")
    cIntervals = 4950 if args.odd_widths else 1681
    cHabitats = (cIntervals + 1)**3 * 2
    if args.habitat_start < 0 or args.habitat_count < 0 or args.habitat_start + args.habitat_count > cHabitats:
        raise ValueError(f"Habitat shard must lie within 0..{cHabitats - 1}")
    source = race_precision.sources()
    for mode in ("ext", "dbl"):
        (work / f"{mode}.c").write_text(race_precision.variant(source, mode))
    command = [args.cc, *flags, "-iquote", str(ROOT / "src/core"), "-iquote", str(ROOT / "src/platform"), "-iquote", str(args.generated_dir.resolve()),
               str(work / "ext.c"), str(work / "dbl.c"), str(HERE / "precision_race.c"),
               "-lm", "-o", str(work / "race")]
    print("Whole race calculator: compiling extracted variants", flush=True)
    subprocess.run(command, check=True)
    command = [str(work / "race"), str(args.race_samples), str(int(args.odd_widths)),
               str(args.habitat_start), str(args.habitat_count), str(args.seed), str(int(not args.no_axis_sweep))]
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    records = [json.loads(line) for line in result.stdout.splitlines()]
    names = ("stored habitability accumulator", "integer innate habitability", "habitability / 2000",
             "returned advantage points", "normalized PLAYER byte differences")
    for record, name in zip(records[1:], names):
        record["name"] = name
        print(f"Race {name}: {record['different']}/{record['evaluated']} differ; max={record['max_abs']:.9g}", flush=True)
    if len(records) != 6:
        raise ValueError("Missing whole-race statistics")
    return {"source_sha256": hashlib.sha256(source.encode()).hexdigest(), "environment": records[0]["environment"],
            "intervals_per_axis": cIntervals, "complete_habitat_domain": cHabitats,
            "axis_sweep": not args.no_axis_sweep, "odd_widths": args.odd_widths,
            "habitat_start": args.habitat_start, "habitat_count": args.habitat_count,
            "random_samples": args.race_samples, "golden_witness_checked": True,
            "witness_input_order": [f"axis{i}.{field}" for i in range(3) for field in ("min", "center", "max")] +
                                   ["growth", "trait_bits"] + [f"rgAttr[{i}]" for i in range(16)],
            "metrics": records[1:]}


def write_report(path, report):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n")
    rows = ["# Extended intermediates versus double", "", "This report measures the current extracted source. "
            "Exhaustive means the stated operand grid after validity filters, not all reachable game states.", "",
            f"Source SHA-256: `{report['source_sha256']}`", "",
            "| Source / function | Evaluated | Different | Maximum absolute difference | Maximum ULPs | Coverage |",
            "| --- | ---: | ---: | ---: | ---: | --- |"]
    for case in report["cases"]:
        rows.append(f"| {case['file']}:{','.join(map(str,case['lines']))} `{case['function']}` | "
                    f"{case['evaluated']:,} | {case['different']:,} | {case['max_abs']:.9g} | {case['max_ulps']:,} | "
                    f"{'grid exhaustive' if case['exhaustive_domain'] else 'sampled'} |")
    if "race" in report:
        rows.extend(["", "Whole race calculator (axis sweeps and samples; complete habitats only if every shard is run):", "",
                     "| Output | Evaluated | Different | Maximum absolute difference | Maximum ULPs |",
                     "| --- | ---: | ---: | ---: | ---: |"])
        for metric in report["race"]["metrics"]:
            rows.append(f"| {metric['name']} | {metric['evaluated']:,} | {metric['different']:,} | {metric['max_abs']:.9g} | {metric['max_ulps']:,} |")
    rows.extend(["", "Full domains, expressions, skipped-input counts, and first/largest counterexamples are in the JSON report.", ""])
    path.with_suffix(".md").write_text("\n".join(rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--optimization", choices=("O0", "O2", "O3"), default="O2")
    parser.add_argument("--samples", type=int, default=100000, help="Samples per expression above the exhaustive limit")
    parser.add_argument("--exhaustive-limit", type=int, default=250000, help="Maximum Cartesian grid size; 0 exhausts every declared grid")
    parser.add_argument("--seed", type=lambda s: int(s, 0), default=0x123456789abcdef)
    parser.add_argument("--function", action="append", help="Restrict to a named production function (repeatable)")
    parser.add_argument("--domain", action="append", default=[], help="Override an integer domain: NAME=LOW:HIGH[:STEP]")
    parser.add_argument("--all-float-probabilities", action="store_true", help="Use every binary32 encoding in [0,1] as the prob32 domain (1,065,353,217 values)")
    parser.add_argument("--skip-race", action="store_true", help="Skip whole calculator tests (isolated race expressions still run)")
    parser.add_argument("--race-samples", type=int, default=10000)
    parser.add_argument("--no-axis-sweep", action="store_true", help="Skip the 40,344-case one-axis race sweep")
    parser.add_argument("--odd-widths", action="store_true", help="Race sweeps include every integer width 2..100, as loaded files can")
    parser.add_argument("--habitat-start", type=int, default=0, help="First full Cartesian race habitat index")
    parser.add_argument("--habitat-count", type=int, default=0, help="Number of Cartesian habitats in this shard; 0 skips Cartesian enumeration")
    parser.add_argument("--generated-dir", type=Path, default=ROOT / "dist/host-release/generated")
    parser.add_argument("--sanitize", action="store_true", help="Check undefined arithmetic and float-to-integer overflow")
    parser.add_argument("--fail-on-difference", action="store_true", help="Exit 1 if any sampled output differs")
    parser.add_argument("--report", type=Path, default=ROOT / "dist/scaffold/precision/report.json")
    args = parser.parse_args()
    if not 0 <= args.samples <= 100000000 or not 0 <= args.race_samples <= 100000000 or args.exhaustive_limit < 0 or not 0 < args.seed < 2**64:
        parser.error("Invalid samples, exhaustive limit, or seed")
    cases, cCasts, sha = extract()
    cases = [bind(case) for case in cases]
    if args.function:
        known = {case["function"] for case in cases}
        if not set(args.function) <= known:
            parser.error("Unknown functions: " + str(set(args.function) - known))
        cases = [case for case in cases if case["function"] in args.function]
    rgDomains = domains()
    if args.all_float_probabilities:
        rgDomains["prob32"] = Domain("prob32", 0, 0x3f800000, description="every nonnegative binary32 encoding from +0 through 1", float_bits=True)
    for override in args.domain:
        try:
            name, bounds = override.split("=", 1)
            fields = list(map(int, bounds.split(":")))
            low, high = fields[:2]
            step = fields[2] if len(fields) == 3 else 1
            if name not in rgDomains or len(fields) not in (2, 3) or low > high or step <= 0:
                raise ValueError()
            rgDomains[name] = Domain(name, low, high, step, description=f"overridden integer grid {low}..{high}, step {step}")
        except ValueError:
            parser.error("Invalid --domain: " + override)
    used = {v["domain"] for case in cases for v in case["inputs"]}
    rgDomains = {name: value for name, value in rgDomains.items() if name in used}
    compiler = subprocess.check_output([args.cc, "--version"], text=True).splitlines()[0]
    flags = ["-" + args.optimization, "-std=gnu11", "-Wall", "-Wextra", "-Wno-unused-variable",
             "-Wno-unused-function", "-fsigned-char", "-fno-fast-math", "-ffp-contract=off"]
    if args.sanitize:
        flags += ["-fsanitize=undefined,float-cast-overflow", "-fno-sanitize-recover=all"]
    print(f"{len(cases)} expressions, {sum(c['casts'] for c in cases)} of {cCasts} production casts; {compiler}", flush=True)
    report = {"reference_commit": precision_reference.REF, "source_sha256": sha, "compiler": compiler, "flags": flags, "seed": args.seed,
              "harness_sha256": {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                                 for name in ("precision.py", "precision_probe.c", "precision_race.c", "race_precision.py")},
              "runtime": {"platform": platform.platform(), "libc": platform.libc_ver()},
              "data_sha256": {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                              for name in ("data/parts/planetary.yaml", "data/parts/bombs.yaml")},
              "samples_per_large_domain": args.samples, "exhaustive_limit": args.exhaustive_limit,
              "production_casts": cCasts, "covered_casts": sum(c["casts"] for c in cases),
              "domains": {name: {"description": d.description, "count": d.count} for name, d in rgDomains.items()},
              "limitations": ["Operand grids are stated bounds, not proofs of reachability or exhaustive IEEE coverage.",
                              "Member reads and pure function results are independent operands; expression tests do not propagate upstream changes.",
                              "Invalid divisors, inverse-attenuation ratios above eight, nonfinite outputs, and out-of-int32 results are skipped and counted."],
              "cases": cases}
    with tempfile.TemporaryDirectory(prefix="stars-precision-") as directory:
        work = Path(directory)
        source = work / "probe.c"
        source.write_text(generate(cases, rgDomains, args.samples, args.exhaustive_limit, args.seed))
        subprocess.run([args.cc, *flags, str(source), "-lm", "-o", str(work / "probe")], check=True)
        proc = subprocess.Popen([str(work / "probe")], stdout=subprocess.PIPE, text=True)
        for line in proc.stdout:
            value = json.loads(line)
            if "environment" in value:
                report.update(value)
            else:
                case = cases[value["case"]]
                case.update(value)
                print(f"{case['file']}:{case['lines'][0]} {case['function']}: "
                      f"{case['different']}/{case['evaluated']} differ, max={case['max_abs']:.9g}, "
                      f"ULPs={case['max_ulps']}, skipped={case['skipped']}", flush=True)
        if proc.wait() != 0:
            raise RuntimeError("Precision probe failed; no successful report written")
        if not args.skip_race:
            report["race"] = run_race(args, flags, work)
    write_report(args.report.resolve(), report)
    print(f"Reports: {args.report.resolve()} and {args.report.resolve().with_suffix('.md')}")
    if args.fail_on_difference and (any(case["different"] for case in cases) or
                                   any(m["different"] for m in report.get("race", {}).get("metrics", []))):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
