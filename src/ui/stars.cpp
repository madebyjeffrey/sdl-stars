#include "win.h"
#include "uistate.h"
#include "ownership.h"
#include <stars_compat.h>

static int IStarsMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) try {
    MSG      msg;
    uint32_t lSeed;
    int16_t  fSeed;

    hInst = hInstance;
    szBase[0] = 0;
    ini.SetWFlags(0);
    memset(&tutor, 0, sizeof(TUTOR));
    memset(&vtimer, 0, sizeof(TIMER));
    vtimer.fAutoGenWhenIn = TRUE;
    if (!hPrevInstance && InitMDIApp() == 0) {
        AlertSz(PszFormatIds(idsUnableInitializeStars, NULL), MB_ICONHAND);
        return 0;
    }
    fSeed = FALSE;
    lSeed = 0;
    if (!FCreateStuff()) {
        return 0;
    }
    if (!FGetSystemColors()) {
        AlertSz(PszFormatIds(idsUnableInitializeStars, NULL), MB_ICONHAND);
        return 0;
    }
    if (InitInstance(nCmdShow) == 0) {
        AlertSz(PszFormatIds(idsUnableInitializeStars, NULL), MB_ICONHAND);
        return 0;
    }
    if (!ParseCmdLine(lpCmdLine, &fSeed, &lSeed)) { FreeStuff(); return 2; }
    /* The original seeded before FCreateStuff; nothing in between draws a
       random number, so seeding after the command line is the same. */
    Randomize2(fSeed ? lSeed : GetTickCount());
    StarsRethrowPendingError();
    PostMessage(hwndFrame, WM_STARS_STARTUP, 0, 0);
    while (FUiGetMessage(&msg, NULL, 0, 0)) {
        StarsRethrowPendingError();
        if ((msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL) && !FRouteMouseWheel(&msg))
            continue;
        if (hwndTitle) {
            if (IUiTranslateAccelerator(hwndFrame, hAccelTitle, &msg) == 0) {
                TranslateMessage(&msg);
                LUiDispatchMessage(&msg);
            }
        } else if (IsIconic(hwndFrame) != 0 || IUiTranslateAccelerator(hwndFrame, hAccel, &msg) == 0) {
            TranslateMessage(&msg);
            if (((msg.message != WM_KEYDOWN && msg.message != WM_KEYUP) || !FHandleKey(msg.hwnd, msg.message, msg.wParam, msg.lParam)) &&
                (msg.message != WM_CHAR || !FHandleChar(msg.hwnd, msg.wParam, msg.lParam))) {
                LUiDispatchMessage(&msg);
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
        AlertSz(PszFormatIds(static_cast<StringId>(error.ids ? error.ids : idsMemory), NULL), MB_ICONHAND);
        FreeStuff();
    } catch (...) {
    }
    return 2;
} catch (...) {
    StarsPrimaryErrorScope primary;
    try {
        AlertSz(PszFormatIds(idsUnableInitializeStars, NULL), MB_ICONHAND);
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
    crButtonFace = GetSysColor(COLOR_BTNFACE);
    hbrButtonFace = HbrGet(crButtonFace);
    crButtonHilite = GetSysColor(COLOR_BTNHIGHLIGHT);
    hbrButtonHilite = HbrGet(crButtonHilite);
    crButtonShadow = GetSysColor(COLOR_BTNSHADOW);
    hbrButtonShadow = HbrGet(crButtonShadow);
    crButtonText = GetSysColor(COLOR_BTNTEXT);
    hbrButtonText = HbrGet(crButtonText);
    hbrWindowFrame = HbrGet(GetSysColor(COLOR_WINDOWFRAME));
    hbrDesktop = HbrGet(GetSysColor(COLOR_BACKGROUND));
    crWindow = GetSysColor(COLOR_WINDOW);
    hbrWindow = HbrGet(crWindow);
    crWindowText = GetSysColor(COLOR_WINDOWTEXT);
    hbrWindowText = HbrGet(crWindowText);
    dyTitleBar = GetSystemMetrics(SM_CYCAPTION);
    dxWinFrame = GetSystemMetrics(SM_CXFRAME);
    dyWinFrame = GetSystemMetrics(SM_CYFRAME);
    if (hdibPlaque) {
        lpbi = (BITMAPINFO *)GlobalLock(hdibPlaque);
        lpbi->bmiColors[249].rgbRed = crButtonFace;
        lpbi->bmiColors[249].rgbGreen = LOWORD(crButtonFace) >> 8;
        lpbi->bmiColors[249].rgbBlue = HIWORD(crButtonFace);
        GlobalUnlock(hdibPlaque);
    }
    if (hdibToolbar) {
        lpbi = (BITMAPINFO *)GlobalLock(hdibToolbar);
        lpbi->bmiColors[253].rgbRed = crButtonFace;
        lpbi->bmiColors[253].rgbGreen = LOWORD(crButtonFace) >> 8;
        lpbi->bmiColors[253].rgbBlue = HIWORD(crButtonFace);
        GlobalUnlock(hdibToolbar);
    }
    hdc = GetDC(NULL);
    vcScreenColors = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);
    return TRUE;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

INT_PTR CALLBACK About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT    rc;
    HDC     hdc;
    int16_t i;
    HWND    hwndCtl;
    FARPROC lpProc;

    switch (message) {
    case WM_INITDIALOG:
        iAbout1st = -11;
        iAboutPartial = 0;
        FUiSetWindowText(GetDlgItem(hwnd, IDC_ABOUT_DEMO_TEXT), SzVersion());
        uTimerId = SetTimer(hwnd, 14, 50, NULL);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_TIMER:
        hwndCtl = GetDlgItem(hwnd, IDC_ABOUT_CREDITS_TEXT);
        iAboutPartial += 2;
        if (iAboutPartial >= dyArial8) {
            iAboutPartial = 0;
            iAbout1st++;
            if (iAbout1st > 78) {
                iAbout1st = -11;
            }
        }
        GetClientRect(hwndCtl, &rc);
        hdc = GetDC(hwndCtl);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, crButtonFace);
        SetTextColor(hdc, crButtonText);
        IntersectClipRect(hdc, 0, 0, rc.right, rc.bottom);
        rc.top -= iAboutPartial;
        rc.bottom = rc.top + dyArial8;
        for (i = iAbout1st; i < iAbout1st + 10; i++) {
            if (i >= 0 && i < 77) {
                RcCtrTextOut(hdc, &rc, PszGetCompressedString(static_cast<StringId>(idsDesignProgramming + i)), -1);
            } else if (i >= 77) {
                break;
            }
            OffsetRect(&rc, 0, dyArial8);
        }
        rc.bottom = 1000;
        FillRect(hdc, &rc, hbrButtonFace);
        SelectClipRgn(hdc, NULL);
        ReleaseDC(hwnd, hdc);
        break;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            KillTimer(hwnd, uTimerId);
            uTimerId = 0;
            EndDialog(hwnd, 1);
            return 1;
        }
        break;
    }
    return 0;
}

