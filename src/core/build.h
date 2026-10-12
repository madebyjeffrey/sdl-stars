#ifndef STARS_DECOMPILED_BUILD_H
#define STARS_DECOMPILED_BUILD_H

#include <stdint.h>

extern HullSlotType rggrbitPartsSB[8];
extern StringId     rgidsPartsSB[8];
extern HullSlotType rggrbitParts[13];
extern StringId     rgidsParts[13];
extern HullSlotType rghstCat[14];
extern StringId     rgidsCat[14];

int16_t PctJammerFromHul(HUL *lphul);
SHDEF  *NthValidShdef(int16_t n);
SHDEF  *NthValidEnemyShdef(int16_t n);
void    KillQueuedMassPackets(PLANET *lppl);
void    KillQueuedShips(PLANET *lppl);

#endif
