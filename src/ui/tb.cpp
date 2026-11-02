#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
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
std::int16_t vrgpctZoom[9] = {25, 38, 50, 75, 100, 125, 150, 200, 400};

static void SyncToolbar(APPSESSION &app, HWND hwnd) {
    HWND hwndButtons = ControlIO().pfnGetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    std::int16_t itb;

    for (itb = tbNormalView; itb <= tbShipCounts; itb++) {
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + itb, FIsButtonDown(app, static_cast<ToolbarButton>(itb)));
    }
}

LRESULT  TbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

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
    std::int16_t       i, j, c, pct, iSel;
    int           iRadar;
    RECT          rc;
    PAINTSTRUCT   ps;
    HDC           hdc;
    char          rgszTip[18][256];

    hwndButtons = ControlIO().pfnGetDlgItem(hwnd, IDC_STARS_TOOLBAR);
    switch (msg) {
    case win32::WM_CREATE:
        if (!win32::InitCommonControlsEx(&icc)) return -1;
        hwndButtons = ui::CreateWindowEx(0, win32::TOOLBARCLASSNAMEA, nullptr,
            win32::WS_CHILD | win32::WS_VISIBLE | win32::TBSTYLE_TOOLTIPS | win32::CCS_NORESIZE | win32::CCS_NOPARENTALIGN | win32::CCS_NODIVIDER,
            0, 2, 10, 32, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STARS_TOOLBAR)), app.lpui->hInst, nullptr);
        if (!hwndButtons) return -1;
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETBITMAPSIZE, 0, win32::MAKELPARAM(24, 23));
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETBUTTONSIZE, 0, win32::MAKELPARAM(29, 28));
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETINDENT, 4, 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETMAXTEXTROWS, 0, 0);
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_SETEXTENDEDSTYLE, 0, win32::TBSTYLE_EX_DRAWDDARROWS);
        // FGetSystemColors has mapped palette entry 253 to the button
        // face. Use that writable DIB instead of the magenta source resource.
        lpbi = static_cast<BITMAPINFO *>(win32::GlobalLock(app.lpui->hdibToolbar));
        if (!lpbi) return -1;
        hdc = win32::GetDC(hwndButtons);
        hdcMem = win32::CreateCompatibleDC(hdc);
        hbmp = win32::CreateCompatibleBitmap(hdc, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight);
        hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, hbmp));
        if (!hdcMem || !hbmp || !win32::StretchDIBits(hdcMem, 0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                0, 0, lpbi->bmiHeader.biWidth, lpbi->bmiHeader.biHeight,
                reinterpret_cast<BYTE *>(lpbi) + lpbi->bmiHeader.biSize + PaletteSize(&lpbi->bmiHeader), lpbi, win32::DIB_RGB_COLORS, win32::SRCCOPY)) {
            if (hbmpSav) win32::SelectObject(hdcMem, hbmpSav);
            win32::DeleteDC(hdcMem); if (hbmp) win32::DeleteObject(hbmp);
            win32::ReleaseDC(hwndButtons, hdc); win32::GlobalUnlock(app.lpui->hdibToolbar);
            return -1;
        }
        win32::SelectObject(hdcMem, hbmpSav); win32::DeleteDC(hdcMem);
        win32::ReleaseDC(hwndButtons, hdc); win32::GlobalUnlock(app.lpui->hdibToolbar);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hbmp));
        add.hInst = nullptr; add.nID = reinterpret_cast<UINT_PTR>(hbmp);
        if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_ADDBITMAP, 18, reinterpret_cast<LPARAM>(&add)) < 0) return -1;
        c = 0; iRadar = -1;
        for (i = 0; i < 29; i++) {
            itb = static_cast<ToolbarButton>(static_cast<std::int8_t>(vrgTBBtn[i]));
            if (itb == tbShipDesignFilterMenu || itb == tbEnemyClassFilterMenu) continue;
            rgbtn[c].fsState = win32::TBSTATE_ENABLED;
            rgbtn[c].idCommand = IDC_STARS_TOOLBUTTON_FIRST + itb;
            rgbtn[c].iString = -1;
            if (itb < tbNormalView) {
                rgbtn[c].fsStyle = win32::BTNS_SEP;
                rgbtn[c].iBitmap = DxOfBtn(app, itb);
                if (itb == tbScannerRange) iRadar = c;
            } else {
                rgbtn[c].iBitmap = itb;
                TextIO().pfnCchGetString(app, static_cast<StringId>(idsNormalView + static_cast<int>(itb)), std::span{rgszTip[itb]});
                rgbtn[c].iString = reinterpret_cast<INT_PTR>(rgszTip[itb]);
                rgbtn[c].fsStyle = itb <= tbNoPlayerInfoView ? win32::BTNS_CHECKGROUP : win32::BTNS_CHECK;
                if (itb == tbShipDesignFilter || itb == tbEnemyClassFilter) rgbtn[c].fsStyle |= win32::BTNS_DROPDOWN;
                if (itb == tbMineFields || itb == tbZoomMenu) rgbtn[c].fsStyle = win32::BTNS_WHOLEDROPDOWN;
            }
            c++;
        }
        if (!ControlIO().pfnSendMessage(hwndButtons, win32::TB_ADDBUTTONS, c, reinterpret_cast<LPARAM>(rgbtn))) return -1;
        if (iRadar < 0 || !ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETITEMRECT, iRadar, reinterpret_cast<LPARAM>(&rc))) return -1;
        app.lpui->hwndTBRadar = ui::CreateWindowEx(0, "COMBOBOX", nullptr, win32::CBS_DROPDOWN | win32::CBS_AUTOHSCROLL | win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_VSCROLL,
            rc.left, (28 - app.state.dyArial8 - 8) / 2 + 4, rc.right - rc.left, 11 * app.state.dyArial8 + 28, hwnd, nullptr, app.lpui->hInst, nullptr);
        if (!app.lpui->hwndTBRadar) return -1;
        ControlIO().pfnSetWindowPos(app.lpui->hwndTBRadar, win32::HWND_TOP, 0, 0, 0, 0, win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOACTIVATE);
        ControlIO().pfnSendMessage(app.lpui->hwndTBRadar, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        iSel = -1;
        for (j = 0; j < 10; j++) {
            pct = 100 - 10 * j;
            if (pct == app.lpui->vpctRadarView) iSel = j;
            CchSprintf(app.state.szWork, app.state.PCTDPCTPCT, pct);
            ControlIO().pfnSendMessage(app.lpui->hwndTBRadar, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
        }
        ControlIO().pfnSendMessage(app.lpui->hwndTBRadar, win32::CB_SETEXTENDEDUI, 4, 0);
        ControlIO().pfnSendMessage(app.lpui->hwndTBRadar, win32::CB_SETCURSEL, iSel, 0);
        CchSprintf(app.state.szWork, app.state.PCTDPCTPCT, app.lpui->vpctRadarView); ControlIO().pfnSetWindowText(app.lpui->hwndTBRadar, app.state.szWork);
        app.lpui->lpfnRealComboProc = reinterpret_cast<WNDPROC>(win32::GetWindowLongPtrA(app.lpui->hwndTBRadar, win32::GWLP_WNDPROC));
        win32::SetWindowLongPtrA(app.lpui->hwndTBRadar, win32::GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(app.lpui->lpfnFakeComboProc));
        hwndCE = win32::GetWindow(app.lpui->hwndTBRadar, win32::GW_CHILD);
        if (hwndCE) {
            app.lpui->lpfnRealCEProc = reinterpret_cast<WNDPROC>(win32::GetWindowLongPtrA(hwndCE, win32::GWLP_WNDPROC));
            win32::SetWindowLongPtrA(hwndCE, win32::GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(app.lpui->lpfnFakeCEProc));
        }
        SyncToolbar(app, hwnd);
        return 0;
    case win32::WM_NCDESTROY:
        hbmp = reinterpret_cast<HBITMAP>(win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA));
        if (hbmp) win32::DeleteObject(hbmp);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, 0);
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    case win32::WM_MOUSEMOVE:
        if (hwnd == app.lpui->hwndTBRadar || win32::GetParent(hwnd) == app.lpui->hwndTBRadar) {
            win32::GetWindowRect(app.lpui->hwndTBRadar, &rc);
            ShowTooltip(app, idsScannerEffective, &rc);
        }
        return 0;
    case win32::WM_SIZE:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ui::MoveWindow(hwndButtons, 0, 2, rc.right, 32, 1);
        return 0;
    case win32::WM_COMMAND:
        if (reinterpret_cast<HWND>(lParam) == hwndButtons) {
            itb = static_cast<ToolbarButton>(LOWORD(wParam) - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb >= tbNormalView && itb <= tbShipCounts) {
                ExecuteButton(app, itb, itb <= tbNoPlayerInfoView || !FIsButtonDown(app, itb));
                SyncToolbar(app, hwnd);
            }
            return 0;
        }
        if (reinterpret_cast<HWND>(lParam) == app.lpui->hwndTBRadar && HIWORD(wParam) == win32::CBN_KILLFOCUS) win32::PostMessageA(hwnd, 1524, 0, 0);
        return 0;
    case win32::WM_NOTIFY:
        lpnm = reinterpret_cast<NMTOOLBAR *>(lParam);
        if (lpnm && lpnm->hdr.hwndFrom == hwndButtons && lpnm->hdr.code == win32::TBN_DROPDOWN) {
            itb = static_cast<ToolbarButton>(lpnm->iItem - IDC_STARS_TOOLBUTTON_FIRST);
            if (itb == tbShipDesignFilter) itb = tbShipDesignFilterMenu;
            else if (itb == tbEnemyClassFilter) itb = tbEnemyClassFilterMenu;
            ExecuteButton(app, itb, !FIsButtonDown(app, itb));
            SyncToolbar(app, hwnd);
            return win32::TBDDRET_DEFAULT;
        }
        return 0;
    case 1524: TerminateToolbarFocus(app, 0); return 0;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc); win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace); return 1;
    case win32::WM_CTLCOLORMSGBOX: case win32::WM_CTLCOLOREDIT: case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN: case win32::WM_CTLCOLORDLG: case win32::WM_CTLCOLORSCROLLBAR: case win32::WM_CTLCOLORSTATIC:
        win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace); return reinterpret_cast<LRESULT>(app.lpui->hbrButtonFace);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps); ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawToolbar(app, hdc, &rc); win32::EndPaint(hwnd, &ps); return 0;
    }
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawToolbar(APPSESSION &app, HDC hdc, RECT *prc) {
    static_cast<void>(hdc); static_cast<void>(prc);
    SyncToolbar(app, app.lpui->hwndTb);
}

