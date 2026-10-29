#include "acutest.h"
[[maybe_unused]] static char szTestLiteral1[] = "Original";
[[maybe_unused]] static char szTestLiteral2[] = "\xc3";

#include "stars_test.h"

static void test_FRunLogRecord_rejects_invalid_utf8_name(void) {
    char szDir[MAX_PATH];
    RTCHGNAME rt;
    FLEET *lpfl;
    int16_t cOut;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("LogFleetName_utf8", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    lpfl = LpflStarsTestAddFleet(0, idPlanetDeepSpace, 0, 1);
    LogChangeName(grobjFleet, lpfl->id, szTestLiteral1);
    memset(&rt, 0, sizeof(rt));
    rt.id = lpfl->id;
    cOut = 31;
    TEST_ASSERT(FCompressUserString(szTestLiteral2, (char *)rt.rgb + 1, &cOut));
    rt.rgb[0] = cOut;
    TEST_CHECK(!FRunLogRecord(rtLogFleetName, offsetof(RTCHGNAME, rgb) + cOut + 1, (uint8_t *)&rt));
    TEST_CHECK(strcmp(lpfl->lpszName, "Original") == 0);
    rt.rgb[0] = 0;
    memcpy(rt.rgb + 1, "\xc3\xa9", 3);
    TEST_CHECK(!FRunLogRecord(rtLogFleetName, offsetof(RTCHGNAME, rgb) + 3, (uint8_t *)&rt));
    TEST_CHECK(strcmp(lpfl->lpszName, "Original") == 0);
    TEST_CHECK(FRunLogRecord(rtLogFleetName, offsetof(RTCHGNAME, rgb) + 4, (uint8_t *)&rt));
    TEST_CHECK(strcmp(lpfl->lpszName, "\xc3\xa9") == 0);
}

// Mineral upload: minerals sent from a planet to another player's fleet
// were taken from the planet, but only what fit in the fleet's hold was
// delivered and the rest was destroyed.
static void test_FRunLogRecord_refunds_undelivered_cargo(void) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET     *lppl;
    FLEET      *lpfl;
    int16_t     ish;
    int16_t     idfl;
    int32_t     wtCargo;
    int32_t     wtPlanet;
    uint8_t     rgb[16];

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FRunLogRecord_refund", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(0);
    TEST_ASSERT(lppl != NULL);
    // One of the AI's ships that can carry cargo, orbiting player 0's
    // homeworld.
    for (ish = 0; ish < 16 && (rglpshdef[1][ish].fFree || WtMaxShdefStat(&rglpshdef[1][ish], 2) == 0); ish++) {
    }
    TEST_ASSERT(ish < 16);
    wtCargo = WtMaxShdefStat(&rglpshdef[1][ish], 2);
    TEST_ASSERT(wtCargo < 100);
    lpfl = LpflStarsTestAddFleet(1, lppl->id, ish, 1);
    idfl = lpfl->id;
    wtPlanet = lppl->rgwtMin[0];
    TEST_ASSERT(wtPlanet >= 100);

    // Player 0's order: 100 kT of ironium from the planet to the fleet.
    gd.fGeneratingTurn = TRUE;
    idPlayer = 0;
    memset(rgb, 0, sizeof(rgb));
    ((RTXFER *)rgb)->id1 = lppl->id;
    ((RTXFER *)rgb)->id2 = idfl;
    ((RTXFER *)rgb)->grobj1 = grobjPlanet;
    ((RTXFER *)rgb)->grobj2 = grobjFleet;
    ((RTXFER *)rgb)->grbitItems = 1;
    ((RTXFER *)rgb)->rgcQuan[0] = -100;
    TEST_ASSERT(FRunLogRecord(rtLogCargoXfer8, 7, rgb));

    lppl = LpplStarsTestHomeworld(0);
    lpfl = LpflFromId(idfl);
    TEST_CHECK_(lpfl->rgwtMin[0] == wtCargo, "fleet holds %ld of %ld", (long)lpfl->rgwtMin[0], (long)wtCargo);
    TEST_CHECK_(lppl->rgwtMin[0] == wtPlanet - wtCargo, "planet has %ld, had %ld", (long)lppl->rgwtMin[0], (long)wtPlanet);
}

