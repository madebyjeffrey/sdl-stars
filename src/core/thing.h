#ifndef STARS_DECOMPILED_THING_H
#define STARS_DECOMPILED_THING_H

#include "common.h"
#include <stdint.h>

THING  *LpthNew(APPSESSION &app, int16_t iplr, ThingType ith);
void    FreeLpth(APPSESSION &app, THING *lpth);
int16_t CPlanetsInCircle(APPSESSION &app, POINT16 pt, int32_t r2);
int16_t IValidateWormholePos(APPSESSION &app, THING *lpthWorm);
int16_t PctWormholeMoves(THING *lpth);
void    DoThingInteractions(APPSESSION &app, int16_t fPostMove);
int16_t IdmGiveTraderPart(APPSESSION &app, GrbitTrader grbitTrader, int16_t iplr, uint16_t *piGoto);

#endif
