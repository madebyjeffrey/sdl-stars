#include "win.h"

char    vrgTBBtn[29] = {tbNormalView,
                        tbSurfaceMineralView,
                        tbMineralConcView,
                        tbPlanetValueView,
                        tbPopulationView,
                        tbNoPlayerInfoView,
                        tbSeparator,
                        tbAddWaypoints,
                        tbSeparator,
                        tbScannerCoverage,
                        tbSpacer,
                        tbScannerRange,
                        tbSeparator,
                        tbMineFields,
                        tbSeparator,
                        tbFleetPaths,
                        tbSeparator,
                        tbPlanetNames,
                        tbShipCounts,
                        tbSeparator,
                        tbIdleFleets,
                        tbSeparator,
                        tbShipDesignFilter,
                        tbShipDesignFilterMenu,
                        tbSeparator,
                        tbEnemyClassFilter,
                        tbEnemyClassFilterMenu,
                        tbSeparator,
                        tbZoomMenu};
int16_t vrgpctZoom[9] = {25, 38, 50, 75, 100, 125, 150, 200, 400};

static void SyncToolbar(HWND hwnd) {
    HWND hwndButtons = GetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    int16_t itb;

    for (itb = tbNormalView; itb <= tbShipCounts; itb++) {
        SendMessage(hwndButtons, TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + itb, FIsButtonDown(static_cast<ToolbarButton>(itb)));
    }
}

