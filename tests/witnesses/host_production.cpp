#include <cassert>
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
static int    cheapFixture;
static PLANET planet;

static uint16_t extras[1];
static int32_t  score[1];
static int16_t  owners[1];
static struct {
    void  *p;
    size_t bytes;
    int    live;
} heap[8];
static int      heaps;
static int      checks, queries, builds, applications, allocations, reallocations, frees, moves, messages, failResize, script;
static int      notice[16], noticePlayer[16], noticeCount[16], noticeRowCount[16], traceIndex[260], traceAlchemy[260], traceCount[260];
static int16_t  gross, capAbsolute, capOperable, capTerraform, warp, forcedBuilt;
static uint16_t forcedStatus;
static int      applied, partialCount, mutateQuery, resourceDelta, extraRows;
static int32_t  quote[4], seenResources[4], scriptResources;
static char     phases[64];
struct FIXTUREESCAPE {
    int stage;
};
static void event(char c) {
    size_t n = strlen(phases);
    assert(n + 1 < sizeof(phases));
    phases[n] = c;
    phases[n + 1] = 0;
}
static int slot(const void *p) {
    for (int i = heaps - 1; i >= 0; i--)
        if (heap[i].live && heap[i].p == p)
            return i;
    abort();
}
static void *TestLpReAlloc(void *p, uint16_t size, HeapType ht) {
    assert(ht == htOrd);
    reallocations++;
    if (failResize)
        throw FIXTUREESCAPE{8};
    int    at = slot(p);
    size_t old = heap[at].bytes, n = size + (size_t)extraRows * 4;
    if (n < old)
        n = old;
    void *next = realloc(p, n);
    assert(next);
    heap[at].live = 0;
    assert(heaps < 8);
    if (n > old)
        memset((char *)next + old, 0xa5, n - old);
    heap[heaps].p = next;
    heap[heaps].bytes = n;
    heap[heaps++].live = 1;
    return next;
}
static void TestFreeLp(void *p, HeapType ht) {
    assert(ht == htOrd);
    int at = slot(p);
    heap[at].live = 0;
    frees++;
    free(p);
}
static void *TestcheckedMove(void *dst, const void *src, size_t size) {
    uintptr_t a = (uintptr_t)dst, b = (uintptr_t)src;
    int       goodA = 0, goodB = 0;
    for (int i = 0; i < heaps; i++)
        if (heap[i].live) {
            uintptr_t first = (uintptr_t)heap[i].p, last = first + heap[i].bytes;
            if (a >= first && a <= last && size <= last - a)
                goodA = 1;
            if (b >= first && b <= last && size <= last - b)
                goodB = 1;
        }
    if (!goodA || !goodB)
        throw FIXTUREESCAPE{7};
    moves++;
    return memmove(dst, src, size);
}
static void TestMineMinerals(void) {
    event('M');
    assert(rgplr[0].lResLastYear == 1234);
}
static void TestUpdatePopulations(void) {
    event('P');
    planet.rgwtMin[3] += 10;
}
static void TestUpdateResearchStatus(int16_t generating) {
    assert(generating);
    event('R');
}
static void TestRandomEvents(void) {
    event('E');
    planet.rgwtMin[3] += 20;
}
static int16_t TestFSendPlrMsg2(int16_t player, MessageId id, MsgGoto a, int16_t b, int16_t c) {
    (void)a;
    (void)b;
    (void)c;
    if (player < 0 || player >= 16)
        throw FIXTUREESCAPE{10};
    assert(messages < 16);
    notice[messages] = id;
    noticePlayer[messages] = player;
    noticeCount[messages] = planet.lpplprod ? planet.lpplprod->iprodMac : -1;
    noticeRowCount[messages] = planet.lpplprod && planet.lpplprod->iprodMac ? (int)planet.lpplprod->rgprod()[0].cItem : -1;
    messages++;
    return TRUE;
}
static int16_t TestCResourcesAtPlanet(PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    queries++;
    event('Q');
    if (mutateQuery) {
        p->fNoResearch = !p->fNoResearch;
        p->rgwtMin[0] = 999;
    }
    return gross;
}
static int16_t TestCMaxMines(PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    return capAbsolute;
}
static int16_t TestCMaxFactories(PLANET *p, int16_t player) { return TestCMaxMines(p, player); }
static int16_t TestCMaxDefenses(PLANET *p, int16_t player) { return TestCMaxMines(p, player); }
static int16_t TestCMaxOperableMines(PLANET *p, int16_t player, int16_t next) {
    assert(next);
    (void)TestCMaxMines(p, player);
    return capOperable;
}
static int16_t TestCMaxOperableFactories(PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableMines(p, player, next); }
static int16_t TestCMaxOperableDefenses(PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableMines(p, player, next); }
static int16_t TestIpctCanTerraformLppl(PLANET *p) {
    assert(p == &planet);
    return capTerraform;
}
static int32_t TestChgPopFromPlanet(PLANET *p, int16_t update) {
    assert(p == &planet && !update);
    return -1;
}
static int16_t TestPctPlanetDesirability(PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    return 100;
}
static int16_t TestIWarpMAFromLppl(PLANET *p, int16_t *flag) {
    assert(p == &planet && !flag);
    return warp;
}
static int16_t TestGetRaceGrbit(PLAYER *p, RaceGrbit bit) {
    assert(p == rgplr && bit == ibitRaceMineralAlchemy);
    return (int16_t)cheapFixture;
}
static void TestGetProductionCosts(PLANET *p, PROD *row, uint32_t *out, int16_t player, int16_t one) {
    (void)row;
    assert(p == &planet && !player && one);
    memcpy(out, quote, 16);
}
static int16_t TestFBuildObject(PLANET *p, GrobjClass kind, int16_t item, int16_t count, int32_t *resources) {
    (void)kind;
    (void)item;
    assert(p == &planet && count > 0);
    applications++;
    event('A');
    memcpy(seenResources, resources, 16);
    resources[0] += resourceDelta;
    return (int16_t)applied;
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d: %s (builds %d status %u)\n", checks, #x, builds, forcedStatus);                                                       \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static int16_t TestCBuildProdItem(PLANET *p, PROD *row, PROD *partial, int32_t *resources, int16_t alchemy, int16_t *status, int16_t calc) {
    assert(p == &planet && partial && !partial->cItem && status && !calc && builds < 260);
    traceIndex[builds] = (int)(row - p->lpplprod->rgprod());
    traceAlchemy[builds] = alchemy;
    traceCount[builds] = row->cItem;
    builds++;
    event('B');
    if (!script)
        return CBuildProdItem(p, row, partial, resources, alchemy, status, calc);
    *status = (int16_t)forcedStatus;
    resources[3] = scriptResources;
    if (partialCount) {
        memset(partial, 0, sizeof(*partial));
        partial->cItem = partialCount;
        partial->iItem = 11;
        partial->grobj = 1;
        partial->pct = 59;
        partial->unused = 31;
    }
    return forcedBuilt;
}
static uint16_t TestExtent(const void *p) { return (uint16_t)heap[slot(p)].bytes; }

