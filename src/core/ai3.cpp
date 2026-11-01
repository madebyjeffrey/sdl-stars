#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "capacity_io.h"
#include "random_io.h"

void DoMacintiAiTurn(APPSESSION &app, PROD *rgprod) {
    int16_t  iLatestCargo;
    int16_t  cColFleet;
    int32_t  rgResCost[4];
    int16_t  idPlanDst;
    int16_t  j;
    FLEET   *lpflEnemy;
    int32_t  rgResAvail[4];
    PLANET  *lpplMac;
    int16_t  iLatestCruiser;
    int16_t  ishLastBattle;
    THING   *lpthWorm;
    int16_t  cFlMineLayers;
    int16_t  fShouldColonize;
    int16_t  cFlDestroyers;
    int16_t  cshDestroyer;
    int16_t  iAiLvl;
    int16_t  iLatestBattle;
    PLANET  *lppl;
    int16_t  cFlCargo;
    int16_t  iLatestBomber;
    int16_t  ifl;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  cGenesis;
    int16_t  cFlArmadas;
    int16_t  cRes;
    int16_t  cFr;
    int16_t  iroCur;
    int16_t  iLatestMiner;
    int16_t  fUsingTempColonizer;
    int16_t  iLatestDestroyer;
    int16_t  ipl;
    int16_t  iLatestColony;
    uint16_t cRecyclePeriod;
    uint8_t  rgRecycleShdef[16];
    uint8_t *lpb;
    int16_t  cFlMineLayersBase;
    int16_t  cFlMiners;
    uint16_t rgCosts[4];
    FLEET   *lpflAttack;
    int16_t  iPlanet;
    int32_t  l;
    int16_t  fWrite;
    int16_t  fTonsOfMinerals;
    uint8_t  rgRecycleSBShdef[10];
    PROD    *lpprod;
    PART     part;
    SHDEF    shdef;
    PLANET  *lpplDest;
    int32_t  lLeast;
    PLANET  *lpplBest;
    int16_t  iplDest;
    int32_t  cMine;
    int16_t  id;
    int16_t  cConc;
    int16_t  iLatest;
    int16_t  dy;
    int32_t  lDist;
    int16_t  dx;
    ORDER    ord;
    PLANET  *lpplDrop;
    uint8_t  rgSplitShdef[16];

    iAiLvl = app.state.rgplr[app.state.idPlayer].lvlAi;
    iPlanet = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    iroCur = IroEnsureAi(app, vrgAiMacintiResOrder, cAiMacintiResOrder, nullptr, 15);
    if (app.state.game.turn < 40 || (!app.state.rgshdef[7].fFree && app.state.rgshdef[7].hul.ihuldef == ihuldefColonyShip)) {
        fUsingTempColonizer = 1;
        if (FLookupPartX(app, &part, hstEngine, iengineGalaxyScoop) == mdPartAvailAvailable && app.state.rgshdef[7].cExist == 0) {
            shdef = app.state.rgshdef[7];
            shdef.fFree = 1;
            FChangeAiShdef(app, &shdef, 7);
            fUsingTempColonizer = 0;
        }
    } else {
        fUsingTempColonizer = 0;
    }
    ishLastBattle = 7 - fUsingTempColonizer;
    if (fUsingTempColonizer) {
        MergeAllShdefs(app, 892);
    } else {
        MergeAllShdefs(app, 1020);
    }
    MergeAllShdefs(app, 1);
    MergeAllShdefs(app, -16384);
    MergeAllShdefs(app, 12288);
    j = 6;
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
    cshDestroyer = CheckAiShdefStatus(app, 12, 13, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    for (i = 12; i <= 13; i++) {
        if (rgRecycleShdef[i] != 0 && !app.state.rgshdef[i].fFree && app.state.rgshdef[i].hul.ihuldef == ihuldefNubian) {
            rgRecycleShdef[i] = 0;
        }
    }
    CheckAiShdefStatus(app, 14, 15, 5000, &iLatestMiner, rgRecycleShdef);
    if (!app.state.rgshdef[15].fFree && FLookupPartX(app, &part, hstMining, iminingAlienMiner) == mdPartAvailAvailable) {
        i = iLatestMiner == 14 ? 15 : 14;
        if (app.state.rgshdef[14].hul.rghs[2].iItem != iminingAlienMiner) {
            i = 14;
        LUpgradeMiner:
            if (app.state.rgshdef[i].cExist == 0) {
                shdef = app.state.rgshdef[i];
                shdef.fFree = 1;
                FChangeAiShdef(app, &shdef, i);
            } else {
                rgRecycleShdef[14] = 3;
            }
        } else if (app.state.rgshdef[15].hul.rghs[2].iItem != iminingAlienMiner && FLookupPartX(app, &part, hstEngine, iengineGalaxyScoop) == mdPartAvailAvailable &&
                   app.state.rgshdef[14].hul.rghs[0].iItem != iengineGalaxyScoop) {
            i = 15;
            goto LUpgradeMiner;
        }
    }
    CheckAiShdefStatus(app, 10, 11, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    CheckAiShdefStatus(app, 8, 9, cRecyclePeriod, &iLatestBomber, rgRecycleShdef);
    CheckAiShdefStatus(app, 2, 4, cRecyclePeriod, &iLatestCruiser, rgRecycleShdef);
    CheckAiShdefStatus(app, 5, ishLastBattle, cRecyclePeriod, &iLatestBattle, rgRecycleShdef);
    if (app.state.game.turn > 80) {
        SplitOutShdefs(app, rgRecycleShdef);
        memset(rgSplitShdef, 0, 16);
        rgSplitShdef[0] = 2;
        SplitOutShdefs(app, rgSplitShdef);
        memset(rgSplitShdef, 0, 16);
        rgSplitShdef[1] = 2;
        SplitOutShdefs(app, rgSplitShdef);
        memset(rgSplitShdef, 0, 16);
        rgSplitShdef[15] = 2;
        rgSplitShdef[14] = 2;
        SplitOutShdefs(app, rgSplitShdef);
        memset(rgSplitShdef, 0, 16);
        rgSplitShdef[11] = 2;
        rgSplitShdef[10] = 2;
        SplitOutShdefs(app, rgSplitShdef);
    }
    EnsureMacintiShdefs(app);
    EnsureMacintiStarbaseDesigns(app, rgRecycleSBShdef);
    app.state.vAiMacRecycleSB = rgRecycleSBShdef;
    fShouldColonize = FShouldWeBuildColonizers(app, &cColFleet);
    if (app.state.rgshdef[1].hul.rghs[0].iItem == iengineGalaxyScoop) {
        iLatestColony = 1;
    } else if (!app.state.rgshdef[7].fFree && app.state.rgshdef[7].hul.ihuldef == ihuldefColonyShip) {
        iLatestColony = 7;
    } else {
        iLatestColony = 1;
    }
    if (iLatestColony == 1 && fUsingTempColonizer && static_cast<uint16_t>((app.state.game.turn - app.state.rgshdef[1].turn)) > 5) {
        rgRecycleShdef[7] = 2;
    }
    lpb = app.state.vlpbAiPlanet + 14;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        *lpb = 0;
        i++;
        lpb += 16;
    }
    cFlMiners = 0;
    cFlCargo = 0;
    cFlDestroyers = 0;
    cFlArmadas = 0;
    cFlMineLayers = 0;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer) {
            if (iLatestMiner != ishdefNone && (lpfl->rgcsh[14] != 0 || lpfl->rgcsh[15] != 0)) {
                cFlMiners++;
            }
            if (lpfl->rgcsh[0] != 0) {
                cFlMineLayers++;
            }
            if (iLatestDestroyer != ishdefNone && (lpfl->rgcsh[12] != 0 || lpfl->rgcsh[13] != 0)) {
                cFlDestroyers++;
            }
            if (iLatestCargo != ishdefNone && (lpfl->rgcsh[10] != 0 || lpfl->rgcsh[11] != 0)) {
                cFlCargo++;
                if (lpfl->idPlanet == idPlanetDeepSpace && lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet && lpfl->rgwtMin[3] > 0) {
                    app.state.vlpbAiPlanet[lpfl->lpplord->rgord()[1].id * 16 + 14] = app.state.vlpbAiPlanet[lpfl->lpplord->rgord()[1].id * 16 + 0xe] | 1;
                }
            }
            for (i = 2; i <= 9; i++) {
                if (lpfl->rgcsh[i] != 0) {
                    cFlArmadas++;
                    break;
                }
            }
        }
    }
    cFlMineLayersBase = cFlMineLayers;
    lpb = app.state.vlpbAiPlanet + 13;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        *lpb = 0;
        i++;
        lpb += 16;
    }
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        app.state.vlpbAiPlanet[lppl->id * 16 + 13] = 1;
    }
    if (app.state.game.turn > 120 && FLookupPartX(app, &part, hstPlanetary, iplanetaryNeutronShield) == mdPartAvailAvailable) {
        cGenesis = app.state.rgplr[app.state.idPlayer].cPlanet / 20;
        if (cGenesis > 10) {
            cGenesis = 10;
        }
    } else {
        cGenesis = 0;
    }
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
        if (lppl->fStarbase && lppl->rgwtMin[3] >= 200 && lppl->isb != 0 && (lppl->isb != 1 || app.state.game.turn <= 25)) {
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            if (!lppl->lpplprod || lppl->lpplprod->iprodMac < 24) {
                ProductionIO().pfnInitialize(app, rgprod);
                fWrite = 0;
                i = 0;
                for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm); lpprod++) {
                    i++;
                }
                if (i < app.state.lpplProdGlob->iprodMac) {
                    FinishProduction(app, 0);
                } else {
                    cRes = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                    if (app.state.game.turn > 120 && ColonyIO().pfnWarp(app, lppl, &j) >= 10 && lppl->rgwtMin[3] > 10000 && RandomIO().pfnRandom(app, 4) == 0) {
                        lpplBest = nullptr;
                        lLeast = 100000;
                        i = 0;
                        lpprod = app.state.lpplProdGlob->rgprod();
                        while (i < app.state.lpplProdGlob->iprodMac) {
                            if (lpprod->grobj == grobjPlanet && lpprod->iItem >= iobjPacketIron && lpprod->iItem <= iobjPacketMixed)
                                goto LTryCargo;
                            i++;
                            lpprod++;
                        }
                        j = RandomIO().pfnRandom(app, 3);
                        for (i = j; i < j + 3 && lppl->rgwtMin[i % 3] <= 5000; i++) {
                        }
                        if (i != j + 3) {
                            i %= 3;
                            for (iplDest = 0; iplDest < app.state.vclpplAi; iplDest++) {
                                lpplDest = app.state.vrglpplAi[iplDest];
                                if (!app.state.vrglpplAi[iplDest])
                                    break;
                                if (lpplDest->rgwtMin[i] < lLeast && ColonyIO().pfnWarp(app, lpplDest, &j) >= 10) {
                                    l = LDistance2(app.state.rgptPlan[lppl->id], app.state.rgptPlan[lpplDest->id]);
                                    if (l < 91204) {
                                        lLeast = lpplDest->rgwtMin[i];
                                        lpplBest = lpplDest;
                                    }
                                }
                            }
                            if (lpplBest && lLeast < static_cast<int32_t>((lppl->rgwtMin[i] / 5))) {
                                l = static_cast<int32_t>((lppl->rgwtMin[i] / 5));
                                if (l > 20000) {
                                    l = 20000;
                                }
                                l = static_cast<int32_t>((l / 100));
                                AddItemToQueue(app, i + 14, LOWORD(l), grobjPlanet, addItemEnd);
                                FinishProduction(app, 1);
                                app.state.sel.pl.iWarpFling = 7;
                                app.state.sel.pl.idFling = lpplBest->id + 1;
                                PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
                                continue;
                            }
                        }
                    }
                LTryCargo:
                    if (iLatestCargo != ishdefNone && cFlCargo < 64 && cFlCargo < app.state.rgplr[app.state.idPlayer].cPlanet / 4 && RandomIO().pfnRandom(app, 3) == 0) {
                        cFlCargo++;
                        AddItemToQueue(app, iLatestCargo, 1, grobjFleet, addItemEnd);
                        fWrite = 1;
                    }
                    if ((!fShouldColonize || (cColFleet > 40 && (app.state.game.turn > 120 || cColFleet > 100))) && RandomIO().pfnRandom(app, 100) >= 8)
                        goto TryShip2;
                    if (cColFleet >= 50 && app.state.game.turn > 120)
                        goto TryShip2;
                    for (i = 0; i <= 2; i++) {
                        if (lppl->rgwtMin[i] < 30)
                            goto FinishProd;
                    }
                    if (!FShouldPlanetBuildColonizer(app, lppl))
                        goto TryShip2;
                    cColFleet++;
                    AddItemToQueue(app, iLatestColony, 1, grobjFleet, addItemEnd);
                    fWrite = 1;
                    if (app.state.game.turn < 5)
                        goto FinishProd;
                    l = static_cast<uint32_t>((lppl->rgwtMin[3] * PctTrueMaxGrowth(app, app.state.idPlayer)));
                    if (l > 2300 && cRes > 35 && iAiLvl > 0) {
                        cColFleet++;
                        AddItemToQueue(app, iLatestColony, 1, grobjFleet, addItemEnd);
                        if (l > 3600 && cRes > 50 && iAiLvl > 1) {
                            AddItemToQueue(app, iLatestColony, 1, grobjFleet, addItemEnd);
                        }
                    }
                TryShip2:
                    if (iLatestMiner != ishdefNone && cFlMiners < 60 && app.state.rgshdef[iLatestMiner].cExist < 5000 && RandomIO().pfnRandom(app, 2) == 0) {
                        id = lppl->id;
                        cMine = 0;
                        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                            lpfl = app.state.rglpfl[ifl];
                            if (!app.state.rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->rgcsh[iLatestMiner] > 0 && lpfl->iPlayer == app.state.idPlayer) {
                                cMine = ColonyIO().pfnFleetMining(app, lpfl);
                                break;
                            }
                        }
                        if (cMine > 1000)
                            goto TryShip2b;
                        cFr = 0;
                        for (i = 0; i < 3; i++) {
                            cFr += lppl->rgMinConc[i];
                        }
                        cFr = 3 * cFr;
                        if ((cMine < cFr && cFr > 150) || (cFlMiners < 30 && RandomIO().pfnRandom(app, 10) != 0)) {
                            cFlMiners++;
                            AddItemToQueue(app, iLatestMiner, 1, grobjFleet, addItemEnd);
                            fWrite = 1;
                        }
                    }
                TryShip2b:
                    if (app.state.rgshdef[0].hul.ihuldef == ihuldefFrigate && cFlMineLayers < 60 &&
                        (iLatestMiner != ishdefNone ? app.state.rgshdef[iLatestMiner].cExist : 0) < 7500 && RandomIO().pfnRandom(app, 4) == 0) {
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
                            cFlMineLayers += 3;
                            AddItemToQueue(app, 0, 4, grobjFleet, addItemEnd);
                            fWrite = 1;
                        }
                    }
                    for (i = 0; i <= 2 && lppl->rgwtMin[i] >= 5000; i++) {
                    }
                    fTonsOfMinerals = i == 2;
                    if (iLatestBomber != ishdefNone && cFlArmadas < 140 && (cFlArmadas < 60 || cRes > 2000)) {
                        id = lppl->id;
                        if (cFlArmadas > 110 && RandomIO().pfnRandom(app, 3) == 0)
                            goto LAddBombers;
                        for (ifl = 0; ifl < app.state.cFleet && (lpfl = app.state.rglpfl[ifl]) != 0; ifl++) {
                            if (lpfl->idPlanet != id || lpfl->iPlayer != app.state.idPlayer)
                                continue;
                            if (!FPotentMacWarFleet(app, lpfl, nullptr))
                                continue;

                            if (iLatestBomber == ishdefNone || lpfl->rgcsh[8] + lpfl->rgcsh[9] >= app.state.vrgAiArmadaPotency[2])
                                break;

                        LAddBombers:
                            cFlArmadas += 2;
                            AddItemToQueue(app, iLatestBomber, !fTonsOfMinerals ? 4 : 12, grobjFleet, addItemEnd);
                            fWrite = 1;
                            goto FinishProd;
                        }
                    }
                    if (cGenesis > 0 && lppl->rgwtMin[3] > 10000) {
                        i = 0;
                        for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjPlanet || lpprod->iItem != iobjGenesis);
                             lpprod++) {
                            i++;
                        }
                        if (i < app.state.lpplProdGlob->iprodMac)
                            goto LTryCruiser;
                        for (i = 0; i <= 2 && lppl->rgwtMin[i] >= 2000; i++) {
                        }
                        if (i == 2)
                            goto LTryCruiser;
                        cConc = 0;
                        for (i = 0; i < 3; i++) {
                            cConc += lppl->rgMinConc[i];
                        }
                        if (cConc < 15 || (cConc < 30 && RandomIO().pfnRandom(app, 3) != 0) || (cConc < 60 && RandomIO().pfnRandom(app, 5) != 0)) {
                            cGenesis--;
                            AddItemToQueue(app, iobjGenesis, 1, grobjPlanet, addItemEnd);
                            AddItemToQueue(app, mdIdleTerraform, 75, grobjPlanet, addItemEnd);
                            fWrite = 1;
                        }
                    }
                LTryCruiser:
                    if (iLatestCruiser != ishdefNone && cFlArmadas < 130 && app.state.game.turn > 20 && (cFlArmadas < 50 || cRes > 2000)) {
                        if (!fTonsOfMinerals && RandomIO().pfnRandom(app, 3) != 0)
                            goto TryShip3;
                        if (iLatestBattle != ishdefNone && RandomIO().pfnRandom(app, 3) == 0) {
                            iLatest = iLatestBattle;
                        } else {
                            iLatest = iLatestCruiser;
                        }
                        cFlArmadas++;
                        AddItemToQueue(app, iLatest, !fTonsOfMinerals ? 2 : 10, grobjFleet, addItemEnd);
                        fWrite = 1;
                        if (fTonsOfMinerals)
                            goto FinishProd;
                    }
                TryShip3:
                    if (iLatestDestroyer != ishdefNone && cFlDestroyers < (app.state.game.turn >= 120 ? 60 : 80) && cshDestroyer < 2000) {
                        GetResourcesAvailable(app, lppl, rgResAvail);
                        GetProdQCost(app, lppl, rgResCost);
                        for (i = 0; i < 4; i++) {
                            rgResAvail[i] -= rgResCost[i];
                            if (rgResAvail[i] < 0)
                                goto FinishProd;
                        }
                        GetTrueHullCost(app.state.idPlayer, &app.state.rgshdef[iLatestDestroyer].hul, rgCosts);
                        for (i = 0; i < 20; i++) {
                            for (j = 0; j < 4; j++) {
                                rgResAvail[j] -= static_cast<uint32_t>(rgCosts[j]);
                                if (rgResAvail[j] < 0)
                                    break;
                            }
                            if (j < 4)
                                break;
                        }
                        if (i > 0) {
                            fWrite = 1;
                            AddItemToQueue(app, iLatestDestroyer, i, grobjFleet, addItemEnd);
                            cFlDestroyers++;
                        }
                    }
                FinishProd:
                    FinishProduction(app, fWrite);
                }
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
            lDist = static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy));
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
            if (lpfl->rgcsh[0] > 0 && app.state.game.turn > 40) {
                if (lpfl->cord > 1) {
                    if (lpfl->lpplord->rgord()[0].grTask == grTaskNone)
                        continue;
                    ClearAiCurrentTask(app, lpfl, 1);
                    continue;
                }
                if ((cFlMineLayersBase > 55 || (cFlMineLayersBase > 40 && RandomIO().pfnRandom(app, 3) != 0)) && FFindBuddyAndJoinUp(app, lpfl, 0, 0, 72, 108))
                    continue;
                if (lpfl->rgcsh[0] >= 7 && RandomIO().pfnRandom(app, 5) == 0 && (idPlanDst = IdRandomPlanetNearby(app, lpfl->pt, 105, 1)) != idplNone &&
                    idPlanDst != lpfl->idPlanet) {
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
                } else if (lpfl->lpplord->rgord()[0].grTask != grTaskLayMines) {
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskLayMines;
                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTime = 5;
                    app.state.sel.fl.lpplord->rgord()[0].tlm().cTimeOld = 5;
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    continue;
                }
            } else if ((lpfl->rgcsh[14] > 0 || lpfl->rgcsh[15] > 0) && lpfl->cord == 1) {
                if (lpfl->lpplord->rgord()[0].grTask != grTaskMine) {
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskMine;
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    continue;
                }
                if ((cFlMiners > 58 || (cFlMiners > 48 && RandomIO().pfnRandom(app, 3) != 0)) && FFindBuddyAndJoinUp(app, lpfl, 14, 15, 72, 108))
                    continue;
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == app.state.idPlayer) {
                        j = 0;
                        for (i = 0; i < 3; i++) {
                            j += lppl->rgMinConc[i];
                        }
                        if (j < 30 || (j < 60 && RandomIO().pfnRandom(app, 3) == 0)) {
                            FRetargetMiner(app, lpfl);
                            continue;
                        }
                    }
                }
                if (RandomIO().pfnRandom(app, 10) == 0) {
                    FRetargetMiner(app, lpfl);
                }
            } else if (FIsAiAttack(app, lpfl)) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
                for (j = 2; j <= ishLastBattle && lpfl->rgcsh[j] <= 0; j++) {
                }
                if (j <= ishLastBattle && ((lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) || lpfl->idPlanet != idPlanetDeepSpace)) {
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
            if (app.state.game.turn <= 10 && (lpfl->rgcsh[0] > 0 || lpfl->rgcsh[2] != 0)) {
            LRecycle:
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                continue;
            }

            if (lpfl->cord > 1) {
                if (!fUsingTempColonizer || rgRecycleShdef[7] == 0 || lpfl->rgcsh[7] == 0)
                    continue;
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.cord = 1;
                app.state.sel.fl.lpplord->iordMac = 1;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                FMoveToNearestStarbase(app, lpfl, 0);
                continue;
            }
            if (lpfl->rgcsh[1] == 0 && (!fUsingTempColonizer || lpfl->rgcsh[7] == 0))
                continue;
            if (fUsingTempColonizer && iLatestColony == 1 && lpfl->idPlanet != idPlanetDeepSpace)
                goto LRecycle;

            if (iAiLvl > 1 && lpfl->idPlanet != idPlanetDeepSpace) {
                lpplDrop = LpplFromId(app, lpfl->idPlanet);
                if (lpplDrop && lpplDrop->iPlayer != iplrNone && lpplDrop->iPlayer != app.state.idPlayer && !lpplDrop->fStarbase && lpplDrop->uPopGuess < 50)
                    continue;
                if (lpplDrop && lpplDrop->iPlayer == iplrNone) {
                    if (lpfl->rgwtMin[3] == 0) {
                        FMoveToNearestStarbase(app, lpfl, 1);
                        continue;
                    }
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskColonize;
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    continue;
                }
            }
            idPlanDst = IdNearestColonizablePlanet(app, lpfl, &lpthWorm);
            if (idPlanDst == idplNone && !lpthWorm) {
                if (lpfl->idPlanet == idPlanetDeepSpace)
                    continue;
                ChangeMainObjSel(app, grobjFleet, lpfl->id);
                app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskScrap;
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                continue;
            }
            ChangeMainObjSel(app, grobjFleet, lpfl->id);
            if (lpfl->idPlanet != idPlanetDeepSpace) {
                lppl = LpplFromId(app, lpfl->idPlanet);
                if (lppl) {
                    l = static_cast<int32_t>((lppl->rgwtMin[3] / 10));
                    if (l > 25) {
                        l = 25;
                    }
                } else {
                    l = 0;
                }
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, LOWORD(l));
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
            }
            if (idPlanDst != idplNone) {
                FColonizeAiFleet(app, lpfl, idPlanDst);
                continue;
            }
            FGotoWormholeAiFleet(app, lpfl, lpthWorm);
            continue;
        }
    }
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
            IdTargetMacFreighter(app, lpfl);
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
                if ((lpfl->cord > 1 && lpfl->idPlanet == idPlanetDeepSpace && lpfl->lpplord->rgord()[1].grobj == grobjPlanet) ||
                    FMoveToNearestStarbase(app, lpfl, 0))
                    continue;
            }
            for (i = 2; i <= 9; i++) {
                if (lpfl->rgcsh[i] > 0 && (i != 7 || !fUsingTempColonizer)) {
                    if (cFlArmadas > 100 || (cFlArmadas > 90 && RandomIO().pfnRandom(app, 3) == 0)) {
                        l = 0;
                        for (j = 2; j <= 9; j++) {
                            l += lpfl->rgcsh[j];
                        }
                        if ((RandomIO().pfnRandom(app, 100) > l - 10 || RandomIO().pfnRandom(app, 20) == 0) && FFindBuddyAndJoinUp(app, lpfl, 2, 9, 100, 200))
                            break;
                    }
                    TargetMacArmada(app, lpfl);
                    break;
                }
            }
            if (i > 9 && FIsAiAttack(app, lpfl) && (lpfl->cord <= 1 || lpfl->lpplord->rgord()[1].grobj != grobjFleet) &&
                ((cFlDestroyers <= (app.state.game.turn <= 120 ? 70 : 50) && (cFlDestroyers <= (app.state.game.turn <= 120 ? 60 : 40) || RandomIO().pfnRandom(app, 3) != 0)) ||
                 ((iLatestDestroyer != ishdefNone && lpfl->rgcsh[iLatestDestroyer] >= 20 && RandomIO().pfnRandom(app, 20) != 0) || !FFindBuddyAndJoinUp(app, lpfl, 12, 13, 36, 72)))) {
                IdTargetAttack(app, lpfl, lpflAttack, lpflEnemy, app.state.game.fAisBand);
            }
        }
    }
    HandleBasicAiTasks(app, iroCur, rgprod, 0, rgResAvail, rgResCost);
    FillProductionQueue(app);
    return;
}

