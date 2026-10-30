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
    rgb[0] = (uint8_t)lppl->id; rgb[1] = (uint8_t)((uint16_t)lppl->id >> 8);
    rgb[2] = (uint8_t)idfl; rgb[3] = (uint8_t)((uint16_t)idfl >> 8);
    rgb[4] = 0x21; rgb[5] = 1; rgb[6] = 0x9c;
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
    rgb[0] = 1; // mdChg=1, player=0; design occupies bits 8–12.
    rgb[1] = 16 + isb;
    pb = rgb + cbWireChgShDef;
    WriteRtShDef(&shdef, &pb);
    TEST_ASSERT(FRunLogRecord(rtLogShDef, (int16_t)(pb - rgb), rgb));
    TEST_ASSERT(lppl->lpplprod != NULL && lppl->lpplprod->iprodMac == 1);
    TEST_CHECK_(lppl->lpplprod->rgprod()[0].pct == 0, "the edited design kept %d%%", lppl->lpplprod->rgprod()[0].pct);
}


// Rejected and unknown design edits retain their distinct partial-effect
// receipts. In particular, failed replacement can clear progress and decrement
// the roster count; decoding must not turn the workflow into a transaction.
static void test_FRunLogRecord_design_partial_effects(void) {
    char szDir[MAX_PATH];
    PLANET *lppl;
    SHDEF shdef, shdefBefore;
    uint8_t rgb[] = {1, 0xe0}; // mode=1, player=0, slot=0; all junk bits set.
    uint8_t rgbShort[] = {1};
    uint8_t rgbWritten[160];
    HDR hdr;
    int16_t cBefore;
    int16_t i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("design_log_partial_effects", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());
    idPlayer = 0;
    gd.fGeneratingTurn = TRUE;
    for (i = 0; i < cPlanet && lpPlanets[i].iPlayer != 0; i++) {}
    TEST_ASSERT(i < cPlanet);
    lppl = &lpPlanets[i];
    if (lppl->lpplprod) FreePl(lppl->lpplprod);
    lppl->lpplprod = LpplAlloc<PLPROD>(4, 3, htOrd);
    lppl->lpplprod->iprodMac = 1;
    lppl->lpplprod->rgprod()[0] = {};
    lppl->lpplprod->rgprod()[0].grobj = grobjFleet;
    lppl->lpplprod->rgprod()[0].iItem = 0;
    lppl->lpplprod->rgprod()[0].pct = 37;
    rglpshdef[0][0].fFree = FALSE;
    rglpshdef[0][0].cExist = 1;
    cBefore = rgplr[0].cShDef;
    TEST_CHECK(!FRunLogRecord(rtLogShDef, 1, rgbShort));
    TEST_CHECK(!FRunLogRecord(rtLogShDef, sizeof(rgb), rgb));
    TEST_CHECK(lppl->lpplprod->rgprod()[0].pct == 37 && rgplr[0].cShDef == cBefore);
    rglpshdef[0][0].cExist = 0;
    shdefBefore = rglpshdef[0][0];
    rgb[0] = 15; // An unrecognized mode is accepted after clearing progress.
    TEST_CHECK(FRunLogRecord(rtLogShDef, sizeof(rgb), rgb));
    TEST_CHECK(lppl->lpplprod->rgprod()[0].pct == 0 && rgplr[0].cShDef == cBefore);
    TEST_CHECK(memcmp(&rglpshdef[0][0], &shdefBefore, sizeof(shdefBefore)) == 0);
    lppl->lpplprod->rgprod()[0].pct = 37;
    rgb[0] = 1;
    TEST_CHECK(!FRunLogRecord(rtLogShDef, sizeof(rgb), rgb));
    TEST_CHECK(lppl->lpplprod->rgprod()[0].pct == 0 && rgplr[0].cShDef == cBefore - 1);
    TEST_CHECK(memcmp(&rglpshdef[0][0], &shdefBefore, sizeof(shdefBefore)) == 0);
    // The real writer emits the same two-byte header and a full design. Only
    // previously indeterminate junk bits are now initialized to zero.
    shdef = shdefBefore;
    shdef.det = detSome;
    gd.fGeneratingTurn = FALSE;
    LogChangeShDef(&shdef);
    TEST_CHECK(shdef.det == detAll);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgbWritten));
    TEST_CHECK(hdr.rt == rtLogShDef && hdr.cb > 2 && rgbWritten[0] == 1 && rgbWritten[1] == 0);
    TEST_CHECK(rgbWritten[2] == detAll && rgbWritten[4] == shdef.hul.ihuldef);
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