int16_t FHandleChar(HWND hwnd, uint16_t ch, int32_t lParam) {
    HWND hwndF;

    if ((hwndScanner && (ch == 43 || ch == 45)) || (ch == 118 || ch == 86)) {
        hwndF = GetFocus();
        if (!hwndMessage || hwndF != hwndMsgEdit) {
            LUiSendMessage(hwndScanner, WM_CHAR, ch, lParam);
            return TRUE;
        }
    }
    return FALSE;
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

    if (iMsg == WM_KEYDOWN) {
        if (iKey == VK_ESCAPE && hwndBrowser && GetActiveWindow() == hwndBrowser) {
            FUiDestroyWindow(hwndBrowser);
            return TRUE;
        }
        if (iKey == VK_ESCAPE && hwndPopup) {
            LUiSendMessage(hwndPopup, WM_LBUTTONUP, 0, 0);
            return TRUE;
        }
        if (iKey == VK_ESCAPE && hwndReportDlg) {
            FUiDestroyWindow(hwndReportDlg);
            return TRUE;
        }
    } else if (iMsg == WM_KEYUP && hwndTb && (iKey == VK_ESCAPE || iKey == VK_RETURN)) {
        hwndF = GetParent(GetFocus());
        if (hwndF == hwndTb || GetParent(hwndF) == hwndTb) {
            TerminateToolbarFocus(iKey == VK_ESCAPE);
        }
    }
    if (iKey == VK_SHIFT && hwndScanner) {
        GetCursorPos16(&pt);
        hwndOver = WindowFromPoint(PointFrom16(pt));
        if (hwndOver == hwndScanner) {
            LUiSendMessage(hwndOver, WM_SETCURSOR, (WPARAM)hwndOver, 0);
        }
    }
    if (iMsg != WM_KEYDOWN) {
        return FALSE;
    }
    switch (iKey) {
    default:
        if (iKey < '0' || iKey > '9') {
            switch (iKey) {
            default:
                return FALSE;
            case VK_OEM_COMMA:
            case VK_OEM_PERIOD:
            case VK_OEM_4:
            case VK_OEM_6:
                break;
            }
        }
        /* fallthrough */
    case VK_BACK:
    case VK_DELETE:
    case VK_DOWN:
    case VK_UP:
    case VK_HOME:
    case VK_END:
        hwndF = GetFocus();
        if (hwndMessage) {
            if (hwndTb && (hwndTb == hwndF || GetParent(hwndF) == hwndTb || GetParent(GetParent(hwndF)) == hwndTb)) {
                return FALSE;
            }
            for (i = 0; i < 3; i++) {
                if (hwndF == rghwndOrderDD[i]) {
                    return FALSE;
                }
            }
            if (hwndF == hwndFleetCompLB || hwndF == hwndPlanetProdLB || hwndF == hwndMsgEdit || hwndF == hwndMsgDrop || hwndF == hwndOrderED ||
                hwndF == hwndMsgScroll || hwndF == hwndFleetCompLB || hwndF == hwndShipDD) {
                return FALSE;
            }
            if (hwndBrowser && hwndF == GetDlgItem(hwndBrowser, IDC_BROWSER_COMPONENT_CATEGORY)) {
                return FALSE;
            }
        }
        if (iKey >= '0' && iKey <= '9') {
            if (iKey >= '1' && iKey <= '6') {
                md = iKey - 49;
                if (md != (grbitScan & grbitScanViewMask)) {
                    ExecuteButton(static_cast<ToolbarButton>(iKey - 49), TRUE);
                    InvalidateRect(hwndTb, NULL, FALSE);
                }
                return TRUE;
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
                if (GetKeyState(VK_SHIFT) < 0) {
                    itb = tbShipCounts;
                } else {
                    itb = tbPlanetNames;
                }
            }
            ExecuteButton(itb, FIsButtonDown(itb) == 0);
            InvalidateRect(hwndTb, NULL, FALSE);
            return TRUE;
        }
        switch (iKey) {
        default:
            return FALSE;
        case VK_BACK:
        case VK_DELETE:
            if (sel.grobj != grobjFleet)
                break;
            iKey = VK_BACK;
            DeleteCurWayPoint(8);
            break;
        case VK_END:
        case VK_HOME:
        case VK_UP:
        case VK_DOWN:
            if (hwndF == hwndShipLB) {
                return FALSE;
            }
            LUiSendMessage(hwndMessage, WM_KEYDOWN, iKey, dw);
            break;
        case VK_OEM_COMMA:
        case VK_OEM_PERIOD:
            if (sel.grobj == grobjFleet && (sel.iwpAct > 0 || sel.fl.cord > 1)) {
                iwp = sel.iwpAct <= 0 ? 1 : sel.iwpAct;
                iWarp = sel.fl.lpplord->rgord()[iwp].iWarp;
                if (iKey == VK_OEM_COMMA) {
                    iWarp--;
                } else {
                    iWarp++;
                }
                if (iWarp >= 0 && iWarp <= 11) {
                    SetScanPathWarp(iwp, iWarp);
                    FLookupFleet(idWriteBack, &sel.fl);
                    DrawPlanShip(NULL, static_cast<TileBits>(tileFleetOrders | tileFleetComp | tileMinimized));
                }
            }
            return TRUE;
        case VK_OEM_4:
        case VK_OEM_6:
            pt.x = 0;
            pt.y = 0;
            ExecuteReportClick(pt, rptEnemyFleets, 0, iKey == VK_OEM_4 ? -2 : -1);
            return TRUE;
        }
        return TRUE;
    }
}