void EnsureMacintiShdefs(APPSESSION &app) {
    int16_t ish;
    int16_t i;
    PART    part;
    int16_t fAdvanced;
    SHDEF   shdef;
    int16_t shBase;

    for (ish = 14; ish <= 15; ish++) {
        if (app.state.rgshdef[ish].fFree) {
            fAdvanced = app.state.rgplr[app.state.idPlayer].lvlAi >= lvlAiTough;
            if ((!fAdvanced || ish != 15 || app.state.rgplr[app.state.idPlayer].rgTech[3] >= 15) &&
                !FCreateAiShdef(app, ish, static_cast<HulDef>(ihuldefUltraMiner - (fAdvanced == 0)), static_cast<uint8_t *>(&vrgMacAip[vrgMacIshAip[imacRecipeMaxiMiner + fAdvanced]])) && ish == 14) {
                if (fAdvanced) {
                    FCreateAiShdef(app, ish, ihuldefMiner, static_cast<uint8_t *>(&vrgMacAip[macRecipeUltraMinerOrMiner]));
                } else {
                    FCreateAiShdef(app, ish, ihuldefMiniMiner, static_cast<uint8_t *>(&vrgMacAip[macRecipeMiniMiner]));
                }
            }
        }
    }
    if (app.state.rgshdef[12].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 6 &&
        !FCreateAiShdef(app, 12, ihuldefNubian, static_cast<uint8_t *>(&vrgMacAip[macRecipeNubianMixedBeams]))) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 12, ihuldefDestroyer, static_cast<uint8_t *>(&vrgMacAip[vrgMacIshAip[imacRecipeDestroyerBeamA + RandomIO().pfnRandom(app, 4)]])) == 0; i++) {
        }
    }
    if (app.state.rgshdef[13].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 10 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 9 &&
        !FCreateAiShdef(app, 13, ihuldefNubian, static_cast<uint8_t *>(&vrgMacAip[macRecipeNubianMixedBeams]))) {
        for (i = 0; i < 5 && FCreateAiShdef(app, 13, ihuldefDestroyer, static_cast<uint8_t *>(&vrgMacAip[vrgMacIshAip[imacRecipeDestroyerTorpedoShield + RandomIO().pfnRandom(app, 4)]])) == 0;
             i++) {
        }
    }
    if (app.state.rgshdef[10].fFree && !FCreateAiShdef(app, 10, ihuldefLargeFreighter, static_cast<uint8_t *>(&vrgMacAip[macRecipeFreighter]))) {
        FCreateAiShdef(app, 10, ihuldefMediumFreighter, static_cast<uint8_t *>(&vrgMacAip[macRecipeFreighter]));
    }
    if (app.state.rgshdef[11].fFree) {
        FCreateAiShdef(app, 11, ihuldefLargeFreighter, static_cast<uint8_t *>(&vrgMacAip[macRecipeFreighter]));
    }
    if (app.state.game.turn < 20 && !app.state.rgshdef[2].fFree && app.state.rgshdef[2].cExist == 0) {
        shdef = app.state.rgshdef[2];
        shdef.fFree = 1;
        FChangeAiShdef(app, &shdef, 2);
    }
    for (ish = 2; ish <= 4; ish++) {
        if (app.state.rgshdef[ish].fFree && (ish == 2 || static_cast<uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 20)) {
            for (i = 0; i < 5 && FCreateAiShdef(app, ish, ihuldefCruiser, static_cast<uint8_t *>(&vrgMacAip[vrgMacIshAip[imacRecipeCruiserAntiMatterMcm + RandomIO().pfnRandom(app, 4)]])) == 0;
                 i++) {
            }
        }
    }
    if (app.state.game.turn < 40 && app.state.rgshdef[7].fFree) {
        FCreateAiShdef(app, 7, ihuldefColonyShip, static_cast<uint8_t *>(&vrgMacAip[macRecipeOrbitalConstructionColonizer]));
    }
    if (FLookupPartX(app, &part, hstEngine, iengineGalaxyScoop) == mdPartAvailAvailable && !app.state.rgshdef[1].fFree && app.state.rgshdef[1].cExist == 0 &&
        app.state.rgshdef[1].hul.rghs[0].iItem != iengineGalaxyScoop) {
        shdef = app.state.rgshdef[1];
        shdef.fFree = 1;
        FChangeAiShdef(app, &shdef, 1);
        FCreateAiShdef(app, 1, ihuldefColonyShip, static_cast<uint8_t *>(&vrgMacAip[macRecipeOrbitalConstructionColonizer]));
    }
    for (ish = 5; ish <= 7; ish++) {
        if (app.state.rgshdef[ish].fFree && (ish == 5 || (!app.state.rgshdef[ish - 1].fFree && static_cast<uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 20))) {
            shBase = ish == 5 ? imacRecipeBattleshipMissileBeam : imacRecipeBattleshipBeamA;
            if (ish == 7) {
                shBase = RandomIO().pfnRandom(app, 2) == 0 ? imacRecipeBattleshipBeamA : imacRecipeBattleshipMissileBeam;
            }
            if (RandomIO().pfnRandom(app, 3) == 0 || !FCreateAiShdef(app, ish, ihuldefNubian, static_cast<uint8_t *>(&vrgMacAip[macRecipeNubianMixedBeamsMcm]))) {
                for (i = 0; i < 5 && FCreateAiShdef(app, ish, ihuldefBattleship, static_cast<uint8_t *>(&vrgMacAip[vrgMacIshAip[RandomIO().pfnRandom(app, 4) + shBase]])) == 0; i++) {
                }
            }
        }
    }
    for (ish = 8; ish <= 9; ish++) {
        if (app.state.rgshdef[ish].fFree && app.state.rgplr[app.state.idPlayer].rgTech[1] >= 14 &&
            ((ish == 8 || (!app.state.rgshdef[ish - 1].fFree && static_cast<uint16_t>((app.state.game.turn - app.state.rgshdef[ish - 1].turn)) > 15)) &&
             !FCreateAiShdef(app, ish, ihuldefBattleship, static_cast<uint8_t *>(&vrgMacAip[macRecipeBattleshipMcm])))) {
            FCreateAiShdef(app, ish, ihuldefB52Bomber, static_cast<uint8_t *>(&vrgMacAip[ish == 8 ? macRecipeB52SmartThenNormal : macRecipeB52RetroThenSmart]));
        }
    }
    if (app.state.rgshdef[0].hul.ihuldef != ihuldefFrigate && app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && app.state.rgshdef[0].cExist == 0 && app.state.rgplr[app.state.idPlayer].rgTech[5] >= 4 &&
        app.state.rgplr[app.state.idPlayer].rgTech[4] >= 5 && app.state.rgplr[app.state.idPlayer].rgTech[3] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[2] >= 6 && app.state.rgplr[app.state.idPlayer].rgTech[0] >= 6) {
        shdef = app.state.rgshdef[0];
        shdef.fFree = 1;
        FChangeAiShdef(app, &shdef, 0);
        FCreateAiShdef(app, 0, ihuldefFrigate, static_cast<uint8_t *>(&vrgMacAip[macRecipeFrigateScoutMineLayer]));
    }
    return;
}

