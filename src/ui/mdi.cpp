#include "win.h"
#include "text_io.h"
#include "sessionrestore.h"
#include "control_io.h"
#include "presentation_io.h"
#include "preferencesui.h"
#include "profile_text_io.h"
#include "choice_io.h"
#include "alert_io.h"
#include "record_io.h"
#include "desktop_choice_io.h"
#include <stars_compat.h>
#include "ownership.h"
#include <SDL3/SDL.h>

char rgTOWidth[2][2] = {{-3}, {2, 1}};

int16_t InitMDIApp() {
    WNDCLASS wc;

    wc.style = 11;
    wc.lpfnWndProc = FrameWndProcDeferred;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = win32::LoadIcon(hInst, "StarsIco");
    wc.hCursor = win32::LoadCursor(NULL, MAKEINTRESOURCE(32512));
    wc.hbrBackground = (HBRUSH)13;
    wc.lpszMenuName = "StarsMenu";
    wc.lpszClassName = szFrame;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 523;
    wc.lpfnWndProc = MessageWndProc;
    wc.hIcon = 0;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.lpszClassName = szMessage;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 523;
    wc.lpfnWndProc = ScannerWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = szScan;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 523;
    wc.lpfnWndProc = MineWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.lpszClassName = szMine;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 520;
    wc.lpfnWndProc = TbWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.lpszClassName = szTb;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 0x200;
    wc.lpfnWndProc = PlanetWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.hIcon = 0;
    wc.lpszClassName = szPlanet;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.lpfnWndProc = PlanetPaneWndProc;
    wc.lpszClassName = "StarsPlanetPane";
    if (win32::RegisterClass(&wc) == 0) return FALSE;
    wc.style = 2560;
    wc.lpfnWndProc = PopupWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(WHITE_BRUSH);
    wc.hIcon = 0;
    wc.lpszClassName = szPopup;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 2560;
    wc.lpfnWndProc = TooltipWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(WHITE_BRUSH);
    wc.hIcon = 0;
    wc.lpszClassName = szTooltip;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 0x200;
    wc.lpfnWndProc = BrowserWndProc;
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.hIcon = 0;
    wc.lpszClassName = szBrowser;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 0;
    wc.lpfnWndProc = TitleWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = win32::LoadCursor(NULL, MAKEINTRESOURCE(32512));
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szTitle;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    wc.style = 11;
    wc.lpfnWndProc = ReportDlg;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = win32::LoadCursor(NULL, MAKEINTRESOURCE(32512));
    wc.hbrBackground = (HBRUSH)win32::GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szReport;
    if (win32::RegisterClass(&wc) == 0) {
        return FALSE;
    }
    return TRUE;
}

void CreateChildWindows() {
    char    szData[512];
    char    szNameUtf8[128], szPlayerUtf8[256];
    size_t  cch;
    POINT16 pt;
    char   *psz;
    char    szGame[15];

    if (idPlayer != iplrNone) {
        psz = strrchr(szBase, chDirSep);
#ifdef _WIN32
        if (!psz) psz = strrchr(szBase, '/');
        if (!psz) psz = strrchr(szBase, ':');
#endif
        psz = psz ? psz + 1 : szBase;
        cch = CchUtf8Prefix(psz, 8);
        memcpy(szGame, psz, cch); szGame[cch] = 0;
        for (psz = szGame; *psz; psz++) if (*psz >= 'A' && *psz <= 'Z') *psz += 'a' - 'A';
        win32::wsprintf(&szGame[strlen(szGame)], ".m%d", idPlayer + 1);
        CchCopyUtf8(szNameUtf8, sizeof(szNameUtf8), game.szName, strlen(game.szName));
        psz = TextIO().pfnPlayerName(idPlayer, FALSE, TRUE, FALSE, 0, NULL);
        CchCopyUtf8(szPlayerUtf8, sizeof(szPlayerUtf8), psz, strlen(psz));
        snprintf(szData, sizeof(szData), "Stars! -- %s -- %s -- %s", szNameUtf8, szPlayerUtf8, szGame);
    } else {
        TextIO().pfnCchGetString(idsStarsSHostMode, szWork);
        win32::wsprintf(szData, szWork, game.szName);
        CchCopyUtf8(szNameUtf8, sizeof(szNameUtf8), szData, strlen(szData));
        strcpy(szData, szNameUtf8);
    }
    FSetNativeWindowTitle(hwndFrame, szData);
    if (idPlayer != iplrNone) {
        if (!hwndScanner) {
            hwndScanner = ui::CreateWindowEx(0, szScan, NULL, WS_CHILD | WS_VISIBLE, -200, -200, 10, 10, hwndFrame, NULL, hInst, NULL);
        } else {
            ControlIO().pfnInvalidateRect(hwndScanner, NULL, TRUE);
            yScanTop = 1000;
            xScanTop = 1000;
            SetScanScrollBars(hwndScanner);
        }
        if (!hwndMine) {
            hwndMine = ui::CreateWindowEx(0, szMine, NULL, WS_CHILD | WS_VISIBLE, -500, -500, pt.x, pt.y, hwndFrame, NULL, hInst, NULL);
        } else {
            ControlIO().pfnInvalidateRect(hwndMine, NULL, TRUE);
        }
        if (!hwndPlanet) {
            hwndPlanet = CreatePlanetPane(hwndFrame);
        } else {
            ControlIO().pfnInvalidateRect(hwndPlanet, NULL, TRUE);
        }
        if (!hwndTb) {
            hwndTb = ui::CreateWindowEx(0, szTb, NULL, WS_CHILD | WS_VISIBLE, -500, -500, 10, 10, hwndFrame, NULL, hInst, NULL);
        } else {
            ControlIO().pfnInvalidateRect(hwndTb, NULL, TRUE);
        }
        if (hwndMessage) {
            ControlIO().pfnDestroyWindow(hwndMessage);
        }
        hwndMessage = ui::CreateWindowEx(0, szMessage, NULL, WS_CHILD | WS_VISIBLE, -500, -500, 10, 10, hwndFrame, NULL, hInst, NULL);
        ControlIO().pfnRefitFrameChildren();
    }
    return;
}

// PostOpenGame finishes opening a game GenerateWorld just created, as the
// File menu does after opening one.
void PostOpenGame() {
    ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
    return;
}

LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC          hdc;
    int16_t      i;
    HPALETTE     hpalSav;
    TEXTMETRIC   tm;
    POINT16      pt;
    FARPROC      lpProc;
    int16_t      fRet;
    int16_t      fErrSav;
    int16_t      idCur;
    int16_t      id;
    uint16_t     uTimerIdOld;
    char         szExt[4];
    int16_t      iOffset;
    int16_t      idPlanet;
    RECT         rc;
    RECT         rc2;
    PAINTSTRUCT  ps;
    int16_t      yOffset;
    HBRUSH       hbrSav;
    HCURSOR      hcs;
    POINT16      ptOld;
    PaneSplitter grSel;
    int16_t      grSelRaw;
    POINT16      ptAct;
    POINT16      ptD;
    POINT16      ptStart;
    POINT16      ptChg;

    switch (msg) {
    case WM_CREATE:
        hdc = win32::GetDC(hwnd);
        FCreateFonts(hdc);
        win32::GetTextMetrics(hdc, &tm);
        dySysFont = tm.tmHeight;
        dySBar = (dyArial8 + 12) * 2;
        win32::ReleaseDC(hwnd, hdc);
        InitTiles();
        EnsureTileSize(iWindowLayout == layoutSmall);
        return 0;
    case WM_ACTIVATE:
        return 0;
    case WM_QUERYNEWPALETTE:
    MapIt:
        if (hwndTitle) {
            return ControlIO().pfnSendMessage(hwndTitle, msg, wParam, lParam);
        }
        hdc = win32::GetDC(hwnd);
        hpalSav = win32::SelectPalette(hdc, vhpal, FALSE);
        i = win32::RealizePalette(hdc);
        win32::SelectPalette(hdc, hpalSav, FALSE);
        win32::ReleaseDC(hwnd, hdc);
        if (i != 0) {
            ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
            return 1;
        }
        return 0;
    case WM_PALETTECHANGED:
        if ((HWND)wParam != hwnd)
            goto MapIt;
        return 0;
    case WM_INITMENU:
        InitializeMenu((HMENU)wParam);
        return 0;
    case WM_STARS_STARTUP:
        idPlayer = iplrNone;
        if (ini.fCmdLine) {
            ini.fCmdLine = FALSE;
            if (FRunCmdLine()) {
            LExit:
                if (gd.fExitWindows) {
                    ExitWindows(vretExitValue, 0);
                    return 0;
                }
                win32::PostQuitMessage(vretExitValue);
                return 0;
            }
            CommandHandler(hwnd, 0xed9);
            if (ini.fTry)
                goto LExit;
            if (ini.fGen)
                goto LNop;
            if (game.lid == 0)
                goto LShowStartup;
            if (idPlayer != iplrNone && (ini.fDumpPlanets || ini.fDumpFleets || ini.fDumpMap)) {
                if (ini.fDumpMap) {
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_DEBUG_DUMP_UNIVERSE, 0);
                }
                if (ini.fDumpPlanets) {
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_DEBUG_DUMP_PLANETS, 0);
                }
                if (ini.fDumpFleets) {
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_DEBUG_DUMP_FLEETS, 0);
                }
                goto LExit;
            } else {
                ui::ShowWindow(hwndFrame, SW_SHOW);
                InitializeMenu(NULL);
                win32::PostMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                if (!ini.fWait)
                    goto LNop;
                ini.fWait = FALSE;
                CommandHandler(hwnd, 0x6a);
                goto LNop;
            }
        }
    LShowStartup:
        if (!hwndTitle) {
            ShowTitleScreen();
        }
        ini.fStartupFile = FALSE;
        DestroyCurGame();
    LNop:
        return 0;
    case WM_ENTERIDLE:
        if (gd.fTutorial && tutor.fChange) {
            PresentationIO().pfnTutor();
        }
        return 0;
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
        FGetSystemColors();
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = 520;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = 380;
        return 0;
    case WM_SIZE:
        if (wParam != 2 && wParam != 0)
            goto Default;
        vfs.dx = LOWORD(lParam);
        vfs.dy = HIWORD(lParam);
        ControlIO().pfnRefitFrameChildren();
        return 0;
    case WM_COMMAND:
        CommandHandler(hwnd, wParam);
        return 0;
    case WM_CLOSE:
        ControlIO().pfnDestroyWindow(hwnd);
        return 0;
    case WM_STARS_HOST:
        ControlIO().pfnBringUpHostDlg();
        return 1;
    case WM_STARS_CONTINUE:
        ShowTutor(FALSE);
        game.fDirty = FALSE;
        DestroyCurGame();
        fErrSav = fFileErrSilent;
        fFileErrSilent = TRUE;
        gd.fDontDoLogFiles = TRUE;
        if (!FLoadGame(szBase, "m1")) {
            fFileErrSilent = fErrSav;
            gd.fDontDoLogFiles = FALSE;
            return 0;
        }
        gd.fDontDoLogFiles = FALSE;
        fFileErrSilent = fErrSav;
        idPlayer = 0;
        if (wParam == 2506) {
            gd.fGeneratingTurn = TRUE;
            win32::wsprintf(szWork, "%s.x1", szBase);
            if (FLoadLogFile(szWork)) {
                FRunLogFile();
            }
            gd.fGeneratingTurn = FALSE;
        }
        CreateChildWindows();
        ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (wParam == 2506) {
            ControlIO().pfnSendMessage(hwndMessage, WM_KEYDOWN, VK_END, 0);
        }
        tutor.idt = idtWelcomeStarsTutorialWillGuideThrough36;
        tutor.fTurnDone = FALSE;
        tutor.fAutoComplete = wParam == 2506;
        PresentationIO().pfnTutor();
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) != 0xf030 && (wParam & 0xfff0) != 0xf120)
            goto Default;
        idCur = idPlayer;
        uTimerIdOld = uTimerId;
        if (uTimerId != 0) {
            win32::KillTimer(NULL, uTimerId);
            uTimerId = 0;
            CreateChildWindows();
        }
        if (idPlayer != iplrNone && FNewTurnAvail(idPlayer)) {
            if (uTimerIdOld != 0) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsNewTurnAvailable, NULL), MB_ICONASTERISK);
                id = 6;
            } else {
                id = AlertIO().pfnAlertSz(TextIO().pfnFormat(idsNewTurnAvailableWouldLikeLoad, NULL), MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL);
            }
            if (id == 6) {
                win32::wsprintf(szExt, MPCTD, idPlayer + 1);
                DestroyCurGame();
                if (!FLoadGame(szBase, szExt)) {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                } else {
                    CreateChildWindows();
                }
            } else if (id == 2) {
                if (uTimerIdOld != 0) {
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_TURN_WAIT_NEW, 0);
                } else {
                    win32::PostMessage(hwndFrame, WM_SYSCOMMAND, 0xf020, 0);
                }
                return 1;
            }
            ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            goto Default;
        }
        if (uTimerIdOld == 0)
            goto Default;
        if (uTimerType == hostTimerAutoGen) {
            win32::PostMessage(hwnd, WM_STARS_HOST, 0, 0);
            goto Default;
        }
        if (uTimerType == hostTimerWaitTurn) {
            id = AlertIO().pfnAlertSz(TextIO().pfnFormat(idsTurnHasSubmittedChangesMadeAfterTurn, NULL), MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL);
            if (id == 6 && !FMarkFile(dtLog, idPlayer, mdMarkDone, FALSE)) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsNewTurnCurrentlyGeneratedHostNewTurn, NULL), MB_ICONHAND);
                win32::wsprintf(szExt, MPCTD, idPlayer + 1);
                DestroyCurGame();
                if (!FLoadGame(szBase, szExt)) {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                } else {
                    CreateChildWindows();
                }
            } else if (id == 2) {
                win32::PostMessage(hwndFrame, WM_COMMAND, IDM_TURN_WAIT_NEW, 0);
                return 1;
            }
        }
        ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (sel.pt.x <= 1000 || sel.pt.y <= 1000)
            goto Default;
        CtrPointScan(sel.pt, TRUE);
        goto Default;
    case WM_CHAR:
        if (hwndScanner && (wParam == '-' || wParam == '+')) {
            ControlIO().pfnSendMessage(hwndScanner, WM_CHAR, wParam, lParam);
            return 0;
        }
        if (hwndMessage && (wParam == '-' || wParam == '+' || wParam == '\r')) {
            ControlIO().pfnSendMessage(hwndMessage, WM_CHAR, wParam, lParam);
            return 0;
        }
        if (hwndPlanet && (wParam == 'f' || wParam == 'F')) {
            ControlIO().pfnSendMessage(hwndPlanet, WM_CHAR, wParam, lParam);
            return 0;
        }
        if (hwndPlanet && sel.grobj == grobjPlanet && (wParam == 'q' || wParam == 'Q')) {
            ChangeProduction(FALSE);
            return 0;
        }
        if (!(sel.grobj & (grobjPlanet | grobjFleet))) {
            return 0;
        }
        iOffset = 0;
        idPlanet = 0;
        switch (wParam) {
        case 'n':
            iOffset = 1;
            break;
        case 'N':
        case 'P':
            if (sel.grobj == grobjPlanet) {
                idPlanet = IdFindAdjStarbase(sel.pl.id, wParam == 'N');
                break;
            }
            iOffset = wParam == 'N' ? 1 : -1;
            break;
        case 'p':
            iOffset = -1;
            break;
        case 'r':
        case 'R':
            if (sel.grobj == grobjFleet) {
                ShipCommandProc(hwndPlanet, 0, (LPARAM)rghwndBtn[6]);
            }
        }
        if (iOffset == 0 && idPlanet == 0) {
            return 0;
        }
        if (sel.grobj == grobjFleet) {
            PresentationIO().pfnSelectFleet(iOffset, idPlanet);
            return 0;
        }
        PresentationIO().pfnSelectPlanet(iOffset, idPlanet);
        return 0;
    case WM_QUERYDRAGICON:
        if (idPlayer == iplrNone) {
            return (LRESULT)hiconHost;
        }
        if (uTimerId == 0) {
            return (LRESULT)hiconStars;
        }
        return (LRESULT)hiconWait;
    case WM_ERASEBKGND:
        if (win32::IsIconic(hwnd) != 0) {
            ControlIO().pfnGetClientRect(hwnd, &rc);
            win32::FillRect((HDC)wParam, &rc, hbrDesktop);
            return 0;
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (hwndScanner) {
            ControlIO().pfnGetClientRect(hwndScanner, &rc2);
            win32::MapWindowPoints(hwndScanner, hwnd, (POINT *)&rc2, 2);
            win32::ExcludeClipRect((HDC)wParam, rc2.left, rc2.top, rc2.right, rc2.bottom);
        }
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_PAINT:
        if (win32::IsIconic(hwnd) != 0) {
            hdc = win32::BeginPaint(hwnd, &ps);
            win32::DrawIcon(hdc, 2, 2, idPlayer == iplrNone && game.lid != 0 ? hiconHost : uTimerId == 0 ? hiconStars : hiconWait);
            win32::EndPaint(hwnd, &ps);
            return 0;
        }
        hdc = win32::BeginPaint(hwnd, &ps);
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonShadow);
        switch (iWindowLayout) {
        case layoutLarge:
        default:
            win32::PatBlt(hdc, vfs.xTop + 5, 0, 2, vfs.dy, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y1 + 5, vfs.xTop + 2, 2, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y2 + 5, vfs.xTop + 2, 2, PATCOPY);
            win32::SelectObject(hdc, hbrButtonHilite);
            win32::PatBlt(hdc, vfs.xTop + 1, 0, 1, vfs.y1 + 2, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y1 + 1, vfs.xTop + 1, 1, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 1, vfs.y1 + 6, 1, vfs.y2 - vfs.y1 - 4, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y2 + 1, vfs.xTop + 1, 1, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 1, vfs.y2 + 6, 1, vfs.dy - vfs.y2 - 6, PATCOPY);
            break;
        case layoutMedium:
        case layoutSmall:
            yOffset = !gd.fToolbar ? 0 : 36;
            win32::PatBlt(hdc, vfs.xTop + 5, yOffset, 2, vfs.y2 + 2 - yOffset, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y1 + 5, vfs.xTop + 2, 2, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 5, vfs.y2 + 5, vfs.dx - vfs.xTop - 5, 2, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 5, vfs.y2 + 6, 2, vfs.dy - vfs.y2 - 5, PATCOPY);
            win32::SelectObject(hdc, hbrButtonHilite);
            win32::PatBlt(hdc, vfs.xTop + 1, yOffset, 1, vfs.y1 + 2 - yOffset, PATCOPY);
            win32::PatBlt(hdc, 0, vfs.y1 + 1, vfs.xTop + 1, 1, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 1, vfs.y1 + 6, 1, vfs.dy - vfs.y1 - 6, PATCOPY);
            win32::PatBlt(hdc, vfs.xTop + 6, vfs.y2 + 1, vfs.dx - vfs.xTop - 6, 1, PATCOPY);
        }
        win32::SelectObject(hdc, hbrSav);
        win32::EndPaint(hwnd, &ps);
        return 0;
    case WM_SETCURSOR:
        hcs = 0;
        if (win32::IsIconic(hwnd) != 0)
            goto Default;
        GetCursorPos16(&pt);
        ScreenToClient16(hwndFrame, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        hcs = HcrsFromFrameWindowPt(pt, NULL);
        if (!hcs)
            goto Default;
        win32::SetCursor(hcs);
        return 1;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hcs = HcrsFromFrameWindowPt(pt, &grSelRaw);
        if (!hcs) {
            return 0;
        }
        grSel = static_cast<PaneSplitter>(grSelRaw);
        hdc = win32::GetDC(hwnd);
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbr50Screen);
        ptD.y = 0;
        ptD.x = 0;
        InvertPaneBorder(hdc, grSel, ptD, NULL);
        ptStart = pt;
        win32::SetCapture(hwnd);
        ptOld = pt;
        ptAct.y = 0;
        ptAct.x = 0;
        while (FGetMouseMove(&pt)) {
            if (pt.x != ptOld.x || pt.y != ptOld.y) {
                ptD.x = pt.x - ptStart.x;
                ptD.y = pt.y - ptStart.y;
                ptChg.x = pt.x - ptOld.x;
                ptChg.y = pt.y - ptOld.y;
                ptAct = InvertPaneBorder(hdc, grSel, ptD, &ptChg);
                ptOld = pt;
            }
        }
        InvertPaneBorder(hdc, grSel, ptD, NULL);
        win32::ReleaseCapture();
        win32::SelectObject(hdc, hbrSav);
        win32::ReleaseDC(hwnd, hdc);
        if (ptAct.x == 0 && ptAct.y == 0) {
            return 0;
        }
        if (grSel & splitVertical) {
            if (iWindowLayout == layoutLarge) {
                vfs.dxPlanWant = vfs.xTop + ptAct.x;
            } else {
                vfs.dx2PlanWant = vfs.xTop + ptAct.x;
            }
        }
        if (grSel & splitMessages) {
            if (iWindowLayout == layoutLarge) {
                vfs.dyMsgWant = vfs.y2 - vfs.y1 - 8 - ptAct.y;
            } else {
                vfs.dy2MsgWant = vfs.dy - vfs.y1 - 8 - ptAct.y;
            }
        }
        if (grSel & splitLower) {
            if (iWindowLayout == layoutLarge) {
                vfs.dyMsgWant = vfs.y2 - vfs.y1 - 8 + ptAct.y;
                vfs.dyMinWant = vfs.dy - vfs.y2 - 8 - ptAct.y;
            } else {
                vfs.dy2MinWant = vfs.dy - vfs.y2 - 8 - ptAct.y;
            }
        }
        ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
        ControlIO().pfnRefitFrameChildren();
        return 0;
    case WM_DESTROY:
        if (uTimerId != 0) {
            win32::KillTimer(NULL, uTimerId);
        }
        WriteIniSettings();
        uTimerId = 0;
        if (gd.fHostMode) {
            FMarkFile(dtHost, iplrNone, mdMarkInUse, FALSE);
        }
        DestroyCurGame();
        if (gd.fExitWindows) {
            ExitWindows(vretExitValue, 0);
            return 0;
        }
        win32::PostQuitMessage(vretExitValue);
        return 0;
    default:
    Default:
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

