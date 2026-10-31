#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"

LRESULT CALLBACK PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC                hdc;
    PAINTSTRUCT        ps;
    XFER               xf;
    int16_t            i;
    char              *psz;
    int32_t            lSel;
    RECT               rc;
    POINT16            pt;
    HCURSOR            hcs;
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    PLANET            *lpplMac;
    PLANET            *lppl;
    FLEET             *lpfl;

    switch (message) {
    case WM_MDIACTIVATE:
        hwndActive = (lParam == (LPARAM)hwnd) == 0 ? NULL : hwnd;
        return 0;
    case WM_CREATE:
        SetPlanetTitleBar(hwnd);
        for (i = 0; i < 3; i++) {
            rghwndOrderDD[i] = ui::CreateWindowEx(0, szCombobox, "OrdDD", (i == 1 ? 528 : 0) | (CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL), 100, 100, 200,
                                            i == 2 ? 160 : 80, hwnd, NULL, hInst, NULL);
            ControlIO().pfnSendMessage(rghwndOrderDD[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        for (i = idsTaskHere; i <= idsTransferFleet; i++) {
            psz = TextIO().pfnCompressed(static_cast<StringId>(i));
            ControlIO().pfnSendMessage(rghwndOrderDD[0], CB_ADDSTRING, 0, (LPARAM)psz);
        }
        hwndOrderED = ui::CreateWindowEx(0, szEdit, NULL, ES_RIGHT | WS_CHILD | WS_BORDER, 100, 100, 200, 50, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndOrderED, EM_LIMITTEXT, 4, 0);
        ControlIO().pfnSendMessage(hwndOrderED, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        lpfnRealEditProc = (WNDPROC)win32::GetWindowLongPtr(hwndOrderED, GWLP_WNDPROC);
        win32::SetWindowLongPtr(hwndOrderED, GWLP_WNDPROC, (LONG_PTR)lpfnFakeEditProc);
        hwndBattleDD = ui::CreateWindowEx(0, szCombobox, "BattleDD", CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndBattleDD, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndShipDD = ui::CreateWindowEx(0, szCombobox, "ShipDD", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd,
                                  NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndShipDD, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        ControlIO().pfnGetClientRect(hwndShipDD, &rc);
        dyShipDD = rc.bottom;
        hwndShipLB = ui::CreateWindowEx(0, szListbox, "ShipLB", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_DISABLENOSCROLL | WS_CHILD | WS_BORDER | WS_VSCROLL, 100, 100,
                                  200, 80, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndShipLB, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndFleetCompLB =
            ui::CreateWindowEx(0, szListbox, "FleetCompLB", LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndFleetCompLB, WM_SETFONT, (WPARAM)rghfontArial8[0], 0);
        hwndPlanetProdLB =
            ui::CreateWindowEx(0, szListbox, "PlanetProdLB", LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndPlanetProdLB, WM_SETFONT, (WPARAM)rghfontArial8[0], 0);
        for (i = 0; i < 13; i++) {
            psz = TextIO().pfnCompressed(static_cast<StringId>(idsCargo2 + i));
            rghwndBtn[i] = ui::CreateWindowEx(0, szButton, psz, WS_CHILD, 100, 100, 100, dyArial8 * 2, hwnd, NULL, hInst, NULL);
            ControlIO().pfnSendMessage(rghwndBtn[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        hwndRepCB =
            ui::CreateWindowEx(0, szButton, TextIO().pfnCompressed(idsRepeatOrders), BS_AUTOCHECKBOX | WS_CHILD, 100, 100, 150, dyArial8, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndRepCB, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        break;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == hwndRepCB) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            win32::SetTextColor((HDC)wParam, crButtonText);
            return (LRESULT)hbrButtonFace;
        }
        goto Default;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = dxWinFrame * 2 + 198;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = dyWinFrame * 2 + 198 + dyTitleBar;
        goto Default;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        ControlIO().pfnSetFocus(hwndFrame);
        PlanetClick(LOWORD(lParam), HIWORD(lParam), wParam, message == WM_RBUTTONDOWN);
        return 0;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnDrawPlanShip(hdc, tileAll);
        win32::EndPaint(hwnd, &ps);
        break;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        hcs = ClickInShipOrders(pt, 0, TRUE, FALSE);
        if (!hcs) {
            hcs = ClickInPlanetOrders(pt, 0, TRUE, FALSE);
        }
        if (!hcs)
            goto Default;
        win32::SetCursor(hcs);
        return 1;
    case WM_DRAWITEM:
        lpdis = (DRAWITEMSTRUCT *)lParam;
        if (lpdis->itemID == -1) {
            HandleFocusState(lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case ODA_DRAWENTIRE:
                DrawCBEntireItem(lpdis, -4);
                break;
            case ODA_SELECT:
                DrawCBEntireItem(lpdis, -4);
                break;
            case ODA_FOCUS:
                DrawCBEntireItem(lpdis, -4);
            }
        }
        return 1;
    case WM_MEASUREITEM:
        lpmis = (MEASUREITEMSTRUCT *)lParam;
        lpmis->itemHeight = dyArial8 + 2;
        return 1;
    case WM_CHAR:
        if (wParam != 'f' && wParam != 'F') {
            return 0;
        }
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == idPlayer) {
                if (sel.grobj == grobjPlanet && sel.id == lppl->id)
                    break;
                PresentationIO().pfnSelectPlanet(0, lppl->id);
                return 0;
            }
        }
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->iPlayer == idPlayer) {
                if (sel.grobj == grobjFleet && sel.id == lpfl->id)
                    break;
                PresentationIO().pfnSelectFleet(0, lpfl->id);
                return 0;
            }
        }
        return 0;
    case WM_COMMAND:
        if (sel.grobj == grobjFleet) {
            ShipCommandProc(hwnd, wParam, lParam);
            return 0;
        }
        if ((HWND)lParam == hwndShipDD) {
            if (HIWORD(wParam) == 1) {
                ControlIO().pfnDrawPlanShip(NULL, static_cast<TileBits>(tileShipList | tileErase));
            }
        } else {
            if ((HWND)lParam == rghwndBtn[4] && HIWORD(wParam) == 0) {
                if (ControlIO().pfnGetKeyState(VK_SHIFT) < 0) {
                    PresentationIO().pfnSelectPlanet(0, IdFindAdjStarbase(sel.pl.id, FALSE));
                    goto LRefocus;
                }
                PresentationIO().pfnSelectPlanet(-1, 0);
            LRefocus:
                ControlIO().pfnSetFocus(hwndFrame);
                return 0;
            } else if ((HWND)lParam == rghwndBtn[5] && HIWORD(wParam) == 0) {
                if (ControlIO().pfnGetKeyState(VK_SHIFT) < 0) {
                    PresentationIO().pfnSelectPlanet(0, IdFindAdjStarbase(sel.pl.id, TRUE));
                    goto LRefocus;
                }
                PresentationIO().pfnSelectPlanet(1, 0);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[0] && HIWORD(wParam) == 0) {
                lSel = ControlIO().pfnSendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
                if (lSel == -1 || !FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, idflNone)) {
                    return 0;
                }
                TransferStuff(sel.pl.id, grobjPlanet, xf.id, xf.grobj, mdXferCargo);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[1] && HIWORD(wParam) == 0) {
                lSel = ControlIO().pfnSendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
                if (lSel != -1 && FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, idflNone) && xf.grobj == grobjFleet) {
                    PresentationIO().pfnSelectFleet(0, xf.id);
                }
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[2] && HIWORD(wParam) == 0) {
                pt.x = 610;
                pt.y = 470;
                ShipBuilder(pt);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[11] && HIWORD(wParam) == 0) {
                ChangeProduction(FALSE);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[12] && HIWORD(wParam) == 0) {
                if (AlertIO().pfnAlertSz(TextIO().pfnFormat(idsSureWantDeleteEverythingPlanetsProductionQueue, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES) {
                    return 0;
                }
                ChangeProduction(TRUE);
                goto LRefocus;
            } else {
                if ((HWND)lParam != hwndPlanetProdLB || HIWORD(wParam) != 1)
                    goto Default;
                ControlIO().pfnDrawPlanShip(NULL, tileProductionOrOrbit);
                if (!gd.fTutorial)
                    goto Default;
                tutor.fProgress = TRUE;
                PresentationIO().pfnTutor();
                goto Default;
            }
        }
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

int16_t FDrawTileNC(HDC hdc, TILE *ptile, RECT *prc, char *pszTitle) {
    int16_t bt;
    RECT    rcT;

    bt = 112;
    prc->left = ptile->iCol * 198 + 4;
    prc->right = prc->left + 190;
    prc->top = ptile->yTop;
    prc->bottom = (!ptile->fPopped ? dyArial8 + 3 : ptile->dyFull) + prc->top;
    if (ptile->fMinDraw && !ptile->fMinTitle)
        goto FinishUp;

    if (!ptile->fMinDraw) {
        _Draw3dFrame(hdc, prc, 0);
    }
    rcT = *prc;
    ExpandRc(&rcT, -1, -1);
    rcT.bottom = rcT.top + dyArial8 + 2;
    win32::SelectObject(hdc, rghfontArial8[1]);
    win32::SetTextColor(hdc, crButtonText);
    win32::SetBkColor(hdc, crButtonFace);
    if (!ptile->fMinDraw) {
        _Draw3dFrame(hdc, &rcT, 0);
    }
    RcCtrTextOut(hdc, &rcT, pszTitle, -1);
    win32::SetRect(&rcT, prc->right - 17, prc->top + 1, prc->right, rcT.bottom + 1);
    if (ptile->fPopped) {
        bt = bt;
    } else {
        bt |= 1;
    }
    DrawBtn(hdc, &rcT, bt, FALSE, NULL);
    win32::SelectObject(hdc, hbrButtonShadow);
    win32::PatBlt(hdc, prc->right - 18, prc->top + 1, 1, rcT.bottom - prc->top - 1, PATCOPY);

FinishUp:
    prc->top += dyArial8 + 4;
    return ptile->fPopped;
}

void DrawPlanetMinSum(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int16_t yTop;
    int16_t xRight;
    int16_t c;
    int16_t i;
    int16_t xLeft;
    HBRUSH  hbrSav;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl();
    if (ptile->fFixCtls) {
        rgrcRef[6].top = -5;
        rgrcRef[6].bottom = -6;
        rgrcRef[7].top = -5;
        rgrcRef[7].bottom = -6;
        rgrcRef[8].top = -5;
        rgrcRef[8].bottom = -6;
        rgrcRef[9].top = -5;
        rgrcRef[9].bottom = -6;
        rgrcRef[11].top = -5;
        rgrcRef[11].bottom = -6;
        rgrcRef[10].top = -5;
        rgrcRef[10].bottom = -6;
    }
    if (FDrawTileNC(hdc, ptile, &rc, TextIO().pfnCompressed(idsMineralsHand))) {
        if (!ppl) {
            ppl = &sel.pl;
        }
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        dxRight = dxMaxMineralQuan;
        win32::SetRect(&rgrcRef[6], xLeft, yTop, xRight, 3 * dyArial8 + yTop);
        for (i = 0; i <= 2; i++) {
            if (!ptile->fMinDraw) {
                win32::SelectObject(hdc, rghfontArial8[1]);
                win32::SetTextColor(hdc, rgcrMinerals[i]);
                win32::TextOut(hdc, xLeft, yTop, rgszMinerals[i], win32::lstrlen(rgszMinerals[i]));
            }
            win32::SelectObject(hdc, rghfontArial8[0]);
            win32::SetTextColor(hdc, crButtonText);
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsLdkt), ppl->rgwtMin[i]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
        }
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonHilite);
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        win32::SetRect(&rgrcRef[7], xLeft, yTop, xRight, dyArial8 * 2 + yTop);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsMines4, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsD), ColonyIO().pfnMinesOperating(ppl));
        } else {
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDD), ppl->cMines, ColonyIO().pfnOperableMines(ppl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight * 2);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsFactories4, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = TextIO().pfnCchGetString(idsN, szWork);
        } else {
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDD), ppl->cFactories, ColonyIO().pfnOperableFactories(ppl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight * 2);
        yTop += dyArial8;
    }
    return;
}

