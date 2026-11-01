#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

BattleUnitFlags mpimdgrbitBU[8] = {grBuClassUnarmed, grBuClassUnarmed, grBuClassScout,   grBuClassWarship,
                                   grBuClassUtility, grBuClassBomber,  grBuClassUnarmed, grBuClassUnarmed};

LRESULT  PopupWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;

    switch (message) {
    case win32::WM_CREATE:
        return 0;
    case win32::WM_DESTROY:
        // Native close paths do not pass through the mouse-release handler.
        if (app.lpui->hwndPopup == hwnd) {
            app.lpui->hwndPopup = 0;
            app.lpui->GlobalPD.grPopup = static_cast<GrPopupType>(0);
        }
        return 0;
    case win32::WM_ERASEBKGND:
        switch (app.lpui->GlobalPD.grPopup) {
        case grPopupComponent:
        case grPopupShdef:
        case grPopupShdefSB:
            ControlIO().pfnGetClientRect(hwnd, &rc);
            win32::FillRect((HDC)wParam, &rc, app.lpui->hbrButtonFace);
            return 1;
        default:
            goto Default;
        }
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawPopup(app, hwnd, hdc);
        win32::EndPaint(hwnd, &ps);
        return 0;
    case win32::WM_LBUTTONUP:
    case win32::WM_RBUTTONUP:
        if (!app.lpui->hwndPopup) {
            return 0;
        }
        ControlIO().pfnDestroyWindow(app.lpui->hwndPopup);
        app.lpui->hwndPopup = 0;
        app.lpui->GlobalPD.grPopup = static_cast<GrPopupType>(0);
        win32::ReleaseCapture();
        if (!app.state.gd.fTutorial) {
            return 0;
        }
        app.state.tutor.fProgress = 1;
        PresentationIO().pfnTutor(app);
        return 0;
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
}

int16_t FIsPopupHullType(APPSESSION &app, int16_t ishdef) {
    HullCategory imd;

    if (app.lpui->GlobalPD.grbit == 0 || app.lpui->GlobalPD.grbit == 0xff) {
        return 1;
    }
    imd = static_cast<HullCategory>(PartQueryIO().pfnHull(app.state.rglpshdef[app.lpui->GlobalPD.lpfl->iPlayer][ishdef].hul.ihuldef)->imdCategory);
    return mpimdgrbitBU[imd] & app.lpui->GlobalPD.grbit;
}

