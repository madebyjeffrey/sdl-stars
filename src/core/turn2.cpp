#include "common.h"
#include "appstate.h"
#include "colony_io.h"
#include "turn_io.h"
#include "production_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "random_io.h"

void Produce(APPSESSION &app) {
    int32_t    lResCur;
    int16_t    cMax;
    int32_t    rgResAvail[4];
    int16_t    iprodCur;
    mdProdStat mdStatus;
    int16_t    mdStatusRaw;
    int16_t    cBuilt;
    int16_t    fNoResearch;
    PLANET    *lppl;
    int16_t    i;
    MessageId  idm;
    PROD       prodPartial;
    int16_t    fPrevProdIsAlch;
    int16_t    fAutoBuildDone;
    int32_t    lResearchTake;
    PROD      *lpprod;
    PLANET    *lpplMac;
    int16_t    cMax2;

    TurnIO().pfnMine(app);
    for (i = 0; i < app.state.game.cPlayer; i++) {
        app.state.rgplr[i].lResLastYear = 0;
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (!lppl->lpplprod) {
            if (lppl->iPlayer != iplrNone) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmProductionQueueEmpty, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                lResCur = ResearchIO().pfnResources(app, lppl, lppl->iPlayer);
                if (lResCur != 0 && app.state.vrgPlanResExtra[lppl->id] != 0) {
                    lResCur += static_cast<int32_t>((lResCur * static_cast<uint32_t>(app.state.vrgPlanResExtra[lppl->id]))) / static_cast<int32_t>((static_cast<uint32_t>(app.state.vrgPlanResExtra[lppl->id]) + lResCur));
                }
                app.state.rgplr[lppl->iPlayer].lResLastYear = app.state.rgplr[lppl->iPlayer].lResLastYear + lResCur;
            }
        } else if (lppl->iPlayer != iplrNone && lppl->lpplprod->iprodMac != 0) {
            fNoResearch = lppl->fNoResearch;
            for (i = 0; i < 3; i++) {
                rgResAvail[i] = lppl->rgwtMin[i];
            }
            lResCur = ResearchIO().pfnResources(app, lppl, lppl->iPlayer);
            if (lResCur != 0 && app.state.vrgPlanResExtra[lppl->id] != 0) {
                lResCur += static_cast<int32_t>((lResCur * static_cast<uint32_t>(app.state.vrgPlanResExtra[lppl->id]))) / static_cast<int32_t>((static_cast<uint32_t>(app.state.vrgPlanResExtra[lppl->id]) + lResCur));
            }
            rgResAvail[3] = lResCur;
            if (rgResAvail[3] != 0) {
                if (fNoResearch) {
                    lResearchTake = 0;
                } else {
                    lResearchTake = static_cast<int32_t>((rgResAvail[3] * static_cast<int16_t>(app.state.rgplr[lppl->iPlayer].pctResearch))) / 100;
                    rgResAvail[3] -= lResearchTake;
                    app.state.rgplr[lppl->iPlayer].lResLastYear = app.state.rgplr[lppl->iPlayer].lResLastYear + lResearchTake;
                }
                fAutoBuildDone = 1;
            TopOfQueue:
                fPrevProdIsAlch = 0;
                iprodCur = 0;
                while (lppl->lpplprod && iprodCur < lppl->lpplprod->iprodMac) {
                    lpprod = &lppl->lpplprod->rgprod()[iprodCur];
                    if (lpprod->cItem <= 0)
                        goto RemoveFromQueue;
                    if (lpprod->grobj == grobjPlanet) {
                        if ((lpprod->iItem >= iobjPlanetaryScannerFirst && lpprod->iItem <= iobjPlanetaryScannerSnooper620X) ||
                            lpprod->iItem == iobjPlanetaryScanner) {
                            if (lppl->iScanner != 31) {
                                TurnIO().pfnMessage2(app, lppl->iPlayer, idmOrderBuildScannerCanceledAlreadyHaveScanner, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                                goto RemoveFromQueue;
                            }
                        } else if (lpprod->iItem >= iobjPacketIron && lpprod->iItem <= iobjPacketMixed) {
                            if (ColonyIO().pfnWarp(app, lppl, nullptr) == 0 || lppl->idFling == 0) {
                                TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasOrdersBuildMineralPacketEitherDoesnt, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                                goto RemoveFromQueue;
                            }
                        } else {
                            switch (lpprod->iItem) {
                            case mdIdleFactory:
                                cMax = ColonyIO().pfnMaxFactories(app, lppl, lppl->iPlayer);
                                cMax2 = ColonyIO().pfnOperableFactories(app, lppl, lppl->iPlayer, 1);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cFactories;
                            LCantBuildP:
                                idm = idmHasOrdersBuildPlanetaryInstallationsBeyondMaximu;
                            LCantBuildP2:
                                if (cMax < static_cast<int32_t>(lpprod->cItem)) {
                                    TurnIO().pfnMessage2(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                                    if (cMax <= 0)
                                        goto RemoveFromQueue;
                                    lpprod->cItem = cMax;
                                }
                                break;
                            case mdIdleMine:
                                cMax = ColonyIO().pfnMaxMines(app, lppl, lppl->iPlayer);
                                cMax2 = ColonyIO().pfnOperableMines(app, lppl, lppl->iPlayer, 1);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cMines;
                                goto LCantBuildP;
                            case mdIdleDefense:
                                cMax = ColonyIO().pfnMaxDefenses(app, lppl, lppl->iPlayer);
                                cMax2 = ColonyIO().pfnOperableDefenses(app, lppl, lppl->iPlayer, 1);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cDefenses;
                                goto LCantBuildP;
                            case mdIdleTerraform:
                                cMax = ColonyIO().pfnTerraformPercent(app, lppl);
                                idm = idmHasOrdersTerraformBeyondMaximumAllowedOrders;
                                goto LCantBuildP2;
                            }
                        }
                    }
                    if (lpprod->iItem == iobjAlchemy && lpprod->grobj == grobjPlanet && iprodCur < lppl->lpplprod->iprodMac - 1) {
                        fPrevProdIsAlch = 1;
                        iprodCur++;
                        continue;
                    }
                    prodPartial.cItem = 0;
                    cBuilt = ProductionIO().pfnBuild(app, lppl, lpprod, &prodPartial, rgResAvail, fPrevProdIsAlch, &mdStatusRaw, 0);
                    mdStatus = static_cast<mdProdStat>(mdStatusRaw);
                    if (fAutoBuildDone && (mdStatus == mdProdStatSomeAuto || mdStatus == mdProdStatNoneAuto)) {
                        fAutoBuildDone = 0;
                    }
                    if (cBuilt > 0 && !ProductionIO().pfnApply(app, lppl, static_cast<GrobjClass>(lpprod->grobj), lpprod->iItem, cBuilt, rgResAvail) &&
                        (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory)) {
                        lpprod->cItem = 0;
                    }
                    if (lppl->iPlayer == iplrNone && !lppl->lpplprod)
                        goto TopOfQueue;
                    if (mdStatus == mdProdStatComplete) {
                    RemoveFromQueue:
                        if (lppl->lpplprod->iprodMac == fPrevProdIsAlch + 1) {
                            FreePl(app, lppl->lpplprod);
                            lppl->lpplprod = nullptr;
                            break;
                        }
                        if (iprodCur < lppl->lpplprod->iprodMac - 1) {
                            HeapIO().pfnMove(lppl->lpplprod->rgprod() + (iprodCur - fPrevProdIsAlch), lppl->lpplprod->rgprod() + (iprodCur + 1),
                                    (lppl->lpplprod->iprodMac - iprodCur - 1) * 4);
                        }
                        lppl->lpplprod->iprodMac -= fPrevProdIsAlch + 1;
                        iprodCur -= fPrevProdIsAlch + 1;
                    } else if (static_cast<int16_t>(mdStatus) >= mdProdStatSome) {
                        if (prodPartial.cItem > 0) {
                            if (lppl->lpplprod->iprodMac == lppl->lpplprod->iprodMax) {
                                lppl->lpplprod = LpplReAlloc(app, lppl->lpplprod, lppl->lpplprod->iprodMac + 1);
                            }
                            HeapIO().pfnMove(&lppl->lpplprod->rgprod()[1], lppl->lpplprod->rgprod(), lppl->lpplprod->iprodMac * sizeof(PROD));
                            lppl->lpplprod->rgprod()[0] = prodPartial;
                            lppl->lpplprod->iprodMac++;
                        }
                        break;
                    }
                    iprodCur++;
                    fPrevProdIsAlch = 0;
                }
                if (!lppl->lpplprod || (iprodCur >= lppl->lpplprod->iprodMac && fAutoBuildDone)) {
                    TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasCompletedOrdersProductionQueueEmpty, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                }
                for (i = 0; i < 3; i++) {
                    lppl->rgwtMin[i] = rgResAvail[i];
                }
                app.state.rgplr[lppl->iPlayer].lResLastYear = app.state.rgplr[lppl->iPlayer].lResLastYear + rgResAvail[3];
            }
        }
    }
    TurnIO().pfnPopulation(app);
    TurnIO().pfnResearch(app, 1);
    if (!app.state.game.fNoRandom) {
        TurnIO().pfnEvents(app);
    }
    /* The score counts each planet as it produced and grew. The original
       counted it at the end of the turn, so loading colonists at waypoint 1
       hid a player's resources from the published scores. vrgwtPopScore
       and vrgiplrPopScore hold each planet's population and owner. */
    if (app.state.vrgwtPopScore) {
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            app.state.vrgwtPopScore[lppl->id] = lppl->rgwtMin[3];
            app.state.vrgiplrPopScore[lppl->id] = lppl->iPlayer;
        }
    }
    return;
}

