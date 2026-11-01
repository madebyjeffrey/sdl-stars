#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "race_query_io.h"
#include "random_io.h"

void DoCyberAiTurn(APPSESSION &app, PROD *rgprod) {
    std::int32_t        rgResCost[4];
    std::int16_t        cSBDefenderFleets;
    std::int32_t        rgResAvail[4];
    FLEET         *lpflEnemy;
    std::int32_t        cExistColony;
    std::int16_t        cFlMineLayers;
    std::uint8_t        rgRecycleShdef[16];
    PLANET        *lppl;
    std::int16_t        cMineLayers;
    std::int16_t        ifl;
    std::int16_t        i;
    FLEET         *lpfl;
    std::int16_t        cFlDestroyers;
    std::int16_t        cFlArmadas;
    std::int16_t        iLatestSBDefender;
    std::int32_t        cExistCargo;
    std::int16_t        iroCur;
    std::int16_t        j;
    FLEET         *lpflAttack;
    std::int16_t        iLatestCargo;
    std::int16_t        pctValueIdeal;
    std::int16_t        ipl;
    std::uint8_t       *lpiHistSize;
    std::int16_t        iBuilt;
    std::int16_t        iLatestDestroyer;
    std::uint16_t       cRecyclePeriod;
    std::int16_t        ishdefLatestSB;
    std::int16_t        pctValue;
    PLANET        *lpplMac;
    std::uint8_t       *lpciPlan;
    std::int32_t        lNewPop;
    std::int16_t        iLatestBattle;
    std::int16_t        dOffsetPlanTemp;
    std::int16_t        iAttackStr;
    std::uint8_t       *lpciPlanTemp;
    std::int16_t        idPlanDst;
    std::int16_t        fWrite;
    std::int16_t        fScrap;
    SHDEF          shdef;
    std::uint8_t        rgRecycleSBShdef[16];
    std::int16_t        id;
    std::uint8_t       *lpb;
    std::int16_t        iStrDef;
    ORDER          ord;
    std::int16_t        cFr;
    std::int16_t        iSBDef;
    PLANET        *lpplEnemy;

    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    cSBDefenderFleets = 0;
    cFlMineLayers = 0;
    cMineLayers = 0;
    cFlArmadas = 0;
    cFlDestroyers = 0;
    lpiHistSize = app.state.vlpbAiData;
    if (AivFromLp(app, lpiHistSize).value() == 2) {
        std::memset(app.state.vlpbAiData, 0, 0x2000);
        AivFromLp(app, lpiHistSize).SetValue(app.state.game.cPlanMax * 2 + 2);
    }
    app.state.fMarkedPlanets = 0;
    iroCur = IroEnsureAi(app, vrgAiCybertronResOrder, cAiCybertronResOrder, &ishdefLatestSB, 17);
    EnsureCyberAiShdefs(app, iroCur);
    MergeAllShdefs(app, 1);
    MergeAllShdefs(app, 48);
    MergeAllShdefs(app, -16384);
    MergeAllShdefs(app, 960);
    MergeAllShdefs(app, 15360);
    iAttackStr = 1;
    if (app.state.game.turn > 50) {
        iAttackStr += static_cast<std::uint32_t>((app.state.game.turn - 50)) / 10;
    }
    if (app.state.game.turn > 100) {
        iAttackStr += static_cast<std::uint32_t>((app.state.game.turn - 100)) / 10 * (static_cast<std::uint32_t>(app.state.game.turn) / 100);
    }
    j = 3;
    if (app.state.game.turn > 130) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 120)) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    app.state.vrgAiCyberArmadaPotency[0] = j;
    app.state.vrgAiCyberArmadaPotency[1] = static_cast<std::int16_t>((j & 0xff)) / 2;
    j = 6;
    if (app.state.game.turn > 115) {
        j += static_cast<std::uint32_t>((app.state.game.turn - 100)) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    app.state.vrgAiCyberArmadaPotency[2] = j;
    app.state.vrgAiCyberArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    std::memset(rgRecycleShdef, 0, 16);
    if (app.state.game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = app.state.game.turn < 200 ? 70 : app.state.game.turn < 400 ? 100 : 300;
    }
    CheckAiShdefStatus(app, 4, 5, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    if (iLatestDestroyer != ishdefNone) {
        rgRecycleShdef[iLatestDestroyer] = 0;
    }
    CheckAiShdefStatus(app, 14, 15, cRecyclePeriod, &iLatestSBDefender, rgRecycleShdef);
    if (iLatestSBDefender != ishdefNone) {
        rgRecycleShdef[iLatestSBDefender] = 0;
    }
    cExistCargo = CheckAiShdefStatus(app, 2, 3, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    if (iLatestCargo != ishdefNone) {
        rgRecycleShdef[iLatestCargo] = 0;
    }
    iLatestBattle = ishdefNone;
    for (i = 0; i <= 1; i++) {
        if (!app.state.rgshdef[i * 4 + 6].fFree && static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[i * 4 + 6].turn)) > cRecyclePeriod) {
            fScrap = 1;
            for (j = i * 4 + 9; j >= i * 4 + 6; j--) {
                if (!app.state.rgshdef[j].fFree) {
                    if (app.state.rgshdef[j].cExist == 0 && (fScrap || j != i * 4 + 6)) {
                        shdef = app.state.rgshdef[j];
                        shdef.fFree = 1;
                        FChangeAiShdef(app, &shdef, j);
                    } else {
                        rgRecycleShdef[j] = 1;
                        fScrap = 0;
                    }
                }
            }
        }
        if (!app.state.rgshdef[i * 4 + 6].fFree && (iLatestBattle == ishdefNone || app.state.rgshdef[iLatestBattle * 4 + 6].turn < app.state.rgshdef[i * 4 + 6].turn)) {
            iLatestBattle = i;
        }
    }
    cExistColony = app.state.rgshdef[1].cExist;
    if (app.state.game.turn > 80) {
        SplitOutShdefs(app, rgRecycleShdef);
        std::memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[0] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
        std::memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[1] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
        std::memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[3] = 2;
        rgRecycleSBShdef[2] = 2;
        SplitOutShdefs(app, rgRecycleSBShdef);
    }
    lpciPlanTemp = app.state.vlpbAiData + dOffsetPlanTemp;
    std::memset(lpciPlanTemp, 0, app.state.game.cPlanMax * cbWireCyberInfo);
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        lpciPlan = app.state.vlpbAiData + (lppl->id * 2 + 2);
        lpciPlanTemp = app.state.vlpbAiData + (dOffsetPlanTemp + lppl->id * 2);
        if (CivFromLp(app, lpciPlan).iPktTarget() > 0) {
            CivFromLp(app, lpciPlan).SetIPktTarget(CivFromLp(app, lpciPlan).iPktTarget() + 0x7ff);
        }
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
        } else if (lppl->iPlayer == app.state.idPlayer && lppl->fStarbase) {
            switch (lppl->isb) {
            case 1:
            case 3:
            case 6:
            case 8:
                if (lppl->rgwtMin[0] < 10) {
                    CtvFromLp(app, lpciPlanTemp).SetFNeedsMin1(1);
                }
                if (lppl->rgwtMin[1] < 10) {
                    CtvFromLp(app, lpciPlanTemp).SetFNeedsMin2(1);
                }
                if (lppl->rgwtMin[2] >= 10)
                    break;
                CtvFromLp(app, lpciPlanTemp).SetFNeedsMin3(1);
                break;
            default:
                if (lppl->rgwtMin[0] < 1000) {
                    CtvFromLp(app, lpciPlanTemp).SetFNeedsMin1(1);
                }
                if (lppl->rgwtMin[1] < 1000) {
                    CtvFromLp(app, lpciPlanTemp).SetFNeedsMin2(1);
                }
                if (lppl->rgwtMin[2] < 1000) {
                    CtvFromLp(app, lpciPlanTemp).SetFNeedsMin3(1);
                }
            }
        }
    }
    lpflAttack = nullptr;
    lpflEnemy = nullptr;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (!app.state.fMarkedPlanets) {
            IdNearestColonizablePlanet(app, lpfl, nullptr);
        }
        if (lpfl->iPlayer != app.state.idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else if (lpfl->rgcsh[14] > 0 || lpfl->rgcsh[15] > 0) {
            cSBDefenderFleets++;
        } else {
            if (lpfl->rgcsh[0] > 0) {
                cFlMineLayers++;
                cMineLayers += lpfl->rgcsh[0];
            }
            for (i = 4; i <= 13 && lpfl->rgcsh[i] <= 0; i++) {
            }
            if (i <= 13) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
                if (j > 5 && ((lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) || lpfl->idPlanet != idPlanetDeepSpace)) {
                    if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) {
                        id = lpfl->lpplord->rgord()[1].id;
                    } else {
                        id = lpfl->idPlanet;
                    }
                    lpb = app.state.vlpbAiPlanet + (10 + 16 * id);
                    if (*lpb != 0) {
                        *lpb |= 0x80;
                    }
                    cFlArmadas++;
                } else {
                    cFlDestroyers++;
                }
            } else if ((lpfl->rgcsh[2] > 0 || lpfl->rgcsh[3] > 0) && lpfl->cord > 1 && lpfl->rgwtMin[3] > 0 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet &&
                       CtvFromLp(app, lpciPlanTemp).At(lpfl->lpplord->rgord()[1].id).cFreightersDst() < 3) {
                CtvFromLp(app, lpciPlanTemp).At(lpfl->lpplord->rgord()[1].id).SetCFreightersDst(CtvFromLp(app, lpciPlanTemp).At(lpfl->lpplord->rgord()[1].id).cFreightersDst() + 1);
            }
        }
    }
    UpdateProgressGauge(app, progressStep4);
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
                if ((lpfl->cord > 1 && lpfl->idPlanet == idPlanetDeepSpace) || FMoveToNearestStarbase(app, lpfl, 0))
                    continue;
            }
            if ((lpfl->rgcsh[14] > 0 || lpfl->rgcsh[15] > 0) && lpfl->idPlanet != idPlanetDeepSpace) {
                iStrDef = 0;
                if (static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[14].turn)) < static_cast<std::uint16_t>((cRecyclePeriod - 10))) {
                    iStrDef += lpfl->rgcsh[14];
                }
                if (static_cast<std::uint16_t>((app.state.game.turn - app.state.rgshdef[15].turn)) < static_cast<std::uint16_t>((cRecyclePeriod - 10))) {
                    iStrDef += lpfl->rgcsh[15];
                }
                if (iStrDef > 0) {
                    CtvFromLp(app, lpciPlanTemp).At(lpfl->idPlanet).SetFDefended(1);
                    CtvFromLp(app, lpciPlanTemp).At(lpfl->idPlanet).SetFNeedDefenders(iStrDef < iAttackStr * 2);
                }
            } else {
                for (i = 4; i <= 13 && lpfl->rgcsh[i] <= 0; i++) {
                }
                if (i <= 13) {
                    if (lpfl->rgcsh[4] > 0 || lpfl->rgcsh[4] > 0) {
                        if (lpfl->rgcsh[4] + lpfl->rgcsh[4] >= iAttackStr || lpfl->cord > 1) {
                            IdTargetAttack(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand);
                        }
                        if (lpfl->cord == 1 && RandomIO().pfnRandom(app, 100) < 75) {
                            FFindBuddyAndJoinUp(app, lpfl, 4, 5, 100, 200);
                        }
                    } else {
                        TargetCyberArmada(app, lpfl);
                        if (lpfl->cord == 1 && RandomIO().pfnRandom(app, 100) < 75) {
                            if (i <= 9) {
                                FFindBuddyAndJoinUp(app, lpfl, 6, 9, 100, 200);
                            } else {
                                FFindBuddyAndJoinUp(app, lpfl, 10, 13, 100, 200);
                            }
                        }
                    }
                } else if (lpfl->cord <= 1) {
                    if (app.state.game.turn <= 5 && lpfl->rgcsh[0] > 0 && lpfl->cord == 1) {
                        ChangeMainObjSel(app, grobjFleet, lpfl->id);
                        app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    } else {
                        if (lpfl->rgcsh[1] > 0) {
                            idPlanDst = IdNearestColonizablePlanet(app, lpfl, nullptr);
                            if (idPlanDst == idplNone) {
                                if (lpfl->lpplord->rgord()[0].grobj != grobjPlanet)
                                    continue;
                                CtvFromLp(app, lpciPlanTemp).At(lpfl->lpplord->rgord()[0].id).SetFIdleColonizers(1);
                                continue;
                            }
                            ChangeMainObjSel(app, grobjFleet, lpfl->id);
                            if (lpfl->idPlanet != idPlanetDeepSpace) {
                                PlanetQueryIO().pfnLookup(app, lpfl->idPlanet, &app.state.sel.pl);
                                if (app.state.sel.pl.iPlayer == app.state.idPlayer) {
                                    XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 250);
                                    FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                                }
                            }
                            if (idPlanDst != idplNone) {
                                if (!FColonizeAiFleet(app, lpfl, idPlanDst) || lpfl->cord <= 1)
                                    continue;
                                app.state.vlpbAiPlanet[lpfl->lpplord->rgord()[1].id * 16 + 15] = 4;
                                continue;
                            }
                        }
                        if (lpfl->rgcsh[2] > 0 || lpfl->rgcsh[3] > 0) {
                            if (lpfl->iplan != 4) {
                                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                                app.state.sel.fl.iplan = 4;
                                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                            }
                            DoCyberFreighter(app, lpfl, lpciPlanTemp);
                        } else if (lpfl->rgcsh[0] > 0 && app.state.game.turn > 40 && lpfl->cord == 1) {
                            if (lpfl->cord > 1) {
                                if (lpfl->lpplord->rgord()[0].grTask != grTaskNone) {
                                    ClearAiCurrentTask(app, lpfl, 1);
                                }
                            } else if ((cFlMineLayers <= 55 && (cFlMineLayers <= 40 || RandomIO().pfnRandom(app, 3) == 0)) || !FFindBuddyAndJoinUp(app, lpfl, 0, 0, 72, 108)) {
                                if (lpfl->rgcsh[0] >= 7 && RandomIO().pfnRandom(app, 5) == 0) {
                                    idPlanDst = IdRandomPlanetNearby(app, lpfl->pt, 105, 1);
                                    if (idPlanDst != idplNone && idPlanDst != lpfl->idPlanet) {
                                        ClearAiCurrentTask(app, lpfl, 1);
                                        ord.id = idPlanDst;
                                        ord.grobj = grobjPlanet;
                                        ord.pt = app.state.rgptPlan[idPlanDst];
                                        ord.grTask = grTaskLayMines;
                                        ord.tlm().cTime = 5;
                                        ord.tlm().cTimeOld = 5;
                                        ord.fValidTask = 1;
                                        ord.iWarp = 4;
                                        FMoveAiFleet(app, lpfl, &ord, 0);
                                        continue;
                                    }
                                }
                                if (lpfl->lpplord->rgord()[0].grTask != grTaskLayMines) {
                                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                                    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskLayMines;
                                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTime = 5;
                                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTimeOld = 5;
                                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    UpdateProgressGauge(app, progressStep4);
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        lpciPlan = app.state.vlpbAiData + (lppl->id * 2 + 2);
        lpciPlanTemp = app.state.vlpbAiData + (dOffsetPlanTemp + lppl->id * 2);
        ChangeMainObjSel(app, grobjPlanet, lppl->id);
        ProductionIO().pfnInitialize(app, rgprod);
        fWrite = 0;
        lNewPop = static_cast<std::uint32_t>((lppl->rgwtMin[3] * PctTrueMaxGrowth(app, app.state.idPlayer)));
        GetResourcesAvailable(app, lppl, rgResAvail);
        GetProdQCost(app, lppl, rgResCost);
        for (i = 0; i < 4; i++) {
            if (rgResCost[i] > rgResAvail[i])
                goto LFinishProduction;
        }
        pctValue = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
        pctValueIdeal = PctPlanetOptValue(app, lppl, app.state.idPlayer);
        if (pctValue < 10) {
            i = LOWORD(static_cast<std::int32_t>(((rgResAvail[3] - rgResCost[3]) / 70))) + 1;
            AddItemToQueue(app, mdIdleTerraform, i, grobjPlanet, addItemEnd);
            fWrite = 1;
        } else {
            if (pctValue < pctValueIdeal && rgResAvail[3] - rgResCost[3] > 70) {
                AddItemToQueue(app, mdIdleTerraform, 1, grobjPlanet, addItemEnd);
                fWrite = 1;
            }
            if (lppl->fStarbase) {
                switch (lppl->isb) {
                default:
                    if ((!CivFromLp(app, lpciPlan).fBltColony() || lNewPop > 5500) && !CtvFromLp(app, lpciPlanTemp).fIdleColonizers() && cExistColony < 40 && FShouldPlanetBuildColonizer(app, lppl)) {
                        AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        CivFromLp(app, lpciPlan).SetFBltColony(1);
                        fWrite = 1;
                        if (lNewPop > 15000 && app.state.game.turn < 100) {
                            AddItemToQueue(app, 1, 1, grobjFleet, addItemEnd);
                        }
                    } else {
                        CivFromLp(app, lpciPlan).SetFBltColony(0);
                    }
                    if (lppl->rgwtMin[3] > 2000 && !app.state.rgshdef[2].fFree && cExistCargo < 50 && CtvFromLp(app, lpciPlanTemp).cIdleFreighters() < 1 &&
                        LpplFindClosestEnum(app, lppl, FEnumDropOffStage2) != 0) {
                        AddItemToQueue(app, iLatestCargo, 1, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                    if (app.state.rgshdef[0].hul.ihuldef == ihuldefFrigate && RandomIO().pfnRandom(app, 4) == 0 && cMineLayers < 10000) {
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
                    iSBDef = !CtvFromLp(app, lpciPlanTemp).fNeedDefenders() ? ishdefNone : iLatestSBDefender;
                    if (iSBDef == ishdefNone && !CtvFromLp(app, lpciPlanTemp).fDefended() && cSBDefenderFleets < 40) {
                        lpplEnemy = LpplFindClosestEnum(app, lppl, FEnumCalcEnemyPlanets);
                        if (!lpplEnemy || LDistance2(app.state.rgptPlan[lppl->id], app.state.rgptPlan[lpplEnemy->id]) > 90000) {
                            iSBDef = RandomIO().pfnRandom(app, 100) >= 10 ? ishdefNone : iLatestSBDefender;
                        } else {
                            iSBDef = RandomIO().pfnRandom(app, 100) >= 50 ? ishdefNone : iLatestSBDefender;
                        }
                    }
                    if (cFlDestroyers > 120) {
                        iLatestDestroyer = ishdefNone;
                    }
                    if (cFlArmadas > 250) {
                        iLatestBattle = ishdefNone;
                    }
                    iBuilt = iAddAttackFleet(app, lppl, iAttackStr, iLatestDestroyer, iLatestBattle, iSBDef);
                    fWrite |= iBuilt != 0;
                    if (iBuilt == 1) {
                        cFlArmadas++;
                    }
                    if (iBuilt == 2) {
                        cSBDefenderFleets++;
                    }
                    if (iBuilt == 3) {
                        cFlDestroyers++;
                    }
                case 1:
                case 3:
                case 6:
                case 8:
                    break;
                }
            }
        }
    LFinishProduction:
        FinishProduction(app, fWrite);
    }
    UpdateProgressGauge(app, progressStep4);
    HandleBasicAiTasks(app, iroCur, rgprod, IshdefAiSBLatest(app), rgResAvail, rgResCost);
    UpdateProgressGauge(app, progressStep4);
    DoCyberPackets(app);
    UpdateProgressGauge(app, progressStep4);
    FillProductionQueue(app);
    return;
}

void DoCyberPackets(APPSESSION &app) {
    std::int16_t        fTwoMA;
    std::int32_t        rgResCost[4];
    std::int32_t        rgResAvail[4];
    std::int16_t        iWarp;
    PLANET        *lpplDst;
    PLANET        *lppl;
    std::int16_t        i;
    std::int16_t        ipl;
    std::int16_t        iWarpDst;
    std::uint8_t       *lpciPlan;
    PROD           rgprod[64];
    std::int16_t        dOffsetPlanTemp;
    std::int16_t        idPlanDst;
    std::uint8_t       *lpciPlanTemp;
    std::int16_t        fWrite;
    std::int16_t        iPacketMax;
    std::int16_t        iMinLimit;
    std::int16_t        iPacketAdd;
    std::uint8_t       *lpciPlanDst;
    std::uint8_t       *lpciPlanT;
    std::int16_t        iWarpSrc;
    std::uint8_t       *plMinMax;
    std::int32_t        lPackets;
    std::int32_t        lMineral;
    std::int16_t        cResLeft;
    double         dDistance;
    double         dMod;
    std::int16_t        cPacket[3];
    std::int32_t        lMinNeeded;
    double         dDistanceTgt;
    std::int16_t        iMin;

    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (lppl->fStarbase) {
            lpciPlan = app.state.vlpbAiData + (lppl->id * 2 + 2);
            lpciPlanTemp = app.state.vlpbAiData + (dOffsetPlanTemp + lppl->id * 2);
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            ProductionIO().pfnInitialize(app, rgprod);
            fWrite = 0;
            GetResourcesAvailable(app, lppl, rgResAvail);
            GetProdQCost(app, lppl, rgResCost);
            if (!CivFromLp(app, lpciPlan).fNeedScanPkt() && (lppl->isb == 1 || lppl->isb == 3 || lppl->isb == 6 || lppl->isb == 8) &&
                (lppl->rgwtMin[0] > 700 || lppl->rgwtMin[1] > 700 || lppl->rgwtMin[2] > 700)) {
                iPacketMax = LOWORD(static_cast<std::int32_t>((rgResAvail[3] / 2)));
                iPacketMax /= 5;
                if (iPacketMax < 7) {
                    lpplDst = nullptr;
                } else {
                    lpplDst = LpplFindClosestEnum(app, lppl, FEnumNeedMinerals);
                }
                if (lpplDst) {
                    lpciPlanDst = app.state.vlpbAiData + (lpplDst->id * 2 + 2);
                    lpciPlanT = app.state.vlpbAiData + (dOffsetPlanTemp + lpplDst->id * 2);
                    iPacketAdd = 0;
                    if (lpplDst->isb == 1 || lpplDst->isb == 3 || lpplDst->isb == 6 || lpplDst->isb == 8) {
                        iMinLimit = 10;
                    } else {
                        iMinLimit = 1000;
                    }
                    if (lpplDst->rgwtMin[0] < iMinLimit && lppl->rgwtMin[0] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(app, iobjPacketIron, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        CtvFromLp(app, lpciPlanT).SetFNeedsMin1(0);
                        fWrite = 1;
                    }
                    if (lpplDst->rgwtMin[1] < iMinLimit && lppl->rgwtMin[1] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(app, iobjPacketBor, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        CtvFromLp(app, lpciPlanT).SetFNeedsMin2(0);
                        fWrite = 1;
                    }
                    if (lpplDst->rgwtMin[2] < iMinLimit && lppl->rgwtMin[2] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(app, iobjPacketGerm, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        CtvFromLp(app, lpciPlanT).SetFNeedsMin3(0);
                        fWrite = 1;
                    }
                    if (!fWrite)
                        goto LFinish;
                    iWarpSrc = ColonyIO().pfnWarp(app, lppl, &fTwoMA) - 4;
                    if (fTwoMA) {
                        iWarpSrc++;
                    }
                    iWarpDst = ColonyIO().pfnWarp(app, lpplDst, &fTwoMA) - 4;
                    if (fTwoMA) {
                        iWarpDst++;
                    }
                    app.state.sel.pl.iWarpFling = iWarpSrc >= iWarpDst ? iWarpDst : iWarpSrc;
                    app.state.sel.pl.idFling = lpplDst->id + 1;
                    PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
                    goto LFinish;
                }
            }
            if (!CivFromLp(app, lpciPlan).fNeedScanPkt() && (app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard || (app.state.rgplr[app.state.idPlayer].lvlAi == lvlAiStandard && RandomIO().pfnRandom(app, 3) == 0))) {
                plMinMax = app.state.vlpbAiData + (dOffsetPlanTemp + app.state.game.cPlanMax * 2);
                rgResAvail[0] -= rgResCost[0];
                rgResAvail[1] -= rgResCost[1];
                rgResAvail[2] -= rgResCost[2];
                lMineral = rgResAvail[0] - 70 + (rgResAvail[1] - 70) + (rgResAvail[2] - 70);
                cResLeft = LOWORD(static_cast<std::int32_t>((rgResAvail[3] / 2)));
                lPackets = static_cast<std::int16_t>((static_cast<std::int16_t>((cResLeft - 5)) / 5));
                MivFromLp(app, plMinMax).SetValue(lMineral < static_cast<std::int32_t>(static_cast<std::uint32_t>((lPackets * 70))) ? lMineral : static_cast<std::uint32_t>((lPackets * 70)));
                if (lMineral > 150) {
                    lpplDst = LpplFindClosestEnum(app, lppl, FEnumPktAttack);
                } else {
                    lpplDst = nullptr;
                }
                if (lpplDst) {
                    cPacket[0] = 0;
                    cPacket[1] = 0;
                    cPacket[2] = 0;
                    lpciPlanDst = app.state.vlpbAiData + (lpplDst->id * 2 + 2);
                    lpciPlanT = app.state.vlpbAiData + (dOffsetPlanTemp + lpplDst->id * 2);
                    iPacketAdd = 0;
                    if (lpplDst->fStarbase) {
                        iWarpDst = ColonyIO().pfnWarp(app, lpplDst, &fTwoMA);
                        if (fTwoMA) {
                            iWarpDst++;
                        }
                    } else {
                        iWarpDst = 0;
                    }
                    iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMA) + 3;
                    dDistance = static_cast<double>(static_cast<std::int16_t>((iWarp * iWarp)));
                    dDistanceTgt = DGetDistance(app.state.rgptPlan[lppl->id].x, app.state.rgptPlan[lppl->id].y, app.state.rgptPlan[lpplDst->id].x, app.state.rgptPlan[lpplDst->id].y);
                    dMod = static_cast<double>((static_cast<double>(static_cast<std::uint32_t>(((iWarp * iWarp - iWarpDst * iWarpDst) * (100 - (lpplDst->uDefGuess + 5))))) / 16000.0));
                    lMinNeeded = static_cast<std::int32_t>(((1000 >= static_cast<std::uint16_t>(((lpplDst->uPopGuess + 25) * 4)) ? static_cast<double>(static_cast<std::uint32_t>(((lpplDst->uPopGuess + 25) * 4)))
                                                                                              : static_cast<double>(1000)) /
                                           dMod));
                    lMineral = lMinNeeded < lMineral ? lMinNeeded : lMineral;
                    if (fTwoMA) {
                        lMineral = static_cast<std::int32_t>((static_cast<double>(lMineral) / std::pow(0.875, static_cast<double>((static_cast<double>(dDistanceTgt) / dDistance)))));
                    } else {
                        lMineral = static_cast<std::int32_t>((static_cast<double>(lMineral) / std::pow(0.75, static_cast<double>((static_cast<double>(dDistanceTgt) / dDistance)))));
                    }
                    for (; lMineral > 0; lMineral -= 70) {
                        iMin = 0;
                        if (rgResAvail[iMin] < rgResAvail[1]) {
                            iMin = 1;
                        }
                        if (rgResAvail[iMin] < rgResAvail[2]) {
                            iMin = 2;
                        }
                        cPacket[iMin]++;
                        rgResAvail[iMin] -= 70;
                    }
                    if (cPacket[0] > 0) {
                        AddItemToQueue(app, iobjPacketIron, cPacket[0], grobjPlanet, addItemFront);
                    }
                    if (cPacket[1] > 0) {
                        AddItemToQueue(app, iobjPacketBor, cPacket[1], grobjPlanet, addItemFront);
                    }
                    if (cPacket[2] > 0) {
                        AddItemToQueue(app, iobjPacketGerm, cPacket[2], grobjPlanet, addItemFront);
                    }
                    app.state.sel.pl.iWarpFling = iWarp - 4;
                    app.state.sel.pl.idFling = lpplDst->id + 1;
                    PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
                    CivFromLp(app, lpciPlanDst).SetIPktTarget(3);
                    if (static_cast<double>(dDistance) < static_cast<double>(dDistanceTgt)) {
                        CivFromLp(app, lpciPlan).SetFNeedScanPkt(1);
                    }
                    CivFromLp(app, lpciPlan).SetFLaunchedPkt(0);
                    fWrite = 1;
                    goto LFinish;
                }
            }
            if (!CivFromLp(app, lpciPlan).fLaunchedPkt() || CivFromLp(app, lpciPlan).fNeedScanPkt()) {
                if (!CivFromLp(app, lpciPlan).fNeedScanPkt()) {
                    i = RandomIO().pfnRandom(app, 7);
                    if (i == CivFromLp(app, lpciPlan).iLstPktDir()) {
                        CivFromLp(app, lpciPlan).SetILstPktDir(i + 1);
                    } else {
                        CivFromLp(app, lpciPlan).SetILstPktDir(i);
                    }
                    idPlanDst = IdGetBestScannerDest(app, lppl, static_cast<CompassDir>(CivFromLp(app, lpciPlan).iLstPktDir()));
                }
                if (CivFromLp(app, lpciPlan).fNeedScanPkt() || idPlanDst != idplNone) {
                    lpciPlanDst = app.state.vlpbAiData + (idPlanDst * 2 + 2);
                    if (CivFromLp(app, lpciPlanDst).iPktTarget() == 0 && FAddPacketToQueue(app, lppl)) {
                        CivFromLp(app, lpciPlanDst).SetIPktTarget(3);
                        CivFromLp(app, lpciPlan).SetFLaunchedPkt(CivFromLp(app, lpciPlan).fNeedScanPkt() == 0);
                        fWrite = 1;
                    } else {
                        CivFromLp(app, lpciPlan).SetFLaunchedPkt(0);
                    }
                    iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMA) - 1;
                    app.state.sel.pl.iWarpFling = iWarp;
                    if (!CivFromLp(app, lpciPlan).fNeedScanPkt()) {
                        app.state.sel.pl.idFling = idPlanDst;
                    } else {
                        CivFromLp(app, lpciPlan).SetFNeedScanPkt(0);
                    }
                    PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
                }
            } else {
                CivFromLp(app, lpciPlan).SetFLaunchedPkt(0);
            }
        LFinish:
            FinishProduction(app, fWrite);
        }
    }
    return;
}

std::int16_t IdGetBestScannerDest(APPSESSION &app, PLANET *lppl, CompassDir iDir) {
    std::int16_t iDistance;
    std::int16_t iWarp;
    PLANET *lpplDst;
    std::int16_t dAdjust;
    std::int16_t iSize;
    POINT16 ptEdge;
    SCAN    scan;

    ptEdge = app.state.rgptPlan[lppl->id];
    iSize = 400 * app.state.game.mdSize + 400;
    iWarp = ColonyIO().pfnWarp(app, lppl, nullptr) + 3;
    iDistance = iWarp * iWarp;
    ptEdge.x -= 1000;
    ptEdge.y -= 1000;
    if (iDir > dirSouthEast) {
        return idplNone;
    }
    switch (iDir) {
    case dirEast:
        ptEdge.x = iSize;
        break;
    case dirNorthEast:
        if (iSize - ptEdge.x > ptEdge.y) {
            ptEdge.x += ptEdge.y;
            ptEdge.y = 0;
            break;
        }
        ptEdge.y -= iSize - ptEdge.x;
        ptEdge.x = iSize;
        break;
    case dirNorth:
        ptEdge.y = 0;
        break;
    case dirNorthWest:
        if (ptEdge.x > ptEdge.y) {
            ptEdge.x -= ptEdge.y;
            ptEdge.y = 0;
            break;
        }
        ptEdge.y -= ptEdge.x;
        ptEdge.x = 0;
        break;
    case dirWest:
        ptEdge.x = 0;
        break;
    case dirSouthWest:
        if (iSize - ptEdge.x > ptEdge.y) {
            ptEdge.y += ptEdge.x;
            ptEdge.x = 0;
            break;
        }
        ptEdge.x -= iSize - ptEdge.y;
        ptEdge.y = iSize;
        break;
    case dirSouth:
        ptEdge.y = iSize;
        break;
    case dirSouthEast:
        if (ptEdge.x > ptEdge.y) {
            ptEdge.y += iSize - ptEdge.x;
            ptEdge.x = iSize;
        } else {
            ptEdge.x += iSize - ptEdge.y;
            ptEdge.y = iSize;
        }
    }
    dAdjust = RandomIO().pfnRandom(app, static_cast<std::int32_t>((static_cast<double>(iSize) * 0.3)));
    dAdjust -= LOWORD(static_cast<std::int32_t>((static_cast<double>(iSize) * 0.15)));
    if (ptEdge.x == 0 || ptEdge.x == iSize) {
        ptEdge.y += dAdjust;
        if (ptEdge.y > iSize) {
            dAdjust = ptEdge.y - iSize;
            ptEdge.y = iSize;
        } else if (ptEdge.y < 0) {
            dAdjust = 0 - ptEdge.y;
            ptEdge.y = 0;
        } else {
            dAdjust = 0;
        }
        if (ptEdge.x == 0) {
            ptEdge.x += dAdjust;
        } else {
            ptEdge.x -= dAdjust;
        }
    } else {
        ptEdge.x += dAdjust;
        if (ptEdge.x > iSize) {
            dAdjust = ptEdge.x - iSize;
            ptEdge.x = iSize;
        } else if (ptEdge.x < 0) {
            dAdjust = 0 - ptEdge.x;
            ptEdge.x = 0;
        } else {
            dAdjust = 0;
        }
        if (ptEdge.y == 0) {
            ptEdge.y += dAdjust;
        } else {
            ptEdge.y -= dAdjust;
        }
    }
    dAdjust = RandomIO().pfnRandom(app, iDistance);
    if (ptEdge.y == 0) {
        ptEdge.y += dAdjust;
    } else if (ptEdge.y == iSize) {
        ptEdge.y -= dAdjust;
    } else if (ptEdge.x == 0) {
        ptEdge.x += dAdjust;
    } else if (ptEdge.x == iSize) {
        ptEdge.x -= dAdjust;
    }
    ptEdge.y += 1000;
    ptEdge.x += 1000;
    if (FFindNearestObject(app, ptEdge, static_cast<GrobjClass>(grobjPlanet | mdNoRecurse), &scan)) {
        lpplDst = LpplFromId(app, scan.idpl);
        if ((lpplDst && lpplDst->iPlayer == app.state.idPlayer) || static_cast<std::int32_t>(LDistance2(app.state.rgptPlan[scan.idpl], app.state.rgptPlan[lppl->id])) < static_cast<std::int16_t>((iDistance * iDistance))) {
            return idplNone;
        }
        return scan.idpl + 1;
    }
    return idplNone;
}

std::int16_t FAddPacketToQueue(APPSESSION &app, PLANET *lppl) {
    std::int32_t rgResCost[4];
    std::int16_t iMineral;
    std::int32_t rgResAvail[4];

    iMineral = 0;
    GetResourcesAvailable(app, lppl, rgResAvail);
    GetProdQCost(app, lppl, rgResCost);
    if (rgResAvail[iMineral] - rgResCost[iMineral] < rgResAvail[1] - rgResCost[1]) {
        iMineral = 1;
    }
    if (rgResAvail[iMineral] - rgResCost[iMineral] < rgResAvail[2] - rgResCost[2]) {
        iMineral = 2;
    }
    if (rgResAvail[iMineral] - rgResCost[iMineral] >= 170) {
        AddItemToQueue(app, iMineral + 14, 1, grobjPlanet, addItemFront);
        return 1;
    }
    return 0;
}

void FillProductionQueue(APPSESSION &app) {
    PLANET *lppl;
    std::int16_t ipl;
    PROD    rgprod[64];

    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        ChangeMainObjSel(app, grobjPlanet, lppl->id);
        ProductionIO().pfnInitialize(app, rgprod);
        FinishProduction(app, FFillProdMinesAndFactories(app, lppl));
    }
    return;
}

std::int16_t FFillProdMinesAndFactories(APPSESSION &app, PLANET *lppl) {
    std::int32_t rgResCost[4];
    std::int32_t rgResAvail[4];
    std::int16_t iAddFactories;
    std::int16_t iAddMines;
    PROD    prod;
    std::int32_t rgAlchCost[4];
    std::int32_t rgMineCost[4];
    std::int32_t rgFactCost[4];
    std::int32_t rgResLeft[4];
    std::int16_t iAddAlchemy;
    std::int16_t iMaxTerra;
    std::int16_t i;
    std::int16_t iMaxFactories;
    std::int16_t iMaxFactBuildable;
    std::int16_t iMaxMines;
    std::int16_t fInsert;
    PROD   *lpprod;
    std::int16_t cAdd;

    fInsert = 0;
    GetResourcesAvailable(app, lppl, rgResAvail);
    GetProdQCost(app, lppl, rgResCost);
    for (i = 0; i < 4; i++) {
        rgResLeft[i] = rgResAvail[i] - rgResCost[i];
    }
    if (rgResLeft[3] < 0) {
        return 0;
    }
    iMaxTerra = 0;
    iMaxMines = 0;
    iAddMines = 0;
    iMaxFactories = 0;
    iAddFactories = 0;
    iAddAlchemy = 0;
    rgMineCost[3] = 0;
    rgFactCost[3] = 0;
    rgAlchCost[3] = 0;
    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleMine) {
            iAddMines += lpprod->cItem;
        }
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleFactory) {
            iAddFactories += lpprod->cItem;
        }
        i++;
        lpprod++;
    }
    for (i = app.state.cProdGlob - 1; i >= 0; i--) {
        if (app.state.pProdGlob[i].grobj == grobjPlanet) {
            if (app.state.pProdGlob[i].iItem == mdIdleMine) {
                iMaxMines = (((0) > (ColonyIO().pfnOperableMines(app, lppl, app.state.idPlayer, 0) - ColonyIO().pfnMinesOperating(app, lppl) - iAddMines)) ? (0) : (ColonyIO().pfnOperableMines(app, lppl, app.state.idPlayer, 0) - ColonyIO().pfnMinesOperating(app, lppl) - iAddMines));
                ProductionIO().pfnQuote(app, lppl, app.state.pProdGlob + i, reinterpret_cast<std::uint32_t *>(rgMineCost), app.state.idPlayer, 1);
            }
            if (app.state.pProdGlob[i].iItem == mdIdleFactory) {
                iMaxFactories = (((0) > (ColonyIO().pfnOperableFactories(app, lppl, app.state.idPlayer, 0) - CFactoriesOperating(app, lppl) - iAddFactories)) ? (0) : (ColonyIO().pfnOperableFactories(app, lppl, app.state.idPlayer, 0) - CFactoriesOperating(app, lppl) - iAddFactories));
                ProductionIO().pfnQuote(app, lppl, app.state.pProdGlob + i, reinterpret_cast<std::uint32_t *>(rgFactCost), app.state.idPlayer, 1);
            }
            if (app.state.pProdGlob[i].iItem == mdIdleAlchemy) {
                ProductionIO().pfnQuote(app, lppl, app.state.pProdGlob + i, reinterpret_cast<std::uint32_t *>(rgAlchCost), app.state.idPlayer, 1);
            }
            if (app.state.pProdGlob[i].iItem == mdIdleTerraform) {
                iMaxTerra = app.state.pProdGlob[i].cItem;
            }
        }
    }
    if (iMaxTerra > 0 && app.state.rgplr[app.state.idPlayer].idAi == idAiMacinti) {
        cAdd = LOWORD(static_cast<std::int32_t>(((rgResLeft[3] + 69) / 70)));
        if (cAdd > iMaxTerra) {
            cAdd = iMaxTerra;
        }
        if (cAdd > 0) {
            AddItemToQueue(app, mdIdleTerraform, cAdd, grobjPlanet, addItemFront);
            return 1;
        }
    }
    if (rgResLeft[0] <= 0 || rgResLeft[1] <= 0 || rgResLeft[2] <= 0) {
        if (lppl->lpplprod) {
            prod = lppl->lpplprod->rgprod()[0];
            if (prod.grobj == grobjPlanet && (prod.iItem == iobjAlchemy || prod.iItem == mdIdleAlchemy)) {
                return 0;
            }
        }
        if (iMaxMines > 0) {
            iAddMines = (((iMaxMines) < (static_cast<std::int16_t>((rgResLeft[3] / rgMineCost[3])))) ? (iMaxMines) : (static_cast<std::int16_t>((rgResLeft[3] / rgMineCost[3]))));
        } else {
            iAddMines = 0;
        }
        rgResLeft[3] -= static_cast<std::uint32_t>((iAddMines * rgMineCost[3]));
        iAddFactories = 0;
        iAddAlchemy = 0;
        fInsert = 1;
        goto LAdd;
    }
    iMaxFactBuildable = iMaxFactories;
    if (iMaxFactories > 0) {
        if (!app.state.gd.fTutorial) {
            iMaxFactories = (((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[2] / rgFactCost[2])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[2] / rgFactCost[2]))));
        } else {
            iMaxFactories = (((((((((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0]))))) < (static_cast<std::int16_t>((rgResLeft[1] / rgFactCost[1])))) ? ((((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0]))))) : (static_cast<std::int16_t>((rgResLeft[1] / rgFactCost[1]))))) < (static_cast<std::int16_t>((rgResLeft[2] / rgFactCost[2])))) ? (((((((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0]))))) < (static_cast<std::int16_t>((rgResLeft[1] / rgFactCost[1])))) ? ((((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[0] / rgFactCost[0]))))) : (static_cast<std::int16_t>((rgResLeft[1] / rgFactCost[1]))))) : (static_cast<std::int16_t>((rgResLeft[2] / rgFactCost[2]))));
        }
    }
    if (iMaxFactories > 0) {
        iAddFactories = (((iMaxFactories) < (static_cast<std::int16_t>((rgResLeft[3] / rgFactCost[3])))) ? (iMaxFactories) : (static_cast<std::int16_t>((rgResLeft[3] / rgFactCost[3]))));
    } else {
        iAddFactories = 0;
    }
    rgResLeft[3] -= static_cast<std::uint32_t>((iAddFactories * rgFactCost[3]));
    if (iMaxMines > 0) {
        iAddMines = (((iMaxMines) < (static_cast<std::int16_t>((rgResLeft[3] / rgMineCost[3])))) ? (iMaxMines) : (static_cast<std::int16_t>((rgResLeft[3] / rgMineCost[3]))));
    } else {
        iAddMines = 0;
    }
    rgResLeft[3] -= static_cast<std::uint32_t>((iAddMines * rgMineCost[3]));
