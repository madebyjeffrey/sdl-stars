#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"

LRESULT  PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC                hdc;
    PAINTSTRUCT        ps;
    XFER               xf;
    std::int16_t            i;
    char              *psz;
    std::int32_t            lSel;
    RECT               rc;
    POINT16            pt;
    HCURSOR            hcs;
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    PLANET            *lpplMac;
    PLANET            *lppl;
    FLEET             *lpfl;

    switch (message) {
    case win32::WM_MDIACTIVATE:
        app.lpui->hwndActive = (lParam == reinterpret_cast<LPARAM>(hwnd)) == 0 ? nullptr : hwnd;
        return 0;
    case win32::WM_CREATE:
        SetPlanetTitleBar(app, hwnd);
        for (i = 0; i < 3; i++) {
            app.lpui->rghwndOrderDD[i] = ui::CreateWindowEx(0, app.lpui->szCombobox, "OrdDD", (i == 1 ? 528 : 0) | (win32::CBS_DROPDOWNLIST | win32::WS_CHILD | win32::WS_VSCROLL), 100, 100, 200,
                                            i == 2 ? 160 : 80, hwnd, nullptr, app.lpui->hInst, nullptr);
            ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[i], win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        }
        for (i = idsTaskHere; i <= idsTransferFleet; i++) {
            szCatalogPsz = TextIO().pfnCompressed(app, static_cast<StringId>(i));
            psz = szCatalogPsz.data();
            ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
        }
        app.lpui->hwndOrderED = ui::CreateWindowEx(0, app.lpui->szEdit, nullptr, win32::ES_RIGHT | win32::WS_CHILD | win32::WS_BORDER, 100, 100, 200, 50, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndOrderED, win32::EM_LIMITTEXT, 4, 0);
        ControlIO().pfnSendMessage(app.lpui->hwndOrderED, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        app.lpui->lpfnRealEditProc = reinterpret_cast<WNDPROC>(win32::GetWindowLongPtrA(app.lpui->hwndOrderED, win32::GWLP_WNDPROC));
        win32::SetWindowLongPtrA(app.lpui->hwndOrderED, win32::GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(app.lpui->lpfnFakeEditProc));
        app.lpui->hwndBattleDD = ui::CreateWindowEx(0, app.lpui->szCombobox, "BattleDD", win32::CBS_DROPDOWNLIST | win32::WS_CHILD | win32::WS_VSCROLL, 100, 100, 200, 80, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndBattleDD, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        app.lpui->hwndShipDD = ui::CreateWindowEx(0, app.lpui->szCombobox, "ShipDD", win32::CBS_DROPDOWNLIST | win32::CBS_OWNERDRAWFIXED | win32::CBS_HASSTRINGS | win32::WS_CHILD | win32::WS_VSCROLL, 100, 100, 200, 80, hwnd,
                                  nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        ControlIO().pfnGetClientRect(app.lpui->hwndShipDD, &rc);
        app.lpui->dyShipDD = rc.bottom;
        app.lpui->hwndShipLB = ui::CreateWindowEx(0, app.lpui->szListbox, "ShipLB", win32::LBS_NOTIFY | win32::LBS_NOINTEGRALHEIGHT | win32::LBS_DISABLENOSCROLL | win32::WS_CHILD | win32::WS_BORDER | win32::WS_VSCROLL, 100, 100,
                                  200, 80, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        app.lpui->hwndFleetCompLB =
            ui::CreateWindowEx(0, app.lpui->szListbox, "FleetCompLB", win32::LBS_NOTIFY | win32::LBS_OWNERDRAWFIXED | win32::LBS_HASSTRINGS | win32::LBS_NOINTEGRALHEIGHT | win32::WS_CHILD | win32::WS_BORDER | win32::WS_VSCROLL,
                         100, 100, 200, 80, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndFleetCompLB, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[0]), 0);
        app.lpui->hwndPlanetProdLB =
            ui::CreateWindowEx(0, app.lpui->szListbox, "PlanetProdLB", win32::LBS_NOTIFY | win32::LBS_OWNERDRAWFIXED | win32::LBS_HASSTRINGS | win32::LBS_NOINTEGRALHEIGHT | win32::WS_CHILD | win32::WS_BORDER | win32::WS_VSCROLL,
                         100, 100, 200, 80, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndPlanetProdLB, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[0]), 0);
        for (i = 0; i < 13; i++) {
            szCatalogPsz = TextIO().pfnCompressed(app, static_cast<StringId>(idsCargo2 + i));
            psz = szCatalogPsz.data();
            app.lpui->rghwndBtn[i] = ui::CreateWindowEx(0, app.lpui->szButton, psz, win32::WS_CHILD, 100, 100, 100, app.state.dyArial8 * 2, hwnd, nullptr, app.lpui->hInst, nullptr);
            ControlIO().pfnSendMessage(app.lpui->rghwndBtn[i], win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        }
        app.lpui->hwndRepCB =
            ui::CreateWindowEx(0, app.lpui->szButton, TextIO().pfnCompressed(app, idsRepeatOrders).data(), win32::BS_AUTOCHECKBOX | win32::WS_CHILD, 100, 100, 150, app.state.dyArial8, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndRepCB, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndRepCB) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            win32::SetTextColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonText);
            return reinterpret_cast<LRESULT>(app.lpui->hbrButtonFace);
        }
        goto Default;
    case win32::WM_GETMINMAXINFO:
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.x = app.lpui->dxWinFrame * 2 + 198;
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.y = app.lpui->dyWinFrame * 2 + 198 + app.lpui->dyTitleBar;
        goto Default;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
    case win32::WM_RBUTTONDOWN:
        ControlIO().pfnSetFocus(app.lpui->hwndFrame);
        PlanetClick(app, LOWORD(lParam), HIWORD(lParam), wParam, message == win32::WM_RBUTTONDOWN);
        return 0;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnDrawPlanShip(app, hdc, tileAll);
        win32::EndPaint(hwnd, &ps);
        break;
    case win32::WM_SETCURSOR:
        hcs = nullptr;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        hcs = ClickInShipOrders(app, pt, 0, 1, 0);
        if (!hcs) {
            hcs = ClickInPlanetOrders(app, pt, 0, 1, 0);
        }
        if (!hcs)
            goto Default;
        win32::SetCursor(hcs);
        return 1;
    case win32::WM_DRAWITEM:
        lpdis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (lpdis->itemID == -1) {
            HandleFocusState(app, lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case win32::ODA_DRAWENTIRE:
                DrawCBEntireItem(app, lpdis, -4);
                break;
            case win32::ODA_SELECT:
                DrawCBEntireItem(app, lpdis, -4);
                break;
            case win32::ODA_FOCUS:
                DrawCBEntireItem(app, lpdis, -4);
            }
        }
        return 1;
    case win32::WM_MEASUREITEM:
        lpmis = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        lpmis->itemHeight = app.state.dyArial8 + 2;
        return 1;
    case win32::WM_CHAR:
        if (wParam != 'f' && wParam != 'F') {
            return 0;
        }
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == app.state.idPlayer) {
                if (app.state.sel.grobj == grobjPlanet && app.state.sel.id == lppl->id)
                    break;
                PresentationIO().pfnSelectPlanet(app, 0, lppl->id);
                return 0;
            }
        }
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (lpfl->iPlayer == app.state.idPlayer) {
                if (app.state.sel.grobj == grobjFleet && app.state.sel.id == lpfl->id)
                    break;
                PresentationIO().pfnSelectFleet(app, 0, lpfl->id);
                return 0;
            }
        }
        return 0;
    case win32::WM_COMMAND:
        if (app.state.sel.grobj == grobjFleet) {
            ShipCommandProc(app, hwnd, wParam, lParam);
            return 0;
        }
        if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndShipDD) {
            if (HIWORD(wParam) == 1) {
                ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileShipList | tileErase));
            }
        } else {
            if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[4] && HIWORD(wParam) == 0) {
                if (ControlIO().pfnGetKeyState(win32::VK_SHIFT) < 0) {
                    PresentationIO().pfnSelectPlanet(app, 0, IdFindAdjStarbase(app, app.state.sel.pl.id, 0));
                    goto LRefocus;
                }
                PresentationIO().pfnSelectPlanet(app, -1, 0);
            LRefocus:
                ControlIO().pfnSetFocus(app.lpui->hwndFrame);
                return 0;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[5] && HIWORD(wParam) == 0) {
                if (ControlIO().pfnGetKeyState(win32::VK_SHIFT) < 0) {
                    PresentationIO().pfnSelectPlanet(app, 0, IdFindAdjStarbase(app, app.state.sel.pl.id, 1));
                    goto LRefocus;
                }
                PresentationIO().pfnSelectPlanet(app, 1, 0);
                goto LRefocus;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[0] && HIWORD(wParam) == 0) {
                lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
                if (lSel == -1 || !FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, idflNone)) {
                    return 0;
                }
                TransferStuff(app, app.state.sel.pl.id, grobjPlanet, xf.id, xf.grobj, mdXferCargo);
                goto LRefocus;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[1] && HIWORD(wParam) == 0) {
                lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
                if (lSel != -1 && FLookupOrbitingXfer(app, app.state.sel.pl.id, LOWORD(lSel), &xf, idflNone) && xf.grobj == grobjFleet) {
                    PresentationIO().pfnSelectFleet(app, 0, xf.id);
                }
                goto LRefocus;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[2] && HIWORD(wParam) == 0) {
                pt.x = 610;
                pt.y = 470;
                ShipBuilder(app, pt);
                goto LRefocus;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[11] && HIWORD(wParam) == 0) {
                ChangeProduction(app, 0);
                goto LRefocus;
            } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndBtn[12] && HIWORD(wParam) == 0) {
                if (AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsSureWantDeleteEverythingPlanetsProductionQueue, nullptr), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) {
                    return 0;
                }
                ChangeProduction(app, 1);
                goto LRefocus;
            } else {
                if (reinterpret_cast<HWND>(lParam) != app.lpui->hwndPlanetProdLB || HIWORD(wParam) != 1)
                    goto Default;
                ControlIO().pfnDrawPlanShip(app, nullptr, tileProductionOrOrbit);
                if (!app.state.gd.fTutorial)
                    goto Default;
                app.state.tutor.fProgress = 1;
                PresentationIO().pfnTutor(app);
                goto Default;
            }
        }
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

