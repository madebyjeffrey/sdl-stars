#include "appstate.h"

APPGAMESTATE::APPGAMESTATE() noexcept {
    game.mdSize = sizeMedium;
    game.mdDensity = densityNormal;
    game.cPlayer = 2;
    game.mdStartDist = startDistModerate;
    vrptBattle.grbitVisible = 65535;
    vrptBattle.irpt = rptBattles;
    vrptBattle.cFields = 15;
    vrptBattle.cFieldFirst = 1;
    vrptBattle.fAscending = 1;
    vrptBattle.ptDlg = POINT16{.x = -1, .y = -1};
    vrptBattle.ptSize = POINT16{.x = 600, .y = 400};
    vrptEFleet.grbitVisible = 65535;
    vrptEFleet.irpt = rptEnemyFleets;
    vrptEFleet.cFields = 12;
    vrptEFleet.cFieldFirst = 1;
    vrptEFleet.fAscending = 1;
    vrptEFleet.ptDlg = POINT16{.x = -1, .y = -1};
    vrptEFleet.ptSize = POINT16{.x = 600, .y = 400};
    vrptFleet.grbitVisible = 65535;
    vrptFleet.irpt = rptFleets;
    vrptFleet.cFields = 12;
    vrptFleet.cFieldFirst = 1;
    vrptFleet.fAscending = 1;
    vrptFleet.ptDlg = POINT16{.x = -1, .y = -1};
    vrptFleet.ptSize = POINT16{.x = 600, .y = 400};
    vrptPlanet.grbitVisible = 65535;
    vrptPlanet.cFields = 15;
    vrptPlanet.cFieldFirst = 1;
    vrptPlanet.fAscending = 1;
    vrptPlanet.ptDlg = POINT16{.x = -1, .y = -1};
    vrptPlanet.ptSize = POINT16{.x = 600, .y = 400};
}
