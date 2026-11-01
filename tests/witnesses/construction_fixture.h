#include "colony_io.h"
#include "appstate.h"
#include "common.h"
#include "heap_io.h"
#include "part_query_io.h"
#include "planet_query_io.h"
#include "production_io.h"
#include "random_io.h"
#include "research_io.h"
#include "turn_io.h"

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include "common.h"
#include "liststorage.h"

static SHDEF ships[16], bases[10];
static FLEET fleetBodies[512], *fleetSlots[512];
static union {
    max_align_t align;
    uint8_t     data[sizeof(PLORD) + 3 * sizeof(ORDER)];
} orderStorage[512];

static PLANET   planet;
static uint16_t extra[999];

static HULDEF catalogHull;
static struct {
    void  *p;
    size_t bytes;
    int    live;
} heap[32];
static int       heaps;
static int       checks, canBuild, catalogCalls, canCalls, lookups, fills, shipKills, packetKills, warpCalls;
static int16_t   cap, operableCap, removed, gross, warpValues[2], twoValues[2], terraformChoice, partIndex;
static int       terraformCalls, randomCalls, randomBounds[16], randomValues[16], messages, newFleets, routes, defaults;
static MessageId messageIds[64];
static int16_t   messageArgs[64][7], messageScanner[64];
static int       warpBase[2], resources[4];
static int32_t   quote[4];
struct CONSTRUCTIONESCAPE {};
static int slot(void *p) {
    for (int i = heaps - 1; i >= 0; i--)
        if (heap[i].live && heap[i].p == p)
            return i;
    abort();
}
void *TestLpAlloc(APPSESSION &, uint16_t size, HeapType type) {
    (void)type;
    assert(heaps < 32 && size);
    void *p = calloc(1, size);
    assert(p);
    heap[heaps].p = p;
    heap[heaps].bytes = size;
    heap[heaps++].live = 1;
    return p;
}
void *TestLpReAlloc(APPSESSION &, void *p, uint16_t size, HeapType type) {
    (void)type;
    int    at = slot(p);
    size_t old = heap[at].bytes;
    void  *next = realloc(p, size);
    assert(next);
    heap[at].live = 0;
    assert(heaps < 32);
    if (size > old)
        memset((char *)next + old, 0, size - old);
    heap[heaps].p = next;
    heap[heaps].bytes = size;
    heap[heaps++].live = 1;
    return next;
}
void TestFreeLp(APPSESSION &, void *p, HeapType type) {
    (void)type;
    int at = slot(p);
    heap[at].live = 0;
    free(p);
}
int16_t TestFSendPlrMsg(APPSESSION &, int16_t player, MessageId id, MsgGoto target, int16_t a, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g) {
    (void)target;
    assert(player >= 0 && player < 16 && messages < 64);
    messageIds[messages] = id;
    int16_t args[7] = {a, b, c, d, e, f, g};
    memcpy(messageArgs[messages], args, sizeof(args));
    messageScanner[messages] = planet.iScanner;
    messages++;
    return 1;
}
int16_t TestFSendPlrMsg2(APPSESSION &app, int16_t player, MessageId id, MsgGoto target, int16_t a, int16_t b) {
    return TestFSendPlrMsg(app, player, id, target, a, b, 0, 0, 0, 0, 0);
}
int16_t TestFRemovePlayerMessage(APPSESSION &, int16_t player, MessageId id, MsgGoto target) {
    assert(player == 0 && target == planet.id);
    (void)id;
    return removed;
}
int16_t TestCMaxFactories(APPSESSION &, PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    return cap;
}
int16_t TestCMaxMines(APPSESSION &app, PLANET *p, int16_t player) { return TestCMaxFactories(app, p, player); }
int16_t TestCMaxDefenses(APPSESSION &app, PLANET *p, int16_t player) { return TestCMaxFactories(app, p, player); }
int16_t TestCMaxOperableFactories(APPSESSION &, PLANET *p, int16_t player, int16_t next) {
    assert(next && p == &planet && !player);
    return operableCap;
}
int16_t TestCMaxOperableMines(APPSESSION &app, PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableFactories(app, p, player, next); }
int16_t TestCMaxOperableDefenses(APPSESSION &app, PLANET *p, int16_t player, int16_t next) { return TestCMaxOperableFactories(app, p, player, next); }
int16_t TestIpctCanTerraformLppl(APPSESSION &, PLANET *p) {
    assert(p == &planet);
    return 10;
}
int32_t TestChgPopFromPlanet(APPSESSION &, PLANET *p, int16_t update) {
    assert(p == &planet && !update);
    return -1;
}
int16_t TestPctPlanetDesirability(APPSESSION &, PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    return 100;
}
int16_t TestCResourcesAtPlanet(APPSESSION &, PLANET *p, int16_t player) {
    assert(p == &planet && !player);
    return gross;
}
void TestGetProductionCosts(APPSESSION &, PLANET *p, PROD *row, uint32_t *cost, int16_t player, int16_t one) {
    assert(p == &planet && !player && one);
    (void)row;
    memcpy(cost, quote, sizeof(quote));
}
void    TestMineMinerals(APPSESSION &) {}
void    TestUpdatePopulations(APPSESSION &) {}
void    TestUpdateResearchStatus(APPSESSION &, int16_t generate) { assert(generate); }
void    TestRandomEvents(APPSESSION &) {}
int16_t TestFCanBuildShdef(APPSESSION &, SHDEF *design, int16_t player) {
    assert(player == 0 && design);
    canCalls++;
    return (int16_t)canBuild;
}
HULDEF *TestLphuldefFromId(int16_t hull) {
    (void)hull;
    catalogCalls++;
    return &catalogHull;
}
int16_t TestIWarpMAFromLppl(APPSESSION &, PLANET *p, int16_t *two) {
    assert(p == &planet);
    int at = warpCalls < 2 ? warpCalls : 1;
    warpBase[at] = p->isb;
    warpCalls++;
    if (two)
        *two = twoValues[at];
    return warpValues[at];
}
int16_t TestFLookupPlanet(APPSESSION &, int16_t id, PLANET *out) {
    assert(id == planet.id);
    lookups++;
    *out = planet;
    return 1;
}
void   TestFillSelProdLB(APPSESSION &) { fills++; }
FLEET *TestLpflNew(APPSESSION &app, int16_t player, int16_t id) {
    assert(!player && id == planet.id);
    newFleets++;
    FLEET *f = &fleetBodies[3];
    f->iPlayer = player;
    f->idPlanet = id;
    f->lpplord = LpStartPl<PLORD, ORDER>(orderStorage[3].data, sizeof(orderStorage[3].data));
    app.state.rgplr[player].cFleet++;
    return f;
}
int32_t TestLGetFleetStat(APPSESSION &, FLEET *f, int16_t stat) {
    assert(f && stat == 1);
    return 12345;
}
void TestAutoRouteFleet(APPSESSION &, FLEET *f, PLANET *p) {
    assert(f && p == &planet);
    routes++;
    f->lpplord->rgord()[1].iWarp = 1;
}
void TestAutoFleetOrder(APPSESSION &, FLEET *f, PLANET *p) {
    assert(f && p == &planet);
    defaults++;
}
int16_t TestIBestTerraform(APPSESSION &, PLANET *p, int16_t help) {
    assert(p == &planet && help);
    terraformCalls++;
    if (terraformChoice < -3 || terraformChoice > 3)
        throw CONSTRUCTIONESCAPE{};
    return terraformChoice;
}
void TestLookupBestPlanetaryScanner(APPSESSION &app, PART *part) {
    assert(app.state.idPlayer == planet.iPlayer);
    part->hs.iItem = (uint8_t)partIndex;
}

