#include "common.h"
#include "appstate.h"
#include "acutest.h"
#include "stars_test.h"

static void TestApplicationStateOwner(void) {
    APPSESSION &session = GetAppSession();
    TEST_CHECK(&game == &session.state.game);
    TEST_CHECK(&ini == &session.state.ini);
    TEST_CHECK(&rglphb == &session.state.rglphb);
    TEST_CHECK(&lFileSeed1 == &session.state.lFileSeed1);
    TEST_CHECK(&lFileSeed2 == &session.state.lFileSeed2);
    TEST_CHECK(sizeof(rgplr) == sizeof(session.state.rgplr));
    game.cPlayer = 7;
    lFileSeed1 = 123;
    lFileSeed2 = 456;
    LpAlloc(31, htMisc);
    TEST_ASSERT(rglphb[htMisc]);
    session.Close(); session.Close();
    for (auto *lphb : rglphb) TEST_CHECK(!lphb);
    session.Reset();
    TEST_CHECK(game.cPlayer == 2 && game.mdSize == sizeMedium);
    TEST_CHECK(lFileSeed1 == 0 && lFileSeed2 == 0);
    TEST_CHECK(&game == &session.state.game);
}

static void CheckAligned(void *lp) {
    TEST_ASSERT(lp != NULL);
    TEST_CHECK((uintptr_t)lp % alignof(max_align_t) == 0);
}

static void TestMixedAllocations(void) {
    void *rglp[64];
    int i;

    for (i = 0; i < 64; i++) {
        rglp[i] = LpAlloc(i + 1, htMisc);
        CheckAligned(rglp[i]);
        memset(rglp[i], i, i + 1);
    }
    for (i = 0; i < 64; i++) {
        TEST_CHECK(((uint8_t *)rglp[i])[i] == i);
    }
    ResetHb(htMisc);
    CheckAligned(LpAlloc(1, htMisc));
    CheckAligned(LpAlloc(sizeof(void *), htMisc));
    FreeHb(rglphb[htMisc]);
    rglphb[htMisc] = NULL;
}

static void TestReuseAndGrowth(void) {
    uint8_t *lp1, *lp2, *lp3, *lpNew;
    HB *lphb;
    uint16_t cbAllocSav, cbRequest;
    int i;

    cbAllocSav = mphtcbAlloc[htMisc];
    // All three allocations must share one block even with 16-byte alignment.
    mphtcbAlloc[htMisc] = 256;
    lp1 = (uint8_t *)LpAlloc(17, htMisc);
    lp2 = (uint8_t *)LpAlloc(17, htMisc);
    lp3 = (uint8_t *)LpAlloc(17, htMisc);
    lphb = rglphb[htMisc];
    // Request more than one allocation can hold, whatever its native alignment.
    cbRequest = RawLoad16(lp1 - 2) + 1;
    // Consume the tail so the next allocation exercises coalescing, not slop.
    while (lphb->cbSlop >= cbRequest) {
        LpAlloc(17, htMisc);
        TEST_ASSERT(rglphb[htMisc] == lphb);
    }
    FreeLp(lp1, htMisc);
    FreeLp(lp2, htMisc);
    lpNew = (uint8_t *)LpAlloc(cbRequest, htMisc);
    CheckAligned(lpNew);
    TEST_CHECK(lpNew == lp1);
    TEST_CHECK(rglphb[htMisc] == lphb);
    memset(lp3, 0x5a, 17);
    lpNew = (uint8_t *)LpReAlloc(lp3, 80, htMisc);
    CheckAligned(lpNew);
    for (i = 0; i < 17; i++) TEST_CHECK(lpNew[i] == 0x5a);
    FreeLp(lpNew, htMisc);
    lpNew = (uint8_t *)LpAlloc(1, htMisc);
    CheckAligned(lpNew);
    memset(lpNew, 0x3c, 1);
    lp1 = (uint8_t *)LpReAlloc(lpNew, 17, htMisc);
    CheckAligned(lp1);
    TEST_CHECK(lp1 == lpNew);
    TEST_CHECK(lp1[0] == 0x3c);
    FreeHb(rglphb[htMisc]);
    rglphb[htMisc] = NULL;
    mphtcbAlloc[htMisc] = cbAllocSav;
}

static void TestSingleBlockGrowth(void) {
    uint8_t *lp;
    HeapType ht;
    int i;

    for (ht = htPlanets; ht <= htThings; ht = static_cast<HeapType>(ht + htThings - htPlanets)) {
        lp = (uint8_t *)LpAlloc(31, ht);
        memset(lp, 0x69, 31);
        lp = (uint8_t *)LpReAlloc(lp, 30000, ht);
        CheckAligned(lp);
        for (i = 0; i < 31; i++) TEST_CHECK(lp[i] == 0x69);
        TEST_CHECK(rglphb[ht]->lphbNext == NULL);
        FreeHb(rglphb[ht]);
        rglphb[ht] = NULL;
    }
}

