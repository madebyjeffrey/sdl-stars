#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "production_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "race_query_io.h"
#include "random_io.h"

void DoMaidAiTurn(APPSESSION &app, PROD *rgprod) {
    int32_t rgResCost[4];
    int32_t rgResAvail[4];
    int16_t iroCur;

    iroCur = IroEnsureAi(app, nullptr, 0, nullptr, app.state.game.turn >= 20 ? 15 : 0);
    app.state.fMarkedPlanets = 0;
    HandleBasicAiTasks(app, iroCur, rgprod, ishdefNone, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

int16_t FPotentISWarFleet(APPSESSION &app, FLEET *lpfl, int16_t iPotency) {
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
        return 1;
    }
    if (cEquiv >= app.state.vrgAiArmadaPotency[0]) {
        return 1;
    }
    return 0;
}

void DoAutomitronAiTurn(APPSESSION &app, PROD *rgprod) {
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

    iPlanet = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    iroCur = IroEnsureAi(app, vrgAiISResOrder, cAiISResOrder, &ishdefSBLatest, app.state.game.turn >= 10 ? 20 : 0);
    if (app.state.game.turn > 50) {
        MergeAllShdefs(app, 7692);
        MergeAllShdefs(app, 64);
        MergeAllShdefs(app, 16384);
    } else if (app.state.game.turn > 30) {
        MergeAllShdefs(app, 16384);
    }
    j = 3;
    if (app.state.game.turn > 130) {
        j += static_cast<uint32_t>((app.state.game.turn - 120)) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    app.state.vrgAiArmadaPotency[0] = j;
    app.state.vrgAiArmadaPotency[1] = static_cast<int16_t>((j & 0xff)) / 2;
    j = 6;
    if (app.state.game.turn > 115) {
        j += static_cast<uint32_t>((app.state.game.turn - 100)) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    app.state.vrgAiArmadaPotency[2] = j;
    app.state.vrgAiArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    memset(rgRecycleShdef, 0, 16);
    if (app.state.game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = app.state.game.turn >= 200 ? 100 : 70;
    }
    CheckAiShdefStatus(app, 11, 12, cRecyclePeriod, &iLatestCruiser, rgRecycleShdef);
    cExistCargo = CheckAiShdefStatus(app, 4, 5, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(app, 2, 3, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(app, 9, 10, cRecyclePeriod, &iLatestBattle, rgRecycleShdef);
    if (app.state.game.turn > 60) {
        SplitOutShdefs(app, rgRecycleShdef);
    }
    EnsureISShdefs(app, iroCur);
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome && PctPlanetOptValue(app, lppl, app.state.idPlayer) > 0) {
            cplanCol++;
        }
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        app.state.vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        if (lppl->iPlayer != app.state.idPlayer && lppl->iPlayer != iplrNone) {
            app.state.vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            if (PctPlanetOptValue(app, lppl, app.state.idPlayer) > 0) {
                app.state.vlpbAiPlanet[lppl->id * 16 + 3] = 1;
            }
            cplBadGuy++;
        } else if (lppl->iPlayer == app.state.idPlayer && PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer) < 0) {
            cplNegative++;
            app.state.vlpbAiPlanet[lppl->id * 16 + 2] = 1;
        } else if (lppl->fStarbase && lppl->rgwtMin[3] >= 1500) {
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            ProductionIO().pfnInitialize(app, rgprod);
            fWrite = 0;
            b = 0;
            i = 0;
            for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < app.state.lpplProdGlob->iprodMac) {
                FinishProduction(app, 0);
            } else {
                cFr = app.state.rgplr[app.state.idPlayer].cPlanet / 10 <= AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 2 ? AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 2
                                                                                            : app.state.rgplr[app.state.idPlayer].cPlanet / 10;
                if (app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5 && iLatestCargo != ishdefNone &&
                    (cExistCargo < cFr || (cExistCargo < static_cast<int16_t>((10 * cFr)) / 7 && RandomIO().pfnRandom(app, 4) == 0))) {
                    AddItemToQueue(app, iLatestCargo, 1, grobjFleet, addItemEnd);
                    fWrite = 1;
                }
                if (cplanCol != 0 && app.state.rgshdef[1].cExist == 0 && app.state.game.turn > 10) {
                    AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                    fWrite = 1;
                }
                l = static_cast<uint32_t>((lppl->rgwtMin[3] * PctTrueMaxGrowth(app, app.state.idPlayer)));
                cRes = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                if (!app.state.rgshdef[6].fFree && RandomIO().pfnRandom(app, 3) == 0) {
                    id = lppl->id;
                    cFr = 0;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->rgcsh[6] > 0 && lpfl->iPlayer == app.state.idPlayer) {
                            cFr = lpfl->rgcsh[6];
                            break;
                        }
                    }
                    if ((cFr < 10 || (cFr < 17 && RandomIO().pfnRandom(app, 8) == 0)) && RandomIO().pfnRandom(app, cFr * 2 + 1) == 0) {
                        AddItemToQueue(app, 6, 3, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                }
                if (iLatestBomber != ishdefNone) {
                    id = lppl->id;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->iPlayer == app.state.idPlayer && FPotentISWarFleet(app, lpfl, 2)) {
                            if (iLatestBomber == ishdefNone || lpfl->rgcsh[2] + lpfl->rgcsh[3] < app.state.vrgAiArmadaPotency[2])
                                break;
                            AddItemToQueue(app, iLatestBomber, 4, grobjFleet, addItemEnd);
                            fWrite = 1;
                            goto FinishProd;
                        }
                    }
                }
                if (iLatestCruiser != ishdefNone && app.state.rgshdef[iLatestCruiser].cExist < static_cast<uint32_t>((app.state.game.cPlanMax / 12 + 8))) {
                    GetResourcesAvailable(app, lppl, rgResAvail);
                    GetProdQCost(app, lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestCruiser].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= static_cast<uint32_t>(rgCosts[j]);
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = 1;
                        AddItemToQueue(app, iLatestCruiser, 1, grobjFleet, addItemEnd);
                    }
                }
                if (iLatestBattle != ishdefNone && app.state.rgshdef[iLatestBattle].cExist < static_cast<uint32_t>((app.state.game.cPlanMax / 24 + 4))) {
                    GetResourcesAvailable(app, lppl, rgResAvail);
                    GetProdQCost(app, lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestBattle].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= static_cast<uint32_t>(rgCosts[j]);
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = 1;
                        AddItemToQueue(app, iLatestBattle, 1, grobjFleet, addItemEnd);
                    }
                }
            FinishProd:
                FinishProduction(app, fWrite);
            }
        }
    }
    lpflAttack = nullptr;
    lpflEnemy = nullptr;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer != app.state.idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (FIsAiAttack(app, lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
            }
            lpfl->fMark = 0;
            if (FIsAiTransport(app, lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord()[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst == idplNone)
                    continue;
                lppl = LpplFromId(app, idPlanDst);
                if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == app.state.idPlayer))
                    continue;
                if (app.state.vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                    memset(&ord, 0, sizeof(ORDER));
                    ord.pt = app.state.rgptPlan[idPlanDst];
                    ord.grobj = grobjPlanet;
                    ord.id = idPlanDst;
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = 1;
                    ord.txp.rgia[3].SetIAction(iActionUnloadAll);
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    if (app.state.sel.fl.lpplord->rgord()[0].id == idPlanDst) {
                        app.state.sel.fl.lpplord->rgord()[0] = ord;
                    } else {
                        app.state.sel.fl.lpplord->rgord()[1] = ord;
                    }
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    app.state.vlpbAiPlanet[idPlanDst * 16 + 3] = app.state.vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                    continue;
                }
            LBlowAwayOrders:
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.cord = 1;
                app.state.sel.fl.lpplord->iordMac = 1;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                ClearAiCurrentTask(app, lpfl, 0);
                continue;
            } else {
                if (lpfl->rgcsh[1] == 0)
                    continue;
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord()[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
                if (idPlanDst == idplNone)
                    continue;
                if (app.state.vlpbAiPlanet[idPlanDst * 16 + 3] != 0)
                    goto LCheckForColDrop;
                lppl = LpplFromId(app, idPlanDst);
                if (lppl && lppl->iPlayer != iplrNone && lppl->iPlayer != app.state.idPlayer)
                    goto LBlowAwayOrders;
            }
        }
    }
    app.state.fMarkedPlanets = 0;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer != app.state.idPlayer)
            continue;
        if (lpfl->cord > 1) {
            if (app.state.rgshdef[0].hul.rghs[0].iItem >= iengineRadiatingHydroRamScoop || lpfl->rgwtMin[4] >= 2)
                continue;
        LScrapFleet:
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            continue;
        }
        if (lpfl->rgcsh[6] != 0) {
            if (lpfl->cord != 1 || lpfl->lpplord->rgord()[0].grTask != grTaskNone)
                continue;
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskLayMines;
            app.state.sel.fl.lpplord->rgord()[0].tlm().cTime = 5;
            app.state.sel.fl.lpplord->rgord()[0].tlm().cTimeOld = 5;
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            continue;
        }
        if (lpfl->rgcsh[1] == 0)
            goto LTryFreighters;
        if (app.state.rgshdef[1].hul.ihuldef != ihuldefMediumFreighter)
            goto LScrapFleet;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if ((lpfl->idPlanet == idPlanetDeepSpace || app.state.sel.pl.iPlayer != app.state.idPlayer || app.state.sel.pl.rgwtMin[3] < 200) && lpfl->rgwtMin[3] == 0) {
            if ((app.state.sel.fl.idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer && app.state.sel.pl.fStarbase) ||
                (app.state.rgshdef[1].hul.rghs[0].iItem >= iengineFuelMizer && FMoveToNearestStarbase(app, lpfl, 0)))
                continue;
            goto LScrapFleet;
        } else {
            lpthWorm = nullptr;
            idPlanDst = IdNearestColonizablePlanet(app, lpfl, nullptr);
            if (lpfl->idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 150);
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
            }
            if (idPlanDst == idplNone)
                continue;
            FColonizeAiFleet(app, lpfl, idPlanDst);
            app.state.vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
            continue;
        }
    LTryFreighters:
        if (!FIsAiTransport(app, lpfl))
            goto LTryBombers;
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac && (lppl->iPlayer != app.state.idPlayer || !lppl->fStarbase); lppl++) {
        }
        lpplHome = lppl == lpplMac ? nullptr : lppl;
        if (!lpplHome)
            goto BestSpeed;
        lppl = nullptr;
        for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
            for (j = 0; j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() && AhvFromLp(app, app.state.vlpbAiData).rgasb(i).rgflid(j) != lpfl->id; j++) {
            }
            if (j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter())
                break;
        }
        if (i < AhvFromLp(app, app.state.vlpbAiData).cStarbase()) {
            lppl = LpplFromId(app, AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet());
        }
        IdTargetFreighter(app, lpfl, !lppl ? lpplHome : lppl);
        continue;
    LTryBombers:
        if (lpfl->rgcsh[2] == 0 && lpfl->rgcsh[3] == 0)
            goto LTryScouts;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if (lpfl->idPlanet != idPlanetDeepSpace) {
            lppl = LpplFromId(app, lpfl->idPlanet);
            if (lppl->iPlayer == app.state.idPlayer) {
                if (lppl->fStarbase) {
                    if ((lpfl->rgcsh[2] < 2 && lpfl->rgcsh[3] < 2) || (lpfl->rgcsh[9] < 3 && lpfl->rgcsh[10] < 3))
                        continue;
                }
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                goto LTargetBomber;
            }
            if (lppl->iPlayer == iplrNone)
                goto LTargetBomber;
            for (lpflT = lpflEnemy; lpflT; lpflT = lpflT->lpflNext) {
                if (lpfl->pt.x == lpflT->pt.x && lpfl->pt.y == lpflT->pt.y && FIsAiAttack(app, lpflT))
                    goto LTargetBomber;
            }
            continue;
        } else {
            lppl = lpplHome;
        }
    LTargetBomber:
        if (app.state.game.fAisBand) {
            lpplDest = LpplFindBestEnum(app, lppl, FEnumCalcArmadaHumanDest);
        } else {
            lpplDest = nullptr;
        }
        if (!lpplDest) {
            lpplDest = LpplFindBestEnum(app, lppl, FEnumCalcArmadaDest);
        }
        lppl = lpplDest;
        if (!lppl)
            continue;
        app.state.vlpbAiPlanet[lppl->id * 16 + 10] = app.state.vlpbAiPlanet[lppl->id * 16 + 0xa] | 0x80;
        ord.id = lppl->id;
        ord.grobj = grobjPlanet;
        ord.pt = app.state.rgptPlan[lppl->id];
        ord.grTask = grTaskNone;
        ord.fValidTask = 1;
        ord.iWarp = 4;
        FMoveAiFleet(app, lpfl, &ord, 0);
        continue;
    LTryScouts:
        if (lpfl->rgcsh[0] == 0)
            goto LTryFighters;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if (app.state.rgshdef[0].hul.rghs[0].iItem < iengineRadiatingHydroRamScoop && lpfl->rgwtMin[4] < 2)
            goto LScrapFleet;
        IdTargetScout(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand, &lpthWorm);
        continue;
    LTryFighters:;
    }