LAdd:
    if (rgAlchCost[3] > 0 && app.state.rgplr[app.state.idPlayer].rgTech[3] == 26 && app.state.rgplr[app.state.idPlayer].rgTech[5] == 26 && app.state.rgplr[app.state.idPlayer].rgTech[0] == 26 &&
        app.state.rgplr[app.state.idPlayer].rgTech[1] == 26 && app.state.rgplr[app.state.idPlayer].rgTech[4] == 26 && app.state.rgplr[app.state.idPlayer].rgTech[2] == 26) {
        iAddAlchemy = 0 <= LOWORD(static_cast<std::int32_t>((rgResLeft[3] / rgAlchCost[3]))) + 1 ? LOWORD(static_cast<std::int32_t>((rgResLeft[3] / rgAlchCost[3]))) + 1 : 0;
    } else {
        iAddAlchemy = 0;
    }
    rgResLeft[3] -= static_cast<std::uint32_t>((iAddAlchemy * rgAlchCost[3]));
    if (iAddFactories > 0) {
        AddItemToQueue(app, mdIdleFactory, iAddFactories, grobjPlanet, addItemEnd);
    }
    if (iAddMines > 0) {
        AddItemToQueue(app, mdIdleMine, iAddMines, grobjPlanet, addItemFront);
    }
    if (iAddAlchemy > 0 && app.state.game.turn > 100) {
        AddItemToQueue(app, mdIdleAlchemy, iAddAlchemy, grobjPlanet, !fInsert ? addItemEnd : addItemFront);
    }
    if (iAddMines + iAddFactories + iAddAlchemy > 0) {
        return 1;
    }
    return 0;
}

