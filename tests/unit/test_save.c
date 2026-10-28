#include "acutest.h"

#include "stars_test.h"

// ScanCloakedStarbase puts a player-0 fleet with a dScanRange2 penetrating
// scanner 100 ly from player 1's homeworld, whose starbase design has
// lVisible lVis2, and returns what SetVisPFFleets lets player 0 see of it.
static DetType DetScanCloakedStarbase(const char *pszTest, int16_t dScanRange2, int32_t lVis2) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl0;
    PLANET     *lppl1;
    FLEET      *lpfl;
    int16_t     ish;
    int16_t     i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl0 = LpplStarsTestHomeworld(0);
    lppl1 = LpplStarsTestHomeworld(1);
    TEST_ASSERT(lppl0 != NULL && lppl1 != NULL && lppl1->fStarbase);

    for (ish = 0; ish < 16 && rglpshdef[0][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < 16);
    rglpshdef[0][ish].dScanRange = 0;
    rglpshdef[0][ish].dScanRange2 = dScanRange2;
    lpfl = LpflStarsTestAddFleet(0, lppl0->id, ish, 1);
    lpfl->idPlanet = idPlanetDeepSpace;
    lpfl->pt.x = rgptPlan[lppl1->id].x + 100;
    lpfl->pt.y = rgptPlan[lppl1->id].y;
    rglpshdefSB[1][lppl1->isb].lVisible = lVis2;

    for (i = 0; i < cPlanet; i++) {
        lpPlanets[i].fInclude = FALSE;
        lpPlanets[i].det = detNone;
    }
    gd.fGeneratingTurn = TRUE;
    SetVisPFFleets(0);
    return lppl1->fInclude ? (DetType)lppl1->det : detNone;
}

// A starbase cloaked to lVisible 2500 (seen at half range) 100 ly away is
// within a 600 ly scanner's reduced 300 ly.
static void test_SetVisPFFleets_cloaked_starbase(void) {
    DetType det;

    det = DetScanCloakedStarbase("SetVisPFFleets_cloaked_starbase", 600, 2500);
    TEST_CHECK_(det >= detSome, "det %d", det);
}

// ISB vs scanning: lRadius2 * lVis2 overflowed 32 bits for scanners over
// about 463 ly, so a lightly cloaked starbase was only obscured.
static void test_SetVisPFFleets_cloaked_starbase_long_range(void) {
    DetType det;

    det = DetScanCloakedStarbase("SetVisPFFleets_cloaked_starbase_long_range", 600, 9000);
    TEST_CHECK_(det >= detSome, "det %d", det);
}

// Claim Adjuster turn files: other players' records were written at full
// detail so a CA player gets their habitat, which also sent their tech,
// research, traits, production template and relations.
static void test_FWriteDataFile_claim_adjuster_sees_only_habitat(void) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl1;
    PLAYER      plrAi;
    int16_t     ish;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FWriteDataFile_claim_adjuster", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    SetRaceStat(&rgplr[0], rsMajorAdv, raTerra);
    lppl1 = LpplStarsTestHomeworld(1);
    TEST_ASSERT(lppl1 != NULL);
    for (ish = 0; ish < 16 && rglpshdef[0][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < 16);
    LpflStarsTestAddFleet(0, lppl1->id, ish, 1);
    TEST_ASSERT(FStarsTestSaveHost());
    TEST_ASSERT(FStarsTestGenerate());

    TEST_ASSERT(FStarsTestLoadHost());
    plrAi = rgplr[1];
    TEST_ASSERT(plrAi.grbitAttr != 0 || plrAi.rgAttr[0] != 0);
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    TEST_ASSERT_(rgplr[1].fInclude, "player 0 doesn't see the AI");
    TEST_CHECK(memcmp(rgplr[1].rgEnvVar, plrAi.rgEnvVar, 3) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgEnvVarMin, plrAi.rgEnvVarMin, 3) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgEnvVarMax, plrAi.rgEnvVarMax, 3) == 0);
    TEST_CHECK_(rgplr[1].grbitAttr == 0, "traits %#lx", (unsigned long)rgplr[1].grbitAttr);
    TEST_CHECK(memcmp(rgplr[1].rgAttr, (int8_t[16]){0}, 16) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgTech, (int8_t[6]){0}, 6) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgResSpent, (uint32_t[6]){0}, sizeof(plrAi.rgResSpent)) == 0);
    TEST_CHECK(rgplr[1].pctResearch == 0 && rgplr[1].lResLastYear == 0 && rgplr[1].pctIdealGrowth == 0);
}

