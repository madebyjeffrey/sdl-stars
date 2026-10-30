#include "acutest.h"
[[maybe_unused]] static char szTestLiteral1[] = "\xf0\x9f\x98\x80";

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
    rglpshdefSB[1][lppl1->isb].SetLVisible(lVis2);

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
    const int8_t rgAttrZero[16] = {}, rgTechZero[6] = {};
    const uint32_t rgResZero[6] = {};

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
    TEST_CHECK(memcmp(rgplr[1].rgAttr, rgAttrZero, 16) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgTech, rgTechZero, 6) == 0);
    TEST_CHECK(memcmp(rgplr[1].rgResSpent, rgResZero, sizeof(plrAi.rgResSpent)) == 0);
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

static void test_FOpenFile_reports_unsupported_versions_and_corruption(void) {
    char  szDir[MAX_PATH];
    char  szFile[MAX_PATH];
    FILE *fp;
    HDR   hdr;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FOpenFile_versions", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    idsFileError = 0;
    TEST_CHECK_(FStarsTestLoadHost(), "current-format host file was rejected");
    TEST_CHECK(idsFileError == 0);

    TEST_ASSERT(FSetHostVersion(83));
    idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost());
    TEST_CHECK(idsFileError == idsSorryFileCreatedOlderVersionStarsIncompatible);

    TEST_ASSERT(FSetHostVersion(86));
    idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost());
    TEST_CHECK(idsFileError == idsFileCreatedNewerVersionStarsMustUpgrade);

    TEST_ASSERT(FSetHostVersion(85));
    snprintf(szFile, sizeof(szFile), "%s.hst", szBase);
    fp = fopen(szFile, "r+b");
    TEST_ASSERT(fp != NULL);
    hdr.rt = rtBOF;
    hdr.cb = 1;
    TEST_CHECK(fwrite(&hdr, sizeof(hdr), 1, fp) == 1);
    fclose(fp);
    idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost());
    TEST_CHECK(idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
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
    TEST_ASSERT(FCompressUserString(szTestLiteral1, (char *)rgb + 1, &cOut));
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
    const uint8_t *lprt = rgb;

    TEST_ASSERT(FStarsTestInit());
    idPlayer = iplrNone;
    memset(rgb, 0, sizeof(rgb));
    memset(rgshdefT, 0, sizeof(rgshdefT));
    rgb[0] = detSome; rgb[2] = ihuldefScout;
    memcpy(rgb + cbWireShDefPartial, "\0\xc3\xa9\0", 4);
    TEST_CHECK(!FReadShDef(lprt, rgshdefT, 0, cbWireShDefPartial + 3));
    TEST_CHECK(FReadShDef(lprt, rgshdefT, 0, cbWireShDefPartial + 4));
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



// The file reader historically borrows the complete reusable scratch backing,
// even when the current record declares zero bytes. Keep that admission and the
// exact next-record consumption, including a full fleet with no orders.
static void test_fleet_scratch_admission(void) {
    const uint8_t partial[] = {0, 0, 0xff, 0xff, 1, 8, 0xff, 0xff, 0, 0x80, 0xff, 0x7f, 1, 0,
                               0xff, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
    const uint8_t full[] = {0, 0, 0, 0, 7, 8, 0xff, 0xff, 0xe8, 3, 0xe8, 3, 0, 0,
                            0, 0, 0, 0, 0xff, 0};
    uint8_t eof[] = {0, 0};
    FLEET fl;
    bool fRead;

    TEST_ASSERT(FStarsTestInit());
    memset(rgbCur, 0xcc, sizeof(rgbCur));
    memcpy(rgbCur, partial, sizeof(partial));
    hdrCur.rt = rtFleetB; hdrCur.cb = 0;
    vlpMemStream = eof;
    fRead = FReadFleet(&fl);
    TEST_CHECK(vlpMemStream == eof + 2);
    vlpMemStream = NULL;
    TEST_ASSERT(fRead);
    TEST_CHECK(fl.id == 0 && fl.iPlayer == -1 && fl.rgcsh[0] == 255 && !fl.fDead && fl.fDone);
    TEST_CHECK(fl.pt.x == -32768 && fl.pt.y == 32767 && fl.dirLong == 0x12345678 && fl.wtFleet() == -1);
    TEST_CHECK(fl.lpplord == NULL && hdrCur.rt == rtEOF);
    memcpy(rgbCur, full, sizeof(full));
    hdrCur.rt = rtFleetA; hdrCur.cb = 0;
    vlpMemStream = eof;
    fRead = FReadFleet(&fl);
    TEST_CHECK(vlpMemStream == eof + 2);
    vlpMemStream = NULL;
    TEST_ASSERT(fRead);
    TEST_CHECK(fl.fDead && fl.fDone && fl.cord == 0 && fl.iplan == 255 && fl.lpplord != NULL);
    TEST_CHECK(fl.lpplord->iordMac == 0 && fl.lpplord->iordMax == 1 && fl.lpplord->rgord()[0].grTask == 0);
    FreePl(fl.lpplord);
    // Wrong full-record tag fails after prefix/count/cargo publication, before
    // order allocation or consuming a following record.
    memcpy(rgbCur, full, sizeof(full));
    hdrCur.rt = rtFleetB; hdrCur.cb = 0;
    vlpMemStream = eof;
    fRead = FReadFleet(&fl);
    TEST_CHECK(vlpMemStream == eof);
    vlpMemStream = NULL;
    TEST_CHECK(!fRead && fl.det == detAll && fl.fDead && fl.lpplord == NULL && fl.iplan == 0);
}

// Fixed bytes are independent of the native metadata type. Failed import keeps
// the installed roster; matching partial knowledge retains an existing full design.
static void test_design_wire_admission(void) {
    SHDEF rgshdefT[16] = {}, before[16];
    uint8_t rgb[147] = {7, 8, ihuldefScout, 0xff, 0x34, 0x12, 0,
                        0xff, 0xff, 0x78, 0x56, 0x34, 0x12, 0, 0, 0, 0x80, 0, 0};
    const char szName[] = "abcdefghijklmnopqrstuvwxyz01234";
    char szPacked[64];
    int16_t cOut = 63;
    size_t cb;
    int16_t ibmp;
    uint8_t *pb;
    uint8_t rgbWritten[147];

    TEST_ASSERT(FStarsTestInit());
    idPlayer = iplrNone;
    memcpy(before, rgshdefT, sizeof(before));
    TEST_CHECK(!FReadShDef(NULL, rgshdefT, 0, 0));
    for (cb = 0; cb < sizeof(rgb); cb++) {
        if (cb < 19) {
            TEST_CHECK(!FReadShDef(rgb, rgshdefT, 0, cb));
            TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
        }
    }
    TEST_ASSERT(FReadShDef(rgb, rgshdefT, 0, 19));
    TEST_CHECK(rgshdefT[2].det == detAll && rgshdefT[2].ishdef == 2 && rgshdefT[2].hul.chs == 0);
    TEST_CHECK(rgshdefT[2].turn == 0xffff && rgshdefT[2].cBuilt == 0x12345678 && rgshdefT[2].cExist == 0x80000000);
    ibmp = (LphuldefFromId(ihuldefScout)->hul.ibmp | 3);
    TEST_CHECK(rgshdefT[2].hul.ibmp == ibmp);
    TEST_CHECK(rgshdefT[2].hul.wtEmpty == LphuldefFromId(ihuldefScout)->hul.wtEmpty);
    strcpy(rgshdefT[2].hul.szClass, "kept full knowledge");
    memcpy(before, rgshdefT, sizeof(before));
    rgb[0] = detSome; rgb[3] = (uint8_t)ibmp;
    memcpy(rgb + 6, "\0partial\0", 9);
    TEST_ASSERT(FReadShDef(rgb, rgshdefT, 0, 15));
    TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
    // Compressed partial names may have a marker above sixteen. It remains
    // hul.chs, rather than becoming a full-design slot validation rule.
    TEST_ASSERT(FCompressUserString(szName, szPacked, &cOut));
    TEST_ASSERT(cOut > 16 && cOut < 32);
    memset(rgshdefT, 0, sizeof(rgshdefT));
    rgb[6] = cOut;
    memcpy(rgb + 7, szPacked, cOut);
    TEST_ASSERT(FReadShDef(rgb, rgshdefT, 0, 7 + cOut));
    TEST_CHECK(rgshdefT[2].hul.chs == cOut && !strcmp(rgshdefT[2].hul.szClass, szName));
    TEST_CHECK(rgshdefT[2].hul.wtEmpty == 0x1234);
    // Partial export uses the catalog name and its six-byte prefix, ignoring
    // custom text and the non-slot marker retained in hul.chs.
    pb = rgbWritten;
    WriteRtShDef(&rgshdefT[2], &pb);
    memset(rgshdefT, 0, sizeof(rgshdefT));
    TEST_ASSERT(FReadShDef(rgbWritten, rgshdefT, 0, pb - rgbWritten));
    TEST_CHECK(!strcmp(rgshdefT[2].hul.szClass, LphuldefFromId(ihuldefScout)->hul.szClass));
    // Existing full-design count/hull rejection remains before publication.
    memcpy(before, rgshdefT, sizeof(before));
    rgb[0] = detAll; rgb[6] = 17;
    TEST_CHECK(!FReadShDef(rgb, rgshdefT, 0, sizeof(rgb)));
    rgb[6] = 0; rgb[2] = 0xff;
    TEST_CHECK(!FReadShDef(rgb, rgshdefT, 0, sizeof(rgb)));
    TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
}

// Exercise the real encrypted writer/reader with the original 34/35-token
// boundaries and an event split at byte 1023, including its odd-byte DV words.
static void test_battle_continuations(void) {
    struct BATTLECASE { int16_t ctok, cKill; int16_t rgcb[5]; int16_t cRec; };
    const BATTLECASE rgcase[] = {{34, 0, {1006}, 1}, {35, 0, {1000, 29, 6}, 3},
                                {69, 0, {1000, 1015, 6}, 3}, {70, 128, {1000, 1015, 29, 1023, 7}, 5}};
    char szDir[MAX_PATH], szFile[MAX_PATH];
    uint8_t *lpb;
    int16_t cb, i, j, ib;
    TOK tok{};
    BTLREC br{};
    KILL kill{};

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("battle_continuations", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%cbattle.bin", szDir, chDirSep);
    lpb = (uint8_t *)LpAlloc(0xffc8, htBattle);
    game.cPlayer = 1;
    for (const BATTLECASE &bcase : rgcase) {
        TEST_CASE_("%d tokens, %d kills", bcase.ctok, bcase.cKill);
        cb = 14 + bcase.ctok * 29 + 6 + bcase.cKill * 8;
        memset(lpb, 0, cb + 2);
        BdvFromLp(lpb).SetId(3);
        BdvFromLp(lpb).SetCtok(bcase.ctok);
        BdvFromLp(lpb).SetCplr(1);
        BdvFromLp(lpb).SetGrfPlr(1);
        BdvFromLp(lpb).SetCbData(cb);
        BdvFromLp(lpb).SetIdPlanet(idPlanetNone);
        BdvFromLp(lpb).SetPt({32767, -1});
        for (i = 0; i < bcase.ctok; i++) {
            tok.id = i; tok.iplr = 0; tok.csh = 0x1234 + i; tok.dpShield = 0x5678 + i;
            tok.dv.dp = 0x9abc + i;
            TEST_ASSERT(FEncodeTok(tok, std::as_writable_bytes(std::span(lpb + 14 + i * 29, 29))));
        }
        br.itok = 33; br.brcDest = 0xab; br.ctok = bcase.cKill; br.itokAttack = 34; br.iRound = 15; br.dzDis = 4;
        ib = 14 + bcase.ctok * 29;
        TEST_ASSERT(FEncodeBattleRecord(br, std::as_writable_bytes(std::span(lpb + ib, 6))));
        for (i = 0; i < bcase.cKill; i++) {
            kill.itok = i % bcase.ctok; kill.grfWeapon = i; kill.cshKill = 0x1234 + i;
            kill.dpShield = 0x5678 + i; kill.dv.dp = 0x9abc + i;
            TEST_ASSERT(FEncodeKill(kill, std::as_writable_bytes(std::span(lpb + ib + 6 + i * 8, 8))));
        }
        lpb[cb] = lpb[cb + 1] = 0xff;
        lpbBattleLog = lpb; lpbBattleCur = lpb + cb;
        StreamOpen(szFile, mdCreate);
        SetFileXorStream(0x12345678, 0x4321, 123, 0, FALSE);
        WriteBattles(0);
        WriteRt(rtEOF, 0, NULL);
        StreamClose();
        StreamOpen(szFile, mdRead);
        SetFileXorStream(0x12345678, 0x4321, 123, 0, FALSE);
        ib = 0;
        for (j = 0; j < bcase.cRec; j++) {
            ReadRt();
            TEST_ASSERT(hdrCur.rt == (j == 0 ? rtBtlData : rtContinue));
            TEST_ASSERT(hdrCur.cb == bcase.rgcb[j]);
            TEST_CHECK(memcmp(rgbCur, lpb + ib, hdrCur.cb) == 0);
            ib += hdrCur.cb;
        }
        TEST_CHECK(ib == cb);
        ReadRt();
        TEST_CHECK(hdrCur.rt == rtEOF);
        StreamClose();
    }
    FreeHb(rglphb[htBattle]);
    rglphb[htBattle] = NULL;
    lpbBattleLog = lpbBattleCur = NULL;
}

static void test_battle_plan_deletion_wire(void) {
    BTLPLAN plan{};
    HDR hdr;
    uint8_t rgb[64];
    const uint8_t deleted[] = {0xd3, 0xf2};

    TEST_ASSERT(FStarsTestInit());
    plan.iplr = 3; plan.iplan = 13; plan.mdTactic = 2; plan.unused1 = 3; plan.fDelete = 1; plan.fDumpCargo = 1;
    plan.mdTarget1 = 8; plan.mdTarget2 = 7; plan.iplrAttack = 31; plan.unused2 = 7;
    WriteBattlePlan(&plan, TRUE);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtBtlPlan && hdr.cb == 2 && memcmp(rgb, deleted, sizeof(deleted)) == 0);
}

TEST_LIST = {{"battle-plan deletion encodes only its two-byte header", test_battle_plan_deletion_wire},
             {"battle continuation bytes and boundaries", test_battle_continuations},
             {"fleet readers preserve scratch admission and incremental publication", test_fleet_scratch_admission},
             {"ship-design bytes preserve import admission and knowledge", test_design_wire_admission},
             {"UTF-8 records require complete in-record text", test_utf8_record_bounds},
             {"maximum raw UTF-8 battle-plan names round trip", test_utf8_raw_battle_plan},
             {"design and battle-plan readers respect text record boundaries", test_utf8_design_and_plan_bounds},
             {"UTF-8 saved names and byte capacities", test_utf8_saved_names},
             {"SetVisPFInit host sentinel", test_SetVisPFInit_host},
             {"FWriteDataFile Claim Adjuster sees only habitat", test_FWriteDataFile_claim_adjuster_sees_only_habitat},
             {"SetVisPFFleets sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase},
             {"SetVisPFFleets long-range scanner sees a cloaked starbase", test_SetVisPFFleets_cloaked_starbase_long_range},
             {"WriteBOF writes version 2.85", test_WriteBOF_writes_version_2_85},
             {"FOpenFile reports unsupported versions and corrupt headers", test_FOpenFile_reports_unsupported_versions_and_corruption},
             {"FWriteDataFile masks other players' things", test_FWriteDataFile_masks_other_players_things},
             {NULL, NULL}};
