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
    lppl->lpplprod = (PLPROD *)LpplAlloc(4, 3, htOrd);
    lppl->lpplprod->iprodMac = 1;
    lppl->lpplprod->rgprod[0].cItem = 1;
    lppl->lpplprod->rgprod[0].iItem = 16 + isb;
    lppl->lpplprod->rgprod[0].grobj = grobjFleet;
    lppl->lpplprod->rgprod[0].pct = 50;

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
    TEST_CHECK_(lppl->lpplprod->rgprod[0].pct == 0, "the edited design kept %d%%", lppl->lpplprod->rgprod[0].pct);
}

TEST_LIST = {{"FRunLogRecord refunds undelivered cargo", test_FRunLogRecord_refunds_undelivered_cargo},
             {"fleet-name orders reject malformed UTF-8 without changing the name", test_FRunLogRecord_rejects_invalid_utf8_name},
             {"FRunLogRecord design change clears queued progress", test_FRunLogRecord_design_change_clears_progress},
             {NULL, NULL}};
