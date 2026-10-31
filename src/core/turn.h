#ifndef STARS_DECOMPILED_TURN_H
#define STARS_DECOMPILED_TURN_H

#include "common.h"
#include <stdint.h>

extern int16_t rgpctMineHit[3];
extern int16_t rgiWarpSafe[3];
extern int16_t rgrgdmgMinMine[3][2];
extern int16_t rgrgdmgMine[3][2];

void    InitGameStuff(APPSESSION &app);
int16_t FGenerateTurn(APPSESSION &app);
void    EnsureAis(APPSESSION &app);
void    DoOrders(APPSESSION &app, int16_t fPostMovement);
void    MoveThings(APPSESSION &app, int16_t fPostProd);
void    FuelFleets(APPSESSION &app);
void    MoveFleets(APPSESSION &app);
int16_t FTravelThroughMineFields(APPSESSION &app, FLEET *lpfl, int16_t *pdTravel, THING *lpthHit);
void    VerifyTurns(APPSESSION &app);
int16_t CTurnsOutSafe(APPSESSION &app);
int16_t CFindTurnsOutstanding(APPSESSION &app);
int16_t FSetUpBatchProcessing(APPSESSION &app);
int16_t ParseCmdLine(APPSESSION &app, char *lpCmdLine, int16_t *pfSeed, uint32_t *plSeed);
int16_t FRunCmdLine(APPSESSION &app);

#endif
