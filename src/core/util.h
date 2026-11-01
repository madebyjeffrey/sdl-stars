#ifndef STARS_DECOMPILED_UTIL_H
#define STARS_DECOMPILED_UTIL_H

#include "common.h"
#include <cstdint>
#include <array>

extern std::uint32_t rgcrDrawStars2b[5];
extern std::uint32_t rgcrDrawStars2a[5];
extern std::uint32_t rgcrDrawStars[5];
extern std::int32_t  rgDSDivCnt2[5];
extern std::int32_t  rgDSDivCnt[5];
extern std::uint8_t  vrgbTachyon[18];

std::int16_t  FLookupSelPlanet(APPSESSION &app, PLANET *ppl);
std::int16_t  FDupPlanet(APPSESSION &app, PLANET *lppl, PLANET *ppl);
THING   *LpthFromId(APPSESSION &app, std::int16_t idth);
PLANET  *LpplFromId(APPSESSION &app, std::int16_t idPlanet);
void     CalcPctSurvive(APPSESSION &app, PLANET *lppl, float *ppct, float *ppctSmart);
std::int16_t  FLookupPlanet(APPSESSION &app, std::int16_t iPlanet, PLANET *ppl);
std::int32_t  DpOfLpflIshdef(APPSESSION &app, FLEET *lpfl, std::int16_t ishdef);
std::int16_t  FLookupThing(APPSESSION &app, std::int16_t idth, THING *pth);
std::int32_t  LComputePower(APPSESSION &app, SHDEF *lpshdef);
void     ComputeShdefPowers(APPSESSION &app);
std::int32_t  DpShieldOfShdef(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr);
std::int16_t  IStargateFromLppl(APPSESSION &app, PLANET *lppl);
std::array<char, 50> PszPlayerName(APPSESSION &app, std::int16_t iPlayer, std::int16_t fCapital, std::int16_t fPlural, std::int16_t fThe, std::int16_t grWord, PLAYER *pplr);
std::int16_t  FCalcFleetBombDamage(APPSESSION &app, FLEET *lpfl, std::int32_t *pdmgPeople, std::int32_t *pdmgPeopleMin, std::int32_t *pdmgPeopleSmart, std::int32_t *pdmgBldg, std::int32_t *ppctTerra,
                              std::int16_t *pfMulti);
void     LinkFleets(APPSESSION &app, std::int16_t fUnused);
int      ICompFleetPoint(FLEET **ppfl1, FLEET **ppfl2);
int      ICompFleetPoint2(std::int32_t *pl, FLEET **ppfl);
std::int16_t  FLookupSelShip(APPSESSION &app, FLEET *pfl);
FLEET   *LpflFromId(APPSESSION &app, std::int16_t idFleet);
std::int16_t  FLookupFleet(APPSESSION &app, std::int16_t idFleet, FLEET *pfl);
std::int16_t  FDupFleet(APPSESSION &app, FLEET *lpfl, FLEET *pfl);
std::int16_t  FLookupObject(APPSESSION &app, GrobjClass grobj, std::int16_t id, void *pobj);
std::int16_t  FLookupOrbitingXfer(APPSESSION &app, std::int16_t idPlanet, std::int16_t iNth, XFER *pxf, std::int16_t idSkip);
char    *PszGetThingName(APPSESSION &app, std::int16_t id);
char    *PszGetFleetName(APPSESSION &app, std::int16_t id);
std::uint16_t WFromLpfl(APPSESSION &app, FLEET *lpfl);
char    *PszFleetNameFromWord(APPSESSION &app, std::uint16_t w);
char    *PszGetPlanetName(APPSESSION &app, std::int16_t id);
std::int16_t  IflFromLpfl(APPSESSION &app, FLEET *lpfl);
std::int16_t  FDeleteFleet(APPSESSION &app, std::int16_t idFleet, GrobjClass grobjSel, std::int16_t idSel);
FLEET   *LpflNew(APPSESSION &app, std::int16_t iPlr, std::int16_t idPl);
FLEET   *LpflNewSplit(APPSESSION &app, FLEET *pfl);
std::int16_t  FFleetMergeAll(APPSESSION &app, FLEET *pfl);
std::int16_t  FFleetSplitAll(APPSESSION &app, FLEET *pfl);
std::array<char, 360> PszGetLocName(APPSESSION &app, GrobjClass grobj, std::int16_t id, std::int16_t x, std::int16_t y);
std::int16_t  IshdefPrimaryFromLpfl(APPSESSION &app, FLEET *lpfl, std::int16_t *pcDiff);
std::array<char, 360> PszGetDistance(APPSESSION &app, std::int16_t x1, std::int16_t y1, std::int16_t x2, std::int16_t y2);
double   DGetDistance(std::int16_t x1, std::int16_t y1, std::int16_t x2, std::int16_t y2);
std::int16_t  FFindNearestObject(APPSESSION &app, POINT16 pt, GrobjClass grobj, SCAN *pscan);
void     UpdateShdefCost(APPSESSION &app, SHDEF *lpshdef);
std::uint16_t WPackLong(std::int32_t l);
std::int16_t  GetPlanetScannerRange(APPSESSION &app, PLANET *lppl, std::int16_t *pDeep);
std::int16_t  GetCachedFleetScannerRange(APPSESSION &app, FLEET *lpfl, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal);
std::int16_t  GetFleetScannerRange(APPSESSION &app, FLEET *lpfl, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal);
std::int16_t  GetShdefScannerRange(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal);
std::int32_t  LCalcFuelGainFromRamScoops(APPSESSION &app, FLEET *lpfl, std::int16_t iWarp, std::int32_t dTravel);
std::int32_t  CalcPlayerScore(APPSESSION &app, std::int16_t iPlr, SCORE *pscore);
void     GetTrueHullCost(std::int16_t iPlayer, HUL *lphul, std::uint16_t *rgCost);
std::array<char, 40> DecorateHullName(APPSESSION &app, std::int16_t iplr, std::int16_t ish);
std::int16_t  FMatchTarget(APPSESSION &app, FLEET *lpflTarget, MdTarget mdTarget, std::int16_t fExact);
void     ValidateWaypoints(APPSESSION &app);
std::int32_t  ChgPopFromPlanet(APPSESSION &app, PLANET *lppl, std::int16_t fUpdate);
std::int16_t  FCanFleetUseStargates(APPSESSION &app, FLEET *lpfl, POINT16 ptSrc, POINT16 ptDst);
std::int16_t  FFleetCanJumpgate(APPSESSION &app, FLEET *lpfl);
std::int32_t  WtFromLpfl(APPSESSION &app, FLEET *lpfl);
std::int16_t  FCanBuildShdef(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr);
std::int16_t  IshFindSimilarDesign(APPSESSION &app, HUL *lphul, std::int16_t iPlrDst);
void     ClearFile(APPSESSION &app, std::int16_t dt);
void     OutputSz(APPSESSION &app, std::int16_t dt, const char *sz);
void     TurnLog(APPSESSION &app, StringId ids);
std::int16_t  PtToScan(APPSESSION &app, std::int16_t d);
std::int16_t  ScanToPt(APPSESSION &app, std::int16_t d);
char    *SzVersion(APPSESSION &app);
void     ChangeScanSel(APPSESSION &app, SCAN *pscan, std::int16_t fValidScan);
std::int16_t  FFindSomethingAndSelectIt(APPSESSION &app);
std::int16_t  CchGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, std::span<char> sz, std::int16_t iwp, std::int16_t fSmall);

#endif
