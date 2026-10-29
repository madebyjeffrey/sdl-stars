#include "acutest.h"

#include "stars_test.h"

// IshColonyDesign returns iPlr's first design carrying a colonization
// module and that module's slot in *ihs, or -1.
static int16_t IshColonyDesign(int16_t iPlr, int16_t *ihs) {
    int16_t ish;
    int16_t j;
    HUL    *lphul;

    for (ish = 0; ish < 16; ish++) {
        if (rglpshdef[iPlr][ish].fFree)
            continue;
        lphul = &rglpshdef[iPlr][ish].hul;
        for (j = 0; j < lphul->chs; j++) {
            if (lphul->rghs[j].grhst == hstSpecialM && lphul->rghs[j].iItem == ispecialMColonizationModule && lphul->rghs[j].cItem > 0) {
                *ihs = j;
                return ish;
            }
        }
    }
    return -1;
}

// FColonizeWith sends one ship of player 0's colony design, with its module
// count set to cModule, to colonize an empty planet with 100 kT of
// colonists, generates the turn, and reports whether player 0 owns the
// planet afterwards.
static int16_t FColonizeWith(const char *pszTest, int16_t cModule) {
    char    szDir[MAX_PATH];
    int16_t ish;
    int16_t ishNew;
    int16_t ihs;
    int16_t idPlanet;
    int16_t i;
    FLEET  *lpfl;
    ORDER  *lpord;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());

    ish = IshColonyDesign(0, &ihs);
    TEST_ASSERT(ish >= 0);
    for (ishNew = 0; ishNew < 16 && !rglpshdef[0][ishNew].fFree; ishNew++) {
    }
    TEST_ASSERT(ishNew < 16);
    rglpshdef[0][ishNew] = rglpshdef[0][ish];
    rglpshdef[0][ishNew].ishdef = ishNew;
    rglpshdef[0][ishNew].cExist = 0;
    rglpshdef[0][ishNew].cBuilt = 0;
    rglpshdef[0][ishNew].hul.rghs[ihs].cItem = cModule;

    for (idPlanet = -1, i = 0; i < cPlanet; i++) {
        if (lpPlanets[i].iPlayer == iplrNone) {
            idPlanet = lpPlanets[i].id;
            break;
        }
    }
    TEST_ASSERT(idPlanet >= 0);
    lpfl = LpflStarsTestAddFleet(0, idPlanet, ishNew, 1);
    lpfl->rgwtMin[3] = 100;
    lpord = &lpfl->lpplord->rgord()[0];
    lpord->grTask = grTaskColonize;
    lpord->fValidTask = TRUE;
    TEST_ASSERT(FStarsTestSaveHost());

    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestLoadHost());
    return lpPlanets[idPlanet].iPlayer == 0;
}

static void test_colonize(void) { TEST_CHECK(FColonizeWith("colonize", 1)); }

// Colonization module check: the colonize task accepted a slot recorded as
// a colonization module even when it held none (cItem 0).
static void test_colonize_needs_module(void) { TEST_CHECK(!FColonizeWith("colonize_needs_module", 0)); }

// WtStealFromAiHomeworld puts a player-0 freighter carrying a Robber Baron
// scanner at player 1's
// homeworld with an order to load 10 kT of item j there, generates the turn,
// and returns what the ship carries of item j afterwards.
static int32_t WtStealFromAiHomeworld(const char *pszTest, int16_t j) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    int16_t     ish;
    int16_t     ishNew;
    int16_t     ihs;
    int16_t     ifl;
    int16_t     idfl;
    PLANET     *lppl;
    FLEET      *lpfl;
    ORDER      *lpord;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(1);
    TEST_ASSERT(lppl != NULL && lppl->rgwtMin[j] > 10);

    ish = IshColonyDesign(0, &ihs);
    TEST_ASSERT(ish >= 0);
    for (ishNew = 0; ishNew < 16 && !rglpshdef[0][ishNew].fFree; ishNew++) {
    }
    TEST_ASSERT(ishNew < 16);
    // A Medium Freighter with the colony ship's engine and a Robber Baron
    // scanner in its scanner/special slot.
    rglpshdef[0][ishNew] = rglpshdef[0][ish];
    rglpshdef[0][ishNew].ishdef = ishNew;
    rglpshdef[0][ishNew].cExist = 0;
    rglpshdef[0][ishNew].cBuilt = 0;
    rglpshdef[0][ishNew].hul = LphuldefFromId(ihuldefMediumFreighter)->hul;
    rglpshdef[0][ishNew].hul.rghs[0] = rglpshdef[0][ish].hul.rghs[0];
    rglpshdef[0][ishNew].hul.rghs[1].grhst = hstScanner;
    rglpshdef[0][ishNew].hul.rghs[1].iItem = iscannerRobberBaronScanner;
    rglpshdef[0][ishNew].hul.rghs[1].cItem = 1;
    rglpshdef[0][ishNew].hul.rghs[2].cItem = 0;

    lpfl = LpflStarsTestAddFleet(0, lppl->id, ishNew, 1);
    idfl = lpfl->id;
    lpord = &lpfl->lpplord->rgord()[0];
    lpord->grTask = grTaskXfer;
    lpord->fValidTask = TRUE;
    lpord->txp.rgia[j].SetIAction(iActionLoadExact);
    lpord->txp.rgia[j].SetCQuan(10);
    TEST_ASSERT(FStarsTestSaveHost());

    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestLoadHost());
    for (ifl = 0; ifl < cFleet && rglpfl[ifl]; ifl++) {
        if (rglpfl[ifl]->iPlayer == 0 && rglpfl[ifl]->id == idfl)
            return rglpfl[ifl]->rgwtMin[j];
    }
    TEST_ASSERT_(FALSE, "stealing fleet is gone");
    return -1;
}

// A Robber Baron scanner steals minerals from an enemy planet.
static void test_steal_minerals(void) {
    int32_t wt;

    wt = WtStealFromAiHomeworld("steal_minerals", 0);
    TEST_CHECK_(wt == 10, "ironium %ld", (long)wt);
}

// SS pop steal: a waypoint order loaded colonists from a planet it could
// only rob. The binary has "attempted to shanghai colonists ... The attempt
// was unsuccessful" (and the same for fuel) but never sent it.
static void test_steal_colonists_refused(void) {
    int32_t wt;

    wt = WtStealFromAiHomeworld("steal_colonists_refused", 3);
    TEST_CHECK_(wt == 0, "colonists %ld kT", (long)wt);
}

TEST_LIST = {{"colonize", test_colonize}, {"colonize needs a colonization module", test_colonize_needs_module},
             {"steal minerals", test_steal_minerals},
             {"steal colonists refused", test_steal_colonists_refused},
             {NULL, NULL}};
