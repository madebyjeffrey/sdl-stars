#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "fleet_query_io.h"
#include "production_io.h"
#include "heap_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "race_query_io.h"
#include "random_io.h"

void DoAiTurn(APPSESSION &app, std::int16_t iPlayer, std::uint16_t wMdPlr) {
    char    szExt[4];
    PROD    rgprod[64];
    std::int16_t idSav;

    idSav = app.state.idPlayer;
    app.state.fAi = 1;
    CchSprintf(szExt, app.state.MPCTD, iPlayer + 1);
    DestroyCurGame(app);
    if (!FLoadGame(app, app.state.szBase, szExt)) {
        app.state.idPlayer = idSav;
        app.state.fAi = 0;
        return;
    }
    if (app.state.rgplr[app.state.idPlayer].fDead)
        goto Cleanup;
    app.state.vlpbAiPlanet = LpAlloc<std::uint8_t>(app, app.state.game.cPlanMax * 16, htMisc);
    app.state.vrglpplAi = LpAlloc<PLANET *>(app, app.state.game.cPlanMax * sizeof(PLANET *), htMisc);
    if (!app.state.vlpbAiData) {
        app.state.vlpbAiData = LpAlloc<std::uint8_t>(app, 0x2000, htMisc);
        if (app.state.vlpbAiData) {
            AhvFromLp(app, app.state.vlpbAiData).SetCbAiHist(2);
        }
    }
    if (!app.state.vlpbAiPlanet || !app.state.vlpbAiData || !app.state.vrglpplAi)
        goto Cleanup;
    std::fill_n(app.state.vlpbAiPlanet, app.state.game.cPlanMax * 16, 0);
    ComputeShdefPowers(app);
    MarkPlanetsUnderAttack(app);
    IncreaseAIMinefieldSizes(app);
    InitRandomPlanetList(app);
    if (wMdPlr != 0xffff) {
        app.state.rgplr[iPlayer].SetWMdPlr(wMdPlr);
    }
    switch (app.state.rgplr[iPlayer].idAi) {
    case idAiRobotoid:
        DoRobotoidAiTurn(app, rgprod);
        break;
    case idAiCybertron:
        DoCyberAiTurn(app, rgprod);
        break;
    case idAiMacinti:
        DoMacintiAiTurn(app, rgprod);
        break;
    case idAiTurinDrone:
        DoTurinDroneAiTurn(app, rgprod);
        break;
    case idAiMaid:
        DoMaidAiTurn(app, rgprod);
        break;
    case idAiAutomitron:
        DoAutomitronAiTurn(app, rgprod);
        break;
    case idAiRototill:
        DoRototillAiTurn(app, rgprod);
        break;
    }
Cleanup:
    FWriteLogFile(app, app.state.szBase, iPlayer);
    FWriteHistFile(app, iPlayer);
    if (app.state.vrglpplAi) {
        HeapIO().pfnFree(app, app.state.vrglpplAi, htMisc);
        app.state.vrglpplAi = nullptr;
    }
    if (app.state.vlpbAiData) {
        HeapIO().pfnFree(app, app.state.vlpbAiPlanet, htMisc);
        HeapIO().pfnFree(app, app.state.vlpbAiData, htMisc);
        app.state.vlpbAiPlanet = nullptr;
        app.state.vlpbAiData = nullptr;
    }
    app.state.idPlayer = idSav;
    app.state.fAi = 0;
    return;
}

