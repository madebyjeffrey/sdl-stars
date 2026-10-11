#include "acutest.h"
#include "stars_test.h"

static void CheckAligned(void *lp) {
    TEST_ASSERT(lp != NULL);
    TEST_CHECK((uintptr_t)lp % _Alignof(max_align_t) == 0);
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
    uint16_t cbAllocSav;
    int i;

    cbAllocSav = mphtcbAlloc[htMisc];
    mphtcbAlloc[htMisc] = 128;
    lp1 = LpAlloc(17, htMisc);
    lp2 = LpAlloc(17, htMisc);
    lp3 = LpAlloc(17, htMisc);
    lphb = rglphb[htMisc];
    FreeLp(lp1, htMisc);
    FreeLp(lp2, htMisc);
    lpNew = LpAlloc(49, htMisc);
    CheckAligned(lpNew);
    TEST_CHECK(lpNew == lp1);
    TEST_CHECK(rglphb[htMisc] == lphb);
    memset(lp3, 0x5a, 17);
    lpNew = LpReAlloc(lp3, 80, htMisc);
    CheckAligned(lpNew);
    for (i = 0; i < 17; i++) TEST_CHECK(lpNew[i] == 0x5a);
    FreeLp(lpNew, htMisc);
    lpNew = LpAlloc(1, htMisc);
    CheckAligned(lpNew);
    memset(lpNew, 0x3c, 1);
    lp1 = LpReAlloc(lpNew, 17, htMisc);
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

    for (ht = htPlanets; ht <= htThings; ht += htThings - htPlanets) {
        lp = LpAlloc(31, ht);
        memset(lp, 0x69, 31);
        lp = LpReAlloc(lp, 30000, ht);
        CheckAligned(lp);
        for (i = 0; i < 31; i++) TEST_CHECK(lp[i] == 0x69);
        TEST_CHECK(rglphb[ht]->lphbNext == NULL);
        FreeHb(rglphb[ht]);
        rglphb[ht] = NULL;
    }
}

static void TestAllocationOverflow(void) {
    jmp_buf env;
    jmp_buf *penvSav;

    TEST_ASSERT(FStarsTestInit());
    penvSav = penvMem;
    penvMem = &env;
    if (setjmp(env) == 0) {
        LpAlloc(UINT16_MAX, htMisc);
        TEST_CHECK(FALSE);
    } else {
        TEST_CHECK(cStarsTestAlert == 1);
    }
    penvMem = penvSav;
}

TEST_LIST = {{"aligned mixed allocations and reset", TestMixedAllocations},
             {"free coalescing and realloc preservation", TestReuseAndGrowth},
             {"planet and thing heap growth", TestSingleBlockGrowth},
             {"allocation size overflow", TestAllocationOverflow},
             {NULL, NULL}};
