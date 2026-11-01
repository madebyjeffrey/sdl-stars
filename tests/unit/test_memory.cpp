#include "common.h"
#include "appstate.h"
#include <callbackcontext.h>
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "appstate.h"
#include "acutest.h"
#include "stars_test.h"
#ifdef STARS_UI_SDL
#include "uistate.h"

static void TestCallbackApplicationOwners(void) {
    APPUISESSION first, second;
    HWND hwndFirst, hwndSecond;

    {
        COMPATCALLBACKCONTEXTSCOPE context(static_cast<APPSESSION *>(&first));
        hwndFirst = ui::CreateDialog(nullptr, win32::MAKEINTRESOURCE(IDD_ABOUT), nullptr, About);
    }
    TEST_ASSERT(hwndFirst);
    {
        COMPATCALLBACKCONTEXTSCOPE context(static_cast<APPSESSION *>(&second));
        hwndSecond = ui::CreateDialog(nullptr, win32::MAKEINTRESOURCE(IDD_ABOUT), nullptr, About);
    }
    TEST_ASSERT(hwndSecond);
    TEST_CHECK(first.ui.iAbout1st == -11 && second.ui.iAbout1st == -11);
    first.ui.iAboutPartial = 7;
    second.ui.iAboutPartial = 19;
    {
        COMPATCALLBACKCONTEXTSCOPE context(static_cast<APPSESSION *>(&second));
        ui::SendMessage(hwndFirst, win32::WM_INITDIALOG, 0, 0);
        TEST_CHECK(first.ui.iAboutPartial == 0 && second.ui.iAboutPartial == 19);
        TEST_CHECK(PvCompatCallbackContext() == static_cast<APPSESSION *>(&second));
    }
    TEST_CHECK(!PvCompatCallbackContext());
    TEST_CHECK(ui::DestroyWindow(hwndSecond));
    TEST_CHECK(ui::DestroyWindow(hwndFirst));
    TEST_CHECK(!PvCompatCallbackContext());
}
#endif

static void TestApplicationStateOwner(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    APPSESSION &session = app;
    GAME *pgame = &app.state.game;
    INI *pini = &app.state.ini;
    auto *prglphb = &app.state.rglphb;
    int32_t *plSeed1 = &app.state.lFileSeed1;
    int32_t *plSeed2 = &app.state.lFileSeed2;
    auto *lpui = app.lpui;
    app.state.game.cPlayer = 7;
    app.state.lFileSeed1 = 123;
    app.state.lFileSeed2 = 456;
    LpAlloc(app, 31, htMisc);
    TEST_ASSERT(app.state.rglphb[htMisc]);
    session.Close(); session.Close();
    for (auto *lphb : app.state.rglphb) TEST_CHECK(!lphb);
    session.Reset();
    TEST_CHECK(app.state.game.cPlayer == 2 && app.state.game.mdSize == sizeMedium);
    TEST_CHECK(app.state.lFileSeed1 == 0 && app.state.lFileSeed2 == 0);
    TEST_CHECK(&app.state.game == pgame && &app.state.ini == pini);
    TEST_CHECK(&app.state.rglphb == prglphb);
    TEST_CHECK(&app.state.lFileSeed1 == plSeed1 && &app.state.lFileSeed2 == plSeed2);
    TEST_CHECK(app.lpui == lpui);
}

static void CheckAligned(void *lp) {
    TEST_ASSERT(lp != NULL);
    TEST_CHECK((uintptr_t)lp % alignof(max_align_t) == 0);
}

static void TestMixedAllocations(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    void *rglp[64];
    int i;

    for (i = 0; i < 64; i++) {
        rglp[i] = LpAlloc(app, i + 1, htMisc);
        CheckAligned(rglp[i]);
        memset(rglp[i], i, i + 1);
    }
    for (i = 0; i < 64; i++) {
        TEST_CHECK(((uint8_t *)rglp[i])[i] == i);
    }
    ResetHb(app, htMisc);
    CheckAligned(LpAlloc(app, 1, htMisc));
    CheckAligned(LpAlloc(app, sizeof(void *), htMisc));
    FreeHb(app, app.state.rglphb[htMisc]);
    app.state.rglphb[htMisc] = NULL;
}

