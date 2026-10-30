#include "common.h"
#include "acutest.h"
#include "stars_test.h"

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

TEST_LIST = {{"legacy battle conversion retains exact header and kill bytes", TestLegacyBattleConversion},
             {"battle byte views retain physical arena and wire stride", TestBattleByteWindows},
             {"aligned mixed allocations and reset", TestMixedAllocations},
             {"free coalescing and realloc preservation", TestReuseAndGrowth},
             {"planet and thing heap growth", TestSingleBlockGrowth},
             {"allocation size overflow", TestAllocationOverflow},
             {NULL, NULL}};