void DoRobotoidAiTurn(APPSESSION &app, PROD *rgprod) {
    std::int32_t  rgResCost[4];
    FLEET   *lpflEnemy;
    std::int32_t  rgResAvail[4];
    std::int16_t  cExistCargo;
    std::int16_t  cFlDestroyers;
    std::int16_t  iLatestDestroyer;
    std::int16_t  cColFleet;
    THING   *lpthWorm;
    std::int16_t  idPlanDst;
    std::int16_t  j;
    std::uint8_t  rgRecycleShdef[16];
    std::int16_t  fShouldColonize;
    PLANET  *lppl;
    std::int16_t  ifl;
    std::int16_t  i;
    std::uint16_t rgCosts[4];
    FLEET   *lpflAttack;
    FLEET   *lpfl;
    PLANET  *lpplHome;
    std::int16_t  cRes;
    std::int16_t  iroCur;
    std::int16_t  iAiLvl;
    std::int16_t  iLatestCargo;
    std::int16_t  ipl;
    std::int16_t  fTonsOfMinerals;
    std::int16_t  ishdefSBLatest;
    PLANET  *lpplMac;
    std::int16_t  iLatestMeta;
    std::uint16_t cRecyclePeriod;
    std::int16_t  cFr;
    std::int16_t  iLatestBattle;
    std::int16_t  iPlanet;
    std::int16_t  iLatestBomber;
    std::int32_t  l;
    std::int16_t  fWrite;
    PROD    *lpprod;
    std::uint8_t  rgRecycleSBShdef[16];
    std::int16_t  id;
    std::int16_t  iLatest;
    std::int16_t  dy;
    std::int32_t  lDist;
    std::int16_t  dx;
    ORDER    ord;
    std::uint8_t *lpb;
    PLANET  *lpplDrop;

    iAiLvl = app.state.rgplr[app.state.idPlayer].lvlAi;
    iPlanet = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    iroCur = IroEnsureAi(app, vrgAiRobotoidResOrder, cAiRobotoidResOrder, &ishdefSBLatest, app.state.game.turn >= 10 ? 15 : 0);
    if (app.state.game.turn > 50) {
        MergeAllShdefs(app, 1788);
        MergeAllShdefs(app, 1);
        MergeAllShdefs(app, -16384);
    }
    j = 4;
    if (app.state.game.turn > 130) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 120)) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    app.state.vrgAiArmadaPotency[0] = j;
    app.state.vrgAiArmadaPotency[1] = static_cast<std::int16_t>((j & 0xff)) / 2;
    j = 6;
    if (app.state.game.turn > 115) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 100)) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    app.state.vrgAiArmadaPotency[2] = j;
    app.state.vrgAiArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    std::ranges::fill(rgRecycleShdef, 0);
    if (app.state.game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = app.state.game.turn >= 200 ? 100 : 70;
    }
    CheckAiShdefStatus(app, 14, 15, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    for (i = 14; i <= 15; i++) {
        if (rgRecycleShdef[i] != 0 && !app.state.rgshdef[i].fFree && app.state.rgshdef[i].hul.ihuldef == ihuldefNubian) {
            rgRecycleShdef[i] = 0;
        }
    }
    cExistCargo = CheckAiShdefStatus(app, 11, 13, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(app, 9, 10, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(app, 2, 5, cRecyclePeriod, &iLatestMeta, rgRecycleShdef);
    CheckAiShdefStatus(app, 6, 7, static_cast<std::uint32_t>((3 * cRecyclePeriod)) / 2, &iLatestBattle, rgRecycleShdef);
    if (app.state.game.turn > 80) {
        SplitOutShdefs(app, rgRecycleShdef);
        std::ranges::fill(rgRecycleSBShdef, 0);
        rgRecycleSBShdef[0] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
        std::ranges::fill(rgRecycleSBShdef, 0);
        rgRecycleSBShdef[1] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
        std::ranges::fill(rgRecycleSBShdef, 0);
        rgRecycleSBShdef[13] = 2;
        rgRecycleSBShdef[12] = 2;
        rgRecycleSBShdef[11] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
    }
    EnsureRobotoidShdefs(app);
    cFlDestroyers = 0;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && iLatestDestroyer != ishdefNone && (lpfl->rgcsh[14] != 0 || lpfl->rgcsh[15] != 0)) {
            cFlDestroyers++;
        }
    }
    fShouldColonize = FShouldWeBuildColonizers(app, &cColFleet);
    UpdateProgressGauge(app, progressStep4);
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer != app.state.idPlayer && lppl->iPlayer != iplrNone) {
            i = lppl->uPopGuess / 250 + 1;
            if (i > 6) {
                i = 6;
            }
            if (lppl->fStarbase) {
                i++;
            }
            app.state.vlpbAiPlanet[lppl->id * 16 + 10] = i;
            app.state.vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        }
    }
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (lppl->fStarbase && lppl->rgwtMin[3] >= 200) {
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            ProductionIO().pfnInitialize(app, rgprod);
            fWrite = 0;
            i = 0;
            for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < app.state.lpplProdGlob->iprodMac) {
                FinishProduction(app, 0);
            } else {
                cFr =
                    app.state.rgplr[app.state.idPlayer].cPlanet / 8 <= AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 4 ? AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 4 : app.state.rgplr[app.state.idPlayer].cPlanet / 8;
                if (iLatestCargo != ishdefNone && (cExistCargo < static_cast<std::int16_t>((cFr * 8)) / 10 || (cExistCargo < cFr && RandomIO().pfnRandom(app, 3) == 0))) {
                    AddItemToQueue(app, iLatestCargo, 1, grobjFleet, addItemEnd);
                    fWrite = 1;
                }
                if (!fShouldColonize && (cColFleet > 25 || RandomIO().pfnRandom(app, AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 8) != 0))
                    goto TryShip2;
                if (app.state.game.turn < 5)
                    goto TryShip2;
                if (app.state.game.turn <= 20) {
                    AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                }
                AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                fWrite = 1;
                l = static_cast<std::uint32_t>((lppl->rgwtMin[3] * PctTrueMaxGrowth(app, app.state.idPlayer)));
                cRes = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                if (l > 2300 && cRes > 35 && iAiLvl > 0) {
                    AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                    if (l > 3600 && cRes > 50 && iAiLvl > 1) {
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                    }
                }
            TryShip2:
                if (app.state.rgshdef[0].hul.ihuldef == ihuldefFrigate && RandomIO().pfnRandom(app, 4) == 0) {
                    id = lppl->id;
                    cFr = 0;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->rgcsh[0] > 0 && lpfl->iPlayer == app.state.idPlayer) {
                            cFr = lpfl->rgcsh[0];
                            break;
                        }
                    }
                    if ((cFr < 10 || (cFr < 17 && RandomIO().pfnRandom(app, 10) == 0)) && RandomIO().pfnRandom(app, cFr * 2 + 1) == 0) {
                        AddItemToQueue(app, 0, 4, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                }
                for (i = 0; i <= 2 && lppl->rgwtMin[i] >= 5000; i++) {
                }
                fTonsOfMinerals = i == 2;
                if (iLatestBomber != ishdefNone) {
                    id = lppl->id;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == id && lpfl->iPlayer == app.state.idPlayer && FPotentRobWarFleet(app, lpfl, 2)) {
                            if (iLatestBomber == ishdefNone || lpfl->rgcsh[9] + lpfl->rgcsh[10] >= app.state.vrgAiArmadaPotency[2])
                                break;
                            AddItemToQueue(app, iLatestBomber, !fTonsOfMinerals ? 4 : 6, grobjFleet, addItemEnd);
                            fWrite = 1;
                            goto FinishProd;
                        }
                    }
                }
                if (iLatestMeta != ishdefNone && (app.state.rgshdef[iLatestMeta].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 7 + 6)) || RandomIO().pfnRandom(app, 2) != 0)) {
                    if (iLatestBattle != ishdefNone && RandomIO().pfnRandom(app, 2) == 0) {
                        iLatest = iLatestBattle;
                    } else {
                        iLatest = iLatestMeta;
                    }
                    GetResourcesAvailable(app, lppl, rgResAvail);
                    GetProdQCost(app, lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatest].hul, rgCosts);
                    for (j = 0; j < 4; j++) {
                        rgResAvail[j] -= static_cast<std::uint32_t>((static_cast<std::uint32_t>((3 * rgCosts[j])) / 5));
                        if (rgResAvail[j] < 0)
                            goto TryShip3;
                    }
                    AddItemToQueue(app, iLatest, !fTonsOfMinerals ? 1 : 5, grobjFleet, addItemEnd);
                    fWrite = 1;
                }
            TryShip3:
                if (iLatestDestroyer != ishdefNone && app.state.rgshdef[iLatestDestroyer].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 12 + 8))) {
                    GetResourcesAvailable(app, lppl, rgResAvail);
                    GetProdQCost(app, lppl, rgResCost);
                    for (i = 0; i < 4; i++) {
                        rgResAvail[i] -= rgResCost[i];
                        if (rgResAvail[i] < 0)
                            goto FinishProd;
                    }
                    for (i = 0; i < 5; i++) {
                        GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestDestroyer].hul, rgCosts);
                        for (j = 0; j < 4; j++) {
                            rgResAvail[j] -= static_cast<std::uint32_t>(rgCosts[j]);
                            if (rgResAvail[j] < 0)
                                goto FinishProd;
                        }
                        fWrite = 1;
                        AddItemToQueue(app, iLatestDestroyer, 1, grobjFleet, addItemEnd);
                    }
                }
            FinishProd:
                FinishProduction(app, fWrite);
            }
        }
    }
    UpdateProgressGauge(app, progressStep4);
    lpflAttack = nullptr;
    lpflEnemy = nullptr;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjThing) {
            dx = lpfl->pt.x - lpfl->lpplord->rgord()[1].pt.x;
            dy = lpfl->pt.y - lpfl->lpplord->rgord()[1].pt.y;
            lDist = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
            if (lDist > 40000) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.cord = 1;
                app.state.sel.fl.lpplord->iordMac = 1;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            }
        }
        if (lpfl->iPlayer != app.state.idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else {
            if (lpfl->rgcsh[0] > 0 && app.state.game.turn > 40 && lpfl->cord == 1) {
                if (lpfl->rgcsh[0] >= 7 && RandomIO().pfnRandom(app, 5) == 0 && (idPlanDst = IdRandomPlanetNearby(app, lpfl->pt, 105, 1)) != idplNone &&
                    idPlanDst != lpfl->idPlanet) {
                    ClearAiCurrentTask(app, lpfl, 1);
                    ord.id = idPlanDst;
                    ord.grobj = grobjPlanet;
                    ord.pt = app.state.rgptPlan[idPlanDst];
                    ord.grTask = grTaskNone;
                    ord.fValidTask = 1;
                    ord.iWarp = 4;
                    FMoveAiFleet(app, lpfl, &ord, 0);
                } else if (lpfl->lpplord->rgord()[0].grTask == grTaskNone) {
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskLayMines;
                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTime = 5;
                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTimeOld = 5;
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    continue;
                }
            } else if (FIsAiAttack(app, lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
                if (lpfl->lpplord->rgord()[0].grTask == grTaskLayMines) {
                    ClearAiCurrentTask(app, lpfl, 1);
                }
                for (j = 2; j <= 7 && lpfl->rgcsh[j] <= 0; j++) {
                }
                if (j <= 7 && ((lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) || lpfl->idPlanet != idPlanetDeepSpace)) {
                    if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        id = lpfl->lpplord->rgord()[1].id;
                    } else {
                        id = lpfl->idPlanet;
                    }
                    lpb = app.state.vlpbAiPlanet + (10 + 16 * id);
                    if (*lpb != 0) {
                        *lpb |= 0x80;
                    }
                }
            } else if (FIsAiTransport(app, lpfl)) {
                idPlanDst = idplNone;
                if (lpfl->cord <= 1 || lpfl->lpplord->rgord()[0].grTask != grTaskNone) {
                    idPlanDst = lpfl->idPlanet;
                } else if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                    idPlanDst = lpfl->lpplord->rgord()[1].id;
                }
                if (idPlanDst != idplNone) {
                    lppl = LpplFromId(app, idPlanDst);
                    if ((!lppl || lppl->iPlayer != app.state.idPlayer) && lpfl->rgwtMin[3] == 0) {
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.cord = 1;
                        app.state.sel.fl.lpplord->iordMac = 1;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                        ClearAiCurrentTask(app, lpfl, 0);
                    }
                }
            }
            if (app.state.game.turn <= 20 && lpfl->rgcsh[0] > 0) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            } else if (lpfl->cord <= 1 && lpfl->rgcsh[1] != 0 && (app.state.game.turn >= 5 || app.state.game.mdStartDist == startDistClose)) {
                if (iAiLvl > 1 && lpfl->idPlanet != idPlanetDeepSpace) {
                    lpplDrop = LpplFromId(app, lpfl->idPlanet);
                    if (lpplDrop && lpplDrop->iPlayer != iplrNone && lpplDrop->iPlayer != app.state.idPlayer && lpfl->rgwtMin[3] > 0 &&
                        RaceQueryIO().pfnStat(&app.state.rgplr[lpplDrop->iPlayer], rsMajorAdv) != raMacintosh) {
                        ord = {};
                        ord.pt = app.state.rgptPlan[lpplDrop->id];
                        ord.id = lpplDrop->id;
                        ord.grobj = grobjPlanet;
                        ord.fValidTask = 1;
                        ord.grTask = grTaskXfer;
                        ord.txp.rgia[3].SetIAction(iActionUnloadAll);
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.lpplord->rgord()[0] = ord;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                        lpplDrop = LpplFindClosestEnum(app, lpplDrop, FEnumOurStarbase);
                        if (!lpplDrop)
                            continue;
                        ord = {};
                        ord.id = lpplDrop->id;
                        ord.grobj = grobjPlanet;
                        ord.pt = app.state.rgptPlan[lpplDrop->id];
                        ord.grTask = grTaskNone;
                        ord.fValidTask = 1;
                        ord.iWarp = 4;
                        FMoveAiFleet(app, lpfl, &ord, 0);
                        continue;
                    }
                }
                idPlanDst = IdNearestColonizablePlanet(app, lpfl, &lpthWorm);
                if (idPlanDst == idplNone && !lpthWorm) {
                    if (lpfl->idPlanet != idPlanetDeepSpace) {
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    }
                } else {
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    if (lpfl->idPlanet != idPlanetDeepSpace) {
                        XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 10);
                        FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                    }
                    if (idPlanDst != idplNone) {
                        FColonizeAiFleet(app, lpfl, idPlanDst);
                    } else {
                        FGotoWormholeAiFleet(app, lpfl, lpthWorm);
                    }
                }
            }
        }
    }
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl] || lppl->fStarbase)
            break;
    }
    lpplHome = ipl == app.state.vclpplAi ? nullptr : lppl;
    if (!lpplHome)
        goto AtkMissions;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && lpfl->cord <= 1 && FIsAiTransport(app, lpfl)) {
            if (lpfl->iplan != 4) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.iplan = 4;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            }
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
        }
    }
    UpdateProgressGauge(app, progressStep4);
