#ifndef STARS_CONSTRUCTION_BUILDABILITY_FIXTURE_H
#define STARS_CONSTRUCTION_BUILDABILITY_FIXTURE_H
#include "catalog_fixture.h"
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
    if (fInCheck)
        assert(p == &rgplr[idPlayer] && stat == rsMajorAdv);
    Query(0);
    return GetRaceStat(p, stat);
}
static int16_t GateGrbit(PLAYER *p, RaceGrbit bit) {
    if (fInCheck)
        assert(p == &rgplr[idPlayer]);
    Query(100 + bit);
    return GetRaceGrbit(p, bit);
}
static int16_t GateTech(char *need) {
    Query(2);
    return TechStatus(need);
}
static int16_t GateHidden(PART *part) {
    Query(1);
    return FShouldPartBeHidden(part);
}
static int16_t GateBuildable(SHDEF *design, int16_t player) {
    int16_t saved = idPlayer;
    assert(design && player == planet.iPlayer);
    canCalls++;
    fInCheck = 1;
    int16_t value = FCanBuildShdef(design, player);
    fInCheck = 0;
    assert(idPlayer == saved);
    mdCheckResult = value;
    return value;
}
static HULDEF *GateHull(int16_t hull) {
    catalogCalls++;
    return LphuldefFromId(hull);
}
static int16_t GateLookup(PART *part) {
    assert(fInCheck && cGateLookups < 17);
    int      at = cGateLookups;
    unsigned mask = part->hs.grhst, item = part->hs.iItem;
    rgGateRequests[at][0] = idPlayer;
    rgGateRequests[at][1] = mask;
    rgGateRequests[at][2] = item;
    rgGateRequests[at][3] = at == 0 ? -1 : part->hs.cItem;
    rgGateRequests[at][5] = cQueries;
    int16_t status = FLookupPart(part);
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
static FLEET *GateNewFleet(int16_t player, int16_t id) {
    assert(player == planet.iPlayer && id == planet.id);
    newFleets++;
    FLEET *f = &fleetBodies[3];
    f->iPlayer = player;
    f->idPlanet = id;
    f->lpplord = LpStartPl<PLORD, ORDER>(orderStorage[3].data, sizeof(orderStorage[3].data));
    rgplr[player].cFleet++;
    return f;
}
struct CONSTRUCTIONBUILDFIXTURE {
    CONSTRUCTIONFIXTURE fixture;
    RACEQUERYIOSCOPE    raceScope;
    PARTQUERYIOSCOPE    partScope;
    PRODUCTIONIOSCOPE   productionScope;
    CONSTRUCTIONBUILDFIXTURE()
        : raceScope([] {
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
