/* STARS_TEST_PRECISION: whole calculator comparisons, linked with two copies
   extracted from race.c/planet.c by race_precision.py. */
#include "common.h"
#include "precision_probe.c"

PLAYER rgplr[16];
int32_t ext_LInnateRaceHabitability(PLAYER *);
int32_t dbl_LInnateRaceHabitability(PLAYER *);
int16_t ext_CAdvantagePoints(PLAYER *);
int16_t dbl_CAdvantagePoints(PLAYER *);
extern double ext_dInnate, dbl_dInnate;

static PRECISION_STATS rgStats[5];
static int16_t rgMin[5000];
static int16_t rgMax[5000];
static int cIntervals;

static void InitRace(PLAYER *pplr) {
    int8_t rgAttr[16] = {10, 10, 10, 10, 10, 5, 10, 0, 1, 1, 1, 1, 1, 1, 9, 0};
    int8_t rgLo[3] = {31, 61, 9};
    int8_t rgHi[3] = {71, 97, 73};
    int i;

    memset(pplr, 0, sizeof(*pplr));
    memcpy(pplr->rgAttr, rgAttr, sizeof(rgAttr));
    pplr->pctIdealGrowth = 10;
    for (i = 0; i < 3; i++) {
        pplr->rgEnvVarMin[i] = rgLo[i];
        pplr->rgEnvVarMax[i] = rgHi[i];
        pplr->rgEnvVar[i] = (rgLo[i] + rgHi[i]) / 2;
    }
}

static int CheckRace(PLAYER *pplr, int fGolden) {
    PLAYER plrA;
    PLAYER plrB;
    PLAYER plrSav;
    double rg[MAX_INPUTS];
    double dSumA;
    double dSumB;
    int32_t lA;
    int32_t lB;
    int16_t cPtsA;
    int16_t cPtsB;
    int cBytes;
    int i;

    plrA = plrB = *pplr;
    plrSav = rgplr[0];
    for (i = 0; i < 3; i++) {
        rg[i * 3] = pplr->rgEnvVarMin[i];
        rg[i * 3 + 1] = pplr->rgEnvVar[i];
        rg[i * 3 + 2] = pplr->rgEnvVarMax[i];
    }
    rg[9] = pplr->pctIdealGrowth;
    rg[10] = pplr->grbitAttr;
    for (i = 0; i < 16; i++) rg[11 + i] = pplr->rgAttr[i];
    lA = ext_LInnateRaceHabitability(&plrA);
    dSumA = ext_dInnate;
    lB = dbl_LInnateRaceHabitability(&plrB);
    dSumB = dbl_dInnate;
    cPtsA = ext_CAdvantagePoints(&plrA);
    cPtsB = dbl_CAdvantagePoints(&plrB);
    if (memcmp(&plrSav, &rgplr[0], sizeof(plrSav))) {
        fputs("Race calculator did not restore rgplr[0].\n", stderr);
        return 0;
    }
    if (fGolden && (lA != 1470286 || lB != 1470287 || cPtsA != 541 || cPtsB != 541)) {
        fputs("Known race precision witness changed; review the oracle.\n", stderr);
        return 0;
    }
    cBytes = 0;
    for (i = 0; i < (int)sizeof(plrA); i++)
        if (((unsigned char *)&plrA)[i] != ((unsigned char *)&plrB)[i]) cBytes++;
    Record(&rgStats[0], dSumA, dSumB, rg, 27, 2);
    Record(&rgStats[1], lA, lB, rg, 27, 0);
    Record(&rgStats[2], lA / 2000, lB / 2000, rg, 27, 0);
    Record(&rgStats[3], cPtsA, cPtsB, rg, 27, 0);
    Record(&rgStats[4], 0, cBytes, rg, 27, 0);
    return 1;
}

static void SetInterval(PLAYER *pplr, int iAxis, int iInterval) {
    if (iInterval == cIntervals) {
        pplr->rgEnvVarMin[iAxis] = envImmune;
        pplr->rgEnvVarMax[iAxis] = envImmune;
        pplr->rgEnvVar[iAxis] = envImmune;
    } else {
        pplr->rgEnvVarMin[iAxis] = rgMin[iInterval];
        pplr->rgEnvVarMax[iAxis] = rgMax[iInterval];
        pplr->rgEnvVar[iAxis] = (rgMin[iInterval] + rgMax[iInterval]) / 2;
    }
}

