#ifndef STARS_DECOMPILED_SHIP_H
#define STARS_DECOMPILED_SHIP_H

#include "common.h"
#include <stdint.h>

int16_t FCanSplit(APPSESSION &app, int32_t cBoat);
int16_t FCanSplitAll(APPSESSION &app, int32_t cBoat);
int16_t FCanMerge(APPSESSION &app, FLEET *pfl);
void    SelectAdjFleet(APPSESSION &app, int16_t dInc, int16_t idFleet);
int32_t LGetFleetStat(APPSESSION &app, FLEET *lpfl, int16_t grStat);
int16_t WtMaxShdefStat(SHDEF *lpshdef, int16_t grStat);
int16_t FEnumCalcJettison(APPSESSION &app, void *lprt, RecordType rt, int16_t cb, PLANET *lppl, int16_t iFleet);
int32_t GetCargoFree(APPSESSION &app, FLEET *lpfl);
int32_t GetFuelFree(APPSESSION &app, FLEET *lpfl);
int32_t ChgCargo(APPSESSION &app, GrobjClass grobj, int16_t id, MineralType iSupply, int32_t dChg, void *pobj);
int32_t XferSupply(APPSESSION &app, MineralType iSupply, int32_t cQuan);
void    DeleteWpFar(FLEET *lpfl, int16_t iDel, int16_t fRecycle);
int32_t EstFuelUse(APPSESSION &app, FLEET *lpfl, int16_t iOrd, int16_t iWarp, int32_t dTravel, int16_t fRangeOnly);
int16_t IFindIdealWarp(APPSESSION &app, FLEET *lpfl, int16_t fIgnoreScoops);
int32_t LFuelUseToWaypoint(APPSESSION &app, FLEET *lpfl, int16_t iwp, int16_t fMaxCargo);
void    FleetTransferCargoBalance(APPSESSION &app, FLEET *pflNew1, FLEET *pflNew2);
void    DestroyAllIshdefSB(APPSESSION &app, int16_t ishdefSB, int16_t iplr);
void    DestroyAllIshdef(APPSESSION &app, int16_t ishdef, int16_t iplr);
void    RemoveIshdefFromAllQueues(APPSESSION &app, int16_t ishdef, int16_t fSpaceDocks);
int16_t CshQueued(APPSESSION &app, int16_t ishdef, int16_t *pfProgress, int16_t fSpaceDocks);
void    Merge2Fleets(APPSESSION &app, FLEET *lpflDst, FLEET *lpflDel, int16_t fNoDelete);
void    FleetOrdersChangeTarget(APPSESSION &app, FLEET *lpflOld);
void    GetTruePartCost(APPSESSION &app, int16_t iPlayer, PART *ppart, uint16_t *rgCost);
int16_t IWarpBestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord);
int16_t IWarpFastestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord);

#endif
