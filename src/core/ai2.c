#include "common.h"

void DoMaidAiTurn(PROD *rgprod) {
    int32_t rgResCost[4];
    int32_t rgResAvail[4];
    int16_t iroCur;

    iroCur = IroEnsureAi(NULL, 0, NULL, game.turn >= 20 ? 15 : 0);
    fMarkedPlanets = FALSE;
    HandleBasicAiTasks(iroCur, rgprod, ishdefNone, rgResAvail, rgResCost);
    FillProductionQueue();
    return;
}

int16_t FPotentISWarFleet(FLEET *lpfl, int16_t iPotency) {
    int16_t ish;
    int16_t cEquiv;

    cEquiv = 0;
    for (ish = 11; ish <= 12; ish++) {
        cEquiv += lpfl->rgcsh[ish];
    }
    for (ish = 9; ish <= 10; ish++) {
        cEquiv += lpfl->rgcsh[ish] * 2;
    }
    if (iPotency < 2) {
        return TRUE;
    }
    if (cEquiv >= vrgAiArmadaPotency[0]) {
        return TRUE;
    }
    return FALSE;
}

void DoAutomitronAiTurn(PROD *rgprod) {
    int16_t  cExistCargo;
    uint16_t rgCosts[4];
    int32_t  rgResCost[4];
    int16_t  iLatestCruiser;
    int32_t  rgResAvail[4];
    FLEET   *lpflEnemy;
    ORDER    ord;
    uint8_t  rgRecycleShdef[16];
    PLANET  *lpplDest;
    int16_t  cplNegative;
    THING   *lpthWorm;
    int16_t  cFr;
    int16_t  idPlanDst;
    int16_t  cplBadGuy;
    PLANET  *lpplMac;
    PLANET  *lppl;
    PLANET  *lpplHome;
    FLEET   *lpfl;
    int16_t  ifl;
    int16_t  i;
    int16_t  cRes;
    int16_t  iroCur;
    FLEET   *lpflT;
    FLEET   *lpflAttack;
    uint8_t  b;
    int16_t  ishdefSBLatest;
    uint16_t cRecyclePeriod;
    uint16_t cplanCol;
    int16_t  iLatestCargo;
    int16_t  iLatestBomber;
    int16_t  j;
    int16_t  iLatestBattle;
    int32_t  l;
    PROD    *lpprod;
    int16_t  fWrite;
    int16_t  iPlanet;
    int16_t  id;

    iPlanet = rgplr[idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    iroCur = IroEnsureAi(vrgAiISResOrder, cAiISResOrder, &ishdefSBLatest, game.turn >= 10 ? 20 : 0);
    if (game.turn > 50) {
        MergeAllShdefs(7692);
        MergeAllShdefs(64);
        MergeAllShdefs(16384);
    } else if (game.turn > 30) {
        MergeAllShdefs(16384);
    }
    j = 3;
    if (game.turn > 130) {
        j += (uint32_t)(game.turn - 120) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    vrgAiArmadaPotency[0] = j;
    vrgAiArmadaPotency[1] = (int16_t)(j & 0xff) / 2;
    j = 6;
    if (game.turn > 115) {
        j += (uint32_t)(game.turn - 100) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    vrgAiArmadaPotency[2] = j;
    vrgAiArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    memset(rgRecycleShdef, 0, 16);
    if (game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = game.turn >= 200 ? 100 : 70;
    }
    CheckAiShdefStatus(11, 12, cRecyclePeriod, &iLatestCruiser, rgRecycleShdef);
    cExistCargo = CheckAiShdefStatus(4, 5, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(2, 3, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(9, 10, cRecyclePeriod, &iLatestBattle, rgRecycleShdef);
    if (game.turn > 60) {
        SplitOutShdefs(rgRecycleShdef);
    }
    EnsureISShdefs(iroCur);
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome && PctPlanetOptValue(lppl, idPlayer) > 0) {
            cplanCol++;
        }
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        if (lppl->iPlayer != idPlayer && lppl->iPlayer != iplrNone) {
            vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            if (PctPlanetOptValue(lppl, idPlayer) > 0) {
                vlpbAiPlanet[lppl->id * 16 + 3] = 1;
            }
            cplBadGuy++;
        } else if (lppl->iPlayer == idPlayer && PctPlanetDesirability(lppl, idPlayer) < 0) {
            cplNegative++;
            vlpbAiPlanet[lppl->id * 16 + 2] = 1;
        } else if (lppl->fStarbase && lppl->rgwtMin[3] >= 1500) {
            ChangeMainObjSel(grobjPlanet, lppl->id);
            InitProduction(rgprod);
            fWrite = FALSE;
            b = 0;
            i = 0;
            for (lpprod = lpplProdGlob->rgprod; i < lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < lpplProdGlob->iprodMac) {
                FinishProduction(FALSE);
            } else {
                cFr = rgplr[idPlayer].cPlanet / 10 <= ((AIHIST *)vlpbAiData)->cStarbase * 2 ? ((AIHIST *)vlpbAiData)->cStarbase * 2
                                                                                            : rgplr[idPlayer].cPlanet / 10;
                if (rgplr[idPlayer].rgTech[2] >= 5 && iLatestCargo != ishdefNone &&
                    (cExistCargo < cFr || (cExistCargo < (int16_t)(10 * cFr) / 7 && Random(4) == 0))) {
                    AddItemToQueue(iLatestCargo, 1, grobjFleet, addItemEnd);
                    fWrite = TRUE;
                }
                if (cplanCol != 0 && rgshdef[1].cExist == 0 && game.turn > 10) {
                    AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                    fWrite = TRUE;
                }
                l = (uint32_t)(lppl->rgwtMin[3] * PctTrueMaxGrowth(idPlayer));
                cRes = CResourcesAtPlanet(lppl, idPlayer);
                if (!rgshdef[6].fFree && Random(3) == 0) {
                    id = lppl->id;
                    cFr = 0;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->rgcsh[6] > 0 && lpfl->iPlayer == idPlayer) {
                            cFr = lpfl->rgcsh[6];
                            break;
                        }
                    }
                    if ((cFr < 10 || (cFr < 17 && Random(8) == 0)) && Random(cFr * 2 + 1) == 0) {
                        AddItemToQueue(6, 3, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                }
                if (iLatestBomber != ishdefNone) {
                    id = lppl->id;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->iPlayer == idPlayer && FPotentISWarFleet(lpfl, 2)) {
                            if (iLatestBomber == ishdefNone || lpfl->rgcsh[2] + lpfl->rgcsh[3] < vrgAiArmadaPotency[2])
                                break;
                            AddItemToQueue(iLatestBomber, 4, grobjFleet, addItemEnd);
                            fWrite = TRUE;
                            goto FinishProd;
                        }
                    }
                }
                if (iLatestCruiser != ishdefNone && rgshdef[iLatestCruiser].cExist < (uint32_t)(game.cPlanMax / 12 + 8)) {
                    GetResourcesAvailable(lppl, rgResAvail);
                    GetProdQCost(lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(idPlayer, &rgshdef[iLatestCruiser].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= (uint32_t)rgCosts[j];
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = TRUE;
                        AddItemToQueue(iLatestCruiser, 1, grobjFleet, addItemEnd);
                    }
                }
                if (iLatestBattle != ishdefNone && rgshdef[iLatestBattle].cExist < (uint32_t)(game.cPlanMax / 24 + 4)) {
                    GetResourcesAvailable(lppl, rgResAvail);
                    GetProdQCost(lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(idPlayer, &rgshdef[iLatestBattle].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= (uint32_t)rgCosts[j];
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = TRUE;
                        AddItemToQueue(iLatestBattle, 1, grobjFleet, addItemEnd);
                    }
                }
            FinishProd:
                FinishProduction(fWrite);
            }
        }
    }
    lpflAttack = NULL;
    lpflEnemy = NULL;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (FIsAiAttack(lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
            }
            lpfl->fMark = FALSE;
            if (FIsAiTransport(lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst == idplNone)
                    continue;
                lppl = LpplFromId(idPlanDst);
                if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == idPlayer))
                    continue;
                if (vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                    memset(&ord, 0, sizeof(ORDER));
                    ord.pt = rgptPlan[idPlanDst];
                    ord.grobj = grobjPlanet;
                    ord.id = idPlanDst;
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = TRUE;
                    ord.txp.rgia[3].iAction = iActionUnloadAll;
                    ChangeMainObjSel(grobjFleet, lpfl->id);
                    if (sel.fl.lpplord->rgord[0].id == idPlanDst) {
                        sel.fl.lpplord->rgord[0] = ord;
                    } else {
                        sel.fl.lpplord->rgord[1] = ord;
                    }
                    FLookupFleet(idWriteBack, &sel.fl);
                    vlpbAiPlanet[idPlanDst * 16 + 3] = vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                    continue;
                }
            LBlowAwayOrders:
                ChangeMainObjSel(grobjFleet, lpfl->id);
                sel.fl.cord = 1;
                sel.fl.lpplord->iordMac = 1;
                FLookupFleet(idWriteBack, &sel.fl);
                ClearAiCurrentTask(lpfl, FALSE);
                continue;
            } else {
                if (lpfl->rgcsh[1] == 0)
                    continue;
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
                if (idPlanDst == idplNone)
                    continue;
                if (vlpbAiPlanet[idPlanDst * 16 + 3] != 0)
                    goto LCheckForColDrop;
                lppl = LpplFromId(idPlanDst);
                if (lppl && lppl->iPlayer != iplrNone && lppl->iPlayer != idPlayer)
                    goto LBlowAwayOrders;
            }
        }
    }
    fMarkedPlanets = FALSE;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != idPlayer)
            continue;
        if (lpfl->cord > 1) {
            if (rgshdef[0].hul.rghs[0].iItem >= iengineRadiatingHydroRamScoop || lpfl->rgwtMin[4] >= 2)
                continue;
        LScrapFleet:
            ChangeMainObjSel(grobjFleet, lpfl->id);
            sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
            FLookupFleet(idWriteBack, &sel.fl);
            continue;
        }
        if (lpfl->rgcsh[6] != 0) {
            if (lpfl->cord != 1 || lpfl->lpplord->rgord[0].grTask != grTaskNone)
                continue;
            ChangeMainObjSel(grobjFleet, lpfl->id);
            sel.fl.lpplord->rgord[0].grTask = grTaskLayMines;
            sel.fl.lpplord->rgord[0].tlm.cTime = 5;
            sel.fl.lpplord->rgord[0].tlm.cTimeOld = 5;
            FLookupFleet(idWriteBack, &sel.fl);
            continue;
        }
        if (lpfl->rgcsh[1] == 0)
            goto LTryFreighters;
        if (rgshdef[1].hul.ihuldef != ihuldefMediumFreighter)
            goto LScrapFleet;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if ((lpfl->idPlanet == idPlanetDeepSpace || sel.pl.iPlayer != idPlayer || sel.pl.rgwtMin[3] < 200) && lpfl->rgwtMin[3] == 0) {
            if ((sel.fl.idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer && sel.pl.fStarbase) ||
                (rgshdef[1].hul.rghs[0].iItem >= iengineFuelMizer && FMoveToNearestStarbase(lpfl, FALSE)))
                continue;
            goto LScrapFleet;
        } else {
            lpthWorm = NULL;
            idPlanDst = IdNearestColonizablePlanet(lpfl, NULL);
            if (lpfl->idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 150);
                FLookupFleet(lpfl->id, &sel.fl);
            }
            if (idPlanDst == idplNone)
                continue;
            FColonizeAiFleet(lpfl, idPlanDst);
            vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
            continue;
        }
    LTryFreighters:
        if (!FIsAiTransport(lpfl))
            goto LTryBombers;
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac && (lppl->iPlayer != idPlayer || !lppl->fStarbase); lppl++) {
        }
        lpplHome = lppl == lpplMac ? NULL : lppl;
        if (!lpplHome)
            goto BestSpeed;
        lppl = NULL;
        for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
            for (j = 0; j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter && ((AIHIST *)vlpbAiData)->rgasb[i].rgflid[j] != lpfl->id; j++) {
            }
            if (j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter)
                break;
        }
        if (i < ((AIHIST *)vlpbAiData)->cStarbase) {
            lppl = LpplFromId(((AIHIST *)vlpbAiData)->rgasb[i].idPlanet);
        }
        IdTargetFreighter(lpfl, !lppl ? lpplHome : lppl);
        continue;
    LTryBombers:
        if (lpfl->rgcsh[2] == 0 && lpfl->rgcsh[3] == 0)
            goto LTryScouts;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if (lpfl->idPlanet != idPlanetDeepSpace) {
            lppl = LpplFromId(lpfl->idPlanet);
            if (lppl->iPlayer == idPlayer) {
                if (lppl->fStarbase) {
                    if ((lpfl->rgcsh[2] < 2 && lpfl->rgcsh[3] < 2) || (lpfl->rgcsh[9] < 3 && lpfl->rgcsh[10] < 3))
                        continue;
                }
                FLookupFleet(lpfl->id, &sel.fl);
                goto LTargetBomber;
            }
            if (lppl->iPlayer == iplrNone)
                goto LTargetBomber;
            for (lpflT = lpflEnemy; lpflT; lpflT = lpflT->lpflNext) {
                if (lpfl->pt.x == lpflT->pt.x && lpfl->pt.y == lpflT->pt.y && FIsAiAttack(lpflT))
                    goto LTargetBomber;
            }
            continue;
        } else {
            lppl = lpplHome;
        }
    LTargetBomber:
        if (game.fAisBand) {
            lpplDest = LpplFindBestEnum(lppl, FEnumCalcArmadaHumanDest);
        } else {
            lpplDest = NULL;
        }
        if (!lpplDest) {
            lpplDest = LpplFindBestEnum(lppl, FEnumCalcArmadaDest);
        }
        lppl = lpplDest;
        if (!lppl)
            continue;
        vlpbAiPlanet[lppl->id * 16 + 10] = vlpbAiPlanet[lppl->id * 16 + 0xa] | 0x80;
        ord.id = lppl->id;
        ord.grobj = grobjPlanet;
        ord.pt = rgptPlan[lppl->id];
        ord.grTask = grTaskNone;
        ord.fValidTask = TRUE;
        ord.iWarp = 4;
        FMoveAiFleet(lpfl, &ord, FALSE);
        continue;
    LTryScouts:
        if (lpfl->rgcsh[0] == 0)
            goto LTryFighters;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if (rgshdef[0].hul.rghs[0].iItem < iengineRadiatingHydroRamScoop && lpfl->rgwtMin[4] < 2)
            goto LScrapFleet;
        IdTargetScout(lpfl, lpflAttack, lpflEnemy, game.fAisBand, &lpthWorm);
        continue;
    LTryFighters:;
    }
