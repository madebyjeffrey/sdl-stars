#ifndef STARS_DECOMPILED_RACE_H
#define STARS_DECOMPILED_RACE_H

#include "common.h"
#include <cstdint>
#include <string_view>

extern std::int16_t rgRacePrimaryTrait[10];
extern char    rgRW3Spacing[7];
extern std::int16_t rgRaceAdvDisPts[14];
extern char    rgRW3IStat[7];
extern std::int16_t rgRaceDisEnvPts[6];
extern char    rgRW3Width[7];
extern char    rgRaceStatMax[16];
extern char    rgRaceStatMin[16];

std::int16_t  GetRaceStat(PLAYER *pplr, RaceStat iStat);
std::int16_t  SetRaceStat(PLAYER *pplr, RaceStat iStat, std::int16_t iVal);
std::int16_t  GetRaceGrbit(PLAYER *pplr, RaceGrbit ibit);
void     SetRaceGrbit(PLAYER *pplr, RaceGrbit ibit, std::int16_t fSet);
void     BoundsCheckPlayer(PLAYER *pplr);
std::int16_t  CAdvantagePoints(APPSESSION &app, PLAYER *pplr);
std::int32_t  LInnateRaceHabitability(APPSESSION &app, PLAYER *pplr);
std::uint16_t IRaceChecksum(PLAYER *pplr);
void     CreateRandomRace(APPSESSION &app, PLAYER *pplr);
std::int16_t  PctTrueMaxGrowth(APPSESSION &app, std::int16_t iplr);
std::int16_t  FWasRaceFile(APPSESSION &app, std::string_view szFile, std::int16_t fChkPass);

#endif
