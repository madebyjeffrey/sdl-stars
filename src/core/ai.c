#include "common.h"

void DoAiTurn(int16_t iPlayer, uint16_t wMdPlr) {
    char    szExt[4];
    PROD    rgprod[64];
    int16_t idSav;

    idSav = idPlayer;
    fAi = TRUE;
    CchSprintf(szExt, MPCTD, iPlayer + 1);
    DestroyCurGame();
    if (!FLoadGame(szBase, szExt)) {
        idPlayer = idSav;
        fAi = FALSE;
        return;
    }
    if (rgplr[idPlayer].fDead)
        goto Cleanup;
    vlpbAiPlanet = LpAlloc(game.cPlanMax * 16, htMisc);
    vrglpplAi = LpAlloc(game.cPlanMax * sizeof(PLANET *), htMisc);
    if (!vlpbAiData) {
        vlpbAiData = LpAlloc(0x2000, htMisc);
        if (vlpbAiData) {
            ((AIHIST *)vlpbAiData)->cbAiHist = 2;
        }
    }
    if (!vlpbAiPlanet || !vlpbAiData || !vrglpplAi)
        goto Cleanup;
    memset(vlpbAiPlanet, 0, game.cPlanMax * 16);
    ComputeShdefPowers();
    MarkPlanetsUnderAttack();
    IncreaseAIMinefieldSizes();
    InitRandomPlanetList();
    if (wMdPlr != 0xffff) {
        rgplr[iPlayer].wMdPlr = wMdPlr;
    }
    switch (rgplr[iPlayer].idAi) {
    case idAiRobotoid:
        DoRobotoidAiTurn(rgprod);
        break;
    case idAiCybertron:
        DoCyberAiTurn(rgprod);
        break;
    case idAiMacinti:
        DoMacintiAiTurn(rgprod);
        break;
    case idAiTurinDrone:
        DoTurinDroneAiTurn(rgprod);
        break;
    case idAiMaid:
        DoMaidAiTurn(rgprod);
        break;
    case idAiAutomitron:
        DoAutomitronAiTurn(rgprod);
        break;
    case idAiRototill:
        DoRototillAiTurn(rgprod);
        break;
    }
Cleanup:
    FWriteLogFile(szBase, iPlayer);
    FWriteHistFile(iPlayer);
    if (vrglpplAi) {
        FreeLp(vrglpplAi, htMisc);
        vrglpplAi = NULL;
    }
    if (vlpbAiData) {
        FreeLp(vlpbAiPlanet, htMisc);
        FreeLp(vlpbAiData, htMisc);
        vlpbAiPlanet = NULL;
        vlpbAiData = NULL;
    }
    idPlayer = idSav;
    fAi = FALSE;
    return;
}

