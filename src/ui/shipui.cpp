#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "transportpresets.h"
#include "text_io.h"
#include "fleet_query_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "production_io.h"
#include "heap_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "ownership.h"

void DrawShipOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t dxRight;
    int16_t iWarp;
    int16_t yTop;
    POINT16 pt;
    RECT    rcT;
    int16_t dWrong;
    int32_t lTot;
    int16_t c;
    FLEET  *pfl;
    int16_t xRight;
    int16_t iScanActual;
    RECT    rcGauge;
    const char *psz;
    int16_t xLeft;
    ORDER   ord;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl();
    if (ptile->fFixCtls) {
        app.lpui->rgrcRef[0].top = -5;
        app.lpui->rgrcRef[0].bottom = -6;
        app.lpui->rgrcRef[12].top = -5;
        app.lpui->rgrcRef[12].bottom = -6;
        ui::ShowWindow(app.lpui->hwndShipLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndRepCB, win32::SW_HIDE);
        ptile->fFixCtls = 0;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsFleetWaypoints))) {
        ui::ShowWindow(app.lpui->hwndShipLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndRepCB, win32::SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!app.state.gd.fSmallTileMode ? 4 : 2) + rc.top;
        app.lpui->rgrcRef[12].top = -5;
        app.lpui->rgrcRef[12].bottom = -6;
        ControlIO().pfnGetClientRect(app.lpui->hwndShipLB, &rcT);
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        app.lpui->dyShipLB = (app.state.dyArial8 + 2) * (!app.state.gd.fSmallTileMode ? 4 : 3);
        dWrong = app.lpui->dyShipLB - (rcT.bottom - rcT.top);
        if (app.lpui->dxShipLB == xRight - xLeft && dWrong >= 0 && dWrong < app.state.dyArial8) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->dxShipLB = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndShipLB, nullptr, xLeft, yTop, xRight - xLeft, app.lpui->dyShipLB, swp);
        ui::ShowWindow(app.lpui->hwndShipLB, win32::SW_SHOW);
        ControlIO().pfnGetClientRect(app.lpui->hwndShipLB, &rcT);
        app.lpui->dyShipLB = rcT.bottom - rcT.top;
        yTop += (!app.state.gd.fSmallTileMode ? 4 : 2) + app.lpui->dyShipLB;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        if (!ptile->fMinDraw) {
            rcT.top = yTop;
            rcT.bottom = yTop + app.state.dyArial8;
            rcT.left = xLeft;
            rcT.right = xRight;
            win32::FillRect(hdc, &rcT, app.lpui->hbrButtonFace);
        }
        c = TextIO().pfnCchGetString(app, idsComing, app.state.szWork);
        l = GetTextExtent(hdc, app.state.szWork, c);
        c = TextIO().pfnCchGetString(app, idsWayPt, app.state.szWork);
        lTot = GetTextExtent(hdc, app.state.szWork, c);
        if (lTot > l) {
            l = lTot;
        }
        dxRight = xRight - xLeft - LOWORD(l);
        if (!ptile->fMinDraw) {
            c = TextIO().pfnCchGetString(app, app.state.sel.iwpAct <= 0 ? idsWayPt : idsComing, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        if (app.state.sel.iwpAct > 0) {
            ord = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct - 1];
            psz = PszGetLocName(app, static_cast<GrobjClass>(ord.grobj), ord.id, ord.pt.x, ord.pt.y);
            iScanActual = app.state.sel.iwpAct;
        } else if (app.state.sel.fl.cord <= 1) {
            psz = "";
        } else {
            ord = app.state.sel.fl.lpplord->rgord()[1];
            psz = PszGetLocName(app, static_cast<GrobjClass>(ord.grobj), ord.id, ord.pt.x, ord.pt.y);
            ord = app.state.sel.fl.lpplord->rgord()[0];
            iScanActual = 1;
        }
        RightTextOut(app, hdc, xRight, yTop, psz, 0, dxRight);
        yTop += app.state.dyArial8;
        if (app.state.sel.fl.cord > 1 && app.state.sel.iwpAct != 0)
            goto DoDistance;
        app.lpui->rgrcRef[0].top = -5;
        app.lpui->rgrcRef[0].bottom = -6;
        if (app.state.sel.fl.cord <= 1) {
            win32::SetRect(&rc, xLeft - 1, yTop, xRight - 1, app.state.dyArial8 * 4 + yTop);
            win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
            yTop += (app.state.dyArial8 - app.state.gd.fSmallTileMode) * 4 + app.state.gd.fSmallTileMode * 2;
            goto DoCheckBox;
        }
    DoDistance:
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDistance, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        pt = app.state.sel.fl.lpplord->rgord()[iScanActual].pt;
        RightTextOut(app, hdc, xRight, yTop, PszGetDistance(app, ord.pt.x, ord.pt.y, pt.x, pt.y), 0, dxRight);
        yTop += app.state.dyArial8 - app.state.gd.fSmallTileMode;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        if (!ptile->fMinDraw) {
            c = TextIO().pfnCchGetString(app, idsWarpFactor, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        iWarp = app.state.sel.fl.lpplord->rgord()[iScanActual].iWarp;
        if (app.state.sel.iwpAct != 0) {
            win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight - 1, yTop + app.state.dyArial8);
            app.lpui->rgrcRef[0] = rcGauge;
            DrawFleetGauge(app, hdc, &rcGauge, nullptr, 6);
        } else {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (iWarp < 11) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpD2), iWarp);
            } else {
                c = TextIO().pfnCchGetString(app, idsUseStargate, app.state.szWork);
            }
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        }
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsTravelTime, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        c = CchGetETA(app, hdc, pfl, app.state.szWork, iScanActual, 0);
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        win32::SetTextColor(hdc, 0);
        yTop += app.state.dyArial8 - app.state.gd.fSmallTileMode;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsEstFuelUsage, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        lTot = LFuelUseToWaypoint(app, &app.state.sel.fl, iScanActual, 0);
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdmg), lTot);
        if (lTot > app.state.sel.fl.rgwtMin[4]) {
            win32::SetTextColor(hdc, 0xff);
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight - 20);
        if (lTot > app.state.sel.fl.rgwtMin[4]) {
            win32::SetTextColor(hdc, 0);
        }
        yTop += app.state.dyArial8;
    DoCheckBox:
        ControlIO().pfnSendMessage(app.lpui->hwndRepCB, win32::BM_SETCHECK, app.state.sel.fl.fRepOrders, 0);
        ControlIO().pfnSetWindowPos(app.lpui->hwndRepCB, nullptr, xLeft, yTop, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::ShowWindow(app.lpui->hwndRepCB, win32::SW_SHOW);
        win32::SetRect(&app.lpui->rgrcRef[12], xRight - (app.state.dyArial8 | 1), yTop, xRight, (app.state.dyArial8 | 1) + yTop);
        DrawDiamond(app, hdc, &app.lpui->rgrcRef[12], app.lpui->hbrBBlue);
    }
    return;
}

void DrawShipWayPtOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    int16_t  dxKt;
    int16_t  dxT;
    int16_t  swp;
    int16_t  dxRight;
    int16_t  yTop;
    int16_t  yTopMsg;
    StringId ids;
    int16_t  edWid;
    PLANET  *lppl;
    ORDER   *lpord;
    FLEET   *pfl;
    int16_t  i;
    int16_t  fActive;
    int16_t  xRight;
    TaskType grtask;
    char     szT[8];
    int16_t  yBot;
    int16_t  dxRight2;
    const char    *psz;
    int16_t  cch;
    int16_t  xLeft;
    int32_t  l;
    RECT     rc;
    RECT     rcT;
    char    *pszT;
    int16_t  j;
    int32_t  cMine;
    int16_t  dyCur;
    int16_t  c;
    int32_t  rgl[4];

    if (ptile->fFixCtls) {
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(app.lpui->rghwndOrderDD[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_HIDE);
        ptile->fFixCtls = 0;
        app.lpui->rgrcRef[5].top = -5;
        app.lpui->rgrcRef[5].bottom = -6;
        app.lpui->rgrcRef[18].top = -5;
        app.lpui->rgrcRef[18].bottom = -6;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsWaypointTask))) {
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(app.lpui->rghwndOrderDD[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_HIDE);
    } else {
        pfl = obj.pfl();
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!app.state.gd.fSmallTileMode ? 4 : 2) + rc.top;
        yBot = rc.bottom - 4;
        dxRight = xRight - xLeft;
        app.lpui->rgrcRef[5].top = -5;
        app.lpui->rgrcRef[5].bottom = -6;
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        if (app.lpui->rgdxOrderDD[0] == dxRight) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->rgdxOrderDD[0] = dxRight;
        }
        ControlIO().pfnSetWindowPos(app.lpui->rghwndOrderDD[0], nullptr, xLeft, yTop, dxRight, 10 * app.state.dyArial8 + app.lpui->dyShipDD, swp);
        ui::ShowWindow(app.lpui->rghwndOrderDD[0], win32::SW_SHOW);
        yTop += app.lpui->dyShipDD + 3;
        yTopMsg = yTop;
        l = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETCURSEL, 0, 0);
        grtask = static_cast<TaskType>(LOWORD(l));
        if (win32::IsWindowVisible(app.lpui->rghwndOrderDD[1]) == 0) {
            win32::SetRect(&rc, xLeft - 1, yTop, xRight + 1, yBot + 2);
            win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
        } else if (win32::IsWindowVisible(app.lpui->rghwndOrderDD[2]) == 0) {
            win32::SetRect(&rc, xLeft - 1, yTop + app.lpui->dyShipDD + 3, xRight + 1, yBot + 2);
            win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
        }
        switch (grtask) {
        case grTaskXfer:
        case grTaskLayMines:
        case grTaskPatrol:
        case grTaskGive:
            swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
            switch (grtask) {
            case grTaskXfer:
                dxRight2 = dxRight - app.lpui->dyShipDD + 2;
                break;
            case grTaskPatrol:
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                psz = TextIO().pfnCompressed(app, idsWarpFactor);
                cch = strlen(psz);
                dxT = LOWORD(GetTextExtent(hdc, psz, cch)) + 2;
                psz = TextIO().pfnCompressed(app, idsIntercept);
                cch = strlen(psz);
                win32::SetRect(&rc, xLeft, yTop + 4, xLeft + dxT, yBot);
                win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
                win32::DrawTextA(hdc, psz, cch, &rc, win32::DT_NOPREFIX);
                dxRight2 = dxRight - dxT - 2;
                xLeft += dxT + 2;
                break;
            case grTaskGive:
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                cch = TextIO().pfnCchGetString(app, idsTo3, szT);
                szT[cch] = ' ';
                cch++;
                szT[cch] = 0;
                /* NATIVE: original measured psz before it was set; see WIN16-PARITY.md */
                dxT = LOWORD(GetTextExtent(hdc, szT, cch)) + 2;
                win32::SetRect(&rc, xLeft, yTop + 4, xLeft + dxT, yBot);
                win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
                win32::DrawTextA(hdc, szT, cch, &rc, win32::DT_NOPREFIX);
                dxRight2 = dxRight - dxT - 2;
                xLeft += dxT + 2;
                break;
            default:
                dxRight2 = dxRight;
            }
            if (app.lpui->rgdxOrderDD[1] == dxRight2) {
                swp |= win32::SWP_NOSIZE;
            } else {
                app.lpui->rgdxOrderDD[1] = dxRight2;
            }
            ControlIO().pfnSetWindowPos(app.lpui->rghwndOrderDD[1], nullptr, xLeft, yTop, dxRight2, 6 * app.lpui->dyShipDD, swp);
            ui::ShowWindow(app.lpui->rghwndOrderDD[1], win32::SW_SHOW);
            if (grtask == grTaskXfer) {
                win32::SetRect(&rcT, xLeft + dxRight - (app.lpui->dyShipDD | 1) + 8, yTop + 3, xLeft + dxRight, (app.lpui->dyShipDD | 1) + yTop - 5);
                app.lpui->rgrcRef[5] = rcT;
                DrawDiamond(app, hdc, &rcT, app.lpui->hbrBBlue);
                break;
            }
            if (grtask != grTaskPatrol)
                break;
            yTop += app.lpui->dyShipDD + 4;
            win32::SetRect(&rcT, xLeft - dxT - 2, yTop, xRight - 1, yTop + app.state.dyArial8);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            psz = TextIO().pfnCompressed(app, idsWarpFactor);
            cch = strlen(psz);
            win32::DrawTextA(hdc, psz, cch, &rcT, win32::DT_NOPREFIX);
            rcT.left += dxT + 2;
            app.lpui->rgrcRef[18] = rcT;
            DrawFleetGauge(app, hdc, &rcT, nullptr, 7);
            break;
        default:
            if (win32::IsWindowVisible(app.lpui->rghwndOrderDD[1]) != 0) {
                ui::ShowWindow(app.lpui->rghwndOrderDD[1], win32::SW_HIDE);
                win32::SetRect(&rc, xLeft - 1, yTop, xRight + 1, yBot + 2);
                win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
            }
        }
        yTop += app.lpui->dyShipDD + 3;
        lpord = &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct];
        if (lpord->grobj == grobjPlanet) {
            lppl = LpplFromId(app, lpord->id);
        } else {
            lppl = nullptr;
        }
        if (grtask == grTaskXfer) {
            dxKt = 0;
            for (i = 0; i < 5; i++) {
                if (static_cast<int16_t>(LOWORD(GetTextExtent(hdc, app.state.vrgszUnits[i], 2))) > dxKt) {
                    dxKt = LOWORD(GetTextExtent(hdc, app.state.vrgszUnits[i], 2));
                }
            }
            i = LOWORD(ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0));
            if (i == 0) {
                i = 4;
            } else {
                i--;
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            edWid = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN99999Kt), 9));
            dxRight -= edWid + 8;
            swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
            if (app.lpui->rgdxOrderDD[2] == dxRight) {
                swp |= win32::SWP_NOSIZE;
            } else {
                app.lpui->rgdxOrderDD[2] = dxRight;
            }
            ControlIO().pfnSetWindowPos(app.lpui->rghwndOrderDD[2], nullptr, xLeft, yTop, dxRight, 9 * app.lpui->dyShipDD, swp);
            ui::ShowWindow(app.lpui->rghwndOrderDD[2], win32::SW_SHOW);
            l = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_GETCURSEL, 0, 0);
            fActive = FTransportQuantityActive(l, &i);
            psz = app.state.vrgszUnits[i];
            swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
            if (app.lpui->dxOrderED == edWid - dxKt) {
                swp |= win32::SWP_NOSIZE;
            } else {
                app.lpui->dxOrderED = edWid - dxKt;
            }
            ControlIO().pfnSetWindowPos(app.lpui->hwndOrderED, nullptr, xRight - edWid - 4, yTop, edWid - dxKt, app.lpui->dyShipDD, swp);
            ControlIO().pfnEnableWindow(app.lpui->hwndOrderED, fActive);
            ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_SHOW);
            win32::SetRect(&rc, xRight - dxKt - 1, yTop + 2, xRight - 1, yBot);
            win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
            if (fActive) {
                win32::DrawTextA(hdc, psz, 2, &rc, win32::DT_NOPREFIX);
            }
        } else {
            ui::ShowWindow(app.lpui->rghwndOrderDD[2], win32::SW_HIDE);
            ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_HIDE);
        }
        switch (grtask) {
        default:
            break;
        case grTaskScrap:
            ids = idsNoteShipsFleetWillDismantledMineralsCan;
            goto LDisplayMsg;
        case grTaskMerge:
            if (app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grobj == grobjFleet)
                break;
            ids = idsWarningDestinationWaypointFleetMergeWillSucessfu;
            psz = TextIO().pfnCompressed(app, ids);
            win32::SetTextColor(hdc, 127);
            win32::SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            win32::DrawTextA(hdc, psz, strlen(psz), &rc, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            break;
        case grTaskLayMines:
            yTopMsg = yTop;
            l = CLayMinesFromLpfl(app, &app.state.sel.fl, mineAll, ishdefAll);
            if (l <= 0) {
                ids = idsWarningFleetHasMineLayingPods;
                goto LDisplayMsg;
            }
            pszT = TextIO().pfnCompressed(app, idsFleetCanLayLdMinesPerYear);
            win32::wsprintfA(app.state.szWork, pszT, l);
            psz = app.state.szWork;
            goto LDisplayMsg2;
        case grTaskColonize:
            fActive = 0;
            for (i = 0; i < 16; i++) {
                if (app.state.sel.fl.rgcsh[i] > 0) {
                    for (j = 0; j < app.state.rglpshdef[app.state.sel.fl.iPlayer][i].hul.chs; j++) {
                        if (app.state.rglpshdef[app.state.sel.fl.iPlayer][i].hul.rghs[j].grhst == hstSpecialM &&
                            (app.state.rglpshdef[app.state.sel.fl.iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                             app.state.rglpshdef[app.state.sel.fl.iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                            fActive = 1;
                            goto FoundColony;
                        }
                    }
                }
            }
        FoundColony:
            if (fActive) {
                if (app.state.sel.fl.rgwtMin[3] == 0) {
                    ids = idsRememberLoadColonistsBeforeEmbarkingMission;
                    goto LDisplayMsg;
                }
                ids = idsNoteShipsFleetWillDismantledProvideSupplies;
                goto LDisplayMsg;
            }
            ids = idsWarningColonizeMissionCannotCarriedBecauseNone;
        LDisplayMsg:
            psz = TextIO().pfnCompressed(app, ids);
        LDisplayMsg2:
            win32::SetTextColor(hdc, ids == idsNoteShipsFleetWillDismantledProvideSupplies ? app.lpui->crButtonText : 127);
            win32::SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            win32::DrawTextA(hdc, psz, strlen(psz), &rc, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            break;
        case grTaskMine:
            cMine = ColonyIO().pfnFleetMining(app, &app.state.sel.fl);
            if (cMine > 0) {
                if (lppl && (lppl->iPlayer == iplrNone || (lppl->iPlayer == app.state.sel.fl.iPlayer && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh))) {
                    if (app.state.dyArial8 > 14) {
                        win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
                        dyCur = app.lpui->dyArial7;
                    } else {
                        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                        dyCur = app.state.dyArial8;
                    }
                    if (lppl->det <= detMinimal) {
                        ids = idsPlanetaryDataAvailableEstimateMineralMiningRates;
                        goto ShowString;
                    }
                    ColonyIO().pfnEstimateMining(app, lppl, rgl, cMine, 0);
                    c = TextIO().pfnCchGetString(app, idsMiningRatePerYear, app.state.szWork);
                    win32::TextOutA(hdc, xLeft, yTopMsg, app.state.szWork, c);
                    yTopMsg += dyCur;
                    dxRight = xLeft;
                    for (i = 0; i < 3; i++) {
                        win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                        c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, rgl[i]);
                        DxStreamTextOut(hdc, &dxRight, yTopMsg, app.state.szWork, c, 1);
                        win32::SetTextColor(hdc, app.lpui->crButtonText);
                        DxStreamTextOut(hdc, &dxRight, yTopMsg, "kT  ", 4, 1);
                    }
                    goto DoneMine;
                }
                ids = idsNoteCanMineUninhabitedPlanets;
            } else {
                ids = idsWarningFleetContainsShipsRemoteMiningModules;
            }
            if (app.state.dyArial8 > 14) {
                win32::SelectObject(hdc, app.lpui->rghfontArial6[0]);
            } else {
                win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            }
        ShowString:
            win32::SetTextColor(hdc, ids == 229 ? app.lpui->crButtonText : 127);
            psz = TextIO().pfnCompressed(app, ids);
            win32::SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            win32::DrawTextA(hdc, psz, strlen(psz), &rc, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
        DoneMine:
            win32::SetTextColor(hdc, app.lpui->crButtonText);
        }
        return;
    }
    return;
}

void DrawShipPlanet(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    int16_t yTop;
    int16_t dy;
    int16_t i;
    int16_t xRight;
    char   *psz;
    int16_t dx;
    int16_t xLeft;
    RECT    rc;
    THING  *lpth;
    THING  *lpthMac;

    if (obj.pfl()->idPlanet != idPlanetDeepSpace) {
        psz = TextIO().pfnPlanetName(app, obj.pfl()->idPlanet | 0x8000);
    } else {
        psz = TextIO().pfnCompressed(app, idsDeepSpace2);
    }
    if (ptile->fFixCtls) {
        ui::ShowWindow(app.lpui->rghwndBtn[3], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[7], win32::SW_HIDE);
        ptile->fFixCtls = 0;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, psz)) {
        ui::ShowWindow(app.lpui->rghwndBtn[3], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[7], win32::SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!app.state.gd.fSmallTileMode ? 4 : 1) + rc.top;
        dx = static_cast<int16_t>((xRight - xLeft - 16)) / 3;
        dy = 3 * app.state.dyArial8 >> 1;
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[3], obj.pfl()->idPlanet != idPlanetDeepSpace && app.state.sel.pl.iPlayer == app.state.idPlayer);
        ControlIO().pfnSetWindowText(app.lpui->rghwndBtn[7], TextIO().pfnCompressed(app, obj.pfl()->idPlanet == idPlanetDeepSpace ? idsJettison2 : idsXFer));
        if (obj.pfl()->idPlanet == idPlanetDeepSpace) {
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac && (lpth->ith() != ithMineralPacket || obj.pfl()->pt.x != lpth->pt().x() || obj.pfl()->pt.y != lpth->pt().y()); lpth++) {
            }
            ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[7], lpth == lpthMac);
        } else {
            ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[7], 1);
        }
        if (!ptile->fMinDraw) {
            i = 3;
            while (i <= 7) {
                ControlIO().pfnSetWindowPos(app.lpui->rghwndBtn[i], nullptr, xLeft, yTop, dx, dy, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
                ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_SHOW);
                i += 4;
                xLeft += dx * 2 + 16;
            }
        }
    }
    return;
}

void DrawShipCargo(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int32_t l2;
    int16_t yTop;
    int16_t i;
    int16_t c;
    FLEET  *pfl;
    int16_t xRight;
    RECT    rcGauge;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl();
    if (ptile->fFixCtls) {
        app.lpui->rgrcRef[2].top = -5;
        app.lpui->rgrcRef[2].bottom = -6;
        app.lpui->rgrcRef[3].top = -5;
        app.lpui->rgrcRef[3].bottom = -6;
    }
    if (FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsFuelCargo))) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 1;
        dxRight = app.lpui->dxMaxMineralQuan;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = TextIO().pfnCchGetString(app, idsCargo3, app.state.szWork);
        l = GetTextExtent(hdc, app.state.szWork, c);
        c = TextIO().pfnCchGetString(app, idsFuel3, app.state.szWork);
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        if (!ptile->fMinDraw) {
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + app.state.dyArial8);
        app.lpui->rgrcRef[2] = rcGauge;
        DrawFleetGauge(app, hdc, &rcGauge, pfl, 4);
        yTop += (!app.state.gd.fSmallTileMode ? 4 : 2) + app.state.dyArial8;
        if (!ptile->fMinDraw) {
            c = TextIO().pfnCchGetString(app, idsCargo3, app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + app.state.dyArial8);
        app.lpui->rgrcRef[3] = rcGauge;
        DrawFleetGauge(app, hdc, &rcGauge, pfl, 5);
        yTop += app.state.dyArial8 + 4;
        if (!app.state.gd.fSmallTileMode) {
            for (i = 0; i <= 2; i++) {
                if (!ptile->fMinDraw) {
                    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                    win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                    win32::TextOutA(hdc, xLeft, yTop, app.state.rgszMinerals[i], win32::lstrlenA(app.state.rgszMinerals[i]));
                }
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                win32::SetTextColor(hdc, app.lpui->crButtonText);
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdkt), pfl->rgwtMin[i]);
                RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
                yTop += app.state.dyArial8;
            }
            if (!ptile->fMinDraw) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                win32::SetTextColor(hdc, 0xffffff);
                c = TextIO().pfnCchGetString(app, idsColonists2, app.state.szWork);
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                win32::SetTextColor(hdc, app.lpui->crButtonText);
            }
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdkt), pfl->rgwtMin[3]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
        }
    }
    return;
}