static void TestReuseAndGrowth(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t *lp1, *lp2, *lp3, *lpNew;
    HB *lphb;
    uint16_t cbAllocSav, cbRequest;
    int i;

    cbAllocSav = app.state.mphtcbAlloc[htMisc];
    // All three allocations must share one block even with 16-byte alignment.
    app.state.mphtcbAlloc[htMisc] = 256;
    lp1 = (uint8_t *)LpAlloc(app, 17, htMisc);
    lp2 = (uint8_t *)LpAlloc(app, 17, htMisc);
    lp3 = (uint8_t *)LpAlloc(app, 17, htMisc);
    lphb = app.state.rglphb[htMisc];
    // Request more than one allocation can hold, whatever its native alignment.
    cbRequest = RawLoad16(lp1 - 2) + 1;
    // Consume the tail so the next allocation exercises coalescing, not slop.
    while (lphb->cbSlop >= cbRequest) {
        LpAlloc(app, 17, htMisc);
        TEST_ASSERT(app.state.rglphb[htMisc] == lphb);
    }
    FreeLp(app, lp1, htMisc);
    FreeLp(app, lp2, htMisc);
    lpNew = (uint8_t *)LpAlloc(app, cbRequest, htMisc);
    CheckAligned(lpNew);
    TEST_CHECK(lpNew == lp1);
    TEST_CHECK(app.state.rglphb[htMisc] == lphb);
    memset(lp3, 0x5a, 17);
    lpNew = (uint8_t *)LpReAlloc(app, lp3, 80, htMisc);
    CheckAligned(lpNew);
    for (i = 0; i < 17; i++) TEST_CHECK(lpNew[i] == 0x5a);
    FreeLp(app, lpNew, htMisc);
    lpNew = (uint8_t *)LpAlloc(app, 1, htMisc);
    CheckAligned(lpNew);
    memset(lpNew, 0x3c, 1);
    lp1 = (uint8_t *)LpReAlloc(app, lpNew, 17, htMisc);
    CheckAligned(lp1);
    TEST_CHECK(lp1 == lpNew);
    TEST_CHECK(lp1[0] == 0x3c);
    FreeHb(app, app.state.rglphb[htMisc]);
    app.state.rglphb[htMisc] = NULL;
    app.state.mphtcbAlloc[htMisc] = cbAllocSav;
}

static void TestSingleBlockGrowth(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t *lp;
    HeapType ht;
    int i;

    for (ht = htPlanets; ht <= htThings; ht = static_cast<HeapType>(ht + htThings - htPlanets)) {
        lp = (uint8_t *)LpAlloc(app, 31, ht);
        memset(lp, 0x69, 31);
        lp = (uint8_t *)LpReAlloc(app, lp, 30000, ht);
        CheckAligned(lp);
        for (i = 0; i < 31; i++) TEST_CHECK(lp[i] == 0x69);
        TEST_CHECK(app.state.rglphb[ht]->lphbNext == NULL);
        FreeHb(app, app.state.rglphb[ht]);
        app.state.rglphb[ht] = NULL;
    }
}

static void TestAllocationOverflow(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);


    TEST_ASSERT(FStarsTestInit(app));
    try {
        LpAlloc(app, UINT16_MAX, htMisc);
        TEST_CHECK(0);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorMemory);
        TEST_CHECK(cStarsTestAlert == 1);
    }
    TEST_CHECK(!FStarsRecoveryActive());
}


