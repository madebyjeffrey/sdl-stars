#ifndef STARS_CONSTRUCTION_BUILDABILITY_FIXTURE_H
#define STARS_CONSTRUCTION_BUILDABILITY_FIXTURE_H
#include "catalog_fixture.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "construction_fixture.h"
#include "race_query_io.h"
static int  fInCheck, cQueries, cGateLookups, mdCheckResult, rgQueries[128], rgGateRequests[17][6];
static void Query(int tag) {
    if (fInCheck) {
        assert(cQueries < 128);
        rgQueries[cQueries++] = tag;
    }
}
static int16_t GateStat(PLAYER *p, RaceStat stat) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (fInCheck)
        assert(p == &app.state.rgplr[app.state.idPlayer] && stat == rsMajorAdv);
    Query(0);
    return GetRaceStat(p, stat);
}
static int16_t GateGrbit(PLAYER *p, RaceGrbit bit) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (fInCheck)
        assert(p == &app.state.rgplr[app.state.idPlayer]);
    Query(100 + bit);
    return GetRaceGrbit(p, bit);
}
static int16_t GateTech(APPSESSION &app, char *need) {
    Query(2);
    return TechStatus(app, need);
}
static int16_t GateHidden(APPSESSION &app, PART *part) {
    Query(1);
    return FShouldPartBeHidden(app, part);
}
static int16_t GateBuildable(APPSESSION &app, SHDEF *design, int16_t player) {
    int16_t saved = app.state.idPlayer;
    assert(design && player == planet.iPlayer);
    canCalls++;
    fInCheck = 1;
    int16_t value = FCanBuildShdef(app, design, player);
    fInCheck = 0;
    assert(app.state.idPlayer == saved);
    mdCheckResult = value;
    return value;
}
static HULDEF *GateHull(int16_t hull) {
    catalogCalls++;
    return LphuldefFromId(hull);
}
static int16_t GateLookup(APPSESSION &app, PART *part) {
    assert(fInCheck && cGateLookups < 17);
    int      at = cGateLookups;
    unsigned mask = part->hs.grhst, item = part->hs.iItem;
    rgGateRequests[at][0] = app.state.idPlayer;
    rgGateRequests[at][1] = mask;
    rgGateRequests[at][2] = item;
    rgGateRequests[at][3] = at == 0 ? -1 : part->hs.cItem;
    rgGateRequests[at][5] = cQueries;
    int16_t status = FLookupPart(app, part);
    rgGateRequests[at][4] = status;
    if (status != 0) {
        unsigned kind = 16;
        for (unsigned k = 0; k < 16; k++)
            if (mask == (UINT32_C(1) << k))
                kind = k;
        assert(kind < 16 && part->pcom() == LpcomRow(kind, item));
    }
    cGateLookups++;
    return status;
}
static FLEET *GateNewFleet(APPSESSION &app, int16_t player, int16_t id) {
    assert(player == planet.iPlayer && id == planet.id);
    newFleets++;
    FLEET *f = &fleetBodies[3];
    f->iPlayer = player;
    f->idPlanet = id;
    f->lpplord = LpStartPl<PLORD, ORDER>(orderStorage[3].data, sizeof(orderStorage[3].data));
    app.state.rgplr[player].cFleet++;
    return f;
}
struct CONSTRUCTIONBUILDFIXTURE {
    CONSTRUCTIONFIXTURE fixture;
    RACEQUERYIOSCOPE    raceScope;
    PARTQUERYIOSCOPE    partScope;
    PRODUCTIONIOSCOPE   productionScope;
    CONSTRUCTIONBUILDFIXTURE(APPSESSION &app)
        : fixture(app), raceScope([] {
              RACEQUERYIO io;
              io.pfnStat = GateStat;
              io.pfnGrbit = GateGrbit;
              return io;
          }()),
          partScope([] {
              PARTQUERYIO io = PartQueryIO();
              io.pfnLookup = GateLookup;
              io.pfnTechStatus = GateTech;
              io.pfnHidden = GateHidden;
              io.pfnHull = GateHull;
              return io;
          }()),
          productionScope([] {
              PRODUCTIONIO io = ProductionIO();
              io.pfnBuildable = GateBuildable;
              io.pfnNewFleet = GateNewFleet;
              return io;
          }()) {}
};
#endif
