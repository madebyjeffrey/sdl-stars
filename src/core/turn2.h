#ifndef STARS_DECOMPILED_TURN2_H
#define STARS_DECOMPILED_TURN2_H

#include "common.h"
#include <cstdint>

void    Produce(APPSESSION &app);
std::int16_t CBuildProdItem(APPSESSION &app, PLANET *lppl, PROD *lpprod, PROD *pprodPartial, std::int32_t *rgRes, std::int16_t fAlchemy, std::int16_t *pmdStatus, std::int16_t fCalcOnly);
std::int16_t FBuildObject(APPSESSION &app, PLANET *lppl, GrobjClass grobj, std::int16_t iItem, std::int16_t cBuilt, std::int32_t *rgMinerals);
void    CreateShip(APPSESSION &app, std::int16_t iPlr, FLEET *lpfl, std::int16_t ishdef, std::int16_t cShip);
void    RandomEvents(APPSESSION &app);
void    TransferToOthers(APPSESSION &app);
void    DropColonists(APPSESSION &app);
void    HealShips(APPSESSION &app);
void    AutoTerraform(APPSESSION &app);
void    RemoteTerraforming(APPSESSION &app);
std::int16_t FQueueColonistDrop(APPSESSION &app, FLEET *lpfl, PLANET *lppl, std::int32_t cColonists);
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
std::int16_t FPacketDecay(APPSESSION &app, THING *lpth, std::int16_t pctRate);
void    ThingDecay(APPSESSION &app);
void    UnmarkMineFields(APPSESSION &app);
void    SweepForMines(APPSESSION &app);
void    BreedColonistsInTransit(APPSESSION &app);
void    UpdateResearchStatus(APPSESSION &app, std::int16_t fUsePool);
std::int16_t IBestRemoteTerra(APPSESSION &app, PLANET *lppl, std::int16_t iplr, std::int16_t fHelp);

#endif
