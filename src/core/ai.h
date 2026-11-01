#ifndef STARS_DECOMPILED_AI_H
#define STARS_DECOMPILED_AI_H

#include "common.h"
#include <stdint.h>

extern uint8_t        vrgAiRobotoidResOrder[];
extern uint8_t        vrgTDAip[];
extern uint8_t        vrgRobAip[];
extern uint8_t        vrgTDIshAip[];
extern uint8_t        vrgAiTurinDroneResOrder[];
extern RobotoidRecipe vrgRobIshAip[];

void    DoAiTurn(APPSESSION &app, int16_t iPlayer, uint16_t wMdPlr);
void    DoRobotoidAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureRobotoidShdefs(APPSESSION &app);
int16_t IdTargetArmada(APPSESSION &app, FLEET *lpfl);
int16_t FPotentRobWarFleet(APPSESSION &app, FLEET *lpfl, int16_t iPotency);
int16_t FEnumCalcEnemyFleets(APPSESSION &app, FLEET *lpflSrc, FLEET *lpflTest);
int16_t FEnumCalcArmadaDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumCalcArmadaHumanDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
void    DoTurinDroneAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureTurinDroneShdefs(APPSESSION &app, int16_t iroCur);
int16_t FEnumCalcMinerDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumCalcColonistDrop(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);

#endif