int16_t CBuildProdItem(APPSESSION &app, PLANET *lppl, PROD *lpprod, PROD *pprodPartial, int32_t *rgRes, int16_t fAlchemy, int16_t *pmdStatus, int16_t fCalcOnly) {
    int32_t  pctT;
    int16_t  cMax;
    uint32_t iobjOther;
    int32_t  cCanBuild;
    int32_t  lMinNeeded;
    int32_t  lAlchCost;
    PROD     prod;
    int16_t  fAutoBuild;
    int16_t  cBuilt;
    int16_t  cAlchemy;
    int32_t  rgCostPaid[4];
    int16_t  i;
    int16_t  fResourceBlocked;
    int32_t  pctInitial;
    int32_t  pctTooBig;
    int32_t  pct;
    int32_t  rgCost[4];
    int16_t  fMineralBlocked;
    int32_t  AddCost;

    cAlchemy = 0;
    pctInitial = lpprod->pct;
    prod = *lpprod;
    ProductionIO().pfnQuote(app, lppl, lpprod, reinterpret_cast<uint32_t *>(rgCost), lppl->iPlayer, 1);
    cBuilt = 0;
    fAutoBuild = prod.grobj == grobjPlanet && prod.iItem < mdIdleFactory;
    if (fAutoBuild) {
        cMax = 1000;
        switch (prod.iItem) {
        case iobjMine:
            iobjOther = mdIdleMine;
            cMax = ColonyIO().pfnOperableMines(app, lppl, lppl->iPlayer, 1) - lppl->cMines;
            break;
        case iobjFactory:
            iobjOther = mdIdleFactory;
            cMax = ColonyIO().pfnOperableFactories(app, lppl, lppl->iPlayer, 1) - lppl->cFactories;
            break;
        case iobjDefense:
            iobjOther = mdIdleDefense;
            cMax = ColonyIO().pfnOperableDefenses(app, lppl, lppl->iPlayer, 1) - lppl->cDefenses;
            break;
        case iobjAlchemy:
            iobjOther = mdIdleAlchemy;
            break;
        case iobjMinTerraform:
        case iobjMaxTerraform:
            iobjOther = mdIdleTerraform;
            cMax = ColonyIO().pfnTerraformPercent(app, lppl);
            if (cMax <= 0 || prod.iItem != iobjMinTerraform || PlanetQueryIO().pfnGrowth(app, lppl, 0) < 0 || PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer) <= 0)
                break;
            cMax = 0;
            break;
        case iobjPacket:
            iobjOther = iobjPacketMixed;
            if (ColonyIO().pfnWarp(app, lppl, nullptr) == 0 || lppl->idFling == 0) {
                cMax = 0;
            }
        }
        if (cMax < 0) {
            cMax = 0;
        }
        if (static_cast<uint32_t>(prod.cItem) > static_cast<uint32_t>(cMax) || prod.iItem == iobjAlchemy) {
            prod.cItem = cMax;
        }
    }
    for (i = 0; i < 4; i++) {
        rgCostPaid[i] = static_cast<uint32_t>((static_cast<uint32_t>((rgCost[i] * prod.pct)) / 100));
    }
    while (prod.cItem > 0) {
        for (i = 0; i < 4 && rgCost[i] - rgCostPaid[i] <= rgRes[i]; i++) {
        }
        if (i < 4) {
            fMineralBlocked = 0;
            fResourceBlocked = 0;
            pct = 100;
            for (i = 0; i < 4; i++) {
                if (rgCost[i] > 0) {
                    if (rgRes[i] >= rgCost[i]) {
                        pctT = 100;
                    } else {
                        pctT = static_cast<int32_t>((static_cast<int32_t>(((rgRes[i] + rgCostPaid[i]) * 100)) / rgCost[i]));
                        pctTooBig = static_cast<int32_t>((static_cast<int32_t>(((rgRes[i] + rgCostPaid[i] + 1) * 100)) / rgCost[i]));
                        pctT = pctT <= pctTooBig - 1 ? pctTooBig - 1 : pctT;
                    }
                    if (pctT < pct) {
                        lMinNeeded = rgCost[i] - rgCostPaid[i] - rgRes[i];
                        pct = pctT;
                        if (i == 3) {
                            fResourceBlocked = 1;
                        } else {
                            fMineralBlocked = 1;
                        }
                    }
                }
            }
            if (fMineralBlocked && fAutoBuild) {
                if (fAlchemy)
                    goto LAlchemize;
                fAutoBuild = 2;
                break;
            }
            for (i = 0; i < 4; i++) {
                AddCost = static_cast<int32_t>((rgCost[i] * pct)) / 100 - rgCostPaid[i];
                rgRes[i] -= AddCost;
                rgCostPaid[i] += AddCost;
            }
            prod.pct = LOWORD(pct);
            if (!fAlchemy || fResourceBlocked)
                break;
        LAlchemize:
            lAlchCost = static_cast<uint32_t>((RaceQueryIO().pfnGrbit(&app.state.rgplr[lppl->iPlayer], ibitRaceMineralAlchemy) == 0 ? 100 : 25));
            cCanBuild = static_cast<int32_t>((rgRes[3] / lAlchCost));
            if (cCanBuild > lMinNeeded) {
                cCanBuild = lMinNeeded;
            }
            if (cCanBuild > 0) {
                for (i = 0; i < 3; i++) {
                    rgRes[i] += cCanBuild;
                }
                rgRes[i] -= static_cast<uint32_t>((lAlchCost * cCanBuild));
                cAlchemy += LOWORD(cCanBuild);
            }
            if (cCanBuild == lMinNeeded)
                continue;
            if (rgRes[3] > 0 && pprodPartial) {
                memset(pprodPartial, 0, sizeof(PROD));
                pprodPartial->grobj = grobjPlanet;
                pprodPartial->iItem = mdIdleAlchemy;
                pprodPartial->cItem = 1;
                pctT = static_cast<int32_t>((static_cast<int32_t>((rgRes[3] * 100)) / lAlchCost));
                pctTooBig = static_cast<int32_t>((static_cast<int32_t>(((rgRes[3] + 1) * 100)) / lAlchCost));
                pctT = StarsMax(pctT, pctTooBig - 1);
                pprodPartial->pct = LOWORD(pctT);
                rgRes[3] -= static_cast<int32_t>((pctT * lAlchCost)) / 100;
            }
            break;
        } else {
            cBuilt++;
            prod.cItem--;
            prod.pct = 0;
            for (i = 0; i < 4; i++) {
                rgRes[i] -= rgCost[i] - rgCostPaid[i];
                rgCostPaid[i] = 0;
            }
        }
    }
    if (cBuilt > 0 && prod.grobj == grobjPlanet && (prod.iItem == mdIdleAlchemy || prod.iItem == iobjAlchemy)) {
        cAlchemy += cBuilt;
        for (i = 0; i < 3; i++) {
            rgRes[i] += cBuilt;
        }
    }
    if (cAlchemy != 0 && !fCalcOnly && app.state.gd.fGeneratingTurn) {
        TurnIO().pfnMessage2(app, lppl->iPlayer, idmScientistsHaveTransmutedCommonMaterialsKtEach, static_cast<MsgGoto>(lppl->id), lppl->id, cAlchemy);
    }
    if (pmdStatus) {
        if (fAutoBuild == 2) {
            *pmdStatus = cBuilt <= 0 ? mdProdStatNoneAuto : mdProdStatSomeAuto;
        } else if (fAutoBuild && prod.cItem == 0) {
            *pmdStatus = cBuilt <= 0 ? mdProdStatSkippedAuto : mdProdStatCompleteAuto;
        } else if (cBuilt == 0) {
            *pmdStatus = pctInitial == prod.pct ? mdProdStatBlockedSame : mdProdStatBlockedDiff;
        } else if (prod.cItem == 0) {
            *pmdStatus = mdProdStatComplete;
        } else {
            *pmdStatus = mdProdStatSome;
        }
    }
    if (!fCalcOnly && !fAutoBuild) {
        *lpprod = prod;
    }
    if (fAutoBuild && pprodPartial && pprodPartial->cItem == 0 && prod.pct > 0) {
        *pprodPartial = prod;
        pprodPartial->cItem = 1;
        pprodPartial->iItem = LOWORD(iobjOther);
    }
    return cBuilt;
}

int16_t FBuildObject(APPSESSION &app, PLANET *lppl, GrobjClass grobj, int16_t iItem, int16_t cBuilt, int32_t *rgMinerals) {
    int16_t       iWarp;
    int16_t       i;
    FLEET        *lpfl;
    MessageId     idm;
    int16_t       fTwoMAs;
    SHDEF        *lpshdef;
    int16_t       cAllowed;
    int32_t       dpOrig;
    int16_t       cshDamaged;
    int16_t       cshOrig;
    uint16_t      dpShdef;
    THING        *lpthMac;
    PacketDecay   iDecayRate;
    THING        *lpth;
    RaceAttribute raMajor;
    int16_t       iWarpAsked;
    int16_t       cSize;
    int16_t       rgwt[3];
    int32_t       l;
    EnvType       iEnv;
    PART          part;

    if (grobj == grobjFleet) {
        if (iItem >= 16) {
            iItem -= 16;
            lpshdef = app.state.rglpshdefSB[lppl->iPlayer] + iItem;
            if (lpshdef->fFree || !ProductionIO().pfnBuildable(app, lpshdef, lppl->iPlayer)) {
                return 0;
            }
            idm = idmHasBuiltNew;
            if (lpshdef->hul.wtCargoMax != 0) {
                idm = static_cast<MessageId>(idm + 1);
                if (static_cast<uint32_t>(lpshdef->hul.wtCargoMax) == 0xffff) {
                    idm = static_cast<MessageId>(idm + 1);
                }
            }
            TurnIO().pfnMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id), lppl->id, lppl->iPlayer << 5 | (iItem + 0x10), PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.wtCargoMax, 0, 0,
                        0, 0);
            if (lppl->fStarbase && static_cast<int16_t>(app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef) > static_cast<int16_t>(app.state.rglpshdefSB[lppl->iPlayer][iItem].hul.ihuldef)) {
                ProductionIO().pfnKillShips(app, lppl);
            }
            iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMAs);
            if (lppl->fStarbase) {
                app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].cExist = app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].cExist - 1;
            } else {
                lppl->fStarbase = 1;
            }
            lppl->isb = iItem;
            if (iWarp <= 0) {
                iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMAs);
                if (iWarp > 0) {
                    lppl->iWarpFling = iWarp + fTwoMAs - 4;
                } else {
                    lppl->iWarpFling = 0;
                    lppl->idFling = 0;
                    ProductionIO().pfnKillPackets(app, lppl);
                }
            }
            lpshdef->cBuilt++;
            lpshdef->cExist++;
            return 1;
        }
        if (!lppl->fStarbase || iItem >= 16) {
            return 0;
        }
        lpshdef = app.state.rglpshdef[lppl->iPlayer] + iItem;
        if (lpshdef->fFree || !ProductionIO().pfnBuildable(app, lpshdef, lppl->iPlayer)) {
            TurnIO().pfnMessage2(app, lppl->iPlayer, idmStarbaseFailedBuildNewShipTypeBecause, static_cast<MsgGoto>(lppl->id), iItem + 1, 0);
            return 0;
        }
        if (app.state.rgplr[lppl->iPlayer].cFleet == 0x200) {
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i] || lpfl->iPlayer > lppl->iPlayer)
                    break;
                if (lpfl->iPlayer >= lppl->iPlayer && lpfl->lpplord->rgord()[0].pt.x == app.state.rgptPlan[lppl->id].x &&
                    lpfl->lpplord->rgord()[0].pt.y == app.state.rgptPlan[lppl->id].y && 32766 - cBuilt > lpfl->rgcsh[iItem]) {
                    if (lpfl->rgcsh[iItem] != 0 && lpfl->rgdv[iItem].pctDp() != 0) {
                        dpShdef = app.state.rglpshdef[lpfl->iPlayer][iItem].hul.dp;
                        cshOrig = lpfl->rgcsh[iItem];
                        cshDamaged = LOWORD(static_cast<int32_t>((lpfl->rgdv[iItem].pctSh() * cshOrig)) / 100);
                        if (cshDamaged == 0) {
                            cshDamaged = 1;
                        }
                        dpOrig = static_cast<int32_t>((static_cast<int32_t>((static_cast<uint32_t>(dpShdef) * lpfl->rgdv[iItem].pctDp())) / 10 * cshDamaged)) / 50;
                        lpfl->rgdv[iItem].SetPctSh(LOWORD(static_cast<int32_t>((cshDamaged * 100)) / static_cast<int16_t>((cshOrig + cBuilt))));
                        if (lpfl->rgdv[iItem].pctSh() == 0) {
                            lpfl->rgdv[iItem].SetPctSh(1);
                        }
                        cshDamaged = LOWORD(static_cast<int32_t>((lpfl->rgdv[iItem].pctSh() * static_cast<int16_t>((cshOrig + cBuilt)))) / 100);
                        if (cshDamaged == 0) {
                            cshDamaged = 1;
                        }
                        lpfl->rgdv[iItem].SetPctDp(LOWORD(static_cast<int32_t>((static_cast<int32_t>((dpOrig * 5)) / cshDamaged)) * 100 / static_cast<int32_t>(dpShdef)));
                    } else {
                        lpfl->rgdv[iItem].dp = 0;
                    }
                    CreateShip(app, lppl->iPlayer, lpfl, iItem, cBuilt);
                    TurnIO().pfnMessage(app, lppl->iPlayer, idmStarbaseBuiltNewSDueLack27b, static_cast<MsgGoto>(lpfl->id | 0x8000), lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, lpfl->id, 0, 0,
                                0);
                    return 1;
                }
            }
            TurnIO().pfnMessage(app, lppl->iPlayer, idmStarbaseBuiltNewShipSTypeLost, static_cast<MsgGoto>(lppl->id), lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, 0, 0, 0, 0);
            return 0;
        }
        lpfl = ProductionIO().pfnNewFleet(app, lppl->iPlayer, lppl->id);
        CreateShip(app, lppl->iPlayer, lpfl, iItem, cBuilt);
        lpfl->rgwtMin[4] = ProductionIO().pfnFleetStat(app, lpfl, 1);
        if (lppl->idRoute != 0) {
            ProductionIO().pfnRoute(app, lpfl, lppl);
            if (cBuilt == 1) {
                idm = lpfl->lpplord->rgord()[1].iWarp == 0 ? idmStarbaseHasBuiltNewWhichWillRouted : idmStarbaseHasBuiltNewWhichRouted;
                TurnIO().pfnMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lppl->id, lppl->iPlayer << 5 | iItem, lppl->idRoute - 1, 0, 0, 0, 0);
            } else {
                idm = lpfl->lpplord->rgord()[1].iWarp == 0 ? idmStarbaseHasBuiltNewShipsWhichWill : idmStarbaseHasBuiltNewShipsWhichRouted;
                TurnIO().pfnMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, lppl->idRoute - 1, 0, 0, 0);
            }
        } else {
            ProductionIO().pfnDefaultOrder(app, lpfl, lppl);
            if (cBuilt == 1) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmStarbaseHasBuiltNew, static_cast<MsgGoto>(lpfl->id | 0x8000), lppl->id, lppl->iPlayer << 5 | iItem);
            } else {
                TurnIO().pfnMessage(app, lppl->iPlayer, idmStarbaseHasBuiltNewShips, static_cast<MsgGoto>(lpfl->id | 0x8000), lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, 0, 0, 0, 0);
            }
        }
    } else {
        if (grobj != grobjPlanet) {
            return 0;
        }
        if (static_cast<uint16_t>(iItem) > iobjPlanetaryScanner) {
            return 0;
        }
        switch (iItem) {
        case iobjFactory:
        case mdIdleFactory:
            cAllowed = ColonyIO().pfnMaxFactories(app, lppl, lppl->iPlayer) - lppl->cFactories;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt <= 0)
                return 0;
            lppl->cFactories += cBuilt;
            idm = idmHaveBuiltFactory;
        SendMsgFactMine:
            cBuilt += TurnIO().pfnRemoveMessage(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id));
            if (cBuilt > 1) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, static_cast<MessageId>(idm + 1), static_cast<MsgGoto>(lppl->id), cBuilt, lppl->id);
            } else {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idm, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
            }
            break;
        case iobjMine:
        case mdIdleMine:
            cAllowed = ColonyIO().pfnMaxMines(app, lppl, lppl->iPlayer) - lppl->cMines;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt > 0) {
                lppl->cMines += cBuilt;
                idm = idmHaveBuiltMine;
                goto SendMsgFactMine;
            }
            return 0;
        case iobjDefense:
        case mdIdleDefense:
            cAllowed = ColonyIO().pfnMaxDefenses(app, lppl, lppl->iPlayer) - lppl->cDefenses;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt > 0) {
                lppl->cDefenses += cBuilt;
                idm = idmHaveBuiltDefenseOutpost;
                goto SendMsgFactMine;
            }
            return 0;
        case 10:
            break;
        case iobjAlchemy:
        case mdIdleAlchemy:
            break;
        case iobjPacket:
        case iobjPacketIron:
        case iobjPacketBor:
        case iobjPacketGerm:
        case iobjPacketMixed:
            raMajor = static_cast<RaceAttribute>(RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv));
            iWarp = ColonyIO().pfnWarp(app, lppl, &fTwoMAs);
            if (iWarp == 0) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmMineralPacketFormedHasDisintegratedBecausePlanet, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                return 0;
            }
            if (lppl->idFling == 0) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmMineralPacketFormedHasDisintegratedBecauseDidnt, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                return 0;
            }
            if (iItem == iobjPacket) {
                iItem = iobjPacketMixed;
            }
            if (iItem == iobjPacketMixed) {
                cSize = raMajor == raMassAccel ? 25 : 40;
            } else {
                cSize = raMajor == raMassAccel ? 70 : 100;
            }
            for (i = 0; i < 3; i++) {
                if (i == iItem - 14 || iItem == iobjPacketMixed) {
                    l = static_cast<uint32_t>((cSize * cBuilt));
                    if (l > 32760) {
                        l = 32760;
                    }
                    rgwt[i] = LOWORD(l);
                } else {
                    rgwt[i] = 0;
                }
            }
            iWarpAsked = lppl->iWarpFling + 4;
            if (iWarpAsked < 5 || iWarpAsked > iWarp + 3) {
                iWarpAsked = iWarp + fTwoMAs;
            }
            if (iWarpAsked <= iWarp + fTwoMAs) {
                iDecayRate = decayNone;
            } else {
                iDecayRate = static_cast<PacketDecay>(iWarpAsked - iWarp - fTwoMAs);
            }
            if (raMajor == raStargate && static_cast<int16_t>(iDecayRate) < decay50Pct) {
                iDecayRate = static_cast<PacketDecay>(iDecayRate + 1);
            }
            iWarp = iWarpAsked - 4;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac &&
                   (lpth->iplr() != lppl->iPlayer || lpth->ith() != ithMineralPacket || lpth->pt().x() != app.state.rgptPlan[lppl->id].x || lpth->pt().y() != app.state.rgptPlan[lppl->id].y ||
                    lpth->thp().iWarp() != iWarp || lpth->thp().idPlanet() != lppl->idFling - 1 || lpth->thp().iDecayRate() != iDecayRate || lpth->thp().wtMax() >= 1630);
                 lpth++) {
            }
            if (lpth != lpthMac) {
                lpth->thp().SetWtMax(0);
                for (i = 0; i < 3; i++) {
                    lpth->thp().SetRgwtMin(i, lpth->thp().rgwtMin(i) + (rgwt[i]));
                    if (lpth->thp().rgwtMin(i) < 0) {
                        lpth->thp().SetRgwtMin(i, 32760);
                    }
                    { const auto wtMaxForThing = +(static_cast<int16_t>((lpth->thp().rgwtMin(i) + 9)) / 10); auto &thForThing = *(lpth); thForThing.thp().SetWtMax(thForThing.thp().wtMax() + wtMaxForThing); }
                }
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasProducedMineralPacketWhichHasCombined, static_cast<MsgGoto>(lppl->id), lppl->id, lppl->idFling - 1);
                return 1;
            }
            lpth = LpthNew(app, lppl->iPlayer, ithMineralPacket);
            if (!lpth) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasOrdersBuildMineralPacketEitherDoesnt, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                return 1;
            }
            for (i = 0; i < 3; i++) {
                lpth->thp().SetRgwtMin(i, rgwt[i]);
                { const auto wtMaxForThing = +(static_cast<int16_t>((rgwt[i] + 9)) / 10); auto &thForThing = *(lpth); thForThing.thp().SetWtMax(thForThing.thp().wtMax() + wtMaxForThing); }
            }
            lpth->thp().SetIWarp(iWarp);
            lpth->thp().SetIDecayRate(iDecayRate);
            lpth->thp().SetIdPlanet(lppl->idFling - 1);
            lpth->pt().Set(app.state.rgptPlan[lppl->id]);
            TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasProducedMineralPacketWhichHasDestination, static_cast<MsgGoto>(lppl->id), lppl->id, lppl->idFling - 1);
            return 1;
        case iobjGenesis:
            for (i = 0; i < app.state.game.cPlayer; i++) {
                TurnIO().pfnMessage2(app, i, idmStrongFundamentalForcesHaveRebirthed, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
            }
            if (RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                lppl->cFactories = 0;
                lppl->cMines = 0;
                lppl->cDefenses = 0;
                lppl->iScanner = 31;
            }
            for (i = 0; i < 3; i++) {
                lppl->rgwtMin[i] = 0;
                lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i] = RandomIO().pfnRandom(app, 50) + RandomIO().pfnRandom(app, 50) + 1;
                lppl->rgMinConc[i] = RandomIO().pfnRandom(app, 40) + RandomIO().pfnRandom(app, 40) + 25;
            }
            return 1;
        case iobjMinTerraform:
        case iobjMaxTerraform:
        case mdIdleTerraform:
            while (cBuilt-- != 0) {
                i = ColonyIO().pfnBestTerraform(app, lppl, 1);
                if (i != 0) {
                    iEnv = static_cast<EnvType>(abs(i) - 1);
                    cAllowed = lppl->rgEnvVar[iEnv] + (i <= 0 ? -1 : 1);
                    if (1 > (99 >= cAllowed ? cAllowed : 99)) {
                        cAllowed = 1;
                    } else if (99 < cAllowed) {
                        cAllowed = 99;
                    }
                    lppl->rgEnvVar[iEnv] = cAllowed;
                    TurnIO().pfnMessage(app, lppl->iPlayer, idmTerraformingEffortsHave, static_cast<MsgGoto>(lppl->id), lppl->id, i > 0, iEnv, iEnv * 256 + cAllowed, 0, 0, 0);
                }
            }
            return 1;
        case iobjPlanetaryScanner:
            app.state.idPlayer = lppl->iPlayer;
            ProductionIO().pfnScanner(app, &part);
            app.state.idPlayer = iplrNone;
            iItem = part.hs.iItem + 18;
            /* fallthrough */
        case iobjPlanetaryScannerFirst:
        case iobjPlanetaryScannerViewer90:
        case iobjPlanetaryScannerScoper150:
        case iobjPlanetaryScannerScoper220:
        case iobjPlanetaryScannerScoper280:
        case iobjPlanetaryScannerSnooper320X:
        case iobjPlanetaryScannerSnooper400X:
        case iobjPlanetaryScannerSnooper500X:
        case iobjPlanetaryScannerSnooper620X:
            TurnIO().pfnMessage(app, lppl->iPlayer, idmHasBuiltNewPlanetaryScanner, static_cast<MsgGoto>(lppl->id), lppl->id, -32768, iItem - 18, 0, 0, 0, 0);
            lppl->iScanner = iItem - 18;
            break;
        }
    }
    return 1;
}

