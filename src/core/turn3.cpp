#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "turn_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "race_query_io.h"

void SatisfyOrders(APPSESSION &app, std::int16_t iPass) {
    std::int16_t        fMining;
    std::int32_t        amountWP;
    XferActionType action;
    PLANET         pl;
    std::int32_t        l2;
    std::int16_t        j;
    std::int32_t        amount;
    std::int16_t        iflWP;
    std::int16_t        fSentBadFleetXfer;
    std::int16_t        ifltcur;
    FLEET         *lpfl;
    std::int16_t        fAtPlanet;
    MessageId      idm;
    std::int16_t        iLoad;
    std::uint16_t       xWP;
    std::int16_t        fOptFuel;
    std::int16_t        fStealing;
    FLEET         *lpflWP;
    std::int16_t        fHasPermission;
    std::int16_t        fFulfilled;
    ORDER          ord;
    std::int16_t        fFueling;
    std::int32_t        wtOptimalFuel;
    std::int16_t        fDunnage;
    std::int32_t        amountEdit;
    std::int32_t        l;
    std::int16_t        fDone;
    std::uint16_t       idWP;
    THING         *lpthWP;
    std::int32_t        cFuel2;
    std::int32_t        iExcess;
    std::int32_t        lMaxFuel;
    std::int32_t        wtFuelOrig;
    std::int32_t        lT;
    std::uint16_t       iGoto;
    SHDEF         *lpshdefT;
    std::int32_t        lXferMinerals;
    std::int16_t        i;
    std::int32_t        lAmt;
    SHDEF          shdefT;
    std::int16_t        csh;
    std::int16_t        fBleeding;
    std::int32_t        lResUltimate;
    std::int16_t        fUltimate;
    std::int16_t        fColonize;
    std::int32_t        rgwt[3];
    std::int32_t        cMine;
    PLANET        *lppl;
    std::int32_t        rglQuan[4];
    FLEET         *lpflDest;
    std::int16_t        ishLastFree;
    std::int16_t        rgishMap[16];
    std::int16_t        ishMatch;
    std::int16_t        ish;
    FLEET         *lpflNew;
    std::int16_t        iplrDest;
    SHDEF         *lpshdefDest;
    THING         *lpthMac;
    std::int32_t        dy;
    THING         *lpthBest;
    THING         *lpth;
    std::int32_t        lBest;
    std::int32_t        dx;

    if (app.state.cFleet > 0) {
        for (ifltcur = 0; ifltcur < app.state.cFleet; ifltcur++) {
            lpfl = app.state.rglpfl[ifltcur];
            if (!app.state.rglpfl[ifltcur])
                break;
            if (iPass == 1) {
                lpfl->SetFCompChg(0);
                lpfl->SetFTargeted(0);
            }
            if (!lpfl->fDead && !lpfl->fSkipped()) {
                ord = lpfl->lpplord->rgord()[0];
                if (ord.grTask == grTaskXfer) {
                    fFulfilled = 1;
                    fDone = 1;
                    fSentBadFleetXfer = 0;
                    fHasPermission = 1;
                    fMining = 0;
                    fFueling = 0;
                    fStealing = 0;
                    fDunnage = 0;
                    lpflWP = nullptr;
                    lpthWP = nullptr;
                    fAtPlanet = lpfl->idPlanet != idPlanetDeepSpace && PlanetQueryIO().pfnLookup(app, lpfl->idPlanet, &pl) != 0;
                    idWP = ord.id;
                    switch (ord.grobj) {
                    case grobjFleet:
                        lpflWP = LpflFromId(app, ord.id);
                        idWP |= 0x8000;
                        xWP = 0xffff;
                        fHasPermission = lpfl->iPlayer == lpflWP->iPlayer;
                        if (!fHasPermission) {
                            GetFleetScannerRange(app, lpfl, nullptr, nullptr, &fHasPermission);
                            if (!fHasPermission)
                                break;
                            fStealing = 1;
                            break;
                        }
                        if (!lpflWP->fHereAllTurn || !fAtPlanet)
                            break;
                        if (ColonyIO().pfnFleetMining(app, lpflWP) > 0 && (pl.iPlayer == iplrNone || pl.iPlayer == lpfl->iPlayer)) {
                            fMining = 1;
                            xWP = 0xffff;
                            idWP = lpflWP->id | 0x8000;
                            break;
                        }
                        if (ProductionIO().pfnFleetStat(app, lpflWP, 2) != 0 || pl.iPlayer != lpfl->iPlayer)
                            break;
                        fFueling = 1;
                        xWP = 0xffff;
                        idWP = lpflWP->id | 0x8000;
                        break;
                    case grobjThing:
                        lpthWP = LpthFromId(app, ord.id);
                        xWP = 0xfffe;
                        break;
                    case grobjPlanet:
                        xWP = 0xffff;
                        fHasPermission = lpfl->iPlayer == pl.iPlayer ? 3 : 0;
                        if (fHasPermission)
                            break;
                        GetFleetScannerRange(app, lpfl, nullptr, nullptr, &fHasPermission);
                        if (fHasPermission == 1) {
                            fHasPermission = 0;
                        }
                        if (fHasPermission) {
                            fStealing = 1;
                        }
                        if (fHasPermission || pl.iPlayer != iplrNone)
                            break;
                        for (iflWP = 0; iflWP < app.state.cFleet; iflWP++) {
                            lpflWP = app.state.rglpfl[iflWP];
                            if (!app.state.rglpfl[iflWP])
                                break;
                            if (!lpflWP->fDead && lpflWP->iPlayer == lpfl->iPlayer && lpfl->idPlanet == lpflWP->idPlanet && lpflWP != lpfl &&
                                lpflWP->fHereAllTurn && ColonyIO().pfnFleetMining(app, lpflWP) > 0) {
                                fMining = 2;
                                fHasPermission = 1;
                                ord.grobj = grobjFleet;
                                ord.id = lpflWP->id;
                                xWP = 0xffff;
                                idWP = lpflWP->id | 0x8000;
                                break;
                            }
                        }
                        break;
                    case grobjOther:
                        xWP = ord.pt.x;
                        idWP = ord.pt.y;
                    }
                    iLoad = (iPass - 1) & 1;
                    fOptFuel = 0;
                    if (ord.grobj == grobjThing && (!lpthWP || lpthWP->ith() != ithMineralPacket)) {
                        if (lpthWP) {
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHadOrdersTransferCargoFutilePursuit, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpthWP->ith());
                        }
                        goto CancelOrder;
                    }
                LTryDunnage:
                    for (j = 0; j < 5; j++) {
                        action = static_cast<XferActionType>(ord.txp.rgia[j].iAction());
                        if (action != iActionNone && (fDunnage != 2 || action == iActionLoadDunnage) &&
                            (j != 4 || ord.grobj == grobjFleet || ord.grobj == grobjOther) && (j < 3 || ord.grobj != grobjThing)) {
                            amountWP = 0;
                            switch (ord.grobj) {
                            case grobjFleet:
                                if ((!fMining && !fFueling) || j == 4) {
                                    amountWP = lpflWP->rgwtMin[j];
                                    break;
                                }
                                /* fallthrough */
                            case grobjPlanet:
                                if (j >= 4)
                                    break;
                                amountWP = pl.rgwtMin[j];
                                break;
                            case grobjThing:
                                if (j < 3) {
                                    amountWP = lpthWP->thp().rgwtMin(j);
                                }
                            }
                            switch (action) {
                            default:
                                break;
                            case iActionFillPercent:
                            case iActionWaitPercent:
                                if (iLoad == 0)
                                    continue;
                                if (j == 4) {
                                    amount = ProductionIO().pfnFleetStat(app, lpfl, 1);
                                } else {
                                    amount = ProductionIO().pfnFleetStat(app, lpfl, 2);
                                }
                                amount = StarsMin(2000000, amount);
                                if (amount < 65536) {
                                    amountEdit = static_cast<std::uint32_t>((static_cast<std::uint32_t>((amount * ord.txp.rgia[j].cQuan())) / 100));
                                } else {
                                    amountEdit = static_cast<std::uint32_t>((static_cast<std::int32_t>((amount / 100)) * ord.txp.rgia[j].cQuan()));
                                }
                                amountEdit -= lpfl->rgwtMin[j];
                                if (amountEdit < 0) {
                                    amountEdit = 0;
                                }
                                break;
                            case iActionLoadExact:
                                if (iLoad == 0)
                                    continue;
                                amountEdit = ord.txp.rgia[j].cQuan();
                                break;
                            case iActionUnloadExact:
                                if (iLoad != 0)
                                    continue;
                                /* fallthrough */
                            case iActionSetAmount:
                            case iActionSetWaypoint:
                                amountEdit = ord.txp.rgia[j].cQuan();
                                break;
                            case iActionLoadAll:
                                if (iLoad == 0)
                                    continue;
                                /* fallthrough */
                            case iActionLoadDunnage:
                                amountEdit = amountWP;
                                break;
                            case iActionUnloadAll:
                                if (iLoad != 0)
                                    continue;
                                amountEdit = lpfl->rgwtMin[j];
                                break;
                            }
                            switch (action) {
                            default:
                                break;
                            case iActionSetWaypoint:
                                amount = amountWP - amountEdit;
                                if (amount < 0) {
                                    if (iLoad != 0)
                                        continue;
                                    amount = -amount;
                                    amount = StarsMin(amount, lpfl->rgwtMin[j]);
                                    goto Unload;
                                }
                                if (iLoad == 0)
                                    continue;
                                goto Load;
                            case iActionSetAmount:
                                amount = amountEdit - lpfl->rgwtMin[j];
                                if (amount >= 0) {
                                    if (iLoad == 0)
                                        continue;
                                    if (amount <= amountWP)
                                        goto Load;
                                    fDone = 0;
                                    if (iPass != 4)
                                        goto Load;
                                    idm = j == 3 ? idmAttemptedSetNumberBoardUnfortunatelyCouldntProvi : idmAttemptedSetAmountBoardUnfortunatelyCouldntProvi;
                                    TurnIO().pfnMessage(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, j, ord.txp.rgia[j].cQuan(), 0, xWP, idWP, 0);
                                    goto Load;
                                }
                                if (iLoad != 0)
                                    continue;
                                amount = -amount;
                                goto Unload;
                            case iActionLoadDunnage:
                                if (j == 4) {
                                    wtOptimalFuel = 0;
                                    fOptFuel = 1;
                                    continue;
                                }
                                if (iLoad == 0)
                                    continue;
                                if (fDunnage < 2) {
                                    fDunnage = 1;
                                    continue;
                                }
                                amount = amountWP;
                                if (amount != 0)
                                    goto Load;
                                continue;
                            case iActionLoadAll:
                            case iActionLoadExact:
                            case iActionFillPercent:
                            case iActionWaitPercent:
                                amount = amountEdit;
                            Load:
                                if (iLoad == 0)
                                    continue;
                                if (amount == 0)
                                    continue;
                                if (j == 4) {
                                    l = GetFuelFree(app, lpfl);
                                } else {
                                    l = GetCargoFree(app, lpfl);
                                }
                                amount = StarsMin(l, amount);
                                if (!fHasPermission || ord.grobj == grobjOther) {
                                    if (j == 4 && fOptFuel) {
                                        amount = 0;
                                        continue;
                                    }
                                    if (iPass == 4) {
                                        switch (ord.grobj) {
                                        case grobjPlanet:
                                            idm = idmAttemptedLoadPlanetDontControlOrderHas;
                                            break;
                                        case grobjFleet:
                                            idm = idmAttemptedLoadFleetDontControlOrderHas;
                                            break;
                                        case grobjOther:
                                            idm = idmAttemptedLoadDeepSpaceAttemptUnsuccessful;
                                        }
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, j);
                                        goto CancelOrder;
                                    }
                                    fDone = 0;
                                    continue;
                                }
                                if (fStealing && (j == 3 || j == 4)) {
                                    /* A thief can't take colonists or fuel. The original marked
                                       the item done but loaded it anyway; its unused messages
                                       report the attempt as unsuccessful. */
                                    TurnIO().pfnMessage(app, lpfl->iPlayer,
                                                j == 3 ? idmAttemptedShanghaiColonistsAttemptUnsuccessful : idmAttemptedStealMgFuelAttemptUnsuccessful,
                                                static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(amount), HIWORD(amount), xWP, idWP, 0, 0);
                                    fDone = 1;
                                    continue;
                                }
                                if (amount != 0) {
                                    l = StarsMin(amount, amountWP);
                                    l2 = ChgCargo(app, static_cast<GrobjClass>(ord.grobj), ord.id, static_cast<MineralType>(j), -l, nullptr);
                                    if (l2 != 0) {
                                        l = ChgCargo(app, grobjFleet, lpfl->id, static_cast<MineralType>(j), -l2, nullptr);
                                        if (l != 0) {
                                            if (ord.grobj == grobjFleet && lpfl->iPlayer != lpflWP->iPlayer) {
                                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasStolen, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l), HIWORD(l), j, idWP & 0x7fff, 0,
                                                            0);
                                            } else {
                                                TurnIO().pfnMessage(app, lpfl->iPlayer, j == 3 ? idmHasBeamed : idmHasLoaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l),
                                                            HIWORD(l), j, xWP, idWP, 0);
                                            }
                                        }
                                    }
                                    if ((fFueling || fMining) && amount != -l2) {
                                        l = amount + l2;
                                        l2 = ChgCargo(app, grobjPlanet, pl.id, static_cast<MineralType>(j), -l, nullptr);
                                        if (l2 != 0) {
                                            l = ChgCargo(app, grobjFleet, lpfl->id, static_cast<MineralType>(j), -l2, nullptr);
                                            if (fMining) {
                                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasLoadedMiningRobotsWorking, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l), HIWORD(l),
                                                            j, lpflWP->id, pl.id, 0);
                                            } else {
                                                TurnIO().pfnMessage(app, lpfl->iPlayer, j == 3 ? idmHasBeamed : idmHasLoaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l),
                                                            HIWORD(l), j, xWP, pl.id, 0);
                                            }
                                        } else {
                                            l = 0;
                                        }
                                    }
                                    if (amount != l && action == iActionWaitPercent) {
                                        fFulfilled = 0;
                                    }
                                    if (l != 0 && action == iActionLoadDunnage && j == 4) {
                                        wtOptimalFuel = l;
                                    }
                                    continue;
                                } else if (action == iActionWaitPercent) {
                                    if (j == 4) {
                                        fDone = 0;
                                        continue;
                                    }
                                    fFulfilled = 0;
                                }
                                continue;
                            case iActionUnloadAll:
                            case iActionUnloadExact:
                                amount = (((static_cast<std::uint32_t>(lpfl->rgwtMin[j])) < (static_cast<std::uint32_t>(amountEdit))) ? (static_cast<std::uint32_t>(lpfl->rgwtMin[j])) : (static_cast<std::uint32_t>(amountEdit)));
                            Unload:
                                if (iLoad != 0)
                                    continue;
                                if (j == 3 && ord.grobj == grobjPlanet && pl.iPlayer != lpfl->iPlayer) {
                                    if (pl.iPlayer == iplrNone && !pl.fWasInhabited) {
                                        idm = idmHasTriedBeamColonistsPlanetUninhabitedMust;
                                    LCantDrop:
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, pl.id);
                                        goto CancelOrder;
                                    }
                                    if (RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh) {
                                        idm = idmCaptainHasAttemptedBeamColonistsOverruledBridge;
                                        goto LCantDrop;
                                    }
                                    if (pl.fStarbase) {
                                        idm = idmHasTriedBeamColonistsPlanetsStarbaseWould;
                                        goto LCantDrop;
                                    }
                                    FQueueColonistDrop(app, lpfl, &pl, amount);
                                } else if (j == 3 && ord.grobj == grobjFleet && lpfl->iPlayer != (static_cast<std::uint16_t>(ord.id) >> 9 & 0xf)) {
                                    idm = idmAllowedTransferColonistsAnotherPlayer;
                                    goto LCantDrop;
                                } else if (ord.grobj == grobjFleet && app.state.rgplr[lpflWP->iPlayer].rgmdRelation[lpfl->iPlayer] == 2) {
                                    amount = 0;
                                } else if (j == 3 && ord.grobj == grobjOther) {
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasTriedBeamColonistsDeepSpaceOrder, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                    goto CancelOrder;
                                } else if (amount != 0) {
                                    if (fFueling && j != 4) {
                                        l = ChgCargo(app, grobjPlanet, pl.id, static_cast<MineralType>(j), amount, nullptr);
                                        TurnIO().pfnMessage(app, lpfl->iPlayer, j == 3 ? idmHasBeamed2 : idmHasUnloaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l), HIWORD(l),
                                                    j, xWP, pl.id, 0);
                                    } else {
                                        l = ChgCargo(app, static_cast<GrobjClass>(ord.grobj), ord.id, static_cast<MineralType>(j), amount, nullptr);
                                        if (l > 0) {
                                            TurnIO().pfnMessage(app, lpfl->iPlayer, j == 3 ? idmHasBeamed2 : idmHasUnloaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l),
                                                        HIWORD(l), j, xWP, idWP, 0);
                                        }
                                        amount = l;
                                    }
                                }
                                if (amount != 0) {
                                    l = ChgCargo(app, grobjFleet, lpfl->id, static_cast<MineralType>(j), -amount, nullptr);
                                }
                                ord.txp.rgia[j].SetIAction(iActionNone);
                                continue;
                            }
                        }
                    }
                    if (fOptFuel && iLoad != 0 && fDunnage != 1) {
                        if (lpfl->cord <= 1) {
                            amount = 0;
                            iExcess = lpfl->rgwtMin[4];
                            goto SetOptAmount;
                        }
                        amount = EstFuelUse(app, lpfl, 0, -1, -1, 0);
                        if (amount > lpfl->rgwtMin[4]) {
                            fDone = 0;
                            if (wtOptimalFuel != 0) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasLoaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(wtOptimalFuel), HIWORD(wtOptimalFuel), 4, xWP,
                                            idWP, 0);
                            }
                            if (iPass == 4 && !fHasPermission) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmFailedLoadFuel, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, xWP, idWP, 0, 0, 0, 0);
                                goto FinishFleet;
                            }
                            if (iPass != 2)
                                goto FinishFleet;
                            lMaxFuel = ProductionIO().pfnFleetStat(app, lpfl, 1);
                            if (lMaxFuel < amount) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmWillNeverMakeWaypointFuelCapacityMg, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(lMaxFuel),
                                            HIWORD(lMaxFuel), LOWORD(amount), HIWORD(amount), 0, 0);
                                goto FinishFleet;
                            }
                            cFuel2 = amount - lpfl->rgwtMin[4];
                            if (fMining == 2) {
                                idWP = pl.id;
                            }
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmThereIsntEnoughFuelAvailableAllowGet, static_cast<MsgGoto>(lpfl->id | 0x8000), xWP, idWP, lpfl->id, LOWORD(cFuel2),
                                        HIWORD(cFuel2), 0, 0);
                            goto FinishFleet;
                        }
                        if (amount < lpfl->rgwtMin[4]) {
                            wtFuelOrig = lpfl->rgwtMin[4];
                            do {
                                lpfl->rgwtMin[4] = amount;
                                amount = EstFuelUse(app, lpfl, 0, -1, -1, 0);
                            } while (amount < lpfl->rgwtMin[4]);
                            iExcess = wtFuelOrig - amount;
                            lpfl->rgwtMin[4] = wtFuelOrig;
                        SetOptAmount:
                            if (iExcess != 0) {
                                l2 = ChgCargo(app, static_cast<GrobjClass>(ord.grobj), ord.id, Fuel, iExcess, nullptr);
                                if (l2 != 0) {
                                    l = ChgCargo(app, grobjFleet, lpfl->id, Fuel, -l2, nullptr);
                                }
                            } else {
                                l = 0;
                            }
                            if (l2 != 0) {
                                l += wtOptimalFuel;
                                if (l > 0) {
                                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasLoaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(l), HIWORD(l), 4, xWP, idWP, 0);
                                } else if (l < 0) {
                                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasUnloaded, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, -LOWORD(l),
                                                LOWORD(static_cast<std::uint32_t>((static_cast<std::uint32_t>(-l) >> 0x10))), 4, xWP, idWP, 0);
                                }
                            }
                        }
                        if (fDone && fFulfilled) {
                            ord.txp.rgia[4].SetIAction(iActionNone);
                            ord.txp.rgia[4].SetCQuan(0);
                        }
                    }
                FinishFleet:
                    if (!fFulfilled && GetCargoFree(app, lpfl) > 0) {
                        fDone = 0;
                    }
                    if (fDone && fDunnage == 1 && (GetCargoFree(app, lpfl) > 0 || fOptFuel)) {
                        fDunnage = 2;
                        goto LTryDunnage;
                    }
                    if (fDone && iLoad != 0)
                        goto CancelOrder;
                    if (fMining == 2) {
                        ord.id = pl.id;
                        ord.grobj = grobjPlanet;
                    }
                    lpfl->lpplord->rgord()[0] = ord;
                    continue;
                } else if ((ord.grTask == grTaskScrap && iPass == 1) || ord.grTask == grTaskColonize) {
                    if (lpfl->idPlanet == idPlanetDeepSpace) {
                        if (ord.grTask == grTaskColonize) {
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasOrderColonizeCurrentlyOrbitPlanetOrder, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            goto CancelOrder;
                        }
                        pl.id = idplNone;
                        pl.iPlayer = iplrNone;
                        pl.fStarbase = 0;
                        goto LScrap;
                    }
                    if (!PlanetQueryIO().pfnLookup(app, lpfl->idPlanet, &pl) && ord.grTask == grTaskColonize)
                        goto CancelOrder;
                LScrap:
                    if (ord.grTask == grTaskColonize) {
                        if (pl.iPlayer != iplrNone) {
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasOrdersColonizeAlreadyPopulatedColonizeOrder, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, pl.id, pl.id, 0, 0, 0,
                                        0);
                            goto CancelOrder;
                        }
                        if (lpfl->rgwtMin[3] == 0) {
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasOrdersColonizeHaveFailedBringAlong, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, pl.id);
                            goto CancelOrder;
                        }
                    }
                    for (i = 0; i < 3; i++) {
                        rgwt[i] = 0;
                    }
                    fColonize = 0;
                    csh = 0;
                    std::ranges::fill(app.state.rgTechBattle, 0);
                    std::ranges::fill(app.state.rgTechTrader, 0);
                    for (i = 0; i < 16; i++) {
                        if (lpfl->rgcsh[i] > 0) {
                            csh += lpfl->rgcsh[i];
                            MarkTechsSeen(app, &app.state.rglpshdef[lpfl->iPlayer][i].hul, lpfl->iPlayer);
                            for (j = 0; j < app.state.rglpshdef[lpfl->iPlayer][i].hul.chs; j++) {
                                /* cItem: the original accepted a slot that once held a module. */
                                if (app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst == hstSpecialM && app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].cItem > 0 &&
                                    (app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                                     app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                                    fColonize = 1;
                                }
                            }
                        }
                    }
                    if (ord.grTask == grTaskColonize && !fColonize) {
                        TurnIO().pfnMessage(app, lpfl->iPlayer, idmHasOrdersColonizeNoneShipsHaveColonization, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, pl.id, lpfl->id, 0, 0, 0, 0);
                    } else {
                        lXferMinerals = 0;
                        fUltimate = ord.grTask == grTaskScrap && pl.iPlayer != iplrNone && RaceQueryIO().pfnGrbit(&app.state.rgplr[pl.iPlayer], ibitRaceUltimateRecycling) != 0;
                        fBleeding = RaceQueryIO().pfnGrbit(&app.state.rgplr[lpfl->iPlayer], ibitRaceBleedingEdgeTech);
                        app.state.gd.fDontCalcBleed = 1;
                        app.state.idPlayer = lpfl->iPlayer;
                        for (i = 0; i <= 2; i++) {
                            lAmt = 0;
                            for (j = 0; j < 16; j++) {
                                if (lpfl->rgcsh[j] > 0) {
                                    if (fBleeding) {
                                        shdefT = app.state.rglpshdef[lpfl->iPlayer][j];
                                        UpdateShdefCost(app, &shdefT);
                                        lpshdefT = &shdefT;
                                    } else {
                                        lpshdefT = app.state.rglpshdef[lpfl->iPlayer] + j;
                                    }
                                    lT = static_cast<std::uint32_t>((lpfl->rgcsh[j] * static_cast<std::uint32_t>(lpshdefT->hul.rgwtOreCost[i])));
                                    if (lpshdefT->fGift) {
                                        lT = static_cast<std::int32_t>((lT / 4));
                                    }
                                    lAmt += lT;
                                }
                            }
                            if (ord.grTask == grTaskColonize) {
                                lAmt = static_cast<std::int32_t>((lAmt * 3)) / 4;
                            } else if (pl.id == idplNone) {
                                lAmt = static_cast<std::int32_t>((lAmt / 3));
                            } else if (pl.fStarbase) {
                                if (fUltimate) {
                                    lAmt = static_cast<std::int32_t>((lAmt * 9)) / 10;
                                } else {
                                    lAmt = static_cast<std::int32_t>((lAmt * 4)) / 5;
                                }
                            } else if (fUltimate) {
                                lAmt = static_cast<std::int32_t>((lAmt * 9)) / 20;
                            } else {
                                lAmt = static_cast<std::int32_t>((lAmt / 3));
                            }
                            lAmt += lpfl->rgwtMin[i];
                            lXferMinerals += lAmt;
                            if (pl.id == idplNone) {
                                pl.rgwtMin[i] = lAmt;
                            } else {
                                app.state.lpPlanets[pl.id].rgwtMin[i] = app.state.lpPlanets[pl.id].rgwtMin[i] + lAmt;
                            }
                        }
                        iGoto = pl.id;
                        if (fUltimate) {
                            lResUltimate = 0;
                            for (j = 0; j < 16; j++) {
                                if (lpfl->rgcsh[j] > 0) {
                                    if (fBleeding) {
                                        shdefT = app.state.rglpshdef[lpfl->iPlayer][j];
                                        UpdateShdefCost(app, &shdefT);
                                        lpshdefT = &shdefT;
                                    } else {
                                        lpshdefT = app.state.rglpshdef[lpfl->iPlayer] + j;
                                    }
                                    lT = static_cast<std::uint32_t>((lpfl->rgcsh[j] * static_cast<std::uint32_t>(lpshdefT->hul.resCost)));
                                    if (lpshdefT->fGift) {
                                        lT = static_cast<std::int32_t>((lT / 4));
                                    }
                                    lResUltimate += lT;
                                }
                            }
                            if (lResUltimate > 65535) {
                                lResUltimate = 65535;
                            }
                            app.state.vrgPlanResExtra[pl.id] = app.state.vrgPlanResExtra[pl.id] + LOWORD(lResUltimate);
                            if (lResUltimate > 0 && pl.iPlayer != iplrNone) {
                                lAmt = ResearchIO().pfnResources(app, &pl, pl.iPlayer);
                                lResUltimate = static_cast<std::int32_t>((static_cast<std::int32_t>((lResUltimate * lAmt)) / (lResUltimate + lAmt)));
                            }
                            idm = static_cast<MessageId>(idmHasDismantledKtMineralsWhichHaveDeposited2 + pl.fStarbase);
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(iGoto), WFromLpfl(app, lpfl), LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, LOWORD(lResUltimate),
                                        HIWORD(lResUltimate), 0);
                            idm = static_cast<MessageId>(idmHasDismantledKtMineralsWhichHaveDeposited5 + pl.fStarbase);
                            if (pl.fStarbase) {
                                i = ITechLearnATech(app, pl.iPlayer, 0, 0, static_cast<MessageId>(idmNone), &iGoto);
                                if (i != 0) {
                                    idm = idmHasDismantledKtMineralsWhichHaveDeposited3;
                                    if (i < 0) {
                                        i = -(i + 1);
                                    } else {
                                        i--;
                                        idm = static_cast<MessageId>(idm + 1);
                                    }
                                } else {
                                    iGoto = pl.id;
                                }
                            } else {
                                iGoto = pl.id;
                            }
                            TurnIO().pfnMessage(app, pl.iPlayer, idm, static_cast<MsgGoto>(iGoto), lpfl->id, LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, LOWORD(lResUltimate),
                                        HIWORD(lResUltimate), i);
                        } else if (pl.id == idplNone) {
                            lpthWP = nullptr;
                            DropSalvage(app, &lpthWP, pl.rgwtMin, lpfl->iplr(), &ord.pt);
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasDismantledScrapLeftDeepSpace, gotoThing, lpthWP->idFull(), WFromLpfl(app, lpfl));
                        } else {
                            idm = static_cast<MessageId>(idmHasDismantledKtMineralsWhichHaveDeposited + pl.fStarbase);
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(iGoto), WFromLpfl(app, lpfl), LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, 0, 0, 0);
                            idm = static_cast<MessageId>(idmHasDismantledKtMineralsWhichHaveDeposited4 + pl.fStarbase);
                            if (pl.fStarbase) {
                                i = ITechLearnATech(app, pl.iPlayer, 0, 0, static_cast<MessageId>(idmNone), &iGoto);
                                if (i != 0) {
                                    idm = idmHasDismantledKtMineralsStarbaseOrbitingProcess;
                                    if (i < 0) {
                                        i = -(i + 1);
                                    } else {
                                        i--;
                                        idm = static_cast<MessageId>(idm + 1);
                                    }
                                } else {
                                    iGoto = pl.id;
                                }
                            }
                            TurnIO().pfnMessage(app, pl.iPlayer, idm, static_cast<MsgGoto>(iGoto), lpfl->id, LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, i, 0, 0);
                        }
                        app.state.idPlayer = iplrNone;
                        app.state.gd.fDontCalcBleed = 0;
                        TurnIO().pfnRemoveMessage(app, lpfl->iPlayer, idmHasCompletedAssignedOrders, static_cast<MsgGoto>(lpfl->id | 0x8000));
                        lpfl->fDead = 1;
                        if (ord.grTask == grTaskColonize) {
                            FQueueColonistDrop(app, lpfl, &pl, lpfl->rgwtMin[3]);
                        } else if (lpfl->idPlanet != idPlanetDeepSpace && app.state.lpPlanets[lpfl->idPlanet].iPlayer == lpfl->iPlayer) {
                            app.state.lpPlanets[lpfl->idPlanet].rgwtMin[3] = app.state.lpPlanets[lpfl->idPlanet].rgwtMin[3] + lpfl->rgwtMin[3];
                        }
                    }
                CancelOrder:
                    if (lpfl->cord == 1 && !lpfl->fDead && lpfl->lpplord->rgord()[0].grTask != grTaskNone) {
                        TurnIO().pfnRemoveMessage(app, lpfl->iPlayer, idmHasCompletedAssignedOrders, static_cast<MsgGoto>(lpfl->id | 0x8000));
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasCompletedAssignedOrders, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                    }
                    lpfl->lpplord->rgord()[0].grTask = grTaskNone;
                } else if (ord.grTask == grTaskMine) {
                    cMine = 0;
                    if (iPass != 3 || !lpfl->fHereAllTurn)
                        continue;
                    if (lpfl->idPlanet == idPlanetDeepSpace) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmRemoteMiningRobotsHadOrdersMineDeep, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                        goto CancelOrder;
                    }
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (!lppl)
                        goto CancelOrder;
                    cMine = ColonyIO().pfnFleetMining(app, lpfl);
                    if (cMine == 0) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHadOrdersMineFleetDoesntHaveAny, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id);
                        goto CancelOrder;
                    }
                    if (lppl->iPlayer != iplrNone) {
                        if (RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh)
                            continue;
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmRemoteMiningRobotsHadOrdersMinePlanet, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id);
                        goto CancelOrder;
                    }
                    ColonyIO().pfnEstimateMining(app, lppl, rglQuan, cMine, 1);
                } else if (ord.grTask == grTaskAutoRoute) {
                    if (lpfl->cord != 1 || lpfl->idPlanet == idPlanetDeepSpace)
                        continue;
                    lppl = LpplFromId(app, lpfl->idPlanet);
                    if (lppl->iPlayer == lpfl->iPlayer && lppl->idRoute != 0) {
                        ProductionIO().pfnRoute(app, lpfl, lppl);
                        TurnIO().pfnMessage(app, lpfl->iPlayer, lpfl->lpplord->rgord()[1].iWarp == 0 ? idmHasReroutedUnfortuentlyDoesHaveEnoughFuel : idmHasRerouted,
                                    static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpfl->idPlanet, lppl->idRoute - 1, 0, 0, 0, 0);
                        continue;
                    }
                    if (iPass != 4)
                        continue;
                    ProductionIO().pfnDefaultOrder(app, lpfl, lppl);
                    ord = lpfl->lpplord->rgord()[0];
                    if (ord.grTask == grTaskMerge)
                        goto LDoMerge;
                } else if (ord.grTask == grTaskMerge) {
                    if (!(iPass & 1)) {
                    LDoMerge:
                        if (ord.grobj != grobjFleet) {
                        NMNF:
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmUnableCompleteMergeOrdersWaypointDestinationWasn, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            goto CancelOrder;
                        }
                        lpflDest = LpflFromId(app, ord.id);
                        if (!lpflDest || lpflDest->fDead)
                            goto NMNF;
                        if (lpfl == lpflDest)
                            goto CancelOrder;
                        if (lpflDest->iPlayer != lpfl->iPlayer) {
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmUnableCompleteMergeOrdersDestinationFleetWasnt, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            goto CancelOrder;
                        }
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasMerged, static_cast<MsgGoto>(lpflDest->id | 0x8000), WFromLpfl(app, lpfl), lpflDest->id);
                        TurnIO().pfnRemoveMessage(app, lpfl->iPlayer, idmHasCompletedAssignedOrders, static_cast<MsgGoto>(lpfl->id | 0x8000));
                        Merge2Fleets(app, lpflDest, lpfl, 1);
                        goto CancelOrder;
                    }
                } else if (ord.grTask == grTaskGive) {
                    if (iPass != 4)
                        continue;
                    iplrDest = ord.tsell().iPlrX;
                    if (iplrDest >= lpfl->iPlayer) {
                        iplrDest++;
                    }
                    if (iplrDest < 0 || iplrDest >= app.state.game.cPlayer || app.state.rgplr[iplrDest].fDead) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmCouldntGiveAwayBecausePlayerDead, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                        goto CancelOrder;
                    }
                    if (app.state.rgplr[iplrDest].fAi || app.state.rgplr[iplrDest].rgmdRelation[lpfl->iPlayer] == 2) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmSnubAttemptedGiftRefuseFleet, static_cast<MsgGoto>(lpfl->id | 0x8000), 0x30 | iplrDest, 0);
                        goto CancelOrder;
                    }
                    if (lpfl->rgwtMin[3] > 0) {
                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmCouldntGiveAwayBecauseThereColonistsBoard, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                        goto CancelOrder;
                    }
                    ishLastFree = -1;
                    for (ish = 0; ish < 16; ish++) {
                        if (lpfl->rgcsh[ish] == 0) {
                            rgishMap[ish] = ishdefNone;
                        } else {
                            ishMatch = IshFindSimilarDesign(app, &app.state.rglpshdef[lpfl->iPlayer][ish].hul, iplrDest);
                            if (ishMatch != ishdefNone) {
                                rgishMap[ish] = ishMatch;
                            } else {
                                do {
                                    ishLastFree++;
                                } while (ishLastFree < 16 && !app.state.rglpshdef[iplrDest][ishLastFree].fFree);
                                if (ishLastFree >= 16) {
                                SellNoCap:
                                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmCouldntGiveAwayBecauseDidntHaveAdministrative, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0x30 | iplrDest);
                                    TurnIO().pfnMessage2(app, iplrDest, idmAttemptedGiveFleetDontHaveEnoughExcess, gotoNone, 0x30 | lpfl->iPlayer, 0);
                                    goto CancelOrder;
                                }
                                rgishMap[ish] = ishLastFree;
                            }
                        }
                    }
                    if (app.state.rgplr[iplrDest].cFleet >= 0x200)
                        goto SellNoCap;
                    lpflNew = ProductionIO().pfnNewFleet(app, iplrDest, lpfl->idPlanet);
                    if (!lpflNew)
                        goto SellNoCap;
                    if (iplrDest < lpfl->iPlayer) {
                        ifltcur++;
                    }
                    lpflNew->pt = lpfl->pt;
                    lpflNew->lpplord->rgord()[0].pt = lpfl->pt;
                    for (ish = 0; ish <= 4; ish++) {
                        lpflNew->rgwtMin[ish] = lpfl->rgwtMin[ish];
                    }
                    for (ish = 0; ish < 16; ish++) {
                        if (lpfl->rgcsh[ish] > 0) {
                            lpshdefDest = app.state.rglpshdef[iplrDest] + rgishMap[ish];
                            if (lpshdefDest->fFree) {
                                *lpshdefDest = app.state.rglpshdef[lpfl->iPlayer][ish];
                                lpshdefDest->ishdef = rgishMap[ish];
                                lpshdefDest->fGift = 1;
                                lpshdefDest->cBuilt = 0;
                                lpshdefDest->cExist = 0;
                                app.state.rgplr[iplrDest].cShDef++;
                            }
                            lpflNew->rgcsh[rgishMap[ish]] = lpflNew->rgcsh[rgishMap[ish]] + lpfl->rgcsh[ish];
                            lpflNew->rgdv[rgishMap[ish]].dp = lpfl->rgdv[ish].dp;
                            lpshdefDest->cBuilt += lpfl->rgcsh[ish];
                            lpshdefDest->cExist += lpfl->rgcsh[ish];
                        }
                    }
                    lpfl->fDead = 1;
                    TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasSuccessfullyGiven, static_cast<MsgGoto>(lpflNew->id | 0x8000), WFromLpfl(app, lpfl), 0x30 | iplrDest);
                    TurnIO().pfnMessage2(app, iplrDest, idmHaveGiven, static_cast<MsgGoto>(lpflNew->id | 0x8000), 0x30 | lpfl->iPlayer, WFromLpfl(app, lpflNew));
                    lpfl->fDead = 1;
                } else if (ord.grTask == grTaskLayMines ||
                           (ord.grTask == grTaskNone && lpfl->cord > 1 && RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raMines &&
                            lpfl->lpplord->rgord()[1].grTask == grTaskLayMines)) {
                    idm = idmHasDispersedMines;
                    if (iPass == 3 && (lpfl->fHereAllTurn || RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raMines)) {
                        cMine = CLayMinesFromLpfl(app, lpfl, mineAll, ishdefAll);
                        if (cMine == 0) {
                            TurnIO().pfnMessage2(app, lpfl->iPlayer, idmHasAttemptedLayMinesOrderHasCanceled, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            goto CancelOrder;
                        }
                        if (ord.grTask == grTaskLayMines) {
                            if (lpfl->lpplord->rgord()[0].tsell().iPlrX == 0) {
                                lpfl->lpplord->rgord()[0].grTask = grTaskNone;
                            } else if (lpfl->lpplord->rgord()[0].tsell().iPlrX != 5) {
                                lpfl->lpplord->rgord()[0].tsell().iPlrX--;
                            }
                        }
                        for (j = 0; j < 3; j++) {
                            cMine = CLayMinesFromLpfl(app, lpfl, static_cast<MineFieldType>(j), ishdefAll);
                            if (cMine != 0) {
                                if (!lpfl->fHereAllTurn) {
                                    cMine = static_cast<std::int32_t>((cMine / 2));
                                }
                                lpthBest = nullptr;
                                lBest = 10000000;
                                lpth = app.state.lpThings;
                                lpthMac = app.state.lpThings + app.state.cThing;
                                for (; lpth < lpthMac; lpth++) {
                                    if (lpth->iplr() == lpfl->iPlayer && lpth->ith() == ithMinefield && lpth->thm().iType() == j) {
                                        dx = static_cast<std::int16_t>((lpfl->pt.x - lpth->pt().x()));
                                        dy = static_cast<std::int16_t>((lpfl->pt.y - lpth->pt().y()));
                                        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                                        if (lpth->thm().cMines() >= l && l < lBest) {
                                            lBest = l;
                                            lpthBest = lpth;
                                        }
                                    }
                                }
                                if (lpthBest && lpthBest->thm().cMines() < 1000000) {
                                    { const auto xForThing = (LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((lpthBest->pt().x() * lpthBest->thm().cMines())) + static_cast<std::uint32_t>((lpfl->pt.x * cMine)))) /
                                                         (cMine + lpthBest->thm().cMines()))))); auto &thForThing = *(lpthBest); thForThing.pt().SetX(xForThing); }
                                    { const auto yForThing = (LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((lpthBest->pt().y() * lpthBest->thm().cMines())) + static_cast<std::uint32_t>((lpfl->pt.y * cMine)))) /
                                                         (cMine + lpthBest->thm().cMines()))))); auto &thForThing = *(lpthBest); thForThing.pt().SetY(yForThing); }
                                    lpthBest->thm().SetCMines(lpthBest->thm().cMines() + cMine);
                                    idm = idmHasIncreasedMinefieldMines;
                                } else {
                                    lpth = LpthNew(app, lpfl->iPlayer, ithMinefield);
                                    if (!lpth) {
                                        TurnIO().pfnMessage2(app, lpfl->iPlayer, idmFailedLayMinesYearDueTechnicalDifficulties, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                                        continue;
                                    }
                                    lpth->pt().Set(lpfl->pt);
                                    lpth->thm().SetCMines(cMine);
                                    lpth->thm().SetIType(j);
                                }
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idm, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, LOWORD(cMine), HIWORD(cMine), 0, 0, 0, 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}