BestSpeed:
    HandleBasicAiTasks(iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue();
    return;
}

void EnsureISShdefs(int16_t iroCur) {
    SHDEF   shdef;
    int16_t i;

    if (rgshdef[4].fFree && rgplr[idPlayer].rgTech[2] >= 5) {
        FCreateAiShdef(4, ihuldefMediumFreighter, (uint8_t *)&vrgISAip[isRecipeMediumFreighter]);
    }
    if (rgshdef[5].fFree && rgplr[idPlayer].rgTech[2] >= 7) {
        FCreateAiShdef(5, ihuldefSuperFreighter, (uint8_t *)&vrgISAip[isRecipeSuperFreighter]);
    }
    if (rgshdef[14].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 6 && rgplr[idPlayer].rgTech[3] >= 4 &&
        rgplr[idPlayer].rgTech[2] >= 5) {
        for (i = 0; i < 4 && FCreateAiShdef(14, ihuldefDestroyer, (uint8_t *)&vrgISAip[vrgISIshAip[iisRecipeDestroyerTorpedo + Random(1)]]) == 0; i++) {
        }
    }
    if (rgshdef[1].fFree || rgshdef[1].cExist == 0) {
        if (!rgshdef[1].fFree) {
            shdef = rgshdef[1];
            shdef.fFree = TRUE;
            FChangeAiShdef(&shdef, 1);
        }
        FCreateAiShdef(1, ihuldefMediumFreighter, (uint8_t *)&vrgISAip[isRecipeColonizingMediumFreighter]);
    }
    if (rgshdef[0].fFree || rgshdef[0].cExist == 0) {
        if (!rgshdef[0].fFree) {
            shdef = rgshdef[0];
            shdef.fFree = TRUE;
            FChangeAiShdef(&shdef, 0);
        }
        FCreateAiShdef(0, ihuldefScout, (uint8_t *)&vrgISAip[isRecipeScoutStreamingBeam]);
    }
    if (rgshdef[6].fFree && rgplr[idPlayer].rgTech[3] >= 4 && rgplr[idPlayer].rgTech[2] >= 5 && rgplr[idPlayer].rgTech[5] >= 6) {
        FCreateAiShdef(6, ihuldefPrivateer, (uint8_t *)&vrgISAip[isRecipePrivateerMineLayer]);
    }
    if (rgshdef[2].fFree && rgplr[idPlayer].rgTech[1] >= 8 && rgplr[idPlayer].rgTech[4] >= 7 && rgplr[idPlayer].rgTech[3] >= 6 &&
        rgplr[idPlayer].rgTech[2] >= 7) {
        FCreateAiShdef(2, ihuldefB17Bomber, (uint8_t *)&vrgISAip[isRecipeB17Bomber]);
    }
    if (rgshdef[3].fFree && rgplr[idPlayer].rgTech[1] >= 11 && rgplr[idPlayer].rgTech[4] >= 12 && rgplr[idPlayer].rgTech[3] >= 15 &&
        rgplr[idPlayer].rgTech[2] >= 9) {
        FCreateAiShdef(3, ihuldefB52Bomber, (uint8_t *)&vrgISAip[isRecipeB52Bomber]);
    }
    if (rgshdef[9].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 6 && rgplr[idPlayer].rgTech[3] >= 13 &&
        rgplr[idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(9, ihuldefBattleship, (uint8_t *)&vrgISAip[vrgISIshAip[iisRecipeBattleshipBeamA + Random(4)]]) == 0; i++) {
        }
    }
    return;
}

