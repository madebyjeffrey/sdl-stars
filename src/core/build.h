#ifndef STARS_DECOMPILED_BUILD_H
#define STARS_DECOMPILED_BUILD_H

#include "common.h"
#include <stdint.h>

extern HullSlotType rggrbitPartsSB[8];
extern StringId     rgidsPartsSB[8];
extern HullSlotType rggrbitParts[13];
extern StringId     rgidsParts[13];
extern HullSlotType rghstCat[14];
extern StringId     rgidsCat[14];

int16_t PctJammerFromHul(APPSESSION &app, HUL *lphul);
SHDEF  *NthValidShdef(APPSESSION &app, int16_t n);
SHDEF  *NthValidEnemyShdef(APPSESSION &app, int16_t n);
void    KillQueuedMassPackets(APPSESSION &app, PLANET *lppl);
void    KillQueuedShips(APPSESSION &app, PLANET *lppl);

#endif
