#ifndef STARS_DECOMPILED_MINE_H
#define STARS_DECOMPILED_MINE_H

#include "common.h"
#include <stdint.h>

void    GetMineFieldCounts(APPSESSION &app, uint16_t id, int16_t *pithm, int16_t *pcthm);
int16_t FOtherStuffAtScanSel(APPSESSION &app);
void    EstMineralsMined(APPSESSION &app, PLANET *lppl, int32_t *plQuan, int32_t cMines, int16_t fApply);

#endif
