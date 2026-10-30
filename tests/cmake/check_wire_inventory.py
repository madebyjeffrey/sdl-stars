"""Emit a conservative, source-located inventory; never infer wire layouts from C++.

The report includes signatures as well as calls. Casts are candidates, not a
claim that every pointer denotes serialized bytes. See CPP-WIRE-INVENTORY.md.
"""
from collections import Counter
from pathlib import Path
import argparse
import json
import re

parser = argparse.ArgumentParser()
parser.add_argument("root", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
root = args.root.resolve()

# Preserve character offsets/newlines while excluding comments and literals.
def mask(text):
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: re.sub(r"[^\n]", " ", m[0]), text, flags=re.S)

def close(text, start, opening="(", closing=")"):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == opening:
            depth += 1
        elif text[i] == closing:
            depth -= 1
            if depth == 0:
                return i + 1
    raise AssertionError(f"Unbalanced {opening} at {start}")

# New record types must get an explicit family disposition, even when their
# read/write implementation is still covered by the conservative byte audit.
record_phase = {
    "rtEOF": "B11", "rtPlr": "B11", "rtGame": "B11", "rtBOF": "B11",
    "rtLogCargoXfer8": "B10", "rtLogCargoXfer16": "B10", "rtLogCargoXfer32": "B10",
    "rtLogFleetOrderDelete": "B06", "rtLogFleetOrderInsert": "B06", "rtLogFleetOrderUpdate": "B06",
    "rtLogFleetFlagBit9": "B08", "rtLogFleetOrderAttrNib": "B06", "rtMsg": "B10",
    "rtPlanet": "B11", "rtPlanetB": "B11", "rtFleetA": "B08", "rtFleetB": "B08",
    "rtOrderA": "B06", "rtOrderB": "B06", "rtString": "B10", "rtSel": "B11",
    "rtLogFleetCargoXfer": "B08", "rtLogFleetSplit": "B08", "rtShDef": "B07", "rtLogShDef": "B07",
    "rtProdQ": "B05", "rtLogPlanetProdQ": "B05", "rtBtlPlan": "B09", "rtBtlData": "B09",
    "rtContinue": "B09", "rtHistHdr": "B04", "rtMsgFilt": "B10", "rtLogResearch": "B10",
    "rtLogPlanetRouting": "B11", "rtLogRelations": "B11", "rtChgPassword": "B10",
    "rtLogFleetMerge": "B08", "rtPlrMsg": "B10", "rtAiData": "B09", "rtThing": "B11",
    "rtLogFleetPlan": "B08", "rtLogThingByteParam": "B11", "rtLogFleetName": "B10",
    "rtScore": "B11", "rtLogPlayerZpq1": "B11", "rtMax": "sentinel",
}
tails = {
    "BTLREC": ("rgkill", "B09", "6 + ctok * 8", "battle/replay"),
    "BTLREC26": ("rgkill", "B09", "6 + ctok * 8", "legacy battle/replay"),
    "MSGPLR": ("rgbMsg", "B10", "offsetof(MSGPLR,rgbMsg) + abs(cLen)", "runtime; RTPLRMSG is the wire counterpart"),
    "PL": ("rgb", "B12", "4 + iMax * cbItem", "runtime pool; iMac live items"),
    "PLPROD": ("rgprod", "B05", "4 + iprodMax * 4", "runtime pool; rtProdQ wire prefix is zero"),
    "RTCHGPRODQ": ("rgprod", "B05", "2 + count * 4", "wire; count derives from payload"),
    "RTSHDEF": ("rghs", "B07", "17 + chs * 4", "wire ship design"),
    "RTFLEET": ("rgb", "B08", "14 + conditional payload", "wire fleet"),
    "RTFLEETIDS": ("rgid", "B08", "0 + count * 2", "wire; C++ sizeof is not its prefix"),
    "RTPLRMSG": ("rgbMsg", "B10", "12 + abs(cLen)", "wire; pointer marker is zero"),
    "PLORD": ("rgord", "B06", "4 + iordMax * sizeof(ORDER)", "runtime; wire orders use 18/8 bytes"),
    "BTLDATA": ("rgtok", "B09", "14 + ctok * 29 + battle events", "wire; cbData bounds complete data"),
}
migrated_tails = {"PLPROD", "RTCHGPRODQ", "PLORD", "RTSHDEF", "RTFLEET", "RTFLEETIDS"}
report = {"schema": 1, "contract": "spec/S01-game-file-records.md", "records": [], "tails": [], "zero_storage": [], "sites": []}
paths = sorted(p for d in ("src", "compat/win32/src", "compat/win32/include")
               for p in (root / d).rglob("*") if p.suffix in (".h", ".c", ".cpp", ".mm"))