void DrawFleetComp(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    int32_t cBoat;
    int16_t swp;
    int16_t dxRight;
    int16_t yTop;
    RECT    rcT;
    int16_t dyWrong;
    int16_t c;
    int16_t i;
    FLEET  *pfl;
    int16_t xStart;
    int16_t xRight;
    int16_t dxLabel;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl();
    if (ptile->fFixCtls) {
        ui::ShowWindow(app.lpui->hwndFleetCompLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[8], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[9], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[10], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndBattleDD, win32::SW_HIDE);
        ptile->fFixCtls = 0;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsFleetComposition))) {
        ui::ShowWindow(app.lpui->hwndFleetCompLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[8], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[9], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[10], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndBattleDD, win32::SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 3;
        ControlIO().pfnGetClientRect(app.lpui->hwndFleetCompLB, &rcT);
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        app.lpui->dyFleetCompLB = (app.state.dyArial8 + 2) * (!app.state.gd.fSmallTileMode ? 5 : 3);
        dyWrong = app.lpui->dyFleetCompLB - (rcT.bottom - rcT.top);
        if (app.lpui->dxFleetCompLB == xRight - xLeft && dyWrong >= 0 && dyWrong < app.state.dyArial8) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->dxFleetCompLB = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndFleetCompLB, nullptr, xLeft, yTop, xRight - xLeft, app.lpui->dyFleetCompLB, swp);
        ui::ShowWindow(app.lpui->hwndFleetCompLB, win32::SW_SHOW);
        ControlIO().pfnGetClientRect(app.lpui->hwndFleetCompLB, &rcT);
        app.lpui->dyFleetCompLB = rcT.bottom - rcT.top;
        yTop += (!app.state.gd.fSmallTileMode ? 4 : 2) + app.lpui->dyFleetCompLB;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = TextIO().pfnCchGetString(app, idsBattlePlan2, app.state.szWork);
        dxLabel = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
        if (!ptile->fMinDraw) {
            win32::TextOutA(hdc, xLeft, yTop + 4, app.state.szWork, c);
        }
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        if (app.lpui->dxBattleDD == xRight - xLeft - dxLabel) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->dxBattleDD = xRight - xLeft - dxLabel;
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndBattleDD, nullptr, xLeft + dxLabel, yTop, app.lpui->dxBattleDD, 5 * app.lpui->dyShipDD, swp);
        ui::ShowWindow(app.lpui->hwndBattleDD, win32::SW_SHOW);
        yTop += app.lpui->dyShipDD + 3;
        c = TextIO().pfnCchGetString(app, idsEstRange, app.state.szWork);
        l = GetTextExtent(hdc, app.state.szWork, c);
        if (!ptile->fMinDraw) {
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        c = TextIO().pfnCchGetString(app, idsN9999LY, app.state.szWork);
        dxRight = LOWORD(GetTextExtent(hdc, app.state.szWork, c)) + 6;
        i = IFindIdealWarp(app, nullptr, 0);
        l = EstFuelUse(app, &app.state.sel.fl, 0, i, -1, 1);
        if (l >= 1000000000) {
            c = TextIO().pfnCchGetString(app, idsInfinite, app.state.szWork);
        } else {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdLY), l);
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        if (!app.state.gd.fSmallTileMode) {
            yTop += app.state.dyArial8;
            if (!ptile->fMinDraw) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                c = TextIO().pfnCchGetString(app, idsPercentCloaked, app.state.szWork);
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            i = PctCloakFromLpfl(app, &app.state.sel.fl);
            if (i == 0) {
                c = TextIO().pfnCchGetString(app, idsNone2, app.state.szWork);
            } else {
                c = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, i);
            }
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        }
        yTop += app.state.dyArial8 + 2 - app.state.gd.fSmallTileMode;
        xStart = xLeft;
        c = static_cast<int16_t>((xRight - xLeft - 10)) / 3;
        i = 8;
        while (i <= 10) {
            ControlIO().pfnSetWindowPos(app.lpui->rghwndBtn[i], nullptr, xStart, yTop, c, (app.state.dyArial8 >> 1) + app.state.dyArial8, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_SHOW);
            i++;
            xStart += c + 6;
        }
        cBoat = 0;
        for (i = 0; i < 16; i++) {
            cBoat += app.state.sel.fl.rgcsh[i];
        }
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[8], FCanSplit(app, cBoat));
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[9], FCanSplitAll(app, cBoat));
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[10], FCanMerge(app, pfl));
    }
    return;
}

void ShipCommandProc(APPSESSION &app, HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int16_t fPercent;
    FARPROC lpProc;
    int32_t lSel;
    XFER    xf;
    char    szT[34];
    char    szFleetName[80];
    int16_t ishdef;
    int16_t grbit;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t rgifl[512];
    int16_t ish;
    int16_t ishPrimary;
    FLEET  *lpflBest;

    fPercent = 0;
    if (HIWORD(wParam) == 0) {
        ControlIO().pfnSetFocus(app.lpui->hwndFrame);
    }
    if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[4] && HIWORD(wParam) == 0) {
        PresentationIO().pfnSelectFleet(app, -1, 0);
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[5] && HIWORD(wParam) == 0) {
        PresentationIO().pfnSelectFleet(app, 1, 0);
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[6] && HIWORD(wParam) == 0) {
        CchSprintf(szFleetName, "%s", PszGetFleetName(app, app.state.sel.fl.id));
        strcpy(app.state.szWork, szFleetName);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRenameDlg, 0);
        lpProc = (RenameDlg);
        if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RENAME), app.lpui->hwndFrame, lpProc, 0) != 0) {
            strcpy(szT, app.state.szWork);
            if (strcmp(szT, PszGetFleetName(app, app.state.sel.fl.id)) != 0) {
                LogChangeName(app, grobjFleet, app.state.sel.fl.id, szT);
                PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
                FillOrdersLB(app);
                ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileFleetOrders | tileBitmap | tileErase));
                ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
                ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
                SetMineralTitleBar(app, app.lpui->hwndMine);
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[3] && HIWORD(wParam) == 0) {
        PresentationIO().pfnSelectPlanet(app, 0, app.state.sel.fl.idPlanet);
        SetFleetDropDownSel(app, app.state.sel.fl.id);
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[7] && HIWORD(wParam) == 0) {
        if (app.state.sel.fl.idPlanet != idPlanetDeepSpace) {
            TransferStuff(app, app.state.sel.fl.id, grobjFleet, app.state.sel.fl.idPlanet, grobjPlanet, mdXferCargo);
        } else {
            TransferStuff(app, app.state.sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndShipDD) {
        if (HIWORD(wParam) == 1) {
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileShipList | tileErase));
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndShipLB) {
        if (HIWORD(wParam) == 1) {
            lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_GETCURSEL, 0, 0);
            SetScanWp(app, LOWORD(lSel));
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndFleetCompLB) {
        if (HIWORD(wParam) == 1) {
            lSel = ControlIO().pfnSendMessage(app.lpui->hwndFleetCompLB, win32::LB_GETCURSEL, 0, 0);
            if (lSel >= 0) {
                for (ishdef = 0; ishdef < 16 && (app.state.sel.fl.rgcsh[ishdef] <= 0 || lSel-- > 0); ishdef++) {
                }
                app.lpui->GlobalPD.grPopup = grPopupShdef;
                app.lpui->GlobalPD.lpshdef = &app.state.rgshdef[ishdef];
                app.lpui->GlobalPD.fHideCounts = 0;
                app.lpui->GlobalPD.fShowDamage = 1;
                app.lpui->GlobalPD.SetFToken(0);
                app.lpui->GlobalPD.fSummary = 0;
                Popup(app, app.lpui->hwndFleetCompLB, 10, 10);
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndBattleDD) {
        if (HIWORD(wParam) == 1) {
            lSel = ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::CB_GETCURSEL, 0, 0);
            if (lSel != -1) {
                if (lSel == 0) {
                    lpProc = (BattlePlansDlg);
                    lSel = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_BATTLE_PLANS), app.lpui->hwndFrame, lpProc, 0);
                    app.state.sel.fl.iplan = lSel;
                } else {
                    app.state.sel.fl.iplan = lSel - 1;
                }
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[0] && HIWORD(wParam) == 0) {
        lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, app.state.sel.fl.id)) {
            TransferStuff(app, app.state.sel.fl.id, grobjFleet, xf.id, xf.grobj, mdXferCargo);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[1] && HIWORD(wParam) == 0) {
        lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, app.state.sel.fl.id) && xf.grobj == grobjFleet) {
            PresentationIO().pfnSelectFleet(app, 0, xf.id);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[2] && HIWORD(wParam) == 0) {
        lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, app.state.sel.fl.id) && xf.grobj == grobjFleet) {
            TransferStuff(app, app.state.sel.fl.id, grobjFleet, xf.id, grobjFleet, mdXferShips);
            if (app.lpui->grbitScan & grbitScanFleetPaths) {
                ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            }
            PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[8] && HIWORD(wParam) == 0) {
        TransferStuff(app, app.state.sel.fl.id, grobjFleet, -1, grobjOther, mdXferShips);
        PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[9] && HIWORD(wParam) == 0) {
        FFleetSplitAll(app, &app.state.sel.fl);
        FillShipDD(app, app.state.sel.fl.id);
        grbit = -31819;
        FleetQueryIO().pfnLookup(app, app.state.sel.fl.id, &app.state.sel.fl);
        FillFleetCompLB(app);
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(grbit));
        ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[10] && HIWORD(wParam) == 0) {
        app.state.vrgiflMerge = rgifl;
        app.state.vcflMerge = 0;
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
            if (lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead && lpfl->pt.x == app.state.sel.fl.pt.x && lpfl->pt.y == app.state.sel.fl.pt.y) {
                rgifl[app.state.vcflMerge++] = ifl;
            }
        }
        lpfl = nullptr;
        lpProc = (MergeFleetsDlg);
        if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_MERGE_FLEETS), app.lpui->hwndFrame, lpProc, 0) != 0) {
            for (ifl = 0; ifl < app.state.vcflMerge; ifl++) {
                if (app.state.vrgiflMerge[ifl] != iflNone) {
                    if (app.state.rglpfl[app.state.vrgiflMerge[ifl]]->id == app.state.sel.fl.id) {
                        lpfl = app.state.rglpfl[app.state.vrgiflMerge[ifl]];
                    }
                    app.state.vrgiflMerge[ifl] = app.state.rglpfl[app.state.vrgiflMerge[ifl]]->id;
                }
            }
            for (ifl = 0; ifl < app.state.vcflMerge; ifl++) {
                if (app.state.vrgiflMerge[ifl] != iflNone) {
                    if (!lpfl) {
                        lpfl = LpflFromId(app, app.state.vrgiflMerge[ifl]);
                    } else if (app.state.vrgiflMerge[ifl] != lpfl->id) {
                        break;
                    }
                }
            }
            if (ifl == app.state.vcflMerge) {
                lpfl = nullptr;
            } else if (lpfl->id != app.state.sel.fl.id) {
                PresentationIO().pfnSelectFleet(app, 0, lpfl->id);
            }
        }
        if (lpfl) {
            FFleetMergeAll(app, &app.state.sel.fl);
            FillShipDD(app, app.state.sel.fl.id);
            grbit = -31819;
            FleetQueryIO().pfnLookup(app, app.state.sel.fl.id, &app.state.sel.fl);
            FillFleetCompLB(app);
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(grbit));
            ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
            if (app.lpui->grbitScan & grbitScanFleetPaths) {
                ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            }
            app.state.vrgiflMerge = 0;
            app.state.vcflMerge = 0;
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndRepCB && HIWORD(wParam) == 0) {
        app.state.sel.fl.fRepOrders = LOWORD(ControlIO().pfnSendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0));
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndOrderDD[0]) {
        if (HIWORD(wParam) == 1) {
            lSel = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETCURSEL, 0, 0);
            if (LOWORD(lSel) != app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask) {
                if (lSel == 3 || app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask == grTaskMine) {
                    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
                }
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask = LOWORD(lSel);
                memset(reinterpret_cast<uint8_t *>(&app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct]) + 8, 0, 10);
                switch (LOWORD(lSel)) {
                case 7:
                    app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tptl().iDist = 0;
                    app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tptl().iWarp = 0;
                    break;
                case 9:
                    app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX = 0;
                    break;
                case 6:
                    app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX = 5;
                    break;
                case 4:
                    if (app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grobj != grobjFleet) {
                        lpflBest = nullptr;
                        ishPrimary = 0;
                        for (ish = 1; ish < 16; ish++) {
                            if (app.state.sel.fl.rgcsh[ish] > app.state.sel.fl.rgcsh[ishPrimary]) {
                                ishPrimary = ish;
                            }
                        }
                        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                            lpfl = app.state.rglpfl[ifl];
                            if (!app.state.rglpfl[ifl])
                                break;
                            if (lpfl->pt.x == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt.x && lpfl->pt.y == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt.y &&
                                lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead && lpfl->id != app.state.sel.fl.id) {
                                if (lpfl->rgcsh[ishPrimary] > 0) {
                                    lpflBest = lpfl;
                                    if (lpfl->cord == 1)
                                        break;
                                } else if (!lpflBest || (lpflBest->cord > 1 && lpfl->cord == 1)) {
                                    lpflBest = lpfl;
                                }
                            }
                        }
                        if (lpflBest) {
                            app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grobj = grobjFleet;
                            app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].id = lpflBest->id;
                            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                            FillOrdersLB(app);
                        }
                    }
                }
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                ControlIO().pfnUpdateOrdersDDs(app, 1);
                ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileStarbaseOrWaypoint | tileErase));
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndOrderDD[1]) {
        if (HIWORD(wParam) == 1) {
            lSel = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0);
            switch (app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask) {
            case grTaskPatrol:
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tptl().iDist = LOWORD(lSel);
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                ControlIO().pfnUpdateOrdersDDs(app, 1);
                break;
            case grTaskGive:
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX = LOWORD(lSel);
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                ControlIO().pfnUpdateOrdersDDs(app, 1);
                break;
            case grTaskXfer:
                ControlIO().pfnUpdateOrdersDDs(app, 2);
                ControlIO().pfnDrawPlanShip(app, nullptr, tileStarbaseOrWaypoint);
                break;
            default:
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tlm().cTime = LOWORD(lSel);
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tlm().cTimeOld = LOWORD(lSel);
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            }
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndOrderDD[2]) {
        if (HIWORD(wParam) == 1) {
            ChangeTransportAction(app);
        }
    } else if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndOrderED && HIWORD(wParam) == 768) {
        ChangeTransportAmount(app);
    }
    return;
}

