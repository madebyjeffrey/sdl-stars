#include <cassert>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "colony_io.h"
#include "common.h"
#include "heap_io.h"
#include "liststorage.h"
#include "planet_query_io.h"
#include "production_io.h"
#include "race_query_io.h"
#include "random_io.h"
#include "research_io.h"
#include "turn_io.h"
static FLEET  fleet;
static FLEET *fleetPointers[2];
alignas(PLORD) static unsigned char orderBytes[sizeof(PLORD) + sizeof(ORDER)];
static PLORD  *orders;
static int     fleetMines;
static PLANET  original, lastPlanet;
static PLANET *seen;
static PROD    savedRows[255];
static struct {
    void  *p;
    size_t bytes;
    int    live;
} heap[8];
static int      heaps;
static int      checks, allocs, frees, failAt, year, builds, growths, draws, messages, guards, actualMining, script;
static int16_t  resource, resourceLater, forcedBuilt, terminalStatus;
static uint16_t wordStatus;
static int      terminalYear, firstYear;
static unsigned spare;
static int      mineYield, capacity, capacityLater, unitOre, unitResources, mutateResource, queryOwner, firstAfterResource;
static int      tracedIndex[400], tracedItem[400], tracedPartial[400], tracedAlchemy[400], tracedYear[400];
static unsigned tracedCount[400], tracedPct[400], tracedUnused[400];
static int32_t  lastResources[4];
static int16_t  drawValues[400];

struct FIXTUREESCAPE {
    int stage;
};
static void escape(int stage) {
    if (failAt == stage)
        throw FIXTUREESCAPE{stage};
}
static int slot(const void *p) {
    for (int i = heaps - 1; i >= 0; i--)
        if (heap[i].p == p && heap[i].live)
            return i;
    abort();
}
static void *TestLpAlloc(APPSESSION &, uint16_t cb, HeapType ht) {
    assert(ht == htOrd && cb >= 4);
    escape(1);
    assert(heaps < 8);
    size_t actual = cb + 4 * spare;
    void  *p = malloc(actual);
    assert(p);
    memset(p, 0xa5, actual);
    heap[heaps].p = p;
    heap[heaps].bytes = actual;
    heap[heaps++].live = 1;
    allocs++;
    return p;
}
static void TestFreeLp(APPSESSION &, void *p, HeapType ht) {
    assert(ht == htOrd);
    int i = slot(p);
    assert(seen && seen->lpplprod == p);
    lastPlanet = *seen;
    assert(seen->lpplprod->iprodMac <= 255);
    memcpy(savedRows, seen->lpplprod->rgprod(), seen->lpplprod->iprodMac * 4);
    heap[i].live = 0;
    frees++;
    free(p);
}
static int32_t TestChgPopFromPlanet(APPSESSION &, PLANET *p, int16_t update) {
    assert(update);
    seen = p;
    escape(5);
    growths++;
    p->rgwtMin[3] += 100;
    return 100;
}
static int16_t TestCResourcesAtPlanet(APPSESSION &, PLANET *p, int16_t player) {
    seen = p;
    queryOwner = player;
    escape(3);
    if (mutateResource) {
        original.iPlayer = 1;
        p->fNoResearch = mutateResource == 2;
    }
    firstAfterResource = p->cFactories;
    return year > 1 ? resourceLater : resource;
}
static int16_t TestCMaxOperableMines(APPSESSION &, PLANET *p, int16_t player, int16_t next) {
    (void)p;
    (void)player;
    assert(next);
    return year > 1 ? capacityLater : capacity;
}
static int16_t TestCMaxOperableFactories(APPSESSION &app, PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableMines(app, p, player, next); }
static int16_t TestCMaxOperableDefenses(APPSESSION &app, PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableMines(app, p, player, next); }
static int16_t TestIpctCanTerraformLppl(APPSESSION &, PLANET *p) {
    (void)p;
    return 10;
}
static int16_t TestPctPlanetDesirability(APPSESSION &, PLANET *p, int16_t player) {
    (void)p;
    (void)player;
    return 100;
}
static int16_t TestIWarpMAFromLppl(APPSESSION &, PLANET *p, int16_t *flag) {
    (void)p;
    assert(!flag);
    return 10;
}
static int16_t TestFSendPlrMsg2(APPSESSION &, int16_t player, MessageId id, MsgGoto a, int16_t b, int16_t c) {
    (void)player;
    (void)a;
    (void)b;
    (void)c;
    assert(id == idmScientistsHaveTransmutedCommonMaterialsKtEach);
    messages++;
    return 1;
}
static void TestGetProductionCosts(APPSESSION &app, PLANET *p, PROD *row, uint32_t *out, int16_t player, int16_t one) {
    (void)p;
    assert(one && player == p->iPlayer);
    int32_t quote[4] = {unitOre, unitOre, unitOre, unitResources};
    if (row->grobj == grobjPlanet && (row->iItem == iobjAlchemy || row->iItem == mdIdleAlchemy)) {
        quote[0] = quote[1] = quote[2] = 0;
        quote[3] = (app.state.rgplr[player].grbitAttr & (UINT32_C(1) << ibitRaceMineralAlchemy)) ? 25 : 100;
    }
    memcpy(out, quote, sizeof(quote));
}
static int16_t TestCMinesOperating(APPSESSION &, PLANET *p) { return p->cMines; }
static int32_t TestCMineFromLpfl(APPSESSION &, FLEET *f) {
    (void)f;
    return fleetMines;
}
static int16_t TestRandom(APPSESSION &app, int16_t bound) {
    assert(bound == 100 && draws < 400);
    int16_t value = Random(app, bound);
    drawValues[draws++] = value;
    return value;
}

