#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

LRESULT  MineWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;
    POINT16     pt;
    HtMineType  ht;
    COLORREF    crFore;
    uint16_t    dxMax;
    COLORREF    crBack;
    int16_t     cch;
    RECT        rc2;
    int16_t     fDetonate;
    RTLOGTHING  rtlt;
    std::byte   rgbThing[cbWireLogThing];

    switch (message) {
    case win32::WM_CREATE:
        app.lpui->hwndMineCB = ui::CreateWindowEx(0, app.lpui->szButton, TextIO().pfnCompressed(app, idsDetonateMineFieldYear), win32::BS_AUTOCHECKBOX | win32::WS_CHILD, 100, 100, 150, app.state.dyArial8, hwnd, NULL,
                                  app.lpui->hInst, NULL);
        ControlIO().pfnSendMessage(app.lpui->hwndMineCB, win32::WM_SETFONT, (WPARAM)app.lpui->rghfontArial8[1], 0);
        SetMineralTitleBar(app, hwnd);
        break;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
    case win32::WM_RBUTTONDOWN:
        if (app.lpui->hwndPopup)
            break;
        MineClick(app, LOWORD(lParam), HIWORD(lParam), message, wParam);
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        win32::SetBkColor((HDC)wParam, app.lpui->crButtonFace);
        return (LRESULT)app.lpui->hbrButtonFace;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        ht = HtMineWindow(app, hwnd, pt.x, pt.y);
        if (ht == htMineNone)
            goto Default;
        win32::SetCursor(ht == htMineScanSel ? app.lpui->hcurHand : app.lpui->hcurArrowHelp);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::SetRect(&rc2, 4, 4, rc.right - 4, app.state.dyArial8 * 2 - 4);
        _Draw3dFrame(app, hdc, &rc2, 0);
        crFore = win32::SetTextColor(hdc, app.lpui->crButtonText);
        crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
        cch = strlen(app.lpui->szMineralTitle);
        dxMax = rc2.right - rc2.left - 24;
        while (cch > 0 && LOWORD(GetTextExtent(hdc, app.lpui->szMineralTitle, cch)) > dxMax) {
            cch = (int16_t)CchUtf8Prefix(app.lpui->szMineralTitle, cch - 1);
        }
        rc2.right -= 8;
        RcCtrTextOut(app, hdc, &rc2, app.lpui->szMineralTitle, cch);
        rc2.right += 8;
        win32::SetTextColor(hdc, crFore);
        win32::SetBkColor(hdc, crBack);
        win32::SetRect(&rc2, rc2.right - (rc2.bottom - rc2.top) + 2, rc2.top + 3, rc2.right - 4, rc2.bottom - 2);
        DrawSelectionArrow(app, hdc, &rc2, FOtherStuffAtScanSel(app));
        rc.top += app.state.dyArial8 * 2 - 4;
        DrawMineSurvey(app, hdc, &rc);
        win32::EndPaint(hwnd, &ps);
        break;
    case win32::WM_COMMAND:
        if ((HWND)lParam != app.lpui->hwndMineCB || HIWORD(wParam) != 0)
            break;
        fDetonate = LOWORD(ControlIO().pfnSendMessage(app.lpui->hwndMineCB, win32::BM_GETCHECK, 0, 0));
        rtlt.idFull = app.state.lpThings[app.state.sel.scan.ith].idFull();
        rtlt.fDetonate = fDetonate;
        FEncodeLogThing(rtlt, rgbThing);
        WriteMemRt(app, rtThing, cbWireLogThing, rgbThing);
        app.state.lpThings[app.state.sel.scan.ith].thm().SetFDetonate(fDetonate);
        break;
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void InvalidateMineralBars(APPSESSION &app) {
    HDC     hdc;
    int16_t dyRow;
    HFONT   hfontSav;
    RECT    rcPop;
    int16_t dx;
    int16_t dxPop;
    RECT    rc;

    ControlIO().pfnGetClientRect(app.lpui->hwndMine, &rc);
    hdc = win32::GetDC(app.lpui->hwndMine);
    hfontSav = (HFONT)win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    rc.right -= LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN999mr), 5)) + 6;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    dxPop = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsPopulation1000000), 21)) + 6;
    dx = LOWORD(GetTextExtent(hdc, app.state.rgszPlanetAttr[1], strlen(app.state.rgszPlanetAttr[1]))) + 6;
    if (dx * 4 > rc.right) {
        rc.left += LOWORD(GetTextExtent(hdc, app.state.rgszPlanetAttr[1], 4)) + 6;
    } else {
        rc.left += dx;
    }
    win32::SelectObject(hdc, hfontSav);
    win32::ReleaseDC(app.lpui->hwndMine, hdc);
    rc.top += app.state.dyArial8 * 2 - 4;
    rcPop.top = rc.top + 2;
    rcPop.bottom = rcPop.top + app.state.dyArial8;
    rcPop.right = rc.right;
    rcPop.left = rc.right - dxPop;
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, &rcPop, 1);
    dyRow = (int16_t)(rc.bottom - rc.top - app.state.dyArial8 * 4 - 2) / 6;
    dyRow = (dyRow + 1) & 0xfffe;
    rc.top += (5 * app.state.dyArial8 >> 1) + 3 * dyRow + 1;
    rc.bottom = 3 * dyRow + rc.top;
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, &rc, 0);
    return;
}

