#include "win.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "fleet_query_io.h"
#include "text_io.h"
#include "control_io.h"
#include "alert_io.h"
#include "desktop_choice_io.h"
#include "uistate.h"
#include "ownership.h"
#include <callbackcontext.h>
#include <stars_compat.h>

static int IStarsMain(APPSESSION &app, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) try {
    win32::MSG      msg;
    std::uint32_t lSeed;
    std::int16_t  fSeed;

    app.lpui->hInst = hInstance;
    app.state.szBase[0] = 0;
    app.state.ini.SetWFlags(0);
    app.state.tutor = {};
    app.lpui->vtimer = {};
    app.lpui->vtimer.fAutoGenWhenIn = 1;
    if (!hPrevInstance && InitMDIApp(app) == 0) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableInitializeStars, nullptr).data(), win32::MB_ICONHAND);
        return 0;
    }
    fSeed = 0;
    lSeed = 0;
    if (!FCreateStuff(app)) {
        return 0;
    }
    if (!FGetSystemColors(app)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableInitializeStars, nullptr).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (DesktopChoiceIO().pfnInitInstance(app, nCmdShow) == 0) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableInitializeStars, nullptr).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (!ParseCmdLine(app, lpCmdLine, &fSeed, &lSeed)) { FreeStuff(app); return 2; }
    /* The original seeded before FCreateStuff; nothing in between draws a
       random number, so seeding after the command line is the same. */
    Randomize2(app, fSeed ? lSeed : win32::GetTickCount());
    StarsRethrowPendingError();
    win32::PostMessageA(app.lpui->hwndFrame, WM_STARS_STARTUP, 0, 0);
    while (ui::GetMessage(&msg, nullptr, 0, 0)) {
        StarsRethrowPendingError();
        if ((msg.message == win32::WM_MOUSEWHEEL || msg.message == win32::WM_MOUSEHWHEEL) && !FRouteMouseWheel(&msg))
            continue;
        if (app.lpui->hwndTitle) {
            if (ui::TranslateAccelerator(app.lpui->hwndFrame, app.lpui->hAccelTitle, &msg) == 0) {
                win32::TranslateMessage(&msg);
                ui::DispatchMessage(&msg);
            }
        } else if (win32::IsIconic(app.lpui->hwndFrame) != 0 || ui::TranslateAccelerator(app.lpui->hwndFrame, app.lpui->hAccel, &msg) == 0) {
            win32::TranslateMessage(&msg);
            if (((msg.message != win32::WM_KEYDOWN && msg.message != win32::WM_KEYUP) || !FHandleKey(app, msg.hwnd, msg.message, msg.wParam, msg.lParam)) &&
                (msg.message != win32::WM_CHAR || !FHandleChar(app, msg.hwnd, msg.wParam, msg.lParam))) {
                ui::DispatchMessage(&msg);
            }
        }
    }
    StarsRethrowPendingError();
    CloseScannerInspector();
    ClosePlanetPane(app);
    FreeStuff(app);
    return static_cast<std::int16_t>(msg.wParam);
} catch (const StarsError &error) {
    StarsPrimaryErrorScope primary;
    try {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, static_cast<StringId>(error.ids ? error.ids : idsMemory), nullptr).data(), win32::MB_ICONHAND);
        FreeStuff(app);
    } catch (...) {
    }
    return 2;
} catch (...) {
    StarsPrimaryErrorScope primary;
    try {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableInitializeStars, nullptr).data(), win32::MB_ICONHAND);
        FreeStuff(app);
    } catch (...) {
    }
    return 2;
}

