#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
[[maybe_unused]] static char szTestLiteral1[] = "\xf0\x9f\x98\x80";

#include "stars_test.h"
#include <algorithm>
#include <iterator>
#include "record_io.h"

static bool fCloseSizeSource;
static int cClosedSizeSources;
static void OpenSizeSource(APPSESSION &app, const char *psz, MdOpenFlags md) {
    StreamOpen(app, psz, md);
    fCloseSizeSource = std::string_view(psz).ends_with(".m1");
}
static void ReadSizeSource(APPSESSION &app) {
    ReadRt(app);
    if (fCloseSizeSource && app.state.hdrCur.rt == rtEOF) {
        StreamClose(app);
        cClosedSizeSources++;
    }
}
static void test_Load_requires_size_result(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("load-size-result", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, nullptr, 0));
    DestroyCurGame(app);
    fCloseSizeSource = false; cClosedSizeSources = 0;
    RECORDIO io = RecordIO();
    io.pfnStreamOpen = OpenSizeSource;
    io.pfnReadRt = ReadSizeSource;
    RECORDIOSCOPE recordScope(io);
    TEST_CHECK(!FLoadGame(app, app.state.szBase, "m1"));
    TEST_CHECK(cClosedSizeSources == 1 && app.state.hf == -1 && !FStarsErrorPending());
}