static void TestEstMineralsMined(APPSESSION &app, PLANET *p, int32_t *out, int32_t count, int16_t apply) {
    assert(count == -1 && apply);
    seen = p;
    year++;
    escape(2);
    if (actualMining) {
        EstMineralsMined(app, p, out, count, apply);
        return;
    }
    for (int i = 0; i < 3; i++) {
        p->rgwtMin[i] += mineYield;
        out[i] = 999;
    } // estimator reads stocks, not out
}
static void *TestcheckedCopy(void *dst, const void *src, size_t n) {
    uintptr_t a = (uintptr_t)dst, b = (uintptr_t)src;
    int       goodDst = 0, goodSrc = 0;
    if (n && a < b + n && b < a + n) {
        guards++;
        throw FIXTUREESCAPE{6};
    }
    for (int i = 0; i < heaps; i++)
        if (heap[i].live) {
            uintptr_t p = (uintptr_t)heap[i].p, e = p + heap[i].bytes;
            if (a >= p && a <= e && n <= e - a)
                goodDst = 1;
            if (b >= p && b <= e && n <= e - b)
                goodSrc = 1;
        }
    if (!goodDst || !goodSrc) {
        guards++;
        throw FIXTUREESCAPE{6};
    }
    return memcpy(dst, src, n);
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s (year=%d builds=%d)\n", checks, #x, year, builds);                                                          \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static int16_t TestCBuildProdItem(APPSESSION &app, PLANET *p, PROD *row, PROD *partial, int32_t *res, int16_t alchemy, int16_t *status, int16_t calc) {
    seen = p;
    escape(4);
    assert(!calc && builds < 400);
    int       index = -1;
    uintptr_t at = (uintptr_t)row, base = (uintptr_t)p->lpplprod->rgprod();
    if (at >= base && at < base + (heap[slot(p->lpplprod)].bytes - 4))
        index = (int)((at - base) / 4);
    assert((index == -1) == (partial == NULL));
    tracedIndex[builds] = index;
    tracedItem[builds] = row->iItem;
    tracedCount[builds] = row->cItem;
    tracedPct[builds] = row->pct;
    tracedUnused[builds] = row->unused;
    tracedPartial[builds] = partial != NULL;
    tracedAlchemy[builds] = alchemy;
    tracedYear[builds] = year;
    builds++;
    int16_t built;
    if (script == 0)
        built = CBuildProdItem(app, p, row, partial, res, alchemy, status, calc);
    else if (script == 2) {
        built = index == 0 ? 1 : 0;
        *status = (int16_t)(index == 0 ? wordStatus : static_cast<uint16_t>(mdProdStatBlockedSame));
    } else if (script == 3) {
        built = year == 1 ? 2 : 0;
        *status = year == 1 ? mdProdStatSomeAuto : mdProdStatSkippedAuto;
    } else if (script == 4) {
        row->iItem = mdIdleMine;
        row->grobj = grobjPlanet;
        row->cItem = 0;
        built = 3;
        *status = mdProdStatComplete;
    } else {
        built = year == terminalYear ? forcedBuilt : (year == firstYear ? 1 : 0);
        *status = year == terminalYear ? terminalStatus : static_cast<int16_t>(year == firstYear ? mdProdStatSomeAuto : mdProdStatBlockedSame);
    }
    memcpy(lastResources, res, 16);
    return built;
}
static uint16_t TestExtent(APPSESSION &, const void *p) { return (uint16_t)heap[slot(p)].bytes; }

