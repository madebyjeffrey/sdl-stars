#!/usr/bin/env python3
"""Compare the race calculator with an experimental binary64-only copy.

Extracts the current functions, preserving their types and operation order.
Neither the game nor its baseline is modified. This is a diagnostic, not a
proof of equivalence: zero displayed-point differences do not establish parity.
"""

import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

import precision_reference


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "common.h"
#include <float.h>
#include <fenv.h>
#include <stdlib.h>

PLAYER rgplr[16];
int32_t ext_LInnateRaceHabitability(PLAYER *);
int32_t dbl_LInnateRaceHabitability(PLAYER *);
int16_t ext_CAdvantagePoints(PLAYER *);
int16_t dbl_CAdvantagePoints(PLAYER *);
extern double ext_dInnate, dbl_dInnate;

static uint32_t dwSeed = 0x12345678;
static uint32_t DwRand(void) {
    dwSeed ^= dwSeed << 13;
    dwSeed ^= dwSeed >> 17;
    dwSeed ^= dwSeed << 5;
    return dwSeed;
}

static void ShowRace(PLAYER *pplr) {
    int i;

    printf(" growth=%d flags=%x env=", pplr->pctIdealGrowth, (unsigned)pplr->grbitAttr);
    for (i = 0; i < 3; i++)
        printf(" %d:%d:%d", pplr->rgEnvVarMin[i], pplr->rgEnvVar[i], pplr->rgEnvVarMax[i]);
    printf(" attrs=");
    for (i = 0; i < 16; i++)
        printf(" %d", pplr->rgAttr[i]);
    puts("");
}

