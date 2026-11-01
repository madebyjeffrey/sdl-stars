#ifndef STARS_DECOMPILED_BATTLE_H
#define STARS_DECOMPILED_BATTLE_H

#include "common.h"
#include <cstdint>

extern std::uint8_t rgbrcStart[136];

std::int16_t FDeleteBattlePlan(APPSESSION &app, std::int16_t iplan, std::int16_t fWarn);
std::int16_t FFleetHasBombs(APPSESSION &app, FLEET *lpfl);
std::int16_t FHullHasBombs(HUL *lphul);
std::int16_t FFleetHasTeeth(APPSESSION &app, FLEET *lpfl);
std::int16_t FHullHasTeeth(HUL *lphul);
std::int16_t FFuelTanker(SHDEF *lpshdef);
void    CheckTarget(APPSESSION &app, TOK *ptok, FLEET *lpfl, std::int16_t ishdef);
std::int16_t FDumpCargo(APPSESSION &app, FLEET *lpfl);
void    DropSalvage(APPSESSION &app, THING **plpth, std::int32_t *rgwtMinerals, std::int16_t iplr, POINT16 *ppt);
std::int16_t CplrBattle(APPSESSION &app, FLEET *lpfl, std::uint16_t *rggrfAttack, std::uint16_t *pgrfPlayer, std::uint16_t *pgrfSpectator);
std::int16_t SpdOfShip(APPSESSION &app, FLEET *lpfl, std::int16_t ishdef, TOK *ptok, std::int16_t fDumpCargo, SHDEF *lpshdef);
SHDEF  *LpshdefFromTok(APPSESSION &app, TOK *ptok);
std::int16_t FCanKillTok(APPSESSION &app, TOK *ptok1, TOK *ptok2);
void    DoBattles(APPSESSION &app, std::int16_t fPostMovement);
void    RegenShield(APPSESSION &app, TOK *ptok);
std::int16_t InitFromHuldef(APPSESSION &app, HUL *lphul, std::int16_t *ppctBC);
void    CheckInitiative(APPSESSION &app, TOK *ptok);
void    CheckWeapons(APPSESSION &app, TOK *ptok, std::int16_t *pfDampeningField, std::uint8_t *pinit);
void    RandomizeTokOrder(APPSESSION &app);
void    InitializeBoard(APPSESSION &app, FLEET *lpfl, std::int16_t ibrc, std::uint16_t grfPlayer, std::uint8_t *pinit, std::int16_t *pinitMin, std::int16_t *pinitMac);
std::int16_t DzFromBrcBrc(std::uint8_t brc1, std::uint8_t brc2);
std::int32_t DpFromPtokBrcToBrc(APPSESSION &app, TOK *ptok, std::uint8_t brcSrc, std::uint8_t brcTarget, TOK *ptokTarget, std::int16_t fProximity);
std::int16_t DzMoveRangeToConsider(APPSESSION &app, TOK *ptok, std::uint16_t grfAttack, std::uint8_t *pbrc);
std::int16_t FDoesPrimaryTargetTypeExist(APPSESSION &app, TOK *ptok, std::uint16_t grfAttack);
std::int16_t FIsTargetOfMdTarget(TOK *ptok, MdTarget mdTarget);
std::int32_t ScoreGuessBattleDamage(APPSESSION &app, TOK *ptokSrc, std::uint8_t brc, std::int16_t fPrimary, std::uint16_t grfAttack);
std::int32_t ScoreFromGiveAndTakeAndTactic(std::int32_t dpGive, std::int32_t dpTake, BattleTactic mdTactic);
std::int16_t DxyMoveTokTo(APPSESSION &app, TOK *ptok, std::int16_t spdMove, std::uint16_t grfAttack);
std::int32_t CTorpHit(APPSESSION &app, std::int32_t cTorpBase, TOK *ptok, std::int16_t pctBase, std::int16_t pctBC);
std::int16_t FAttack(APPSESSION &app, std::int16_t itokAttacker, std::int16_t init, std::uint8_t *lpbtlrec, std::uint16_t grfAttack);
void    KillShips(APPSESSION &app, TOK *ptok, std::int16_t cshKill, std::int16_t ishdef, FLEET *lpfl, std::int16_t fFallout);
void    CreateSalvage(APPSESSION &app, FLEET *pfl, THING **plpth);
std::int16_t FDamageTok(APPSESSION &app, TOK *ptok, std::int16_t itok, std::int32_t *pdpBeam, std::int32_t dpTorp, GrfWeapon grfWeapon, std::int16_t fShieldsOnly, std::int32_t *pcTorp);
std::int16_t DxyFromSpdRound(std::uint16_t spd, std::int16_t iRound);
std::int16_t FDoCoolBattle(APPSESSION &app, FLEET *lpfl, std::int16_t cplr, std::uint16_t *rggrfAttack, std::uint16_t grfPlayer, std::uint16_t grfSpectator);
std::int16_t ITechLearnATech(APPSESSION &app, std::int16_t iplr, std::int16_t x, std::int16_t y, MessageId idm, std::uint16_t *piGoto);
void SendBattleMessages(APPSESSION &app, FLEET *lpflBtl, std::int16_t cplr, std::int16_t idBtl, std::uint16_t *rgPlrLosses, std::int16_t grfPlayer, std::int16_t cShipsInvolved, std::int16_t cShdefsInvolved,
                        std::uint16_t grfSpectator);
std::int16_t FAttackPlayer(APPSESSION &app, FLEET *lpfl, std::int16_t iplr);
void    DoBombing(APPSESSION &app);
std::int16_t CBattles(APPSESSION &app);

#endif