static void TestBattleByteWindows(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t *lpb;
    HB *lphb;
    TOK tok{};
    KILL kill;

    TEST_ASSERT(FStarsTestInit(app));
    lpb = (uint8_t *)LpAlloc(app, 64, htBattle);
    TEST_CHECK((uintptr_t)lpb % 2 == 0);
    lphb = LphbFromLpHt(app, lpb, htBattle);
    TEST_ASSERT(lphb != NULL);
    BdvFromLp(app, lpb).SetId(0x1234);
    BdvFromLp(app, lpb).SetCplr(2);
    BdvFromLp(app, lpb).SetCtok(1);
    BdvFromLp(app, lpb).SetCbData(57);
    BdvFromLp(app, lpb).SetPt({1000, -1});
    tok.id = 511; tok.iplr = 15; tok.csh = 32768;
    TEST_ASSERT(FEncodeTok(tok, std::as_writable_bytes(std::span(lpb + cbWireBattleData, cbWireTok))));
    TEST_CHECK(BdvFromLp(app, lpb).id() == 0x1234 && BdvFromLp(app, lpb).cplr() == 2);
    TEST_CHECK(BdvFromLp(app, lpb).pt().x() == 1000 && BdvFromLp(app, lpb).pt().y() == -1);
    TEST_CHECK(BdvFromLp(app, lpb).rgtok(0).id() == 511 && BdvFromLp(app, lpb).rgtok(0).iplr() == 15 && BdvFromLp(app, lpb).rgtok(0).csh() == 32768);
    TEST_CHECK(BdvFromLp(app, lpb).FirstEvent() == lpb + 43 && BdvFromLp(app, lpb).Next() == lpb + 57);
    {
        NATIVEBATTLEEVENTVIEW event = BevFromLp(app, lpb + 43);
        event.SetCtok(1);
        event.rgkill(0).SetCshKill(65535);
        event.rgkill(0).dv().SetPctSh(100);
        event.rgkill(0).dv().SetPctDp(511);
        kill = event.rgkill(0).ReadValue();
        TEST_CHECK(event.Next() == lpb + 57 && kill.cshKill == 65535 && kill.dv.pctSh() == 100 && kill.dv.pctDp() == 511);
    }
    TEST_CHECK(BbwFromLp(app, (uint8_t *)lphb->hmem + lphb->cbBlock).Lpb() == (uint8_t *)lphb->hmem + lphb->cbBlock);
    FreeHb(app, app.state.rglphb[htBattle]);
    app.state.rglphb[htBattle] = NULL;
}


static void TestLegacyBattleConversion(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t *lpb;
    const uint8_t legacy[] = {2, 3, 0xab, 1, 0xcd, 0xef};
    const uint8_t current[] = {2, 3, 1, 0, 0xcd, 0xab};
    const uint8_t kill[] = {7, 0xa5, 0x34, 0x12, 0xff, 0xff, 0x78, 0x56};

    TEST_ASSERT(FStarsTestInit(app));
    lpb = (uint8_t *)LpAlloc(app, 64, htBattle);
    BdvFromLp(app, lpb).SetId(1);
    BdvFromLp(app, lpb).SetCtok(0);
    BdvFromLp(app, lpb).SetCbData(28);
    memcpy(lpb + 14, legacy, sizeof(legacy));
    memcpy(lpb + 20, kill, sizeof(kill));
    lpb[28] = lpb[29] = 0xff;
    UpdateBattleRecords(app);
    TEST_CHECK(memcmp(lpb + 14, current, sizeof(current)) == 0);
    TEST_CHECK(memcmp(lpb + 20, kill, sizeof(kill)) == 0);
    TEST_CHECK(BdvFromLp(app, lpb).cbData() == 28 && lpb[28] == 0xff && lpb[29] == 0xff);
    FreeHb(app, app.state.rglphb[htBattle]);
    app.state.rglphb[htBattle] = NULL;
}