AtkMissions:
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer) {
            for (i = 0; i < 16 && (lpfl->rgcsh[i] <= 0 || rgRecycleShdef[i] != 0); i++) {
            }
            if (i == 16) {
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == app.state.idPlayer && (lppl->fStarbase || RandomIO().pfnRandom(app, 5) == 0)) {
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                        continue;
                    }
                }
                if ((lpfl->cord > 1 && lpfl->idPlanet == idPlanetDeepSpace && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) ||
                    FMoveToNearestStarbase(app, lpfl, 0))
                    continue;
            }
            for (i = 2; i <= 10; i++) {
                if (lpfl->rgcsh[i] > 0) {
                    IdTargetArmada(app, lpfl);
                    break;
                }
            }
            if (i > 10 && FIsAiAttack(app, lpfl) && (lpfl->cord <= 1 || lpfl->lpplord->rgord()[1].grobj != grobjFleet) &&
                ((cFlDestroyers <= (app.state.game.turn <= 120 ? 70 : 50) && (cFlDestroyers <= (app.state.game.turn <= 120 ? 60 : 40) || RandomIO().pfnRandom(app, 3) != 0)) ||
                 ((iLatestDestroyer != ishdefNone && lpfl->rgcsh[iLatestDestroyer] >= 20 && RandomIO().pfnRandom(app, 20) != 0) || !FFindBuddyAndJoinUp(app, lpfl, 14, 15, 36, 72)))) {
                IdTargetAttack(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand);
            }
        }
    }
    HandleBasicAiTasks(app, iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

void EnsureRobotoidShdefs(APPSESSION &app) {
    std::int16_t ish;
    std::int16_t i;
    std::int16_t shBase;
    SHDEF   shdef;

    for (ish = 11; ish <= 13; ish++) {
        if (app.state.rgshdef[ish].fFree && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 2 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= (ish - 11) * 3 + 4 &&
            (ish == 11 || static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 14)) {
            if (app.state.rgplr[app.state.idPlayer].rgTech[3] < 10) {
                FCreateAiShdef(app, ish, ihuldefPrivateer, static_cast<std::uint8_t *>(&vrgRobAip[ish == 11 ? robRecipePrivateerBeam : robRecipePrivateerTorpedo]));
            } else {
                for (i = 0; i < 5 && FCreateAiShdef(app, ish, ihuldefMetaMorph, static_cast<std::uint8_t *>(&vrgRobAip[vrgRobIshAip[irobRecipeMetaMorphCargoBeamA + RandomIO().pfnRandom(app, 6)]])) == 0;
                     i++) {
                }
            }
        }
    }
    if (app.state.rgshdef[14].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[0] >= 2) {
        for (i = 0; i < 5 && (FCreateAiShdef(app, 14, ihuldefNubian, static_cast<std::uint8_t *>(&vrgRobAip[robRecipeNubianMixedBeams])) ||
                              !FCreateAiShdef(app, 14, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgRobAip[vrgRobIshAip[irobRecipeDestroyerBeamA + RandomIO().pfnRandom(app, 4)]])));
             i++) {
        }
    }
    if (app.state.rgshdef[15].fFree && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 10 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 8 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 9 &&
        app.state.rgplr[app.state.idPlayer].rgTech[1] >= 14 && !FCreateAiShdef(app, 15, ihuldefNubian, static_cast<std::uint8_t *>(&vrgRobAip[robRecipeNubianMixedBeams]))) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 15, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgRobAip[vrgRobIshAip[irobRecipeDestroyerTorpedoShield + RandomIO().pfnRandom(app, 4)]])) == 0;
             i++) {
        }
    }
    for (ish = 2; ish <= 5; ish++) {
        if (app.state.rgshdef[ish].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 10 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 10 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 9 &&
            app.state.rgplr[app.state.idPlayer].rgTech[0] >= 6 && (ish == 2 || (!app.state.rgshdef[ish - 1].fFree && static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 12))) {
            shBase = !((ish - 2) & 1) ? irobRecipeMetaMorphBeamDefense : irobRecipeMetaMorphMissileA;
            for (i = 0; i < 5 && FCreateAiShdef(app, ish, ihuldefMetaMorph, static_cast<std::uint8_t *>(&vrgRobAip[vrgRobIshAip[RandomIO().pfnRandom(app, 4) + shBase]])) == 0; i++) {
            }
        }
    }
    for (ish = 6; ish <= 7; ish++) {
        if (app.state.rgshdef[ish].fFree && app.state.rgplr[app.state.idPlayer].rgTech[5] >= 4 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 10 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 12 &&
            app.state.rgplr[app.state.idPlayer].rgTech[2] >= 12 && app.state.rgplr[app.state.idPlayer].rgTech[0] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 15 &&
            (ish == 6 || (!app.state.rgshdef[ish - 1].fFree && static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 20))) {
            shBase = ish == 6 ? irobRecipeBattleshipMissileBeam : irobRecipeBattleshipBeamA;
            for (i = 0; i < 5 && FCreateAiShdef(app, ish, ihuldefBattleship, static_cast<std::uint8_t *>(&vrgRobAip[vrgRobIshAip[RandomIO().pfnRandom(app, 4) + shBase]])) == 0; i++) {
            }
        }
    }
    for (ish = 9; ish <= 10; ish++) {
        if (app.state.rgshdef[ish].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 14 &&
            ((ish == 9 || (!app.state.rgshdef[ish - 1].fFree && static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 15)) &&
             !FCreateAiShdef(app, ish, ihuldefBattleship, static_cast<std::uint8_t *>(&vrgRobAip[robRecipeBattleshipMcm])))) {
            FCreateAiShdef(app, ish, ihuldefB52Bomber, static_cast<std::uint8_t *>(&vrgRobAip[ish == 9 ? robRecipeB52SmartThenNormal : robRecipeB52RetroThenSmart]));
        }
    }
    if (app.state.rgshdef[0].hul.ihuldef != ihuldefFrigate && app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && app.state.rgshdef[0].cExist == 0 && app.state.rgplr[app.state.idPlayer].rgTech[5] >= 4 &&
        app.state.rgplr[app.state.idPlayer].rgTech[4] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[0] >= 6) {
        shdef = app.state.rgshdef[0];
        shdef.fFree = 1;
        FChangeAiShdef(app, &shdef, 0);
        FCreateAiShdef(app, 0, ihuldefFrigate, static_cast<std::uint8_t *>(&vrgRobAip[robRecipeFrigateScoutMineLayer]));
    }
    return;
}