void DrawMineSurvey(APPSESSION &app, HDC hdc, RECT *prc) {
    PLANET     pl;
    HBRUSH     hbrSav;
    int32_t    l2;
    COLORREF   crFore;
    int16_t    c2;
    int32_t    rgl[3];
    int16_t    c;
    int16_t    i;
    FLEET     *lpfl;
    COLORREF   crBack;
    HDC        hdcMem;
    int16_t    bkMode;
    char      *psz;
    int16_t    cch;
    RECT       rcGauge;
    char       szT[80];
    GrobjClass grobj;
    int32_t    l;
    RECT       rc;
    int32_t    cMass;
    int16_t    yTop;
    int16_t    xLeft;
    int32_t    cShip;
    int16_t    iOffset;
    ORDER     *lpord;
    char       szWP[30];
    char      *pszT;
    THING     *lpth;
    int32_t    pctDecay;
    int16_t    ibmp;
    int32_t    lDecay;
    int16_t    iplrbmp;
    THING     *lpthDest;
    int16_t    fCanTerraform;
    int16_t    rgMin[3];
    int16_t    xL;
    int16_t    dyRow;
    int16_t    iMax;
    int16_t    yBot;
    int16_t    fShortLabels;
    int16_t    cNum;
    int16_t    dy;
    PLAYER     plrSav;
    int16_t    iMin;
    int16_t    xEnd;
    int16_t    yCur;
    int16_t    dxBar;
    int16_t    dxNum;
    int16_t    xR;
    int16_t    dxRLabels;
    int16_t    iCur;
    int16_t    rgCost[3];
    int16_t    rgMax[3];
    int16_t    dxLabels;
    int16_t    dx;
    int16_t    xBeg;
    int16_t    dNum;
    HBITMAP    hbmpSav;
    COLORREF   crBkSav;
    COLORREF   crTextSav;
    int16_t    dBest;
    POINT16    pt;
    int16_t    iT;
    int32_t    rglT[3];
    int16_t    ifl;
    int32_t    cMines;
    int16_t    iPass;

    hdcMem = win32::CreateCompatibleDC(hdc);
    crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    crFore = win32::SetTextColor(hdc, 0xffff);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    grobj = app.state.sel.scan.grobj;
    if (grobj == grobjOther) {
        if (app.state.sel.scan.grobjFull & grobjPlanet) {
            grobj = grobjPlanet;
        } else if (app.state.sel.scan.grobjFull & grobjFleet) {
            grobj = grobjFleet;
        } else if (app.state.sel.scan.grobjFull & grobjThing) {
            grobj = grobjThing;
        } else {
            grobj = grobjNone;
        }
    }
    if (grobj == grobjNone)
        goto FinishUp;
    if (grobj == grobjFleet) {
        cMass = 0;
        if (app.state.sel.scan.ifl == iflNone)
            goto FinishUp;
        xLeft = prc->left + 6;
        yTop = prc->top + 6;
        hbrSav = (HBRUSH)win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        win32::PatBlt(hdc, xLeft, yTop, 70, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft, yTop + 2, 2, 68, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 16, yTop + 68, 2, 38, win32::PATCOPY);
        hbrSav = (HBRUSH)win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, xLeft + 2, yTop + 68, 15, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 1, yTop + 69, 15, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 52, yTop + 68, 18, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 17, yTop + 104, 37, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 16, yTop + 105, 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 52, yTop + 70, 2, 34, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 2, yTop + 2, 66, 66, win32::BLACKNESS);
        win32::PatBlt(hdc, xLeft + 18, yTop + 68, 34, 36, win32::BLACKNESS);
        win32::SelectObject(hdc, hbrSav);
        lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
        DrawFleetBitmap(app, lpfl, hdc, xLeft + 2, yTop + 2, 0, -1, 0, 0, -1, 0);
        iOffset = app.state.rgplr[lpfl->iplr()].iPlrBmp;
        win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        win32::RealizePalette(hdc);
        DibBlt(hdc, xLeft + 19, yTop + 71, 32, 32, app.lpui->hdibRaces, (iOffset & 7) * 0x20, (3 - (iOffset >> 3)) * 0x20, 32, 32, 13369376);
        win32::SetTextColor(hdc, app.lpui->crButtonText);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        cShip = 0;
        for (i = 0; i < 16; i++) {
            cShip += lpfl->rgcsh[i];
        }
        TextIO().pfnCchGetString(app, idsShipCountLd, szT);
        c = win32::wsprintfA(app.state.szWork, szT, cShip);
        win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        if (lpfl->det == detAll || lpfl->det == detMore) {
            cMass = WtFromLpfl(app, lpfl);
            c = TextIO().pfnCchGetString(app, idsFuel2, szT);
            l = GetTextExtent(hdc, szT, c);
            c2 = TextIO().pfnCchGetString(app, idsCargo, app.state.szWork);
            l2 = GetTextExtent(hdc, app.state.szWork, c2);
            if (l2 > l) {
                l = l2;
            }
            win32::SetRect(&rcGauge, prc->left + 90 + LOWORD(l), yTop, prc->right - 4, yTop + app.state.dyArial8);
            if (rcGauge.left < rcGauge.right) {
                win32::TextOutA(hdc, prc->left + 86, yTop, szT, c);
                yTop += app.state.dyArial8 + 2;
                win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c2);
                DrawFleetGauge(app, hdc, &rcGauge, lpfl, 4);
                OffsetRc(&rcGauge, 0, app.state.dyArial8 + 4);
                DrawFleetGauge(app, hdc, &rcGauge, lpfl, 5);
                yTop += app.state.dyArial8 + 2;
            }
        } else {
            cMass = lpfl->wtFleet();
        }
        c = TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsFleetMassLdkt : idsMassLdkt, szT);
        c = win32::wsprintfA(app.state.szWork, szT, cMass);
        win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        if (lpfl->det == detAll) {
            lpord = lpfl->lpplord->rgord();
            if (lpfl->cord > 1) {
                lpord++;
            }
            TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWaypointS : idsWpS, szWP);
            if (lpfl->cord == 1) {
                c = win32::wsprintfA(szT, szWP, TextIO().pfnCompressed(app, idsNone));
            } else {
                c = win32::wsprintfA(szT, szWP, PszGetLocName(app, static_cast<GrobjClass>(lpord->grobj), lpord->id, lpord->pt.x, lpord->pt.y));
            }
            win32::TextOutA(hdc, prc->left + 86, yTop, szT, c);
            yTop += app.state.dyArial8 + 2;
            TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWaypointTaskS : idsTaskS, szT);
            c = win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(idsTaskHere + lpord->grTask)));
            win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
            yTop += app.state.dyArial8 + 2;
            if (lpfl->cord > 1) {
                if (lpord->iWarp == 11) {
                    c = TextIO().pfnCchGetString(app, idsUseStargate, app.state.szWork);
                } else {
                    TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWarpSpeedD : idsWarpD, szT);
                    c = win32::wsprintfA(app.state.szWork, szT, lpord->iWarp);
                }
            } else {
                c = TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWarpSpeedStopped : idsWarpStopped, app.state.szWork);
            }
            win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
            yTop += app.state.dyArial8 + 2;
            l = CMineSweepFromLpfl(app, lpfl);
            if (l <= 0)
                goto FinishUp;
            pszT = TextIO().pfnCompressed(app, idsFleetCanDestroyLdMinesPerYear);
            c = win32::wsprintfA(app.state.szWork, pszT, l);
            win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
            yTop += app.state.dyArial8 + 2;
            goto FinishUp;
        }
        if (!lpfl->fdirValid())
            goto FinishUp;
        if (lpfl->iwarpFlt() != 0) {
            TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWarpSpeedD : idsWarpD, szT);
            c = win32::wsprintfA(app.state.szWork, szT, lpfl->iwarpFlt());
        } else {
            c = TextIO().pfnCchGetString(app, !app.state.gd.fSmallTileMode ? idsWarpSpeedStopped : idsWarpStopped, app.state.szWork);
        }
        win32::TextOutA(hdc, prc->left + 86, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        goto FinishUp;
    } else if (grobj == grobjThing) {
        if (app.state.sel.scan.ith == ithNone)
            goto FinishUp;
        lpth = app.state.lpThings + app.state.sel.scan.ith;
        iplrbmp = app.state.rgplr[lpth->iplr()].iPlrBmp;
        xLeft = prc->left + 6;
        yTop = prc->top + 6;
        hbrSav = (HBRUSH)win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        win32::PatBlt(hdc, xLeft, yTop, 70, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft, yTop + 2, 2, 68, win32::PATCOPY);
        switch (lpth->ith()) {
        case ithMinefield:
            ibmp = lpth->thm().iType();
            break;
        case ithMineralPacket:
            ibmp = (lpth->thp().iWarp() != 0) + 3;
            break;
        case ithMysteryTrader:
            ibmp = 6;
            break;
        default:
            ibmp = 5;
        }
        win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        win32::RealizePalette(hdc);
        if (lpth->ith() != ithWormhole && lpth->ith() != ithMysteryTrader) {
            win32::PatBlt(hdc, xLeft + 16, yTop + 68, 2, 38, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, xLeft + 2, yTop + 68, 15, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 1, yTop + 69, 15, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 52, yTop + 68, 18, 2, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 17, yTop + 104, 37, 2, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 16, yTop + 105, 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 52, yTop + 70, 2, 34, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 2, yTop + 2, 66, 66, win32::BLACKNESS);
            win32::PatBlt(hdc, xLeft + 18, yTop + 68, 34, 36, win32::BLACKNESS);
            DibBlt(hdc, xLeft + 19, yTop + 71, 32, 32, app.lpui->hdibRaces, (iplrbmp & 7) * 0x20, (3 - (iplrbmp >> 3)) * 0x20, 32, 32, 13369376);
        } else {
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, xLeft + 2, yTop + 68, 68, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 1, yTop + 69, 69, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xLeft + 2, yTop + 2, 66, 66, win32::BLACKNESS);
        }
        win32::SelectObject(hdc, hbrSav);
        DibBlt(hdc, xLeft + 2, yTop + 2, 64, 64, app.lpui->hdibThings, ibmp * 64, 0, 64, 64, 13369376);
        if (lpth->ith() == ithMineralPacket) {
            xLeft += 80;
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            if (lpth->thp().iWarp() != 0) {
                TextIO().pfnCchGetString(app, idsTravelingWarpD, szT);
                c = win32::wsprintfA(app.state.szWork, szT, lpth->thp().iWarp() + 4);
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
                yTop += app.state.dyArial8 + 2;
                TextIO().pfnCchGetString(app, idsDestination, szT);
                psz = TextIO().pfnPlanetName(app, lpth->thp().idPlanet());
                strcat(szT, psz);
                win32::TextOutA(hdc, xLeft, yTop, szT, strlen(szT));
            }
            yTop += (int16_t)(3 * app.state.dyArial8) / 2;
            xLeft += LOWORD(GetTextExtent(hdc, app.state.rgszMinerals[2], strlen(app.state.rgszMinerals[2])));
            for (i = 0; i < 3; i++) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsS2), app.state.rgszMinerals[i]);
                RightTextOut(app, hdc, xLeft, yTop, app.state.szWork, c, 0);
                c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, lpth->thp().rgwtMin(i));
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
                yTop += app.state.dyArial8 + 2;
            }
            goto FinishUp;
        } else if (lpth->ith() == ithWormhole) {
            xLeft += 94;
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            cch = TextIO().pfnCchGetString(app, idsLocation, szT);
            xLeft += LOWORD(GetTextExtent(hdc, szT, cch)) + 20;
            RightTextOut(app, hdc, xLeft - 4, yTop, szT, cch, 0);
            cch = TextIO().pfnCchGetString(app, idsDD5, szT);
            c = win32::wsprintfA(app.state.szWork, szT, lpth->pt().x(), lpth->pt().y());
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            yTop += (int16_t)(3 * app.state.dyArial8) / 2;
            c = TextIO().pfnCchGetString(app, idsDestination2, app.state.szWork);
            RightTextOut(app, hdc, xLeft - 4, yTop, app.state.szWork, c, 0);
            if ((1 << app.state.idPlayer & lpth->thw().grbitPlrTrav()) && (lpthDest = LpthFromId(app, lpth->thw().idPartner())) != 0) {
                c = win32::wsprintfA(app.state.szWork, szT, lpthDest->pt().x(), lpthDest->pt().y());
            } else {
                c = TextIO().pfnCchGetString(app, idsUnknown2, app.state.szWork);
            }
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            yTop += (int16_t)(3 * app.state.dyArial8) / 2;
            c = TextIO().pfnCchGetString(app, idsStability, app.state.szWork);
            RightTextOut(app, hdc, xLeft - 4, yTop, app.state.szWork, c, 0);
            c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsRockSolid + PctWormholeMoves(lpth)), app.state.szWork);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            goto FinishUp;
        } else if (lpth->ith() == ithMysteryTrader) {
            xLeft += 80;
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            win32::SetRect(&rc, xLeft, yTop, prc->right - 8, prc->bottom - 8);
            if (!(1 << app.state.idPlayer & lpth->tht().grbitPlr())) {
                psz = TextIO().pfnCompressed(app, idsTraderRequestsInterestedPartiesSendFleetLeast);
                yTop += win32::DrawTextA(hdc, psz, strlen(psz), &rc, win32::DT_WORDBREAK | win32::DT_NOPREFIX) + 8;
            }
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsTraderTravelingWarpD), lpth->tht().iWarp());
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
            goto FinishUp;
        }
        if (lpth->ith() != ithMinefield)
            goto FinishUp;
        xLeft += 80;
        win32::SetTextColor(hdc, app.lpui->crButtonText);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        TextIO().pfnCchGetString(app, idsLocationDD, szT);
        c = win32::wsprintfA(app.state.szWork, szT, lpth->pt().x(), lpth->pt().y());
        win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        TextIO().pfnCchGetString(app, idsFieldTypeS, szT);
        c = win32::wsprintfA(app.state.szWork, szT, app.state.rgszMineField[lpth->thm().iType()]);
        win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        TextIO().pfnCchGetString(app, idsFieldRadiusDLYLdMines, szT);
        c = win32::wsprintfA(app.state.szWork, szT, LOWORD((int32_t)sqrt((double)lpth->thm().cMines())), lpth->thm().cMines());
        win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        pctDecay = (int16_t)(((RaceQueryIO().pfnStat(&app.state.rgplr[lpth->iplr()], rsMajorAdv) != raMines) * 3 + 1) * CPlanetsInCircle(app, lpth->pt(), lpth->thm().cMines()) + 2);
        if (pctDecay > 50) {
            pctDecay = 50;
        }
        if (lpth->thm().fDetonate()) {
            pctDecay += 25;
        }
        lDecay = (int32_t)(lpth->thm().cMines() * pctDecay) / 100;
        if (lDecay < pctDecay) {
            lDecay = pctDecay;
        }
        if (lpth->thm().iType() != mineSpeedBump) {
            lDecay = 10 <= lDecay ? lDecay : 10;
        }
        TextIO().pfnCchGetString(app, idsDecayRateLdYear, szT);
        c = win32::wsprintfA(app.state.szWork, szT, lDecay);
        win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        if (lpth->iplr() != app.state.idPlayer)
            goto FinishUp;
        GetMineFieldCounts(app, lpth->idFull(), &i, &c2);
        TextIO().pfnCchGetString(app, idsFieldDD, szT);
        c = win32::wsprintfA(app.state.szWork, szT, i, c2);
        win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        yTop += app.state.dyArial8 + 2;
        goto FinishUp;
    } else {
        if (app.state.sel.scan.idpl != idplNone && PlanetQueryIO().pfnLookup(app, app.state.sel.scan.idpl, &pl)) {
            fShortLabels = 0;
            plrSav = app.state.rgplr[app.state.idPlayer];
            rc = *prc;
            if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raTerra && pl.iPlayer != iplrNone &&
                (app.state.idPlayer == pl.iPlayer || app.state.rgplr[app.state.idPlayer].rgmdRelation[pl.iPlayer] == 1)) {
                for (c = 0; c < 3; c++) {
                    app.state.rgplr[app.state.idPlayer].rgEnvVar[c] = app.state.rgplr[pl.iPlayer].rgEnvVar[c];
                    app.state.rgplr[app.state.idPlayer].rgEnvVarMin[c] = app.state.rgplr[pl.iPlayer].rgEnvVarMin[c];
                    app.state.rgplr[app.state.idPlayer].rgEnvVarMax[c] = app.state.rgplr[pl.iPlayer].rgEnvVarMax[c];
                }
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            dxRLabels = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN999mr), 5)) + 6;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            dxLabels = LOWORD(GetTextExtent(hdc, app.state.rgszPlanetAttr[1], strlen(app.state.rgszPlanetAttr[1]))) + 6;
            if (dxLabels * 4 > rc.right) {
                fShortLabels = 1;
                dxLabels = LOWORD(GetTextExtent(hdc, app.state.rgszPlanetAttr[1], 4)) + 6;
            }
            xL = rc.left + dxLabels;
            xR = rc.right - dxRLabels;
            dyRow = (int16_t)(rc.bottom - rc.top - app.state.dyArial8 * 4 - 2) / 6;
            dyRow = (dyRow + 1) & 0xfffe;
            yCur = rc.top + 2;
            if (pl.fStarbase) {
                hdcMem = win32::CreateCompatibleDC(hdc);
                hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, app.lpui->hbmpMono);
                crTextSav = win32::SetTextColor(hdc, 0);
                crBkSav = win32::SetBkColor(hdc, 0xffffff);
                win32::BitBlt(hdc, rc.right - 22, yCur + 2, 13, 16, hdcMem, 0, 12, win32::SRCAND);
                win32::SetTextColor(hdc, 0xffff);
                win32::SetBkColor(hdc, 0);
                win32::BitBlt(hdc, rc.right - 22, yCur + 2, 13, 16, hdcMem, 0, 12, win32::SRCPAINT);
                win32::SetTextColor(hdc, crTextSav);
                win32::SetBkColor(hdc, crBkSav);
                win32::SelectObject(hdcMem, hbmpSav);
                win32::DeleteDC(hdcMem);
            }
            if (pl.det >= detSome) {
                c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsVal + (fShortLabels == 0)), app.state.szWork);
                dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
                win32::SetTextColor(hdc, app.lpui->crButtonText);
                win32::TextOutA(hdc, xL, yCur, app.state.szWork, c);
                dNum = PlanetQueryIO().pfnDesirability(app, &pl, app.state.idPlayer);
                c = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, dNum);
                win32::SetTextColor(hdc, dNum >= 0 ? 32512 : 0xff);
                win32::TextOutA(hdc, xL + dx, yCur, app.state.szWork, c);
                if (!fShortLabels) {
                    dx += LOWORD(GetTextExtent(hdc, app.state.szWork, c));
                    dBest = PctPlanetOptValue(app, &pl, app.state.idPlayer);
                    if (dBest > dNum) {
                        if (dBest > 0) {
                            win32::SetTextColor(hdc, dBest > 10 ? 32512 : 32639);
                        }
                        c = win32::wsprintfA(app.state.szWork, " (%d%%)", dBest);
                        win32::TextOutA(hdc, xL + dx, yCur, app.state.szWork, c);
                    }
                }
            }
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            if (pl.iPlayer != iplrNone) {
                strcpy(szT, TextIO().pfnCompressed(app, static_cast<StringId>(idsPop + (fShortLabels == 0))));
            }
            if (pl.det == detAll) {
                c = strlen(szT);
                c += CommaFormatLong(app, &szT[c], (uint32_t)(pl.rgwtMin[3] * 100));
                RightTextOut(app, hdc, xR, yCur, szT, c, 0);
            } else if (pl.iPlayer == iplrNone) {
                c = TextIO().pfnCchGetString(app, idsUninhabited, szT);
                RightTextOut(app, hdc, xR, yCur, szT, c, 0);
            } else {
                if (pl.det >= detSome) {
                    l = (int32_t)(pl.uPopGuess * 4);
                    strcpy(app.state.szWork, szT);
                    c = strlen(szT);
                    if (l > 0) {
                        c += win32::wsprintfA(&app.state.szWork[c], TextIO().pfnCompressed(app, idsCLd00), 177, l);
                    } else {
                        c += TextIO().pfnCchGetString(app, idsMsg1264, &app.state.szWork[c]);
                    }
                    RightTextOut(app, hdc, xR, yCur, app.state.szWork, c, 0);
                }
                win32::SetTextColor(hdc, 0xff);
                RightTextOut(app, hdc, xR, yCur + app.state.dyArial8 - 2, TextIO().pfnPlayerName(app, pl.iPlayer, 0, 1, 0, 0, NULL), 0, 0);
            }
            yCur += app.state.dyArial8 - 2;
            dNum = app.state.game.turn - pl.turn;
            if (dNum == 0) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, !fShortLabels ? idsReportCurrent : idsCurrent));
            } else if (fShortLabels) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsOld2), dNum);
            } else {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsReportDYear), dNum);
                if (dNum > 1) {
                    strcat(app.state.szWork, "s");
                    c++;
                }
                c += TextIO().pfnCchGetString(app, idsOld, &app.state.szWork[c]);
            }
            win32::SetTextColor(hdc, dNum > 5 ? 0xff : 0);
            win32::TextOutA(hdc, xL, yCur, app.state.szWork, c);
            yCur += app.state.dyArial8;
            hbrSav = (HBRUSH)win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
            win32::PatBlt(hdc, xL, yCur, xR - xL, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xL + 1, yCur + 1, xR - xL - 2, 3 * dyRow - 1, win32::BLACKNESS);
            win32::PatBlt(hdc, xL, yCur, 1, 3 * dyRow, win32::PATCOPY);
            win32::PatBlt(hdc, xL + 2, yCur + dyRow, xR - xL - 4, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xL + 2, dyRow * 2 + yCur, xR - xL - 4, 1, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, xR - 1, yCur + 1, 1, 3 * dyRow, win32::PATCOPY);
            win32::PatBlt(hdc, xL, 3 * dyRow + yCur, xR - xL, 1, win32::PATCOPY);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            win32::SetBkMode(hdc, win32::TRANSPARENT);
            fCanTerraform = ColonyIO().pfnCanTerraform(app, &pl, rgMin, rgMax, rgCost, 1);
            dx = xR - xL - 4;
            dy = (dyRow - app.state.dyArial8) >> 1;
            i = 0;
            while (i < 3) {
                iMin = app.state.rgplr[app.state.idPlayer].rgEnvVarMin[i];
                iMax = app.state.rgplr[app.state.idPlayer].rgEnvVarMax[i];
                iCur = pl.rgEnvVar[i];
                RightTextOut(app, hdc, xL - 2, yCur + dy, !fShortLabels ? app.state.rgszPlanetAttr[i] : app.lpui->rgszPlanetAttrAbbr[i], 0, 0);
                if (pl.det >= detSome) {
                    psz = PszCalcEnvVar(app, static_cast<EnvType>(i), iCur);
                    c = strlen(psz);
                    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                    win32::TextOutA(hdc, xR + 2, yCur + dy, psz, c);
                }
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                win32::SelectObject(hdc, app.lpui->rghbrPlanetAttr[i][0]);
                win32::PatBlt(hdc, xL + 2 + win32::MulDiv(iMin, dx, 100), yCur + 2, win32::MulDiv(iMax - iMin, dx, 100), dyRow - 3, win32::PATCOPY);
                if (pl.det >= detSome) {
                    win32::SelectObject(hdc, app.lpui->rghbrPlanetAttr[i][1]);
                    pt.x = xL + 2 + win32::MulDiv(pl.rgEnvVarOrig[i], dx, 100);
                    pt.y = dyRow / 2 + yCur;
                    c = dyRow / 4 - 1;
                    if (c < 2) {
                        c = 2;
                    }
                    win32::PatBlt(hdc, pt.x - c, pt.y, c * 2 + 1, 1, win32::PATCOPY);
                    win32::PatBlt(hdc, pt.x, pt.y - c, 1, c * 2 + 1, win32::PATCOPY);
                    yTop = yCur + 3;
                    yBot = yCur + dyRow - 3;
                    c = 1;
                    xBeg = xL + 2 + win32::MulDiv(iCur, dx, 100);
                    while (1) {
                        win32::PatBlt(hdc, xBeg, yTop, 1, 1, win32::PATCOPY);
                        win32::PatBlt(hdc, xBeg + c - 1, yTop++, 1, 1, win32::PATCOPY);
                        if (yTop > yBot)
                            break;
                        win32::PatBlt(hdc, xBeg, yBot, 1, 1, win32::PATCOPY);
                        win32::PatBlt(hdc, xBeg + c - 1, yBot--, 1, 1, win32::PATCOPY);
                        xBeg--;
                        c += 2;
                    }
                    if (fCanTerraform) {
                        yTop--;
                        if (rgMin[i] != envNone) {
                            dxBar = 0 <= iCur - rgMin[i] ? iCur - rgMin[i] : 0;
                        } else {
                            dxBar = 0;
                        }
                        xBeg = xL + 2 + win32::MulDiv(iCur - dxBar, dx, 100);
                        if (rgMax[i] != envNone) {
                            dxBar = 0 <= rgMax[i] - iCur ? rgMax[i] - iCur : 0;
                        } else {
                            dxBar = 0;
                        }
                        xEnd = xL + 2 + win32::MulDiv(iCur + dxBar, dx, 100);
                        win32::PatBlt(hdc, xBeg, yTop, xEnd - xBeg + 1, 1, win32::PATCOPY);
                    }
                }
                i++;
                yCur += dyRow;
            }
            yCur += app.state.dyArial8 >> 1;
            win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
            win32::PatBlt(hdc, xL, yCur, xR - xL, 1, win32::PATCOPY);
            win32::PatBlt(hdc, xL + 1, yCur + 1, xR - xL - 2, 3 * dyRow + 1, win32::BLACKNESS);
            win32::PatBlt(hdc, xL, yCur, 1, 3 * dyRow + 2, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, xR - 1, yCur + 1, 1, 3 * dyRow + 2, win32::PATCOPY);
            win32::PatBlt(hdc, xL, 3 * dyRow + yCur + 2, xR - xL, 1, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->cMinGrafMax);
            dxNum = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            dxBar = xR - xL - 3;
            cNum = dxBar / ((dxNum >> 1) + dxNum);
            dNum = app.lpui->cMinGrafMax / cNum;
            if (app.lpui->cMinGrafMax < 500) {
                dNum = (int16_t)(dNum + 49) / 50 * 10;
            } else if (app.lpui->cMinGrafMax < 1000) {
                dNum = (int16_t)(dNum + 49) / 50 * 50;
            } else if (app.lpui->cMinGrafMax < 2500) {
                dNum = (int16_t)(dNum + 99) / 100 * 100;
            } else if (app.lpui->cMinGrafMax < 7500) {
                dNum = (int16_t)(dNum + 249) / 250 * 250;
            } else if (app.lpui->cMinGrafMax < 15000) {
                dNum = (int16_t)(dNum + 499) / 500 * 500;
            } else {
                dNum = (int16_t)(dNum + 999) / 1000 * 1000;
            }
            cNum = app.lpui->cMinGrafMax / dNum;
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            dy = 3 * dyRow + yCur + 4;
            for (i = 0; i <= cNum; i++) {
                xBeg = LOWORD((int32_t)((int32_t)((uint32_t)(i * dNum) * dxBar) / app.lpui->cMinGrafMax)) + xL;
                win32::PatBlt(hdc, xBeg, yCur + 2, 1, 3 * dyRow - 1, win32::PATCOPY);
                c = win32::wsprintfA(app.state.szWork, app.state.PCTD, i * dNum);
                CtrTextOut(hdc, xBeg, dy, app.state.szWork, c);
            }
            RightTextOut(app, hdc, xL - 4, dy, "kT", 2, 0);
            if (pl.iPlayer == app.state.idPlayer) {
                ColonyIO().pfnEstimateMining(app, &pl, rgl, -1, 0);
            } else {
                rgl[2] = 0;
                rgl[1] = 0;
                rgl[0] = 0;
                if (pl.iPlayer == iplrNone) {
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->idPlanet == pl.id && lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead && lpfl->lpplord->rgord()[0].grTask == grTaskMine) {
                            cMines = ColonyIO().pfnFleetMining(app, lpfl);
                            if (cMines > 0) {
                                ColonyIO().pfnEstimateMining(app, &pl, rglT, cMines, 0);
                                for (iT = 0; iT < 3; iT++) {
                                    rgl[iT] += rglT[iT];
                                }
                            }
                        }
                    }
                }
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            dy = ((dyRow - app.state.dyArial8) >> 1) + 2;
            i = 0;
            while (i <= 2) {
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                RightTextOut(app, hdc, xL - 2, yCur + dy, app.state.rgszMinerals[i], fShortLabels ? 4 : strlen(app.state.rgszMinerals[i]), 0);
                dx = LOWORD((int32_t)((int32_t)(dxBar * (app.lpui->cMinGrafMax < rgl[i] + pl.rgwtMin[i] ? app.lpui->cMinGrafMax : rgl[i] + pl.rgwtMin[i])) / app.lpui->cMinGrafMax));
                win32::SetRect(&rcGauge, xL + 1, yCur + 4, xL + dx + 1, yCur + dyRow - 1);
                if (dx != 0) {
                    win32::FillRect(hdc, &rcGauge, app.lpui->rghbrMinSum[i][1]);
                }
                if ((int32_t)(rgl[i] + pl.rgwtMin[i]) > app.lpui->cMinGrafMax) {
                    win32::SetTextColor(hdc, app.lpui->crButtonText);
                    win32::TextOutA(hdc, xR, yCur + dy, "+", 1);
                }
                dx = LOWORD((int32_t)((int32_t)(dxBar * (app.lpui->cMinGrafMax < pl.rgwtMin[i] ? app.lpui->cMinGrafMax : pl.rgwtMin[i])) / app.lpui->cMinGrafMax));
                win32::SetRect(&rcGauge, xL + 1, yCur + 4, xL + dx + 1, yCur + dyRow - 1);
                if (dx != 0) {
                    win32::FillRect(hdc, &rcGauge, app.lpui->rghbrMinSum[i][0]);
                }
                if (pl.det >= detSome) {
                    for (iPass = 0; iPass < 2; iPass++) {
                        win32::SelectObject(hdc, app.lpui->rghbrMinSum[i][iPass]);
                        yTop = yCur + 4;
                        yBot = yCur + dyRow - 2;
                        c = 1;
                        xBeg = xL + 1 + iPass + win32::MulDiv(100 >= pl.rgMinConc[i] ? pl.rgMinConc[i] : 100, dxBar - 8, 100);
                        while (1) {
                            win32::PatBlt(hdc, xBeg, yTop++, c, 1, win32::PATCOPY);
                            if (yTop > yBot)
                                break;
                            win32::PatBlt(hdc, xBeg--, yBot--, c, 1, win32::PATCOPY);
                            c += 2;
                        }
                    }
                }
                i++;
                yCur += dyRow;
            }
            win32::SetBkMode(hdc, win32::OPAQUE);
            win32::SelectObject(hdc, hbrSav);
            app.state.rgplr[app.state.idPlayer] = plrSav;
            goto FinishUp;
        } else {
            if (app.state.sel.scan.idpl == idplNone)
                goto FinishUp;
            hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, app.lpui->hbmpUnknownPlanet);
            win32::SetTextColor(hdc, 0);
            win32::BitBlt(hdc, ((prc->right - prc->left - 0x40) >> 1) + prc->left, ((prc->bottom - prc->top - 0x40) >> 1) + prc->top, 64, 64, hdcMem, 0, 0, win32::SRCCOPY);
            win32::SelectObject(hdcMem, hbmpSav);
        }
    }
