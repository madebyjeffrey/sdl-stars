#ifndef STARS_DECOMPILED_AIUTIL_H
#define STARS_DECOMPILED_AIUTIL_H

#include "common.h"
#include <stdint.h>

extern int32_t vrgAiPacketDist[2];
extern uint8_t vrgSBAip[];
extern uint8_t vrgSBMacAisb[6];
extern AIPART  vrgAiParts[];

int16_t  FCreateAiShdef(APPSESSION &app, int16_t ishdef, HulDef ihul, uint8_t *rgaip);
int16_t  FGetAIPart(APPSESSION &app, AiPartPreference aip, PART *ppart);
void     PickANameAndBmp(APPSESSION &app, SHDEF *pshdef, StringId ids, int16_t cids, int16_t ibmpStart);
int16_t  FChangeAiShdef(APPSESSION &app, SHDEF *pshdef, int16_t ishdef);
int16_t  XferAiSupply(APPSESSION &app, GrobjClass grobjSrc, int16_t idSrc, GrobjClass grobjDst, int16_t idDst, MineralType iSupply, int16_t cQuan);
int16_t  XferAiTroopers(APPSESSION &app, int16_t idSrc, int16_t idDst, int16_t cQuan);
int16_t  FColonizeAiFleet(APPSESSION &app, FLEET *lpfl, int16_t idPlanet);
int16_t  FGotoWormholeAiFleet(APPSESSION &app, FLEET *lpfl, THING *lpthWorm);
int16_t  IdNearestColonizablePlanet(APPSESSION &app, FLEET *lpflCol, THING **plpthWorm);
THING   *LpthWormFind(APPSESSION &app, POINT16 *ppt, int32_t d2);
uint32_t UlFleetPower(APPSESSION &app, FLEET *lpfl);
int16_t  IdNearestUnknownPlanet(APPSESSION &app, FLEET *lpfl, THING **plpthWorm);
void     AddMinesToBlockedQueues(APPSESSION &app);
int16_t  FFleetInField(FLEET *lpfl, THING *lpth);
void     SetAiFleetIdealSpeed(APPSESSION &app, FLEET *lpfl, int16_t wtFuelMax, int16_t cMinefields, THING **rglpth);
int16_t  IdTargetAttack(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans);
int16_t  IdTargetFreighter(APPSESSION &app, FLEET *lpflFr, PLANET *lpplHome);
int16_t  FSalvageTargetFreighter2(APPSESSION &app, FLEET *lpflFr, int16_t fNeedy, int16_t iWorst, int16_t pctFull, int32_t wtCargoMax, int32_t scoreBest, THING **plpthBest,
                                  int16_t *pidBest);
int16_t  FMoveAiFleet(APPSESSION &app, FLEET *lpfl, ORDER *pord, int16_t fAppend);
void     AddItemToQueue(APPSESSION &app, uint16_t iItem, uint16_t cItem, GrobjClass grobj, AddItemMode mdAddItem);
int16_t  IroEnsureAi(APPSESSION &app, uint8_t *lpbRes, int16_t cRes, int16_t *pishdefSBLatest, int16_t pct);
void     KeepFleetsMoving(APPSESSION &app);
int16_t  FShouldWeBuildColonizers(APPSESSION &app, int16_t *pcCol);
int16_t  FIsAiAttack(APPSESSION &app, FLEET *lpfl);
int16_t  FIsTurinDroneAiAttack(APPSESSION &app, FLEET *lpfl);
int16_t  FIsAiTransport(APPSESSION &app, FLEET *lpfl);
void     ValidateStarbaseHistory(APPSESSION &app);
void     GetResourcesAvailable(APPSESSION &app, PLANET *lppl, int32_t *rgRes);
void     GetProdQCost(APPSESSION &app, PLANET *lppl, int32_t *rgCost);
void     MergeAllShdefs(APPSESSION &app, int16_t grbitish);
FLEET   *LpflFindClosestEnum(APPSESSION &app, FLEET *lpfl, int16_t (*pfn)(APPSESSION &, FLEET *, FLEET *));
PLANET  *LpplFindClosestEnum(APPSESSION &app, PLANET *lppl, int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *));
PLANET  *LpplFindBestEnum(APPSESSION &app, PLANET *lppl, int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *));
int16_t  IdRandomPlanetNearby(APPSESSION &app, POINT16 pt, int16_t cDist, int16_t fAvoidStarbases);
void     ClearAiCurrentTask(APPSESSION &app, FLEET *lpfl, int16_t fChangeSel);
int16_t  FEnumOurStarbase(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest);
int16_t  FFleetMightHaveTeeth(APPSESSION &app, FLEET *lpfl);
int16_t  IdTargetScout(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans, THING **plpthWorm);
void     MarkPlanetsUnderAttack(APPSESSION &app);
void     FixPlanetsUnderAttack(APPSESSION &app, PROD *rgprod);
void     QuickBuildDefenses(APPSESSION &app, PLANET *lppl, PROD *rgprod);
int16_t  IdplFindClosestStarbase(APPSESSION &app, FLEET *lpfl, int16_t fBigOnes);
int16_t  FMoveToNearestStarbase(APPSESSION &app, FLEET *lpfl, int16_t fBigOnes);
void     MoveToNearestPlanetOrEnemy(APPSESSION &app, FLEET *lpfl, int16_t dEnemyRange);
void     EnsureAiStarbaseDesigns(APPSESSION &app);
void     EnsureMacintiStarbaseDesigns(APPSESSION &app, uint8_t *rgSB);
int16_t  FCreateAiStarbase(APPSESSION &app, int16_t ishdef, int16_t iLevel, AiStarbaseRecipe aisb, isbhull isb);
int16_t  FAIFling(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail);
int16_t  IshdefAiSBLatestOF(APPSESSION &app);
int16_t  IshdefAiSBLatest(APPSESSION &app);
void     QueueAiStarbases(APPSESSION &app, PROD *rgprod, int16_t ishdefSBLatest);
int16_t  FUpgradeAiStarbase(APPSESSION &app, PLANET *lppl, int16_t ishdefSBLatest);
int16_t  FQueueAiTerraforming(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost);
int16_t  FQueueAiScanner(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost);
int16_t  FQueueAiDefenses(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost);
void     HandleBasicAiTasks(APPSESSION &app, int16_t iroCur, PROD *rgprod, int16_t ishdefSBLatest, int32_t *rgResAvail, int32_t *rgResCost);
void     SplitOutShdefs(APPSESSION &app, uint8_t *rgbIsh);
int16_t  CheckAiShdefStatus(APPSESSION &app, int16_t ishBeg, int16_t ishEnd, uint16_t cRecyclePeriod, int16_t *piLatest, uint8_t *rgbOld);
void     IncreaseAIMinefieldSizes(APPSESSION &app);
int16_t  FFindBuddyAndJoinUp(APPSESSION &app, FLEET *lpfl, int16_t ishLo, int16_t ishHi, int32_t lMaxDist1, int32_t lMaxDist2);
int16_t  FShouldPlanetBuildColonizer(APPSESSION &app, PLANET *lpplSrc);
void     InitRandomPlanetList(APPSESSION &app);

#endif
