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

// Format 2.85 is UTF-8; older and newer formats are refused.
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
        return vers.verMajor == 2 && vers.verMinor == 85 && vers.verInc == 0;
    }
    vers.verMinor = verMinor;
    fseek(fp, 2 + offsetof(RTBOF, wVersion), SEEK_SET);
    fwrite(&vers, sizeof(vers), 1, fp);
    fclose(fp);
    return TRUE;
}

static void test_WriteBOF_writes_version_2_85(void) {
    char szDir[MAX_PATH];

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("WriteBOF_version", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_CHECK_(FSetHostVersion(-1), "host file isn't 2.85");

    TEST_ASSERT(FSetHostVersion(83));
    TEST_CHECK_(!FStarsTestLoadHost(), "a 2.83 host file loaded");
    TEST_ASSERT(FSetHostVersion(86));
    TEST_CHECK_(!FStarsTestLoadHost(), "a 2.86 host file loaded");
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

static void test_SetVisPFInit_host(void) {
    THING th;
    SHDEF rgshdef[16], rgshdefSB[10];

    TEST_ASSERT(FStarsTestInit());
    memset(&th, 0, sizeof(th));
    memset(rgshdef, 0, sizeof(rgshdef));
    memset(rgshdefSB, 0, sizeof(rgshdefSB));
    rglpshdef[0] = rgshdef;
    rglpshdefSB[0] = rgshdefSB;
    th.ith = ithMineralPacket;
    th.iplr = 0;
    lpThings = &th;
    cThing = 1;
    game.cPlayer = 1;
    SetVisPFInit(iplrNone);
    TEST_CHECK(rgplr[0].fInclude);
    TEST_CHECK(rgplr[0].det == detAll);
    TEST_CHECK(th.thp.fInclude);
    lpThings = NULL;
    cThing = 0;
    rglpshdef[0] = rglpshdefSB[0] = NULL;
}

static void test_utf8_saved_names(void) {
    char szDir[MAX_PATH], szExpected[32], szPacked[64], szRound[32];
    int16_t cPacked = sizeof(szPacked), cRound = 31;
    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("utf8_saved_names", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());
    memset(szExpected, 'a', 27);
    memcpy(szExpected + 27, "\xf0\x9f\x98\x80", 5);
    strcpy(rgplr[0].szName, szExpected);
    strcpy(rgplr[0].szNames, "caf\xc3\xa9-\xce\xa9");
    TEST_ASSERT(FCompressUserString(szExpected, szPacked, &cPacked));
    TEST_ASSERT(FDecompressUserString(szPacked, cPacked, szRound, &cRound));
    TEST_CHECK(strcmp(szRound, szExpected) == 0);
    TEST_ASSERT(FStarsTestSaveHost());
    TEST_ASSERT(FStarsTestLoadHost());
    TEST_CHECK(strcmp(rgplr[0].szName, szExpected) == 0);
    TEST_CHECK(strcmp(rgplr[0].szNames, "caf\xc3\xa9-\xce\xa9") == 0);
}

static void test_utf8_record_bounds(void) {
    uint8_t rgb[64];
    char sz[32];
    size_t cbRead;
    int16_t cOut;

    memcpy(rgb, "\0\xc3\xa9\0", 4);
    TEST_CHECK(!FReadUserString(rgb, 3, sz, sizeof(sz), NULL));
    TEST_CHECK(sz[0] == 0);
    TEST_CHECK(FReadUserString(rgb, 4, sz, sizeof(sz), &cbRead));
    TEST_CHECK(cbRead == 4 && !strcmp(sz, "\xc3\xa9"));
    rgb[1] = 0xff;
    TEST_CHECK(!FReadUserString(rgb, 4, sz, sizeof(sz), NULL));
    cOut = 63;
    TEST_ASSERT(FCompressUserString("\xf0\x9f\x98\x80", (char *)rgb + 1, &cOut));
    rgb[0] = cOut;
    TEST_CHECK(!FReadUserString(rgb, cOut, sz, sizeof(sz), NULL));
    TEST_CHECK(FReadUserString(rgb, cOut + 1, sz, sizeof(sz), &cbRead));
    TEST_CHECK(cbRead == (size_t)cOut + 1 && !strcmp(sz, "\xf0\x9f\x98\x80"));
    rgb[0] = 1; rgb[1] = 0x0b;
    TEST_CHECK(!FReadUserString(rgb, 2, sz, sizeof(sz), NULL));
    TEST_CHECK(sz[0] == 0);
    rgb[1] = 0xff;
    TEST_CHECK(!FReadUserString(rgb, 2, sz, sizeof(sz), NULL));
}

static void test_utf8_design_and_plan_bounds(void) {
    uint8_t rgb[64];
    SHDEF rgshdefT[16];
    BTLPLAN btlplan;
    RTSHDEF *lprt = (RTSHDEF *)rgb;

    TEST_ASSERT(FStarsTestInit());
    idPlayer = iplrNone;
    memset(rgb, 0, sizeof(rgb));
    memset(rgshdefT, 0, sizeof(rgshdefT));
    lprt->det = detSome; lprt->ihuldef = ihuldefScout;
    memcpy(rgb + offsetof(RTSHDEF, chs), "\0\xc3\xa9\0", 4);
    TEST_CHECK(!FReadShDef(lprt, rgshdefT, 0, offsetof(RTSHDEF, chs) + 3));
    TEST_CHECK(FReadShDef(lprt, rgshdefT, 0, offsetof(RTSHDEF, chs) + 4));
    TEST_CHECK(!strcmp(rgshdefT[0].hul.szClass, "\xc3\xa9"));
    memset(rgb, 0, sizeof(rgb));
    memcpy(rgb + 4, "\0\xc3\xa9\0", 4);
    idsFileError = 0;
    memset(&btlplan, 0x55, sizeof(btlplan));
    TEST_CHECK(!UnpackBattlePlan(rgb, &btlplan, 0, 7));
    TEST_CHECK(btlplan.szName[0] == 0x55);
    TEST_CHECK(idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    idsFileError = 0;
    TEST_CHECK(UnpackBattlePlan(rgb, &btlplan, 0, 8));
    TEST_CHECK(idsFileError == 0 && !strcmp(btlplan.szName, "\xc3\xa9"));
}

static void test_utf8_raw_battle_plan(void) {
    BTLPLAN btlplan = {0};
    BTLPLAN btlplanRead;
    HDR hdr;
    uint8_t rgb[64];
    char szPacked[32];
    int16_t cOut = 31;
    int i;

    TEST_ASSERT(FStarsTestInit());
    for (i = 0; i < 10; i++) memcpy(btlplan.szName + i * 3, "\xe6\x9d\xb1", 3);
    btlplan.szName[30] = 'X'; btlplan.szName[31] = 0;
    TEST_ASSERT(!FCompressUserString(btlplan.szName, szPacked, &cOut));
    WriteBattlePlan(&btlplan, TRUE);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtBtlPlan && hdr.cb == sizeof(BTLPLAN) + 1 && rgb[4] == 0);
    TEST_ASSERT(UnpackBattlePlan(rgb, &btlplanRead, 0, hdr.cb));
    TEST_CHECK(!strcmp(btlplanRead.szName, btlplan.szName));
}

TEST_LIST = {{"UTF-8 records require complete in-record text", test_utf8_record_bounds},
             {"maximum raw UTF-8 battle-plan names round trip", test_utf8_raw_battle_plan},
             {"design and battle-plan readers respect text record boundaries", test_utf8_design_and_plan_bounds},
             {"UTF-8 saved names and byte capacities", test_utf8_saved_names},
             {"SetVisPFInit host sentinel", test_SetVisPFInit_host},
             {"FWriteDataFile Claim Adjuster sees only habitat", test_FWriteDataFile_claim_adjuster_sees_only_habitat},
             {"SetVisPFFleets sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase},
             {"SetVisPFFleets long-range scanner sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase_long_range},
             {"WriteBOF writes version 2.85", test_WriteBOF_writes_version_2_85},
             {"FWriteDataFile masks other players' things", test_FWriteDataFile_masks_other_players_things},
             {NULL, NULL}};
