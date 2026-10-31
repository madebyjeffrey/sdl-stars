#ifndef STARS_PRODUCTIONIO_H
#define STARS_PRODUCTIONIO_H
#include "common.h"

struct PRODUCTIONIO {
    decltype(&InitProduction)             pfnInitialize = InitProduction;
    decltype(&FillSelProdLB)              pfnRefresh = FillSelProdLB;
    decltype(&GetProductionCosts)         pfnQuote = GetProductionCosts;
    decltype(&FCanBuildShdef)             pfnBuildable = FCanBuildShdef;
    decltype(&CBuildProdItem)             pfnBuild = CBuildProdItem;
    decltype(&FBuildObject)               pfnApply = FBuildObject;
    decltype(&KillQueuedShips)            pfnKillShips = KillQueuedShips;
    decltype(&KillQueuedMassPackets)      pfnKillPackets = KillQueuedMassPackets;
    decltype(&LookupBestPlanetaryScanner) pfnScanner = LookupBestPlanetaryScanner;
    decltype(&LpflNew)                    pfnNewFleet = LpflNew;
    decltype(&LGetFleetStat)              pfnFleetStat = LGetFleetStat;
    decltype(&AutoRouteFleet)             pfnRoute = AutoRouteFleet;
    decltype(&AutoFleetOrder)             pfnDefaultOrder = AutoFleetOrder;
};
const PRODUCTIONIO &ProductionIO() noexcept;
class PRODUCTIONIOSCOPE {
    PRODUCTIONIO        io;
    const PRODUCTIONIO *lpPrev;

  public:
    explicit PRODUCTIONIOSCOPE(const PRODUCTIONIO &io) noexcept;
    ~PRODUCTIONIOSCOPE() noexcept;
    PRODUCTIONIOSCOPE(const PRODUCTIONIOSCOPE &) = delete;
    PRODUCTIONIOSCOPE &operator=(const PRODUCTIONIOSCOPE &) = delete;
};
#endif
