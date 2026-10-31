#include "win.h"
#include "control_io.h"
#include "presentation_io.h"
#include "text_io.h"

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
    HWND hwndButtons = ControlIO().pfnGetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    int16_t itb;

    for (itb = tbNormalView; itb <= tbShipCounts; itb++) {
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + itb, FIsButtonDown(static_cast<ToolbarButton>(itb)));
    }
}

LRESULT  TbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND          hwndButtons;
    HWND          hwndCE;
    INITCOMMONCONTROLSEX icc = {sizeof(icc), win32::ICC_BAR_CLASSES};
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

    hwndButtons = ControlIO().pfnGetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    switch (msg) {
    case win32::WM_CREATE:
        if (!win32::InitCommonControlsEx(&icc)) return -1;
        hwndButtons = ui::CreateWindowEx(0, win32::TOOLBARCLASSNAMEA, NULL,
            win32::WS_CHILD | win32::WS_VISIBLE | win32::TBSTYLE_TOOLTIPS | win32::CCS_NORESIZE | win32::CCS_NOPARENTALIGN | win32::CCS_NODIVIDER,
            0, 2, 10, 32, hwnd, (HMENU)(INT_PTR)IDC_STARS_TOOLBAR, hInst, NULL);
        if (!hwndButtons) return -1;
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETBITMAPSIZE, 0, win32::MAKELPARAM(24, 23));
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETBUTTONSIZE, 0, win32::MAKELPARAM(29, 28));
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETINDENT, 4, 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETMAXTEXTROWS, 0, 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETEXTENDEDSTYLE, 0, win32::TBSTYLE_EX_DRAWDDARROWS);
        // FGetSystemColors has mapped palette entry 253 to the button
        // face. Use that writable DIB instead of the magenta source resource.
        lpbi = (BITMAPINFO *)win32::GlobalLock(hdibToolbar);
        if (!lpbi) return -1;
        hdc = win32::GetDC(hwndButtons);
        hdcMem = win32::CreateCompatibleDC(hdc);
        hbmp = win32::CreateCompatibleBitmap(hdc, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight);
        hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, hbmp);
        if (!hdcMem || !hbmp || !win32::StretchDIBits(hdcMem, 0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                (BYTE *)lpbi + lpbi->bmiHeader.biSize + PaletteSize(&lpbi->bmiHeader), lpbi, win32::DIB_RGB_COLORS, win32::SRCCOPY)) {
            if (hbmpSav) win32::SelectObject(hdcMem, hbmpSav);
            win32::DeleteDC(hdcMem); if (hbmp) win32::DeleteObject(hbmp);
            win32::ReleaseDC(hwndButtons, hdc); win32::GlobalUnlock(hdibToolbar);
            return -1;
        }
        win32::SelectObject(hdcMem, hbmpSav); win32::DeleteDC(hdcMem);
        win32::ReleaseDC(hwndButtons, hdc); win32::GlobalUnlock(hdibToolbar);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, (LONG_PTR)hbmp);
        add.hInst = NULL; add.nID = (UINT_PTR)hbmp;
        if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_ADDBITMAP, 18, (LPARAM)&add) < 0) return -1;
        c = 0; iRadar = -1;
        for (i = 0; i < 29; i++) {
            itb = (ToolbarButton)(int8_t)vrgTBBtn[i];
            if (itb == tbShipDesignFilterMenu || itb == tbEnemyClassFilterMenu) continue;
            rgbtn[c].fsState = win32::TBSTATE_ENABLED;
            rgbtn[c].idCommand = IDC_STARS_TOOLBUTTON_FIRST + itb;
            rgbtn[c].iString = -1;
            if (itb < tbNormalView) {
                rgbtn[c].fsStyle = win32::BTNS_SEP;
                rgbtn[c].iBitmap = DxOfBtn(itb);
                if (itb == tbScannerRange) iRadar = c;
            } else {
                rgbtn[c].iBitmap = itb;
                TextIO().pfnCchGetString(static_cast<StringId>(idsNormalView + static_cast<int>(itb)), rgszTip[itb]);
                rgbtn[c].iString = (INT_PTR)rgszTip[itb];
                rgbtn[c].fsStyle = itb <= tbNoPlayerInfoView ? win32::BTNS_CHECKGROUP : win32::BTNS_CHECK;
                if (itb == tbShipDesignFilter || itb == tbEnemyClassFilter) rgbtn[c].fsStyle |= win32::BTNS_DROPDOWN;
                if (itb == tbMineFields || itb == tbZoomMenu) rgbtn[c].fsStyle = win32::BTNS_WHOLEDROPDOWN;
            }
            c++;
        }
        if (!ControlIO().pfnSendMessage(hwndButtons, win32::TB_ADDBUTTONS, c, (LPARAM)rgbtn)) return -1;
        if (iRadar < 0 || !ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETITEMRECT, iRadar, (LPARAM)&rc)) return -1;
        hwndTBRadar = ui::CreateWindowEx(0, "COMBOBOX", NULL, win32::CBS_DROPDOWN | win32::CBS_AUTOHSCROLL | win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_VSCROLL,
            rc.left, (28 - dyArial8 - 8) / 2 + 4, rc.right - rc.left, 11 * dyArial8 + 28, hwnd, NULL, hInst, NULL);
        if (!hwndTBRadar) return -1;
        ControlIO().pfnSetWindowPos(hwndTBRadar, win32::HWND_TOP, 0, 0, 0, 0, win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOACTIVATE);
        ControlIO().pfnSendMessage(hwndTBRadar, win32::WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        iSel = -1;
        for (j = 0; j < 10; j++) {
            pct = 100 - 10 * j;
            if (pct == vpctRadarView) iSel = j;
            win32::wsprintfA(szWork, PCTDPCTPCT, pct);
            ControlIO().pfnSendMessage(hwndTBRadar, win32::CB_ADDSTRING, 0, (LPARAM)szWork);
        }
        ControlIO().pfnSendMessage(hwndTBRadar, win32::CB_SETEXTENDEDUI, 4, 0);
        ControlIO().pfnSendMessage(hwndTBRadar, win32::CB_SETCURSEL, iSel, 0);
        win32::wsprintfA(szWork, PCTDPCTPCT, vpctRadarView); ControlIO().pfnSetWindowText(hwndTBRadar, szWork);
        lpfnRealComboProc = (WNDPROC)win32::GetWindowLongPtrA(hwndTBRadar, win32::GWLP_WNDPROC);
        win32::SetWindowLongPtrA(hwndTBRadar, win32::GWLP_WNDPROC, (LONG_PTR)lpfnFakeComboProc);
        hwndCE = win32::GetWindow(hwndTBRadar, win32::GW_CHILD);
        if (hwndCE) {
            lpfnRealCEProc = (WNDPROC)win32::GetWindowLongPtrA(hwndCE, win32::GWLP_WNDPROC);
            win32::SetWindowLongPtrA(hwndCE, win32::GWLP_WNDPROC, (LONG_PTR)lpfnFakeCEProc);
        }
        SyncToolbar(hwnd);
        return 0;
    case win32::WM_NCDESTROY:
        hbmp = (HBITMAP)win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA);
        if (hbmp) win32::DeleteObject(hbmp);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, 0);
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    case win32::WM_MOUSEMOVE:
        if (hwnd == hwndTBRadar || win32::GetParent(hwnd) == hwndTBRadar) {
            win32::GetWindowRect(hwndTBRadar, &rc);
            ShowTooltip(idsScannerEffective, &rc);
        }
        return 0;
    case win32::WM_SIZE:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ui::MoveWindow(hwndButtons, 0, 2, rc.right, 32, 1);
        return 0;
    case win32::WM_COMMAND:
        if ((HWND)lParam == hwndButtons) {
            itb = static_cast<ToolbarButton>(LOWORD(wParam) - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb >= tbNormalView && itb <= tbShipCounts) {
                ExecuteButton(itb, itb <= tbNoPlayerInfoView || !FIsButtonDown(itb));
                SyncToolbar(hwnd);
            }
            return 0;
        }
        if ((HWND)lParam == hwndTBRadar && HIWORD(wParam) == win32::CBN_KILLFOCUS) win32::PostMessageA(hwnd, 1524, 0, 0);
        return 0;
    case win32::WM_NOTIFY:
        lpnm = (NMTOOLBAR *)lParam;
        if (lpnm && lpnm->hdr.hwndFrom == hwndButtons && lpnm->hdr.code == win32::TBN_DROPDOWN) {
            itb = static_cast<ToolbarButton>(lpnm->iItem - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb == tbShipDesignFilter) itb = tbShipDesignFilterMenu;
            else if (itb == tbEnemyClassFilter) itb = tbEnemyClassFilterMenu;
            ExecuteButton(itb, !FIsButtonDown(itb));
            SyncToolbar(hwnd);
            return win32::TBDDRET_NODEFAULT;
        }
        return 0;
    case 1524: TerminateToolbarFocus(0); return 0;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc); win32::FillRect((HDC)wParam, &rc, hbrButtonFace); return 1;
    case win32::WM_CTLCOLORMSGBOX: case win32::WM_CTLCOLOREDIT: case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN: case win32::WM_CTLCOLORDLG: case win32::WM_CTLCOLORSCROLLBAR: case win32::WM_CTLCOLORSTATIC:
        win32::SetBkColor((HDC)wParam, crButtonFace); return (LRESULT)hbrButtonFace;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps); ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawToolbar(hdc, &rc); win32::EndPaint(hwnd, &ps); return 0;
    }
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawToolbar(HDC hdc, RECT *prc) {
    (void)hdc; (void)prc;
    SyncToolbar(hwndTb);
}