std::int16_t FGetSystemColors(APPSESSION &app) {
    HDC         hdc;
    BITMAPINFO *lpbi;

    if (app.lpui->hbrButtonFace) {
        FreeHbr(app, app.lpui->hbrButtonFace);
    }
    if (app.lpui->hbrButtonHilite) {
        FreeHbr(app, app.lpui->hbrButtonHilite);
    }
    if (app.lpui->hbrButtonShadow) {
        FreeHbr(app, app.lpui->hbrButtonShadow);
    }
    if (app.lpui->hbrButtonText) {
        FreeHbr(app, app.lpui->hbrButtonText);
    }
    if (app.lpui->hbrWindowText) {
        FreeHbr(app, app.lpui->hbrWindowText);
    }
    if (app.lpui->hbrWindow) {
        FreeHbr(app, app.lpui->hbrWindow);
    }
    if (app.lpui->hbrWindowFrame) {
        FreeHbr(app, app.lpui->hbrWindowFrame);
    }
    if (app.lpui->hbrDesktop) {
        FreeHbr(app, app.lpui->hbrDesktop);
    }
    app.lpui->crButtonFace = win32::GetSysColor(win32::COLOR_BTNFACE);
    app.lpui->hbrButtonFace = HbrGet(app, app.lpui->crButtonFace);
    app.lpui->crButtonHilite = win32::GetSysColor(win32::COLOR_BTNHIGHLIGHT);
    app.lpui->hbrButtonHilite = HbrGet(app, app.lpui->crButtonHilite);
    app.lpui->crButtonShadow = win32::GetSysColor(win32::COLOR_BTNSHADOW);
    app.lpui->hbrButtonShadow = HbrGet(app, app.lpui->crButtonShadow);
    app.lpui->crButtonText = win32::GetSysColor(win32::COLOR_BTNTEXT);
    app.lpui->hbrButtonText = HbrGet(app, app.lpui->crButtonText);
    app.lpui->hbrWindowFrame = HbrGet(app, win32::GetSysColor(win32::COLOR_WINDOWFRAME));
    app.lpui->hbrDesktop = HbrGet(app, win32::GetSysColor(win32::COLOR_BACKGROUND));
    app.lpui->crWindow = win32::GetSysColor(win32::COLOR_WINDOW);
    app.lpui->hbrWindow = HbrGet(app, app.lpui->crWindow);
    app.lpui->crWindowText = win32::GetSysColor(win32::COLOR_WINDOWTEXT);
    app.lpui->hbrWindowText = HbrGet(app, app.lpui->crWindowText);
    app.lpui->dyTitleBar = win32::GetSystemMetrics(win32::SM_CYCAPTION);
    app.lpui->dxWinFrame = win32::GetSystemMetrics(win32::SM_CXFRAME);
    app.lpui->dyWinFrame = win32::GetSystemMetrics(win32::SM_CYFRAME);
    if (app.lpui->hdibPlaque) {
        lpbi = static_cast<BITMAPINFO *>(win32::GlobalLock(app.lpui->hdibPlaque));
        lpbi->bmiColors[249].rgbRed = app.lpui->crButtonFace;
        lpbi->bmiColors[249].rgbGreen = LOWORD(app.lpui->crButtonFace) >> 8;
        lpbi->bmiColors[249].rgbBlue = HIWORD(app.lpui->crButtonFace);
        win32::GlobalUnlock(app.lpui->hdibPlaque);
    }
    if (app.lpui->hdibToolbar) {
        lpbi = static_cast<BITMAPINFO *>(win32::GlobalLock(app.lpui->hdibToolbar));
        lpbi->bmiColors[253].rgbRed = app.lpui->crButtonFace;
        lpbi->bmiColors[253].rgbGreen = LOWORD(app.lpui->crButtonFace) >> 8;
        lpbi->bmiColors[253].rgbBlue = HIWORD(app.lpui->crButtonFace);
        win32::GlobalUnlock(app.lpui->hdibToolbar);
    }
    hdc = win32::GetDC(nullptr);
    app.lpui->vcScreenColors = win32::GetDeviceCaps(hdc, win32::BITSPIXEL) * win32::GetDeviceCaps(hdc, win32::PLANES);
    win32::ReleaseDC(nullptr, hdc);
    return 1;
}

void FreeStuff(APPSESSION &app) noexcept {
    if (!StarsCompatCanShutdown() || !FBeginAppUiClose(app)) return;
    NATIVECLEANUPSCOPE rollback([&app]() noexcept { CancelAppUiClose(app); });
    try {
        try { CloseScannerInspector(); } catch (...) { StarsCapturePendingError(); }
        try { ClosePlanetPane(app); } catch (...) { StarsCapturePendingError(); }
        // All remaining native windows, timers and async payloads stop while
        // their borrowed application values and stable heaps are still alive.
        StarsCompatProcessExit();
        ResetScannerInspectorSession();
        CloseLibraryUiSession();
        ResetRaceUiSession();
        ResetScannerInputSession();
        ResetMessageInputSession();
        ResetReportInputSession();
        EndAppUiClose(app);
    } catch (...) {
        StarsCapturePendingError();
    }
}

int ui::WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    APPSESSION &app = ui::app;

    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    BeginAppUiSession(app);
    NATIVECLEANUPSCOPE close([&app]() noexcept { FreeStuff(app); });
    int result = IStarsMain(app, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    {
        // The inner handlers report primary failures before native UI teardown.
        StarsPrimaryErrorScope primary(result == 2);
        FreeStuff(app);
    }
    if (FStarsErrorPending()) {
        try { StarsRethrowPendingError(); } catch (...) { result = 2; }
    }
    return result;
}

