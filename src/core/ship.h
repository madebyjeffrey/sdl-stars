#ifndef STARS_DECOMPILED_SHIP_H
#define STARS_DECOMPILED_SHIP_H

#include <stdint.h>

int16_t FCanSplit(int32_t cBoat);
int16_t FCanSplitAll(int32_t cBoat);
int16_t FCanMerge(FLEET *pfl);
void    SelectAdjFleet(int16_t dInc, int16_t idFleet);
int32_t LGetFleetStat(FLEET *lpfl, int16_t grStat);
int16_t WtMaxShdefStat(SHDEF *lpshdef, int16_t grStat);
int16_t FEnumCalcJettison(void *lprt, RecordType rt, int16_t cb, PLANET *lppl, int16_t iFleet);
int32_t GetCargoFree(FLEET *lpfl);
int32_t GetFuelFree(FLEET *lpfl);
int32_t ChgCargo(GrobjClass grobj, int16_t id, MineralType iSupply, int32_t dChg, void *pobj);
int32_t XferSupply(MineralType iSupply, int32_t cQuan);
void    DeleteWpFar(FLEET *lpfl, int16_t iDel, int16_t fRecycle);
int32_t EstFuelUse(FLEET *lpfl, int16_t iOrd, int16_t iWarp, int32_t dTravel, int16_t fRangeOnly);
int16_t IFindIdealWarp(FLEET *lpfl, int16_t fIgnoreScoops);
int32_t LFuelUseToWaypoint(FLEET *lpfl, int16_t iwp, int16_t fMaxCargo);
void    FleetTransferCargoBalance(FLEET *pflNew1, FLEET *pflNew2);
void    DestroyAllIshdefSB(int16_t ishdefSB, int16_t iplr);
void    DestroyAllIshdef(int16_t ishdef, int16_t iplr);
void    RemoveIshdefFromAllQueues(int16_t ishdef, int16_t fSpaceDocks);
int16_t CshQueued(int16_t ishdef, int16_t *pfProgress, int16_t fSpaceDocks);
void    Merge2Fleets(FLEET *lpflDst, FLEET *lpflDel, int16_t fNoDelete);
void    FleetOrdersChangeTarget(FLEET *lpflOld);
void    GetTruePartCost(int16_t iPlayer, PART *ppart, uint16_t *rgCost);
int16_t IWarpBestForWaypoint(FLEET *lpfl, ORDER *lpord);
int16_t IWarpFastestForWaypoint(FLEET *lpfl, ORDER *lpord);

#endif