std::int16_t FDrawTileNC(APPSESSION &app, HDC hdc, TILE *ptile, RECT *prc, char *pszTitle) {
    std::int16_t bt;
    RECT    rcT;

    bt = 112;
    prc->left = ptile->iCol * 198 + 4;
    prc->right = prc->left + 190;
    prc->top = ptile->yTop;
    prc->bottom = (!ptile->fPopped ? app.state.dyArial8 + 3 : ptile->dyFull) + prc->top;
    if (ptile->fMinDraw && !ptile->fMinTitle)
        goto FinishUp;

    if (!ptile->fMinDraw) {
        _Draw3dFrame(app, hdc, prc, 0);
    }
    rcT = *prc;
    ExpandRc(&rcT, -1, -1);
    rcT.bottom = rcT.top + app.state.dyArial8 + 2;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    if (!ptile->fMinDraw) {
        _Draw3dFrame(app, hdc, &rcT, 0);
    }
    RcCtrTextOut(app, hdc, &rcT, pszTitle, -1);
    win32::SetRect(&rcT, prc->right - 17, prc->top + 1, prc->right, rcT.bottom + 1);
    if (ptile->fPopped) {
        bt = bt;
    } else {
        bt |= 1;
    }
    DrawBtn(app, hdc, &rcT, bt, 0, nullptr);
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, prc->right - 18, prc->top + 1, 1, rcT.bottom - prc->top - 1, win32::PATCOPY);

FinishUp:
    prc->top += app.state.dyArial8 + 4;
    return ptile->fPopped;
}

void DrawPlanetMinSum(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::int16_t dxRight;
    std::int16_t yTop;
    std::int16_t xRight;
    std::int16_t c;
    std::int16_t i;
    std::int16_t xLeft;
    HBRUSH  hbrSav;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl();
    if (ptile->fFixCtls) {
        app.lpui->rgrcRef[6].top = -5;
        app.lpui->rgrcRef[6].bottom = -6;
        app.lpui->rgrcRef[7].top = -5;
        app.lpui->rgrcRef[7].bottom = -6;
        app.lpui->rgrcRef[8].top = -5;
        app.lpui->rgrcRef[8].bottom = -6;
        app.lpui->rgrcRef[9].top = -5;
        app.lpui->rgrcRef[9].bottom = -6;
        app.lpui->rgrcRef[11].top = -5;
        app.lpui->rgrcRef[11].bottom = -6;
        app.lpui->rgrcRef[10].top = -5;
        app.lpui->rgrcRef[10].bottom = -6;
    }
    if (FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsMineralsHand).data())) {
        if (!ppl) {
            ppl = &app.state.sel.pl;
        }
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        dxRight = app.lpui->dxMaxMineralQuan;
        win32::SetRect(&app.lpui->rgrcRef[6], xLeft, yTop, xRight, 3 * app.state.dyArial8 + yTop);
        for (i = 0; i <= 2; i++) {
            if (!ptile->fMinDraw) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                win32::TextOutA(hdc, xLeft, yTop, app.state.rgszMinerals[i], win32::lstrlenA(app.state.rgszMinerals[i]));
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdkt).data(), ppl->rgwtMin[i]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
        }
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonHilite));
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        win32::SetRect(&app.lpui->rgrcRef[7], xLeft, yTop, xRight, app.state.dyArial8 * 2 + yTop);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsMines4, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsD).data(), ColonyIO().pfnMinesOperating(app, ppl));
        } else {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD).data(), ppl->cMines, ColonyIO().pfnOperableMines(app, ppl, app.state.idPlayer, 0));
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight * 2);
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsFactories4, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
            c = TextIO().pfnCchGetString(app, idsN, std::span{app.state.szWork});
        } else {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD).data(), ppl->cFactories, ColonyIO().pfnOperableFactories(app, ppl, app.state.idPlayer, 0));
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight * 2);
        yTop += app.state.dyArial8;
    }
    return;
}

void DrawPlanetStats(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::int16_t dxRight;
    std::int32_t l2;
    std::int16_t yTop;
    std::int16_t xRight;
    std::int16_t c;
    std::int16_t cRes;
    std::int16_t dRangeP;
    float   pct;
    std::int16_t cResAvail;
    std::int16_t dRange;
    char   *psz;
    std::int16_t xLeft;
    HBRUSH  hbrSav;
    std::int32_t l;
    RECT    rc;
    PART    part;

    if (FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsStatus).data())) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = TextIO().pfnCchGetString(app, idsPopulation4, std::span{app.state.szWork});
        l = GetTextExtent(hdc, app.state.szWork, c);
        c = TextIO().pfnCchGetString(app, idsScannerType, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(app, idsScannerRange, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(app, idsDefenses4, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(app, idsDefenseType, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(app, idsDefCoverage, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = TextIO().pfnCchGetString(app, idsResourcesYear, std::span{app.state.szWork});
        l2 = GetTextExtent(hdc, app.state.szWork, c);
        if (l2 > l) {
            l = l2;
        }
        dxRight = xRight - xLeft - LOWORD(l);
        if (!ptile->fMinDraw) {
            c = TextIO().pfnCchGetString(app, idsPopulation4, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::SetRect(&app.lpui->rgrcRef[9], xLeft, yTop, xRight, yTop + app.state.dyArial8);
        c = CchCopyUtf8Prefix(std::span{app.state.szWork}, CommaFormatLong(app, static_cast<std::uint32_t>((app.state.sel.pl.rgwtMin[3] * 100))).data());
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsResourcesYear, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        win32::SetRect(&app.lpui->rgrcRef[8], xLeft, yTop, xRight, yTop + app.state.dyArial8);
        cResAvail = ResearchIO().pfnResources(app, &app.state.sel.pl, app.state.idPlayer);
        cRes = cResAvail;
        if (!app.state.sel.pl.fNoResearch) {
            cResAvail -= win32::MulDiv(cRes, app.state.rgplr[app.state.idPlayer].pctResearch, 100);
        }
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD).data(), cResAvail, cRes);
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        yTop += app.state.dyArial8;
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonHilite));
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsScannerType, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        win32::SetRect(&app.lpui->rgrcRef[11], xLeft, yTop, xRight, app.state.dyArial8 * 2 + yTop);
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
            TextIO().pfnCchGetString(app, idsOrganic, std::span{app.state.szWork});
            dRange = GetPlanetScannerRange(app, &app.state.sel.pl, &dRangeP);
        } else if (app.state.sel.pl.iScanner == 31) {
            TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            dRange = 0;
        } else {
            ProductionIO().pfnScanner(app, &part);
            CchCopyUtf8Prefix(std::span{app.state.szWork}, part.pcom().szName());
            dRange = GetPlanetScannerRange(app, &app.state.sel.pl, &dRangeP);
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, 0, dxRight);
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsScannerRange, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        if (dRange > 0) {
            if (dRangeP > 0) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDDLY).data(), dRangeP, dRange);
            } else if (dRange < 100) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDLightYears).data(), dRange);
            } else {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDLY).data(), dRange);
            }
        } else {
            TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            c = 0;
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        yTop += app.state.dyArial8;
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonHilite));
        win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDefenses4, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::SetRect(&app.lpui->rgrcRef[10], xLeft, yTop, xRight, 3 * app.state.dyArial8 + yTop);
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
            c = TextIO().pfnCchGetString(app, idsN, std::span{app.state.szWork});
        } else {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD).data(), app.state.sel.pl.cDefenses, ColonyIO().pfnOperableDefenses(app, &app.state.sel.pl, app.state.idPlayer, 0));
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDefenseType, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        if (app.state.sel.pl.cDefenses == 0) {
            TextIO().pfnCchGetString(app, RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, std::span{app.state.szWork});
            dRange = 0;
        } else {
            FGetBestDefensePart(app, &part);
            CchCopyUtf8Prefix(std::span{app.state.szWork}, part.pcom().szName());
            dRange = 1;
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, 0, dxRight);
        yTop += app.state.dyArial8;
        if (!ptile->fMinDraw) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDefCoverage, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        if (dRange != 0) {
            CalcPctSurvive(app, &app.state.sel.pl, &pct, nullptr);
            pct = static_cast<float>((static_cast<double>(1.0) - pct));
            c = win32::wsprintfA(app.state.szWork, app.state.PCTDXPCTDPCTPCT, LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))),
                         LOWORD(static_cast<std::int32_t>(((pct - static_cast<double>(static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))))) / 100.0) * 10000))));
        } else {
            c = TextIO().pfnCchGetString(app, RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, std::span{app.state.szWork});
            psz = app.state.szWork;
        }
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
    }
    return;
}