static PLPROD *queue(APPSESSION &app, unsigned capacity, unsigned count) {
    PLPROD *q = LpplAlloc<PLPROD>(app, 4, capacity, htOrd);
    q->iprodMac = count;
    for (unsigned i = 0; i < capacity; i++)
        q->rgprod()[i] = PROD{};
    return q;
}
static PROD row(unsigned count, unsigned item, unsigned kind) {
    PROD p = {};
    p.cItem = count;
    p.iItem = item;
    p.grobj = kind;
    return p;
}
static void reset(APPSESSION &app) {
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    heaps = allocs = frees = failAt = year = builds = growths = draws = messages = guards = actualMining = script = 0;
    spare = 0;
    seen = NULL;
    memset(&original, 0, sizeof(original));
    memset(&lastPlanet, 0, sizeof(lastPlanet));
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    app.state.gd.fGeneratingTurn = 0;
    resource = resourceLater = 100;
    forcedBuilt = 1;
    terminalStatus = mdProdStatComplete;
    terminalYear = 1;
    firstYear = 0;
    mineYield = 0;
    capacity = capacityLater = 10;
    unitOre = 0;
    unitResources = 10;
    mutateResource = 0;
    queryOwner = -1;
    firstAfterResource = -1;
    original.id = 5;
    original.iPlayer = 0;
    original.rgwtMin[3] = 1000;
    original.rgwtMin[0] = original.rgwtMin[1] = original.rgwtMin[2] = 100;
    for (int i = 0; i < 3; i++)
        original.rgMinConc[i] = 50;
    app.state.cFleet = 0;
    memset(&fleet, 0, sizeof(fleet));
    app.state.rglpfl[0] = app.state.rglpfl[1] = NULL;
    app.state.lRandSeed1 = 7;
    app.state.lRandSeed2 = 11;
}
static void begin(void) {
    allocs = frees = year = builds = growths = 0;
    seen = NULL;
}
static int live(void) {
    int n = 0;
    for (int i = 0; i < heaps; i++)
        n += heap[i].live;
    return n;
}
static void estimate(APPSESSION &app, PLPROD *q, uint16_t request, int16_t *first, int16_t *last) {
    EstimateItemProdSched(app, &original, q, static_cast<ProdItemType>(request), first, last);
}
int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HEAPIO ioHeap;
    ioHeap.pfnAllocate = TestLpAlloc;
    ioHeap.pfnFree = TestFreeLp;
    ioHeap.pfnExtent = TestExtent;
    ioHeap.pfnCopy = TestcheckedCopy;
    HEAPIOSCOPE scopeHeap(ioHeap);
    TURNIO      ioTurn;
    ioTurn.pfnMessage2 = TestFSendPlrMsg2;
    TURNIOSCOPE scopeTurn(ioTurn);
    COLONYIO    ioColony;
    ioColony.pfnOperableMines = TestCMaxOperableMines;
    ioColony.pfnOperableFactories = TestCMaxOperableFactories;
    ioColony.pfnOperableDefenses = TestCMaxOperableDefenses;
    ioColony.pfnTerraformPercent = TestIpctCanTerraformLppl;
    ioColony.pfnWarp = TestIWarpMAFromLppl;
    ioColony.pfnMinesOperating = TestCMinesOperating;
    ioColony.pfnFleetMining = TestCMineFromLpfl;
    ioColony.pfnEstimateMining = TestEstMineralsMined;
    COLONYIOSCOPE scopeColony(ioColony);
    PLANETQUERYIO ioPlanetQuery;
    ioPlanetQuery.pfnGrowth = TestChgPopFromPlanet;
    ioPlanetQuery.pfnDesirability = TestPctPlanetDesirability;
    PLANETQUERYIOSCOPE scopePlanetQuery(ioPlanetQuery);
    RESEARCHIO         ioResearch;
    ioResearch.pfnResources = TestCResourcesAtPlanet;
    RESEARCHIOSCOPE scopeResearch(ioResearch);
    PRODUCTIONIO    ioProduction;
    ioProduction.pfnQuote = TestGetProductionCosts;
    ioProduction.pfnBuild = TestCBuildProdItem;
    PRODUCTIONIOSCOPE scopeProduction(ioProduction);
    RANDOMIO          ioRandom;
    ioRandom.pfnRandom = TestRandom;
    RANDOMIOSCOPE scopeRandom(ioRandom);
    app.state.rglpfl = fleetPointers;
    orders = LpStartPl<PLORD, ORDER>(orderBytes, sizeof(orderBytes));
    static_assert(sizeof(PROD) == 4 && sizeof(PLPROD) == 4 && offsetof(PLANET, lpplprod) == 52, "native forecast layout");
    int16_t first, last;
    reset(app);
    original.lpplprod = queue(app, 0, 0);
    PLANET before = original;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 100 && last == 100 && year == 99 && growths == 99 && !builds && frees == 1 && allocs == 1);
    CHECK(memcmp(&before, &original, sizeof(before)) == 0);
    reset(app);
    original.lpplprod = queue(app, 255, 255);
    for (int i = 0; i < 255; i++)
        original.lpplprod->rgprod()[i] = row(1, 0, 2);
    script = 1;
    begin();
    estimate(app, NULL, 254, &first, &last);
    CHECK(first == 1 && last == 1 && builds == 255 && !growths);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 7, 1);
    before = original;
    PROD prior = original.lpplprod->rgprod()[0];
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 1 && last == 1 && year == 1 && builds == 1 && growths == 0 && lastPlanet.cFactories == 0);
    CHECK(memcmp(&before, &original, sizeof(before)) == 0 && memcmp(&prior, original.lpplprod->rgprod(), 4) == 0);
    for (unsigned status = 0; status < 65536; status++) {
        reset(app);
        original.lpplprod = queue(app, 2, 2);
        original.lpplprod->rgprod()[0] = row(1, 0, 2);
        original.lpplprod->rgprod()[1] = row(1, 0, 2);
        script = 2;
        wordStatus = status;
        begin();
        estimate(app, NULL, 0, &first, &last);
        int early = status <= 2, breaks = (int16_t)status >= mdProdStatSome;
        CHECK(first == 1 && last == (status == 2 ? 0 : early ? 1 : 100));
        CHECK(year == (early ? 1 : 99) && growths == (early ? 0 : 99) && builds == (early ? 1 : 99 * (breaks ? 1 : 2)) && frees == 1);
    }
    for (int yearDone = 1; yearDone <= 99; yearDone++)
        for (int status = 0; status < 3; status++)
            for (int built = 0; built < 2; built++) {
                reset(app);
                original.lpplprod = queue(app, 1, 1);
                original.lpplprod->rgprod()[0] = row(1, 0, 2);
                script = 1;
                terminalYear = yearDone;
                terminalStatus = status;
                forcedBuilt = built;
                begin();
                estimate(app, NULL, 0, &first, &last);
                CHECK(year == yearDone && first == (built ? yearDone : 0) && last == (status == 2 ? (built ? yearDone - 1 : 0) : yearDone));
                CHECK(growths == yearDone - 1 && frees == 1);
            }
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 0, 2);
    script = 3;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 1 && last == 1 && year == 2 && growths == 1);
    for (unsigned request = 32768; request < 65536; request++)
        for (int suppress = 0; suppress < 2; suppress++) {
            reset(app);
            original.lpplprod = queue(app, 0, 0);
            original.fNoResearch = suppress;
            resource = (int16_t)request;
            app.state.rgplr[0].pctResearch = 33;
            begin();
            estimate(app, NULL, request, &first, &last);
            int32_t reserved = suppress ? 0 : (int32_t)resource * 33 / 100;
            CHECK(first == (int16_t)((int32_t)resource - reserved + (request == 65535 ? reserved : 0)) && last == 0);
            CHECK(year == 1 && !growths && !builds && frees == 1);
        }
    for (int pct = -128; pct <= 127; pct++) {
        reset(app);
        original.lpplprod = queue(app, 0, 0);
        resource = -32768;
        app.state.rgplr[0].pctResearch = pct;
        begin();
        estimate(app, NULL, 65534, &first, &last);
        CHECK(first == (int16_t)(resource - (int32_t)resource * pct / 100) && last == 0);
    }
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 0, 2);
    script = 1;
    terminalYear = 100;
    firstYear = 2;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 2 && last == 100 && year == 99 && growths == 99);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 0, 2);
    script = 1;
    terminalYear = 100;
    firstYear = 2;
    begin();
    first = 0;
    estimate(app, NULL, 0, &first, &first);
    CHECK(first == 100);
    reset(app);
    original.lpplprod = queue(app, 0, 0);
    resource = 32767;
    app.state.rgplr[0].pctResearch = 0;
    begin();
    estimate(app, NULL, 65534, &first, &first);
    CHECK(first == 32767);
    reset(app);
    original.lpplprod = queue(app, 3, 3);
    original.lpplprod->rgprod()[0] = row(1, 3, 1);
    original.lpplprod->rgprod()[1] = row(0, 7, 1);
    original.lpplprod->rgprod()[2] = row(1, 0, 2);
    script = 1;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == -1 && last == -1 && !builds && !growths && frees == 1);
    begin();
    estimate(app, NULL, 2, &first, &last);
    CHECK(first == 1 && last == 1 && builds == 1 && tracedIndex[0] == 2 && tracedAlchemy[0]);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 3, 1);
    resource = resourceLater = 0;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(first == 0 && last == 0 && tracedCount[0] == 1020 && tracedItem[0] == 3 && builds == 1);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 1, 1);
    original.lpplprod->rgprod()[0].unused = 31;
    resource = 5;
    resourceLater = 20;
    unitOre = 1;
    original.rgwtMin[0] = original.rgwtMin[1] = original.rgwtMin[2] = 100;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 2 && last == 2 && builds == 3 && growths == 1 && lastPlanet.cFactories == 1 && original.cFactories == 0);
    CHECK(tracedIndex[0] == 0 && tracedIndex[1] == -1 && tracedIndex[2] == 0 && tracedItem[1] == 7 && tracedPct[1] == 59 && tracedUnused[1] == 31);
    CHECK(tracedPartial[0] && !tracedPartial[1] && tracedPartial[2]);
    reset(app);
    original.lpplprod = queue(app, 2, 2);
    original.lpplprod->rgprod()[0] = row(1, 1, 1);
    original.lpplprod->rgprod()[1] = row(0, 0, 2);
    original.cMines = 4095;
    script = 4;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(lastPlanet.cMines == 2 && !lastPlanet.cFactories && !growths);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 11, 1);
    resource = resourceLater = 100;
    app.state.gd.fGeneratingTurn = 1;
    begin();
    estimate(app, NULL, 0, &first, &last);
    CHECK(first == 1 && last == 1 && messages == 1 && frees == 1);
    for (int stage = 1; stage <= 5; stage++) {
        reset(app);
        original.lpplprod = queue(app, 1, 1);
        original.lpplprod->rgprod()[0] = row(1, 7, 1);
        failAt = stage;
        script = 1;
        terminalYear = 100;
        first = 123;
        last = 456;
        begin();
        int jumped = 0;
        try {
            estimate(app, NULL, 0, &first, &last);
            CHECK(0);
        } catch (const FIXTUREESCAPE &error) {
            jumped = error.stage;
        }
        CHECK(jumped == stage && frees == 0 && live() == (stage == 1 ? 1 : 2));
        CHECK(first == (stage == 1 ? 123 : 0) && last == (stage == 1 ? 456 : 0));
    }
    reset(app);
    original.lpplprod = queue(app, 256, 2);
    first = 123;
    last = 456;
    begin();
    int jumped = 0;
    try {
        estimate(app, NULL, 65534, &first, &last);
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        jumped = error.stage;
    }
    CHECK(jumped == 6 && guards == 1 && !frees && first == 123 && last == 456);
    reset(app);
    original.lpplprod = queue(app, 256, 2);
    spare = 2;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(first == 100 && last == 0 && frees == 1);
    reset(app);
    original.lpplprod = queue(app, 1, 1);
    original.lpplprod->rgprod()[0] = row(1, 7, 1);
    PLPROD *explicitQ = queue(app, 0, 0);
    begin();
    estimate(app, explicitQ, 65534, &first, &last);
    CHECK(!builds && first == 100 && original.lpplprod->rgprod()[0].cItem == 1);
    reset(app);
    original.lpplprod = queue(app, 0, 0);
    app.state.rgplr[0].pctResearch = 0;
    app.state.rgplr[1].pctResearch = 50;
    resource = 100;
    mutateResource = 1;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(queryOwner == 0 && original.iPlayer == 1 && first == 50);
    reset(app);
    original.lpplprod = queue(app, 0, 0);
    app.state.rgplr[1].pctResearch = 50;
    resource = 100;
    mutateResource = 2;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(first == 100);
    for (int generating = 0; generating < 2; generating++) {
        reset(app);
        original.lpplprod = queue(app, 1, 1);
        original.lpplprod->rgprod()[0] = row(1, 0, 2);
        original.cMines = 1;
        actualMining = 1;
        resource = 100;
        app.state.gd.fGeneratingTurn = generating;
        app.state.rgplr[0].rgAttr[rsMajorAdv] = raMacintosh;
        fleet.idPlanet = original.id;
        fleet.iPlayer = 0;
        fleet.fHereAllTurn = 1;
        fleet.cord = 1;
        fleetMines = 2;
        orders->rgord()[0].grTask = grTaskMine;
        fleet.lpplord = orders;
        app.state.rglpfl[0] = &fleet;
        app.state.cFleet = 1;
        before = original;
        begin();
        estimate(app, NULL, 65535, &first, &last);
        CHECK(!fleet.fHereAllTurn && draws == 3 * generating && ((app.state.lRandSeed1 != 7 || app.state.lRandSeed2 != 11) == generating));
        CHECK(memcmp(&before, &original, sizeof(before)) == 0 && first == 90 && lastPlanet.rgwtMin[0] == 100 + 1 + (generating && drawValues[0] < 50));
    }
    reset(app);
    original.lpplprod = queue(app, 0, 0);
    actualMining = 1;
    resource = 0;
    app.state.rgplr[0].rgAttr[rsMajorAdv] = raMacintosh;
    fleet.idPlanet = original.id;
    fleet.iPlayer = 0;
    fleet.fHereAllTurn = 1;
    fleet.cord = 1;
    fleetMines = 2;
    fleet.lpplord = orders;
    orders->rgord()[0].grTask = grTaskMine;
    app.state.rglpfl[0] = NULL;
    app.state.rglpfl[1] = &fleet;
    app.state.cFleet = 2;
    begin();
    estimate(app, NULL, 65534, &first, &last);
    CHECK(fleet.fHereAllTurn);
    for (int ar = 0; ar < 2; ar++)
        for (int apply = 0; apply < 2; apply++)
            for (int task = 0; task < 16; task++)
                for (int own = 0; own < 2; own++)
                    for (int same = 0; same < 2; same++)
                        for (int dead = 0; dead < 2; dead++)
                            for (int cord = 0; cord < 3; cord++)
                                for (int positive = 0; positive < 3; positive++)
                                    for (int hole = 0; hole < 2; hole++) {
                                        reset(app);
                                        app.state.rgplr[0].rgAttr[rsMajorAdv] = ar ? raMacintosh : raCheapCol;
                                        fleet.idPlanet = original.id + !same;
                                        fleet.iPlayer = own ? 0 : 1;
                                        fleet.fDead = dead;
                                        fleet.cord = cord;
                                        fleetMines = positive == 0 ? -1 : positive == 1 ? 0 : 2;
                                        fleet.fHereAllTurn = 1;
                                        fleet.lpplord = orders;
                                        orders->rgord()[0].grTask = task;
                                        app.state.rglpfl[0] = hole ? NULL : &fleet;
                                        app.state.rglpfl[1] = &fleet;
                                        app.state.cFleet = hole ? 2 : 1;
                                        PLANET  copy = original;
                                        int32_t amounts[3];
                                        EstMineralsMined(app, &copy, amounts, -1, apply);
                                        int marked = ar && apply && task == grTaskMine && own && same && !dead && cord <= 1 && positive == 2 && !hole;
                                        CHECK(fleet.fHereAllTurn == (unsigned)!marked);
                                    }
    reset(app);
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    printf("M09 forecast/build/mining linked witnesses: %d passed\n", checks);
    return 0;
}