LRESULT CALLBACK TbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND          hwndButtons;
    HWND          hwndCE;
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    TBBUTTON      rgbtn[29] = {0};
    TBADDBITMAP   add;
    BITMAPINFO   *lpbi;
    HBITMAP       hbmp, hbmpSav;
    HDC           hdcMem;
    NMTOOLBAR    *lpnm;
    ToolbarButton itb;
    int16_t       i, j, c, pct, iSel;
    int           iRadar;
    RECT          rc;
    PAINTSTRUCT   ps;
    HDC           hdc;
    char          rgszTip[18][256];

    hwndButtons = GetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    switch (msg) {
    case WM_CREATE:
        if (!InitCommonControlsEx(&icc)) return -1;
        hwndButtons = CreateWindow(TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER,
            0, 2, 10, 32, hwnd, (HMENU)(INT_PTR)IDC_STARS_TOOLBAR, hInst, NULL);
        if (!hwndButtons) return -1;
        SendMessage(hwndButtons, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(hwndButtons, TB_SETBITMAPSIZE, 0, MAKELPARAM(24, 23));
        SendMessage(hwndButtons, TB_SETBUTTONSIZE, 0, MAKELPARAM(29, 28));
        SendMessage(hwndButtons, TB_SETINDENT, 4, 0);
        SendMessage(hwndButtons, TB_SETMAXTEXTROWS, 0, 0);
        SendMessage(hwndButtons, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
        // FGetSystemColors has mapped palette entry 253 to the button
        // face. Use that writable DIB instead of the magenta source resource.
        lpbi = (BITMAPINFO *)GlobalLock(hdibToolbar);
        if (!lpbi) return -1;
        hdc = GetDC(hwndButtons);
        hdcMem = CreateCompatibleDC(hdc);
        hbmp = CreateCompatibleBitmap(hdc, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight);
        hbmpSav = (HBITMAP)SelectObject(hdcMem, hbmp);
        if (!hdcMem || !hbmp || !StretchDIBits(hdcMem, 0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                (BYTE *)lpbi + lpbi->bmiHeader.biSize + PaletteSize(&lpbi->bmiHeader), lpbi, DIB_RGB_COLORS, SRCCOPY)) {
            if (hbmpSav) SelectObject(hdcMem, hbmpSav);
            DeleteDC(hdcMem); if (hbmp) DeleteObject(hbmp);
            ReleaseDC(hwndButtons, hdc); GlobalUnlock(hdibToolbar);
            return -1;
        }
        SelectObject(hdcMem, hbmpSav); DeleteDC(hdcMem);
        ReleaseDC(hwndButtons, hdc); GlobalUnlock(hdibToolbar);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hbmp);
        add.hInst = NULL; add.nID = (UINT_PTR)hbmp;
        if (SendMessage(hwndButtons, TB_ADDBITMAP, 18, (LPARAM)&add) < 0) return -1;
        c = 0; iRadar = -1;
        for (i = 0; i < 29; i++) {
            itb = (ToolbarButton)(int8_t)vrgTBBtn[i];
            if (itb == tbShipDesignFilterMenu || itb == tbEnemyClassFilterMenu) continue;
            rgbtn[c].fsState = TBSTATE_ENABLED;
            rgbtn[c].idCommand = IDC_STARS_TOOLBUTTON_FIRST + itb;
            rgbtn[c].iString = -1;
            if (itb < tbNormalView) {
                rgbtn[c].fsStyle = BTNS_SEP;
                rgbtn[c].iBitmap = DxOfBtn(itb);
                if (itb == tbScannerRange) iRadar = c;
            } else {
                rgbtn[c].iBitmap = itb;
                CchGetString(static_cast<StringId>(idsNormalView + static_cast<int>(itb)), rgszTip[itb]);
                rgbtn[c].iString = (INT_PTR)rgszTip[itb];
                rgbtn[c].fsStyle = itb <= tbNoPlayerInfoView ? BTNS_CHECKGROUP : BTNS_CHECK;
                if (itb == tbShipDesignFilter || itb == tbEnemyClassFilter) rgbtn[c].fsStyle |= BTNS_DROPDOWN;
                if (itb == tbMineFields || itb == tbZoomMenu) rgbtn[c].fsStyle = BTNS_WHOLEDROPDOWN;
            }
            c++;
        }
        if (!SendMessage(hwndButtons, TB_ADDBUTTONS, c, (LPARAM)rgbtn)) return -1;
        if (iRadar < 0 || !SendMessage(hwndButtons, TB_GETITEMRECT, iRadar, (LPARAM)&rc)) return -1;
        hwndTBRadar = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            rc.left, (28 - dyArial8 - 8) / 2 + 4, rc.right - rc.left, 11 * dyArial8 + 28, hwnd, NULL, hInst, NULL);
        if (!hwndTBRadar) return -1;
        SetWindowPos(hwndTBRadar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SendMessage(hwndTBRadar, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        iSel = -1;
        for (j = 0; j < 10; j++) {
            pct = 100 - 10 * j;
            if (pct == vpctRadarView) iSel = j;
            wsprintf(szWork, PCTDPCTPCT, pct);
            SendMessage(hwndTBRadar, CB_ADDSTRING, 0, (LPARAM)szWork);
        }
        SendMessage(hwndTBRadar, CB_SETEXTENDEDUI, 4, 0);
        SendMessage(hwndTBRadar, CB_SETCURSEL, iSel, 0);
        wsprintf(szWork, PCTDPCTPCT, vpctRadarView); SetWindowText(hwndTBRadar, szWork);
        lpfnRealComboProc = (WNDPROC)GetWindowLongPtr(hwndTBRadar, GWLP_WNDPROC);
        SetWindowLongPtr(hwndTBRadar, GWLP_WNDPROC, (LONG_PTR)lpfnFakeComboProc);
        hwndCE = GetWindow(hwndTBRadar, GW_CHILD);
        if (hwndCE) {
            lpfnRealCEProc = (WNDPROC)GetWindowLongPtr(hwndCE, GWLP_WNDPROC);
            SetWindowLongPtr(hwndCE, GWLP_WNDPROC, (LONG_PTR)lpfnFakeCEProc);
        }
        SyncToolbar(hwnd);
        return 0;
    case WM_NCDESTROY:
        hbmp = (HBITMAP)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (hbmp) DeleteObject(hbmp);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    case WM_MOUSEMOVE:
        if (hwnd == hwndTBRadar || GetParent(hwnd) == hwndTBRadar) {
            GetWindowRect(hwndTBRadar, &rc);
            ShowTooltip(idsScannerEffective, &rc);
        }
        return 0;
    case WM_SIZE:
        GetClientRect(hwnd, &rc);
        MoveWindow(hwndButtons, 0, 2, rc.right, 32, TRUE);
        return 0;
    case WM_COMMAND:
        if ((HWND)lParam == hwndButtons) {
            itb = static_cast<ToolbarButton>(LOWORD(wParam) - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb >= tbNormalView && itb <= tbShipCounts) {
                ExecuteButton(itb, itb <= tbNoPlayerInfoView || !FIsButtonDown(itb));
                SyncToolbar(hwnd);
            }
            return 0;
        }
        if ((HWND)lParam == hwndTBRadar && HIWORD(wParam) == CBN_KILLFOCUS) PostMessage(hwnd, 1524, 0, 0);
        return 0;
    case WM_NOTIFY:
        lpnm = (NMTOOLBAR *)lParam;
        if (lpnm && lpnm->hdr.hwndFrom == hwndButtons && lpnm->hdr.code == TBN_DROPDOWN) {
            itb = static_cast<ToolbarButton>(lpnm->iItem - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb == tbShipDesignFilter) itb = tbShipDesignFilterMenu;
            else if (itb == tbEnemyClassFilter) itb = tbEnemyClassFilterMenu;
            ExecuteButton(itb, !FIsButtonDown(itb));
            SyncToolbar(hwnd);
            return TBDDRET_NODEFAULT;
        }
        return 0;
    case 1524: TerminateToolbarFocus(FALSE); return 0;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc); FillRect((HDC)wParam, &rc, hbrButtonFace); return 1;
    case WM_CTLCOLORMSGBOX: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: case WM_CTLCOLORDLG: case WM_CTLCOLORSCROLLBAR: case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wParam, crButtonFace); return (LRESULT)hbrButtonFace;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps); GetClientRect(hwnd, &rc);
        DrawToolbar(hdc, &rc); EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawToolbar(HDC hdc, RECT *prc) {
    (void)hdc; (void)prc;
    SyncToolbar(hwndTb);
}