void DrawPlanetStarbase(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    std::int16_t  fTwo;
    std::int16_t  dxRight;
    std::int16_t  iWarp;
    std::int16_t  bt;
    std::int16_t  yTop;
    std::int16_t  xRight;
    std::int16_t  c;
    SHDEF   *lpshdef;
    COLORREF crForeSav;
    std::uint16_t w;
    char    *psz;
    std::int16_t  xLeft;
    HBRUSH   hbrSav;
    std::int32_t  l;
    RECT     rc;

    if (ptile->fFixCtls) {
        app.lpui->rgrcRef[13].top = -5;
        app.lpui->rgrcRef[13].bottom = -6;
        app.lpui->rgrcRef[14].top = -5;
        app.lpui->rgrcRef[14].bottom = -6;
        app.lpui->rgrcRef[16].top = -5;
        app.lpui->rgrcRef[16].bottom = -6;
        app.lpui->rgrcRef[15].top = -5;
        app.lpui->rgrcRef[15].bottom = -6;
        ptile->fFixCtls = 0;
    }
    if (app.state.sel.pl.fStarbase) {
        lpshdef = app.state.rglpshdefSB[app.state.idPlayer] + app.state.sel.pl.isb;
        CchCopyUtf8Prefix(std::span{app.state.szWork}, lpshdef->hul.szClass);
        psz = app.state.szWork;
    } else {
        szCatalogPsz = TextIO().pfnCompressed(app, idsStarbase2);
        psz = szCatalogPsz.data();
    }
    if (FDrawTileNC(app, hdc, ptile, &rc, psz)) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        win32::SetRect(&rc, xLeft - 2, yTop, xRight + 2, rc.bottom - 2);
        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
        if (!app.state.sel.pl.fStarbase) {
            win32::SetRect(&app.lpui->rgrcRef[14], -5, -5, -6, -6);
            app.lpui->rgrcRef[15] = app.lpui->rgrcRef[14];
            app.lpui->rgrcRef[16] = app.lpui->rgrcRef[14];
        } else {
            win32::SetRect(&app.lpui->rgrcRef[14], xLeft, yTop, xRight, app.state.dyArial8 * 4 + yTop);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDockCapacity, std::span{app.state.szWork});
            l = GetTextExtent(hdc, app.state.szWork, c);
            dxRight = xRight - xLeft - LOWORD(l);
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            w = PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.wtCargoMax;
            if (w == 0) {
                c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            } else if (static_cast<std::uint32_t>(w) == 0xffff) {
                c = TextIO().pfnCchGetString(app, idsUnlimited, std::span{app.state.szWork});
            } else {
                c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, w);
            }
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsArmor2, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            w = lpshdef->hul.dp;
            if (w == 0) {
                c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            } else {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLddp).data(), w, 0);
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsShields2, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            l = DpShieldOfShdef(app, lpshdef, app.state.idPlayer);
            if (l == 0) {
                c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            } else {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLddp).data(), l);
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDamage2, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            w = app.state.sel.pl.pctDp;
            if (w != 0) {
                if (w < 5) {
                    w = 5;
                }
                c = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, static_cast<std::uint32_t>(w) / 5);
                crForeSav = win32::SetTextColor(hdc, 127);
            } else {
                c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            }
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            if (w != 0) {
                win32::SetTextColor(hdc, crForeSav);
            }
            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonHilite));
            win32::PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, win32::PATCOPY);
            win32::SelectObject(hdc, hbrSav);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            win32::SetRect(&app.lpui->rgrcRef[16], xLeft, yTop, xRight, yTop + app.state.dyArial8);
            c = TextIO().pfnCchGetString(app, idsMassDriver, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            iWarp = ColonyIO().pfnWarp(app, &app.state.sel.pl, &fTwo);
            if (iWarp > 0) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpD).data(), iWarp);
                if (fTwo) {
                    app.state.szWork[c++] = '+';
                }
            } else {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsNone4).data());
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsDestination3, std::span{app.state.szWork});
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
            if (app.state.sel.pl.idFling == 0) {
                c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
            } else {
                psz = PszGetCompressedPlanet(app, app.state.rgidPlan[app.state.sel.pl.idFling - 1]);
                c = 0;
                CchCopyUtf8Prefix(std::span{app.state.szWork}, psz);
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
            yTop += app.state.dyArial8;
            c = static_cast<std::int16_t>((xRight - xLeft)) / 3;
            win32::SetRect(&app.lpui->rgrcRef[13], xLeft, yTop, xLeft + c, yTop + app.state.dyArial8 + 6);
            bt = 8;
            if (iWarp == 0) {
                bt |= 4;
            }
            DrawBtn(app, hdc, &app.lpui->rgrcRef[13], bt, app.state.gd.fSetMassMode, TextIO().pfnCompressed(app, idsSetDest).data());
            if (iWarp > 0) {
                win32::SetRect(&app.lpui->rgrcRef[15], xLeft + c + 4, yTop + 3, xRight, yTop + app.state.dyArial8 + 3);
                DrawMassWarpGauge(app, hdc, &app.lpui->rgrcRef[15], !fTwo ? iWarp : -iWarp, app.state.sel.pl.iWarpFling + 4);
            } else {
                win32::SetRect(&app.lpui->rgrcRef[15], -5, -5, -6, -6);
                app.lpui->rgrcRef[16] = app.lpui->rgrcRef[15];
            }
        }
    }
    return;
}

void DrawMassWarpGauge(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t iBest, std::int16_t iCur) {
    std::int32_t lMax;
    std::int16_t c;
    std::int16_t fTwoMAs;
    std::int16_t iMode;
    HBRUSH  hbr;
    std::int32_t lCur;
    std::int32_t l;

    fTwoMAs = iBest < 0;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (iCur < 5) {
        iCur = 5;
    }
    if (iBest < 0) {
        iBest = -iBest;
    }
    lMax = static_cast<std::int16_t>((iBest - 1));
    if (iCur <= iBest + fTwoMAs) {
        hbr = app.lpui->hbrPurple;
    } else if (iCur < iBest + fTwoMAs + 3) {
        hbr = app.lpui->hbrYellow;
    } else {
        hbr = app.lpui->hbrRed;
    }
    lCur = static_cast<std::int16_t>((iCur - 4));
    l = LDrawGauge(app, hdc, prc, 1, &lCur, &hbr, lMax);
    iMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpLd).data(), l + 4);
    l = GetTextExtent(hdc, app.state.szWork, c);
    RcCtrTextOut(app, hdc, prc, app.state.szWork, c);
    win32::SetBkMode(hdc, iMode);
    return;
}

