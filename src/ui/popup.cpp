#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

BattleUnitFlags mpimdgrbitBU[8] = {grBuClassUnarmed, grBuClassUnarmed, grBuClassScout,   grBuClassWarship,
                                   grBuClassUtility, grBuClassBomber,  grBuClassUnarmed, grBuClassUnarmed};

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;

    switch (message) {
    case WM_CREATE:
        return 0;
    case WM_DESTROY:
        // Native close paths do not pass through the mouse-release handler.
        if (hwndPopup == hwnd) {
            hwndPopup = 0;
            GlobalPD.grPopup = static_cast<GrPopupType>(0);
        }
        return 0;
    case WM_ERASEBKGND:
        switch (GlobalPD.grPopup) {
        case grPopupComponent:
        case grPopupShdef:
        case grPopupShdefSB:
            ControlIO().pfnGetClientRect(hwnd, &rc);
            win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
            return 1;
        default:
            goto Default;
        }
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawPopup(hwnd, hdc);
        win32::EndPaint(hwnd, &ps);
        return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        if (!hwndPopup) {
            return 0;
        }
        ControlIO().pfnDestroyWindow(hwndPopup);
        hwndPopup = 0;
        GlobalPD.grPopup = static_cast<GrPopupType>(0);
        win32::ReleaseCapture();
        if (!gd.fTutorial) {
            return 0;
        }
        tutor.fProgress = TRUE;
        PresentationIO().pfnTutor();
        return 0;
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
}

int16_t FIsPopupHullType(int16_t ishdef) {
    HullCategory imd;

    if (GlobalPD.grbit == 0 || GlobalPD.grbit == 0xff) {
        return TRUE;
    }
    imd = static_cast<HullCategory>(PartQueryIO().pfnHull(rglpshdef[GlobalPD.lpfl->iPlayer][ishdef].hul.ihuldef)->imdCategory);
    return mpimdgrbitBU[imd] & GlobalPD.grbit;
}