// File format version: 2.8 writes 2.84, which 2.6j and 2.7 refuse, and still
// reads their 2.83 files. Later versions are refused.
static int16_t FSetHostVersion(int16_t verMinor) {
    char  szFile[MAX_PATH];
    VERS  vers;
    FILE *fp;

    snprintf(szFile, sizeof(szFile), "%s.hst", szBase);
    fp = fopen(szFile, "r+b");
    if (fp == NULL)
        return FALSE;
    // The RTBOF record follows its 2-byte header and isn't encrypted.
    fseek(fp, 2 + offsetof(RTBOF, wVersion), SEEK_SET);
    if (fread(&vers, sizeof(vers), 1, fp) != 1) {
        fclose(fp);
        return FALSE;
    }
    if (verMinor < 0) {
        fclose(fp);
        return vers.verMajor == 2 && vers.verMinor == 84 && vers.verInc == 0;
    }
    vers.verMinor = verMinor;
    fseek(fp, 2 + offsetof(RTBOF, wVersion), SEEK_SET);
    fwrite(&vers, sizeof(vers), 1, fp);
    fclose(fp);
    return TRUE;
}

static void test_WriteBOF_writes_version_2_84(void) {
    char szDir[MAX_PATH];

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("WriteBOF_version", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_CHECK_(FSetHostVersion(-1), "host file isn't 2.84");

    TEST_ASSERT(FSetHostVersion(83));
    TEST_CHECK_(FStarsTestLoadHost(), "a 2.83 host file didn't load");
    TEST_ASSERT(FSetHostVersion(85));
    TEST_CHECK_(!FStarsTestLoadHost(), "a 2.85 host file loaded");
}

// LpthFind returns the loaded game's thing of type ith whose owner
// is iplr, or NULL.
static THING *LpthFind(ThingType ith, int16_t iplr) {
    int16_t i;

    for (i = 0; i < cThing; i++) {
        if (lpThings[i].ith == ith && lpThings[i].iplr == iplr)
            return &lpThings[i];
    }
    return NULL;
}

// Turn-file knowledge leaks: a player's turn file carried whole THING
// records, so it showed which other players had seen a minefield, the
// Mystery Trader or a wormhole, who had travelled through the wormhole,
// and the part the trader carries.
static void test_FWriteDataFile_masks_other_players_things(void) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl;
    THING      *lpth;
    POINT16     pt;
    uint16_t    idWorm1;
    uint16_t    idWorm2;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FWriteDataFile_things", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(0);
    TEST_ASSERT(lppl != NULL);
    pt = rgptPlan[lppl->id];
    // Everything sits on player 0's homeworld, where its scanner sees it.
    while ((lpth = LpthFind(ithWormhole, 0)) != NULL)
        FreeLpth(lpth);
    lpth = LpthNew(1, ithMinefield);
    lpth->pt = pt;
    lpth->thm.cMines = 2500;
    lpth->thm.iType = mineStandard;
    lpth->thm.grbitPlr = 1 << 1;
    lpth->thm.grbitPlrNow = 1 << 1;
    lpth = LpthNew(0, ithMysteryTrader);
    lpth->pt = pt;
    lpth->tht.ptDest = pt;
    lpth->tht.grbitPlr = 1 << 1;
    lpth->tht.grbitTrader = grbitTraderBeam;
    lpth = LpthNew(0, ithWormhole);
    lpth->pt = pt;
    lpth->thw.grbitPlr = (1 << 0) | (1 << 1);
    lpth->thw.grbitPlrTrav = 1 << 1;
    idWorm1 = lpth->idFull;
    lpth = LpthNew(0, ithWormhole);
    lpth->pt.x = pt.x + 200;
    lpth->pt.y = pt.y;
    lpth->thw.grbitPlr = 1 << 1;
    lpth->thw.grbitPlrTrav = 1 << 1;
    idWorm2 = lpth->idFull;
    lpth->thw.idPartner = idWorm1;
    LpthFromId(idWorm1)->thw.idPartner = idWorm2;
    TEST_ASSERT(FWriteDataFile(szBase, 0, FALSE));

    TEST_ASSERT(FStarsTestLoadPlayer(0));
    lpth = LpthFind(ithMinefield, 1);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the minefield");
    TEST_CHECK_(lpth->thm.grbitPlr == 1 && lpth->thm.grbitPlrNow == 1, "minefield seen by %#x, now %#x", lpth->thm.grbitPlr, lpth->thm.grbitPlrNow);
    lpth = LpthFind(ithMysteryTrader, 0);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the trader");
    TEST_CHECK_(lpth->tht.grbitPlr == 0 && lpth->tht.grbitTrader == 0, "trader met by %#x, carrying %#x", lpth->tht.grbitPlr, lpth->tht.grbitTrader);
    lpth = LpthFind(ithWormhole, 0);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the wormhole");
    TEST_CHECK_(lpth->thw.grbitPlr == 1 && lpth->thw.grbitPlrTrav == 0, "wormhole seen by %#x, travelled by %#x", lpth->thw.grbitPlr, lpth->thw.grbitPlrTrav);
}