static void TestAllocationOverflow(void) {

    TEST_ASSERT(FStarsTestInit());
    try {
        LpAlloc(UINT16_MAX, htMisc);
        TEST_CHECK(FALSE);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorMemory);
        TEST_CHECK(cStarsTestAlert == 1);
    }
    TEST_CHECK(!FStarsRecoveryActive());
}


static void TestBattleByteWindows(void) {
    uint8_t *lpb;
    HB *lphb;
    TOK tok{};
    KILL kill;

    TEST_ASSERT(FStarsTestInit());
    lpb = (uint8_t *)LpAlloc(64, htBattle);
    TEST_CHECK((uintptr_t)lpb % 2 == 0);
    lphb = LphbFromLpHt(lpb, htBattle);
    TEST_ASSERT(lphb != NULL);
    BdvFromLp(lpb).SetId(0x1234);
    BdvFromLp(lpb).SetCplr(2);
    BdvFromLp(lpb).SetCtok(1);
    BdvFromLp(lpb).SetCbData(57);
    BdvFromLp(lpb).SetPt({1000, -1});
    tok.id = 511; tok.iplr = 15; tok.csh = 32768;
    TEST_ASSERT(FEncodeTok(tok, std::as_writable_bytes(std::span(lpb + cbWireBattleData, cbWireTok))));
    TEST_CHECK(BdvFromLp(lpb).id() == 0x1234 && BdvFromLp(lpb).cplr() == 2);
    TEST_CHECK(BdvFromLp(lpb).pt().x() == 1000 && BdvFromLp(lpb).pt().y() == -1);
    TEST_CHECK(BdvFromLp(lpb).rgtok(0).id() == 511 && BdvFromLp(lpb).rgtok(0).iplr() == 15 && BdvFromLp(lpb).rgtok(0).csh() == 32768);
    TEST_CHECK(BdvFromLp(lpb).FirstEvent() == lpb + 43 && BdvFromLp(lpb).Next() == lpb + 57);
    {
        NATIVEBATTLEEVENTVIEW event = BevFromLp(lpb + 43);
        event.SetCtok(1);
        event.rgkill(0).SetCshKill(65535);
        event.rgkill(0).dv().SetPctSh(100);
        event.rgkill(0).dv().SetPctDp(511);
        kill = event.rgkill(0).ReadValue();
        TEST_CHECK(event.Next() == lpb + 57 && kill.cshKill == 65535 && kill.dv.pctSh() == 100 && kill.dv.pctDp() == 511);
    }
    TEST_CHECK(BbwFromLp((uint8_t *)lphb->hmem + lphb->cbBlock).Lpb() == (uint8_t *)lphb->hmem + lphb->cbBlock);
    FreeHb(rglphb[htBattle]);
    rglphb[htBattle] = NULL;
}


static void TestLegacyBattleConversion(void) {
    uint8_t *lpb;
    const uint8_t legacy[] = {2, 3, 0xab, 1, 0xcd, 0xef};
    const uint8_t current[] = {2, 3, 1, 0, 0xcd, 0xab};
    const uint8_t kill[] = {7, 0xa5, 0x34, 0x12, 0xff, 0xff, 0x78, 0x56};

    TEST_ASSERT(FStarsTestInit());
    lpb = (uint8_t *)LpAlloc(64, htBattle);
    BdvFromLp(lpb).SetId(1);
    BdvFromLp(lpb).SetCtok(0);
    BdvFromLp(lpb).SetCbData(28);
    memcpy(lpb + 14, legacy, sizeof(legacy));
    memcpy(lpb + 20, kill, sizeof(kill));
    lpb[28] = lpb[29] = 0xff;
    UpdateBattleRecords();
    TEST_CHECK(memcmp(lpb + 14, current, sizeof(current)) == 0);
    TEST_CHECK(memcmp(lpb + 20, kill, sizeof(kill)) == 0);
    TEST_CHECK(BdvFromLp(lpb).cbData() == 28 && lpb[28] == 0xff && lpb[29] == 0xff);
    FreeHb(rglphb[htBattle]);
    rglphb[htBattle] = NULL;
}