void DrawPopup(APPSESSION &app, HWND hwnd, HDC hdc) {
    COLORREF crBack;
    char     szT[80];
    int16_t  yCur;
    int16_t  i;
    int16_t  c;
    int16_t  bkMode;
    HFONT    hfontSav;
    char    *psz;
    int16_t  dx;
    COLORREF crFore;
    RECT     rc;
    char    *lpsz;
    int16_t  csh;
    int16_t  dpT;
    char     szTB[40];

    crBack = win32::SetBkColor(hdc, 0xffffff);
    crFore = win32::SetTextColor(hdc, 0);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    hfontSav = (HFONT)win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    ControlIO().pfnGetClientRect(hwnd, &rc);
    switch (app.lpui->GlobalPD.grPopup) {
    default:
        break;
    case grPopupMineral:
        CtrTextOut(hdc, rc.right >> 1, 4, app.state.rgszMinerals[app.lpui->GlobalPD.rgi[0]], 0);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        psz = TextIO().pfnCompressed(app, idsMineralConcentration);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
        RightTextOut(app, hdc, dx, app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsSurface), 0, 0);
        if (app.lpui->GlobalPD.rgi[2] >= 0) {
            c = win32::wsprintfA(app.state.szWork, "%ldkT", app.lpui->GlobalPD.rgi[2]);
        } else {
            strcpy(app.state.szWork, TextIO().pfnCompressed(app, idsUnknown2));
            c = strlen(app.state.szWork);
        }
        win32::TextOutA(hdc, dx, app.state.dyArial8 + 4, app.state.szWork, c);
        RightTextOut(app, hdc, dx, app.state.dyArial8 * 2 + 4, TextIO().pfnCompressed(app, idsMineralConcentration), 0, 0);
        if (app.lpui->GlobalPD.rgi[3] > 0) {
            c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, app.lpui->GlobalPD.rgi[3]);
            if (app.lpui->GlobalPD.rgi[1] != 0) {
                c += win32::wsprintfA(&app.state.szWork[c], TextIO().pfnCompressed(app, app.lpui->GlobalPD.rgi[3] < 30 ? idsN30 : idsHw));
            }
        } else {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsUnknown2));
        }
        win32::TextOutA(hdc, dx, app.state.dyArial8 * 2 + 4, app.state.szWork, c);
        if (app.lpui->GlobalPD.rgi[4] < 0)
            break;
        RightTextOut(app, hdc, dx, 3 * app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsMiningRate), 0, 0);
        TextIO().pfnCchGetString(app, idsLdktYr, szT);
        c = win32::wsprintfA(app.state.szWork, szT, app.lpui->GlobalPD.rgi[4]);
        win32::TextOutA(hdc, dx, 3 * app.state.dyArial8 + 4, app.state.szWork, c);
        break;
    case grPopupPlayer:
        CtrTextOut(hdc, rc.right >> 1, 4, TextIO().pfnPlayerName(app, app.lpui->GlobalPD.iPlayer, 1, 1, 1, 0, nullptr), 0);
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPlayerD), app.lpui->GlobalPD.iPlayer + 1);
        CtrTextOut(hdc, rc.right >> 1, app.state.dyArial8 + 4, app.state.szWork, c);
        break;
    case grPopupFleet:
        if (rc.bottom - rc.top < app.state.dyArial8 * 2) {
            c = TextIO().pfnCchGetString(app, idsNone2, app.state.szWork);
            win32::TextOutA(hdc, 4, 4, app.state.szWork, c);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            break;
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = TextIO().pfnCchGetString(app, idsShipName, app.state.szWork);
        win32::TextOutA(hdc, 4, 4, app.state.szWork, c);
        RightTextOut(app, hdc, rc.right - 4 - app.lpui->GlobalPD.dxDamage, 4, "#", 1, 0);
        if (app.lpui->GlobalPD.dxDamage != 0) {
            c = TextIO().pfnCchGetString(app, idsDamage2, app.state.szWork);
            RightTextOut(app, hdc, rc.right - 4, 4, app.state.szWork, c, 0);
            win32::PatBlt(hdc, 4, app.state.dyArial8 + 2, rc.right - 8, 1, win32::BLACKNESS);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        yCur = app.state.dyArial8 + 4;
        for (i = 0; i < 16; i++) {
            if (app.lpui->GlobalPD.lpfl->rgcsh[i] > 0 && (app.lpui->GlobalPD.grbit == 0 || FIsPopupHullType(app, i))) {
                if (app.lpui->GlobalPD.fRedDamage) {
                    if (app.lpui->GlobalPD.lpfl->rgdv[i].dp != 0) {
                        win32::SetTextColor(hdc, 0xff);
                    } else {
                        win32::SetTextColor(hdc, 0);
                    }
                }
                DecorateHullName(app, app.lpui->GlobalPD.lpfl->iplr(), i, szTB);
                lpsz = szTB;
                win32::TextOutA(hdc, 4, yCur, lpsz, strlen(lpsz));
                c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->GlobalPD.lpfl->rgcsh[i]);
                RightTextOut(app, hdc, rc.right - 4 - app.lpui->GlobalPD.dxDamage, yCur, app.state.szWork, c, 0);
                if (app.lpui->GlobalPD.fRedDamage && app.lpui->GlobalPD.lpfl->rgdv[i].dp != 0) {
                    csh = app.lpui->GlobalPD.lpfl->rgcsh[i];
                    csh = LOWORD((int32_t)((uint32_t)(app.lpui->GlobalPD.lpfl->rgdv[i].dp & 0x7f) * csh) / 0x64);
                    if (csh <= 0) {
                        csh = 1;
                    }
                    dpT = (uint32_t)(app.lpui->GlobalPD.lpfl->rgdv[i].dp >> 7 & 0x1ff) / 5;
                    if (dpT == 0) {
                        dpT = 1;
                    }
                    c = win32::wsprintfA(app.state.szWork, "%d@%d%%", csh, dpT);
                    RightTextOut(app, hdc, rc.right - 4, yCur, app.state.szWork, c, 0);
                }
                yCur += app.state.dyArial8;
            }
        }
        if (!app.lpui->GlobalPD.fRedDamage)
            break;
        win32::SetTextColor(hdc, 0);
        break;
    case grPopupUnknownObj:
        psz = TextIO().pfnCompressed(app, idsPlanet);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        RightTextOut(app, hdc, dx, 4, psz, strlen(psz), 0);
        RightTextOut(app, hdc, dx, app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsId), 0, 0);
        RightTextOut(app, hdc, dx, app.state.dyArial8 * 2 + 4, TextIO().pfnCompressed(app, idsX), 0, 0);
        RightTextOut(app, hdc, dx, 3 * app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsY), 0, 0);
        psz = TextIO().pfnPlanetName(app, app.state.sel.scan.idpl);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::TextOutA(hdc, dx, 4, psz, strlen(psz));
        c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.sel.scan.idpl + 1);
        win32::TextOutA(hdc, dx, app.state.dyArial8 + 4, app.state.szWork, c);
        c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.sel.scan.pt.x);
        win32::TextOutA(hdc, dx, app.state.dyArial8 * 2 + 4, app.state.szWork, c);
        c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.sel.scan.pt.y);
        win32::TextOutA(hdc, dx, 3 * app.state.dyArial8 + 4, app.state.szWork, c);
        break;
    case grPopupPlanetEnv:
        PtDisplayPlanetStateInfo(app, hdc, 1);
        break;
    case grPopupShipOrders:
        PtDisplayZipOrdInfo(app, hdc, rc.right >> 1, 1);
        break;
    case grPopupPlanet:
        PtDisplayPlanetPopInfo(app, hdc, 1);
        break;
    case grPopupPlanetIndustry:
        PtDisplayFactoryMineInfo(app, hdc, rc.right, 1);
        break;
    case grPopupResources:
        PtDisplayResourceInfo(app, hdc, rc.right, 1);
        break;
    case grPopupComponent:
        DisplayComponentInfo(app, hdc, rc.right, rc.bottom, &app.lpui->GlobalPD.part);
        break;
    case grPopupString:
        PtDisplayString(app, hdc, rc.right, 1);
        break;
    case grPopupShdef:
    case grPopupShdefSB:
        app.state.fStarbaseMode = (int16_t)app.lpui->GlobalPD.lpshdef->hul.ihuldef >= ihuldefOrbitalFort;
        DrawSlotDlg(app, hwnd, hdc, &rc, -1);
        rc.top = app.state.dyArial8 + 306;
        rc.left += 6;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        strcpy(app.state.szWork, app.lpui->GlobalPD.lpshdef->hul.szClass);
        CtrTextOut(hdc, ((rc.right - 0x4c) >> 1) + 0x4c, 6, app.state.szWork, 0);
        DrawBuildSelHull(app, hwnd, hdc, -1, &rc);
    }
    win32::SelectObject(hdc, hfontSav);
    win32::SetBkMode(hdc, bkMode);
    win32::SetTextColor(hdc, crFore);
    win32::SetBkColor(hdc, crBack);
    return;
}