std::int16_t IdTargetArmada(APPSESSION &app, FLEET *lpfl) {
    std::int16_t cshWar;
    FLEET  *lpflTarget;
    PLANET *lpplTarget;
    ORDER   ord;
    std::int16_t ish;
    PLANET *lppl;
    std::int32_t cCol;
    std::int16_t cshBomb;
    std::int32_t pctDef;
    std::int32_t lPopUs;
    std::int32_t lPopEnemy;
    std::int32_t cXfer;

    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord()[1];
        if (LDistance2(lpfl->pt, ord.pt) > 62500 && ord.grobj == grobjFleet)
            goto LTryNewTarget;
        if (ord.grobj == grobjFleet) {
            return 0;
        }
        if (ord.grobj == grobjPlanet) {
            lppl = LpplFromId(app, ord.id);
            if (!lppl || ((lppl->iPlayer != iplrNone && (lppl->iPlayer != app.state.idPlayer || lppl->fStarbase)) || lppl->turn != app.state.game.turn)) {
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
    lpfl->fMark = 1;
    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    if (lpfl->idPlanet == idPlanetDeepSpace) {
        MoveToNearestPlanetOrEnemy(app, lpfl, 150);
        return 0;
    }
    lppl = LpplFromId(app, lpfl->idPlanet);
    if (lppl->iPlayer == app.state.idPlayer && lppl->fStarbase) {
        if (cshWar >= app.state.vrgAiArmadaPotency[0] && cshBomb >= app.state.vrgAiArmadaPotency[2]) {
            if (app.state.sel.pl.rgwtMin[3] > 3000) {
                cCol = static_cast<std::int32_t>((app.state.sel.pl.rgwtMin[3] / 10));
            } else if (app.state.sel.pl.rgwtMin[3] > 2000) {
                cCol = static_cast<std::int32_t>((app.state.sel.pl.rgwtMin[3] / 15));
            } else if (app.state.sel.pl.rgwtMin[3] > 1000) {
                cCol = static_cast<std::int32_t>((app.state.sel.pl.rgwtMin[3] / 20));
            } else {
                cCol = 0;
            }
            if (cCol > 0) {
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, LOWORD(cCol));
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
            }
        TargetPotentArmada:
            if (app.state.game.fAisBand) {
                lpplTarget = LpplFindBestEnum(app, lppl, FEnumCalcArmadaHumanDest);
            } else {
                lpplTarget = nullptr;
            }
            if (!lpplTarget) {
                lpplTarget = LpplFindBestEnum(app, lppl, FEnumCalcArmadaDest);
            }
        TargetEveryArmada:
            if (lpplTarget) {
                app.state.vlpbAiPlanet[lpplTarget->id * 16 + 10] = app.state.vlpbAiPlanet[lpplTarget->id * 16 + 0xa] | 0x80;
                ord.id = lpplTarget->id;
                ord.grobj = grobjPlanet;
                ord.pt = app.state.rgptPlan[lpplTarget->id];
            FinishTargeting:
                ord.grTask = grTaskNone;
                ord.fValidTask = 1;
                ord.iWarp = 4;
                if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
                    return -1;
                }
            } else {
                lpflTarget = LpflFindClosestEnum(app, lpfl, FEnumCalcEnemyFleets);
                if (lpflTarget) {
                    ord.id = lpflTarget->id;
                    ord.grobj = grobjFleet;
                    ord.pt = lpflTarget->pt;
                    goto FinishTargeting;
                }
            }
        } else if (app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && (cshWar > app.state.vrgAiArmadaPotency[0] * 2 || cshWar >= 60)) {
            if (RandomIO().pfnRandom(app, 10) < 5 || (cshWar > app.state.vrgAiArmadaPotency[0] * 3 && RandomIO().pfnRandom(app, 10) < 7) || (cshWar > 120 && RandomIO().pfnRandom(app, 10) < 7))
                goto TargetPotentArmada;
        }
    } else if (cshWar < app.state.vrgAiArmadaPotency[1] || cshBomb < app.state.vrgAiArmadaPotency[3]) {
        ClearAiCurrentTask(app, lpfl, 0);
        if (app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && ((cshWar > app.state.vrgAiArmadaPotency[0] * 2 && RandomIO().pfnRandom(app, 10) < 5) ||
                                                      (cshWar > app.state.vrgAiArmadaPotency[0] * 4 && RandomIO().pfnRandom(app, 10) < 7) || (cshWar > 120 && RandomIO().pfnRandom(app, 10) < 7)))
            goto TargetPotentArmada;
        lpplTarget = LpplFindClosestEnum(app, lppl, FEnumOurStarbase);
        goto TargetEveryArmada;
    } else if (lppl->iPlayer == iplrNone) {
        goto TargetPotentArmada;
    } else if (lppl->iPlayer == app.state.idPlayer) {
        if (lppl->rgwtMin[3] > 1000) {
            XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, static_cast<std::int16_t>(LOWORD(lppl->rgwtMin[3])) / 5);
            FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
        }
        goto TargetPotentArmada;
    } else if (lppl->iPlayer != app.state.idPlayer) {
        lPopUs = lpfl->rgwtMin[3];
        lPopEnemy = static_cast<std::int32_t>((lppl->uPopGuess * 4));
        pctDef = static_cast<std::uint32_t>((lppl->uDefGuess * 6)) + 6;
        pctDef = static_cast<std::int32_t>((pctDef * 3)) / 4;
        lPopEnemy = static_cast<std::int32_t>((static_cast<std::int32_t>((lPopEnemy * 100)) / (100 - pctDef)));
        if (lPopEnemy < static_cast<std::int32_t>((lPopUs / 5)) || (lPopEnemy < 200 && lPopUs > 350) || (lPopEnemy < 10 && lPopUs > 150)) {
            cXfer = static_cast<std::int32_t>((lPopEnemy * 5)) / 4;
            if (lpfl->rgwtMin[3] < (cXfer <= static_cast<std::int32_t>((lpfl->rgwtMin[3] / 2)) ? static_cast<std::int32_t>((lpfl->rgwtMin[3] / 2)) : cXfer)) {
                cXfer = lpfl->rgwtMin[3];
            } else if (cXfer <= static_cast<std::int32_t>((lpfl->rgwtMin[3] / 2))) {
                cXfer = static_cast<std::int32_t>((lpfl->rgwtMin[3] / 2));
            }
            if (cXfer > 30000) {
                cXfer = 30000;
            }
            XferAiTroopers(app, lpfl->id, lppl->id, LOWORD(cXfer));
            FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
        }
    }
    return 0;
}

