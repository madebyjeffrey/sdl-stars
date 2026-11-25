#ifndef STARS_RESEARCHIO_H
#define STARS_RESEARCHIO_H
#include "common.h"

struct RESEARCHIO {
    decltype(&GetTechLevelCost)      pfnTechCost = GetTechLevelCost;
    decltype(&CResourcesAtPlanet)    pfnResources = CResourcesAtPlanet;
    decltype(&EstimateItemProdSched) pfnEstimate = EstimateItemProdSched;
};
const RESEARCHIO &ResearchIO() noexcept;
class RESEARCHIOSCOPE {
    RESEARCHIO        io;
    const RESEARCHIO *lpPrev;

  public:
    explicit RESEARCHIOSCOPE(const RESEARCHIO &io) noexcept;
    ~RESEARCHIOSCOPE() noexcept;
    RESEARCHIOSCOPE(const RESEARCHIOSCOPE &) = delete;
    RESEARCHIOSCOPE &operator=(const RESEARCHIOSCOPE &) = delete;
};
#endif