void DrawFleetGauge(APPSESSION &app, HDC hdc, RECT *prc, FLEET *lpfl, int16_t grbit) {
    HBRUSH  rghbr[5];
    int32_t lMax;
    int16_t c;
    int16_t i;
    int32_t rgSize[5];
    int16_t iMode;
    int16_t cSections;
    int32_t l;

    if (!lpfl) {
        lpfl = &app.state.sel.fl;
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    cSections = 1;
    lMax = ProductionIO().pfnFleetStat(app, lpfl, 2);
    if (grbit >= 0 && grbit <= 4) {
        rghbr[0] = app.lpui->rghbrMineral[grbit];
        rgSize[0] = lpfl->rgwtMin[grbit];
        if (grbit == 4) {
            lMax = ProductionIO().pfnFleetStat(app, lpfl, 1);
        }
    } else {
        switch (grbit) {
        case 5:
            for (i = 0; i <= 3; i++) {
                rghbr[i] = app.lpui->rghbrMineral[i];
                rgSize[i] = lpfl->rgwtMin[i];
            }
            cSections = 4;
            break;
        case 6:
            lMax = 11;
            rgSize[0] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].iWarp;
            if (rgSize[0] > 10 || (rgSize[0] == 10 && IFindIdealWarp(app, &app.state.sel.fl, 0) < 10)) {
                rghbr[0] = app.lpui->rghbrMineral[2];
                break;
            }
            rghbr[0] = app.lpui->rghbrMineral[4];
            break;
        case 7:
            lMax = 10;
            rgSize[0] = static_cast<uint32_t>(app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX);
            if (rgSize[0] > 10 || (rgSize[0] == 10 && IFindIdealWarp(app, &app.state.sel.fl, 0) < 10)) {
                rghbr[0] = app.lpui->rghbrMineral[2];
            } else {
                rghbr[0] = app.lpui->rghbrMineral[4];
            }
        }
    }
    l = LDrawGauge(app, hdc, prc, cSections, rgSize, rghbr, lMax);
    iMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (grbit == 6) {
        if (l != 0) {
            if (l < 11) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpLd), l);
            } else {
                c = TextIO().pfnCchGetString(app, idsUseStargate, app.state.szWork);
            }
        } else {
            c = TextIO().pfnCchGetString(app, idsStopped, app.state.szWork);
        }
    } else if (grbit == 7) {
        if (l != 0) {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpLd), l);
        } else {
            c = TextIO().pfnCchGetString(app, idsAutomatic, app.state.szWork);
        }
    } else if (cSections == 1 && grbit != 4) {
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdkt), l);
    } else if (grbit == 4) {
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdLdmg), l, lMax);
    } else {
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdLdkt), l, lMax);
    }
    l = GetTextExtent(hdc, app.state.szWork, c);
    if (static_cast<int16_t>(LOWORD(l)) < prc->right - prc->left - 3) {
        RcCtrTextOut(app, hdc, prc, app.state.szWork, c);
    }
    win32::SetBkMode(hdc, iMode);
    return;
}

void DrawFleetBitmap(APPSESSION &app, FLEET *lpfl, HDC hdc, int16_t x, int16_t y, int16_t fFrame, int16_t ibmp, int16_t cDiff, int16_t fShrink, int16_t ibmpRace, int16_t csh) {
    int16_t dxyPlus;
    int16_t yCur;
    int16_t c;
    int16_t i;
    int16_t dxy;
    int16_t dx;
    int16_t xCur;
    int16_t dxyPlusWidth;
    HBRUSH  hbrSav;

    if (fFrame) {
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonShadow));
        win32::PatBlt(hdc, x, y, 68, 2, win32::PATCOPY);
        win32::PatBlt(hdc, x, y, 2, 68, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, x + 2, y + 66, 66, 1, win32::PATCOPY);
        win32::PatBlt(hdc, x + 3, y + 66, 65, 1, win32::PATCOPY);
        win32::PatBlt(hdc, x + 66, y + 2, 1, 64, win32::PATCOPY);
        win32::PatBlt(hdc, x + 67, y + 1, 1, 65, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        x += 2;
        y += 2;
    }
    if (ibmp < 0) {
        i = IshdefPrimaryFromLpfl(app, lpfl, &cDiff);
        ibmp = app.state.rglpshdef[lpfl->iPlayer][i].hul.ibmp;
    }
    ibmp %= 148;
    win32::SelectPalette(hdc, app.lpui->vhpal, 0);
    win32::RealizePalette(hdc);
    if (fShrink) {
        DibBlt(hdc, x, y, 32, 32, app.lpui->rghdibShipsT[ibmp >> 5], ((ibmp & 0x1f) >> 2) * 0x20, (3 - (ibmp & 3)) * 0x20, 32, 32, 13369376);
        dxy = 32;
        dxyPlus = 5;
        dxyPlusWidth = 1;
        if (ibmpRace >= 0) {
            DibBlt(hdc, x, y + 24, 8, 8, app.lpui->hdibRacesX, (ibmpRace & 7) * 8, (3 - ((ibmpRace & 0x1f) >> 3)) * 8, 8, 8, 13369376);
        }
    } else {
        DibBlt(hdc, x, y, 64, 64, app.lpui->rghdibShips[ibmp >> 5], ((ibmp & 0x1f) >> 2) * 0x40, (3 - (ibmp & 3)) * 0x40, 64, 64, 13369376);
        dxy = 64;
        dxyPlus = 8;
        dxyPlusWidth = 2;
        if (ibmpRace >= 0) {
            DibBlt(hdc, x, y + 48, 16, 16, app.lpui->hdibRacesT, (ibmpRace & 7) * 0x10, (3 - ((ibmpRace & 0x1f) >> 3)) * 0x10, 16, 16, 13369376);
        }
    }
    if (csh == 0 || fShrink) {
        if (cDiff > 4) {
            cDiff = 4;
        }
        cDiff--;
        for (i = 0; i < cDiff; i++) {
            xCur = ((i & 1) ^ ((i & 2) == 2)) == 0 ? x + 2 : x + dxy - 2 - dxyPlus;
            yCur = !(i & 2) ? y + 2 : y + dxy - 2 - dxyPlus;
            win32::PatBlt(hdc, xCur, static_cast<int16_t>((dxyPlus - 1)) / 2 + yCur, dxyPlus, dxyPlusWidth, win32::WHITENESS);
            win32::PatBlt(hdc, static_cast<int16_t>((dxyPlus - 1)) / 2 + xCur, yCur, dxyPlusWidth, dxyPlus, win32::WHITENESS);
        }
    } else {
        win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
        win32::SetTextColor(hdc, 0xffffff);
        if (cDiff > 1) {
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, cDiff);
            win32::TextOutA(hdc, x + 1, y + 1, app.state.szWork, c);
        }
        if (csh > 1) {
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, csh);
            dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            win32::TextOutA(hdc, x + dxy - 1 - dx, y + 1, app.state.szWork, c);
        }
    }
    return;
}

int16_t TransferStuff(APPSESSION &app, int16_t id1, GrobjClass grobj1, int16_t id2, GrobjClass grobj2, MdXfer mdXfer) {
    XFER    xfer[2];
    FARPROC lpProcXfer;
    int16_t rgValidHull[16];
    int32_t lPopPrev;
    int16_t iDelFleet;
    int16_t i;
    FLEET  *lpfl;
    int16_t fSuccess;
    int16_t grbit;
    int16_t j;
    BTN     rgbtn[32];
    POINT16 pt;
    RECT    rc;

    lPopPrev = -1;
    xfer[0].id = id1;
    xfer[0].grobj = grobj1;
    xfer[1].id = id2;
    xfer[1].grobj = grobj2;
    XFER *pxferSav = app.state.pxfer;
    MdXfer mdXferSav = app.lpui->mdXferDlg;
    NATIVECLEANUPSCOPE transferRoot([pxferSav, mdXferSav, &app]() noexcept { app.state.pxfer = pxferSav; app.lpui->mdXferDlg = mdXferSav; });
    app.state.pxfer = xfer;
    app.lpui->mdXferDlg = mdXfer;
    for (i = 0; i < 2; i++) {
        if (xfer[i].grobj != grobjOther) {
            if (!FLookupObject(app, xfer[i].grobj, xfer[i].id, xfer[i].LpvLookup(xfer[i].grobj))) {
                return 0;
            }
            if (xfer[i].grobj == grobjPlanet) {
                lPopPrev = xfer[i].pl().rgwtMin[3];
            }
        } else if (mdXfer == mdXferShips) {
            lpfl = LpflNewSplit(app, &xfer[0].fl());
            xfer[1].fl() = *lpfl;
            xfer[1].id = lpfl->id;
            xfer[1].grobj = grobjFleet;
        } else {
            xfer[i].SetIdObj(idflNone);
            for (j = 0; j < 4; j++) {
                xfer[i].pl().rgwtMin[j] = 0;
            }
            EnumLogRts(app, reinterpret_cast<int16_t (*)(APPSESSION &, void *, int16_t, int16_t, void *, int16_t)>(FEnumCalcJettison), &xfer[i].pl(), id1);
        }
    }
    if (mdXfer == mdXferShips) {
        app.lpui->cXferValidHulls = 0;
        for (i = 0; i < 16; i++) {
            if (xfer[0].fl().rgcsh[i] != 0 || (xfer[1].grobj == grobjFleet && xfer[1].fl().rgcsh[i] != 0)) {
                rgValidHull[app.lpui->cXferValidHulls++] = i;
            }
        }
        app.lpui->rgXferValidHulls = rgValidHull;
    }
    app.lpui->rgbtnXfer = rgbtn;
    app.lpui->crgbtnXfer = 32;
    if (app.state.gd.fTutorial) {
        PresentationIO().pfnTutor(app);
    }
    lpProcXfer = (TransferDlg);
    fSuccess = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_TRANSFER), app.lpui->hwndFrame, lpProcXfer, 0);
    if (fSuccess) {
        iDelFleet = -1;
        if (mdXfer == mdXferShips) {
            for (i = 0; i < 16 && xfer[1].fl().rgcsh[i] <= 0; i++) {
            }
            if (i == 16 && grobj2 == grobjOther)
                goto CancelSplit;
            FleetTransferCargoBalance(app, &xfer[0].fl(), &xfer[1].fl());
        }
        for (i = 0; i < 2; i++) {
            switch (xfer[i].grobj) {
            default:
                break;
            case grobjPlanet:
            case grobjOther:
                PlanetQueryIO().pfnLookup(app, idWriteBack, &xfer[i].pl());
                if (xfer[i].grobj != grobjPlanet || lPopPrev != xfer[i].pl().rgwtMin[3])
                    break;
                lPopPrev = -1;
                break;
            case grobjFleet:
                FleetQueryIO().pfnLookup(app, idWriteBack, &xfer[i].fl());
                if (mdXfer != mdXferShips)
                    break;
                for (j = 0; j < 16 && xfer[i].fl().rgcsh[j] == 0; j++) {
                }
                if (j != 16)
                    break;
                iDelFleet = i;
                break;
            case grobjThing:
                FLookupThing(app, idWriteBack, &xfer[i].th());
            }
        }
        if (iDelFleet != -1) {
            FDeleteFleet(app, xfer[iDelFleet].idObj(), grobjFleet, xfer[iDelFleet == 0].idObj());
        }
        if (mdXfer == mdXferShips) {
            FillShipDD(app, app.state.sel.fl.id);
            if (app.lpui->grbitScan & grbitScanFleetPaths) {
                ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            }
        }
        if (app.state.sel.grobj == grobjPlanet) {
            grbit = -32691;
            PlanetQueryIO().pfnLookup(app, app.state.sel.pl.id, &app.state.sel.pl);
            ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, nullptr);
        } else {
            grbit = -31819;
            FleetQueryIO().pfnLookup(app, app.state.sel.fl.id, &app.state.sel.fl);
            FillFleetCompLB(app);
        }
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(grbit));
        if (app.state.sel.scan.grobj == grobjFleet) {
            ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
        } else {
            InvalidateMineralBars(app);
        }
        if (lPopPrev != -1 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh && (app.lpui->grbitScan & grbitScanCoverage)) {
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 0);
        } else if (lPopPrev != -1 && (app.lpui->grbitScan & grbitScanViewMask) == 4) {
        LInvalScanPlan:
            pt = app.state.sel.pt;
            LogicalToScan(app, &pt);
            rc.right = pt.x;
            rc.bottom = pt.y;
            rc.left = pt.x;
            rc.top = pt.y;
            win32::InflateRect(&rc, 20, 20);
            rc.top -= 20;
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, &rc, 0);
        } else if ((app.lpui->grbitScan & grbitScanViewMask) == 1) {
            goto LInvalScanPlan;
        }
    } else if (mdXfer == mdXferShips && grobj2 == grobjOther) {
    CancelSplit:
        FDeleteFleet(app, xfer[1].idObj(), grobjNone, 0);
        CancelMemRt(app, rtLogFleetSplit);
    }
    app.lpui->mdXferDlg = mdXferNone;
    return 0;
}

