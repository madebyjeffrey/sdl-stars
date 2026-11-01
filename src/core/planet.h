#ifndef STARS_DECOMPILED_PLANET_H
#define STARS_DECOMPILED_PLANET_H

#include "common.h"
#include <stdint.h>

int16_t FGetBestDefensePart(APPSESSION &app, PART *ppart);
char   *PszProductionETA(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *etaFirst, int16_t *etaLast);
void    ChangeMainObjSel(APPSESSION &app, GrobjClass grobjNew, int16_t iObjSel);
void    SelectAdjPlanet(APPSESSION &app, int16_t dInc, int16_t idPlanet);
int16_t IdFindAdjStarbase(APPSESSION &app, int16_t idPlanet, int16_t fNext);
int16_t IBestTerraform(APPSESSION &app, PLANET *lppl, int16_t fHelp);
char   *PszCalcEnvVar(APPSESSION &app, EnvType iEnv, int16_t iVar);
char   *PszCalcGravity(APPSESSION &app, int16_t iGravity);
int16_t PctPlanetCapacity(APPSESSION &app, PLANET *lppl);
int16_t PctPlanetOptValue(APPSESSION &app, PLANET *lppl, int16_t iPlr);
int16_t PctPlanetDesirability(APPSESSION &app, PLANET *lppl, int16_t iPlr);
int32_t CalcPlanetMaxPop(APPSESSION &app, int16_t idpl, int16_t iplr);
int16_t CMaxMines(APPSESSION &app, PLANET *lppl, int16_t iplr);
int16_t CMaxOperableMines(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear);
int16_t CMinesOperating(APPSESSION &app, PLANET *lppl);
int16_t CFactoriesOperating(APPSESSION &app, PLANET *lppl);
int16_t CMaxFactories(APPSESSION &app, PLANET *lppl, int16_t iplr);
int16_t CMaxOperableFactories(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear);
int16_t CMaxDefenses(APPSESSION &app, PLANET *lppl, int16_t iplr);
int16_t CMaxOperableDefenses(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear);
int16_t CResourcesAtPlanet(APPSESSION &app, PLANET *lppl, int16_t iplr);
int16_t IWarpMAFromLppl(APPSESSION &app, PLANET *lppl, int16_t *pfTwo);
int16_t StargateRangeFromLppl(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t ish);
int16_t FProdIsTerra(PROD *lpprod);
int16_t IpctCanTerraformLppl(APPSESSION &app, PLANET *lppl);
int16_t FCanTerraformLppl(APPSESSION &app, PLANET *lppl, int16_t *rgEnvMin, int16_t *rgEnvMax, int16_t *rgEnvCost, int16_t fHelp);
void    UninhabitPlanet(APPSESSION &app, PLANET *lppl);
int16_t PctCloakFromHuldef(APPSESSION &app, HUL *lphul, int16_t iplr, int16_t *ppctSteal);

int16_t FProdItemLine(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, int16_t iprod, int16_t fShowNever, char *szLine);
char   *PszProdQueueTop(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod);


#endif