std::int16_t FPotentRobWarFleet(APPSESSION &app, FLEET *lpfl, std::int16_t iPotency) {
    std::int16_t ish;
    std::int16_t cEquiv;

    cEquiv = 0;
    for (ish = 2; ish <= 5; ish++) {
        cEquiv += lpfl->rgcsh[ish];
    }
    for (ish = 6; ish <= 7; ish++) {
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

std::int16_t FEnumCalcEnemyFleets(APPSESSION &app, FLEET *lpflSrc, FLEET *lpflTest) {
    if (lpflTest->iPlayer != app.state.idPlayer) {
        return 1;
    }
    return 0;
}

std::int16_t FEnumCalcArmadaDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::int16_t id;
    std::uint8_t b;
    std::int32_t l2;

    if (lpplSrc == lpplTest) {
        return 0;
    }
    id = lpplTest->id;
    b = app.state.vlpbAiPlanet[id * 16 + 10];
    if (b != 0) {
        l2 = LDistance2(app.state.rgptPlan[lpplSrc->id], app.state.rgptPlan[lpplTest->id]);
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
        if (!(b & 0x80) || RandomIO().pfnRandom(app, 4) == 0) {
            return b;
        }
    }
    return 0;
}

std::int16_t FEnumCalcArmadaHumanDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::int16_t id;
    std::uint8_t b;
    std::int32_t l2;

    if (lpplSrc == lpplTest) {
        return 0;
    }
    id = lpplTest->id;
    if (app.state.rgplr[lpplTest->iPlayer].fAi) {
        return 0;
    }
    b = app.state.vlpbAiPlanet[id * 16 + 10];
    if (b != 0) {
        l2 = LDistance2(app.state.rgptPlan[lpplSrc->id], app.state.rgptPlan[lpplTest->id]);
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
        if (!(b & 0x80) || RandomIO().pfnRandom(app, 4) == 0) {
            return b;
        }
    }
    return 0;
}

