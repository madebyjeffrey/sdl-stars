#ifndef STARS_DECOMPILED_CREATE_H
#define STARS_DECOMPILED_CREATE_H

#include <stdint.h>

extern int16_t vrgvcMax[10];
extern char    rgNG3Width[9][2];
extern uint8_t vrgWormholeMin[5];
extern BTLPLAN rgbtlplanT[];
extern PLAYER  vrgplrComp[6][4];
extern uint8_t vrgWormholeVar[5];

void    InitBattlePlan(BTLPLAN *lpbtlplan, int16_t iplan, int16_t iplr);
int16_t GenerateWorld(int16_t fBatchMode);
int16_t CreateStartupShip(int16_t iplr, int16_t idPlanet, int16_t ishdef, int16_t fAddShdef);
int16_t GenNewGameFromFile(char *pszFile);
void    CreateTutorWorld();
void    InitNewGamePlr(int16_t iStepMaxSoFar, AiLevel lvlAi);
void    InitNewGame3();
PLAYER *LpplrComp(AiRace idAi, AiLevel lvlAi);
void    SetVCCheck(GAME *pgame, VictoryCondition vc, int16_t fChecked);
int16_t GetVCCheck(GAME *pgame, VictoryCondition vc);
int16_t SetVCVal(GAME *pgame, VictoryCondition vc, int16_t val);
int16_t GetVCVal(GAME *pgame, VictoryCondition vc, int16_t fRaw);

#endif