// These are actual two-/three-byte allocations: deletion and rejection must
// not peek at a missing target/name prefix, including an ignored wrong owner.
static void test_battle_plan_prefix_replay(void) {
    uint8_t deletion[] = {0, 0x40};
    uint8_t wrongOwner[] = {1, 0x40};
    uint8_t badIndex[] = {0xf0, 0x40};
    uint8_t shortPlan[] = {0, 0};
    uint8_t threeBytes[] = {0, 0, 0};
    uint8_t oneByte[] = {0};
    uint8_t invalidName[] = {0, 0, 0, 0, 0, 0xc3, 0};
    uint8_t validName[] = {0x10, 0xb2, 0x35, 0xe4, 0, 'X', 0};
    BTLPLAN rgplan[16]{};

    TEST_ASSERT(FStarsTestInit());
    idPlayer = 0; rglpbtlplan[0] = rgplan; rgcbtlplan[0] = 2;
    std::strcpy(rgplan[0].szName, "First"); std::strcpy(rgplan[1].szName, "Second"); rgplan[1].iplan = 1;
    TEST_CHECK(FRunLogRecord(rtBtlPlan, 2, wrongOwner) && rgcbtlplan[0] == 2);
    TEST_CHECK(FRunLogRecord(rtBtlPlan, 2, badIndex) && rgcbtlplan[0] == 2);
    TEST_CHECK(!FRunLogRecord(rtBtlPlan, 0, NULL));
    TEST_CHECK(!FRunLogRecord(rtBtlPlan, 1, oneByte));
    TEST_CHECK(!FRunLogRecord(rtBtlPlan, 2, shortPlan));
    TEST_CHECK(!FRunLogRecord(rtBtlPlan, 3, threeBytes));
    TEST_CHECK(FRunLogRecord(rtBtlPlan, 2, deletion));
    TEST_CHECK(rgcbtlplan[0] == 1 && rgplan[0].iplan == 0 && !strcmp(rgplan[0].szName, "Second"));
    TEST_CHECK(!FRunLogRecord(rtBtlPlan, sizeof(invalidName), invalidName));
    TEST_CHECK(rgcbtlplan[0] == 1 && !strcmp(rgplan[0].szName, "Second"));
    TEST_CHECK(FRunLogRecord(rtBtlPlan, sizeof(validName), validName));
    TEST_CHECK(rgcbtlplan[0] == 2 && rgplan[1].iplan == 1 && !strcmp(rgplan[1].szName, "X"));
    TEST_CHECK(rgplan[1].iplr == 0 && rgplan[1].mdTactic == 2 && rgplan[1].unused1 == 3 && rgplan[1].fDumpCargo);
    TEST_CHECK(rgplan[1].mdTarget1 == 5 && rgplan[1].mdTarget2 == 3 && rgplan[1].iplrAttack == 4 && rgplan[1].unused2 == 7);
    rglpbtlplan[0] = NULL; rgcbtlplan[0] = 0;
}