void DrawPopup(HWND hwnd, HDC hdc) {
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
    bkMode = win32::SetBkMode(hdc, OPAQUE);
    hfontSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[1]);
    ControlIO().pfnGetClientRect(hwnd, &rc);
    switch (GlobalPD.grPopup) {
    default:
        break;
    case grPopupMineral:
        CtrTextOut(hdc, rc.right >> 1, 4, rgszMinerals[GlobalPD.rgi[0]], 0);
        win32::SelectObject(hdc, rghfontArial8[0]);
        psz = TextIO().pfnCompressed(idsMineralConcentration);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
        RightTextOut(hdc, dx, dyArial8 + 4, TextIO().pfnCompressed(idsSurface), 0, 0);
        if (GlobalPD.rgi[2] >= 0) {
            c = win32::wsprintf(szWork, "%ldkT", GlobalPD.rgi[2]);
        } else {
            strcpy(szWork, TextIO().pfnCompressed(idsUnknown2));
            c = strlen(szWork);
        }
        win32::TextOut(hdc, dx, dyArial8 + 4, szWork, c);
        RightTextOut(hdc, dx, dyArial8 * 2 + 4, TextIO().pfnCompressed(idsMineralConcentration), 0, 0);
        if (GlobalPD.rgi[3] > 0) {
            c = win32::wsprintf(szWork, PCTLD, GlobalPD.rgi[3]);
            if (GlobalPD.rgi[1] != 0) {
                c += win32::wsprintf(&szWork[c], TextIO().pfnCompressed(GlobalPD.rgi[3] < 30 ? idsN30 : idsHw));
            }
        } else {
            c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsUnknown2));
        }
        win32::TextOut(hdc, dx, dyArial8 * 2 + 4, szWork, c);
        if (GlobalPD.rgi[4] < 0)
            break;
        RightTextOut(hdc, dx, 3 * dyArial8 + 4, TextIO().pfnCompressed(idsMiningRate), 0, 0);
        TextIO().pfnCchGetString(idsLdktYr, szT);
        c = win32::wsprintf(szWork, szT, GlobalPD.rgi[4]);
        win32::TextOut(hdc, dx, 3 * dyArial8 + 4, szWork, c);
        break;
    case grPopupPlayer:
        CtrTextOut(hdc, rc.right >> 1, 4, TextIO().pfnPlayerName(GlobalPD.iPlayer, TRUE, TRUE, TRUE, 0, NULL), 0);
        c = win32::wsprintf(szWork, TextIO().pfnCompressed(idsPlayerD), GlobalPD.iPlayer + 1);
        CtrTextOut(hdc, rc.right >> 1, dyArial8 + 4, szWork, c);
        break;
    case grPopupFleet:
        if (rc.bottom - rc.top < dyArial8 * 2) {
            c = TextIO().pfnCchGetString(idsNone2, szWork);
            win32::TextOut(hdc, 4, 4, szWork, c);
            win32::SelectObject(hdc, rghfontArial8[0]);
            break;
        }
        win32::SelectObject(hdc, rghfontArial8[1]);
        c = TextIO().pfnCchGetString(idsShipName, szWork);
        win32::TextOut(hdc, 4, 4, szWork, c);
        RightTextOut(hdc, rc.right - 4 - GlobalPD.dxDamage, 4, "#", 1, 0);
        if (GlobalPD.dxDamage != 0) {
            c = TextIO().pfnCchGetString(idsDamage2, szWork);
            RightTextOut(hdc, rc.right - 4, 4, szWork, c, 0);
            win32::PatBlt(hdc, 4, dyArial8 + 2, rc.right - 8, 1, BLACKNESS);
        }
        win32::SelectObject(hdc, rghfontArial8[0]);
        yCur = dyArial8 + 4;
        for (i = 0; i < 16; i++) {
            if (GlobalPD.lpfl->rgcsh[i] > 0 && (GlobalPD.grbit == 0 || FIsPopupHullType(i))) {
                if (GlobalPD.fRedDamage) {
                    if (GlobalPD.lpfl->rgdv[i].dp != 0) {
                        win32::SetTextColor(hdc, 0xff);
                    } else {
                        win32::SetTextColor(hdc, 0);
                    }
                }
                DecorateHullName(GlobalPD.lpfl->iplr(), i, szTB);
                lpsz = szTB;
                win32::TextOut(hdc, 4, yCur, lpsz, strlen(lpsz));
                c = win32::wsprintf(szWork, PCTD, GlobalPD.lpfl->rgcsh[i]);
                RightTextOut(hdc, rc.right - 4 - GlobalPD.dxDamage, yCur, szWork, c, 0);
                if (GlobalPD.fRedDamage && GlobalPD.lpfl->rgdv[i].dp != 0) {
                    csh = GlobalPD.lpfl->rgcsh[i];
                    csh = LOWORD((int32_t)((uint32_t)(GlobalPD.lpfl->rgdv[i].dp & 0x7f) * csh) / 0x64);
                    if (csh <= 0) {
                        csh = 1;
                    }
                    dpT = (uint32_t)(GlobalPD.lpfl->rgdv[i].dp >> 7 & 0x1ff) / 5;
                    if (dpT == 0) {
                        dpT = 1;
                    }
                    c = win32::wsprintf(szWork, "%d@%d%%", csh, dpT);
                    RightTextOut(hdc, rc.right - 4, yCur, szWork, c, 0);
                }
                yCur += dyArial8;
            }
        }
        if (!GlobalPD.fRedDamage)
            break;
        win32::SetTextColor(hdc, 0);
        break;
    case grPopupUnknownObj:
        psz = TextIO().pfnCompressed(idsPlanet);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
        win32::SelectObject(hdc, rghfontArial8[1]);
        RightTextOut(hdc, dx, 4, psz, strlen(psz), 0);
        RightTextOut(hdc, dx, dyArial8 + 4, TextIO().pfnCompressed(idsId), 0, 0);
        RightTextOut(hdc, dx, dyArial8 * 2 + 4, TextIO().pfnCompressed(idsX), 0, 0);
        RightTextOut(hdc, dx, 3 * dyArial8 + 4, TextIO().pfnCompressed(idsY), 0, 0);
        psz = TextIO().pfnPlanetName(sel.scan.idpl);
        win32::SelectObject(hdc, rghfontArial8[0]);
        win32::TextOut(hdc, dx, 4, psz, strlen(psz));
        c = win32::wsprintf(szWork, PCTD, sel.scan.idpl + 1);
        win32::TextOut(hdc, dx, dyArial8 + 4, szWork, c);
        c = win32::wsprintf(szWork, PCTD, sel.scan.pt.x);
        win32::TextOut(hdc, dx, dyArial8 * 2 + 4, szWork, c);
        c = win32::wsprintf(szWork, PCTD, sel.scan.pt.y);
        win32::TextOut(hdc, dx, 3 * dyArial8 + 4, szWork, c);
        break;
    case grPopupPlanetEnv:
        PtDisplayPlanetStateInfo(hdc, TRUE);
        break;
    case grPopupShipOrders:
        PtDisplayZipOrdInfo(hdc, rc.right >> 1, TRUE);
        break;
    case grPopupPlanet:
        PtDisplayPlanetPopInfo(hdc, TRUE);
        break;
    case grPopupPlanetIndustry:
        PtDisplayFactoryMineInfo(hdc, rc.right, TRUE);
        break;
    case grPopupResources:
        PtDisplayResourceInfo(hdc, rc.right, TRUE);
        break;
    case grPopupComponent:
        DisplayComponentInfo(hdc, rc.right, rc.bottom, &GlobalPD.part);
        break;
    case grPopupString:
        PtDisplayString(hdc, rc.right, TRUE);
        break;
    case grPopupShdef:
    case grPopupShdefSB:
        fStarbaseMode = (int16_t)GlobalPD.lpshdef->hul.ihuldef >= ihuldefOrbitalFort;
        DrawSlotDlg(hwnd, hdc, &rc, -1);
        rc.top = dyArial8 + 306;
        rc.left += 6;
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkMode(hdc, TRANSPARENT);
        strcpy(szWork, GlobalPD.lpshdef->hul.szClass);
        CtrTextOut(hdc, ((rc.right - 0x4c) >> 1) + 0x4c, 6, szWork, 0);
        DrawBuildSelHull(hwnd, hdc, -1, &rc);
    }
    win32::SelectObject(hdc, hfontSav);
    win32::SetBkMode(hdc, bkMode);
    win32::SetTextColor(hdc, crFore);
    win32::SetBkColor(hdc, crBack);
    return;
}