void DoRototillAiTurn(PROD *rgprod) {
    int32_t  rgResCost[4];
    int32_t  rgResAvail[4];
    FLEET   *lpflEnemy;
    ORDER    ord;
    PLANET  *lpplDest;
    int16_t  cplNegative;
    THING   *lpthWorm;
    int16_t  fColonyShipInQueue;
    int16_t  idPlanDst;
    int16_t  cplBadGuy;
    PLANET  *lpplMac;
    PLANET  *lppl;
    PLANET  *lpplHome;
    FLEET   *lpfl;
    int16_t  ifl;
    int16_t  i;
    int16_t  iroCur;
    FLEET   *lpflT;
    FLEET   *lpflAttack;
    uint8_t  b;
    int16_t  ishdefSBLatest;
    int16_t  fBomberInQueue;
    uint16_t cplanCol;
    int16_t  j;
    PROD    *lpprod;
    int16_t  fWrite;
    int16_t  iPlanet;
    uint8_t  bT;

    iPlanet = rgplr[idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    fColonyShipInQueue = FALSE;
    fBomberInQueue = FALSE;
    iroCur = IroEnsureAi(NULL, 0, &ishdefSBLatest, game.turn >= 20 ? 15 : 0);
    EnsureCAShdefs(iroCur);
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome && PctPlanetOptValue(lppl, idPlayer) > 0) {
            cplanCol++;
        }
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome) {
            b = 0;
            for (i = 0; i < 3; i++) {
                if (lppl->rgMinConc[i] > 66) {
                    bT = 75;
                } else {
                    bT = (int16_t)lppl->rgMinConc[i] / 2;
                }
                b += bT;
            }
            if (b & 0x80) {
                b = 127;
            }
            vlpbAiPlanet[lppl->id * 16 + 1] = b;
        }
        if (lppl->iPlayer != idPlayer && lppl->iPlayer != iplrNone) {
            vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            if (PctPlanetOptValue(lppl, idPlayer) > 0) {
                vlpbAiPlanet[lppl->id * 16 + 3] = 1;
            }
            cplBadGuy++;
        } else if (lppl->iPlayer == idPlayer && PctPlanetDesirability(lppl, idPlayer) < 0) {
            cplNegative++;
            vlpbAiPlanet[lppl->id * 16 + 2] = 1;
        } else if (lppl->fStarbase && lppl->rgwtMin[3] >= 1000) {
            ChangeMainObjSel(grobjPlanet, lppl->id);
            InitProduction(rgprod);
            fWrite = FALSE;
            b = 0;
            i = 0;
            for (lpprod = lpplProdGlob->rgprod; i < lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem <= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < lpplProdGlob->iprodMac) {
                FinishProduction(FALSE);
            } else {
                if (game.turn == 0) {
                    AddItemToQueue(0, 1, grobjFleet, addItemEnd);
                    AddItemToQueue(0, 1, grobjFleet, addItemEnd);
                } else if (!fColonyShipInQueue && (rgshdef[1].cExist == 0 || rgshdef[1].cExist + 1 < (uint32_t)cplanCol)) {
                    fColonyShipInQueue = TRUE;
                    AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                    fWrite = TRUE;
                }
                FinishProduction(fWrite);
            }
        }
    }
    lpflAttack = NULL;
    lpflEnemy = NULL;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (FIsTurinDroneAiAttack(lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
            }
            lpfl->fMark = FALSE;
            if ((lpfl->rgcsh[7] != 0 || lpfl->rgcsh[8] != 0) && lpfl->cord >= 1) {
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    if (LpplFromId(lpfl->idPlanet)->iPlayer != iplrNone) {
                    LBlowAwayOrders:
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.cord = 1;
                        sel.fl.lpplord->iordMac = 1;
                        FLookupFleet(idWriteBack, &sel.fl);
                        ClearAiCurrentTask(lpfl, FALSE);
                        continue;
                    }
                    idPlanDst = lpfl->idPlanet;
                } else {
                    if (lpfl->cord <= 1)
                        continue;
                    idPlanDst = lpfl->lpplord->rgord[1].id;
                }
                vlpbAiPlanet[idPlanDst * 16 + 1] = vlpbAiPlanet[idPlanDst * 16 + 1] | 0x80;
                continue;
            }
            if (FIsAiTransport(lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst == idplNone)
                    continue;
                lppl = LpplFromId(idPlanDst);
                if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == idPlayer))
                    continue;
                if (vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                    memset(&ord, 0, sizeof(ORDER));
                    ord.pt = rgptPlan[idPlanDst];
                    ord.grobj = grobjPlanet;
                    ord.id = idPlanDst;
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = TRUE;
                    ord.txp.rgia[3].iAction = iActionUnloadAll;
                    ChangeMainObjSel(grobjFleet, lpfl->id);
                    if (sel.fl.lpplord->rgord[0].id == idPlanDst) {
                        sel.fl.lpplord->rgord[0] = ord;
                    } else {
                        sel.fl.lpplord->rgord[1] = ord;
                    }
                    FLookupFleet(idWriteBack, &sel.fl);
                    vlpbAiPlanet[idPlanDst * 16 + 3] = vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                    continue;
                }
                goto LBlowAwayOrders;
            } else {
                if (lpfl->rgcsh[1] == 0)
                    continue;
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
                if (idPlanDst == idplNone)
                    continue;
                if (vlpbAiPlanet[idPlanDst * 16 + 3] != 0)
                    goto LCheckForColDrop;
                lppl = LpplFromId(idPlanDst);
                if (lppl && lppl->iPlayer != iplrNone && lppl->iPlayer != idPlayer)
                    goto LBlowAwayOrders;
            }
        }
    }
    fMarkedPlanets = FALSE;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != idPlayer)
            continue;
        if (lpfl->rgcsh[7] != 0 || lpfl->rgcsh[8] != 0) {
            if (lpfl->idPlanet == idPlanetDeepSpace)
                continue;
            ChangeMainObjSel(grobjFleet, lpfl->id);
            b = vlpbAiPlanet[lpfl->idPlanet * 16 + 1];
            if (b >= 4)
                continue;
            lppl = LpplFindBestEnum(&sel.pl, FEnumCalcMinerDest);
            if (!lppl)
                continue;
            ord.id = lppl->id;
            ord.grobj = grobjPlanet;
            ord.pt = rgptPlan[lppl->id];
            ord.grTask = grTaskMine;
            ord.fValidTask = TRUE;
            ord.iWarp = 6;
            FMoveAiFleet(lpfl, &ord, TRUE);
            vlpbAiPlanet[lppl->id * 16 + 1] = vlpbAiPlanet[lppl->id * 16 + 1] | 0x80;
            vlpbAiPlanet[lpfl->idPlanet * 16 + 1] = vlpbAiPlanet[lpfl->idPlanet * 16 + 1] & 0x80;
            continue;
        }
        if (lpfl->cord > 1)
            continue;
        if (lpfl->rgcsh[1] == 0)
            goto LTryFreighters;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if ((lpfl->idPlanet == idPlanetDeepSpace || sel.pl.iPlayer != idPlayer || sel.pl.rgwtMin[3] < 50) && lpfl->rgwtMin[3] == 0) {
            if ((sel.fl.idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer && sel.pl.fStarbase) ||
                (rgshdef[1].hul.rghs[0].iItem >= iengineFuelMizer && FMoveToNearestStarbase(lpfl, FALSE)))
                continue;
        LScrapFleet:
            ChangeMainObjSel(grobjFleet, lpfl->id);
            sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
            FLookupFleet(idWriteBack, &sel.fl);
            continue;
        } else {
            lpthWorm = NULL;
            idPlanDst = IdNearestColonizablePlanet(lpfl, rgshdef[1].hul.rghs[0].iItem <= iengineQuickJump5 ? NULL : &lpthWorm);
            if (lpfl->idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 25);
                FLookupFleet(lpfl->id, &sel.fl);
            }
            if (idPlanDst != idplNone) {
                FColonizeAiFleet(lpfl, idPlanDst);
                vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
                continue;
            }
            if (!lpthWorm)
                continue;
            FGotoWormholeAiFleet(lpfl, lpthWorm);
            continue;
        }
    LTryFreighters:
        if (!FIsAiTransport(lpfl))
            goto LTryBombers;
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac && (lppl->iPlayer != idPlayer || !lppl->fStarbase); lppl++) {
        }
        lpplHome = lppl == lpplMac ? NULL : lppl;
        if (!lpplHome)
            goto BestSpeed;
        lppl = NULL;
        for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
            for (j = 0; j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter && ((AIHIST *)vlpbAiData)->rgasb[i].rgflid[j] != lpfl->id; j++) {
            }
            if (j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter)
                break;
        }
        if (i < ((AIHIST *)vlpbAiData)->cStarbase) {
            lppl = LpplFromId(((AIHIST *)vlpbAiData)->rgasb[i].idPlanet);
        }
        IdTargetFreighter(lpfl, !lppl ? lpplHome : lppl);
        continue;
    LTryBombers:
        if (lpfl->rgcsh[13] == 0 && lpfl->rgcsh[14] == 0)
            goto LTryScouts;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if (lpfl->idPlanet != idPlanetDeepSpace) {
            lppl = LpplFromId(lpfl->idPlanet);
            if (lppl->iPlayer == idPlayer) {
                if (lppl->fStarbase) {
                    if (lpfl->rgcsh[13] < 2 && lpfl->rgcsh[14] < 2)
                        continue;
                }
                FLookupFleet(lpfl->id, &sel.fl);
                goto LTargetBomber;
            }
            if (lppl->iPlayer == iplrNone)
                goto LTargetBomber;
            for (lpflT = lpflEnemy; lpflT; lpflT = lpflT->lpflNext) {
                if (lpfl->pt.x == lpflT->pt.x && lpfl->pt.y == lpflT->pt.y && FIsAiAttack(lpflT))
                    goto LTargetBomber;
            }
            continue;
        } else {
            lppl = lpplHome;
        }
    LTargetBomber:
        if (game.fAisBand) {
            lpplDest = LpplFindBestEnum(lppl, FEnumCalcArmadaHumanDest);
        } else {
            lpplDest = NULL;
        }
        if (!lpplDest) {
            lpplDest = LpplFindBestEnum(lppl, FEnumCalcArmadaDest);
        }
        lppl = lpplDest;
        if (!lppl)
            continue;
        vlpbAiPlanet[lppl->id * 16 + 10] = vlpbAiPlanet[lppl->id * 16 + 0xa] | 0x80;
        ord.id = lppl->id;
        ord.grobj = grobjPlanet;
        ord.pt = rgptPlan[lppl->id];
        ord.grTask = grTaskNone;
        ord.fValidTask = TRUE;
        ord.iWarp = 4;
        FMoveAiFleet(lpfl, &ord, FALSE);
        continue;
    LTryScouts:
        if (lpfl->rgcsh[0] == 0)
            goto LTryFighters;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if (lpfl->rgcsh[0] != 0 && rgshdef[0].hul.rghs[0].iItem == iengineQuickJump5 && lpfl->rgwtMin[4] < 2)
            goto LScrapFleet;
        IdTargetScout(lpfl, lpflAttack, lpflEnemy, game.fAisBand, &lpthWorm);
        continue;
    LTryFighters:;
    }
BestSpeed:
    HandleBasicAiTasks(iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue();
    return;
}

void EnsureCAShdefs(int16_t iroCur) { return; }