void DrawPlanetStats(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int32_t l2;
    int16_t yTop;
    int16_t xRight;
    int16_t c;
    int16_t cRes;
    int16_t dRangeP;
    float   pct;
    int16_t cResAvail;
    int16_t dRange;
    char   *psz;
    int16_t xLeft;
    HBRUSH  hbrSav;
    int32_t l;
    RECT    rc;
    PART    part;

    if (FDrawTileNC(hdc, ptile, &rc, TextIO().pfnCompressed(idsStatus))) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        win32::SelectObject(hdc, rghfontArial8[1]);
        c = TextIO().pfnCchGetString(idsPopulation4, szWork);
        l = GetTextExtent(hdc, szWork, c);
        c = TextIO().pfnCchGetString(idsScannerType, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(idsScannerRange, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(idsDefenses4, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(idsDefenseType, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(idsDefCoverage, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(idsResourcesYear, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        dxRight = xRight - xLeft - LOWORD(l);
        if (!ptile->fMinDraw) {
            c = TextIO().pfnCchGetString(idsPopulation4, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
        }
        win32::SelectObject(hdc, rghfontArial8[0]);
        win32::SetRect(&rgrcRef[9], xLeft, yTop, xRight, yTop + dyArial8);
        c = CommaFormatLong(szWork, (uint32_t)(sel.pl.rgwtMin[3] * 100));
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsResourcesYear, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        win32::SetRect(&rgrcRef[8], xLeft, yTop, xRight, yTop + dyArial8);
        cResAvail = ResearchIO().pfnResources(&sel.pl, idPlayer);
        cRes = cResAvail;
        if (!sel.pl.fNoResearch) {
            cResAvail -= win32::MulDiv(cRes, rgplr[idPlayer].pctResearch, 100);
        }
        c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDD), cResAvail, cRes);
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonHilite);
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsScannerType, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        win32::SetRect(&rgrcRef[11], xLeft, yTop, xRight, dyArial8 * 2 + yTop);
        if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            TextIO().pfnCchGetString(idsOrganic, szWork);
            dRange = GetPlanetScannerRange(&sel.pl, &dRangeP);
        } else if (sel.pl.iScanner == 31) {
            TextIO().pfnCchGetString(idsNone4, szWork);
            dRange = 0;
        } else {
            ProductionIO().pfnScanner(&part);
            strcpy(szWork, part.pcom().szName());
            dRange = GetPlanetScannerRange(&sel.pl, &dRangeP);
        }
        RightTextOut(hdc, xRight, yTop, szWork, 0, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsScannerRange, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        if (dRange > 0) {
            if (dRangeP > 0) {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDDLY), dRangeP, dRange);
            } else if (dRange < 100) {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDLightYears), dRange);
            } else {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDLY), dRange);
            }
        } else {
            TextIO().pfnCchGetString(idsNone4, szWork);
            c = 0;
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonHilite);
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDefenses4, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
        }
        win32::SelectObject(hdc, rghfontArial8[0]);
        win32::SetRect(&rgrcRef[10], xLeft, yTop, xRight, 3 * dyArial8 + yTop);
        if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = TextIO().pfnCchGetString(idsN, szWork);
        } else {
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDD), sel.pl.cDefenses, ColonyIO().pfnOperableDefenses(&sel.pl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDefenseType, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        if (sel.pl.cDefenses == 0) {
            TextIO().pfnCchGetString(RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, szWork);
            dRange = 0;
        } else {
            FGetBestDefensePart(&part);
            strcpy(szWork, part.pcom().szName());
            dRange = 1;
        }
        RightTextOut(hdc, xRight, yTop, szWork, 0, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDefCoverage, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
        }
        if (dRange != 0) {
            CalcPctSurvive(&sel.pl, &pct, NULL);
            pct = (float)((double)1.0 - pct);
            c = win32::wsprintf(szWork, PCTDXPCTDPCTPCT, LOWORD((int32_t)((double)pct * 100)),
                         LOWORD((int32_t)((pct - (double)(int16_t)LOWORD((int32_t)((double)pct * 100)) / 100.0) * 10000)));
        } else {
            c = TextIO().pfnCchGetString(RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, szWork);
            psz = szWork;
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
    }
    return;
}