void DoCyberFreighter(APPSESSION &app, FLEET *lpfl, std::uint8_t *lpciPlanTemp) {
    ORDER   ord;
    PLANET *lpplDst;
    PLANET *lpplCur;
    std::int16_t fDropOff;
    std::int16_t idPlanDst;
    SCAN    scan;

    fDropOff = 0;
    lpplCur = LpplFromId(app, lpfl->idPlanet);
    if (!lpplCur) {
        if (FFindNearestObject(app, lpfl->pt, static_cast<GrobjClass>(grobjPlanet | mdNoRecurse), &scan)) {
            lpplDst = LpplFromId(app, scan.idpl);
        } else {
            lpplDst = nullptr;
        }
        goto LTarget;
    }
    if (lpplCur->iPlayer == app.state.idPlayer) {
        if (lpplCur->rgwtMin[3] > 2000) {
            XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 1000);
            FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
            fDropOff = 1;
        } else {
            XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, -1000);
            FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
        }
    } else if (lpplCur->iPlayer != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[lpplCur->iPlayer], rsMajorAdv) != raMacintosh && !lpplCur->fStarbase) {
        FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
        idPlanDst = lpplCur->id;
        std::memset(&ord, 0, sizeof(ORDER));
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
        FMoveToNearestStarbase(app, lpfl, 0);
        FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
    } else {
        fDropOff = lpfl->rgwtMin[3] > 0;
    }
    if (fDropOff) {
        lpplDst = LpplFindClosestEnum(app, lpplCur, FEnumDropOffStage1);
        if (!lpplDst) {
            lpplDst = LpplFindClosestEnum(app, lpplCur, FEnumDropOffStage2);
        }
        if (!lpplDst) {
            if (lpplCur->iPlayer == app.state.idPlayer) {
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, -1000);
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                if (CtvFromLp(app, lpciPlanTemp).At(lpplCur->id).cIdleFreighters() < 3) {
                    CtvFromLp(app, lpciPlanTemp).At(lpplCur->id).SetCIdleFreighters(CtvFromLp(app, lpciPlanTemp).At(lpplCur->id).cIdleFreighters() + 1);
                }
            }
        } else {
            if (CtvFromLp(app, lpciPlanTemp).At(lpplDst->id).cFreightersDst() < 3) {
                CtvFromLp(app, lpciPlanTemp).At(lpplDst->id).SetCFreightersDst(CtvFromLp(app, lpciPlanTemp).At(lpplDst->id).cFreightersDst() + 1);
            }
        }
    } else {
        lpplDst = LpplFindClosestEnum(app, lpplCur, FEnumPickUp);
    }