// Invalid technology still leaves a valid percentage applied. Invalid
// percentage rejects from one physical byte before touching technology data.
static void test_control_record_partial_effects(void) {
    uint8_t negative[] = {0x80}, tooLarge[] = {101};
    uint8_t badNow[] = {25, 0x06}, badNext[] = {50, 0x80}, valid[] = {100, 0x75};
    uint8_t password[] = {0x78, 0x56, 0x34, 0x12};
    uint8_t minusOne[] = {0xff, 0xff, 0xff, 0xff};
    uint8_t rgb[32];
    HDR hdr;

    TEST_ASSERT(FStarsTestInit());
    idPlayer = 0; rgplr[0].pctResearch = 17; rgplr[0].iTechCur = 0x12;
    TEST_CHECK(!FRunLogRecord(rtLogResearch, 1, negative));
    TEST_CHECK(!FRunLogRecord(rtLogResearch, 1, tooLarge));
    TEST_CHECK(rgplr[0].pctResearch == 17 && rgplr[0].iTechCur == 0x12);
    TEST_CHECK(!FRunLogRecord(rtLogResearch, 2, badNow));
    TEST_CHECK(rgplr[0].pctResearch == 25 && rgplr[0].iTechCur == 0x12);
    TEST_CHECK(!FRunLogRecord(rtLogResearch, 2, badNext));
    TEST_CHECK(rgplr[0].pctResearch == 50 && rgplr[0].iTechCur == 0x12);
    TEST_CHECK(FRunLogRecord(rtLogResearch, 0, valid)); // Declared length never became the old field-read bound.
    TEST_CHECK(rgplr[0].pctResearch == 100 && rgplr[0].iTechCur == 0x75);
    gd.fGeneratingTurn = FALSE; rgplr[0].lSalt = 42;
    TEST_CHECK(FRunLogRecord(rtChgPassword, 0, NULL) && rgplr[0].lSalt == 42);
    gd.fGeneratingTurn = TRUE;
    TEST_CHECK(FRunLogRecord(rtChgPassword, 0, password) && rgplr[0].lSalt == 0x12345678);
    TEST_CHECK(FRunLogRecord(rtChgPassword, 4, minusOne) && rgplr[0].lSalt == -1);
    gd.fGeneratingTurn = FALSE;
    WriteMemResearch(static_cast<int16_t>(0x7319));
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogResearch && hdr.cb == 2 && rgb[0] == 25 && rgb[1] == 0x73);
    WriteMemResearch(static_cast<int16_t>(0xff80));
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.cb == 2 && rgb[0] == 0x80 && rgb[1] == 0xff);
    WriteMemPassword(-1);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtChgPassword && hdr.cb == 4 && memcmp(rgb, minusOne, 4) == 0);
}

static void test_cargo_log_wire_widths(void) {
    struct XFERCASE { int32_t rgQuan[5]; RecordType rt; uint8_t mask; uint8_t rgb[12]; int16_t cb; };
    const XFERCASE cases[] = {
        {{127, -127, 0, 1, 0}, rtLogCargoXfer8, 0x0b, {0x7f, 0x81, 1}, 3},
        {{-128, 32767, 0, 0, 1}, rtLogCargoXfer16, 0x13, {0x80, 0xff, 0xff, 0x7f, 1, 0}, 6},
        {{-32768, 32768, 0, -1, 0}, rtLogCargoXfer32, 0x0b, {0, 0x80, 0xff, 0xff, 0, 0x80, 0, 0, 0xff, 0xff, 0xff, 0xff}, 12},
    };
    LOGXFER source{}, target{};
    HDR hdr;
    uint8_t rgb[32];
    const uint8_t prefix[] = {0x41, 1, 0x8e, 2, 0x42};

    TEST_ASSERT(FStarsTestInit());
    source.id = 321; source.grobj = grobjFleet; target.id = 654; target.grobj = grobjOther;
    for (const XFERCASE &v : cases) {
        imemLogCur = 0; imemLogPrev = -1; hdrPrev.rt = rtEOF;
        memcpy(source.rgdItem, v.rgQuan, sizeof(source.rgdItem));
        LogMakeValidXfer(&source, &target);
        TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
        TEST_CHECK(hdr.rt == v.rt && hdr.cb == 6 + v.cb);
        TEST_CHECK(memcmp(rgb, prefix, sizeof(prefix)) == 0 && rgb[5] == v.mask && memcmp(rgb + 6, v.rgb, v.cb) == 0);
    }
    imemLogCur = 0; imemLogPrev = -1; hdrPrev.rt = rtEOF;
    source.rgdItem[0] = 127; source.rgdItem[1] = -127; source.rgdItem[2] = 0; source.rgdItem[3] = 1; source.rgdItem[4] = 0;
    LogMakeValidXfer(&source, &target);
    source.rgdItem[0] = -126; source.rgdItem[1] = 127; source.rgdItem[3] = -1;
    LogMakeValidXfer(&source, &target);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogCargoXfer8 && hdr.cb == 7 && rgb[5] == 1 && rgb[6] == 1 && imemLogCur == 9);
    source.rgdItem[0] = -1; source.rgdItem[1] = 0; source.rgdItem[3] = 0;
    LogMakeValidXfer(&source, &target);
    TEST_CHECK(imemLogCur == 0 && hdrPrev.rt == rtEOF);
    // Writer-only signed-ID fixture: old unsigned wire IDs do not compare
    // equal to negative native IDs. Never replay a negative lookup ID here.
    imemLogCur = 0; imemLogPrev = -1; hdrPrev.rt = rtEOF;
    source.id = -1; source.rgdItem[0] = 5;
    LogMakeValidXfer(&source, &target);
    source.rgdItem[0] = 6;
    LogMakeValidXfer(&source, &target);
    TEST_CHECK(imemLogCur == 18);
}