FinishUp:
    win32::SetBkMode(hdc, bkMode);
    win32::SetTextColor(hdc, crFore);
    win32::SetBkColor(hdc, crBack);
    win32::DeleteDC(hdcMem);
    return;
}

HtMineType HtMineWindow(APPSESSION &app, HWND hwnd, int16_t x, int16_t y) {
    PLANET     pl;
    int16_t    dyRow;
    int16_t    yCur;
    GrobjClass grobj;
    RECT       rc;

    grobj = app.state.sel.scan.grobj;
    if (grobj == grobjOther) {
        if (app.state.sel.scan.grobjFull & grobjPlanet) {
            grobj = grobjPlanet;
        } else if (app.state.sel.scan.grobjFull & grobjFleet) {
            grobj = grobjFleet;
        } else {
            grobj = grobjNone;
        }
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    rc.top += app.state.dyArial8 * 2;
    if (y < rc.top - 5 && y > 4 && x < rc.right - 4 && x > rc.right - rc.top && FOtherStuffAtScanSel(app)) {
        return htMineScanSel;
    }
    if (grobj == grobjFleet || (grobj == grobjThing && app.state.lpThings[app.state.sel.scan.ith].ith() != ithWormhole && app.state.lpThings[app.state.sel.scan.ith].ith() != ithMysteryTrader)) {
        if (x >= rc.left + 25 && x < rc.left + 57 && y >= rc.top + 71 && y < rc.top + 103) {
            return htMineOwner;
        }
        if (grobj == grobjThing && app.state.lpThings[app.state.sel.scan.ith].ith() == ithMinefield && x >= rc.left + 9 && x < rc.left + 73 && y >= rc.top + 9 && y < rc.top + 73) {
            return htMineMinefieldType;
        }
        if (grobj == grobjThing) {
            return htMineNone;
        }
        if (x >= rc.left + 9 && x < rc.left + 73 && y >= rc.top + 9 && y < rc.top + 73) {
            return htMineShipOrFleet;
        }
        if (x >= rc.left + 92 && y >= rc.top + 4 && y < rc.top + 7 + app.state.dyArial8) {
            return htMineShipOrFleet;
        }
        return htMineNone;
    }
    if (app.state.sel.scan.idpl == idplNone || !PlanetQueryIO().pfnLookup(app, app.state.sel.scan.idpl, &pl)) {
        return htMineNone;
    }
    rc.top -= 4;
    yCur = rc.top + 2;
    dyRow = (int16_t)(rc.bottom - rc.top - app.state.dyArial8 * 4 - 2) / 6;
    dyRow = (dyRow + 1) & 0xfffe;
    yCur += app.state.dyArial8 * 2 - 2;
    if (y < yCur) {
        if (y > rc.top) {
            if (x < rc.right - 24) {
                if (pl.iPlayer != app.state.idPlayer && x > (int16_t)(3 * rc.right) / 5 && y >= yCur - app.state.dyArial8 && pl.iPlayer != iplrNone) {
                    return htMineOwner;
                }
                return htMinePlanet;
            }
            if (pl.fStarbase) {
                return htMineStarbase;
            }
            return htMineNone;
        }
        return htMineNone;
    }
    if (y < 3 * dyRow + yCur) {
        return static_cast<HtMineType>((int16_t)(y - yCur) / dyRow + 6);
    }
    yCur += 3 * dyRow + (app.state.dyArial8 >> 1) + 1;
    if (y < 3 * dyRow + yCur) {
        return static_cast<HtMineType>((int16_t)(y - yCur) / dyRow + 1);
    }
    return htMineScale;
}

void MineClick(APPSESSION &app, int16_t x, int16_t y, int16_t msg, int16_t sks) {
    PLANET    *lppl;
    HtMineType ht;
    PART       part;
    char       rgsz[16][32];
    char      *rgpsz[16];
    int16_t    c;
    FLEET     *lpfl;
    int16_t    rgid[16];
    int16_t    ishdef;
    int16_t    rgMin[3];
    PLANET     pl;
    int16_t    rgCost[3];
    int16_t    rgMax[3];
    int16_t    fOurs;
    int16_t    i;
    int16_t    idNew;
    SCAN       scan;
    int16_t    rgi[9];
    int16_t    iChecked;
    char      *psz[9];
    int32_t    rglQuan[3];
    int32_t    rglT[3];
    int16_t    ifl;
    int32_t    cMines;
    int32_t    lVal;

    ht = HtMineWindow(app, app.lpui->hwndMine, x, y);
    if ((msg != 516 || ht == htMineScanSel || ht == htMineShipOrFleet) && ht <= htMineMinefieldType) {
        switch (ht) {
        default:
            break;
        case htMineOwner:
            if (app.state.sel.scan.grobj == grobjPlanet) {
                lppl = LpplFromId(app, app.state.sel.scan.idpl);
                app.lpui->GlobalPD.iPlayer = lppl->iPlayer;
            } else if (app.state.sel.scan.grobj == grobjThing) {
                app.lpui->GlobalPD.iPlayer = app.state.lpThings[app.state.sel.scan.ith].iplr();
            } else {
                app.lpui->GlobalPD.iPlayer = app.state.rglpfl[app.state.sel.scan.ifl]->iplr();
            }
            app.lpui->GlobalPD.grPopup = grPopupPlayer;
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMineMinefieldType:
            part.hs.iItem = app.lpui->mpiTypeiItem[app.state.lpThings[app.state.sel.scan.ith].thm().iType()];
            part.hs.grhst = hstMines;
            PartQueryIO().pfnLookup(app, &part);
            app.lpui->GlobalPD.grPopup = grPopupComponent;
            app.lpui->GlobalPD.part = part;
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMineShipOrFleet:
            if (msg == 516) {
                lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
                c = 0;
                for (ishdef = 0; ishdef < 16; ishdef++) {
                    if (lpfl->rgcsh[ishdef] > 0) {
                        rgid[c] = ishdef;
                        strcpy(rgsz[c], app.state.rglpshdef[lpfl->iPlayer][ishdef].hul.szClass);
                        rgpsz[c] = rgsz[c];
                        c++;
                    }
                }
                if (c > 1) {
                    c = ControlIO().pfnPopupMenu(app, app.lpui->hwndMine, x, y, c, NULL, rgpsz, -1, 1);
                    if (c == -1)
                        break;
                } else {
                    c = 0;
                }
                app.lpui->GlobalPD.grPopup = grPopupShdef;
                app.lpui->GlobalPD.lpshdef = app.state.rglpshdef[lpfl->iPlayer] + rgid[c];
                app.lpui->GlobalPD.fHideCounts = app.state.idPlayer != lpfl->iPlayer;
                app.lpui->GlobalPD.fShowDamage = 0;
                app.lpui->GlobalPD.SetFToken(0);
                app.lpui->GlobalPD.fSummary = 1;
            } else {
                app.lpui->GlobalPD.grPopup = grPopupFleet;
                app.lpui->GlobalPD.lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
                app.lpui->GlobalPD.fRedDamage = app.lpui->GlobalPD.lpfl->det == detAll;
                app.lpui->GlobalPD.grbit = 0xff;
            }
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMineStarbase:
            app.lpui->GlobalPD.grPopup = grPopupShdef;
            lppl = LpplFromId(app, app.state.sel.scan.idpl);
            app.lpui->GlobalPD.lpshdef = app.state.rglpshdefSB[lppl->iPlayer] + lppl->isb;
            app.lpui->GlobalPD.fHideCounts = app.state.idPlayer != lppl->iPlayer;
            app.lpui->GlobalPD.fShowDamage = 1;
            app.lpui->GlobalPD.SetFToken(0);
            app.lpui->GlobalPD.fSummary = 1;
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMinePlanet:
            app.lpui->GlobalPD.grPopup = grPopupPlanet;
            app.lpui->GlobalPD.idPlanet = app.state.sel.scan.idpl;
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMineEnvVar0:
        case htMineEnvVar1:
        case htMineEnvVar2:
            PlanetQueryIO().pfnLookup(app, app.state.sel.scan.idpl, &pl);
            app.lpui->GlobalPD.grPopup = grPopupPlanetEnv;
            app.lpui->GlobalPD.idPlanet = pl.id;
            app.lpui->GlobalPD.iPlanetVar = ht - 6;
            if (pl.det >= detSome) {
                app.lpui->GlobalPD.iPlanVal = pl.rgEnvVar[app.lpui->GlobalPD.iPlanetVar];
            } else {
                app.lpui->GlobalPD.iPlanVal = envNone;
            }
            if (pl.det >= detSome && ColonyIO().pfnCanTerraform(app, &pl, rgMin, rgMax, rgCost, 1)) {
                app.lpui->GlobalPD.iPlanMin = rgMin[app.lpui->GlobalPD.iPlanetVar];
                app.lpui->GlobalPD.SetIPlanMax(rgMax[app.lpui->GlobalPD.iPlanetVar]);
                if (app.lpui->GlobalPD.iPlanMin == envNone) {
                    app.lpui->GlobalPD.iPlanMin = app.lpui->GlobalPD.iPlanVal;
                }
                if (app.lpui->GlobalPD.iPlanMax == envNone) {
                    app.lpui->GlobalPD.SetIPlanMax(app.lpui->GlobalPD.iPlanVal);
                }
                if (app.lpui->GlobalPD.iPlanMin == app.lpui->GlobalPD.iPlanMax)
                    goto NoTerra;
            } else {
            NoTerra:
                app.lpui->GlobalPD.iPlanMin = envNone;
                app.lpui->GlobalPD.SetIPlanMax(envNone);
            }
            app.lpui->GlobalPD.iPlrVal = app.state.rgplr[app.state.idPlayer].rgEnvVar[app.lpui->GlobalPD.iPlanetVar];
            app.lpui->GlobalPD.SetIPlrMin(app.state.rgplr[app.state.idPlayer].rgEnvVarMin[app.lpui->GlobalPD.iPlanetVar]);
            app.lpui->GlobalPD.iPlrMax = app.state.rgplr[app.state.idPlayer].rgEnvVarMax[app.lpui->GlobalPD.iPlanetVar];
            Popup(app, app.lpui->hwndMine, x, y);
            break;
        case htMineScanSel:
            if (msg == 516) {
                PopupMineralScanChoices(app, app.lpui->hwndMine, x, y);
                break;
            }
            scan = app.state.sel.scan;
            scan.iwp = 0;
            if (scan.grobj == grobjThing) {
                i = scan.ith + 1;
            CheckThing:
                for (; i < app.state.cThing; i++) {
                    if (app.state.lpThings[i].pt().x() == scan.pt.x && app.state.lpThings[i].pt().y() == scan.pt.y)
                        break;
                }
                if (i < app.state.cThing) {
                    scan.ith = i;
                    scan.grobj = grobjThing;
                    idNew = app.state.lpThings[i].idFull();
                    fOurs = 0;
                    goto ChangeIt;
                }
                if (scan.grobjFull & grobjPlanet)
                    goto CheckPlanet;
                if (scan.grobjFull & grobjFleet)
                    goto CheckFleet;
                if (scan.grobjFull & grobjThing) {
                    i = 0;
                    goto CheckThing;
                }
            }
            i = scan.grobj == grobjFleet ? scan.ifl + 1 : 0;
            for (; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i] || (scan.pt.x == lpfl->pt.x && scan.pt.y == lpfl->pt.y))
                    break;
            }
            if (i < app.state.cFleet) {
                scan.ifl = i;
                scan.grobj = grobjFleet;
                idNew = app.state.rglpfl[i]->id;
                fOurs = app.state.rglpfl[i]->iPlayer == app.state.idPlayer;
                goto ChangeIt;
            }
            if (scan.grobjFull & grobjThing) {
                i = 0;
                goto CheckThing;
            }
            if (scan.grobjFull & grobjPlanet) {
            CheckPlanet:
                scan.grobj = grobjPlanet;
                idNew = scan.idpl;
                lppl = LpplFromId(app, idNew);
                if (!lppl) {
                    fOurs = 0;
                } else {
                    fOurs = lppl->iPlayer == app.state.idPlayer;
                }
                goto ChangeIt;
            } else {
            CheckFleet:
                for (i = 0; i < app.state.cFleet; i++) {
                    lpfl = app.state.rglpfl[i];
                    if (!app.state.rglpfl[i] || (scan.pt.x == lpfl->pt.x && scan.pt.y == lpfl->pt.y))
                        break;
                }
                if (i == app.state.cFleet && (scan.grobjFull & grobjThing)) {
                    i = 0;
                    goto CheckThing;
                }
                scan.grobj = grobjFleet;
                scan.ifl = i;
                idNew = app.state.rglpfl[i]->id;
                fOurs = app.state.rglpfl[i]->iPlayer == app.state.idPlayer;
            }
        ChangeIt:
            if (!fOurs || app.state.sel.grobj != grobjFleet) {
                scan.iwp = app.state.sel.scan.iwp;
            }
            ChangeScanSel(app, &scan, 2);
            if (!fOurs)
                break;
            RedrawScanSel(app, NULL, 0);
            ChangeMainObjSel(app, scan.grobj, idNew);
            RedrawScanSel(app, NULL, 1);
            break;
        case htMineScale:
            iChecked = -1;
            rgi[0] = 100;
            rgi[1] = 500;
            rgi[2] = 1000;
            rgi[3] = 2500;
            rgi[4] = 5000;
            rgi[5] = 7500;
            rgi[6] = 10000;
            rgi[7] = 20000;
            rgi[8] = 30000;
            for (i = 0; i < 9; i++) {
                win32::wsprintfA(rgsz[i], app.lpui->PCTDKT, rgi[i]);
                psz[i] = rgsz[i];
                if (rgi[i] == app.lpui->cMinGrafMax) {
                    iChecked = i;
                }
            }
            i = ControlIO().pfnPopupMenu(app, app.lpui->hwndMine, x, y, 9, NULL, psz, iChecked, 1);
            if (i == -1 || rgi[i] == app.lpui->cMinGrafMax)
                break;
            app.lpui->cMinGrafMax = rgi[i];
            ControlIO().pfnInvalidateRect(app.lpui->hwndMine, NULL, 1);
            if ((app.lpui->grbitScan & grbitScanViewMask) != 1)
                break;
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, NULL, 1);
            break;
        case htMineMineralConc1:
        case htMineMineralConc2:
        case htMineMineralConc3:
            PlanetQueryIO().pfnLookup(app, app.state.sel.scan.idpl, &pl);
            app.lpui->GlobalPD.grPopup = grPopupMineral;
            app.lpui->GlobalPD.SetRgi(0, (int16_t)(ht - 1));
            for (i = 1; i <= 4; i++) {
                app.lpui->GlobalPD.SetRgi(i, -1);
            }
            if (pl.det >= detSome) {
                app.lpui->GlobalPD.SetRgi(3, (uint32_t)pl.rgpctMinLevel[ht + 2]);
                app.lpui->GlobalPD.SetRgi(1, pl.fHomeworld);
                if (pl.det > detSome) {
                    lVal = 0;
                    app.lpui->GlobalPD.SetRgi(2, pl.rgwtMin[ht - 1]);
                    ColonyIO().pfnEstimateMining(app, &pl, rglQuan, -1, 0);
                    app.lpui->GlobalPD.SetRgi(4, rglQuan[ht - 1]);
                    if (pl.iPlayer == iplrNone) {
                        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                            lpfl = app.state.rglpfl[ifl];
                            if (!app.state.rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == pl.id && lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead && lpfl->lpplord->rgord()[0].grTask == grTaskMine) {
                                cMines = ColonyIO().pfnFleetMining(app, lpfl);
                                if (cMines > 0) {
                                    ColonyIO().pfnEstimateMining(app, &pl, rglT, cMines, 0);
                                    lVal += rglT[ht - 1];
                                }
                            }
                        }
                        if (lVal > 0) {
                            app.lpui->GlobalPD.SetRgi(4, lVal);
                        }
                    }
                }
            }
            Popup(app, app.lpui->hwndMine, x, y);
        }
    }
    return;
}