// Cheap Starbase: a partly built queue item keeps its completion
// percentage (PROD.pct) and the rest is costed from the current design. A
// player could build most of a cheap starbase design, then edit the design
// into an expensive one and finish it for the rest of the cheap one's cost.
static void test_FRunLogRecord_design_change_clears_progress(void) {
    char     szDir[MAX_PATH];
    PLANET  *lppl;
    SHDEF    shdef;
    int16_t  isb;
    int16_t  i;
    uint8_t  rgb[160];
    uint8_t *pb;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FRunLogRecord_design_change", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());
    // A second starbase design, half built at a colony without a starbase.
    for (isb = 0; isb < 10 && !rglpshdefSB[0][isb].fFree; isb++) {
    }
    TEST_ASSERT(isb < 10);
    rglpshdefSB[0][isb] = rglpshdefSB[0][0];
    rglpshdefSB[0][isb].ishdef = 16 + isb;
    rglpshdefSB[0][isb].cExist = 0;
    rglpshdefSB[0][isb].cBuilt = 0;
    rgplr[0].cshdefSB++;
    for (i = 0; i < cPlanet && (lpPlanets[i].iPlayer != iplrNone || lpPlanets[i].fStarbase); i++) {
    }
    TEST_ASSERT(i < cPlanet);
    lppl = &lpPlanets[i];
    lppl->iPlayer = 0;
    lppl->rgwtMin[3] = 1000;
    lppl->lpplprod = LpplAlloc<PLPROD>(4, 3, htOrd);
    lppl->lpplprod->iprodMac = 1;
    lppl->lpplprod->rgprod()[0].cItem = 1;
    lppl->lpplprod->rgprod()[0].iItem = 16 + isb;
    lppl->lpplprod->rgprod()[0].grobj = grobjFleet;
    lppl->lpplprod->rgprod()[0].pct = 50;

    // Player 0's order: replace the design.
    gd.fGeneratingTurn = TRUE;
    idPlayer = 0;
    shdef = rglpshdefSB[0][isb];
    shdef.det = detAll;
    memset(rgb, 0, sizeof(rgb));
    ((RTCHGSHDEF *)rgb)->ishdef = 16 + isb;
    ((RTCHGSHDEF *)rgb)->iPlr = 0;
    ((RTCHGSHDEF *)rgb)->mdChg = 1;
    pb = (uint8_t *)&((RTCHGSHDEF *)rgb)->rtshdef;
    WriteRtShDef(&shdef, &pb);
    TEST_ASSERT(FRunLogRecord(rtLogShDef, (int16_t)(pb - rgb), rgb));
    TEST_ASSERT(lppl->lpplprod != NULL && lppl->lpplprod->iprodMac == 1);
    TEST_CHECK_(lppl->lpplprod->rgprod()[0].pct == 0, "the edited design kept %d%%", lppl->lpplprod->rgprod()[0].pct);
}