int16_t TestRandom(APPSESSION &app, int16_t bound) {
    assert(randomCalls < 16);
    int16_t result = Random(app, bound);
    randomBounds[randomCalls] = bound;
    randomValues[randomCalls++] = result;
    return result;
}

void TestKillQueuedShips(APPSESSION &app, PLANET *p) {
    shipKills++;
    KillQueuedShips(app, p);
}
void TestKillQueuedMassPackets(APPSESSION &app, PLANET *p) {
    packetKills++;
    KillQueuedMassPackets(app, p);
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "construction witness %d: %s\n", checks, #x);                                                                                      \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
static uint16_t TestExtent(APPSESSION &, const void *lp) { return (uint16_t)heap[slot(const_cast<void *>(lp))].bytes; }
struct CONSTRUCTIONFIXTURE {
    HEAPIOSCOPE        scopeHeap;
    TURNIOSCOPE        scopeTurn;
    COLONYIOSCOPE      scopeColony;
    PLANETQUERYIOSCOPE scopePlanetQuery;
    RESEARCHIOSCOPE    scopeResearch;
    PRODUCTIONIOSCOPE  scopeProduction;
    PARTQUERYIOSCOPE   scopePartQuery;
    RANDOMIOSCOPE      scopeRandom;
    CONSTRUCTIONFIXTURE(APPSESSION &app)
        : scopeHeap([] {
              HEAPIO io;
              io.pfnAllocate = TestLpAlloc;
              io.pfnResize = TestLpReAlloc;
              io.pfnFree = TestFreeLp;
              io.pfnExtent = TestExtent;
              return io;
          }()),
          scopeTurn([] {
              TURNIO io;
              io.pfnMessage = TestFSendPlrMsg;
              io.pfnMessage2 = TestFSendPlrMsg2;
              io.pfnRemoveMessage = TestFRemovePlayerMessage;
              io.pfnMine = TestMineMinerals;
              io.pfnPopulation = TestUpdatePopulations;
              io.pfnResearch = TestUpdateResearchStatus;
              io.pfnEvents = TestRandomEvents;
              return io;
          }()),
          scopeColony([] {
              COLONYIO io;
              io.pfnMaxFactories = TestCMaxFactories;
              io.pfnMaxMines = TestCMaxMines;
              io.pfnMaxDefenses = TestCMaxDefenses;
              io.pfnOperableFactories = TestCMaxOperableFactories;
              io.pfnOperableMines = TestCMaxOperableMines;
              io.pfnOperableDefenses = TestCMaxOperableDefenses;
              io.pfnTerraformPercent = TestIpctCanTerraformLppl;
              io.pfnWarp = TestIWarpMAFromLppl;
              io.pfnBestTerraform = TestIBestTerraform;
              return io;
          }()),
          scopePlanetQuery([] {
              PLANETQUERYIO io;
              io.pfnGrowth = TestChgPopFromPlanet;
              io.pfnDesirability = TestPctPlanetDesirability;
              io.pfnLookup = TestFLookupPlanet;
              return io;
          }()),
          scopeResearch([] {
              RESEARCHIO io;
              io.pfnResources = TestCResourcesAtPlanet;
              return io;
          }()),
          scopeProduction([] {
              PRODUCTIONIO io;
              io.pfnRefresh = TestFillSelProdLB;
              io.pfnQuote = TestGetProductionCosts;
              io.pfnBuildable = TestFCanBuildShdef;
              io.pfnNewFleet = TestLpflNew;
              io.pfnFleetStat = TestLGetFleetStat;
              io.pfnRoute = TestAutoRouteFleet;
              io.pfnDefaultOrder = TestAutoFleetOrder;
              io.pfnScanner = TestLookupBestPlanetaryScanner;
              io.pfnKillShips = TestKillQueuedShips;
              io.pfnKillPackets = TestKillQueuedMassPackets;
              return io;
          }()),
          scopePartQuery([] {
              PARTQUERYIO io;
              io.pfnHull = TestLphuldefFromId;
              return io;
          }()),
          scopeRandom([] {
              RANDOMIO io;
              io.pfnRandom = TestRandom;
              return io;
          }()) {
        app.state.rglpfl = fleetSlots;
        app.state.lpPlanets = &planet;
        app.state.cPlanet = 1;
        app.state.vrgPlanResExtra = extra;
    }
};

[[maybe_unused]] static PROD row(unsigned count, unsigned item, unsigned kind, unsigned pct) {
    PROD r = {};
    r.cItem = count;
    r.iItem = item;
    r.grobj = kind;
    r.pct = pct;
    r.unused = 31;
    return r;
}
static void reset(APPSESSION &app) {
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    heaps = 0;
    memset(&app.state.game, 0, sizeof(app.state.game));
    memset(&app.state.gd, 0, sizeof(app.state.gd));
    memset(&app.state.sel, 0, sizeof(app.state.sel));
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(ships, 0, sizeof(ships));
    memset(bases, 0, sizeof(bases));
    memset(&planet, 0, sizeof(planet));
    memset(fleetBodies, 0, 4 * sizeof(fleetBodies[0]));
    memset(orderStorage, 0, 4 * sizeof(orderStorage[0]));
    for (int i = 0; i < 4; i++) {
        fleetSlots[i] = &fleetBodies[i];
        fleetBodies[i].lpplord = LpStartPl<PLORD, ORDER>(orderStorage[i].data, sizeof(orderStorage[i].data));
    }
    app.state.rglpshdef[0] = ships;
    app.state.rglpshdefSB[0] = bases;
    app.state.lpThings = NULL;
    app.state.cThing = app.state.cThingAlloc = app.state.cFleet = 0;
    canBuild = 1;
    catalogCalls = canCalls = lookups = fills = shipKills = packetKills = warpCalls = 0;
    cap = operableCap = 100;
    removed = 0;
    gross = 100;
    warpValues[0] = warpValues[1] = 10;
    twoValues[0] = twoValues[1] = 0;
    terraformChoice = 1;
    terraformCalls = randomCalls = messages = newFleets = routes = defaults = 0;
    partIndex = 0;
    app.state.idPlayer = 3;
    app.state.lRandSeed1 = 7;
    app.state.lRandSeed2 = 11;
    app.state.game.cPlayer = 3;
    app.state.game.fNoRandom = 1;
    app.state.gd.fGeneratingTurn = 1;
    planet.id = 5;
    planet.iPlayer = 0;
    planet.iScanner = 31;
    planet.idFling = 8;
    planet.iWarpFling = 6;
    planet.rgwtMin[0] = 10;
    planet.rgwtMin[1] = 20;
    planet.rgwtMin[2] = 30;
    planet.rgwtMin[3] = 1000;
    app.state.rgptPlan[5] = POINT16{10, 20};
    catalogHull.hul.wtCargoMax = 123;
    quote[0] = quote[1] = quote[2] = 0;
    quote[3] = 10;
    extra[5] = 0;
    for (int i = 0; i < 4; i++)
        resources[i] = 10 * (i + 1);
}
static int16_t call(APPSESSION &app, unsigned kind, int item, int built) { return FBuildObject(app, &planet, (GrobjClass)kind, (int16_t)item, (int16_t)built, (int32_t *)resources); }