POINT16 InvertPaneBorder(HDC hdc, PaneSplitter grSel, POINT16 dpt, POINT16 *pdptPrev) {
    int16_t notMin;
    int16_t dChg;
    POINT16 dptT;
    POINT16 dptPrev;
    int16_t dyAboveMinCur;
    POINT16 dptOld;
    int16_t dyMsgCur;
    int16_t dyMinAboveH2;
    int16_t dyPlanMin;
    int16_t dxScanMin;
    int16_t x;
    POINT16 pt;
    int16_t dyMin;

    dptOld = dpt;
    if (pdptPrev && grSel != splitVertical) {
        pt.x = dpt.x - pdptPrev->x;
        pt.y = dpt.y - pdptPrev->y;
        InvertPaneBorder(hdc, grSel, pt, NULL);
    }
    switch (iWindowLayout) {
    case layoutLarge:
    default:
        dxScanMin = 100;
        dyPlanMin = 50;
        dyMsgCur = vfs.y2 - vfs.y1 - 8;
        dyAboveMinCur = vfs.y2 - vfs.y1 - 8;
        notMin = 1;
        dyMinAboveH2 = (0xd * dyArial8 >> 1) + 0xa;
        break;
    case layoutMedium:
    case layoutSmall:
        dxScanMin = 200;
        dyPlanMin = 100;
        dyMsgCur = vfs.dy - vfs.y1 - 8;
        dyAboveMinCur = vfs.y2;
        notMin = -1;
        dyMinAboveH2 = 100;
    }
    if (vfs.xTop + dpt.x < 198) {
        dpt.x = 198 - vfs.xTop;
    }
    if (vfs.dx - vfs.xTop - dpt.x < dxScanMin) {
        dpt.x = vfs.dx - vfs.xTop - dxScanMin;
    }
    if (vfs.xTop + dpt.x > 396) {
        dpt.x = 396 - vfs.xTop;
    }
    if (grSel & splitMessages) {
        if (vfs.y1 + dpt.y < dyPlanMin) {
            dpt.y = dyPlanMin - vfs.y1;
        } else {
            dyMin = (0xd * dyArial8 >> 1) + 0xa;
            if (dyMsgCur - dpt.y < dyMin) {
                dpt.y = dyMsgCur - dyMin;
            }
        }
    }
    if (grSel & splitLower) {
        if (dyAboveMinCur + dpt.y < dyMinAboveH2) {
            dpt.y = (dyMinAboveH2 - dyAboveMinCur) * notMin;
        } else {
            dyMin = 13 * dyArial8 - 36;
            if (vfs.dy - vfs.y2 - 8 - dpt.y < dyMin) {
                dpt.y = vfs.dy - vfs.y2 - 8 - dyMin;
            }
        }
    }
    if (pdptPrev) {
        dptT = dpt;
        dpt.x = dptOld.x - pdptPrev->x;
        dpt.y = dptOld.y - pdptPrev->y;
        if (vfs.xTop + dpt.x < 198) {
            dpt.x = 198 - vfs.xTop;
        }
        if (vfs.dx - vfs.xTop - dpt.x < dxScanMin) {
            dpt.x = vfs.dx - vfs.xTop - dxScanMin;
        }
        if (vfs.xTop + dpt.x > 396) {
            dpt.x = 396 - vfs.xTop;
        }
        if (grSel & splitMessages) {
            if (vfs.y1 + dpt.y < 50) {
                dpt.y = 50 - vfs.y1;
            } else {
                dyMin = (0xd * dyArial8 >> 1) + 0xa;
                if (dyMsgCur - dpt.y < dyMin) {
                    dpt.y = dyMsgCur - dyMin;
                }
            }
        } else if (grSel & splitLower) {
            if (dyAboveMinCur + dpt.y < dyMinAboveH2) {
                dpt.y = (dyMinAboveH2 - dyAboveMinCur) * notMin;
            } else {
                dyMin = 13 * dyArial8 - 36;
                if (vfs.dy - vfs.y2 - 8 - dpt.y < dyMin) {
                    dpt.y = vfs.dy - vfs.y2 - 8 - dyMin;
                }
            }
        }
        dptPrev.x = dptT.x - dpt.x;
        dptPrev.y = dptT.y - dpt.y;
        dpt = dptT;
    }
    // Splitter values can combine the three boundary bits.
    switch (static_cast<uint16_t>(grSel)) {
    default:
        dpt.y = 0;
        if (!pdptPrev || abs(dptPrev.x) >= 6) {
            if (pdptPrev) {
                win32::PatBlt(hdc, vfs.xTop + dpt.x - dptPrev.x + 1, 0, 6, vfs.dy, PATINVERT);
            }
            win32::PatBlt(hdc, vfs.xTop + dpt.x + 1, 0, 6, vfs.dy, PATINVERT);
            break;
        }
        dChg = dptPrev.x;
        if (dChg == 0)
            break;
        if (dChg < 0) {
            x = vfs.xTop + dpt.x + 1;
            dChg = -dChg;
        } else {
            x = vfs.xTop + dpt.x - dptPrev.x + 1;
        }
        win32::PatBlt(hdc, x, 0, dChg, vfs.dy, PATINVERT);
        win32::PatBlt(hdc, x + 6, 0, dChg, vfs.dy, PATINVERT);
        break;
    case splitMessages:
        dpt.x = 0;
        win32::PatBlt(hdc, 0, vfs.y1 + dpt.y + 1, vfs.xTop + 1, 6, PATINVERT);
        break;
    case splitLower:
        dpt.x = 0;
        if (iWindowLayout == layoutLarge) {
            win32::PatBlt(hdc, 0, vfs.y2 + dpt.y + 1, vfs.xTop + 1, 6, PATINVERT);
            break;
        }
        win32::PatBlt(hdc, vfs.xTop + 7, vfs.y2 + dpt.y + 1, vfs.dx - vfs.xTop - 7, 6, PATINVERT);
        break;
    case 3:
        win32::PatBlt(hdc, vfs.xTop + dpt.x + 1, 0, 6, vfs.dy, PATINVERT);
        win32::PatBlt(hdc, 0, vfs.y1 + dpt.y + 1, vfs.xTop + dpt.x + 1, 6, PATINVERT);
        break;
    case 5:
        win32::PatBlt(hdc, vfs.xTop + dpt.x + 1, 0, 6, vfs.dy, PATINVERT);
        if (iWindowLayout == layoutLarge) {
            win32::PatBlt(hdc, 0, vfs.y2 + dpt.y + 1, vfs.xTop + dpt.x + 1, 6, PATINVERT);
            break;
        }
        win32::PatBlt(hdc, vfs.xTop + dpt.x + 7, vfs.y2 + dpt.y + 1, vfs.dx - dpt.x - vfs.xTop - 7, 6, PATINVERT);
        break;
    case 7:
        win32::PatBlt(hdc, vfs.xTop + dpt.x + 1, 0, 6, vfs.dy, PATINVERT);
        win32::PatBlt(hdc, 0, vfs.y1 + dpt.y + 1, vfs.xTop + dpt.x + 1, 6, PATINVERT);
        win32::PatBlt(hdc, vfs.xTop + dpt.x + 7, vfs.y2 + dpt.y + 1, vfs.dx - dpt.x - vfs.xTop - 7, 6, PATINVERT);
    }
    return dpt;
}

