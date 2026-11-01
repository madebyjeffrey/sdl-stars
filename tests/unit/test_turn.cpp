#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"
#include "record_io.h"

static void OpenClosedBatchSource(APPSESSION &app, const char *psz, MdOpenFlags md) {
    StreamOpen(app, psz, md);
    StreamClose(app);
}
static void test_BatchInput_contract(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath], szFile[cbTestPath + 64];
    struct CASE { std::string sz; std::size_t cbFirst; std::string szExpected = "alpha.hst"; };
    const CASE rgCases[] = {{"alpha.hst\r\nbeta.hst\r\n", 11}, {"alpha.hst\nbeta.hst\n", 10},
                            {std::string("alpha.hst\r\n") + std::string(32768 - 11, 'x'), 11},
                            {std::string("alpha.hst\r\n") + std::string(65536 - 11, 'x'), 11},
                            {std::string("alpha.hst\r\n") + std::string(65536 + 64 - 11, 'x'), 11},
                            {"alpha.hst", 9}, {"alpha-\xce\xa9.hst\n", 13, "alpha-\xce\xa9.hst"},
                            {std::string(sizeof(app.state.szBase) - 1, 'a') + "\n", sizeof(app.state.szBase), std::string(sizeof(app.state.szBase) - 1, 'a')}};

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("batch-input", szDir, sizeof(szDir)));
    CchSnprintf(std::span{szFile}, "%s%sinput.txt", szDir, szDirSep);
    for (const auto &cs : rgCases) {
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
        TEST_ASSERT(lpfile);
        TEST_ASSERT(CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{cs.sz})) == cs.sz.size());
        lpfile.reset();
        TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szFile) >= 0);
        TEST_CHECK(FSetUpBatchProcessing(app));
        TEST_CHECK(std::string_view(app.state.szBase) == cs.szExpected);
        TEST_CHECK(static_cast<std::size_t>(app.state.lpchBatchMac - app.state.lpchBatch) == cs.sz.size() - cs.cbFirst);
        TEST_CHECK(app.state.hf == -1);
        TEST_CHECK(app.state.szBatch == cs.sz);
    }
    const std::string rgszBad[] = {"", "\n", "\r\n", std::string(sizeof(app.state.szBase), 'a') + "\n", "\xff\n", std::string("a\0b\n", 4)};
    for (const auto &sz : rgszBad) {
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
        TEST_ASSERT(lpfile);
        TEST_ASSERT(CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{sz})) == sz.size());
        lpfile.reset();
        TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szFile) >= 0);
        TEST_CHECK(!FSetUpBatchProcessing(app));
        TEST_CHECK(app.state.szBase[0] == 0 && app.state.hf == -1 && app.state.szBatch.empty());
        TEST_CHECK(app.state.lpchBatch == 0 && app.state.lpchBatchMac == 0);
    }
    TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szFile) >= 0);
    RECORDIO io = RecordIO();
    io.pfnStreamOpen = OpenClosedBatchSource;
    RECORDIOSCOPE recordScope(io);
    TEST_CHECK(!FSetUpBatchProcessing(app));
    TEST_CHECK(app.state.szBase[0] == 0 && app.state.hf == -1 && app.state.szBatch.empty());
    TEST_CHECK(app.state.lpchBatch == 0 && app.state.lpchBatchMac == 0 && !FStarsErrorPending());
}