// Trimmed insert/update records keep the old byte-prefix and neighboring-row
// effects, while the signed index and task-value admission remain unchanged.
static void test_FRunLogRecord_order_bytes(void) {
    char szDir[MAX_PATH];
    FLEET *lpfl;
    PLORD *lpplord;
    ORDER ordLast, ordNeighbor;
    HB *lphb;
    uint8_t *lpbFlag;
    uintptr_t ibFlag;
    uint16_t id, wBefore, wAfter;
    bool fResult;
    uint8_t rgb[40] = {};
    const uint8_t rgbShort[] = {0xe8, 3, 0xd0, 7, 13, 0, 0x41, 0xb2, 0xab};
    const uint8_t rgbFull[] = {0xe8, 3, 0xd0, 7, 13, 0, 0x41, 0xb2, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa};
    uint8_t rgbMissing[] = {0xff, 0xff};

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FRunLogRecord_order_bytes", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    lpfl = LpflStarsTestAddFleet(0, idPlanetDeepSpace, 0, 1);
    TEST_ASSERT(lpfl != NULL);
    FreePl(lpfl->lpplord);
    lpfl->lpplord = LpplAlloc<PLORD>(18, 2, htOrd);
    lpplord = lpfl->lpplord;
    memset(lpplord->rgord(), 0, 2 * sizeof(ORDER));
    lpplord->iordMac = lpfl->cord = 2;
    lpplord->rgord()[0].pt = {101, 201};
    lpplord->rgord()[1].pt = {102, 202};
    lpplord->rgord()[1].id = 18;
    lpplord->rgord()[1].txp.rgia[4].w = 0xbeef;
    lpplord->rgord()[1].fNoAutoTrack = TRUE;
    ordLast = lpplord->rgord()[1];
    id = static_cast<uint16_t>(lpfl->id);
    rgb[0] = static_cast<uint8_t>(id % 256);
    rgb[1] = static_cast<uint8_t>(id / 256);
    rgb[2] = 1;
    memcpy(rgb + 4, rgbShort, sizeof(rgbShort));
    TEST_ASSERT(FRunLogRecord(rtLogFleetOrderInsert, 4 + sizeof(rgbShort), rgb));
    lpplord = lpfl->lpplord;
    TEST_CHECK(lpfl->cord == 3 && lpplord->iordMac == 3 && lpplord->iordMax == 5);
    TEST_CHECK(lpplord->rgord()[0].pt.x == 101 && lpplord->rgord()[0].pt.y == 201);
    TEST_CHECK(memcmp(&lpplord->rgord()[2], &ordLast, sizeof(ORDER)) == 0);
    TEST_CHECK(lpplord->rgord()[1].pt.x == 1000 && lpplord->rgord()[1].pt.y == 2000 && lpplord->rgord()[1].id == 13);
    TEST_CHECK(lpplord->rgord()[1].grTask == 1 && lpplord->rgord()[1].iWarp == 4 && lpplord->rgord()[1].grobj == 2 &&
               lpplord->rgord()[1].fValidTask && !lpplord->rgord()[1].fNoAutoTrack && lpplord->rgord()[1].fUnused == 2);
    TEST_CHECK(lpplord->rgord()[1].txp.rgia[0].w == 0xab && lpplord->rgord()[1].txp.rgia[4].w == 0 && rgb[11] == 0xb2);

    // An oversized update used to copy the prefix of the following row too.
    // Keep that effect when the destination remains inside the owning pool.
    lpplord->rgord()[1].id = 0x1234;
    ordNeighbor = lpplord->rgord()[1];
    ordNeighbor.pt = {11, 22};
    ordNeighbor.id = 0x1278;
    rgb[2] = 0;
    memcpy(rgb + 4, rgbFull, sizeof(rgbFull));
    rgb[22] = 11; rgb[23] = 0; rgb[24] = 22; rgb[25] = 0; rgb[26] = 0x78;
    TEST_ASSERT(FRunLogRecord(rtLogFleetOrderUpdate, 27, rgb));
    TEST_CHECK(lpfl->cord == 3 && lpplord->iordMac == 3);
    TEST_CHECK(memcmp(&lpplord->rgord()[1], &ordNeighbor, sizeof(ORDER)) == 0 && memcmp(&lpplord->rgord()[2], &ordLast, sizeof(ORDER)) == 0);
    TEST_CHECK(lpplord->rgord()[0].txp.rgia[4].w == 0xaa99 && !lpplord->rgord()[0].fNoAutoTrack);

    // The original task update accepts a negative signed value and truncates
    // it to four bits. No new nonnegative-value policy is introduced.
    rgb[2] = rgb[3] = 0;
    rgb[4] = rgb[5] = 0xff;
    TEST_ASSERT(FRunLogRecord(rtLogFleetOrderAttrNib, 6, rgb));
    TEST_CHECK(lpplord->rgord()[0].grTask == 15 && lpplord->rgord()[0].iWarp == 4);
    rgb[2] = rgb[3] = 0xff;
    TEST_ASSERT(FRunLogRecord(rtLogFleetFlagBit9, 4, rgb));
    TEST_CHECK(lpfl->fRepOrders);

    // Preserve a bounded negative-index mutation without forming an ORDER
    // outside its array. Restore its neighboring bytes before any heap use.
    lphb = LphbFromLpHt(lpplord, htOrd);
    TEST_ASSERT(lphb != NULL);
    ibFlag = reinterpret_cast<uintptr_t>(lpplord) - reinterpret_cast<uintptr_t>(lphb->hmem) - 8;
    TEST_ASSERT(ibFlag + 2 <= lphb->cbBlock);
    lpbFlag = static_cast<uint8_t *>(lphb->hmem) + ibFlag;
    wBefore = RawLoad16(lpbFlag);
    rgb[4] = 3; rgb[5] = 0;
    fResult = FRunLogRecord(rtLogFleetOrderAttrNib, 6, rgb);
    wAfter = RawLoad16(lpbFlag);
    RawStore16(lpbFlag, wBefore);
    TEST_CHECK(fResult && wAfter == static_cast<uint16_t>((wBefore & 0xfff0) | 3));

    // Deleting two rows retains the high-bit flag and index interpretation.
    rgb[2] = 1; rgb[3] = 0x80;
    TEST_ASSERT(FRunLogRecord(rtLogFleetOrderDelete, 4, rgb));
    TEST_CHECK(lpfl->cord == 1 && lpplord->iordMac == 1 && lpplord->rgord()[0].grTask == 15);
    // Lookup failure must not consume an absent index/task suffix.
    TEST_CHECK(!FRunLogRecord(rtLogFleetOrderInsert, sizeof(rgbMissing), rgbMissing));
    TEST_CHECK(!FRunLogRecord(rtLogFleetOrderAttrNib, sizeof(rgbMissing), rgbMissing));
}