INT_PTR  TransferDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    int16_t     dyMore;
    PAINTSTRUCT ps;
    POINT16     pt;
    HWND        hwndBtn;
    RECT        rcBtn;
    int16_t     dx;
    RECT        rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyTransferDlg, 1);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (app.lpui->mdXferDlg == mdXferShips) {
            ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsShipTransfer));
            if (app.lpui->cXferValidHulls > 10) {
                dyMore = rc.bottom / 2;
                rc.bottom += dyMore;
                ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, rc.right, rc.bottom, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
                ControlIO().pfnGetClientRect(hwnd, &rc);
                dyMore -= win32::GetSystemMetrics(win32::SM_CYCAPTION) + 2;
                dx = win32::GetSystemMetrics(win32::SM_CXDLGFRAME) + 4;
                hwndBtn = ControlIO().pfnGetDlgItem(hwnd, IDOK);
                win32::GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                ControlIO().pfnSetWindowPos(hwndBtn, nullptr, pt.x, pt.y, 0, 0, win32::SWP_NOSIZE);
                hwndBtn = ControlIO().pfnGetDlgItem(hwnd, IDC_HELP);
                win32::GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                ControlIO().pfnSetWindowPos(hwndBtn, nullptr, pt.x, pt.y, 0, 0, win32::SWP_NOSIZE);
                hwndBtn = ControlIO().pfnGetDlgItem(hwnd, IDCANCEL);
                win32::GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                ControlIO().pfnSetWindowPos(hwndBtn, nullptr, pt.x, pt.y, 0, 0, win32::SWP_NOSIZE);
            }
        }
        FSetupXferBtns(app, &rc);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawXferDlg(app, hwnd, hdc, &rc, SupplyAll);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        return FTrackXfer(app, hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyTransferDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<uint32_t>((app.lpui->mdXferDlg == mdXferShips ? 1080 : 1075)));
            return 1;
        }
        /* fallthrough */
    default:
        return 0;
    }
}

int16_t FTrackXfer(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t fkb) {
    POINT16 ptOld;
    POINT16 pt;
    int32_t dChg;
    BTNT    btnt;
    int32_t cCur;
    int16_t i;
    int16_t iBtn;
    int16_t iVal;
    BTN     btn;
    int32_t cNew;
    RECT    rc, rcBtn;

    ControlIO().pfnGetClientRect(hwnd, &rc);
    pt.x = x;
    pt.y = y;
    for (i = 0; i < app.lpui->crgbtnXfer; i++) {
        /* NATIVE: Packed BTN arrays can misalign their Win32 RECT members. */
        memcpy(&rcBtn, &app.lpui->rgbtnXfer[i].rc, sizeof(rcBtn));
        if (!(app.lpui->rgbtnXfer[i].bt & 4) && win32::PtInRect(&rcBtn, PointFrom16(pt))) break;
    }
    if (i == app.lpui->crgbtnXfer) {
        return 0;
    }
    iBtn = i >> 1;
    btn = app.lpui->rgbtnXfer[i];
    iVal = btn.iVal & 0x7f;
    if (!btn.fVisible) {
        if (iVal > 4)
            goto FinishUp;
        if (app.state.pxfer[1].grobj == grobjThing) {
            if (iVal == 4 || iVal == 3)
                goto FinishUp;
        } else if (app.state.pxfer[btn.iSide].iPlayerObj() != app.state.idPlayer) {
            goto FinishUp;
        }
        win32::SetCapture(hwnd);
        ptOld.y = -1;
        ptOld.x = -1;
        while (FGetMouseMove(&pt)) {
            if (pt.x != ptOld.x || pt.y != ptOld.y) {
                ptOld = pt;
                if (btn.iSide == 1 && app.state.pxfer[1].grobj == grobjThing) {
                    cNew = static_cast<uint32_t>((app.state.pxfer[1].th().thp().wtMax() * 10));
                } else if (iVal == 4) {
                    cNew = ProductionIO().pfnFleetStat(app, &app.state.pxfer[btn.iSide].fl(), 1);
                } else {
                    cNew = ProductionIO().pfnFleetStat(app, &app.state.pxfer[btn.iSide].fl(), 2);
                }
                cNew = static_cast<int32_t>((static_cast<int16_t>((pt.x - btn.rc.left)) * cNew)) / static_cast<int16_t>((btn.rc.right - btn.rc.left - 2));
                cCur = ChgCargo(app, app.state.pxfer[btn.iSide].grobj, app.state.pxfer[btn.iSide].id, static_cast<MineralType>(iVal), 0, app.state.pxfer[btn.iSide].LpvCargo(app.state.pxfer[btn.iSide].grobj));
                dChg = cNew - cCur;
                if (XferSupply(app, static_cast<MineralType>(iVal), btn.iSide == 0 ? dChg : -dChg) != 0) {
                    DrawXferDlg(app, hwnd, nullptr, &rc, static_cast<MineralType>(iVal));
                }
            }
        }
        win32::ReleaseCapture();
    } else {
        InitBtnTrack(&btnt, hwnd, nullptr, &btn.rc, btn.bt, 80, 0, 0, nullptr);
        if (fkb & 8) {
            dChg = static_cast<uint32_t>((!(fkb & 4) ? 100 : 1000));
        } else if (fkb & 4) {
            dChg = 10;
        } else {
            dChg = 1;
        }
        while (FTrackBtn(app, &btnt)) {
            if (app.lpui->mdXferDlg == mdXferShips) {
                i = static_cast<int16_t>(LOWORD(dChg)) < app.state.pxfer[btn.iSide == 0].fl().rgcsh[iVal] ? LOWORD(dChg) : app.state.pxfer[btn.iSide == 0].fl().rgcsh[iVal];
                if (i != 0) {
                    if (app.state.pxfer[btn.iSide].fl().rgcsh[iVal] >= 32766 - i) {
                        i = 1;
                    }
                    app.state.pxfer[btn.iSide].fl().rgcsh[iVal] = app.state.pxfer[btn.iSide].fl().rgcsh[iVal] + i;
                    app.state.pxfer[btn.iSide == 0].fl().rgcsh[iVal] -= i;
                    DrawXferDlg(app, hwnd, btnt.hdc, &rc, static_cast<MineralType>(iBtn));
                }
            } else if (iVal >= 0 && iVal <= 4 && XferSupply(app, static_cast<MineralType>(iVal), btn.iSide == 0 ? dChg : -dChg) != 0) {
                DrawXferDlg(app, hwnd, btnt.hdc, &rc, static_cast<MineralType>(iVal));
            }
        }
    }
FinishUp:
    UpdateXferBtns(app);
    DrawXferDlg(app, hwnd, nullptr, &rc, SupplyButtonsOnly);
    return 1;
}

void UpdateXferBtns(APPSESSION &app) {
    int16_t iSide;
    int16_t i;
    int16_t iLastButton;
    int16_t iVal;
    int32_t lLeft;

    iLastButton = app.lpui->mdXferDlg == mdXferShips ? app.lpui->cXferValidHulls * 2 : 4;
    for (i = 0; i < app.lpui->crgbtnXfer; i++) {
        iVal = app.lpui->rgbtnXfer[i].iVal;
        if (app.lpui->rgbtnXfer[i].fVisible && (iVal <= iLastButton || app.lpui->mdXferDlg == mdXferShips)) {
            iSide = app.lpui->rgbtnXfer[i].iSide;
            if (app.lpui->mdXferDlg == mdXferShips) {
                if (app.state.pxfer[iSide].fl().rgcsh[iVal] == 32766) {
                    lLeft = 0;
                } else {
                    lLeft = app.state.pxfer[iSide == 0].fl().rgcsh[iVal];
                }
            } else {
                lLeft = ChgCargo(app, app.state.pxfer[iSide == 0].grobj, app.state.pxfer[iSide == 0].id, static_cast<MineralType>(iVal), 0, app.state.pxfer[(iSide == 0)].LpvCargo(app.state.pxfer[(iSide == 0)].grobj));
                if (lLeft == 0 || app.state.pxfer[iSide].grobj != grobjFleet) {
                    if (app.state.pxfer[iSide].grobj == grobjPlanet && iVal == 4) {
                        lLeft = 0;
                    }
                } else if (iVal == 4) {
                    lLeft = GetFuelFree(app, &app.state.pxfer[iSide].fl());
                } else {
                    lLeft = GetCargoFree(app, &app.state.pxfer[iSide].fl());
                }
            }
            if (lLeft == 0) {
                app.lpui->rgbtnXfer[i].bt |= 4;
            } else {
                app.lpui->rgbtnXfer[i].bt &= 0xfffb;
            }
        }
    }
    return;
}

void DrawXferDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, MineralType iSupply) {
    RECT    rgrc[2];
    int16_t fCreatedDC;
    int16_t i;
    int16_t dxCtr;

    fCreatedDC = 0;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    dxCtr = prc->right >> 1;
    if (iSupply < Ironium) {
        win32::PatBlt(hdc, dxCtr, 0, 1, prc->bottom, win32::BLACKNESS);
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if (app.lpui->rgbtnXfer[i].fVisible) {
                DrawBtn(app, hdc, &app.lpui->rgbtnXfer[i].rc, app.lpui->rgbtnXfer[i].bt, 0, nullptr);
            }
        }
        if (iSupply == SupplyButtonsOnly)
            goto RelDC;
    }
    GetXferLeftRightRcs(app, prc, rgrc, &rgrc[1]);
    for (i = 0; i < 2; i++) {
        if (app.lpui->mdXferDlg == mdXferShips) {
            DrawFleetShipsXferSide(app, hdc, &rgrc[i], &app.state.pxfer[i].fl(), iSupply);
        } else {
            switch (app.state.pxfer[i].grobj) {
            default:
                break;
            case grobjFleet:
                DrawFleetCargoXferSide(app, hdc, &rgrc[i], &app.state.pxfer[i].fl(), iSupply);
                break;
            case grobjPlanet:
            case grobjOther:
                DrawPlanetXferSide(app, hdc, &rgrc[i], &app.state.pxfer[i].pl(), iSupply);
                break;
            case grobjThing:
                DrawThingXferSide(app, hdc, &rgrc[i], &app.state.pxfer[i].th(), iSupply);
            }
        }
    }
RelDC:
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

void GetXferLeftRightRcs(APPSESSION &app, RECT *prcWhole, RECT *prcLeft, RECT *prcRight) {
    win32::SetRect(prcLeft, 0, 0, prcWhole->right >> 1, prcWhole->bottom);
    ExpandRc(prcLeft, -(app.state.dyArial8 + 3) - 4, -4);
    prcLeft->left -= app.state.dyArial8 + 1;
    win32::SetRect(prcRight, prcWhole->right >> 1, 0, prcWhole->right, prcWhole->bottom);
    ExpandRc(prcRight, -(app.state.dyArial8 + 3) - 4, -4);
    prcRight->right += app.state.dyArial8 + 1;
    return;
}