LTarget:
    if (lpplDst) {
        std::memset(&ord, 0, sizeof(ORDER));
        ord.pt = app.state.rgptPlan[lpplDst->id];
        ord.grobj = grobjPlanet;
        ord.id = lpplDst->id;
        ord.grTask = grTaskNone;
        ord.fValidTask = 1;
        ord.iWarp = static_cast<std::uint16_t>(IFindIdealWarp(app, lpfl, 1));
        if (FMoveAiFleet(app, lpfl, &ord, 0)) {
            lpfl->fMark = 1;
        }
    }
    return;
}

std::int16_t FEnumDropOffStage1(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::uint8_t       *lpciPlanTemp;
    std::int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    lpciPlanTemp = app.state.vlpbAiData + dOffsetPlanTemp;
    if (CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).cFreightersDst() == 3) {
        return 0;
    }
    if (lpplTest->iPlayer == app.state.idPlayer && lpplTest->rgwtMin[3] < 200 && CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).cFreightersDst() == 0) {
        if (LDistance2(app.state.rgptPlan[lpplSrc->id], app.state.rgptPlan[lpplTest->id]) <= 160000) {
            return 1;
        }
        return 0;
    }
    return 0;
}

std::int16_t FEnumDropOffStage2(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    std::uint8_t       *lpciPlanTemp;
    std::int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    lpciPlanTemp = app.state.vlpbAiData + dOffsetPlanTemp;
    if (CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).cFreightersDst() == 3) {
        return 0;
    }
    if (lpplTest->iPlayer != app.state.idPlayer || static_cast<std::uint32_t>((CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).cFreightersDst() * 210)) + lpplTest->rgwtMin[3] >= 1000) {
        return 0;
    }
    if (LDistance2(app.state.rgptPlan[lpplSrc->id], app.state.rgptPlan[lpplTest->id]) <= 160000) {
        return 1;
    }
    return 0;
}