// A 31-character name that needs three nibbles per character cannot fit
// the compressed-name limit. Its raw record needs 37 bytes, not 36.
static void test_WriteBattlePlan_fits_maximum_raw_name(void) {
    char szDir[MAX_PATH];
    char szFile[MAX_PATH];
    BTLPLAN btlplan;
    BTLPLAN btlRead;
    int16_t cch;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("WriteBattlePlan_raw_name", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sgame.hst", szDir, szDirSep);
    for (cch = 30; cch <= 31; cch++) {
        memset(&btlplan, 0, sizeof(btlplan));
        btlplan.iplan = 6;
        btlplan.mdTactic = 3;
        memset(btlplan.szName, '/', cch);
        TEST_ASSERT(FCreateFile(dtHost, iplrNone, szFile));
        WriteBattlePlan(&btlplan, FALSE);
        WriteRt(rtEOF, 0, NULL);
        StreamClose();

        StreamOpen(szFile, mdRead);
        ReadRt();
        TEST_ASSERT(hdrCur.rt == rtBOF);
        ReadRt();
        TEST_ASSERT(hdrCur.rt == rtBtlPlan);
        TEST_CHECK(hdrCur.cb == 4 + 1 + cch + 1);
        TEST_CHECK(rgbCur[4] == 0);
        TEST_CHECK(memcmp(rgbCur + 5, btlplan.szName, cch + 1) == 0);
        UnpackBattlePlan(rgbCur, &btlRead, 6);
        TEST_CHECK(strcmp(btlRead.szName, btlplan.szName) == 0);
        TEST_CHECK(btlRead.mdTactic == btlplan.mdTactic);
        StreamClose();
    }
}

TEST_LIST = {{"FWriteDataFile Claim Adjuster sees only habitat", test_FWriteDataFile_claim_adjuster_sees_only_habitat},
             {"SetVisPFFleets sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase},
             {"SetVisPFFleets long-range scanner sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase_long_range},
             {"WriteBOF writes version 2.84", test_WriteBOF_writes_version_2_84},
             {"FWriteDataFile masks other players' things", test_FWriteDataFile_masks_other_players_things},
             {"WriteBattlePlan fits maximum raw name", test_WriteBattlePlan_fits_maximum_raw_name},
             {NULL, NULL}};