void DrawPlanetProduction(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::int16_t swp;
    std::int16_t dxRight;
    std::int16_t yTop;
    std::int16_t xStart;
    std::int16_t xRight;
    char    szT[40];
    std::int16_t i;
    std::int16_t c;
    std::int16_t dyWrong;
    char   *psz;
    std::int16_t iSel;
    std::int16_t cch;
    std::int16_t xLeft;
    RECT    rcT;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl();
    if (ptile->fFixCtls) {
        ui::ShowWindow(app.lpui->hwndPlanetProdLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[11], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[12], win32::SW_HIDE);
        app.lpui->rgrcRef[17].top = -5;
        app.lpui->rgrcRef[17].bottom = -6;
        ptile->fFixCtls = 0;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, idsProduction).data())) {
        ui::ShowWindow(app.lpui->hwndPlanetProdLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[11], win32::SW_HIDE);
        ui::ShowWindow(app.lpui->rghwndBtn[12], win32::SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        yTop += 4;
        ControlIO().pfnGetClientRect(app.lpui->hwndPlanetProdLB, &rcT);
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        app.lpui->dyPlanetProdLB = (app.state.dyArial8 + 2) * (!app.state.gd.fSmallTileMode ? 5 : 3);
        dyWrong = app.lpui->dyPlanetProdLB - (rcT.bottom - rcT.top);
        if (app.lpui->dxPlanetProdLB == xRight - xLeft && dyWrong >= 0 && dyWrong < app.state.dyArial8) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->dxPlanetProdLB = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndPlanetProdLB, nullptr, xLeft, yTop, xRight - xLeft, app.lpui->dyPlanetProdLB, swp);
        ui::ShowWindow(app.lpui->hwndPlanetProdLB, win32::SW_SHOW);
        ControlIO().pfnGetClientRect(app.lpui->hwndPlanetProdLB, &rcT);
        app.lpui->dyPlanetProdLB = rcT.bottom - rcT.top;
        yTop += app.lpui->dyPlanetProdLB + 4;
        iSel = LOWORD(ControlIO().pfnSendMessage(app.lpui->hwndPlanetProdLB, win32::LB_GETCURSEL, 0, 0));
        if (iSel < 0) {
            iSel = 0;
        }
        if (!ppl->lpplprod || ppl->lpplprod->iprodMac == 0) {
            c = 0;
            app.state.szWork[0] = 0;
        } else {
            psz = PszProductionETA(app, ppl, nullptr, static_cast<ProdItemType>(iSel), nullptr, nullptr);
            c = std::strlen(psz);
        }
        if (c != 0) {
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            cch = TextIO().pfnCchGetString(app, idsCompletion, std::span{szT});
            win32::TextOutA(hdc, xLeft, yTop, szT, cch);
            dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        } else {
            RightTextOut(app, hdc, xRight, yTop, app.state.szWork, 0, xRight - xLeft);
        }
        yTop += app.state.dyArial8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        cch = TextIO().pfnCchGetString(app, idsRoute3, std::span{szT});
        win32::TextOutA(hdc, xLeft, yTop, szT, cch);
        dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
        if (app.state.sel.pl.idRoute == 0) {
            c = TextIO().pfnCchGetString(app, idsNone4, std::span{app.state.szWork});
        } else {
            psz = PszGetCompressedPlanet(app, app.state.rgidPlan[app.state.sel.pl.idRoute - 1]);
            c = 0;
            CchCopyUtf8Prefix(std::span{app.state.szWork}, psz);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        RightTextOut(app, hdc, xRight, yTop, app.state.szWork, c, dxRight);
        yTop += app.state.dyArial8;
        c = static_cast<std::int16_t>((xRight - xLeft - 16)) / 3;
        xStart = xLeft;
        i = 11;
        while (i <= 12) {
            ControlIO().pfnSetWindowPos(app.lpui->rghwndBtn[i], nullptr, xStart, yTop, c, (app.state.dyArial8 >> 1) + app.state.dyArial8, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_SHOW);
            i++;
            xStart += c + 8;
        }
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[12], app.state.sel.pl.lpplprod && app.state.sel.pl.lpplprod->iprodMac != 0);
        win32::SetRect(&app.lpui->rgrcRef[17], xStart, yTop, xStart + c, yTop + app.state.dyArial8 + (app.state.dyArial8 >> 1));
        DrawBtn(app, hdc, &app.lpui->rgrcRef[17], 8, app.state.gd.fSetRouteMode, TextIO().pfnCompressed(app, idsRoute2).data());
    }
    return;
}

void DrawPlanShipBitmap(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::int16_t yTop;
    std::int16_t dy;
    std::int16_t xRight;
    std::int16_t i;
    char   *psz;
    std::int16_t dx;
    std::int16_t xLeft;
    HBRUSH  hbrSav;
    std::int16_t iOffset;
    RECT    rc;

    if (app.state.sel.grobj == grobjPlanet) {
        psz = TextIO().pfnPlanetName(app, obj.ppl()->id);
        i = obj.ppl()->id;
        i += 8;
        iOffset = i % 28;
    } else {
        psz = PszGetFleetName(app, obj.pfl()->id);
    }
    if (ptile->fFixCtls) {
        for (i = 4; i <= 6; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
        ptile->fFixCtls = 0;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, psz)) {
        for (i = 4; i <= 6; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
    } else {
        xLeft = rc.left + 12;
        xRight = rc.right - 12;
        yTop = (!app.state.gd.fSmallTileMode ? 6 : 2) + rc.top;
        if (app.state.sel.grobj == grobjFleet) {
            DrawFleetBitmap(app, &app.state.sel.fl, hdc, xLeft, yTop, 1, -1, 0, 0, -1, 0);
            goto DoBtns;
        }
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonShadow));
        win32::PatBlt(hdc, xLeft, yTop, 70, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft, yTop + 2, 2, 68, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, xLeft + 2, yTop + 68, 68, 2, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 1, yTop + 69, 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, xLeft + 2, yTop + 2, 66, 1, win32::BLACKNESS);
        win32::PatBlt(hdc, xLeft + 2, yTop + 3, 1, 65, win32::BLACKNESS);
        win32::PatBlt(hdc, xLeft + 3, yTop + 67, 65, 1, win32::BLACKNESS);
        win32::PatBlt(hdc, xLeft + 67, yTop + 3, 1, 64, win32::BLACKNESS);
        win32::SelectObject(hdc, hbrSav);
        win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        win32::RealizePalette(hdc);
        DibBlt(hdc, xLeft + 3, yTop + 3, 64, 64, app.lpui->hdibPlanets, iOffset % 7 * 64, iOffset / 7 * 64, 64, 64, 13369376);
    DoBtns:
        dx = xRight - xLeft - 95;
        dy = 3 * app.state.dyArial8 >> 1;
        xLeft = xRight - dx;
        if (!ptile->fMinDraw) {
            if (app.state.gd.fSmallTileMode) {
                yTop -= 2;
                dy -= 2;
            } else {
                yTop -= 4;
            }
            iOffset = app.state.sel.grobj == grobjFleet ? 6 : 5;
            i = 4;
            while (i <= iOffset) {
                ControlIO().pfnSetWindowPos(app.lpui->rghwndBtn[i], nullptr, xLeft, yTop, dx, dy, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
                ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_SHOW);
                i++;
                yTop += (!app.state.gd.fSmallTileMode ? 3 : 2) + dy;
            }
        }
    }
    return;
}

