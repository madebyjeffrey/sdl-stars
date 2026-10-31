#include "win.h"
#include "fleet_query_io.h"
#include "text_io.h"
#include "control_io.h"
#include "alert_io.h"
#include "desktop_choice_io.h"
#include "uistate.h"
#include "ownership.h"
#include <callbackcontext.h>
#include <stars_compat.h>

static int IStarsMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) try {
    win32::MSG      msg;
    uint32_t lSeed;
    int16_t  fSeed;

    hInst = hInstance;
    szBase[0] = 0;
    ini.SetWFlags(0);
    memset(&tutor, 0, sizeof(TUTOR));
    memset(&vtimer, 0, sizeof(TIMER));
    vtimer.fAutoGenWhenIn = 1;
    if (!hPrevInstance && InitMDIApp() == 0) {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableInitializeStars, NULL), win32::MB_ICONHAND);
        return 0;
    }
    fSeed = 0;
    lSeed = 0;
    if (!FCreateStuff()) {
        return 0;
    }
    if (!FGetSystemColors()) {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableInitializeStars, NULL), win32::MB_ICONHAND);
        return 0;
    }
    if (DesktopChoiceIO().pfnInitInstance(nCmdShow) == 0) {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableInitializeStars, NULL), win32::MB_ICONHAND);
        return 0;
    }
    if (!ParseCmdLine(lpCmdLine, &fSeed, &lSeed)) { FreeStuff(); return 2; }
    /* The original seeded before FCreateStuff; nothing in between draws a
       random number, so seeding after the command line is the same. */
    Randomize2(fSeed ? lSeed : win32::GetTickCount());
    StarsRethrowPendingError();
    win32::PostMessageA(hwndFrame, WM_STARS_STARTUP, 0, 0);
    while (ui::GetMessage(&msg, NULL, 0, 0)) {
        StarsRethrowPendingError();
        if ((msg.message == win32::WM_MOUSEWHEEL || msg.message == win32::WM_MOUSEHWHEEL) && !FRouteMouseWheel(&msg))
            continue;
        if (hwndTitle) {
            if (ui::TranslateAccelerator(hwndFrame, hAccelTitle, &msg) == 0) {
                win32::TranslateMessage(&msg);
                ui::DispatchMessage(&msg);
            }
        } else if (win32::IsIconic(hwndFrame) != 0 || ui::TranslateAccelerator(hwndFrame, hAccel, &msg) == 0) {
            win32::TranslateMessage(&msg);
            if (((msg.message != win32::WM_KEYDOWN && msg.message != win32::WM_KEYUP) || !FHandleKey(msg.hwnd, msg.message, msg.wParam, msg.lParam)) &&
                (msg.message != win32::WM_CHAR || !FHandleChar(msg.hwnd, msg.wParam, msg.lParam))) {
                ui::DispatchMessage(&msg);
            }
        }
    }
    StarsRethrowPendingError();
    CloseScannerInspector();
    ClosePlanetPane();
    FreeStuff();
    return (int16_t)msg.wParam;
} catch (const StarsError &error) {
    StarsPrimaryErrorScope primary;
    try {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(static_cast<StringId>(error.ids ? error.ids : idsMemory), NULL), win32::MB_ICONHAND);
        FreeStuff();
    } catch (...) {
    }
    return 2;
} catch (...) {
    StarsPrimaryErrorScope primary;
    try {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableInitializeStars, NULL), win32::MB_ICONHAND);
        FreeStuff();
    } catch (...) {
    }
    return 2;
}