void DrawBitmapButton(HDC hdc, POINT16 pt, ToolbarButton ibtn, int16_t fDown) {
    (void)hdc; (void)pt;
    SendMessage(GetDlgItem(hwndTb, IDC_STARS_TOOLBAR), TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + ibtn, fDown);
}

int16_t ItbFromPpt(POINT16 *ppt) {
    int16_t i;
    HWND hwndButtons = GetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {ppt->x, ppt->y - 2};
    RECT rc;
    TBBUTTON btn;

    i = (int16_t)SendMessage(hwndButtons, TB_HITTEST, 0, (LPARAM)&pt);
    if (i < 0) {
        i = -i - 1;
    }
    if (SendMessage(hwndButtons, TB_GETBUTTON, i, (LPARAM)&btn) &&
        SendMessage(hwndButtons, TB_GETITEMRECT, i, (LPARAM)&rc) && PtInRect(&rc, pt)) {
        ppt->x = rc.left; ppt->y = rc.top + 2;
        return btn.idCommand - IDC_STARS_TOOLBUTTON_FIRST;
    }
    return -1;
}

int16_t DxOfBtn(ToolbarButton itb) {
    if (itb >= tbNormalView) {
        if (itb != tbShipDesignFilterMenu && itb != tbEnemyClassFilterMenu) {
            return 29;
        }
        return 11;
    }
    switch (itb) {
    case tbSeparator:
        return 6;
    case tbSpacer:
        return 2;
    case tbScannerRange:
        if (dyArial8 < 16) {
            return 60;
        }
        return 70;
    default:
        return 0;
    }
}