int16_t FSetupXferBtns(APPSESSION &app, RECT *prc) {
    int16_t cBtn;
    int16_t iMax;
    int16_t dy;
    int16_t iMin;
    int16_t i;
    int16_t fThingXfer;
    int16_t j;
    int16_t dxCtr;
    RECT    rcRight;
    int16_t dxLabels;
    RECT    rcBtn;
    RECT    rcLeft;
    RECT    rc;

    cBtn = 0;
    dxLabels = app.lpui->mdXferDlg == mdXferShips ? 140 : 75;
    fThingXfer = app.state.pxfer[1].grobj == grobjThing;
    dxCtr = prc->right >> 1;
    dy = app.state.dyArial8 + 10;
    iMax = app.lpui->mdXferDlg == mdXferShips ? app.lpui->cXferValidHulls : 5;
    if (app.lpui->mdXferDlg != mdXferShips) {
        dy += (app.state.dyArial8 + 6) * 2;
    }
    i = 0;
    while (i < iMax) {
        if (i == 4 && app.lpui->mdXferDlg != mdXferShips) {
            dy -= (app.state.dyArial8 + 6) * 6;
        }
        win32::SetRect(&rcBtn, dxCtr - (app.state.dyArial8 + 3) + 1, dy, dxCtr + 1, app.state.dyArial8 + 3 + dy);
        for (j = 0; j < 2; j++) {
            app.lpui->rgbtnXfer[cBtn].rc = rcBtn;
            app.lpui->rgbtnXfer[cBtn].bt = j == 0 ? 2 : 3;
            if (fThingXfer && i >= 4) {
                app.lpui->rgbtnXfer[cBtn].fVisible = 0;
                app.lpui->rgbtnXfer[cBtn].rc.bottom = -100;
            } else {
                app.lpui->rgbtnXfer[cBtn].fVisible = 1;
            }
            app.lpui->rgbtnXfer[cBtn].iSide = j;
            app.lpui->rgbtnXfer[cBtn].iVal = app.lpui->mdXferDlg == mdXferShips ? app.lpui->rgXferValidHulls[i] : i;
            cBtn++;
            OffsetRc(&rcBtn, app.state.dyArial8 + 2, 0);
        }
        i++;
        dy += app.state.dyArial8 + 6;
    }
    GetXferLeftRightRcs(app, prc, &rcLeft, &rcRight);
    if (app.lpui->mdXferDlg != mdXferShips) {
        if (app.state.pxfer->grobj != grobjPlanet) {
            rc = rcLeft;
            i = 0;
        } else {
            if (app.state.pxfer[1].grobj == grobjPlanet)
                goto NoGauges;
            rc = rcRight;
            i = 1;
        }
        for (; i < 2; i++) {
            win32::SetRect(&rcBtn, rc.left + dxLabels + 10, rc.top + app.state.dyArial8 + 6, rc.right - 4, app.state.dyArial8 * 2 + rc.top + 6);
            if (app.lpui->mdXferDlg == mdXferShips) {
                iMin = 0;
                iMax = app.lpui->cXferValidHulls;
            } else {
                iMin = 0;
                iMax = 5;
            }
            for (j = iMin; j < iMax; j++) {
                if (app.lpui->mdXferDlg != mdXferShips) {
                    if (j == 0) {
                        OffsetRc(&rcBtn, 0, (app.state.dyArial8 + 6) * 2);
                    } else if (j == 4) {
                        OffsetRc(&rcBtn, 0, -(app.state.dyArial8 + 6) * 6);
                    }
                }
                app.lpui->rgbtnXfer[cBtn].rc = rcBtn;
                app.lpui->rgbtnXfer[cBtn].bt = 0;
                app.lpui->rgbtnXfer[cBtn].fVisible = 0;
                app.lpui->rgbtnXfer[cBtn].iSide = i;
                app.lpui->rgbtnXfer[cBtn].iVal = j + 128;
                cBtn++;
                OffsetRc(&rcBtn, 0, app.state.dyArial8 + 6);
            }
            if (app.state.pxfer[1].grobj == grobjPlanet)
                break;
            rc = rcRight;
        }
    }
NoGauges:
    app.lpui->crgbtnXfer = cBtn;
    UpdateXferBtns(app);
    return 1;
}

void DrawThingXferSide(APPSESSION &app, HDC hdc, RECT *prc, THING *pth, MineralType iSupply) {
    int16_t yTop;
    int16_t i;
    int16_t xRight;
    int16_t dxLabels;
    RECT    rcGauge;
    int16_t xLeft;
    RECT    rc;

    dxLabels = 75;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + app.state.dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(app, hdc, &rc, PszGetThingName(app, pth->idFull()), 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3 + (app.state.dyArial8 + 6);
    if (iSupply == SupplyAll) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        for (i = -1; i < 3; i++) {
            if (i != 4 && i != 4) {
                RightTextOut(app, hdc, xLeft + dxLabels, yTop, TextIO().pfnCompressed(app, static_cast<StringId>(i == -1 ? idsPacketShell : idsIronium + i)), 0, 0);
            }
            yTop += app.state.dyArial8 + 6;
        }
    }
    if (iSupply != Fuel && iSupply != Colonists) {
        yTop = rc.bottom + 3 + (app.state.dyArial8 + 6);
        xLeft += dxLabels + 6;
        win32::SetRect(&rcGauge, xLeft, yTop, xRight, yTop + app.state.dyArial8);
        DrawThingGauge(app, hdc, &rcGauge, pth, 5);
        for (i = 0; i < 3; i++) {
            OffsetRc(&rcGauge, 0, app.state.dyArial8 + 6);
            if (iSupply == SupplyAll || iSupply == i) {
                DrawThingGauge(app, hdc, &rcGauge, pth, i);
                if (iSupply == i)
                    break;
            }
        }
    }
    return;
}

void DrawFleetCargoXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply) {
    int16_t yTop;
    int16_t fOtherPlr;
    int16_t c;
    int16_t i;
    int16_t xRight;
    FLEET   fl;
    int16_t dxLabels;
    RECT    rcGauge;
    int16_t xLeft;
    RECT    rc;
    int16_t iMap;

    fOtherPlr = pfl->iPlayer != app.state.idPlayer;
    dxLabels = 75;
    fl = *pfl;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + app.state.dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(app, hdc, &rc, PszGetFleetName(app, fl.id), 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        for (i = 0; i < 6; i++) {
            if (i != 6 || !fOtherPlr) {
                RightTextOut(app, hdc, xLeft + dxLabels, yTop, TextIO().pfnCompressed(app, static_cast<StringId>(idsFuel + i)), 0, 0);
            }
            yTop += app.state.dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    xLeft += dxLabels + 6;
    if (fOtherPlr) {
        xRight = xLeft + app.lpui->dxMaxMineralQuan;
        win32::SetRect(&rc, xLeft - 2, yTop - 1, xLeft + app.lpui->dxMaxMineralQuan + 2, yTop + app.state.dyArial8 + 1);
        i = 0;
        while (i < 6) {
            if (i == 1) {
                OffsetRc(&rc, 0, app.state.dyArial8 + 6);
            } else {
                if (i == 0) {
                    iMap = 4;
                } else {
                    iMap = i - 2;
                }
                if (iSupply == SupplyAll || iSupply == iMap) {
                    _Draw3dFrame(app, hdc, &rc, iSupply == iMap);
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, static_cast<StringId>(idsLdkt + (iMap == 4))), fl.rgwtMin[iMap]);
                    RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, 0);
                    if (iSupply == i)
                        break;
                }
                OffsetRc(&rc, 0, app.state.dyArial8 + 6);
            }
            i++;
            yTop += app.state.dyArial8 + 6;
        }
    } else {
        win32::SetRect(&rcGauge, xLeft, yTop, xRight, yTop + app.state.dyArial8);
        if (iSupply == SupplyAll || iSupply == Fuel) {
            DrawFleetGauge(app, hdc, &rcGauge, &fl, 4);
        }
        if (iSupply != Fuel) {
            yTop += app.state.dyArial8 + 6;
            OffsetRc(&rcGauge, 0, app.state.dyArial8 + 6);
            DrawFleetGauge(app, hdc, &rcGauge, &fl, 5);
            yTop += app.state.dyArial8 + 6;
            for (i = 0; i <= 3; i++) {
                OffsetRc(&rcGauge, 0, app.state.dyArial8 + 6);
                if (iSupply == SupplyAll || iSupply == i) {
                    DrawFleetGauge(app, hdc, &rcGauge, &fl, i);
                    if (iSupply == i)
                        break;
                }
                yTop += app.state.dyArial8 + 6;
            }
        }
    }
    return;
}

void DrawFleetShipsXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply) {
    int16_t yTop;
    int16_t fOtherPlr;
    int16_t c;
    int16_t i;
    int16_t xRight;
    FLEET   fl;
    int16_t xLeft;
    RECT    rc;

    fOtherPlr = pfl->iPlayer != app.state.idPlayer;
    fl = *pfl;
    rc = *prc;
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + app.state.dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(app, hdc, &rc, PszGetFleetName(app, fl.id), 0);
    }
    xLeft = prc->right - 4 - app.lpui->dxMaxMineralQuan - 2;
    xRight = xLeft + app.lpui->dxMaxMineralQuan;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        for (i = 0; i < app.lpui->cXferValidHulls; i++) {
            RightTextOut(app, hdc, xLeft - 8, yTop, app.state.rgshdef[app.lpui->rgXferValidHulls[i]].hul.szClass, 0, 0);
            yTop += app.state.dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    win32::SetRect(&rc, xLeft - 2, yTop - 1, xLeft + app.lpui->dxMaxMineralQuan + 2, yTop + app.state.dyArial8 + 1);
    for (i = 0; i < app.lpui->cXferValidHulls; i++) {
        if (iSupply == SupplyAll || iSupply == i) {
            _Draw3dFrame(app, hdc, &rc, iSupply == i);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, pfl->rgcsh[app.lpui->rgXferValidHulls[i]]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, 0);
            if (iSupply == i)
                break;
        }
        OffsetRc(&rc, 0, app.state.dyArial8 + 6);
        yTop += app.state.dyArial8 + 6;
    }
    return;
}

void DrawPlanetXferSide(APPSESSION &app, HDC hdc, RECT *prc, PLANET *ppl, MineralType iSupply) {
    PLANET  pl;
    int16_t yTop;
    int16_t c;
    int16_t i;
    int16_t xRight;
    char   *psz;
    int16_t xLeft;
    RECT    rc;

    pl = *ppl;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + app.state.dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(app, hdc, &rc, 0);
        if (pl.id != idplNone) {
            psz = TextIO().pfnPlanetName(app, pl.id);
        } else {
            psz = TextIO().pfnCompressed(app, idsDeepSpace);
        }
        RcCtrTextOut(app, hdc, &rc, psz, 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        for (i = 0; i < 6; i++) {
            if (i > 1) {
                RightTextOut(app, hdc, xLeft + 75, yTop, TextIO().pfnCompressed(app, static_cast<StringId>(idsFuel + i)), 0, 0);
            }
            yTop += app.state.dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    xLeft += 81;
    xRight = xLeft + app.lpui->dxMaxMineralQuan + 12;
    win32::SetRect(&rc, xLeft - 2, yTop - 1, xLeft + app.lpui->dxMaxMineralQuan + 14, yTop + app.state.dyArial8 + 1);
    i = 0;
    while (i <= 4) {
        if (i == 0) {
            yTop += (app.state.dyArial8 + 6) * 2;
            OffsetRc(&rc, 0, (app.state.dyArial8 + 6) * 2);
        } else if (i == 4) {
            yTop -= (app.state.dyArial8 + 6) * 6;
            OffsetRc(&rc, 0, (app.state.dyArial8 + 6) * 6);
        }
        if ((iSupply == SupplyAll || iSupply == i) && i != 4) {
            _Draw3dFrame(app, hdc, &rc, iSupply == i);
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdkt), pl.rgwtMin[i]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, 0);
            if (iSupply == i)
                break;
        }
        OffsetRc(&rc, 0, app.state.dyArial8 + 6);
        i++;
        yTop += app.state.dyArial8 + 6;
    }
    return;
}