void DoRobotoidAiTurn(PROD *rgprod) {
    int32_t  rgResCost[4];
    FLEET   *lpflEnemy;
    int32_t  rgResAvail[4];
    int16_t  cExistCargo;
    int16_t  cFlDestroyers;
    int16_t  iLatestDestroyer;
    int16_t  cColFleet;
    THING   *lpthWorm;
    int16_t  idPlanDst;
    int16_t  j;
    uint8_t  rgRecycleShdef[16];
    int16_t  fShouldColonize;
    PLANET  *lppl;
    int16_t  ifl;
    int16_t  i;
    uint16_t rgCosts[4];
    FLEET   *lpflAttack;
    FLEET   *lpfl;
    PLANET  *lpplHome;
    int16_t  cRes;
    int16_t  iroCur;
    int16_t  iAiLvl;
    int16_t  iLatestCargo;
    int16_t  ipl;
    int16_t  fTonsOfMinerals;
    int16_t  ishdefSBLatest;
    PLANET  *lpplMac;
    int16_t  iLatestMeta;
    uint16_t cRecyclePeriod;
    int16_t  cFr;
    int16_t  iLatestBattle;
    int16_t  iPlanet;
    int16_t  iLatestBomber;
    int32_t  l;
    int16_t  fWrite;
    PROD    *lpprod;
    uint8_t  rgRecycleSBShdef[16];
    int16_t  id;
    int16_t  iLatest;
    int16_t  dy;
    int32_t  lDist;
    int16_t  dx;
    ORDER    ord;
    uint8_t *lpb;
    PLANET  *lpplDrop;

    iAiLvl = rgplr[idPlayer].lvlAi;
    iPlanet = rgplr[idPlayer].idPlanetHome;
    iroCur = IroEnsureAi(vrgAiRobotoidResOrder, cAiRobotoidResOrder, &ishdefSBLatest, game.turn >= 10 ? 15 : 0);
    if (game.turn > 50) {
        MergeAllShdefs(1788);
        MergeAllShdefs(1);
        MergeAllShdefs(-16384);
    }
    j = 4;
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
    CheckAiShdefStatus(14, 15, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    for (i = 14; i <= 15; i++) {
        if (rgRecycleShdef[i] != 0 && !rgshdef[i].fFree && rgshdef[i].hul.ihuldef == ihuldefNubian) {
            rgRecycleShdef[i] = 0;
        }
    }
    cExistCargo = CheckAiShdefStatus(11, 13, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(9, 10, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(2, 5, cRecyclePeriod, &iLatestMeta, rgRecycleShdef);
    CheckAiShdefStatus(6, 7, (uint32_t)(3 * cRecyclePeriod) / 2, &iLatestBattle, rgRecycleShdef);
    if (game.turn > 80) {
        SplitOutShdefs(rgRecycleShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[0] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[1] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[13] = 2;
        rgRecycleSBShdef[12] = 2;
        rgRecycleSBShdef[11] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
    }
    EnsureRobotoidShdefs();
    cFlDestroyers = 0;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && iLatestDestroyer != ishdefNone && (lpfl->rgcsh[14] != 0 || lpfl->rgcsh[15] != 0)) {
            cFlDestroyers++;
        }
    }
    fShouldColonize = FShouldWeBuildColonizers(&cColFleet);
    UpdateProgressGauge(progressStep4);
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer != idPlayer && lppl->iPlayer != iplrNone) {
            i = lppl->uPopGuess / 250 + 1;
            if (i > 6) {
                i = 6;
            }
            if (lppl->fStarbase) {
                i++;
            }
            vlpbAiPlanet[lppl->id * 16 + 10] = i;
            vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        }
    }
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        if (lppl->fStarbase && lppl->rgwtMin[3] >= 200) {
            ChangeMainObjSel(grobjPlanet, lppl->id);
            InitProduction(rgprod);
            fWrite = FALSE;
            i = 0;
            for (lpprod = lpplProdGlob->rgprod; i < lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < lpplProdGlob->iprodMac) {
                FinishProduction(FALSE);
            } else {
                cFr =
                    rgplr[idPlayer].cPlanet / 8 <= ((AIHIST *)vlpbAiData)->cStarbase * 4 ? ((AIHIST *)vlpbAiData)->cStarbase * 4 : rgplr[idPlayer].cPlanet / 8;
                if (iLatestCargo != ishdefNone && (cExistCargo < (int16_t)(cFr * 8) / 10 || (cExistCargo < cFr && Random(3) == 0))) {
                    AddItemToQueue(iLatestCargo, 1, grobjFleet, addItemEnd);
                    fWrite = TRUE;
                }
                if (!fShouldColonize && (cColFleet > 25 || Random(((AIHIST *)vlpbAiData)->cStarbase * 8) != 0))
                    goto TryShip2;
                if (game.turn < 5)
                    goto TryShip2;
                if (game.turn <= 20) {
                    AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                }
                AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                fWrite = TRUE;
                l = (uint32_t)(lppl->rgwtMin[3] * PctTrueMaxGrowth(idPlayer));
                cRes = CResourcesAtPlanet(lppl, idPlayer);
                if (l > 2300 && cRes > 35 && iAiLvl > 0) {
                    AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                    if (l > 3600 && cRes > 50 && iAiLvl > 1) {
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                    }
                }
            TryShip2:
                if (rgshdef[0].hul.ihuldef == ihuldefFrigate && Random(4) == 0) {
                    id = lppl->id;
                    cFr = 0;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->rgcsh[0] > 0 && lpfl->iPlayer == idPlayer) {
                            cFr = lpfl->rgcsh[0];
                            break;
                        }
                    }
                    if ((cFr < 10 || (cFr < 17 && Random(10) == 0)) && Random(cFr * 2 + 1) == 0) {
                        AddItemToQueue(0, 4, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                }
                for (i = 0; i <= 2 && lppl->rgwtMin[i] >= 5000; i++) {
                }
                fTonsOfMinerals = i == 2;
                if (iLatestBomber != ishdefNone) {
                    id = lppl->id;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->iPlayer == idPlayer && FPotentRobWarFleet(lpfl, 2)) {
                            if (iLatestBomber == ishdefNone || lpfl->rgcsh[9] + lpfl->rgcsh[10] >= vrgAiArmadaPotency[2])
                                break;
                            AddItemToQueue(iLatestBomber, !fTonsOfMinerals ? 4 : 6, grobjFleet, addItemEnd);
                            fWrite = TRUE;
                            goto FinishProd;
                        }
                    }
                }
                if (iLatestMeta != ishdefNone && (rgshdef[iLatestMeta].cExist < (uint32_t)(game.cPlanMax / 7 + 6) || Random(2) != 0)) {
                    if (iLatestBattle != ishdefNone && Random(2) == 0) {
                        iLatest = iLatestBattle;
                    } else {
                        iLatest = iLatestMeta;
                    }
                    GetResourcesAvailable(lppl, rgResAvail);
                    GetProdQCost(lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    GetTrueHullCost(idPlayer, &rgshdef[iLatest].hul, rgCosts);
                    for (j = 0; j < 4; j++) {
                        rgResAvail[j] -= (uint32_t)((uint32_t)(3 * rgCosts[j]) / 5);
                        if (rgResAvail[j] < 0)
                            goto TryShip3;
                    }
                    AddItemToQueue(iLatest, !fTonsOfMinerals ? 1 : 5, grobjFleet, addItemEnd);
                    fWrite = TRUE;
                }
            TryShip3:
                if (iLatestDestroyer != ishdefNone && rgshdef[iLatestDestroyer].cExist < (uint32_t)(game.cPlanMax / 12 + 8)) {
                    GetResourcesAvailable(lppl, rgResAvail);
                    GetProdQCost(lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(idPlayer, &rgshdef[iLatestDestroyer].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= (uint32_t)rgCosts[j];
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = TRUE;
                        AddItemToQueue(iLatestDestroyer, 1, grobjFleet, addItemEnd);
                    }
                }
            FinishProd:
                FinishProduction(fWrite);
            }
        }
    }
    UpdateProgressGauge(progressStep4);
    lpflAttack = NULL;
    lpflEnemy = NULL;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjThing) {
            dx = lpfl->pt.x - lpfl->lpplord->rgord[1].pt.x;
            dy = lpfl->pt.y - lpfl->lpplord->rgord[1].pt.y;
            lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
            if (lDist > 40000) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                sel.fl.cord = 1;
                sel.fl.lpplord->iordMac = 1;
                FLookupFleet(idWriteBack, &sel.fl);
            }
        }
        if (lpfl->iPlayer != idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (lpfl->rgcsh[0] > 0 && game.turn > 40 && lpfl->cord == 1) {
                if (lpfl->rgcsh[0] >= 7 && Random(5) == 0 && (idPlanDst = IdRandomPlanetNearby(lpfl->pt, 105, TRUE)) != idplNone &&
                    idPlanDst != lpfl->idPlanet) {
                    ClearAiCurrentTask(lpfl, TRUE);
                    ord.id = idPlanDst;
                    ord.grobj = grobjPlanet;
                    ord.pt = rgptPlan[idPlanDst];
                    ord.grTask = grTaskNone;
                    ord.fValidTask = TRUE;
                    ord.iWarp = 4;
                    FMoveAiFleet(lpfl, &ord, FALSE);
                } else if (lpfl->lpplord->rgord[0].grTask == grTaskNone) {
                    ChangeMainObjSel(grobjFleet, lpfl->id);
                    sel.fl.lpplord->rgord[0].grTask = grTaskLayMines;
                    sel.fl.lpplord->rgord[0].tlm.cTime = 5;
                    sel.fl.lpplord->rgord[0].tlm.cTimeOld = 5;
                    FLookupFleet(idWriteBack, &sel.fl);
                    continue;
                }
            } else if (FIsAiAttack(lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
                if (lpfl->lpplord->rgord[0].grTask == grTaskLayMines) {
                    ClearAiCurrentTask(lpfl, TRUE);
                }
                for (j = 2; j <= 7 && lpfl->rgcsh[j] <= 0; j++) {
                }
                if (j <= 7 && ((lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet) || lpfl->idPlanet != idPlanetDeepSpace)) {
                    if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        id = lpfl->lpplord->rgord[1].id;
                    } else {
                        id = lpfl->idPlanet;
                    }
                    lpb = vlpbAiPlanet + (10 + 16 * id);
                    if (*lpb != 0) {
                        *lpb |= 0x80;
                    }
                }
            } else if (FIsAiTransport(lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord <= 1 || lpfl->lpplord->rgord[0].grTask != grTaskNone) {
                    idPlanDst = lpfl->idPlanet;
                } else if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                    idPlanDst = lpfl->lpplord->rgord[1].id;
                }
                if (idPlanDst != idplNone) {
                    lppl = LpplFromId(idPlanDst);
                    if ((!lppl || lppl->iPlayer != idPlayer) && lpfl->rgwtMin[3] == 0) {
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.cord = 1;
                        sel.fl.lpplord->iordMac = 1;
                        FLookupFleet(idWriteBack, &sel.fl);
                        ClearAiCurrentTask(lpfl, FALSE);
                    }
                }
            }
            if (game.turn <= 20 && lpfl->rgcsh[0] > 0) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
                FLookupFleet(idWriteBack, &sel.fl);
            } else if (lpfl->cord <= 1 && lpfl->rgcsh[1] != 0 && (game.turn >= 5 || game.mdStartDist == startDistClose)) {
                if (iAiLvl > 1 && lpfl->idPlanet != idPlanetDeepSpace) {
                    lpplDrop = LpplFromId(lpfl->idPlanet);
                    if (lpplDrop && lpplDrop->iPlayer != iplrNone && lpplDrop->iPlayer != idPlayer && lpfl->rgwtMin[3] > 0 &&
                        GetRaceStat(&rgplr[lpplDrop->iPlayer], rsMajorAdv) != raMacintosh) {
                        memset(&ord, 0, sizeof(ORDER));
                        ord.pt = rgptPlan[lpplDrop->id];
                        ord.id = lpplDrop->id;
                        ord.grobj = grobjPlanet;
                        ord.fValidTask = TRUE;
                        ord.grTask = grTaskXfer;
                        ord.txp.rgia[3].iAction = iActionUnloadAll;
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.lpplord->rgord[0] = ord;
                        FLookupFleet(idWriteBack, &sel.fl);
                        lpplDrop = LpplFindClosestEnum(lpplDrop, FEnumOurStarbase);
                        if (!lpplDrop)
                            continue;
                        memset(&ord, 0, sizeof(ORDER));
                        ord.id = lpplDrop->id;
                        ord.grobj = grobjPlanet;
                        ord.pt = rgptPlan[lpplDrop->id];
                        ord.grTask = grTaskNone;
                        ord.fValidTask = TRUE;
                        ord.iWarp = 4;
                        FMoveAiFleet(lpfl, &ord, FALSE);
                        continue;
                    }
                }
                idPlanDst = IdNearestColonizablePlanet(lpfl, &lpthWorm);
                if (idPlanDst == idplNone && !lpthWorm) {
                    if (lpfl->idPlanet != idPlanetDeepSpace) {
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
                        FLookupFleet(idWriteBack, &sel.fl);
                    }
                } else {
                    ChangeMainObjSel(grobjFleet, lpfl->id);
                    if (lpfl->idPlanet != idPlanetDeepSpace) {
                        XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 10);
                        FLookupFleet(lpfl->id, &sel.fl);
                    }
                    if (idPlanDst != idplNone) {
                        FColonizeAiFleet(lpfl, idPlanDst);
                    } else {
                        FGotoWormholeAiFleet(lpfl, lpthWorm);
                    }
                }
            }
        }
    }
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl] || lppl->fStarbase)
            break;
    }
    lpplHome = ipl == vclpplAi ? NULL : lppl;
    if (!lpplHome)
        goto AtkMissions;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && lpfl->cord <= 1 && FIsAiTransport(lpfl)) {
            if (lpfl->iplan != 4) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                sel.fl.iplan = 4;
                FLookupFleet(idWriteBack, &sel.fl);
            }
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
        }
    }
    UpdateProgressGauge(progressStep4);
