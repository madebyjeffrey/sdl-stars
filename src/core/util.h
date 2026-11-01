#ifndef STARS_DECOMPILED_UTIL_H
#define STARS_DECOMPILED_UTIL_H

#include "common.h"
#include <stdint.h>

extern uint32_t rgcrDrawStars2b[5];
extern uint32_t rgcrDrawStars2a[5];
extern uint32_t rgcrDrawStars[5];
extern int32_t  rgDSDivCnt2[5];
extern int32_t  rgDSDivCnt[5];
extern uint8_t  vrgbTachyon[18];

int16_t  FLookupSelPlanet(APPSESSION &app, PLANET *ppl);
int16_t  FDupPlanet(APPSESSION &app, PLANET *lppl, PLANET *ppl);
THING   *LpthFromId(APPSESSION &app, int16_t idth);
PLANET  *LpplFromId(APPSESSION &app, int16_t idPlanet);
void     CalcPctSurvive(APPSESSION &app, PLANET *lppl, float *ppct, float *ppctSmart);
int16_t  FLookupPlanet(APPSESSION &app, int16_t iPlanet, PLANET *ppl);
int32_t  DpOfLpflIshdef(APPSESSION &app, FLEET *lpfl, int16_t ishdef);
int16_t  FLookupThing(APPSESSION &app, int16_t idth, THING *pth);
int32_t  LComputePower(APPSESSION &app, SHDEF *lpshdef);
void     ComputeShdefPowers(APPSESSION &app);
int32_t  DpShieldOfShdef(APPSESSION &app, SHDEF *lpshdef, int16_t iplr);
int16_t  IStargateFromLppl(APPSESSION &app, PLANET *lppl);
char    *PszPlayerName(APPSESSION &app, int16_t iPlayer, int16_t fCapital, int16_t fPlural, int16_t fThe, int16_t grWord, PLAYER *pplr);
int16_t  FCalcFleetBombDamage(APPSESSION &app, FLEET *lpfl, int32_t *pdmgPeople, int32_t *pdmgPeopleMin, int32_t *pdmgPeopleSmart, int32_t *pdmgBldg, int32_t *ppctTerra,
                              int16_t *pfMulti);
void     LinkFleets(APPSESSION &app, int16_t fUnused);
int      ICompFleetPoint(FLEET **ppfl1, FLEET **ppfl2);
int      ICompFleetPoint2(int32_t *pl, FLEET **ppfl);
int16_t  FLookupSelShip(APPSESSION &app, FLEET *pfl);
FLEET   *LpflFromId(APPSESSION &app, int16_t idFleet);
int16_t  FLookupFleet(APPSESSION &app, int16_t idFleet, FLEET *pfl);
int16_t  FDupFleet(APPSESSION &app, FLEET *lpfl, FLEET *pfl);
int16_t  FLookupObject(APPSESSION &app, GrobjClass grobj, int16_t id, void *pobj);
int16_t  FLookupOrbitingXfer(APPSESSION &app, int16_t idPlanet, int16_t iNth, XFER *pxf, int16_t idSkip);
char    *PszGetThingName(APPSESSION &app, int16_t id);
char    *PszGetFleetName(APPSESSION &app, int16_t id);
uint16_t WFromLpfl(APPSESSION &app, FLEET *lpfl);
char    *PszFleetNameFromWord(APPSESSION &app, uint16_t w);
char    *PszGetPlanetName(APPSESSION &app, int16_t id);
int16_t  IflFromLpfl(APPSESSION &app, FLEET *lpfl);
int16_t  FDeleteFleet(APPSESSION &app, int16_t idFleet, GrobjClass grobjSel, int16_t idSel);
FLEET   *LpflNew(APPSESSION &app, int16_t iPlr, int16_t idPl);
FLEET   *LpflNewSplit(APPSESSION &app, FLEET *pfl);
int16_t  FFleetMergeAll(APPSESSION &app, FLEET *pfl);
int16_t  FFleetSplitAll(APPSESSION &app, FLEET *pfl);
char    *PszGetLocName(APPSESSION &app, GrobjClass grobj, int16_t id, int16_t x, int16_t y);
int16_t  IshdefPrimaryFromLpfl(APPSESSION &app, FLEET *lpfl, int16_t *pcDiff);
char    *PszGetDistance(APPSESSION &app, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
double   DGetDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
int16_t  FFindNearestObject(APPSESSION &app, POINT16 pt, GrobjClass grobj, SCAN *pscan);
void     UpdateShdefCost(APPSESSION &app, SHDEF *lpshdef);
uint16_t WPackLong(int32_t l);
int16_t  GetPlanetScannerRange(APPSESSION &app, PLANET *lppl, int16_t *pDeep);
int16_t  GetCachedFleetScannerRange(APPSESSION &app, FLEET *lpfl, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal);
int16_t  GetFleetScannerRange(APPSESSION &app, FLEET *lpfl, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal);
int16_t  GetShdefScannerRange(APPSESSION &app, SHDEF *lpshdef, int16_t iplr, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal);
int32_t  LCalcFuelGainFromRamScoops(APPSESSION &app, FLEET *lpfl, int16_t iWarp, int32_t dTravel);
int32_t  CalcPlayerScore(APPSESSION &app, int16_t iPlr, SCORE *pscore);
void     GetTrueHullCost(int16_t iPlayer, HUL *lphul, uint16_t *rgCost);
void     DecorateHullName(APPSESSION &app, int16_t iplr, int16_t ish, char *psz);
int16_t  FMatchTarget(APPSESSION &app, FLEET *lpflTarget, MdTarget mdTarget, int16_t fExact);
void     ValidateWaypoints(APPSESSION &app);
int32_t  ChgPopFromPlanet(APPSESSION &app, PLANET *lppl, int16_t fUpdate);
int16_t  FCanFleetUseStargates(APPSESSION &app, FLEET *lpfl, POINT16 ptSrc, POINT16 ptDst);
int16_t  FFleetCanJumpgate(APPSESSION &app, FLEET *lpfl);
int32_t  WtFromLpfl(APPSESSION &app, FLEET *lpfl);
int16_t  FCanBuildShdef(APPSESSION &app, SHDEF *lpshdef, int16_t iplr);
int16_t  IshFindSimilarDesign(APPSESSION &app, HUL *lphul, int16_t iPlrDst);
void     ClearFile(APPSESSION &app, int16_t dt);
void     OutputSz(APPSESSION &app, int16_t dt, const char *sz);
void     TurnLog(APPSESSION &app, StringId ids);
int16_t  PtToScan(APPSESSION &app, int16_t d);
int16_t  ScanToPt(APPSESSION &app, int16_t d);
char    *SzVersion(APPSESSION &app);
void     ChangeScanSel(APPSESSION &app, SCAN *pscan, int16_t fValidScan);
int16_t  FFindSomethingAndSelectIt(APPSESSION &app);
int16_t  CchGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, char *sz, int16_t iwp, int16_t fSmall);

#endif