void DrawPlanetStarbase(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t  fTwo;
    int16_t  dxRight;
    int16_t  iWarp;
    int16_t  bt;
    int16_t  yTop;
    int16_t  xRight;
    int16_t  c;
    SHDEF   *lpshdef;
    COLORREF crForeSav;
    uint16_t w;
    char    *psz;
    int16_t  xLeft;
    HBRUSH   hbrSav;
    int32_t  l;
    RECT     rc;

    if (ptile->fFixCtls) {
        rgrcRef[13].top = -5;
        rgrcRef[13].bottom = -6;
        rgrcRef[14].top = -5;
        rgrcRef[14].bottom = -6;
        rgrcRef[16].top = -5;
        rgrcRef[16].bottom = -6;
        rgrcRef[15].top = -5;
        rgrcRef[15].bottom = -6;
        ptile->fFixCtls = FALSE;
    }
    if (sel.pl.fStarbase) {
        lpshdef = rglpshdefSB[idPlayer] + sel.pl.isb;
        strcpy(szWork, lpshdef->hul.szClass);
        psz = szWork;
    } else {
        psz = TextIO().pfnCompressed(idsStarbase2);
    }
    if (FDrawTileNC(hdc, ptile, &rc, psz)) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        win32::SetRect(&rc, xLeft - 2, yTop, xRight + 2, rc.bottom - 2);
        win32::FillRect(hdc, &rc, hbrButtonFace);
        if (!sel.pl.fStarbase) {
            win32::SetRect(&rgrcRef[14], -5, -5, -6, -6);
            rgrcRef[15] = rgrcRef[14];
            rgrcRef[16] = rgrcRef[14];
        } else {
            win32::SetRect(&rgrcRef[14], xLeft, yTop, xRight, dyArial8 * 4 + yTop);
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDockCapacity, szWork);
            l = GetTextExtent(hdc, szWork, c);
            dxRight = xRight - xLeft - LOWORD(l);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
            w = PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.wtCargoMax;
            if (w == 0) {
                c = TextIO().pfnCchGetString(idsNone4, szWork);
            } else if ((uint32_t)w == 0xffff) {
                c = TextIO().pfnCchGetString(idsUnlimited, szWork);
            } else {
                c = win32::wsprintf(szWork, PCTDKT, w);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsArmor2, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            w = lpshdef->hul.dp;
            if (w == 0) {
                c = TextIO().pfnCchGetString(idsNone4, szWork);
            } else {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsLddp), w, 0);
            }
            win32::SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsShields2, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            l = DpShieldOfShdef(lpshdef, idPlayer);
            if (l == 0) {
                c = TextIO().pfnCchGetString(idsNone4, szWork);
            } else {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsLddp), l);
            }
            win32::SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDamage2, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
            w = sel.pl.pctDp;
            if (w != 0) {
                if (w < 5) {
                    w = 5;
                }
                c = win32::wsprintf(szWork, PCTDPCTPCT, (uint32_t)w / 5);
                crForeSav = win32::SetTextColor(hdc, 127);
            } else {
                c = TextIO().pfnCchGetString(idsNone4, szWork);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            if (w != 0) {
                win32::SetTextColor(hdc, crForeSav);
            }
            hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonHilite);
            win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
            win32::SelectObject(hdc, hbrSav);
            win32::SelectObject(hdc, rghfontArial8[1]);
            win32::SetRect(&rgrcRef[16], xLeft, yTop, xRight, yTop + dyArial8);
            c = TextIO().pfnCchGetString(idsMassDriver, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            iWarp = ColonyIO().pfnWarp(&sel.pl, &fTwo);
            if (iWarp > 0) {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsWarpD), iWarp);
                if (fTwo) {
                    szWork[c++] = '+';
                }
            } else {
                c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsNone4));
            }
            win32::SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsDestination3, szWork);
            win32::TextOut(hdc, xLeft, yTop, szWork, c);
            if (sel.pl.idFling == 0) {
                c = TextIO().pfnCchGetString(idsNone4, szWork);
            } else {
                psz = PszGetCompressedPlanet(rgidPlan[sel.pl.idFling - 1]);
                c = 0;
                strcpy(szWork, psz);
            }
            win32::SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            c = (int16_t)(xRight - xLeft) / 3;
            win32::SetRect(&rgrcRef[13], xLeft, yTop, xLeft + c, yTop + dyArial8 + 6);
            bt = 8;
            if (iWarp == 0) {
                bt |= 4;
            }
            DrawBtn(hdc, &rgrcRef[13], bt, gd.fSetMassMode, TextIO().pfnCompressed(idsSetDest));
            if (iWarp > 0) {
                win32::SetRect(&rgrcRef[15], xLeft + c + 4, yTop + 3, xRight, yTop + dyArial8 + 3);
                DrawMassWarpGauge(hdc, &rgrcRef[15], !fTwo ? iWarp : -iWarp, sel.pl.iWarpFling + 4);
            } else {
                win32::SetRect(&rgrcRef[15], -5, -5, -6, -6);
                rgrcRef[16] = rgrcRef[15];
            }
        }
    }
    return;
}

void DrawMassWarpGauge(HDC hdc, RECT *prc, int16_t iBest, int16_t iCur) {
    int32_t lMax;
    int16_t c;
    int16_t fTwoMAs;
    int16_t iMode;
    HBRUSH  hbr;
    int32_t lCur;
    int32_t l;

    fTwoMAs = iBest < 0;
    win32::SelectObject(hdc, rghfontArial8[1]);
    if (iCur < 5) {
        iCur = 5;
    }
    if (iBest < 0) {
        iBest = -iBest;
    }
    lMax = (int16_t)(iBest - 1);
    if (iCur <= iBest + fTwoMAs) {
        hbr = hbrPurple;
    } else if (iCur < iBest + fTwoMAs + 3) {
        hbr = hbrYellow;
    } else {
        hbr = hbrRed;
    }
    lCur = (int16_t)(iCur - 4);
    l = LDrawGauge(hdc, prc, 1, &lCur, &hbr, lMax);
    iMode = win32::SetBkMode(hdc, TRANSPARENT);
    c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsWarpLd), l + 4);
    l = GetTextExtent(hdc, szWork, c);
    RcCtrTextOut(hdc, prc, szWork, c);
    win32::SetBkMode(hdc, iMode);
    return;
}

