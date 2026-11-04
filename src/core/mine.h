#ifndef STARS_DECOMPILED_MINE_H
#define STARS_DECOMPILED_MINE_H

#include "common.h"
#include <cstdint>
#include <span>

struct MINEFIELDCOUNTSRESULT {
    std::int16_t ithFound;
    std::int16_t cthTotal;
};

MINEFIELDCOUNTSRESULT GetMineFieldCounts(APPSESSION &app, std::uint16_t id);
std::int16_t FOtherStuffAtScanSel(APPSESSION &app);
void    EstMineralsMined(APPSESSION &app, PLANET *lppl, std::span<std::int32_t, 3> plQuan, std::int32_t cMines, std::int16_t fApply);

#endif