AtkMissions:
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer) {
            for (i = 0; i < 16 && (lpfl->rgcsh[i] <= 0 || rgRecycleShdef[i] != 0); i++) {
            }
            if (i == 16) {
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == idPlayer && (lppl->fStarbase || Random(5) == 0)) {
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
                        FLookupFleet(idWriteBack, &sel.fl);
                        continue;
                    }
                }
                if ((lpfl->cord > 1 && lpfl->idPlanet == idPlanetDeepSpace && lpfl->lpplord->rgord[1].grobj == grobjPlanet) ||
                    FMoveToNearestStarbase(lpfl, FALSE))
                    continue;
            }
            for (i = 2; i <= 10; i++) {
                if (lpfl->rgcsh[i] > 0) {
                    IdTargetArmada(lpfl);
                    break;
                }
            }
            if (i > 10 && FIsAiAttack(lpfl) && (lpfl->cord <= 1 || lpfl->lpplord->rgord[1].grobj != grobjFleet) &&
                ((cFlDestroyers <= (game.turn <= 120 ? 70 : 50) && (cFlDestroyers <= (game.turn <= 120 ? 60 : 40) || Random(3) != 0)) ||
                 ((iLatestDestroyer != ishdefNone && lpfl->rgcsh[iLatestDestroyer] >= 20 && Random(20) != 0) || !FFindBuddyAndJoinUp(lpfl, 14, 15, 36, 72)))) {
                IdTargetAttack(lpfl, lpflAttack, lpflEnemy, game.fAisBand);
            }
        }
    }
    HandleBasicAiTasks(iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue();
    return;
}