void DrawPlanetProduction(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t dxRight;
    int16_t yTop;
    int16_t xStart;
    int16_t xRight;
    char    szT[40];
    int16_t i;
    int16_t c;
    int16_t dyWrong;
    char   *psz;
    int16_t iSel;
    int16_t cch;
    int16_t xLeft;
    RECT    rcT;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl();
    if (ptile->fFixCtls) {
        ui::ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ui::ShowWindow(rghwndBtn[11], SW_HIDE);
        ui::ShowWindow(rghwndBtn[12], SW_HIDE);
        rgrcRef[17].top = -5;
        rgrcRef[17].bottom = -6;
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, TextIO().pfnCompressed(idsProduction))) {
        ui::ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ui::ShowWindow(rghwndBtn[11], SW_HIDE);
        ui::ShowWindow(rghwndBtn[12], SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        yTop += 4;
        ControlIO().pfnGetClientRect(hwndPlanetProdLB, &rcT);
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        dyPlanetProdLB = (dyArial8 + 2) * (!gd.fSmallTileMode ? 5 : 3);
        dyWrong = dyPlanetProdLB - (rcT.bottom - rcT.top);
        if (dxPlanetProdLB == xRight - xLeft && dyWrong >= 0 && dyWrong < dyArial8) {
            swp |= SWP_NOSIZE;
        } else {
            dxPlanetProdLB = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(hwndPlanetProdLB, NULL, xLeft, yTop, xRight - xLeft, dyPlanetProdLB, swp);
        ui::ShowWindow(hwndPlanetProdLB, SW_SHOW);
        ControlIO().pfnGetClientRect(hwndPlanetProdLB, &rcT);
        dyPlanetProdLB = rcT.bottom - rcT.top;
        yTop += dyPlanetProdLB + 4;
        iSel = LOWORD(ControlIO().pfnSendMessage(hwndPlanetProdLB, LB_GETCURSEL, 0, 0));
        if (iSel < 0) {
            iSel = 0;
        }
        if (!ppl->lpplprod || ppl->lpplprod->iprodMac == 0) {
            c = 0;
            szWork[0] = 0;
        } else {
            psz = PszProductionETA(ppl, NULL, static_cast<ProdItemType>(iSel), NULL, NULL);
            c = strlen(psz);
        }
        if (c != 0) {
            win32::SelectObject(hdc, rghfontArial8[1]);
            cch = TextIO().pfnCchGetString(idsCompletion, szT);
            win32::TextOut(hdc, xLeft, yTop, szT, cch);
            dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
            win32::SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        } else {
            RightTextOut(hdc, xRight, yTop, szWork, 0, xRight - xLeft);
        }
        yTop += dyArial8;
        win32::SelectObject(hdc, rghfontArial8[1]);
        cch = TextIO().pfnCchGetString(idsRoute3, szT);
        win32::TextOut(hdc, xLeft, yTop, szT, cch);
        dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
        if (sel.pl.idRoute == 0) {
            c = TextIO().pfnCchGetString(idsNone4, szWork);
        } else {
            psz = PszGetCompressedPlanet(rgidPlan[sel.pl.idRoute - 1]);
            c = 0;
            strcpy(szWork, psz);
        }
        win32::SelectObject(hdc, rghfontArial8[0]);
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        c = (int16_t)(xRight - xLeft - 16) / 3;
        xStart = xLeft;
        i = 11;
        while (i <= 12) {
            ControlIO().pfnSetWindowPos(rghwndBtn[i], NULL, xStart, yTop, c, (dyArial8 >> 1) + dyArial8, SWP_NOZORDER | SWP_NOACTIVATE);
            ui::ShowWindow(rghwndBtn[i], SW_SHOW);
            i++;
            xStart += c + 8;
        }
        ControlIO().pfnEnableWindow(rghwndBtn[12], sel.pl.lpplprod && sel.pl.lpplprod->iprodMac != 0);
        win32::SetRect(&rgrcRef[17], xStart, yTop, xStart + c, yTop + dyArial8 + (dyArial8 >> 1));
        DrawBtn(hdc, &rgrcRef[17], 8, gd.fSetRouteMode, TextIO().pfnCompressed(idsRoute2));
    }
    return;
}

void DrawPlanShipBitmap(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t yTop;
    int16_t dy;
    int16_t xRight;
    int16_t i;
    char   *psz;
    int16_t dx;
    int16_t xLeft;
    HBRUSH  hbrSav;
    int16_t iOffset;
    RECT    rc;

    if (sel.grobj == grobjPlanet) {
        psz = TextIO().pfnPlanetName(obj.ppl()->id);
        i = obj.ppl()->id;
        i += 8;
        iOffset = i % 28;
    } else {
        psz = PszGetFleetName(obj.pfl()->id);
    }
    if (ptile->fFixCtls) {
        for (i = 4; i <= 6; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, psz)) {
        for (i = 4; i <= 6; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
    } else {
        xLeft = rc.left + 12;
        xRight = rc.right - 12;
        yTop = (!gd.fSmallTileMode ? 6 : 2) + rc.top;
        if (sel.grobj == grobjFleet) {
            DrawFleetBitmap(&sel.fl, hdc, xLeft, yTop, TRUE, -1, 0, FALSE, -1, 0);
            goto DoBtns;
        }
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonShadow);
        win32::PatBlt(hdc, xLeft, yTop, 70, 2, PATCOPY);
        win32::PatBlt(hdc, xLeft, yTop + 2, 2, 68, PATCOPY);
        win32::SelectObject(hdc, hbrButtonHilite);
        win32::PatBlt(hdc, xLeft + 2, yTop + 68, 68, 2, PATCOPY);
        win32::PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, PATCOPY);
        win32::PatBlt(hdc, xLeft + 1, yTop + 69, 1, 1, PATCOPY);
        win32::PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, PATCOPY);
        win32::PatBlt(hdc, xLeft + 2, yTop + 2, 66, 1, BLACKNESS);
        win32::PatBlt(hdc, xLeft + 2, yTop + 3, 1, 65, BLACKNESS);
        win32::PatBlt(hdc, xLeft + 3, yTop + 67, 65, 1, BLACKNESS);
        win32::PatBlt(hdc, xLeft + 67, yTop + 3, 1, 64, BLACKNESS);
        win32::SelectObject(hdc, hbrSav);
        win32::SelectPalette(hdc, vhpal, FALSE);
        win32::RealizePalette(hdc);
        DibBlt(hdc, xLeft + 3, yTop + 3, 64, 64, hdibPlanets, iOffset % 7 * 64, iOffset / 7 * 64, 64, 64, 13369376);
    DoBtns:
        dx = xRight - xLeft - 95;
        dy = 3 * dyArial8 >> 1;
        xLeft = xRight - dx;
        if (!ptile->fMinDraw) {
            if (gd.fSmallTileMode) {
                yTop -= 2;
                dy -= 2;
            } else {
                yTop -= 4;
            }
            iOffset = sel.grobj == grobjFleet ? 6 : 5;
            i = 4;
            while (i <= iOffset) {
                ControlIO().pfnSetWindowPos(rghwndBtn[i], NULL, xLeft, yTop, dx, dy, SWP_NOZORDER | SWP_NOACTIVATE);
                ui::ShowWindow(rghwndBtn[i], SW_SHOW);
                i++;
                yTop += (!gd.fSmallTileMode ? 3 : 2) + dy;
            }
        }
    }
    return;
}