int main(int argc, char **argv) {
    PLAYER plr;
    uint64_t cSamples;
    uint64_t i;
    uint64_t iStart;
    uint64_t cCartesian;
    uint64_t q;
    int iAxis;
    int iInterval;
    int iMask;
    int iTT;
    int iGrowth;
    int iMin;
    int cWidth;
    int j;
    int fOdd;
    int fAxis;

    if (argc != 7 || !FCheckPrecision()) return 2;
    cSamples = strtoull(argv[1], NULL, 10);
    fOdd = atoi(argv[2]);
    iStart = strtoull(argv[3], NULL, 10);
    cCartesian = strtoull(argv[4], NULL, 10);
    qwSeed = strtoull(argv[5], NULL, 10);
    fAxis = atoi(argv[6]);
    for (cWidth = fOdd ? 2 : 20; cWidth <= 100; cWidth += fOdd ? 1 : 2) {
        for (iMin = 0; iMin + cWidth <= 100; iMin++) {
            rgMin[cIntervals] = iMin;
            rgMax[cIntervals++] = iMin + cWidth;
        }
    }
    InitRace(&plr);
    if (!CheckRace(&plr, 1)) return 3;
    /* Vary growth and immunity/TT around the witness to check point propagation. */
    for (iMask = 0; iMask < 8; iMask++) {
        for (iTT = 0; iTT < 2; iTT++) {
            for (iGrowth = 1; iGrowth <= 20; iGrowth++) {
                InitRace(&plr);
                for (j = 0; j < 3; j++) if (iMask & (1 << j)) SetInterval(&plr, j, cIntervals);
                plr.grbitAttr = (uint32_t)iTT << ibitRaceTT;
                plr.pctIdealGrowth = iGrowth;
                if (!CheckRace(&plr, 0)) return 3;
            }
        }
    }
    if (fAxis) {
        for (iAxis = 0; iAxis < 3; iAxis++) {
            for (iInterval = 0; iInterval < cIntervals; iInterval++) {
                for (iMask = 0; iMask < 8; iMask++) {
                    if (iMask & (1 << iAxis)) continue;
                    for (iTT = 0; iTT < 2; iTT++) {
                        InitRace(&plr);
                        for (j = 0; j < 3; j++) if (iMask & (1 << j)) SetInterval(&plr, j, cIntervals);
                        SetInterval(&plr, iAxis, iInterval);
                        plr.grbitAttr = (uint32_t)iTT << ibitRaceTT;
                        if (!CheckRace(&plr, 0)) return 3;
                    }
                }
            }
        }
    }
    /* Mixed-radix indices let separate runs exhaust disjoint habitat shards. */
    for (i = iStart; i < iStart + cCartesian; i++) {
        InitRace(&plr);
        q = i;
        plr.grbitAttr = (uint32_t)(q % 2) << ibitRaceTT;
        q /= 2;
        for (j = 0; j < 3; j++) {
            SetInterval(&plr, j, q % (cIntervals + 1));
            q /= cIntervals + 1;
        }
        if (q != 0) return 4;
        if (!CheckRace(&plr, 0)) return 3;
    }
    for (i = 0; i < cSamples; i++) {
        InitRace(&plr);
        for (j = 0; j < 3; j++) SetInterval(&plr, j, QwRand() % (cIntervals + 1));
        plr.grbitAttr = (uint32_t)QwRand();
        plr.pctIdealGrowth = 1 + QwRand() % 20;
        plr.rgAttr[0] = 7 + QwRand() % 19;
        plr.rgAttr[1] = 5 + QwRand() % 11;
        for (j = 2; j <= 4; j++) plr.rgAttr[j] = 5 + QwRand() % 21;
        plr.rgAttr[5] = 2 + QwRand() % 14;
        plr.rgAttr[6] = 5 + QwRand() % 21;
        plr.rgAttr[7] = QwRand() % 7;
        for (j = 8; j <= 13; j++) plr.rgAttr[j] = QwRand() % 3;
        plr.rgAttr[14] = QwRand() % 10;
        if (!CheckRace(&plr, 0)) return 3;
    }
    for (j = 0; j < 5; j++) WriteStats(j, &rgStats[j], 27, 0);
    return 0;
}
