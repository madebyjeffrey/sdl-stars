#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"
#include "text_io.h"
#include "research_io.h"
#include <string>

static void test_ReportLabels_owned_results(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    struct {
        PLORD orders;
        ORDER rgord[2];
    } storage{};
    FLEET fl{};
    int16_t icr;

    TEST_ASSERT(FStarsTestInit(app));
    fl.lpplord = &storage.orders;
    fl.cord = 2;
    storage.orders.iordMac = storage.orders.iordMax = 2;
    storage.rgord[0].fValidTask = 1;
    storage.rgord[0].grTask = grTaskLayMines;
    storage.rgord[0].tlm().cTime = 1;
    const auto szDest = PszGetDestName(app, &fl, nullptr);
    const auto szTask = PszGetTaskName(app, &fl, &icr);
    TEST_CHECK(icr == -1);
    storage.rgord[0].grTask = grTaskAutoRoute;
    const auto szNext = PszGetTaskName(app, &fl, &icr);
    strcpy(app.state.szWork, "unrelated drawing text");
    TEST_CHECK(strcmp(szDest.data(), "(Delayed)") == 0);
    TEST_CHECK(strcmp(szTask.data(), "Lay Mine Field  2y") == 0);
    TEST_CHECK(strcmp(szNext.data(), "Route") == 0);
    TEST_CHECK(strcmp(TextIO().pfnCompressed(app, idsLayMineField).data(), "Lay Mine Field") == 0);
}

static std::int16_t etaLabelFirst, etaLabelLast;
static void EstimateLabel(APPSESSION &, PLANET *, PLPROD *, ProdItemType, std::int16_t *piFirst, std::int16_t *piLast) {
    *piFirst = etaLabelFirst;
    *piLast = etaLabelLast;
}
static std::string SzLabelBytes(const char *psz) { return psz; }
static void test_ProductionLabel_contract(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    struct { PLPROD q; PROD rgprod[1]; } storage{};
    PLANET pl{};
    SHDEF rgshdef[2]{};
    SHDEF *lpshdefSav, *lpshdefSBSav;
    std::int16_t etaFirst, etaLast;
    struct CASE { std::int16_t first, last; ProdItemType item; const char *psz; };
    const CASE rgCases[] = {
        {100, 100, iobjMine, "Unknown"}, {100, 100, mdIdleFactory, "Never"},
        {3, 100, iobjMine, "3 - ??? years"}, {0, 0, iobjMine, "Skipped"},
        {-1, -1, iobjMine, "As Needed"}, {1, 1, iobjMine, "1 year"},
        {2, 2, iobjMine, "2 years"}, {2, 4, iobjMine, "2 - 4 years"}
    };
    TEST_ASSERT(FStarsTestInit(app));
    RESEARCHIO io;
    io.pfnEstimate = EstimateLabel;
    RESEARCHIOSCOPE scopeResearch(io);
    pl.lpplprod = &storage.q;
    storage.q.iprodMac = storage.q.iprodMax = 1;
    storage.rgprod[0].grobj = grobjPlanet;
    for (const auto &cs : rgCases) {
        etaLabelFirst = cs.first; etaLabelLast = cs.last;
        storage.rgprod[0].iItem = cs.item;
        TEST_CHECK(SzLabelBytes(PszProductionETA(app, &pl, nullptr, iobjMine, &etaFirst, &etaLast).data()) == cs.psz);
        TEST_MSG("expected [%s], got [%s]", cs.psz, app.state.szWork);
        TEST_CHECK(etaFirst == cs.first && etaLast == cs.last);
        TEST_CHECK(std::string_view(app.state.szWork) == cs.psz);
    }
    const auto szETA = PszProductionETA(app, &pl, nullptr, iobjMine, nullptr, nullptr);
    storage.q.iprodMac = 0;
    TEST_CHECK(SzLabelBytes(PszProdQueueTop(app, &pl, nullptr).data()) == "      --- Queue is Empty ---");
    storage.q.iprodMac = 1;
    storage.rgprod[0].iItem = iobjMine; storage.rgprod[0].cItem = 12;
    etaLabelFirst = etaLabelLast = 1;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Mines");
    TEST_CHECK(SzLabelBytes(PszProdQueueTop(app, &pl, nullptr).data()) == "*\"  12Mines");
    TEST_MSG("queue [%s]", app.state.szWork);
    const auto szTop = PszProdQueueTop(app, &pl, nullptr);
    const auto szItem = PszNameProdItem(app, &storage.rgprod[0]);
    app.state.idPlayer = 0;
    lpshdefSav = app.state.rglpshdef[0]; lpshdefSBSav = app.state.rglpshdefSB[0];
    app.state.rglpshdef[0] = rgshdef; app.state.rglpshdefSB[0] = rgshdef;
    CchCopyUtf8Prefix(std::span{rgshdef[0].hul.szClass}, "Scout-\xce\xa9");
    CchCopyUtf8Prefix(std::span{app.state.rgshdef[0].hul.szClass}, "Scout-\xce\xa9");
    TEST_CHECK(SzLabelBytes(PszFleetNameFromWord(app, 0x2000).data()) == "Scout-\xce\xa9+ #1");
    const auto szFleet = PszFleetNameFromWord(app, 0x2000);
    storage.rgprod[0].grobj = grobjFleet; storage.rgprod[0].iItem = 0;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Scout-\xce\xa9");
    storage.rgprod[0].iItem = 16;
    app.state.sel.pl.fStarbase = 0;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Scout-\xce\xa9");
    app.state.sel.pl.fStarbase = 1; app.state.sel.pl.isb = 1;
    rgshdef[0].hul.ihuldef = ihuldefSpaceDock; rgshdef[1].hul.ihuldef = ihuldefSpaceStation;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Scout-\xce\xa9 (downgrade)");
    rgshdef[1].hul.ihuldef = ihuldefOrbitalFort;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Scout-\xce\xa9 (upgrade)");
    rgshdef[1].hul.ihuldef = ihuldefSpaceDock;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()) == "Scout-\xce\xa9");
    rgshdef[0].fFree = 1;
    TEST_CHECK(SzLabelBytes(PszNameProdItem(app, &storage.rgprod[0]).data()).empty());
    TEST_CHECK(SzLabelBytes(PszFleetNameFromWord(app, 0).data()) == "Fleet #1");
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "changed");
    TEST_CHECK(std::string_view(szETA.data()) == "2 - 4 years");
    TEST_CHECK(std::string_view(szTop.data()) == "*\"  12Mines");
    TEST_CHECK(std::string_view(szItem.data()) == "Mines");
    TEST_CHECK(std::string_view(szFleet.data()) == "Scout-\xce\xa9+ #1");
    app.state.rglpshdef[0] = lpshdefSav; app.state.rglpshdefSB[0] = lpshdefSBSav;
}