void DrawPlanetShipList(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj) {
    std::int16_t swp;
    std::int16_t fDoneDrawing;
    std::int32_t l2;
    std::int16_t yTop;
    std::int16_t fObjIsThing;
    std::int16_t fUnknown;
    std::int16_t idSkip;
    std::int16_t xStart;
    std::int16_t xRight;
    std::int16_t i;
    std::int16_t c;
    RECT    rcGauge;
    XFER    xf;
    FLEET  *pfl;
    std::int32_t lSel;
    std::int16_t xLeft;
    std::int32_t l;
    RECT    rc;

    pfl = obj.pfl();
    fDoneDrawing = 0;
    fObjIsThing = 0;
    if (ptile->fFixCtls) {
        for (i = 0; i <= 2; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndShipDD, win32::SW_HIDE);
        ptile->fFixCtls = 0;
        app.lpui->rgrcRef[1].top = -5;
        app.lpui->rgrcRef[1].bottom = -6;
        app.lpui->rgrcRef[4].top = -5;
        app.lpui->rgrcRef[4].bottom = -6;
    }
    if (!FDrawTileNC(app, hdc, ptile, &rc, TextIO().pfnCompressed(app, !pfl ? idsFleetsOrbit : idsOtherFleetsHere).data())) {
        for (i = 0; i <= 2; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndShipDD, win32::SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 2;
        swp = win32::SWP_NOZORDER | win32::SWP_NOACTIVATE;
        if (app.lpui->dxShipDD == xRight - xLeft) {
            swp |= win32::SWP_NOSIZE;
        } else {
            app.lpui->dxShipDD = xRight - xLeft;
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndShipDD, nullptr, xLeft, yTop, xRight - xLeft, 5 * app.lpui->dyShipDD, swp);
        ui::ShowWindow(app.lpui->hwndShipDD, win32::SW_SHOW);
        yTop += app.lpui->dyShipDD + 3;
        lSel = ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_GETCURSEL, 0, 0);
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[0], lSel != -1);
        if (lSel == -1) {
            fDoneDrawing = 1;
            fUnknown = 1;
        }
        if (pfl) {
            idSkip = pfl->id;
        } else {
            idSkip = idflNone;
        }
        if (!fDoneDrawing && !FLookupOrbitingXfer(app, idSkip == idflNone ? app.state.sel.pl.id : pfl->idPlanet, LOWORD(lSel), &xf, idSkip)) {
            fDoneDrawing = 1;
        }
        if (fDoneDrawing) {
            ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[1], 0);
        } else {
            fObjIsThing = xf.grobj == grobjThing;
            ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[1], fObjIsThing == 0 && xf.iPlayerObj() == app.state.idPlayer);
        }
        app.lpui->rgrcRef[1].top = -5;
        app.lpui->rgrcRef[1].bottom = -6;
        app.lpui->rgrcRef[4].top = -5;
        app.lpui->rgrcRef[4].bottom = -6;
        fUnknown = fObjIsThing == 0 && xf.detObj() != detAll;
        if (!app.state.gd.fSmallTileMode) {
            if (!fDoneDrawing && !fUnknown) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                c = TextIO().pfnCchGetString(app, idsCargo3, std::span{app.state.szWork});
                l = GetTextExtent(hdc, app.state.szWork, c);
            }
            if (!fDoneDrawing && !fUnknown && !fObjIsThing) {
                c = TextIO().pfnCchGetString(app, idsFuel3, std::span{app.state.szWork});
                l2 = GetTextExtent(hdc, app.state.szWork, c);
                if (l2 > l) {
                    l = l2;
                }
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
                win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + app.state.dyArial8);
                if (idSkip != idflNone) {
                    app.lpui->rgrcRef[1] = rcGauge;
                }
                DrawFleetGauge(app, hdc, &rcGauge, &xf.fl(), 4);
            } else if (ptile->fMinDraw || fUnknown != app.state.gd.fUnknownShip) {
                win32::SetRect(&rcGauge, xLeft, yTop, xRight, app.state.dyArial8 * 2 + yTop + 8);
                win32::FillRect(hdc, &rcGauge, app.lpui->hbrButtonFace);
            }
            app.state.gd.fUnknownShip = fUnknown;
            if (!fObjIsThing) {
                yTop += app.state.dyArial8 + 4;
            }
            if (!fDoneDrawing && !fUnknown) {
                c = TextIO().pfnCchGetString(app, idsCargo3, std::span{app.state.szWork});
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, c);
                win32::SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + app.state.dyArial8);
                app.lpui->rgrcRef[4] = rcGauge;
                if (fObjIsThing) {
                    DrawThingGauge(app, hdc, &rcGauge, &xf.th(), 5);
                } else {
                    DrawFleetGauge(app, hdc, &rcGauge, &xf.fl(), 5);
                }
                if (fObjIsThing) {
                    win32::OffsetRect(&rcGauge, 0, app.state.dyArial8 + 4);
                    win32::FillRect(hdc, &rcGauge, app.lpui->hbrButtonFace);
                    yTop += app.state.dyArial8 + 4;
                }
            }
            yTop += app.state.dyArial8 + 4;
        }
        c = static_cast<std::int16_t>((xRight - xLeft - 10)) / 3;
        xStart = xLeft;
        i = 0;
        while (i <= 2) {
            ControlIO().pfnSetWindowPos(app.lpui->rghwndBtn[static_cast<std::int16_t>((i + 1)) % 3], nullptr, xStart, yTop, c, (app.state.dyArial8 >> 1) + app.state.dyArial8, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
            if (i != 2 || pfl) {
                ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_SHOW);
            }
            i++;
            xStart += c + 6;
        }
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtn[2], (idSkip == idflNone || !fUnknown) && !fObjIsThing);
    }
    return;
}

void PlanetClick(APPSESSION &app, std::int16_t x, std::int16_t y, std::int16_t sks, std::int16_t fRightBtn) {
    std::int16_t  bt;
    POINT16  pt;
    std::int16_t  ctile;
    std::int16_t  dy;
    RECT     rcTitle;
    std::int16_t  i;
    std::int16_t  xRel;
    std::uint16_t iCol;
    std::int16_t  iCur;
    TILE    *prgtile;
    RECT     rc;
    BTNT     btnt;
    HDC      hdc;
    TILE     tile;
    POINT16  ptNew;

    bt = 112;
    if (app.state.sel.grobj == grobjPlanet) {
        prgtile = app.lpui->rgtilePlanet;
        ctile = 6;
    } else {
        if (app.state.sel.grobj != grobjFleet) {
            return;
        }
        prgtile = app.lpui->rgtileShip;
        ctile = 7;
    }
    iCol = static_cast<std::uint32_t>(x) / 198;
    xRel = x - iCol * 198;
    if (xRel >= 4 && xRel < 194) {
        for (i = 0; i < ctile; i++) {
            if (prgtile[i].iCol >= iCol) {
                if (prgtile[i].iCol > iCol) {
                    return;
                }
                if (y >= prgtile[i].yTop && y < (!prgtile[i].fPopped ? app.state.dyArial8 + 3 : prgtile[i].dyFull) + prgtile[i].yTop)
                    break;
            }
        }
        if (i != ctile) {
            pt.x = x;
            pt.y = y;
            rcTitle.top = prgtile[i].yTop;
            rcTitle.bottom = app.state.dyArial8 + 3 + rcTitle.top + 1;
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
                    InitBtnTrack(&btnt, app.lpui->hwndPlanet, nullptr, &rc, bt, 0, 0, 0, nullptr);
                    while (FTrackBtn(app, &btnt)) {
                    }
                    if (btnt.fDown) {
                        prgtile[i].fPopped = prgtile[i].fPopped == 0;
                        ReflowColumn(app, prgtile[i].iCol, i, 1);
                    }
                } else {
                    if (prgtile[i].fPopped) {
                        rcTitle.bottom = rcTitle.top + prgtile[i].dyFull + 1;
                    }
                    hdc = win32::GetDC(app.lpui->hwndPlanet);
                    DrawFuzzyBorder(app, hdc, &rcTitle);
                    win32::SetCapture(app.lpui->hwndPlanet);
                    ptNew = pt;
                    while (FGetMouseMove(&ptNew)) {
                        if (pt.x != ptNew.x || pt.y != ptNew.y) {
                            DrawFuzzyBorder(app, hdc, &rcTitle);
                            OffsetRc(&rcTitle, ptNew.x - pt.x, ptNew.y - pt.y);
                            pt = ptNew;
                            DrawFuzzyBorder(app, hdc, &rcTitle);
                        }
                    }
                    DrawFuzzyBorder(app, hdc, &rcTitle);
                    win32::ReleaseCapture();
                    win32::ReleaseDC(app.lpui->hwndPlanet, hdc);
                    pt.x = ((rcTitle.right - rcTitle.left) >> 1) + rcTitle.left;
                    pt.y = ((rcTitle.bottom - rcTitle.top) >> 1) + rcTitle.top;
                    iCol = 0 > (1 >= pt.x / 198 ? pt.x / 198 : 1) ? 0 : 1 < pt.x / 198 ? 1 : pt.x / 198;
                    iCur = i;
                    for (i = 0; i < ctile && prgtile[i].iCol < iCol; i++) {
                    }
                    for (; i < ctile && prgtile[i].iCol == iCol; i++) {
                        dy = !prgtile[i].fPopped ? app.state.dyArial8 + 3 : prgtile[i].dyFull;
                        if (pt.y < (dy >> 1) + prgtile[i].yTop)
                            break;
                    }
                    if (i == iCur || i == iCur + 1) {
                        i = prgtile[iCur].iCol;
                        if (iCol != i) {
                            prgtile[iCur].iCol = iCol;
                            ReflowColumn(app, iCol, iCur, 1);
                            if (static_cast<std::int16_t>(iCol) < i) {
                                ReflowColumn(app, i, iCur + 1, 1);
                            } else {
                                ReflowColumn(app, i, iCur, 1);
                            }
                        }
                    } else {
                        tile = prgtile[iCur];
                        if (i < iCur) {
                            std::memmove(prgtile + (i + 1), prgtile + i, (iCur - i) * sizeof(TILE));
                            iCur++;
                        } else {
                            std::memmove(prgtile + iCur, prgtile + (iCur + 1), (i - iCur - 1) * sizeof(TILE));
                            i--;
                        }
                        prgtile[i] = tile;
                        prgtile[i].iCol = iCol;
                        if (tile.iCol == iCol) {
                            ReflowColumn(app, iCol, i >= iCur ? iCur : i, 1);
                        } else {
                            ReflowColumn(app, iCol, i, 1);
                            ReflowColumn(app, tile.iCol, iCur, 1);
                        }
                    }
                }
            } else {
                if ((app.state.sel.grobj == grobjFleet && (prgtile[i].grbit == tileFleetOrders || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                 prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == 0x10)) ||
                    prgtile[i].grbit == tileShipList) {
                    ClickInShipOrders(app, pt, sks, 0, fRightBtn);
                } else if (app.state.sel.grobj == grobjPlanet && (prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                        prgtile[i].grbit == tileProductionOrOrbit || prgtile[i].grbit == tilePlanetStats)) {
                    ClickInPlanetOrders(app, pt, sks, 0, fRightBtn);
                }
            }
        }
    }
    return;
}