void CreateShip(APPSESSION &app, int16_t iPlr, FLEET *lpfl, int16_t ishdef, int16_t cShip) {
    lpfl->rgcsh[ishdef] += cShip;
    app.state.rglpshdef[iPlr][ishdef].cExist += cShip;
    app.state.rglpshdef[iPlr][ishdef].cBuilt += cShip;
    return;
}

void RandomEvents(APPSESSION &app) {
    MeteorStrike(app);
    PlanetaryClimateChange(app);
    DiscoverNewMinerals(app);
    MysteryTrader(app);
    return;
}

void TransferToOthers(APPSESSION &app) {
    int32_t   l2;
    int16_t   idDst;
    XFER      rgxf[2];
    int16_t   idSrc;
    int16_t   i;
    MessageId idm;
    XFERFULL *lpxfMax;
    XFERFULL *lpxfCur;
    int32_t   l;

    if (app.state.cXferFull != 0) {
        lpxfCur = app.state.lpxf;
        lpxfMax = app.state.lpxf + app.state.cXferFull;
        for (; lpxfCur < lpxfMax; lpxfCur++) {
            if (!FLookupObject(app, static_cast<GrobjClass>(lpxfCur->grobj2), lpxfCur->id2, rgxf[1].LpvLookup(static_cast<GrobjClass>(lpxfCur->grobj2))))
                goto DoNext;
            if (lpxfCur->grobj1 == 1) {
                rgxf[0].SetIPlayerObj(LpplFromId(app, lpxfCur->id1)->iPlayer);
            } else {
                rgxf[0].SetIPlayerObj(lpxfCur->id1 >> 9 & 0xf);
            }
            for (i = 0; i < 5; i++) {
                l2 = lpxfCur->rgcQuan[i];
                if (l2 != 0) {
                    l = ChgCargo(app, static_cast<GrobjClass>(lpxfCur->grobj2), lpxfCur->id2, static_cast<MineralType>(i), l2, nullptr);
                    if (l != l2) {
                        idm = i == 4 ? idmAttemptedTransferColonistsSuccessfullyReceivedRe : idmAttemptedTransferSuccessfullyReceived;
                        idSrc = lpxfCur->id1 | (lpxfCur->grobj1 == 2 ? 0x8000 : 0);
                        idDst = lpxfCur->id2 | (lpxfCur->grobj2 == 2 ? 0x8000 : 0);
                        if (l == 0) {
                            idm = static_cast<MessageId>(idm + 4);
                        }
                        TurnIO().pfnMessage(app, rgxf[0].iPlayerObj(), idm, static_cast<MsgGoto>(idSrc), idSrc, LOWORD(l2), HIWORD(l2), i, idDst, LOWORD(l), HIWORD(l));
                        idm = i == 4 ? idmReceivedHoweverColonistsSentRemainsOtherColonist : idmReceivedHoweverSentRemainderLostSpace;
                        if (l == 0) {
                            TurnIO().pfnMessage(app, rgxf[1].iPlayerObj(), static_cast<MessageId>(idm + 4), static_cast<MsgGoto>(idDst), idDst, LOWORD(l2), HIWORD(l2), i, idSrc, 0, 0);
                        } else {
                            TurnIO().pfnMessage(app, rgxf[1].iPlayerObj(), idm, static_cast<MsgGoto>(idDst), idDst, LOWORD(l), HIWORD(l), i, idSrc, LOWORD(l2), HIWORD(l2));
                        }
                    } else {
                        idm = i == 4 ? idmSuccessfullyTransferred2 : idmSuccessfullyTransferred;
                        idSrc = lpxfCur->id1 | (lpxfCur->grobj1 == 2 ? 0x8000 : 0);
                        idDst = lpxfCur->id2 | (lpxfCur->grobj2 == 2 ? 0x8000 : 0);
                        TurnIO().pfnMessage(app, rgxf[0].iPlayerObj(), idm, static_cast<MsgGoto>(idSrc), idSrc, LOWORD(l), HIWORD(l), i, idDst, 0, 0);
                        idm = i == 4 ? idmSuccessfullyReceived2 : idmSuccessfullyReceived;
                        TurnIO().pfnMessage(app, rgxf[1].iPlayerObj(), idm, static_cast<MsgGoto>(idDst), idDst, LOWORD(l), HIWORD(l), i, idSrc, 0, 0);
                    }
                }
            }
        DoNext:;
        }
    }
    return;
}