void Popup(HWND hwnd, int16_t x, int16_t y) {
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
    hfontSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[0]);
    switch (GlobalPD.grPopup) {
    default:
        break;
    case grPopupMineral:
        psz = TextIO().pfnCompressed(idsMineralConcentration0000000kt);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        dy = 3 * dyArial8 + 8;
        if (GlobalPD.rgi[4] < 0)
            break;
        dy += dyArial8;
        break;
    case grPopupPlayer:
        win32::SelectObject(hdc, rghfontArial8[1]);
        psz = TextIO().pfnPlayerName(GlobalPD.iPlayer, TRUE, TRUE, TRUE, 0, NULL);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        dx2 = LOWORD(GetTextExtent(hdc, "Player #16", 10)) + 8;
        if (dx2 > dx) {
            dx = dx2;
        }
        dy = dyArial8 * 2 + 8;
        break;
    case grPopupFleet:
        dxR = 0;
        dxDamage = 0;
        dy = dyArial8 + 8;
        win32::SelectObject(hdc, rghfontArial8[1]);
        psz = TextIO().pfnCompressed(idsShipName);
        dxL = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        win32::SelectObject(hdc, rghfontArial8[0]);
        for (i = 0; i < 16; i++) {
            if (GlobalPD.lpfl->rgcsh[i] > 0 && (GlobalPD.grbit == 0 || FIsPopupHullType(i))) {
                dy += dyArial8;
                DecorateHullName(GlobalPD.lpfl->iplr(), i, szTB);
                lpsz = szTB;
                dx = LOWORD(GetTextExtent(hdc, lpsz, strlen(lpsz)));
                dxL = dxL <= dx ? dx : dxL;
                c = win32::wsprintf(szWork, PCTD, GlobalPD.lpfl->rgcsh[i]);
                dx = LOWORD(GetTextExtent(hdc, szWork, c));
                dxR = dxR <= dx ? dx : dxR;
                if (GlobalPD.fRedDamage && (GlobalPD.lpfl->rgdv[i].dp >> 7 & 0x1ff) && dxDamage == 0) {
                    psz = TextIO().pfnCompressed(idsN9999999);
                    dxDamage = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 4;
                }
            }
        }
        if (dy == dyArial8 + 8) {
            psz = TextIO().pfnCompressed(idsShipName);
            dxL = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        }
        GlobalPD.dxDamage = dxDamage;
        dx = dxL + dxR + 16 + dxDamage;
        break;
    case grPopupUnknownObj:
        win32::SelectObject(hdc, rghfontArial8[1]);
        dy = dyArial8 * 4 + 8;
        psz = TextIO().pfnCompressed(idsPlanet);
        dx = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        psz = TextIO().pfnPlanetName(sel.scan.idpl);
        win32::SelectObject(hdc, rghfontArial8[0]);
        dxName = LOWORD(GetTextExtent(hdc, psz, strlen(psz)));
        dxCoord = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(idsN9999), 4));
        dx += dxName <= dxCoord ? dxCoord : dxName;
        break;
    case grPopupPlanetEnv:
        ptT = PtDisplayPlanetStateInfo(hdc, FALSE);
        goto SetDxDy;
    case grPopupShipOrders:
        ptT = PtDisplayZipOrdInfo(hdc, 0, FALSE);
        goto SetDxDy;
    case grPopupPlanet:
        ptT = PtDisplayPlanetPopInfo(hdc, FALSE);
        goto SetDxDy;
    case grPopupResources:
        ptT = PtDisplayResourceInfo(hdc, 200, FALSE);
        goto SetDxDy;
    case grPopupPlanetIndustry:
        ptT = PtDisplayFactoryMineInfo(hdc, 200, FALSE);
    SetDxDy:
        dx = ptT.x + 2;
        dy = ptT.y + 2;
        break;
    case grPopupComponent:
        dx = (dyArial8 <= 14 ? 0 : 40) + 344;
        dy = dyArial10 + 72 + 12 * dyArial8 + 6;
        break;
    case grPopupString:
        ptT = PtDisplayString(hdc, GlobalPD.dxOut, FALSE);
        goto SetDxDy;
    case grPopupShdef:
    case grPopupShdefSB:
        mdBuild = GlobalPD.grPopup == grPopupShdef ? mdBuildShdef : mdBuildHuldef;
        lpshdefBuild = GlobalPD.lpshdef;
        UpdateSlotGlobals();
        dx = 340;
        dy = dyArial8 + 306 + 6 * dyArial8 + 8;
        if (gd.mdScreenSize <= 0 || GlobalPD.grPopup != grPopupShdef)
            break;
        dy += 3 * dyArial8;
        break;
    case grPopupUnknown:
        dx = 120;
        dy = 80;
    }
    win32::SelectObject(hdc, hfontSav);
    win32::ReleaseDC(hwnd, hdc);
    pt.x -= dx;
    pt.y -= dy;
    pt.x = (((0) > ((((pt.x) < (win32::GetSystemMetrics(SM_CXSCREEN) - dx)) ? (pt.x) : (win32::GetSystemMetrics(SM_CXSCREEN) - dx)))) ? (0) : ((((pt.x) < (win32::GetSystemMetrics(SM_CXSCREEN) - dx)) ? (pt.x) : (win32::GetSystemMetrics(SM_CXSCREEN) - dx))));
    pt.y = (((0) > ((((pt.y) < (win32::GetSystemMetrics(SM_CYSCREEN) - dy)) ? (pt.y) : (win32::GetSystemMetrics(SM_CYSCREEN) - dy)))) ? (0) : ((((pt.y) < (win32::GetSystemMetrics(SM_CYSCREEN) - dy)) ? (pt.y) : (win32::GetSystemMetrics(SM_CYSCREEN) - dy))));
    // Popup drawing reads hwndPopup, so publish the handle before the first paint.
    hwndPopup = ui::CreateWindowEx(WS_EX_NOACTIVATE, szPopup, NULL, WS_POPUP | WS_BORDER,
                               pt.x, pt.y, dx, dy, hwnd, NULL, hInst, NULL);
    ControlIO().pfnSendMessage(hwndPopup, WM_SETFONT, (WPARAM)rghfontArial8[0], 0);
    ui::ShowWindow(hwndPopup, SW_SHOW);
    win32::SetCapture(hwndPopup);
    return;
}