void EnsureRobotoidShdefs() {
    int16_t ish;
    int16_t i;
    int16_t shBase;
    SHDEF   shdef;

    for (ish = 11; ish <= 13; ish++) {
        if (rgshdef[ish].fFree && rgplr[idPlayer].rgTech[2] >= 2 && rgplr[idPlayer].rgTech[3] >= (ish - 11) * 3 + 4 &&
            (ish == 11 || (uint16_t)(game.turn - rgshdef[ish - 1].turn) > 14)) {
            if (rgplr[idPlayer].rgTech[3] < 10) {
                FCreateAiShdef(ish, ihuldefPrivateer, (uint8_t *)&vrgRobAip[ish == 11 ? robRecipePrivateerBeam : robRecipePrivateerTorpedo]);
            } else {
                for (i = 0; i < 5 && FCreateAiShdef(ish, ihuldefMetaMorph, (uint8_t *)&vrgRobAip[vrgRobIshAip[irobRecipeMetaMorphCargoBeamA + Random(6)]]) == 0;
                     i++) {
                }
            }
        }
    }
    if (rgshdef[14].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 6 && rgplr[idPlayer].rgTech[3] >= 6 &&
        rgplr[idPlayer].rgTech[2] >= 6 && rgplr[idPlayer].rgTech[0] >= 2) {
        for (i = 0; i < 5 && (FCreateAiShdef(14, ihuldefNubian, (uint8_t *)&vrgRobAip[robRecipeNubianMixedBeams]) ||
                              !FCreateAiShdef(14, ihuldefDestroyer, (uint8_t *)&vrgRobAip[vrgRobIshAip[irobRecipeDestroyerBeamA + Random(4)]]));
             i++) {
        }
    }
    if (rgshdef[15].fFree && rgplr[idPlayer].rgTech[4] >= 10 && rgplr[idPlayer].rgTech[3] >= 8 && rgplr[idPlayer].rgTech[2] >= 9 &&
        rgplr[idPlayer].rgTech[1] >= 14 && !FCreateAiShdef(15, ihuldefNubian, (uint8_t *)&vrgRobAip[robRecipeNubianMixedBeams])) {
        for (i = 0; i < 5 && FCreateAiShdef(15, ihuldefDestroyer, (uint8_t *)&vrgRobAip[vrgRobIshAip[irobRecipeDestroyerTorpedoShield + Random(4)]]) == 0;
             i++) {
        }
    }
    for (ish = 2; ish <= 5; ish++) {
        if (rgshdef[ish].fFree && rgplr[idPlayer].rgTech[1] >= 10 && rgplr[idPlayer].rgTech[3] >= 10 && rgplr[idPlayer].rgTech[2] >= 9 &&
            rgplr[idPlayer].rgTech[0] >= 6 && (ish == 2 || (!rgshdef[ish - 1].fFree && (uint16_t)(game.turn - rgshdef[ish - 1].turn) > 12))) {
            shBase = !((ish - 2) & 1) ? irobRecipeMetaMorphBeamDefense : irobRecipeMetaMorphMissileA;
            for (i = 0; i < 5 && FCreateAiShdef(ish, ihuldefMetaMorph, (uint8_t *)&vrgRobAip[vrgRobIshAip[Random(4) + shBase]]) == 0; i++) {
            }
        }
    }
    for (ish = 6; ish <= 7; ish++) {
        if (rgshdef[ish].fFree && rgplr[idPlayer].rgTech[5] >= 4 && rgplr[idPlayer].rgTech[4] >= 10 && rgplr[idPlayer].rgTech[3] >= 12 &&
            rgplr[idPlayer].rgTech[2] >= 12 && rgplr[idPlayer].rgTech[0] >= 6 && rgplr[idPlayer].rgTech[1] >= 15 &&
            (ish == 6 || (!rgshdef[ish - 1].fFree && (uint16_t)(game.turn - rgshdef[ish - 1].turn) > 20))) {
            shBase = ish == 6 ? irobRecipeBattleshipMissileBeam : irobRecipeBattleshipBeamA;
            for (i = 0; i < 5 && FCreateAiShdef(ish, ihuldefBattleship, (uint8_t *)&vrgRobAip[vrgRobIshAip[Random(4) + shBase]]) == 0; i++) {
            }
        }
    }
    for (ish = 9; ish <= 10; ish++) {
        if (rgshdef[ish].fFree && rgplr[idPlayer].rgTech[1] >= 14 &&
            ((ish == 9 || (!rgshdef[ish - 1].fFree && (uint16_t)(game.turn - rgshdef[ish - 1].turn) > 15)) &&
             !FCreateAiShdef(ish, ihuldefBattleship, (uint8_t *)&vrgRobAip[robRecipeBattleshipMcm]))) {
            FCreateAiShdef(ish, ihuldefB52Bomber, (uint8_t *)&vrgRobAip[ish == 9 ? robRecipeB52SmartThenNormal : robRecipeB52RetroThenSmart]);
        }
    }
    if (rgshdef[0].hul.ihuldef != ihuldefFrigate && rgplr[idPlayer].lvlAi > lvlAiStandard && rgshdef[0].cExist == 0 && rgplr[idPlayer].rgTech[5] >= 4 &&
        rgplr[idPlayer].rgTech[4] >= 5 && rgplr[idPlayer].rgTech[3] >= 6 && rgplr[idPlayer].rgTech[2] >= 6 && rgplr[idPlayer].rgTech[0] >= 6) {
        shdef = rgshdef[0];
        shdef.fFree = TRUE;
        FChangeAiShdef(&shdef, 0);
        FCreateAiShdef(0, ihuldefFrigate, (uint8_t *)&vrgRobAip[robRecipeFrigateScoutMineLayer]);
    }
    return;
}

int16_t IdTargetArmada(FLEET *lpfl) {
    int16_t cshWar;
    FLEET  *lpflTarget;
    PLANET *lpplTarget;
    ORDER   ord;
    int16_t ish;
    PLANET *lppl;
    int32_t cCol;
    int16_t cshBomb;
    int32_t pctDef;
    int32_t lPopUs;
    int32_t lPopEnemy;
    int32_t cXfer;

    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord[1];
        if (LDistance2(lpfl->pt, ord.pt) > 62500 && ord.grobj == grobjFleet)
            goto LTryNewTarget;
        if (ord.grobj == grobjFleet) {
            return 0;
        }
        if (ord.grobj == grobjPlanet) {
            lppl = LpplFromId(ord.id);
            if (!lppl || ((lppl->iPlayer != iplrNone && (lppl->iPlayer != idPlayer || lppl->fStarbase)) || lppl->turn != game.turn)) {
                return 0;
            }
        }
    }