HCURSOR HcrsFromFrameWindowPt(POINT16 pt, int16_t *pgrSel) {
    HCURSOR hcs;
    int16_t fInHBar2;
    int16_t fInHBar1;
    int16_t fInVBar;

    hcs = 0;
    fInVBar = pt.x >= vfs.xTop && pt.x < vfs.xTop + 8;
    fInHBar1 = pt.x < vfs.xTop + 8 && pt.y >= vfs.y1 && pt.y < vfs.y1 + 8;
    switch (iWindowLayout) {
    case layoutLarge:
    default:
        if (pt.x < vfs.xTop + 8 && pt.y >= vfs.y2 && pt.y < vfs.y2 + 8) {
            fInHBar2 = TRUE;
            break;
        }
        fInHBar2 = FALSE;
        break;
    case layoutMedium:
    case layoutSmall:
        fInHBar2 = pt.x >= vfs.xTop && pt.y >= vfs.y2 && pt.y < vfs.y2 + 8;
    }
    if (!fInVBar) {
        if (fInHBar1 || fInHBar2) {
            hcs = hcurResizeNS;
        }
    } else if (fInHBar1 || fInHBar2) {
        hcs = hcurResize4Way;
    } else {
        hcs = hcurResizeWE;
    }
    if (pgrSel) {
        *pgrSel = fInHBar1 * 2 + fInVBar + fInHBar2 * 4;
    }
    return hcs;
}

void RestoreSelection() {
    PLANET *lppl;

    if (ini.idPlayer == idPlayer && ini.lid == game.lid) {
        if (ini.grobjSel == 2) {
            if (LpflFromId(ini.iObjSel) != 0) {
                PresentationIO().pfnSelectFleet(0, ini.iObjSel);
                ini.grobjSel = 0;
            } else {
                ini.grobjSel = 1;
                ini.iObjSel = rgplr[idPlayer].idPlanetHome;
            }
        } else if (ini.grobjSel == 1) {
            lppl = LpplFromId(ini.iObjSel);
            if (!lppl || lppl->iPlayer != idPlayer) {
                ini.iObjSel = rgplr[idPlayer].idPlanetHome;
            } else {
                PresentationIO().pfnSelectPlanet(0, ini.iObjSel);
                ini.grobjSel = 0;
            }
        }
        if (ini.turn == game.turn && ini.iMsg > 0) {
            iMsgCur = ini.iMsg - 1;
            if (iMsgCur >= cMsg + vcmsgplrIn) {
                iMsgCur = imsgNone;
            }
            iMsgCur = IMsgNext(FALSE);
            gd.fGotoVCR = FALSE;
            ControlIO().pfnSetMsgTitle(hwndMessage);
            ControlIO().pfnInvalidateRect(hwndMessage, NULL, TRUE);
            if (gd.fTutorial) {
                tutor.fChange = TRUE;
                PresentationIO().pfnTutor();
            }
        }
    } else {
        ini.grobjSel = 1;
        ini.iObjSel = rgplr[idPlayer].idPlanetHome;
    }
    if (ini.grobjSel != 0) {
        if (ini.iObjSel != -1) {
            lppl = LpplFromId(ini.iObjSel);
            if (!lppl || lppl->iPlayer != idPlayer) {
                PresentationIO().pfnFindSelection();
            } else {
                PresentationIO().pfnSelectPlanet(0, ini.iObjSel);
            }
        } else {
            PresentationIO().pfnFindSelection();
        }
        ini.grobjSel = 0;
    }
    return;
}

void CommandHandler(HWND hwnd, WPARAM wParam) {
    POINT16  pt;
    HMENU    hmenu;
    FARPROC  lpProc;
    int16_t  dy;
    char     szExt[4];
    int16_t  dx;
    int16_t  fRet;
    RECT     rc;
    int16_t  iplrOld;
    char    *psz;
    char     szT[256];
    int16_t  cPageX;
    PLANET  *lpplMac;
    PLANET  *lppl;
    int16_t  i;
    PRINTDLG pd;
    int      iPrintResult;
    int16_t  cPageY;
    int16_t  xPage;
    int16_t  dxMax;
    int16_t  dxDPI;
    int16_t  dyPrintTiny;
    int16_t  dMargin;
    int16_t  y;
    int32_t  ldx;
    HFONT    hfontPrintTiny;
    HFONT    hfontPrint;
    POINT16  ptLegendB;
    POINT16  ptLegendA;
    int16_t  dyPrint;
    int16_t  dyMax;
    HFONT    hfontSav;
    int16_t  dSize;
    int16_t  yPage;
    int16_t  cch;
    int32_t  ldy;
    int16_t  dyDPI;
    int16_t  yOff;
    int16_t  xOff;
    int32_t  x;
    int16_t  idCur;
    HCURSOR  hcurSav;
    HCURSOR  hcurT;
    uint32_t dwTickCur;
    uint32_t dwTickBase;
    int16_t  mf;
    StringId ids;
    int16_t  cObj;
    int16_t  ifl;
    FLEET   *lpfl;
    int16_t  id;

    if (LOWORD(wParam) >= IDM_POPUP_BASE && LOWORD(wParam) < 15100) {
        iPopMenuSel = LOWORD(wParam) - 15000;
    } else {
        switch (LOWORD(wParam)) {
        case IDM_HELP_ABOUT:
            lpProc = MakeProcInstance(About, hInst);
            ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_FILE_EXIT:
            ControlIO().pfnSendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_VIEW_LAYOUT_0:
        case IDM_VIEW_LAYOUT_1:
        case IDM_VIEW_LAYOUT_2:
            if (idPlayer == iplrNone)
                break;
            iWindowLayout = static_cast<WindowLayout>(LOWORD(wParam) - 130);
            ControlIO().pfnInvalidateRect(hwndFrame, NULL, TRUE);
            EnsureTileSize(iWindowLayout == layoutSmall);
            ControlIO().pfnRefitFrameChildren();
            break;
        case IDM_HELP_TUTORIAL2:
        case IDM_HELP_TUTORIAL:
            if (gd.fTutorial) {
                ShowTutor(TRUE);
                break;
            }
            StartTutor(FALSE);
            break;
        case IDM_FILE_NEW_GAME:
        case IDM_TOOL_NEW_GAME:
            if (gd.fTutorial && !FAskKillTutor())
                break;
            DesktopChoiceIO().pfnNewGameWizard(hwnd, FALSE);
            break;
        case IDM_FILE_MRU1:
        case IDM_FILE_MRU2:
        case IDM_FILE_MRU3:
        case IDM_FILE_MRU4:
        case IDM_FILE_MRU5:
        case IDM_FILE_MRU6:
        case IDM_FILE_MRU7:
        case IDM_FILE_MRU8:
        case IDM_FILE_MRU9:
            if ((gd.fTutorial && !FAskKillTutor()) || !vrgszMRU || vrgszMRU[(LOWORD(wParam) - 4300) * 256] == 0)
                break;
            iplrOld = idPlayer;
            strcpy(szT, vrgszMRU + 256 * (LOWORD(wParam) - 4300));
            psz = strrchr(szT, 46);
            if (psz) {
                if (!FIsManagedLibraryGameFile(szT)) {
                    if (FImportDesktopFile(hwnd, szT) && hwndTitle) {
                        RefreshLibraryGames(hwndTitle);
                        RefreshLibraryRaces(hwndTitle);
                    }
                    break;
                }
                if (!FFileExists(szT)) goto LShowMissingRecent;
                ini.fStartupFile = TRUE;
                DestroyCurGame();
                strcpy(szBase, szT);
                if (FOpenGame(hwnd, FALSE) <= 0)
                    break;
                InitializeMenu(NULL);
                CreateChildWindows();
                if (uTimerId == 0) {
                    win32::PostMessage(hwnd, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                }
                if (!game.fTutorial || idPlayer != 0)
                    break;
                StartTutor(FALSE);
                break;
            }
        LShowMissingRecent:
            strcpy(szWork, szT);
            AlertIO().pfnAlertSz(TextIO().pfnFormat(idsCantOpenFile, NULL), MB_ICONHAND);
            break;
        case IDM_RACE_CREATE:
            vplr = vrgplrDef[0];
            RaceCreationWizard(hwnd, FALSE, FALSE);
            break;
        case IDM_VIEW_TOOLBAR:
            hmenu = GetASubMenu(hwnd, menuView);
            gd.fToolbar = gd.fToolbar == 0;
            win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !gd.fToolbar ? MF_UNCHECKED : MF_CHECKED);
            ControlIO().pfnRefitFrameChildren();
            break;
        case IDM_FILE_PRINT_MAP:
            if (idPlayer == iplrNone)
                break;
            lpProc = MakeProcInstance(PrintMapDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_PRINT_MAP), hwndFrame, lpProc, 0);
            FreeProcInstance(lpProc);
            if (!fRet)
                break;
            cPageX = vrgcPrintMapPage[0];
            cPageY = vrgcPrintMapPage[1];
            memset(&pd, 0, sizeof(PRINTDLG));
            pd.lStructSize = sizeof(PRINTDLG);
            pd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;
            if (win32::PrintDlg(&pd) != 0) {
                hfontPrint = NULL;
                hfontPrintTiny = NULL;
                NATIVECLEANUPSCOPE print([&]() noexcept {
                    if (hfontPrint) win32::DeleteObject(hfontPrint);
                    if (hfontPrintTiny) win32::DeleteObject(hfontPrintTiny);
                    win32::DeleteDC(pd.hDC);
                    if (pd.hDevMode) win32::GlobalFree(pd.hDevMode);
                    if (pd.hDevNames) win32::GlobalFree(pd.hDevNames);
                });
                dxMax = win32::GetDeviceCaps(pd.hDC, HORZRES);
                dyMax = win32::GetDeviceCaps(pd.hDC, VERTRES);
                dxDPI = win32::GetDeviceCaps(pd.hDC, LOGPIXELSX);
                dyDPI = win32::GetDeviceCaps(pd.hDC, LOGPIXELSY);
                dMargin = 32;
                hfontPrint = HfontPrinterCreate(pd.hDC, 8, &dyPrint);
                hfontPrintTiny = HfontPrinterCreate(pd.hDC, 5, &dyPrintTiny);
                win32::SetBkMode(pd.hDC, TRANSPARENT);
                rc.top = 0;
                rc.left = 0;
                if ((cPageX > cPageY) == (dxMax > dyMax)) {
                    i = cPageX;
                    cPageX = cPageY;
                    cPageY = i;
                }
                ldx = (uint32_t)(dxMax * cPageX);
                ldy = (uint32_t)(dyMax * cPageY);
                if (ldx > 32000) {
                    ldx = 32000;
                }
                if (ldy > 32000) {
                    ldy = 32000;
                }
                if (ldx < ldy) {
                    if (ldy - dyDPI < ldx) {
                        ldx = ldy - dyDPI;
                    }
                    dSize = LOWORD(ldx);
                    ptLegendA.x = dMargin;
                    ptLegendA.y = dSize + dMargin;
                    ptLegendB.x = dSize / 2;
                    ptLegendB.y = ptLegendA.y;
                } else {
                    if (ldx - (int16_t)((int16_t)(3 * dxDPI) / 2) < ldy) {
                        ldy = ldx - (int16_t)((int16_t)(3 * dxDPI) / 2);
                    }
                    dSize = LOWORD(ldy);
                    ptLegendA.x = dSize + dMargin;
                    ptLegendA.y = dMargin;
                    ptLegendB.x = ptLegendA.x;
                    ptLegendB.y = dSize / 2;
                }
                rc.bottom = dSize;
                rc.right = dSize;
                dSize -= dMargin * 2;
                cch = TextIO().pfnCchGetString(idsStarsUniverseMap, szWork);
                iPrintResult = win32::Escape(pd.hDC, STARTDOC, cch, szWork, NULL);
                if (iPrintResult <= 0) goto LPrintMapCleanup;
                for (xPage = 0; xPage < cPageX; xPage++) {
                    for (yPage = 0; yPage < cPageY; yPage++) {
                        xOff = -dxMax * xPage;
                        yOff = -dyMax * yPage;
                        win32::Rectangle(pd.hDC, xOff, yOff, xOff + rc.right, yOff + rc.bottom);
                        if (hfontPrint) {
                            hfontSav = (HFONT)win32::SelectObject(pd.hDC, hfontPrint);
                        } else {
                            hfontSav = 0;
                        }
                        y = ptLegendA.y + yOff;
                        cch = TextIO().pfnCchGetString(idsStarsUniverseMap, szWork);
                        win32::TextOut(pd.hDC, ptLegendA.x + xOff, y, szWork, cch);
                        y += dyPrint;
                        win32::TextOut(pd.hDC, ptLegendA.x + xOff, y, game.szName, strlen(game.szName));
                        y += dyPrint;
                        psz = TextIO().pfnPlayerName(idPlayer, TRUE, TRUE, TRUE, 0, NULL);
                        win32::TextOut(pd.hDC, ptLegendA.x + xOff, y, psz, strlen(psz));
                        y += dyPrint;
                        cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsYearD), game.turn + 2400);
                        win32::TextOut(pd.hDC, ptLegendA.x + xOff, y, szWork, cch);
                        if ((grbitScan & grbitScanViewMask) != 5) {
                            y = ptLegendB.y + yOff;
                            for (i = 0; i < 5; i++) {
                                cch = TextIO().pfnCchGetString(static_cast<StringId>(idsPlanet3 + i), szWork);
                                win32::TextOut(pd.hDC, ptLegendB.x + xOff, y, szWork, cch);
                                y += dyPrint;
                            }
                            y = ptLegendB.y + yOff;
                            if (hfontPrintTiny) {
                                win32::SelectObject(pd.hDC, hfontPrintTiny);
                            }
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, (int16_t)(3 * dyPrint) / 2 + y - dyPrintTiny / 2, "|", 1);
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, (int16_t)(5 * dyPrint) / 2 + y - dyPrintTiny / 2, "+", 1);
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, (int16_t)(9 * dyPrint) / 2 + y + 8 - dyPrintTiny, "2", 1);
                            if (hfontPrint) {
                                win32::SelectObject(pd.hDC, hfontPrint);
                            }
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, y - 4 + dyPrint / 2, 1);
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, y - 4 + (int16_t)(7 * dyPrint) / 2, 0);
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, (int16_t)(9 * dyPrint) / 2 + y + 8, 0);
                        }
                        if ((grbitScan & grbitScanViewMask) != 5) {
                            if (hfontPrintTiny) {
                                win32::SelectObject(pd.hDC, hfontPrintTiny);
                            }
                            lppl = lpPlanets;
                            lpplMac = lpPlanets + cPlanet;
                            for (; lppl < lpplMac; lppl++) {
                                x = (int16_t)(rgptPlan[lppl->id].x - 1000);
                                y = (int16_t)(dGalInv - 1000 - rgptPlan[lppl->id].y);
                                x = (int32_t)((int32_t)(x * dSize) / dGal) + dMargin + xOff;
                                y = (int32_t)((int32_t)(y * dSize) / dGal) + dMargin + yOff;
                                if (lppl->iPlayer == idPlayer) {
                                    if (lppl->fStarbase) {
                                        CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) + 4 - dyPrintTiny,
                                                   (char *)(rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef == ihuldefOrbitalFort ? "|" : "+"), 1);
                                    }
                                    DrawPlanetPrintDot(pd.hDC, LOWORD(x), LOWORD(y), 1);
                                } else if (lppl->iPlayer != iplrNone) {
                                    cch = win32::wsprintf(szWork, PCTD, lppl->iPlayer + 1);
                                    CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) - dyPrintTiny, szWork, cch);
                                }
                            }
                        }
                        if (hfontPrint) {
                            win32::SelectObject(pd.hDC, hfontPrint);
                        }
                        for (i = 0; i < game.cPlanMax; i++) {
                            x = (int16_t)(rgptPlan[i].x - 1000);
                            y = (int16_t)(dGalInv - 1000 - rgptPlan[i].y);
                            x = (int32_t)((int32_t)(x * dSize) / dGal) + dMargin + xOff;
                            y = (int32_t)((int32_t)(y * dSize) / dGal) + dMargin + yOff;
                            DrawPlanetPrintDot(pd.hDC, LOWORD(x), LOWORD(y), 0);
                            if (grbitScan & grbitScanPlanetNames) {
                                CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) + 14, TextIO().pfnPlanetName(i), 0);
                            }
                        }
                        if (hfontSav) {
                            win32::SelectObject(pd.hDC, hfontSav);
                        }
                        iPrintResult = win32::Escape(pd.hDC, NEWFRAME, 0, NULL, NULL);
                        if (iPrintResult <= 0) {
                            win32::Escape(pd.hDC, ABORTDOC, 0, NULL, NULL);
                            goto LPrintMapCleanup;
                        }
                    }
                }
                iPrintResult = win32::Escape(pd.hDC, ENDDOC, 0, NULL, NULL);