BestSpeed:
    HandleBasicAiTasks(app, iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

void EnsureISShdefs(APPSESSION &app, int16_t iroCur) {
    SHDEF   shdef;
    int16_t i;

    if (app.state.rgshdef[4].fFree && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5) {
        FCreateAiShdef(app, 4, ihuldefMediumFreighter, static_cast<uint8_t *>(&vrgISAip[isRecipeMediumFreighter]));
    }
    if (app.state.rgshdef[5].fFree && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7) {
        FCreateAiShdef(app, 5, ihuldefSuperFreighter, static_cast<uint8_t *>(&vrgISAip[isRecipeSuperFreighter]));
    }
    if (app.state.rgshdef[14].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 4 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5) {
        for (i = 0; i < 4 && FCreateAiShdef(app, 14, ihuldefDestroyer, static_cast<uint8_t *>(&vrgISAip[vrgISIshAip[iisRecipeDestroyerTorpedo + RandomIO().pfnRandom(app, 1)]])) == 0; i++) {
        }
    }
    if (app.state.rgshdef[1].fFree || app.state.rgshdef[1].cExist == 0) {
        if (!app.state.rgshdef[1].fFree) {
            shdef = app.state.rgshdef[1];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 1);
        }
        FCreateAiShdef(app, 1, ihuldefMediumFreighter, static_cast<uint8_t *>(&vrgISAip[isRecipeColonizingMediumFreighter]));
    }
    if (app.state.rgshdef[0].fFree || app.state.rgshdef[0].cExist == 0) {
        if (!app.state.rgshdef[0].fFree) {
            shdef = app.state.rgshdef[0];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 0);
        }
        FCreateAiShdef(app, 0, ihuldefScout, static_cast<uint8_t *>(&vrgISAip[isRecipeScoutStreamingBeam]));
    }
    if (app.state.rgshdef[6].fFree && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 4 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[5] >= 6) {
        FCreateAiShdef(app, 6, ihuldefPrivateer, static_cast<uint8_t *>(&vrgISAip[isRecipePrivateerMineLayer]));
    }
    if (app.state.rgshdef[2].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 8 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 7 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7) {
        FCreateAiShdef(app, 2, ihuldefB17Bomber, static_cast<uint8_t *>(&vrgISAip[isRecipeB17Bomber]));
    }
    if (app.state.rgshdef[3].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 11 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 12 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 15 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 9) {
        FCreateAiShdef(app, 3, ihuldefB52Bomber, static_cast<uint8_t *>(&vrgISAip[isRecipeB52Bomber]));
    }
    if (app.state.rgshdef[9].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 13 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 9, ihuldefBattleship, static_cast<uint8_t *>(&vrgISAip[vrgISIshAip[iisRecipeBattleshipBeamA + RandomIO().pfnRandom(app, 4)]])) == 0; i++) {
        }
    }
    return;
}