void DrawBitmapButton(APPSESSION &app, HDC hdc, POINT16 pt, ToolbarButton ibtn, std::int16_t fDown) {
    static_cast<void>(hdc); static_cast<void>(pt);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR), win32::TB_CHECKBUTTON, IDC_STARS_TOOLBUTTON_FIRST + ibtn, fDown);
}

std::int16_t ItbFromPpt(APPSESSION &app, POINT16 *ppt) {
    std::int16_t i;
    HWND hwndButtons = ControlIO().pfnGetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {ppt->x, ppt->y - 2};
    RECT rc;
    TBBUTTON btn;

    i = static_cast<std::int16_t>(ControlIO().pfnSendMessage(hwndButtons, win32::TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));
    if (i < 0) {
        i = -i - 1;
    }
    if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&btn)) &&
        ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rc)) && win32::PtInRect(&rc, pt)) {
        ppt->x = rc.left; ppt->y = rc.top + 2;
        return btn.idCommand - IDC_STARS_TOOLBUTTON_FIRST;
    }
    return -1;
}

std::int16_t DxOfBtn(APPSESSION &app, ToolbarButton itb) {
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
        if (app.state.dyArial8 < 16) {
            return 60;
        }
        return 70;
    default:
        return 0;
    }
}

std::int16_t FIsButtonDown(APPSESSION &app, ToolbarButton itb) {
    switch (itb) {
    default:
        break;
    case tbNormalView:
    case tbSurfaceMineralView:
    case tbMineralConcView:
    case tbPlanetValueView:
    case tbPopulationView:
    case tbNoPlayerInfoView:
        return (app.lpui->grbitScan & grbitScanViewMask) == itb;
    case tbAddWaypoints:
        return (app.lpui->grbitScan & grbitScanAddWaypoints) != 0;
    case tbScannerCoverage:
        return (app.lpui->grbitScan & grbitScanCoverage) != 0;
    case tbMineFields:
        return (app.lpui->grbitScan & grbitScanMineFields) != 0 && app.lpui->grbitScanMines == 15;
    case tbFleetPaths:
        return (app.lpui->grbitScan & grbitScanFleetPaths) != 0;
    case tbIdleFleets:
        return (app.lpui->grbitScan & grbitScanIdleFleets) != 0;
    case tbPlanetNames:
        return (app.lpui->grbitScan & grbitScanPlanetNames) != 0;
    case tbShipCounts:
        return (app.lpui->grbitScan & grbitScanShipCounts) != 0;
    case tbShipDesignFilter:
        return (app.lpui->grbitScan & grbitScanDesignFilter) != 0;
    case tbEnemyClassFilter:
        return (app.lpui->grbitScan & grbitScanEnemyFilter) != 0;
    case tbShipDesignFilterMenu:
    case tbEnemyClassFilterMenu:
    case tbZoomMenu:
        return 0;
    }
    return 0;
}