INT_PTR  About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    APPUISTATE &uiState = *app.lpui;
    RECT    rc;
    HDC     hdc;
    std::int16_t i;
    HWND    hwndCtl;

    switch (message) {
    case win32::WM_INITDIALOG:
        uiState.iAbout1st = -11;
        uiState.iAboutPartial = 0;
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_ABOUT_DEMO_TEXT), SzVersion(app).data());
        uiState.uTimerId = win32::SetTimer(hwnd, 14, 50, nullptr);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, uiState.hbrButtonFace);
        return 1;
    case win32::WM_TIMER:
        hwndCtl = ControlIO().pfnGetDlgItem(hwnd, IDC_ABOUT_CREDITS_TEXT);
        uiState.iAboutPartial += 2;
        if (uiState.iAboutPartial >= app.state.dyArial8) {
            uiState.iAboutPartial = 0;
            uiState.iAbout1st++;
            if (uiState.iAbout1st > 78) {
                uiState.iAbout1st = -11;
            }
        }
        ControlIO().pfnGetClientRect(hwndCtl, &rc);
        hdc = win32::GetDC(hwndCtl);
        win32::SelectObject(hdc, uiState.rghfontArial8[1]);
        win32::SetBkMode(hdc, win32::OPAQUE);
        win32::SetBkColor(hdc, uiState.crButtonFace);
        win32::SetTextColor(hdc, uiState.crButtonText);
        win32::IntersectClipRect(hdc, 0, 0, rc.right, rc.bottom);
        rc.top -= uiState.iAboutPartial;
        rc.bottom = rc.top + app.state.dyArial8;
        for (i = uiState.iAbout1st; i < uiState.iAbout1st + 10; i++) {
            if (i >= 0 && i < 77) {
                RcCtrTextOut(app, hdc, &rc, TextIO().pfnCompressed(app, static_cast<StringId>(idsDesignProgramming + i)).data(), -1);
            } else if (i >= 77) {
                break;
            }
            win32::OffsetRect(&rc, 0, app.state.dyArial8);
        }
        rc.bottom = 1000;
        win32::FillRect(hdc, &rc, uiState.hbrButtonFace);
        win32::SelectClipRgn(hdc, nullptr);
        win32::ReleaseDC(hwnd, hdc);
        break;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), uiState.crButtonFace);
            return reinterpret_cast<INT_PTR>(uiState.hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            win32::KillTimer(hwnd, uiState.uTimerId);
            uiState.uTimerId = 0;
            ControlIO().pfnEndDialog(hwnd, 1);
            return 1;
        }
        break;
    }
    return 0;
}

std::int16_t FHandleChar(APPSESSION &app, HWND hwnd, std::uint16_t ch, std::int32_t lParam) {
    HWND hwndF;

    if ((app.lpui->hwndScanner && (ch == 43 || ch == 45)) || (ch == 118 || ch == 86)) {
        hwndF = win32::GetFocus();
        if (!app.lpui->hwndMessage || hwndF != app.lpui->hwndMsgEdit) {
            ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, ch, lParam);
            return 1;
        }
    }
    return 0;
}

