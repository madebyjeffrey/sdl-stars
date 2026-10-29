#ifndef STARS_DECOMPILED_TURN_H
#define STARS_DECOMPILED_TURN_H

#include "common.h"
#include <stdint.h>

extern int16_t rgpctMineHit[3];
extern int16_t rgiWarpSafe[3];
extern int16_t rgrgdmgMinMine[3][2];
extern int16_t rgrgdmgMine[3][2];

void    InitGameStuff();
int16_t FGenerateTurn();
void    EnsureAis();
void    DoOrders(int16_t fPostMovement);
void    MoveThings(int16_t fPostProd);
void    FuelFleets();
void    MoveFleets();
int16_t FTravelThroughMineFields(FLEET *lpfl, int16_t *pdTravel, THING *lpthHit);
void    VerifyTurns();
int16_t CTurnsOutSafe();
int16_t CFindTurnsOutstanding();
int16_t FSetUpBatchProcessing();
int16_t ParseCmdLine(char *lpCmdLine, int16_t *pfSeed, uint32_t *plSeed);
int16_t FRunCmdLine();

#endif