void DoTurinDroneAiTurn(APPSESSION &app, PROD *rgprod) {
    std::int32_t  rgResCost[4];
    std::int16_t  iLatestCruiser;
    std::int32_t  rgResAvail[4];
    FLEET   *lpflEnemy;
    std::int16_t  cExistCargo;
    std::int16_t  iLatestDestroyer;
    THING   *lpthWorm;
    PLANET  *lpplDest;
    std::uint8_t  rgRecycleShdef[16];
    std::int16_t  idPlanDst;
    std::int16_t  j;
    std::int16_t  iLatestLayer;
    ORDER    ord;
    PLANET  *lppl;
    std::int16_t  iLatestTroop;
    std::uint16_t rgCosts[4];
    PLANET  *lpplHome;
    FLEET   *lpflAttack;
    FLEET   *lpfl;
    std::int16_t  ifl;
    std::int16_t  i;
    FLEET   *lpflT;
    std::uint8_t  b;
    std::int16_t  cRes;
    std::int16_t  iroCur;
    std::int16_t  iLatestMiner;
    std::int16_t  iLatestCargo;
    std::int16_t  cplMiners;
    PLANET  *lpplMac;
    std::int16_t  ishdefSBLatest;
    std::int16_t  cplNegative;
    std::int16_t  ipl;
    std::uint16_t cRecyclePeriod;
    std::uint16_t cplanCol;
    std::int16_t  cFr;
    std::int16_t  cplBadGuy;
    std::int16_t  iLatestBattle;
    std::int16_t  iLatestBomber;
    std::int32_t  l;
    PROD    *lpprod;
    std::int16_t  fWrite;
    std::int16_t  iPlanet;
    std::uint8_t  bT;
    std::int16_t  pct;
    std::int16_t  id;

    iPlanet = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    cplBadGuy = 0;
    cplNegative = 0;
    cplanCol = 0;
    cplMiners = 0;
    iroCur = IroEnsureAi(app, vrgAiTurinDroneResOrder, cAiTurinDroneResOrder, &ishdefSBLatest, 15);
    if (!app.state.rgshdef[13].fFree) {
        MergeAllShdefs(app, -7952);
    }
    if (!app.state.rgshdef[12].fFree) {
        MergeAllShdefs(app, 4096);
    }
    if (!app.state.rgshdef[10].fFree) {
        MergeAllShdefs(app, 3072);
    }
    if (!app.state.rgshdef[2].fFree) {
        MergeAllShdefs(app, 12);
    }
    j = 3;
    if (app.state.game.turn > 130) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 120)) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    app.state.vrgAiArmadaPotency[0] = j;
    app.state.vrgAiArmadaPotency[1] = static_cast<std::int16_t>((j & 0xff)) / 2;
    j = 6;
    if (app.state.game.turn > 115) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 100)) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    app.state.vrgAiArmadaPotency[2] = j;
    app.state.vrgAiArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    std::ranges::fill(rgRecycleShdef, 0);
    if (app.state.game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = app.state.game.turn >= 200 ? 100 : 70;
    }
    CheckAiShdefStatus(app, 6, 7, cRecyclePeriod, &iLatestCruiser, rgRecycleShdef);
    cExistCargo = CheckAiShdefStatus(app, 8, 9, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(app, 13, 14, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(app, 4, 5, cRecyclePeriod, &iLatestBattle, rgRecycleShdef);
    CheckAiShdefStatus(app, 12, 12, cRecyclePeriod, &iLatestLayer, rgRecycleShdef);
    CheckAiShdefStatus(app, 15, 15, cRecyclePeriod, &iLatestTroop, rgRecycleShdef);
    if (app.state.rgplr[app.state.idPlayer].rgTech[3] >= 7) {
        CheckAiShdefStatus(app, 2, 3, cRecyclePeriod, &iLatestMiner, rgRecycleShdef);
    } else {
        iLatestMiner = ishdefNone;
    }
    CheckAiShdefStatus(app, 10, 11, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    if (app.state.game.turn > 60) {
        SplitOutShdefs(app, rgRecycleShdef);
    }
    EnsureTurinDroneShdefs(app, iroCur);
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
                    bT = static_cast<std::int16_t>(lppl->rgMinConc[i]) / 2;
                }
                b += bT;
            }
            if (b & 0x80) {
                b = 127;
            }
            app.state.vlpbAiPlanet[lppl->id * 16 + 1] = b;
            cplMiners++;
        }
        if (lppl->iPlayer != app.state.idPlayer && lppl->iPlayer != iplrNone) {
            app.state.vlpbAiPlanet[lppl->id * 16 + 10] = lppl->fStarbase + 1;
            pct = PctPlanetOptValue(app, lppl, app.state.idPlayer);
            if (pct > 0) {
                app.state.vlpbAiPlanet[lppl->id * 16 + 3] = pct;
                cplBadGuy++;
            }
        } else if (lppl->iPlayer == app.state.idPlayer) {
            if (PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer) < 0) {
                cplNegative++;
                app.state.vlpbAiPlanet[lppl->id * 16 + 2] = 1;
            } else if (!lppl->fStarbase || lppl->rgwtMin[3] < 200) {
                ChangeMainObjSel(app, grobjPlanet, lppl->id);
                app.state.sel.pl.fNoResearch = lppl->rgwtMin[3] < 200;
                if (static_cast<std::uint32_t>(app.state.sel.pl.fNoResearch) != lppl->fNoResearch) {
                    PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
                }
            } else {
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
                    if (app.state.game.turn == 0) {
                        i = app.state.game.cPlanMax;
                        while (i > 0) {
                            AddItemToQueue(app, 0, 1, grobjFleet, addItemEnd);
                            if (i > 190) {
                                i -= 100;
                            } else {
                                i -= 30;
                            }
                            fWrite = 1;
                        }
                    } else if (!app.state.rgshdef[0].fFree && app.state.rgshdef[0].hul.ihuldef == ihuldefFrigate &&
                               app.state.rgshdef[0].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 4 >= 32 ? 32 : app.state.game.cPlanMax / 4)) &&
                               app.state.rgshdef[0].cExist > static_cast<std::uint32_t>((app.state.rgshdef[0].cBuilt / 10))) {
                        AddItemToQueue(app, 0, 1, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                    cFr = app.state.rgplr[app.state.idPlayer].cPlanet / 10 <= AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 2 ? AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 2
                                                                                                : app.state.rgplr[app.state.idPlayer].cPlanet / 10;
                    if (app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5 && iLatestCargo != ishdefNone &&
                        (cExistCargo < cFr || (cExistCargo < static_cast<std::int16_t>((10 * cFr)) / 7 && RandomIO().pfnRandom(app, 4) == 0))) {
                        AddItemToQueue(app, iLatestCargo, 1, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                    if ((cplanCol != 0 || cplBadGuy != 0) && app.state.rgshdef[1].cExist < 2) {
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                    l = static_cast<std::uint32_t>((lppl->rgwtMin[3] * PctTrueMaxGrowth(app, app.state.idPlayer)));
                    cRes = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                    if (!app.state.rgshdef[12].fFree && RandomIO().pfnRandom(app, 3) == 0) {
                        id = lppl->id;
                        cFr = 0;
                        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                            lpfl = app.state.rglpfl[ifl];
                            if (!app.state.rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->rgcsh[12] > 0 && lpfl->iPlayer == app.state.idPlayer) {
                                cFr = lpfl->rgcsh[12];
                                break;
                            }
                        }
                        if ((cFr < 10 || (cFr < 17 && RandomIO().pfnRandom(app, 8) == 0)) && RandomIO().pfnRandom(app, cFr * 2 + 1) == 0) {
                            AddItemToQueue(app, 12, 3, grobjFleet, addItemEnd);
                            fWrite = 1;
                        }
                    }
                    if (iLatestBomber != ishdefNone) {
                        id = lppl->id;
                        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                            lpfl = app.state.rglpfl[ifl];
                            if (!app.state.rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->iPlayer == app.state.idPlayer && FPotentRobWarFleet(app, lpfl, 2)) {
                                if (iLatestBomber == ishdefNone || lpfl->rgcsh[13] + lpfl->rgcsh[14] < app.state.vrgAiArmadaPotency[2])
                                    break;
                                AddItemToQueue(app, iLatestBomber, 4, grobjFleet, addItemEnd);
                                fWrite = 1;
                                goto FinishProd;
                            }
                        }
                    }
                    if (iLatestBattle != ishdefNone && app.state.rgshdef[iLatestBattle].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 24 + 4))) {
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
                                rgResAvail[j] -= static_cast<std::uint32_t>(rgCosts[j]);
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = 1;
                            AddItemToQueue(app, iLatestBattle, 1, grobjFleet, addItemEnd);
                        }
                    }
                    if (iLatestCruiser != ishdefNone && app.state.rgshdef[iLatestCruiser].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 12 + 8))) {
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
                                rgResAvail[j] -= static_cast<std::uint32_t>(rgCosts[j]);
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = 1;
                            AddItemToQueue(app, iLatestCruiser, 1, grobjFleet, addItemEnd);
                        }
                    }
                    if (iLatestDestroyer != ishdefNone && app.state.rgshdef[iLatestDestroyer].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 4 + 12))) {
                        GetResourcesAvailable(app, lppl, rgResAvail);
                        GetProdQCost(app, lppl, rgResCost);
                        for (i = 0; i < 4; i++) {
                            rgResAvail[i] -= rgResCost[i];
                            if (rgResAvail[i] < 0)
                                goto FinishProd;
                        }
                        for (i = 0; i < 5; i++) {
                            GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestDestroyer].hul, rgCosts);
                            for (j = 0; j < 4; j++) {
                                rgResAvail[j] -= static_cast<std::uint32_t>(rgCosts[j]);
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = 1;
                            AddItemToQueue(app, iLatestDestroyer, 1, grobjFleet, addItemEnd);
                        }
                    }
                    if (iLatestTroop != ishdefNone && !app.state.rgshdef[15].fFree && app.state.rgshdef[iLatestTroop].cExist < static_cast<std::uint32_t>((app.state.game.cPlanMax / 12 + 8))) {
                        GetResourcesAvailable(app, lppl, rgResAvail);
                        GetProdQCost(app, lppl, rgResCost);
                        for (i = 0; i < 4; i++) {
                            rgResAvail[i] -= rgResCost[i];
                            if (rgResAvail[i] < 0)
                                goto FinishProd;
                        }
                        for (i = 0; i < 5; i++) {
                            GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestTroop].hul, rgCosts);
                            for (j = 0; j < 4; j++) {
                                rgResAvail[j] -= static_cast<std::uint32_t>(rgCosts[j]);
                                if (rgResAvail[j] < 0)
                                    goto FinishProd;
                            }
                            fWrite = 1;
                            AddItemToQueue(app, iLatestTroop, 1, grobjFleet, addItemEnd);
                        }
                    }
                FinishProd:
                    FinishProduction(app, fWrite);
                }
            }
        }
    }
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl] || lppl->fStarbase)
            break;
    }
    lpplHome = lppl == lpplMac ? nullptr : lppl;
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
            if ((lpfl->rgcsh[2] != 0 || lpfl->rgcsh[3] != 0) && lpfl->lpplord->rgord()[lpfl->cord - 1].grTask != grTaskNone) {
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
                    lpfl->lpplord->rgord()[1].grTask = grTaskMine;
                    if (lpfl->lpplord->rgord()[1].grobj != grobjPlanet) {
                    }
                    idPlanDst = lpfl->lpplord->rgord()[1].id;
                }
                app.state.vlpbAiPlanet[idPlanDst * 16 + 1] = app.state.vlpbAiPlanet[idPlanDst * 16 + 1] | 0x80;
                continue;
            }
            if (lpfl->rgcsh[8] != 0 || lpfl->rgcsh[9] != 0) {
                idPlanDst = idplNone;
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        idPlanDst = lpfl->lpplord->rgord()[1].id;
                    }
                } else {
                    idPlanDst = lpfl->idPlanet;
                }
            LCheckForColDrop:
                if (idPlanDst != idplNone) {
                    lppl = LpplFromId(app, idPlanDst);
                    if (lppl && (lppl->iPlayer == iplrNone || lppl->iPlayer == app.state.idPlayer))
                        continue;
                    if (app.state.vlpbAiPlanet[idPlanDst * 16 + 3] != 0 && lpfl->rgwtMin[3] > 0 && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh &&
                        !lppl->fStarbase) {
                        ord = {};
                        ord.pt = app.state.rgptPlan[idPlanDst];
                        ord.grobj = grobjPlanet;
                        ord.id = idPlanDst;
                        ord.grTask = grTaskXfer;
                        ord.fValidTask = 1;
                        ord.txp.rgia[3].SetIAction(iActionUnloadAll);
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        if (app.state.sel.fl.lpplord->rgord()[0].id == idPlanDst && app.state.sel.fl.lpplord->rgord()[0].grobj == grobjPlanet) {
                            app.state.sel.fl.lpplord->rgord()[0] = ord;
                        } else {
                            app.state.sel.fl.lpplord->rgord()[1] = ord;
                        }
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                        app.state.vlpbAiPlanet[idPlanDst * 16 + 3] = app.state.vlpbAiPlanet[idPlanDst * 16 + 3] | 0x80;
                        FMoveToNearestStarbase(app, lpfl, 0);
                        continue;
                    }
                    goto LBlowAwayOrders;
                } else if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjOther) {
                    goto LBlowAwayOrders;
                }
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
        if (lpfl->rgcsh[2] != 0 || lpfl->rgcsh[3] != 0) {
            if (app.state.game.turn == 0)
                goto LScrapFleet;
            if (lpfl->idPlanet == idPlanetDeepSpace)
                continue;
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            b = app.state.vlpbAiPlanet[lpfl->idPlanet * 16 + 1];
            if (b >= 4)
                continue;
            lppl = LpplFindBestEnum(app, &app.state.sel.pl, FEnumCalcMinerDest);
            if (!lppl)
                continue;
            ord = {};
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
                (app.state.rgshdef[1].hul.rghs[0].iItem > iengineFuelMizer && FMoveToNearestStarbase(app, lpfl, 0)))
                continue;
        LScrapFleet:
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            continue;
        } else {
            lpthWorm = nullptr;
            idPlanDst = IdNearestColonizablePlanet(app, lpfl, nullptr);
            if (lpfl->idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer) {
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 25);
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                lppl = LpplFromId(app, lpfl->idPlanet);
            } else {
                lppl = lpplHome;
            }
            if (idPlanDst != idplNone) {
                FColonizeAiFleet(app, lpfl, idPlanDst);
                app.state.vlpbAiPlanet[idPlanDst * 16 + 15] = 4;
                continue;
            }
            if (lppl) {
                lpplDest = LpplFindClosestEnum(app, lppl, FEnumCalcColonistDrop);
                if (lpplDest) {
                    app.state.vlpbAiPlanet[lpplDest->id * 16 + 10] = app.state.vlpbAiPlanet[lpplDest->id * 16 + 0xa] | 0x80;
                    ord = {};
                    ord.id = lpplDest->id;
                    ord.grobj = grobjPlanet;
                    ord.pt = app.state.rgptPlan[lpplDest->id];
                    ord.grTask = grTaskXfer;
                    ord.fValidTask = 1;
                    ord.iWarp = 6;
                    ord.txp.rgia[3].SetIAction(iActionUnloadAll);
                    FMoveAiFleet(app, lpfl, &ord, 0);
                    continue;
                }
            }
            if (!lpthWorm || RandomIO().pfnRandom(app, 100) >= 10)
                continue;
            FGotoWormholeAiFleet(app, lpfl, lpthWorm);
            continue;
        }
    LTryFreighters:
        if (lpfl->rgcsh[8] == 0 && lpfl->rgcsh[9] == 0)
            goto LTryBombers;
        if (app.state.game.turn == 0) {
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            goto LScrapFleet;
        }
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
                    if (lpfl->rgcsh[13] + lpfl->rgcsh[14] < app.state.vrgAiArmadaPotency[2] || lpfl->rgcsh[4] + lpfl->rgcsh[5] < app.state.vrgAiArmadaPotency[1])
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
        if (lpfl->rgcsh[0] == 0 && lpfl->rgcsh[10] == 0 && lpfl->rgcsh[11] == 0)
            goto LTryMinelayers;
        if (lpfl->rgcsh[0] != 0 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6 && app.state.rgshdef[0].hul.ihuldef == ihuldefScout)
            goto LScrapFleet;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        IdTargetScout(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand, &lpthWorm);
        continue;
    LTryMinelayers:
        if (lpfl->rgcsh[12] == 0)
            continue;
        if (lpfl->cord != 1 || lpfl->lpplord->rgord()[0].grTask != grTaskNone)
            continue;
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
        app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskLayMines;
        app.state.sel.fl.lpplord->rgord()[0].tlm().cTime = 5;
        app.state.sel.fl.lpplord->rgord()[0].tlm().cTimeOld = 5;
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
        continue;
    }