void DrawBitmapButton(HDC hdc, POINT16 pt, ToolbarButton ibtn, int16_t fDown) {
    (void)hdc; (void)pt;
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwndTb, IDC_STARS_TOOLBAR), win32::TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + ibtn, fDown);
}

int16_t ItbFromPpt(POINT16 *ppt) {
    int16_t i;
    HWND hwndButtons = ControlIO().pfnGetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {ppt->x, ppt->y - 2};
    RECT rc;
    TBBUTTON btn;

    i = (int16_t)ControlIO().pfnSendMessage(hwndButtons, win32::TB_HITTEST, 0, (LPARAM)&pt);
    if (i < 0) {
        i = -i - 1;
    }
    if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETBUTTON, i, (LPARAM)&btn) &&
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETITEMRECT, i, (LPARAM)&rc) && win32::PtInRect(&rc, pt)) {
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
        return 0;
    }
    return 0;
}

static POINT16 PtToolbarMenu(ToolbarButton itb) {
    HWND hwndButtons = ControlIO().pfnGetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {0, 0};
    RECT rc;

    if (itb == tbShipDesignFilterMenu) itb = tbShipDesignFilter;
    else if (itb == tbEnemyClassFilterMenu) itb = tbEnemyClassFilter;
    if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + itb, (LPARAM)&rc)) {
        pt.x = rc.left; pt.y = rc.bottom;
        win32::MapWindowPoints(hwndButtons, hwndTb, &pt, 1);
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

    gd.fChgScanner = 1;
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
            TextIO().pfnCchGetString(static_cast<StringId>(i), &szWork[(i - idsMineFields) * 30 + 160]);
            rgszScan[c++] = &szWork[(i - idsMineFields) * 30 + 160];
        }
        rgid[c] = 0;
        szWork[250] = -1;
        szWork[251] = 0;
        rgszScan[c++] = &szWork[250];
        for (i = 0; i < 4; i++) {
            rgid[c] = (uint32_t)((1 << i & grbitScanMines) != 0);
            TextIO().pfnCchGetString(static_cast<StringId>(idsMineFields3 + i), &szWork[i * 30]);
            rgszScan[c++] = &szWork[i * 30];
        }
        pt = PtToolbarMenu(itb);
        iSel = ControlIO().pfnPopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
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
        ControlIO().pfnInvalidateRect(hwndTb, NULL, 1);
        break;
    case tbShipDesignFilterMenu:
        c = 0;
        for (i = idsDesigns; i <= idsDesigns2; i++) {
            rgid[c] = 0;
            TextIO().pfnCchGetString(static_cast<StringId>(i), &szWork[(i - idsDesigns) * 20]);
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
        iSel = ControlIO().pfnPopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
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
                ControlIO().pfnInvalidateRect(hwndTb, NULL, 1);
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
            TextIO().pfnCchGetString(static_cast<StringId>(i), &szWork[(i - idsDesigns) * 25 + 200]);
            rgszScan[c++] = &szWork[(i - idsDesigns) * 25 + 200];
        }
        rgid[c] = 0;
        szWork[300] = -1;
        szWork[301] = 0;
        rgszScan[c++] = &szWork[300];
        for (i = 0; i < 8; i++) {
            rgid[c] = (uint32_t)((1 << i & grbitScanEShip) != 0);
            TextIO().pfnCchGetString(static_cast<StringId>(idsColony + i), &szWork[i * 25]);
            rgszScan[c++] = &szWork[i * 25];
        }
        pt = PtToolbarMenu(itb);
        iSel = ControlIO().pfnPopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
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
                ControlIO().pfnInvalidateRect(hwndTb, NULL, 1);
            }
        }
        if (!(grbitScan & grbitScanEnemyFilter))
            return;
        break;
    case tbZoomMenu:
        c = 0;
        for (i = 0; i < 9; i++) {
            rgid[c] = (uint32_t)(iScanZoom + 4 == i);
            win32::wsprintfA(&szWork[i * 8], PCTDPCTPCT, vrgpctZoom[i]);
            rgszScan[c++] = &szWork[i * 8];
        }
        pt = PtToolbarMenu(itb);
        iSel = ControlIO().pfnPopupMenu(hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
        if (iSel != -1) {
            CommandHandler(hwndFrame, iSel + 3901);
        }
        return;
    default:
        return;
    }
    if (itb != tbAddWaypoints) {
        ControlIO().pfnInvalidateRect(hwndScanner, NULL, 1);
    }
    if (gd.fTutorial) {
        PresentationIO().pfnTutor();
    }
}