void SetMineralTitleBar(APPSESSION &app, HWND hwnd) {
    char       szDeepSpace[40];
    char       szSummary[40];
    int16_t    fVisCB;
    char      *psz;
    GrobjClass grobj;
    RECT       rc;

    fVisCB = 0;
    TextIO().pfnCchGetString(app, idsDeepSpace, szDeepSpace);
    TextIO().pfnCchGetString(app, idsSummary, szSummary);
    grobj = app.state.sel.scan.grobj == grobjOther ? app.state.sel.scan.grobjFull : app.state.sel.scan.grobj;
    if (grobj & grobjPlanet) {
        if (app.state.sel.scan.idpl != idplNone) {
            psz = TextIO().pfnPlanetName(app, app.state.sel.scan.idpl);
            strcat(psz, szSummary);
        }
    } else if (grobj & grobjFleet) {
        if (app.state.sel.scan.ifl != iflNone) {
            psz = PszGetFleetName(app, app.state.rglpfl[app.state.sel.scan.ifl]->id);
            strcat(psz, szSummary);
        }
    } else if (!(grobj & grobjThing)) {
        psz = szDeepSpace;
    } else if (app.state.sel.scan.ith != ithNone) {
        fVisCB = 1;
        psz = PszGetThingName(app, app.state.lpThings[app.state.sel.scan.ith].idFull());
        strcat(psz, szSummary);
    }
    strcpy(app.lpui->szMineralTitle, psz);
    ControlIO().pfnInvalidateRect(hwnd, NULL, 1);
    if (fVisCB) {
        fVisCB = app.state.lpThings[app.state.sel.scan.ith].ith() == ithMinefield && app.state.lpThings[app.state.sel.scan.ith].thm().iType() == mineStandard && app.state.lpThings[app.state.sel.scan.ith].iplr() == app.state.idPlayer &&
                 RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMines;
    }
    if (fVisCB) {
        ControlIO().pfnSendMessage(app.lpui->hwndMineCB, win32::BM_SETCHECK, app.state.lpThings[app.state.sel.scan.ith].thm().fDetonate() != 0, 0);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    ControlIO().pfnSetWindowPos(app.lpui->hwndMineCB, NULL, 80, rc.bottom - app.state.dyArial8 * 2, rc.right - 88, app.state.dyArial8 + 4, (!fVisCB ? 0x80 : 0x40) | 4);
    return;
}

void DrawSelectionArrow(APPSESSION &app, HDC hdc, RECT *prc, int16_t fEnabled) {
    HBITMAP hbmpSav;
    HDC     hdcMem;
    int16_t xCtr;

    hdcMem = win32::CreateCompatibleDC(hdc);
    hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, app.lpui->hbmpScanner);
    xCtr = (int16_t)(prc->right - prc->left) / 2 + prc->left;
    win32::BitBlt(hdc, xCtr - 5, ((prc->bottom - prc->top) >> 1) + prc->top - 5, 11, 12, hdcMem, 35, 57, win32::SRCAND);
    win32::BitBlt(hdc, xCtr - 5, ((prc->bottom - prc->top) >> 1) + prc->top - 5, 11, 12, hdcMem, !fEnabled ? 0 : 24, !fEnabled ? 45 : 57, win32::SRCPAINT);
    win32::SelectObject(hdcMem, hbmpSav);
    win32::DeleteDC(hdcMem);
    return;
}

