#ifndef STARS_DECOMPILED_PRODUCE_H
#define STARS_DECOMPILED_PRODUCE_H

#include "common.h"
#include <cstdint>

void  InitProduction(APPSESSION &app, PROD *rgprod);
void  FinishProduction(APPSESSION &app, std::int16_t fWrite);
std::array<char, 360> PszNameProdItem(APPSESSION &app, PROD *lpprod);
void  GetProductionCosts(APPSESSION &app, PLANET *lppl, PROD *lpprod, std::uint32_t *rgCost, std::int16_t iplr, std::int16_t fOnlyOne);
void  EstimateItemProdSched(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, std::int16_t *piFirst, std::int16_t *piLast);

#endif