# Include every generic byte-copy and size/offset expression, rather than trying
# to guess which ones are important from names such as rgbCur or pbIn.
operations = r"ReadRt|WriteRt|WriteMemRt|FGetPrevLogRt|XorFileBuf|RgFromStream|RgToStream|CbReadFile|CbWriteFile|CbReadNative|CWriteNative|RawLoad16|RawLoad32|RawStore16|RawStore32|FindResourceA|LoadResource|LockResource|SizeofResource|memcpy|memmove|memset|memcmp|sizeof|offsetof"
native_io = (root / "src/platform/fileio.h").read_text(encoding="utf-8")
operations += "|" + "|".join(sorted(set(re.findall(r"(?m)^[A-Za-z_]\w*\s+\**([A-Za-z_]\w*)\s*\(", mask(native_io)))))
operations += r"|read|write|_read|_write|fread|fwrite|_lread|_lwrite|ReadFile|WriteFile"
for path in paths:
    name = path.relative_to(root).as_posix()
    original = path.read_text(encoding="utf-8")
    text = mask(original)
    def site(kind, start, end, **extra):
        report["sites"].append(dict(path=name, line=original.count("\n", 0, start) + 1,
                                   kind=kind, text=" ".join(original[start:end].split()), **extra))
    # Definitions, signatures and calls deliberately all appear in the output.
    for match in re.finditer(r"\b(" + operations + r")\s*\(", text):
        opening = text.index("(", match.start())
        site("byte-operation", match.start(), close(text, opening), operation=match[1])
    for match in re.finditer(r"\(\s*(?:(?:const|volatile|struct|union)\s+)*[A-Za-z_]\w*(?:::\w+)*(?:\s+const)?\s*\*+(?:\s*const)?\s*\)", text):
        site("pointer-cast-candidate", match.start(), match.end())
    for match in re.finditer(r"\b(?:reinterpret_cast|static_cast|const_cast)\s*<[^;{}<>]*\*[^;{}<>]*>", text):
        site("pointer-cast-candidate", match.start(), match.end())
    # Typedef pointer aliases (LPVOID etc.) are also candidates, not just '*'.
    for match in re.finditer(r"\(\s*(?:LP[A-Z]\w*|PVOID|PBYTE|PWORD|PDWORD)\s*\)", text):
        site("pointer-cast-candidate", match.start(), match.end())
    for match in re.finditer(r"^\s*#\s*pragma\s+pack[^\n]*|\balignas\s*\([^)]*\)|__attribute__\s*\(\([^;\n]*", text, re.M):
        site("packing-or-alignment", match.start(), match.end())
    # Include multi-declarator bitfield groups; not only the final field on a line.
    for match in re.finditer(r"\b[A-Za-z_]\w*\s*:\s*\d+\b", text):
        site("bitfield", match.start(), match.end())
    for match in re.finditer(r"^.*(?:<<|>>|(?<![&])&(?![&])|(?<![|])\|(?![|])).*$", text, re.M):
        site("bit-operation-candidate", match.start(), match.end())
    for match in re.finditer(r"\bsizeof\s+(?!\s*\()\w+", text):
        site("byte-operation", match.start(), match.end(), operation="sizeof")
    for match in re.finditer(r"\b(?:struct|union)\s*\{", text):
        end = close(text, text.index("{", match.start()), "{", "}")
        site("anonymous-aggregate", match.start(), end)
    for match in re.finditer(r"(?m)^\s*(?:extern\s+)?(?!return\b)[A-Za-z_]\w*\s+([A-Za-z_]\w*)\s*\[\s*0\s*\]\s*;", text):
        # All owned zero-tail declarations currently live in structs.h. A new
        # declaration anywhere else fails until its lifetime/width is reviewed.
        if name in ("src/ui/globalsui.c", "src/ui/globalsui.h") and match[1] == "_ctype":
            report["zero_storage"].append(dict(path=name, name="_ctype", checkpoint="B14", role="unused reconstruction symbol; no consumers"))
            site("zero-storage", match.start(), match.end(), checkpoint="B14")
            continue
        assert name == "src/core/structs.h", f"Uninventoried tail: {name}:{match[0]}"
        definitions = list(re.finditer(r"\bstruct\s+(\w+)\s*\{", text[:match.start()]))
        assert definitions, f"Tail outside named type: {name}:{match[0]}"
        tag = definitions[-1][1]
        aliases = re.findall(r"typedef\s+struct\s+" + re.escape(tag) + r"\s+(\w+)\s*;", text)
        owner = aliases[0] if aliases else tag
        member, phase, length, role = tails[owner]
        assert match[1] == member, f"Unknown tail member: {owner}:{match[0]}"
        report["tails"].append(dict(type=owner, member=member, checkpoint=phase, length=length, role=role))
        site("zero-tail", match.start(), match.end(), type=owner, checkpoint=phase)
    if name == "src/core/enums.h":
        start = text.index("enum RecordType")
        body = text[text.index("{", start):close(text, text.index("{", start), "{", "}")]
        for match in re.finditer(r"\b(rt\w+)\s*=\s*(\d+)", body):
            assert match[1] in record_phase, f"Uninventoried record type: {match[1]}"
            report["records"].append(dict(name=match[1], value=int(match[2]), checkpoint=record_phase[match[1]]))
assert {row["type"] for row in report["tails"]} == set(tails) - migrated_tails, "Tail inventory changed: update the migration disposition"
assert {row["name"] for row in report["records"]} == set(record_phase), "Record inventory changed"
# Independent historical sizes/offsets remain part of the report. These are
# expected C reference measurements, not new expectations derived from codecs.
layout = root / "tests/compat/record_layout.inc"
report["frozen_measurements"] = [" ".join(line.split()) for line in layout.read_text().splitlines() if "STARS_" in line or "printf(" in line]
report["migrated_tails"] = [dict(type=name, member=tails[name][0], checkpoint=tails[name][1], length=tails[name][2], role=tails[name][3]) for name in sorted(migrated_tails)]
report["counts"] = dict(sorted(Counter(row["kind"] for row in report["sites"]).items()))
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(f"Wire inventory: {len(paths)} files; {len(report['records'])} record names; {len(tails) - len(migrated_tails)} active tails / {len(migrated_tails)} migrated; {len(report['frozen_measurements'])} frozen measurements")
print("; ".join(f"{kind}: {count}" for kind, count in report["counts"].items()))
print(f"Full source-located report: {args.output}")
