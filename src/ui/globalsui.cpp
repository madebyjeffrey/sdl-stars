#include "uistate.h"

APPUISTATE::APPUISTATE() noexcept {
    rgtilePlanet[0].yTop = 1;
    rgtilePlanet[0].dyFull = 85;
    rgtilePlanet[0].grbit = tileBitmap;
    rgtilePlanet[0].pfn = DrawPlanShipBitmap;
    rgtilePlanet[0].fPopped = 1;
    rgtilePlanet[0].fMinTitle = 1;
    rgtilePlanet[0].idh = idhPlanetTile;
    rgtilePlanet[1].yTop = 6;
    rgtilePlanet[1].dyFull = 5;
    rgtilePlanet[1].grbit = tileMineralsOrCargo;
    rgtilePlanet[1].pfn = DrawPlanetMinSum;
    rgtilePlanet[1].id = 1;
    rgtilePlanet[1].fPopped = 1;
    rgtilePlanet[1].idh = idhMineralsOnHandTile;
    rgtilePlanet[2].yTop = 8;
    rgtilePlanet[2].dyFull = 6;
    rgtilePlanet[2].grbit = tilePlanetStats;
    rgtilePlanet[2].pfn = DrawPlanetStats;
    rgtilePlanet[2].id = 4;
    rgtilePlanet[2].fPopped = 1;
    rgtilePlanet[2].idh = idhStatusTile;
    rgtilePlanet[3].yTop = 6;
    rgtilePlanet[3].dyFull = 22;
    rgtilePlanet[3].grbit = tileShipList;
    rgtilePlanet[3].pfn = DrawPlanetShipList;
    rgtilePlanet[3].iCol = 1;
    rgtilePlanet[3].id = 5;
    rgtilePlanet[3].fPopped = 1;
    rgtilePlanet[3].fNullPtr = 1;
    rgtilePlanet[3].idh = idhFleetsInOrbitTile;
    rgtilePlanet[4].yTop = 10;
    rgtilePlanet[4].dyFull = 20;
    rgtilePlanet[4].grbit = tileProductionOrOrbit;
    rgtilePlanet[4].pfn = DrawPlanetProduction;
    rgtilePlanet[4].iCol = 1;
    rgtilePlanet[4].id = 6;
    rgtilePlanet[4].fPopped = 1;
    rgtilePlanet[4].idh = idhProductionTile;
    rgtilePlanet[5].yTop = 8;
    rgtilePlanet[5].dyFull = 15;
    rgtilePlanet[5].grbit = tileStarbaseOrWaypoint;
    rgtilePlanet[5].pfn = DrawPlanetStarbase;
    rgtilePlanet[5].iCol = 1;
    rgtilePlanet[5].id = 7;
    rgtilePlanet[5].fPopped = 1;
    rgtilePlanet[5].fNullPtr = 1;
    rgtilePlanet[5].fMinTitle = 1;
    rgtilePlanet[5].idh = idhStarbaseTile;
    rgtileShip[0].yTop = 1;
    rgtileShip[0].dyFull = 85;
    rgtileShip[0].grbit = tileBitmap;
    rgtileShip[0].pfn = DrawPlanShipBitmap;
    rgtileShip[0].fPopped = 1;
    rgtileShip[0].fMinTitle = 1;
    rgtileShip[0].idh = idhFleetTile;
    rgtileShip[1].yTop = 3;
    rgtileShip[1].dyFull = 5;
    rgtileShip[1].grbit = tileProductionOrOrbit;
    rgtileShip[1].pfn = DrawShipPlanet;
    rgtileShip[1].id = 5;
    rgtileShip[1].fPopped = 1;
    rgtileShip[1].fMinTitle = 1;
    rgtileShip[1].idh = idhLocationTile;
    rgtileShip[2].yTop = 11;
    rgtileShip[2].dyFull = 19;
    rgtileShip[2].grbit = tileFleetOrders;
    rgtileShip[2].pfn = DrawShipOrders;
    rgtileShip[2].id = 3;
    rgtileShip[2].fPopped = 1;
    rgtileShip[2].idh = idhFleetWaypointsTile;
    rgtileShip[3].yTop = 6;
    rgtileShip[3].dyFull = 12;
    rgtileShip[3].grbit = tileStarbaseOrWaypoint;
    rgtileShip[3].pfn = DrawShipWayPtOrders;
    rgtileShip[3].id = 4;
    rgtileShip[3].fPopped = 1;
    rgtileShip[3].idh = idhWaypointTaskTile;
    rgtileShip[4].yTop = 7;
    rgtileShip[4].dyFull = 14;
    rgtileShip[4].grbit = tileMineralsOrCargo;
    rgtileShip[4].pfn = DrawShipCargo;
    rgtileShip[4].iCol = 1;
    rgtileShip[4].id = 1;
    rgtileShip[4].fPopped = 1;
    rgtileShip[4].idh = idhFuelAndCargoTile;
    rgtileShip[5].yTop = 12;
    rgtileShip[5].dyFull = 16;
    rgtileShip[5].grbit = tileFleetComp;
    rgtileShip[5].pfn = DrawFleetComp;
    rgtileShip[5].iCol = 1;
    rgtileShip[5].id = 9;
    rgtileShip[5].fPopped = 1;
    rgtileShip[5].idh = idhFleetCompositionTile;
    rgtileShip[6].yTop = 6;
    rgtileShip[6].dyFull = 22;
    rgtileShip[6].grbit = tileShipList;
    rgtileShip[6].pfn = DrawPlanetShipList;
    rgtileShip[6].iCol = 1;
    rgtileShip[6].id = 8;
    rgtileShip[6].fPopped = 1;
    rgtileShip[6].idh = idhOtherFleetsHereTile;
}

APPUISESSION ui::app;

void BeginAppUiSession(APPSESSION &app) noexcept {
    auto &session = static_cast<APPUISESSION &>(app);
    if (session.phase == APPUISESSIONPHASE::closed) {
        session.BindFiles();
        session.phase = APPUISESSIONPHASE::ready;
    }
}
bool FBeginAppUiClose(APPSESSION &app) noexcept {
    auto &session = static_cast<APPUISESSION &>(app);
    if (session.phase != APPUISESSIONPHASE::ready) return false;
    session.phase = APPUISESSIONPHASE::closing;
    return true;
}
void CancelAppUiClose(APPSESSION &app) noexcept {
    auto &session = static_cast<APPUISESSION &>(app);
    if (session.phase == APPUISESSIONPHASE::closing) session.phase = APPUISESSIONPHASE::ready;
}
void EndAppUiClose(APPSESSION &app) noexcept {
    auto &session = static_cast<APPUISESSION &>(app);
    session.Close();
    session.state = APPGAMESTATE{};
    session.ui = APPUISTATE{};
    session.phase = APPUISESSIONPHASE::closed;
}