LPrintMapCleanup:
                print.Close();
                if (iPrintResult <= 0 && iPrintResult != SP_APPABORT)
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnablePrintGameMapPrinterMayOff, NULL), MB_ICONHAND);
            } else {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnablePrintGameMapPrinterMayOff, NULL), MB_ICONHAND);
            }
            break;
        case IDM_VIEW_PLAYER_COLORS:
            hmenu = GetASubMenu(hwnd, menuView);
            grbitScan = static_cast<GrbitScan>(grbitScan ^ grbitScanPlayerColors);
            win32::CheckMenuItem(hmenu, IDM_VIEW_PLAYER_COLORS, !(grbitScan & grbitScanPlayerColors) ? MF_UNCHECKED : MF_CHECKED);
            gd.fChgScanner = TRUE;
            if (!(grbitScan & (grbitScanPlanetNames | grbitScanShipCounts)))
                break;
            ControlIO().pfnInvalidateRect(hwndScanner, NULL, FALSE);
            break;
        case IDM_VIEW_FIND:
        case IDM_VIEW_FIND2:
            if (idPlayer == iplrNone)
                break;
            lpProc = MakeProcInstance(FindDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_FIND), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_GAME_SCORE:
        case IDM_GAME_SCORE2:
            if (idPlayer == iplrNone)
                break;
            lpProc = MakeProcInstance(ScoreXDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_SCORE), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_FILE_HOST_GAME:
            if (!game.fSinglePlr &&
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsGameAlreadyHostedAnotherInstanceStarsWould, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES)
                break;
            /* fallthrough */
        case IDM_TURN_GENERATE:
        case IDM_DEBUG_GEN_10_TURNS:
        case IDM_DEBUG_GEN_100_TURNS:
        case IDM_DEBUG_GEN_1000_TURNS:
            idCur = idPlayer;
            switch (LOWORD(wParam)) {
            case IDM_DEBUG_GEN_100_TURNS:
                iPassCnt = 100;
                break;
            case IDM_DEBUG_GEN_1000_TURNS:
                iPassCnt = 1000;
                break;
            case IDM_DEBUG_GEN_10_TURNS:
                iPassCnt = 10;
                break;
            default:
                iPassCnt = 0;
            }
            if (game.fSinglePlr && iPassCnt != 0) {
                win32::wsprintf(szWork, TextIO().pfnCompressed(idsSureWantForceGenerateDTurnsRow), iPassCnt);
                if (CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), szWork, "Stars!", MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL) != IDYES)
                    break;
            }
            if (idPlayer == iplrNone)
                break;
            if (gd.fTutorial) {
                PresentationIO().pfnTutor();
                if (!tutor.fTurnDone) {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsTutorialTurnWillGeneratedHaveYetCompleted, NULL), MB_ICONHAND);
                    break;
                }
                ShowTutor(FALSE);
                if (game.turn <= 35) {
                    Randomize(1234567890);
                    FWriteHistFile(idCur);
                    if (!FWriteTutorialMFile(game.turn + 1)) {
                        AlertIO().pfnAlertSz(TextIO().pfnFormat(static_cast<StringId>(idsFileError), NULL), MB_ICONHAND);
                        break;
                    }
                    if (game.turn == 35 && !FWriteTutorialMFile(game.turn + 2)) {
                        AlertIO().pfnAlertSz(TextIO().pfnFormat(static_cast<StringId>(idsFileError), NULL), MB_ICONHAND);
                        break;
                    }
                    strcpy(szWork, szBase);
                    strcat(szWork, ".x1");
                    RemoveNativeFile(szWork);
                    RecordIO().pfnDirtyGame(FALSE);
                    ShowProgressGauge();
                    dwTickBase = win32::GetTickCount();
                    do {
                        UpdateProgressGauge(static_cast<ProgressStep>((LOWORD(dwTickCur) - LOWORD(dwTickBase)) * 2));
                        dwTickCur = win32::GetTickCount();
                    } while (dwTickCur >= dwTickBase && dwTickCur < dwTickBase + 500);
                    goto LTutorialFinishUp;
                }
            }
            if (!game.fSinglePlr)
                goto LWaitForTurn;
            idsFileError = -1;
            if (FCheckFile(dtHost, iplrNone, mdMarkInUse)) {
                if (FBadFileError(static_cast<StringId>(idsFileError))) {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(static_cast<StringId>(idsFileError), NULL), MB_ICONHAND);
                    break;
                }
                if (AlertIO().pfnAlertSz(TextIO().pfnFormat(idsGameAlreadyHostedAnotherInstanceStarsWould, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES)
                    goto LWaitForTurn;
                break;
            }
            hcurSav = win32::SetCursor(win32::LoadCursor(NULL, MAKEINTRESOURCE(32514)));
            FWriteLogFile(szBase, idCur);
            FWriteHistFile(idCur);
        RepGen:
            ShowProgressGauge();
            EnsureAis();
            hcurT = win32::SetCursor(win32::LoadCursor(NULL, IDC_WAIT));
            FGenerateTurn();
            win32::SetCursor(hcurT);
            switch (LOWORD(wParam)) {
            case IDM_DEBUG_GEN_10_TURNS:
            case IDM_DEBUG_GEN_100_TURNS:
            case IDM_DEBUG_GEN_1000_TURNS:
                iPassCnt--;
                if (iPassCnt > 0) {
                    HideProgressGauge();
                    if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) >= 0 || ControlIO().pfnGetAsyncKeyState(VK_CONTROL) >= 0)
                        goto RepGen;
                }
            }
            iPassCnt = 0;
        LTutorialFinishUp:
            DestroyCurGame();
            win32::wsprintf(szExt, MPCTD, idCur + 1);
            if (!FLoadGame(szBase, szExt)) {
                win32::SetCursor(hcurSav);
                HideProgressGauge();
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                break;
            }
            HideProgressGauge();
            idPlayer = idCur;
            CreateChildWindows();
            ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            win32::SetCursor(hcurSav);
            if (!gd.fTutorial)
                break;
            tutor.fTurnDone = FALSE;
            tutor.fAutoComplete = FALSE;
            PresentationIO().pfnTutor();
            break;
        case IDM_VIEW_BROWSER_TOGGLE:
        case IDM_VIEW_BROWSER_TOGGLE2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            if (!hwndBrowser) {
                mf = 8;
                ui::CreateDialog(hInst, MAKEINTRESOURCE(IDD_BROWSER), hwndFrame, lpfnBrowserDlgProc);
            } else {
                mf = 0;
                ControlIO().pfnDestroyWindow(hwndBrowser);
            }
            hmenu = GetASubMenu(hwnd, menuHelp);
            win32::CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, mf);
            break;
        case IDM_REPORT_CYCLE:
            if (!hwndReportDlg) {
                wParam = IDM_REPORT_PLANET;
            } else if (vprptCur == &vrptPlanet) {
                wParam = IDM_REPORT_FLEET;
            } else if (vprptCur == &vrptFleet) {
                wParam = IDM_REPORT_ENEMY_FLEET;
            } else {
                wParam = IDM_REPORT_BATTLE;
            }
            /* fallthrough */
        case IDM_REPORT_PLANET:
        case IDM_REPORT_FLEET:
        case IDM_REPORT_ENEMY_FLEET:
        case IDM_REPORT_BATTLE:
            cObj = 0;
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            hmenu = GetASubMenu(hwnd, menuReport);
        LRetryReport:
            if (!hwndReportDlg) {
                mf = 8;
                switch (LOWORD(wParam)) {
                case IDM_REPORT_FLEET:
                    ids = idsFleetSummaryReportDFleetC;
                    vprptCur = &vrptFleet;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->iPlayer == idPlayer) {
                            cObj++;
                        }
                    }
                    break;
                case IDM_REPORT_ENEMY_FLEET:
                    ids = idsOthersFleetsSummaryReportDFleetC;
                    vprptCur = &vrptEFleet;
                    for (ifl = 0; ifl < cFleet; ifl++) {
                        lpfl = rglpfl[ifl];
                        if (!rglpfl[ifl])
                            break;
                        if (lpfl->iPlayer != idPlayer) {
                            cObj++;
                        }
                    }
                    break;
                case IDM_REPORT_BATTLE:
                    vprptCur = &vrptBattle;
                    ids = idsBattleSummaryReportDBattleC;
                    cObj = CBattles();
                    break;
                default:
                    vprptCur = &vrptPlanet;
                    ids = idsPlanetSummaryReportDPlanetC;
                    lppl = lpPlanets;
                    lpplMac = lpPlanets + cPlanet;
                    for (; lppl < lpplMac; lppl++) {
                        if (lppl->iPlayer == idPlayer) {
                            cObj++;
                        }
                    }
                }
                psz = TextIO().pfnCompressed(ids);
                win32::wsprintf(szWork, psz, cObj, cObj == 1 ? 32 : 115);
                hwndReportDlg = ui::CreateWindowEx(0, szReport, szWork, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0, 0, 100,
                                             100, hwndFrame, NULL, hInst, NULL);
                ControlIO().pfnSetWindowPos(hwndReportDlg, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
                win32::CheckMenuItem(hmenu, LOWORD(wParam), mf);
                break;
            } else {
                if (LOWORD(wParam) != IDM_REPORT_BATTLE || vprptCur != &vrptBattle) {
                    ids = idsPlayerLogFileAppearsCorruptUnableLoad;
                } else {
                    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
                }
                mf = 0;
                ControlIO().pfnDestroyWindow(hwndReportDlg);
                win32::CheckMenuItem(hmenu, LOWORD(wParam) == IDM_REPORT_FLEET ? IDM_REPORT_FLEET : IDM_REPORT_PLANET, mf);
                if (ids != idsPlayerLogFileAppearsCorruptUnableLoad) {
                    return;
                }
                goto LRetryReport;
            }
        case IDM_HELP_CONTEXT_13002:
        case IDM_HELP_INTRO:
            win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, (uint32_t)(LOWORD(wParam) == IDM_HELP_INTRO ? 4501 : 13002));
            break;
        case IDM_HELP_CONTENTS:
        case IDM_HELP_CONTENTS2:
            win32::WinHelp(hwnd, szHelpFile, HELP_INDEX, 0);
            break;
        case IDM_TURN_WAIT_NEW:
        case IDM_GAME_WAIT_FOR_TURN:
        LWaitForTurn:
            if (!lpPlanets || idPlayer == iplrNone || game.fSinglePlr)
                break;
            if (FNewTurnAvail(idPlayer)) {
            LNewTurnAvail:
                FWriteHistFile(idPlayer);
                if (game.fDirty) {
                    id = AlertIO().pfnAlertSz(TextIO().pfnFormat(idsSorryTurnHasAlreadyGeneratedAnyChanges, NULL), MB_OKCANCEL | MB_ICONEXCLAMATION);
                    if (id == 2)
                        break;
                } else {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsNewTurnAvailable, NULL), MB_ICONASTERISK);
                }
                win32::wsprintf(szExt, MPCTD, idPlayer + 1);
                game.fDirty = FALSE;
                DestroyCurGame();
                if (!FLoadGame(szBase, szExt)) {
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                } else {
                    CreateChildWindows();
                    ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                }
            } else {
                gd.fSubmit = TRUE;
                FWriteLogFile(szBase, idPlayer);
                FWriteHistFile(idPlayer);
                ControlIO().pfnSetWindowText(hwndFrame, TextIO().pfnCompressed(idsWaitingNewTurn));
                ui::ShowWindow(hwndFrame, SW_SHOWMINIMIZED);
                uTimerId = win32::SetTimer(NULL, hostTimerWaitTurn, 10000, lpfnHostTimerProc);
                uTimerType = hostTimerWaitTurn;
                HostTimerProc(NULL, WM_NULL, uTimerId, 0);
            }
            break;
        case IDM_TURN_SAVE_SUBMIT:
            wParam = IDM_TURN_END_B;
            /* fallthrough */
        case IDM_FILE_SAVE:
        case IDM_TURN_END_A:
        case IDM_TURN_END_B:
            if (!hwndScanner) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsGameCurrentlyLoaded, NULL), MB_ICONHAND);
                break;
            }
            if (idPlayer == iplrNone) {
                FWriteDataFile(szBase, idPlayer, FALSE);
                break;
            }
            if (FNewTurnAvail(idPlayer))
                goto LNewTurnAvail;
            gd.fSubmit = LOWORD(wParam) == IDM_TURN_END_B;
            FWriteLogFile(szBase, idPlayer);
            FWriteHistFile(idPlayer);
            break;
        case IDM_LIBRARY_EXPORT_ORDERS:
            FExportLibraryOrders(hwnd, NULL);
            break;
        case IDM_FILE_OPEN_GAME:
        case IDM_TOOL_OPEN_GAME:
            if (gd.fTutorial && !FAskKillTutor()) break;
            if (ini.fStartupFile && FIsManagedLibraryGameFile(szBase)) {
                if (FOpenGame(hwnd, FALSE) <= 0) break;
                InitializeMenu(NULL);
                if (uTimerId == 0) win32::PostMessage(hwnd, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                break;
            }
            fRet = FImportDesktopFile(hwnd, ini.fStartupFile ? szBase : NULL);
            ini.fStartupFile = FALSE;
            if (fRet && hwndTitle) {
                RefreshLibraryGames(hwndTitle);
                RefreshLibraryRaces(hwndTitle);
            }
            break;
        case IDM_TITLE_NEW_GAME:
        case IDM_TITLE_OPEN_GAME:
        case IDM_TITLE_CONTINUE:
        case IDM_TITLE_EXIT:
            if (!hwndTitle)
                break;
            win32::PostMessage(hwndTitle, WM_COMMAND, LOWORD(wParam) - 250, 0);
            break;
        case IDM_FILE_RETURN_TO_TITLE:
            if (gd.fTutorial && !FAskKillTutor())
                break;
            if (!FCanLeaveCurrentGame())
                break;
            WriteIniSettings();
            DestroyCurGame();
            strcpy(szBase, szWork);
            ini.grobjSel = 0;
            ini.iObjSel = 0;
            ini.idPlayer = iplrNone;
            InitializeMenu(NULL);
            ShowTitleScreen();
            ui::ShowWindow(hwndFrame, SW_HIDE);
            break;
        case IDM_GAME_RESEARCH:
        case IDM_GAME_RESEARCH2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            lpProc = MakeProcInstance(ResearchDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_RESEARCH), hwndFrame, lpProc, 0);
            FreeProcInstance(lpProc);
            if (!fRet || sel.grobj != grobjPlanet)
                break;
            if (sel.pl.lpplprod) {
                ControlIO().pfnFillPlanetProdLB(hwndPlanetProdLB, sel.pl.lpplprod, NULL);
            }
            ControlIO().pfnDrawPlanShip(NULL, static_cast<TileBits>(tilePlanetStats | tileProductionOrOrbit));
            break;
        case IDM_GAME_BATTLE_PLANS1:
        case IDM_GAME_BATTLE_PLANS2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            lpProc = MakeProcInstance(BattlePlansDlg, hInst);
            ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_BATTLE_PLANS), hwndFrame, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_GAME_RELATIONS:
        case IDM_GAME_RELATIONS2:
            if (game.lid == 0 || idPlayer == iplrNone || game.fSinglePlr)
                break;
            lpProc = MakeProcInstance(RelationsDlg, hInst);
            ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_RELATIONS), hwndFrame, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_RACE_EDIT1:
        case IDM_RACE_EDIT2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            vplr = rgplr[idPlayer];
            RaceCreationWizard(hwnd, TRUE, FALSE);
            break;
        case IDM_VIEW_GAME_PARAMS:
        case IDM_VIEW_GAME_PARAMS2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            DesktopChoiceIO().pfnNewGameWizard(hwnd, TRUE);
            break;
        case IDM_GAME_SHIP_BUILDER:
        case IDM_GAME_SHIP_BUILDER2:
            if (game.lid == 0 || idPlayer == iplrNone)
                break;
            pt.x = 610;
            pt.y = 450;
            if (hwndPopup) {
                ControlIO().pfnSendMessage(hwndPopup, WM_RBUTTONUP, 0, 0);
            }
            ShipBuilder(pt);
            break;
        case IDM_DEBUG_DUMP_UNIVERSE:
            DumpUniverse();
            break;
        case IDM_DEBUG_DUMP_PLANETS:
            DumpPlanets();
            break;
        case IDM_DEBUG_DUMP_FLEETS:
            DumpFleets();
            break;
        case IDM_CMD_CHANGE_PASSWORD:
            if (game.lid == 0 || idPlayer == iplrNone || ((game.fSinglePlr && lSaltCur <= 0) || !FCheckPassword()))
                break;
            lpProc = MakeProcInstance(NewPasswordDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_PASSWORD), hwndFrame, lpProc, 0);
            FreeProcInstance(lpProc);
            break;
        case IDM_SCAN_ZOOM_0:
        case IDM_SCAN_ZOOM_1:
        case IDM_SCAN_ZOOM_2:
        case IDM_SCAN_ZOOM_3:
        case IDM_SCAN_ZOOM_4:
        case IDM_SCAN_ZOOM_5:
        case IDM_SCAN_ZOOM_6:
        case IDM_SCAN_ZOOM_7:
        case IDM_SCAN_ZOOM_8:
            if (!hwndScanner) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsCantChangeZoomFactorUntilGameOpen, NULL), MB_ICONHAND);
                break;
            }
            hmenu = GetASubMenu(hwnd, menuView);
            hmenu = win32::GetSubMenu(hmenu, 3);
            win32::CheckMenuItem(hmenu, iScanZoom + 4, MF_BYPOSITION);
            ControlIO().pfnGetClientRect(hwndScanner, &rc);
            rc.right = ScanToPt(rc.right) >> 1;
            rc.bottom = ScanToPt(rc.bottom) >> 1;
            dx = xScanTop;
            dy = dGalInv - yScanTop;
            iScanZoom = static_cast<ScanZoom>(LOWORD(wParam) - 3905);
            win32::CheckMenuItem(hmenu, iScanZoom + 4, MF_CHECKED | MF_BYPOSITION);
            win32::DrawMenuBar(hwnd);
            SetScanScrollBars(hwndScanner);
            ControlIO().pfnInvalidateRect(hwndScanner, NULL, TRUE);
            if (sel.scan.grobj != grobjNone) {
                pt = sel.scan.pt;
            } else {
                pt.x = dx + rc.right;
                pt.y = dy - rc.bottom;
            }
            CtrPointScan(pt, FALSE);
            break;
        case IDM_FRAME_POST_OPEN:
            RestoreGameWindows();
            break;
        case IDM_FLEET_DELETE_WAYPOINT:
        case IDM_FLEET_INSERT_WAYPOINT:
            if (sel.grobj == grobjFleet && win32::GetFocus() != hwndOrderED) {
                DeleteCurWayPoint(LOWORD(wParam) == IDM_FLEET_DELETE_WAYPOINT);
            }
            goto Default;
        default:
        Default:
            ui::DefWindowProc(hwnd, WM_COMMAND, wParam, 0);
        }
    }
}