static void test_cargo_lookup_prefixes(void) {
    char szDir[MAX_PATH];
    FLEET *lpfl;
    PLANET *lppl;
    uint8_t missing[] = {0xff, 1, 0, 0, 0x42};
    uint8_t foreign[] = {0, 0, 0xff, 3, 0x22};
    int16_t ish;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("cargo_lookup_prefixes", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost()); idPlayer = 0;
    TEST_ASSERT(LpflFromId(511) == NULL && LpflFromId(1023) == NULL);
    for (ish = 0; ish < 16 && rglpshdef[0][ish].fFree; ish++) {}
    TEST_ASSERT(ish < 16);
    lppl = LpplStarsTestHomeworld(0); TEST_ASSERT(lppl != NULL);
    lpfl = LpflStarsTestAddFleet(0, lppl->id, ish, 1);
    foreign[0] = (uint8_t)lpfl->id; foreign[1] = (uint8_t)((uint16_t)lpfl->id >> 8);
    const RecordType rgrt[] = {rtLogCargoXfer8, rtLogCargoXfer16, rtLogCargoXfer32};
    for (RecordType rt : rgrt) {
        TEST_CHECK(!FRunLogRecord(rt, 0, missing));
        TEST_CHECK(FRunLogRecord(rt, 0, foreign));
    }
}

static int16_t FEnumHeaderMutation(void *lp, int16_t rt, int16_t cb, void *lpPass, int16_t iPass) {
    int *pc = (int *)lpPass;
    (void)iPass;
    if (*pc == 0) {
        TEST_CHECK(rt == 63 && cb == 1 && lp == lpLog + 2);
        lpLog[0] = 2; // callback changes the stride used after it returns
    } else {
        TEST_CHECK(rt == 62 && cb == 0 && lp == lpLog + 6);
    }
    (*pc)++;
    return TRUE;
}

static void test_log_header_bytes_and_callback(void) {
    const uint8_t rgb[] = {0x11, 0x22, 0x33};
    uint8_t readback[3];
    HDR hdr;
    int c = 0;

    TEST_ASSERT(FStarsTestInit());
    fAi = TRUE; fLogOff = FALSE; imemLogCur = 0; imemLogPrev = -1;
    WriteMemRt(static_cast<RecordType>(63), 3, (void *)rgb);
    TEST_CHECK(imemLogCur == 5 && lpLog[0] == 3 && lpLog[1] == 0xfc && !memcmp(lpLog + 2, rgb, 3));
    TEST_ASSERT(FGetPrevLogRt(&hdr, readback));
    TEST_CHECK(hdr.cb == 3 && hdr.rt == 63 && !memcmp(readback, rgb, 3));
    lpLog[0] = 1; lpLog[1] = 0xfc;
    lpLog[4] = 0; lpLog[5] = 0xf8;
    imemLogCur = 6;
    EnumLogRts(FEnumHeaderMutation, &c, 0);
    TEST_CHECK(c == 2);
}