void DropColonists(APPSESSION &app) {
    COLDROP *lpcdLook;
    int16_t  fTie;
    int32_t  cMax;
    PLANET   pl;
    int32_t  lDefensePower;
    int32_t  cPowerTot;
    COLDROP *lpcdCur;
    int16_t  iMax;
    int16_t  idPlanet;
    int16_t  iplrOldOwner;
    int32_t  cColTot;
    int32_t  lOldPop;
    int32_t  c2nd;
    int16_t  i;
    int32_t  rgcPower[16];
    int16_t  cSides;
    float    pctSurvive;
    int32_t  rgcCol[16];
    int32_t  lPower;
    COLDROP *lpcdMax;
    int16_t  cpq;
    int16_t  iDst;
    PROD     prod;
    int16_t  ipq;
    int16_t  iTech;
    int16_t  iBonus;

    if (app.state.cColDrop != 0) {
        lpcdCur = app.state.lpcd;
        lpcdMax = app.state.lpcd + app.state.cColDrop;
        for (; lpcdCur < lpcdMax; lpcdCur++) {
            if (lpcdCur->idPlanetDst != idplNone && lpcdCur->cColonist != 0) {
                memset(rgcCol, 0, 64);
                memset(rgcPower, 0, 64);
                cPowerTot = 0;
                cColTot = 0;
                idPlanet = lpcdCur->idPlanetDst;
                PlanetQueryIO().pfnLookup(app, idPlanet, &pl);
                iplrOldOwner = pl.iPlayer;
                CalcPctSurvive(app, &pl, &pctSurvive, nullptr);
                pctSurvive = static_cast<float>((pctSurvive + (static_cast<double>(1.0) - pctSurvive) / 4.0));
                for (lpcdLook = lpcdCur; lpcdLook < lpcdMax; lpcdLook++) {
                    if (idPlanet == lpcdLook->idPlanetDst) {
                        if (RaceQueryIO().pfnStat(&app.state.rgplr[lpcdLook->idPlr], rsMajorAdv) == raMacintosh && (!lpcdLook->fCanColonize || pl.iPlayer != iplrNone)) {
                            TurnIO().pfnMessage2(app, lpcdLook->idPlr, idmColonistsAttemptingSetShopReducedProtoplasmicBlo, static_cast<MsgGoto>(pl.id), pl.id, 0);
                        } else if (pl.iPlayer == iplrNone && !lpcdLook->fCanColonize) {
                            TurnIO().pfnMessage(app, lpcdLook->idPlr, idmColonistsForcedTransportDiedBecauseDidColonize, static_cast<MsgGoto>(pl.id), LOWORD(lpcdLook->cColonist),
                                        HIWORD(lpcdLook->cColonist), pl.id, 0, 0, 0, 0);
                        } else if (pl.fStarbase && pl.iPlayer != iplrNone) {
                            TurnIO().pfnMessage2(app, lpcdLook->idPlr, idmColonistsAssaultingHaveKilledForcesOrbitingStarb, static_cast<MsgGoto>(pl.id), pl.id, 0);
                        } else {
                            rgcCol[lpcdLook->idPlr] = rgcCol[lpcdLook->idPlr] + lpcdLook->cColonist;
                            cColTot += lpcdLook->cColonist;
                            if (RaceQueryIO().pfnStat(&app.state.rgplr[lpcdLook->idPlr], rsMajorAdv) == raAttack) {
                                lPower = 165;
                            } else if (RaceQueryIO().pfnStat(&app.state.rgplr[lpcdLook->idPlr], rsMajorAdv) == raMacintosh) {
                                lPower = 0;
                            } else {
                                lPower = 110;
                            }
                            lPower = static_cast<int32_t>((static_cast<double>((static_cast<int32_t>((lpcdLook->cColonist * lPower)) / 100)) * pctSurvive));
                            cPowerTot += lPower;
                            rgcPower[lpcdLook->idPlr] = rgcPower[lpcdLook->idPlr] + lPower;
                        }
                        lpcdLook->idPlanetDst = idplNone;
                    }
                }
                if (pl.iPlayer != iplrNone) {
                    if (RaceQueryIO().pfnStat(&app.state.rgplr[pl.iPlayer], rsMajorAdv) == raDefend) {
                        lPower = 200;
                    } else {
                        lPower = 100;
                    }
                    lDefensePower = static_cast<int32_t>((pl.rgwtMin[3] * lPower)) / 100;
                    if (lDefensePower > cPowerTot) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (static_cast<double>(pctSurvive) == static_cast<double>(1.0)) {
                                    TurnIO().pfnMessage(app, i, idmColonistsDroppedMassacredGroundTroops, static_cast<MsgGoto>(pl.id), LOWORD(rgcCol[i]), HIWORD(rgcCol[i]), pl.id,
                                                pl.iPlayer | 0x30, 0, 0, 0);
                                    TurnIO().pfnMessage(app, pl.iPlayer, idmGroundTroopsValiantlyDestroyedAttackingBarbarian, static_cast<MsgGoto>(pl.id), pl.id, LOWORD(rgcCol[i]),
                                                HIWORD(rgcCol[i]), i | 0x30, 0, 0, 0);
                                } else {
                                    TurnIO().pfnMessage(app, i, idmColonistsDroppedDestroyedPlanetaryDefensesRestMa, static_cast<MsgGoto>(pl.id), LOWORD(rgcCol[i]), HIWORD(rgcCol[i]), pl.id,
                                                static_cast<int32_t>(((static_cast<double>(1.0) - pctSurvive) * 10000)), pl.iPlayer | 0x30, 0, 0);
                                    TurnIO().pfnMessage(app, pl.iPlayer, idmPlanetaryDefensesGroundTroopsDestroyedInvadingTr, static_cast<MsgGoto>(pl.id), pl.id, LOWORD(rgcCol[i]),
                                                HIWORD(rgcCol[i]), i | 0x30, 0, 0, 0);
                                }
                            }
                        }
                        pl.rgwtMin[3] -= static_cast<int32_t>((static_cast<int32_t>((pl.rgwtMin[3] * cPowerTot)) / lDefensePower));
                        goto WritePlanet;
                    }
                    lOldPop = pl.rgwtMin[3];
                    UninhabitPlanet(app, &pl);
                } else {
                    lDefensePower = 0;
                    lOldPop = 0;
                }
                cMax = -1;
                c2nd = 0;
                cSides = 0;
                fTie = 0;
                iMax = 0;
                for (i = 0; i < app.state.game.cPlayer; i++) {
                    if (rgcCol[i] != 0) {
                        cSides++;
                        if (rgcPower[i] >= cMax) {
                            if (rgcPower[i] == cMax) {
                                fTie = 1;
                            } else {
                                fTie = 0;
                                c2nd = cMax;
                                cMax = rgcPower[i];
                                iMax = i;
                            }
                        }
                    }
                }
                if (cMax < 0)
                    goto IncCur;
                if (fTie) {
                    for (i = 0; i < app.state.game.cPlayer; i++) {
                        if (rgcCol[i] != 0) {
                            TurnIO().pfnMessage2(app, i, idmInvolvedWayAssaultNobodysTroopsSurvivedBrutal, static_cast<MsgGoto>(pl.id), cSides, pl.id);
                        }
                    }
                    if (iplrOldOwner != iplrNone) {
                        TurnIO().pfnMessage2(app, iplrOldOwner, idmMultitudeEnemiesHaveMountedProngAttackResulting, static_cast<MsgGoto>(pl.id), cSides, pl.id);
                        pl.iPlayer = iplrNone;
                    }
                } else {
                    if (iplrOldOwner != iplrNone) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (i == iMax) {
                                    TurnIO().pfnMessage2(app, i, idmTroopsCrushSColonistsControlPlanet, static_cast<MsgGoto>(pl.id), iplrOldOwner | 0x20, pl.id);
                                } else {
                                    TurnIO().pfnMessage2(app, i, idmColonistsDroppedDestroyedSpiritedFighting, static_cast<MsgGoto>(pl.id), pl.id, 0);
                                }
                            }
                        }
                        TurnIO().pfnMessage(app, iplrOldOwner, idmHaveAttackedFirstRateStormTroopersThough, static_cast<MsgGoto>(pl.id), iMax | 0x30, pl.id, LOWORD(rgcCol[iMax]),
                                    HIWORD(rgcCol[iMax]), 0, 0, 0);
                        memset(app.state.rgTechBattle, 0, 6);
                        memset(app.state.rgTechTrader, 0, 13);
                        for (i = 0; i < 6; i++) {
                            app.state.rgTechBattle[i] = app.state.rgplr[iplrOldOwner].rgTech[i];
                        }
                        i = ITechLearnATech(app, iMax, -1, pl.id, idmWreckageDiscoveredBattleHasBoostedResearchResour, nullptr);
                        pl.iPlayer = iplrNone;
                    } else if (cSides > 1) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (i == iMax) {
                                    TurnIO().pfnMessage2(app, i, idmInvolvedWayRaceUninhabitedPlanetForcesCrush, static_cast<MsgGoto>(pl.id), cSides, pl.id);
                                } else {
                                    TurnIO().pfnMessage(app, i, idmColonistsDestroyedWayRaceUninhabitedPlanetContro, static_cast<MsgGoto>(pl.id), cSides, pl.id, iMax | 0xb0, 0, 0, 0, 0);
                                }
                            }
                        }
                    } else {
                        TurnIO().pfnMessage2(app, iMax, static_cast<MessageId>(idmColonistsControl + (RaceQueryIO().pfnStat(&app.state.rgplr[iMax], rsMajorAdv) == raMacintosh)), static_cast<MsgGoto>(pl.id), pl.id, 0);
                    }
                    if (iMax != -1) {
                        cpq = app.state.rgplr[iMax].zpq1.cpq;
                        pl.fNoResearch = app.state.rgplr[iMax].zpq1.fNoResearch;
                        if (cpq > 0) {
                            pl.lpplprod = LpplAlloc<PLPROD>(app, 4, app.state.rgplr[iMax].zpq1.cpq, htOrd);
                            memset(&prod, 0, sizeof(PROD));
                            prod.grobj = grobjPlanet;
                            iDst = 0;
                            for (ipq = 0; ipq < cpq; ipq++) {
                                if ((RaceQueryIO().pfnStat(&app.state.rgplr[iMax], rsMajorAdv) != raMacintosh || app.state.rgplr[iMax].zpq1.rgpq[ipq].mdIdle() > iobjDefense) &&
                                    (RaceQueryIO().pfnStat(&app.state.rgplr[iMax], rsMajorAdv) != raTerra ||
                                     (app.state.rgplr[iMax].zpq1.rgpq[ipq].mdIdle() != iobjMinTerraform && app.state.rgplr[iMax].zpq1.rgpq[ipq].mdIdle() != iobjMaxTerraform))) {
                                    prod.iItem = app.state.rgplr[iMax].zpq1.rgpq[ipq].mdIdle();
                                    prod.cItem = app.state.rgplr[iMax].zpq1.rgpq[ipq].cQuan();
                                    pl.lpplprod->rgprod()[iDst] = prod;
                                    iDst++;
                                }
                            }
                            if (iDst > 0) {
                                pl.lpplprod->iprodMac = iDst;
                                LpplFromId(app, pl.id)->lpplprod = pl.lpplprod;
                            } else {
                                FreePl(app, pl.lpplprod);
                                pl.lpplprod = nullptr;
                            }
                        }
                    }
                    pl.iPlayer = iMax;
                    if (RaceQueryIO().pfnStat(&app.state.rgplr[iMax], rsMajorAdv) == raMacintosh) {
                        pl.fStarbase = 1;
                        pl.isb = 0;
                        app.state.rglpshdefSB[iMax]->cExist++;
                        app.state.rglpshdefSB[iMax]->cBuilt++;
                    }
                    if (cPowerTot == 0 || cMax == 0) {
                        pl.rgwtMin[3] = rgcCol[iMax];
                    } else {
                        lPower = static_cast<int32_t>((static_cast<int32_t>((cMax * (cPowerTot - lDefensePower))) / cPowerTot));
                        pl.rgwtMin[3] = static_cast<int32_t>((static_cast<int32_t>((rgcCol[iMax] * lPower)) / cMax));
                    }
                    if (c2nd > 0) {
                        pl.rgwtMin[3] = static_cast<int32_t>((static_cast<int32_t>((pl.rgwtMin[3] * (cMax - c2nd))) / cMax));
                    }
                    if (pl.rgwtMin[3] < 1) {
                        pl.rgwtMin[3] = 1;
                    }
                }
            WritePlanet:
                if (pl.iPlayer != iplrNone && pl.fArtifact) {
                    pl.fArtifact = 0;
                    if (!app.state.game.fNoRandom) {
                        iTech = RandomIO().pfnRandom(app, 6);
                        iBonus = RandomIO().pfnRandom(app, 301) + 100;
                        if (pl.rgwtMin[3] < 10) {
                            iBonus = static_cast<int16_t>((LOWORD(pl.rgwtMin[3]) * iBonus)) / 10;
                        }
                        TurnIO().pfnMessage(app, pl.iPlayer, idmColonistsSettlingHaveFoundStrangeArtifactBoostin, gotoResearch, pl.id, iTech, iBonus, 0, 0, 0, 0);
                        app.state.rgplr[pl.iPlayer].rgResSpent[iTech] = app.state.rgplr[pl.iPlayer].rgResSpent[iTech] + iBonus;
                        if (app.state.game.fSlowTech) {
                            iBonus >>= 1;
                        }
                    }
                }
                PlanetQueryIO().pfnLookup(app, idWriteBack, &pl);
            }
        IncCur:;
        }
        app.state.cColDrop = 0;
    }
    return;
}

