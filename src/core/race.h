#ifndef STARS_DECOMPILED_RACE_H
#define STARS_DECOMPILED_RACE_H

#include <stdint.h>

extern int16_t rgRacePrimaryTrait[10];
extern char    rgRW3Spacing[7];
extern int16_t rgRaceAdvDisPts[14];
extern char    rgRW3IStat[7];
extern int16_t rgRaceDisEnvPts[6];
extern char    rgRW3Width[7];
extern char    rgRaceStatMax[16];
extern char    rgRaceStatMin[16];

int16_t  GetRaceStat(PLAYER *pplr, RaceStat iStat);
int16_t  SetRaceStat(PLAYER *pplr, RaceStat iStat, int16_t iVal);
int16_t  GetRaceGrbit(PLAYER *pplr, RaceGrbit ibit);
void     SetRaceGrbit(PLAYER *pplr, RaceGrbit ibit, int16_t fSet);
void     BoundsCheckPlayer(PLAYER *pplr);
int16_t  CAdvantagePoints(PLAYER *pplr);
int32_t  LInnateRaceHabitability(PLAYER *pplr);
uint16_t IRaceChecksum(PLAYER *pplr);
void     CreateRandomRace(PLAYER *pplr);
int16_t  PctTrueMaxGrowth(int16_t iplr);
int16_t  FWasRaceFile(char *szFile, int16_t fChkPass);

#endif
