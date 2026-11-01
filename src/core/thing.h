#ifndef STARS_DECOMPILED_THING_H
#define STARS_DECOMPILED_THING_H

#include "common.h"
#include <cstdint>

THING  *LpthNew(APPSESSION &app, std::int16_t iplr, ThingType ith);
void    FreeLpth(APPSESSION &app, THING *lpth);
std::int16_t CPlanetsInCircle(APPSESSION &app, POINT16 pt, std::int32_t r2);
std::int16_t IValidateWormholePos(APPSESSION &app, THING *lpthWorm);
std::int16_t PctWormholeMoves(THING *lpth);
void    DoThingInteractions(APPSESSION &app, std::int16_t fPostMove);
std::int16_t IdmGiveTraderPart(APPSESSION &app, GrbitTrader grbitTrader, std::int16_t iplr, std::uint16_t *piGoto);

#endif