LTryNewTarget:
    cshWar = 0;
    for (ish = 2; ish <= 5; ish++) {
        cshWar += lpfl->rgcsh[ish];
    }
    for (ish = 6; ish <= 7; ish++) {
        cshWar += lpfl->rgcsh[ish] * 2;
    }
    cshBomb = lpfl->rgcsh[9] + lpfl->rgcsh[10];
    lpfl->fMark = TRUE;
    ChangeMainObjSel(grobjFleet, lpfl->id);
    if (lpfl->idPlanet == idPlanetDeepSpace) {
        MoveToNearestPlanetOrEnemy(lpfl, 150);
        return 0;
    }
    lppl = LpplFromId(lpfl->idPlanet);
    if (lppl->iPlayer == idPlayer && lppl->fStarbase) {
        if (cshWar >= vrgAiArmadaPotency[0] && cshBomb >= vrgAiArmadaPotency[2]) {
            if (sel.pl.rgwtMin[3] > 3000) {
                cCol = (int32_t)(sel.pl.rgwtMin[3] / 10);
            } else if (sel.pl.rgwtMin[3] > 2000) {
                cCol = (int32_t)(sel.pl.rgwtMin[3] / 15);
            } else if (sel.pl.rgwtMin[3] > 1000) {
                cCol = (int32_t)(sel.pl.rgwtMin[3] / 20);
            } else {
                cCol = 0;
            }
            if (cCol > 0) {
                XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, LOWORD(cCol));
                FLookupFleet(lpfl->id, &sel.fl);
            }
        TargetPotentArmada:
            if (game.fAisBand) {
                lpplTarget = LpplFindBestEnum(lppl, FEnumCalcArmadaHumanDest);
            } else {
                lpplTarget = NULL;
            }
            if (!lpplTarget) {
                lpplTarget = LpplFindBestEnum(lppl, FEnumCalcArmadaDest);
            }
        TargetEveryArmada:
            if (lpplTarget) {
                vlpbAiPlanet[lpplTarget->id * 16 + 10] = vlpbAiPlanet[lpplTarget->id * 16 + 0xa] | 0x80;
                ord.id = lpplTarget->id;
                ord.grobj = grobjPlanet;
                ord.pt = rgptPlan[lpplTarget->id];
            FinishTargeting:
                ord.grTask = grTaskNone;
                ord.fValidTask = TRUE;
                ord.iWarp = 4;
                if (!FMoveAiFleet(lpfl, &ord, FALSE)) {
                    return -1;
                }
            } else {
                lpflTarget = LpflFindClosestEnum(lpfl, FEnumCalcEnemyFleets);
                if (lpflTarget) {
                    ord.id = lpflTarget->id;
                    ord.grobj = grobjFleet;
                    ord.pt = lpflTarget->pt;
                    goto FinishTargeting;
                }
            }
        } else if (rgplr[idPlayer].lvlAi > lvlAiStandard && (cshWar > vrgAiArmadaPotency[0] * 2 || cshWar >= 60)) {
            if (Random(10) < 5 || (cshWar > vrgAiArmadaPotency[0] * 3 && Random(10) < 7) || (cshWar > 120 && Random(10) < 7))
                goto TargetPotentArmada;
        }
    } else if (cshWar < vrgAiArmadaPotency[1] || cshBomb < vrgAiArmadaPotency[3]) {
        ClearAiCurrentTask(lpfl, FALSE);
        if (rgplr[idPlayer].lvlAi > lvlAiStandard && ((cshWar > vrgAiArmadaPotency[0] * 2 && Random(10) < 5) ||
                                                      (cshWar > vrgAiArmadaPotency[0] * 4 && Random(10) < 7) || (cshWar > 120 && Random(10) < 7)))
            goto TargetPotentArmada;
        lpplTarget = LpplFindClosestEnum(lppl, FEnumOurStarbase);
        goto TargetEveryArmada;
    } else if (lppl->iPlayer == iplrNone) {
        goto TargetPotentArmada;
    } else if (lppl->iPlayer == idPlayer) {
        if (lppl->rgwtMin[3] > 1000) {
            XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, (int16_t)LOWORD(lppl->rgwtMin[3]) / 5);
            FLookupFleet(lpfl->id, &sel.fl);
        }
        goto TargetPotentArmada;
    } else if (lppl->iPlayer != idPlayer) {
        lPopUs = lpfl->rgwtMin[3];
        lPopEnemy = (int32_t)(lppl->uPopGuess * 4);
        pctDef = (uint32_t)(lppl->uDefGuess * 6) + 6;
        pctDef = (int32_t)(pctDef * 3) / 4;
        lPopEnemy = (int32_t)((int32_t)(lPopEnemy * 100) / (100 - pctDef));
        if (lPopEnemy < (int32_t)(lPopUs / 5) || (lPopEnemy < 200 && lPopUs > 350) || (lPopEnemy < 10 && lPopUs > 150)) {
            cXfer = (int32_t)(lPopEnemy * 5) / 4;
            if (lpfl->rgwtMin[3] < (cXfer <= (int32_t)(lpfl->rgwtMin[3] / 2) ? (int32_t)(lpfl->rgwtMin[3] / 2) : cXfer)) {
                cXfer = lpfl->rgwtMin[3];
            } else if (cXfer <= (int32_t)(lpfl->rgwtMin[3] / 2)) {
                cXfer = (int32_t)(lpfl->rgwtMin[3] / 2);
            }
            if (cXfer > 30000) {
                cXfer = 30000;
            }
            XferAiTroopers(lpfl->id, lppl->id, LOWORD(cXfer));
            FLookupFleet(lpfl->id, &sel.fl);
        }
    }
    return 0;
}