int main(int argc, char **argv) {
    PLAYER plr, plrA, plrB;
    int8_t rgMin[3] = {31, 61, 9};
    int8_t rgMax[3] = {71, 97, 73};
    int8_t rgAttr[16] = {10, 10, 10, 10, 10, 5, 10, 0, 1, 1, 1, 1, 1, 1, 9, 0};
    int32_t lA, lB, lRootA, lRootB;
    int16_t cPtsA, cPtsB;
    unsigned cSamples, i, j, cWidth, iMin, cDiff, cScaledDiff, cPointsDiff, cRootDiff;
    uint16_t wControl;
    uint32_t dwMxcsr;

    setvbuf(stdout, NULL, _IONBF, 0);
    cSamples = argc > 1 ? strtoul(argv[1], NULL, 10) : 0;
    printf("double bytes=%zu mantissa=%d; long double bytes=%zu mantissa=%d max_exp=%d; eval=%d round=%d\n",
           sizeof(double), DBL_MANT_DIG, sizeof(long double), LDBL_MANT_DIG,
           LDBL_MAX_EXP, FLT_EVAL_METHOD, fegetround());
#if defined(__i386__) || defined(__x86_64__)
    __asm__("fnstcw %0" : "=m"(wControl));
    __asm__("stmxcsr %0" : "=m"(dwMxcsr));
    printf("x87_control=0x%04x mxcsr=0x%08x\n", (unsigned)wControl, (unsigned)dwMxcsr);
    if ((wControl & 0x0300) != 0x0300 || (wControl & 0x0c00) != 0 || (dwMxcsr & 0x6000) != 0) {
        fputs("Requires full extended x87 precision and nearest rounding for x87 and SSE.\n", stderr);
        return 2;
    }
#else
    fputs("This diagnostic requires an x86 oracle; use software extended arithmetic elsewhere.\n", stderr);
    return 2;
#endif
    if (DBL_MANT_DIG != 53 || LDBL_MANT_DIG != 64 || LDBL_MAX_EXP != 16384 || FLT_EVAL_METHOD != 0 ||
        fegetround() != FE_TONEAREST) {
        fputs("Requires binary64 double, x87 extended long double, no excess evaluation precision, and nearest rounding.\n", stderr);
        return 2;
    }

    cRootDiff = 0;
    for (i = 0; i <= 30000; i++) {
        lRootA = (int32_t)((long double)sqrt((double)((long double)i / 3.0)) + 0.9);
        lRootB = (int32_t)((double)sqrt((double)((double)i / 3.0)) + 0.9);
        if (lRootA != lRootB)
            cRootDiff++;
    }
    printf("desirability root inputs=30001 differences=%u\n", cRootDiff);

    memset(&plr, 0, sizeof(plr));
    memcpy(plr.rgAttr, rgAttr, sizeof(rgAttr));
    plr.pctIdealGrowth = 10;
    for (j = 0; j < 3; j++) {
        plr.rgEnvVarMin[j] = rgMin[j];
        plr.rgEnvVarMax[j] = rgMax[j];
        plr.rgEnvVar[j] = (rgMin[j] + rgMax[j]) / 2;
    }
    plrA = plr;
    plrB = plr;
    lA = ext_LInnateRaceHabitability(&plrA);
    lB = dbl_LInnateRaceHabitability(&plrB);
    cPtsA = ext_CAdvantagePoints(&plrA);
    cPtsB = dbl_CAdvantagePoints(&plrB);
    printf("witness habitability=%d/%d divided_by_2000=%d/%d points=%d/%d\n",
           lA, lB, lA / 2000, lB / 2000, cPtsA, cPtsB);
    printf("witness stored_sum=%.17g/%.17g hex=%a/%a\n",
           ext_dInnate, dbl_dInnate, ext_dInnate, dbl_dInnate);
    printf("witness before_integer_cast=%.21Lg/%.17g\n",
           (long double)ext_dInnate / 10.0 + 0.5, dbl_dInnate / 10.0 + 0.5);
    ShowRace(&plr);

    cDiff = cScaledDiff = cPointsDiff = 0;
    for (i = 0; i < cSamples; i++) {
        memset(&plr, 0, sizeof(plr));
        for (j = 0; j < 3; j++) {
            if (DwRand() % 8 == 0) {
                plr.rgEnvVar[j] = plr.rgEnvVarMin[j] = plr.rgEnvVarMax[j] = envImmune;
            } else {
                cWidth = 20 + 2 * (DwRand() % 41);
                iMin = DwRand() % (101 - cWidth);
                plr.rgEnvVarMin[j] = iMin;
                plr.rgEnvVarMax[j] = iMin + cWidth;
                plr.rgEnvVar[j] = iMin + cWidth / 2;
            }
        }
        plr.grbitAttr = (DwRand() % 2) << 1;
        plr.pctIdealGrowth = 1 + DwRand() % 20;
        plr.rgAttr[0] = 7 + DwRand() % 19;
        plr.rgAttr[1] = 5 + DwRand() % 11;
        plr.rgAttr[2] = 5 + DwRand() % 21;
        plr.rgAttr[3] = 5 + DwRand() % 21;
        plr.rgAttr[4] = 5 + DwRand() % 21;
        plr.rgAttr[5] = 2 + DwRand() % 14;
        plr.rgAttr[6] = 5 + DwRand() % 21;
        for (j = 8; j <= 13; j++)
            plr.rgAttr[j] = DwRand() % 3;
        plr.rgAttr[14] = DwRand() % 10;
        plrA = plr;
        plrB = plr;
        lA = ext_LInnateRaceHabitability(&plrA);
        lB = dbl_LInnateRaceHabitability(&plrB);
        if (lA != lB) {
            cDiff++;
            if (lA / 2000 != lB / 2000)
                cScaledDiff++;
            cPtsA = ext_CAdvantagePoints(&plrA);
            cPtsB = dbl_CAdvantagePoints(&plrB);
            if (cPtsA != cPtsB)
                cPointsDiff++;
            if (cDiff <= 12 || cPtsA != cPtsB) {
                printf("sample=%u habitability=%d/%d points=%d/%d", i, lA, lB, cPtsA, cPtsB);
                ShowRace(&plr);
            }
        }
    }
    printf("seed=0x12345678 samples=%u habitability_diffs=%u scaled_diffs=%u points_diffs=%u\n",
           cSamples, cDiff, cScaledDiff, cPointsDiff);
    // A successful diagnostic may report differences. It does not certify migration.
    return cRootDiff != 0 ? 1 : 0;
}
'''


def sources():
    race = precision_reference.read_source("race.c").split("uint16_t IRaceChecksum(PLAYER *pplr) {")[0]
    planet = precision_reference.read_source("planet.c")
    planet = "int16_t PctPlanetDesirability" + planet.split("int16_t PctPlanetDesirability", 1)[1]
    planet = planet.split("int32_t CalcPlanetMaxPop", 1)[0]
    if not race.endswith("\n\n") or "return LOWORD(pctPos);" not in planet:
        raise ValueError("Source boundaries changed; review the extraction before running")
    return race + "\n" + planet


def variant(source, mode):
    names = re.findall(r"^(?:int16_t|int32_t|char|void)\s+(\w+)\s*(?:\[|\()", source, re.M)
    defines = "".join(f"#define {name} {mode}_{name}\n" for name in names)
    if mode == "dbl":
        source = source.replace("long double", "double")
    source = source.replace('#include "common.h"', '#include "common.h"\ndouble ' + mode + '_dInnate;')
    cast = "long double" if mode == "ext" else "double"
    final = f"    return (int32_t)(({cast})lInnate / 10.0 + 0.5);"
    if source.count(final) != 1:
        raise ValueError("Habitability return changed; review the instrumentation")
    source = source.replace(final, f"    {mode}_dInnate = lInnate;\n" + final)
    return defines + source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc", help="Native GCC-compatible compiler")
    parser.add_argument("--optimization", choices=("O0", "O2", "O3"), default="O2")
    parser.add_argument("--samples", type=int, default=0, help="Deterministic random races in addition to the witness")
    parser.add_argument("--generated-dir", type=Path, default=None, help="Deprecated; headers are generated from the immutable reference")
    args = parser.parse_args()
    if not 0 <= args.samples <= 100000000:
        parser.error("--samples must be between 0 and 100000000")
    source = sources()
    print("extracted source sha256=" + hashlib.sha256(source.encode()).hexdigest(), flush=True)
    subprocess.run([args.cc, "--version"], check=True)
    with tempfile.TemporaryDirectory(prefix="stars-race-precision-") as directory:
        work = Path(directory)
        for mode in ("ext", "dbl"):
            (work / f"{mode}.c").write_text(variant(source, mode))
        (work / "main.c").write_text(HARNESS)
        command = [args.cc, *precision_reference.oracle_flags(), "-" + args.optimization, "-std=gnu11", "-fsigned-char",
                   "-fno-fast-math", "-ffp-contract=off", *precision_reference.include_flags(work), str(work / "ext.c"),
                   str(work / "dbl.c"), str(work / "main.c"), "-lm", "-o", str(work / "probe")]
        print("compiler command=" + " ".join(command), flush=True)
        subprocess.run(command, check=True)
        subprocess.run([str(work / "probe"), str(args.samples)], check=True)


if __name__ == "__main__":
    main()