int16_t PopupMenu(HWND hwnd, int16_t x, int16_t y, int16_t cString, int32_t *rgids, const char *const *rgsz, int16_t iChecked, int16_t fRightBtn) {
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
    iPopMenuSel = -1;
    for (i = 0; i < cString; i++) {
        if (rgids && (iChecked != -2 || !rgsz)) {
            if (rgids[i] == -1) {
                win32::AppendMenu(hmenuPopup, MF_SEPARATOR, 0, NULL);
            } else {
                if (rgids[i] & 0x10000000) {
                    psz = "Deep Space";
                } else if (rgids[i] & 0x40000000) {
                    psz = TextIO().pfnCompressed(static_cast<StringId>(LOWORD(rgids[i])));
                } else if (rgids[i] & 0x20000000) {
                    psz = PszGetThingName(LOWORD(rgids[i]));
                } else if (rgids[i] & 0x80000000) {
                    psz = PszGetFleetName(LOWORD(rgids[i]) | 0x8000);
                } else {
                    psz = TextIO().pfnPlanetName(LOWORD(rgids[i]));
                }
                pszT = szTemp;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                win32::AppendMenu(hmenuPopup, i == iChecked ? MF_CHECKED : MF_BYCOMMAND, i + 15000, szTemp);
            }
        } else if (!rgsz[i]) {
            pszTitle = rgsz[i + 1];
            fChecked = !rgids ? FALSE : LOWORD(rgids[i + 1]);
            hmenuSub = win32::CreatePopupMenu();
            for (i += 2; i < cString && rgsz[i]; i++) {
                if (!rgids) {
                    fCheckedCur = i == iChecked;
                    fChecked |= fCheckedCur;
                } else {
                    fCheckedCur = LOWORD(rgids[i]);
                }
                if (*rgsz[i] == -1 && rgsz[i][1] == 0) {
                    win32::AppendMenu(hmenuSub, MF_SEPARATOR, 0, NULL);
                } else {
                    pszT = szTemp;
                    psz = rgsz[i];
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    win32::AppendMenu(hmenuSub, !fCheckedCur ? MF_BYCOMMAND : MF_CHECKED, i + 15000, szTemp);
                }
            }
            win32::AppendMenu(hmenuPopup, (!fChecked ? 0 : 8) | 0x10, (UINT_PTR)hmenuSub, pszTitle);
        } else if (*rgsz[i] == -1 && rgsz[i][1] == 0) {
            win32::AppendMenu(hmenuPopup, MF_SEPARATOR, 0, NULL);
        } else {
            pszT = szTemp;
            psz = rgsz[i];
            while (*psz != 0) {
                if ((*pszT++ = *psz++) == '&') {
                    *pszT++ = '&';
                }
            }
            *pszT = 0;
            win32::AppendMenu(hmenuPopup, (iChecked == -2 ? rgids[i] : i == iChecked) == 0 ? MF_BYCOMMAND : MF_CHECKED, i + 15000, szTemp);
        }
    }
    if (fRightBtn) {
        tpm = TPM_RIGHTBUTTON;
    } else {
        tpm = TPM_LEFTBUTTON;
    }
    ui::TrackPopupMenu(hmenuPopup, tpm, pt.x, pt.y, 0, hwndFrame, NULL);
    win32::DestroyMenu(hmenuPopup);
    if (hmenuSub) {
        win32::DestroyMenu(hmenuSub);
    }
    /* The original accepted only the first 100 items; the target popups now
       list every object at a location. */
    if (ui::PeekMessage(&msg, hwndFrame, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15000 + (WPARAM)cString) {
        iPopMenuSel = msg.wParam - 15000;
    }
    return iPopMenuSel;
}

POINT16 PtDisplayPlanetStateInfo(HDC hdc, int16_t fPrint) {
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
    win32::SelectObject(hdc, rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, rgszPlanetAttr[GlobalPD.iPlanetVar], 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsCurrently), 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[1]);
    if (GlobalPD.iPlanVal >= 0) {
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), GlobalPD.iPlanVal), 0, fPrint);
    } else {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsUnknown2), 0, fPrint);
    }
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    if (GlobalPD.iPlrMin == envImmune) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonistsImmune), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsEffects), 0, fPrint);
        DxStreamTextOut(hdc, &x, y, rgszPlanetAttr[GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonistsPreferPlanetsWhere), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        DxStreamTextOut(hdc, &x, y, rgszPlanetAttr[GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsBetween), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), GlobalPD.iPlrMin), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsAnd), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), GlobalPD.iPlrMax), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    if (GlobalPD.iPlanMin > -1) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsCurrentlyPossessTechnology), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsModify), 0, fPrint);
        DxStreamTextOut(hdc, &x, y, rgszPlanetAttr[GlobalPD.iPlanetVar], 0, fPrint);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsOn2), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWithinRange), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), GlobalPD.iPlanMin), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsTo), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), GlobalPD.iPlanMax), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        y += dyArial8;
    }
    x = 4;
    if (GlobalPD.iPlanMin > -1 && GlobalPD.iPlrMin != envImmune && GlobalPD.iPlanVal != GlobalPD.iPlrVal) {
        iNewVal = envNone;
        if (GlobalPD.iPlanVal < GlobalPD.iPlrVal) {
            if (GlobalPD.iPlanMax > GlobalPD.iPlanVal) {
                iNewVal = GlobalPD.iPlrVal >= GlobalPD.iPlanMax ? GlobalPD.iPlanMax : GlobalPD.iPlrVal;
            }
        } else if (GlobalPD.iPlanMin < GlobalPD.iPlanVal) {
            iNewVal = GlobalPD.iPlrVal <= GlobalPD.iPlanMin ? GlobalPD.iPlanMin : GlobalPD.iPlrVal;
        }
        if (iNewVal != envNone) {
            lppl = LpplFromId(GlobalPD.idPlanet);
            pctDesireOld = PlanetQueryIO().pfnDesirability(lppl, idPlayer);
            iValSav = lppl->rgEnvVar[GlobalPD.iPlanetVar];
            lppl->rgEnvVar[GlobalPD.iPlanetVar] = iNewVal;
            pctDesire = PlanetQueryIO().pfnDesirability(lppl, idPlayer);
            lppl->rgEnvVar[GlobalPD.iPlanetVar] = iValSav;
            if (pctDesireOld < pctDesire) {
                cch = TextIO().pfnCchGetString(idsIfTerraform, szWork);
                WrapTextOut(hdc, &x, &y, szWork, cch, 4, xMax - 4, NULL, FALSE, fPrint);
                win32::SelectObject(hdc, rghfontArial8[1]);
                WrapTextOut(hdc, &x, &y, rgszPlanetAttr[GlobalPD.iPlanetVar], 0, 4, xMax - 4, NULL, FALSE, fPrint);
                win32::SelectObject(hdc, rghfontArial8[0]);
                WrapTextOut(hdc, &x, &y, TextIO().pfnCompressed(idsTo), 0, 4, xMax - 4, NULL, FALSE, fPrint);
                win32::SelectObject(hdc, rghfontArial8[1]);
                WrapTextOut(hdc, &x, &y, PszCalcEnvVar(static_cast<EnvType>(GlobalPD.iPlanetVar), iNewVal), 0, 4, xMax - 4, NULL, FALSE, fPrint);
                win32::SelectObject(hdc, rghfontArial8[0]);
                cch = TextIO().pfnCchGetString(idsPlanetsValueWouldImprove, szWork);
                WrapTextOut(hdc, &x, &y, szWork, cch, 4, xMax - 4, NULL, FALSE, fPrint);
                win32::SelectObject(hdc, rghfontArial8[1]);
                cch = win32::wsprintf(szWork, "%d%%.  ", pctDesire);
                WrapTextOut(hdc, &x, &y, szWork, cch, 4, xMax - 4, NULL, FALSE, fPrint);
            }
        }
    }
    if (GlobalPD.iPlrMin != envImmune && GlobalPD.iPlanVal != GlobalPD.iPlrVal && GlobalPD.iPlanVal != envNone) {
        if (GlobalPD.iPlanVal < GlobalPD.iPlrMin) {
            dChg = GlobalPD.iPlrMin - GlobalPD.iPlanVal;
        } else if (GlobalPD.iPlanVal > GlobalPD.iPlrMax) {
            dChg = GlobalPD.iPlanVal - GlobalPD.iPlrMax;
        } else {
            dChg = -abs(GlobalPD.iPlanVal - GlobalPD.iPlrVal);
        }
        if (dChg < 0) {
            dChg = -dChg;
            ids = idsValueDAwayIdealValueRace;
        } else {
            ids = idsValueDOutsideHabitableRangeRace;
        }
        TextIO().pfnCchGetString(ids, szWork);
        cch = win32::wsprintf(szOut, szWork, dChg);
        win32::SelectObject(hdc, rghfontArial8[0]);
        WrapTextOut(hdc, &x, &y, szOut, cch, 4, xMax - 4, NULL, FALSE, fPrint);
    }
    if (x > 4) {
        y += dyArial8;
    }
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayPlanetPopInfo(HDC hdc, int16_t fPrint) {
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
    PlanetQueryIO().pfnLookup(GlobalPD.idPlanet, &pl);
    win32::SelectObject(hdc, rghfontArial8[0]);
    if (pl.iPlayer == idPlayer) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsPopulation3), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsIs), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::wsprintf(szWork, PCTLD00, pl.rgwtMin[3]);
        if (pl.rgwtMin[3] == 0) {
            szWork[1] = 0;
        }
        DxStreamTextOut(hdc, &x, y, szWork, 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else if (pl.iPlayer != iplrNone) {
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsEnemyPopulation), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        if (pl.det >= detSome) {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsApproximately), 0, fPrint);
            win32::SelectObject(hdc, rghfontArial8[1]);
            win32::wsprintf(szWork, "%d00", pl.uPopGuess * 4);
            if (pl.uPopGuess == 0) {
                szWork[1] = 0;
            }
            DxStreamTextOut(hdc, &x, y, szWork, 0, fPrint);
            win32::SelectObject(hdc, rghfontArial8[0]);
        } else {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsUnknown), 0, fPrint);
        }
        DxStreamTextOut(hdc, &x, y, ".", 1, fPrint);
    } else {
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsUninhabited2), 0, fPrint);
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    if (pl.det < detSome)
        goto AfterDesire;
    lMax = CalcPlanetMaxPop(GlobalPD.idPlanet, idPlayer);
    pctDesire = PlanetQueryIO().pfnDesirability(&pl, idPlayer);
    if (pctDesire < 0) {
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWillKillOffApproximately), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        c = win32::wsprintf(szWork, PCTDXPCTDPCTPCT, (int16_t)-pctDesire / 10, (int16_t)-pctDesire % 10);
        DxStreamTextOut(hdc, &x, y, szWork, c, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsOf), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        if (pl.iPlayer == idPlayer) {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonistsEachTurn), 0, fPrint);
        } else {
            DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonistsSettleEveryTurn), 0, fPrint);
        }
    } else if (pl.iPlayer == idPlayer && lMax > 0) {
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWillSupportPopulation), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        win32::SelectObject(hdc, rghfontArial8[1]);
        c = win32::wsprintf(szWork, PCTLD00, lMax);
        DxStreamTextOut(hdc, &x, y, szWork, c, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonists3), 0, fPrint);
    } else if (lMax > 0) {
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsIfColonize), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnPlanetName(GlobalPD.idPlanet), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWould), 0, fPrint);
        if (x > xMax) {
            xMax = x;
        }
        x = 4;
        y += dyArial8;
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsSupport), 0, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        c = win32::wsprintf(szWork, PCTLD00, lMax);
        DxStreamTextOut(hdc, &x, y, szWork, c, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsColonists3), 0, fPrint);
    } else {
        y -= dyArial8;
    }
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    if (pl.iPlayer == idPlayer && pctDesire >= 0 && pl.rgwtMin[3] < lMax) {
        c = TextIO().pfnCchGetString(idsPopulation, szT);
        WrapTextOut(hdc, &x, &y, szT, c, 4, xMax, NULL, FALSE, fPrint);
        win32::SelectObject(hdc, rghfontArial8[1]);
        psz = TextIO().pfnPlanetName(pl.id);
        WrapTextOut(hdc, &x, &y, psz, 0, 4, xMax, NULL, FALSE, fPrint);
        win32::SelectObject(hdc, rghfontArial8[0]);
        lPopChg = PlanetQueryIO().pfnGrowth(&pl, FALSE);
        if (pctDesire == 0 || lPopChg <= 0) {
            c = TextIO().pfnCchGetString(idsWillGrowYear, szT);
        } else {
            psz = TextIO().pfnCompressed(idsWillGrowLd00Ld00Year);
            c = win32::wsprintf(szT, psz, lPopChg, pl.rgwtMin[3] + lPopChg);
        }
        WrapTextOut(hdc, &x, &y, szT, c, 4, xMax, NULL, FALSE, fPrint);
        x = 4;
        y += dyArial8;
        goto AfterDesire;
    }
    if (pl.iPlayer == idPlayer || pl.iPlayer == iplrNone)
        goto AfterDesire;
    win32::SelectObject(hdc, rghfontArial8[1]);
    psz = TextIO().pfnPlanetName(pl.id);
    WrapTextOut(hdc, &x, &y, psz, 0, 4, xMax, NULL, FALSE, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    if (pl.uDefGuess == 0) {
        c = TextIO().pfnCchGetString(idsAppearsHavePlanetaryDefenses, szWork);
    } else {
        psz = TextIO().pfnCompressed(idsHasPlanetaryDefensesApproximatelyDCoverage);
        c = win32::wsprintf(szT, psz, pl.uDefGuess * 6 + 3);
        psz = szT;
    }
    WrapTextOut(hdc, &x, &y, psz, c, 4, xMax, NULL, FALSE, fPrint);
    x = 4;
    y += dyArial8;
AfterDesire:
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayZipOrdInfo(HDC hdc, int16_t xCtr, int16_t fPrint) {
    POINT16 pt;
    int16_t y;
    int16_t xMax;
    char   *psz;
    int16_t x;

    y = 4;
    xMax = 4;
    x = 4;
    win32::SelectObject(hdc, rghfontArial8[1]);
    if (fPrint) {
        psz = TextIO().pfnCompressed(idsZipord);
        CtrTextOut(hdc, xCtr, y, psz, strlen(psz));
    }
    y += dyArial8;
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsZipordProvidesAbilityQuicklySetFleets), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsTransportOrdersOne3CommonSetsSelect), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsZipordClickDiamondRightMouse), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsButtonClickOrderChoice), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8 * 2;
    win32::SelectObject(hdc, rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsQuikload), 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsLoadMineralsAvailable), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (dyArial8 >> 1) + dyArial8;
    win32::SelectObject(hdc, rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsQuikdrop), 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsUnloadEverythingFleetCarrying), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (dyArial8 >> 1) + dyArial8;
    win32::SelectObject(hdc, rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWaitload), 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsWaitFullLoadMinerals), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += (dyArial8 >> 1) + dyArial8;
    win32::SelectObject(hdc, rghfontArial8[1]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsClear2), 0, fPrint);
    win32::SelectObject(hdc, rghfontArial8[0]);
    DxStreamTextOut(hdc, &x, y, TextIO().pfnCompressed(idsRemoveTransportOrders), 0, fPrint);
    if (x > xMax) {
        xMax = x;
    }
    x = 4;
    y += dyArial8;
    pt.x = xMax + 4;
    pt.y = y + 4;
    return pt;
}