static PROD row(unsigned count, unsigned item, unsigned kind) {
    PROD r = {};
    r.cItem = count;
    r.iItem = item;
    r.grobj = kind;
    r.unused = 7;
    return r;
}
static PLPROD *queue(unsigned maximum, unsigned count, unsigned physical) {
    assert(heaps < 8);
    size_t  n = 4 + 4 * physical;
    PLPROD *p = static_cast<PLPROD *>(malloc(n));
    assert(p);
    memset(p, 0, n);
    p = LpStartPl<PLPROD, PROD>(p, n);
    heap[heaps].p = p;
    heap[heaps].bytes = n;
    heap[heaps++].live = 1;
    allocations++;
    p->cbItem = 4;
    p->ht = htOrd;
    p->iprodMax = maximum;
    p->iprodMac = count;
    for (unsigned i = 0; i < physical; i++)
        p->rgprod()[i] = row(1, 7, 3);
    return p;
}
static void reset(void) {
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    heaps = queries = builds = applications = allocations = reallocations = frees = moves = messages = failResize = script = 0;
    partialCount = mutateQuery = resourceDelta = extraRows = 0;
    gross = 100;
    capAbsolute = capOperable = 100;
    capTerraform = 10;
    warp = 10;
    forcedBuilt = 1;
    forcedStatus = 0;
    applied = 1;
    scriptResources = 7;
    memset(&planet, 0, sizeof(planet));
    memset(rgplr, 0, sizeof(rgplr));
    memset(quote, 0, sizeof(quote));
    quote[3] = 10;
    planet.iScanner = 31;
    planet.idFling = 5;
    planet.rgwtMin[3] = 1000;
    planet.rgwtMin[0] = 10;
    planet.rgwtMin[1] = 20;
    planet.rgwtMin[2] = 30;
    game.cPlayer = 2;
    game.fNoRandom = 0;
    gd.fGeneratingTurn = 1;
    extras[0] = 0;
    rgplr[0].lResLastYear = 1234;
    rgplr[1].lResLastYear = 5678;
    phases[0] = 0;
    score[0] = -1;
    owners[0] = -1;
    memset(seenResources, 0, sizeof(seenResources));
}
static int has(int id) {
    for (int i = 0; i < messages; i++)
        if (notice[i] == id)
            return 1;
    return 0;
}
static int16_t narrow(int value) { return (int16_t)(uint16_t)value; }
int            main(void) {
    HEAPIO ioHeap;
    ioHeap.pfnResize = TestLpReAlloc;
    ioHeap.pfnFree = TestFreeLp;
    ioHeap.pfnExtent = TestExtent;
    ioHeap.pfnMove = TestcheckedMove;
    HEAPIOSCOPE scopeHeap(ioHeap);
    TURNIO      ioTurn;
    ioTurn.pfnMessage2 = TestFSendPlrMsg2;
    ioTurn.pfnMine = TestMineMinerals;
    ioTurn.pfnPopulation = TestUpdatePopulations;
    ioTurn.pfnResearch = TestUpdateResearchStatus;
    ioTurn.pfnEvents = TestRandomEvents;
    TURNIOSCOPE scopeTurn(ioTurn);
    COLONYIO    ioColony;
    ioColony.pfnMaxMines = TestCMaxMines;
    ioColony.pfnMaxFactories = TestCMaxFactories;
    ioColony.pfnMaxDefenses = TestCMaxDefenses;
    ioColony.pfnOperableMines = TestCMaxOperableMines;
    ioColony.pfnOperableFactories = TestCMaxOperableFactories;
    ioColony.pfnOperableDefenses = TestCMaxOperableDefenses;
    ioColony.pfnTerraformPercent = TestIpctCanTerraformLppl;
    ioColony.pfnWarp = TestIWarpMAFromLppl;
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
    ioProduction.pfnApply = TestFBuildObject;
    PRODUCTIONIOSCOPE scopeProduction(ioProduction);
    RACEQUERYIO       ioRaceQuery;
    ioRaceQuery.pfnGrbit = TestGetRaceGrbit;
    RACEQUERYIOSCOPE scopeRaceQuery(ioRaceQuery);
    lpPlanets = &planet;
    cPlanet = 1;
    vrgPlanResExtra = extras;
    vrgwtPopScore = score;
    vrgiplrPopScore = owners;
    static_assert(sizeof(PROD) == 4 && sizeof(PLPROD) == 4 && offsetof(PLANET, lpplprod) == 52, "native queue/planet layout");
    reset();
    Produce();
    CHECK(queries == 1 && !builds && has(idmProductionQueueEmpty) && rgplr[0].lResLastYear == 100);
    CHECK(!strcmp(phases, "MQPRE") && score[0] == 1030 && owners[0] == 0 && rgplr[1].lResLastYear == 0);
    reset();
    planet.iPlayer = -1;
    game.fNoRandom = 1;
    Produce();
    CHECK(!queries && !messages && !strcmp(phases, "MPR") && owners[0] == -1 && score[0] == 1010);
    reset();
    planet.lpplprod = queue(0, 0, 0);
    Produce();
    CHECK(!queries && !builds && !messages && rgplr[0].lResLastYear == 0);
    reset();
    planet.lpplprod = queue(1, 1, 1);
    gross = 0;
    Produce();
    CHECK(queries == 1 && !builds && planet.lpplprod->iprodMac == 1 && !messages);
    reset();
    extras[0] = 100;
    Produce();
    CHECK(rgplr[0].lResLastYear == 150);
    unsigned       undefinedExtra = 0;
    const uint16_t extraValues[] = {0, 1, 100, 32767, 32768, 65534, 65535};
    for (int raw = -32768; raw <= 32767; raw++)
        for (unsigned e = 0; e < sizeof(extraValues) / sizeof(extraValues[0]); e++) {
            int     extra = extraValues[e];
            int64_t expected = raw;
            if (raw && extra) {
                int32_t denominator = (int32_t)((uint32_t)extra + (uint32_t)raw);
                int32_t numerator = (int32_t)((uint32_t)raw * (uint32_t)extra);
                if (!denominator) {
                    undefinedExtra++;
                    continue;
                } // never execute undefined native division
                expected += (int64_t)numerator / denominator;
            }
            assert(expected >= INT32_MIN && expected <= INT32_MAX);
            reset();
            gross = (int16_t)raw;
            extras[0] = (uint16_t)extra;
            Produce();
            CHECK(rgplr[0].lResLastYear == expected && !builds && queries == 1);
        }
    CHECK(undefinedExtra == 4);
    reset();
    planet.lpplprod = queue(1, 1, 1);
    planet.lpplprod->rgprod()[0] = row(1, 7, 3);
    rgplr[0].pctResearch = 20;
    applied = 0;
    resourceDelta = 7;
    Produce();
    CHECK(builds == 1 && applications == 1 && !planet.lpplprod && seenResources[3] == 70 && rgplr[0].lResLastYear == 90);
    CHECK(planet.rgwtMin[0] == 17 && frees == 1 && has(idmHasCompletedOrdersProductionQueueEmpty));
    reset();
    planet.lpplprod = queue(1, 1, 1);
    planet.lpplprod->rgprod()[0] = row(2, 7, 3);
    gross = 15;
    applied = 0;
    Produce();
    CHECK(builds == 1 && applications == 1 && planet.lpplprod && planet.lpplprod->rgprod()[0].cItem == 0 && planet.lpplprod->rgprod()[0].pct == 59 &&
          rgplr[0].lResLastYear == 0);
    rgplr[0].lResLastYear = 1234;
    builds = applications = 0;
    phases[0] = 0;
    Produce();
    CHECK(!builds && !planet.lpplprod && rgplr[0].lResLastYear == 15);
    for (unsigned automatic = 0; automatic < 7; automatic++) {
        reset();
        planet.lpplprod = queue(1, 1, 1);
        planet.lpplprod->rgprod()[0] = row(3, automatic, 1);
        gross = 15;
        applied = 0;
        Produce();
        CHECK(builds == 1 && applications == 1 && planet.lpplprod && planet.lpplprod->iprodMac == 2);
        CHECK(planet.lpplprod->rgprod()[1].iItem == automatic && planet.lpplprod->rgprod()[1].cItem == 3 && planet.lpplprod->rgprod()[0].pct == 59);
        CHECK(rgplr[0].lResLastYear == 0);
    }
    reset();
    planet.lpplprod = queue(1, 1, 1);
    rgplr[0].pctResearch = 20;
    mutateQuery = 1;
    script = 1;
    forcedBuilt = 0;
    forcedStatus = 5;
    Produce();
    CHECK(planet.fNoResearch && planet.rgwtMin[0] == 10 && rgplr[0].lResLastYear == 27);
    reset();
    planet.lpplprod = queue(1, 1, 1);
    planet.fNoResearch = 1;
    rgplr[0].pctResearch = 20;
    mutateQuery = 1;
    script = 1;
    forcedBuilt = 0;
    forcedStatus = 5;
    Produce();
    CHECK(!planet.fNoResearch && rgplr[0].lResLastYear == 7);
    // Scripted status/built/partial pairs exercise caller control, not native
    // builder reachability. Every signed interpretation of the status word.
    for (unsigned status = 0; status < 65536; status++)
        for (int partial = 0; partial < 2; partial++)
            for (int success = 0; success < 2; success++) {
                reset();
                planet.lpplprod = queue(1, 1, 1);
                rgplr[0].pctResearch = 20;
                script = 1;
                forcedStatus = status;
                partialCount = partial;
                applied = success;
                Produce();
                int stop = (int16_t)status >= 5, noticeExpected = status == 0 || (!stop && status != 3 && status != 4);
                CHECK(builds == 1 && applications == 1 && rgplr[0].lResLastYear == 27 && has(idmHasCompletedOrdersProductionQueueEmpty) == noticeExpected);
                if (status == 0) {
                    CHECK(!planet.lpplprod && frees == 1);
                } else {
                    CHECK(planet.lpplprod && planet.lpplprod->iprodMac == 1 + stop * partial && reallocations == stop * partial);
                    PROD *r = planet.lpplprod->rgprod();
                    CHECK(r[stop * partial].cItem == (unsigned)success);
                    if (stop && partial)
                        CHECK(r[0].cItem == 1 && r[0].iItem == 11 && r[0].pct == 59 && r[0].unused == 31);
                }
            }
    // Manual validation, including signed cap narrowing and nonplanet bypass.
    for (unsigned kind = 0; kind < 8; kind++)
        for (unsigned item = 0; item < 128; item++)
            for (int scanner = 0; scanner < 2; scanner++)
                for (int mass = 0; mass < 2; mass++)
                    for (int destination = 0; destination < 2; destination++)
                        for (int cap = -1; cap <= 4; cap++) {
                            reset();
                            planet.lpplprod = queue(1, 1, 1);
                            planet.lpplprod->rgprod()[0] = row(3, item, kind);
                            planet.iScanner = scanner ? 31 : 0;
                            warp = mass;
                            planet.idFling = destination;
                            capAbsolute = (int16_t)(cap - 1);
                            capOperable = capTerraform = (int16_t)cap;
                            planet.cFactories = 1;
                            planet.cMines = 2;
                            planet.cDefenses = 3;
                            script = 1;
                            forcedBuilt = 0;
                            forcedStatus = 5;
                            int clipped = 3, drop = 0, msg = 0;
                            if (kind == 1) {
                                if (item >= 18 && item <= 27) {
                                    drop = !scanner;
                                    msg = drop ? idmOrderBuildScannerCanceledAlreadyHaveScanner : 0;
                                } else if (item >= 14 && item <= 17) {
                                    drop = !mass || !destination;
                                    msg = drop ? idmHasOrdersBuildMineralPacketEitherDoesnt : 0;
                                } else if (item == 7 || item == 8 || item == 9 || item == 12) {
                                    int maximum = item == 12 ? cap : narrow(cap - (item == 7 ? 1 : item == 8 ? 2 : 3));
                                    if (maximum < 3) {
                                        msg =
                                            item == 12 ? idmHasOrdersTerraformBeyondMaximumAllowedOrders : idmHasOrdersBuildPlanetaryInstallationsBeyondMaximu;
                                        if (maximum <= 0)
                                            drop = 1;
                                        else
                                            clipped = maximum;
                                    }
                                }
                            }
                            Produce();
                            CHECK(builds == !drop && (!builds || traceCount[0] == clipped));
                            if (msg)
                                CHECK(has(msg));
                        }
    const int16_t  capacities[] = {-32768, -1, 0, 1023, 1024, 32767};
    const unsigned installedCounts[] = {0, 1, 4095}, manualItems[] = {7, 8, 9, 12}, requestedCounts[] = {3, 1023};
    for (unsigned a = 0; a < 6; a++)
        for (unsigned o = 0; o < 6; o++)
            for (unsigned c = 0; c < 3; c++)
                for (unsigned m = 0; m < 4; m++)
                    for (unsigned r = 0; r < 2; r++) {
                        reset();
                        unsigned item = manualItems[m], installed = installedCounts[c], requested = requestedCounts[r];
                        planet.lpplprod = queue(1, 1, 1);
                        planet.lpplprod->rgprod()[0] = row(requested, item, 1);
                        capAbsolute = capacities[a];
                        capOperable = capTerraform = capacities[o];
                        if (item == 7)
                            planet.cFactories = installed;
                        else if (item == 8)
                            planet.cMines = installed;
                        else
                            planet.cDefenses = installed;
                        script = 1;
                        forcedBuilt = 0;
                        forcedStatus = 5;
                        int maximum = item == 12 ? capTerraform : narrow((capAbsolute > capOperable ? capAbsolute : capOperable) - (int)installed);
                        int reduced = maximum < (int)requested;
                        Produce();
                        CHECK(builds == !(reduced && maximum <= 0));
                        if (builds)
                            CHECK(traceCount[0] == (reduced ? maximum : (int)requested));
                        if (reduced)
                            CHECK(messages >= 1 && noticeRowCount[0] == (int)requested);
                    }
    reset();
    planet.lpplprod = queue(3, 3, 3);
    planet.lpplprod->rgprod()[0] = row(1, 3, 1);
    planet.lpplprod->rgprod()[1] = row(1, 7, 3);
    script = 1;
    forcedBuilt = 0;
    Produce();
    CHECK(builds == 2 && traceIndex[0] == 1 && traceAlchemy[0] && traceIndex[1] == 0 && !traceAlchemy[1] && !planet.lpplprod && moves == 1);
    reset();
    planet.lpplprod = queue(3, 3, 3);
    planet.lpplprod->rgprod()[0] = planet.lpplprod->rgprod()[1] = row(1, 3, 1);
    script = 1;
    forcedBuilt = 0;
    Produce();
    CHECK(builds == 1 && traceIndex[0] == 2 && traceAlchemy[0] && planet.lpplprod->iprodMac == 1 && planet.lpplprod->rgprod()[0].iItem == 3);
    for (unsigned used = 1; used <= 255; used++) {
        reset();
        planet.lpplprod = queue(used, used, used);
        script = 1;
        forcedStatus = 5;
        partialCount = 1;
        Produce();
        CHECK(reallocations == 1 && planet.lpplprod->iprodMax == (used + 1) % 256 && planet.lpplprod->iprodMac == (used + 1) % 256);
        CHECK(planet.lpplprod->rgprod()[0].pct == 59 && planet.lpplprod->rgprod()[used].unused == 7);
    }
    reset();
    planet.lpplprod = queue(0, 255, 255);
    script = 1;
    forcedStatus = 5;
    partialCount = 1;
    int escaped = 0;
    try {
        Produce();
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 7 && reallocations == 0 && planet.lpplprod->iprodMac == 255);
    reset();
    planet.lpplprod = queue(0, 255, 256);
    script = 1;
    forcedStatus = 5;
    partialCount = 1;
    Produce();
    CHECK(!reallocations && planet.lpplprod->iprodMac == 0);
    reset();
    planet.lpplprod = queue(1, 1, 1);
    script = 1;
    forcedStatus = 5;
    partialCount = 1;
    failResize = 1;
    applied = 0;
    rgplr[0].pctResearch = 20;
    escaped = 0;
    try {
        Produce();
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 8 && applications == 1 && !frees && planet.lpplprod->iprodMac == 1 && planet.lpplprod->rgprod()[0].cItem == 0);
    CHECK(rgplr[0].lResLastYear == 20 && planet.rgwtMin[0] == 10 && !strcmp(phases, "MQBA"));
    reset();
    planet.lpplprod = queue(1, 1, 1);
    planet.lpplprod->rgprod()[0] = row(3, 1, 1);
    gross = 15;
    failResize = 1;
    applied = 0;
    rgplr[0].pctResearch = 20;
    escaped = 0;
    try {
        Produce();
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 8 && builds == 1 && applications == 1 && seenResources[3] == 0 && !frees);
    CHECK(planet.lpplprod->iprodMac == 1 && planet.lpplprod->rgprod()[0].cItem == 3 && rgplr[0].lResLastYear == 3);
    CHECK(planet.rgwtMin[0] == 10 && !strcmp(phases, "MQBA"));
    reset();
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    printf("M09 host production source: %d assertions passed\n", checks);
    return 0;
}