static POINT16 PtToolbarMenu(APPSESSION &app, ToolbarButton itb) {
    HWND hwndButtons = ControlIO().pfnGetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR);
    POINT pt = {0, 0};
    RECT rc;

    if (itb == tbShipDesignFilterMenu) itb = tbShipDesignFilter;
    else if (itb == tbEnemyClassFilterMenu) itb = tbEnemyClassFilter;
    if (ControlIO().pfnSendMessage(hwndButtons, win32::TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + itb, reinterpret_cast<LPARAM>(&rc))) {
        pt.x = rc.left; pt.y = rc.bottom;
        win32::MapWindowPoints(hwndButtons, app.lpui->hwndTb, &pt, 1);
    }
    return PointTo16(pt);
}

void ExecuteButton(APPSESSION &app, ToolbarButton itb, std::int16_t fDown) {
    std::uint16_t grbitNew;
    POINT16  pt;
    char    *rgszScan[12];
    std::int16_t  c;
    std::int16_t  i;
    std::uint16_t grbit;
    std::int32_t  rgid[12];
    std::int16_t  iSel;
    std::uint16_t grbitSh;
    std::int16_t  ish;

    app.state.gd.fChgScanner = 1;
    switch (itb) {
    case tbNormalView:
    case tbSurfaceMineralView:
    case tbMineralConcView:
    case tbPlanetValueView:
    case tbPopulationView:
    case tbNoPlayerInfoView:
        if (!fDown)
            return;
        app.lpui->grbitScan = static_cast<GrbitScan>(itb + (app.lpui->grbitScan & grbitScanToggleMask));
        break;
    case tbAddWaypoints:
        grbitNew = 16;
    LBitDiddle:
        if (fDown) {
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitNew);
        } else {
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan & ~grbitNew);
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
        if (!(app.lpui->grbitScan & grbitScanMineFields)) {
            app.lpui->grbitScanMines = 0;
        }
        for (i = idsMineFields; i <= idsMineFields2; i++) {
            if (i == idsMineFields) {
                rgid[c] = static_cast<std::uint32_t>((app.lpui->grbitScanMines == 15));
            } else {
                rgid[c] = static_cast<std::uint32_t>((app.lpui->grbitScanMines == 0));
            }
            TextIO().pfnCchGetString(app, static_cast<StringId>(i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>((i - idsMineFields) * 30 + 160), 30));
            rgszScan[c++] = &app.state.szWork[(i - idsMineFields) * 30 + 160];
        }
        rgid[c] = 0;
        app.state.szWork[250] = -1;
        app.state.szWork[251] = 0;
        rgszScan[c++] = &app.state.szWork[250];
        for (i = 0; i < 4; i++) {
            rgid[c] = static_cast<std::uint32_t>(((1 << i & app.lpui->grbitScanMines) != 0));
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsMineFields3 + i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i * 30), 30));
            rgszScan[c++] = &app.state.szWork[i * 30];
        }
        pt = PtToolbarMenu(app, itb);
        iSel = ControlIO().pfnPopupMenu(app, app.lpui->hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
        if (iSel == -1)
            return;
        if (iSel >= 3) {
            iSel -= 3;
            app.lpui->grbitScanMines ^= 1 << iSel;
        } else if (iSel == 0) {
            app.lpui->grbitScanMines = 15;
        } else {
            app.lpui->grbitScanMines = 0;
        }
        if (app.lpui->grbitScanMines != 0) {
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitScanMineFields);
        } else {
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan & 0xffbf);
        }
        ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
        break;
    case tbShipDesignFilterMenu:
        c = 0;
        for (i = idsDesigns; i <= idsDesigns2; i++) {
            rgid[c] = 0;
            TextIO().pfnCchGetString(app, static_cast<StringId>(i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>((i - idsDesigns) * 20), 20));
            rgszScan[c++] = &app.state.szWork[(i - idsDesigns) * 20];
        }
        rgid[c] = 0;
        app.state.szWork[200] = -1;
        app.state.szWork[201] = 0;
        rgszScan[c++] = &app.state.szWork[200];
        ish = 0;
        grbitSh = 1;
        while (ish < 16) {
            if (!app.state.rgshdef[ish].fFree) {
                rgid[c] = static_cast<std::uint32_t>(((grbitSh & app.lpui->grbitScanShip) != 0));
                rgszScan[c++] = app.state.rgshdef[ish].hul.szClass;
            }
            ish++;
            grbitSh *= 2;
        }
        pt = PtToolbarMenu(app, itb);
        iSel = ControlIO().pfnPopupMenu(app, app.lpui->hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
        if (iSel == -1)
            return;
        if (iSel < 4) {
            if (iSel == 0) {
                app.lpui->grbitScanShip = 0xffff;
            } else if (iSel == 1) {
                app.lpui->grbitScanShip ^= 0xffff;
            } else {
                app.lpui->grbitScanShip = 0;
            }
            if (!(app.lpui->grbitScan & grbitScanDesignFilter) && app.lpui->grbitScanShip != 0)
                goto LInvalS;
        } else {
            iSel -= 4;
            for (ish = 0; ish < 16; ish++) {
                if (!app.state.rgshdef[ish].fFree) {
                    iSel--;
                    if (iSel < 0)
                        break;
                }
            }
            app.lpui->grbitScanShip ^= 1 << ish;
            if (!(app.lpui->grbitScan & grbitScanDesignFilter) && (1 << ish & app.lpui->grbitScanShip)) {
            LInvalS:
                app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitScanDesignFilter);
                ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
            }
        }
        if (!(app.lpui->grbitScan & grbitScanDesignFilter))
            return;
        break;
    case tbEnemyClassFilterMenu:
        grbit = 1;
        c = 0;
        for (i = idsDesigns; i <= idsDesigns2; i++) {
            rgid[c] = 0;
            TextIO().pfnCchGetString(app, static_cast<StringId>(i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>((i - idsDesigns) * 25 + 200), 25));
            rgszScan[c++] = &app.state.szWork[(i - idsDesigns) * 25 + 200];
        }
        rgid[c] = 0;
        app.state.szWork[300] = -1;
        app.state.szWork[301] = 0;
        rgszScan[c++] = &app.state.szWork[300];
        for (i = 0; i < 8; i++) {
            rgid[c] = static_cast<std::uint32_t>(((1 << i & app.lpui->grbitScanEShip) != 0));
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsColony + i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i * 25), 25));
            rgszScan[c++] = &app.state.szWork[i * 25];
        }
        pt = PtToolbarMenu(app, itb);
        iSel = ControlIO().pfnPopupMenu(app, app.lpui->hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
        if (iSel == -1)
            return;
        if (iSel < 4) {
            if (iSel == 0) {
                app.lpui->grbitScanEShip = 0xff;
            } else if (iSel == 1) {
                app.lpui->grbitScanEShip ^= 0xff;
            } else {
                app.lpui->grbitScanEShip = 0;
            }
            if (!(app.lpui->grbitScan & grbitScanEnemyFilter) && app.lpui->grbitScanEShip != 0)
                goto LInvalE;
        } else {
            iSel -= 4;
            app.lpui->grbitScanEShip ^= 1 << iSel;
            if (!(app.lpui->grbitScan & grbitScanEnemyFilter) && (1 << iSel & app.lpui->grbitScanEShip)) {
            LInvalE:
                app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitScanEnemyFilter);
                ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
            }
        }
        if (!(app.lpui->grbitScan & grbitScanEnemyFilter))
            return;
        break;
    case tbZoomMenu:
        c = 0;
        for (i = 0; i < 9; i++) {
            rgid[c] = static_cast<std::uint32_t>((app.state.iScanZoom + 4 == i));
            CchSprintf(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i * 8)), app.state.PCTDPCTPCT, vrgpctZoom[i]);
            rgszScan[c++] = &app.state.szWork[i * 8];
        }
        pt = PtToolbarMenu(app, itb);
        iSel = ControlIO().pfnPopupMenu(app, app.lpui->hwndTb, pt.x, pt.y, c, rgid, rgszScan, -2, 0);
        if (iSel != -1) {
            CommandHandler(app, app.lpui->hwndFrame, iSel + 3901);
        }
        return;
    default:
        return;
    }
    if (itb != tbAddWaypoints) {
        ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
    }
    if (app.state.gd.fTutorial) {
        PresentationIO().pfnTutor(app);
    }
}

