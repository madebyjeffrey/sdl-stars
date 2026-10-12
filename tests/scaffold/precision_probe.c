/* STARS_TEST_PRECISION: included by the generated differential probe. */
#include <float.h>
#include <fenv.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LOWORD
#define LOWORD(l) ((uint16_t)(uint32_t)(l))
#endif
#define MAX_INPUTS 32

typedef struct {
    uint64_t cTried;
    uint64_t cSkipped;
    uint64_t cDifferent;
    uint64_t cUlpMax;
    double dAbsMax;
    double dRelMax;
    long double dAbsSum;
    double dFirstExt;
    double dFirstDbl;
    double dMaxExt;
    double dMaxDbl;
    double rgFirst[MAX_INPUTS];
    double rgMax[MAX_INPUTS];
} PRECISION_STATS;

static uint64_t qwSeed = UINT64_C(0x123456789abcdef);

static uint64_t QwRand(void) {
    qwSeed ^= qwSeed << 13;
    qwSeed ^= qwSeed >> 7;
    qwSeed ^= qwSeed << 17;
    return qwSeed;
}

static int FCheckPrecision(void) {
    uint16_t wControl;
    uint32_t dwMxcsr;

#if defined(__i386__) || defined(__x86_64__)
    __asm__("fnstcw %0" : "=m"(wControl));
    __asm__("stmxcsr %0" : "=m"(dwMxcsr));
#else
    fputs("Requires an x86 extended-precision oracle.\n", stderr);
    return 0;
#endif
    printf("{\"environment\":{\"double_bytes\":%zu,\"long_double_bytes\":%zu,"
           "\"double_mantissa\":%d,\"long_double_mantissa\":%d,\"long_double_max_exp\":%d,"
           "\"eval_method\":%d,\"round\":%d,\"x87_control\":%u,\"mxcsr\":%u}}\n",
           sizeof(double), sizeof(long double), DBL_MANT_DIG, LDBL_MANT_DIG,
           LDBL_MAX_EXP, FLT_EVAL_METHOD, fegetround(), (unsigned)wControl, (unsigned)dwMxcsr);
    if (DBL_MANT_DIG != 53 || LDBL_MANT_DIG != 64 || LDBL_MAX_EXP != 16384 ||
        FLT_EVAL_METHOD != 0 || fegetround() != FE_TONEAREST ||
        (wControl & 0x0f00) != 0x0300 || (dwMxcsr & 0xe040) != 0) {
        fputs("Requires binary64, full x87 extended precision, nearest rounding, and no FTZ/DAZ.\n", stderr);
        return 0;
    }
    return 1;
}

/* Ordered bit keys measure representable steps, including negative outputs. */
static uint64_t UlpKey(double d, int fFloat) {
    uint64_t qw;
    uint32_t dw;
    float pct;

    if (d == 0.0) d = 0.0;
    if (fFloat) {
        pct = (float)d;
        memcpy(&dw, &pct, sizeof(dw));
        return dw & UINT32_C(0x80000000) ? (uint32_t)(~dw + 1) : dw | UINT32_C(0x80000000);
    }
    memcpy(&qw, &d, sizeof(qw));
    return qw & UINT64_C(0x8000000000000000) ? ~qw + 1 : qw | UINT64_C(0x8000000000000000);
}

static void Record(PRECISION_STATS *lp, double dExt, double dDbl, const double *rg, int cInputs, int grResult) {
    double dAbs;
    double dRel;
    uint64_t qwExt;
    uint64_t qwDbl;
    uint64_t cUlp;

    lp->cTried++;
    if (dExt == dDbl) return;
    dAbs = fabs(dExt - dDbl);
    dRel = dExt != 0.0 ? dAbs / fabs(dExt) : dAbs;
    if (lp->cDifferent == 0) {
        lp->dFirstExt = dExt;
        lp->dFirstDbl = dDbl;
        memcpy(lp->rgFirst, rg, cInputs * sizeof(*rg));
    }
    lp->cDifferent++;
    lp->dAbsSum += dAbs;
    if (dAbs > lp->dAbsMax) {
        lp->dAbsMax = dAbs;
        lp->dMaxExt = dExt;
        lp->dMaxDbl = dDbl;
        memcpy(lp->rgMax, rg, cInputs * sizeof(*rg));
    }
    if (dRel > lp->dRelMax) lp->dRelMax = dRel;
    if (grResult != 0) {
        qwExt = UlpKey(dExt, grResult == 1);
        qwDbl = UlpKey(dDbl, grResult == 1);
        cUlp = qwExt > qwDbl ? qwExt - qwDbl : qwDbl - qwExt;
        if (cUlp > lp->cUlpMax) lp->cUlpMax = cUlp;
    }
}

static void WriteWitness(double dExt, double dDbl, const double *rg, int cInputs) {
    int i;

    printf("{\"extended\":%.17g,\"double\":%.17g,\"extended_hex\":\"%a\",\"double_hex\":\"%a\",\"inputs\":[",
           dExt, dDbl, dExt, dDbl);
    for (i = 0; i < cInputs; i++) printf("%s%.17g", i != 0 ? "," : "", rg[i]);
    printf("]}");
}

static void WriteStats(int iCase, PRECISION_STATS *lp, int cInputs, int fExhaustive) {
    printf("{\"case\":%d,\"exhaustive_domain\":%s,\"evaluated\":%" PRIu64 ",\"skipped\":%" PRIu64
           ",\"different\":%" PRIu64 ",\"max_abs\":%.17g,\"max_rel\":%.17g,\"max_ulps\":%" PRIu64
           ",\"mean_abs\":%.17g,\"first\":",
           iCase, fExhaustive ? "true" : "false", lp->cTried, lp->cSkipped, lp->cDifferent,
           lp->dAbsMax, lp->dRelMax, lp->cUlpMax, lp->cTried != 0 ? (double)(lp->dAbsSum / lp->cTried) : 0.0);
    if (lp->cDifferent != 0) WriteWitness(lp->dFirstExt, lp->dFirstDbl, lp->rgFirst, cInputs);
    else printf("null");
    printf(",\"largest\":");
    if (lp->cDifferent != 0) WriteWitness(lp->dMaxExt, lp->dMaxDbl, lp->rgMax, cInputs);
    else printf("null");
    puts("}");
}
