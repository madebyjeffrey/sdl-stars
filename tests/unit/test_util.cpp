#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"

static void test_FleetName_utf8_boundary(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath];
    FLEET *lpfl;
    const char *pszName = "AAAAAAAAAAAAAAAAAAAAAAAAAAA\xc3\xa9";
    const char *psz;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("FleetName_utf8", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    strcpy(app.state.rglpshdef[0][0].hul.szClass, pszName);
    lpfl = LpflStarsTestAddFleet(app, 0, idPlanetDeepSpace, 0, 1);
    lpfl->rgcsh[1] = 1;
    psz = PszFleetNameFromWord(app, 0x2000);
    TEST_CHECK(FUtf8Valid(psz, strlen(psz)));
    TEST_CHECK(strncmp(psz, "AAAAAAAAAAAAAAAAAAAAAAAAAAA+", 28) == 0);
    psz = PszGetFleetName(app, lpfl->id);
    TEST_CHECK(FUtf8Valid(psz, strlen(psz)));
}

// SpaceDockWithArmor returns a Space Dock design with cArmor Superlatanium
// (1500 dp each) in its shield/armor slot.
static SHDEF SpaceDockWithArmor(int16_t cArmor) {
    SHDEF shdef;

    memset(&shdef, 0, sizeof(shdef));
    shdef.hul = LphuldefFromId(ihuldefSpaceDock)->hul;
    shdef.det = detAll;
    shdef.hul.rghs[2].grhst = hstArmor;
    shdef.hul.rghs[2].iItem = iarmorSuperlatanium;
    shdef.hul.rghs[2].cItem = cArmor;
    return shdef;
}

// LoadPlayer loads player 1's turn file of a new tiny game, with or without
// Regenerating Shields.
static void LoadPlayer(APPSESSION &app, const char *pszTest, int16_t fRS) {
    char szDir[cbTestPath];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    SetRaceGrbit(&app.state.rgplr[0], ibitRaceRegeneratingShields, fRS);
}

static void test_UpdateShdefCost_armor(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    SHDEF shdef;

    LoadPlayer(app, "UpdateShdefCost_armor", 0);
    shdef = SpaceDockWithArmor(20);
    UpdateShdefCost(app, &shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 30000, "dp %u", shdef.hul.dp);
    shdef = SpaceDockWithArmor(24);
    UpdateShdefCost(app, &shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 36000, "dp %u", shdef.hul.dp);
}

// Space Dock armor overflow: with RS the armor slot's strength was halved
// after it had overflowed signed 16 bits, so 24 Superlatanium (36000 dp)
// gave the hull about 50000 dp instead of 18250.
static void test_UpdateShdefCost_rs_armor_overflow(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    SHDEF shdef;

    LoadPlayer(app, "UpdateShdefCost_rs_armor_overflow", 1);
    shdef = SpaceDockWithArmor(20);
    UpdateShdefCost(app, &shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 15000, "dp %u", shdef.hul.dp);
    shdef = SpaceDockWithArmor(24);
    UpdateShdefCost(app, &shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 18000, "dp %u", shdef.hul.dp);
}

// AddPursuer adds a player-0 fleet in deep space at pt whose second waypoint
// pursues fleet idTarget, last seen at ptTarget.
static FLEET *AddPursuer(APPSESSION &app, int16_t ish, POINT16 pt, int16_t idTarget, POINT16 ptTarget) {
    FLEET *lpfl;
    ORDER *lpord;

    lpfl = LpflStarsTestAddFleet(app, 0, idPlanetDeepSpace, ish, 1);
    lpfl->pt = pt;
    lpfl->lpplord->rgord()[0].pt = pt;
    lpfl->lpplord->iordMac = 2;
    lpfl->cord = 2;
    lpord = &lpfl->lpplord->rgord()[1];
    memset(lpord, 0, sizeof(*lpord));
    lpord->pt = ptTarget;
    lpord->id = idTarget;
    lpord->grobj = grobjFleet;
    lpord->iWarp = 5;
    return lpfl;
}

