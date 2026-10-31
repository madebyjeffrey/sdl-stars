#ifndef STARS_DECOMPILED_PRODUCE_H
#define STARS_DECOMPILED_PRODUCE_H

#include "common.h"
#include <stdint.h>

void  InitProduction(APPSESSION &app, PROD *rgprod);
void  FinishProduction(APPSESSION &app, int16_t fWrite);
char *PszNameProdItem(APPSESSION &app, PROD *lpprod);
void  GetProductionCosts(APPSESSION &app, PLANET *lppl, PROD *lpprod, uint32_t *rgCost, int16_t iplr, int16_t fOnlyOne);
void  EstimateItemProdSched(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *piFirst, int16_t *piLast);

#endif
