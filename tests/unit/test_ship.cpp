#include "acutest.h"

#include "stars_test.h"

// FAddScoutPair loads a new one-player game and adds two fleets of one ship
// each of player 0's first design, orbiting its homeworld.
static int16_t FAddScoutPair(const char *pszTest, FLEET **plpflDst, FLEET **plpflSrc, int16_t *pishdef) {
    char    szDir[cbTestPath];
    PLANET *lppl;
    int16_t ish;

    if (!FStarsTestInit() || !FStarsTestDir(pszTest, szDir, sizeof(szDir)) || !FStarsTestNewGame(szDir, 12345, NULL, 0) ||
        !FStarsTestLoadHost())
        return 0;
    lppl = LpplStarsTestHomeworld(0);
    if (lppl == NULL)
        return 0;
    for (ish = 0; ish < 16 && rglpshdef[0][ish].fFree; ish++) {
    }
    if (ish == 16)
        return 0;
    *plpflDst = LpflStarsTestAddFleet(0, lppl->id, ish, 1);
    *plpflSrc = LpflStarsTestAddFleet(0, lppl->id, ish, 1);
    *pishdef = ish;
    return 1;
}


// Merge log IDs retain their ascending input order, duplicate entries and odd
// trailing-byte treatment. Native view sizeof is never a record prefix length.
static void test_fleet_id_log_bytes(void) {
    FLEET *lpflDst, *lpflSrc;
    int16_t ishdef, idDst, idSrc;
    int16_t rgifl[5];
    uint8_t rgb[16] = {};
    HDR hdr;
    bool fReplay;

    TEST_ASSERT(FAddScoutPair("fleet_id_log_bytes", &lpflDst, &lpflSrc, &ishdef));
    idPlayer = 0;
    idDst = lpflDst->id; idSrc = lpflSrc->id;
    rgifl[0] = idDst; rgifl[1] = idSrc; rgifl[2] = iflNone; rgifl[3] = idSrc; rgifl[4] = idDst;
    vrgiflMerge = rgifl; vcflMerge = 5;
    gd.fGeneratingTurn = 0;
    LogMergeFleet(idDst);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogFleetMerge && hdr.cb == 6);
    TEST_CHECK(rgb[0] == (uint8_t)idDst && rgb[1] == (uint8_t)((uint16_t)idDst >> 8));
    TEST_CHECK(rgb[2] == (uint8_t)idSrc && rgb[3] == (uint8_t)((uint16_t)idSrc >> 8));
    TEST_CHECK(rgb[4] == rgb[2] && rgb[5] == rgb[3]);
    rgb[6] = 0xa5;
    gd.fGeneratingTurn = 1;
    fReplay = FRunLogRecord(rtLogFleetMerge, 7, rgb);
    // FRunLogRecord temporarily installs its stack ID array in this global.
    vrgiflMerge = NULL; vcflMerge = 0;
    TEST_ASSERT(fReplay);
    lpflDst = LpflFromId(idDst);
    TEST_ASSERT(lpflDst != NULL);
    TEST_CHECK(lpflDst->rgcsh[ishdef] == 2 && LpflFromId(idSrc) == NULL && lpflDst->fCompChg());
    gd.fGeneratingTurn = 0;
    LogSplitFleet(idDst);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogFleetSplit && hdr.cb == 2 && rgb[0] == (uint8_t)idDst && rgb[1] == (uint8_t)((uint16_t)idDst >> 8));
    gd.fGeneratingTurn = 1;
    TEST_ASSERT(FRunLogRecord(rtLogFleetSplit, 2, rgb));
}