POINT16 PtDisplayFactoryMineInfo(HDC hdc, int16_t dx, int16_t fPrint) {
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
    if (GlobalPD.grbit != 0) {
        pszType = "Factory";
        pszTypes = "Factories";
    } else {
        pszType = "Mine";
        pszTypes = "Mines";
    }
    win32::SelectObject(hdc, rghfontArial8[1]);
    if (fPrint) {
        TextIO().pfnCchGetString(idsSInfo, szT);
        c = win32::wsprintf(szWork, szT, pszType);
        CtrTextOut(hdc, dx >> 1, y, szWork, c);
    }
    y += dyArial8 + 4;
    if (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
        win32::SelectObject(hdc, rghfontArial8[0]);
        psz = TextIO().pfnCompressed(GlobalPD.grbit == 0 ? idsRaceIncapableBuildingMinesHoweverColonistsHave : idsRaceIncapableBuildingFactories);
        WrapTextOut(hdc, &x, &y, psz, 0, 4, dx, &xMax, FALSE, fPrint);
        goto LDone;
    }
    for (i = 0; i <= 8; i++) {
        win32::SelectObject(hdc, rghfontArial8[1]);
        switch (i) {
        case 0:
        case 2:
        case 4:
        case 6:
        case 8:
            win32::SelectObject(hdc, rghfontArial8[0]);
            psz = TextIO().pfnCompressed(ids);
            ids = static_cast<StringId>(ids + 1);
            break;
        case 1:
            cnt = GlobalPD.cCur;
        SetQuan:
            win32::wsprintf(szWork, "%d %s", cnt, cnt == 1 ? pszType : pszTypes);
            psz = szWork;
            break;
        case 3:
            psz = TextIO().pfnPlanetName(GlobalPD.idPlan);
            break;
        case 5:
            cnt = GlobalPD.cMax;
            goto SetQuan;
        case 7:
            win32::wsprintf(szWork, PCTD, GlobalPD.cOperate);
            psz = szWork;
            break;
        }
        WrapTextOut(hdc, &x, &y, psz, 0, 4, dx, &xMax, FALSE, fPrint);
    }
LDone:
    pt.x = xMax + 4;
    pt.y = y + dyArial8 + 2;
    return pt;
}