void DrawPlanetShipList(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t fDoneDrawing;
    int32_t l2;
    int16_t yTop;
    int16_t fObjIsThing;
    int16_t fUnknown;
    int16_t idSkip;
    int16_t xStart;
    int16_t xRight;
    int16_t i;
    int16_t c;
    RECT    rcGauge;
    XFER    xf;
    FLEET  *pfl;
    int32_t lSel;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl();
    fDoneDrawing = FALSE;
    fObjIsThing = FALSE;
    if (ptile->fFixCtls) {
        for (i = 0; i <= 2; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ui::ShowWindow(hwndShipDD, SW_HIDE);
        ptile->fFixCtls = FALSE;
        rgrcRef[1].top = -5;
        rgrcRef[1].bottom = -6;
        rgrcRef[4].top = -5;
        rgrcRef[4].bottom = -6;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, TextIO().pfnCompressed(!pfl ? idsFleetsOrbit : idsOtherFleetsHere))) {
        for (i = 0; i <= 2; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ui::ShowWindow(hwndShipDD, SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 2;
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        if (dxShipDD == xRight - xLeft) {
            swp |= SWP_NOSIZE;
        } else {
            dxShipDD = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(hwndShipDD, NULL, xLeft, yTop, xRight - xLeft, 5 * dyShipDD, swp);
        ui::ShowWindow(hwndShipDD, SW_SHOW);
        yTop += dyShipDD + 3;
        lSel = ControlIO().pfnSendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
        ControlIO().pfnEnableWindow(rghwndBtn[0], lSel != -1);
        if (lSel == -1) {
            fDoneDrawing = TRUE;
            fUnknown = TRUE;
        }
        if (pfl) {
            idSkip = pfl->id;
        } else {
            idSkip = idflNone;
        }
        if (!fDoneDrawing && !FLookupOrbitingXfer(idSkip == idflNone ? sel.pl.id : pfl->idPlanet, LOWORD(lSel), &xf, idSkip)) {
            fDoneDrawing = TRUE;
        }
        if (fDoneDrawing) {
            ControlIO().pfnEnableWindow(rghwndBtn[1], FALSE);
        } else {
            fObjIsThing = xf.grobj == grobjThing;
            ControlIO().pfnEnableWindow(rghwndBtn[1], fObjIsThing == 0 && xf.iPlayerObj() == idPlayer);
        }
        rgrcRef[1].top = -5;
        rgrcRef[1].bottom = -6;
        rgrcRef[4].top = -5;
        rgrcRef[4].bottom = -6;
        fUnknown = fObjIsThing == 0 && xf.detObj() != detAll;
        if (!gd.fSmallTileMode) {
            if (!fDoneDrawing && !fUnknown) {
                win32::SelectObject(hdc, rghfontArial8[1]);
                c = TextIO().pfnCchGetString(idsCargo3, szWork);
                l = GetTextExtent(hdc, szWork, c);
            }
            if (!fDoneDrawing && !fUnknown && !fObjIsThing) {
                c = TextIO().pfnCchGetString(idsFuel3, szWork);
                l2 = GetTextExtent(hdc, szWork, c);
                if (l2 > l) {
                    l = l2;
                }
                win32::TextOut(hdc, xLeft, yTop, szWork, c);
                win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
                if (idSkip != idflNone) {
                    rgrcRef[1] = rcGauge;
                }
                DrawFleetGauge(hdc, &rcGauge, &xf.fl(), 4);
            } else if (ptile->fMinDraw || fUnknown != gd.fUnknownShip) {
                win32::SetRect(&rcGauge, xLeft, yTop, xRight, dyArial8 * 2 + yTop + 8);
                win32::FillRect(hdc, &rcGauge, hbrButtonFace);
            }
            gd.fUnknownShip = fUnknown;
            if (!fObjIsThing) {
                yTop += dyArial8 + 4;
            }
            if (!fDoneDrawing && !fUnknown) {
                c = TextIO().pfnCchGetString(idsCargo3, szWork);
                win32::TextOut(hdc, xLeft, yTop, szWork, c);
                win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
                rgrcRef[4] = rcGauge;
                if (fObjIsThing) {
                    DrawThingGauge(hdc, &rcGauge, &xf.th(), 5);
                } else {
                    DrawFleetGauge(hdc, &rcGauge, &xf.fl(), 5);
                }
                if (fObjIsThing) {
                    win32::OffsetRect(&rcGauge, 0, dyArial8 + 4);
                    win32::FillRect(hdc, &rcGauge, hbrButtonFace);
                    yTop += dyArial8 + 4;
                }
            }
            yTop += dyArial8 + 4;
        }
        c = (int16_t)(xRight - xLeft - 10) / 3;
        xStart = xLeft;
        i = 0;
        while (i <= 2) {
            ControlIO().pfnSetWindowPos(rghwndBtn[(int16_t)(i + 1) % 3], NULL, xStart, yTop, c, (dyArial8 >> 1) + dyArial8, SWP_NOZORDER | SWP_NOACTIVATE);
            if (i != 2 || pfl) {
                ui::ShowWindow(rghwndBtn[i], SW_SHOW);
            }
            i++;
            xStart += c + 6;
        }
        ControlIO().pfnEnableWindow(rghwndBtn[2], (idSkip == idflNone || !fUnknown) && !fObjIsThing);
    }
    return;
}

void PlanetClick(int16_t x, int16_t y, int16_t sks, int16_t fRightBtn) {
    int16_t  bt;
    POINT16  pt;
    int16_t  ctile;
    int16_t  dy;
    RECT     rcTitle;
    int16_t  i;
    int16_t  xRel;
    uint16_t iCol;
    int16_t  iCur;
    TILE    *prgtile;
    RECT     rc;
    BTNT     btnt;
    HDC      hdc;
    TILE     tile;
    POINT16  ptNew;

    bt = 112;
    if (sel.grobj == grobjPlanet) {
        prgtile = rgtilePlanet;
        ctile = 6;
    } else {
        if (sel.grobj != grobjFleet) {
            return;
        }
        prgtile = rgtileShip;
        ctile = 7;
    }
    iCol = (uint32_t)x / 198;
    xRel = x - iCol * 198;
    if (xRel >= 4 && xRel < 194) {
        for (i = 0; i < ctile; i++) {
            if (prgtile[i].iCol >= iCol) {
                if (prgtile[i].iCol > iCol) {
                    return;
                }
                if (y >= prgtile[i].yTop && y < (!prgtile[i].fPopped ? dyArial8 + 3 : prgtile[i].dyFull) + prgtile[i].yTop)
                    break;
            }
        }
        if (i != ctile) {
            pt.x = x;
            pt.y = y;
            rcTitle.top = prgtile[i].yTop;
            rcTitle.bottom = dyArial8 + 3 + rcTitle.top + 1;
            rcTitle.left = iCol * 198 + 4;
            rcTitle.right = rcTitle.left + 191;
            if (win32::PtInRect(&rcTitle, PointFrom16(pt)) != 0 && !fRightBtn) {
                rc = rcTitle;
                rc.top++;
                rc.left = rc.right - 17;
                if (win32::PtInRect(&rc, PointFrom16(pt)) != 0) {
                    OffsetRc(&rc, -1, 0);
                    if (prgtile[i].fPopped) {
                        bt = bt;
                    } else {
                        bt |= 1;
                    }
                    InitBtnTrack(&btnt, hwndPlanet, NULL, &rc, bt, 0, FALSE, FALSE, NULL);
                    while (FTrackBtn(&btnt)) {
                    }
                    if (btnt.fDown) {
                        prgtile[i].fPopped = prgtile[i].fPopped == 0;
                        ReflowColumn(prgtile[i].iCol, i, TRUE);
                    }
                } else {
                    if (prgtile[i].fPopped) {
                        rcTitle.bottom = rcTitle.top + prgtile[i].dyFull + 1;
                    }
                    hdc = win32::GetDC(hwndPlanet);
                    DrawFuzzyBorder(hdc, &rcTitle);
                    win32::SetCapture(hwndPlanet);
                    ptNew = pt;
                    while (FGetMouseMove(&ptNew)) {
                        if (pt.x != ptNew.x || pt.y != ptNew.y) {
                            DrawFuzzyBorder(hdc, &rcTitle);
                            OffsetRc(&rcTitle, ptNew.x - pt.x, ptNew.y - pt.y);
                            pt = ptNew;
                            DrawFuzzyBorder(hdc, &rcTitle);
                        }
                    }
                    DrawFuzzyBorder(hdc, &rcTitle);
                    win32::ReleaseCapture();
                    win32::ReleaseDC(hwndPlanet, hdc);
                    pt.x = ((rcTitle.right - rcTitle.left) >> 1) + rcTitle.left;
                    pt.y = ((rcTitle.bottom - rcTitle.top) >> 1) + rcTitle.top;
                    iCol = 0 > (1 >= pt.x / 198 ? pt.x / 198 : 1) ? 0 : 1 < pt.x / 198 ? 1 : pt.x / 198;
                    iCur = i;
                    for (i = 0; i < ctile && prgtile[i].iCol < iCol; i++) {
                    }
                    for (; i < ctile && prgtile[i].iCol == iCol; i++) {
                        dy = !prgtile[i].fPopped ? dyArial8 + 3 : prgtile[i].dyFull;
                        if (pt.y < (dy >> 1) + prgtile[i].yTop)
                            break;
                    }
                    if (i == iCur || i == iCur + 1) {
                        i = prgtile[iCur].iCol;
                        if (iCol != i) {
                            prgtile[iCur].iCol = iCol;
                            ReflowColumn(iCol, iCur, TRUE);
                            if ((int16_t)iCol < i) {
                                ReflowColumn(i, iCur + 1, TRUE);
                            } else {
                                ReflowColumn(i, iCur, TRUE);
                            }
                        }
                    } else {
                        tile = prgtile[iCur];
                        if (i < iCur) {
                            memmove(prgtile + (i + 1), prgtile + i, (iCur - i) * sizeof(TILE));
                            iCur++;
                        } else {
                            memmove(prgtile + iCur, prgtile + (iCur + 1), (i - iCur - 1) * sizeof(TILE));
                            i--;
                        }
                        prgtile[i] = tile;
                        prgtile[i].iCol = iCol;
                        if (tile.iCol == iCol) {
                            ReflowColumn(iCol, i >= iCur ? iCur : i, TRUE);
                        } else {
                            ReflowColumn(iCol, i, TRUE);
                            ReflowColumn(tile.iCol, iCur, TRUE);
                        }
                    }
                }
            } else {
                if ((sel.grobj == grobjFleet && (prgtile[i].grbit == tileFleetOrders || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                 prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == 0x10)) ||
                    prgtile[i].grbit == tileShipList) {
                    ClickInShipOrders(pt, sks, FALSE, fRightBtn);
                } else if (sel.grobj == grobjPlanet && (prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                        prgtile[i].grbit == tileProductionOrOrbit || prgtile[i].grbit == tilePlanetStats)) {
                    ClickInPlanetOrders(pt, sks, FALSE, fRightBtn);
                }
            }
        }
    }
    return;
}

HCURSOR ClickInPlanetOrders(POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn) {
    int16_t i;
    int32_t rglQuan[3];
    int16_t iWarp;
    BTNT    btnt;

    if (sel.grobj != grobjPlanet) {
        return NULL;
    }
    if (fRightBtn) {
        return NULL;
    }
    if (win32::PtInRect(&rgrcRef[6], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        i = (int16_t)(pt.y - rgrcRef[6].top) / dyArial8;
        GlobalPD.grPopup = grPopupMineral;
        GlobalPD.SetRgi(0, i);
        GlobalPD.SetRgi(2, sel.pl.rgwtMin[i]);
        GlobalPD.SetRgi(3, (uint32_t)sel.pl.rgMinConc[i]);
        ColonyIO().pfnEstimateMining(&sel.pl, rglQuan, -1, FALSE);
        GlobalPD.SetRgi(4, rglQuan[i]);
        GlobalPD.SetRgi(1, sel.pl.fHomeworld);
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[7], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupPlanetIndustry;
        GlobalPD.idPlan = sel.pl.id;
        GlobalPD.SetFFactory(pt.y >= rgrcRef[7].top + dyArial8);
        if (GlobalPD.fFactory) {
            GlobalPD.cMax = ColonyIO().pfnMaxFactories(&sel.pl, idPlayer);
            GlobalPD.cCur = sel.pl.cFactories;
            GlobalPD.cOperate = ColonyIO().pfnOperableFactories(&sel.pl, idPlayer, FALSE);
        } else {
            GlobalPD.cMax = ColonyIO().pfnMaxMines(&sel.pl, idPlayer);
            GlobalPD.cCur = sel.pl.cMines;
            GlobalPD.cOperate = ColonyIO().pfnOperableMines(&sel.pl, idPlayer, FALSE);
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[8], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupResources;
        GlobalPD.idPlanet = sel.pl.id;
        GlobalPD.iPlanetVar = GlobalPD.iPlanVal = ResearchIO().pfnResources(&sel.pl, idPlayer);
        if (!sel.pl.fNoResearch) {
            GlobalPD.iPlanVal -= win32::MulDiv(GlobalPD.iPlanetVar, rgplr[idPlayer].pctResearch, 100);
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[9], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupPlanet;
        GlobalPD.idPlanet = sel.pl.id;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[10], PointFrom16(pt)) != 0) {
        if (sel.pl.cDefenses == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        FGetBestDefensePart(&GlobalPD.part);
        GlobalPD.grPopup = grPopupComponent;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[11], PointFrom16(pt)) != 0) {
        if (sel.pl.iScanner == 31 && RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            TextIO().pfnCchGetString(idsRaceCannotBuildPlanetaryScannersStarbasesHave, szPopupBuffer);
        } else {
            ProductionIO().pfnScanner(&GlobalPD.part);
            GlobalPD.grPopup = grPopupComponent;
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[13], PointFrom16(pt)) != 0) {
        iWarp = ColonyIO().pfnWarp(&sel.pl, NULL);
        if (iWarp == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurHand;
        }
        InitBtnTrack(&btnt, hwndPlanet, NULL, &rgrcRef[13], 8, 80, gd.fSetMassMode, TRUE, TextIO().pfnCompressed(idsSetDest));
        while (FTrackBtn(&btnt)) {
        }
        gd.fSetMassMode ^= btnt.fDown;
    } else if (win32::PtInRect(&rgrcRef[14], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupShdef;
        GlobalPD.lpshdef = rglpshdefSB[idPlayer] + sel.pl.isb;
        GlobalPD.fHideCounts = FALSE;
        GlobalPD.fShowDamage = TRUE;
        GlobalPD.SetFToken(FALSE);
        GlobalPD.fSummary = FALSE;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[16], PointFrom16(pt)) != 0) {
        iWarp = ColonyIO().pfnWarp(&sel.pl, NULL);
        if (iWarp == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.part.hs.grhst = hstSpecialSB;
        GlobalPD.part.hs.iItem = iWarp + 2;
        PartQueryIO().pfnLookup(&GlobalPD.part);
        GlobalPD.grPopup = grPopupComponent;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&rgrcRef[15], PointFrom16(pt)) != 0) {
        ClickInShipOrders(pt, sks, FALSE, fRightBtn);
    } else if (win32::PtInRect(&rgrcRef[17], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurHand;
        }
        InitBtnTrack(&btnt, hwndPlanet, NULL, &rgrcRef[17], 8, 80, gd.fSetRouteMode, TRUE, TextIO().pfnCompressed(idsRoute2));
        while (FTrackBtn(&btnt)) {
        }
        gd.fSetRouteMode ^= btnt.fDown;
    }
    return NULL;
}

void EnsureTileSize(int16_t fSmallTiles) {
    int16_t    iMul;
    int16_t    i;
    GrobjClass grobjSav;

    if (fSmallTiles != gd.fSmallTileMode) {
        gd.fSmallTileMode = fSmallTiles;
        iMul = !fSmallTiles ? 1 : -1;
        for (i = 0; i < 6; i++) {
            if (rgtilePlanet[i].grbit == tileProductionOrOrbit) {
                rgtilePlanet[i].dyFull += (dyArial8 + 2) * 2 * iMul;
            }
            if (rgtilePlanet[i].grbit == tileShipList) {
                rgtilePlanet[i].dyFull += (dyArial8 + 4) * 2 * iMul;
            }
            if (rgtilePlanet[i].grbit == tileBitmap) {
                rgtilePlanet[i].dyFull += 10 * iMul;
            }
        }
        for (i = 0; i < 7; i++) {
            if (rgtileShip[i].grbit == tileMineralsOrCargo) {
                rgtileShip[i].dyFull += (dyArial8 * 4 + 2) * iMul;
            }
            if (rgtileShip[i].grbit == tileFleetComp) {
                rgtileShip[i].dyFull += ((dyArial8 + 2) * 2 + 4 + dyArial8) * iMul;
            }
            if (rgtileShip[i].grbit == tileFleetOrders) {
                rgtileShip[i].dyFull += (dyArial8 + 9) * iMul;
            }
            if (rgtileShip[i].grbit == tileShipList) {
                rgtileShip[i].dyFull += (dyArial8 + 4) * 2 * iMul;
            }
            if (rgtileShip[i].grbit == tileBitmap) {
                rgtileShip[i].dyFull += 10 * iMul;
            }
            if (rgtileShip[i].grbit == tileStarbaseOrWaypoint) {
                rgtileShip[i].dyFull += iMul * 2;
            }
            if (rgtileShip[i].grbit == tileProductionOrOrbit) {
                rgtileShip[i].dyFull += 6 * iMul;
            }
        }
        grobjSav = sel.grobj;
        sel.grobj = grobjPlanet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(i, -1, FALSE);
        }
        sel.grobj = grobjFleet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(i, -1, FALSE);
        }
        sel.grobj = grobjSav;
    }
    return;
}

void ReflowColumn(int16_t iCol, int16_t iTile, int16_t fRedraw) {
    HDC     hdc;
    int16_t yTop;
    int16_t ctile;
    int16_t i;
    int16_t grbit;
    TILE   *ptile;
    RECT    rc;

    grbit = 0;
    if (sel.grobj == grobjFleet) {
        ptile = rgtileShip;
        ctile = 7;
    } else {
        ptile = rgtilePlanet;
        ctile = 6;
    }
    yTop = 4;
    if (iTile == -1) {
        for (i = 0; i < ctile && ptile[i].iCol != iCol; i++) {
        }
        if (i == ctile) {
            return;
        }
        iTile = i;
    } else {
        for (i = 0; i < iTile; i++) {
            if (ptile[i].iCol == iCol) {
                yTop += (!ptile[i].fPopped ? dyArial8 + 3 : ptile[i].dyFull) + 4;
            }
        }
    }
    if (fRedraw) {
        ControlIO().pfnGetClientRect(hwndPlanet, &rc);
        rc.top = yTop;
        rc.left = 198 * iCol + 4;
        rc.right = rc.left + 191;
        hdc = win32::GetDC(hwndPlanet);
        win32::FillRect(hdc, &rc, hbrButtonFace);
    }
    for (; iTile < ctile && ptile[iTile].iCol == iCol; iTile++) {
        ptile[iTile].yTop = yTop;
        ptile[iTile].fFixCtls = TRUE;
        grbit |= ptile[iTile].grbit;
        yTop += (!ptile[iTile].fPopped ? dyArial8 + 3 : ptile[iTile].dyFull) + 4;
    }
    if (fRedraw) {
        ControlIO().pfnDrawPlanShip(hdc, static_cast<TileBits>(grbit));
        win32::ReleaseDC(hwndPlanet, hdc);
    }
    return;
}

void HandleFocusState(DRAWITEMSTRUCT *lpdis, int16_t inflate) {
    if (lpdis->itemState & ODS_FOCUS) {
        win32::FrameRect(lpdis->hDC, &lpdis->rcItem, hbr50Screen);
    }
    return;
}

void DrawCBEntireItem(DRAWITEMSTRUCT *lpdis, int16_t inflate) {
    int16_t fListbox;
    int16_t fSelected;
    RECT    rc;

    fSelected = lpdis->itemState & ODS_SELECTED;
    rc = lpdis->rcItem;
    fListbox = lpdis->hwndItem == hwndFleetCompLB || lpdis->hwndItem == hwndPlanetProdLB || inflate > 0;
    if (inflate > 0) {
        inflate = -inflate;
    }
    ControlIO().pfnSendMessage(lpdis->hwndItem, !fListbox ? CB_GETLBTEXT : LB_GETTEXT, lpdis->itemID, (LPARAM)szWork);
    DrawProductionItem(lpdis->hDC, &rc, szWork, inflate, fSelected, fListbox);
    HandleFocusState(lpdis, inflate + 2);
    return;
}

void DrawProductionItem(HDC hdc, RECT *prc, char *psz, int16_t inflate, int16_t fSelected, int16_t fListbox) {
    HFONT    hfntSav;
    char    *pch;
    int16_t  ichT;
    COLORREF cr;
    int16_t  pctDmg;
    char     szT[20];
    RECT     rcIn;
    int16_t  ich;
    int16_t  fDoubleDraw;
    COLORREF crForeSav;
    int16_t  fFleet;
    RECT     rcDraw;
    int16_t  dx;
    HBRUSH   hbr;
    int16_t  fItalic;
    int16_t  cch;
    int16_t  bkSav;
    RECT     rc;

    fItalic = FALSE;
    fDoubleDraw = FALSE;
    fFleet = FALSE;
    rcIn = *prc;
    rc = rcIn;
    if (!fSelected) {
        hbr = hbrWindow;
        switch (*psz) {
        default:
            cr = 0xff;
            break;
        case 'I':
            fItalic = TRUE;
            /* fallthrough */
        case ' ':
        LDefCase:
            if (crWindow == 0) {
                cr = 0xffffff;
                break;
            }
            cr = 0;
            break;
        case 'P':
            fDoubleDraw = TRUE;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = TRUE;
            goto LDefCase;
        case '*':
            cr = 32512;
            break;
        case '#':
            cr = 8323072;
            break;
        case '&':
            cr = 8355711;
        }
    } else {
        cr = crWindow;
        switch (*psz) {
        default:
            hbr = hbrRed;
            break;
        case 'I':
            fItalic = TRUE;
            /* fallthrough */
        case ' ':
        LDefCaseSel:
            if (crWindow == 0) {
                hbr = (HBRUSH)win32::GetStockObject(WHITE_BRUSH);
                break;
            }
            hbr = (HBRUSH)win32::GetStockObject(BLACK_BRUSH);
            break;
        case 'P':
            fDoubleDraw = TRUE;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = TRUE;
            goto LDefCaseSel;
        case '*':
            hbr = hbrGreen;
            break;
        case '#':
            hbr = hbrBlue;
            break;
        case '&':
            hbr = hbrGray;
        }
    }
    if (fListbox == 2) {
        hbr = hbrButtonFace;
    }
    win32::FillRect(hdc, &rcIn, hbr);
    if (fDoubleDraw) {
        rcDraw = rcIn;
        dx = rcIn.right - rcIn.left;
        dx = (int16_t)(dx * pctDmg) / 100;
        rcDraw.right = rcDraw.left + dx;
        win32::FillRect(hdc, &rcDraw, hbrRed);
    }
    if (inflate != 0) {
        win32::InflateRect(&rcIn, -2, -1);
    }
    if (!fListbox) {
        ich = 1;
    } else if (fFleet) {
        ich = 7;
    } else {
        ich = 6;
        if ((psz[1] - ' ') & 2) {
            fItalic = TRUE;
        }
    }
    crForeSav = win32::SetTextColor(hdc, cr);
    bkSav = win32::SetBkMode(hdc, TRANSPARENT);
    if (fItalic) {
        hfntSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[3]);
    }
    pch = psz + ich;
    cch = strlen(pch);
    dx = LOWORD(GetTextExtent(hdc, pch, cch));
    while (cch > 0 && dx > rcIn.right - rcIn.left) {
        cch = (int16_t)CchUtf8Prefix(pch, cch - 1);
        dx = LOWORD(GetTextExtent(hdc, pch, cch));
    }
    win32::TextOut(hdc, rcIn.left, rcIn.top, pch, cch);
    if (fItalic) {
        win32::SelectObject(hdc, hfntSav);
    }
    if (ich >= 6) {
        if ((psz[ich - 5] - ' ') & 2) {
            if (psz[ich - 1] == '*') {
                ich = TextIO().pfnCchGetString(idsNeeded, szT);
                goto LRightOut;
            }
            TextIO().pfnCchGetString(idsUpTo, szT);
        } else {
            szT[0] = 0;
        }
        ich = strlen(szT);
        for (ichT = 2 - fFleet; ichT < 6 && psz[ichT + fFleet] == ' '; ichT++) {
        }
        strncpy(&szT[ich], psz + (ichT + fFleet), 6 - ichT);
        ich += 6 - ichT;
        if (!fFleet && ((psz[fDoubleDraw + 1] - ' ') & 1)) {
            szT[ich++] = '%';
        }
    LRightOut:
        RightTextOut(hdc, rcIn.right, rcIn.top, szT, ich, 0);
    }
    win32::SetTextColor(hdc, crForeSav);
    win32::SetBkMode(hdc, bkSav);
    return;
}

void DrawPlanShip(HDC hdc, TileBits grbit) {
    HFONT    hfontSav;
    OBJ      objNull;
    int16_t  ctile;
    COLORREF crFore;
    OBJ      obj;
    int16_t  fMin;
    int16_t  i;
    COLORREF crBack;
    int16_t  fErase;
    TILE    *ptile;
    int16_t  fDC;
    RECT     rc;

    fDC = FALSE;
    objNull.SetPfl(0);
    if (sel.id == -1) {
        for (i = 0; i < 13; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ui::ShowWindow(hwndOrderED, SW_HIDE);
        ui::ShowWindow(hwndShipDD, SW_HIDE);
        ui::ShowWindow(hwndBattleDD, SW_HIDE);
        ui::ShowWindow(hwndShipLB, SW_HIDE);
        ui::ShowWindow(hwndFleetCompLB, SW_HIDE);
        ui::ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ui::ShowWindow(hwndRepCB, SW_HIDE);
        for (i = 0; i < 19; i++) {
            rgrcRef[i].bottom = -6;
            rgrcRef[i].top = -5;
        }
        if (rgplr[idPlayer].fDead && hdc) {
            ControlIO().pfnGetClientRect(hwndPlanet, &rc);
            win32::SetBkColor(hdc, crButtonFace);
            win32::SetTextColor(hdc, crButtonText);
            i = TextIO().pfnCchGetString(idsDeceased, szWork);
            DiaganolTextOut(hdc, &rc, szWork, i);
        }
    } else {
        if (sel.grobj == grobjFleet) {
            ptile = rgtileShip;
            ctile = 7;
            obj.SetPfl(&sel.fl);
        } else {
            ptile = rgtilePlanet;
            ctile = 6;
            obj.SetPpl(&sel.pl);
        }
        if (!hdc) {
            fDC = TRUE;
            hdc = win32::GetDC(hwndPlanet);
        }
        hfontSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[0]);
        crBack = win32::SetBkColor(hdc, crButtonFace);
        crFore = win32::SetTextColor(hdc, crButtonText);
        fErase = (grbit & tileErase) != 0;
        fMin = (grbit & tileMinimized) != 0;
        for (i = 0; i < ctile; i++) {
            if (grbit & ptile[i].grbit) {
                ptile[i].fErase = fErase;
                ptile[i].fMinDraw = fMin;
                ptile[i].pfn(hdc, ptile + i, !ptile[i].fNullPtr ? obj : objNull);
            }
        }
        win32::SetTextColor(hdc, crFore);
        win32::SetBkColor(hdc, crBack);
        win32::SelectObject(hdc, hfontSav);
        if (fDC) {
            win32::ReleaseDC(hwndPlanet, hdc);
        }
    }
    QueuePlanetPaneLayout();
    return;
}

void SetPlanetTitleBar(HWND hwnd) {
    char  szTitle[30];
    char *psz;

    if (sel.grobj == grobjPlanet) {
        psz = TextIO().pfnPlanetName(sel.pl.id);
        TextIO().pfnCchGetString(idsPlanet2, szTitle);
        win32::lstrcat(szTitle, psz);
        psz = szTitle;
    } else if (sel.grobj == grobjFleet) {
        psz = PszGetFleetName(sel.fl.id);
    } else {
        psz = TextIO().pfnCompressed(idsPlanetView);
    }
    ControlIO().pfnSetWindowText(hwnd, psz);
    return;
}

void FillShipDD(int16_t idSkip) {
    THING  *lpthMac;
    int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    POINT16 ptSel;

    ControlIO().pfnSendMessage(hwndShipDD, CB_RESETCONTENT, 0, 0);
    if (sel.grobj == grobjPlanet) {
        ptSel = rgptPlan[sel.id];
    } else {
        ptSel = sel.fl.pt;
    }
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if ((idSkip == idflNone && sel.id == lpfl->idPlanet) ||
            (idSkip != idflNone && lpfl->id != idSkip && lpfl->pt.x == sel.fl.pt.x && lpfl->pt.y == sel.fl.pt.y)) {
            PszGetFleetName(lpfl->id);
            memmove(&szWork[1], szWork, 50);
            szWork[0] = lpfl->iPlayer == idPlayer ? 32 : 120;
            ControlIO().pfnSendMessage(hwndShipDD, CB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMineralPacket && lpth->pt().x() == ptSel.x && lpth->pt().y() == ptSel.y) {
            PszGetThingName(lpth->idFull());
            memmove(&szWork[1], szWork, 50);
            szWork[0] = lpth->iplr() == idPlayer ? 32 : 120;
            ControlIO().pfnSendMessage(hwndShipDD, CB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    ControlIO().pfnSendMessage(hwndShipDD, CB_SETCURSEL, 0, 0);
    return;
}

void FillPlanetProdLB(HWND hwnd, PLPROD *lpplprod, PLANET *lppl) {
    int32_t rgwtMin[4];
    int16_t i;
    char    szTemp[80];
    int32_t resCost;
    char   *psz;

    /* Given a planet, leave its queue's first item in szWork; see
       PszProdQueueTop. */
    if (lppl) {
        PszProdQueueTop(lppl, lpplprod);
        return;
    }
    lppl = &sel.pl;
    if (!hwnd) {
        hwnd = hwndPlanetProdLB;
    }
    ControlIO().pfnSendMessage(hwnd, LB_RESETCONTENT, 0, 0);
    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    if (!lpplprod || lpplprod->iprodMac == 0) {
        psz = TextIO().pfnCompressed(idsQueueEmpty);
    } else {
        if (!hwndProdDlg)
            goto NoMsg;
        psz = TextIO().pfnCompressed(idsTopQueue);
    }
    ControlIO().pfnSendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)psz);
NoMsg:
    if (lpplprod) {
        resCost = 0;
        for (i = 0; i < 4; i++) {
            rgwtMin[i] = 0;
        }
        for (i = 0; i < lpplprod->iprodMac; i++) {
            FProdItemLine(lppl, lpplprod, i, TRUE, szTemp);
            ControlIO().pfnSendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)szTemp);
        }
    }
    return;
}

// ShowPlanetSel refills the planet tile's production queue when
// ChangeMainObjSel selects a planet.
void ShowPlanetSel() {
    ControlIO().pfnFillPlanetProdLB(NULL, NULL, NULL);
    ControlIO().pfnSendMessage(hwndPlanetProdLB, LB_SETCURSEL, 0, 0);
    return;
}

// ShowFleetSel refills the fleet tiles' lists when ChangeMainObjSel selects
// a fleet.
void ShowFleetSel() {
    FillOrdersLB();
    FillFleetCompLB();
    FillBattleDD(sel.fl.iplan + 1);
    ControlIO().pfnSendMessage(rghwndOrderDD[0], CB_SETCURSEL, sel.fl.lpplord->rgord()[0].grTask, 0);
    return;
}

// ShowMainObjSel redraws the planet window for the object ChangeMainObjSel
// selected; fSameType is set when it is the same kind as before.
void ShowMainObjSel(int16_t fSameType, int16_t idSkip) {
    int16_t i;

    if (!fSameType) {
        for (i = 0; i < 13; i++) {
            ui::ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ui::ShowWindow(hwndOrderED, SW_HIDE);
        ui::ShowWindow(hwndShipDD, SW_HIDE);
        ui::ShowWindow(hwndBattleDD, SW_HIDE);
        ui::ShowWindow(hwndShipLB, SW_HIDE);
        ui::ShowWindow(hwndFleetCompLB, SW_HIDE);
        ui::ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ui::ShowWindow(hwndRepCB, SW_HIDE);
        for (i = 0; i < 19; i++) {
            rgrcRef[i].bottom = -6;
            rgrcRef[i].top = -5;
        }
    }
    FillShipDD(idSkip);
    if (fSameType) {
        ControlIO().pfnDrawPlanShip(NULL, static_cast<TileBits>(0x4fff));
    } else {
        ControlIO().pfnInvalidateRect(hwndPlanet, NULL, TRUE);
        if ((grbitScan & grbitScanAddWaypoints) && sel.grobj == grobjPlanet) {
            grbitScan = static_cast<GrbitScan>(grbitScan & 0xffef);
            ControlIO().pfnInvalidateRect(hwndTb, NULL, TRUE);
        }
    }
    SetPlanetTitleBar(hwndPlanet);
    if (gd.fTutorial) {
        PresentationIO().pfnTutor();
    }
    return;
}

// FillSelProdLB refills the planet tile's production queue for the
// selected planet.
void FillSelProdLB() {
    ControlIO().pfnFillPlanetProdLB(NULL, NULL, NULL);
    return;
}

// RedrawPlanShip redraws the planet window's tiles in grbit.
void RedrawPlanShip(TileBits grbit) {
    ControlIO().pfnDrawPlanShip(NULL, grbit);
    return;
}