void HealShips(APPSESSION &app) {
    int16_t pctShipHeal;
    int16_t dpHeal;
    PLANET *lppl;
    int16_t i;
    FLEET  *lpfl;
    SHDEF  *lpshdef;
    int16_t pct;
    int16_t ishdef;
    PLANET *lpplMac;

    pctShipHeal = 0;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (!lpfl->fDead && !lpfl->fNoHeal) {
            dpHeal = 0;
            pctShipHeal = 0;
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (lpfl->rgdv[ishdef].dp != 0) {
                    dpHeal = 1;
                }
                if (lpfl->rgcsh[ishdef] != 0) {
                    if (app.state.rglpshdef[lpfl->iPlayer][ishdef].hul.ihuldef == ihuldefSuperFuelXport) {
                        pctShipHeal = 50;
                    } else if (pctShipHeal < 5 && app.state.rglpshdef[lpfl->iPlayer][ishdef].hul.ihuldef == ihuldefFuelTransport) {
                        pctShipHeal = 25;
                    }
                }
            }
            if (dpHeal != 0) {
                if (!lpfl->fHereAllTurn) {
                    pct = 5;
                } else if (lpfl->idPlanet == idPlanetDeepSpace) {
                    pct = 10;
                } else {
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (lppl->iPlayer != lpfl->iPlayer) {
                        if (lppl->iPlayer == iplrNone) {
                            pct = 15;
                        } else {
                            pct = 15;
                        }
                    } else if (lppl->fStarbase && !lppl->fNoHeal) {
                        lpshdef = app.state.rglpshdefSB[lppl->iPlayer] + lppl->isb;
                        if (PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.wtCargoMax != 0) {
                            pct = 100;
                        } else {
                            pct = 40;
                        }
                    } else {
                        pct = 25;
                    }
                }
                if (RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raDefend) {
                    pct *= 2;
                }
                pct += pctShipHeal;
                for (ishdef = 0; ishdef < 16; ishdef++) {
                    if (lpfl->rgdv[ishdef].dp != 0) {
                        if (lpfl->rgdv[ishdef].pctDp() > static_cast<uint16_t>(pct)) {
                            lpfl->rgdv[ishdef].SetPctDp(lpfl->rgdv[ishdef].pctDp() - (pct));
                        } else {
                            lpfl->rgdv[ishdef].dp = 0;
                        }
                    }
                }
            }
        }
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->fStarbase && !lppl->fNoHeal) {
            if (RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raDefend) {
                pct = 75;
            } else {
                pct = 50;
            }
            if (lppl->pctDp != 0) {
                lpshdef = app.state.rglpshdefSB[lppl->iPlayer] + lppl->isb;
                if (static_cast<uint16_t>(pct) > lppl->pctDp) {
                    lppl->pctDp = 0;
                } else {
                    lppl->pctDp -= pct;
                }
            }
        }
    }
    return;
}

void AutoTerraform(APPSESSION &app) {
    int16_t rgMax[3];
    int16_t rgp[16];
    PLANET *lppl;
    int16_t i;
    int16_t rgMin[3];
    int16_t rgCost[3];
    int16_t fTerra;
    PLANET *lpplMac;

    fTerra = 0;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        rgp[i] = RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raTerra;
        if (rgp[i] != 0) {
            fTerra = 1;
        }
    }
    if (fTerra) {
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer != iplrNone && rgp[lppl->iPlayer] != 0) {
                if (lppl->fStarbase && lppl->iPlayer == iplrNone) {
                    lppl->fStarbase = 0;
                }
                i = RandomIO().pfnRandom(app, 3);
                if (app.state.rgplr[lppl->iPlayer].rgEnvVar[i] != envImmune && app.state.rgplr[lppl->iPlayer].rgEnvVar[i] != lppl->rgEnvVarOrig[i] && RandomIO().pfnRandom(app, 10) == 0) {
                    if (lppl->rgwtMin[3] >= 1000 || RandomIO().pfnRandom(app, 1000) < static_cast<int16_t>(LOWORD(lppl->rgwtMin[3]))) {
                        if (app.state.rgplr[lppl->iPlayer].rgEnvVar[i] < lppl->rgEnvVarOrig[i]) {
                            lppl->rgEnvVarOrig[i]--;
                        } else {
                            lppl->rgEnvVarOrig[i]++;
                        }
                        TurnIO().pfnMessage2(app, lppl->iPlayer, idmEngineersHaveManagedImproveUnderlying1, static_cast<MsgGoto>(lppl->id), lppl->id, i);
                    }
                }
                if (ColonyIO().pfnCanTerraform(app, lppl, rgMin, rgMax, rgCost, 1)) {
                    for (i = 0; i < 3; i++) {
                        if (rgMin[i] != envNone) {
                            lppl->rgEnvVar[i] = rgMin[i];
                        } else if (rgMax[i] != envNone) {
                            lppl->rgEnvVar[i] = rgMax[i];
                        }
                    }
                    i = PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer);
                    TurnIO().pfnMessage2(app, lppl->iPlayer, idmHasAutoTerraformedValue, static_cast<MsgGoto>(lppl->id), lppl->id, i);
                }
            }
        }
    }
    return;
}

void RemoteTerraforming(APPSESSION &app) {
    int16_t fHelp;
    int16_t iBest;
    int16_t pctCur;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t cDone;
    EnvType iEnv;
    int16_t cAllowed;
    int32_t ipct;
    int16_t pctNew;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (!lpfl->fDead && lpfl->idPlanet != idPlanetDeepSpace && app.state.lpPlanets[lpfl->idPlanet].iPlayer != iplrNone) {
            ipct = PctTerraFromLpfl(app, lpfl);
            if (ipct > 0) {
                lppl = app.state.lpPlanets + lpfl->idPlanet;
                fHelp = lpfl->iPlayer == lppl->iPlayer || app.state.rgplr[lpfl->iPlayer].rgmdRelation[lppl->iPlayer] == 1 || lpfl->iPlayer == lppl->iPlayer;
                if (fHelp || !lppl->fStarbase) {
                    pctCur = PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer);
                    cDone = 0;
                    while (ipct-- > 0) {
                        iBest = IBestRemoteTerra(app, lppl, lpfl->iPlayer, fHelp);
                        if (iBest == 0)
                            break;
                        iEnv = static_cast<EnvType>(abs(iBest) - 1);
                        cAllowed = lppl->rgEnvVar[iEnv] + (iBest <= 0 ? -1 : 1);
                        if (1 > (99 >= cAllowed ? cAllowed : 99)) {
                            cAllowed = 1;
                        } else if (99 < cAllowed) {
                            cAllowed = 99;
                        }
                        lppl->rgEnvVar[iEnv] = cAllowed;
                        cDone++;
                    }
                    pctNew = PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer);
                    TurnIO().pfnMessage(app, lpfl->iPlayer, static_cast<MessageId>((!fHelp ? idmHasDegradedValue : idmHasImprovedValue) + (pctCur == pctNew)), static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id,
                                pctCur, pctNew, 0, 0, 0);
                    if (lpfl->iPlayer != lppl->iPlayer && pctNew != pctCur) {
                        TurnIO().pfnMessage(app, lppl->iPlayer, !fHelp ? idmHasDegradedValue : idmHasImprovedValue, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, pctCur, pctNew, 0, 0, 0);
                    }
                }
            }
        }
    }
    return;
}

int16_t FQueueColonistDrop(APPSESSION &app, FLEET *lpfl, PLANET *lppl, int32_t cColonists) {
    int16_t  iColDrop;
    COLDROP *lpcdT;

    if (cColonists <= 0) {
        return 1;
    }
    iColDrop = 0;
    lpcdT = app.state.lpcd;
    for (; iColDrop < app.state.cColDrop && (lpcdT->idFleetSrc != lpfl->id || lpcdT->idPlanetDst != lppl->id); iColDrop++) {
        lpcdT++;
    }
    if (iColDrop == app.state.cColDrop) {
        if (app.state.cColDrop >= 1000) {
            return 0;
        }
        lpcdT->idFleetSrc = lpfl->id;
        lpcdT->idPlr = lpfl->iPlayer;
        lpcdT->idPlanetDst = lppl->id;
        lpcdT->cColonist = 0;
        lpcdT->fCanColonize = 1;
        app.state.cColDrop++;
    }
    lpcdT->cColonist += cColonists;
    return LOWORD(cColonists);
}

void UpdatePopulations(APPSESSION &app) {
    int32_t lPopChg;
    PLANET *lppl;
    PLANET *lpplMac;
    int32_t lPopOld;
    int16_t fMac;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0)
            goto NextPlanet;
        lPopChg = PlanetQueryIO().pfnGrowth(app, lppl, 1);
        if (lPopChg != 0 && lPopChg < 0 && lppl->rgwtMin[3] > 0) {
            lPopOld = lppl->rgwtMin[3] - lPopChg;
            if (PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer) < 0) {
                TurnIO().pfnMessage(app, lppl->iPlayer, idmPopulationHasDecreased, static_cast<MsgGoto>(lppl->id), lppl->id, LOWORD(lPopOld), HIWORD(lPopOld), LOWORD(lppl->rgwtMin[3]),
                            HIWORD(lppl->rgwtMin[3]), 0, 0);
            } else {
                TurnIO().pfnMessage(app, lppl->iPlayer, idmPopulationHasDecreasedColonistsDueOvercrowding, static_cast<MsgGoto>(lppl->id), lppl->id, -LOWORD(lPopChg),
                            LOWORD(static_cast<uint32_t>((static_cast<uint32_t>(-lPopChg) >> 0x10))), 0, 0, 0, 0);
            }
        }
    NextPlanet:
        if (lppl->iPlayer != iplrNone && lppl->rgwtMin[3] == 0) {
            fMac = RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh;
            TurnIO().pfnMessage2(app, lppl->iPlayer, static_cast<MessageId>((lPopChg < 0 ? idmColonistsHaveDiedOffLongerControlPlanet : idmColonistsHaveJumpedShipLongerControlPlanet) + fMac),
                         static_cast<MsgGoto>(lppl->id), lppl->id, 0);
            UninhabitPlanet(app, lppl);
        }
        if (lppl->iPlayer == iplrNone) {
            UninhabitPlanet(app, lppl);
        }
    }
    return;
}

void UpdateGuesses(APPSESSION &app) {
    PLANET *lppl;
    float   pct;
    PLANET *lpplMac;
    int32_t l;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->rgwtMin[3] != 0) {
            l = lppl->rgwtMin[3];
            if (RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
                l = 0;
            } else {
                l += -static_cast<int32_t>((l >> 3)) + RandomIO().pfnRandom(app, static_cast<int32_t>((l >> 2)));
                l = static_cast<int32_t>((l >> 2));
                if (l > 4090) {
                    l = 4090;
                } else if (l < 1) {
                    l = 1;
                }
            }
            lppl->uPopGuess = LOWORD(l);
            if (lppl->cDefenses == 0) {
                lppl->uDefGuess = 0;
            } else {
                CalcPctSurvive(app, lppl, &pct, nullptr);
                l = 100 - static_cast<int32_t>((static_cast<double>(pct) * 100.0 + 0.5)) + 4;
                l = static_cast<int32_t>((l / 6));
                if (l < 1) {
                    l = 1;
                } else if (l > 15) {
                    l = 15;
                }
                lppl->uDefGuess = LOWORD(l);
            }
        } else {
            lppl->SetUGuesses(0);
        }
    }
    return;
}

void MineMinerals(APPSESSION &app) {
    int32_t rglQuan[3];
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        ColonyIO().pfnEstimateMining(app, lppl, rglQuan, -1, 1);
    }
    return;
}

void MeteorStrike(APPSESSION &app) {
    int16_t rgEnv[3];
    int16_t iT;
    int32_t rgQuan[4];
    int16_t iSize;
    PLANET *lppl;
    int16_t rgAffect[3];
    int16_t i;
    int16_t iConc;
    int16_t j;

    if (RandomIO().pfnRandom(app, 20) == 0) {
        lppl = app.state.lpPlanets + RandomIO().pfnRandom(app, app.state.cPlanet);
        if ((lppl->iPlayer == iplrNone || lppl->rgwtMin[3] <= 50 || app.state.game.turn >= 20) && app.state.game.turn >= 10) {
            iSize = RandomIO().pfnRandom(app, 4);
            for (i = 0; i < 3; i++) {
                rgEnv[i] = i;
            }
            for (i = 0; i < 3; i++) {
                j = RandomIO().pfnRandom(app, 3);
                iT = rgEnv[i];
                rgEnv[i] = rgEnv[j];
                rgEnv[j] = iT;
            }
            for (i = 0; i < 3; i++) {
                rgAffect[i] = i;
                rgQuan[i] = static_cast<int16_t>((RandomIO().pfnRandom(app, 250) + 50));
            }
            for (i = 0; i < 2; i++) {
                j = RandomIO().pfnRandom(app, 3 - i) + i;
                iT = rgAffect[i];
                rgAffect[i] = rgAffect[j];
                rgAffect[j] = iT;
            }
            for (i = 0; i < app.state.game.cPlayer; i++) {
                TurnIO().pfnMessage(app, i,
                            static_cast<MessageId>((i == lppl->iPlayer && RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) != raMacintosh ? idmSmallCometHasCrashedPlanetKilling25
                                                                                                     : idmSmallCometHasCrashedBringingNewMinerals) +
                                iSize),
                            static_cast<MsgGoto>(lppl->id), lppl->id, rgEnv[0], rgEnv[1], rgEnv[2], 0, 0, 0);
            }
            if (lppl->iPlayer != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                lppl->rgwtMin[3] -= static_cast<int32_t>((lppl->rgwtMin[3] * static_cast<int16_t>((20 * iSize + 25)))) / 100;
            }
            for (i = 0; i <= iSize && i < 3; i++) {
                rgQuan[rgAffect[i]] = rgQuan[rgAffect[i]] + static_cast<int16_t>((RandomIO().pfnRandom(app, 17000) + 3000));
                iConc = lppl->rgMinConc[rgAffect[i]];
                iConc += RandomIO().pfnRandom(app, 50) + 50;
                if (iSize == 3) {
                    iConc += RandomIO().pfnRandom(app, 15) + 15;
                }
                if (iConc > 200) {
                    iConc = 200;
                }
                lppl->rgMinConc[rgAffect[i]] = iConc;
            }
            for (i = 0; i < 3; i++) {
                lppl->rgwtMin[i] += static_cast<int32_t>((rgQuan[i] >> 4));
            }
            for (i = 0; i < 3 && i <= iSize; i++) {
                iT = RandomIO().pfnRandom(app, 3) + 3;
                if (iSize == 3) {
                    iT += RandomIO().pfnRandom(app, 3) + 3;
                }
                if (RandomIO().pfnRandom(app, 2) != 0) {
                    iT = -iT;
                }
                j = lppl->rgEnvVar[i] + iT;
                if (j < 1) {
                    j = 1;
                } else if (j > 99) {
                    j = 99;
                }
                lppl->rgEnvVar[i] = j;
                j = lppl->rgEnvVarOrig[i] + iT;
                if (j < 1) {
                    j = 1;
                } else if (j > 99) {
                    j = 99;
                }
                lppl->rgEnvVarOrig[i] = j;
            }
            TossNonAutoBuildItems(app, lppl);
        }
    }
    return;
}