void InitializeMenu(HMENU hmenu) {
    int16_t cMenu;
    int16_t i;
    HMENU   hmenuSub;

    if (!hmenu) {
        hmenu = win32::GetMenu(hwndFrame);
    }
    hmenuSub = GetASubMenu(hwndFrame, menuFile);
    for (i = 4300; i <= 4308; i++) {
        win32::DeleteMenu(hmenuSub, i, MF_BYCOMMAND);
    }
    for (i = 0; i < 9 && vrgszMRU[i * 256] != 0; i++) {
        szWork[0] = '&';
        szWork[1] = i + '1';
        szWork[2] = ' ';
        strcpy(&szWork[3], vrgszMRU + 256 * i);
        win32::InsertMenu(hmenuSub, i + 9, MF_BYPOSITION, i + 4300, szWork);
    }
    win32::EnableMenuItem(hmenu, 106, szBase[0] != 0 && !game.fSinglePlr ? MF_ENABLED : MF_GRAYED | MF_DISABLED);
    win32::EnableMenuItem(hmenu, 105, szBase[0] == 0 ? MF_GRAYED | MF_DISABLED : MF_ENABLED);
    win32::EnableMenuItem(hmenu, 270, szBase[0] != 0 && (!game.fSinglePlr || lSaltCur > 0) ? MF_ENABLED : MF_GRAYED | MF_DISABLED);
    win32::EnableMenuItem(hmenu, 2014, szBase[0] != 0 && !game.fSinglePlr ? MF_ENABLED : MF_GRAYED | MF_DISABLED);
    win32::EnableMenuItem(hmenu, 3803, szBase[0] != 0 && !game.fSinglePlr ? MF_ENABLED : MF_GRAYED | MF_DISABLED);
    hmenu = GetASubMenu(hwndFrame, menuView);
    win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !gd.fToolbar ? MF_UNCHECKED : MF_CHECKED);
    win32::CheckMenuItem(hmenu, IDM_VIEW_PLAYER_COLORS, !(grbitScan & grbitScanPlayerColors) ? MF_UNCHECKED : MF_CHECKED);
    if (!hwndScanner) {
        win32::EnableMenuItem(win32::GetMenu(hwndFrame), 1, MF_GRAYED | MF_DISABLED | MF_BYPOSITION);
    } else {
        win32::EnableMenuItem(win32::GetMenu(hwndFrame), 1, MF_BYPOSITION);
        hmenu = GetASubMenu(hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 3);
        win32::CheckMenuItem(hmenu, iScanZoom + 4, MF_CHECKED | MF_BYPOSITION);
        cMenu = win32::GetMenuItemCount(hmenu);
        for (i = 0; i < cMenu; i++) {
            win32::EnableMenuItem(hmenu, i, MF_BYPOSITION);
        }
        hmenu = GetASubMenu(hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 4);
        win32::CheckMenuItem(hmenu, iWindowLayout, MF_CHECKED | MF_BYPOSITION);
    }
    win32::DrawMenuBar(hwndFrame);
    return;
}

HMENU GetASubMenu(HWND hwnd, MainMenu iMenu) {
    int16_t fChildMenu;
    HMENU   hmenu;

    fChildMenu = hwndActive && win32::IsZoomed(hwndActive) != 0;
    hmenu = win32::GetMenu(hwnd);
    hmenu = win32::GetSubMenu(hmenu, iMenu + fChildMenu);
    return hmenu;
}

int16_t FOpenGame(HWND hwnd, int16_t fRaceOnly) {
    OPENFILENAME ofn;
    uint16_t     i;
    char         szFile[256];
    char        *pch;
    char         szFileTitle[256];
    char         szFilter[256];
    int16_t      fRet;

    if (ini.fStartupFile) {
        strcpy(szFile, szBase);
        pch = strrchr(szBase, chDirSep);
        if (pch)
            *pch = 0;
        pch = strrchr(szFile, 46);
        if (!pch) {
            SetSzWorkFromDt(dtHost, iplrNone);
            strcpy(szFile, szWork);
            pch = strrchr(szFile, 46);
        }
        ofn.nFileExtension = pch - szFile + 1;
        ofn.nFileOffset = 0;
        fFileErrSilent = TRUE;
        goto LGotFileName;
    }
    szFile[0] = 0;
    TextIO().pfnCchGetString(!fRaceOnly ? idsStarsGameFilesMHstRStars : idsStarsGameFilesRFiles, szFilter);
    for (i = 0; szFilter[i] != 0; i++) {
        if (szFilter[i] == '|') {
            szFilter[i] = 0;
        }
    }
    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 0x100;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 0x100;
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (DesktopChoiceIO().pfnFNativeOpenFileName(&ofn) == 0) {
        return 0;
    }
LGotFileName:
    szDirName[0] = 0;
    fRet = FWasRaceFile(&szFile[ofn.nFileOffset], fRaceOnly == 0);
    if (fRaceOnly) {
        if (fRet > 0) {
            if (!FIsManagedLibraryRaceFile(szFile)) return FImportLibraryRace(hwnd, szFile);
            pch = szFile + strlen(szFile);
            while (pch > szFile && pch[-1] != '/' && pch[-1] != '\\') pch--;
            return FLoadLibraryRace(hwnd, pch);
        }
        return fRet;
    }
    if (fRet) {
        fFileErrSilent = FALSE;
        ini.fStartupFile = FALSE;
        if (fRet == -1) {
            return -1;
        }
        SetSzRaceFile(&szFile[ofn.nFileOffset]);
        RaceCreationWizard(hwnd, FALSE, FALSE);
        return 0;
    }
    szFile[ofn.nFileExtension - 1] = 0;
    DestroyCurGame();
    strcpy(szBase, szFile);
    if (!FLoadGame(szFile, &szFile[ofn.nFileExtension])) {
        if (ini.fStartupFile) {
            ini.SetWFlags(0);
            fFileErrSilent = FALSE;
        }
        return 0;
    }
    if (ini.fStartupFile) {
        fFileErrSilent = FALSE;
        ini.fStartupFile = FALSE;
        pch = strrchr(szFile, chDirSep);
        if (pch) {
            i = pch - szFile;
            strncpy(szDirName, szFile, i);
            szDirName[i] = 0;
        }
        if (idPlayer == iplrNone && !ini.fGen) {
            gd.fClose = TRUE;
        }
        if (game.lid != ini.lid || game.turn > ini.turn) {
            ini.fTry = FALSE;
        }
    }
    sel.grobjFull = grobjNone;
    sel.grobj = grobjNone;
    sel.iwpAct = iwpNone;
    sel.id = -1;
    sel.scan.grobjFull = grobjNone;
    sel.scan.grobj = grobjNone;
    sel.scan.iwp = iwpNone;
    sel.scan.ifl = iflNone;
    sel.scan.idpl = idplNone;
    fOrdersVis = FALSE;
    CreateChildWindows();
    return FinishOpenGame();
}

