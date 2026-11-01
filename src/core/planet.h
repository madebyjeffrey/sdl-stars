#ifndef STARS_DECOMPILED_PLANET_H
#define STARS_DECOMPILED_PLANET_H

#include "common.h"
#include <cstdint>

std::int16_t FGetBestDefensePart(APPSESSION &app, PART *ppart);
std::array<char, 360> PszProductionETA(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, std::int16_t *etaFirst, std::int16_t *etaLast);
void    ChangeMainObjSel(APPSESSION &app, GrobjClass grobjNew, std::int16_t iObjSel);
void    SelectAdjPlanet(APPSESSION &app, std::int16_t dInc, std::int16_t idPlanet);
std::int16_t IdFindAdjStarbase(APPSESSION &app, std::int16_t idPlanet, std::int16_t fNext);
std::int16_t IBestTerraform(APPSESSION &app, PLANET *lppl, std::int16_t fHelp);
std::array<char, 360> PszCalcEnvVar(APPSESSION &app, EnvType iEnv, std::int16_t iVar);
std::array<char, 360> PszCalcGravity(APPSESSION &app, std::int16_t iGravity);
std::int16_t PctPlanetCapacity(APPSESSION &app, PLANET *lppl);
std::int16_t PctPlanetOptValue(APPSESSION &app, PLANET *lppl, std::int16_t iPlr);
std::int16_t PctPlanetDesirability(APPSESSION &app, PLANET *lppl, std::int16_t iPlr);
std::int32_t CalcPlanetMaxPop(APPSESSION &app, std::int16_t idpl, std::int16_t iplr);
std::int16_t CMaxMines(APPSESSION &app, PLANET *lppl, std::int16_t iplr);
std::int16_t CMaxOperableMines(APPSESSION &app, PLANET *lppl, std::int16_t iplr, std::int16_t fNextYear);
std::int16_t CMinesOperating(APPSESSION &app, PLANET *lppl);
std::int16_t CFactoriesOperating(APPSESSION &app, PLANET *lppl);
std::int16_t CMaxFactories(APPSESSION &app, PLANET *lppl, std::int16_t iplr);
std::int16_t CMaxOperableFactories(APPSESSION &app, PLANET *lppl, std::int16_t iplr, std::int16_t fNextYear);
std::int16_t CMaxDefenses(APPSESSION &app, PLANET *lppl, std::int16_t iplr);
std::int16_t CMaxOperableDefenses(APPSESSION &app, PLANET *lppl, std::int16_t iplr, std::int16_t fNextYear);
std::int16_t CResourcesAtPlanet(APPSESSION &app, PLANET *lppl, std::int16_t iplr);
std::int16_t IWarpMAFromLppl(APPSESSION &app, PLANET *lppl, std::int16_t *pfTwo);
std::int16_t StargateRangeFromLppl(APPSESSION &app, PLANET *lppl, std::int16_t iplr, std::int16_t ish);
std::int16_t FProdIsTerra(PROD *lpprod);
std::int16_t IpctCanTerraformLppl(APPSESSION &app, PLANET *lppl);
std::int16_t FCanTerraformLppl(APPSESSION &app, PLANET *lppl, std::int16_t *rgEnvMin, std::int16_t *rgEnvMax, std::int16_t *rgEnvCost, std::int16_t fHelp);
void    UninhabitPlanet(APPSESSION &app, PLANET *lppl);
std::int16_t PctCloakFromHuldef(APPSESSION &app, HUL *lphul, std::int16_t iplr, std::int16_t *ppctSteal);

std::int16_t FProdItemLine(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, std::int16_t iprod, std::int16_t fShowNever, std::span<char> szLine);
std::array<char, 360> PszProdQueueTop(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod);


#endif