// Use the actual AI validator, including whole-row compaction and the old
// donor-count decrement / receiver-count postincrement / ID publication order.
static void TestAiHistoryBytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    PLANET rgpl[4]{};
    PLANET *lpplSav;
    int16_t cPlanetSav, cPlanMaxSav;
    int32_t seed1, seed2;
    int16_t i, j;

    TEST_ASSERT(FStarsTestInit(app));
    app.state.vlpbAiData = (uint8_t *)LpAlloc(app, 8192, htMisc);
    memset(app.state.vlpbAiData, 0xa5, 8192);
    app.state.idPlayer = 0; app.state.rgplr[0].idAi = idAiRobotoid;
    lpplSav = app.state.lpPlanets; cPlanetSav = app.state.cPlanet; cPlanMaxSav = app.state.game.cPlanMax;
    app.state.lpPlanets = rgpl; app.state.cPlanet = app.state.game.cPlanMax = 4;
    for (i = 0; i < 4; i++) { rgpl[i].id = i; rgpl[i].iPlayer = i == 3 ? 1 : 0; rgpl[i].fStarbase = 1; }
    seed1 = app.state.lRandSeed1; seed2 = app.state.lRandSeed2;
    AhvFromLp(app, app.state.vlpbAiData).SetCbAiHist(2); AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(-1);
    app.state.game.turn = 19;
    ValidateStarbaseHistory(app);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).cStarbase() == 0 && AhvFromLp(app, app.state.vlpbAiData).cbAiHist() == 2 && app.state.vlpbAiData[4] == 0xa5);
    app.state.game.turn = 20;
    AhvFromLp(app, app.state.vlpbAiData).SetCbAiHist(4); AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(65);
    ValidateStarbaseHistory(app);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).cStarbase() == 0 && AhvFromLp(app, app.state.vlpbAiData).cbAiHist() == 4);
    AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(3);
    for (i = 0; i < 3; i++) {
        AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetIdPlanet(i == 0 ? 3 : i - 1);
        AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetCFreighter(i == 1 ? 9 : 4);
        for (j = 0; j < 8; j++) AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetRgflid(j, (i == 1 ? 200 : 100) + j);
    }
    ValidateStarbaseHistory(app);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).cStarbase() == 2 && AhvFromLp(app, app.state.vlpbAiData).cbAiHist() == 44);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(0).idPlanet() == 0 && AhvFromLp(app, app.state.vlpbAiData).rgasb(0).cFreighter() == 1);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(0).rgflid(0) == 103 && AhvFromLp(app, app.state.vlpbAiData).rgasb(0).rgflid(7) == 207);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(1).idPlanet() == 1 && AhvFromLp(app, app.state.vlpbAiData).rgasb(1).cFreighter() == 3);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(1).rgflid(2) == 102 && AhvFromLp(app, app.state.vlpbAiData).rgasb(1).rgflid(3) == 103);
    ValidateStarbaseHistory(app);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(0).cFreighter() == 2 && AhvFromLp(app, app.state.vlpbAiData).rgasb(0).rgflid(1) == 102);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).rgasb(1).cFreighter() == 2 && app.state.lRandSeed1 == seed1 && app.state.lRandSeed2 == seed2);
    app.state.rgplr[0].idAi = idAiCybertron;
    AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(-123);
    ValidateStarbaseHistory(app);
    TEST_CHECK(AhvFromLp(app, app.state.vlpbAiData).cStarbase() == -123); // Alternate history is opaque to this validator.
    // Callback traversal indexes the same temporary words by planet ID.
    CtvFromLp(app, app.state.vlpbAiData + 10).SetWInfo1(0xa500);
    CtvFromLp(app, app.state.vlpbAiData + 10).At(1).SetWInfo1(0xa500);
    app.state.rgptPlan[0] = {1000, 1000}; app.state.rgptPlan[1] = {1100, 1000};
    TEST_CHECK(FEnumDropOffStage1(app, &rgpl[0], &rgpl[1]));
    CtvFromLp(app, app.state.vlpbAiData + 10).At(1).SetCFreightersDst(3);
    TEST_CHECK(!FEnumDropOffStage1(app, &rgpl[0], &rgpl[1]) && !FEnumDropOffStage2(app, &rgpl[0], &rgpl[1]));
    CtvFromLp(app, app.state.vlpbAiData + 10).At(1).SetCFreightersDst(1);
    TEST_CHECK(!FEnumDropOffStage1(app, &rgpl[0], &rgpl[1]) && FEnumDropOffStage2(app, &rgpl[0], &rgpl[1]));
    TEST_CHECK(CtvFromLp(app, app.state.vlpbAiData + 10).At(1).wInfo1() == 0xa508);
    FreeLp(app, app.state.vlpbAiData, htMisc); app.state.vlpbAiData = NULL;
    app.state.lpPlanets = lpplSav; app.state.cPlanet = cPlanetSav; app.state.game.cPlanMax = cPlanMaxSav;
}

