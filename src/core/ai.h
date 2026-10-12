#ifndef STARS_DECOMPILED_AI_H
#define STARS_DECOMPILED_AI_H

#include <stdint.h>

extern uint8_t        vrgAiRobotoidResOrder[];
extern uint8_t        vrgTDAip[];
extern uint8_t        vrgRobAip[];
extern uint8_t        vrgTDIshAip[];
extern uint8_t        vrgAiTurinDroneResOrder[];
extern RobotoidRecipe vrgRobIshAip[];

void    DoAiTurn(int16_t iPlayer, uint16_t wMdPlr);
void    DoRobotoidAiTurn(PROD *rgprod);
void    EnsureRobotoidShdefs();
int16_t IdTargetArmada(FLEET *lpfl);
int16_t FPotentRobWarFleet(FLEET *lpfl, int16_t iPotency);
int16_t FEnumCalcEnemyFleets(FLEET *lpflSrc, FLEET *lpflTest);
int16_t FEnumCalcArmadaDest(PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumCalcArmadaHumanDest(PLANET *lpplSrc, PLANET *lpplTest);
void    DoTurinDroneAiTurn(PROD *rgprod);
void    EnsureTurinDroneShdefs(int16_t iroCur);
int16_t FEnumCalcMinerDest(PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumCalcColonistDrop(PLANET *lpplSrc, PLANET *lpplTest);

#endif