int16_t FGetSystemColors() {
    HDC         hdc;
    BITMAPINFO *lpbi;

    if (hbrButtonFace) {
        FreeHbr(hbrButtonFace);
    }
    if (hbrButtonHilite) {
        FreeHbr(hbrButtonHilite);
    }
    if (hbrButtonShadow) {
        FreeHbr(hbrButtonShadow);
    }
    if (hbrButtonText) {
        FreeHbr(hbrButtonText);
    }
    if (hbrWindowText) {
        FreeHbr(hbrWindowText);
    }
    if (hbrWindow) {
        FreeHbr(hbrWindow);
    }
    if (hbrWindowFrame) {
        FreeHbr(hbrWindowFrame);
    }
    if (hbrDesktop) {
        FreeHbr(hbrDesktop);
    }
    crButtonFace = win32::GetSysColor(win32::COLOR_BTNFACE);
    hbrButtonFace = HbrGet(crButtonFace);
    crButtonHilite = win32::GetSysColor(win32::COLOR_BTNHIGHLIGHT);
    hbrButtonHilite = HbrGet(crButtonHilite);
    crButtonShadow = win32::GetSysColor(win32::COLOR_BTNSHADOW);
    hbrButtonShadow = HbrGet(crButtonShadow);
    crButtonText = win32::GetSysColor(win32::COLOR_BTNTEXT);
    hbrButtonText = HbrGet(crButtonText);
    hbrWindowFrame = HbrGet(win32::GetSysColor(win32::COLOR_WINDOWFRAME));
    hbrDesktop = HbrGet(win32::GetSysColor(win32::COLOR_BACKGROUND));
    crWindow = win32::GetSysColor(win32::COLOR_WINDOW);
    hbrWindow = HbrGet(crWindow);
    crWindowText = win32::GetSysColor(win32::COLOR_WINDOWTEXT);
    hbrWindowText = HbrGet(crWindowText);
    dyTitleBar = win32::GetSystemMetrics(win32::SM_CYCAPTION);
    dxWinFrame = win32::GetSystemMetrics(win32::SM_CXFRAME);
    dyWinFrame = win32::GetSystemMetrics(win32::SM_CYFRAME);
    if (hdibPlaque) {
        lpbi = (BITMAPINFO *)win32::GlobalLock(hdibPlaque);
        lpbi->bmiColors[249].rgbRed = crButtonFace;
        lpbi->bmiColors[249].rgbGreen = LOWORD(crButtonFace) >> 8;
        lpbi->bmiColors[249].rgbBlue = HIWORD(crButtonFace);
        win32::GlobalUnlock(hdibPlaque);
    }
    if (hdibToolbar) {
        lpbi = (BITMAPINFO *)win32::GlobalLock(hdibToolbar);
        lpbi->bmiColors[253].rgbRed = crButtonFace;
        lpbi->bmiColors[253].rgbGreen = LOWORD(crButtonFace) >> 8;
        lpbi->bmiColors[253].rgbBlue = HIWORD(crButtonFace);
        win32::GlobalUnlock(hdibToolbar);
    }
    hdc = win32::GetDC(NULL);
    vcScreenColors = win32::GetDeviceCaps(hdc, win32::BITSPIXEL) * win32::GetDeviceCaps(hdc, win32::PLANES);
    win32::ReleaseDC(NULL, hdc);
    return 1;
}

void FreeStuff() noexcept {
    if (!StarsCompatCanShutdown() || !FBeginAppUiClose()) return;
    NATIVECLEANUPSCOPE rollback([]() noexcept { CancelAppUiClose(); });
    try {
        try { CloseScannerInspector(); } catch (...) { StarsCapturePendingError(); }
        try { ClosePlanetPane(); } catch (...) { StarsCapturePendingError(); }
        // All remaining native windows, timers and async payloads stop while
        // their borrowed application values and stable heaps are still alive.
        StarsCompatProcessExit();
        ResetScannerInspectorSession();
        CloseLibraryUiSession();
        ResetRaceUiSession();
        ResetScannerInputSession();
        ResetMessageInputSession();
        ResetReportInputSession();
        EndAppUiClose();
    } catch (...) {
        StarsCapturePendingError();
    }
}

int ui::WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&GetAppSession());
    BeginAppUiSession();
    NATIVECLEANUPSCOPE close([]() noexcept { FreeStuff(); });
    int result = IStarsMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    {
        // The inner handlers report primary failures before native UI teardown.
        StarsPrimaryErrorScope primary(result == 2);
        FreeStuff();
    }
    if (FStarsErrorPending()) {
        try { StarsRethrowPendingError(); } catch (...) { result = 2; }
    }
    return result;
}

INT_PTR  About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    APPUISTATE &uiState = static_cast<APPUISESSION &>(app).ui;
    RECT    rc;
    HDC     hdc;
    int16_t i;
    HWND    hwndCtl;
    FARPROC lpProc;

    switch (message) {
    case win32::WM_INITDIALOG:
        uiState.iAbout1st = -11;
        uiState.iAboutPartial = 0;
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_ABOUT_DEMO_TEXT), SzVersion());
        uiState.uTimerId = win32::SetTimer(hwnd, 14, 50, NULL);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, uiState.hbrButtonFace);
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
                RcCtrTextOut(hdc, &rc, TextIO().pfnCompressed(static_cast<StringId>(idsDesignProgramming + i)), -1);
            } else if (i >= 77) {
                break;
            }
            win32::OffsetRect(&rc, 0, app.state.dyArial8);
        }
        rc.bottom = 1000;
        win32::FillRect(hdc, &rc, uiState.hbrButtonFace);
        win32::SelectClipRgn(hdc, NULL);
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
            win32::SetBkColor((HDC)wParam, uiState.crButtonFace);
            return (INT_PTR)uiState.hbrButtonFace;
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

int16_t FHandleChar(HWND hwnd, uint16_t ch, int32_t lParam) {
    HWND hwndF;

    if ((hwndScanner && (ch == 43 || ch == 45)) || (ch == 118 || ch == 86)) {
        hwndF = win32::GetFocus();
        if (!hwndMessage || hwndF != hwndMsgEdit) {
            ControlIO().pfnSendMessage(hwndScanner, win32::WM_CHAR, ch, lParam);
            return 1;
        }
    }
    return 0;
}