void BringUpHostDlg() {
    FARPROC lpProc;
    int16_t fRet;
    HCURSOR hcurSav;

    if (!gd.fHostMode) {
        if (!gd.fReadOnly) {
            FMarkFile(dtHost, iplrNone, mdMarkInUse, TRUE);
        }
        gd.fHostMode = TRUE;
    }
    ui::ShowWindow(hwndFrame, SW_HIDE);
    if (ini.fWait) {
        ini.fWait = FALSE;
        goto LAutoMode;
    }
Top:
    lpProc = MakeProcInstance(HostModeDialog, hInst);
    fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_HOST_MODE), hwndFrame, lpProc, 0);
    FreeProcInstance(lpProc);
    if (fRet == -1) {
    LAutoMode:
        ui::ShowWindow(hwndFrame, SW_SHOWMINIMIZED);
        uTimerId = win32::SetTimer(NULL, hostTimerAutoGen, 10000, lpfnHostTimerProc);
        uTimerType = hostTimerAutoGen;
        HostTimerProc(NULL, WM_NULL, uTimerId, 0);
        return;
    }
    if (fRet) {
    LNextGen:
        if (gd.fProgressTxt) {
            ShowProgressGauge();
        }
        EnsureAis();
        hcurSav = win32::SetCursor(win32::LoadCursor(NULL, IDC_WAIT));
        FGenerateTurn();
        win32::SetCursor(hcurSav);
        if (iPassCnt != 0) {
            iPassCnt--;
            if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) >= 0 && ControlIO().pfnGetAsyncKeyState(VK_CONTROL) >= 0)
                goto LNextGen;
        }
        iPassCnt = 0;
        HideProgressGauge();
        goto Top;
    }
    if (!gd.fReadOnly) {
        FMarkFile(dtHost, iplrNone, mdMarkInUse, FALSE);
    }
    gd.fHostMode = FALSE;
    DestroyCurGame();
    if (ini.fGen) {
        return;
    }
    ShowTitleScreen();
    return;
}

void DrawHostDialog2(HWND hwnd, HDC hdcIn) {
    uint32_t dsec;
    HDC      hdc;
    uint16_t dhour;
    int16_t  bkMode;
    int16_t  yCur;
    int16_t  i;
    uint16_t dmin;
    int16_t  dday;
    int16_t  cch;
    RECT     rcDiamond;
    COLORREF crBackSav;
    int16_t  x;
    char     szStat[30];

    if (hdcIn) {
        hdc = hdcIn;
    } else {
        hdc = win32::GetDC(hwnd);
    }
    bkMode = win32::SetBkMode(hdc, OPAQUE);
    crBackSav = win32::SetBkColor(hdc, crButtonFace);
    win32::SelectObject(hdc, rghfontArial8[1]);
    x = dyArial8 + 10 + LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(idsN16), 4));
    yCur = 48;
    win32::SetRect(&rcDiamond, 6, yCur, dyArial8 + 7, yCur + dyArial8 + 1);
    for (i = 0; i < game.cPlayer; i++) {
        DrawDiamond(hdc, &rcDiamond, hbrBBlue);
        cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsD2), i + 1);
        RightTextOut(hdc, x, yCur, szWork, cch, 0);
        win32::SetTextColor(hdc, rgOut[i] <= 0 ? 32512 : 127);
        TextIO().pfnCchGetString(static_cast<StringId>(idsTurned + rgOut[i]), szStat);
        if (gd.fNoHostNames) {
            cch = win32::wsprintf(szWork, " %s", szStat);
        } else {
            cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsSS), TextIO().pfnPlayerName(i, TRUE, TRUE, TRUE, 0, NULL), szStat);
        }
        if (rgplr[i].fHacker) {
            strcat(szWork, " - HACKER");
            cch += 9;
        }
        win32::TextOut(hdc, x + 4, yCur, szWork, cch);
        win32::SetTextColor(hdc, crWindowText);
        win32::OffsetRect(&rcDiamond, 0, dyArial8 + 4);
        yCur += dyArial8 + 4;
    }
    cch = win32::wsprintf(szWork, PCTD, game.turn + 2401);
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_NEXT_YEAR_TEXT), szWork);
    dsec = (uint32_t)((win32::GetTickCount() - ctickLast) / 1000);
    if (dsec < 60) {
        cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDSeconds), LOWORD(dsec));
    } else {
        dmin = LOWORD((uint32_t)(dsec / 60));
        dsec -= (uint32_t)(60 * dmin);
        if (dmin < 60) {
            cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsD02d), dmin, LOWORD(dsec));
        } else {
            dhour = (uint32_t)dmin / 60;
            dmin -= 60 * dhour;
            if (dhour < 24) {
                cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsD02d02d), dhour, dmin, LOWORD(dsec));
            } else {
                dday = (uint32_t)dhour / 24;
                dhour -= dday * 24;
                cch = win32::wsprintf(szWork, TextIO().pfnCompressed(idsDDaysD02d02d), dday, dhour, dmin, LOWORD(dsec));
            }
        }
    }
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_TIME_SINCE_TEXT), szWork);
    win32::SetBkMode(hdc, bkMode);
    win32::SetBkColor(hdc, crBackSav);
    if (!hdcIn) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

INT_PTR CALLBACK HostModeDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    FARPROC     lpProc;
    int16_t     fRet;
    RECT        rc;
    int16_t     mf;
    POINT16     pt;
    int16_t     tpm;
    int16_t     i;
    int16_t     iRet;
    int16_t     iSel;
    int16_t     iDiamond;
    HMENU       hmenuPopup;
    win32::MSG         msg;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyHostModeDlg, TRUE);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_GAME_NAME_TEXT), game.szName);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_FILE_TEXT), szBase);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), gd.fReadOnly == 0 && (vtimer.fAutoGenWhenIn || vtimer.mdForce != 0));
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_GENERATE_NOW), gd.fReadOnly == 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_PASSWORD), gd.fReadOnly == 0);
        uTimerId = win32::SetTimer(hwnd, 13, 10000, NULL);
        /* fallthrough */
    case WM_TIMER:
        if (fProcessingTimer)
            goto Done;
        {
            fProcessingTimer = TRUE;
            CFindTurnsOutstanding();
            DrawHostDialog2(hwnd, NULL);
            fProcessingTimer = FALSE;
        }
    Done:
        if (message != WM_TIMER) {
            return 1;
        }
        return 0;
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
        win32::SetBkColor((HDC)wParam, crButtonFace);
        return (INT_PTR)hbrButtonFace;
    case WM_SETCURSOR:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < 6 || pt.x >= dyArial8 + 7 || pt.y < 48)
            break;
        iDiamond = (int16_t)(pt.y - 48) / (dyArial8 + 4);
        if (iDiamond >= game.cPlayer || (int16_t)(pt.y - 48) % (dyArial8 + 4) >= dyArial8 + 1)
            break;
        if (message == WM_SETCURSOR) {
            win32::SetCursor(hcurHand);
            return 1;
        }
        if (!rgplr[iDiamond].fAi) {
            iSel = 0;
        } else if (rgplr[iDiamond].idAi == idAiMaid) {
            iSel = 2;
        } else {
            iSel = 1;
        }
        hmenuPopup = win32::CreatePopupMenu();
        iPopMenuSel = -1;
        for (i = 0; i < 3; i++) {
            TextIO().pfnCchGetString(static_cast<StringId>(idsHumanControlled + i), szWork);
            mf = i == 1                                                         ? rgplr[iDiamond].fAi != 0 && rgplr[iDiamond].idAi != idAiMaid ? 0 : 3
                 : rgplr[iDiamond].fAi == 0 || rgplr[iDiamond].idAi == idAiMaid ? 0
                                                                                : 3;
            win32::AppendMenu(hmenuPopup, (i == iSel ? 8 : 0) | mf, i + 15000, szWork);
        }
        ClientToScreen16(hwnd, &pt);
        tpm = message == WM_LBUTTONDOWN ? TPM_LEFTBUTTON : TPM_RIGHTBUTTON;
        ui::TrackPopupMenu(hmenuPopup, TPM_CENTERALIGN | tpm, pt.x, pt.y, 0, hwnd, NULL);
        win32::DestroyMenu(hmenuPopup);
        iRet = -1;
        if (ui::PeekMessage(&msg, hwnd, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15100) {
            iRet = msg.wParam - 15000;
        }
        if (iRet != -1 && iSel != iRet) {
            rgplr[iDiamond].fAi = iRet > 0;
            if (iRet == 2) {
                rgplr[iDiamond].idAi = idAiMaid;
            }
            rgplr[iDiamond].lSalt = ~rgplr[iDiamond].lSalt;
            FMarkFile(dtTurn, iDiamond, mdMarkAi, iRet != 0);
            FMarkFile(dtHost, iDiamond, mdMarkAi, iRet != 0);
            gd.fAisDone = FALSE;
            fProcessingTimer = TRUE;
            CFindTurnsOutstanding();
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), gd.fAllAis == 0 && (vtimer.fAutoGenWhenIn || vtimer.mdForce != 0));
            DrawHostDialog2(hwnd, NULL);
            fProcessingTimer = FALSE;
        }
        return 0;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        if (ctickLast == 0) {
            CFindTurnsOutstanding();
        }
        if (!gd.fReadOnly) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), gd.fAllAis == 0 && (vtimer.fAutoGenWhenIn || vtimer.mdForce != 0));
        }
        DrawHostDialog2(hwnd, hdc);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_HOST_GENERATE_NOW:
        case IDCANCEL:
        case IDC_HOST_AUTO_GENERATE:
            if (LOWORD(wParam) == IDC_HOST_GENERATE_NOW) {
                if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) < 0) {
                    if (ControlIO().pfnGetAsyncKeyState(VK_CONTROL) < 0) {
                        iPassCnt = 999;
                    } else {
                        iPassCnt = 9;
                    }
                } else if (ControlIO().pfnGetAsyncKeyState(VK_CONTROL) < 0) {
                    iPassCnt = 99;
                } else {
                    iPassCnt = 0;
                }
                if (iPassCnt != 0) {
                    win32::wsprintf(szWork, TextIO().pfnCompressed(idsSureWantForceGenerateDTurnsRow), iPassCnt + 1);
                    if (CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), szWork, "Stars!", MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL) != IDYES) {
                        iPassCnt = 0;
                        return 1;
                    }
                } else if (CFindTurnsOutstanding() != 0 &&
                           AlertIO().pfnAlertSz(TextIO().pfnFormat(idsSureWishGenerateOptionDoesGuaranteePlayers, NULL), MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL) != IDYES) {
                    return 1;
                }
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyHostModeDlg, FALSE);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDCANCEL ? 0 : LOWORD(wParam) == IDC_HOST_AUTO_GENERATE ? -1 : 1);
            if (LOWORD(wParam) == IDC_HOST_AUTO_GENERATE) {
                EnsureAis();
            } else if (LOWORD(wParam) == IDCANCEL && gd.fClose) {
                win32::PostQuitMessage(vretExitValue);
            }
            return 1;
        case IDC_HOST_PASSWORD:
            if (!FCheckPassword())
                break;
            lpProc = MakeProcInstance(NewPasswordDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_PASSWORD), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            ControlIO().pfnSetFocus(hwnd);
            return fRet;
        case 1029:
            lpProc = MakeProcInstance(HostOptionsDialog, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_HOST_OPTIONS), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            ControlIO().pfnSetFocus(hwnd);
            if (fRet && !gd.fReadOnly) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), gd.fAllAis == 0 && (vtimer.fAutoGenWhenIn || vtimer.mdForce != 0));
            }
            return fRet;
        case IDC_HELP:
            win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhHostModeDialog);
            return 1;
        }
        break;
    case WM_DESTROY:
        win32::KillTimer(hwnd, uTimerId);
        uTimerId = 0;
        break;
    }
    return 0;
}

INT_PTR CALLBACK HostOptionsDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_INITDIALOG:
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
        win32::SetBkColor((HDC)wParam, crButtonFace);
        return (INT_PTR)hbrButtonFace;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawHostOptions(hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhHostModeDialog);
            return 1;
        }
        break;
    case WM_DESTROY:
        break;
    }
    return 0;
}

void DrawHostOptions(HWND hwnd, HDC hdc, int16_t iDraw) { return; }

