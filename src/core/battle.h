#ifndef STARS_DECOMPILED_BATTLE_H
#define STARS_DECOMPILED_BATTLE_H

#include "common.h"
#include <stdint.h>

extern uint8_t rgbrcStart[136];

int16_t FDeleteBattlePlan(APPSESSION &app, int16_t iplan, int16_t fWarn);
int16_t FFleetHasBombs(APPSESSION &app, FLEET *lpfl);
int16_t FHullHasBombs(HUL *lphul);
int16_t FFleetHasTeeth(APPSESSION &app, FLEET *lpfl);
int16_t FHullHasTeeth(HUL *lphul);
int16_t FFuelTanker(SHDEF *lpshdef);
void    CheckTarget(APPSESSION &app, TOK *ptok, FLEET *lpfl, int16_t ishdef);
int16_t FDumpCargo(APPSESSION &app, FLEET *lpfl);
void    DropSalvage(APPSESSION &app, THING **plpth, int32_t *rgwtMinerals, int16_t iplr, POINT16 *ppt);
int16_t CplrBattle(APPSESSION &app, FLEET *lpfl, uint16_t *rggrfAttack, uint16_t *pgrfPlayer, uint16_t *pgrfSpectator);
int16_t SpdOfShip(APPSESSION &app, FLEET *lpfl, int16_t ishdef, TOK *ptok, int16_t fDumpCargo, SHDEF *lpshdef);
SHDEF  *LpshdefFromTok(APPSESSION &app, TOK *ptok);
int16_t FCanKillTok(APPSESSION &app, TOK *ptok1, TOK *ptok2);
void    DoBattles(APPSESSION &app, int16_t fPostMovement);
void    RegenShield(APPSESSION &app, TOK *ptok);
int16_t InitFromHuldef(APPSESSION &app, HUL *lphul, int16_t *ppctBC);
void    CheckInitiative(APPSESSION &app, TOK *ptok);
void    CheckWeapons(APPSESSION &app, TOK *ptok, int16_t *pfDampeningField, uint8_t *pinit);
void    RandomizeTokOrder(APPSESSION &app);
void    InitializeBoard(APPSESSION &app, FLEET *lpfl, int16_t ibrc, uint16_t grfPlayer, uint8_t *pinit, int16_t *pinitMin, int16_t *pinitMac);
int16_t DzFromBrcBrc(uint8_t brc1, uint8_t brc2);
int32_t DpFromPtokBrcToBrc(APPSESSION &app, TOK *ptok, uint8_t brcSrc, uint8_t brcTarget, TOK *ptokTarget, int16_t fProximity);
int16_t DzMoveRangeToConsider(APPSESSION &app, TOK *ptok, uint16_t grfAttack, uint8_t *pbrc);
int16_t FDoesPrimaryTargetTypeExist(APPSESSION &app, TOK *ptok, uint16_t grfAttack);
int16_t FIsTargetOfMdTarget(TOK *ptok, MdTarget mdTarget);
int32_t ScoreGuessBattleDamage(APPSESSION &app, TOK *ptokSrc, uint8_t brc, int16_t fPrimary, uint16_t grfAttack);
int32_t ScoreFromGiveAndTakeAndTactic(int32_t dpGive, int32_t dpTake, BattleTactic mdTactic);
int16_t DxyMoveTokTo(APPSESSION &app, TOK *ptok, int16_t spdMove, uint16_t grfAttack);
int32_t CTorpHit(APPSESSION &app, int32_t cTorpBase, TOK *ptok, int16_t pctBase, int16_t pctBC);
int16_t FAttack(APPSESSION &app, int16_t itokAttacker, int16_t init, uint8_t *lpbtlrec, uint16_t grfAttack);
void    KillShips(APPSESSION &app, TOK *ptok, int16_t cshKill, int16_t ishdef, FLEET *lpfl, int16_t fFallout);
void    CreateSalvage(APPSESSION &app, FLEET *pfl, THING **plpth);
int16_t FDamageTok(APPSESSION &app, TOK *ptok, int16_t itok, int32_t *pdpBeam, int32_t dpTorp, GrfWeapon grfWeapon, int16_t fShieldsOnly, int32_t *pcTorp);
int16_t DxyFromSpdRound(uint16_t spd, int16_t iRound);
int16_t FDoCoolBattle(APPSESSION &app, FLEET *lpfl, int16_t cplr, uint16_t *rggrfAttack, uint16_t grfPlayer, uint16_t grfSpectator);
int16_t ITechLearnATech(APPSESSION &app, int16_t iplr, int16_t x, int16_t y, MessageId idm, uint16_t *piGoto);
void SendBattleMessages(APPSESSION &app, FLEET *lpflBtl, int16_t cplr, int16_t idBtl, uint16_t *rgPlrLosses, int16_t grfPlayer, int16_t cShipsInvolved, int16_t cShdefsInvolved,
                        uint16_t grfSpectator);
int16_t FAttackPlayer(APPSESSION &app, FLEET *lpfl, int16_t iplr);
void    DoBombing(APPSESSION &app);
int16_t CBattles(APPSESSION &app);

#endif