int16_t FIsButtonDown(ToolbarButton itb) {
    switch (itb) {
    default:
        break;
    case tbNormalView:
    case tbSurfaceMineralView:
    case tbMineralConcView:
    case tbPlanetValueView:
    case tbPopulationView:
    case tbNoPlayerInfoView:
        return (grbitScan & grbitScanViewMask) == itb;
    case tbAddWaypoints:
        return (grbitScan & grbitScanAddWaypoints) != 0;
    case tbScannerCoverage:
        return (grbitScan & grbitScanCoverage) != 0;
    case tbMineFields:
        return (grbitScan & grbitScanMineFields) != 0 && grbitScanMines == 15;
    case tbFleetPaths:
        return (grbitScan & grbitScanFleetPaths) != 0;
    case tbIdleFleets:
        return (grbitScan & grbitScanIdleFleets) != 0;
    case tbPlanetNames:
        return (grbitScan & grbitScanPlanetNames) != 0;
    case tbShipCounts:
        return (grbitScan & grbitScanShipCounts) != 0;
    case tbShipDesignFilter:
        return (grbitScan & grbitScanDesignFilter) != 0;
    case tbEnemyClassFilter:
        return (grbitScan & grbitScanEnemyFilter) != 0;
    case tbShipDesignFilterMenu:
    case tbEnemyClassFilterMenu:
    case tbZoomMenu:
        return FALSE;
    }
    return FALSE;
}

static POINT16 PtToolbarMenu(ToolbarButton itb) {
    HWND hwndButtons = GetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {0, 0};
    RECT rc;

    if (itb == tbShipDesignFilterMenu) itb = tbShipDesignFilter;
    else if (itb == tbEnemyClassFilterMenu) itb = tbEnemyClassFilter;
    if (SendMessage(hwndButtons, TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + itb, (LPARAM)&rc)) {
        pt.x = rc.left; pt.y = rc.bottom;
        MapWindowPoints(hwndButtons, hwndTb, &pt, 1);
    }
    return PointTo16(pt);
}