static void test_routing_writer_bytes(void) {
    PLANET pl{}, plNew{};
    const uint8_t golden[] = {0x45, 0x23, 0xab, 0x7a, 0x55, 1};
    uint8_t rgb[6];
    HDR hdr;

    TEST_ASSERT(FStarsTestInit());
    fAi = TRUE; fLogOff = FALSE; gd.fGeneratingTurn = FALSE; fValidLx = FALSE;
    imemLogCur = 0; imemLogPrev = -1; hdrPrev.rt = rtEOF;
    pl.id = plNew.id = 0x2345;
    plNew.fNoResearch = 1; plNew.idFling = 0x155; plNew.iWarpFling = 15; plNew.idRoute = 0x2aa;
    LogChangePlanet(&pl, &plNew);
    TEST_ASSERT(FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogPlanetRouting && hdr.cb == 6 && !memcmp(rgb, golden, sizeof(golden)));
}

static void test_routing_replay_read_boundaries(void) {
    char szDir[MAX_PATH];
    uint8_t missing[] = {0xff, 1};
    uint8_t routing[] = {0, 0, 0xab, 0x7a, 0x55, 1};
    uint8_t noThing[] = {0xff, 0x7f};
    uint8_t foreign[2];
    PLANET *lppl;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("routing-prefixes", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost()); idPlayer = 0;
    TEST_CHECK(!FRunLogRecord(rtLogPlanetRouting, 0, missing));
    lppl = LpplStarsTestHomeworld(0); TEST_ASSERT(lppl != NULL);
    routing[0] = (uint8_t)lppl->id; routing[1] = (uint8_t)((uint16_t)lppl->id >> 8);
    TEST_CHECK(FRunLogRecord(rtLogPlanetRouting, 0, routing));
    TEST_CHECK(lppl->fNoResearch && lppl->idFling == 0x155 && lppl->iWarpFling == 15 && lppl->idRoute == 0x2aa);
    lppl->iPlayer = 1;
    memcpy(foreign, routing, sizeof(foreign));
    TEST_CHECK(!FRunLogRecord(rtLogPlanetRouting, 0, foreign));
    TEST_CHECK(!FRunLogRecord(rtLogPlanetRouting, 0, missing));
    {
        StarsRestore<int16_t> restoreCount(cThing);
        cThing = 0;
        TEST_CHECK(!FRunLogRecord(rtThing, 0, noThing));
    }
    TEST_CHECK(rtLogThingByteParam == rtThing);
    TEST_CHECK(!FRunLogRecord(rtLogThingByteParam, 0, noThing));
    TEST_CHECK(FRunLogRecord(static_cast<RecordType>(63), 0, NULL));
}

static void ChangeZipProdDuringAlert() {
    vrgZipProd[0].fNoResearch = 0x42;
    vrgZipProd[0].cpq = 0xab;
}