static void test_ETALabel_contract(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    struct { PLORD ord; ORDER rgord[2]; } storage{};
    FLEET fl{};
    std::int16_t cYears;
    TEST_ASSERT(FStarsTestInit(app));
    fl.lpplord = &storage.ord;
    storage.ord.iordMac = storage.ord.iordMax = 2;
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "untouched");
    fl.cord = 1;
    TEST_CHECK(SzLabelBytes(PszGetETA(app, nullptr, &fl, &cYears).data()) == "-- " && cYears == 0);
    TEST_CHECK(std::string_view(app.state.szWork) == "untouched");
    fl.cord = 2;
    storage.rgord[1].pt.x = 100; storage.rgord[1].iWarp = 5;
    TEST_CHECK(SzLabelBytes(PszGetETA(app, nullptr, &fl, &cYears).data()) == "4y" && cYears == 4);
    const auto szETA = PszGetETA(app, nullptr, &fl, nullptr);
    storage.rgord[1].iWarp = 0;
    TEST_CHECK(SzLabelBytes(PszGetETA(app, nullptr, &fl, &cYears).data()) == "Never" && cYears == 32000);
    storage.rgord[0].fValidTask = 1;
    for (const auto task : {grTaskMerge, grTaskScrap, grTaskLayMines}) {
        storage.rgord[0].grTask = task;
        TEST_CHECK(SzLabelBytes(PszGetETA(app, nullptr, &fl, &cYears).data()) == "-- " && cYears == 0);
        TEST_CHECK(std::string_view(app.state.szWork) == "Never");
    }
    TEST_CHECK(std::string_view(szETA.data()) == "4y");
}

