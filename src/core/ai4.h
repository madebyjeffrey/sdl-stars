#ifndef STARS_DECOMPILED_AI4_H
#define STARS_DECOMPILED_AI4_H

#include "common.h"
#include <cstdint>

extern CybertronRecipe vrgCyberIshAip[];
extern std::uint8_t         vrgCyberAip[];
extern std::uint8_t         vrgAiCybertronResOrder[];

void    DoCyberAiTurn(APPSESSION &app, PROD *rgprod);
void    DoCyberPackets(APPSESSION &app);
std::int16_t IdGetBestScannerDest(APPSESSION &app, PLANET *lppl, CompassDir iDir);
std::int16_t FAddPacketToQueue(APPSESSION &app, PLANET *lppl);
void    FillProductionQueue(APPSESSION &app);
std::int16_t FFillProdMinesAndFactories(APPSESSION &app, PLANET *lppl);
void    DoCyberFreighter(APPSESSION &app, FLEET *lpfl, std::uint8_t *lpciPlanTemp);
std::int16_t FEnumDropOffStage1(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumDropOffStage2(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumPickUp(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumNeedMinerals(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumPktAttack(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumCalcEnemyPlanets(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t iBuildCyberStarbase(APPSESSION &app, PLANET *lppl);
void    EnsureCyberAiShdefs(APPSESSION &app, std::int16_t iroCur);
std::int16_t iAddAttackFleet(APPSESSION &app, PLANET *lppl, std::int16_t iAttackStr, std::int16_t iBestDestroyer, std::int16_t iBestBattle, std::int16_t iBestSBDefender);
void    TargetCyberArmada(APPSESSION &app, FLEET *lpfl);

#endif