void Popup(APPSESSION &app, HWND hwnd, int16_t x, int16_t y) {
    HDC     hdc;
    POINT16 pt;
    int16_t dy;
    int16_t i;
    int16_t c;
    HFONT   hfontSav;
    char   *psz;
    int16_t dx;
    POINT16 ptT;
    int16_t dx2;
    int16_t dxDamage;
    int16_t dxL;
    char   *lpsz;
    int16_t dxR;
    char    szTB[40];
    int16_t dxName;
    int16_t dxCoord;

    pt.x = x;
    pt.y = y;
    ClientToScreen16(hwnd, &pt);
    hdc = win32::GetDC(hwnd);
    hfontSav = (HFONT)win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    switch (app.lpui->GlobalPD.grPopup) {
    default:
        break;
    case grPopupMineral:
        psz = TextIO().pfnCompressed(app, idsMineralConcentration0000000kt);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        dy = 3 * app.state.dyArial8 + 8;
        if (app.lpui->GlobalPD.rgi[4] < 0)
            break;
        dy += app.state.dyArial8;
        break;
    case grPopupPlayer:
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        psz = TextIO().pfnPlayerName(app, app.lpui->GlobalPD.iPlayer, 1, 1, 1, 0, nullptr);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        dx2 = LOWORD(GetTextExtent(hdc, "Player #16", 10)) + 8;
        if (dx2 > dx) {
            dx = dx2;
        }
        dy = app.state.dyArial8 * 2 + 8;
        break;
    case grPopupFleet:
        dxR = 0;
        dxDamage = 0;
        dy = app.state.dyArial8 + 8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        psz = TextIO().pfnCompressed(app, idsShipName);
        dxL = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        for (i = 0; i < 16; i++) {
            if (app.lpui->GlobalPD.lpfl->rgcsh[i] > 0 && (app.lpui->GlobalPD.grbit == 0 || FIsPopupHullType(app, i))) {
                dy += app.state.dyArial8;
                DecorateHullName(app, app.lpui->GlobalPD.lpfl->iplr(), i, szTB);
                lpsz = szTB;
                dx = LOWORD(GetTextExtent(hdc, lpsz, strlen(lpsz)));
                dxL = dxL <= dx ? dx : dxL;
                c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->GlobalPD.lpfl->rgcsh[i]);
                dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
                dxR = dxR <= dx ? dx : dxR;
                if (app.lpui->GlobalPD.fRedDamage && (app.lpui->GlobalPD.lpfl->rgdv[i].dp >> 7 & 0x1ff) && dxDamage == 0) {
                    psz = TextIO().pfnCompressed(app, idsN9999999);
                    dxDamage = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
                }
            }
        }
        if (dy == app.state.dyArial8 + 8) {
            psz = TextIO().pfnCompressed(app, idsShipName);
            dxL = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        }
        app.lpui->GlobalPD.dxDamage = dxDamage;
        dx = dxL + dxR + 16 + dxDamage;
        break;
    case grPopupUnknownObj:
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        dy = app.state.dyArial8 * 4 + 8;
        psz = TextIO().pfnCompressed(app, idsPlanet);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        psz = TextIO().pfnPlanetName(app, app.state.sel.scan.idpl);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        dxName = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        dxCoord = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN9999), 4));
        dx += dxName <= dxCoord ? dxCoord : dxName;
        break;
    case grPopupPlanetEnv:
        ptT = PtDisplayPlanetStateInfo(app, hdc, 0);
        goto SetDxDy;
    case grPopupShipOrders:
        ptT = PtDisplayZipOrdInfo(app, hdc, 0, 0);
        goto SetDxDy;
    case grPopupPlanet:
        ptT = PtDisplayPlanetPopInfo(app, hdc, 0);
        goto SetDxDy;
    case grPopupResources:
        ptT = PtDisplayResourceInfo(app, hdc, 200, 0);
        goto SetDxDy;
    case grPopupPlanetIndustry:
        ptT = PtDisplayFactoryMineInfo(app, hdc, 200, 0);
    SetDxDy:
        dx = ptT.x + 2;
        dy = ptT.y + 2;
        break;
    case grPopupComponent:
        dx = (app.state.dyArial8 <= 14 ? 0 : 40) + 344;
        dy = app.lpui->dyArial10 + 72 + 12 * app.state.dyArial8 + 6;
        break;
    case grPopupString:
        ptT = PtDisplayString(app, hdc, app.lpui->GlobalPD.dxOut, 0);
        goto SetDxDy;
    case grPopupShdef:
    case grPopupShdefSB:
        app.lpui->mdBuild = app.lpui->GlobalPD.grPopup == grPopupShdef ? mdBuildShdef : mdBuildHuldef;
        app.lpui->lpshdefBuild = app.lpui->GlobalPD.lpshdef;
        UpdateSlotGlobals(app);
        dx = 340;
        dy = app.state.dyArial8 + 306 + 6 * app.state.dyArial8 + 8;
        if (app.state.gd.mdScreenSize <= 0 || app.lpui->GlobalPD.grPopup != grPopupShdef)
            break;
        dy += 3 * app.state.dyArial8;
        break;
    case grPopupUnknown:
        dx = 120;
        dy = 80;
    }
    win32::SelectObject(hdc, hfontSav);
    win32::ReleaseDC(hwnd, hdc);
    pt.x -= dx;
    pt.y -= dy;
    pt.x = (((0) > ((((pt.x) < (win32::GetSystemMetrics(win32::SM_CXSCREEN) - dx)) ? (pt.x) : (win32::GetSystemMetrics(win32::SM_CXSCREEN) - dx)))) ? (0) : ((((pt.x) < (win32::GetSystemMetrics(win32::SM_CXSCREEN) - dx)) ? (pt.x) : (win32::GetSystemMetrics(win32::SM_CXSCREEN) - dx))));
    pt.y = (((0) > ((((pt.y) < (win32::GetSystemMetrics(win32::SM_CYSCREEN) - dy)) ? (pt.y) : (win32::GetSystemMetrics(win32::SM_CYSCREEN) - dy)))) ? (0) : ((((pt.y) < (win32::GetSystemMetrics(win32::SM_CYSCREEN) - dy)) ? (pt.y) : (win32::GetSystemMetrics(win32::SM_CYSCREEN) - dy))));
    // Popup drawing reads hwndPopup, so publish the handle before the first paint.
    app.lpui->hwndPopup = ui::CreateWindowEx(win32::WS_EX_NOACTIVATE, app.lpui->szPopup, nullptr, win32::WS_POPUP | win32::WS_BORDER,
                               pt.x, pt.y, dx, dy, hwnd, nullptr, app.lpui->hInst, nullptr);
    ControlIO().pfnSendMessage(app.lpui->hwndPopup, win32::WM_SETFONT, (WPARAM)app.lpui->rghfontArial8[0], 0);
    ui::ShowWindow(app.lpui->hwndPopup, win32::SW_SHOW);
    win32::SetCapture(app.lpui->hwndPopup);
    return;
}