void DrawDiamond(APPSESSION &app, HDC hdc, RECT *prc, HBRUSH hbr) {
    HBRUSH  hbrSav;
    int16_t yTop;
    int16_t yBot;
    int16_t xCtr;
    int16_t dx;
    int16_t xCur;

    xCtr = (int16_t)(prc->right - prc->left) / 2 + prc->left;
    yTop = prc->top + 1;
    yBot = prc->bottom - 2;
    xCur = xCtr - 1;
    hbrSav = (HBRUSH)win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
    while (yTop <= yBot) {
        win32::PatBlt(hdc, xCur, yTop++, 2, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xCur--, yBot--, 2, 1, win32::PATCOPY);
    }
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, xCtr, prc->top, 1, 1, win32::PATCOPY);
    win32::PatBlt(hdc, xCtr, prc->bottom - 1, 1, 1, win32::PATCOPY);
    yTop = prc->top + 1;
    yBot = prc->bottom - 2;
    xCur = xCtr;
    while (yTop <= yBot) {
        win32::PatBlt(hdc, xCur, yTop++, 2, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xCur++, yBot--, 2, 1, win32::PATCOPY);
    }
    yTop = prc->top + 4;
    yBot = prc->bottom - 5;
    xCur = xCtr;
    dx = 1;
    win32::SelectObject(hdc, hbr);
    while (yTop <= yBot) {
        win32::PatBlt(hdc, xCur, yTop++, dx, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xCur--, yBot--, dx, 1, win32::PATCOPY);
        dx += 2;
    }
    win32::SelectObject(hdc, hbrSav);
    return;
}