int16_t FRetargetMiner(APPSESSION &app, FLEET *lpfl) {
    int16_t cConc;
    ORDER   ord;
    int16_t cConcBest;
    PLANET *lppl;
    int16_t cConcCur;
    int16_t ipl;
    PLANET *lpplBest;

    lpplBest = nullptr;
    cConcCur = 0;
    cConcBest = 0;
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (LDistance2(lpfl->pt, app.state.rgptPlan[lppl->id]) < 5184) {
            cConc = lppl->rgMinConc[0] * 8 + lppl->rgMinConc[1] * 10 + lppl->rgMinConc[2] * 7;
            if (lppl->id == lpfl->idPlanet) {
                cConcCur = cConc;
            }
            if (cConc > cConcBest) {
                lpplBest = lppl;
                cConcBest = cConc;
            }
        }
    }
    if (!lpplBest || static_cast<int16_t>((6 * cConcCur)) / 5 >= cConcBest) {
        return 0;
    }
    ord.id = lpplBest->id;
    ord.grobj = grobjPlanet;
    ord.pt = app.state.rgptPlan[lpplBest->id];
    ord.grTask = grTaskNone;
    ord.fValidTask = 1;
    ord.iWarp = 6;
    return FMoveAiFleet(app, lpfl, &ord, 0);
}