static void test_ship_transfer_log_bytes(void) {
    FLEET *lpflDst, *lpflSrc;
    int16_t ishdef;
    LOGXFERF lxf1{}, lxf2{};
    uint8_t rgb[48] = {}, missing[] = {0xff, 0x01}; // Positive unused slot; negative IDs request write-back.
    HDR hdr;
    int16_t idDst, idSrc;

    TEST_ASSERT(FAddScoutPair("ship_transfer_log_bytes", &lpflDst, &lpflSrc, &ishdef));
    idPlayer = 0;
    idDst = lpflDst->id; idSrc = lpflSrc->id;
    lpflSrc->rgcsh[ishdef] = 5;
    lxf1.grobj = grobjFleet; lxf1.id = idDst;
    lxf2.grobj = grobjFleet; lxf2.id = idSrc;
    lxf1.rgdItem[ishdef] = 2;
    gd.fGeneratingTurn = 0;
    LogMakeValidXferf(&lxf1, &lxf2);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogFleetCargoXfer && hdr.cb == 9 && rgb[4] == 0x22);
    TEST_CHECK(rgb[0] == (uint8_t)idDst && rgb[1] == (uint8_t)((uint16_t)idDst >> 8) && rgb[2] == (uint8_t)idSrc && rgb[3] == (uint8_t)((uint16_t)idSrc >> 8));
    TEST_CHECK(rgb[5] == (uint8_t)(1u << ishdef) && rgb[6] == (uint8_t)((1u << ishdef) >> 8) && rgb[7] == 2 && rgb[8] == 0);
    gd.fGeneratingTurn = 1;
    TEST_ASSERT(LpflFromId(0x1ff) == NULL);
    TEST_CHECK(!FRunLogRecord(rtLogFleetCargoXfer, 2, missing));
    TEST_CHECK(!FRunLogRecord(rtLogFleetMerge, 2, missing));
    TEST_ASSERT(FRunLogRecord(rtLogFleetCargoXfer, 9, rgb));
    TEST_CHECK(LpflFromId(idDst)->rgcsh[ishdef] == 3 && LpflFromId(idSrc)->rgcsh[ishdef] == 3);
}

// Repair after gating: a fleet that gated (or fought, or hit mines) can't
// heal this turn, but merging it by waypoint into a fleet that stayed put
// dropped its fNoHeal, so HealShips repaired the merged ships.
static void test_Merge2Fleets_keeps_no_heal(void) {
    FLEET  *lpflDst;
    FLEET  *lpflSrc;
    int16_t ishdef;

    TEST_ASSERT(FAddScoutPair("Merge2Fleets_no_heal", &lpflDst, &lpflSrc, &ishdef));
    lpflSrc->rgdv[ishdef].SetPctSh(100);
    lpflSrc->rgdv[ishdef].SetPctDp(100);
    lpflSrc->fNoHeal = 1;
    lpflDst->fHereAllTurn = 1;

    Merge2Fleets(lpflDst, lpflSrc, 1);
    TEST_ASSERT(lpflDst->rgcsh[ishdef] == 2);
    TEST_CHECK(lpflDst->fNoHeal);
    HealShips();
    TEST_CHECK_(lpflDst->rgdv[ishdef].pctDp() == 100, "pctDp %d", lpflDst->rgdv[ishdef].pctDp());
}

// 32k ships per fleet: fleet ship counts are int16_t. A waypoint merge added
// them without a limit, so 32000 + 1000 ships became a negative count.
static void test_Merge2Fleets_caps_ship_count(void) {
    FLEET  *lpflDst;
    FLEET  *lpflSrc;
    int16_t ishdef;

    TEST_ASSERT(FAddScoutPair("Merge2Fleets_caps", &lpflDst, &lpflSrc, &ishdef));
    lpflDst->rgcsh[ishdef] = 32000;
    lpflSrc->rgcsh[ishdef] = 1000;

    Merge2Fleets(lpflDst, lpflSrc, 1);
    TEST_CHECK_(lpflDst->rgcsh[ishdef] == 32767, "destination has %d", lpflDst->rgcsh[ishdef]);
    TEST_CHECK_(lpflSrc->rgcsh[ishdef] == 233, "source kept %d", lpflSrc->rgcsh[ishdef]);
    TEST_CHECK(!lpflSrc->fDead);
}

