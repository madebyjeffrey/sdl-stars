#ifndef STARS_DECOMPILED_PRODUCE_H
#define STARS_DECOMPILED_PRODUCE_H

#include <stdint.h>

void  InitProduction(PROD *rgprod);
void  FinishProduction(int16_t fWrite);
char *PszNameProdItem(PROD *lpprod);
void  GetProductionCosts(PLANET *lppl, PROD *lpprod, uint32_t *rgCost, int16_t iplr, int16_t fOnlyOne);
void  EstimateItemProdSched(PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *piFirst, int16_t *piLast);

#endif
