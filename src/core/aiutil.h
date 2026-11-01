#ifndef STARS_DECOMPILED_AIUTIL_H
#define STARS_DECOMPILED_AIUTIL_H

#include "common.h"
#include <cstdint>

extern std::int32_t vrgAiPacketDist[2];
extern std::uint8_t vrgSBAip[];
extern std::uint8_t vrgSBMacAisb[6];
extern AIPART  vrgAiParts[];

std::int16_t  FCreateAiShdef(APPSESSION &app, std::int16_t ishdef, HulDef ihul, std::uint8_t *rgaip);
std::int16_t  FGetAIPart(APPSESSION &app, AiPartPreference aip, PART *ppart);
void     PickANameAndBmp(APPSESSION &app, SHDEF *pshdef, StringId ids, std::int16_t cids, std::int16_t ibmpStart);
std::int16_t  FChangeAiShdef(APPSESSION &app, SHDEF *pshdef, std::int16_t ishdef);
std::int16_t  XferAiSupply(APPSESSION &app, GrobjClass grobjSrc, std::int16_t idSrc, GrobjClass grobjDst, std::int16_t idDst, MineralType iSupply, std::int16_t cQuan);
std::int16_t  XferAiTroopers(APPSESSION &app, std::int16_t idSrc, std::int16_t idDst, std::int16_t cQuan);
std::int16_t  FColonizeAiFleet(APPSESSION &app, FLEET *lpfl, std::int16_t idPlanet);
std::int16_t  FGotoWormholeAiFleet(APPSESSION &app, FLEET *lpfl, THING *lpthWorm);
std::int16_t  IdNearestColonizablePlanet(APPSESSION &app, FLEET *lpflCol, THING **plpthWorm);
THING   *LpthWormFind(APPSESSION &app, POINT16 *ppt, std::int32_t d2);
std::uint32_t UlFleetPower(APPSESSION &app, FLEET *lpfl);
std::int16_t  IdNearestUnknownPlanet(APPSESSION &app, FLEET *lpfl, THING **plpthWorm);
void     AddMinesToBlockedQueues(APPSESSION &app);
std::int16_t  FFleetInField(FLEET *lpfl, THING *lpth);
void     SetAiFleetIdealSpeed(APPSESSION &app, FLEET *lpfl, std::int16_t wtFuelMax, std::int16_t cMinefields, THING **rglpth);
std::int16_t  IdTargetAttack(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, std::int16_t fOnlyHumans);
std::int16_t  IdTargetFreighter(APPSESSION &app, FLEET *lpflFr, PLANET *lpplHome);
std::int16_t  FSalvageTargetFreighter2(APPSESSION &app, FLEET *lpflFr, std::int16_t fNeedy, std::int16_t iWorst, std::int16_t pctFull, std::int32_t wtCargoMax, std::int32_t scoreBest, THING **plpthBest,
                                  std::int16_t *pidBest);