HCURSOR ClickInPlanetOrders(APPSESSION &app, POINT16 pt, std::int16_t sks, std::int16_t fCursor, std::int16_t fRightBtn) {
    std::array<char, cchCatalogText> szCatalogButtonText{};
    std::int16_t i;
    std::int32_t rglQuan[3];
    std::int16_t iWarp;
    BTNT    btnt;

    if (app.state.sel.grobj != grobjPlanet) {
        return nullptr;
    }
    if (fRightBtn) {
        return nullptr;
    }
    if (win32::PtInRect(&app.lpui->rgrcRef[6], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        i = static_cast<std::int16_t>((pt.y - app.lpui->rgrcRef[6].top)) / app.state.dyArial8;
        app.lpui->GlobalPD.grPopup = grPopupMineral;
        app.lpui->GlobalPD.SetRgi(0, i);
        app.lpui->GlobalPD.SetRgi(2, app.state.sel.pl.rgwtMin[i]);
        app.lpui->GlobalPD.SetRgi(3, static_cast<std::uint32_t>(app.state.sel.pl.rgMinConc[i]));
        ColonyIO().pfnEstimateMining(app, &app.state.sel.pl, rglQuan, -1, 0);
        app.lpui->GlobalPD.SetRgi(4, rglQuan[i]);
        app.lpui->GlobalPD.SetRgi(1, app.state.sel.pl.fHomeworld);
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[7], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        app.lpui->GlobalPD.grPopup = grPopupPlanetIndustry;
        app.lpui->GlobalPD.idPlan = app.state.sel.pl.id;
        app.lpui->GlobalPD.SetFFactory(pt.y >= app.lpui->rgrcRef[7].top + app.state.dyArial8);
        if (app.lpui->GlobalPD.fFactory) {
            app.lpui->GlobalPD.cMax = ColonyIO().pfnMaxFactories(app, &app.state.sel.pl, app.state.idPlayer);
            app.lpui->GlobalPD.cCur = app.state.sel.pl.cFactories;
            app.lpui->GlobalPD.cOperate = ColonyIO().pfnOperableFactories(app, &app.state.sel.pl, app.state.idPlayer, 0);
        } else {
            app.lpui->GlobalPD.cMax = ColonyIO().pfnMaxMines(app, &app.state.sel.pl, app.state.idPlayer);
            app.lpui->GlobalPD.cCur = app.state.sel.pl.cMines;
            app.lpui->GlobalPD.cOperate = ColonyIO().pfnOperableMines(app, &app.state.sel.pl, app.state.idPlayer, 0);
        }
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[8], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        app.lpui->GlobalPD.grPopup = grPopupResources;
        app.lpui->GlobalPD.idPlanet = app.state.sel.pl.id;
        app.lpui->GlobalPD.iPlanetVar = app.lpui->GlobalPD.iPlanVal = ResearchIO().pfnResources(app, &app.state.sel.pl, app.state.idPlayer);
        if (!app.state.sel.pl.fNoResearch) {
            app.lpui->GlobalPD.iPlanVal -= win32::MulDiv(app.lpui->GlobalPD.iPlanetVar, app.state.rgplr[app.state.idPlayer].pctResearch, 100);
        }
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[9], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        app.lpui->GlobalPD.grPopup = grPopupPlanet;
        app.lpui->GlobalPD.idPlanet = app.state.sel.pl.id;
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[10], PointFrom16(pt)) != 0) {
        if (app.state.sel.pl.cDefenses == 0) {
            return nullptr;
        }
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        FGetBestDefensePart(app, &app.lpui->GlobalPD.part);
        app.lpui->GlobalPD.grPopup = grPopupComponent;
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[11], PointFrom16(pt)) != 0) {
        if (app.state.sel.pl.iScanner == 31 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raMacintosh) {
            return nullptr;
        }
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsRaceCannotBuildPlanetaryScannersStarbasesHave, std::span{app.lpui->szPopupBuffer});
        } else {
            ProductionIO().pfnScanner(app, &app.lpui->GlobalPD.part);
            app.lpui->GlobalPD.grPopup = grPopupComponent;
        }
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[13], PointFrom16(pt)) != 0) {
        iWarp = ColonyIO().pfnWarp(app, &app.state.sel.pl, nullptr);
        if (iWarp == 0) {
            return nullptr;
        }
        if (fCursor) {
            return app.lpui->hcurHand;
        }
        szCatalogButtonText = TextIO().pfnCompressed(app, idsSetDest);
        InitBtnTrack(&btnt, app.lpui->hwndPlanet, nullptr, &app.lpui->rgrcRef[13], 8, 80, app.state.gd.fSetMassMode, 1, szCatalogButtonText.data());
        while (FTrackBtn(app, &btnt)) {
        }
        app.state.gd.fSetMassMode ^= btnt.fDown;
    } else if (win32::PtInRect(&app.lpui->rgrcRef[14], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        app.lpui->GlobalPD.grPopup = grPopupShdef;
        app.lpui->GlobalPD.lpshdef = app.state.rglpshdefSB[app.state.idPlayer] + app.state.sel.pl.isb;
        app.lpui->GlobalPD.fHideCounts = 0;
        app.lpui->GlobalPD.fShowDamage = 1;
        app.lpui->GlobalPD.SetFToken(0);
        app.lpui->GlobalPD.fSummary = 0;
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[16], PointFrom16(pt)) != 0) {
        iWarp = ColonyIO().pfnWarp(app, &app.state.sel.pl, nullptr);
        if (iWarp == 0) {
            return nullptr;
        }
        if (fCursor) {
            return app.lpui->hcurArrowHelp;
        }
        app.lpui->GlobalPD.part.hs.grhst = hstSpecialSB;
        app.lpui->GlobalPD.part.hs.iItem = iWarp + 2;
        PartQueryIO().pfnLookup(app, &app.lpui->GlobalPD.part);
        app.lpui->GlobalPD.grPopup = grPopupComponent;
        Popup(app, app.lpui->hwndPlanet, pt.x, pt.y);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[15], PointFrom16(pt)) != 0) {
        ClickInShipOrders(app, pt, sks, 0, fRightBtn);
    } else if (win32::PtInRect(&app.lpui->rgrcRef[17], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return app.lpui->hcurHand;
        }
        szCatalogButtonText = TextIO().pfnCompressed(app, idsRoute2);
        InitBtnTrack(&btnt, app.lpui->hwndPlanet, nullptr, &app.lpui->rgrcRef[17], 8, 80, app.state.gd.fSetRouteMode, 1, szCatalogButtonText.data());
        while (FTrackBtn(app, &btnt)) {
        }
        app.state.gd.fSetRouteMode ^= btnt.fDown;
    }
    return nullptr;
}

void EnsureTileSize(APPSESSION &app, std::int16_t fSmallTiles) {
    std::int16_t    iMul;
    std::int16_t    i;
    GrobjClass grobjSav;

    if (fSmallTiles != app.state.gd.fSmallTileMode) {
        app.state.gd.fSmallTileMode = fSmallTiles;
        iMul = !fSmallTiles ? 1 : -1;
        for (i = 0; i < 6; i++) {
            if (app.lpui->rgtilePlanet[i].grbit == tileProductionOrOrbit) {
                app.lpui->rgtilePlanet[i].dyFull += (app.state.dyArial8 + 2) * 2 * iMul;
            }
            if (app.lpui->rgtilePlanet[i].grbit == tileShipList) {
                app.lpui->rgtilePlanet[i].dyFull += (app.state.dyArial8 + 4) * 2 * iMul;
            }
            if (app.lpui->rgtilePlanet[i].grbit == tileBitmap) {
                app.lpui->rgtilePlanet[i].dyFull += 10 * iMul;
            }
        }
        for (i = 0; i < 7; i++) {
            if (app.lpui->rgtileShip[i].grbit == tileMineralsOrCargo) {
                app.lpui->rgtileShip[i].dyFull += (app.state.dyArial8 * 4 + 2) * iMul;
            }
            if (app.lpui->rgtileShip[i].grbit == tileFleetComp) {
                app.lpui->rgtileShip[i].dyFull += ((app.state.dyArial8 + 2) * 2 + 4 + app.state.dyArial8) * iMul;
            }
            if (app.lpui->rgtileShip[i].grbit == tileFleetOrders) {
                app.lpui->rgtileShip[i].dyFull += (app.state.dyArial8 + 9) * iMul;
            }
            if (app.lpui->rgtileShip[i].grbit == tileShipList) {
                app.lpui->rgtileShip[i].dyFull += (app.state.dyArial8 + 4) * 2 * iMul;
            }
            if (app.lpui->rgtileShip[i].grbit == tileBitmap) {
                app.lpui->rgtileShip[i].dyFull += 10 * iMul;
            }
            if (app.lpui->rgtileShip[i].grbit == tileStarbaseOrWaypoint) {
                app.lpui->rgtileShip[i].dyFull += iMul * 2;
            }
            if (app.lpui->rgtileShip[i].grbit == tileProductionOrOrbit) {
                app.lpui->rgtileShip[i].dyFull += 6 * iMul;
            }
        }
        grobjSav = app.state.sel.grobj;
        app.state.sel.grobj = grobjPlanet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(app, i, -1, 0);
        }
        app.state.sel.grobj = grobjFleet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(app, i, -1, 0);
        }
        app.state.sel.grobj = grobjSav;
    }
    return;
}

