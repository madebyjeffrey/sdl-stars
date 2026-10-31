#ifndef STARS_DECOMPILED_TURN2_H
#define STARS_DECOMPILED_TURN2_H

#include "common.h"
#include <stdint.h>

void    Produce(APPSESSION &app);
int16_t CBuildProdItem(APPSESSION &app, PLANET *lppl, PROD *lpprod, PROD *pprodPartial, int32_t *rgRes, int16_t fAlchemy, int16_t *pmdStatus, int16_t fCalcOnly);
int16_t FBuildObject(APPSESSION &app, PLANET *lppl, GrobjClass grobj, int16_t iItem, int16_t cBuilt, int32_t *rgMinerals);
void    CreateShip(APPSESSION &app, int16_t iPlr, FLEET *lpfl, int16_t ishdef, int16_t cShip);
void    RandomEvents(APPSESSION &app);
void    TransferToOthers(APPSESSION &app);
void    DropColonists(APPSESSION &app);
void    HealShips(APPSESSION &app);
void    AutoTerraform(APPSESSION &app);
void    RemoteTerraforming(APPSESSION &app);
int16_t FQueueColonistDrop(APPSESSION &app, FLEET *lpfl, PLANET *lppl, int32_t cColonists);
void    UpdatePopulations(APPSESSION &app);
void    UpdateGuesses(APPSESSION &app);
void    MineMinerals(APPSESSION &app);
void    MeteorStrike(APPSESSION &app);
void    TossNonAutoBuildItems(APPSESSION &app, PLANET *lppl);
void    PlanetaryClimateChange(APPSESSION &app);
void    DiscoverNewMinerals(APPSESSION &app);
void    MysteryTrader(APPSESSION &app);
void    UpdatePlayerScores(APPSESSION &app);
void    CreateBackupDir(APPSESSION &app);
int16_t FPacketDecay(APPSESSION &app, THING *lpth, int16_t pctRate);
void    ThingDecay(APPSESSION &app);
void    UnmarkMineFields(APPSESSION &app);
void    SweepForMines(APPSESSION &app);
void    BreedColonistsInTransit(APPSESSION &app);
void    UpdateResearchStatus(APPSESSION &app, int16_t fUsePool);
int16_t IBestRemoteTerra(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fHelp);

#endif