POINT16 PtDisplayResourceInfo(HDC hdc, int16_t dx, int16_t fPrint) {
    int16_t  iMax;
    POINT16  pt;
    StringId ids;
    int16_t  y;
    int16_t  xMax;
    int16_t  i;
    char    *psz;
    int16_t  cnt;
    int16_t  x;

    iMax = (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) + 8;
    ids = idsGenerates;
    dx -= 8;
    xMax = 4;
    x = 4;
    y = 2;
    win32::SelectObject(hdc, rghfontArial8[1]);
    if (fPrint) {
        psz = TextIO().pfnCompressed(idsResourceInfo);
        CtrTextOut(hdc, dx >> 1, y, psz, strlen(psz));
    }
    y += dyArial8 + 4;
    for (i = 0; i <= iMax; i++) {
        win32::SelectObject(hdc, rghfontArial8[1]);
        switch (i) {
        case 6:
            if (GlobalPD.iPlanVal == GlobalPD.iPlanetVar)
                goto OutOfFor;
            /* fallthrough */
        case 1:
        case 3:
        case 5:
        case 8:
        case 9:
            win32::SelectObject(hdc, rghfontArial8[0]);
            psz = TextIO().pfnCompressed(ids);
            ids = static_cast<StringId>(ids + 1);
            break;
        case 2:
            cnt = GlobalPD.iPlanetVar;
        SetQuan:
            win32::wsprintf(szWork, PCTD, cnt);
            psz = szWork;
            break;
        case 0:
            psz = TextIO().pfnPlanetName(GlobalPD.idPlanet);
            break;
        case 4:
            cnt = GlobalPD.iPlanetVar - GlobalPD.iPlanVal;
            if (cnt > 0)
                goto SetQuan;
            psz = TextIO().pfnCompressed(idsNone2);
            break;
        case 7:
            cnt = GlobalPD.iPlanVal;
            goto SetQuan;
        }
        WrapTextOut(hdc, &x, &y, psz, 0, 4, dx, &xMax, FALSE, fPrint);
    }
OutOfFor:
    pt.x = xMax + 4;
    pt.y = y + dyArial8 + 2;
    return pt;
}

POINT16 PtDisplayString(HDC hdc, int16_t dx, int16_t fPrint) {
    POINT16 pt;
    int16_t y;
    int16_t xMax;
    int16_t x;

    dx -= 8;
    xMax = 4;
    x = 4;
    y = 2;
    win32::SelectObject(hdc, rghfontArial8[0]);
    WrapTextOut(hdc, &x, &y, GlobalPD.psz, 0, 4, dx, &xMax, FALSE, fPrint);
    pt.x = xMax + 4;
    pt.y = y + dyArial8 + 2;
    return pt;
}