void TerminateToolbarFocus(APPSESSION &app, std::int16_t fCancel) {
    std::int16_t pct;
    char   *psz;

    app.state.gd.fChgScanner = 1;
    if (!fCancel) {
        ControlIO().pfnGetWindowText(app.lpui->hwndTBRadar, app.state.szWork, 20);
        psz = app.state.szWork;
        pct = 0;
        for (; *psz >= '0' && *psz <= '9'; psz++) {
            pct = 10 * pct + (*psz - '0');
        }
        if (*psz != 0 && *psz != '%') {
            pct = 0;
        }
    } else {
        pct = app.lpui->vpctRadarView;
    }
    if (pct < 2) {
        pct = 2;
    } else if (pct > 100) {
        pct = 100;
    }
    ControlIO().pfnSendMessage(app.lpui->hwndTBRadar, win32::CB_SETCURSEL, static_cast<std::int16_t>((100 - pct)) / 10, 0);
    CchSprintf(app.state.szWork, app.state.PCTDPCTPCT, pct);
    ControlIO().pfnSetWindowText(app.lpui->hwndTBRadar, app.state.szWork);
    if (pct != app.lpui->vpctRadarView) {
        app.lpui->vpctRadarView = pct;
        if (!(app.lpui->grbitScan & grbitScanCoverage)) {
            ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
        }
        app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitScanCoverage);
        ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
    }
    ControlIO().pfnSetFocus(app.lpui->hwndFrame);
    return;
}

