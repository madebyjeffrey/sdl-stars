#ifndef STARS_DECOMPILED_AI4_H
#define STARS_DECOMPILED_AI4_H

#include "common.h"
#include <stdint.h>

extern CybertronRecipe vrgCyberIshAip[];
extern uint8_t         vrgCyberAip[];
extern uint8_t         vrgAiCybertronResOrder[];

void    DoCyberAiTurn(APPSESSION &app, PROD *rgprod);
void    DoCyberPackets(APPSESSION &app);
int16_t IdGetBestScannerDest(APPSESSION &app, PLANET *lppl, CompassDir iDir);
int16_t FAddPacketToQueue(APPSESSION &app, PLANET *lppl);
void    FillProductionQueue(APPSESSION &app);
int16_t FFillProdMinesAndFactories(APPSESSION &app, PLANET *lppl);
void    DoCyberFreighter(APPSESSION &app, FLEET *lpfl, uint8_t *lpciPlanTemp);
int16_t FEnumDropOffStage1(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumDropOffStage2(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumPickUp(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumNeedMinerals(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumPktAttack(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t FEnumCalcEnemyPlanets(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t iBuildCyberStarbase(APPSESSION &app, PLANET *lppl);
void    EnsureCyberAiShdefs(APPSESSION &app, int16_t iroCur);
int16_t iAddAttackFleet(APPSESSION &app, PLANET *lppl, int16_t iAttackStr, int16_t iBestDestroyer, int16_t iBestBattle, int16_t iBestSBDefender);
void    TargetCyberArmada(APPSESSION &app, FLEET *lpfl);

#endif