VOID CALLBACK HostTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime) {
    HWND    hwndT;
    char    szExt[4];
    int16_t cOut;
    int16_t fSav;
    int16_t idCur;
    HCURSOR hcurSav;

    if (!fProcessingTimer) {
        fProcessingTimer = TRUE;
        fSav = fFileErrSilent;
        if (uTimerType == hostTimerWaitTurn) {
            if (!FNewTurnAvail(idPlayer))
                goto Done;
            idCur = idPlayer;
            win32::KillTimer(hwnd, uTimerId);
            win32::wsprintf(szExt, MPCTD, idPlayer + 1);
            DestroyCurGame();
            if (!FLoadGame(szBase, szExt)) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                goto Done;
            }
            idPlayer = idCur;
            CreateChildWindows();
            uTimerId = win32::SetTimer(NULL, hostTimerTurnReady, 1000, lpfnHostTimerProc);
            uTimerType = hostTimerTurnReady;
            win32::FlashWindow(hwndFrame, TRUE);
            ControlIO().pfnSetWindowText(hwndFrame, TextIO().pfnCompressed(idsNewTurnAvailable2));
            ControlIO().pfnMessageBeep(MB_ICONEXCLAMATION);
            cOut = 1;
            goto RedrawText;
        }
        if (uTimerType == hostTimerTurnReady) {
            win32::FlashWindow(hwndFrame, TRUE);
            goto Done;
        }
    Loop:
        cOut = CFindTurnsOutstanding();
        if (gd.fAllAis) {
            AlertIO().pfnAlertSz(TextIO().pfnFormat(idsAutoGenerateDisabledBecauseHumanPlayersDead, NULL), MB_ICONHAND);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), FALSE);
            goto Done;
        }
        win32::wsprintf(szWork, TextIO().pfnCompressed(idsHostModeDPlayer), cOut);
        if (cOut != 1) {
            strcat(szWork, "s");
        }
        strcat(szWork, TextIO().pfnCompressed(idsOut));
        ControlIO().pfnSetWindowText(hwndFrame, szWork);
    RedrawText:
        hwndT = win32::GetWindow(hwndFrame, GW_HWNDPREV);
        if (win32::GetWindow(hwndT, GW_OWNER) == hwndFrame) {
            ControlIO().pfnInvalidateRect(hwndT, NULL, TRUE);
        }
        if (cOut == 0) {
            if (gd.fProgressTxt) {
                ShowProgressGauge();
            }
            EnsureAis();
            hcurSav = win32::SetCursor(win32::LoadCursor(NULL, IDC_WAIT));
            FGenerateTurn();
            win32::SetCursor(hcurSav);
            HideProgressGauge();
            if (ini.fGen) {
                win32::PostQuitMessage(vretExitValue);
            } else {
                EnsureAis();
                goto Loop;
            }
        }
    Done:
        fProcessingTimer = FALSE;
        fFileErrSilent = fSav;
    }
    return;
}

void GetWindowRc(HWND hwnd, RECT *prc) {
    WINDOWPLACEMENT wndpl;

    wndpl.length = 22;
    win32::GetWindowPlacement(hwnd, &wndpl);
    *prc = wndpl.rcNormalPosition;
    prc->right -= prc->left;
    prc->bottom -= prc->top;
    return;
}

void SetWindowIniString(char *sz, HWND hwnd) {
    char ch;
    RECT rc;

    if (win32::IsZoomed(hwnd) != 0) {
        ch = 'M';
    } else if (win32::IsIconic(hwnd) != 0) {
        ch = 'I';
    } else {
        ch = 'R';
    }
    GetWindowRc(hwnd, &rc);
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsC04d04d04d04d), ch, rc.left, rc.top, rc.right, rc.bottom);
    return;
}