void TossNonAutoBuildItems(APPSESSION &app, PLANET *lppl) {
    int16_t iDst;
    int16_t iSrc;

    if (lppl->lpplprod) {
        iDst = 0;
        for (iSrc = 0; iSrc < lppl->lpplprod->iprodMac; iSrc++) {
            if (lppl->lpplprod->rgprod()[iSrc].grobj == grobjPlanet && lppl->lpplprod->rgprod()[iSrc].iItem < mdIdleFactory) {
                if (iSrc > iDst) {
                    lppl->lpplprod->rgprod()[iDst] = lppl->lpplprod->rgprod()[iSrc];
                }
                iDst++;
            }
        }
        if (iDst > 0) {
            lppl->lpplprod->iprodMac = iDst;
        } else {
            FreePl(app, lppl->lpplprod);
            lppl->lpplprod = nullptr;
        }
    }
    return;
}

void PlanetaryClimateChange(APPSESSION &app) {
    int16_t iT;
    PLANET *lppl;
    int16_t i;
    int16_t j;

    if (RandomIO().pfnRandom(app, 20) == 0) {
        lppl = app.state.lpPlanets + RandomIO().pfnRandom(app, app.state.cPlanet);
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] <= 50 || app.state.game.turn >= 20) {
            i = RandomIO().pfnRandom(app, 3);
            if (lppl->iPlayer != iplrNone) {
                TurnIO().pfnMessage2(app, lppl->iPlayer, idmFundamentalChangesEnvironmentHavePermanentlyAlte, static_cast<MsgGoto>(lppl->id), lppl->id, i);
            }
            iT = RandomIO().pfnRandom(app, 3) + 3;
            if (iT == 3) {
                iT += RandomIO().pfnRandom(app, 3) + 3;
            }
            if (RandomIO().pfnRandom(app, 2) != 0) {
                iT = -iT;
            }
            j = lppl->rgEnvVar[i] + iT;
            if (j < 1) {
                j = 1;
            } else if (j > 99) {
                j = 99;
            }
            lppl->rgEnvVar[i] = j;
            j = lppl->rgEnvVarOrig[i] + iT;
            if (j < 1) {
                j = 1;
            } else if (j > 99) {
                j = 99;
            }
            lppl->rgEnvVarOrig[i] = j;
            TossNonAutoBuildItems(app, lppl);
        }
    }
    return;
}

void DiscoverNewMinerals(APPSESSION &app) {
    PLANET *lppl;
    int16_t i;

    if (RandomIO().pfnRandom(app, 15 - app.state.game.mdSize) == 0) {
        lppl = app.state.lpPlanets + RandomIO().pfnRandom(app, app.state.cPlanet);
        if (app.state.game.turn >= 10) {
            i = RandomIO().pfnRandom(app, 3);
            if (lppl->iPlayer != iplrNone) {
                TurnIO().pfnMessage(app, lppl->iPlayer, idmSurveyorsHaveDiscoveredPreviouslyUnknownDepositS, static_cast<MsgGoto>(lppl->id), lppl->id, i, 0, 0, 0, 0, 0);
            }
            if (lppl->rgMinConc[i] < 180) {
                lppl->rgMinConc[i] += RandomIO().pfnRandom(app, 15) + 5;
            }
        }
    }
    return;
}

void MysteryTrader(APPSESSION &app) {
    int16_t     iSrc;
    int16_t     cRand;
    int16_t     i;
    THING      *lpth;
    GrbitTrader grbitTrader;
    int16_t     rgC[4];

    if (app.state.game.turn >= 40) {
        if (static_cast<uint32_t>(app.state.game.turn) % 100 == 71) {
            cRand = 2;
        } else if (static_cast<uint32_t>(app.state.game.turn) % 100 == 33) {
            cRand = 3;
        } else if ((app.state.game.turn & 0x7f) == 0x31) {
            cRand = 4;
        } else {
            if (app.state.game.turn & 1) {
                return;
            }
            cRand = 7;
        }
        if (RandomIO().pfnRandom(app, cRand) == 0) {
            lpth = LpthNew(app, 0, ithMysteryTrader);
            if (lpth) {
                { const auto iWarpForThing = (RandomIO().pfnRandom(app, 5) + 8); auto &thForThing = *(lpth); thForThing.tht().SetIWarp(iWarpForThing); }
                for (i = 0; i < 4; i += 2) {
                    rgC[i] = RandomIO().pfnRandom(app, 400 * app.state.game.mdSize + 361) + 1020;
                }
                if (RandomIO().pfnRandom(app, 2) == 0) {
                    rgC[1] = 1020;
                    rgC[3] = 400 * app.state.game.mdSize + 1380;
                } else {
                    rgC[1] = 400 * app.state.game.mdSize + 1380;
                    rgC[3] = 1020;
                }
                iSrc = RandomIO().pfnRandom(app, 2);
                lpth->pt().SetX(rgC[iSrc]);
                lpth->pt().SetY(rgC[iSrc == 0]);
                lpth->tht().ptDest().SetX(rgC[iSrc + 2]);
                { const auto yForThing = (rgC[(iSrc == 0) + 2]); auto &thForThing = *(lpth); thForThing.tht().ptDest().SetY(yForThing); }
                if (app.state.game.turn < 100) {
                    cRand = 5;
                } else if (app.state.game.turn < 250) {
                    cRand = 3;
                } else {
                    cRand = 2;
                }
                if (lpth->tht().iWarp() <= 9) {
                    cRand++;
                } else if (lpth->tht().iWarp() >= 11) {
                    cRand--;
                }
                if (RandomIO().pfnRandom(app, 10) < cRand) {
                    if (RandomIO().pfnRandom(app, 6) == 0) {
                        lpth->tht().SetGrbitTrader(grbitTraderLifeboat);
                    } else {
                        lpth->tht().SetGrbitTrader(grbitTraderNone);
                    }
                } else {
                    grbitTrader = static_cast<GrbitTrader>(1 << RandomIO().pfnRandom(app, 13));
                    switch (grbitTrader) {
                    default:
                        break;
                    case grbitTraderTorp:
                    case grbitTraderBeam:
                    case grbitTraderGenesis:
                    case grbitTraderJumpgate:
                        grbitTrader = static_cast<GrbitTrader>(1 << RandomIO().pfnRandom(app, 13));
                        if (((app.state.game.turn < 120 && grbitTrader == grbitTraderBeam) || (app.state.game.turn < 150 && grbitTrader == grbitTraderGenesis) ||
                             (app.state.game.turn < 180 && grbitTrader == grbitTraderJumpgate)) &&
                            RandomIO().pfnRandom(app, 2) != 0) {
                            grbitTrader = grbitTraderNone;
                        }
                    }
                    lpth->tht().SetGrbitTrader(grbitTrader);
                }
                for (i = 0; i < app.state.game.cPlayer; i++) {
                    TurnIO().pfnMessage2(app, i, idmMysteriousTradingVesselBroadcastingProposalHasDe, gotoThing, lpth->idFull(), 0);
                }
            }
        }
    }
    return;
}

void UpdatePlayerScores(APPSESSION &app) {
    int32_t  lScoreTot;
    int16_t  cFirst;
    SCORE    score;
    int16_t  cDead;
    int16_t  c;
    int16_t  i;
    uint8_t  rgcCond[16];
    uint16_t wWinners2;
    int32_t  rglScore[16];
    int16_t  iScoreMax;
    int16_t  j;
    uint16_t wWinners;
    int16_t  imsg;
    int32_t  lScore2nd;
    int32_t  lScoreMax;

    cDead = 0;
    cFirst = 0;
    iScoreMax = 0;
    lScore2nd = 0;
    lScoreTot = 0;
    app.state.gd.fGameOverMan = 0;
    memset(rgcCond, 0, 16);
    for (i = 0; i < app.state.game.cPlayer; i++) {
        rglScore[i] = CalcPlayerScore(app, i, &score);
        app.state.vlprgScoreX[i].score = score;
        app.state.vlprgScoreX[i].iPlayer = i;
        app.state.vlprgScoreX[i].fValid = 1;
        app.state.vlprgScoreX[i].grbitVC = 0;
        lScoreTot += rglScore[i];
        if (score.cPlanet == 0 && score.rgcsh[0] == 0 && score.rgcsh[1] == 0 && score.rgcsh[2] == 0 && !app.state.rgplr[i].fDead) {
            app.state.rgplr[i].fDead = 1;
            for (j = 0; j < app.state.game.cPlayer; j++) {
                if (j != i) {
                    FSendPrependedPlrMsg(app, j, idmTracesHaveEliminatedGalaxyMayRestPeace, gotoScore, i | 0x30, 0, 0, 0, 0, 0, 0);
                }
            }
        }
        if (score.cPlanet >= LMulDiv(app.state.cPlanet, GetVCVal(app, &app.state.game, vcOwnsPercentPlanets, 0), 100)) {
            app.state.vlprgScoreX[i].grbitVC |= 1;
            if (GetVCCheck(&app.state.game, vcOwnsPercentPlanets) != 0) {
                rgcCond[i]++;
            }
        }
        if (static_cast<int32_t>((static_cast<uint32_t>((score.rgcsh[2] & 0x1fff)) << (score.rgcsh[2] >> 0xd << 1))) >= GetVCVal(app, &app.state.game, vcOwnsCapitalShips, 0)) {
            app.state.vlprgScoreX[i].grbitVC |= 0x20;
            if (GetVCCheck(&app.state.game, vcOwnsCapitalShips) != 0) {
                rgcCond[i]++;
            }
        }
        if (rglScore[i] >= GetVCVal(app, &app.state.game, vcExceedsScore, 0)) {
            app.state.vlprgScoreX[i].grbitVC |= 4;
            if (GetVCCheck(&app.state.game, vcExceedsScore) != 0) {
                rgcCond[i]++;
            }
        }
        c = 0;
        for (j = 0; j < 6; j++) {
            if (app.state.rgplr[i].rgTech[j] >= GetVCVal(app, &app.state.game, vcAttainsTechLevel, 0)) {
                c++;
            }
        }
        if (c >= GetVCVal(app, &app.state.game, vcAttainsTechFields, 0)) {
            app.state.vlprgScoreX[i].grbitVC |= 2;
            if (GetVCCheck(&app.state.game, vcAttainsTechLevel) != 0) {
                rgcCond[i]++;
            }
        }
        if (static_cast<int32_t>((score.cResources / 1000)) >= GetVCVal(app, &app.state.game, vcProductionCapacity, 0)) {
            app.state.vlprgScoreX[i].grbitVC |= 0x10;
            if (GetVCCheck(&app.state.game, vcProductionCapacity) != 0) {
                rgcCond[i]++;
            }
        }
    }
    if (app.state.game.cPlayer == 1) {
        app.state.vlprgScoreX->SetIRank(1);
    } else {
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fDead) {
                cDead++;
            }
            app.state.rgplr[i].wScore = 1;
            for (j = 0; j < app.state.game.cPlayer; j++) {
                if (rglScore[j] > rglScore[i]) {
                    app.state.rgplr[i].wScore++;
                }
            }
            if (app.state.rgplr[i].wScore == 1) {
                iScoreMax = i;
                lScoreMax = rglScore[i];
                cFirst++;
            } else if (app.state.rgplr[i].wScore == 2) {
                lScore2nd = rglScore[i];
            }
        }
        if (cFirst > 1) {
            lScore2nd = lScoreMax;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.vlprgScoreX[i].turn = app.state.rgplr[i].wScore;
        }
        if (static_cast<int16_t>(app.state.game.turn) >= GetVCVal(app, &app.state.game, vcHighestScoreAfterYears, 0) && cFirst == 1) {
            app.state.vlprgScoreX[iScoreMax].grbitVC |= 0x40;
            if (GetVCCheck(&app.state.game, vcHighestScoreAfterYears) != 0) {
                rgcCond[iScoreMax]++;
            }
        }
        if (cDead + 1 >= app.state.game.cPlayer) {
            app.state.gd.fGameOverMan = 1;
            if (!app.state.rgplr[iScoreMax].fDead) {
                FSendPrependedPlrMsg(app, iScoreMax, idmTracesEveryOtherRivalHaveEliminatedGalaxy, gotoScore, 0, 0, 0, 0, 0, 0, 0);
            }
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (i != iScoreMax) {
                    FSendPrependedPlrMsg(app, i, idmDeadPlanetsHaveOverrunSpaceshipsDefeated, gotoScore, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        } else {
            if (lScoreMax >= static_cast<int32_t>((lScore2nd * static_cast<int16_t>((GetVCVal(app, &app.state.game, vcExceedsSecondPlaceBy, 0) + 100)))) / 100) {
                app.state.vlprgScoreX[iScoreMax].grbitVC |= 8;
                if (GetVCCheck(&app.state.game, vcExceedsSecondPlaceBy) != 0) {
                    rgcCond[iScoreMax]++;
                }
            }
            if (app.state.game.turn >= static_cast<uint16_t>(GetVCVal(app, &app.state.game, vcMinYearsBeforeWin, 0))) {
                wWinners = 0;
                j = GetVCVal(app, &app.state.game, vcMeetsNumCriteria, 0);
                if (j >= 1) {
                    for (i = app.state.game.cPlayer - 1; i >= 0; i--) {
                        wWinners *= 2;
                        if (rgcCond[i] >= j) {
                            app.state.vlprgScoreX[i].fWinner = 1;
                            wWinners |= 1;
                        }
                    }
                    if (wWinners != 0) {
                        app.state.gd.fGameOverMan = 1;
                    }
                }
                if (app.state.gd.fGameOverMan) {
                    i = 0;
                    j = 1;
                    while (i < app.state.game.cPlayer) {
                        wWinners2 = wWinners;
                        if (app.state.rgplr[i].fDead) {
                            imsg = idmDeadPlanetsHaveOverrunSpaceshipsDefeated;
                        } else if (!(j & wWinners)) {
                            imsg = idmForcesHaveDeclaredWinnerGameAdvisedAccept;
                        } else if ((j ^ wWinners) != 0) {
                            imsg = idmAlongHaveDeclaredWinnersGameMayContinue;
                            wWinners2 &= ~j;
                        } else {
                            imsg = idmHaveDeclaredWinnerGameMayContinuePlay;
                        }
                        FSendPrependedPlrMsg(app, i, static_cast<MessageId>(imsg), gotoScore, wWinners2, 0, 0, 0, 0, 0, 0);
                        i++;
                        j *= 2;
                    }
                }
            }
        }
    }
    return;
}