// Use the actual AI validator, including whole-row compaction and the old
// donor-count decrement / receiver-count postincrement / ID publication order.
static void TestAiHistoryBytes(void) {
    PLANET rgpl[4]{};
    PLANET *lpplSav;
    int16_t cPlanetSav, cPlanMaxSav;
    int32_t seed1, seed2;
    int16_t i, j;

    TEST_ASSERT(FStarsTestInit());
    vlpbAiData = (uint8_t *)LpAlloc(8192, htMisc);
    memset(vlpbAiData, 0xa5, 8192);
    idPlayer = 0; rgplr[0].idAi = idAiRobotoid;
    lpplSav = lpPlanets; cPlanetSav = cPlanet; cPlanMaxSav = game.cPlanMax;
    lpPlanets = rgpl; cPlanet = game.cPlanMax = 4;
    for (i = 0; i < 4; i++) { rgpl[i].id = i; rgpl[i].iPlayer = i == 3 ? 1 : 0; rgpl[i].fStarbase = TRUE; }
    seed1 = lRandSeed1; seed2 = lRandSeed2;
    AhvFromLp(vlpbAiData).SetCbAiHist(2); AhvFromLp(vlpbAiData).SetCStarbase(-1);
    game.turn = 19;
    ValidateStarbaseHistory();
    TEST_CHECK(AhvFromLp(vlpbAiData).cStarbase() == 0 && AhvFromLp(vlpbAiData).cbAiHist() == 2 && vlpbAiData[4] == 0xa5);
    game.turn = 20;
    AhvFromLp(vlpbAiData).SetCbAiHist(4); AhvFromLp(vlpbAiData).SetCStarbase(65);
    ValidateStarbaseHistory();
    TEST_CHECK(AhvFromLp(vlpbAiData).cStarbase() == 0 && AhvFromLp(vlpbAiData).cbAiHist() == 4);
    AhvFromLp(vlpbAiData).SetCStarbase(3);
    for (i = 0; i < 3; i++) {
        AhvFromLp(vlpbAiData).rgasb(i).SetIdPlanet(i == 0 ? 3 : i - 1);
        AhvFromLp(vlpbAiData).rgasb(i).SetCFreighter(i == 1 ? 9 : 4);
        for (j = 0; j < 8; j++) AhvFromLp(vlpbAiData).rgasb(i).SetRgflid(j, (i == 1 ? 200 : 100) + j);
    }
    ValidateStarbaseHistory();
    TEST_CHECK(AhvFromLp(vlpbAiData).cStarbase() == 2 && AhvFromLp(vlpbAiData).cbAiHist() == 44);
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(0).idPlanet() == 0 && AhvFromLp(vlpbAiData).rgasb(0).cFreighter() == 1);
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(0).rgflid(0) == 103 && AhvFromLp(vlpbAiData).rgasb(0).rgflid(7) == 207);
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(1).idPlanet() == 1 && AhvFromLp(vlpbAiData).rgasb(1).cFreighter() == 3);
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(1).rgflid(2) == 102 && AhvFromLp(vlpbAiData).rgasb(1).rgflid(3) == 103);
    ValidateStarbaseHistory();
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(0).cFreighter() == 2 && AhvFromLp(vlpbAiData).rgasb(0).rgflid(1) == 102);
    TEST_CHECK(AhvFromLp(vlpbAiData).rgasb(1).cFreighter() == 2 && lRandSeed1 == seed1 && lRandSeed2 == seed2);
    rgplr[0].idAi = idAiCybertron;
    AhvFromLp(vlpbAiData).SetCStarbase(-123);
    ValidateStarbaseHistory();
    TEST_CHECK(AhvFromLp(vlpbAiData).cStarbase() == -123); // Alternate history is opaque to this validator.
    // Callback traversal indexes the same temporary words by planet ID.
    CtvFromLp(vlpbAiData + 10).SetWInfo1(0xa500);
    CtvFromLp(vlpbAiData + 10).At(1).SetWInfo1(0xa500);
    rgptPlan[0] = {1000, 1000}; rgptPlan[1] = {1100, 1000};
    TEST_CHECK(FEnumDropOffStage1(&rgpl[0], &rgpl[1]));
    CtvFromLp(vlpbAiData + 10).At(1).SetCFreightersDst(3);
    TEST_CHECK(!FEnumDropOffStage1(&rgpl[0], &rgpl[1]) && !FEnumDropOffStage2(&rgpl[0], &rgpl[1]));
    CtvFromLp(vlpbAiData + 10).At(1).SetCFreightersDst(1);
    TEST_CHECK(!FEnumDropOffStage1(&rgpl[0], &rgpl[1]) && FEnumDropOffStage2(&rgpl[0], &rgpl[1]));
    TEST_CHECK(CtvFromLp(vlpbAiData + 10).At(1).wInfo1() == 0xa508);
    FreeLp(vlpbAiData, htMisc); vlpbAiData = NULL;
    lpPlanets = lpplSav; cPlanet = cPlanetSav; game.cPlanMax = cPlanMaxSav;
}