// When a pursued fleet is gone, ValidateWaypoints retargets each pursuer at a
// fleet where it was last seen. 2.6j RC3 added a first pass that skipped
// fleets another pursuer had already taken, spreading pursuers over the
// pieces of a split fleet; the jrc4 binary (0008:497a) dropped it again, so
// every pursuer takes the heaviest matching fleet.
static void test_ValidateWaypoints_pursuers_take_heaviest(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char    szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    int16_t ish0;
    int16_t ish1;
    POINT16 ptFled;
    POINT16 ptStart;
    FLEET  *lpflHeavy;
    FLEET  *lpflLight;
    FLEET  *lpflA;
    FLEET   *lpflB;
    MdTarget mdTarget;
    int16_t  i;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("ValidateWaypoints_pursuers", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    for (ish0 = 0; ish0 < 16 && app.state.rglpshdef[0][ish0].fFree; ish0++) {
    }
    for (ish1 = 0; ish1 < 16 && app.state.rglpshdef[1][ish1].fFree; ish1++) {
    }
    TEST_ASSERT(ish0 < 16 && ish1 < 16);

    ptFled.x = 1200;
    ptFled.y = 1200;
    ptStart.x = 1100;
    ptStart.y = 1100;
    lpflHeavy = LpflStarsTestAddFleet(app, 1, idPlanetDeepSpace, ish1, 5);
    lpflLight = LpflStarsTestAddFleet(app, 1, idPlanetDeepSpace, ish1, 1);
    lpflHeavy->pt = lpflLight->pt = ptFled;
    // Target the AI design's category ("Any" never matches exactly).
    mdTarget = FMatchTarget(app, lpflHeavy, mdTargetArmedShips, 1) ? mdTargetArmedShips : mdTargetUnarmedShips;
    TEST_ASSERT(FMatchTarget(app, lpflHeavy, mdTarget, 1) && FMatchTarget(app, lpflLight, mdTarget, 1));
    for (i = 0; i < app.state.rgcbtlplan[0]; i++)
        app.state.rglpbtlplan[0][i].mdTarget1 = mdTarget;
    lpflA = AddPursuer(app, ish0, ptStart, (1 << 9) | 400, ptFled);
    lpflB = AddPursuer(app, ish0, ptStart, (1 << 9) | 401, ptFled);

    ValidateWaypoints(app);
    TEST_CHECK_(lpflA->lpplord->rgord()[1].id == lpflHeavy->id, "first pursuer took %#x, heavy %#x", lpflA->lpplord->rgord()[1].id, lpflHeavy->id);
    TEST_CHECK_(lpflB->lpplord->rgord()[1].id == lpflHeavy->id, "second pursuer took %#x, heavy %#x light %#x", lpflB->lpplord->rgord()[1].id, lpflHeavy->id,
                lpflLight->id);
}

static void test_PtToScan_negative(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int16_t iZoomSav = app.state.iScanZoom;

    app.state.iScanZoom = zoom38;
    TEST_CHECK(PtToScan(app, -341) == -128);
    app.state.iScanZoom = zoom75;
    TEST_CHECK(PtToScan(app, -341) == -256);
    app.state.iScanZoom = zoom125;
    TEST_CHECK(PtToScan(app, -341) == -427);
    app.state.iScanZoom = zoom150;
    TEST_CHECK(PtToScan(app, -341) == -512);
    app.state.iScanZoom = (ScanZoom)(iZoomSav);
}

static void test_FLookupPart_host(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    PART part;
    NATIVEPARTCOMMONVIEW comPrior;
    ENGINE *pengine;
    int16_t cMassSav;
    int16_t idPlayerSav = app.state.idPlayer;

    memset(&part, 0, sizeof(part));
    app.state.idPlayer = iplrNone;
    TEST_CHECK(FLookupPartX(app, &part, hstEngine, iengineSettlersDelight) == mdPartAvailAvailable);
    TEST_CHECK(part.pengine() == LpengineFromId(iengineSettlersDelight));
    pengine = part.pengine();
    comPrior = part.pcom();
    part.hs.grhst = hstArmor;
    part.hs.iItem = 255;
    TEST_CHECK(FLookupPart(app, &part) == mdPartAvailInvalid);
    TEST_CHECK(part.pcom() == pengine && part.pcom().cMass() == pengine->cMass);
    TEST_CHECK(FLookupPartX(app, &part, hstArmor, 0) == mdPartAvailAvailable);
    TEST_CHECK(part.parmor() == rgarmor && comPrior == pengine);
    cMassSav = pengine->cMass;
    pengine->cMass = -32768;
    TEST_CHECK(comPrior.cMass() == -32768);
    pengine->cMass = cMassSav;
    app.state.idPlayer = idPlayerSav;
}

// Lookup's Other/unknown fallback is THING; cargo's default is FLEET. The
// shared frame must retain initialized cargo outside the eighteen-byte copy.
static void test_Xfer_lookup_and_cargo_fallbacks(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath];
    XFER xf{};
    THING *lpth;
    int16_t id;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("Xfer_fallbacks", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost(app));
    while (app.state.cThing != 0) FreeLpth(app, app.state.lpThings);
    lpth = LpthNew(app, 0, ithMinefield);
    TEST_ASSERT(lpth != NULL);
    lpth->pt().Set({123, 456});
    id = lpth->idFull();
    xf.fl().rgwtMin[0] = 42;
    TEST_ASSERT(FLookupObject(app, grobjOther, id, xf.LpvLookup(grobjOther)));
    TEST_CHECK(xf.idObj() == id && xf.iPlayerObj() == 123);
    TEST_CHECK(ChgCargo(app, static_cast<GrobjClass>(3), id, static_cast<MineralType>(0), 0,
                        xf.LpvCargo(static_cast<GrobjClass>(3))) == 42);
    TEST_CHECK(ChgCargo(app, grobjOther, id, static_cast<MineralType>(0), 0, xf.LpvCargo(grobjOther)) == 0);
    TEST_CHECK(xf.th().idFull() == id && xf.th().pt().x() == 123 && xf.th().pt().y() == 456);
}