void CreateBackupDir(APPSESSION &app) {
    char *pchT;

    strcpy(app.state.szBackup, app.state.szBase);
    pchT = strrchr(app.state.szBackup, chDirSep);
    if (!pchT) {
        pchT = app.state.szBackup;
    } else {
        pchT++;
    }
    *pchT = 0;
    if (app.state.vcBackupDirs <= 1) {
        strcpy(pchT, "backup");
    } else if (app.state.vcBackupDirs <= 99) {
        CchSprintf(pchT, "backup%d", static_cast<uint32_t>(app.state.game.turn) % app.state.vcBackupDirs);
    } else {
        CchSprintf(pchT, "backup.%03d", static_cast<uint32_t>(app.state.game.turn) % app.state.vcBackupDirs);
    }
    MakeDir(app.state.szBackup);
    strcat(app.state.szBackup, szDirSep);
    return;
}

int16_t FPacketDecay(APPSESSION &app, THING *lpth, int16_t pctRate) {
    uint16_t iRateMin;
    int16_t  iRate;
    int16_t  i;
    uint16_t wDecay;
    int32_t  lDecay;

    if (lpth->thp().iDecayRate() <= decayNone) {
        return 0;
    }
    switch (lpth->thp().iDecayRate()) {
    case decay10Pct:
        iRate = 10;
        break;
    case decay25Pct:
        iRate = 25;
        break;
    case decay50Pct:
        iRate = 50;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[lpth->iplr()], rsMajorAdv) == raMassAccel) {
        iRate /= 2;
        iRateMin = 5;
    } else {
        iRateMin = 10;
    }
    lDecay = 0;
    for (i = 0; i < 3; i++) {
        if (lpth->thp().rgwtMin(i) != 0) {
            wDecay = LOWORD(static_cast<int32_t>((static_cast<uint32_t>((lpth->thp().rgwtMin(i) * iRate)) * pctRate)) / 10000);
            wDecay = iRateMin <= wDecay ? wDecay : iRateMin;
            if (lpth->thp().rgwtMin(i) <= static_cast<int16_t>(wDecay)) {
                wDecay = lpth->thp().rgwtMin(i);
            }
            lpth->thp().SetRgwtMin(i, lpth->thp().rgwtMin(i) - wDecay);
            lDecay += lpth->thp().rgwtMin(i);
        }
    }
    if (lDecay == 0) {
        FreeLpth(app, lpth);
        return 1;
    }
    { const auto wtMaxForThing = (LOWORD(static_cast<int32_t>(((lDecay + 9) / 10)))); auto &thForThing = *(lpth); thForThing.thp().SetWtMax(wtMaxForThing); }
    return 0;
}

void ThingDecay(APPSESSION &app) {
    THING   *lpthMac;
    int32_t  pctDecay;
    int16_t  i;
    int16_t  ifl;
    FLEET   *lpfl;
    THING   *lpth;
    uint16_t wDecay;
    int32_t  lDecay;
    int16_t  fMineExpert;
    int32_t  dy;
    int32_t  dx;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        lpfl->fBombed = 0;
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMineralPacket) {
            if (lpth->thp().iWarp() == 0) {
                if (lpth->thp().fMoved()) {
                    lpth->thp().SetFMoved(0);
                    continue;
                }
                lDecay = 0;
                for (i = 0; i < 3; i++) {
                    if (lpth->thp().rgwtMin(i) != 0) {
                        wDecay = 10 <= lpth->thp().rgwtMin(i) / 10 ? lpth->thp().rgwtMin(i) / 10 : 10;
                        lpth->thp().SetRgwtMin(i, lpth->thp().rgwtMin(i) - wDecay);
                        if (lpth->thp().rgwtMin(i) < 0) {
                            lpth->thp().SetRgwtMin(i, 0);
                        }
                        lDecay += lpth->thp().rgwtMin(i);
                    }
                }
                if (lDecay == 0) {
                    FreeLpth(app, lpth);
                LFixUpLpth:
                    lpth--;
                    lpthMac--;
                    continue;
                } else {
                    { const auto wtMaxForThing = (LOWORD(static_cast<int32_t>(((lDecay + 9) / 10)))); auto &thForThing = *(lpth); thForThing.thp().SetWtMax(wtMaxForThing); }
                    continue;
                }
            } else if (FPacketDecay(app, lpth, 100)) {
                goto LFixUpLpth;
            }
            continue;
        } else if (lpth->ith() == ithMinefield) {
            fMineExpert = RaceQueryIO().pfnStat(&app.state.rgplr[lpth->iplr()], rsMajorAdv) == raMines;
            if (lpth->thm().fDetonate()) {
                lDecay = lpth->thm().cMines();
                for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                    lpfl = app.state.rglpfl[ifl];
                    if (!app.state.rglpfl[ifl])
                        break;
                    if (!lpfl->fDead) {
                        dx = static_cast<int16_t>((lpfl->pt.x - lpth->pt().x()));
                        dy = static_cast<int16_t>((lpfl->pt.y - lpth->pt().y()));
                        /* The original set fBombed even when the fleet was
                           immune to this field, sparing it from every other
                           detonating field. */
                        if (!lpfl->fBombed && static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy)) <= lDecay && !FTravelThroughMineFields(app, lpfl, nullptr, lpth)) {
                            lpfl->fBombed = 1;
                        }
                    }
                }
            }
            pctDecay = static_cast<int16_t>((((fMineExpert == 0) * 3 + 1) * CPlanetsInCircle(app, lpth->pt(), lpth->thm().cMines()) + 2));
            if (pctDecay > 50) {
                pctDecay = 50;
            }
            if (lpth->thm().fDetonate()) {
                pctDecay += 25;
            }
            lDecay = static_cast<int32_t>((lpth->thm().cMines() * pctDecay)) / 100;
            if (lDecay < pctDecay) {
                lDecay = pctDecay;
            }
            if (lpth->thm().iType() != mineSpeedBump) {
                lDecay = 10 <= lDecay ? lDecay : 10;
            }
            if (lDecay >= lpth->thm().cMines()) {
                FreeLpth(app, lpth);
                lpth--;
                lpthMac--;
            } else {
                lpth->thm().SetCMines(lpth->thm().cMines() - lDecay);
            }
        }
    }
    return;
}

void UnmarkMineFields(APPSESSION &app) {
    THING *lpthMac;
    THING *lpth;

    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMinefield) {
            lpth->thm().SetGrbitPlrNow(0);
        }
    }
    return;
}

void SweepForMines(APPSESSION &app) {
    int16_t  iplr;
    THING   *lpthMac;
    POINT16  pt;
    int32_t  dy;
    int32_t  lCur;
    PLANET  *lppl;
    int16_t  ifl;
    FLEET   *lpfl;
    THING   *lpth;
    int32_t  cMineCur;
    int32_t  dx;
    int32_t  cMine;
    uint16_t grbitPlr;
    PLANET  *lpplMac;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        cMine = CMineSweepFromLpfl(app, lpfl);
        if (cMine > 0 && !lpfl->fDead) {
            iplr = lpfl->iplr();
            grbitPlr = 1 << lpfl->iplr();
            pt = lpfl->pt;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->ith() == ithMinefield && lpth->iplr() != iplr && FAttackPlayer(app, lpfl, lpth->iplr())) {
                    dx = static_cast<int16_t>((pt.x - lpth->pt().x()));
                    dy = static_cast<int16_t>((pt.y - lpth->pt().y()));
                    lCur = static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy));
                    if (lpth->thm().cMines() >= static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy))) {
                        if (lpth->thm().iType() == mineSpeedBump) {
                            cMineCur = static_cast<int32_t>((cMine / 3));
                        } else {
                            cMineCur = cMine;
                        }
                        if (cMineCur < 2) {
                            cMineCur = 2;
                        }
                        if (lpth->thm().cMines() - cMineCur < lCur - 1) {
                            cMineCur = lpth->thm().cMines() - lCur + 1;
                        }
                        if (cMineCur > lpth->thm().cMines()) {
                            cMineCur = lpth->thm().cMines();
                        }
                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasSweptMinesMineField, static_cast<MsgGoto>(0x8000 | lpfl->id), lpfl->id, LOWORD(cMineCur), HIWORD(cMineCur), lpth->iplr(),
                                    lpth->thm().iType(), lpth->pt().x(), lpth->pt().y());
                        TurnIO().pfnMessage(app, lpth->iplr(), idmSomeoneHasSweptMinesMineField, gotoThing, lpth->idFull(), LOWORD(cMineCur), HIWORD(cMineCur), lpth->thm().iType(),
                                    lpth->pt().x(), lpth->pt().y(), 0);
                        lpth->thm().SetCMines(lpth->thm().cMines() - cMineCur);
                        if (lpth->thm().cMines() <= 0) {
                            FreeLpth(app, lpth);
                            lpth--;
                            lpthMac--;
                        } else {
                            lpth->thm().SetGrbitPlr(lpth->thm().grbitPlr() | (1 << lpfl->iPlayer));
                        }
                    }
                }
            }
        }
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->fStarbase && lppl->iPlayer != iplrNone) {
            cMine = CMineSweepFromLphul(app, &app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul);
            if (cMine > 0) {
                iplr = lppl->iPlayer;
                grbitPlr = 1 << lppl->iPlayer;
                pt = app.state.rgptPlan[lppl->id];
                lpth = app.state.lpThings;
                lpthMac = app.state.lpThings + app.state.cThing;
                for (; lpth < lpthMac; lpth++) {
                    if (lpth->ith() == ithMinefield && lpth->iplr() != iplr && iplr != lpth->iplr() && app.state.rgplr[iplr].rgmdRelation[lpth->iplr()] != 1) {
                        dx = static_cast<int16_t>((pt.x - lpth->pt().x()));
                        dy = static_cast<int16_t>((pt.y - lpth->pt().y()));
                        lCur = static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy));
                        if (lpth->thm().cMines() >= static_cast<uint32_t>((dx * dx)) + static_cast<uint32_t>((dy * dy))) {
                            if (lpth->thm().iType() == mineSpeedBump) {
                                cMineCur = static_cast<int32_t>((cMine / 3));
                            } else {
                                cMineCur = cMine;
                            }
                            if (cMineCur < 2) {
                                cMineCur = 2;
                            }
                            if (lpth->thm().cMines() - cMineCur < lCur - 1) {
                                cMineCur = lpth->thm().cMines() - lCur + 1;
                            }
                            if (cMineCur > lpth->thm().cMines()) {
                                cMineCur = lpth->thm().cMines();
                            }
                            TurnIO().pfnMessage(app, iplr, idmStarbaseHasSweptMinesMineField, static_cast<MsgGoto>(lppl->id), lppl->id, LOWORD(cMineCur), HIWORD(cMineCur), lpth->iplr(),
                                        lpth->thm().iType(), lpth->pt().x(), lpth->pt().y());
                            TurnIO().pfnMessage(app, lpth->iplr(), idmSomeoneHasSweptMinesMineField, gotoThing, lpth->idFull(), LOWORD(cMineCur), HIWORD(cMineCur),
                                        lpth->thm().iType(), lpth->pt().x(), lpth->pt().y(), 0);
                            lpth->thm().SetCMines(lpth->thm().cMines() - cMineCur);
                            if (lpth->thm().cMines() <= 0) {
                                FreeLpth(app, lpth);
                                lpth--;
                                lpthMac--;
                            } else {
                                lpth->thm().SetGrbitPlr(lpth->thm().grbitPlr() | (1 << lppl->iPlayer));
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void BreedColonistsInTransit(APPSESSION &app) {
    int16_t fNoBreeders;
    char    grfBreeder[16];
    int32_t lColGain;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t i;
    int32_t lColGainAct;

    fNoBreeders = 1;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if ((grfBreeder[i] = RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raDefend) == 1) {
            fNoBreeders = 0;
        }
    }
    if (!fNoBreeders) {
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
            if (!lpfl->fDead && grfBreeder[lpfl->iPlayer] != 0 && lpfl->rgwtMin[3] != 0) {
                lColGain = static_cast<int32_t>((lpfl->rgwtMin[3] * static_cast<int16_t>(app.state.rgplr[lpfl->iPlayer].pctIdealGrowth))) / 200;
                if (lColGain <= 0) {
                    if (RandomIO().pfnRandom(app, 3) != 0)
                        continue;
                    lColGain = 1;
                }
                lColGainAct = ChgCargo(app, grobjFleet, lpfl->id, Colonists, lColGain, nullptr);
                if (lColGainAct > 0) {
                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmColonistsHaveMadeGoodUseTimeIncreasing, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(lColGainAct));
                }
                if (lColGainAct < lColGain && lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == lpfl->iPlayer) {
                        lColGain -= lColGainAct;
                        lppl->rgwtMin[3] += lColGain;
                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmBreedingActivitiesHaveOverflowedLivingSpaceColon, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(lColGain),
                                    HIWORD(lColGain), lpfl->idPlanet, 0, 0, 0);
                    }
                }
            }
        }
    }
    return;
}