static void test_zipprod_replay_and_callback_order(void) {
    uint8_t incoming[26], expected[26], actual[26];
    uint8_t *lpLogSav;
    int16_t imemSav, imemPrevSav;
    HDR hdr, hdrSav;
    ZIPPRODQ rgZipSav[5];
    int16_t fDirtySav, fLogOffSav;
    std::array<uint8_t, 32100> rgbLog{};

    TEST_ASSERT(FStarsTestInit());
    idPlayer = 0;
    for (int i = 0; i < 5; i++) rgZipSav[i] = vrgZipProd[i];
    fDirtySav = game.fDirty; fLogOffSav = fLogOff;
    for (int i = 0; i < 26; i++) incoming[i] = static_cast<uint8_t>(i * 7 + 5);
    gd.fGeneratingTurn = TRUE;
    for (int cb = 0; cb <= 26; cb++) {
        rgplr[0].zpq1.fNoResearch = 0x11; rgplr[0].zpq1.cpq = 0x22;
        expected[0] = 0x11; expected[1] = 0x22;
        for (int i = 0; i < 12; i++) {
            rgplr[0].zpq1.rgpq[i].w = static_cast<uint16_t>(0xa5c0 + i);
            expected[2 + i * 2] = static_cast<uint8_t>(0xc0 + i); expected[3 + i * 2] = 0xa5;
        }
        memcpy(expected, incoming, cb);
        TEST_CHECK(FRunLogRecord(rtLogPlayerZpq1, cb, cb == 0 ? nullptr : incoming));
        FEncodeZipProdQ1(rgplr[0].zpq1, std::as_writable_bytes(std::span(actual)));
        TEST_CHECK(!memcmp(actual, expected, 26));
    }
    TEST_CHECK(!FRunLogRecord(rtLogPlayerZpq1, -1, nullptr));
    TEST_CHECK(!FRunLogRecord(rtLogPlayerZpq1, 27, nullptr));
    gd.fGeneratingTurn = FALSE;
    TEST_CHECK(FRunLogRecord(rtLogPlayerZpq1, -1, nullptr));
    lpLogSav = lpLog; imemSav = imemLogCur; imemPrevSav = imemLogPrev; hdrSav = hdrPrev;
    lpLog = rgbLog.data(); imemLogCur = 31999; fLogOff = FALSE; game.fDirty = TRUE;
    vrgZipProd[0].fNoResearch = 1; vrgZipProd[0].cpq = 0;
    pfnStarsTestAlertObserver = ChangeZipProdDuringAlert;
    WriteMemZpqRt(2);
    pfnStarsTestAlertObserver = nullptr;
    FDecodeHdr(std::as_bytes(std::span(rgbLog)).subspan(31999, 2), hdr);
    TEST_CHECK(hdr.rt == rtLogPlayerZpq1 && hdr.cb == 2 && imemLogCur == 32003);
    TEST_CHECK(rgbLog[32001] == 0x42 && rgbLog[32002] == 0xab);
    // A historical over-count copies the following physical templates too.
    imemLogCur = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 13; j++) vrgZipProd[i].szName[j] = static_cast<char>(i * 17 + j);
        vrgZipProd[i].fValid = static_cast<uint8_t>(i + 3);
        vrgZipProd[i].fNoResearch = static_cast<uint8_t>(i + 5); vrgZipProd[i].cpq = static_cast<uint8_t>(i + 7);
        for (int j = 0; j < 12; j++) vrgZipProd[i].rgpq[j].w = static_cast<uint16_t>(i * 256 + j);
    }
    WriteMemZpqRt(186);
    for (int ib = 0; ib < 186; ib++) {
        const int templateIndex = (ib + 14) / 40, offset = (ib + 14) % 40;
        uint8_t ch;
        if (offset < 13) ch = static_cast<uint8_t>(templateIndex * 17 + offset);
        else if (offset == 13) ch = static_cast<uint8_t>(templateIndex + 3);
        else if (offset == 14) ch = static_cast<uint8_t>(templateIndex + 5);
        else if (offset == 15) ch = static_cast<uint8_t>(templateIndex + 7);
        else ch = static_cast<uint8_t>((offset & 1) ? templateIndex : (offset - 16) / 2);
        TEST_CHECK(rgbLog[2 + ib] == ch);
    }
    lpLog = lpLogSav; imemLogCur = imemSav; imemLogPrev = imemPrevSav; hdrPrev = hdrSav;
    for (int i = 0; i < 5; i++) vrgZipProd[i] = rgZipSav[i];
    game.fDirty = fDirtySav; fLogOff = fLogOffSav;
}