static void test_BatchRun_games(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szFirst[cbTestPath], szSecond[cbTestPath], szFile[cbTestPath + 64];
    std::int16_t fSeed = 0;
    std::uint32_t lSeed = 0;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("batch-first", szFirst, sizeof(szFirst)));
    TEST_ASSERT(FStarsTestNewGame(app, szFirst, 12345, nullptr, 0));
    const std::string szFirstBase(app.state.szBase);
    TEST_ASSERT(FStarsTestDir("batch-second", szSecond, sizeof(szSecond)));
    TEST_ASSERT(FStarsTestNewGame(app, szSecond, 67890, nullptr, 0));
    const std::string szSecondBase(app.state.szBase);
    CchSnprintf(std::span{szFile}, "%s%sinput.txt", szFirst, szDirSep);
    const std::string szInput = szFirstBase + "\n" + szSecondBase;
    {
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
        TEST_ASSERT(lpfile && CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{szInput})) == szInput.size());
    }
    std::string szCmd = std::string("-b ") + szFile;
    TEST_ASSERT(ParseCmdLine(app, szCmd.data(), &fSeed, &lSeed));
    TEST_CHECK(app.state.ini.fBatch && app.state.ini.fGen && app.state.ini.fStartupFile);
    TEST_CHECK(FRunCmdLine(app));
    TEST_CHECK(app.state.lpchBatch == app.state.lpchBatchMac && app.state.szBatch == szInput);
    for (const auto &szBase : {szFirstBase, szSecondBase}) {
        TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szBase) >= 0);
        TEST_ASSERT(FStarsTestLoadHost(app));
        TEST_CHECK(app.state.game.turn == 1);
        DestroyCurGame(app);
    }
    // A rejected later path must stop the batch after the preceding game.
    const std::string szBadInput = szFirstBase + "\r\n" + std::string(sizeof(app.state.szBase), 'x');
    {
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
        TEST_ASSERT(lpfile && CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{szBadInput})) == szBadInput.size());
    }
    TEST_ASSERT(ParseCmdLine(app, szCmd.data(), &fSeed, &lSeed));
    TEST_CHECK(FRunCmdLine(app));
    TEST_CHECK(!app.state.ini.fBatch && !app.state.ini.fGen && app.state.szBase[0] == 0 && app.state.hf == -1);
    TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szFirstBase) >= 0);
    TEST_ASSERT(FStarsTestLoadHost(app));
    TEST_CHECK(app.state.game.turn == 2);
}

// test_generate_one_turn checks the turn-test plumbing: create a game, load
// its host file, generate a turn, and load the result.
static void test_generate_one_turn(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char        szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("generate_one_turn", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));

    TEST_ASSERT(FStarsTestLoadHost(app));
    TEST_CHECK_(app.state.game.turn == 0, "turn %d", app.state.game.turn);
    TEST_CHECK_(app.state.game.cPlayer == 2, "players %d", app.state.game.cPlayer);
    TEST_CHECK(app.state.cPlanet > 0);

    TEST_ASSERT(FStarsTestGenerate(app));
    TEST_ASSERT(FStarsTestLoadHost(app));
    TEST_CHECK_(app.state.game.turn == 1, "turn %d", app.state.game.turn);
}

// FReadFile reads up to cbMax bytes of szFile into rgb and returns the count
// in *pcb.
static int16_t FReadFile(const char *szFile, uint8_t *rgb, size_t cbMax, size_t *pcb) {
    FILE *fp;

    fp = fopen(szFile, "rb");
    if (fp == NULL)
        return 0;
    *pcb = fread(rgb, 1, cbMax, fp);
    fclose(fp);
    return 1;
}

