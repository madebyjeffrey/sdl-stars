#ifndef STARS_DECOMPILED_SHIP_H
#define STARS_DECOMPILED_SHIP_H

#include "common.h"
#include <cstdint>

std::int16_t FCanSplit(APPSESSION &app, std::int32_t cBoat);
std::int16_t FCanSplitAll(APPSESSION &app, std::int32_t cBoat);
std::int16_t FCanMerge(APPSESSION &app, FLEET *pfl);
void    SelectAdjFleet(APPSESSION &app, std::int16_t dInc, std::int16_t idFleet);
std::int32_t LGetFleetStat(APPSESSION &app, FLEET *lpfl, std::int16_t grStat);
std::int16_t WtMaxShdefStat(SHDEF *lpshdef, std::int16_t grStat);
std::int16_t FEnumCalcJettison(APPSESSION &app, void *lprt, RecordType rt, std::int16_t cb, PLANET *lppl, std::int16_t iFleet);
std::int32_t GetCargoFree(APPSESSION &app, FLEET *lpfl);
std::int32_t GetFuelFree(APPSESSION &app, FLEET *lpfl);
std::int32_t ChgCargo(APPSESSION &app, GrobjClass grobj, std::int16_t id, MineralType iSupply, std::int32_t dChg, void *pobj);
std::int32_t XferSupply(APPSESSION &app, MineralType iSupply, std::int32_t cQuan);
void    DeleteWpFar(FLEET *lpfl, std::int16_t iDel, std::int16_t fRecycle);
std::int32_t EstFuelUse(APPSESSION &app, FLEET *lpfl, std::int16_t iOrd, std::int16_t iWarp, std::int32_t dTravel, std::int16_t fRangeOnly);
std::int16_t IFindIdealWarp(APPSESSION &app, FLEET *lpfl, std::int16_t fIgnoreScoops);
std::int32_t LFuelUseToWaypoint(APPSESSION &app, FLEET *lpfl, std::int16_t iwp, std::int16_t fMaxCargo);
void    FleetTransferCargoBalance(APPSESSION &app, FLEET *pflNew1, FLEET *pflNew2);
void    DestroyAllIshdefSB(APPSESSION &app, std::int16_t ishdefSB, std::int16_t iplr);
void    DestroyAllIshdef(APPSESSION &app, std::int16_t ishdef, std::int16_t iplr);
void    RemoveIshdefFromAllQueues(APPSESSION &app, std::int16_t ishdef, std::int16_t fSpaceDocks);
std::int16_t CshQueued(APPSESSION &app, std::int16_t ishdef, std::int16_t *pfProgress, std::int16_t fSpaceDocks);
void    Merge2Fleets(APPSESSION &app, FLEET *lpflDst, FLEET *lpflDel, std::int16_t fNoDelete);
void    FleetOrdersChangeTarget(APPSESSION &app, FLEET *lpflOld);
void    GetTruePartCost(APPSESSION &app, std::int16_t iPlayer, PART *ppart, std::uint16_t *rgCost);
std::int16_t IWarpBestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord);
std::int16_t IWarpFastestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord);

#endif