int16_t IdTargetMacFreighter(APPSESSION &app, FLEET *lpfl) {
    int32_t cMax;
    int32_t cColLeft;
    int16_t cResGainMost;
    int32_t cColHaul;
    int16_t cResGain;
    ORDER   ord;
    PLANET *lpplHere;
    int16_t pctCapMost;
    int16_t pctCapHere;
    int16_t cResLost;
    PLANET *lppl;
    int16_t pctKilled;
    int16_t i;
    int32_t lDist;
    int16_t ipl;
    PLANET *lpplBest;
    int16_t iM;

    if (lpfl->idPlanet == idPlanetDeepSpace) {
        lpplHere = nullptr;
        goto LFindPickup;
    }
    lpplHere = LpplFromId(app, lpfl->idPlanet);
    if (!lpplHere || lpplHere->iPlayer != app.state.idPlayer)
        goto LFindPickup;
    pctCapHere = CapacityIO().pfnPctPlanetCapacity(app, lpplHere);
    if (pctCapHere <= 25 || lpplHere->rgwtMin[3] < 1000)
        goto LFindPickup;
    cColHaul = ProductionIO().pfnFleetStat(app, lpfl, 2);
    for (i = 0; i <= 2; i++) {
        cColHaul -= lpfl->rgwtMin[i];
    }
    cMax = static_cast<int32_t>((lpplHere->rgwtMin[3] / 20));
    if (cColHaul > cMax) {
        cColHaul = cMax;
    }
    if (cColHaul <= 0)
        goto LFindPickup;
    cResLost = ResearchIO().pfnResources(app, lpplHere, app.state.idPlayer);
    lpplHere->rgwtMin[3] -= cColHaul;
    cResLost -= ResearchIO().pfnResources(app, lpplHere, app.state.idPlayer);
    lpplHere->rgwtMin[3] += cColHaul;
    lpplBest = nullptr;
    cResGainMost = 0;
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (lppl != lpplHere && !(app.state.vlpbAiPlanet[lppl->id * 16 + 0xe] & 1)) {
            lDist = LDistance2(lpfl->pt, app.state.rgptPlan[lppl->id]);
            if (lDist <= 40000) {
                if (lDist > 22500) {
                    pctKilled = 12;
                } else if (lDist > 10000) {
                    pctKilled = 9;
                } else if (lDist > 2500) {
                    pctKilled = 6;
                } else {
                    pctKilled = 3;
                }
                cColLeft = cColHaul - static_cast<int32_t>((cColHaul * pctKilled)) / 100;
                lppl->rgwtMin[3] += cColLeft;
                cResGain = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                lppl->rgwtMin[3] -= cColLeft;
                cResGain -= ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
                if (cResGain > cResGainMost) {
                    cResGainMost = cResGain;
                    lpplBest = lppl;
                }
            }
        }
    }
    if (!lpplBest || cResGainMost < cResLost + 5 || (cResGainMost < cResLost + 10 && app.state.game.turn > 80) || (cResGainMost < cResLost + 15 && app.state.game.turn > 160))
        goto LFindPickup;
    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, LOWORD(cColHaul));
    FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
    app.state.vlpbAiPlanet[lpplBest->id * 16 + 14] = app.state.vlpbAiPlanet[lpplBest->id * 16 + 0xe] | 1;
