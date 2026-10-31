#ifndef STARS_DECOMPILED_SHIP2_H
#define STARS_DECOMPILED_SHIP2_H

#include "common.h"
#include <stdint.h>

int16_t FStargateJump(APPSESSION &app, FLEET *lpfl, int16_t isbsSrc, int16_t isbsDst, int16_t dDist);
int16_t MdCalcStargateDamage(APPSESSION &app, int16_t isbsSrc, int16_t isbsDst, int16_t dDist, int16_t wt, int16_t *ppctDmg);
void    KillUsedWaypoints(APPSESSION &app);
void    NoAutoTrackFleet(APPSESSION &app, FLEET *lpflTarget);
void    AutoRouteFleet(APPSESSION &app, FLEET *lpfl, PLANET *lppl);
int16_t FColonizer(APPSESSION &app, FLEET *lpfl);
int16_t FScout(APPSESSION &app, FLEET *lpfl);
void    AutoFleetOrder(APPSESSION &app, FLEET *lpfl, PLANET *lppl);
int32_t CMineFromLpfl(APPSESSION &app, FLEET *lpfl);
int32_t PctTerraFromLpfl(APPSESSION &app, FLEET *lpfl);
int32_t CLayMinesFromLpfl(APPSESSION &app, FLEET *lpfl, MineFieldType iType, int16_t ishdef);
int32_t CMineSweepFromLpfl(APPSESSION &app, FLEET *lpfl);
int32_t CMineSweepFromLphul(APPSESSION &app, HUL *lphul);
int16_t PctCloakFromLpfl(APPSESSION &app, FLEET *lpfl);
int16_t CPtsCloakFromLphs(APPSESSION &app, HS *lphs);
void    MarkTechsSeen(APPSESSION &app, HUL *lphul, int16_t iplr);

#endif
