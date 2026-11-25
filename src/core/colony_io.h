#ifndef STARS_COLONYIO_H
#define STARS_COLONYIO_H
#include "common.h"

struct COLONYIO {
    decltype(&FCanTerraformLppl)     pfnCanTerraform = FCanTerraformLppl;
    decltype(&CMaxMines)             pfnMaxMines = CMaxMines;
    decltype(&CMaxFactories)         pfnMaxFactories = CMaxFactories;
    decltype(&CMaxDefenses)          pfnMaxDefenses = CMaxDefenses;
    decltype(&CMaxOperableMines)     pfnOperableMines = CMaxOperableMines;
    decltype(&CMaxOperableFactories) pfnOperableFactories = CMaxOperableFactories;
    decltype(&CMaxOperableDefenses)  pfnOperableDefenses = CMaxOperableDefenses;
    decltype(&IpctCanTerraformLppl)  pfnTerraformPercent = IpctCanTerraformLppl;
    decltype(&IWarpMAFromLppl)       pfnWarp = IWarpMAFromLppl;
    decltype(&IBestTerraform)        pfnBestTerraform = IBestTerraform;
    decltype(&CMinesOperating)       pfnMinesOperating = CMinesOperating;
    decltype(&CMineFromLpfl)         pfnFleetMining = CMineFromLpfl;
    decltype(&EstMineralsMined)      pfnEstimateMining = EstMineralsMined;
};
const COLONYIO &ColonyIO() noexcept;
class COLONYIOSCOPE {
    COLONYIO        io;
    const COLONYIO *lpPrev;

  public:
    explicit COLONYIOSCOPE(const COLONYIO &io) noexcept;
    ~COLONYIOSCOPE() noexcept;
    COLONYIOSCOPE(const COLONYIOSCOPE &) = delete;
    COLONYIOSCOPE &operator=(const COLONYIOSCOPE &) = delete;
};
#endif