static void test_zipprod_comparison_physical_tail(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH + 64];
    PLAYER plrSav;
    ZIPPRODQ rgZipSav[5];
    GAME gameSav;
    HDR hdr, hdrSav;
    int16_t imemSav, imemPrevSav, fLogOffSav, idPlayerSav, cmsgSav;
    uint8_t rgb[32];

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("zipprod-physical-tail", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sgame", szDir, szDirSep);
    plrSav = rgplr[0]; gameSav = game; hdrSav = hdrPrev;
    imemSav = imemLogCur; imemPrevSav = imemLogPrev; fLogOffSav = fLogOff; idPlayerSav = idPlayer; cmsgSav = vcmsgplrOut;
    for (int i = 0; i < 5; i++) rgZipSav[i] = vrgZipProd[i];
    vrgZipProd[0] = ZIPPRODQ{}; vrgZipProd[0].cpq = 13;
    strcpy(vrgZipProd[1].szName, "QR");
    rgplr[0] = PLAYER{}; rgplr[0].zpq1 = vrgZipProd[0].zpq1();
    rgplr[0].rgmdRelation[0] = 'Q'; rgplr[0].rgmdRelation[1] = 'R';
    idPlayer = 0; game.cPlayer = 1; fLogOff = FALSE; vcmsgplrOut = 0;
    imemLogCur = 0; imemLogPrev = -1; hdrPrev.rt = rtEOF;
    TEST_CHECK(FWriteLogFile(szFile, 0));
    TEST_CHECK(imemLogCur == 0);
    // Only a byte beyond the 26-byte template differs. It still triggers the
    // old 28-byte comparison and copies the following template's name bytes.
    rgplr[0].rgmdRelation[1] = 'S'; hdrPrev.rt = rtEOF;
    TEST_CHECK(FWriteLogFile(szFile, 0));
    TEST_CHECK(imemLogCur == 30 && FGetPrevLogRt(&hdr, rgb));
    TEST_CHECK(hdr.rt == rtLogPlayerZpq1 && hdr.cb == 28 && rgb[1] == 13 && rgb[26] == 'Q' && rgb[27] == 'R');
    SetSzWorkFromDt(dtLog, 0);
    TEST_CHECK(RemoveNativeFile(szWork) == 0);
    rgplr[0] = plrSav; game = gameSav; hdrPrev = hdrSav;
    imemLogCur = imemSav; imemLogPrev = imemPrevSav; fLogOff = fLogOffSav; idPlayer = idPlayerSav; vcmsgplrOut = cmsgSav;
    for (int i = 0; i < 5; i++) vrgZipProd[i] = rgZipSav[i];
}

TEST_LIST = {{"default template comparison retains adjacent physical bytes", test_zipprod_comparison_physical_tail},
             {"production template replay retains odd bytes and callback timing", test_zipprod_replay_and_callback_order},
             {"routing writer preserves exact prefix bytes", test_routing_writer_bytes},
             {"routing and thing logs retain prefix/no-read paths", test_routing_replay_read_boundaries},
             {"log framing uses bytes and re-reads callback-mutated strides", test_log_header_bytes_and_callback},
             {"cargo log widths, coalescing and signed ID comparisons", test_cargo_log_wire_widths},
             {"cargo lookup failures stop before a missing mask", test_cargo_lookup_prefixes},
             {"control records retain partial research and password reads", test_control_record_partial_effects},
             {"battle-plan replay retains short deletion and publication", test_battle_plan_prefix_replay},
             {"design log codecs retain partial-effect receipts", test_FRunLogRecord_design_partial_effects},
             {"orders retain typed storage, stride and pool capacity", test_order_storage_lifetime},
             {"order log bytes preserve prefixes, neighbors and signed admission", test_FRunLogRecord_order_bytes},
             {"production log bytes preserve replay and incomplete-row effects", test_FRunLogRecord_production_bytes},
             {"production queues retain typed storage and pool capacity", test_production_storage_lifetime},
             {"FRunLogRecord refunds undelivered cargo", test_FRunLogRecord_refunds_undelivered_cargo},
             {"fleet-name orders reject malformed UTF-8 without changing the name", test_FRunLogRecord_rejects_invalid_utf8_name},
             {"FRunLogRecord design change clears queued progress", test_FRunLogRecord_design_change_clears_progress},
             {NULL, NULL}};