std::int16_t FEnumPickUp(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer == app.state.idPlayer && lpplTest->rgwtMin[3] > 2200 && lpplTest->fStarbase) {
        return 1;
    }
    return 0;
}

std::int16_t FEnumNeedMinerals(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    double         dDistance;
    std::int16_t        iWarpSrc;
    std::int16_t        fTwoMA;
    std::int16_t        iMinLimit;
    std::uint8_t       *lpciPlan;
    std::int16_t        iWarpDst;
    std::uint8_t       *lpciPlanTemp;
    std::int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    lpciPlan = app.state.vlpbAiData + 2;
    lpciPlanTemp = app.state.vlpbAiData + dOffsetPlanTemp;
    switch (lpplTest->isb) {
    case 1:
    case 3:
    case 6:
    case 8:
        iMinLimit = 10;
        break;
    default:
        iMinLimit = 1000;
    }
    if (lpplTest->iPlayer != app.state.idPlayer || !lpplTest->fStarbase || CivFromLp(app, lpciPlan).At(lpplTest->id).iPktTarget() > 0) {
        return 0;
    }
    if ((lpplSrc->rgwtMin[0] > 700 && CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).fNeedsMin1()) || (lpplSrc->rgwtMin[1] > 700 && CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).fNeedsMin2()) ||
        (lpplSrc->rgwtMin[2] > 700 && CtvFromLp(app, lpciPlanTemp).At(lpplTest->id).fNeedsMin3())) {
        iWarpSrc = ColonyIO().pfnWarp(app, lpplSrc, &fTwoMA);
        if (fTwoMA) {
            iWarpSrc++;
        }
        iWarpDst = ColonyIO().pfnWarp(app, lpplTest, &fTwoMA);
        if (fTwoMA) {
            iWarpDst++;
        }
        dDistance = static_cast<double>((iWarpSrc >= iWarpDst ? static_cast<double>(iWarpDst) : static_cast<double>(iWarpSrc)));
        dDistance = static_cast<double>((static_cast<double>(dDistance) * dDistance * 3.5));
        if (static_cast<double>(LDistance2(app.state.rgptPlan[lpplSrc->id], app.state.rgptPlan[lpplTest->id])) <= static_cast<double>(dDistance) * dDistance) {
            return 1;
        }
        return 0;
    }
    return 0;
}