void TerminateToolbarFocus(int16_t fCancel) {
    int16_t pct;
    char   *psz;

    gd.fChgScanner = 1;
    if (!fCancel) {
        ControlIO().pfnGetWindowText(hwndTBRadar, szWork, 20);
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
    ControlIO().pfnSendMessage(hwndTBRadar, win32::CB_SETCURSEL, (int16_t)(100 - pct) / 10, 0);
    win32::wsprintfA(szWork, PCTDPCTPCT, pct);
    ControlIO().pfnSetWindowText(hwndTBRadar, szWork);
    if (pct != vpctRadarView) {
        vpctRadarView = pct;
        if (!(grbitScan & grbitScanCoverage)) {
            ControlIO().pfnInvalidateRect(hwndTb, NULL, 1);
        }
        grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanCoverage);
        ControlIO().pfnInvalidateRect(hwndScanner, NULL, 1);
    }
    ControlIO().pfnSetFocus(hwndFrame);
    return;
}

void ShowTooltip(StringId ids, RECT *prc) {
    HDC     hdc;
    HFONT   hfontSav;
    int16_t fVisCur;
    int16_t cch;
    int16_t fShowNow;

    fVisCur = hwndTooltip && win32::IsWindowVisible(hwndTooltip) != 0;
    if ((int16_t)ids < idsUniverseDefinitionFileSeemsMissingCorrupt || !prc) {
        if (hwndTooltip) {
            ControlIO().pfnDestroyWindow(hwndTooltip);
        }
        if (fVisCur) {
            vtickTooltipLast = win32::GetTickCount();
        }
    } else if (ids != vidsTooltip || win32::EqualRect(prc, &vrcTooltip) == 0) {
        vidsTooltip = ids;
        cch = TextIO().pfnCchGetString(static_cast<StringId>(vidsTooltip), szWork);
        vrcTooltip = *prc;
        hdc = win32::GetDC(NULL);
        hfontSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[0]);
        dxTip = LOWORD(GetTextExtent(hdc, szWork, cch));
        win32::SelectObject(hdc, hfontSav);
        win32::ReleaseDC(NULL, hdc);
        if (!hwndTooltip) {
            ui::CreateWindowEx(win32::WS_EX_NOACTIVATE, szTooltip, NULL, win32::WS_POPUP, prc->left, prc->bottom + 2,
                           dxTip + 6, dyArial8 + 6, hwndTb, NULL, hInst, NULL);
        } else {
            ControlIO().pfnInvalidateRect(hwndTooltip, NULL, 1);
        }
        ControlIO().pfnSetWindowPos(hwndTooltip, (HWND)-1, 0, 0, dxTip + 6, dyArial8 + 6, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_NOOWNERZORDER);
        fShowNow = vtickTooltipLast + 400 >= win32::GetTickCount() || fVisCur != 0;
        TooltipWndProc(hwndTooltip, 1523, fShowNow, 0);
    }
    return;
}