// Merging all fleets at a location clamped an overflowing count to 32766,
// losing the rest of the ships.
static void test_FFleetMergeAll_keeps_overflow_fleet(void) {
    FLEET   *lpflDst;
    FLEET   *lpflSrc;
    FLEET    fl;
    int16_t  ishdef;
    int16_t  idSrc;
    int16_t  rgifl[2];

    TEST_ASSERT(FAddScoutPair("FFleetMergeAll_overflow", &lpflDst, &lpflSrc, &ishdef));
    lpflDst->rgcsh[ishdef] = 32000;
    lpflSrc->rgcsh[ishdef] = 1000;
    idSrc = lpflSrc->id;
    rgifl[0] = lpflDst->id;
    rgifl[1] = idSrc;
    vrgiflMerge = rgifl;
    vcflMerge = 2;
    gd.fGeneratingTurn = 1;
    fl = *lpflDst;

    FFleetMergeAll(&fl);
    lpflDst = LpflFromId(rgifl[0]);
    lpflSrc = LpflFromId(idSrc);
    TEST_ASSERT(lpflDst != NULL);
    TEST_CHECK_(lpflDst->rgcsh[ishdef] == 32000, "destination has %d", lpflDst->rgcsh[ishdef]);
    TEST_CHECK_(lpflSrc != NULL && lpflSrc->rgcsh[ishdef] == 1000, "the 1000-ship fleet was merged");
}

// FAddScoutLeg loads a new game, puts one of player 0's first design at its
// homeworld in sel.fl, as the scanner has a selected fleet, and gives it a
// waypoint dx light years east with lFuel mg of fuel, more than its tank
// holds if need be.
static int16_t FAddScoutLeg(const char *pszTest, int16_t dx, int32_t lFuel) {
    FLEET  *lpflDst;
    FLEET  *lpflSrc;
    ORDER  *lpord;
    int16_t ishdef;

    if (!FAddScoutPair(pszTest, &lpflDst, &lpflSrc, &ishdef))
        return 0;
    idPlayer = 0;
    sel.fl = *lpflDst;
    sel.fl.rgwtMin[4] = lFuel;
    lpord = &sel.fl.lpplord->rgord()[1];
    *lpord = lpord[-1];
    lpord->pt.x += dx;
    lpord->grobj = grobjOther;
    lpord->id = 1;
    sel.fl.cord = 2;
    sel.fl.lpplord->iordMac = 2;
    return 1;
}

// LFuelAtWarp returns the fuel sel.fl uses to reach waypoint 1 at iWarp.
static int32_t LFuelAtWarp(int16_t iWarp) {
    sel.fl.lpplord->rgord()[1].iWarp = iWarp;
    return LFuelUseToWaypoint(&sel.fl, 1, 1);
}

// CYearsAtWarp returns the years sel.fl takes to reach waypoint 1 at iWarp.
static int16_t CYearsAtWarp(int16_t iWarp) {
    ORDER *lpord;
    double dbl;

    lpord = sel.fl.lpplord->rgord();
    dbl = DGetDistance(lpord[0].pt.x, lpord[0].pt.y, lpord[1].pt.x, lpord[1].pt.y);
    return (int16_t)ceil(dbl / (iWarp * iWarp));
}

// CheckFastestWarp checks the warp IWarpFastestForWaypoint picks for sel.fl's
// waypoint 1: the fuel fits, no faster warp up to 9 that fits arrives
// sooner, and a slower one arrives later. It returns the warp.
static int16_t IWarpCheckFastest(int16_t iWarpBest) {
    int32_t lFuel;
    int16_t iWarp;
    int16_t i;

    lFuel = sel.fl.rgwtMin[4];
    iWarp = IWarpFastestForWaypoint(&sel.fl, &sel.fl.lpplord->rgord()[1]);
    TEST_CHECK_(iWarp >= iWarpBest && iWarp <= 9, "fastest warp %d, best warp %d", iWarp, iWarpBest);
    TEST_CHECK_(LFuelAtWarp(iWarp) <= lFuel, "warp %d needs %d mg of %d", iWarp, LFuelAtWarp(iWarp), lFuel);
    for (i = iWarp + 1; i <= 9; i++) {
        TEST_CHECK_(LFuelAtWarp(i) > lFuel || CYearsAtWarp(i) == CYearsAtWarp(iWarp), "warp %d fits and arrives sooner than warp %d", i,
                    iWarp);
    }
    if (iWarp > 1) {
        TEST_CHECK_(CYearsAtWarp(iWarp - 1) > CYearsAtWarp(iWarp), "warp %d arrives as soon as warp %d", iWarp - 1, iWarp);
    }
    return iWarp;
}