void ShowTooltip(APPSESSION &app, StringId ids, RECT *prc) {
    HDC     hdc;
    HFONT   hfontSav;
    std::int16_t fVisCur;
    std::int16_t cch;
    std::int16_t fShowNow;

    fVisCur = app.lpui->hwndTooltip && win32::IsWindowVisible(app.lpui->hwndTooltip) != 0;
    if (static_cast<std::int16_t>(ids) < idsUniverseDefinitionFileSeemsMissingCorrupt || !prc) {
        if (app.lpui->hwndTooltip) {
            ControlIO().pfnDestroyWindow(app.lpui->hwndTooltip);
        }
        if (fVisCur) {
            app.lpui->vtickTooltipLast = win32::GetTickCount();
        }
    } else if (ids != app.lpui->vidsTooltip || win32::EqualRect(prc, &app.lpui->vrcTooltip) == 0) {
        app.lpui->vidsTooltip = ids;
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(app.lpui->vidsTooltip), std::span{app.state.szWork});
        app.lpui->vrcTooltip = *prc;
        hdc = win32::GetDC(nullptr);
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[0]));
        app.lpui->dxTip = LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
        win32::SelectObject(hdc, hfontSav);
        win32::ReleaseDC(nullptr, hdc);
        if (!app.lpui->hwndTooltip) {
            ui::CreateWindowEx(win32::WS_EX_NOACTIVATE, app.lpui->szTooltip, nullptr, win32::WS_POPUP, prc->left, prc->bottom + 2,
                           app.lpui->dxTip + 6, app.state.dyArial8 + 6, app.lpui->hwndTb, nullptr, app.lpui->hInst, nullptr);
        } else {
            ControlIO().pfnInvalidateRect(app.lpui->hwndTooltip, nullptr, 1);
        }
        ControlIO().pfnSetWindowPos(app.lpui->hwndTooltip, reinterpret_cast<HWND>(-1), 0, 0, app.lpui->dxTip + 6, app.state.dyArial8 + 6, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_NOOWNERZORDER);
        fShowNow = app.lpui->vtickTooltipLast + 400 >= win32::GetTickCount() || fVisCur != 0;
        TooltipWndProc(app.lpui->hwndTooltip, 1523, fShowNow, 0);
    }
    return;
}

