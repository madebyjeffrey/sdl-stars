#ifndef STARS_DECOMPILED_MINE_H
#define STARS_DECOMPILED_MINE_H

#include "common.h"
#include <cstdint>

void    GetMineFieldCounts(APPSESSION &app, std::uint16_t id, std::int16_t *pithm, std::int16_t *pcthm);
std::int16_t FOtherStuffAtScanSel(APPSESSION &app);
void    EstMineralsMined(APPSESSION &app, PLANET *lppl, std::int32_t *plQuan, std::int32_t cMines, std::int16_t fApply);

#endif