std::int16_t FHandleKey(APPSESSION &app, HWND hwnd, std::int16_t iMsg, std::int16_t iKey, std::uint32_t dw) {
    HWND          hwndF;
    POINT16       pt;
    HWND          hwndOver;
    std::int16_t       i;
    ToolbarButton itb;
    std::uint16_t      md;
    std::int16_t       iWarp;
    std::int16_t       iwp;

    if (iMsg == win32::WM_KEYDOWN) {
        if (iKey == win32::VK_ESCAPE && app.lpui->hwndBrowser && win32::GetActiveWindow() == app.lpui->hwndBrowser) {
            ControlIO().pfnDestroyWindow(app.lpui->hwndBrowser);
            return 1;
        }
        if (iKey == win32::VK_ESCAPE && app.lpui->hwndPopup) {
            ControlIO().pfnSendMessage(app.lpui->hwndPopup, win32::WM_LBUTTONUP, 0, 0);
            return 1;
        }
        if (iKey == win32::VK_ESCAPE && app.lpui->hwndReportDlg) {
            ControlIO().pfnDestroyWindow(app.lpui->hwndReportDlg);
            return 1;
        }
    } else if (iMsg == win32::WM_KEYUP && app.lpui->hwndTb && (iKey == win32::VK_ESCAPE || iKey == win32::VK_RETURN)) {
        hwndF = win32::GetParent(win32::GetFocus());
        if (hwndF == app.lpui->hwndTb || win32::GetParent(hwndF) == app.lpui->hwndTb) {
            TerminateToolbarFocus(app, iKey == win32::VK_ESCAPE);
        }
    }
    if (iKey == win32::VK_SHIFT && app.lpui->hwndScanner) {
        GetCursorPos16(&pt);
        hwndOver = win32::WindowFromPoint(PointFrom16(pt));
        if (hwndOver == app.lpui->hwndScanner) {
            ControlIO().pfnSendMessage(hwndOver, win32::WM_SETCURSOR, reinterpret_cast<WPARAM>(hwndOver), 0);
        }
    }
    if (iMsg != win32::WM_KEYDOWN) {
        return 0;
    }
    switch (iKey) {
    default:
        if (iKey < '0' || iKey > '9') {
            switch (iKey) {
            default:
                return 0;
            case win32::VK_OEM_COMMA:
            case win32::VK_OEM_PERIOD:
            case win32::VK_OEM_4:
            case win32::VK_OEM_6:
                break;
            }
        }
        /* fallthrough */
    case win32::VK_BACK:
    case win32::VK_DELETE:
    case win32::VK_DOWN:
    case win32::VK_UP:
    case win32::VK_HOME:
    case win32::VK_END:
        hwndF = win32::GetFocus();
        if (app.lpui->hwndMessage) {
            if (app.lpui->hwndTb && (app.lpui->hwndTb == hwndF || win32::GetParent(hwndF) == app.lpui->hwndTb || win32::GetParent(win32::GetParent(hwndF)) == app.lpui->hwndTb)) {
                return 0;
            }
            for (i = 0; i < 3; i++) {
                if (hwndF == app.lpui->rghwndOrderDD[i]) {
                    return 0;
                }
            }
            if (hwndF == app.lpui->hwndFleetCompLB || hwndF == app.lpui->hwndPlanetProdLB || hwndF == app.lpui->hwndMsgEdit || hwndF == app.lpui->hwndMsgDrop || hwndF == app.lpui->hwndOrderED ||
                hwndF == app.lpui->hwndMsgScroll || hwndF == app.lpui->hwndFleetCompLB || hwndF == app.lpui->hwndShipDD) {
                return 0;
            }
            if (app.lpui->hwndBrowser && hwndF == ControlIO().pfnGetDlgItem(app.lpui->hwndBrowser, IDC_BROWSER_COMPONENT_CATEGORY)) {
                return 0;
            }
        }
        if (iKey >= '0' && iKey <= '9') {
            if (iKey >= '1' && iKey <= '6') {
                md = iKey - 49;
                if (md != (app.lpui->grbitScan & grbitScanViewMask)) {
                    ExecuteButton(app, static_cast<ToolbarButton>(iKey - 49), 1);
                    ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 0);
                }
                return 1;
            }
            switch (iKey) {
            case '7':
                itb = tbScannerCoverage;
                break;
            case '8':
                itb = tbMineFields;
                break;
            case '9':
                itb = tbFleetPaths;
                break;
            case '0':
                if (ControlIO().pfnGetKeyState(win32::VK_SHIFT) < 0) {
                    itb = tbShipCounts;
                } else {
                    itb = tbPlanetNames;
                }
            }
            ExecuteButton(app, itb, FIsButtonDown(app, itb) == 0);
            ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 0);
            return 1;
        }
        switch (iKey) {
        default:
            return 0;
        case win32::VK_BACK:
        case win32::VK_DELETE:
            if (app.state.sel.grobj != grobjFleet)
                break;
            iKey = win32::VK_BACK;
            DeleteCurWayPoint(app, 8);
            break;
        case win32::VK_END:
        case win32::VK_HOME:
        case win32::VK_UP:
        case win32::VK_DOWN:
            if (hwndF == app.lpui->hwndShipLB) {
                return 0;
            }
            ControlIO().pfnSendMessage(app.lpui->hwndMessage, win32::WM_KEYDOWN, iKey, dw);
            break;
        case win32::VK_OEM_COMMA:
        case win32::VK_OEM_PERIOD:
            if (app.state.sel.grobj == grobjFleet && (app.state.sel.iwpAct > 0 || app.state.sel.fl.cord > 1)) {
                iwp = app.state.sel.iwpAct <= 0 ? 1 : app.state.sel.iwpAct;
                iWarp = app.state.sel.fl.lpplord->rgord()[iwp].iWarp;
                if (iKey == win32::VK_OEM_COMMA) {
                    iWarp--;
                } else {
                    iWarp++;
                }
                if (iWarp >= 0 && iWarp <= 11) {
                    SetScanPathWarp(app, iwp, iWarp);
                    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                    ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileFleetOrders | tileFleetComp | tileMinimized));
                }
            }
            return 1;
        case win32::VK_OEM_4:
        case win32::VK_OEM_6:
            pt.x = 0;
            pt.y = 0;
            ExecuteReportClick(app, pt, rptEnemyFleets, 0, iKey == win32::VK_OEM_4 ? -2 : -1);
            return 1;
        }
        return 1;
    }
}