void ExecuteButton(ToolbarButton itb, int16_t fDown) {
    uint16_t grbitNew;
    POINT16  pt;
    char    *rgszScan[12];
    int16_t  c;
    int16_t  i;
    uint16_t grbit;
    int32_t  rgid[12];
    int16_t  iSel;
    uint16_t grbitSh;
    int16_t  ish;

    gd.fChgScanner = TRUE;
    switch (itb) {
    case tbNormalView:
    case tbSurfaceMineralView:
    case tbMineralConcView:
    case tbPlanetValueView:
    case tbPopulationView:
    case tbNoPlayerInfoView:
        if (!fDown)
            return;
        grbitScan = static_cast<GrbitScan>(itb + (grbitScan & grbitScanToggleMask));
        break;
    case tbAddWaypoints:
        grbitNew = 16;
    LBitDiddle:
        if (fDown) {
            grbitScan = static_cast<GrbitScan>(grbitScan | grbitNew);
        } else {
            grbitScan = static_cast<GrbitScan>(grbitScan & ~grbitNew);
        }
        break;
    case tbScannerCoverage:
        grbitNew = 32;
        goto LBitDiddle;
    case tbFleetPaths:
        grbitNew = 128;
        goto LBitDiddle;
    case tbIdleFleets:
        grbitNew = 0x100;
        goto LBitDiddle;
    case tbPlanetNames:
        grbitNew = 0x400;
        goto LBitDiddle;
    case tbShipCounts:
        grbitNew = 0x1000;
        goto LBitDiddle;
    case tbShipDesignFilter:
        grbitNew = 0x200;
        goto LBitDiddle;
    case tbEnemyClassFilter:
        grbitNew = 0x800;
        goto LBitDiddle;
    case tbMineFields:
        grbit = 1;
        c = 0;
        if (!(grbitScan & grbitScanMineFields)) {
            grbitScanMines = 0;
        }
        for (i = idsMineFields; i <= idsMineFields2; i++) {
            if (i == idsMineFields) {
                rgid[c] = (uint32_t)(grbitScanMines == 15);
            } else {
                rgid[c] = (uint32_t)(grbitScanMines == 0);
            }
            CchGetString(static_cast<StringId>(i), &szWork[(i - idsMineFields) * 30 + 160]);
            rgszScan[c++] = &szWork[(i - idsMineFields) * 30 + 160];
        }
        rgid[c] = 0;
        szWork[250] = -1;
        szWork[251] = 0;
        rgszScan[c++] = &szWork[250];
        for (i = 0; i < 4; i++) {
            rgid[c] = (uint32_t)((1 << i & grbitScanMines) != 0);
            CchGetString(static_cast<StringId>(idsMineFields3 + i), &szWork[i * 30]);
            rgszScan[c++] = &szWork[i * 30];
        }
        pt = PtToolbarMenu(itb);
        iSel = PopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, FALSE);
        if (iSel == -1)
            return;
        if (iSel >= 3) {
            iSel -= 3;
            grbitScanMines ^= 1 << iSel;
        } else if (iSel == 0) {
            grbitScanMines = 15;
        } else {
            grbitScanMines = 0;
        }
        if (grbitScanMines != 0) {
            grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanMineFields);
        } else {
            grbitScan = static_cast<GrbitScan>(grbitScan & 0xffbf);
        }
        InvalidateRect(hwndTb, NULL, TRUE);
        break;
    case tbShipDesignFilterMenu:
        c = 0;
        for (i = idsDesigns; i <= idsDesigns2; i++) {
            rgid[c] = 0;
            CchGetString(static_cast<StringId>(i), &szWork[(i - idsDesigns) * 20]);
            rgszScan[c++] = &szWork[(i - idsDesigns) * 20];
        }
        rgid[c] = 0;
        szWork[200] = -1;
        szWork[201] = 0;
        rgszScan[c++] = &szWork[200];
        ish = 0;
        grbitSh = 1;
        while (ish < 16) {
            if (!rgshdef[ish].fFree) {
                rgid[c] = (uint32_t)((grbitSh & grbitScanShip) != 0);
                rgszScan[c++] = rgshdef[ish].hul.szClass;
            }
            ish++;
            grbitSh *= 2;
        }
        pt = PtToolbarMenu(itb);
        iSel = PopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, FALSE);
        if (iSel == -1)
            return;
        if (iSel < 4) {
            if (iSel == 0) {
                grbitScanShip = 0xffff;
            } else if (iSel == 1) {
                grbitScanShip ^= 0xffff;
            } else {
                grbitScanShip = 0;
            }
            if (!(grbitScan & grbitScanDesignFilter) && grbitScanShip != 0)
                goto LInvalS;
        } else {
            iSel -= 4;
            for (ish = 0; ish < 16; ish++) {
                if (!rgshdef[ish].fFree) {
                    iSel--;
                    if (iSel < 0)
                        break;
                }
            }
            grbitScanShip ^= 1 << ish;
            if (!(grbitScan & grbitScanDesignFilter) && (1 << ish & grbitScanShip)) {
            LInvalS:
                grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanDesignFilter);
                InvalidateRect(hwndTb, NULL, TRUE);
            }
        }
        if (!(grbitScan & grbitScanDesignFilter))
            return;
        break;
    case tbEnemyClassFilterMenu:
        grbit = 1;
        c = 0;
        for (i = idsDesigns; i <= idsDesigns2; i++) {
            rgid[c] = 0;
            CchGetString(static_cast<StringId>(i), &szWork[(i - idsDesigns) * 25 + 200]);
            rgszScan[c++] = &szWork[(i - idsDesigns) * 25 + 200];
        }
        rgid[c] = 0;
        szWork[300] = -1;
        szWork[301] = 0;
        rgszScan[c++] = &szWork[300];
        for (i = 0; i < 8; i++) {
            rgid[c] = (uint32_t)((1 << i & grbitScanEShip) != 0);
            CchGetString(static_cast<StringId>(idsColony + i), &szWork[i * 25]);
            rgszScan[c++] = &szWork[i * 25];
        }
        pt = PtToolbarMenu(itb);
        iSel = PopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, FALSE);
        if (iSel == -1)
            return;
        if (iSel < 4) {
            if (iSel == 0) {
                grbitScanEShip = 0xff;
            } else if (iSel == 1) {
                grbitScanEShip ^= 0xff;
            } else {
                grbitScanEShip = 0;
            }
            if (!(grbitScan & grbitScanEnemyFilter) && grbitScanEShip != 0)
                goto LInvalE;
        } else {
            iSel -= 4;
            grbitScanEShip ^= 1 << iSel;
            if (!(grbitScan & grbitScanEnemyFilter) && (1 << iSel & grbitScanEShip)) {
            LInvalE:
                grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanEnemyFilter);
                InvalidateRect(hwndTb, NULL, TRUE);
            }
        }
        if (!(grbitScan & grbitScanEnemyFilter))
            return;
        break;
    case tbZoomMenu:
        c = 0;
        for (i = 0; i < 9; i++) {
            rgid[c] = (uint32_t)(iScanZoom + 4 == i);
            wsprintf(&szWork[i * 8], PCTDPCTPCT, vrgpctZoom[i]);
            rgszScan[c++] = &szWork[i * 8];
        }
        pt = PtToolbarMenu(itb);
        iSel = PopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, FALSE);
        if (iSel != -1) {
            CommandHandler(hwndFrame, iSel + 3901);
        }
        return;
    default:
        return;
    }
    if (itb != tbAddWaypoints) {
        InvalidateRect(hwndScanner, NULL, TRUE);
    }
    if (gd.fTutorial) {
        AdvanceTutor();
    }
}