void ReflowColumn(APPSESSION &app, std::int16_t iCol, std::int16_t iTile, std::int16_t fRedraw) {
    HDC     hdc;
    std::int16_t yTop;
    std::int16_t ctile;
    std::int16_t i;
    std::int16_t grbit;
    TILE   *ptile;
    RECT    rc;

    grbit = 0;
    if (app.state.sel.grobj == grobjFleet) {
        ptile = app.lpui->rgtileShip;
        ctile = 7;
    } else {
        ptile = app.lpui->rgtilePlanet;
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
                yTop += (!ptile[i].fPopped ? app.state.dyArial8 + 3 : ptile[i].dyFull) + 4;
            }
        }
    }
    if (fRedraw) {
        ControlIO().pfnGetClientRect(app.lpui->hwndPlanet, &rc);
        rc.top = yTop;
        rc.left = 198 * iCol + 4;
        rc.right = rc.left + 191;
        hdc = win32::GetDC(app.lpui->hwndPlanet);
        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    }
    for (; iTile < ctile && ptile[iTile].iCol == iCol; iTile++) {
        ptile[iTile].yTop = yTop;
        ptile[iTile].fFixCtls = 1;
        grbit |= ptile[iTile].grbit;
        yTop += (!ptile[iTile].fPopped ? app.state.dyArial8 + 3 : ptile[iTile].dyFull) + 4;
    }
    if (fRedraw) {
        ControlIO().pfnDrawPlanShip(app, hdc, static_cast<TileBits>(grbit));
        win32::ReleaseDC(app.lpui->hwndPlanet, hdc);
    }
    return;
}

void HandleFocusState(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate) {
    if (lpdis->itemState & win32::ODS_FOCUS) {
        win32::FrameRect(lpdis->hDC, &lpdis->rcItem, app.lpui->hbr50Screen);
    }
    return;
}

void DrawCBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate) {
    std::int16_t fListbox;
    std::int16_t fSelected;
    RECT    rc;

    fSelected = lpdis->itemState & win32::ODS_SELECTED;
    rc = lpdis->rcItem;
    fListbox = lpdis->hwndItem == app.lpui->hwndFleetCompLB || lpdis->hwndItem == app.lpui->hwndPlanetProdLB || inflate > 0;
    if (inflate > 0) {
        inflate = -inflate;
    }
    ControlIO().pfnSendMessage(lpdis->hwndItem, !fListbox ? win32::CB_GETLBTEXT : win32::LB_GETTEXT, lpdis->itemID, reinterpret_cast<LPARAM>(app.state.szWork));
    DrawProductionItem(app, lpdis->hDC, &rc, app.state.szWork, inflate, fSelected, fListbox);
    HandleFocusState(app, lpdis, inflate + 2);
    return;
}

void DrawProductionItem(APPSESSION &app, HDC hdc, RECT *prc, char *psz, std::int16_t inflate, std::int16_t fSelected, std::int16_t fListbox) {
    HFONT    hfntSav;
    char    *pch;
    std::int16_t  ichT;
    COLORREF cr;
    std::int16_t  pctDmg;
    char     szT[20];
    RECT     rcIn;
    std::int16_t  ich;
    std::int16_t  fDoubleDraw;
    COLORREF crForeSav;
    std::int16_t  fFleet;
    RECT     rcDraw;
    std::int16_t  dx;
    HBRUSH   hbr;
    std::int16_t  fItalic;
    std::int16_t  cch;
    std::int16_t  bkSav;
    RECT     rc;

    fItalic = 0;
    fDoubleDraw = 0;
    fFleet = 0;
    rcIn = *prc;
    rc = rcIn;
    if (!fSelected) {
        hbr = app.lpui->hbrWindow;
        switch (*psz) {
        default:
            cr = 0xff;
            break;
        case 'I':
            fItalic = 1;
            /* fallthrough */
        case ' ':
        LDefCase:
            if (app.lpui->crWindow == 0) {
                cr = 0xffffff;
                break;
            }
            cr = 0;
            break;
        case 'P':
            fDoubleDraw = 1;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = 1;
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
        cr = app.lpui->crWindow;
        switch (*psz) {
        default:
            hbr = app.lpui->hbrRed;
            break;
        case 'I':
            fItalic = 1;
            /* fallthrough */
        case ' ':
        LDefCaseSel:
            if (app.lpui->crWindow == 0) {
                hbr = static_cast<HBRUSH>(win32::GetStockObject(win32::WHITE_BRUSH));
                break;
            }
            hbr = static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH));
            break;
        case 'P':
            fDoubleDraw = 1;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = 1;
            goto LDefCaseSel;
        case '*':
            hbr = app.lpui->hbrGreen;
            break;
        case '#':
            hbr = app.lpui->hbrBlue;
            break;
        case '&':
            hbr = app.lpui->hbrGray;
        }
    }
    if (fListbox == 2) {
        hbr = app.lpui->hbrButtonFace;
    }
    win32::FillRect(hdc, &rcIn, hbr);
    if (fDoubleDraw) {
        rcDraw = rcIn;
        dx = rcIn.right - rcIn.left;
        dx = static_cast<std::int16_t>((dx * pctDmg)) / 100;
        rcDraw.right = rcDraw.left + dx;
        win32::FillRect(hdc, &rcDraw, app.lpui->hbrRed);
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
            fItalic = 1;
        }
    }
    crForeSav = win32::SetTextColor(hdc, cr);
    bkSav = win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (fItalic) {
        hfntSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[3]));
    }
    pch = psz + ich;
    cch = std::strlen(pch);
    dx = LOWORD(GetTextExtent(hdc, pch, cch));
    while (cch > 0 && dx > rcIn.right - rcIn.left) {
        cch = static_cast<std::int16_t>(CchUtf8Prefix(pch, cch - 1));
        dx = LOWORD(GetTextExtent(hdc, pch, cch));
    }
    win32::TextOutA(hdc, rcIn.left, rcIn.top, pch, cch);
    if (fItalic) {
        win32::SelectObject(hdc, hfntSav);
    }
    if (ich >= 6) {
        if ((psz[ich - 5] - ' ') & 2) {
            if (psz[ich - 1] == '*') {
                ich = TextIO().pfnCchGetString(app, idsNeeded, std::span{szT});
                goto LRightOut;
            }
            TextIO().pfnCchGetString(app, idsUpTo, std::span{szT});
        } else {
            szT[0] = 0;
        }
        ich = std::strlen(szT);
        for (ichT = 2 - fFleet; ichT < 6 && psz[ichT + fFleet] == ' '; ichT++) {
        }
        std::strncpy(&szT[ich], psz + (ichT + fFleet), 6 - ichT);
        ich += 6 - ichT;
        if (!fFleet && ((psz[fDoubleDraw + 1] - ' ') & 1)) {
            szT[ich++] = '%';
        }
    LRightOut:
        RightTextOut(app, hdc, rcIn.right, rcIn.top, szT, ich, 0);
    }
    win32::SetTextColor(hdc, crForeSav);
    win32::SetBkMode(hdc, bkSav);
    return;
}