// The planet dump passed the 32-bit mine and factory counts as two 16-bit
// words each, as the Win16 binary pushed them, so the Factories column
// printed 0 and the factory count shifted into Def %.
static void test_DumpPlanets_mines_and_factories(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char    szDir[cbTestPath];
    char    szFile[cbTestPath];
    char    szLine[1024];
    char   *rgpszField[16];
    char   *psz;
    char    szName[64];
    PLANET *lppl;
    FILE   *fp;
    int     cField;
    int     fFound;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("DumpPlanets_mines_and_factories", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    lppl = LpplStarsTestHomeworld(app, 0);
    TEST_ASSERT(lppl != NULL);
    TEST_ASSERT(lppl->cMines != lppl->cFactories || lppl->cMines != 0);
    CchCopyUtf8Prefix(std::span{szName}, PszGetCompressedPlanet(app, app.state.rgidPlan[lppl->id]).data());

    DumpPlanets(app);
    snprintf(szFile, sizeof(szFile), "%s.pla", app.state.szBase);
    fp = fopen(szFile, "rb");
    TEST_ASSERT_(fp != NULL, "open %s", szFile);
    fFound = 0;
    while (fgets(szLine, sizeof(szLine), fp)) {
        cField = 0;
        for (psz = strtok(szLine, "\t\r\n"); psz && cField < 16; psz = strtok(NULL, "\t\r\n"))
            rgpszField[cField++] = psz;
        if (cField < 10 || strcmp(rgpszField[0], szName) != 0)
            continue;
        fFound = 1;
        // Planet Name, Owner, Starbase Type, Report Age, Population, Value,
        // Production Queue, Mines, Factories, Def %.
        TEST_CHECK_(atol(rgpszField[7]) == lppl->cMines, "mines %s, expected %d", rgpszField[7], lppl->cMines);
        TEST_CHECK_(atol(rgpszField[8]) == lppl->cFactories, "factories %s, expected %d", rgpszField[8], lppl->cFactories);
        TEST_CHECK_(strchr(rgpszField[9], '%') != NULL, "def %s", rgpszField[9]);
    }
    fclose(fp);
    TEST_CHECK_(fFound, "no row for %s", szName);
}

// FSameAsGolden compares a dump with its expected copy in
// tests/unit/golden, or writes the copy when STARS_UPDATE_GOLDEN is set.
static int16_t FSameAsGolden(APPSESSION &app, const char *szExt) {
    char   szFile[cbTestPath];
    char   szGolden[cbTestPath];
    char   rgb1[4096];
    char   rgb2[4096];
    size_t cb1;
    size_t cb2;
    FILE  *pf1;
    FILE  *pf2;
    int    fSame;

    snprintf(szFile, sizeof(szFile), "%s.%s", app.state.szBase, szExt);
    snprintf(szGolden, sizeof(szGolden), "%s/dumps.%s", STARS_TEST_GOLDEN_DIR, szExt);
    pf1 = fopen(szFile, "rb");
    if (pf1 == NULL)
        return 0;
    pf2 = fopen(szGolden, getenv("STARS_UPDATE_GOLDEN") ? "wb" : "rb");
    if (pf2 == NULL) {
        fclose(pf1);
        return 0;
    }
    fSame = 1;
    if (getenv("STARS_UPDATE_GOLDEN")) {
        while ((cb1 = fread(rgb1, 1, sizeof(rgb1), pf1)) > 0)
            fwrite(rgb1, 1, cb1, pf2);
    } else {
        do {
            cb1 = fread(rgb1, 1, sizeof(rgb1), pf1);
            cb2 = fread(rgb2, 1, sizeof(rgb2), pf2);
            if (cb1 != cb2 || memcmp(rgb1, rgb2, cb1) != 0)
                fSame = 0;
        } while (fSame && cb1 > 0);
    }
    fclose(pf1);
    fclose(pf2);
    return fSame;
}

// The universe, planet and fleet dumps (-dm, -dp, -df) are what TotalHost
// reads for its movies, and stars-host writes them off Windows. They must
// match the Windows build's, kept in tests/unit/golden, in both the
// original layout and the per-player one ([Misc] NewReports=1).
static void test_Dumps_match_golden_files(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    int16_t     fPerPlayer;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("Dumps_match_golden_files", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestGenerate(app));
    TEST_ASSERT(FStarsTestGenerate(app));
    TEST_ASSERT(FStarsTestLoadPlayer(app, 0));
    for (fPerPlayer = 0; fPerPlayer <= 1; fPerPlayer++) {
        app.state.gd.fPerPlayerDumps = fPerPlayer;
        DumpUniverse(app);
        DumpPlanets(app);
        DumpFleets(app);
    }
    TEST_CHECK_(FSameAsGolden(app, "map"), "game.map differs from tests/unit/golden/dumps.map");
    TEST_CHECK_(FSameAsGolden(app, "pla"), "game.pla differs from tests/unit/golden/dumps.pla");
    TEST_CHECK_(FSameAsGolden(app, "fle"), "game.fle differs from tests/unit/golden/dumps.fle");
    TEST_CHECK_(FSameAsGolden(app, "p1"), "game.p1 differs from tests/unit/golden/dumps.p1");
    TEST_CHECK_(FSameAsGolden(app, "f1"), "game.f1 differs from tests/unit/golden/dumps.f1");
}

TEST_LIST = {
    {"production labels retain ETA, queue, upgrade and encoded-fleet text", test_ProductionLabel_contract},
    {"fleet ETA labels retain duration and no-result effects", test_ETALabel_contract},
    {"fleet report labels retain owned text across lookups", test_ReportLabels_owned_results},
    {"DumpPlanets mines and factories", test_DumpPlanets_mines_and_factories}, {"Dumps match golden files", test_Dumps_match_golden_files}, {NULL, NULL}};