HCURSOR ClickInShipOrders(APPSESSION &app, POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn) {
    int32_t    lCur;
    HDC        hdc;
    PLANET     pl;
    int16_t    iWarp;
    POINT16    ptOld;
    int16_t    idPlan;
    int32_t    lMax;
    int32_t    lSel;
    int16_t    iSkip;
    int32_t    xRnd;
    int16_t    grbit;
    XFER       xf;
    int32_t    lNew;
    int16_t    irc;
    int32_t    dx;
    int32_t    lTempMin;
    int16_t    fFirst;
    int16_t    fTwoMAs;
    int32_t    lTempMax;
    int16_t    i;
    int16_t    fSep;
    int16_t    c;
    ORDER     *lpord;
    THING     *lpth;
    FLEET     *lpfl;
    int32_t   *rgid;
    int32_t    idFirst;
    int32_t    idSel;
    int16_t    iChecked;
    THING     *lpthMac;
    SCAN       scan;

    lTempMin = 0;
    irc = -1;
    if (app.state.sel.grobj == grobjNone) {
        return nullptr;
    }
    if (win32::PtInRect(&app.lpui->rgrcRef[5], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        if (fRightBtn) {
            ApplyTransportPresetPopup(app, pt);
        } else {
            app.lpui->GlobalPD.grPopup = grPopupShipOrders;
            Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
        }
    } else if (win32::PtInRect(&app.lpui->rgrcRef[12], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        if (fRightBtn) {
            iChecked = -1;
            lpord = &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct];
            FFindNearestObject(app, lpord->pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact), &scan);
            /* Room for every fleet and thing here, the planet or deep space
               and a separator. The original stopped at 100 entries, so
               objects past them couldn't be targeted (target list
               overload). */
            rgid = LpAlloc<int32_t>(app, (app.state.cFleet + app.state.cThing + 3) * sizeof(int32_t), htMisc);
            if (scan.idpl != idplNone) {
                rgid[0] = scan.idpl;
            } else {
                rgid[0] = 268435456;
            }
            rgid[1] = -1;
            c = 2;
            if (lpord->grobj == grobjPlanet || lpord->grobj == grobjOther) {
                iChecked = 0;
            }
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i])
                    break;
                if (scan.pt.x == lpfl->pt.x && scan.pt.y == lpfl->pt.y && lpfl->id != app.state.sel.fl.id) {
                    if (lpord->grobj == grobjFleet && lpord->id == lpfl->id) {
                        iChecked = c;
                    }
                    rgid[c++] = lpfl->id | 0x80000000;
                }
            }
            if (c == 2) {
                c = 1;
            }
            fSep = c == 0;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (scan.pt.x == lpth->pt().x() && scan.pt.y == lpth->pt().y()) {
                    if (!fSep) {
                        rgid[c++] = -1;
                        fSep = 1;
                    }
                    rgid[c++] = static_cast<uint32_t>(static_cast<uint16_t>(lpth->idFull())) | 0x20000000;
                }
            }
            i = ControlIO().pfnPopupMenu(app, app.lpui->hwndPlanet, pt.x, pt.y, c, rgid, nullptr, iChecked, 1);
            idFirst = rgid[0];
            idSel = i >= 0 ? rgid[i] : 0;
            HeapIO().pfnFree(app, rgid, htMisc);
            if (i >= 0) {
                if (i == 0 && idFirst == 268435456) {
                    lpord->grobj = grobjOther;
                    lpord->id = -1;
                } else if (idSel & 0x20000000) {
                    lpord->grobj = grobjThing;
                    lpord->id = LOWORD(idSel);
                } else if (idSel & 0x80000000) {
                    lpord->grobj = grobjFleet;
                    lpord->id = LOWORD(idSel);
                } else {
                    lpord->grobj = grobjPlanet;
                    lpord->id = LOWORD(idFirst);
                }
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                FillOrdersLB(app);
                SetOrdersLbSel(app, app.state.sel.iwpAct);
            }
        } else {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsRightClickBlueDiamondBringPopupMenu, app.lpui->szPopupBuffer);
            Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
        }
    } else {
        if (fRightBtn) {
            return nullptr;
        }
        if (win32::PtInRect(app.lpui->rgrcRef, PointFrom16(pt)) != 0) {
            if (app.state.sel.grobj != grobjFleet && fCursor) {
                return nullptr;
            }
            irc = 0;
            lTempMax = 11;
            lMax = 11;
            lCur = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].iWarp;
            grbit = 6;
        } else if (win32::PtInRect(&app.lpui->rgrcRef[15], PointFrom16(pt)) != 0) {
            irc = 15;
            iWarp = ColonyIO().pfnWarp(app, &app.state.sel.pl, &fTwoMAs);
            lTempMax = static_cast<int16_t>((iWarp - 1));
            lMax = static_cast<int16_t>((iWarp - 1));
            lTempMin = 1;
            lCur = app.state.sel.pl.iWarpFling;
        } else if (win32::PtInRect(&app.lpui->rgrcRef[1], PointFrom16(pt)) != 0) {
            if (app.state.sel.grobj == grobjFleet) {
                idPlan = app.state.sel.fl.idPlanet;
                iSkip = app.state.sel.fl.id;
            } else {
                iSkip = -1;
                idPlan = app.state.sel.pl.id;
            }
            lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
            FLookupOrbitingXfer(app, idPlan, LOWORD(lSel), &xf, iSkip);
            if (xf.grobj != grobjFleet || xf.iPlayerObj() != app.state.idPlayer) {
                return nullptr;
            }
            irc = 1;
            lMax = ProductionIO().pfnFleetStat(app, &xf.fl(), 1);
            lCur = xf.fl().rgwtMin[4];
            grbit = 4;
            if (app.state.sel.grobj == grobjFleet) {
                lTempMax = lCur + app.state.sel.fl.rgwtMin[4];
                lTempMin = lCur - (ProductionIO().pfnFleetStat(app, &app.state.sel.fl, 1) - app.state.sel.fl.rgwtMin[4]);
            } else {
                lTempMax = lCur;
            }
        } else {
            if (win32::PtInRect(&app.lpui->rgrcRef[3], PointFrom16(pt)) != 0) {
                if (fCursor) {
                    return app.lpui->hcurHand;
                }
                if (app.state.sel.fl.idPlanet != idPlanetDeepSpace) {
                    TransferStuff(app, app.state.sel.fl.id, grobjFleet, app.state.sel.fl.idPlanet, grobjPlanet, mdXferCargo);
                } else {
                    lpth = app.state.lpThings;
                    lpthMac = app.state.lpThings + app.state.cThing;
                    for (; lpth < lpthMac && (lpth->ith() != ithMineralPacket || app.state.sel.fl.pt.x != lpth->pt().x() || app.state.sel.fl.pt.y != lpth->pt().y()); lpth++) {
                    }
                    if (lpth == lpthMac) {
                        TransferStuff(app, app.state.sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
                    } else {
                        ControlIO().pfnMessageBeep(win32::MB_OK);
                    }
                }
                return nullptr;
            }
            if (win32::PtInRect(&app.lpui->rgrcRef[4], PointFrom16(pt)) != 0) {
                if (fCursor) {
                    return app.lpui->hcurHand;
                }
                lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
                if (lSel == -1) {
                    return nullptr;
                }
                if (FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, app.state.sel.grobj == grobjFleet ? app.state.sel.fl.id : idflNone)) {
                    TransferStuff(app, app.state.sel.id, app.state.sel.grobj, xf.id, xf.grobj, mdXferCargo);
                }
                return nullptr;
            }
            if (win32::PtInRect(&app.lpui->rgrcRef[18], PointFrom16(pt)) != 0) {
                if ((app.state.sel.grobj != grobjFleet && fCursor) || app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask != grTaskPatrol) {
                    return nullptr;
                }
                irc = 18;
                lTempMax = 10;
                lMax = 10;
                lCur = static_cast<uint32_t>(app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX);
                grbit = 7;
            }
        }
    }
    if (irc == -1) {
        return nullptr;
    }
    if (fCursor) {
        return app.lpui->hcurHand;
    }
    dx = static_cast<int16_t>((app.lpui->rgrcRef[irc].right - app.lpui->rgrcRef[irc].left - 2));
    xRnd = static_cast<int32_t>((dx / (lMax + 1))) >> 1;
    hdc = win32::GetDC(app.lpui->hwndPlanet);
    win32::SetCapture(app.lpui->hwndPlanet);
    ptOld.y = -1;
    ptOld.x = -1;
    lTempMax = lMax < lTempMax ? lMax : lTempMax;
    lTempMin = 0 <= lTempMin ? lTempMin : 0;
    fFirst = 1;
    while (fFirst || FGetMouseMove(&pt)) {
        fFirst = 0;
        if (pt.x != ptOld.x || pt.y != ptOld.y) {
            ptOld = pt;
            lNew = static_cast<int32_t>((static_cast<int32_t>(((static_cast<int16_t>((pt.x - app.lpui->rgrcRef[irc].left)) + xRnd) * lMax)) / dx));
            if (lTempMin > (lNew < lTempMax ? lNew : lTempMax)) {
                lNew = lTempMin;
            } else if (lNew >= lTempMax) {
                lNew = lTempMax;
            }
            if (lNew != lCur) {
                switch (irc) {
                case 0:
                    SetScanPathWarp(app, app.state.sel.iwpAct, LOWORD(lNew));
                    ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileFleetOrders | tileMinimized));
                    break;
                case 18:
                    app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX = LOWORD(lNew);
                    ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileStarbaseOrWaypoint | tileMinimized));
                    break;
                case 15:
                    DrawMassWarpGauge(app, hdc, &app.lpui->rgrcRef[15], !fTwoMAs ? iWarp : -iWarp, LOWORD(lNew) + 4);
                    break;
                case 2:
                    app.state.sel.fl.rgwtMin[4] = lNew;
                    DrawFleetGauge(app, hdc, &app.lpui->rgrcRef[irc], nullptr, grbit);
                    break;
                case 1:
                    if (app.state.sel.grobj == grobjFleet) {
                        app.state.sel.fl.rgwtMin[4] -= lNew - lCur;
                        DrawFleetGauge(app, hdc, &app.lpui->rgrcRef[2], &app.state.sel.fl, grbit);
                    } else {
                        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileMineralsOrCargo | tileMinimized));
                    }
                    xf.fl().rgwtMin[4] = lNew;
                    DrawFleetGauge(app, hdc, &app.lpui->rgrcRef[irc], &xf.fl(), grbit);
                }
                lCur = lNew;
            }
        }
    }
    grbit = app.state.sel.scan.grobj == grobjOther ? app.state.sel.scan.grobjFull : app.state.sel.scan.grobj;
    if (irc == 2) {
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
        PlanetQueryIO().pfnLookup(app, idWriteBack, &pl);
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileFleetOrders | tileFleetComp | tileMinimized));
        if ((grbit & 1) && app.state.sel.fl.idPlanet == app.state.sel.scan.idpl) {
        FixMinWin:
            InvalidateMineralBars(app);
        } else if ((grbit & 2) && app.state.sel.fl.id == app.state.rglpfl[app.state.sel.scan.ifl]->id) {
            ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
        }
    } else if (irc == 1) {
        FleetQueryIO().pfnLookup(app, idWriteBack, &xf.fl());
        if (app.state.sel.grobj == grobjFleet) {
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileMineralsOrCargo | tileFleetOrders | tileFleetComp | tileMinimized));
        } else {
            PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
            if ((grbit & 1) && app.state.sel.pl.id == app.state.sel.scan.idpl)
                goto FixMinWin;
        }
    } else if (irc == 0 || irc == 18) {
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    } else if (irc == 15 && LOWORD(lCur) != app.state.sel.pl.iWarpFling) {
        app.state.sel.pl.iWarpFling = LOWORD(lCur);
        PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
    }
    win32::ReleaseCapture();
    return reinterpret_cast<HCURSOR>(static_cast<uintptr_t>(win32::ReleaseDC(app.lpui->hwndPlanet, hdc)));
}

void DeleteCurWayPoint(APPSESSION &app, int16_t fBackup) {
    POINT16 pt;
    POINT16 rgpt[3];
    int16_t cpt;
    SCAN    scan;
    int16_t ipt;
    RECT    rc;

    if (app.state.sel.fl.cord < 2 || app.state.sel.iwpAct == 0) {
        ControlIO().pfnMessageBeep(win32::MB_ICONASTERISK);
    } else {
        if (app.lpui->grbitScan & grbitScanFleetPaths) {
            rgpt[0] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt;
            rgpt[1] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct - 1].pt;
            if (app.state.sel.iwpAct < app.state.sel.fl.cord - 1) {
                cpt = 3;
                rgpt[2] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt;
            } else {
                cpt = 2;
            }
        }
        RedrawScanSel(app, nullptr, 0);
        memmove(&app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct], &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1], (app.state.sel.fl.cord - app.state.sel.iwpAct - 1) * sizeof(ORDER));
        app.state.sel.fl.cord--;
        app.state.sel.fl.lpplord->iordMac--;
        app.state.sel.iwpAct--;
        if (app.state.sel.iwpAct < app.state.sel.fl.cord - 1) {
            pt = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt;
            if (pt.x == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt.x && pt.y == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt.y) {
                memmove(&app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1], &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 2], (app.state.sel.fl.cord - app.state.sel.iwpAct - 2) * sizeof(ORDER));
                app.state.sel.fl.cord--;
                app.state.sel.fl.lpplord->iordMac--;
            }
        }
        if (!fBackup && app.state.sel.iwpAct < app.state.sel.fl.cord - 1) {
            app.state.sel.iwpAct++;
        }
        RedrawScanSel(app, nullptr, 0);
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
        FFindNearestObject(app, app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact), &scan);
        app.state.sel.iwpAct = -2;
        ChangeScanSel(app, &scan, 1);
        if (app.lpui->grbitScan & grbitScanFleetPaths) {
            for (ipt = 0; ipt < cpt; ipt++) {
                LogicalToScan(app, &rgpt[ipt]);
            }
            BoundPoints(&rc, rgpt, cpt);
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, &rc, 1);
        }
    }
    return;
}