void DrawPlanShip(APPSESSION &app, HDC hdc, TileBits grbit) {
    HFONT    hfontSav;
    OBJ      objNull;
    std::int16_t  ctile;
    COLORREF crFore;
    OBJ      obj;
    std::int16_t  fMin;
    std::int16_t  i;
    COLORREF crBack;
    std::int16_t  fErase;
    TILE    *ptile;
    std::int16_t  fDC;
    RECT     rc;

    fDC = 0;
    objNull.SetPfl(nullptr);
    if (app.state.sel.id == -1) {
        for (i = 0; i < 13; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(app.lpui->rghwndOrderDD[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndShipDD, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndBattleDD, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndShipLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndFleetCompLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndPlanetProdLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndRepCB, win32::SW_HIDE);
        for (i = 0; i < 19; i++) {
            app.lpui->rgrcRef[i].bottom = -6;
            app.lpui->rgrcRef[i].top = -5;
        }
        if (app.state.rgplr[app.state.idPlayer].fDead && hdc) {
            ControlIO().pfnGetClientRect(app.lpui->hwndPlanet, &rc);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            i = TextIO().pfnCchGetString(app, idsDeceased, std::span{app.state.szWork});
            DiaganolTextOut(app, hdc, &rc, app.state.szWork, i);
        }
    } else {
        if (app.state.sel.grobj == grobjFleet) {
            ptile = app.lpui->rgtileShip;
            ctile = 7;
            obj.SetPfl(&app.state.sel.fl);
        } else {
            ptile = app.lpui->rgtilePlanet;
            ctile = 6;
            obj.SetPpl(&app.state.sel.pl);
        }
        if (!hdc) {
            fDC = 1;
            hdc = win32::GetDC(app.lpui->hwndPlanet);
        }
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[0]));
        crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
        crFore = win32::SetTextColor(hdc, app.lpui->crButtonText);
        fErase = (grbit & tileErase) != 0;
        fMin = (grbit & tileMinimized) != 0;
        for (i = 0; i < ctile; i++) {
            if (grbit & ptile[i].grbit) {
                ptile[i].fErase = fErase;
                ptile[i].fMinDraw = fMin;
                ptile[i].pfn(app, hdc, ptile + i, !ptile[i].fNullPtr ? obj : objNull);
            }
        }
        win32::SetTextColor(hdc, crFore);
        win32::SetBkColor(hdc, crBack);
        win32::SelectObject(hdc, hfontSav);
        if (fDC) {
            win32::ReleaseDC(app.lpui->hwndPlanet, hdc);
        }
    }
    QueuePlanetPaneLayout(app);
    return;
}

void SetPlanetTitleBar(APPSESSION &app, HWND hwnd) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    char  szTitle[30];
    char *psz;

    if (app.state.sel.grobj == grobjPlanet) {
        psz = TextIO().pfnPlanetName(app, app.state.sel.pl.id);
        TextIO().pfnCchGetString(app, idsPlanet2, std::span{szTitle});
        win32::lstrcatA(szTitle, psz);
        psz = szTitle;
    } else if (app.state.sel.grobj == grobjFleet) {
        psz = PszGetFleetName(app, app.state.sel.fl.id);
    } else {
        szCatalogPsz = TextIO().pfnCompressed(app, idsPlanetView);
        psz = szCatalogPsz.data();
    }
    ControlIO().pfnSetWindowText(hwnd, psz);
    return;
}

void FillShipDD(APPSESSION &app, std::int16_t idSkip) {
    THING  *lpthMac;
    std::int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    POINT16 ptSel;

    ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_RESETCONTENT, 0, 0);
    if (app.state.sel.grobj == grobjPlanet) {
        ptSel = app.state.rgptPlan[app.state.sel.id];
    } else {
        ptSel = app.state.sel.fl.pt;
    }
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if ((idSkip == idflNone && app.state.sel.id == lpfl->idPlanet) ||
            (idSkip != idflNone && lpfl->id != idSkip && lpfl->pt.x == app.state.sel.fl.pt.x && lpfl->pt.y == app.state.sel.fl.pt.y)) {
            PszGetFleetName(app, lpfl->id);
            std::memmove(&app.state.szWork[1], app.state.szWork, 50);
            app.state.szWork[0] = lpfl->iPlayer == app.state.idPlayer ? 32 : 120;
            ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
        }
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMineralPacket && lpth->pt().x() == ptSel.x && lpth->pt().y() == ptSel.y) {
            PszGetThingName(app, lpth->idFull());
            std::memmove(&app.state.szWork[1], app.state.szWork, 50);
            app.state.szWork[0] = lpth->iplr() == app.state.idPlayer ? 32 : 120;
            ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
        }
    }
    ControlIO().pfnSendMessage(app.lpui->hwndShipDD, win32::CB_SETCURSEL, 0, 0);
    return;
}

void FillPlanetProdLB(APPSESSION &app, HWND hwnd, PLPROD *lpplprod, PLANET *lppl) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    std::int32_t rgwtMin[4];
    std::int16_t i;
    char    szTemp[80];
    std::int32_t resCost;
    char   *psz;

    /* Given a planet, leave its queue's first item in szWork; see
       PszProdQueueTop. */
    if (lppl) {
        PszProdQueueTop(app, lppl, lpplprod);
        return;
    }
    lppl = &app.state.sel.pl;
    if (!hwnd) {
        hwnd = app.lpui->hwndPlanetProdLB;
    }
    ControlIO().pfnSendMessage(hwnd, win32::LB_RESETCONTENT, 0, 0);
    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    if (!lpplprod || lpplprod->iprodMac == 0) {
        szCatalogPsz = TextIO().pfnCompressed(app, idsQueueEmpty);
        psz = szCatalogPsz.data();
    } else {
        if (!app.lpui->hwndProdDlg)
            goto NoMsg;
        szCatalogPsz = TextIO().pfnCompressed(app, idsTopQueue);
        psz = szCatalogPsz.data();
    }
    ControlIO().pfnSendMessage(hwnd, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
NoMsg:
    if (lpplprod) {
        resCost = 0;
        for (i = 0; i < 4; i++) {
            rgwtMin[i] = 0;
        }
        for (i = 0; i < lpplprod->iprodMac; i++) {
            FProdItemLine(app, lppl, lpplprod, i, 1, szTemp);
            ControlIO().pfnSendMessage(hwnd, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szTemp));
        }
    }
    return;
}

// ShowPlanetSel refills the planet tile's production queue when
// ChangeMainObjSel selects a planet.
void ShowPlanetSel(APPSESSION &app) {
    ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, nullptr);
    ControlIO().pfnSendMessage(app.lpui->hwndPlanetProdLB, win32::LB_SETCURSEL, 0, 0);
    return;
}

// ShowFleetSel refills the fleet tiles' lists when ChangeMainObjSel selects
// a fleet.
void ShowFleetSel(APPSESSION &app) {
    FillOrdersLB(app);
    FillFleetCompLB(app);
    FillBattleDD(app, app.state.sel.fl.iplan + 1);
    ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[0], win32::CB_SETCURSEL, app.state.sel.fl.lpplord->rgord()[0].grTask, 0);
    return;
}

// ShowMainObjSel redraws the planet window for the object ChangeMainObjSel
// selected; fSameType is set when it is the same kind as before.
void ShowMainObjSel(APPSESSION &app, std::int16_t fSameType, std::int16_t idSkip) {
    std::int16_t i;

    if (!fSameType) {
        for (i = 0; i < 13; i++) {
            ui::ShowWindow(app.lpui->rghwndBtn[i], win32::SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ui::ShowWindow(app.lpui->rghwndOrderDD[i], win32::SW_HIDE);
        }
        ui::ShowWindow(app.lpui->hwndOrderED, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndShipDD, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndBattleDD, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndShipLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndFleetCompLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndPlanetProdLB, win32::SW_HIDE);
        ui::ShowWindow(app.lpui->hwndRepCB, win32::SW_HIDE);
        for (i = 0; i < 19; i++) {
            app.lpui->rgrcRef[i].bottom = -6;
            app.lpui->rgrcRef[i].top = -5;
        }
    }
    FillShipDD(app, idSkip);
    if (fSameType) {
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(0x4fff));
    } else {
        ControlIO().pfnInvalidateRect(app.lpui->hwndPlanet, nullptr, 1);
        if ((app.lpui->grbitScan & grbitScanAddWaypoints) && app.state.sel.grobj == grobjPlanet) {
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan & 0xffef);
            ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
        }
    }
    SetPlanetTitleBar(app, app.lpui->hwndPlanet);
    if (app.state.gd.fTutorial) {
        PresentationIO().pfnTutor(app);
    }
    return;
}

// FillSelProdLB refills the planet tile's production queue for the
// selected planet.
void FillSelProdLB(APPSESSION &app) {
    ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, nullptr);
    return;
}

// RedrawPlanShip redraws the planet window's tiles in grbit.
void RedrawPlanShip(APPSESSION &app, TileBits grbit) {
    ControlIO().pfnDrawPlanShip(app, nullptr, grbit);
    return;
}