int16_t PopupMenu(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t cString, int32_t *rgids, const char *const *rgsz, int16_t iChecked, int16_t fRightBtn) {
    const char *pszTitle;
    int16_t tpm;
    POINT16 pt;
    int16_t i;
    char    szTemp[128];
    HMENU   hmenuSub;
    HMENU   hmenuPopup;
    char   *pszT;
    const char   *psz;
    win32::MSG     msg;
    int16_t fChecked;
    int16_t fCheckedCur;

    hmenuSub = 0;
    pt.x = x;
    pt.y = y;
    ClientToScreen16(hwnd, &pt);
    hmenuPopup = win32::CreatePopupMenu();
    app.lpui->iPopMenuSel = -1;
    for (i = 0; i < cString; i++) {
        if (rgids && (iChecked != -2 || !rgsz)) {
            if (rgids[i] == -1) {
                win32::AppendMenuA(hmenuPopup, win32::MF_SEPARATOR, 0, nullptr);
            } else {
                if (rgids[i] & 0x10000000) {
                    psz = "Deep Space";
                } else if (rgids[i] & 0x40000000) {
                    psz = TextIO().pfnCompressed(app, static_cast<StringId>(LOWORD(rgids[i])));
                } else if (rgids[i] & 0x20000000) {
                    psz = PszGetThingName(app, LOWORD(rgids[i]));
                } else if (rgids[i] & 0x80000000) {
                    psz = PszGetFleetName(app, LOWORD(rgids[i]) | 0x8000);
                } else {
                    psz = TextIO().pfnPlanetName(app, LOWORD(rgids[i]));
                }
                pszT = szTemp;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                win32::AppendMenuA(hmenuPopup, i == iChecked ? win32::MF_CHECKED : win32::MF_BYCOMMAND, i + 15000, szTemp);
            }
        } else if (!rgsz[i]) {
            pszTitle = rgsz[i + 1];
            fChecked = !rgids ? 0 : LOWORD(rgids[i + 1]);
            hmenuSub = win32::CreatePopupMenu();
            for (i += 2; i < cString && rgsz[i]; i++) {
                if (!rgids) {
                    fCheckedCur = i == iChecked;
                    fChecked |= fCheckedCur;
                } else {
                    fCheckedCur = LOWORD(rgids[i]);
                }
                if (*rgsz[i] == -1 && rgsz[i][1] == 0) {
                    win32::AppendMenuA(hmenuSub, win32::MF_SEPARATOR, 0, nullptr);
                } else {
                    pszT = szTemp;
                    psz = rgsz[i];
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    win32::AppendMenuA(hmenuSub, !fCheckedCur ? win32::MF_BYCOMMAND : win32::MF_CHECKED, i + 15000, szTemp);
                }
            }
            win32::AppendMenuA(hmenuPopup, (!fChecked ? 0 : 8) | 0x10, (UINT_PTR)hmenuSub, pszTitle);
        } else if (*rgsz[i] == -1 && rgsz[i][1] == 0) {
            win32::AppendMenuA(hmenuPopup, win32::MF_SEPARATOR, 0, nullptr);
        } else {
            pszT = szTemp;
            psz = rgsz[i];
            while (*psz != 0) {
                if ((*pszT++ = *psz++) == '&') {
                    *pszT++ = '&';
                }
            }
            *pszT = 0;
            win32::AppendMenuA(hmenuPopup, (iChecked == -2 ? rgids[i] : i == iChecked) == 0 ? win32::MF_BYCOMMAND : win32::MF_CHECKED, i + 15000, szTemp);
        }
    }
    if (fRightBtn) {
        tpm = win32::TPM_RIGHTBUTTON;
    } else {
        tpm = win32::TPM_LEFTBUTTON;
    }
    ui::TrackPopupMenu(hmenuPopup, tpm, pt.x, pt.y, 0, app.lpui->hwndFrame, nullptr);
    win32::DestroyMenu(hmenuPopup);
    if (hmenuSub) {
        win32::DestroyMenu(hmenuSub);
    }
    /* The original accepted only the first 100 items; the target popups now
       list every object at a location. */
    if (ui::PeekMessage(&msg, app.lpui->hwndFrame, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15000 + (WPARAM)cString) {
        app.lpui->iPopMenuSel = msg.wParam - 15000;
    }
    return app.lpui->iPopMenuSel;
}

POINT16 PtDisplayPlanetStateInfo(APPSESSION &app, HDC hdc, int16_t fPrint) {
    POINT16  pt;
    int16_t  y;
    int16_t  xMax;
    int16_t  cch;
    int16_t  x;
    int16_t  iNewVal;
    PLANET  *lppl;
    int16_t  pctDesireOld;
    int16_t  pctDesire;
    int16_t  iValSav;
    StringId ids;
    int16_t  dChg;
    char     szOut[90];

    y = 4;
    xMax = 4;
    x = 4;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, app.state.rgszPlanetAttr[app.lpui->GlobalPD.iPlanetVar], 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsCurrently), 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (app.lpui->GlobalPD.iPlanVal >= 0) {
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), app.lpui->GlobalPD.iPlanVal), 0, fPrint);
    } else {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsUnknown2), 0, fPrint);
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    if (app.lpui->GlobalPD.iPlrMin == envImmune) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonistsImmune), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsEffects), 0, fPrint);
        DxStreamTextOut(hdc, &x, y, app.state.rgszPlanetAttr[app.lpui->GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonistsPreferPlanetsWhere), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        DxStreamTextOut(hdc, &x, y, app.state.rgszPlanetAttr[app.lpui->GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsBetween), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), app.lpui->GlobalPD.iPlrMin), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsAnd), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), app.lpui->GlobalPD.iPlrMax), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    if (app.lpui->GlobalPD.iPlanMin > -1) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsCurrentlyPossessTechnology), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsModify), 0, fPrint);
        DxStreamTextOut(hdc, &x, y, app.state.rgszPlanetAttr[app.lpui->GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsOn2), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWithinRange), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), app.lpui->GlobalPD.iPlanMin), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsTo), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), app.lpui->GlobalPD.iPlanMax), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        y += app.state.dyArial8;
    }
    x = 4;
    if (app.lpui->GlobalPD.iPlanMin > -1 && app.lpui->GlobalPD.iPlrMin != envImmune && app.lpui->GlobalPD.iPlanVal != app.lpui->GlobalPD.iPlrVal) {
        iNewVal = envNone;
        if (app.lpui->GlobalPD.iPlanVal < app.lpui->GlobalPD.iPlrVal) {
            if (app.lpui->GlobalPD.iPlanMax > app.lpui->GlobalPD.iPlanVal) {
                iNewVal = app.lpui->GlobalPD.iPlrVal >= app.lpui->GlobalPD.iPlanMax ? app.lpui->GlobalPD.iPlanMax : app.lpui->GlobalPD.iPlrVal;
            }
        } else if (app.lpui->GlobalPD.iPlanMin < app.lpui->GlobalPD.iPlanVal) {
            iNewVal = app.lpui->GlobalPD.iPlrVal <= app.lpui->GlobalPD.iPlanMin ? app.lpui->GlobalPD.iPlanMin : app.lpui->GlobalPD.iPlrVal;
        }
        if (iNewVal != envNone) {
            lppl = LpplFromId(app, app.lpui->GlobalPD.idPlanet);
            pctDesireOld = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
            iValSav = lppl->rgEnvVar[app.lpui->GlobalPD.iPlanetVar];
            lppl->rgEnvVar[app.lpui->GlobalPD.iPlanetVar] = iNewVal;
            pctDesire = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
            lppl->rgEnvVar[app.lpui->GlobalPD.iPlanetVar] = iValSav;
            if (pctDesireOld < pctDesire) {
                cch = TextIO().pfnCchGetString(app, idsIfTerraform, app.state.szWork);
                WrapTextOut(app, hdc, &x, &y, app.state.szWork, cch, 4, xMax - 4, nullptr, 0, fPrint);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                WrapTextOut(app, hdc, &x, &y, app.state.rgszPlanetAttr[app.lpui->GlobalPD.iPlanetVar], 0, 4, xMax - 4, nullptr, 0, fPrint);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                WrapTextOut(app, hdc, &x, &y, TextIO().pfnCompressed(app, idsTo), 0, 4, xMax - 4, nullptr, 0, fPrint);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                WrapTextOut(app, hdc, &x, &y, PszCalcEnvVar(app, static_cast<EnvType>(app.lpui->GlobalPD.iPlanetVar), iNewVal), 0, 4, xMax - 4, nullptr, 0, fPrint);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                cch = TextIO().pfnCchGetString(app, idsPlanetsValueWouldImprove, app.state.szWork);
                WrapTextOut(app, hdc, &x, &y, app.state.szWork, cch, 4, xMax - 4, nullptr, 0, fPrint);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                cch = win32::wsprintfA(app.state.szWork, "%d%%.  ", pctDesire);
                WrapTextOut(app, hdc, &x, &y, app.state.szWork, cch, 4, xMax - 4, nullptr, 0, fPrint);
            }
        }
    }
    if (app.lpui->GlobalPD.iPlrMin != envImmune && app.lpui->GlobalPD.iPlanVal != app.lpui->GlobalPD.iPlrVal && app.lpui->GlobalPD.iPlanVal != envNone) {
        if (app.lpui->GlobalPD.iPlanVal < app.lpui->GlobalPD.iPlrMin) {
            dChg = app.lpui->GlobalPD.iPlrMin - app.lpui->GlobalPD.iPlanVal;
        } else if (app.lpui->GlobalPD.iPlanVal > app.lpui->GlobalPD.iPlrMax) {
            dChg = app.lpui->GlobalPD.iPlanVal - app.lpui->GlobalPD.iPlrMax;
        } else {
            dChg = -abs(app.lpui->GlobalPD.iPlanVal - app.lpui->GlobalPD.iPlrVal);
        }
        if (dChg < 0) {
            dChg = -dChg;
            ids = idsValueDAwayIdealValueRace;
        } else {
            ids = idsValueDOutsideHabitableRangeRace;
        }
        TextIO().pfnCchGetString(app, ids, app.state.szWork);
        cch = win32::wsprintfA(szOut, app.state.szWork, dChg);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        WrapTextOut(app, hdc, &x, &y, szOut, cch, 4, xMax - 4, nullptr, 0, fPrint);
    }
    if (x > 4) {
        y += app.state.dyArial8;
    }
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayPlanetPopInfo(APPSESSION &app, HDC hdc, int16_t fPrint) {
    PLANET  pl;
    char    szT[150];
    POINT16 pt;
    int16_t y;
    int16_t xMax;
    int16_t c;
    char   *psz;
    int32_t lMax;
    int16_t pctDesire;
    int16_t x;
    int32_t lPopChg;

    y = 4;
    xMax = 4;
    x = 4;
    PlanetQueryIO().pfnLookup(app, app.lpui->GlobalPD.idPlanet, &pl);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    if (pl.iPlayer == app.state.idPlayer) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsPopulation3), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsIs), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::wsprintfA(app.state.szWork, app.lpui->PCTLD00, pl.rgwtMin[3]);
        if (pl.rgwtMin[3] == 0) {
            app.state.szWork[1] = 0;
        }
        DxStreamTextOut(hdc, &x, y, app.state.szWork, 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else if (pl.iPlayer != iplrNone) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsEnemyPopulation), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        if (pl.det >= detSome) {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsApproximately), 0, fPrint);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            win32::wsprintfA(app.state.szWork, "%d00", pl.uPopGuess * 4);
            if (pl.uPopGuess == 0) {
                app.state.szWork[1] = 0;
            }
            DxStreamTextOut(hdc, &x, y, app.state.szWork, 0, fPrint);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        } else {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsUnknown), 0, fPrint);
        }
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsUninhabited2), 0, fPrint);
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    if (pl.det < detSome)
        goto AfterDesire;
    lMax = CalcPlanetMaxPop(app, app.lpui->GlobalPD.idPlanet, app.state.idPlayer);
    pctDesire = PlanetQueryIO().pfnDesirability(app, &pl, app.state.idPlayer);
    if (pctDesire < 0) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWillKillOffApproximately), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = win32::wsprintfA(app.state.szWork, app.state.PCTDXPCTDPCTPCT, (int16_t)-pctDesire / 10, (int16_t)-pctDesire % 10);
        DxStreamTextOut(hdc, &x, y, app.state.szWork, c, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsOf), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        if (pl.iPlayer == app.state.idPlayer) {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonistsEachTurn), 0, fPrint);
        } else {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonistsSettleEveryTurn), 0, fPrint);
        }
    } else if (pl.iPlayer == app.state.idPlayer && lMax > 0) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWillSupportPopulation), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = win32::wsprintfA(app.state.szWork, app.lpui->PCTLD00, lMax);
        DxStreamTextOut(hdc, &x, y, app.state.szWork, c, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonists3), 0, fPrint);
    } else if (lMax > 0) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsIfColonize), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWould), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += app.state.dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsSupport), 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = win32::wsprintfA(app.state.szWork, app.lpui->PCTLD00, lMax);
        DxStreamTextOut(hdc, &x, y, app.state.szWork, c, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsColonists3), 0, fPrint);
    } else {
        y -= app.state.dyArial8;
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    if (pl.iPlayer == app.state.idPlayer && pctDesire >= 0 && pl.rgwtMin[3] < lMax) {
        c = TextIO().pfnCchGetString(app, idsPopulation, szT);
        WrapTextOut(app, hdc, &x, &y, szT, c, 4, xMax, nullptr, 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        psz = TextIO().pfnPlanetName(app, pl.id);
        WrapTextOut(app, hdc, &x, &y, psz, 0, 4, xMax, nullptr, 0, fPrint);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        lPopChg = PlanetQueryIO().pfnGrowth(app, &pl, 0);
        if (pctDesire == 0 || lPopChg <= 0) {
            c = TextIO().pfnCchGetString(app, idsWillGrowYear, szT);
        } else {
            psz = TextIO().pfnCompressed(app, idsWillGrowLd00Ld00Year);
            c = win32::wsprintfA(szT, psz, lPopChg, pl.rgwtMin[3] + lPopChg);
        }
        WrapTextOut(app, hdc, &x, &y, szT, c, 4, xMax, nullptr, 0, fPrint);
        x = 4;
        y += app.state.dyArial8;
        goto AfterDesire;
    }
    if (pl.iPlayer == app.state.idPlayer || pl.iPlayer == iplrNone)
        goto AfterDesire;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    psz = TextIO().pfnPlanetName(app, pl.id);
    WrapTextOut(app, hdc, &x, &y, psz, 0, 4, xMax, nullptr, 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    if (pl.uDefGuess == 0) {
        c = TextIO().pfnCchGetString(app, idsAppearsHavePlanetaryDefenses, app.state.szWork);
    } else {
        psz = TextIO().pfnCompressed(app, idsHasPlanetaryDefensesApproximatelyDCoverage);
        c = win32::wsprintfA(szT, psz, pl.uDefGuess * 6 + 3);
        psz = szT;
    }
    WrapTextOut(app, hdc, &x, &y, psz, c, 4, xMax, nullptr, 0, fPrint);
    x = 4;
    y += app.state.dyArial8;
AfterDesire:
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayZipOrdInfo(APPSESSION &app, HDC hdc, int16_t xCtr, int16_t fPrint) {
    POINT16 pt;
    int16_t y;
    int16_t xMax;
    char   *psz;
    int16_t x;

    y = 4;
    xMax = 4;
    x = 4;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (fPrint) {
        psz = TextIO().pfnCompressed(app, idsZipord);
        CtrTextOut(hdc, xCtr, y, psz, strlen(psz));
    }
    y += app.state.dyArial8;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsZipordProvidesAbilityQuicklySetFleets), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsTransportOrdersOne3CommonSetsSelect), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsZipordClickDiamondRightMouse), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsButtonClickOrderChoice), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8 * 2;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsQuikload), 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsLoadMineralsAvailable), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (app.state.dyArial8 >> 1) + app.state.dyArial8;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsQuikdrop), 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsUnloadEverythingFleetCarrying), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (app.state.dyArial8 >> 1) + app.state.dyArial8;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWaitload), 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsWaitFullLoadMinerals), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (app.state.dyArial8 >> 1) + app.state.dyArial8;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsClear2), 0, fPrint);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(app, idsRemoveTransportOrders), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += app.state.dyArial8;
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayFactoryMineInfo(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint) {
    const char *pszTypes;
    char     szT[40];
    POINT16  pt;
    StringId ids;
    const char *pszType;
    int16_t  y;
    int16_t  xMax;
    int16_t  i;
    int16_t  c;
    char    *psz;
    int16_t  cnt;
    int16_t  x;

    ids = idsHave;
    dx -= 8;
    xMax = 4;
    x = 4;
    y = 2;
    if (app.lpui->GlobalPD.grbit != 0) {
        pszType = "Factory";
        pszTypes = "Factories";
    } else {
        pszType = "Mine";
        pszTypes = "Mines";
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (fPrint) {
        TextIO().pfnCchGetString(app, idsSInfo, szT);
        c = win32::wsprintfA(app.state.szWork, szT, pszType);
        CtrTextOut(hdc, dx >> 1, y, app.state.szWork, c);
    }
    y += app.state.dyArial8 + 4;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        psz = TextIO().pfnCompressed(app, app.lpui->GlobalPD.grbit == 0 ? idsRaceIncapableBuildingMinesHoweverColonistsHave : idsRaceIncapableBuildingFactories);
        WrapTextOut(app, hdc, &x, &y, psz, 0, 4, dx, &xMax, 0, fPrint);
        goto LDone;
    }
    for (i = 0; i <= 8; i++) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        switch (i) {
        case 0:
        case 2:
        case 4:
        case 6:
        case 8:
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            psz = TextIO().pfnCompressed(app, ids);
            ids = static_cast<StringId>(ids + 1);
            break;
        case 1:
            cnt = app.lpui->GlobalPD.cCur;
        SetQuan:
            win32::wsprintfA(app.state.szWork, "%d %s", cnt, cnt == 1 ? pszType : pszTypes);
            psz = app.state.szWork;
            break;
        case 3:
            psz = TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlan);
            break;
        case 5:
            cnt = app.lpui->GlobalPD.cMax;
            goto SetQuan;
        case 7:
            win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->GlobalPD.cOperate);
            psz = app.state.szWork;
            break;
        }
        WrapTextOut(app, hdc, &x, &y, psz, 0, 4, dx, &xMax, 0, fPrint);
    }
