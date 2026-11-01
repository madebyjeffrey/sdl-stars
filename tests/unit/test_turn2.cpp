#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"

// AddDetonatingField adds iplr's detonating standard minefield of radius
// 100 ly centered on pt.
static void AddDetonatingField(APPSESSION &app, int16_t iplr, POINT16 pt) {
    THING *lpth;

    lpth = LpthNew(app, iplr, ithMinefield);
    lpth->pt().Set(pt);
    lpth->thm().SetCMines(10000);
    lpth->thm().SetIType(mineStandard);
    lpth->thm().SetFDetonate(1);
    lpth->thm().SetGrbitPlr(1 << iplr);
}

// AddScout adds one ship of player 0's first design at its homeworld and
// returns the fleet's id; *pish is the design.
static int16_t IdflAddScout(APPSESSION &app, int16_t *pish) {
    PLANET *lppl;
    FLEET  *lpfl;
    int16_t ish;

    lppl = LpplStarsTestHomeworld(app, 0);
    if (lppl == NULL)
        return -1;
    for (ish = 0; ish < 16 && app.state.rglpshdef[0][ish].fFree; ish++) {
    }
    if (ish == 16)
        return -1;
    lpfl = LpflStarsTestAddFleet(app, 0, lppl->id, ish, 1);
    lpfl->pt = app.state.rgptPlan[lppl->id];
    *pish = ish;
    return lpfl->id;
}

// A detonating minefield over a fleet crashed the native build:
// FTravelThroughMineFields read the travel distance through ThingDecay's
// NULL pointer, which Win16 read harmlessly from the start of its data
// segment.
static void test_ThingDecay_detonation_damages_fleet(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    FLEET      *lpfl;
    int16_t     ish;
    int16_t     idfl;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("ThingDecay_detonation", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    idfl = IdflAddScout(app, &ish);
    TEST_ASSERT(idfl >= 0);
    lpfl = LpflFromId(app, idfl);
    AddDetonatingField(app, 1, lpfl->pt);

    ThingDecay(app);
    lpfl = LpflFromId(app, idfl);
    TEST_ASSERT(lpfl != NULL);
    TEST_CHECK_(lpfl->fDead || lpfl->rgdv[ish].dp != 0, "the detonating field did no damage");
}

// Exploding minefield dodge: a fleet is checked against one detonating field
// a turn. ThingDecay used up that check on a field the fleet is immune to
// (its owner's own field, for mine layers), so another player's field
// detonating over it did no damage.
static void test_ThingDecay_immune_field_keeps_check(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    FLEET      *lpfl;
    int16_t     ish;
    int16_t     idfl;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("ThingDecay_immune_field", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    idfl = IdflAddScout(app, &ish);
    TEST_ASSERT(idfl >= 0);
    // A mine layer, immune to its owner's minefields. Player 0's field comes
    // first in lpThings.
    app.state.rglpshdef[0][ish].hul.ihuldef = ihuldefMiniMineLayer;
    lpfl = LpflFromId(app, idfl);
    AddDetonatingField(app, 0, lpfl->pt);
    AddDetonatingField(app, 1, lpfl->pt);

    ThingDecay(app);
    lpfl = LpflFromId(app, idfl);
    TEST_ASSERT(lpfl != NULL);
    TEST_CHECK_(lpfl->fDead || lpfl->rgdv[ish].dp != 0, "player 1's detonating field did no damage");
}

// LResourceScoreAfterLoading generates a turn in which two of player 0's
// cargo ships arrive at its homeworld, loading all the colonists they can
// there if fLoad, and returns the resources player 0's score shows.
static int32_t LResourceScoreAfterLoading(APPSESSION &app, const char *pszTest, int16_t fLoad, int32_t *pwtLoaded) {
    char    szDir[cbTestPath];
    PLANET *lppl;
    FLEET  *lpfl;
    ORDER  *lpord;
    int16_t ish;
    int16_t idfl;

    if (!FStarsTestInit(app) || !FStarsTestDir(pszTest, szDir, sizeof(szDir)) || !FStarsTestNewGame(app, szDir, 12345, NULL, 0) || !FStarsTestLoadHost(app))
        return -1;
    lppl = LpplStarsTestHomeworld(app, 0);
    for (ish = 0; ish < 16 && (app.state.rglpshdef[0][ish].fFree || WtMaxShdefStat(&app.state.rglpshdef[0][ish], 2) == 0); ish++) {
    }
    if (lppl == NULL || ish == 16)
        return -1;
    lpfl = LpflStarsTestAddFleet(app, 0, idPlanetDeepSpace, ish, 2);
    lpfl->pt.x = app.state.rgptPlan[lppl->id].x + 20;
    lpfl->pt.y = app.state.rgptPlan[lppl->id].y;
    lpfl->rgwtMin[4] = LGetFleetStat(app, lpfl, 1);
    lpord = lpfl->lpplord->rgord();
    lpord[0].pt = lpfl->pt;
    lpfl->cord = 2;
    lpfl->lpplord->iordMac = 2;
    memset(&lpord[1], 0, sizeof(ORDER));
    lpord[1].pt = app.state.rgptPlan[lppl->id];
    lpord[1].grobj = grobjPlanet;
    lpord[1].id = lppl->id;
    lpord[1].iWarp = 5;
    lpord[1].fValidTask = 1;
    lpord[1].grTask = fLoad ? grTaskXfer : grTaskNone;
    lpord[1].txp.rgia[3].SetIAction(iActionLoadAll);
    idfl = lpfl->id;
    if (!FStarsTestSaveHost(app) || !FStarsTestGenerate(app) || !FStarsTestLoadPlayer(app, 0))
        return -1;
    lpfl = LpflFromId(app, idfl);
    *pwtLoaded = lpfl ? lpfl->rgwtMin[3] : -1;
    return app.state.vlprgScoreX[0].score.cResources;
}

// False public scores: production uses the planets' populations, but the
// score was taken from them at the end of the turn, after waypoint 1
// orders. Loading colonists onto ships after production lowered the
// published resources, and unloading them at waypoint 0 next turn restored
// them before production.
static void test_UpdatePlayerScores_ignores_loading_after_production(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int32_t cResPlain;
    int32_t cResLoaded;
    int32_t wtLoaded;

    cResPlain = LResourceScoreAfterLoading(app, "UpdatePlayerScores_plain", 0, &wtLoaded);
    TEST_ASSERT(cResPlain > 0);
    TEST_ASSERT(wtLoaded == 0);
    cResLoaded = LResourceScoreAfterLoading(app, "UpdatePlayerScores_loaded", 1, &wtLoaded);
    TEST_ASSERT_(wtLoaded > 0, "the ships loaded no colonists");
    TEST_CHECK_(cResLoaded == cResPlain, "score shows %ld resources after loading %ld, %ld without", (long)cResLoaded, (long)wtLoaded, (long)cResPlain);
}

TEST_LIST = {{"ThingDecay detonation damages a fleet", test_ThingDecay_detonation_damages_fleet},
             {"ThingDecay immune field keeps the fleet's check", test_ThingDecay_immune_field_keeps_check},
             {"UpdatePlayerScores ignores loading after production", test_UpdatePlayerScores_ignores_loading_after_production},
             {NULL, NULL}};