std::int16_t FEnumPktAttack(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    double     dDistance;
    std::int16_t    fTwoMA;
    double     dMod;
    std::int16_t    iWarp;
    std::uint8_t   *plMinMax;
    std::int32_t    lMineral;
    std::int32_t    lMinNeeded;
    std::uint8_t       *lpciPlan;
    std::int16_t    iWarpDst;
    std::int16_t    dOffsetPlanTemp;
    double     dDistanceTgt;

    lMineral = 0;
    iWarpDst = 0;
    dOffsetPlanTemp = app.state.game.cPlanMax * 2 + 2;
    lpciPlan = app.state.vlpbAiData + 2;
    plMinMax = app.state.vlpbAiData + (dOffsetPlanTemp + app.state.game.cPlanMax * 2);
    if (lpplTest->iPlayer == app.state.idPlayer || lpplTest->iPlayer == iplrNone || CivFromLp(app, lpciPlan).At(lpplTest->id).iPktTarget() > 0 ||
        RaceQueryIO().pfnStat(&app.state.rgplr[lpplTest->iPlayer], rsMajorAdv) == raMacintosh) {
        return 0;
    }
    if (lpplTest->fStarbase) {
        if (app.state.rglpshdefSB[lpplTest->iPlayer][lpplTest->isb].det != detAll) {
            return 0;
        }
        iWarpDst = ColonyIO().pfnWarp(app, lpplTest, &fTwoMA);
        if (fTwoMA) {
            iWarpDst++;
        }
    }
    dDistance = iWarp = ColonyIO().pfnWarp(app, lpplSrc, &fTwoMA) + 3;
    if (iWarp == iWarpDst) {
        return 0;
    }
    dDistance = static_cast<double>((static_cast<double>(dDistance) * dDistance));
    dDistanceTgt = DGetDistance(app.state.rgptPlan[lpplSrc->id].x, app.state.rgptPlan[lpplSrc->id].y, app.state.rgptPlan[lpplTest->id].x, app.state.rgptPlan[lpplTest->id].y);
    if (static_cast<double>(dDistanceTgt) <= static_cast<double>(dDistance) * 2.5) {
        if (fTwoMA) {
            lMineral = static_cast<std::int32_t>((static_cast<double>(static_cast<std::int32_t>(MivFromLp(app, plMinMax).value())) * std::pow(0.875, static_cast<double>((static_cast<double>(dDistanceTgt) / dDistance)))));
        } else {
            lMineral = static_cast<std::int32_t>((static_cast<double>(static_cast<std::int32_t>(MivFromLp(app, plMinMax).value())) * std::pow(0.75, static_cast<double>((static_cast<double>(dDistanceTgt) / dDistance)))));
        }
        if (lpplTest->uDefGuess > 99 || lpplTest->uPopGuess == 0) {
            return 0;
        }
        dMod = static_cast<double>((static_cast<double>(static_cast<std::uint32_t>(((iWarp * iWarp - iWarpDst * iWarpDst) * (100 - (lpplTest->uDefGuess + 5))))) / 16000.0));
        lMinNeeded =
            static_cast<std::int32_t>(((1000 >= static_cast<std::uint16_t>(((lpplTest->uPopGuess + 25) * 4)) ? static_cast<double>(static_cast<std::uint32_t>(((lpplTest->uPopGuess + 25) * 4))) : static_cast<double>(1000)) /
                      dMod));
        if (lMinNeeded <= lMineral) {
            return 1;
        }
        return 0;
    }
    return 0;
}

