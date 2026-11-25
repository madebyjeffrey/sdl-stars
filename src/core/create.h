#ifndef STARS_DECOMPILED_CREATE_H
#define STARS_DECOMPILED_CREATE_H

#include "common.h"
#include <cstdint>
#include <string_view>

extern std::int16_t vrgvcMax[10];
extern char    rgNG3Width[9][2];
extern std::uint8_t vrgWormholeMin[5];
extern BTLPLAN rgbtlplanT[];
extern PLAYER  vrgplrComp[6][4];
extern std::uint8_t vrgWormholeVar[5];

void    InitBattlePlan(APPSESSION &app, BTLPLAN *lpbtlplan, std::int16_t iplan, std::int16_t iplr);
std::int16_t GenerateWorld(APPSESSION &app, std::int16_t fBatchMode);
std::int16_t CreateStartupShip(APPSESSION &app, std::int16_t iplr, std::int16_t idPlanet, std::int16_t ishdef, std::int16_t fAddShdef);
std::int16_t GenNewGameFromFile(APPSESSION &app, std::string_view pszFile);
void    CreateTutorWorld(APPSESSION &app);
void    InitNewGamePlr(APPSESSION &app, std::int16_t iStepMaxSoFar, AiLevel lvlAi);
void    InitNewGame3();
PLAYER *LpplrComp(AiRace idAi, AiLevel lvlAi);
void    SetVCCheck(GAME *pgame, VictoryCondition vc, std::int16_t fChecked);
std::int16_t GetVCCheck(GAME *pgame, VictoryCondition vc);
std::int16_t SetVCVal(APPSESSION &app, GAME *pgame, VictoryCondition vc, std::int16_t val);
std::int16_t GetVCVal(APPSESSION &app, GAME *pgame, VictoryCondition vc, std::int16_t fRaw);

#endif