// Queue log replay preserves matched partial progress, clears unmatched input
// progress in place, retains reserved bits and ignores a trailing partial row.
static void test_FRunLogRecord_production_bytes(void) {
    char szDir[MAX_PATH];
    PLANET *lppl;
    uint8_t rgb[] = {0, 0, 2, 0x1c, 0x72, 0xfb, 3, 0x20, 0xf2, 0x39, 0xa5};
    int16_t cPlanetSav;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("FRunLogRecord_production_bytes", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(0);
    TEST_ASSERT(lppl != NULL);
    FreePl(lppl->lpplprod);
    lppl->lpplprod = LpplAlloc<PLPROD>(4, 1, htOrd);
    lppl->lpplprod->iprodMac = 1;
    lppl->lpplprod->rgprod()[0] = {};
    lppl->lpplprod->rgprod()[0].iItem = 7;
    lppl->lpplprod->rgprod()[0].grobj = grobjPlanet;
    lppl->lpplprod->rgprod()[0].pct = 22;
    rgb[0] = static_cast<uint8_t>(lppl->id % 256);
    rgb[1] = static_cast<uint8_t>(lppl->id / 256);
    idPlayer = 0;
    TEST_ASSERT(FRunLogRecord(rtLogPlanetProdQ, sizeof(rgb), rgb));
    TEST_CHECK(lppl->lpplprod->iprodMac == 2 && lppl->lpplprod->iprodMax == 4);
    TEST_CHECK(lppl->lpplprod->rgprod()[0].cItem == 2 && lppl->lpplprod->rgprod()[0].iItem == 7 &&
               lppl->lpplprod->rgprod()[0].grobj == grobjPlanet && lppl->lpplprod->rgprod()[0].pct == 55 &&
               lppl->lpplprod->rgprod()[0].unused == 31);
    TEST_CHECK(lppl->lpplprod->rgprod()[1].cItem == 3 && lppl->lpplprod->rgprod()[1].iItem == 8 &&
               lppl->lpplprod->rgprod()[1].grobj == grobjPlanet && lppl->lpplprod->rgprod()[1].pct == 0 &&
               lppl->lpplprod->rgprod()[1].unused == 7);
    TEST_CHECK(rgb[6] == 3 && rgb[7] == 0x20 && rgb[8] == 2 && rgb[9] == 0x38 && rgb[10] == 0xa5);
    // A zero declared payload still reads the available ID prefix, then clears
    // the queue under the old signed-count rule. It gains no new rejection.
    TEST_ASSERT(FRunLogRecord(rtLogPlanetProdQ, 0, rgb));
    TEST_CHECK(!lppl->lpplprod);
    cPlanetSav = cPlanet;
    cPlanet = 0;
    TEST_CHECK(!FRunLogRecord(rtLogPlanetProdQ, 0, nullptr));
    cPlanet = cPlanetSav;
}

static void test_order_storage_lifetime(void) {
    PLORD *lpplord;
    PLORD *lpplordOld;
    ORDER *lpord;
    void *lpOther;
    uint16_t cbSlot;
    int16_t i;

    TEST_ASSERT(FStarsTestInit());
    lpplord = LpplAlloc<PLORD>(18, 3, htOrd);
    lpplord->iordMac = 3;
    lpplord->cAlloc = 9;
    lpplord->fMark = TRUE;
    lpord = lpplord->rgord();
    cbSlot = CbPlSlot(lpplord);
    for (i = 0; i < 3; i++) {
        lpord[i] = {};
        lpord[i].pt = {static_cast<int16_t>(100 + i), static_cast<int16_t>(200 + i)};
        lpord[i].id = i;
        lpord[i].txp.rgia[4].w = static_cast<uint16_t>(0xf120 + i);
    }
    TEST_CHECK(reinterpret_cast<const uint8_t *>(&lpord[1]) - reinterpret_cast<const uint8_t *>(&lpord[0]) == 18);
    lpplordOld = lpplord;
    lpplord = LpplReAlloc(lpplord, 3);
    TEST_CHECK(lpplord == lpplordOld && lpplord->rgord() == lpord && CbPlSlot(lpplord) == cbSlot);
    lpOther = LpAlloc(512, htOrd);
    lpplord = LpplReAlloc(lpplord, 32);
    TEST_CHECK(lpplord != lpplordOld && lpplord->iordMax == 32 && lpplord->iordMac == 3);
    TEST_CHECK(lpplord->cAlloc == 9 && lpplord->fMark);
    for (i = 0; i < 3; i++) {
        TEST_CHECK(lpplord->rgord()[i].pt.x == 100 + i && lpplord->rgord()[i].pt.y == 200 + i && lpplord->rgord()[i].id == i);
        TEST_CHECK(lpplord->rgord()[i].txp.rgia[4].w == static_cast<uint16_t>(0xf120 + i));
    }
    lpplordOld = lpplord;
    cbSlot = CbPlSlot(lpplord);
    lpplord = LpplReAlloc(lpplord, 0);
    TEST_CHECK(lpplord == lpplordOld && lpplord->iordMax == 0 && CbPlSlot(lpplord) == cbSlot);
    lpplord = LpplReAlloc(lpplord, 256);
    TEST_CHECK(lpplord->iordMax == 0 && lpplord->iordMac == 3 && CbPlSlot(lpplord) >= 4612);
    lpplord->rgord()[255] = {};
    lpplord->rgord()[255].txp.rgia[4].w = 0xffff;
    TEST_CHECK(lpplord->rgord()[255].txp.rgia[4].iAction() == 15 && lpplord->rgord()[0].id == 0);
    FreePl(lpplord);
    FreeLp(lpOther, htOrd);
}

// Typed queue construction keeps the old physical slot, byte counters and
// spare capacity, including growth that must move past another live allocation.
static void test_production_storage_lifetime(void) {
    PLPROD *lpplprod;
    PLPROD *lpplprodOld;
    PROD *lpprod;
    void *lpOther;
    uint16_t cbSlot;
    int16_t i;

    TEST_ASSERT(FStarsTestInit());
    lpplprod = LpplAlloc<PLPROD>(4, 3, htOrd);
    lpplprod->iprodMac = 3;
    lpplprod->cAlloc = 11;
    lpplprod->fMark = TRUE;
    lpprod = lpplprod->rgprod();
    cbSlot = CbPlSlot(lpplprod);
    TEST_CHECK(cbSlot >= 16);
    for (i = 0; i < 3; i++) {
        lpprod[i] = {};
        lpprod[i].cItem = i + 1;
        lpprod[i].pct = i * 17;
        lpprod[i].unused = i + 7;
    }
    lpplprodOld = lpplprod;
    lpplprod = LpplReAlloc(lpplprod, 3);
    TEST_CHECK(lpplprod == lpplprodOld && lpplprod->rgprod() == lpprod && CbPlSlot(lpplprod) == cbSlot);
    lpOther = LpAlloc(512, htOrd);
    lpplprod = LpplReAlloc(lpplprod, 32);
    TEST_CHECK(lpplprod != lpplprodOld && lpplprod->iprodMax == 32 && lpplprod->iprodMac == 3);
    TEST_CHECK(lpplprod->cAlloc == 11 && lpplprod->fMark);
    for (i = 0; i < 3; i++) {
        TEST_CHECK(lpplprod->rgprod()[i].cItem == static_cast<uint32_t>(i + 1));
        TEST_CHECK(lpplprod->rgprod()[i].pct == static_cast<uint32_t>(i * 17));
        TEST_CHECK(lpplprod->rgprod()[i].unused == static_cast<uint32_t>(i + 7));
    }
    lpplprodOld = lpplprod;
    cbSlot = CbPlSlot(lpplprod);
    lpplprod = LpplReAlloc(lpplprod, 0);
    TEST_CHECK(lpplprod == lpplprodOld && lpplprod->iprodMax == 0 && CbPlSlot(lpplprod) == cbSlot);
    lpplprod = LpplReAlloc(lpplprod, 256);
    TEST_CHECK(lpplprod->iprodMax == 0 && lpplprod->iprodMac == 3 && CbPlSlot(lpplprod) >= 1028);
    lpplprod->rgprod()[255] = {};
    lpplprod->rgprod()[255].cItem = 1023;
    TEST_CHECK(lpplprod->rgprod()[255].cItem == 1023 && lpplprod->rgprod()[0].cItem == 1);
    FreePl(lpplprod);
    FreeLp(lpOther, htOrd);
}

TEST_LIST = {{"orders retain typed storage, stride and pool capacity", test_order_storage_lifetime},
             {"order log bytes preserve prefixes, neighbors and signed admission", test_FRunLogRecord_order_bytes},
             {"production log bytes preserve replay and incomplete-row effects", test_FRunLogRecord_production_bytes},
             {"production queues retain typed storage and pool capacity", test_production_storage_lifetime},
             {"FRunLogRecord refunds undelivered cargo", test_FRunLogRecord_refunds_undelivered_cargo},
             {"fleet-name orders reject malformed UTF-8 without changing the name", test_FRunLogRecord_rejects_invalid_utf8_name},
             {"FRunLogRecord design change clears queued progress", test_FRunLogRecord_design_change_clears_progress},
             {NULL, NULL}};