std::int16_t FEnumCalcEnemyPlanets(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer != app.state.idPlayer && lpplTest->iPlayer != iplrNone) {
        return 1;
    }
    return 0;
}

std::int16_t iBuildCyberStarbase(APPSESSION &app, PLANET *lppl) {
    std::int16_t ishdefSB;
    PROD    rgprod[64];

    if (lppl->fStarbase || PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer) < 15 || lppl->rgwtMin[3] < 500) {
        return -1;
    }
    ChangeMainObjSel(app, grobjPlanet, lppl->id);
    ProductionIO().pfnInitialize(app, rgprod);
    if (lppl->rgMinConc[0] > 15 && lppl->rgMinConc[1] > 15 && lppl->rgMinConc[2] > 15) {
        ishdefSB = IshdefAiSBLatest(app);
    } else {
        ishdefSB = IshdefAiSBLatestOF(app);
    }
    return ishdefSB;
}

void EnsureCyberAiShdefs(APPSESSION &app, std::int16_t iroCur) {
    std::int16_t low;
    std::int16_t ish;
    std::int16_t ishCur;
    std::int16_t i;
    std::int16_t high;
    SHDEF   shdef;

    if (!app.state.rgshdef[0].fFree && app.state.rgshdef[0].hul.ihuldef != ihuldefFrigate && app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && app.state.rgshdef[0].cExist == 0 && app.state.game.turn > 5) {
        shdef = app.state.rgshdef[0];
        shdef.fFree = 1;
        FChangeAiShdef(app, &shdef, 0);
    }
    if (app.state.rgshdef[0].fFree) {
        FCreateAiShdef(app, 0, ihuldefFrigate, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeFrigateScoutMineLayer]));
    }
    if (app.state.rgshdef[4].fFree && app.state.game.turn > 30 &&
        (app.state.game.turn > 75 || !FCreateAiShdef(app, 4, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeDestroyerBeamThrust])))) {
        for (i = 5; i > 0 && FCreateAiShdef(app, 4, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerBeamThrust + RandomIO().pfnRandom(app, i)]])) == 0;
             i--) {
        }
    }
    if (app.state.rgshdef[5].fFree && !app.state.rgshdef[4].fFree && app.state.game.turn > static_cast<std::uint16_t>((app.state.rgshdef[4].turn + 20)) &&
        (app.state.game.turn > 75 || !FCreateAiShdef(app, 5, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeDestroyerTorpedoThrust])))) {
        for (i = 5; i > 0 && FCreateAiShdef(app, 5, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerTorpedoThrust + RandomIO().pfnRandom(app, i)]])) == 0;
             i--) {
        }
    }
    if (app.state.rgshdef[2].fFree && app.state.game.turn > 20) {
        FCreateAiShdef(app, 2, ihuldefPrivateer, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipePrivateerBeam]));
    }
    if (app.state.rgshdef[3].fFree && !app.state.rgshdef[2].fFree && app.state.game.turn > static_cast<std::uint16_t>((app.state.rgshdef[2].turn + 20))) {
        FCreateAiShdef(app, 3, ihuldefPrivateer, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipePrivateerTorpedo]));
    }
    for (ish = 6; ish <= 10; ish += 4) {
        if (app.state.rgshdef[ish].fFree && ((ish == 6 && app.state.game.turn > 40) || (!app.state.rgshdef[6].fFree && app.state.game.turn > static_cast<std::uint16_t>((app.state.rgshdef[6].turn + 30))))) {
            ishCur = ish + 2;
            for (i = 3; i > 0; i--) {
                if (FCreateAiShdef(app, ishCur, ihuldefNubian, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeNubianMissileBeam + RandomIO().pfnRandom(app, i)]]))) {
                    ishCur--;
                    break;
                }
            }
            for (i = 4; i > 0; i--) {
                if (FCreateAiShdef(app, ishCur, ihuldefBattleship,
                                   static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMissileTorpedoThrust + RandomIO().pfnRandom(app, i)]]))) {
                    ishCur--;
                    break;
                }
            }
            for (i = 3; i > 0; i--) {
                if (FCreateAiShdef(app, ishCur, ihuldefBattleship, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMixedBeamsThrust + RandomIO().pfnRandom(app, i)]]))) {
                    ishCur--;
                    break;
                }
            }
            if (ishCur < ish)
                goto LBomber;
            high = 9 - (ishCur - ish) * 3;
            low = icyberRecipeBattleshipMixedBeamsThrust - high;
        LCruiser:
            for (i = high; i > 0; i--) {
                if (FCreateAiShdef(app, ishCur, ihuldefCruiser, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[RandomIO().pfnRandom(app, i) + low]])))
                    break;
            }
            ishCur--;
            if (ishCur < ish)
                goto LBomber;
            high = 3;
            low = ishCur == ish ? icyberRecipeCruiserStreamingBeams : icyberRecipeCruiserTorpedo;
            goto LCruiser;
        LBomber:
            if (!FCreateAiShdef(app, ish + 3, ihuldefNubian, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeNubianMcm])) &&
                !FCreateAiShdef(app, ish + 3, ihuldefBattleship, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeBattleshipMcm])) &&
                !FCreateAiShdef(app, ish + 3, ihuldefB52Bomber, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeB52RetroThenSmart]))) {
                FCreateAiShdef(app, ish + 3, ihuldefB52Bomber, static_cast<std::uint8_t *>(&vrgCyberAip[cyberRecipeB52SmartThenNormal]));
            }
        }
    }
    for (ish = 14; ish <= 15; ish++) {
        if (app.state.rgshdef[ish].fFree && ((ish == 14 && app.state.game.turn > 30) || (!app.state.rgshdef[14].fFree && app.state.game.turn > static_cast<std::uint16_t>((app.state.rgshdef[14].turn + 20))))) {
            for (i = 7; i > 0 && FCreateAiShdef(app, ish, ihuldefBattleship,
                                                static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMixedBeamsThrust + RandomIO().pfnRandom(app, i)]])) == 0;
                 i--) {
            }
            if (i == 0) {
                for (i = 9;
                     i > 0 && FCreateAiShdef(app, ish, ihuldefCruiser, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeCruiserStreamingBeams + RandomIO().pfnRandom(app, i)]])) == 0;
                     i--) {
                }
            }
            if (i == 0) {
                for (i = 10;
                     i > 0 && FCreateAiShdef(app, ish, ihuldefDestroyer, static_cast<std::uint8_t *>(&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerBeamThrust + RandomIO().pfnRandom(app, i)]])) == 0;
                     i--) {
                }
            }
        }
    }
    return;
}