// When a player submits no turn, generation keeps a backup of the turn file
// the player last received. The copy's arguments were reversed (backup to
// live file, after the backup had been removed), so no backup was kept.
static void test_generate_backs_up_unsubmitted_turn(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char    szDir[cbTestPath];
    char    szFile[cbTestPath];
    uint8_t rgbBefore[16384];
    uint8_t rgbBackup[16384];
    size_t  cbBefore;
    size_t  cbBackup;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("generate_backs_up_unsubmitted_turn", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    snprintf(szFile, sizeof(szFile), "%s%sgame.m1", szDir, szDirSep);
    TEST_ASSERT(FReadFile(szFile, rgbBefore, sizeof(rgbBefore), &cbBefore));

    TEST_ASSERT(FStarsTestGenerate(app));
    snprintf(szFile, sizeof(szFile), "%s%sbackup%sgame.m1", szDir, szDirSep, szDirSep);
    TEST_ASSERT_(FReadFile(szFile, rgbBackup, sizeof(rgbBackup), &cbBackup), "no %s", szFile);
    TEST_CHECK(cbBackup == cbBefore && memcmp(rgbBackup, rgbBefore, cbBefore) == 0);
}

// SetFleetDest gives a fleet at a planet one waypoint at warp 9: toward
// another fleet when lpflTarget is set, otherwise to a point dx ly east.
static void SetFleetDest(APPSESSION &app, FLEET *lpfl, FLEET *lpflTarget, int16_t dx) {
    ORDER *lpord;

    lpfl->rgwtMin[4] = LGetFleetStat(app, lpfl, 1);
    lpfl->cord = 2;
    lpfl->lpplord->iordMac = 2;
    lpord = &lpfl->lpplord->rgord()[1];
    memset(lpord, 0, sizeof(ORDER));
    lpord->iWarp = 9;
    lpord->fValidTask = 1;
    lpord->grTask = grTaskNone;
    if (lpflTarget) {
        lpord->grobj = grobjFleet;
        lpord->id = lpflTarget->id;
        lpord->pt = lpflTarget->pt;
    } else {
        lpord->grobj = grobjOther;
        lpord->id = idPlanetDeepSpace;
        lpord->pt.x = lpfl->pt.x + dx;
        lpord->pt.y = lpfl->pt.y;
    }
}

// "Stuck" pursuit: A chases B, which chases C. When A caught up with B,
// MoveFleets marked B done although it hadn't finished its own pursuit, so
// B stopped where A caught it.
static void test_MoveFleets_caught_pursuer_keeps_moving(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char    szDir[cbTestPath];
    PLANET *lppl;
    FLEET  *lpflA;
    FLEET  *lpflB;
    FLEET  *lpflC;
    POINT16 ptStart;
    int16_t ish;
    int16_t rgid[3];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("MoveFleets_stuck", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost(app));
    lppl = LpplStarsTestHomeworld(app, 0);
    TEST_ASSERT(lppl != NULL);
    for (ish = 0; ish < 16 && app.state.rglpshdef[0][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < 16);
    // Fleets move in fleet order, so A goes before B and B before C.
    rgid[0] = LpflStarsTestAddFleet(app, 0, lppl->id, ish, 1)->id;
    rgid[1] = LpflStarsTestAddFleet(app, 0, lppl->id, ish, 1)->id;
    rgid[2] = LpflStarsTestAddFleet(app, 0, lppl->id, ish, 1)->id;
    lpflA = LpflFromId(app, rgid[0]);
    lpflB = LpflFromId(app, rgid[1]);
    lpflC = LpflFromId(app, rgid[2]);
    ptStart = lpflC->pt;
    SetFleetDest(app, lpflC, NULL, 200);
    SetFleetDest(app, lpflB, lpflC, 0);
    SetFleetDest(app, lpflA, lpflB, 0);

    MoveFleets(app);
    lpflA = LpflFromId(app, rgid[0]);
    lpflB = LpflFromId(app, rgid[1]);
    lpflC = LpflFromId(app, rgid[2]);
    TEST_ASSERT(lpflA != NULL && lpflB != NULL && lpflC != NULL);
    TEST_ASSERT_(lpflC->pt.x > ptStart.x + 20, "C moved %d ly", lpflC->pt.x - ptStart.x);
    TEST_CHECK_(lpflB->pt.x == lpflC->pt.x && lpflB->pt.y == lpflC->pt.y, "B stopped %d ly from the start", lpflB->pt.x - ptStart.x);
    TEST_CHECK_(lpflA->pt.x == lpflB->pt.x && lpflA->pt.y == lpflB->pt.y, "A stopped %d ly from the start", lpflA->pt.x - ptStart.x);
}

// East/West speed bump immunity (reported for 2.6j): a fleet at warp 9
// crossing another player's speed bump field should be stopped, whichever
// way it travels. With a 14% chance per light-year, 80 light-years inside
// the field all but guarantee a hit.
static void test_FTravelThroughMineFields_speed_bump_any_direction(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char                 szDir[cbTestPath];
    const char          *rgszAi[] = {"#1 4"};
    static const int16_t rgdxy[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    PLANET              *lppl;
    FLEET               *lpfl;
    THING               *lpth;
    POINT16              pt;
    int16_t              ish;
    int16_t              idir;
    int16_t              dTravel;

    for (idir = 0; idir < 4; idir++) {
        TEST_CASE_("direction %d,%d", rgdxy[idir][0], rgdxy[idir][1]);
        TEST_ASSERT(FStarsTestInit(app));
        TEST_ASSERT(FStarsTestDir("FTravelThroughMineFields_speed_bump", szDir, sizeof(szDir)));
        TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
        TEST_ASSERT(FStarsTestLoadHost(app));
        lppl = LpplStarsTestHomeworld(app, 0);
        TEST_ASSERT(lppl != NULL);
        for (ish = 0; ish < 16 && app.state.rglpshdef[0][ish].fFree; ish++) {
        }
        TEST_ASSERT(ish < 16);
        lpfl = LpflStarsTestAddFleet(app, 0, lppl->id, ish, 1);
        pt = lpfl->pt;
        SetFleetDest(app, lpfl, NULL, 0);
        lpfl->lpplord->rgord()[1].pt.x = pt.x + 81 * rgdxy[idir][0];
        lpfl->lpplord->rgord()[1].pt.y = pt.y + 81 * rgdxy[idir][1];
        lpth = LpthNew(app, 1, ithMinefield);
        lpth->pt().SetX(pt.x + 40 * rgdxy[idir][0]);
        lpth->pt().SetY(pt.y + 40 * rgdxy[idir][1]);
        lpth->thm().SetCMines(10000);
        lpth->thm().SetIType(mineSpeedBump);
        lpth->thm().SetGrbitPlr(1 << 1);

        dTravel = 81;
        TEST_CHECK_(!FTravelThroughMineFields(app, lpfl, &dTravel, NULL), "the fleet crossed the field");
        TEST_CHECK_(dTravel < 81, "the fleet travelled %d ly", dTravel);
    }
}

// With a seed that does not select a course change, a trader sends no messages.
// It must still move when no retargeting message was selected this pass.
static void test_MoveThings_trader_without_retargeting_moves(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath];
    THING *lpth;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("MoveThings_trader", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost(app));
    while (app.state.cThing != 0) FreeLpth(app, app.state.lpThings);
    lpth = LpthNew(app, 0, ithMysteryTrader);
    TEST_ASSERT(lpth != NULL);
    lpth->pt().Set({1200, 1200});
    lpth->tht().ptDest().Set({2000, 1200});
    lpth->tht().SetIWarp(9);
    PushRandom(app, 123456, 789012);
    TEST_ASSERT(Random(app, 25) != 0);
    PopRandom(app);
    PushRandom(app, 123456, 789012);
    MoveThings(app, 0);
    PopRandom(app);
    TEST_CHECK(lpth->pt().x() == 1281 && lpth->pt().y() == 1200);
    TEST_CHECK(lpth->tht().ptDest().x() == 2000 && lpth->tht().ptDest().y() == 1200);
}

TEST_LIST = {{"batch command iterates owned LF paths and stops on invalid names", test_BatchRun_games}, {"batch input preserves names and full file sizes", test_BatchInput_contract}, {"MoveThings trader without retargeting moves", test_MoveThings_trader_without_retargeting_moves},
             {"generate one turn", test_generate_one_turn},
             {"generate backs up an unsubmitted turn", test_generate_backs_up_unsubmitted_turn},
             {"MoveFleets caught pursuer keeps moving", test_MoveFleets_caught_pursuer_keeps_moving},
             {"FTravelThroughMineFields speed bump stops a fleet any direction", test_FTravelThroughMineFields_speed_bump_any_direction},
             {NULL, NULL}};
