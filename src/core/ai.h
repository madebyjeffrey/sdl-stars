#ifndef STARS_DECOMPILED_AI_H
#define STARS_DECOMPILED_AI_H

#include "common.h"
#include <cstdint>

extern std::uint8_t        vrgAiRobotoidResOrder[];
extern std::uint8_t        vrgTDAip[];
extern std::uint8_t        vrgRobAip[];
extern std::uint8_t        vrgTDIshAip[];
extern std::uint8_t        vrgAiTurinDroneResOrder[];
extern RobotoidRecipe vrgRobIshAip[];

void    DoAiTurn(APPSESSION &app, std::int16_t iPlayer, std::uint16_t wMdPlr);
void    DoRobotoidAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureRobotoidShdefs(APPSESSION &app);
std::int16_t IdTargetArmada(APPSESSION &app, FLEET *lpfl);
std::int16_t FPotentRobWarFleet(APPSESSION &app, FLEET *lpfl, std::int16_t iPotency);
std::int16_t FEnumCalcEnemyFleets(APPSESSION &app, FLEET *lpflSrc, FLEET *lpflTest);
std::int16_t FEnumCalcArmadaDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumCalcArmadaHumanDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
void    DoTurinDroneAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureTurinDroneShdefs(APPSESSION &app, std::int16_t iroCur);
std::int16_t FEnumCalcMinerDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t FEnumCalcColonistDrop(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);

#endif