void TerminateToolbarFocus(int16_t fCancel) {
    int16_t pct;
    char   *psz;

    gd.fChgScanner = TRUE;
    if (!fCancel) {
        GetWindowText(hwndTBRadar, szWork, 20);
        psz = szWork;
        pct = 0;
        for (; *psz >= '0' && *psz <= '9'; psz++) {
            pct = 10 * pct + (*psz - '0');
        }
        if (*psz != 0 && *psz != '%') {
            pct = 0;
        }
    } else {
        pct = vpctRadarView;
    }
    if (pct < 2) {
        pct = 2;
    } else if (pct > 100) {
        pct = 100;
    }
    SendMessage(hwndTBRadar, CB_SETCURSEL, (int16_t)(100 - pct) / 10, 0);
    wsprintf(szWork, PCTDPCTPCT, pct);
    SetWindowText(hwndTBRadar, szWork);
    if (pct != vpctRadarView) {
        vpctRadarView = pct;
        if (!(grbitScan & grbitScanCoverage)) {
            InvalidateRect(hwndTb, NULL, TRUE);
        }
        grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanCoverage);
        InvalidateRect(hwndScanner, NULL, TRUE);
    }
    SetFocus(hwndFrame);
    return;
}

void ShowTooltip(StringId ids, RECT *prc) {
    HDC     hdc;
    HFONT   hfontSav;
    int16_t fVisCur;
    int16_t cch;
    int16_t fShowNow;

    fVisCur = hwndTooltip && IsWindowVisible(hwndTooltip) != 0;
    if ((int16_t)ids < idsUniverseDefinitionFileSeemsMissingCorrupt || !prc) {
        if (hwndTooltip) {
            DestroyWindow(hwndTooltip);
        }
        if (fVisCur) {
            vtickTooltipLast = GetTickCount();
        }
    } else if (ids != vidsTooltip || EqualRect(prc, &vrcTooltip) == 0) {
        vidsTooltip = ids;
        cch = CchGetString(static_cast<StringId>(vidsTooltip), szWork);
        vrcTooltip = *prc;
        hdc = GetDC(NULL);
        hfontSav = (HFONT)SelectObject(hdc, rghfontArial8[0]);
        dxTip = LOWORD(GetTextExtent(hdc, szWork, cch));
        SelectObject(hdc, hfontSav);
        ReleaseDC(NULL, hdc);
        if (!hwndTooltip) {
            CreateWindowExA(WS_EX_NOACTIVATE, szTooltip, NULL, WS_POPUP, prc->left, prc->bottom + 2,
                           dxTip + 6, dyArial8 + 6, hwndTb, NULL, hInst, NULL);
        } else {
            InvalidateRect(hwndTooltip, NULL, TRUE);
        }
        SetWindowPos(hwndTooltip, (HWND)-1, 0, 0, dxTip + 6, dyArial8 + 6, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        fShowNow = vtickTooltipLast + 400 >= GetTickCount() || fVisCur != 0;
        TooltipWndProc(hwndTooltip, 1523, fShowNow, 0);
    }
    return;
}

LRESULT CALLBACK TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    POINT16     pt;
    PAINTSTRUCT ps;
    RECT        rc;
    int16_t     bkSav;
    int16_t     cch;

    switch (msg) {
    case WM_CREATE:
        hwndTooltip = hwnd;
        break;
    case WM_DESTROY:
        hwndTooltip = 0;
        if (vidTimerTooltip != -1) {
            KillTimer(hwnd, vidTimerTooltip);
            vidTimerTooltip = -1;
        }
        vidsTooltip = -1;
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        DestroyWindow(hwnd);
        return 0;
    case 1523:
        if (wParam == 0) {
            if (vidTimerTooltip != -1) {
                KillTimer(hwnd, vidTimerTooltip);
            }
            if (SetTimer(hwnd, 926, 700, NULL) != 0) {
                vidTimerTooltip = 926;
            } else {
                vidTimerTooltip = -1;
            }
            return 0;
        }
        wParam = 926;
        /* fallthrough */
    case WM_TIMER:
        if (wParam != 926)
            break;
        if (msg != WM_TIMER || IsWindowVisible(hwnd) == 0) {
            vtickTooltip1stVis = GetTickCount();
            GetCursorPos16(&pt);
            if (PtInRect(&vrcTooltip, PointFrom16(pt)) == 0)
                goto LKillTip;
            // Anchor to the hovered control, independently of the pointer's
            // position inside it; keep the complete tip within the frame.
            GetWindowRect(hwndFrame, &rc);
            pt.x = max(rc.left, min(vrcTooltip.left, rc.right - dxTip - 6));
            pt.y = vrcTooltip.bottom + 2;
            if (pt.y + dyArial8 + 6 > rc.bottom) {
                pt.y = max(rc.top, vrcTooltip.top - dyArial8 - 8);
            }
            SetWindowPos(hwnd, (HWND)-1, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
            UpdateWindow(hwnd);
            if (vidTimerTooltip != -1) {
                KillTimer(hwnd, vidTimerTooltip);
            }
            if (SetTimer(hwnd, 926, 50, NULL) != 0) {
                vidTimerTooltip = 926;
            } else {
                vidTimerTooltip = -1;
            }
            return 0;
        } else {
            vtickTooltipLast = GetTickCount();
            GetCursorPos16(&pt);
            if (PtInRect(&vrcTooltip, PointFrom16(pt)) == 0 || vtickTooltip1stVis + 10000 < vtickTooltipLast) {
            LKillTip:
                DestroyWindow(hwnd);
            }
            return 0;
        }
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrTooltip);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        FrameRect(hdc, &rc, hbrWindowFrame);
        SelectObject(hdc, rghfontArial8[0]);
        cch = CchGetString(static_cast<StringId>(vidsTooltip), szWork);
        bkSav = SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, crWindowText);
        ExtTextOut(hdc, 3, 3, 0, NULL, szWork, cch, NULL);
        SetBkMode(hdc, bkSav);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK FakeComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        ShowTooltip(static_cast<StringId>(idsNoString), NULL);
        break;
    case WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return CallWindowProc(lpfnRealComboProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK FakeCEProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        ShowTooltip(static_cast<StringId>(idsNoString), NULL);
        break;
    case WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return CallWindowProc(lpfnRealCEProc, hwnd, msg, wParam, lParam);
}