void WriteIniSettings() {
    int16_t  ctile;
    char     szPd[3];
    TILE    *rgtile;
    int16_t  i;
    int16_t  iPass;
    char     szEntry[16];
    char     szIniFile[16];
    char    *psz;
    char     szSection[16];
    uint16_t iCol;
    char     ch;

    szPd[0] = '%';
    szPd[1] = 'd';
    szPd[2] = 0;
    TextIO().pfnCchGetString(idsWindows, szSection);
    TextIO().pfnCchGetString(idsStarsIni, szIniFile);
    TextIO().pfnCchGetString(idsResolution, szEntry);
    i = 0;
    if (vcScreenColors <= 4) {
        i |= 1;
    }
    if (gd.mdScreenSize == 0) {
        i |= 2;
    }
    win32::wsprintf(szWork, szPd, i);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsMain, szEntry);
    SetWindowIniString(szWork, hwndFrame);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsReportfleetwin, szEntry);
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsC04d04d04d04d), 77, vrptFleet.ptDlg.x, vrptFleet.ptDlg.y, vrptFleet.ptDlg.x + vrptFleet.ptSize.x,
             vrptFleet.ptDlg.y + vrptFleet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsReportefleetwin, szEntry);
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsC04d04d04d04d), 77, vrptEFleet.ptDlg.x, vrptEFleet.ptDlg.y, vrptEFleet.ptDlg.x + vrptEFleet.ptSize.x,
             vrptEFleet.ptDlg.y + vrptEFleet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsReportbtlwin, szEntry);
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsC04d04d04d04d), 77, vrptBattle.ptDlg.x, vrptBattle.ptDlg.y, vrptBattle.ptDlg.x + vrptBattle.ptSize.x,
             vrptBattle.ptDlg.y + vrptBattle.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsReportplanwin, szEntry);
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsC04d04d04d04d), 77, vrptPlanet.ptDlg.x, vrptPlanet.ptDlg.y, vrptPlanet.ptDlg.x + vrptPlanet.ptSize.x,
             vrptPlanet.ptDlg.y + vrptPlanet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsLayout, szEntry);
    win32::wsprintf(szWork, szPd, iWindowLayout);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle1width, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dxPlanWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle1height, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dyMsgWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle1height2, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dyMinWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle2width, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dx2PlanWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle2height, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dy2MsgWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsStyle2height2, szEntry);
    win32::wsprintf(szWork, szPd, vfs.dy2MinWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsToolbar, szEntry);
    win32::wsprintf(szWork, szPd, gd.fToolbar);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    iPass = 2;
    rgtile = rgtilePlanet;
    ctile = 6;
    TextIO().pfnCchGetString(idsPlanettiles, szEntry);
    while (iPass-- != 0) {
        psz = szWork;
        iCol = 0;
        i = 0;
        while (i < ctile) {
            while (rgtile[i].iCol > iCol) {
                iCol++;
                *psz = '*';
                psz++;
            }
            *psz = !rgtile[i].fPopped ? 97 : 65;
            *psz += rgtile[i].id;
            i++;
            psz++;
        }
        *psz = 0;
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsShiptiles, szEntry);
        rgtile = rgtileShip;
        ctile = 7;
    }
    TextIO().pfnCchGetString(idsSelection, szEntry);
    switch (sel.grobj) {
    default:
        break;
    case grobjNone:
        ch = 'N';
        break;
    case grobjPlanet:
        ch = 'P';
        break;
    case grobjFleet:
        ch = 'S';
        break;
    case grobjOther:
        ch = 'E';
    }
    win32::wsprintf(szWork, TextIO().pfnCompressed(idsCCD), ch, (int16_t)(int8_t)LOBYTE(idPlayer + 66), sel.id);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsMessage, szEntry);
    win32::wsprintf(szWork, PCTD, iMsgCur);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsGameid, szEntry);
    win32::wsprintf(szWork, "%lx", game.lid);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsScanzoom, szEntry);
    szWork[0] = iScanZoom + '5';
    szWork[1] = 0;
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    if (gd.fChgScanner) {
        itoa(grbitScan, szWork, 10);
        TextIO().pfnCchGetString(idsScanmodev25, szEntry);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        itoa(grbitScanShip, szWork, 10);
        TextIO().pfnCchGetString(idsScanfilterv25, szEntry);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        itoa(grbitScanEShip, szWork, 10);
        TextIO().pfnCchGetString(idsScanefilterv25, szEntry);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        itoa(grbitScanMines, szWork, 10);
        TextIO().pfnCchGetString(idsScanmines, szEntry);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        itoa(vpctRadarView, szWork, 10);
        TextIO().pfnCchGetString(idsScanradar, szEntry);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    }
    itoa(cMinGrafMax, szWork, 10);
    TextIO().pfnCchGetString(idsMineralscale, szEntry);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    if (idPlayer != iplrNone) {
        TextIO().pfnCchGetString(idsFiles, szSection);
        TextIO().pfnCchGetString(idsWait2, szEntry);
        szWork[0] = (uTimerId != 0) + '0';
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        if (gd.fWriteTurnNum) {
            itoa(game.turn, szWork, 10);
            TextIO().pfnCchGetString(idsTurn, szEntry);
            ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
            gd.fWriteTurnNum = FALSE;
        }
        TextIO().pfnCchGetString(idsFile1, szEntry);
        win32::wsprintf(szWork, "%s.m%d", szBase, idPlayer + 1);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    }
    TextIO().pfnCchGetString(idsMisc, szSection);
    if (gd.fChgReports) {
        TextIO().pfnCchGetString(idsReportplanfld, szEntry);
        win32::wsprintf(szWork, PCTD, LOWORD(vrptPlanet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportplansort, szEntry);
        i = vrptPlanet.icolSort;
        if (vrptPlanet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintf(szWork, PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportfleetfld, szEntry);
        win32::wsprintf(szWork, PCTD, LOWORD(vrptFleet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportfleetsort, szEntry);
        i = vrptFleet.icolSort;
        if (vrptFleet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintf(szWork, PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportefleetfld, szEntry);
        win32::wsprintf(szWork, PCTD, LOWORD(vrptEFleet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportefltsort, szEntry);
        i = vrptEFleet.icolSort;
        if (vrptEFleet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintf(szWork, PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportbtlfld, szEntry);
        win32::wsprintf(szWork, PCTD, LOWORD(vrptBattle.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportbtlsort, szEntry);
        i = vrptBattle.icolSort;
        if (vrptBattle.fAscending) {
            i |= 0x100;
        }
        win32::wsprintf(szWork, PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
        TextIO().pfnCchGetString(idsReportdefgraph, szEntry);
        win32::wsprintf(szWork, PCTD, gd.iCurGraph);
        ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    }
    TextIO().pfnCchGetString(idsHistoryinfo, szEntry);
    win32::wsprintf(szWork, PCTD, uDateInstalled);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    TextIO().pfnCchGetString(idsVcrspeed, szEntry);
    win32::wsprintf(szWork, PCTD, viSpeedVCR);
    ProfileTextIO().pfnWriteString(szSection, szEntry, szWork, szIniFile);
    WriteIniPresets(szIniFile);
    return;
}

void RefitFrameChildren() {
    int16_t dyMsg;
    HMENU   hmenu;
    int16_t i;
    int16_t dyMinMin;
    int16_t dyMsgMin;
    int16_t dyMin;
    int16_t dyT;
    int16_t dyTot;
    int16_t yScanner;

    if (hwndFrame && win32::IsIconic(hwndFrame) == 0) {
        switch (iWindowLayout) {
        case layoutLarge:
        default:
            if (vfs.dx - vfs.dxPlanWant < 100) {
                vfs.xTop = vfs.dx - 100;
            } else {
                vfs.xTop = vfs.dxPlanWant;
            }
            vfs.xTop = (((vfs.xTop) > (198 + win32::GetSystemMetrics(SM_CXVSCROLL))) ? (vfs.xTop) : (198 + win32::GetSystemMetrics(SM_CXVSCROLL)));
            dyMsgMin = (0xd * dyArial8 >> 1) + 0xa;
            dyMinMin = 13 * dyArial8 - 36;
            dyMsg = vfs.dyMsgWant <= dyMsgMin ? dyMsgMin : vfs.dyMsgWant;
            vfs.dyMsgWant = dyMsg;
            dyMin = vfs.dyMinWant <= dyMinMin ? dyMinMin : vfs.dyMinWant;
            vfs.dyMinWant = dyMin;
            if (vfs.dy - (dyMsg + dyMin + 16) < 50) {
                dyT = vfs.dy - 66;
                dyTot = dyMsg + dyMin;
                dyMsg = win32::MulDiv(dyT, dyMsg, dyTot);
                dyMin = win32::MulDiv(dyT, dyMin, dyTot);
                if (dyMsg < dyMsgMin) {
                    dyMin -= dyMsgMin - dyMsg;
                    dyMsg = dyMsgMin;
                } else if (dyMin < dyMinMin) {
                    dyMsg -= dyMinMin - dyMin;
                    dyMin = dyMinMin;
                }
            }
            vfs.y1 = vfs.dy - dyMin - dyMsg - 16;
            vfs.y2 = vfs.y1 + dyMsg + 8;
            if (!hwndScanner)
                break;
            if (gd.fToolbar) {
                ui::MoveWindow(hwndTb, vfs.xTop + 8, 0, vfs.dx - vfs.xTop - 8, 36, TRUE);
                yScanner = 36;
            } else {
                ui::MoveWindow(hwndTb, 0, -100, 50, 50, TRUE);
                yScanner = 0;
            }
            ui::MoveWindow(hwndScanner, vfs.xTop + 8, yScanner, vfs.dx - vfs.xTop - 8, vfs.dy - yScanner, TRUE);
            MovePlanetPane(0, 0, vfs.xTop, vfs.y1);
            ui::MoveWindow(hwndMessage, 0, vfs.y1 + 8, vfs.xTop, dyMsg, TRUE);
            ui::MoveWindow(hwndMine, 0, vfs.y2 + 8, vfs.xTop, dyMin, TRUE);
            break;
        case layoutMedium:
        case layoutSmall:
            if (vfs.dx - vfs.dx2PlanWant < 200) {
                vfs.xTop = vfs.dx - 200;
            } else {
                vfs.xTop = vfs.dx2PlanWant;
            }
            vfs.xTop = (((vfs.xTop) > (198 + win32::GetSystemMetrics(SM_CXVSCROLL))) ? (vfs.xTop) : (198 + win32::GetSystemMetrics(SM_CXVSCROLL)));
            dyMsgMin = (0xd * dyArial8 >> 1) + 0xa;
            dyMinMin = 13 * dyArial8 - 36;
            dyMsg = vfs.dy2MsgWant <= dyMsgMin ? dyMsgMin : vfs.dy2MsgWant;
            vfs.dy2MsgWant = dyMsg;
            dyMin = vfs.dy2MinWant <= dyMinMin ? dyMinMin : vfs.dy2MinWant;
            vfs.dy2MinWant = dyMin;
            if (vfs.dy - (dyMsg + 8) < 100) {
                dyMsg = vfs.dy - 108;
            }
            if (vfs.dy - (dyMin + 8) < 100) {
                dyMin = vfs.dy - 108;
            }
            vfs.y1 = vfs.dy - dyMsg - 8;
            vfs.y2 = vfs.dy - dyMin - 8;
            if (hwndScanner) {
                if (gd.fToolbar) {
                    ui::MoveWindow(hwndTb, 0, 0, vfs.dx, 36, TRUE);
                    yScanner = 36;
                } else {
                    ui::MoveWindow(hwndTb, 0, -100, 50, 50, TRUE);
                    yScanner = 0;
                }
                ui::MoveWindow(hwndScanner, vfs.xTop + 8, yScanner, vfs.dx - vfs.xTop - 8, vfs.y2 - yScanner, TRUE);
                MovePlanetPane(0, yScanner, vfs.xTop, vfs.y1 - yScanner);
                ui::MoveWindow(hwndMessage, 0, vfs.y1 + 8, vfs.xTop, dyMsg, TRUE);
                ui::MoveWindow(hwndMine, vfs.xTop + 8, vfs.y2 + 8, vfs.dx - vfs.xTop - 8, dyMin, TRUE);
            }
        }
        hmenu = GetASubMenu(hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 4);
        for (i = 130; i <= 132; i++) {
            win32::CheckMenuItem(hmenu, i, i - 130 == iWindowLayout ? MF_CHECKED : MF_UNCHECKED);
        }
    }
    return;
}

LRESULT CALLBACK TitleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    HDC         hdc;
    int16_t     i;
    HPALETTE    hpalSav;
    RECT        rc;
    int16_t     dy;
    int16_t     dxGap;
    int16_t     dx;
    int16_t     xCur;
    char       *psz;
    char        szRaceName[256];
    PAINTSTRUCT ps;
    RECT        rcWnd;
    HBRUSH      hbrSav;
    RECT        rcT;
    LOGFONT    *plf;
    HFONT       hfont;
    HFONT       hfontSav;
    HWND        hwndButton;
    int16_t     idButton;
    int16_t     fIdleSav;

    switch (msg) {
    case WM_CREATE:
        if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            fprintf(stderr, "[DEBUG-library-startup] TitleWndProc WM_CREATE begin hwnd=%p visible=%d\n",
                    (void *)hwnd, win32::IsWindowVisible(hwnd));
        if (vcScreenColors >= 8) {
            vhdibTitle = HdibLoadBigResource(IDDIB_SPLASH);
            if (!vhpalSplash && vhdibTitle) {
                vhpalSplash = HpalFromDib(vhdibTitle);
            }
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        dx = (((120) < ((rc.right - 48) / 5 - 12)) ? (120) : ((rc.right - 48) / 5 - 12));
        dxGap = (rc.right - 5 * dx) / 6;
        xCur = dxGap;
        dy = rc.bottom <= 500 ? dyArial8 * 2 : (int16_t)(5 * dyArial8) / 2;
        for (i = 0; i < 5; i++) {
            idButton = i == 3 ? IDC_LIBRARY_TUTORIAL : i == 4 ? 3 : i;
            psz = TextIO().pfnCompressed(static_cast<StringId>(i == 3 ? idsLibraryStartTutorial : idsNewGame + idButton));
            hwndButton = ui::CreateWindowEx(0, "BUTTON", psz, WS_CHILD | WS_VISIBLE | WS_TABSTOP, xCur, rc.bottom - dy - (int16_t)(5 * dyArial8) / 2, dx, dy, hwnd,
                                      (HMENU)(uintptr_t)idButton, hInst, NULL);
            if (i != 3) rghwndBtnSplash[idButton] = hwndButton;
            if (i == 2 && (szBase[0] == 0 || !FFileExists(szBase))) {
                ControlIO().pfnEnableWindow(rghwndBtnSplash[2], FALSE);
            }
            if (rc.bottom < 500) {
                ControlIO().pfnSendMessage(hwndButton, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
            }
            xCur += dx + dxGap;
        }
        InitLibraryGames(hwnd, &rc);
        InitLibraryRaces(hwnd, &rc);
        if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            fprintf(stderr, "[DEBUG-library-startup] TitleWndProc library initialized visible=%d\n", win32::IsWindowVisible(hwnd));
        ControlIO().pfnSetWindowText(rghwndBtnSplash[1], TextIO().pfnCompressed(idsLibraryOpenGame));
        // Paint while the native window is hidden: startup cleanup can delay
        // the first queued WM_PAINT after CreateWindow makes it visible.
        ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
        ControlIO().pfnUpdateWindow(hwnd);
        for (i = 0; i < 4; i++) {
            ControlIO().pfnUpdateWindow(rghwndBtnSplash[i]);
        }
        ControlIO().pfnUpdateWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TUTORIAL));
        break;
    case WM_QUERYNEWPALETTE:
    MapIt:
        if (vcScreenColors < 8)
            break;
        hdc = win32::GetDC(hwnd);
        hpalSav = win32::SelectPalette(hdc, vhpalSplash, FALSE);
        i = win32::RealizePalette(hdc);
        win32::SelectPalette(hdc, hpalSav, FALSE);
        win32::ReleaseDC(hwnd, hdc);
        if (i != 0) {
            ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
            return 1;
        }
        return 0;
    case WM_PALETTECHANGED:
        if ((HWND)wParam != hwnd)
            goto MapIt;
        break;
    case WM_DESTROY:
        if (vhdibTitle) {
            win32::GlobalUnlock(vhdibTitle);
            win32::FreeResource(vhdibTitle);
            vhdibTitle = 0;
        }
        if (fFreeingTitle)
            goto Default;
        if (gd.fExitWindows) {
            ExitWindows(vretExitValue, 0);
            goto Default;
        }
        WriteIniSettings();
        win32::PostQuitMessage(vretExitValue);
        goto Default;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LIBRARY_TUTORIAL:
            fIdleSav = gd.fNoIdleChecks;
            gd.fNoIdleChecks = TRUE;
            StartTutor(FALSE);
            gd.fNoIdleChecks = fIdleSav;
            break;
        case 0:
            DesktopChoiceIO().pfnNewGameWizard(hwnd, FALSE);
            if (!lpPlanets && game.lid == 0) {
                ui::ShowWindow(hwndFrame, SW_HIDE);
                ui::ShowWindow(hwndTitle, SW_SHOW);
                ControlIO().pfnSetFocus(hwndTitle);
                break;
            }
            if (!fFreeingTitle) {
                fFreeingTitle = TRUE;
                ControlIO().pfnDestroyWindow(hwndTitle);
                hwndTitle = 0;
            }
            ui::ShowWindow(hwndFrame, SW_SHOW);
            break;
        case IDC_LIBRARY_IMPORT:
            if (FImportLibraryGame(hwnd, NULL)) {
                RefreshLibraryGames(hwnd);
                RefreshLibraryHostGames(hwnd);
            }
            break;
        case IDC_LIBRARY_IMPORT_FOLDER:
            ShowLibraryImportFolderPicker(hwnd);
            break;
        case IDC_LIBRARY_GAMES_VIEW:
            SelectLibraryView(hwnd, FALSE);
            break;
        case IDC_LIBRARY_HOST_VIEW:
            SelectLibraryView(hwnd, TRUE);
            break;
        case IDC_LIBRARY_IMPORT_ORDERS:
            if (FImportLibraryOrders(hwnd, NULL)) RefreshLibraryHostGames(hwnd);
            break;
        case IDC_LIBRARY_GAME_EXPORT:
            FExportSelectedLibraryGame(hwnd, NULL);
            break;
        case IDC_LIBRARY_DUPLICATE:
        case IDC_LIBRARY_RENAME:
            if (FActOnSelectedLibraryGame(hwnd, LOWORD(wParam) == IDC_LIBRARY_RENAME)) {
                RefreshLibraryGames(hwnd);
                RefreshLibraryHostGames(hwnd);
            }
            break;
        case IDC_LIBRARY_GAME_DELETE:
            if (FDeleteSelectedLibraryGame(hwnd)) {
                RefreshLibraryGames(hwnd);
                RefreshLibraryHostGames(hwnd);
            }
            break;
        case IDC_LIBRARY_RACES:
            if (HIWORD(wParam) != LBN_DBLCLK || !FSelectLibraryRace(hwnd, szRaceName)) break;
            if (FEditLibraryRace(hwnd, szRaceName)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_RACE_NEW:
            if (FBeginLibraryRace(hwnd)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_RACE_EDIT:
            if (FSelectLibraryRace(hwnd, szRaceName) && FEditLibraryRace(hwnd, szRaceName)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_RACE_DUPLICATE:
            if (FSelectLibraryRace(hwnd, szRaceName) && FDuplicateLibraryRace(hwnd, szRaceName)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_RACE_EXPORT:
            if (FSelectLibraryRace(hwnd, szRaceName)) FExportLibraryRace(hwnd, szRaceName, NULL);
            break;
        case IDC_LIBRARY_RACE_DELETE:
            if (FSelectLibraryRace(hwnd, szRaceName) && FDeleteLibraryRace(hwnd, szRaceName)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_RACE_IMPORT:
            if (FImportLibraryRaceAndEdit(hwnd, NULL)) RefreshLibraryRaces(hwnd);
            break;
        case IDC_LIBRARY_GAMES:
            if (HIWORD(wParam) == LBN_SELCHANGE) { RefreshLibraryPlayers(hwnd); break; }
            if (HIWORD(wParam) != LBN_DBLCLK) break;
            if (!FSelectLibraryGame(hwnd)) break;
            goto LOpenGame;
        case IDC_LIBRARY_PLAYERS:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                ControlIO().pfnEnableWindow(rghwndBtnSplash[1], ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_GETCURSEL, 0, 0) != CB_ERR);
            break;
        case IDC_LIBRARY_HOST_GAMES:
            if (HIWORD(wParam) != LBN_DBLCLK) break;
            if (!FSelectLibraryHostGame(hwnd)) break;
            goto LOpenGame;
        case IDOK:
            if (!FSelectLibraryGame(hwnd)) break;
        LOpenGame:
            if (FOpenGame(hwnd, FALSE) > 0) {
                if (!fFreeingTitle) {
                    fFreeingTitle = TRUE;
                    ControlIO().pfnDestroyWindow(hwndTitle);
                    hwndTitle = 0;
                }
                if (idPlayer != iplrNone) {
                    ui::ShowWindow(hwndFrame, SW_SHOW);
                }
                RememberLibraryGame();
                InitializeMenu(NULL);
                win32::PostMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                if (game.fTutorial && idPlayer == 0) {
                    StartTutor(FALSE);
                }
            } else if (win32::IsWindow(hwndTitle)) {
                ControlIO().pfnSetFocus(hwnd);
            }
            ini.fStartupFile = FALSE;
            break;
        case IDCANCEL:
            if (!FContinueLibraryGame(hwnd)) break;
            goto LOpenGame;
        case 3:
            if (gd.fExitWindows) {
                ExitWindows(vretExitValue, 0);
            } else {
                WriteIniSettings();
                win32::PostQuitMessage(vretExitValue);
            }
        }
        break;
    case WM_STARS_IMPORT_FOLDER:
        if (auto *pszPath = static_cast<char *>(PvCompatMessagePayload(wParam))) {
            if (FImportLibraryGame(hwnd, pszPath)) {
                RefreshLibraryGames(hwnd);
                RefreshLibraryHostGames(hwnd);
            }
        }
        break;
    case WM_STARS_EXPORT_FOLDER:
        if (auto *pszPath = static_cast<char *>(PvCompatMessagePayload(wParam))) {
            FExportSelectedLibraryGame(hwnd, pszPath);
        }
        break;
    case WM_PAINT:
        if (win32::IsIconic(hwnd) != 0) {
            hdc = win32::BeginPaint(hwnd, &ps);
            win32::DrawIcon(hdc, 2, 2, hiconStars);
            win32::EndPaint(hwnd, &ps);
            break;
        }
        plf = (LOGFONT *)win32::LocalAlloc(64, sizeof(LOGFONT));
        plfOwner.reset(plf);
        hdc = win32::BeginPaint(hwnd, &ps);
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonFace);
        ControlIO().pfnGetClientRect(hwnd, &rcWnd);
        if (vcScreenColors >= 8) {
            win32::SetTextColor(hdc, 50298879);
            if (!vhdibTitle)
                goto LTry16Color;
            win32::SelectPalette(hdc, vhpalSplash, FALSE);
            win32::RealizePalette(hdc);
            DibBlt(hdc, 0, 0, rcWnd.right, rcWnd.bottom, vhdibTitle, 0, 0, 800, 600, 13369376);
        } else {
        LTry16Color:
            rc.left = 0;
            rc.right = rcWnd.right;
            rc.bottom = 0;
            dx = rcWnd.right;
            DrawABunchOfStars(hdc, &rcWnd);
            plf->lfHeight = (int16_t)-rcWnd.bottom / 3;
            strcpy(plf->lfFaceName, rgszArial[3]);
            hfont = win32::CreateFontIndirect(plf);
            win32::SetTextColor(hdc, 10158235);
            if (hfont) {
                hfontSav = (HFONT)win32::SelectObject(hdc, hfont);
                win32::SetBkMode(hdc, TRANSPARENT);
                rcT = rcWnd;
                rcT.bottom = (int16_t)(3 * rcT.bottom) / 4;
                RcCtrTextOut(hdc, &rcT, "Stars!", 6);
                win32::SelectObject(hdc, hfontSav);
                win32::DeleteObject(hfont);
            }
        }
        win32::SelectObject(hdc, rghfontArial10[1]);
        win32::SetBkMode(hdc, TRANSPARENT);
        SzVersion();
        win32::GetWindowRect(rghwndBtnSplash[0], &rcT);
        win32::MapWindowPoints(NULL, hwnd, (POINT *)&rcT, 2);
        rcWnd.top = rcT.top - (int16_t)(9 * dyArial8) / 2;
        rcWnd.bottom = (int16_t)(3 * dyArial8) / 2 + rcWnd.top;
        RcCtrTextOut(hdc, &rcWnd, szWork, strlen(szWork));
        win32::EndPaint(hwnd, &ps);
        plfOwner.reset();
        break;
    default:
    Default:
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void RestoreGameWindows() {
    if (!hwndScanner)
        return;
    gd.fNoScannerDraw = TRUE;
    RestoreSelection();
    ControlIO().pfnRefitFrameChildren();
    gd.fNoScannerDraw = FALSE;
    ControlIO().pfnInvalidateRect(hwndScanner, NULL, TRUE);
    ControlIO().pfnUpdateWindow(hwndScanner);
    return;
}

int16_t FinishOpenGame() {
    GrobjClass grobjIni;
    if (idPlayer != iplrNone) {
        grobjIni = static_cast<GrobjClass>(ini.grobjSel);
        ControlIO().pfnSendMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (grobjIni != grobjNone && ini.grobjSel == 0) {
            return 1;
        }
        ini.grobjSel = 0;
    } else {
        if (hwndTitle && !fFreeingTitle) {
            fFreeingTitle = TRUE;
            ControlIO().pfnDestroyWindow(hwndTitle);
            hwndTitle = 0;
        }
        ControlIO().pfnBringUpHostDlg();
        return 0;
    }
    if (ini.grobjSel == 0 && cPlanet != 0) {
        PresentationIO().pfnFindSelection();
    }
    return 1;
}