LMoveToLpplBest:
    memset(&ord, 0, sizeof(ORDER));
    ord.grobj = grobjPlanet;
    ord.pt = app.state.rgptPlan[lpplBest->id];
    ord.id = lpplBest->id;
    ord.grTask = grTaskXfer;
    ord.fValidTask = 1;
    ord.iWarp = 4;
    for (i = 0; i <= 3; i++) {
        ord.txp.rgia[i].SetIAction(iActionUnloadAll);
    }
    if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
        return -1;
    }
    return lpplBest->id;
LFindPickup:
    if (lpplHere) {
        for (iM = 0; iM <= 2 && lpplHere->rgwtMin[iM] < 2500; iM++) {
        }
        if (iM <= 2) {
            lpplBest = nullptr;
            cMax = 1000;
            for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
                lppl = app.state.vrglpplAi[ipl];
                if (!app.state.vrglpplAi[ipl])
                    break;
                if (lppl != lpplHere) {
                    lDist = LDistance2(lpfl->pt, app.state.rgptPlan[lppl->id]);
                    if (lDist <= 40000 && lppl->rgwtMin[iM] < cMax) {
                        cMax = lppl->rgwtMin[iM];
                        lpplBest = lppl;
                    }
                }
            }
            if (lpplBest && cMax < 200) {
                cColHaul = ProductionIO().pfnFleetStat(app, lpfl, 2);
                for (i = 0; i <= 2; i++) {
                    cColHaul -= lpfl->rgwtMin[i];
                }
                cMax = static_cast<int32_t>((lpplHere->rgwtMin[iM] / 5));
                if (cColHaul > cMax) {
                    cColHaul = cMax;
                }
                if (cColHaul > 0) {
                    ChangeMainObjSel(app, grobjFleet, lpfl->id);
                }
                XferAiSupply(app, grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, static_cast<MineralType>(iM), LOWORD(cColHaul));
                FleetQueryIO().pfnLookup(app, lpfl->id, &app.state.sel.fl);
                goto LMoveToLpplBest;
            }
        }
    }
    lpplBest = nullptr;
    pctCapMost = 0;
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (lppl != lpplHere) {
            lDist = LDistance2(lpfl->pt, app.state.rgptPlan[lppl->id]);
            if (lDist <= 40000) {
                pctCapHere = CapacityIO().pfnPctPlanetCapacity(app, lppl);
                if (lDist > 22500) {
                    pctCapHere -= 6;
                } else if (lDist > 10000) {
                    pctCapHere -= 4;
                } else if (lDist > 2500) {
                    pctCapHere -= 2;
                }
                if (pctCapMost < pctCapHere) {
                    lpplBest = lppl;
                    pctCapMost = pctCapHere;
                }
            }
        }
    }
    if (lpplBest)
        goto LMoveToLpplBest;
    return -1;
}