void PopupMineralScanChoices(APPSESSION &app, HWND hwnd, int16_t x, int16_t y) {
    int16_t fSep;
    int16_t id;
    int16_t fOurs;
    PLANET *lppl;
    int16_t i;
    int16_t c;
    THING  *lpth;
    FLEET  *lpfl;
    THING  *lpthMac;
    int32_t rgid[100];
    int16_t idNew;
    int16_t iChecked;
    SCAN    scan;

    iChecked = -1;
    if (app.state.sel.scan.idpl != idplNone) {
        rgid[0] = app.state.sel.scan.idpl;
        rgid[1] = -1;
        c = 2;
        if (app.state.sel.scan.grobj == grobjPlanet) {
            iChecked = 0;
        }
    } else {
        c = 0;
    }
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (app.state.sel.scan.pt.x == lpfl->pt.x && app.state.sel.scan.pt.y == lpfl->pt.y) {
            if (app.state.sel.scan.grobj == grobjFleet && app.state.rglpfl[app.state.sel.scan.ifl]->id == lpfl->id) {
                iChecked = c;
            }
            rgid[c++] = lpfl->id | 0x80000000;
            if (c >= 100)
                break;
        }
    }
    if (c == 2 && app.state.sel.scan.idpl != idplNone) {
        c = 1;
    }
    fSep = c == 0;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (app.state.sel.scan.pt.x == lpth->pt().x() && app.state.sel.scan.pt.y == lpth->pt().y()) {
            if (!fSep) {
                rgid[c++] = -1;
                fSep = 1;
            }
            if (app.state.sel.scan.grobj == grobjThing && (int16_t)((uint8_t *)lpth - (uint8_t *)app.state.lpThings) / 18 == app.state.sel.scan.ith) {
                iChecked = c;
            }
            rgid[c++] = (uint32_t)(uint16_t)lpth->idFull() | 0x20000000;
        }
    }
    i = ControlIO().pfnPopupMenu(app, hwnd, x, y, c, rgid, NULL, iChecked, 1);
    if (i >= 0) {
        scan = app.state.sel.scan;
        if (rgid[i] & 0x80000000) {
            scan.grobj = grobjFleet;
            id = LOWORD(rgid[i]);
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i] || lpfl->id == id)
                    break;
            }
            scan.ifl = i;
            idNew = id;
            fOurs = lpfl->iPlayer == app.state.idPlayer;
        } else if (rgid[i] & 0x20000000) {
            scan.grobj = grobjThing;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac && lpth->idFull() != LOWORD(rgid[i]); lpth++) {
            }
            scan.ith = (int16_t)((uint8_t *)lpth - (uint8_t *)app.state.lpThings) / 18;
            idNew = lpth->idFull();
            fOurs = 0;
        } else {
            scan.grobj = grobjPlanet;
            idNew = app.state.sel.scan.idpl;
            lppl = LpplFromId(app, idNew);
            fOurs = lppl->iPlayer == app.state.idPlayer;
        }
        ChangeScanSel(app, &scan, 2);
        if (fOurs) {
            RedrawScanSel(app, NULL, 0);
            ChangeMainObjSel(app, scan.grobj, idNew);
            RedrawScanSel(app, NULL, 1);
        }
    }
    return;
}

// InvalidateMine redraws the mineral window.
void InvalidateMine(APPSESSION &app) {
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, NULL, 1);
    return;
}