int16_t FPotentRobWarFleet(FLEET *lpfl, int16_t iPotency) {
    int16_t ish;
    int16_t cEquiv;

    cEquiv = 0;
    for (ish = 2; ish <= 5; ish++) {
        cEquiv += lpfl->rgcsh[ish];
    }
    for (ish = 6; ish <= 7; ish++) {
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

int16_t FEnumCalcEnemyFleets(FLEET *lpflSrc, FLEET *lpflTest) {
    if (lpflTest->iPlayer != idPlayer) {
        return TRUE;
    }
    return FALSE;
}

int16_t FEnumCalcArmadaDest(PLANET *lpplSrc, PLANET *lpplTest) {
    int16_t id;
    uint8_t b;
    int32_t l2;

    if (lpplSrc == lpplTest) {
        return FALSE;
    }
    id = lpplTest->id;
    b = vlpbAiPlanet[id * 16 + 10];
    if (b != 0) {
        l2 = LDistance2(rgptPlan[lpplSrc->id], rgptPlan[lpplTest->id]);
        if (l2 < 2500) {
            b += 7;
        } else if (l2 < 10000) {
            b += 5;
        } else if (l2 < 22500) {
            b += 4;
        } else if (l2 < 40000) {
            b += 3;
        } else if (l2 < 90000) {
            b += 2;
        } else if (l2 < 250000) {
            b++;
        }
        if (!(b & 0x80) || Random(4) == 0) {
            return b;
        }
    }
    return FALSE;
}

int16_t FEnumCalcArmadaHumanDest(PLANET *lpplSrc, PLANET *lpplTest) {
    int16_t id;
    uint8_t b;
    int32_t l2;

    if (lpplSrc == lpplTest) {
        return FALSE;
    }
    id = lpplTest->id;
    if (rgplr[lpplTest->iPlayer].fAi) {
        return FALSE;
    }
    b = vlpbAiPlanet[id * 16 + 10];
    if (b != 0) {
        l2 = LDistance2(rgptPlan[lpplSrc->id], rgptPlan[lpplTest->id]);
        if (l2 < 2500) {
            b += 7;
        } else if (l2 < 10000) {
            b += 5;
        } else if (l2 < 22500) {
            b += 4;
        } else if (l2 < 40000) {
            b += 3;
        } else if (l2 < 90000) {
            b += 2;
        } else if (l2 < 250000) {
            b++;
        }
        if (!(b & 0x80) || Random(4) == 0) {
            return b;
        }
    }
    return FALSE;
}

void DoTurinDroneAiTurn(PROD *rgprod) {
    int32_t  rgResCost[4];
    int16_t  iLatestCruiser;
    int32_t  rgResAvail[4];
    FLEET   *lpflEnemy;
    int16_t  cExistCargo;
    int16_t  iLatestDestroyer;
    THING   *lpthWorm;
    PLANET  *lpplDest;
    uint8_t  rgRecycleShdef[16];
    int16_t  idPlanDst;
    int16_t  j;
    int16_t  iLatestLayer;
    ORDER    ord;
    PLANET  *lppl;
    int16_t  iLatestTroop;
    uint16_t rgCosts[4];
    PLANET  *lpplHome;
    FLEET   *lpflAttack;
    FLEET   *lpfl;
    int16_t  ifl;
    int16_t  i;
    FLEET   *lpflT;
    uint8_t  b;
    int16_t  cRes;
    int16_t  iroCur;
    int16_t  iLatestMiner;
    int16_t  iLatestCargo;
    int16_t  cplMiners;
    PLANET  *lpplMac;
    int16_t  ishdefSBLatest;
    int16_t  cplNegative;
    int16_t  ipl;
    uint16_t cRecyclePeriod;
    uint16_t cplanCol;
    int16_t  cFr;
    int16_t  cplBadGuy;
    int16_t  iLatestBattle;
    int16_t  iLatestBomber;
    int32_t  l;
    PROD    *lpprod;
    int16_t  fWrite;
    int16_t  iPlanet;
    uint8_t  bT;
    int16_t  pct;
    int16_t  id;

    iPlanet = rgplr[idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    cplMiners = 0;
    iroCur = IroEnsureAi(vrgAiTurinDroneResOrder, cAiTurinDroneResOrder, &ishdefSBLatest, 15);
    if (!rgshdef[13].fFree) {
        MergeAllShdefs(-7952);
    }
    if (!rgshdef[12].fFree) {
        MergeAllShdefs(4096);
    }
    if (!rgshdef[10].fFree) {
        MergeAllShdefs(3072);
    }
    if (!rgshdef[2].fFree) {
        MergeAllShdefs(12);
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
    CheckAiShdefStatus(6, 7, cRecyclePeriod, &iLatestCruiser, rgRecycleShdef);
    cExistCargo = CheckAiShdefStatus(8, 9, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(13, 14, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(4, 5, cRecyclePeriod, &iLatestBattle, rgRecycleShdef);
    CheckAiShdefStatus(12, 12, cRecyclePeriod, &iLatestLayer, rgRecycleShdef);
    CheckAiShdefStatus(15, 15, cRecyclePeriod, &iLatestTroop, rgRecycleShdef);
    if (rgplr[idPlayer].rgTech[3] >= 7) {
        CheckAiShdefStatus(2, 3, cRecyclePeriod, &iLatestMiner, rgRecycleShdef);
    } else {
        iLatestMiner = ishdefNone;
    }
    CheckAiShdefStatus(10, 11, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    if (game.turn > 60) {
        SplitOutShdefs(rgRecycleShdef);
    }
    EnsureTurinDroneShdefs(iroCur);
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
            cplMiners++;
        }
        if (lppl->iPlayer != idPlayer && lppl->iPlayer != iplrNone) {
            vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            pct = PctPlanetOptValue(lppl, idPlayer);
            if (pct > 0) {
                vlpbAiPlanet[lppl->id * 16 + 3] = pct;
                cplBadGuy++;
            }
        } else if (lppl->iPlayer == idPlayer) {
            if (PctPlanetDesirability(lppl, idPlayer) < 0) {
                cplNegative++;
                vlpbAiPlanet[lppl->id * 16 + 2] = 1;
            } else if (!lppl->fStarbase || lppl->rgwtMin[3] < 200) {
                ChangeMainObjSel(grobjPlanet, lppl->id);
                sel.pl.fNoResearch = lppl->rgwtMin[3] < 200;
                if ((uint32_t)sel.pl.fNoResearch != lppl->fNoResearch) {
                    FLookupPlanet(idWriteBack, &sel.pl);
                }
            } else {
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
                    if (game.turn == 0) {
                        i = game.cPlanMax;
                        while (i > 0) {
                            AddItemToQueue(0, 1, grobjFleet, addItemEnd);
                            if (i > 190) {
                                i -= 100;
                            } else {
                                i -= 30;
                            }
                            fWrite = TRUE;
                        }
                    } else if (!rgshdef[0].fFree && rgshdef[0].hul.ihuldef == ihuldefFrigate &&
                               rgshdef[0].cExist < (uint32_t)(game.cPlanMax / 4 >= 32 ? 32 : game.cPlanMax / 4) &&
                               rgshdef[0].cExist > (uint32_t)(rgshdef[0].cBuilt / 10)) {
                        AddItemToQueue(0, 1, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                    cFr = rgplr[idPlayer].cPlanet / 10 <= ((AIHIST *)vlpbAiData)->cStarbase * 2 ? ((AIHIST *)vlpbAiData)->cStarbase * 2
                                                                                                : rgplr[idPlayer].cPlanet / 10;
                    if (rgplr[idPlayer].rgTech[2] >= 5 && iLatestCargo != ishdefNone &&
                        (cExistCargo < cFr || (cExistCargo < (int16_t)(10 * cFr) / 7 && Random(4) == 0))) {
                        AddItemToQueue(iLatestCargo, 1, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                    if ((cplanCol != 0 || cplBadGuy != 0) && rgshdef[1].cExist < 2) {
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                    l = (uint32_t)(lppl->rgwtMin[3] * PctTrueMaxGrowth(idPlayer));
                    cRes = CResourcesAtPlanet(lppl, idPlayer);
                    if (!rgshdef[12].fFree && Random(3) == 0) {
                        id = lppl->id;
                        cFr = 0;
                        for (ifl = 0; ifl < cFleet; ifl++) {
                            lpfl = rglpfl[ifl];
                            if (!rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->rgcsh[12] > 0 && lpfl->iPlayer == idPlayer) {
                                cFr = lpfl->rgcsh[12];
                                break;
                            }
                        }
                        if ((cFr < 10 || (cFr < 17 && Random(8) == 0)) && Random(cFr * 2 + 1) == 0) {
                            AddItemToQueue(12, 3, grobjFleet, addItemEnd);
                            fWrite = TRUE;
                        }
                    }
                    if (iLatestBomber != ishdefNone) {
                        id = lppl->id;
                        for (ifl = 0; ifl < cFleet; ifl++) {
                            lpfl = rglpfl[ifl];
                            if (!rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->iPlayer == idPlayer && FPotentRobWarFleet(lpfl, 2)) {
                                if (iLatestBomber == ishdefNone || lpfl->rgcsh[13] + lpfl->rgcsh[14] < vrgAiArmadaPotency[2])
                                    break;
                                AddItemToQueue(iLatestBomber, 4, grobjFleet, addItemEnd);
                                fWrite = TRUE;
                                goto FinishProd;
                            }
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
                    if (iLatestDestroyer != ishdefNone && rgshdef[iLatestDestroyer].cExist < (uint32_t)(game.cPlanMax / 4 + 12)) {
                        GetResourcesAvailable(lppl, rgResAvail);
                        GetProdQCost(lppl, rgResCost);
                        for (i = 0; i < 4; i++) {
                            rgResAvail[i] -= rgResCost[i];
                            if (rgResAvail[i] < 0)
                                goto FinishProd;
                        }
                        for (i = 0; i < 5; i++) {
                            GetTrueHullCost(idPlayer, &rgshdef[iLatestDestroyer].hul, rgCosts);
                            for (j = 0; j < 4; j++) {
                                rgResAvail[j] -= (uint32_t)rgCosts[j];
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = TRUE;
                            AddItemToQueue(iLatestDestroyer, 1, grobjFleet, addItemEnd);
                        }
                    }
                    if (iLatestTroop != ishdefNone && !rgshdef[15].fFree && rgshdef[iLatestTroop].cExist < (uint32_t)(game.cPlanMax / 12 + 8)) {
                        GetResourcesAvailable(lppl, rgResAvail);
                        GetProdQCost(lppl, rgResCost);
                        for (i = 0; i < 4; i++) {
                            rgResAvail[i] -= rgResCost[i];
                            if (rgResAvail[i] < 0)
                                goto FinishProd;
                        }
                        for (i = 0; i < 5; i++) {
                            GetTrueHullCost(idPlayer, &rgshdef[iLatestTroop].hul, rgCosts);
                            for (j = 0; j < 4; j++) {
                                rgResAvail[j] -= (uint32_t)rgCosts[j];
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = TRUE;
                            AddItemToQueue(iLatestTroop, 1, grobjFleet, addItemEnd);
                        }
                    }
                FinishProd:
                    FinishProduction(fWrite);
                }
            }
        }
    }
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl] || lppl->fStarbase)
            break;
    }
    lpplHome = lppl == lpplMac ? NULL : lppl;
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
            if ((lpfl->rgcsh[2] != 0 || lpfl->rgcsh[3] != 0) && lpfl->lpplord->rgord[lpfl->cord - 1].grTask != grTaskNone) {
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
                    lpfl->lpplord->rgord[1].grTask = grTaskMine;
                    if (lpfl->lpplord->rgord[1].grobj != grobjPlanet) {
                    }
                    idPlanDst = lpfl->lpplord->rgord[1].id;
                }
                vlpbAiPlanet[idPlanDst * 16 + 1] = vlpbAiPlanet[idPlanDst * 16 + 1] | 0x80;
                continue;
            }
            if (lpfl->rgcsh[8] != 0 || lpfl->rgcsh[9] != 0) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst != idplNone) {
                    lppl = LpplFromId(idPlanDst);
                    if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == idPlayer))
                        continue;
                    if (vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh &&
                        !lppl->fStarbase) {
                        memset(&ord, 0, sizeof(ORDER));
                        ord.pt = rgptPlan[idPlanDst];
                        ord.grobj = grobjPlanet;
                        ord.id = idPlanDst;
                        ord.grTask = grTaskXfer;
                        ord.fValidTask = TRUE;
                        ord.txp.rgia[3].iAction = iActionUnloadAll;
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        if (sel.fl.lpplord->rgord[0].id == idPlanDst && sel.fl.lpplord->rgord[0].grobj == grobjPlanet) {
                            sel.fl.lpplord->rgord[0] = ord;
                        } else {
                            sel.fl.lpplord->rgord[1] = ord;
                        }
                        FLookupFleet(idWriteBack, &sel.fl);
                        vlpbAiPlanet[idPlanDst * 16 + 3] = vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                        FMoveToNearestStarbase(lpfl, FALSE);
                        continue;
                    }
                    goto LBlowAwayOrders;
                } else if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjOther) {
                    goto LBlowAwayOrders;
                }
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
        if (lpfl->rgcsh[2] != 0 || lpfl->rgcsh[3] != 0) {
            if (game.turn == 0)
                goto LScrapFleet;
            if (lpfl->idPlanet == idPlanetDeepSpace)
                continue;
            ChangeMainObjSel(grobjFleet, lpfl->id);
            b = vlpbAiPlanet[lpfl->idPlanet * 16 + 1];
            if (b >= 4)
                continue;
            lppl = LpplFindBestEnum(&sel.pl, FEnumCalcMinerDest);
            if (!lppl)
                continue;
            memset(&ord, 0, sizeof(ORDER));
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
                (rgshdef[1].hul.rghs[0].iItem > iengineFuelMizer && FMoveToNearestStarbase(lpfl, FALSE)))
                continue;
        LScrapFleet:
            ChangeMainObjSel(grobjFleet, lpfl->id);
            sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
            FLookupFleet(idWriteBack, &sel.fl);
            continue;
        } else {
            lpthWorm = NULL;
            idPlanDst = IdNearestColonizablePlanet(lpfl, NULL);
            if (lpfl->idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer) {
                ChangeMainObjSel(grobjFleet, lpfl->id);
                XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 25);
                FLookupFleet(lpfl->id, &sel.fl);
                lppl = LpplFromId(lpfl->idPlanet);
            } else {
                lppl = lpplHome;
            }
            if (idPlanDst != idplNone) {
                FColonizeAiFleet(lpfl, idPlanDst);
                vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
                continue;
            }
            if (lppl) {
                lpplDest = LpplFindClosestEnum(lppl, FEnumCalcColonistDrop);
                if (lpplDest) {
                    vlpbAiPlanet[lpplDest->id * 16 + 10] = vlpbAiPlanet[lpplDest->id * 16 + 0xa] | 0x80;
                    memset(&ord, 0, sizeof(ORDER));
                    ord.id = lpplDest->id;
                    ord.grobj = grobjPlanet;
                    ord.pt = rgptPlan[lpplDest->id];
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = TRUE;
                    ord.iWarp = 6;
                    ord.txp.rgia[3].iAction = iActionUnloadAll;
                    FMoveAiFleet(lpfl, &ord, FALSE);
                    continue;
                }
            }
            if (!lpthWorm || Random(100) >= 10)
                continue;
            FGotoWormholeAiFleet(lpfl, lpthWorm);
            continue;
        }
    LTryFreighters:
        if (lpfl->rgcsh[8] == 0 && lpfl->rgcsh[9] == 0)
            goto LTryBombers;
        if (game.turn == 0) {
            ChangeMainObjSel(grobjFleet, lpfl->id);
            goto LScrapFleet;
        }
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
                    if (lpfl->rgcsh[13] + lpfl->rgcsh[14] < vrgAiArmadaPotency[2] || lpfl->rgcsh[4] + lpfl->rgcsh[5] < vrgAiArmadaPotency[1])
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
        if (lpfl->rgcsh[0] == 0 && lpfl->rgcsh[10] == 0 && lpfl->rgcsh[11] == 0)
            goto LTryMinelayers;
        if (lpfl->rgcsh[0] != 0 && rgplr[idPlayer].rgTech[3] >= 6 && rgshdef[0].hul.ihuldef == ihuldefScout)
            goto LScrapFleet;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        IdTargetScout(lpfl, lpflAttack, lpflEnemy, game.fAisBand, &lpthWorm);
        continue;
    LTryMinelayers:
        if (lpfl->rgcsh[12] == 0)
            continue;
        if (lpfl->cord != 1 || lpfl->lpplord->rgord[0].grTask != grTaskNone)
            continue;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        sel.fl.lpplord->rgord[0].grTask = grTaskLayMines;
        sel.fl.lpplord->rgord[0].tlm.cTime = 5;
        sel.fl.lpplord->rgord[0].tlm.cTimeOld = 5;
        FLookupFleet(idWriteBack, &sel.fl);
        continue;
    }
BestSpeed:
    HandleBasicAiTasks(iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue();
    return;
}

void EnsureTurinDroneShdefs(int16_t iroCur) {
    SHDEF   shdef;
    int16_t i;

    if (rgshdef[8].fFree && rgplr[idPlayer].rgTech[2] >= 5 && rgplr[idPlayer].rgTech[3] >= 8) {
        FCreateAiShdef(8, ihuldefRogue, (uint8_t *)&vrgTDAip[tdRecipeRogueCargoScanner]);
    }
    if (rgshdef[9].fFree && rgplr[idPlayer].rgTech[2] >= 7 && rgplr[idPlayer].rgTech[3] >= 11) {
        FCreateAiShdef(9, ihuldefGalleon, (uint8_t *)&vrgTDAip[tdRecipeGalleonCargoScanner]);
    }
    if (rgshdef[10].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 5 && rgplr[idPlayer].rgTech[3] >= 4 &&
        rgplr[idPlayer].rgTech[2] >= 5) {
        for (i = 0; i < 4 && FCreateAiShdef(10, ihuldefDestroyer, (uint8_t *)&vrgTDAip[vrgTDIshAip[itdRecipeDestroyerTorpedo + Random(1)]]) == 0; i++) {
        }
    }
    if (rgshdef[1].fFree || rgshdef[1].cExist == 0) {
        if (!rgshdef[1].fFree && rgshdef[1].hul.ihuldef != ihuldefPrivateer) {
            shdef = rgshdef[1];
            shdef.fFree = TRUE;
            FChangeAiShdef(&shdef, 1);
        }
        FCreateAiShdef(1, ihuldefColonyShip, (uint8_t *)&vrgTDAip[tdRecipeColonizer]);
    }
    if (rgshdef[0].fFree || rgshdef[0].cExist == 0) {
        if (!rgshdef[0].fFree) {
            shdef = rgshdef[0];
            shdef.fFree = TRUE;
            FChangeAiShdef(&shdef, 0);
        }
        FCreateAiShdef(0, ihuldefFrigate, (uint8_t *)&vrgTDAip[tdRecipeFrigateScout]);
    }
    if ((rgshdef[2].fFree || rgshdef[2].cExist == 0) && rgplr[idPlayer].rgTech[3] >= 7 && rgplr[idPlayer].rgTech[4] >= 4) {
        if (!rgshdef[2].fFree) {
            shdef = rgshdef[2];
            shdef.fFree = TRUE;
            FChangeAiShdef(&shdef, 2);
        }
        FCreateAiShdef(2, ihuldefMiner, (uint8_t *)&vrgTDAip[tdRecipeMiner]);
    }
    if ((rgshdef[12].fFree || rgshdef[12].cExist == 0) && rgplr[idPlayer].rgTech[3] >= 4 && rgplr[idPlayer].rgTech[5] >= 4) {
        FCreateAiShdef(12, ihuldefPrivateer, (uint8_t *)&vrgTDAip[tdRecipePrivateerMineLayer]);
    }
    if (rgshdef[13].fFree && rgplr[idPlayer].rgTech[1] >= 8 && rgplr[idPlayer].rgTech[4] >= 7 && rgplr[idPlayer].rgTech[3] >= 6) {
        FCreateAiShdef(13, ihuldefStealthBomber, (uint8_t *)&vrgTDAip[tdRecipeStealthBomber]);
    }
    if (rgshdef[14].fFree && rgplr[idPlayer].rgTech[1] >= 11 && rgplr[idPlayer].rgTech[4] >= 12 && rgplr[idPlayer].rgTech[3] >= 15 &&
        rgplr[idPlayer].rgTech[2] >= 9) {
        FCreateAiShdef(14, ihuldefStealthBomber, (uint8_t *)&vrgTDAip[tdRecipeStealthBomber]);
    }
    if (rgshdef[4].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 6 && rgplr[idPlayer].rgTech[3] >= 13 &&
        rgplr[idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(4, ihuldefBattleship, (uint8_t *)&vrgTDAip[vrgTDIshAip[itdRecipeBattleshipBeamA + Random(4)]]) == 0; i++) {
        }
    }
    if (rgshdef[15].fFree && rgplr[idPlayer].rgTech[1] >= 5 && rgplr[idPlayer].rgTech[4] >= 6 && rgplr[idPlayer].rgTech[3] >= 13 &&
        rgplr[idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(15, ihuldefRogue, (uint8_t *)&vrgTDAip[tdRecipeRogueCargoScanner]) == 0; i++) {
        }
    }
    return;
}

int16_t FEnumCalcMinerDest(PLANET *lpplSrc, PLANET *lpplTest) {
    int16_t id;
    uint8_t b;

    if (lpplSrc == lpplTest) {
        return FALSE;
    }
    id = lpplTest->id;
    b = vlpbAiPlanet[id * 16 + 1];
    if (b != 0 && (Random(100) < 25 || !(b & 0x80))) {
        return b;
    }
    return FALSE;
}

int16_t FEnumCalcColonistDrop(PLANET *lpplSrc, PLANET *lpplTest) {
    int16_t id;
    uint8_t bWant;
    uint8_t bEnemy;

    if (lpplSrc == lpplTest) {
        return FALSE;
    }
    id = lpplTest->id;
    bEnemy = vlpbAiPlanet[lpplTest->id * 16 + 10];
    bWant = vlpbAiPlanet[lpplTest->id * 16 + 3];
    if (bEnemy == 1 && bWant != 0 && (!(bEnemy & 0x80) || Random(100) < 25)) {
        if (GetRaceStat(&rgplr[lpplTest->iPlayer], rsMajorAdv) == raMacintosh) {
            return FALSE;
        }
        return bWant;
    }
    return FALSE;
}