LRESULT  TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    POINT16     pt;
    PAINTSTRUCT ps;
    RECT        rc;
    int16_t     bkSav;
    int16_t     cch;

    switch (msg) {
    case win32::WM_CREATE:
        hwndTooltip = hwnd;
        break;
    case win32::WM_DESTROY:
        hwndTooltip = 0;
        if (vidTimerTooltip != -1) {
            win32::KillTimer(hwnd, vidTimerTooltip);
            vidTimerTooltip = -1;
        }
        vidsTooltip = -1;
        break;
    case win32::WM_MOUSEMOVE:
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ControlIO().pfnDestroyWindow(hwnd);
        return 0;
    case 1523:
        if (wParam == 0) {
            if (vidTimerTooltip != -1) {
                win32::KillTimer(hwnd, vidTimerTooltip);
            }
            if (win32::SetTimer(hwnd, 926, 700, NULL) != 0) {
                vidTimerTooltip = 926;
            } else {
                vidTimerTooltip = -1;
            }
            return 0;
        }
        wParam = 926;
        /* fallthrough */
    case win32::WM_TIMER:
        if (wParam != 926)
            break;
        if (msg != win32::WM_TIMER || win32::IsWindowVisible(hwnd) == 0) {
            vtickTooltip1stVis = win32::GetTickCount();
            GetCursorPos16(&pt);
            if (win32::PtInRect(&vrcTooltip, PointFrom16(pt)) == 0)
                goto LKillTip;
            // Anchor to the hovered control, independently of the pointer's
            // position inside it; keep the complete tip within the frame.
            win32::GetWindowRect(hwndFrame, &rc);
            pt.x = (((rc.left) > (StarsMin(vrcTooltip.left, rc.right - dxTip - 6))) ? (rc.left) : (StarsMin(vrcTooltip.left, rc.right - dxTip - 6)));
            pt.y = vrcTooltip.bottom + 2;
            if (pt.y + dyArial8 + 6 > rc.bottom) {
                pt.y = StarsMax(rc.top, vrcTooltip.top - dyArial8 - 8);
            }
            ControlIO().pfnSetWindowPos(hwnd, (HWND)-1, pt.x, pt.y, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW | win32::SWP_NOOWNERZORDER);
            ControlIO().pfnUpdateWindow(hwnd);
            if (vidTimerTooltip != -1) {
                win32::KillTimer(hwnd, vidTimerTooltip);
            }
            if (win32::SetTimer(hwnd, 926, 50, NULL) != 0) {
                vidTimerTooltip = 926;
            } else {
                vidTimerTooltip = -1;
            }
            return 0;
        } else {
            vtickTooltipLast = win32::GetTickCount();
            GetCursorPos16(&pt);
            if (win32::PtInRect(&vrcTooltip, PointFrom16(pt)) == 0 || vtickTooltip1stVis + 10000 < vtickTooltipLast) {
            LKillTip:
                ControlIO().pfnDestroyWindow(hwnd);
            }
            return 0;
        }
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrTooltip);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FrameRect(hdc, &rc, hbrWindowFrame);
        win32::SelectObject(hdc, rghfontArial8[0]);
        cch = TextIO().pfnCchGetString(static_cast<StringId>(vidsTooltip), szWork);
        bkSav = win32::SetBkMode(hdc, win32::TRANSPARENT);
        win32::SetTextColor(hdc, crWindowText);
        win32::ExtTextOutA(hdc, 3, 3, 0, NULL, szWork, cch, NULL);
        win32::SetBkMode(hdc, bkSav);
        win32::EndPaint(hwnd, &ps);
        return 0;
    }
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT  FakeComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ShowTooltip(static_cast<StringId>(idsNoString), NULL);
        break;
    case win32::WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return ui::CallWindowProc(lpfnRealComboProc, hwnd, msg, wParam, lParam);
}

LRESULT  FakeCEProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ShowTooltip(static_cast<StringId>(idsNoString), NULL);
        break;
    case win32::WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return ui::CallWindowProc(lpfnRealCEProc, hwnd, msg, wParam, lParam);
}
