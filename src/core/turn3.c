#include "common.h"

void SatisfyOrders(int16_t iPass) {
    int16_t        fMining;
    int32_t        amountWP;
    XferActionType action;
    PLANET         pl;
    int32_t        l2;
    int16_t        j;
    int32_t        amount;
    int16_t        iflWP;
    int16_t        fSentBadFleetXfer;
    int16_t        ifltcur;
    FLEET         *lpfl;
    int16_t        fAtPlanet;
    MessageId      idm;
    int16_t        iLoad;
    uint16_t       xWP;
    int16_t        fOptFuel;
    int16_t        fStealing;
    FLEET         *lpflWP;
    int16_t        fHasPermission;
    int16_t        fFulfilled;
    ORDER          ord;
    int16_t        fFueling;
    int32_t        wtOptimalFuel;
    int16_t        fDunnage;
    int32_t        amountEdit;
    int32_t        l;
    int16_t        fDone;
    uint16_t       idWP;
    THING         *lpthWP;
    int32_t        cFuel2;
    int32_t        iExcess;
    int32_t        lMaxFuel;
    int32_t        wtFuelOrig;
    int32_t        lT;
    uint16_t       iGoto;
    SHDEF         *lpshdefT;
    int32_t        lXferMinerals;
    int16_t        i;
    int32_t        lAmt;
    SHDEF          shdefT;
    int16_t        csh;
    int16_t        fBleeding;
    int32_t        lResUltimate;
    int16_t        fUltimate;
    int16_t        fColonize;
    int32_t        rgwt[3];
    int32_t        cMine;
    PLANET        *lppl;
    int32_t        rglQuan[4];
    FLEET         *lpflDest;
    int16_t        ishLastFree;
    int16_t        rgishMap[16];
    int16_t        ishMatch;
    int16_t        ish;
    FLEET         *lpflNew;
    int16_t        iplrDest;
    SHDEF         *lpshdefDest;
    THING         *lpthMac;
    int32_t        dy;
    THING         *lpthBest;
    THING         *lpth;
    int32_t        lBest;
    int32_t        dx;

    if (cFleet > 0) {
        for (ifltcur = 0; ifltcur < cFleet; ifltcur++) {
            lpfl = rglpfl[ifltcur];
            if (!rglpfl[ifltcur])
                break;
            if (iPass == 1) {
                lpfl->fCompChg = FALSE;
                lpfl->fTargeted = FALSE;
            }
            if (!lpfl->fDead && !lpfl->fSkipped) {
                ord = lpfl->lpplord->rgord[0];
                if (ord.grTask == grTaskXfer) {
                    fFulfilled = TRUE;
                    fDone = TRUE;
                    fSentBadFleetXfer = FALSE;
                    fHasPermission = TRUE;
                    fMining = 0;
                    fFueling = FALSE;
                    fStealing = FALSE;
                    fDunnage = 0;
                    lpflWP = NULL;
                    lpthWP = NULL;
                    fAtPlanet = lpfl->idPlanet != idPlanetDeepSpace && FLookupPlanet(lpfl->idPlanet, &pl) != 0;
                    idWP = ord.id;
                    switch (ord.grobj) {
                    case grobjFleet:
                        lpflWP = LpflFromId(ord.id);
                        idWP |= 0x8000;
                        xWP = 0xffff;
                        fHasPermission = lpfl->iPlayer == lpflWP->iPlayer;
                        if (!fHasPermission) {
                            GetFleetScannerRange(lpfl, NULL, NULL, &fHasPermission);
                            if (!fHasPermission)
                                break;
                            fStealing = TRUE;
                            break;
                        }
                        if (!lpflWP->fHereAllTurn || !fAtPlanet)
                            break;
                        if (CMineFromLpfl(lpflWP) > 0 && (pl.iPlayer == iplrNone || pl.iPlayer == lpfl->iPlayer)) {
                            fMining = 1;
                            xWP = 0xffff;
                            idWP = lpflWP->id | 0x8000;
                            break;
                        }
                        if (LGetFleetStat(lpflWP, 2) != 0 || pl.iPlayer != lpfl->iPlayer)
                            break;
                        fFueling = TRUE;
                        xWP = 0xffff;
                        idWP = lpflWP->id | 0x8000;
                        break;
                    case grobjThing:
                        lpthWP = LpthFromId(ord.id);
                        xWP = 0xfffe;
                        break;
                    case grobjPlanet:
                        xWP = 0xffff;
                        fHasPermission = lpfl->iPlayer == pl.iPlayer ? 3 : FALSE;
                        if (fHasPermission)
                            break;
                        GetFleetScannerRange(lpfl, NULL, NULL, &fHasPermission);
                        if (fHasPermission == 1) {
                            fHasPermission = FALSE;
                        }
                        if (fHasPermission) {
                            fStealing = TRUE;
                        }
                        if (fHasPermission || pl.iPlayer != iplrNone)
                            break;
                        for (iflWP = 0; iflWP < cFleet; iflWP++) {
                            lpflWP = rglpfl[iflWP];
                            if (!rglpfl[iflWP])
                                break;
                            if (!lpflWP->fDead && lpflWP->iPlayer == lpfl->iPlayer && lpfl->idPlanet == lpflWP->idPlanet && lpflWP != lpfl &&
                                lpflWP->fHereAllTurn && CMineFromLpfl(lpflWP) > 0) {
                                fMining = 2;
                                fHasPermission = TRUE;
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
                    fOptFuel = FALSE;
                    if (ord.grobj == grobjThing && (!lpthWP || lpthWP->ith != ithMineralPacket)) {
                        if (lpthWP) {
                            FSendPlrMsg2(lpfl->iPlayer, idmHadOrdersTransferCargoFutilePursuit, lpfl->id | 0x8000, lpfl->id, lpthWP->ith);
                        }
                        goto CancelOrder;
                    }
                LTryDunnage:
                    for (j = 0; j < 5; j++) {
                        action = ord.txp.rgia[j].iAction;
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
                                    amountWP = lpthWP->thp.rgwtMin[j];
                                }
                            }
                            switch (action) {
                            case iActionFillPercent:
                            case iActionWaitPercent:
                                if (iLoad == 0)
                                    continue;
                                if (j == 4) {
                                    amount = LGetFleetStat(lpfl, 1);
                                } else {
                                    amount = LGetFleetStat(lpfl, 2);
                                }
                                amount = min(2000000, amount);
                                if (amount < 65536) {
                                    amountEdit = (uint32_t)((uint32_t)(amount * ord.txp.rgia[j].cQuan) / 100);
                                } else {
                                    amountEdit = (uint32_t)((int32_t)(amount / 100) * ord.txp.rgia[j].cQuan);
                                }
                                amountEdit -= lpfl->rgwtMin[j];
                                if (amountEdit < 0) {
                                    amountEdit = 0;
                                }
                                break;
                            case iActionLoadExact:
                                if (iLoad == 0)
                                    continue;
                                amountEdit = ord.txp.rgia[j].cQuan;
                                break;
                            case iActionUnloadExact:
                                if (iLoad != 0)
                                    continue;
                                /* fallthrough */
                            case iActionSetAmount:
                            case iActionSetWaypoint:
                                amountEdit = ord.txp.rgia[j].cQuan;
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
                            case iActionSetWaypoint:
                                amount = amountWP - amountEdit;
                                if (amount < 0) {
                                    if (iLoad != 0)
                                        continue;
                                    amount = -amount;
                                    amount = min(amount, lpfl->rgwtMin[j]);
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
                                    fDone = FALSE;
                                    if (iPass != 4)
                                        goto Load;
                                    idm = j == 3 ? idmAttemptedSetNumberBoardUnfortunatelyCouldntProvi : idmAttemptedSetAmountBoardUnfortunatelyCouldntProvi;
                                    FSendPlrMsg(lpfl->iPlayer, idm, lpfl->id | 0x8000, lpfl->id, j, ord.txp.rgia[j].cQuan, 0, xWP, idWP, 0);
                                    goto Load;
                                }
                                if (iLoad != 0)
                                    continue;
                                amount = -amount;
                                goto Unload;
                            case iActionLoadDunnage:
                                if (j == 4) {
                                    wtOptimalFuel = 0;
                                    fOptFuel = TRUE;
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
                                    l = GetFuelFree(lpfl);
                                } else {
                                    l = GetCargoFree(lpfl);
                                }
                                amount = min(l, amount);
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
                                        FSendPlrMsg2(lpfl->iPlayer, idm, lpfl->id | 0x8000, lpfl->id, j);
                                        goto CancelOrder;
                                    }
                                    fDone = FALSE;
                                    continue;
                                }
                                if (fStealing && (j == 3 || j == 4)) {
                                    /* A thief can't take colonists or fuel. The original marked
                                       the item done but loaded it anyway; its unused messages
                                       report the attempt as unsuccessful. */
                                    FSendPlrMsg(lpfl->iPlayer,
                                                j == 3 ? idmAttemptedShanghaiColonistsAttemptUnsuccessful : idmAttemptedStealMgFuelAttemptUnsuccessful,
                                                lpfl->id | 0x8000, lpfl->id, LOWORD(amount), HIWORD(amount), xWP, idWP, 0, 0);
                                    fDone = TRUE;
                                    continue;
                                }
                                if (amount != 0) {
                                    l = min(amount, amountWP);
                                    l2 = ChgCargo(ord.grobj, ord.id, j, -l, NULL);
                                    if (l2 != 0) {
                                        l = ChgCargo(grobjFleet, lpfl->id, j, -l2, NULL);
                                        if (l != 0) {
                                            if (ord.grobj == grobjFleet && lpfl->iPlayer != lpflWP->iPlayer) {
                                                FSendPlrMsg(lpfl->iPlayer, idmHasStolen, lpfl->id | 0x8000, lpfl->id, LOWORD(l), HIWORD(l), j, idWP & 0x7fff, 0,
                                                            0);
                                            } else {
                                                FSendPlrMsg(lpfl->iPlayer, j == 3 ? idmHasBeamed : idmHasLoaded, lpfl->id | 0x8000, lpfl->id, LOWORD(l),
                                                            HIWORD(l), j, xWP, idWP, 0);
                                            }
                                        }
                                    }
                                    if ((fFueling || fMining) && amount != -l2) {
                                        l = amount + l2;
                                        l2 = ChgCargo(grobjPlanet, pl.id, j, -l, NULL);
                                        if (l2 != 0) {
                                            l = ChgCargo(grobjFleet, lpfl->id, j, -l2, NULL);
                                            if (fMining) {
                                                FSendPlrMsg(lpfl->iPlayer, idmHasLoadedMiningRobotsWorking, lpfl->id | 0x8000, lpfl->id, LOWORD(l), HIWORD(l),
                                                            j, lpflWP->id, pl.id, 0);
                                            } else {
                                                FSendPlrMsg(lpfl->iPlayer, j == 3 ? idmHasBeamed : idmHasLoaded, lpfl->id | 0x8000, lpfl->id, LOWORD(l),
                                                            HIWORD(l), j, xWP, pl.id, 0);
                                            }
                                        } else {
                                            l = 0;
                                        }
                                    }
                                    if (amount != l && action == iActionWaitPercent) {
                                        fFulfilled = FALSE;
                                    }
                                    if (l != 0 && action == iActionLoadDunnage && j == 4) {
                                        wtOptimalFuel = l;
                                    }
                                    continue;
                                } else if (action == iActionWaitPercent) {
                                    if (j == 4) {
                                        fDone = FALSE;
                                        continue;
                                    }
                                    fFulfilled = FALSE;
                                }
                                continue;
                            case iActionUnloadAll:
                            case iActionUnloadExact:
                                amount = min((uint32_t)lpfl->rgwtMin[j], (uint32_t)amountEdit);
                            Unload:
                                if (iLoad != 0)
                                    continue;
                                if (j == 3 && ord.grobj == grobjPlanet && pl.iPlayer != lpfl->iPlayer) {
                                    if (pl.iPlayer == iplrNone && !pl.fWasInhabited) {
                                        idm = idmHasTriedBeamColonistsPlanetUninhabitedMust;
                                    LCantDrop:
                                        FSendPlrMsg2(lpfl->iPlayer, idm, lpfl->id | 0x8000, lpfl->id, pl.id);
                                        goto CancelOrder;
                                    }
                                    if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh) {
                                        idm = idmCaptainHasAttemptedBeamColonistsOverruledBridge;
                                        goto LCantDrop;
                                    }
                                    if (pl.fStarbase) {
                                        idm = idmHasTriedBeamColonistsPlanetsStarbaseWould;
                                        goto LCantDrop;
                                    }
                                    FQueueColonistDrop(lpfl, &pl, amount);
                                } else if (j == 3 && ord.grobj == grobjFleet && lpfl->iPlayer != ((uint16_t)ord.id >> 9 & 0xf)) {
                                    idm = idmAllowedTransferColonistsAnotherPlayer;
                                    goto LCantDrop;
                                } else if (ord.grobj == grobjFleet && rgplr[lpflWP->iPlayer].rgmdRelation[lpfl->iPlayer] == 2) {
                                    amount = 0;
                                } else if (j == 3 && ord.grobj == grobjOther) {
                                    FSendPlrMsg2(lpfl->iPlayer, idmHasTriedBeamColonistsDeepSpaceOrder, lpfl->id | 0x8000, lpfl->id, 0);
                                    goto CancelOrder;
                                } else if (amount != 0) {
                                    if (fFueling && j != 4) {
                                        l = ChgCargo(grobjPlanet, pl.id, j, amount, NULL);
                                        FSendPlrMsg(lpfl->iPlayer, j == 3 ? idmHasBeamed2 : idmHasUnloaded, lpfl->id | 0x8000, lpfl->id, LOWORD(l), HIWORD(l),
                                                    j, xWP, pl.id, 0);
                                    } else {
                                        l = ChgCargo(ord.grobj, ord.id, j, amount, NULL);
                                        if (l > 0) {
                                            FSendPlrMsg(lpfl->iPlayer, j == 3 ? idmHasBeamed2 : idmHasUnloaded, lpfl->id | 0x8000, lpfl->id, LOWORD(l),
                                                        HIWORD(l), j, xWP, idWP, 0);
                                        }
                                        amount = l;
                                    }
                                }
                                if (amount != 0) {
                                    l = ChgCargo(grobjFleet, lpfl->id, j, -amount, NULL);
                                }
                                ord.txp.rgia[j].iAction = iActionNone;
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
                        amount = EstFuelUse(lpfl, 0, -1, -1, FALSE);
                        if (amount > lpfl->rgwtMin[4]) {
                            fDone = FALSE;
                            if (wtOptimalFuel != 0) {
                                FSendPlrMsg(lpfl->iPlayer, idmHasLoaded, lpfl->id | 0x8000, lpfl->id, LOWORD(wtOptimalFuel), HIWORD(wtOptimalFuel), 4, xWP,
                                            idWP, 0);
                            }
                            if (iPass == 4 && !fHasPermission) {
                                FSendPlrMsg(lpfl->iPlayer, idmFailedLoadFuel, lpfl->id | 0x8000, lpfl->id, xWP, idWP, 0, 0, 0, 0);
                                goto FinishFleet;
                            }
                            if (iPass != 2)
                                goto FinishFleet;
                            lMaxFuel = LGetFleetStat(lpfl, 1);
                            if (lMaxFuel < amount) {
                                FSendPlrMsg(lpfl->iPlayer, idmWillNeverMakeWaypointFuelCapacityMg, lpfl->id | 0x8000, lpfl->id, LOWORD(lMaxFuel),
                                            HIWORD(lMaxFuel), LOWORD(amount), HIWORD(amount), 0, 0);
                                goto FinishFleet;
                            }
                            cFuel2 = amount - lpfl->rgwtMin[4];
                            if (fMining == 2) {
                                idWP = pl.id;
                            }
                            FSendPlrMsg(lpfl->iPlayer, idmThereIsntEnoughFuelAvailableAllowGet, lpfl->id | 0x8000, xWP, idWP, lpfl->id, LOWORD(cFuel2),
                                        HIWORD(cFuel2), 0, 0);
                            goto FinishFleet;
                        }
                        if (amount < lpfl->rgwtMin[4]) {
                            wtFuelOrig = lpfl->rgwtMin[4];
                            do {
                                lpfl->rgwtMin[4] = amount;
                                amount = EstFuelUse(lpfl, 0, -1, -1, FALSE);
                            } while (amount < lpfl->rgwtMin[4]);
                            iExcess = wtFuelOrig - amount;
                            lpfl->rgwtMin[4] = wtFuelOrig;
                        SetOptAmount:
                            if (iExcess != 0) {
                                l2 = ChgCargo(ord.grobj, ord.id, Fuel, iExcess, NULL);
                                if (l2 != 0) {
                                    l = ChgCargo(grobjFleet, lpfl->id, Fuel, -l2, NULL);
                                }
                            } else {
                                l = 0;
                            }
                            if (l2 != 0) {
                                l += wtOptimalFuel;
                                if (l > 0) {
                                    FSendPlrMsg(lpfl->iPlayer, idmHasLoaded, lpfl->id | 0x8000, lpfl->id, LOWORD(l), HIWORD(l), 4, xWP, idWP, 0);
                                } else if (l < 0) {
                                    FSendPlrMsg(lpfl->iPlayer, idmHasUnloaded, lpfl->id | 0x8000, lpfl->id, -LOWORD(l),
                                                LOWORD((uint32_t)((uint32_t)-l >> 0x10)), 4, xWP, idWP, 0);
                                }
                            }
                        }
                        if (fDone && fFulfilled) {
                            ord.txp.rgia[4].iAction = iActionNone;
                            ord.txp.rgia[4].cQuan = 0;
                        }
                    }
                FinishFleet:
                    if (!fFulfilled && GetCargoFree(lpfl) > 0) {
                        fDone = FALSE;
                    }
                    if (fDone && fDunnage == 1 && (GetCargoFree(lpfl) > 0 || fOptFuel)) {
                        fDunnage = 2;
                        goto LTryDunnage;
                    }
                    if (fDone && iLoad != 0)
                        goto CancelOrder;
                    if (fMining == 2) {
                        ord.id = pl.id;
                        ord.grobj = grobjPlanet;
                    }
                    lpfl->lpplord->rgord[0] = ord;
                    continue;
                } else if ((ord.grTask == grTaskScrap && iPass == 1) || ord.grTask == grTaskColonize) {
                    if (lpfl->idPlanet == idPlanetDeepSpace) {
                        if (ord.grTask == grTaskColonize) {
                            FSendPlrMsg2(lpfl->iPlayer, idmHasOrderColonizeCurrentlyOrbitPlanetOrder, lpfl->id | 0x8000, lpfl->id, 0);
                            goto CancelOrder;
                        }
                        pl.id = idplNone;
                        pl.iPlayer = iplrNone;
                        pl.fStarbase = FALSE;
                        goto LScrap;
                    }
                    if (!FLookupPlanet(lpfl->idPlanet, &pl) && ord.grTask == grTaskColonize)
                        goto CancelOrder;
                LScrap:
                    if (ord.grTask == grTaskColonize) {
                        if (pl.iPlayer != iplrNone) {
                            FSendPlrMsg(lpfl->iPlayer, idmHasOrdersColonizeAlreadyPopulatedColonizeOrder, lpfl->id | 0x8000, lpfl->id, pl.id, pl.id, 0, 0, 0,
                                        0);
                            goto CancelOrder;
                        }
                        if (lpfl->rgwtMin[3] == 0) {
                            FSendPlrMsg2(lpfl->iPlayer, idmHasOrdersColonizeHaveFailedBringAlong, lpfl->id | 0x8000, lpfl->id, pl.id);
                            goto CancelOrder;
                        }
                    }
                    for (i = 0; i < 3; i++) {
                        rgwt[i] = 0;
                    }
                    fColonize = FALSE;
                    csh = 0;
                    memset(rgTechBattle, 0, 6);
                    memset(rgTechTrader, 0, 13);
                    for (i = 0; i < 16; i++) {
                        if (lpfl->rgcsh[i] > 0) {
                            csh += lpfl->rgcsh[i];
                            MarkTechsSeen(&rglpshdef[lpfl->iPlayer][i].hul, lpfl->iPlayer);
                            for (j = 0; j < rglpshdef[lpfl->iPlayer][i].hul.chs; j++) {
                                /* cItem: the original accepted a slot that once held a module. */
                                if (rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst == hstSpecialM && rglpshdef[lpfl->iPlayer][i].hul.rghs[j].cItem > 0 &&
                                    (rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                                     rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                                    fColonize = TRUE;
                                }
                            }
                        }
                    }
                    if (ord.grTask == grTaskColonize && !fColonize) {
                        FSendPlrMsg(lpfl->iPlayer, idmHasOrdersColonizeNoneShipsHaveColonization, lpfl->id | 0x8000, lpfl->id, pl.id, lpfl->id, 0, 0, 0, 0);
                    } else {
                        lXferMinerals = 0;
                        fUltimate = ord.grTask == grTaskScrap && pl.iPlayer != iplrNone && GetRaceGrbit(&rgplr[pl.iPlayer], ibitRaceUltimateRecycling) != 0;
                        fBleeding = GetRaceGrbit(&rgplr[lpfl->iPlayer], ibitRaceBleedingEdgeTech);
                        gd.fDontCalcBleed = TRUE;
                        idPlayer = lpfl->iPlayer;
                        for (i = 0; i <= 2; i++) {
                            lAmt = 0;
                            for (j = 0; j < 16; j++) {
                                if (lpfl->rgcsh[j] > 0) {
                                    if (fBleeding) {
                                        shdefT = rglpshdef[lpfl->iPlayer][j];
                                        UpdateShdefCost(&shdefT);
                                        lpshdefT = &shdefT;
                                    } else {
                                        lpshdefT = rglpshdef[lpfl->iPlayer] + j;
                                    }
                                    lT = (uint32_t)(lpfl->rgcsh[j] * (uint32_t)lpshdefT->hul.rgwtOreCost[i]);
                                    if (lpshdefT->fGift) {
                                        lT = (int32_t)(lT / 4);
                                    }
                                    lAmt += lT;
                                }
                            }
                            if (ord.grTask == grTaskColonize) {
                                lAmt = (int32_t)(lAmt * 3) / 4;
                            } else if (pl.id == idplNone) {
                                lAmt = (int32_t)(lAmt / 3);
                            } else if (pl.fStarbase) {
                                if (fUltimate) {
                                    lAmt = (int32_t)(lAmt * 9) / 10;
                                } else {
                                    lAmt = (int32_t)(lAmt * 4) / 5;
                                }
                            } else if (fUltimate) {
                                lAmt = (int32_t)(lAmt * 9) / 20;
                            } else {
                                lAmt = (int32_t)(lAmt / 3);
                            }
                            lAmt += lpfl->rgwtMin[i];
                            lXferMinerals += lAmt;
                            if (pl.id == idplNone) {
                                pl.rgwtMin[i] = lAmt;
                            } else {
                                lpPlanets[pl.id].rgwtMin[i] = lpPlanets[pl.id].rgwtMin[i] + lAmt;
                            }
                        }
                        iGoto = pl.id;
                        if (fUltimate) {
                            lResUltimate = 0;
                            for (j = 0; j < 16; j++) {
                                if (lpfl->rgcsh[j] > 0) {
                                    if (fBleeding) {
                                        shdefT = rglpshdef[lpfl->iPlayer][j];
                                        UpdateShdefCost(&shdefT);
                                        lpshdefT = &shdefT;
                                    } else {
                                        lpshdefT = rglpshdef[lpfl->iPlayer] + j;
                                    }
                                    lT = (uint32_t)(lpfl->rgcsh[j] * (uint32_t)lpshdefT->hul.resCost);
                                    if (lpshdefT->fGift) {
                                        lT = (int32_t)(lT / 4);
                                    }
                                    lResUltimate += lT;
                                }
                            }
                            if (lResUltimate > 65535) {
                                lResUltimate = 65535;
                            }
                            vrgPlanResExtra[pl.id] = vrgPlanResExtra[pl.id] + LOWORD(lResUltimate);
                            if (lResUltimate > 0 && pl.iPlayer != iplrNone) {
                                lAmt = CResourcesAtPlanet(&pl, pl.iPlayer);
                                lResUltimate = (int32_t)((int32_t)(lResUltimate * lAmt) / (lResUltimate + lAmt));
                            }
                            idm = idmHasDismantledKtMineralsWhichHaveDeposited2 + pl.fStarbase;
                            FSendPlrMsg(lpfl->iPlayer, idm, iGoto, WFromLpfl(lpfl), LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, LOWORD(lResUltimate),
                                        HIWORD(lResUltimate), 0);
                            idm = idmHasDismantledKtMineralsWhichHaveDeposited5 + pl.fStarbase;
                            if (pl.fStarbase) {
                                i = ITechLearnATech(pl.iPlayer, 0, 0, idmNone, &iGoto);
                                if (i != 0) {
                                    idm = idmHasDismantledKtMineralsWhichHaveDeposited3;
                                    if (i < 0) {
                                        i = -(i + 1);
                                    } else {
                                        i--;
                                        idm++;
                                    }
                                } else {
                                    iGoto = pl.id;
                                }
                            } else {
                                iGoto = pl.id;
                            }
                            FSendPlrMsg(pl.iPlayer, idm, iGoto, lpfl->id, LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, LOWORD(lResUltimate),
                                        HIWORD(lResUltimate), i);
                        } else if (pl.id == idplNone) {
                            lpthWP = NULL;
                            DropSalvage(&lpthWP, pl.rgwtMin, lpfl->iplr, &ord.pt);
                            FSendPlrMsg2(lpfl->iPlayer, idmHasDismantledScrapLeftDeepSpace, gotoThing, lpthWP->idFull, WFromLpfl(lpfl));
                        } else {
                            idm = idmHasDismantledKtMineralsWhichHaveDeposited + pl.fStarbase;
                            FSendPlrMsg(lpfl->iPlayer, idm, iGoto, WFromLpfl(lpfl), LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, 0, 0, 0);
                            idm = idmHasDismantledKtMineralsWhichHaveDeposited4 + pl.fStarbase;
                            if (pl.fStarbase) {
                                i = ITechLearnATech(pl.iPlayer, 0, 0, idmNone, &iGoto);
                                if (i != 0) {
                                    idm = idmHasDismantledKtMineralsStarbaseOrbitingProcess;
                                    if (i < 0) {
                                        i = -(i + 1);
                                    } else {
                                        i--;
                                        idm++;
                                    }
                                } else {
                                    iGoto = pl.id;
                                }
                            }
                            FSendPlrMsg(pl.iPlayer, idm, iGoto, lpfl->id, LOWORD(lXferMinerals), HIWORD(lXferMinerals), pl.id, i, 0, 0);
                        }
                        idPlayer = iplrNone;
                        gd.fDontCalcBleed = FALSE;
                        FRemovePlayerMessage(lpfl->iPlayer, idmHasCompletedAssignedOrders, lpfl->id | 0x8000);
                        lpfl->fDead = TRUE;
                        if (ord.grTask == grTaskColonize) {
                            FQueueColonistDrop(lpfl, &pl, lpfl->rgwtMin[3]);
                        } else if (lpfl->idPlanet != idPlanetDeepSpace && lpPlanets[lpfl->idPlanet].iPlayer == lpfl->iPlayer) {
                            lpPlanets[lpfl->idPlanet].rgwtMin[3] = lpPlanets[lpfl->idPlanet].rgwtMin[3] + lpfl->rgwtMin[3];
                        }
                    }
                CancelOrder:
                    if (lpfl->cord == 1 && !lpfl->fDead && lpfl->lpplord->rgord[0].grTask != grTaskNone) {
                        FRemovePlayerMessage(lpfl->iPlayer, idmHasCompletedAssignedOrders, lpfl->id | 0x8000);
                        FSendPlrMsg2(lpfl->iPlayer, idmHasCompletedAssignedOrders, lpfl->id | 0x8000, lpfl->id, 0);
                    }
                    lpfl->lpplord->rgord[0].grTask = grTaskNone;
                } else if (ord.grTask == grTaskMine) {
                    cMine = 0;
                    if (iPass != 3 || !lpfl->fHereAllTurn)
                        continue;
                    if (lpfl->idPlanet == idPlanetDeepSpace) {
                        FSendPlrMsg2(lpfl->iPlayer, idmRemoteMiningRobotsHadOrdersMineDeep, lpfl->id | 0x8000, lpfl->id, 0);
                        goto CancelOrder;
                    }
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (!lppl)
                        goto CancelOrder;
                    cMine = CMineFromLpfl(lpfl);
                    if (cMine == 0) {
                        FSendPlrMsg2(lpfl->iPlayer, idmHadOrdersMineFleetDoesntHaveAny, lpfl->id | 0x8000, lpfl->id, lppl->id);
                        goto CancelOrder;
                    }
                    if (lppl->iPlayer != iplrNone) {
                        if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh)
                            continue;
                        FSendPlrMsg2(lpfl->iPlayer, idmRemoteMiningRobotsHadOrdersMinePlanet, lpfl->id | 0x8000, lpfl->id, lppl->id);
                        goto CancelOrder;
                    }
                    EstMineralsMined(lppl, rglQuan, cMine, TRUE);
                } else if (ord.grTask == grTaskAutoRoute) {
                    if (lpfl->cord != 1 || lpfl->idPlanet == idPlanetDeepSpace)
                        continue;
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (lppl->iPlayer == lpfl->iPlayer && lppl->idRoute != 0) {
                        AutoRouteFleet(lpfl, lppl);
                        FSendPlrMsg(lpfl->iPlayer, lpfl->lpplord->rgord[1].iWarp == 0 ? idmHasReroutedUnfortuentlyDoesHaveEnoughFuel : idmHasRerouted,
                                    lpfl->id | 0x8000, lpfl->id, lpfl->idPlanet, lppl->idRoute - 1, 0, 0, 0, 0);
                        continue;
                    }
                    if (iPass != 4)
                        continue;
                    AutoFleetOrder(lpfl, lppl);
                    ord = lpfl->lpplord->rgord[0];
                    if (ord.grTask == grTaskMerge)
                        goto LDoMerge;
                } else if (ord.grTask == grTaskMerge) {
                    if (!(iPass & 1)) {
                    LDoMerge:
                        if (ord.grobj != grobjFleet) {
                        NMNF:
                            FSendPlrMsg2(lpfl->iPlayer, idmUnableCompleteMergeOrdersWaypointDestinationWasn, lpfl->id | 0x8000, lpfl->id, 0);
                            goto CancelOrder;
                        }
                        lpflDest = LpflFromId(ord.id);
                        if (!lpflDest || lpflDest->fDead)
                            goto NMNF;
                        if (lpfl == lpflDest)
                            goto CancelOrder;
                        if (lpflDest->iPlayer != lpfl->iPlayer) {
                            FSendPlrMsg2(lpfl->iPlayer, idmUnableCompleteMergeOrdersDestinationFleetWasnt, lpfl->id | 0x8000, lpfl->id, 0);
                            goto CancelOrder;
                        }
                        FSendPlrMsg2(lpfl->iPlayer, idmHasMerged, lpflDest->id | 0x8000, WFromLpfl(lpfl), lpflDest->id);
                        FRemovePlayerMessage(lpfl->iPlayer, idmHasCompletedAssignedOrders, lpfl->id | 0x8000);
                        Merge2Fleets(lpflDest, lpfl, TRUE);
                        goto CancelOrder;
                    }
                } else if (ord.grTask == grTaskGive) {
                    if (iPass != 4)
                        continue;
                    iplrDest = ord.tsell.iPlrX;
                    if (iplrDest >= lpfl->iPlayer) {
                        iplrDest++;
                    }
                    if (iplrDest < 0 || iplrDest >= game.cPlayer || rgplr[iplrDest].fDead) {
                        FSendPlrMsg2(lpfl->iPlayer, idmCouldntGiveAwayBecausePlayerDead, lpfl->id | 0x8000, lpfl->id, 0);
                        goto CancelOrder;
                    }
                    if (rgplr[iplrDest].fAi || rgplr[iplrDest].rgmdRelation[lpfl->iPlayer] == 2) {
                        FSendPlrMsg2(lpfl->iPlayer, idmSnubAttemptedGiftRefuseFleet, lpfl->id | 0x8000, 0x30 | iplrDest, 0);
                        goto CancelOrder;
                    }
                    if (lpfl->rgwtMin[3] > 0) {
                        FSendPlrMsg2(lpfl->iPlayer, idmCouldntGiveAwayBecauseThereColonistsBoard, lpfl->id | 0x8000, lpfl->id, 0);
                        goto CancelOrder;
                    }
                    ishLastFree = -1;
                    for (ish = 0; ish < 16; ish++) {
                        if (lpfl->rgcsh[ish] == 0) {
                            rgishMap[ish] = ishdefNone;
                        } else {
                            ishMatch = IshFindSimilarDesign(&rglpshdef[lpfl->iPlayer][ish].hul, iplrDest);
                            if (ishMatch != ishdefNone) {
                                rgishMap[ish] = ishMatch;
                            } else {
                                do {
                                    ishLastFree++;
                                } while (ishLastFree < 16 && !rglpshdef[iplrDest][ishLastFree].fFree);
                                if (ishLastFree >= 16) {
                                SellNoCap:
                                    FSendPlrMsg2(lpfl->iPlayer, idmCouldntGiveAwayBecauseDidntHaveAdministrative, lpfl->id | 0x8000, lpfl->id, 0x30 | iplrDest);
                                    FSendPlrMsg2(iplrDest, idmAttemptedGiveFleetDontHaveEnoughExcess, gotoNone, 0x30 | lpfl->iPlayer, 0);
                                    goto CancelOrder;
                                }
                                rgishMap[ish] = ishLastFree;
                            }
                        }
                    }
                    if (rgplr[iplrDest].cFleet >= 0x200)
                        goto SellNoCap;
                    lpflNew = LpflNew(iplrDest, lpfl->idPlanet);
                    if (!lpflNew)
                        goto SellNoCap;
                    if (iplrDest < lpfl->iPlayer) {
                        ifltcur++;
                    }
                    lpflNew->pt = lpfl->pt;
                    lpflNew->lpplord->rgord[0].pt = lpfl->pt;
                    for (ish = 0; ish <= 4; ish++) {
                        lpflNew->rgwtMin[ish] = lpfl->rgwtMin[ish];
                    }
                    for (ish = 0; ish < 16; ish++) {
                        if (lpfl->rgcsh[ish] > 0) {
                            lpshdefDest = rglpshdef[iplrDest] + rgishMap[ish];
                            if (lpshdefDest->fFree) {
                                *lpshdefDest = rglpshdef[lpfl->iPlayer][ish];
                                lpshdefDest->ishdef = rgishMap[ish];
                                lpshdefDest->fGift = TRUE;
                                lpshdefDest->cBuilt = 0;
                                lpshdefDest->cExist = 0;
                                rgplr[iplrDest].cShDef++;
                            }
                            lpflNew->rgcsh[rgishMap[ish]] = lpflNew->rgcsh[rgishMap[ish]] + lpfl->rgcsh[ish];
                            lpflNew->rgdv[rgishMap[ish]].dp = lpfl->rgdv[ish].dp;
                            lpshdefDest->cBuilt += lpfl->rgcsh[ish];
                            lpshdefDest->cExist += lpfl->rgcsh[ish];
                        }
                    }
                    lpfl->fDead = TRUE;
                    FSendPlrMsg2(lpfl->iPlayer, idmHasSuccessfullyGiven, lpflNew->id | 0x8000, WFromLpfl(lpfl), 0x30 | iplrDest);
                    FSendPlrMsg2(iplrDest, idmHaveGiven, lpflNew->id | 0x8000, 0x30 | lpfl->iPlayer, WFromLpfl(lpflNew));
                    lpfl->fDead = TRUE;
                } else if (ord.grTask == grTaskLayMines ||
                           (ord.grTask == grTaskNone && lpfl->cord > 1 && GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMines &&
                            lpfl->lpplord->rgord[1].grTask == grTaskLayMines)) {
                    idm = idmHasDispersedMines;
                    if (iPass == 3 && (lpfl->fHereAllTurn || GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMines)) {
                        cMine = CLayMinesFromLpfl(lpfl, mineAll, ishdefAll);
                        if (cMine == 0) {
                            FSendPlrMsg2(lpfl->iPlayer, idmHasAttemptedLayMinesOrderHasCanceled, lpfl->id | 0x8000, lpfl->id, 0);
                            goto CancelOrder;
                        }
                        if (ord.grTask == grTaskLayMines) {
                            if (lpfl->lpplord->rgord[0].tsell.iPlrX == 0) {
                                lpfl->lpplord->rgord[0].grTask = grTaskNone;
                            } else if (lpfl->lpplord->rgord[0].tsell.iPlrX != 5) {
                                lpfl->lpplord->rgord[0].tsell.iPlrX--;
                            }
                        }
                        for (j = 0; j < 3; j++) {
                            cMine = CLayMinesFromLpfl(lpfl, j, ishdefAll);
                            if (cMine != 0) {
                                if (!lpfl->fHereAllTurn) {
                                    cMine = (int32_t)(cMine / 2);
                                }
                                lpthBest = NULL;
                                lBest = 10000000;
                                lpth = lpThings;
                                lpthMac = lpThings + cThing;
                                for (; lpth < lpthMac; lpth++) {
                                    if (lpth->iplr == lpfl->iPlayer && lpth->ith == ithMinefield && lpth->thm.iType == j) {
                                        dx = (int16_t)(lpfl->pt.x - lpth->pt.x);
                                        dy = (int16_t)(lpfl->pt.y - lpth->pt.y);
                                        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                        if (lpth->thm.cMines >= l && l < lBest) {
                                            lBest = l;
                                            lpthBest = lpth;
                                        }
                                    }
                                }
                                if (lpthBest && lpthBest->thm.cMines < 1000000) {
                                    lpthBest->pt.x =
                                        LOWORD((int32_t)((int32_t)((uint32_t)(lpthBest->pt.x * lpthBest->thm.cMines) + (uint32_t)(lpfl->pt.x * cMine)) /
                                                         (cMine + lpthBest->thm.cMines)));
                                    lpthBest->pt.y =
                                        LOWORD((int32_t)((int32_t)((uint32_t)(lpthBest->pt.y * lpthBest->thm.cMines) + (uint32_t)(lpfl->pt.y * cMine)) /
                                                         (cMine + lpthBest->thm.cMines)));
                                    lpthBest->thm.cMines += cMine;
                                    idm = idmHasIncreasedMinefieldMines;
                                } else {
                                    lpth = LpthNew(lpfl->iPlayer, ithMinefield);
                                    if (!lpth) {
                                        FSendPlrMsg2(lpfl->iPlayer, idmFailedLayMinesYearDueTechnicalDifficulties, lpfl->id | 0x8000, lpfl->id, 0);
                                        continue;
                                    }
                                    lpth->pt = lpfl->pt;
                                    lpth->thm.cMines = cMine;
                                    lpth->thm.iType = j;
                                }
                                FSendPlrMsg(lpfl->iPlayer, idm, lpfl->id | 0x8000, lpfl->id, LOWORD(cMine), HIWORD(cMine), 0, 0, 0, 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}