void DoRototillAiTurn(APPSESSION &app, PROD *rgprod) {
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

    iPlanet = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    fColonyShipInQueue = 0;
    fBomberInQueue = 0;
    iroCur = IroEnsureAi(app, nullptr, 0, &ishdefSBLatest, app.state.game.turn >= 20 ? 15 : 0);
    EnsureCAShdefs(iroCur);
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome && PctPlanetOptValue(app, lppl, app.state.idPlayer) > 0) {
            cplanCol++;
        }
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        app.state.vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        if (lppl->iPlayer == iplrNone && lppl->det >= detSome) {
            b = 0;
            for (i = 0; i < 3; i++) {
                if (lppl->rgMinConc[i] > 66) {
                    bT = 75;
                } else {
                    bT = static_cast<int16_t>(lppl->rgMinConc[i]) / 2;
                }
                b += bT;
            }
            if (b & 0x80) {
                b = 127;
            }
            app.state.vlpbAiPlanet[lppl->id * 16 + 1] = b;
        }
        if (lppl->iPlayer != app.state.idPlayer && lppl->iPlayer != iplrNone) {
            app.state.vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            if (PctPlanetOptValue(app, lppl, app.state.idPlayer) > 0) {
                app.state.vlpbAiPlanet[lppl->id * 16 + 3] = 1;
            }
            cplBadGuy++;
        } else if (lppl->iPlayer == app.state.idPlayer && PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer) < 0) {
            cplNegative++;
            app.state.vlpbAiPlanet[lppl->id * 16 + 2] = 1;
        } else if (lppl->fStarbase && lppl->rgwtMin[3] >= 1000) {
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            ProductionIO().pfnInitialize(app, rgprod);
            fWrite = 0;
            b = 0;
            i = 0;
            for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem <= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < app.state.lpplProdGlob->iprodMac) {
                FinishProduction(app, 0);
            } else {
                if (app.state.game.turn == 0) {
                    AddItemToQueue(app, 0, 1, grobjFleet, addItemEnd);
                    AddItemToQueue(app, 0, 1, grobjFleet, addItemEnd);
                } else if (!fColonyShipInQueue && (app.state.rgshdef[1].cExist == 0 || app.state.rgshdef[1].cExist + 1 < static_cast<uint32_t>(cplanCol))) {
                    fColonyShipInQueue = 1;
                    AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                    fWrite = 1;
                }
                FinishProduction(app, fWrite);
            }
        }
    }
    lpflAttack = nullptr;
    lpflEnemy = nullptr;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer != app.state.idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (FIsTurinDroneAiAttack(app, lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
            }
            lpfl->fMark = 0;
            if ((lpfl->rgcsh[7] != 0 || lpfl->rgcsh[8] != 0) && lpfl->cord >= 1) {
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    if (LpplFromId(app, lpfl->idPlanet)->iPlayer != iplrNone) {
                    LBlowAwayOrders:
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.cord = 1;
                        app.state.sel.fl.lpplord->iordMac = 1;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                        ClearAiCurrentTask(app, lpfl, 0);
                        continue;
                    }
                    idPlanDst = lpfl->idPlanet;
                } else {
                    if (lpfl->cord <= 1)
                        continue;
                    idPlanDst = lpfl->lpplord->rgord()[1].id;
                }
                app.state.vlpbAiPlanet[idPlanDst * 16 + 1] = app.state.vlpbAiPlanet[idPlanDst * 16 + 1] | 0x80;
                continue;
            }
            if (FIsAiTransport(app, lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord()[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst == idplNone)
                    continue;
                lppl = LpplFromId(app, idPlanDst);
                if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == app.state.idPlayer))
                    continue;
                if (app.state.vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                    memset(&ord, 0, sizeof(ORDER));
                    ord.pt = app.state.rgptPlan[idPlanDst];
                    ord.grobj = grobjPlanet;
                    ord.id = idPlanDst;
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = 1;
                    ord.txp.rgia[3].SetIAction(iActionUnloadAll);
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    if (app.state.sel.fl.lpplord->rgord()[0].id == idPlanDst) {
                        app.state.sel.fl.lpplord->rgord()[0] = ord;
                    } else {
                        app.state.sel.fl.lpplord->rgord()[1] = ord;
                    }
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    app.state.vlpbAiPlanet[idPlanDst * 16 + 3] = app.state.vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                    continue;
                }
                goto LBlowAwayOrders;
            } else {
                if (lpfl->rgcsh[1] == 0)
                    continue;
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord()[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
                if (idPlanDst == idplNone)
                    continue;
                if (app.state.vlpbAiPlanet[idPlanDst * 16 + 3] != 0)
                    goto LCheckForColDrop;
                lppl = LpplFromId(app, idPlanDst);
                if (lppl && lppl->iPlayer != iplrNone && lppl->iPlayer != app.state.idPlayer)
                    goto LBlowAwayOrders;
            }
        }
    }
    app.state.fMarkedPlanets = 0;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer != app.state.idPlayer)
            continue;
        if (lpfl->rgcsh[7] != 0 || lpfl->rgcsh[8] != 0) {
            if (lpfl->idPlanet == idPlanetDeepSpace)
                continue;
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            b = app.state.vlpbAiPlanet[lpfl->idPlanet * 16 + 1];
            if (b >= 4)
                continue;
            lppl = LpplFindBestEnum(app, &app.state.sel.pl, FEnumCalcMinerDest);
            if (!lppl)
                continue;
            ord.id = lppl->id;
            ord.grobj = grobjPlanet;
            ord.pt = app.state.rgptPlan[lppl->id];
            ord.grTask = grTaskMine;
            ord.fValidTask = 1;
            ord.iWarp = 6;
            FMoveAiFleet(app, lpfl, &ord, 1);
            app.state.vlpbAiPlanet[lppl->id * 16 + 1] = app.state.vlpbAiPlanet[lppl->id * 16 + 1] | 0x80;
            app.state.vlpbAiPlanet[lpfl->idPlanet * 16 + 1] = app.state.vlpbAiPlanet[lpfl->idPlanet * 16 + 1] & 0x80;
            continue;
        }
        if (lpfl->cord > 1)
            continue;
        if (lpfl->rgcsh[1] == 0)
            goto LTryFreighters;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if ((lpfl->idPlanet == idPlanetDeepSpace || app.state.sel.pl.iPlayer != app.state.idPlayer || app.state.sel.pl.rgwtMin[3] < 50) && lpfl->rgwtMin[3] == 0) {
            if ((app.state.sel.fl.idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer && app.state.sel.pl.fStarbase) ||
                (app.state.rgshdef[1].hul.rghs[0].iItem >= iengineFuelMizer && FMoveToNearestStarbase(app, lpfl, 0)))
                continue;
        LScrapFleet:
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            continue;
        } else {
            lpthWorm = nullptr;
            idPlanDst = IdNearestColonizablePlanet(app, lpfl, app.state.rgshdef[1].hul.rghs[0].iItem <= iengineQuickJump5 ? nullptr : &lpthWorm);
            if (lpfl->idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 25);
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
            }
            if (idPlanDst != idplNone) {
                FColonizeAiFleet(app, lpfl, idPlanDst);
                app.state.vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
                continue;
            }
            if (!lpthWorm)
                continue;
            FGotoWormholeAiFleet(app, lpfl, lpthWorm);
            continue;
        }
    LTryFreighters:
        if (!FIsAiTransport(app, lpfl))
            goto LTryBombers;
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac && (lppl->iPlayer != app.state.idPlayer || !lppl->fStarbase); lppl++) {
        }
        lpplHome = lppl == lpplMac ? nullptr : lppl;
        if (!lpplHome)
            goto BestSpeed;
        lppl = nullptr;
        for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
            for (j = 0; j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() && AhvFromLp(app, app.state.vlpbAiData).rgasb(i).rgflid(j) != lpfl->id; j++) {
            }
            if (j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter())
                break;
        }
        if (i < AhvFromLp(app, app.state.vlpbAiData).cStarbase()) {
            lppl = LpplFromId(app, AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet());
        }
        IdTargetFreighter(app, lpfl, !lppl ? lpplHome : lppl);
        continue;
    LTryBombers:
        if (lpfl->rgcsh[13] == 0 && lpfl->rgcsh[14] == 0)
            goto LTryScouts;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if (lpfl->idPlanet != idPlanetDeepSpace) {
            lppl = LpplFromId(app, lpfl->idPlanet);
            if (lppl->iPlayer == app.state.idPlayer) {
                if (lppl->fStarbase) {
                    if (lpfl->rgcsh[13] < 2 && lpfl->rgcsh[14] < 2)
                        continue;
                }
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                goto LTargetBomber;
            }
            if (lppl->iPlayer == iplrNone)
                goto LTargetBomber;
            for (lpflT = lpflEnemy; lpflT; lpflT = lpflT->lpflNext) {
                if (lpfl->pt.x == lpflT->pt.x && lpfl->pt.y == lpflT->pt.y && FIsAiAttack(app, lpflT))
                    goto LTargetBomber;
            }
            continue;
        } else {
            lppl = lpplHome;
        }
    LTargetBomber:
        if (app.state.game.fAisBand) {
            lpplDest = LpplFindBestEnum(app, lppl, FEnumCalcArmadaHumanDest);
        } else {
            lpplDest = nullptr;
        }
        if (!lpplDest) {
            lpplDest = LpplFindBestEnum(app, lppl, FEnumCalcArmadaDest);
        }
        lppl = lpplDest;
        if (!lppl)
            continue;
        app.state.vlpbAiPlanet[lppl->id * 16 + 10] = app.state.vlpbAiPlanet[lppl->id * 16 + 0xa] | 0x80;
        ord.id = lppl->id;
        ord.grobj = grobjPlanet;
        ord.pt = app.state.rgptPlan[lppl->id];
        ord.grTask = grTaskNone;
        ord.fValidTask = 1;
        ord.iWarp = 4;
        FMoveAiFleet(app, lpfl, &ord, 0);
        continue;
    LTryScouts:
        if (lpfl->rgcsh[0] == 0)
            goto LTryFighters;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        if (lpfl->rgcsh[0] != 0 && app.state.rgshdef[0].hul.rghs[0].iItem == iengineQuickJump5 && lpfl->rgwtMin[4] < 2)
            goto LScrapFleet;
        IdTargetScout(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand, &lpthWorm);
        continue;
    LTryFighters:;
    }
BestSpeed:
    HandleBasicAiTasks(app, iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

void EnsureCAShdefs(int16_t iroCur) { return; }
