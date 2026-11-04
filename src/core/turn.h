#ifndef STARS_DECOMPILED_TURN_H
#define STARS_DECOMPILED_TURN_H

#include "common.h"
#include <cstdint>
#include <optional>
#include <string_view>

extern std::int16_t rgpctMineHit[3];
extern std::int16_t rgiWarpSafe[3];
extern std::int16_t rgrgdmgMinMine[3][2];
extern std::int16_t rgrgdmgMine[3][2];

void    InitGameStuff(APPSESSION &app);
std::int16_t FGenerateTurn(APPSESSION &app);
void    EnsureAis(APPSESSION &app);
void    DoOrders(APPSESSION &app, std::int16_t fPostMovement);
void    MoveThings(APPSESSION &app, std::int16_t fPostProd);
void    FuelFleets(APPSESSION &app);
void    MoveFleets(APPSESSION &app);
struct MINEFIELDTRAVELRESULT {
    bool fSafe;
    std::optional<std::int16_t> dTravel;
};
MINEFIELDTRAVELRESULT FTravelThroughMineFields(APPSESSION &app, FLEET *lpfl, std::optional<std::int16_t> dTravel, THING *lpthHit);
void    VerifyTurns(APPSESSION &app);
std::int16_t CTurnsOutSafe(APPSESSION &app);
std::int16_t CFindTurnsOutstanding(APPSESSION &app);
std::int16_t FSetUpBatchProcessing(APPSESSION &app);
struct CMDLINERESULT {
    bool fValid;
    std::optional<std::uint32_t> lSeed;
};
CMDLINERESULT ParseCmdLine(APPSESSION &app, std::string_view szCmdLine);
std::int16_t FRunCmdLine(APPSESSION &app);

#endif