// Alt+click waypoints: with fuel to spare the fastest useful warp arrives
// sooner than the usual one, and nothing faster arrives sooner still.
// 150 ly takes 2 years at warp 9 and 3 at warp 8.
static void test_IWarpFastestForWaypoint_plenty_of_fuel(void) {
    int16_t iWarpBest;
    int16_t iWarp;

    TEST_ASSERT(FAddScoutLeg("IWarpFastest_plenty", 150, 1000));
    iWarpBest = IWarpBestForWaypoint(&sel.fl, &sel.fl.lpplord->rgord()[1]);
    iWarp = IWarpCheckFastest(iWarpBest);
    TEST_CHECK_(iWarp == 9, "fastest warp %d", iWarp);
    TEST_CHECK_(CYearsAtWarp(iWarp) < CYearsAtWarp(iWarpBest), "warp %d is no sooner than warp %d", iWarp, iWarpBest);
}

// With less fuel the fastest warp is held to what the tank allows: one
// short of what two warps above the ideal needs stops it one above.
static void test_IWarpFastestForWaypoint_low_fuel(void) {
    int16_t iWarpIdeal;
    int16_t iWarp;

    TEST_ASSERT(FAddScoutLeg("IWarpFastest_low_fuel", 300, 0));
    iWarpIdeal = IFindIdealWarp(NULL, 0);
    TEST_ASSERT_(iWarpIdeal <= 7, "ideal warp %d", iWarpIdeal);
    sel.fl.rgwtMin[4] = LFuelAtWarp(iWarpIdeal + 2) - 1;
    iWarp = IWarpCheckFastest(iWarpIdeal);
    TEST_CHECK_(iWarp == iWarpIdeal + 1, "fastest warp %d, ideal warp %d", iWarp, iWarpIdeal);
}

static void test_jettison_log_bytes(void) {
    FLEET *lpflDst, *lpflSrc;
    int16_t ishdef;
    PLANET pl{};
    uint8_t wrongObject[] = {0, 0, 0, 0, 0x11};
    uint8_t rgb[10]{};

    TEST_ASSERT(FAddScoutPair("jettison_log_bytes", &lpflDst, &lpflSrc, &ishdef));
    TEST_CHECK(FEnumCalcJettison(NULL, rtLogCargoXfer32, 0, NULL, 0));
    TEST_CHECK(FEnumCalcJettison(wrongObject, rtLogCargoXfer8, 0, NULL, 0));
    TEST_CHECK(FEnumCalcJettison(wrongObject, rtLogCargoXfer16, 0, NULL, 0));
    rgb[0] = (uint8_t)lpflSrc->id; rgb[1] = (uint8_t)((uint16_t)lpflSrc->id >> 8);
    rgb[4] = 0x42; rgb[5] = 5; rgb[6] = 0x81; rgb[7] = 0x7e;
    pl.rgwtMin[0] = 1000; pl.rgwtMin[2] = 500;
    TEST_CHECK(FEnumCalcJettison(rgb, rtLogCargoXfer8, 0, &pl, lpflDst->id));
    TEST_CHECK(pl.rgwtMin[0] == 1127 && pl.rgwtMin[2] == 374);
    rgb[6] = 0; rgb[7] = 0x80; rgb[8] = 0xff; rgb[9] = 0x7f;
    pl.rgwtMin[0] = 1000; pl.rgwtMin[2] = 500;
    TEST_CHECK(FEnumCalcJettison(rgb, rtLogCargoXfer16, 0, &pl, lpflDst->id));
    TEST_CHECK(pl.rgwtMin[0] == 33768 && pl.rgwtMin[2] == -32267);
}

TEST_LIST = {{"jettison preserves signed log quantities and lookup read paths", test_jettison_log_bytes},
             {"fleet ID log bytes preserve order, duplicates and prefix length", test_fleet_id_log_bytes},
             {"ship-transfer log bytes retain signed quantities and lookup order", test_ship_transfer_log_bytes},
             {"Merge2Fleets keeps no-heal", test_Merge2Fleets_keeps_no_heal},
             {"Merge2Fleets caps ship counts", test_Merge2Fleets_caps_ship_count},
             {"FFleetMergeAll keeps a fleet that would overflow", test_FFleetMergeAll_keeps_overflow_fleet},
             {"IWarpFastestForWaypoint with plenty of fuel", test_IWarpFastestForWaypoint_plenty_of_fuel},
             {"IWarpFastestForWaypoint with little fuel", test_IWarpFastestForWaypoint_low_fuel},
             {NULL, NULL}};