int16_t FHandleKey(HWND hwnd, int16_t iMsg, int16_t iKey, uint32_t dw) {
    HWND          hwndF;
    POINT16       pt;
    HWND          hwndOver;
    int16_t       i;
    ToolbarButton itb;
    uint16_t      md;
    int16_t       iWarp;
    int16_t       iwp;

    if (iMsg == win32::WM_KEYDOWN) {
        if (iKey == win32::VK_ESCAPE && hwndBrowser && win32::GetActiveWindow() == hwndBrowser) {
            ControlIO().pfnDestroyWindow(hwndBrowser);
            return 1;
        }
        if (iKey == win32::VK_ESCAPE && hwndPopup) {
            ControlIO().pfnSendMessage(hwndPopup, win32::WM_LBUTTONUP, 0, 0);
            return 1;
        }
        if (iKey == win32::VK_ESCAPE && hwndReportDlg) {
            ControlIO().pfnDestroyWindow(hwndReportDlg);
            return 1;
        }
    } else if (iMsg == win32::WM_KEYUP && hwndTb && (iKey == win32::VK_ESCAPE || iKey == win32::VK_RETURN)) {
        hwndF = win32::GetParent(win32::GetFocus());
        if (hwndF == hwndTb || win32::GetParent(hwndF) == hwndTb) {
            TerminateToolbarFocus(iKey == win32::VK_ESCAPE);
        }
    }
    if (iKey == win32::VK_SHIFT && hwndScanner) {
        GetCursorPos16(&pt);
        hwndOver = win32::WindowFromPoint(PointFrom16(pt));
        if (hwndOver == hwndScanner) {
            ControlIO().pfnSendMessage(hwndOver, win32::WM_SETCURSOR, (WPARAM)hwndOver, 0);
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
        if (hwndMessage) {
            if (hwndTb && (hwndTb == hwndF || win32::GetParent(hwndF) == hwndTb || win32::GetParent(win32::GetParent(hwndF)) == hwndTb)) {
                return 0;
            }
            for (i = 0; i < 3; i++) {
                if (hwndF == rghwndOrderDD[i]) {
                    return 0;
                }
            }
            if (hwndF == hwndFleetCompLB || hwndF == hwndPlanetProdLB || hwndF == hwndMsgEdit || hwndF == hwndMsgDrop || hwndF == hwndOrderED ||
                hwndF == hwndMsgScroll || hwndF == hwndFleetCompLB || hwndF == hwndShipDD) {
                return 0;
            }
            if (hwndBrowser && hwndF == ControlIO().pfnGetDlgItem(hwndBrowser, IDC_BROWSER_COMPONENT_CATEGORY)) {
                return 0;
            }
        }
        if (iKey >= '0' && iKey <= '9') {
            if (iKey >= '1' && iKey <= '6') {
                md = iKey - 49;
                if (md != (grbitScan & grbitScanViewMask)) {
                    ExecuteButton(static_cast<ToolbarButton>(iKey - 49), 1);
                    ControlIO().pfnInvalidateRect(hwndTb, NULL, 0);
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
            ExecuteButton(itb, FIsButtonDown(itb) == 0);
            ControlIO().pfnInvalidateRect(hwndTb, NULL, 0);
            return 1;
        }
        switch (iKey) {
        default:
            return 0;
        case win32::VK_BACK:
        case win32::VK_DELETE:
            if (sel.grobj != grobjFleet)
                break;
            iKey = win32::VK_BACK;
            DeleteCurWayPoint(8);
            break;
        case win32::VK_END:
        case win32::VK_HOME:
        case win32::VK_UP:
        case win32::VK_DOWN:
            if (hwndF == hwndShipLB) {
                return 0;
            }
            ControlIO().pfnSendMessage(hwndMessage, win32::WM_KEYDOWN, iKey, dw);
            break;
        case win32::VK_OEM_COMMA:
        case win32::VK_OEM_PERIOD:
            if (sel.grobj == grobjFleet && (sel.iwpAct > 0 || sel.fl.cord > 1)) {
                iwp = sel.iwpAct <= 0 ? 1 : sel.iwpAct;
                iWarp = sel.fl.lpplord->rgord()[iwp].iWarp;
                if (iKey == win32::VK_OEM_COMMA) {
                    iWarp--;
                } else {
                    iWarp++;
                }
                if (iWarp >= 0 && iWarp <= 11) {
                    SetScanPathWarp(iwp, iWarp);
                    FleetQueryIO().pfnLookup(idWriteBack, &sel.fl);
                    ControlIO().pfnDrawPlanShip(NULL, static_cast<TileBits>(tileFleetOrders | tileFleetComp | tileMinimized));
                }
            }
            return 1;
        case win32::VK_OEM_4:
        case win32::VK_OEM_6:
            pt.x = 0;
            pt.y = 0;
            ExecuteReportClick(pt, rptEnemyFleets, 0, iKey == win32::VK_OEM_4 ? -2 : -1);
            return 1;
        }
        return 1;
    }
}