void TargetMacArmada(APPSESSION &app, FLEET *lpfl) {
    FLEET  *lpflTarget;
    ORDER   ord;
    int16_t cshBomb;
    PLANET *lppl;
    int16_t cshWar;
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
    FPotentMacWarFleet(app, lpfl, &cshWar);
    cshBomb = lpfl->rgcsh[8] + lpfl->rgcsh[9];
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

int16_t FPotentMacWarFleet(APPSESSION &app, FLEET *lpfl, int16_t *pcEquiv) {
    int16_t ish;
    int16_t cEquiv;

    cEquiv = 0;
    for (ish = 2; ish <= 4; ish++) {
        cEquiv += lpfl->rgcsh[ish];
    }
    for (ish = 5; ish <= 7; ish++) {
        cEquiv += lpfl->rgcsh[ish] * 2;
    }
    if (cEquiv >= app.state.vrgAiArmadaPotency[0])
        goto Success;

    for (ish = 8; ish <= 9; ish++) {
        if (lpfl->rgcsh[ish] != 0 && !app.state.rgshdef[ish].fFree && app.state.rgshdef[ish].hul.ihuldef == ihuldefBattleship) {
            cEquiv += lpfl->rgcsh[ish] * 2;
        }
    }
    if (cEquiv > app.state.vrgAiArmadaPotency[0]) {
    Success:
        if (pcEquiv) {
            *pcEquiv = cEquiv;
        }
        return 1;
    }

    /* The original returned without storing, leaving TargetMacArmada's cshWar uninitialized. */
    if (pcEquiv) {
        *pcEquiv = cEquiv;
    }
    return 0;
}