std::int16_t iAddAttackFleet(APPSESSION &app, PLANET *lppl, std::int16_t iAttackStr, std::int16_t iBestDestroyer, std::int16_t iBestBattle, std::int16_t iBestSBDefender) {
    std::int16_t fRet;
    std::int16_t iMaxFactories;
    std::int16_t iMaxMines;
    std::int16_t iRand;

    fRet = 0;
    iRand = RandomIO().pfnRandom(app, 100);
    iMaxMines = (((0) > (ColonyIO().pfnOperableMines(app, lppl, app.state.idPlayer, 0) - ColonyIO().pfnMinesOperating(app, lppl))) ? (0) : (ColonyIO().pfnOperableMines(app, lppl, app.state.idPlayer, 0) - ColonyIO().pfnMinesOperating(app, lppl)));
    iMaxFactories = (((0) > (ColonyIO().pfnOperableFactories(app, lppl, app.state.idPlayer, 0) - CFactoriesOperating(app, lppl))) ? (0) : (ColonyIO().pfnOperableFactories(app, lppl, app.state.idPlayer, 0) - CFactoriesOperating(app, lppl)));
    if (RandomIO().pfnRandom(app, 100) < (iMaxMines < 100 || iMaxFactories < 100 ? 90 : 60)) {
        return 0;
    }
    if (iBestBattle != ishdefNone && iRand > 50) {
        if (!app.state.rgshdef[iBestBattle * 4 + 6].fFree) {
            AddItemToQueue(app, iBestBattle * 4 + 6, 2, grobjFleet, addItemEnd);
        }
        if (!app.state.rgshdef[iBestBattle * 4 + 7].fFree) {
            AddItemToQueue(app, iBestBattle * 4 + 7, 2, grobjFleet, addItemEnd);
        }
        if (!app.state.rgshdef[iBestBattle * 4 + 8].fFree && RandomIO().pfnRandom(app, 100) < 75) {
            AddItemToQueue(app, iBestBattle * 4 + 8, 1, grobjFleet, addItemEnd);
        }
        if (!app.state.rgshdef[iBestBattle * 4 + 9].fFree && RandomIO().pfnRandom(app, 100) < 50) {
            AddItemToQueue(app, iBestBattle * 4 + 9, 1, grobjFleet, addItemEnd);
        }
        return 1;
    }
    if (iBestSBDefender != ishdefNone && iRand > 25) {
        AddItemToQueue(app, iBestSBDefender, 1, grobjFleet, addItemEnd);
        return 2;
    }
    if (iBestDestroyer != ishdefNone) {
        AddItemToQueue(app, iBestDestroyer, 1, grobjFleet, addItemEnd);
        return 3;
    }
    return 0;
}

void TargetCyberArmada(APPSESSION &app, FLEET *lpfl) {
    FLEET  *lpflTarget;
    ORDER   ord;
    PLANET *lppl;
    std::int16_t cshBomb;
    std::int16_t cshWar;
    PLANET *lpplTarget;

    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord()[1];
        if (LDistance2(lpfl->pt, ord.pt) > 62500 && ord.grobj == grobjFleet)
            goto LTryNewTarget;
        if (ord.grobj == grobjFleet) {
            return;
        }
        if (ord.grobj == grobjPlanet) {
            lppl = LpplFromId(app, ord.id);
            if (!lppl || ((lppl->iPlayer != iplrNone && (lppl->iPlayer != app.state.idPlayer || lppl->fStarbase)) || lppl->turn != app.state.game.turn)) {
                return;
            }
        }
    }
LTryNewTarget:
    cshWar = 0;
    cshWar += lpfl->rgcsh[6];
    cshWar += lpfl->rgcsh[7];
    cshWar += lpfl->rgcsh[8] * 2;
    cshWar += lpfl->rgcsh[10];
    cshWar += lpfl->rgcsh[11];
    cshWar += lpfl->rgcsh[12] * 2;
    cshBomb = lpfl->rgcsh[9] + lpfl->rgcsh[13];
    lpfl->fMark = 1;
    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    if (lpfl->idPlanet == idPlanetDeepSpace) {
        MoveToNearestPlanetOrEnemy(app, lpfl, 450);
        return;
    }
    lppl = LpplFromId(app, lpfl->idPlanet);
    if (lppl->iPlayer == app.state.idPlayer) {
        if (cshWar >= app.state.vrgAiArmadaPotency[0] && cshBomb >= app.state.vrgAiArmadaPotency[2]) {
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
                if (!FMoveAiFleet(app, lpfl, &ord, 0))
                    return;
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
    }
}
