#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"

constexpr auto iplrCyber = 1;
constexpr auto ishSBDefend = 14;

// CshDesign counts iPlr's ships of design ishdef in the loaded game.
static int32_t CshDesign(APPSESSION &app, int16_t iPlr, int16_t ishdef) {
    int32_t csh;
    int16_t ifl;

    csh = 0;
    for (ifl = 0; ifl < app.state.cFleet && app.state.rglpfl[ifl]; ifl++) {
        if (app.state.rglpfl[ifl]->iPlayer == iPlr && !app.state.rglpfl[ifl]->fDead)
            csh += app.state.rglpfl[ifl]->rgcsh[ishdef];
    }
    return csh;
}

// The Cybertron AI keeps its newest starbase-defender design. The original
// tested iLatestDestroyer after the defender CheckAiShdefStatus call, so an
// old but newest defender design was marked for recycling and its fleets at
// a starbase were scrapped.
static void test_cyber_keeps_latest_sb_defender(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#5 4"};
    PLANET     *lppl;
    SHDEF      *lpshdef;
    int16_t     ish;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("cyber_keeps_latest_sb_defender", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));

    lppl = LpplStarsTestHomeworld(app, iplrCyber);
    TEST_ASSERT(lppl != NULL);
    TEST_ASSERT(lppl->fStarbase);

    // Give the Cybertron one starbase-defender design, older than its
    // recycle period, and one ship of it at its homeworld starbase.
    for (ish = 0; ish < ishSBDefend && app.state.rglpshdef[iplrCyber][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < ishSBDefend);
    lpshdef = &app.state.rglpshdef[iplrCyber][ishSBDefend];
    *lpshdef = app.state.rglpshdef[iplrCyber][ish];
    lpshdef->ishdef = ishSBDefend;
    lpshdef->turn = (uint16_t)(app.state.game.turn - 60);
    lpshdef->cExist = 0;
    lpshdef->cBuilt = 0;
    app.state.rglpshdef[iplrCyber][ishSBDefend + 1].fFree = 1;
    LpflStarsTestAddFleet(app, iplrCyber, lppl->id, ishSBDefend, 1);
    TEST_ASSERT(FStarsTestSaveHost(app));

    // The AI gives its orders while one turn generates; the next carries them out.
    TEST_ASSERT(FStarsTestGenerate(app));
    TEST_ASSERT(FStarsTestGenerate(app));
    TEST_ASSERT(FStarsTestLoadHost(app));
    TEST_CHECK_(CshDesign(app, iplrCyber, ishSBDefend) == 1, "defenders %ld", (long)CshDesign(app, iplrCyber, ishSBDefend));
}

static void test_cyber_packet_respects_diplomacy(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath];
    const char *rgszAi[] = {"#5 4"};

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("cyber_packet_diplomacy", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 2089, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost(app));
    auto *lpplSrc = LpplStarsTestHomeworld(app, iplrCyber);
    auto *lpplTest = LpplStarsTestHomeworld(app, 0);
    TEST_ASSERT(lpplSrc && lpplTest);
    app.state.idPlayer = iplrCyber;
    app.state.rgptPlan[lpplTest->id] = app.state.rgptPlan[lpplSrc->id];
    ++app.state.rgptPlan[lpplTest->id].x;
    lpplTest->fStarbase = false;
    lpplTest->uPopGuess = 10;
    lpplTest->uDefGuess = 0;
    const std::size_t ibMinerals = 4 * app.state.game.cPlanMax + 2;
    const std::size_t cbAi = ibMinerals + sizeof(std::int32_t);
    app.state.vlpbAiData = LpAlloc<std::uint8_t>(app, cbAi, htMisc);
    TEST_ASSERT(app.state.vlpbAiData);
    std::fill_n(app.state.vlpbAiData, cbAi, 0);
    MivFromLp(app, app.state.vlpbAiData + ibMinerals).SetValue(12000);

    const struct { std::int8_t mdSrc; std::int8_t mdDst; bool fEligible; const char *psz; } rgCases[] = {
        {2, 0, true, "enemy colony qualifies"},
        {0, 1, true, "incoming Friend declaration leaves a neutral colony eligible"},
        {1, 2, false, "sender's Friend declaration excludes the colony"},
        {1, 1, false, "mutual Friend colony is excluded"},
    };
    for (const auto &cs : rgCases) {
        app.state.rgplr[iplrCyber].rgmdRelation[0] = cs.mdSrc;
        app.state.rgplr[0].rgmdRelation[iplrCyber] = cs.mdDst;
        TEST_CHECK_(bool(FEnumPktAttack(app, lpplSrc, lpplTest)) == cs.fEligible, "%s", cs.psz);
    }
    FreeLp(app, app.state.vlpbAiData, htMisc);
    app.state.vlpbAiData = nullptr;
}

TEST_LIST = {{"Cybertron keeps its latest starbase defender", test_cyber_keeps_latest_sb_defender},
             {"Cybertron respects the sender's packet diplomacy", test_cyber_packet_respects_diplomacy},
             {NULL, NULL}};