#ifdef STARS_UI_SDL
static void test_TransferStuff_restores_borrowed_frame_on_lookup_failure(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    XFER rgSaved[2]{};
    XFER *pxferSav;
    MdXfer mdXferSav;

    TEST_ASSERT(FStarsTestInit(app));
    pxferSav = app.state.pxfer; mdXferSav = app.lpui->mdXferDlg;
    app.state.pxfer = rgSaved; app.lpui->mdXferDlg = mdXferShips;
    TEST_CHECK(!TransferStuff(app, INT16_MAX, grobjPlanet, 0, grobjOther, mdXferCargo));
    TEST_CHECK(app.state.pxfer == rgSaved && app.lpui->mdXferDlg == mdXferShips);
    app.state.pxfer = pxferSav; app.lpui->mdXferDlg = mdXferSav;
}
#endif

TEST_LIST = {
#ifdef STARS_UI_SDL
    {"transfer lookup failure restores the outer borrowed frame", test_TransferStuff_restores_borrowed_frame_on_lookup_failure},
#endif
{"transfer lookup/cargo fallbacks retain prefix and tail", test_Xfer_lookup_and_cargo_fallbacks},
             {"FLookupPart host sentinel", test_FLookupPart_host},
             {"generated fleet names preserve complete UTF-8", test_FleetName_utf8_boundary},
             {"PtToScan negative coordinates", test_PtToScan_negative},
             {"ValidateWaypoints pursuers take the heaviest fleet", test_ValidateWaypoints_pursuers_take_heaviest},
             {"UpdateShdefCost armor", test_UpdateShdefCost_armor},
             {"UpdateShdefCost RS armor overflow", test_UpdateShdefCost_rs_armor_overflow},
             {NULL, NULL}};