// ScanCloakedStarbase puts a player-0 fleet with a dScanRange2 penetrating
// scanner 100 ly from player 1's homeworld, whose starbase design has
// lVisible lVis2, and returns what SetVisPFFleets lets player 0 see of it.
static DetType DetScanCloakedStarbase(APPSESSION &app, const char *pszTest, int16_t dScanRange2, int32_t lVis2) {
    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl0;
    PLANET     *lppl1;
    FLEET      *lpfl;
    int16_t     ish;
    int16_t     i;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    lppl0 = LpplStarsTestHomeworld(app, 0);
    lppl1 = LpplStarsTestHomeworld(app, 1);
    TEST_ASSERT(lppl0 != NULL && lppl1 != NULL && lppl1->fStarbase);

    for (ish = 0; ish < 16 && app.state.rglpshdef[0][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < 16);
    app.state.rglpshdef[0][ish].dScanRange = 0;
    app.state.rglpshdef[0][ish].dScanRange2 = dScanRange2;
    lpfl = LpflStarsTestAddFleet(app, 0, lppl0->id, ish, 1);
    lpfl->idPlanet = idPlanetDeepSpace;
    lpfl->pt.x = app.state.rgptPlan[lppl1->id].x + 100;
    lpfl->pt.y = app.state.rgptPlan[lppl1->id].y;
    app.state.rglpshdefSB[1][lppl1->isb].SetLVisible(lVis2);

    for (i = 0; i < app.state.cPlanet; i++) {
        app.state.lpPlanets[i].fInclude = 0;
        app.state.lpPlanets[i].det = detNone;
    }
    app.state.gd.fGeneratingTurn = 1;
    SetVisPFFleets(app, 0);
    return lppl1->fInclude ? (DetType)lppl1->det : detNone;
}

// A starbase cloaked to lVisible 2500 (seen at half range) 100 ly away is
// within a 600 ly scanner's reduced 300 ly.
static void test_SetVisPFFleets_cloaked_starbase(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    DetType det;

    det = DetScanCloakedStarbase(app, "SetVisPFFleets_cloaked_starbase", 600, 2500);
    TEST_CHECK_(det >= detSome, "det %d", det);
}

// ISB vs scanning: lRadius2 * lVis2 overflowed 32 bits for scanners over
// about 463 ly, so a lightly cloaked starbase was only obscured.
static void test_SetVisPFFleets_cloaked_starbase_long_range(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    DetType det;

    det = DetScanCloakedStarbase(app, "SetVisPFFleets_cloaked_starbase_long_range", 600, 9000);
    TEST_CHECK_(det >= detSome, "det %d", det);
}

// Claim Adjuster turn files: other players' records were written at full
// detail so a CA player gets their habitat, which also sent their tech,
// research, traits, production template and relations.
static void test_FWriteDataFile_claim_adjuster_sees_only_habitat(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const int8_t rgAttrZero[16] = {}, rgTechZero[6] = {};
    const uint32_t rgResZero[6] = {};

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl1;
    PLAYER      plrAi;
    int16_t     ish;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("FWriteDataFile_claim_adjuster", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    SetRaceStat(&app.state.rgplr[0], rsMajorAdv, raTerra);
    lppl1 = LpplStarsTestHomeworld(app, 1);
    TEST_ASSERT(lppl1 != NULL);
    for (ish = 0; ish < 16 && app.state.rglpshdef[0][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < 16);
    LpflStarsTestAddFleet(app, 0, lppl1->id, ish, 1);
    TEST_ASSERT(FStarsTestSaveHost(app));
    TEST_ASSERT(FStarsTestGenerate(app));

    TEST_ASSERT(FStarsTestLoadHost(app));
    plrAi = app.state.rgplr[1];
    TEST_ASSERT(plrAi.grbitAttr != 0 || plrAi.rgAttr[0] != 0);
    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    TEST_ASSERT_(app.state.rgplr[1].fInclude, "player 0 doesn't see the AI");
    TEST_CHECK(memcmp(app.state.rgplr[1].rgEnvVar, plrAi.rgEnvVar, 3) == 0);
    TEST_CHECK(memcmp(app.state.rgplr[1].rgEnvVarMin, plrAi.rgEnvVarMin, 3) == 0);
    TEST_CHECK(memcmp(app.state.rgplr[1].rgEnvVarMax, plrAi.rgEnvVarMax, 3) == 0);
    TEST_CHECK_(app.state.rgplr[1].grbitAttr == 0, "traits %#lx", (unsigned long)app.state.rgplr[1].grbitAttr);
    TEST_CHECK(memcmp(app.state.rgplr[1].rgAttr, rgAttrZero, 16) == 0);
    TEST_CHECK(memcmp(app.state.rgplr[1].rgTech, rgTechZero, 6) == 0);
    TEST_CHECK(memcmp(app.state.rgplr[1].rgResSpent, rgResZero, sizeof(plrAi.rgResSpent)) == 0);
    TEST_CHECK(app.state.rgplr[1].pctResearch == 0 && app.state.rgplr[1].lResLastYear == 0 && app.state.rgplr[1].pctIdealGrowth == 0);
}

// Format 2.85 is UTF-8; older and newer formats are refused.
static int16_t FSetHostVersion(APPSESSION &app, int16_t verMinor) {
    char  szFile[cbTestPath];
    uint8_t rgbVersion[2];
    uint16_t wVersion;
    FILE *fp;

    snprintf(szFile, sizeof(szFile), "%s.hst", app.state.szBase);
    fp = fopen(szFile, "r+b");
    if (fp == NULL)
        return 0;
    // The RTBOF record follows its 2-byte header and isn't encrypted.
    fseek(fp, 2 + 8, SEEK_SET);
    if (fread(rgbVersion, sizeof(rgbVersion), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    wVersion = rgbVersion[0] | (uint16_t(rgbVersion[1]) << 8);
    if (verMinor < 0) {
        fclose(fp);
        return (wVersion >> 12) == 2 && ((wVersion >> 5) & 127) == 85 && (wVersion & 31) == 0;
    }
    wVersion = (wVersion & 0xf01f) | ((uint16_t(verMinor) & 127) << 5);
    rgbVersion[0] = (uint8_t)wVersion; rgbVersion[1] = (uint8_t)(wVersion >> 8);
    fseek(fp, 2 + 8, SEEK_SET);
    fwrite(rgbVersion, sizeof(rgbVersion), 1, fp);
    fclose(fp);
    return 1;
}

static void test_WriteBOF_writes_version_2_85(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("WriteBOF_version", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_CHECK_(FSetHostVersion(app, -1), "host file isn't 2.85");

    TEST_ASSERT(FSetHostVersion(app, 83));
    TEST_CHECK_(!FStarsTestLoadHost(app), "a 2.83 host file loaded");
    TEST_ASSERT(FSetHostVersion(app, 86));
    TEST_CHECK_(!FStarsTestLoadHost(app), "a 2.86 host file loaded");
}

static void test_FOpenFile_reports_unsupported_versions_and_corruption(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char  szDir[cbTestPath];
    char  szFile[cbTestPath];
    FILE *fp;
    HDR   hdr;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("FOpenFile_versions", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    app.state.idsFileError = 0;
    TEST_CHECK_(FStarsTestLoadHost(app), "current-format host file was rejected");
    TEST_CHECK(app.state.idsFileError == 0);

    TEST_ASSERT(FSetHostVersion(app, 83));
    app.state.idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost(app));
    TEST_CHECK(app.state.idsFileError == idsSorryFileCreatedOlderVersionStarsIncompatible);

    TEST_ASSERT(FSetHostVersion(app, 86));
    app.state.idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost(app));
    TEST_CHECK(app.state.idsFileError == idsFileCreatedNewerVersionStarsMustUpgrade);

    TEST_ASSERT(FSetHostVersion(app, 85));
    snprintf(szFile, sizeof(szFile), "%s.hst", app.state.szBase);
    fp = fopen(szFile, "r+b");
    TEST_ASSERT(fp != NULL);
    hdr.rt = rtBOF;
    hdr.cb = 1;
    TEST_CHECK(fwrite(&hdr, sizeof(hdr), 1, fp) == 1);
    fclose(fp);
    app.state.idsFileError = 0;
    TEST_CHECK(!FStarsTestLoadHost(app));
    TEST_CHECK(app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
}

// LpthFind returns the loaded game's thing of type ith whose owner
// is iplr, or NULL.
static THING *LpthFind(APPSESSION &app, ThingType ith, int16_t iplr) {
    int16_t i;

    for (i = 0; i < app.state.cThing; i++) {
        if (app.state.lpThings[i].ith() == ith && app.state.lpThings[i].iplr() == iplr)
            return &app.state.lpThings[i];
    }
    return NULL;
}

// Turn-file knowledge leaks: a player's turn file carried whole THING
// records, so it showed which other players had seen a minefield, the
// Mystery Trader or a wormhole, who had travelled through the wormhole,
// and the part the trader carries.
static void test_FWriteDataFile_masks_other_players_things(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl;
    THING      *lpth;
    POINT16     pt;
    uint16_t    idWorm1;
    uint16_t    idWorm2;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("FWriteDataFile_things", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    lppl = LpplStarsTestHomeworld(app, 0);
    TEST_ASSERT(lppl != NULL);
    pt = app.state.rgptPlan[lppl->id];
    // Everything sits on player 0's homeworld, where its scanner sees it.
    while ((lpth = LpthFind(app, ithWormhole, 0)) != NULL)
        FreeLpth(app, lpth);
    lpth = LpthNew(app, 1, ithMinefield);
    lpth->pt().Set(pt);
    lpth->thm().SetCMines(2500);
    lpth->thm().SetIType(mineStandard);
    lpth->thm().SetGrbitPlr(1 << 1);
    lpth->thm().SetGrbitPlrNow(1 << 1);
    lpth = LpthNew(app, 0, ithMysteryTrader);
    lpth->pt().Set(pt);
    lpth->tht().ptDest().Set(pt);
    lpth->tht().SetGrbitPlr(1 << 1);
    lpth->tht().SetGrbitTrader(grbitTraderBeam);
    lpth = LpthNew(app, 0, ithWormhole);
    lpth->pt().Set(pt);
    { const auto grbitPlrForThing = ((1 << 0) | (1 << 1)); auto &thForThing = *(lpth); thForThing.thw().SetGrbitPlr(grbitPlrForThing); }
    lpth->thw().SetGrbitPlrTrav(1 << 1);
    idWorm1 = lpth->idFull();
    lpth = LpthNew(app, 0, ithWormhole);
    lpth->pt().SetX(pt.x + 200);
    lpth->pt().SetY(pt.y);
    lpth->thw().SetGrbitPlr(1 << 1);
    lpth->thw().SetGrbitPlrTrav(1 << 1);
    idWorm2 = lpth->idFull();
    lpth->thw().SetIdPartner(idWorm1);
    { const auto idPartnerForThing = (idWorm2); auto &thForThing = *(LpthFromId(app, idWorm1)); thForThing.thw().SetIdPartner(idPartnerForThing); }
    TEST_ASSERT(FWriteDataFile(app, app.state.szBase, 0, 0));

    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    lpth = LpthFind(app, ithMinefield, 1);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the minefield");
    TEST_CHECK_(lpth->thm().grbitPlr() == 1 && lpth->thm().grbitPlrNow() == 1, "minefield seen by %#x, now %#x", lpth->thm().grbitPlr(), lpth->thm().grbitPlrNow());
    lpth = LpthFind(app, ithMysteryTrader, 0);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the trader");
    TEST_CHECK_(lpth->tht().grbitPlr() == 0 && lpth->tht().grbitTrader() == 0, "trader met by %#x, carrying %#x", lpth->tht().grbitPlr(), lpth->tht().grbitTrader());
    lpth = LpthFind(app, ithWormhole, 0);
    TEST_ASSERT_(lpth != NULL, "player 0 doesn't see the wormhole");
    TEST_CHECK_(lpth->thw().grbitPlr() == 1 && lpth->thw().grbitPlrTrav() == 0, "wormhole seen by %#x, travelled by %#x", lpth->thw().grbitPlr(), lpth->thw().grbitPlrTrav());
}

static void test_SetVisPFInit_host(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    THING th;
    SHDEF rgshdef[16], rgshdefSB[10];

    TEST_ASSERT(FStarsTestInit(app));
    memset(&th, 0, sizeof(th));
    memset(rgshdef, 0, sizeof(rgshdef));
    memset(rgshdefSB, 0, sizeof(rgshdefSB));
    app.state.rglpshdef[0] = rgshdef;
    app.state.rglpshdefSB[0] = rgshdefSB;
    th.SetIth(ithMineralPacket);
    th.SetIplr(0);
    app.state.lpThings = &th;
    app.state.cThing = 1;
    app.state.game.cPlayer = 1;
    SetVisPFInit(app, iplrNone);
    TEST_CHECK(app.state.rgplr[0].fInclude);
    TEST_CHECK(app.state.rgplr[0].det == detAll);
    TEST_CHECK(th.thp().fInclude());
    app.state.lpThings = NULL;
    app.state.cThing = 0;
    app.state.rglpshdef[0] = app.state.rglpshdefSB[0] = NULL;
}

static void test_utf8_saved_names(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szExpected[32], szPacked[64], szRound[32];
    int16_t cPacked = sizeof(szPacked), cRound = 31;
    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("utf8_saved_names", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost(app));
    memset(szExpected, 'a', 27);
    memcpy(szExpected + 27, "\xf0\x9f\x98\x80", 5);
    strcpy(app.state.rgplr[0].szName, szExpected);
    strcpy(app.state.rgplr[0].szNames, "caf\xc3\xa9-\xce\xa9");
    const auto cbPacked1 = FCompressUserString(app, szExpected, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked1) cPacked = static_cast<std::int16_t>(*cbPacked1);
    TEST_ASSERT(cbPacked1);
    TEST_ASSERT(FDecompressUserString(app, std::as_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))), std::span(szRound, static_cast<std::size_t>(cRound) + 1)));
    TEST_CHECK(strcmp(szRound, szExpected) == 0);
    TEST_ASSERT(FStarsTestSaveHost(app));
    TEST_ASSERT(FStarsTestLoadHost(app));
    TEST_CHECK(strcmp(app.state.rgplr[0].szName, szExpected) == 0);
    TEST_CHECK(strcmp(app.state.rgplr[0].szNames, "caf\xc3\xa9-\xce\xa9") == 0);
}

static void test_utf8_record_bounds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t rgb[64];
    char sz[32];
    std::optional<size_t> cbRead;
    int16_t cOut;

    memcpy(rgb, "\0\xc3\xa9\0", 4);
    TEST_CHECK(!FReadUserString(app, std::as_bytes(std::span(rgb, 3)), std::span{sz}.first(sizeof(sz))));
    TEST_CHECK(sz[0] == 0);
    TEST_CHECK((cbRead = FReadUserString(app, std::as_bytes(std::span(rgb, 4)), std::span{sz}.first(sizeof(sz)))));
    TEST_CHECK(cbRead == 4 && !strcmp(sz, "\xc3\xa9"));
    rgb[1] = 0xff;
    TEST_CHECK(!FReadUserString(app, std::as_bytes(std::span(rgb, 4)), std::span{sz}.first(sizeof(sz))));
    cOut = 63;
    const auto cbPacked2 = FCompressUserString(app, szTestLiteral1, std::as_writable_bytes(std::span((char *)rgb + 1, static_cast<std::size_t>(cOut))));
    if (cbPacked2) cOut = static_cast<std::int16_t>(*cbPacked2);
    TEST_ASSERT(cbPacked2);
    rgb[0] = cOut;
    TEST_CHECK(!FReadUserString(app, std::as_bytes(std::span(rgb, cOut)), std::span{sz}.first(sizeof(sz))));
    TEST_CHECK((cbRead = FReadUserString(app, std::as_bytes(std::span(rgb, cOut + 1)), std::span{sz}.first(sizeof(sz)))));
    TEST_CHECK(cbRead == (size_t)cOut + 1 && !strcmp(sz, "\xf0\x9f\x98\x80"));
    rgb[0] = 1; rgb[1] = 0x0b;
    TEST_CHECK(!FReadUserString(app, std::as_bytes(std::span(rgb, 2)), std::span{sz}.first(sizeof(sz))));
    TEST_CHECK(sz[0] == 0);
    rgb[1] = 0xff;
    TEST_CHECK(!FReadUserString(app, std::as_bytes(std::span(rgb, 2)), std::span{sz}.first(sizeof(sz))));
}

static void test_utf8_design_and_plan_bounds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t rgb[64];
    SHDEF rgshdefT[16];
    BTLPLAN btlplan;
    const uint8_t *lprt = rgb;

    TEST_ASSERT(FStarsTestInit(app));
    app.state.idPlayer = iplrNone;
    memset(rgb, 0, sizeof(rgb));
    memset(rgshdefT, 0, sizeof(rgshdefT));
    rgb[0] = detSome; rgb[2] = ihuldefScout;
    memcpy(rgb + cbWireShDefPartial, "\0\xc3\xa9\0", 4);
    TEST_CHECK(!FReadShDef(app, lprt, rgshdefT, 0, cbWireShDefPartial + 3));
    TEST_CHECK(FReadShDef(app, lprt, rgshdefT, 0, cbWireShDefPartial + 4));
    TEST_CHECK(!strcmp(rgshdefT[0].hul.szClass, "\xc3\xa9"));
    memset(rgb, 0, sizeof(rgb));
    memcpy(rgb + 4, "\0\xc3\xa9\0", 4);
    app.state.idsFileError = 0;
    memset(&btlplan, 0x55, sizeof(btlplan));
    TEST_CHECK(!UnpackBattlePlan(app, rgb, &btlplan, 0, 7));
    TEST_CHECK(btlplan.szName[0] == 0x55);
    TEST_CHECK(app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    app.state.idsFileError = 0;
    TEST_CHECK(UnpackBattlePlan(app, rgb, &btlplan, 0, 8));
    TEST_CHECK(app.state.idsFileError == 0 && !strcmp(btlplan.szName, "\xc3\xa9"));
}

static int cNameRecords;
static void CountNameRecord(APPSESSION &, RecordType, std::span<const std::byte>) { cNameRecords++; }
static void test_invalid_written_names(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    PLAYER plr{};
    SHDEF shdef{};
    BTLPLAN plan{};
    HDR hdr;
    uint8_t rgb[64];
    TEST_ASSERT(FStarsTestInit(app));
    RECORDIO io = RecordIO();
    io.pfnWriteRt = CountNameRecord;
    RECORDIOSCOPE recordScope(io);
    cNameRecords = 0;
    plr.det = detAll;
    memset(plr.szName, 'x', sizeof(plr.szName));
    app.state.idsFileError = 0;
    WriteRtPlr(app, &plr);
    TEST_CHECK(cNameRecords == 0 && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    plr.szName[0] = '\xff'; plr.szName[1] = 0;
    app.state.idsFileError = 0;
    WriteRtPlr(app, &plr);
    TEST_CHECK(cNameRecords == 0 && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    shdef.det = detAll;
    shdef.hul.ihuldef = ihuldefScout;
    memset(shdef.hul.szClass, 'x', sizeof(shdef.hul.szClass));
    app.state.idsFileError = 0;
    WriteRtShDef(app, &shdef);
    TEST_CHECK(cNameRecords == 0 && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    memset(plan.szName, 'x', sizeof(plan.szName));
    app.state.idsFileError = 0;
    WriteBattlePlan(app, &plan, 0);
    TEST_CHECK(cNameRecords == 0 && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    app.state.idsFileError = 0;
    WriteBattlePlan(app, &plan, 1);
    TEST_CHECK(!FGetPrevLogRt(app, &hdr, rgb) && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    app.state.idsFileError = 0;
    WriteRtString(app, "\xff");
    TEST_CHECK(cNameRecords == 0 && app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
}

static void test_design_encoder_spans(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    SHDEF shdef{}, decoded[16]{};
    std::byte reference[147], rgb[154], expected[154];
    size_t cb;
    const uint8_t *lprt;

    TEST_ASSERT(FStarsTestInit(app));
    app.state.idPlayer = iplrNone;
    shdef.det = detAll;
    shdef.ishdef = 2;
    shdef.hul.ihuldef = ihuldefScout;
    for (int iName = 0; iName < 3; iName++) {
        shdef.hul.chs = iName == 2 ? 16 : 0;
        memset(shdef.hul.szClass, 0, sizeof(shdef.hul.szClass));
        if (iName == 1) strcpy(shdef.hul.szClass, "Scout");
        if (iName == 2) {
            for (int i = 0; i < 10; i++) memcpy(shdef.hul.szClass + i * 3, "\xe6\x9d\xb1", 3);
            shdef.hul.szClass[30] = 'X';
        }
        const auto cbName = CbEncodeRtShDef(app, &shdef, reference);
        const size_t ibName = cbWireShDefFull + shdef.hul.chs * cbWireHs;
        TEST_ASSERT(cbName);
        if (iName == 0) TEST_CHECK(*cbName == cbWireShDefFull + 2 && reference[cbWireShDefFull] == std::byte{0});
        if (iName == 2) {
            TEST_CHECK(*cbName == ibName + 33 && reference[ibName] == std::byte{0});
            TEST_CHECK(!memcmp(reference + ibName + 1, shdef.hul.szClass, 32));
        }
        for (cb = 0; cb <= *cbName + 2; cb++) {
            std::fill(std::begin(rgb), std::end(rgb), std::byte{0x55});
            std::fill(std::begin(expected), std::end(expected), std::byte{0x55});
            const auto written = CbEncodeRtShDef(app, &shdef, std::span{rgb}.subspan(3, cb));
            if (cb < *cbName) TEST_CHECK(!written);
            else {
                TEST_CHECK(written && *written == *cbName);
                memcpy(expected + 3, reference, *cbName);
            }
            TEST_CHECK(!memcmp(rgb, expected, sizeof(rgb)));
        }
        memset(decoded, 0, sizeof(decoded));
        lprt = reinterpret_cast<const uint8_t *>(reference);
        TEST_ASSERT(FReadShDef(app, lprt, decoded, 0, *cbName));
        TEST_CHECK(!strcmp(decoded[2].hul.szClass, shdef.hul.szClass));
    }
    memset(shdef.hul.szClass, 'x', sizeof(shdef.hul.szClass));
    std::fill(std::begin(rgb), std::end(rgb), std::byte{0x55});
    TEST_CHECK(!CbEncodeRtShDef(app, &shdef, rgb));
    TEST_CHECK(std::all_of(std::begin(rgb), std::end(rgb), [](std::byte ch) { return ch == std::byte{0x55}; }));
    shdef.hul.szClass[0] = '\xff'; shdef.hul.szClass[1] = 0;
    TEST_CHECK(!CbEncodeRtShDef(app, &shdef, rgb));
    TEST_CHECK(std::all_of(std::begin(rgb), std::end(rgb), [](std::byte ch) { return ch == std::byte{0x55}; }));
    // A partial design exports the catalog name and ignores custom-name bytes.
    shdef.det = detSome;
    memset(shdef.hul.szClass, 'x', sizeof(shdef.hul.szClass));
    TEST_CHECK(CbEncodeRtShDef(app, &shdef, rgb).has_value());
}

static void test_utf8_raw_battle_plan(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    BTLPLAN btlplan = {};
    BTLPLAN btlplanRead;
    HDR hdr;
    uint8_t rgb[64];
    char szPacked[32];
    int16_t cOut = 31;
    int i;

    TEST_ASSERT(FStarsTestInit(app));
    for (i = 0; i < 10; i++) memcpy(btlplan.szName + i * 3, "\xe6\x9d\xb1", 3);
    btlplan.szName[30] = 'X'; btlplan.szName[31] = 0;
    const auto cbPacked3 = FCompressUserString(app, btlplan.szName, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cOut))));
    if (cbPacked3) cOut = static_cast<std::int16_t>(*cbPacked3);
    TEST_ASSERT(!cbPacked3);
    WriteBattlePlan(app, &btlplan, 1);
    TEST_ASSERT(FGetPrevLogRt(app, &hdr, rgb));
    TEST_CHECK(hdr.rt == rtBtlPlan && hdr.cb == sizeof(BTLPLAN) + 1 && rgb[4] == 0);
    TEST_ASSERT(UnpackBattlePlan(app, rgb, &btlplanRead, 0, hdr.cb));
    TEST_CHECK(!strcmp(btlplanRead.szName, btlplan.szName));
}



// The file reader historically borrows the complete reusable scratch backing,
// even when the current record declares zero bytes. Keep that admission and the
// exact next-record consumption, including a full fleet with no orders.
static void test_fleet_scratch_admission(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t partial[] = {0, 0, 0xff, 0xff, 1, 8, 0xff, 0xff, 0, 0x80, 0xff, 0x7f, 1, 0,
                               0xff, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
    const uint8_t full[] = {0, 0, 0, 0, 7, 8, 0xff, 0xff, 0xe8, 3, 0xe8, 3, 0, 0,
                            0, 0, 0, 0, 0xff, 0};
    uint8_t eof[] = {0, 0};
    FLEET fl;
    bool fRead;

    TEST_ASSERT(FStarsTestInit(app));
    memset(app.state.rgbCur, 0xcc, sizeof(app.state.rgbCur));
    memcpy(app.state.rgbCur, partial, sizeof(partial));
    app.state.hdrCur.rt = rtFleetB; app.state.hdrCur.cb = 0;
    app.state.vlpMemStream = std::as_bytes(std::span{eof});
    fRead = FReadFleet(app, &fl);
    TEST_CHECK(app.state.vlpMemStream->data() == reinterpret_cast<const std::byte *>(eof + 2));
    app.state.vlpMemStream.reset();
    TEST_ASSERT(fRead);
    TEST_CHECK(fl.id == 0 && fl.iPlayer == -1 && fl.rgcsh[0] == 255 && !fl.fDead && fl.fDone);
    TEST_CHECK(fl.pt.x == -32768 && fl.pt.y == 32767 && fl.dirLong == 0x12345678 && fl.wtFleet() == -1);
    TEST_CHECK(fl.lpplord == NULL && app.state.hdrCur.rt == rtEOF);
    memcpy(app.state.rgbCur, full, sizeof(full));
    app.state.hdrCur.rt = rtFleetA; app.state.hdrCur.cb = 0;
    app.state.vlpMemStream = std::as_bytes(std::span{eof});
    fRead = FReadFleet(app, &fl);
    TEST_CHECK(app.state.vlpMemStream->data() == reinterpret_cast<const std::byte *>(eof + 2));
    app.state.vlpMemStream.reset();
    TEST_ASSERT(fRead);
    TEST_CHECK(fl.fDead && fl.fDone && fl.cord == 0 && fl.iplan == 255 && fl.lpplord != NULL);
    TEST_CHECK(fl.lpplord->iordMac == 0 && fl.lpplord->iordMax == 1 && fl.lpplord->rgord()[0].grTask == 0);
    FreePl(app, fl.lpplord);
    // Wrong full-record tag fails after prefix/count/cargo publication, before
    // order allocation or consuming a following record.
    memcpy(app.state.rgbCur, full, sizeof(full));
    app.state.hdrCur.rt = rtFleetB; app.state.hdrCur.cb = 0;
    app.state.vlpMemStream = std::as_bytes(std::span{eof});
    fRead = FReadFleet(app, &fl);
    TEST_CHECK(app.state.vlpMemStream->data() == reinterpret_cast<const std::byte *>(eof));
    app.state.vlpMemStream.reset();
    TEST_CHECK(!fRead && fl.det == detAll && fl.fDead && fl.lpplord == NULL && fl.iplan == 0);
}

// Fixed bytes are independent of the native metadata type. Failed import keeps
// the installed roster; matching partial knowledge retains an existing full design.
static void test_design_wire_admission(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

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

    TEST_ASSERT(FStarsTestInit(app));
    app.state.idPlayer = iplrNone;
    memcpy(before, rgshdefT, sizeof(before));
    TEST_CHECK(!FReadShDef(app, NULL, rgshdefT, 0, 0));
    for (cb = 0; cb < sizeof(rgb); cb++) {
        if (cb < 19) {
            TEST_CHECK(!FReadShDef(app, rgb, rgshdefT, 0, cb));
            TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
        }
    }
    TEST_ASSERT(FReadShDef(app, rgb, rgshdefT, 0, 19));
    TEST_CHECK(rgshdefT[2].det == detAll && rgshdefT[2].ishdef == 2 && rgshdefT[2].hul.chs == 0);
    TEST_CHECK(rgshdefT[2].turn == 0xffff && rgshdefT[2].cBuilt == 0x12345678 && rgshdefT[2].cExist == 0x80000000);
    ibmp = (LphuldefFromId(ihuldefScout)->hul.ibmp | 3);
    TEST_CHECK(rgshdefT[2].hul.ibmp == ibmp);
    TEST_CHECK(rgshdefT[2].hul.wtEmpty == LphuldefFromId(ihuldefScout)->hul.wtEmpty);
    strcpy(rgshdefT[2].hul.szClass, "kept full knowledge");
    memcpy(before, rgshdefT, sizeof(before));
    rgb[0] = detSome; rgb[3] = (uint8_t)ibmp;
    memcpy(rgb + 6, "\0partial\0", 9);
    TEST_ASSERT(FReadShDef(app, rgb, rgshdefT, 0, 15));
    TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
    // Compressed partial names may have a marker above sixteen. It remains
    // hul.chs, rather than becoming a full-design slot validation rule.
    const auto cbPacked4 = FCompressUserString(app, szName, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cOut))));
    if (cbPacked4) cOut = static_cast<std::int16_t>(*cbPacked4);
    TEST_ASSERT(cbPacked4);
    TEST_ASSERT(cOut > 16 && cOut < 32);
    memset(rgshdefT, 0, sizeof(rgshdefT));
    rgb[6] = cOut;
    memcpy(rgb + 7, szPacked, cOut);
    TEST_ASSERT(FReadShDef(app, rgb, rgshdefT, 0, 7 + cOut));
    TEST_CHECK(rgshdefT[2].hul.chs == cOut && !strcmp(rgshdefT[2].hul.szClass, szName));
    TEST_CHECK(rgshdefT[2].hul.wtEmpty == 0x1234);
    // Partial export uses the catalog name and its six-byte prefix, ignoring
    // custom text and the non-slot marker retained in hul.chs.
    pb = rgbWritten;
    const auto cbWritten = CbEncodeRtShDef(app, &rgshdefT[2], std::as_writable_bytes(std::span{rgbWritten}));
    TEST_ASSERT(cbWritten);
    pb += *cbWritten;
    memset(rgshdefT, 0, sizeof(rgshdefT));
    TEST_ASSERT(FReadShDef(app, rgbWritten, rgshdefT, 0, pb - rgbWritten));
    TEST_CHECK(!strcmp(rgshdefT[2].hul.szClass, LphuldefFromId(ihuldefScout)->hul.szClass));
    // Existing full-design count/hull rejection remains before publication.
    memcpy(before, rgshdefT, sizeof(before));
    rgb[0] = detAll; rgb[6] = 17;
    TEST_CHECK(!FReadShDef(app, rgb, rgshdefT, 0, sizeof(rgb)));
    rgb[6] = 0; rgb[2] = 0xff;
    TEST_CHECK(!FReadShDef(app, rgb, rgshdefT, 0, sizeof(rgb)));
    TEST_CHECK(memcmp(rgshdefT, before, sizeof(before)) == 0);
}

// Exercise the real encrypted writer/reader with the original 34/35-token
// boundaries and an event split at byte 1023, including its odd-byte DV words.
static void test_battle_continuations(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    struct BATTLECASE { int16_t ctok, cKill; int16_t rgcb[5]; int16_t cRec; };
    const BATTLECASE rgcase[] = {{34, 0, {1006}, 1}, {35, 0, {1000, 29, 6}, 3},
                                {69, 0, {1000, 1015, 6}, 3}, {70, 128, {1000, 1015, 29, 1023, 7}, 5}};
    char szDir[cbTestPath], szFile[cbTestPath];
    uint8_t *lpb;
    int16_t cb, i, j, ib;
    TOK tok{};
    BTLREC br{};
    KILL kill{};

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("battle_continuations", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%cbattle.bin", szDir, chDirSep);
    lpb = (uint8_t *)LpAlloc(app, 0xffc8, htBattle);
    app.state.game.cPlayer = 1;
    for (const BATTLECASE &bcase : rgcase) {
        TEST_CASE_("%d tokens, %d kills", bcase.ctok, bcase.cKill);
        cb = 14 + bcase.ctok * 29 + 6 + bcase.cKill * 8;
        memset(lpb, 0, cb + 2);
        BdvFromLp(app, lpb).SetId(3);
        BdvFromLp(app, lpb).SetCtok(bcase.ctok);
        BdvFromLp(app, lpb).SetCplr(1);
        BdvFromLp(app, lpb).SetGrfPlr(1);
        BdvFromLp(app, lpb).SetCbData(cb);
        BdvFromLp(app, lpb).SetIdPlanet(idPlanetNone);
        BdvFromLp(app, lpb).SetPt({32767, -1});
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
        app.state.lpbBattleLog = lpb; app.state.lpbBattleCur = lpb + cb;
        StreamOpen(app, szFile, mdCreate);
        SetFileXorStream(app, 0x12345678, 0x4321, 123, 0, 0);
        WriteBattles(app, 0);
        WriteRt(app, rtEOF, std::span<const std::byte>{});
        StreamClose(app);
        StreamOpen(app, szFile, mdRead);
        SetFileXorStream(app, 0x12345678, 0x4321, 123, 0, 0);
        ib = 0;
        for (j = 0; j < bcase.cRec; j++) {
            ReadRt(app);
            TEST_ASSERT(app.state.hdrCur.rt == (j == 0 ? rtBtlData : rtContinue));
            TEST_ASSERT(app.state.hdrCur.cb == bcase.rgcb[j]);
            TEST_CHECK(memcmp(app.state.rgbCur, lpb + ib, app.state.hdrCur.cb) == 0);
            ib += app.state.hdrCur.cb;
        }
        TEST_CHECK(ib == cb);
        ReadRt(app);
        TEST_CHECK(app.state.hdrCur.rt == rtEOF);
        StreamClose(app);
    }
    FreeHb(app, app.state.rglphb[htBattle]);
    app.state.rglphb[htBattle] = NULL;
    app.state.lpbBattleLog = app.state.lpbBattleCur = NULL;
}

static void test_battle_plan_deletion_wire(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    BTLPLAN plan{};
    HDR hdr;
    uint8_t rgb[64];
    const uint8_t deleted[] = {0xd3, 0xf2};

    TEST_ASSERT(FStarsTestInit(app));
    plan.iplr = 3; plan.iplan = 13; plan.mdTactic = 2; plan.unused1 = 3; plan.fDelete = 1; plan.fDumpCargo = 1;
    plan.mdTarget1 = 8; plan.mdTarget2 = 7; plan.iplrAttack = 31; plan.unused2 = 7;
    WriteBattlePlan(app, &plan, 1);
    TEST_ASSERT(FGetPrevLogRt(app, &hdr, rgb));
    TEST_CHECK(hdr.rt == rtBtlPlan && hdr.cb == 2 && memcmp(rgb, deleted, sizeof(deleted)) == 0);
}

static void test_WriteRtScore_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const uint8_t golden[] = {0xd3, 0xa5, 0xfe, 0xff, 1, 0, 0, 0x80, 0xff, 0xff, 0xff, 0x7f,
        0xfe, 0xff, 0xff, 0x7f, 0xff, 0xff, 0, 0x80, 1, 0, 0, 0x80};
    SCOREX sx{}, decoded{};
    int32_t lSeed1, lSeed2;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("score-wire", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sscore", szDir, szDirSep);
    sx.iPlayer = 19; sx.fValid = 0; sx.grbitVC = 0x97; sx.fWinner = 0; sx.fHistory = 1; sx.turn = 65534;
    sx.score.lScore = -2147483647; sx.score.cResources = 2147483647; sx.score.cPlanet = -2; sx.score.cStarbase = 32767;
    sx.score.rgcsh[0] = 65535; sx.score.rgcsh[1] = 32768; sx.score.rgcsh[2] = 1; sx.score.cTechLevels = -32768;
    StreamOpen(app, szFile, mdCreate);
    SetFileSeeds(app, 123456, 789012);
    WriteRtScore(app, &sx);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == 173304275 && lSeed2 == 470715282);
    StreamClose(app);
    StreamOpen(app, szFile, mdRead);
    SetFileSeeds(app, 123456, 789012);
    ReadRt(app);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(app.state.hdrCur.rt == rtScore && app.state.hdrCur.cb == 24 && !memcmp(app.state.rgbCur, golden, sizeof(golden)));
    TEST_CHECK(lSeed1 == 173304275 && lSeed2 == 470715282);
    TEST_CHECK(FDecodeScoreX(std::as_bytes(std::span(app.state.rgbCur)), decoded) && decoded.wWord() == 0xa5d3 && decoded.turn == 65534 && decoded.iRank() == -2);
    StreamClose(app);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void test_WritePlanet_minimal_history_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const uint8_t golden[] = {0xfe, 7, 0x81, 0x83, 9, 0xfe, 0xff};
    PLANET pl{}, decoded{};
    int32_t lSeed1, lSeed2;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("planet-wire", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%splanet", szDir, szDirSep);
    pl.id = -2; pl.iPlayer = 0; pl.det = detMinimal; pl.fHomeworld = 1; pl.fInclude = 1; pl.fStarbase = 1; pl.fFirstYear = 1;
    pl.isb = 9; pl.turn = -2;
    StreamOpen(app, szFile, mdCreate); SetFileSeeds(app, 123456, 789012);
    WritePlanet(app, &pl, rtPlanetB, 1);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == 622877478 && lSeed2 == 1520895743);
    StreamClose(app);
    StreamOpen(app, szFile, mdRead); SetFileSeeds(app, 123456, 789012); ReadRt(app);
    TEST_CHECK(app.state.hdrCur.rt == rtPlanetB && app.state.hdrCur.cb == 7 && !memcmp(app.state.rgbCur, golden, sizeof(golden)));
    TEST_CHECK(FReadPlanet(app, iplrNone, &decoded, 1, 0));
    TEST_CHECK(decoded.id == -2 && decoded.iPlayer == 0 && decoded.det == detMinimal && decoded.fStarbase && decoded.isb == 9 && decoded.turn == -2);
    StreamClose(app);
    // detObscure's historical writer/reader thresholds differ: the writer
    // includes environment bytes that this reader skips before the turn.
    const uint8_t obscure[] = {0xfe, 0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0xfe, 0xff};
    pl = PLANET{}; pl.id = -2; pl.iPlayer = iplrNone; pl.det = detObscure; pl.turn = -2;
    StreamOpen(app, szFile, mdCreate); SetFileSeeds(app, 123456, 789012);
    WritePlanet(app, &pl, rtPlanetB, 1);
    StreamClose(app);
    StreamOpen(app, szFile, mdRead); SetFileSeeds(app, 123456, 789012); ReadRt(app);
    TEST_CHECK(app.state.hdrCur.cb == sizeof(obscure) && !memcmp(app.state.rgbCur, obscure, sizeof(obscure)));
    TEST_CHECK(FReadPlanet(app, iplrNone, &decoded, 1, 0) && decoded.det == detObscure && decoded.turn == 0);
    StreamClose(app);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void test_FReadPlanet_partial_publication(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t minimal[] = {0xfe, 0xff, 3, 0x81, 0, 1, 2, 3, 4, 5, 6, 0xfe, 0xff};
    const uint8_t levelBad[] = {0xfe, 0xff, 3, 0x81, 9, 17};
    const uint8_t envBad[] = {0xfe, 0xff, 3, 0x81, 0, 1, 2, 3, 20, 101};
    const uint8_t evoBad[] = {0xfe, 0xff, 3, 0x85, 0, 1, 2, 3, 10, 20, 30, 40, 101};
    PLANET pl{};

    TEST_ASSERT(FStarsTestInit(app));
    app.state.hdrCur.rt = rtPlanetB; app.state.hdrCur.cb = 0;
    memset(app.state.rgbCur, 0, sizeof(app.state.rgbCur)); memcpy(app.state.rgbCur, minimal, sizeof(minimal));
    TEST_CHECK(FReadPlanet(app, iplrNone, &pl, 1, 0));
    TEST_CHECK(pl.id == -2 && pl.iPlayer == -1 && pl.det == detSome && pl.fInclude && pl.fFirstYear && pl.turn == -2);
    TEST_CHECK(pl.rgMinConc[2] == 3 && pl.rgEnvVar[0] == 4 && pl.rgEnvVarOrig[2] == 6);
    pl.rgpctMinLevel[0] = 90; pl.rgpctMinLevel[1] = 91; pl.rgpctMinLevel[2] = 92; pl.rgMinConc[0] = 93; pl.turn = 123;
    memcpy(app.state.rgbCur, levelBad, sizeof(levelBad));
    TEST_CHECK(!FReadPlanet(app, iplrNone, &pl, 1, 1));
    TEST_CHECK(pl.rgpctMinLevel[0] == 17 && pl.rgpctMinLevel[1] == 91 && pl.rgpctMinLevel[2] == 92 && pl.rgMinConc[0] == 93 && pl.turn == 123);
    pl.rgEnvVar[0] = 70; pl.rgEnvVar[1] = 71; pl.rgEnvVarOrig[0] = 80; pl.rgEnvVarOrig[1] = 81;
    memcpy(app.state.rgbCur, envBad, sizeof(envBad));
    TEST_CHECK(!FReadPlanet(app, iplrNone, &pl, 1, 1));
    TEST_CHECK(pl.rgMinConc[2] == 3 && pl.rgEnvVar[0] == 20 && pl.rgEnvVarOrig[0] == 20 && pl.rgEnvVar[1] == 71 && pl.rgEnvVarOrig[1] == 81);
    memcpy(app.state.rgbCur, evoBad, sizeof(evoBad));
    TEST_CHECK(!FReadPlanet(app, iplrNone, &pl, 1, 1));
    TEST_CHECK(pl.rgEnvVar[0] == 10 && pl.rgEnvVar[2] == 30 && pl.rgEnvVarOrig[0] == 40 && pl.rgEnvVarOrig[1] == 20);
}

static void test_WriteRtPlr_partial_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const uint8_t golden[] = {0xfe, 0xff, 0, 0x80, 0xff, 0xff, 0x89, 0xaf, 0, 0, 0, 0};
    PLAYER plr{}, decoded{};
    int32_t lSeed1, lSeed2;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("player-wire", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%splayer", szDir, szDirSep);
    plr.iPlayer = -2; plr.cShDef = -1; plr.cPlanet = -32768; plr.cFleet = 4095; plr.cshdefSB = 15;
    plr.det = detMinimal; plr.iPlrBmp = 17; plr.fInclude = 1; plr.fAi = 1; plr.lvlAi = 3; plr.idAi = 5;
    StreamOpen(app, szFile, mdCreate); SetFileSeeds(app, 123456, 789012);
    WriteRtPlr(app, &plr);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == 125172514 && lSeed2 == 2112981774);
    StreamClose(app);
    StreamOpen(app, szFile, mdRead); SetFileSeeds(app, 123456, 789012); ReadRt(app);
    TEST_CHECK(app.state.hdrCur.rt == rtPlr && app.state.hdrCur.cb == 12 && !memcmp(app.state.rgbCur, golden, sizeof(golden)));
    ReadRtPlr(app, &decoded, reinterpret_cast<uint8_t *>(app.state.rgbCur), app.state.hdrCur.cb);
    TEST_CHECK(decoded.iPlayer == -2 && decoded.cShDef == -1 && decoded.cPlanet == -32768 && decoded.cFleet == 4095 && decoded.cshdefSB == 15);
    TEST_CHECK(decoded.wMdPlr() == 0xaf89 && decoded.szName[0] == 0 && decoded.szNames[0] == 0 && !decoded.fLearned);
    StreamClose(app);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void test_WriteRtThing_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const uint8_t golden[] = {0x23, 0xe1, 0, 0x80, 0xff, 0x7f, 1, 0, 0, 0x80, 0xdc, 0xfe, 0xff, 0xa5, 0xab, 0x89, 0xfe, 0xff};
    THING th{}, decoded{};
    int32_t lSeed1, lSeed2;
    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("thing-wire", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sthing", szDir, szDirSep);
    TEST_ASSERT(FDecodeThing(std::as_bytes(std::span(golden)), th));
    StreamOpen(app, szFile, mdCreate); SetFileSeeds(app, 123456, 789012); WriteRtThing(app, &th);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == 170401200 && lSeed2 == 2097781771);
    TEST_CHECK(!memcmp(th.RgbRecord().data(), golden, sizeof(golden)));
    StreamClose(app);
    StreamOpen(app, szFile, mdRead); SetFileSeeds(app, 123456, 789012); ReadRt(app);
    TEST_CHECK(app.state.hdrCur.rt == rtThing && app.state.hdrCur.cb == 18 && !memcmp(app.state.rgbCur, golden, sizeof(golden)));
    TEST_CHECK(FDecodeThing(std::as_bytes(std::span(app.state.rgbCur)), decoded) && decoded.idFull() == 0xe123);
    TEST_CHECK(decoded.pt().x() == -32768 && decoded.pt().y() == 32767 && decoded.turn() == 65534);
    StreamClose(app); TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

TEST_LIST = {{"load rejects unavailable size instead of matching error sentinels", test_Load_requires_size_result}, {"thing records retain exact encrypted bytes and seed consumption", test_WriteRtThing_bytes},
             {"player partial record keeps exact bytes and seed consumption", test_WriteRtPlr_partial_bytes},
             {"planet history preserves exact encrypted bytes", test_WritePlanet_minimal_history_bytes},
             {"planet reads retain partial updates and physical scratch", test_FReadPlanet_partial_publication},
             {"score records preserve encrypted bytes and seed consumption", test_WriteRtScore_bytes},
             {"battle-plan deletion encodes only its two-byte header", test_battle_plan_deletion_wire},
             {"battle continuation bytes and boundaries", test_battle_continuations},
             {"fleet readers preserve scratch admission and incremental publication", test_fleet_scratch_admission},
             {"ship-design bytes preserve import admission and knowledge", test_design_wire_admission},
             {"UTF-8 records require complete in-record text", test_utf8_record_bounds},
             {"invalid saved names never publish a record", test_invalid_written_names},
             {"design encoding respects destination spans and keeps wire text", test_design_encoder_spans},
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