LRESULT  FakeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (msg != win32::WM_CHAR || ((wParam >= '0' && wParam <= '9') || wParam == 8)) {
        return ui::CallWindowProc(app.lpui->lpfnRealEditProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}

void SetFleetDropDownSel(APPSESSION &app, int16_t id) {
    int16_t idSkip;
    int16_t i;
    FLEET  *lpfl;
    int16_t iOffset;

    iOffset = 0;
    idSkip = app.state.sel.grobj == grobjFleet ? app.state.sel.fl.id : idflNone;
    for (i = 0; i < app.state.cFleet && app.state.rglpfl[i]->id != id; i++) {
        lpfl = app.state.rglpfl[i];
        if (app.state.sel.pt.x == lpfl->pt.x && app.state.sel.pt.y == lpfl->pt.y && lpfl->id != idSkip) {
            iOffset++;
        }
    }
    ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_SETCURSEL, iOffset, 0);
    ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileShipList | tileMinimized));
    return;
}

void FillFleetCompLB(APPSESSION &app) {
    int16_t i;
    int32_t pctDmg;

    ControlIO().pfnSendMessage(app.lpui->hwndFleetCompLB, win32::LB_RESETCONTENT, 0, 0);
    for (i = 0; i < 16; i++) {
        if (app.state.sel.fl.rgcsh[i] > 0) {
            pctDmg = static_cast<int32_t>((static_cast<uint32_t>((app.state.sel.fl.rgdv[i].pctSh() * app.state.sel.fl.rgdv[i].pctDp())) + 250)) / 500;
            win32::wsprintfA(app.state.szWork, "%c%c%5d%s", pctDmg == 0 ? 81 : 80, pctDmg == 0 ? 32 : static_cast<int16_t>(static_cast<int8_t>(LOBYTE(LOWORD(pctDmg)))), app.state.sel.fl.rgcsh[i],
                     app.state.rgshdef[i].hul.szClass);
            ControlIO().pfnSendMessage(app.lpui->hwndFleetCompLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
        }
    }
    return;
}

void FillOrdersLB(APPSESSION &app) {
    int16_t i;
    char   *psz;
    ORDER   ord;

    ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_RESETCONTENT, 0, 0);
    for (i = 0; i < app.state.sel.fl.cord; i++) {
        ord = app.state.sel.fl.lpplord->rgord()[i];
        psz = PszGetLocName(app, static_cast<GrobjClass>(ord.grobj), ord.id, ord.pt.x, ord.pt.y);
        ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
    }
    SetOrdersLbSel(app, app.state.sel.iwpAct);
    if (app.state.sel.grobj == grobjFleet) {
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileFleetOrders | tileStarbaseOrWaypoint));
    }
    return;
}

void SetOrdersLbSel(APPSESSION &app, int16_t iSel) {
    ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_SETCURSEL, iSel, 0);
    if (iSel > (!app.state.gd.fSmallTileMode ? 2 : 1)) {
        ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_SETTOPINDEX, iSel - (!app.state.gd.fSmallTileMode ? 2 : 1), 0);
    }
    ControlIO().pfnUpdateWindow(app.lpui->hwndShipLB);
    ControlIO().pfnUpdateOrdersDDs(app, 0);
    return;
}

void UpdateOrdersDDs(APPSESSION &app, int16_t iLevel) {
    int32_t rglSel[3];
    int16_t iMin;
    int16_t i;
    char   *psz;
    int16_t iSel;
    int16_t iMax;
    char    szT[80];

    iSel = -1;
    if (iLevel == 0) {
        rglSel[0] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_SETCURSEL, app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask, 0);
    } else {
        rglSel[0] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETCURSEL, 0, 0);
    }
    if (iLevel <= 1) {
    DoMinerals:
        ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_RESETCONTENT, 0, 0);
        switch (rglSel[0]) {
        case 1:
            iMax = ProductionIO().pfnFleetStat(app, &app.state.sel.fl, 2) == 0 ? 1 : 5;
            for (i = 0; i < iMax; i++) {
                if (i == 0) {
                    iMin = 4;
                } else {
                    iMin = i - 1;
                }
                strcpy(&app.state.szWork[1], app.state.rgszMinerals[iMin]);
                if (app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp.rgia[iMin].iAction() != iActionNone) {
                    app.state.szWork[0] = '*';
                    if (iSel == -1) {
                        iSel = iMin;
                    }
                } else {
                    app.state.szWork[0] = ' ';
                }
                ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
            }
            if (iSel == -1 || iSel == 4) {
                iSel = 0;
            } else {
                iSel++;
            }
            rglSel[1] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_SETCURSEL, iSel, 0);
            break;
        case 7:
            psz = TextIO().pfnCompressed(app, idsWithinDLY);
            for (i = 0; i < 11; i++) {
                win32::wsprintfA(app.state.szWork, psz, 50 * i + 50);
                ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
            }
            psz = TextIO().pfnCompressed(app, idsAnyEnemy);
            ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
            iSel = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tptl().iDist;
            rglSel[1] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_SETCURSEL, iSel, 0);
            break;
        case 9:
            szT[0] = ' ';
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (i != app.state.idPlayer) {
                    psz = TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr);
                    strcpy(&szT[1], psz);
                    ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szT));
                }
            }
            iSel = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX;
            rglSel[1] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_SETCURSEL, iSel, 0);
            break;
        case 6:
            for (i = 0; i < 5; i++) {
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDYearC), i + 1, i == 0 ? 32 : 115);
                ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
            }
            ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, idsIindefinitely)));
            rglSel[1] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_SETCURSEL, app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].tsell().iPlrX, 0);
        }
    } else {
        rglSel[1] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0);
        if (rglSel[0] == 1 && iLevel <= 3) {
            iSel = LOWORD(rglSel[1]);
            if (iSel == 0) {
                iSel = 4;
            } else {
                iSel--;
            }
            goto DoMinerals;
        }
    }
    if (iLevel <= 2) {
        ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_RESETCONTENT, 0, 0);
        if (rglSel[0] == 1) {
            for (i = idsAction; i <= idsSetWaypoint; i++) {
                if (i == idsLoadDunnage && rglSel[1] == 0) {
                    psz = TextIO().pfnCompressed(app, idsLoadOptimal);
                } else {
                    psz = TextIO().pfnCompressed(app, static_cast<StringId>(i));
                }
                ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
            }
            iSel = LOWORD(rglSel[1]);
            if (iSel == 0) {
                iSel = 4;
            } else {
                iSel--;
            }
            rglSel[2] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_SETCURSEL, app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp.rgia[iSel].iAction(), 0);
        }
    } else {
        rglSel[2] = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_GETCURSEL, 0, 0);
    }
    if (iLevel <= 3 && rglSel[0] == 1) {
        iSel = LOWORD(rglSel[1]);
        if (iSel == 0) {
            iSel = 4;
        } else {
            iSel--;
        }
        win32::wsprintfA(app.state.szWork, "%u", app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp.rgia[iSel].cQuan());
        ControlIO().pfnSetWindowText(app.lpui->hwndOrderED, app.state.szWork);
    }
    return;
}

void FillBattleDD(APPSESSION &app, int16_t iSel) {
    int16_t i;

    ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::CB_RESETCONTENT, 0, 0);
    TextIO().pfnCchGetString(app, idsBattlePlans, app.state.szWork);
    ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
    for (i = 0; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
        strcpy(app.state.szWork, app.state.rglpbtlplan[app.state.idPlayer][i].szName);
        ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
    }
    ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::CB_SETCURSEL, iSel, 0);
    return;
}

void ApplyTransportPresetPopup(APPSESSION &app, POINT16 pt) {
    int16_t i,cMax,iSkip,fRet;
    FARPROC lpProc;
    char sz255[2];
    const char *rgszZip[11];
    ZIPORDER rgzo[4];
    TASKXPORT *lptxp;
            sz255[0] = -1;
            sz255[1] = 0;
            for (i = 0; i < 4; i++) {
                rgszZip[i] = app.state.rgszZipOrder[i];
            }
            rgszZip[4] = sz255;
            cMax = 5;
            for (i = 0; i < 4; i++) {
                if (app.state.vrgZip[i].fValid) {
                    rgszZip[cMax++] = app.state.vrgZip[i].szName;
                }
            }
            if (cMax > 5) {
                rgszZip[cMax++] = sz255;
            }
            rgszZip[cMax++] = TextIO().pfnCompressed(app, idsCustomize);
            i = ControlIO().pfnPopupMenu(app, app.lpui->hwndPlanet, pt.x, pt.y, cMax, nullptr, rgszZip, -1, 1);
            if (i == cMax - 1) {
                memcpy(rgzo, app.state.vrgZip, 96);
                lpProc = (ZipOrderDlg);
                fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_ZIP_PROD), app.lpui->hwndFrame, lpProc, 0);
                if (!fRet) {
                    memcpy(app.state.vrgZip, rgzo, 96);
                }
            } else if (i > 4) {
                i -= 4;
                iSkip = 0;
                while (i != 0) {
                    if (app.state.vrgZip[iSkip].fValid) {
                        i--;
                        if (i == 0)
                            break;
                    }
                    iSkip++;
                }
                app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp = app.state.vrgZip[iSkip].txp;
                goto LWriteZip;
            } else if (i != -1) {
                lptxp = static_cast<TASKXPORT *>(&app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp);
                switch (i) {
                case 0:
                    lptxp->rgia[4].SetIAction(iActionLoadDunnage);
                    for (i = 0; i < 3; i++) {
                        lptxp->rgia[i].SetIAction(iActionLoadAll);
                    }
                    lptxp->rgia[3].SetIAction(iActionNone);
                    break;
                case 1:
                    lptxp->rgia[4].SetIAction(iActionLoadDunnage);
                    for (i = 0; i <= 3; i++) {
                        lptxp->rgia[i].SetIAction(iActionUnloadAll);
                    }
                    break;
                case 2:
                    lptxp->rgia[4].SetIAction(iActionLoadDunnage);
                    for (i = 0; i < 3; i++) {
                        lptxp->rgia[i].SetIAction(iActionWaitPercent);
                        lptxp->rgia[i].SetCQuan(100);
                    }
                    lptxp->rgia[3].SetIAction(iActionNone);
                    break;
                case 3:
                    for (i = 0; i < 5; i++) {
                        lptxp->rgia[i].SetIAction(iActionNone);
                        lptxp->rgia[i].SetCQuan(0);
                    }
                }
            LWriteZip:
                FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                ControlIO().pfnUpdateOrdersDDs(app, 1);
                ControlIO().pfnDrawPlanShip(app, nullptr, tileStarbaseOrWaypoint);
            }
}

void ChangeTransportAmount(APPSESSION &app) {
    int32_t lSel,lMin;
    int16_t iInit,i,fPercent=0;
    char rgb[8];
        lSel = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_GETCURSEL, 0, 0);
        if (lSel == 5 || lSel == 6) {
            fPercent = 1;
        }
        ControlIO().pfnGetWindowText(app.lpui->hwndOrderED, rgb, 8);
        iInit = atoi(rgb);
        i = 0 <= iInit ? iInit : 0;
        if (fPercent) {
            i = 100 >= i ? i : 100;
        } else {
            i = 4000 >= i ? i : 4000;
        }
        if (iInit != i) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsAmountCargoMaySpecifyHereMustBetween, nullptr), win32::MB_ICONHAND);
            win32::wsprintfA(app.state.szWork, app.state.PCTD, i);
            ControlIO().pfnSetWindowText(app.lpui->hwndOrderED, app.state.szWork);
        }
        lMin = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0);
        if (lMin == 0) {
            lMin = 4;
        } else {
            lMin--;
        }
        app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp.rgia[lMin].SetCQuan(i);
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
}

void ChangeTransportAction(APPSESSION &app) {
    int32_t lSel,lMin;
            lSel = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[2], win32::CB_GETCURSEL, 0, 0);
            lMin = ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0);
            if (lMin == 0) {
                lMin = 4;
            } else {
                lMin--;
            }
            app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp.rgia[lMin].SetIAction(LOWORD(lSel));
            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
            ControlIO().pfnUpdateOrdersDDs(app, 3);
            ControlIO().pfnDrawPlanShip(app, nullptr, tileStarbaseOrWaypoint);
}

int16_t FTransportQuantityActive(int32_t l, int16_t *pi) {
    int16_t i=*pi;
    int16_t fActive;
            fActive = 1;
            switch (l) {
            case 0:
            case 1:
            case 2:
            case 7:
                fActive = 0;
                break;
            case 5:
            case 6:
                i = 5;
            }
    *pi=i;
    return fActive;
}