LRESULT  TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    POINT16     pt;
    PAINTSTRUCT ps;
    RECT        rc;
    std::int16_t     bkSav;
    std::int16_t     cch;

    switch (msg) {
    case win32::WM_CREATE:
        app.lpui->hwndTooltip = hwnd;
        break;
    case win32::WM_DESTROY:
        app.lpui->hwndTooltip = nullptr;
        if (app.lpui->vidTimerTooltip != -1) {
            win32::KillTimer(hwnd, app.lpui->vidTimerTooltip);
            app.lpui->vidTimerTooltip = -1;
        }
        app.lpui->vidsTooltip = -1;
        break;
    case win32::WM_MOUSEMOVE:
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ControlIO().pfnDestroyWindow(hwnd);
        return 0;
    case 1523:
        if (wParam == 0) {
            if (app.lpui->vidTimerTooltip != -1) {
                win32::KillTimer(hwnd, app.lpui->vidTimerTooltip);
            }
            if (win32::SetTimer(hwnd, 926, 700, nullptr) != 0) {
                app.lpui->vidTimerTooltip = 926;
            } else {
                app.lpui->vidTimerTooltip = -1;
            }
            return 0;
        }
        wParam = 926;
        /* fallthrough */
    case win32::WM_TIMER:
        if (wParam != 926)
            break;
        if (msg != win32::WM_TIMER || win32::IsWindowVisible(hwnd) == 0) {
            app.lpui->vtickTooltip1stVis = win32::GetTickCount();
            GetCursorPos16(&pt);
            if (win32::PtInRect(&app.lpui->vrcTooltip, PointFrom16(pt)) == 0)
                goto LKillTip;
            // Anchor to the hovered control, independently of the pointer's
            // position inside it; keep the complete tip within the frame.
            win32::GetWindowRect(app.lpui->hwndFrame, &rc);
            pt.x = (((rc.left) > (StarsMin(app.lpui->vrcTooltip.left, rc.right - app.lpui->dxTip - 6))) ? (rc.left) : (StarsMin(app.lpui->vrcTooltip.left, rc.right - app.lpui->dxTip - 6)));
            pt.y = app.lpui->vrcTooltip.bottom + 2;
            if (pt.y + app.state.dyArial8 + 6 > rc.bottom) {
                pt.y = StarsMax(rc.top, app.lpui->vrcTooltip.top - app.state.dyArial8 - 8);
            }
            ControlIO().pfnSetWindowPos(hwnd, reinterpret_cast<HWND>(-1), pt.x, pt.y, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW | win32::SWP_NOOWNERZORDER);
            ControlIO().pfnUpdateWindow(hwnd);
            if (app.lpui->vidTimerTooltip != -1) {
                win32::KillTimer(hwnd, app.lpui->vidTimerTooltip);
            }
            if (win32::SetTimer(hwnd, 926, 50, nullptr) != 0) {
                app.lpui->vidTimerTooltip = 926;
            } else {
                app.lpui->vidTimerTooltip = -1;
            }
            return 0;
        } else {
            app.lpui->vtickTooltipLast = win32::GetTickCount();
            GetCursorPos16(&pt);
            if (win32::PtInRect(&app.lpui->vrcTooltip, PointFrom16(pt)) == 0 || app.lpui->vtickTooltip1stVis + 10000 < app.lpui->vtickTooltipLast) {
            LKillTip:
                ControlIO().pfnDestroyWindow(hwnd);
            }
            return 0;
        }
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrTooltip);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FrameRect(hdc, &rc, app.lpui->hbrWindowFrame);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(app.lpui->vidsTooltip), std::span{app.state.szWork});
        bkSav = win32::SetBkMode(hdc, win32::TRANSPARENT);
        win32::SetTextColor(hdc, app.lpui->crWindowText);
        win32::ExtTextOutA(hdc, 3, 3, 0, nullptr, app.state.szWork, cch, nullptr);
        win32::SetBkMode(hdc, bkSav);
        win32::EndPaint(hwnd, &ps);
        return 0;
    }
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT  FakeComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    switch (msg) {
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ShowTooltip(app, static_cast<StringId>(idsNoString), nullptr);
        break;
    case win32::WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return ui::CallWindowProc(app.lpui->lpfnRealComboProc, hwnd, msg, wParam, lParam);
}

LRESULT  FakeCEProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    switch (msg) {
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        ShowTooltip(app, static_cast<StringId>(idsNoString), nullptr);
        break;
    case win32::WM_MOUSEMOVE:
        TbWndProc(hwnd, msg, wParam, lParam);
    }
    return ui::CallWindowProc(app.lpui->lpfnRealCEProc, hwnd, msg, wParam, lParam);
}