static void TestThingPhysicalCopies(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HB *lphbSav = app.state.rglphb[htThings];
    uint16_t cbAllocSav = app.state.mphtcbAlloc[htThings];
    uint8_t input[80], expected[80];
    app.state.rglphb[htThings] = nullptr; app.state.mphtcbAlloc[htThings] = 512;
    auto *pth = static_cast<THING *>(LpAlloc(app, 3 * 18, htThings));
    auto storage = TbwFromLp(app, pth).Rgb();
    TEST_ASSERT(storage.size() >= 80);
    TEST_CHECK(sizeof(THING) == 18 && reinterpret_cast<uint8_t *>(pth + 1) - reinterpret_cast<uint8_t *>(pth) == 18);
    for (int i = 0; i < 80; i++) input[i] = static_cast<uint8_t>(i * 11 + 3);
    for (uint16_t cb = 0; cb <= 80; cb++) {
        memset(storage.data(), 0xa5, 80); memset(expected, 0xa5, 80);
        memcpy(expected, input, cb);
        CopyThingBytes(app, pth, input, cb);
        TEST_CHECK(!memcmp(storage.data(), expected, 80));
        TEST_CHECK(pth[1].idFull() == static_cast<uint16_t>(expected[18] | (expected[19] << 8)));
        TEST_CHECK(pth[2].turn() == static_cast<uint16_t>(expected[52] | (expected[53] << 8)));
    }
    FreeHb(app, app.state.rglphb[htThings]); app.state.rglphb[htThings] = lphbSav; app.state.mphtcbAlloc[htThings] = cbAllocSav;
}


// Ownership must preserve slots, spare bytes and reset reuse; explicit release
// and scope teardown must both clear the same published roots.
static void TestTypedHeapOwner(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HB *rglphbSav[12];
    uint16_t cbAllocSav = app.state.mphtcbAlloc[htMisc];
    std::memcpy(rglphbSav, app.state.rglphb, sizeof(rglphbSav));
    std::memset(app.state.rglphb, 0, sizeof(rglphbSav));
    app.state.mphtcbAlloc[htMisc] = 512;
    {
        NATIVEGAMEHEAPS owner(app);
        auto *rg = LpAlloc<int32_t>(app, 9, htMisc);
        const std::size_t cPhysical = CbPlSlot(app, rg) / sizeof(*rg);
        TEST_ASSERT(cPhysical > 2);
        for (std::size_t i = 0; i < cPhysical; i++) rg[i] = static_cast<int32_t>(0x12345600u + i);
        auto *rgOther = LpAlloc<int32_t>(app, 9, htMisc);
        TEST_CHECK(rgOther != rg);
        FreeLp(app, rgOther, htMisc);
        auto *grown = LpReAlloc<int32_t>(app, rg, 80, htMisc);
        for (std::size_t i = 0; i < cPhysical; i++) TEST_CHECK(grown[i] == static_cast<int32_t>(0x12345600u + i));
        grown[0] = 0x5a5a;
        ResetHb(app, htMisc);
        auto *reused = LpAlloc<int32_t>(app, 9, htMisc);
        TEST_CHECK(reused[0] == 0x5a5a);
        FreeHb(app, app.state.rglphb[htMisc]);
        TEST_CHECK(app.state.rglphb[htMisc] == nullptr);
        owner.Close();
        TEST_CHECK(app.state.rglphb[htMisc] == nullptr);
        LpAlloc<int16_t>(app, 2, htMisc);
    }
    for (HB *lphb : app.state.rglphb) TEST_CHECK(lphb == nullptr);
    std::memcpy(app.state.rglphb, rglphbSav, sizeof(rglphbSav));
    app.state.mphtcbAlloc[htMisc] = cbAllocSav;
}

TEST_LIST = {
#ifdef STARS_UI_SDL
             {"foreign callbacks use the target application owner", TestCallbackApplicationOwners},
#endif
             {"application state addresses survive reset and idempotent close", TestApplicationStateOwner},{"typed heap ownership preserves spare bytes, reset and idempotent teardown", TestTypedHeapOwner},
             {"thing records retain partial, neighbor and physical spare copies", TestThingPhysicalCopies},
             {"AI history retains compaction, redistribution and RNG", TestAiHistoryBytes},
             {"legacy battle conversion retains exact header and kill bytes", TestLegacyBattleConversion},
             {"battle byte views retain physical arena and wire stride", TestBattleByteWindows},
             {"aligned mixed allocations and reset", TestMixedAllocations},
             {"free coalescing and realloc preservation", TestReuseAndGrowth},
             {"planet and thing heap growth", TestSingleBlockGrowth},
             {"allocation size overflow", TestAllocationOverflow},
             {NULL, NULL}};