std::int16_t  FMoveAiFleet(APPSESSION &app, FLEET *lpfl, ORDER *pord, std::int16_t fAppend);
void     AddItemToQueue(APPSESSION &app, std::uint16_t iItem, std::uint16_t cItem, GrobjClass grobj, AddItemMode mdAddItem);
std::int16_t  IroEnsureAi(APPSESSION &app, std::uint8_t *lpbRes, std::int16_t cRes, std::int16_t *pishdefSBLatest, std::int16_t pct);
void     KeepFleetsMoving(APPSESSION &app);
std::int16_t  FShouldWeBuildColonizers(APPSESSION &app, std::int16_t *pcCol);
std::int16_t  FIsAiAttack(APPSESSION &app, FLEET *lpfl);
std::int16_t  FIsTurinDroneAiAttack(APPSESSION &app, FLEET *lpfl);
std::int16_t  FIsAiTransport(APPSESSION &app, FLEET *lpfl);
void     ValidateStarbaseHistory(APPSESSION &app);
void     GetResourcesAvailable(APPSESSION &app, PLANET *lppl, std::int32_t *rgRes);
void     GetProdQCost(APPSESSION &app, PLANET *lppl, std::int32_t *rgCost);
void     MergeAllShdefs(APPSESSION &app, std::int16_t grbitish);
FLEET   *LpflFindClosestEnum(APPSESSION &app, FLEET *lpfl, std::int16_t (*pfn)(APPSESSION &, FLEET *, FLEET *));
PLANET  *LpplFindClosestEnum(APPSESSION &app, PLANET *lppl, std::int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *));
PLANET  *LpplFindBestEnum(APPSESSION &app, PLANET *lppl, std::int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *));
std::int16_t  IdRandomPlanetNearby(APPSESSION &app, POINT16 pt, std::int16_t cDist, std::int16_t fAvoidStarbases);
void     ClearAiCurrentTask(APPSESSION &app, FLEET *lpfl, std::int16_t fChangeSel);
std::int16_t  FEnumOurStarbase(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
std::int16_t  FFleetMightHaveTeeth(APPSESSION &app, FLEET *lpfl);
std::int16_t  IdTargetScout(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, std::int16_t fOnlyHumans, THING **plpthWorm);
void     MarkPlanetsUnderAttack(APPSESSION &app);
void     FixPlanetsUnderAttack(APPSESSION &app, PROD *rgprod);
void     QuickBuildDefenses(APPSESSION &app, PLANET *lppl, PROD *rgprod);
std::int16_t  IdplFindClosestStarbase(APPSESSION &app, FLEET *lpfl, std::int16_t fBigOnes);
std::int16_t  FMoveToNearestStarbase(APPSESSION &app, FLEET *lpfl, std::int16_t fBigOnes);
void     MoveToNearestPlanetOrEnemy(APPSESSION &app, FLEET *lpfl, std::int16_t dEnemyRange);
void     EnsureAiStarbaseDesigns(APPSESSION &app);
void     EnsureMacintiStarbaseDesigns(APPSESSION &app, std::uint8_t *rgSB);
std::int16_t  FCreateAiStarbase(APPSESSION &app, std::int16_t ishdef, std::int16_t iLevel, AiStarbaseRecipe aisb, isbhull isb);
std::int16_t  FAIFling(APPSESSION &app, PLANET *lppl, std::int32_t *rgResAvail);
std::int16_t  IshdefAiSBLatestOF(APPSESSION &app);
std::int16_t  IshdefAiSBLatest(APPSESSION &app);
void     QueueAiStarbases(APPSESSION &app, PROD *rgprod, std::int16_t ishdefSBLatest);
std::int16_t  FUpgradeAiStarbase(APPSESSION &app, PLANET *lppl, std::int16_t ishdefSBLatest);
std::int16_t  FQueueAiTerraforming(APPSESSION &app, PLANET *lppl, std::int32_t *rgResAvail, std::int32_t *rgResCost);
std::int16_t  FQueueAiScanner(APPSESSION &app, PLANET *lppl, std::int32_t *rgResAvail, std::int32_t *rgResCost);
std::int16_t  FQueueAiDefenses(APPSESSION &app, PLANET *lppl, std::int32_t *rgResAvail, std::int32_t *rgResCost);
void     HandleBasicAiTasks(APPSESSION &app, std::int16_t iroCur, PROD *rgprod, std::int16_t ishdefSBLatest, std::int32_t *rgResAvail, std::int32_t *rgResCost);
void     SplitOutShdefs(APPSESSION &app, std::uint8_t *rgbIsh);
std::int16_t  CheckAiShdefStatus(APPSESSION &app, std::int16_t ishBeg, std::int16_t ishEnd, std::uint16_t cRecyclePeriod, std::int16_t *piLatest, std::uint8_t *rgbOld);
void     IncreaseAIMinefieldSizes(APPSESSION &app);
std::int16_t  FFindBuddyAndJoinUp(APPSESSION &app, FLEET *lpfl, std::int16_t ishLo, std::int16_t ishHi, std::int32_t lMaxDist1, std::int32_t lMaxDist2);
std::int16_t  FShouldPlanetBuildColonizer(APPSESSION &app, PLANET *lpplSrc);
void     InitRandomPlanetList(APPSESSION &app);

#endif