LDone:
    pt.x = xMax + 4;
    pt.y = y + app.state.dyArial8 + 2;
    return pt;
}

POINT16 PtDisplayResourceInfo(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint) {
    int16_t  iMax;
    POINT16  pt;
    StringId ids;
    int16_t  y;
    int16_t  xMax;
    int16_t  i;
    char    *psz;
    int16_t  cnt;
    int16_t  x;

    iMax = (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) + 8;
    ids = idsGenerates;
    dx -= 8;
    xMax = 4;
    x = 4;
    y = 2;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (fPrint) {
        psz = TextIO().pfnCompressed(app, idsResourceInfo);
        CtrTextOut(hdc, dx >> 1, y, psz, strlen(psz));
    }
    y += app.state.dyArial8 + 4;
    for (i = 0; i <= iMax; i++) {
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        switch (i) {
        case 6:
            if (app.lpui->GlobalPD.iPlanVal == app.lpui->GlobalPD.iPlanetVar)
                goto OutOfFor;
            /* fallthrough */
        case 1:
        case 3:
        case 5:
        case 8:
        case 9:
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            psz = TextIO().pfnCompressed(app, ids);
            ids = static_cast<StringId>(ids + 1);
            break;
        case 2:
            cnt = app.lpui->GlobalPD.iPlanetVar;
        SetQuan:
            win32::wsprintfA(app.state.szWork, app.state.PCTD, cnt);
            psz = app.state.szWork;
            break;
        case 0:
            psz = TextIO().pfnPlanetName(app, app.lpui->GlobalPD.idPlanet);
            break;
        case 4:
            cnt = app.lpui->GlobalPD.iPlanetVar - app.lpui->GlobalPD.iPlanVal;
            if (cnt > 0)
                goto SetQuan;
            psz = TextIO().pfnCompressed(app, idsNone2);
            break;
        case 7:
            cnt = app.lpui->GlobalPD.iPlanVal;
            goto SetQuan;
        }
        WrapTextOut(app, hdc, &x, &y, psz, 0, 4, dx, &xMax, 0, fPrint);
    }
OutOfFor:
    pt.x = xMax + 4;
    pt.y = y + app.state.dyArial8 + 2;
    return pt;
}

POINT16 PtDisplayString(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint) {
    POINT16 pt;
    int16_t y;
    int16_t xMax;
    int16_t x;

    dx -= 8;
    xMax = 4;
    x = 4;
    y = 2;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    WrapTextOut(app, hdc, &x, &y, app.lpui->GlobalPD.psz, 0, 4, dx, &xMax, 0, fPrint);
    pt.x = xMax + 4;
    pt.y = y + app.state.dyArial8 + 2;
    return pt;
}