void UpdateResearchStatus(APPSESSION &app, int16_t fUsePool) {
    mdPartAvail  mdAvail;
    int16_t      fRedoItAll;
    int16_t      iTechCur;
    int16_t      fUsePoolOrig;
    int16_t      iTechNext;
    int16_t      iT;
    int16_t      iItem;
    int16_t      fGeneral;
    int16_t      fChgNow;
    int16_t      i;
    int16_t      ibitCur;
    int32_t      rglFieldSpent[6];
    HullSlotType grbitCur;
    int16_t      cPlrAlive;
    int32_t      lSpent;
    PART         part;
    int32_t      l;
    int16_t      iTT;
    int32_t      l15pct;
    int16_t      iTechNext2;
    char         TechLevel;
    int16_t      jj;
    int16_t      iGoto;
    MessageId    idm;

    cPlrAlive = 0;
    fUsePoolOrig = fUsePool;
    if (fUsePool) {
        for (i = 0; i < 6; i++) {
            rglFieldSpent[i] = 0;
        }
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        fUsePool = fUsePoolOrig;
        fGeneral = RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceGeneralizedResearch);
        iTechCur = app.state.rgplr[i].iTechNow();
        iTechNext = app.state.rgplr[i].iTechNext();
        app.state.idPlayer = i;
        if (!app.state.rgplr[i].fDead) {
            cPlrAlive++;
        }
    RedoItAll:
        fRedoItAll = 0;
        for (iT = 0; iT < 6; iT++) {
            lSpent = app.state.rgplr[i].rgResSpent[iT];
            fChgNow = 0;
            if (app.state.game.fSlowTech) {
                lSpent = static_cast<int32_t>((lSpent * 2));
            }
            if (iT == iTechCur && fUsePool && fGeneral < 2) {
                if (fGeneral) {
                    fRedoItAll = 1;
                    fGeneral = 2;
                    lSpent += static_cast<int32_t>(((app.state.rgplr[i].lResLastYear + 1) / 2));
                    rglFieldSpent[iT] += static_cast<int32_t>(((app.state.rgplr[i].lResLastYear + 1) / 2));
                    for (iTT = 0; iTT < 6; iTT++) {
                        if (iTT != iT) {
                            l15pct = static_cast<int32_t>((static_cast<uint32_t>((app.state.rgplr[i].lResLastYear * 3)) + 19)) / 20;
                            if (app.state.game.fSlowTech) {
                                app.state.rgplr[i].rgResSpent[iTT] += static_cast<int32_t>((l15pct / 2));
                            } else {
                                app.state.rgplr[i].rgResSpent[iTT] += l15pct;
                            }
                            rglFieldSpent[iTT] += l15pct;
                        }
                    }
                } else {
                    lSpent += app.state.rgplr[i].lResLastYear;
                    rglFieldSpent[iT] += app.state.rgplr[i].lResLastYear;
                }
            }
        CheckForBreakthrough:
            if (app.state.rgplr[i].rgTech[iT] < 26 && (!app.state.rgplr[i].fCrippled || app.state.rgplr[i].rgTech[iT] < 10)) {
                l = ResearchIO().pfnTechCost(app, static_cast<TechFieldType>(iT), app.state.rgplr[i].rgTech[iT] + 1, i);
                if (l <= lSpent && !(app.state.rgplr[i].rgTech[iT] >= 26 && (app.state.rgplr[i].fCrippled || app.state.rgplr[i].rgTech[iT] >= 26))) {
                    iTechNext2 = iTechCur;
                    lSpent -= l;
                    app.state.rgplr[i].rgTech[iT]++;
                    TechLevel = app.state.rgplr[i].rgTech[iT];
                    if (TechLevel == 26 && iTechNext == 6) {
                        iTechNext = 7;
                    }
                    if (iTechCur == iT && iTechNext != 6) {
                        if (iTechNext != 7) {
                            iTechNext2 = iTechNext;
                        } else {
                            iTechNext2 = 0;
                            for (jj = 1; jj < 6; jj++) {
                                if (app.state.rgplr[i].rgTech[jj] < LOBYTE(static_cast<int16_t>(((static_cast<uint16_t>(iTechNext2) & 0xff00) | (static_cast<uint16_t>(app.state.rgplr[i].rgTech[iTechNext2]) & 0xff))))) {
                                    iTechNext2 = jj;
                                }
                            }
                        }
                        fChgNow = 1;
                    }
                    TurnIO().pfnMessage(app, i, !fGeneral ? idmScientistsHaveCompletedResearchTechLevelWill : idmScientistsHaveCompletedResearchTechLevelPrimary,
                                gotoResearch, TechLevel, iT, iTechNext2, 0, 0, 0, 0);
                    grbitCur = hstEngine;
                    ibitCur = 0;
                    while (grbitCur != hstNone) {
                        if (grbitCur & (hstEngine | hstScanner | hstShield | hstArmor | hstBeam | hstTorp | hstBomb | hstMining | hstMines | hstSpecialSB |
                                        hstSBHull | hstSpecialE | hstSpecialM | hstTerra | hstHull | hstPlanetary)) {
                            iItem = 0;
                            part.hs.grhst = grbitCur;
                            while (1) {
                                part.hs.iItem = iItem;
                                mdAvail = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(app, &part));
                                if (mdAvail == mdPartAvailInvalid)
                                    break;
                                if (mdAvail == mdPartAvailAvailable &&
                                    app.state.rgplr[i].rgTech[iT] == LOBYTE(static_cast<int16_t>(((static_cast<uint16_t>(iT) & 0xff00) | (static_cast<uint16_t>(part.pcom().rgTech()[iT]) & 0xff))))) {
                                    if (grbitCur == hstSBHull) {
                                        idm = idmRecentBreakthroughHasAlsoGivenHullDesign;
                                        iGoto = -3;
                                    } else if (grbitCur == hstHull) {
                                        idm = idmRecentBreakthroughHasAlsoGivenHullType;
                                        iGoto = -3;
                                    } else if (grbitCur == hstTerra && RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceTT) != 0 &&
                                               (iItem == iterraGravityTerraform3 || iItem == iterraTempTerraform3 || iItem == iterraRadiationTerraform3)) {
                                        iItem++;
                                        continue;
                                    } else {
                                        if (grbitCur == hstPlanetary && iItem >= iplanetarySDI && iItem <= iplanetaryNeutronShield) {
                                            idm = idmRecentBreakthroughHasAlsoTaughtHowBuild;
                                        } else if (grbitCur == hstPlanetary && iItem >= iplanetaryViewer50 && iItem <= iplanetarySnooper620X) {
                                            idm = idmRecentBreakthroughHasAlsoTaughtHowBuild2;
                                        } else {
                                            idm = idmRecentBreakthroughHasAlsoGivenBenefit;
                                        }
                                        iGoto = ibitCur << 8 | 0xc000 | iItem;
                                    }
                                    TurnIO().pfnMessage(app, i, idm, static_cast<MsgGoto>(iGoto), iT, grbitCur, iItem, 0, 0, 0, 0);
                                }
                                iItem++;
                            }
                        }
                        grbitCur = static_cast<HullSlotType>(grbitCur * 2);
                        ibitCur++;
                    }
                    if ((fUsePool || fChgNow || iTechNext == 7) && iTechNext != 6 && iT == iTechCur) {
                        if (iTechNext == 7) {
                            iTechNext = 0;
                            for (jj = 1; jj < 6; jj++) {
                                if (app.state.rgplr[i].rgTech[jj] < LOBYTE(static_cast<int16_t>(((static_cast<uint16_t>(iTechNext) & 0xff00) | (static_cast<uint16_t>(app.state.rgplr[i].rgTech[iTechNext]) & 0xff))))) {
                                    iTechNext = jj;
                                }
                            }
                            app.state.rgplr[app.state.idPlayer].SetITechNow(iTechNext);
                            app.state.rgplr[i].rgResSpent[iT] = 0;
                            iTechCur = iTechNext;
                            iTechNext = 7;
                        } else {
                            app.state.rgplr[app.state.idPlayer].SetITechNext(6);
                            app.state.rgplr[app.state.idPlayer].SetITechNow(iTechNext);
                            app.state.rgplr[i].rgResSpent[iT] = 0;
                            iTechCur = iTechNext;
                            iTechNext = 6;
                        }
                        if (app.state.game.fSlowTech) {
                            lSpent = static_cast<int32_t>(((lSpent + 1) >> 1));
                        }
                        app.state.rgplr[i].rgResSpent[iTechCur] += lSpent;
                        fUsePool = 0;
                        fRedoItAll = 1;
                    } else {
                        goto CheckForBreakthrough;
                    }
                } else {
                    if (app.state.game.fSlowTech) {
                        lSpent = static_cast<int32_t>(((lSpent + 1) >> 1));
                    }
                    app.state.rgplr[i].rgResSpent[iT] = lSpent;
                }
            }
        }
        if (fRedoItAll)
            goto RedoItAll;
    }
    app.state.idPlayer = iplrNone;
    fRedoItAll = 0;
    if (fUsePoolOrig && cPlrAlive > 1) {
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raStealth) {
                for (iT = 0; iT < 6; iT++) {
                    if (rglFieldSpent[iT] > 0) {
                        lSpent = static_cast<int32_t>((rglFieldSpent[iT] / cPlrAlive)) / 2;
                        if (lSpent > 1) {
                            fRedoItAll = 1;
                            TurnIO().pfnMessage2(app, i, idmIntelligenceGatheringActivitiesCombinedSynergist, gotoResearch, iT, LOWORD(lSpent));
                            if (app.state.game.fSlowTech) {
                                lSpent = static_cast<int32_t>(((lSpent + 1) >> 1));
                            }
                            app.state.rgplr[i].rgResSpent[iT] += lSpent;
                        }
                    }
                }
            }
        }
        if (fRedoItAll) {
            TurnIO().pfnResearch(app, 0);
        }
    }
    return;
}

int16_t IBestRemoteTerra(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fHelp) {
    int16_t iBest;
    int16_t i;
    PLAYER  plrSav;

    plrSav = app.state.rgplr[lppl->iPlayer];
    app.state.rgplr[lppl->iPlayer] = app.state.rgplr[iplr];
    for (i = 0; i < 3; i++) {
        app.state.rgplr[lppl->iPlayer].rgEnvVar[i] = plrSav.rgEnvVar[i];
        app.state.rgplr[lppl->iPlayer].rgEnvVarMin[i] = plrSav.rgEnvVarMin[i];
        app.state.rgplr[lppl->iPlayer].rgEnvVarMax[i] = plrSav.rgEnvVarMax[i];
    }
    iBest = ColonyIO().pfnBestTerraform(app, lppl, fHelp);
    app.state.rgplr[lppl->iPlayer] = plrSav;
    return iBest;
}