BestSpeed:
    HandleBasicAiTasks(app, iroCur, rgprod, ishdefSBLatest, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

void EnsureTurinDroneShdefs(APPSESSION &app, std::int16_t iroCur) {
    SHDEF   shdef;
    std::int16_t i;

    if (app.state.rgshdef[8].fFree && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 8) {
        FCreateAiShdef(app, 8, ihuldefRogue, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeRogueCargoScanner]));
    }
    if (app.state.rgshdef[9].fFree && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 11) {
        FCreateAiShdef(app, 9, ihuldefGalleon, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeGalleonCargoScanner]));
    }
    if (app.state.rgshdef[10].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 4 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 5) {
        for (i = 0; i < 4 && FCreateAiShdef(app, 10, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgTDAip[vrgTDIshAip[itdRecipeDestroyerTorpedo + RandomIO().pfnRandom(app, 1)]])) == 0; i++) {
        }
    }
    if (app.state.rgshdef[1].fFree || app.state.rgshdef[1].cExist == 0) {
        if (!app.state.rgshdef[1].fFree && app.state.rgshdef[1].hul.ihuldef != ihuldefPrivateer) {
            shdef = app.state.rgshdef[1];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 1);
        }
        FCreateAiShdef(app, 1, ihuldefColonyShip, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeColonizer]));
    }
    if (app.state.rgshdef[0].fFree || app.state.rgshdef[0].cExist == 0) {
        if (!app.state.rgshdef[0].fFree) {
            shdef = app.state.rgshdef[0];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 0);
        }
        FCreateAiShdef(app, 0, ihuldefFrigate, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeFrigateScout]));
    }
    if ((app.state.rgshdef[2].fFree || app.state.rgshdef[2].cExist == 0) && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 7 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 4) {
        if (!app.state.rgshdef[2].fFree) {
            shdef = app.state.rgshdef[2];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 2);
        }
        FCreateAiShdef(app, 2, ihuldefMiner, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeMiner]));
    }
    if ((app.state.rgshdef[12].fFree || app.state.rgshdef[12].cExist == 0) && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 4 && app.state.rgplr[app.state.idPlayer].rgTech[5] >= 4) {
        FCreateAiShdef(app, 12, ihuldefPrivateer, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipePrivateerMineLayer]));
    }
    if (app.state.rgshdef[13].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 8 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 7 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6) {
        FCreateAiShdef(app, 13, ihuldefStealthBomber, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeStealthBomber]));
    }
    if (app.state.rgshdef[14].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 11 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 12 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 15 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 9) {
        FCreateAiShdef(app, 14, ihuldefStealthBomber, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeStealthBomber]));
    }
    if (app.state.rgshdef[4].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 13 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 4, ihuldefBattleship, static_cast<std::uint8_t *>(&vrgTDAip[vrgTDIshAip[itdRecipeBattleshipBeamA + RandomIO().pfnRandom(app, 4)]])) == 0; i++) {
        }
    }
    if (app.state.rgshdef[15].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[4] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 13 &&
        app.state.rgplr[app.state.idPlayer].rgTech[2] >= 7) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 15, ihuldefRogue, static_cast<std::uint8_t *>(&vrgTDAip[tdRecipeRogueCargoScanner])) == 0; i++) {
        }
    }
    return;
}

std::int16_t FEnumCalcMinerDest(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::int16_t id;
    std::uint8_t b;

    if (lpplSrc == lpplTest) {
        return 0;
    }
    id = lpplTest->id;
    b = app.state.vlpbAiPlanet[id * 16 + 1];
    if (b != 0 && (RandomIO().pfnRandom(app, 100) < 25 || !(b & 0x80))) {
        return b;
    }
    return 0;
}

std::int16_t FEnumCalcColonistDrop(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::int16_t id;
    std::uint8_t bWant;
    std::uint8_t bEnemy;

    if (lpplSrc == lpplTest) {
        return 0;
    }
    id = lpplTest->id;
    bEnemy = app.state.vlpbAiPlanet[lpplTest->id * 16 + 10];
    bWant = app.state.vlpbAiPlanet[lpplTest->id * 16 + 3];
    if (bEnemy == 1 && bWant != 0 && (!(bEnemy & 0x80) || RandomIO().pfnRandom(app, 100) < 25)) {
        if (RaceQueryIO().pfnStat(&app.state.rgplr[lpplTest->iPlayer], rsMajorAdv) == raMacintosh) {
            return 0;
        }
        return bWant;
    }
    return 0;
}