static void TestThingPhysicalCopies(void) {
    HB *lphbSav = rglphb[htThings];
    uint16_t cbAllocSav = mphtcbAlloc[htThings];
    uint8_t input[80], expected[80];
    rglphb[htThings] = nullptr; mphtcbAlloc[htThings] = 512;
    auto *pth = static_cast<THING *>(LpAlloc(3 * 18, htThings));
    auto storage = TbwFromLp(pth).Rgb();
    TEST_ASSERT(storage.size() >= 80);
    TEST_CHECK(sizeof(THING) == 18 && reinterpret_cast<uint8_t *>(pth + 1) - reinterpret_cast<uint8_t *>(pth) == 18);
    for (int i = 0; i < 80; i++) input[i] = static_cast<uint8_t>(i * 11 + 3);
    for (uint16_t cb = 0; cb <= 80; cb++) {
        memset(storage.data(), 0xa5, 80); memset(expected, 0xa5, 80);
        memcpy(expected, input, cb);
        CopyThingBytes(pth, input, cb);
        TEST_CHECK(!memcmp(storage.data(), expected, 80));
        TEST_CHECK(pth[1].idFull() == static_cast<uint16_t>(expected[18] | (expected[19] << 8)));
        TEST_CHECK(pth[2].turn() == static_cast<uint16_t>(expected[52] | (expected[53] << 8)));
    }
    FreeHb(rglphb[htThings]); rglphb[htThings] = lphbSav; mphtcbAlloc[htThings] = cbAllocSav;
}


// Ownership must preserve slots, spare bytes and reset reuse; explicit release
// and scope teardown must both clear the same published roots.
static void TestTypedHeapOwner(void) {
    HB *rglphbSav[12];
    uint16_t cbAllocSav = mphtcbAlloc[htMisc];
    std::memcpy(rglphbSav, rglphb, sizeof(rglphbSav));
    std::memset(rglphb, 0, sizeof(rglphbSav));
    mphtcbAlloc[htMisc] = 512;
    {
        NATIVEGAMEHEAPS owner(rglphb);
        auto *rg = LpAlloc<int32_t>(9, htMisc);
        const std::size_t cPhysical = CbPlSlot(rg) / sizeof(*rg);
        TEST_ASSERT(cPhysical > 2);
        for (std::size_t i = 0; i < cPhysical; i++) rg[i] = static_cast<int32_t>(0x12345600u + i);
        auto *rgOther = LpAlloc<int32_t>(9, htMisc);
        TEST_CHECK(rgOther != rg);
        FreeLp(rgOther, htMisc);
        auto *grown = LpReAlloc<int32_t>(rg, 80, htMisc);
        for (std::size_t i = 0; i < cPhysical; i++) TEST_CHECK(grown[i] == static_cast<int32_t>(0x12345600u + i));
        grown[0] = 0x5a5a;
        ResetHb(htMisc);
        auto *reused = LpAlloc<int32_t>(9, htMisc);
        TEST_CHECK(reused[0] == 0x5a5a);
        FreeHb(rglphb[htMisc]);
        TEST_CHECK(rglphb[htMisc] == nullptr);
        owner.Close();
        TEST_CHECK(rglphb[htMisc] == nullptr);
        LpAlloc<int16_t>(2, htMisc);
    }
    for (HB *lphb : rglphb) TEST_CHECK(lphb == nullptr);
    std::memcpy(rglphb, rglphbSav, sizeof(rglphbSav));
    mphtcbAlloc[htMisc] = cbAllocSav;
}

TEST_LIST = {{"application state aliases borrow one resettable session", TestApplicationStateOwner},{"typed heap ownership preserves spare bytes, reset and idempotent teardown", TestTypedHeapOwner},
             {"thing records retain partial, neighbor and physical spare copies", TestThingPhysicalCopies},
             {"AI history retains compaction, redistribution and RNG", TestAiHistoryBytes},
             {"legacy battle conversion retains exact header and kill bytes", TestLegacyBattleConversion},
             {"battle byte views retain physical arena and wire stride", TestBattleByteWindows},
             {"aligned mixed allocations and reset", TestMixedAllocations},
             {"free coalescing and realloc preservation", TestReuseAndGrowth},
             {"planet and thing heap growth", TestSingleBlockGrowth},
             {"allocation size overflow", TestAllocationOverflow},
             {NULL, NULL}};
