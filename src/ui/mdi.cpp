#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
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
#include "file_selection_ui.h"
#include <stars_compat.h>
#include "ownership.h"
#include <SDL3/SDL.h>

char rgTOWidth[2][2] = {{-3}, {2, 1}};

std::int16_t InitMDIApp(APPSESSION &app) {
    WNDCLASS wc;

    wc.style = 11;
    wc.lpfnWndProc = FrameWndProcDeferred;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = app.lpui->hInst;
    wc.hIcon = win32::LoadIconA(app.lpui->hInst, "StarsIco");
    wc.hCursor = win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(13);
    wc.lpszMenuName = "StarsMenu";
    wc.lpszClassName = app.lpui->szFrame;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 523;
    wc.lpfnWndProc = MessageWndProc;
    wc.hIcon = nullptr;
    wc.lpszMenuName = nullptr;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.lpszClassName = app.lpui->szMessage;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 523;
    wc.lpfnWndProc = ScannerWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH));
    wc.lpszClassName = app.lpui->szScan;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 523;
    wc.lpfnWndProc = MineWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.lpszClassName = app.lpui->szMine;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 520;
    wc.lpfnWndProc = TbWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.lpszClassName = app.lpui->szTb;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 0x200;
    wc.lpfnWndProc = PlanetWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.hIcon = nullptr;
    wc.lpszClassName = app.lpui->szPlanet;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.lpfnWndProc = PlanetPaneWndProc;
    wc.lpszClassName = "StarsPlanetPane";
    if (win32::RegisterClassA(&wc) == 0) return 0;
    wc.style = 2560;
    wc.lpfnWndProc = PopupWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::WHITE_BRUSH));
    wc.hIcon = nullptr;
    wc.lpszClassName = app.lpui->szPopup;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 2560;
    wc.lpfnWndProc = TooltipWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::WHITE_BRUSH));
    wc.hIcon = nullptr;
    wc.lpszClassName = app.lpui->szTooltip;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 0x200;
    wc.lpfnWndProc = BrowserWndProc;
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.hIcon = nullptr;
    wc.lpszClassName = app.lpui->szBrowser;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 0;
    wc.lpfnWndProc = TitleWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = app.lpui->hInst;
    wc.hIcon = nullptr;
    wc.hCursor = win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512));
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = app.lpui->szTitle;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    wc.style = 11;
    wc.lpfnWndProc = ReportDlg;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = app.lpui->hInst;
    wc.hIcon = nullptr;
    wc.hCursor = win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512));
    wc.hbrBackground = static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = app.lpui->szReport;
    if (win32::RegisterClassA(&wc) == 0) {
        return 0;
    }
    return 1;
}

void CreateChildWindows(APPSESSION &app) {
    std::array<char, 50> szPlayerName{};
    char    szData[512];
    char    szNameUtf8[128], szPlayerUtf8[256];
    std::size_t  cch;
    POINT16 pt;
    char   *psz;
    char    szGame[15];

    if (app.state.idPlayer != iplrNone) {
        psz = std::strrchr(app.state.szBase, chDirSep);
#ifdef _WIN32
        if (!psz) psz = std::strrchr(app.state.szBase, '/');
        if (!psz) psz = std::strrchr(app.state.szBase, ':');
#endif
        psz = psz ? psz + 1 : app.state.szBase;
        cch = CchUtf8Prefix(psz, 8);
        std::memcpy(szGame, psz, cch); szGame[cch] = 0;
        for (psz = szGame; *psz; psz++) if (*psz >= 'A' && *psz <= 'Z') *psz += 'a' - 'A';
        win32::wsprintfA(std::span{szGame}.subspan(static_cast<std::size_t>(std::strlen(szGame))), ".m%d", app.state.idPlayer + 1);
        CchCopyUtf8(std::span{szNameUtf8}, app.state.game.szName);
        szPlayerName = TextIO().pfnPlayerName(app, app.state.idPlayer, 0, 1, 0, 0, nullptr);
        psz = szPlayerName.data();
        CchCopyUtf8(std::span{szPlayerUtf8}, psz);
        CchSnprintf(std::span{szData}, "Stars! -- %s -- %s -- %s", szNameUtf8, szPlayerUtf8, szGame);
    } else {
        TextIO().pfnCchGetString(app, idsStarsSHostMode, std::span{app.state.szWork});
        win32::wsprintfA(szData, app.state.szWork, app.state.game.szName);
        CchCopyUtf8(std::span{szNameUtf8}, szData);
        CchCopyUtf8Prefix(std::span{szData}, szNameUtf8);
    }
    FSetNativeWindowTitle(app.lpui->hwndFrame, szData);
    if (app.state.idPlayer != iplrNone) {
        if (!app.lpui->hwndScanner) {
            app.lpui->hwndScanner = ui::CreateWindowEx(0, app.lpui->szScan, nullptr, win32::WS_CHILD | win32::WS_VISIBLE, -200, -200, 10, 10, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        } else {
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            app.lpui->yScanTop = 1000;
            app.lpui->xScanTop = 1000;
            SetScanScrollBars(app, app.lpui->hwndScanner);
        }
        if (!app.lpui->hwndMine) {
            app.lpui->hwndMine = ui::CreateWindowEx(0, app.lpui->szMine, nullptr, win32::WS_CHILD | win32::WS_VISIBLE, -500, -500, pt.x, pt.y, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        } else {
            ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
        }
        if (!app.lpui->hwndPlanet) {
            app.lpui->hwndPlanet = CreatePlanetPane(app, app.lpui->hwndFrame);
        } else {
            ControlIO().pfnInvalidateRect(app.lpui->hwndPlanet, nullptr, 1);
        }
        if (!app.lpui->hwndTb) {
            app.lpui->hwndTb = ui::CreateWindowEx(0, app.lpui->szTb, nullptr, win32::WS_CHILD | win32::WS_VISIBLE, -500, -500, 10, 10, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        } else {
            ControlIO().pfnInvalidateRect(app.lpui->hwndTb, nullptr, 1);
        }
        if (app.lpui->hwndMessage) {
            ControlIO().pfnDestroyWindow(app.lpui->hwndMessage);
        }
        app.lpui->hwndMessage = ui::CreateWindowEx(0, app.lpui->szMessage, nullptr, win32::WS_CHILD | win32::WS_VISIBLE, -500, -500, 10, 10, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnRefitFrameChildren(app);
    }
    return;
}

// PostOpenGame finishes opening a game GenerateWorld just created, as the
// File menu does after opening one.
void PostOpenGame(APPSESSION &app) {
    ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
    return;
}

LRESULT  FrameWndProc(APPSESSION &app, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC          hdc;
    std::int16_t      i;
    HPALETTE     hpalSav;
    TEXTMETRIC   tm;
    POINT16      pt;
    std::int16_t      fRet;
    std::int16_t      fErrSav;
    std::int16_t      idCur;
    std::int16_t      id;
    std::uint16_t     uTimerIdOld;
    char         szExt[4];
    std::int16_t      iOffset;
    std::int16_t      idPlanet;
    RECT         rc;
    RECT         rc2;
    PAINTSTRUCT  ps;
    std::int16_t      yOffset;
    HBRUSH       hbrSav;
    HCURSOR      hcs;
    POINT16      ptOld;
    PaneSplitter grSel;
    std::int16_t      grSelRaw;
    POINT16      ptAct;
    POINT16      ptD;
    POINT16      ptStart;
    POINT16      ptChg;

    switch (msg) {
    case win32::WM_CREATE:
        hdc = win32::GetDC(hwnd);
        FCreateFonts(app, hdc);
        win32::GetTextMetricsA(hdc, &tm);
        app.lpui->dySysFont = tm.tmHeight;
        app.lpui->dySBar = (app.state.dyArial8 + 12) * 2;
        win32::ReleaseDC(hwnd, hdc);
        InitTiles(app);
        EnsureTileSize(app, app.lpui->iWindowLayout == layoutSmall);
        return 0;
    case win32::WM_ACTIVATE:
        return 0;
    case win32::WM_QUERYNEWPALETTE:
    MapIt:
        if (app.lpui->hwndTitle) {
            return ControlIO().pfnSendMessage(app.lpui->hwndTitle, msg, wParam, lParam);
        }
        hdc = win32::GetDC(hwnd);
        hpalSav = win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        i = win32::RealizePalette(hdc);
        win32::SelectPalette(hdc, hpalSav, 0);
        win32::ReleaseDC(hwnd, hdc);
        if (i != 0) {
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
            return 1;
        }
        return 0;
    case win32::WM_PALETTECHANGED:
        if (reinterpret_cast<HWND>(wParam) != hwnd)
            goto MapIt;
        return 0;
    case win32::WM_INITMENU:
        InitializeMenu(app, reinterpret_cast<HMENU>(wParam));
        return 0;
    case WM_STARS_STARTUP:
        app.state.idPlayer = iplrNone;
        if (app.state.ini.fCmdLine) {
            app.state.ini.fCmdLine = 0;
            if (FRunCmdLine(app)) {
            LExit:
                if (app.state.gd.fExitWindows) {
                    ui::RequestQuit(app.state.vretExitValue);
                    return 0;
                }
                win32::PostQuitMessage(app.state.vretExitValue);
                return 0;
            }
            CommandHandler(app, hwnd, 0xed9);
            if (app.state.ini.fTry)
                goto LExit;
            if (app.state.ini.fGen)
                goto LNop;
            if (app.state.game.lid == 0)
                goto LShowStartup;
            if (app.state.idPlayer != iplrNone && (app.state.ini.fDumpPlanets || app.state.ini.fDumpFleets || app.state.ini.fDumpMap)) {
                if (app.state.ini.fDumpMap) {
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_DEBUG_DUMP_UNIVERSE, 0);
                }
                if (app.state.ini.fDumpPlanets) {
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_DEBUG_DUMP_PLANETS, 0);
                }
                if (app.state.ini.fDumpFleets) {
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_DEBUG_DUMP_FLEETS, 0);
                }
                goto LExit;
            } else {
                ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
                InitializeMenu(app, nullptr);
                win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                if (!app.state.ini.fWait)
                    goto LNop;
                app.state.ini.fWait = 0;
                CommandHandler(app, hwnd, 0x6a);
                goto LNop;
            }
        }
    LShowStartup:
        if (!app.lpui->hwndTitle) {
            ShowTitleScreen(app);
        }
        app.state.ini.fStartupFile = 0;
        DestroyCurGame(app);
    LNop:
        return 0;
    case win32::WM_ENTERIDLE:
        if (app.state.gd.fTutorial && app.state.tutor.fChange) {
            PresentationIO().pfnTutor(app);
        }
        return 0;
    case win32::WM_SYSCOLORCHANGE:
    case win32::WM_WININICHANGE:
        FGetSystemColors(app);
        return 0;
    case win32::WM_GETMINMAXINFO:
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.x = 520;
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.y = 380;
        return 0;
    case win32::WM_SIZE:
        if (wParam != 2 && wParam != 0)
            goto Default;
        app.lpui->vfs.dx = LOWORD(lParam);
        app.lpui->vfs.dy = HIWORD(lParam);
        ControlIO().pfnRefitFrameChildren(app);
        return 0;
    case win32::WM_COMMAND:
        CommandHandler(app, hwnd, wParam);
        return 0;
    case win32::WM_CLOSE:
        ControlIO().pfnDestroyWindow(hwnd);
        return 0;
    case WM_STARS_HOST:
        ControlIO().pfnBringUpHostDlg(app);
        return 1;
    case WM_STARS_CONTINUE:
        ShowTutor(app, 0);
        app.state.game.fDirty = 0;
        DestroyCurGame(app);
        fErrSav = app.state.fFileErrSilent;
        app.state.fFileErrSilent = 1;
        app.state.gd.fDontDoLogFiles = 1;
        if (!FLoadGame(app, app.state.szBase, "m1")) {
            app.state.fFileErrSilent = fErrSav;
            app.state.gd.fDontDoLogFiles = 0;
            return 0;
        }
        app.state.gd.fDontDoLogFiles = 0;
        app.state.fFileErrSilent = fErrSav;
        app.state.idPlayer = 0;
        if (wParam == 2506) {
            app.state.gd.fGeneratingTurn = 1;
            win32::wsprintfA(app.state.szWork, "%s.x1", app.state.szBase);
            if (FLoadLogFile(app, app.state.szWork)) {
                FRunLogFile(app);
            }
            app.state.gd.fGeneratingTurn = 0;
        }
        CreateChildWindows(app);
        ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (wParam == 2506) {
            ControlIO().pfnSendMessage(app.lpui->hwndMessage, win32::WM_KEYDOWN, win32::VK_END, 0);
        }
        app.state.tutor.idt = idtWelcomeStarsTutorialWillGuideThrough36;
        app.state.tutor.fTurnDone = 0;
        app.state.tutor.fAutoComplete = wParam == 2506;
        PresentationIO().pfnTutor(app);
        return 0;
    case win32::WM_SYSCOMMAND:
        if ((wParam & 0xfff0) != 0xf030 && (wParam & 0xfff0) != 0xf120)
            goto Default;
        idCur = app.state.idPlayer;
        uTimerIdOld = app.lpui->uTimerId;
        if (app.lpui->uTimerId != 0) {
            win32::KillTimer(nullptr, app.lpui->uTimerId);
            app.lpui->uTimerId = 0;
            CreateChildWindows(app);
        }
        if (app.state.idPlayer != iplrNone && FNewTurnAvail(app, app.state.idPlayer)) {
            if (uTimerIdOld != 0) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsNewTurnAvailable, {}).data(), win32::MB_ICONASTERISK);
                id = 6;
            } else {
                id = AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsNewTurnAvailableWouldLikeLoad, {}).data(), win32::MB_YESNOCANCEL | win32::MB_ICONQUESTION | win32::MB_TASKMODAL);
            }
            if (id == 6) {
                win32::wsprintfA(szExt, app.state.MPCTD, app.state.idPlayer + 1);
                DestroyCurGame(app);
                if (!FLoadGame(app, app.state.szBase, szExt)) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, {}).data(), win32::MB_ICONHAND);
                } else {
                    CreateChildWindows(app);
                }
            } else if (id == 2) {
                if (uTimerIdOld != 0) {
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_TURN_WAIT_NEW, 0);
                } else {
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_SYSCOMMAND, 0xf020, 0);
                }
                return 1;
            }
            ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            goto Default;
        }
        if (uTimerIdOld == 0)
            goto Default;
        if (app.lpui->uTimerType == hostTimerAutoGen) {
            win32::PostMessageA(hwnd, WM_STARS_HOST, 0, 0);
            goto Default;
        }
        if (app.lpui->uTimerType == hostTimerWaitTurn) {
            id = AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsTurnHasSubmittedChangesMadeAfterTurn, {}).data(), win32::MB_YESNOCANCEL | win32::MB_ICONQUESTION | win32::MB_TASKMODAL);
            if (id == 6 && !FMarkFile(app, dtLog, app.state.idPlayer, mdMarkDone, 0)) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsNewTurnCurrentlyGeneratedHostNewTurn, {}).data(), win32::MB_ICONHAND);
                win32::wsprintfA(szExt, app.state.MPCTD, app.state.idPlayer + 1);
                DestroyCurGame(app);
                if (!FLoadGame(app, app.state.szBase, szExt)) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, {}).data(), win32::MB_ICONHAND);
                } else {
                    CreateChildWindows(app);
                }
            } else if (id == 2) {
                win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_TURN_WAIT_NEW, 0);
                return 1;
            }
        }
        ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (app.state.sel.pt.x <= 1000 || app.state.sel.pt.y <= 1000)
            goto Default;
        CtrPointScan(app, app.state.sel.pt, 1);
        goto Default;
    case win32::WM_CHAR:
        if (app.lpui->hwndScanner && (wParam == '-' || wParam == '+')) {
            ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, wParam, lParam);
            return 0;
        }
        if (app.lpui->hwndMessage && (wParam == '-' || wParam == '+' || wParam == '\r')) {
            ControlIO().pfnSendMessage(app.lpui->hwndMessage, win32::WM_CHAR, wParam, lParam);
            return 0;
        }
        if (app.lpui->hwndPlanet && (wParam == 'f' || wParam == 'F')) {
            ControlIO().pfnSendMessage(app.lpui->hwndPlanet, win32::WM_CHAR, wParam, lParam);
            return 0;
        }
        if (app.lpui->hwndPlanet && app.state.sel.grobj == grobjPlanet && (wParam == 'q' || wParam == 'Q')) {
            ChangeProduction(app, 0);
            return 0;
        }
        if (!(app.state.sel.grobj & (grobjPlanet | grobjFleet))) {
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
            if (app.state.sel.grobj == grobjPlanet) {
                idPlanet = IdFindAdjStarbase(app, app.state.sel.pl.id, wParam == 'N');
                break;
            }
            iOffset = wParam == 'N' ? 1 : -1;
            break;
        case 'p':
            iOffset = -1;
            break;
        case 'r':
        case 'R':
            if (app.state.sel.grobj == grobjFleet) {
                ShipCommandProc(app, app.lpui->hwndPlanet, 0, reinterpret_cast<LPARAM>(app.lpui->rghwndBtn[6]));
            }
        }
        if (iOffset == 0 && idPlanet == 0) {
            return 0;
        }
        if (app.state.sel.grobj == grobjFleet) {
            PresentationIO().pfnSelectFleet(app, iOffset, idPlanet);
            return 0;
        }
        PresentationIO().pfnSelectPlanet(app, iOffset, idPlanet);
        return 0;
    case win32::WM_QUERYDRAGICON:
        if (app.state.idPlayer == iplrNone) {
            return reinterpret_cast<LRESULT>(app.lpui->hiconHost);
        }
        if (app.lpui->uTimerId == 0) {
            return reinterpret_cast<LRESULT>(app.lpui->hiconStars);
        }
        return reinterpret_cast<LRESULT>(app.lpui->hiconWait);
    case win32::WM_ERASEBKGND:
        if (win32::IsIconic(hwnd) != 0) {
            ControlIO().pfnGetClientRect(hwnd, &rc);
            win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrDesktop);
            return 0;
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (app.lpui->hwndScanner) {
            ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc2);
            win32::MapWindowPoints(app.lpui->hwndScanner, hwnd, reinterpret_cast<POINT *>(&rc2), 2);
            win32::ExcludeClipRect(reinterpret_cast<HDC>(wParam), rc2.left, rc2.top, rc2.right, rc2.bottom);
        }
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        if (win32::IsIconic(hwnd) != 0) {
            hdc = win32::BeginPaint(hwnd, &ps);
            win32::DrawIcon(hdc, 2, 2, app.state.idPlayer == iplrNone && app.state.game.lid != 0 ? app.lpui->hiconHost : app.lpui->uTimerId == 0 ? app.lpui->hiconStars : app.lpui->hiconWait);
            win32::EndPaint(hwnd, &ps);
            return 0;
        }
        hdc = win32::BeginPaint(hwnd, &ps);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonShadow));
        switch (app.lpui->iWindowLayout) {
        case layoutLarge:
        default:
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 5, 0, 2, app.lpui->vfs.dy, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + 5, app.lpui->vfs.xTop + 2, 2, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y2 + 5, app.lpui->vfs.xTop + 2, 2, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 1, 0, 1, app.lpui->vfs.y1 + 2, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + 1, app.lpui->vfs.xTop + 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 1, app.lpui->vfs.y1 + 6, 1, app.lpui->vfs.y2 - app.lpui->vfs.y1 - 4, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y2 + 1, app.lpui->vfs.xTop + 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 1, app.lpui->vfs.y2 + 6, 1, app.lpui->vfs.dy - app.lpui->vfs.y2 - 6, win32::PATCOPY);
            break;
        case layoutMedium:
        case layoutSmall:
            yOffset = !app.state.gd.fToolbar ? 0 : 36;
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 5, yOffset, 2, app.lpui->vfs.y2 + 2 - yOffset, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + 5, app.lpui->vfs.xTop + 2, 2, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 5, app.lpui->vfs.y2 + 5, app.lpui->vfs.dx - app.lpui->vfs.xTop - 5, 2, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 5, app.lpui->vfs.y2 + 6, 2, app.lpui->vfs.dy - app.lpui->vfs.y2 - 5, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 1, yOffset, 1, app.lpui->vfs.y1 + 2 - yOffset, win32::PATCOPY);
            win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + 1, app.lpui->vfs.xTop + 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 1, app.lpui->vfs.y1 + 6, 1, app.lpui->vfs.dy - app.lpui->vfs.y1 - 6, win32::PATCOPY);
            win32::PatBlt(hdc, app.lpui->vfs.xTop + 6, app.lpui->vfs.y2 + 1, app.lpui->vfs.dx - app.lpui->vfs.xTop - 6, 1, win32::PATCOPY);
        }
        win32::SelectObject(hdc, hbrSav);
        win32::EndPaint(hwnd, &ps);
        return 0;
    case win32::WM_SETCURSOR:
        hcs = nullptr;
        if (win32::IsIconic(hwnd) != 0)
            goto Default;
        GetCursorPos16(&pt);
        ScreenToClient16(app.lpui->hwndFrame, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        hcs = HcrsFromFrameWindowPt(app, pt, nullptr);
        if (!hcs)
            goto Default;
        win32::SetCursor(hcs);
        return 1;
    case win32::WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hcs = HcrsFromFrameWindowPt(app, pt, &grSelRaw);
        if (!hcs) {
            return 0;
        }
        grSel = static_cast<PaneSplitter>(grSelRaw);
        hdc = win32::GetDC(hwnd);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbr50Screen));
        ptD.y = 0;
        ptD.x = 0;
        InvertPaneBorder(app, hdc, grSel, ptD, nullptr);
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
                ptAct = InvertPaneBorder(app, hdc, grSel, ptD, &ptChg);
                ptOld = pt;
            }
        }
        InvertPaneBorder(app, hdc, grSel, ptD, nullptr);
        win32::ReleaseCapture();
        win32::SelectObject(hdc, hbrSav);
        win32::ReleaseDC(hwnd, hdc);
        if (ptAct.x == 0 && ptAct.y == 0) {
            return 0;
        }
        if (grSel & splitVertical) {
            if (app.lpui->iWindowLayout == layoutLarge) {
                app.lpui->vfs.dxPlanWant = app.lpui->vfs.xTop + ptAct.x;
            } else {
                app.lpui->vfs.dx2PlanWant = app.lpui->vfs.xTop + ptAct.x;
            }
        }
        if (grSel & splitMessages) {
            if (app.lpui->iWindowLayout == layoutLarge) {
                app.lpui->vfs.dyMsgWant = app.lpui->vfs.y2 - app.lpui->vfs.y1 - 8 - ptAct.y;
            } else {
                app.lpui->vfs.dy2MsgWant = app.lpui->vfs.dy - app.lpui->vfs.y1 - 8 - ptAct.y;
            }
        }
        if (grSel & splitLower) {
            if (app.lpui->iWindowLayout == layoutLarge) {
                app.lpui->vfs.dyMsgWant = app.lpui->vfs.y2 - app.lpui->vfs.y1 - 8 + ptAct.y;
                app.lpui->vfs.dyMinWant = app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - ptAct.y;
            } else {
                app.lpui->vfs.dy2MinWant = app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - ptAct.y;
            }
        }
        ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
        ControlIO().pfnRefitFrameChildren(app);
        return 0;
    case win32::WM_DESTROY:
        if (app.lpui->uTimerId != 0) {
            win32::KillTimer(nullptr, app.lpui->uTimerId);
        }
        WriteIniSettings(app);
        app.lpui->uTimerId = 0;
        if (app.state.gd.fHostMode) {
            FMarkFile(app, dtHost, iplrNone, mdMarkInUse, 0);
        }
        DestroyCurGame(app);
        if (app.state.gd.fExitWindows) {
            ui::RequestQuit(app.state.vretExitValue);
            return 0;
        }
        win32::PostQuitMessage(app.state.vretExitValue);
        return 0;
    default:
    Default:
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

POINT16 InvertPaneBorder(APPSESSION &app, HDC hdc, PaneSplitter grSel, POINT16 dpt, POINT16 *pdptPrev) {
    std::int16_t notMin;
    std::int16_t dChg;
    POINT16 dptT;
    POINT16 dptPrev;
    std::int16_t dyAboveMinCur;
    POINT16 dptOld;
    std::int16_t dyMsgCur;
    std::int16_t dyMinAboveH2;
    std::int16_t dyPlanMin;
    std::int16_t dxScanMin;
    std::int16_t x;
    POINT16 pt;
    std::int16_t dyMin;

    dptOld = dpt;
    if (pdptPrev && grSel != splitVertical) {
        pt.x = dpt.x - pdptPrev->x;
        pt.y = dpt.y - pdptPrev->y;
        InvertPaneBorder(app, hdc, grSel, pt, nullptr);
    }
    switch (app.lpui->iWindowLayout) {
    case layoutLarge:
    default:
        dxScanMin = 100;
        dyPlanMin = 50;
        dyMsgCur = app.lpui->vfs.y2 - app.lpui->vfs.y1 - 8;
        dyAboveMinCur = app.lpui->vfs.y2 - app.lpui->vfs.y1 - 8;
        notMin = 1;
        dyMinAboveH2 = (0xd * app.state.dyArial8 >> 1) + 0xa;
        break;
    case layoutMedium:
    case layoutSmall:
        dxScanMin = 200;
        dyPlanMin = 100;
        dyMsgCur = app.lpui->vfs.dy - app.lpui->vfs.y1 - 8;
        dyAboveMinCur = app.lpui->vfs.y2;
        notMin = -1;
        dyMinAboveH2 = 100;
    }
    if (app.lpui->vfs.xTop + dpt.x < 198) {
        dpt.x = 198 - app.lpui->vfs.xTop;
    }
    if (app.lpui->vfs.dx - app.lpui->vfs.xTop - dpt.x < dxScanMin) {
        dpt.x = app.lpui->vfs.dx - app.lpui->vfs.xTop - dxScanMin;
    }
    if (app.lpui->vfs.xTop + dpt.x > 396) {
        dpt.x = 396 - app.lpui->vfs.xTop;
    }
    if (grSel & splitMessages) {
        if (app.lpui->vfs.y1 + dpt.y < dyPlanMin) {
            dpt.y = dyPlanMin - app.lpui->vfs.y1;
        } else {
            dyMin = (0xd * app.state.dyArial8 >> 1) + 0xa;
            if (dyMsgCur - dpt.y < dyMin) {
                dpt.y = dyMsgCur - dyMin;
            }
        }
    }
    if (grSel & splitLower) {
        if (dyAboveMinCur + dpt.y < dyMinAboveH2) {
            dpt.y = (dyMinAboveH2 - dyAboveMinCur) * notMin;
        } else {
            dyMin = 13 * app.state.dyArial8 - 36;
            if (app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - dpt.y < dyMin) {
                dpt.y = app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - dyMin;
            }
        }
    }
    if (pdptPrev) {
        dptT = dpt;
        dpt.x = dptOld.x - pdptPrev->x;
        dpt.y = dptOld.y - pdptPrev->y;
        if (app.lpui->vfs.xTop + dpt.x < 198) {
            dpt.x = 198 - app.lpui->vfs.xTop;
        }
        if (app.lpui->vfs.dx - app.lpui->vfs.xTop - dpt.x < dxScanMin) {
            dpt.x = app.lpui->vfs.dx - app.lpui->vfs.xTop - dxScanMin;
        }
        if (app.lpui->vfs.xTop + dpt.x > 396) {
            dpt.x = 396 - app.lpui->vfs.xTop;
        }
        if (grSel & splitMessages) {
            if (app.lpui->vfs.y1 + dpt.y < 50) {
                dpt.y = 50 - app.lpui->vfs.y1;
            } else {
                dyMin = (0xd * app.state.dyArial8 >> 1) + 0xa;
                if (dyMsgCur - dpt.y < dyMin) {
                    dpt.y = dyMsgCur - dyMin;
                }
            }
        } else if (grSel & splitLower) {
            if (dyAboveMinCur + dpt.y < dyMinAboveH2) {
                dpt.y = (dyMinAboveH2 - dyAboveMinCur) * notMin;
            } else {
                dyMin = 13 * app.state.dyArial8 - 36;
                if (app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - dpt.y < dyMin) {
                    dpt.y = app.lpui->vfs.dy - app.lpui->vfs.y2 - 8 - dyMin;
                }
            }
        }
        dptPrev.x = dptT.x - dpt.x;
        dptPrev.y = dptT.y - dpt.y;
        dpt = dptT;
    }
    // Splitter values can combine the three boundary bits.
    switch (static_cast<std::uint16_t>(grSel)) {
    default:
        dpt.y = 0;
        if (!pdptPrev || std::abs(dptPrev.x) >= 6) {
            if (pdptPrev) {
                win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x - dptPrev.x + 1, 0, 6, app.lpui->vfs.dy, win32::PATINVERT);
            }
            win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 1, 0, 6, app.lpui->vfs.dy, win32::PATINVERT);
            break;
        }
        dChg = dptPrev.x;
        if (dChg == 0)
            break;
        if (dChg < 0) {
            x = app.lpui->vfs.xTop + dpt.x + 1;
            dChg = -dChg;
        } else {
            x = app.lpui->vfs.xTop + dpt.x - dptPrev.x + 1;
        }
        win32::PatBlt(hdc, x, 0, dChg, app.lpui->vfs.dy, win32::PATINVERT);
        win32::PatBlt(hdc, x + 6, 0, dChg, app.lpui->vfs.dy, win32::PATINVERT);
        break;
    case splitMessages:
        dpt.x = 0;
        win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + dpt.y + 1, app.lpui->vfs.xTop + 1, 6, win32::PATINVERT);
        break;
    case splitLower:
        dpt.x = 0;
        if (app.lpui->iWindowLayout == layoutLarge) {
            win32::PatBlt(hdc, 0, app.lpui->vfs.y2 + dpt.y + 1, app.lpui->vfs.xTop + 1, 6, win32::PATINVERT);
            break;
        }
        win32::PatBlt(hdc, app.lpui->vfs.xTop + 7, app.lpui->vfs.y2 + dpt.y + 1, app.lpui->vfs.dx - app.lpui->vfs.xTop - 7, 6, win32::PATINVERT);
        break;
    case 3:
        win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 1, 0, 6, app.lpui->vfs.dy, win32::PATINVERT);
        win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + dpt.y + 1, app.lpui->vfs.xTop + dpt.x + 1, 6, win32::PATINVERT);
        break;
    case 5:
        win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 1, 0, 6, app.lpui->vfs.dy, win32::PATINVERT);
        if (app.lpui->iWindowLayout == layoutLarge) {
            win32::PatBlt(hdc, 0, app.lpui->vfs.y2 + dpt.y + 1, app.lpui->vfs.xTop + dpt.x + 1, 6, win32::PATINVERT);
            break;
        }
        win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 7, app.lpui->vfs.y2 + dpt.y + 1, app.lpui->vfs.dx - dpt.x - app.lpui->vfs.xTop - 7, 6, win32::PATINVERT);
        break;
    case 7:
        win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 1, 0, 6, app.lpui->vfs.dy, win32::PATINVERT);
        win32::PatBlt(hdc, 0, app.lpui->vfs.y1 + dpt.y + 1, app.lpui->vfs.xTop + dpt.x + 1, 6, win32::PATINVERT);
        win32::PatBlt(hdc, app.lpui->vfs.xTop + dpt.x + 7, app.lpui->vfs.y2 + dpt.y + 1, app.lpui->vfs.dx - dpt.x - app.lpui->vfs.xTop - 7, 6, win32::PATINVERT);
    }
    return dpt;
}

HCURSOR HcrsFromFrameWindowPt(APPSESSION &app, POINT16 pt, std::int16_t *pgrSel) {
    HCURSOR hcs;
    std::int16_t fInHBar2;
    std::int16_t fInHBar1;
    std::int16_t fInVBar;

    hcs = nullptr;
    fInVBar = pt.x >= app.lpui->vfs.xTop && pt.x < app.lpui->vfs.xTop + 8;
    fInHBar1 = pt.x < app.lpui->vfs.xTop + 8 && pt.y >= app.lpui->vfs.y1 && pt.y < app.lpui->vfs.y1 + 8;
    switch (app.lpui->iWindowLayout) {
    case layoutLarge:
    default:
        if (pt.x < app.lpui->vfs.xTop + 8 && pt.y >= app.lpui->vfs.y2 && pt.y < app.lpui->vfs.y2 + 8) {
            fInHBar2 = 1;
            break;
        }
        fInHBar2 = 0;
        break;
    case layoutMedium:
    case layoutSmall:
        fInHBar2 = pt.x >= app.lpui->vfs.xTop && pt.y >= app.lpui->vfs.y2 && pt.y < app.lpui->vfs.y2 + 8;
    }
    if (!fInVBar) {
        if (fInHBar1 || fInHBar2) {
            hcs = app.lpui->hcurResizeNS;
        }
    } else if (fInHBar1 || fInHBar2) {
        hcs = app.lpui->hcurResize4Way;
    } else {
        hcs = app.lpui->hcurResizeWE;
    }
    if (pgrSel) {
        *pgrSel = fInHBar1 * 2 + fInVBar + fInHBar2 * 4;
    }
    return hcs;
}

void RestoreSelection(APPSESSION &app) {
    PLANET *lppl;

    if (app.state.ini.idPlayer == app.state.idPlayer && app.state.ini.lid == app.state.game.lid) {
        if (app.state.ini.grobjSel == 2) {
            if (LpflFromId(app, app.state.ini.iObjSel)) {
                PresentationIO().pfnSelectFleet(app, 0, app.state.ini.iObjSel);
                app.state.ini.grobjSel = 0;
            } else {
                app.state.ini.grobjSel = 1;
                app.state.ini.iObjSel = app.state.rgplr[app.state.idPlayer].idPlanetHome;
            }
        } else if (app.state.ini.grobjSel == 1) {
            lppl = LpplFromId(app, app.state.ini.iObjSel);
            if (!lppl || lppl->iPlayer != app.state.idPlayer) {
                app.state.ini.iObjSel = app.state.rgplr[app.state.idPlayer].idPlanetHome;
            } else {
                PresentationIO().pfnSelectPlanet(app, 0, app.state.ini.iObjSel);
                app.state.ini.grobjSel = 0;
            }
        }
        if (app.state.ini.turn == app.state.game.turn && app.state.ini.iMsg > 0) {
            app.state.iMsgCur = app.state.ini.iMsg - 1;
            if (app.state.iMsgCur >= app.state.cMsg + app.state.vcmsgplrIn) {
                app.state.iMsgCur = imsgNone;
            }
            app.state.iMsgCur = IMsgNext(app, 0);
            app.state.gd.fGotoVCR = 0;
            ControlIO().pfnSetMsgTitle(app, app.lpui->hwndMessage);
            ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
            if (app.state.gd.fTutorial) {
                app.state.tutor.fChange = 1;
                PresentationIO().pfnTutor(app);
            }
        }
    } else {
        app.state.ini.grobjSel = 1;
        app.state.ini.iObjSel = app.state.rgplr[app.state.idPlayer].idPlanetHome;
    }
    if (app.state.ini.grobjSel != 0) {
        if (app.state.ini.iObjSel != -1) {
            lppl = LpplFromId(app, app.state.ini.iObjSel);
            if (!lppl || lppl->iPlayer != app.state.idPlayer) {
                PresentationIO().pfnFindSelection(app);
            } else {
                PresentationIO().pfnSelectPlanet(app, 0, app.state.ini.iObjSel);
            }
        } else {
            PresentationIO().pfnFindSelection(app);
        }
        app.state.ini.grobjSel = 0;
    }
    return;
}

void CommandHandler(APPSESSION &app, HWND hwnd, WPARAM wParam) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    std::array<char, 50> szPlayerName{};
    std::string_view szWorkBase;
    std::size_t ichWorkExtension;
    std::size_t ichWorkSeparator;
    POINT16  pt;
    HMENU    hmenu;
    std::int16_t  dy;
    char     szExt[4];
    std::int16_t  dx;
    std::int16_t  fRet;
    RECT     rc;
    std::int16_t  iplrOld;
    char    *psz;
    char     szT[256];
    std::int16_t  cPageX;
    PLANET  *lpplMac;
    PLANET  *lppl;
    std::int16_t  i;
    PRINTDLG pd;
    int      iPrintResult;
    std::int16_t  cPageY;
    std::int16_t  xPage;
    std::int16_t  dxMax;
    std::int16_t  dxDPI;
    std::int16_t  dyPrintTiny;
    std::int16_t  dMargin;
    std::int16_t  y;
    std::int32_t  ldx;
    HFONT    hfontPrintTiny;
    HFONT    hfontPrint;
    POINT16  ptLegendB;
    POINT16  ptLegendA;
    std::int16_t  dyPrint;
    std::int16_t  dyMax;
    HFONT    hfontSav;
    std::int16_t  dSize;
    std::int16_t  yPage;
    std::int16_t  cch;
    std::int32_t  ldy;
    std::int16_t  dyDPI;
    std::int16_t  yOff;
    std::int16_t  xOff;
    std::int32_t  x;
    std::int16_t  idCur;
    HCURSOR  hcurSav;
    HCURSOR  hcurT;
    std::uint32_t dwTickCur;
    std::uint32_t dwTickBase;
    std::int16_t  mf;
    StringId ids;
    std::int16_t  cObj;
    std::int16_t  ifl;
    FLEET   *lpfl;
    std::int16_t  id;

    if (LOWORD(wParam) >= IDM_POPUP_BASE && LOWORD(wParam) < 15100) {
        app.lpui->iPopMenuSel = LOWORD(wParam) - 15000;
    } else {
        switch (LOWORD(wParam)) {
        case IDM_HELP_ABOUT:
            ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_ABOUT), hwnd, About, 0);
            break;
        case IDM_FILE_EXIT:
            ControlIO().pfnSendMessage(hwnd, win32::WM_CLOSE, 0, 0);
            break;
        case IDM_VIEW_LAYOUT_0:
        case IDM_VIEW_LAYOUT_1:
        case IDM_VIEW_LAYOUT_2:
            if (app.state.idPlayer == iplrNone)
                break;
            app.lpui->iWindowLayout = static_cast<WindowLayout>(LOWORD(wParam) - 130);
            ControlIO().pfnInvalidateRect(app.lpui->hwndFrame, nullptr, 1);
            EnsureTileSize(app, app.lpui->iWindowLayout == layoutSmall);
            ControlIO().pfnRefitFrameChildren(app);
            break;
        case IDM_HELP_TUTORIAL2:
        case IDM_HELP_TUTORIAL:
            if (app.state.gd.fTutorial) {
                ShowTutor(app, 1);
                break;
            }
            StartTutor(app, 0);
            break;
        case IDM_FILE_NEW_GAME:
        case IDM_TOOL_NEW_GAME:
            if (app.state.gd.fTutorial && !FAskKillTutor(app))
                break;
            DesktopChoiceIO().pfnNewGameWizard(app, hwnd, 0);
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
            if ((app.state.gd.fTutorial && !FAskKillTutor(app)) || !app.lpui->vrgszMRU || app.lpui->vrgszMRU[(LOWORD(wParam) - 4300) * 256] == 0)
                break;
            iplrOld = app.state.idPlayer;
            CchCopyUtf8(std::span{szT}, app.lpui->vrgszMRU + 256 * (LOWORD(wParam) - 4300));
            psz = std::strrchr(szT, 46);
            if (psz) {
                if (!FIsManagedLibraryGameFile(szT)) {
                    if (FImportDesktopFile(app, hwnd, szT) && app.lpui->hwndTitle) {
                        RefreshLibraryGames(app, app.lpui->hwndTitle);
                        RefreshLibraryRaces(app, app.lpui->hwndTitle);
                    }
                    break;
                }
                if (!FFileExists(szT)) goto LShowMissingRecent;
                app.state.ini.fStartupFile = 1;
                DestroyCurGame(app);
                CchCopyUtf8(std::span{app.state.szBase}, szT);
                if (FOpenGame(app, hwnd, 0) <= 0)
                    break;
                InitializeMenu(app, nullptr);
                CreateChildWindows(app);
                if (app.lpui->uTimerId == 0) {
                    win32::PostMessageA(hwnd, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                }
                if (!app.state.game.fTutorial || app.state.idPlayer != 0)
                    break;
                StartTutor(app, 0);
                break;
            }
        LShowMissingRecent:
            CchCopyUtf8(std::span{app.state.szWork}, szT);
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCantOpenFile, {}).data(), win32::MB_ICONHAND);
            break;
        case IDM_RACE_CREATE:
            app.state.vplr = vrgplrDef[0];
            RaceCreationWizard(app, hwnd, 0, 0);
            break;
        case IDM_VIEW_TOOLBAR:
            hmenu = GetASubMenu(app, hwnd, menuView);
            app.state.gd.fToolbar = app.state.gd.fToolbar == 0;
            win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !app.state.gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
            ControlIO().pfnRefitFrameChildren(app);
            break;
        case IDM_FILE_PRINT_MAP:
            if (app.state.idPlayer == iplrNone)
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_PRINT_MAP), app.lpui->hwndFrame, PrintMapDlg, 0);
            if (!fRet)
                break;
            cPageX = app.lpui->vrgcPrintMapPage[0];
            cPageY = app.lpui->vrgcPrintMapPage[1];
            pd = {};
            pd.lStructSize = sizeof(PRINTDLG);
            pd.Flags = win32::PD_RETURNDC | win32::PD_RETURNDEFAULT;
            if (win32::PrintDlgA(&pd) != 0) {
                hfontPrint = nullptr;
                hfontPrintTiny = nullptr;
                NATIVECLEANUPSCOPE print([&]() noexcept {
                    if (hfontPrint) win32::DeleteObject(hfontPrint);
                    if (hfontPrintTiny) win32::DeleteObject(hfontPrintTiny);
                    win32::DeleteDC(pd.hDC);
                    if (pd.hDevMode) win32::GlobalFree(pd.hDevMode);
                    if (pd.hDevNames) win32::GlobalFree(pd.hDevNames);
                });
                dxMax = win32::GetDeviceCaps(pd.hDC, win32::HORZRES);
                dyMax = win32::GetDeviceCaps(pd.hDC, win32::VERTRES);
                dxDPI = win32::GetDeviceCaps(pd.hDC, win32::LOGPIXELSX);
                dyDPI = win32::GetDeviceCaps(pd.hDC, win32::LOGPIXELSY);
                dMargin = 32;
                hfontPrint = HfontPrinterCreate(app, pd.hDC, 8, &dyPrint);
                hfontPrintTiny = HfontPrinterCreate(app, pd.hDC, 5, &dyPrintTiny);
                win32::SetBkMode(pd.hDC, win32::TRANSPARENT);
                rc.top = 0;
                rc.left = 0;
                if ((cPageX > cPageY) == (dxMax > dyMax)) {
                    i = cPageX;
                    cPageX = cPageY;
                    cPageY = i;
                }
                ldx = static_cast<std::uint32_t>((dxMax * cPageX));
                ldy = static_cast<std::uint32_t>((dyMax * cPageY));
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
                    if (ldx - static_cast<std::int16_t>((static_cast<std::int16_t>((3 * dxDPI)) / 2)) < ldy) {
                        ldy = ldx - static_cast<std::int16_t>((static_cast<std::int16_t>((3 * dxDPI)) / 2));
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
                cch = TextIO().pfnCchGetString(app, idsStarsUniverseMap, std::span{app.state.szWork});
                iPrintResult = win32::Escape(pd.hDC, win32::STARTDOC, cch, app.state.szWork, nullptr);
                if (iPrintResult <= 0) goto LPrintMapCleanup;
                for (xPage = 0; xPage < cPageX; xPage++) {
                    for (yPage = 0; yPage < cPageY; yPage++) {
                        xOff = -dxMax * xPage;
                        yOff = -dyMax * yPage;
                        win32::Rectangle(pd.hDC, xOff, yOff, xOff + rc.right, yOff + rc.bottom);
                        if (hfontPrint) {
                            hfontSav = static_cast<HFONT>(win32::SelectObject(pd.hDC, hfontPrint));
                        } else {
                            hfontSav = nullptr;
                        }
                        y = ptLegendA.y + yOff;
                        cch = TextIO().pfnCchGetString(app, idsStarsUniverseMap, std::span{app.state.szWork});
                        win32::TextOutA(pd.hDC, ptLegendA.x + xOff, y, app.state.szWork, cch);
                        y += dyPrint;
                        win32::TextOutA(pd.hDC, ptLegendA.x + xOff, y, app.state.game.szName, std::strlen(app.state.game.szName));
                        y += dyPrint;
                        szPlayerName = TextIO().pfnPlayerName(app, app.state.idPlayer, 1, 1, 1, 0, nullptr);
                        psz = szPlayerName.data();
                        win32::TextOutA(pd.hDC, ptLegendA.x + xOff, y, psz, std::strlen(psz));
                        y += dyPrint;
                        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsYearD).data(), app.state.game.turn + 2400);
                        win32::TextOutA(pd.hDC, ptLegendA.x + xOff, y, app.state.szWork, cch);
                        if ((app.lpui->grbitScan & grbitScanViewMask) != 5) {
                            y = ptLegendB.y + yOff;
                            for (i = 0; i < 5; i++) {
                                cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsPlanet3 + i), std::span{app.state.szWork});
                                win32::TextOutA(pd.hDC, ptLegendB.x + xOff, y, app.state.szWork, cch);
                                y += dyPrint;
                            }
                            y = ptLegendB.y + yOff;
                            if (hfontPrintTiny) {
                                win32::SelectObject(pd.hDC, hfontPrintTiny);
                            }
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, static_cast<std::int16_t>((3 * dyPrint)) / 2 + y - dyPrintTiny / 2, "|", 1);
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, static_cast<std::int16_t>((5 * dyPrint)) / 2 + y - dyPrintTiny / 2, "+", 1);
                            CtrTextOut(pd.hDC, ptLegendB.x + xOff, static_cast<std::int16_t>((9 * dyPrint)) / 2 + y + 8 - dyPrintTiny, "2", 1);
                            if (hfontPrint) {
                                win32::SelectObject(pd.hDC, hfontPrint);
                            }
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, y - 4 + dyPrint / 2, 1);
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, y - 4 + static_cast<std::int16_t>((7 * dyPrint)) / 2, 0);
                            DrawPlanetPrintDot(pd.hDC, ptLegendB.x + xOff, static_cast<std::int16_t>((9 * dyPrint)) / 2 + y + 8, 0);
                        }
                        if ((app.lpui->grbitScan & grbitScanViewMask) != 5) {
                            if (hfontPrintTiny) {
                                win32::SelectObject(pd.hDC, hfontPrintTiny);
                            }
                            lppl = app.state.lpPlanets;
                            lpplMac = app.state.lpPlanets + app.state.cPlanet;
                            for (; lppl < lpplMac; lppl++) {
                                x = static_cast<std::int16_t>((app.state.rgptPlan[lppl->id].x - 1000));
                                y = static_cast<std::int16_t>((app.state.dGalInv - 1000 - app.state.rgptPlan[lppl->id].y));
                                x = static_cast<std::int32_t>((static_cast<std::int32_t>((x * dSize)) / app.state.dGal)) + dMargin + xOff;
                                y = static_cast<std::int32_t>((static_cast<std::int32_t>((y * dSize)) / app.state.dGal)) + dMargin + yOff;
                                if (lppl->iPlayer == app.state.idPlayer) {
                                    if (lppl->fStarbase) {
                                        CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) + 4 - dyPrintTiny,
                                                   const_cast<char *>(reinterpret_cast<const char*>((app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef == ihuldefOrbitalFort ? "|" : "+"))), 1);
                                    }
                                    DrawPlanetPrintDot(pd.hDC, LOWORD(x), LOWORD(y), 1);
                                } else if (lppl->iPlayer != iplrNone) {
                                    cch = win32::wsprintfA(app.state.szWork, app.state.PCTD, lppl->iPlayer + 1);
                                    CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) - dyPrintTiny, app.state.szWork, cch);
                                }
                            }
                        }
                        if (hfontPrint) {
                            win32::SelectObject(pd.hDC, hfontPrint);
                        }
                        for (i = 0; i < app.state.game.cPlanMax; i++) {
                            x = static_cast<std::int16_t>((app.state.rgptPlan[i].x - 1000));
                            y = static_cast<std::int16_t>((app.state.dGalInv - 1000 - app.state.rgptPlan[i].y));
                            x = static_cast<std::int32_t>((static_cast<std::int32_t>((x * dSize)) / app.state.dGal)) + dMargin + xOff;
                            y = static_cast<std::int32_t>((static_cast<std::int32_t>((y * dSize)) / app.state.dGal)) + dMargin + yOff;
                            DrawPlanetPrintDot(pd.hDC, LOWORD(x), LOWORD(y), 0);
                            if (app.lpui->grbitScan & grbitScanPlanetNames) {
                                CtrTextOut(pd.hDC, LOWORD(x), LOWORD(y) + 14, TextIO().pfnPlanetName(app, i).data(), 0);
                            }
                        }
                        if (hfontSav) {
                            win32::SelectObject(pd.hDC, hfontSav);
                        }
                        iPrintResult = win32::Escape(pd.hDC, win32::NEWFRAME, 0, nullptr, nullptr);
                        if (iPrintResult <= 0) {
                            win32::Escape(pd.hDC, win32::ABORTDOC, 0, nullptr, nullptr);
                            goto LPrintMapCleanup;
                        }
                    }
                }
                iPrintResult = win32::Escape(pd.hDC, win32::ENDDOC, 0, nullptr, nullptr);
LPrintMapCleanup:
                print.Close();
                if (iPrintResult <= 0 && iPrintResult != win32::SP_APPABORT)
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnablePrintGameMapPrinterMayOff, {}).data(), win32::MB_ICONHAND);
            } else {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnablePrintGameMapPrinterMayOff, {}).data(), win32::MB_ICONHAND);
            }
            break;
        case IDM_VIEW_PLAYER_COLORS:
            hmenu = GetASubMenu(app, hwnd, menuView);
            app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan ^ grbitScanPlayerColors);
            win32::CheckMenuItem(hmenu, IDM_VIEW_PLAYER_COLORS, !(app.lpui->grbitScan & grbitScanPlayerColors) ? win32::MF_UNCHECKED : win32::MF_CHECKED);
            app.state.gd.fChgScanner = 1;
            if (!(app.lpui->grbitScan & (grbitScanPlanetNames | grbitScanShipCounts)))
                break;
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 0);
            break;
        case IDM_VIEW_FIND:
        case IDM_VIEW_FIND2:
            if (app.state.idPlayer == iplrNone)
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_FIND), hwnd, FindDlg, 0);
            break;
        case IDM_GAME_SCORE:
        case IDM_GAME_SCORE2:
            if (app.state.idPlayer == iplrNone)
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_SCORE), hwnd, ScoreXDlg, 0);
            break;
        case IDM_FILE_HOST_GAME:
            if (!app.state.game.fSinglePlr &&
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsGameAlreadyHostedAnotherInstanceStarsWould, {}).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES)
                break;
            /* fallthrough */
        case IDM_TURN_GENERATE:
        case IDM_DEBUG_GEN_10_TURNS:
        case IDM_DEBUG_GEN_100_TURNS:
        case IDM_DEBUG_GEN_1000_TURNS:
            idCur = app.state.idPlayer;
            switch (LOWORD(wParam)) {
            case IDM_DEBUG_GEN_100_TURNS:
                app.lpui->iPassCnt = 100;
                break;
            case IDM_DEBUG_GEN_1000_TURNS:
                app.lpui->iPassCnt = 1000;
                break;
            case IDM_DEBUG_GEN_10_TURNS:
                app.lpui->iPassCnt = 10;
                break;
            default:
                app.lpui->iPassCnt = 0;
            }
            if (app.state.game.fSinglePlr && app.lpui->iPassCnt != 0) {
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSureWantForceGenerateDTurnsRow).data(), app.lpui->iPassCnt);
                if (CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), app.state.szWork, "Stars!", win32::MB_YESNO | win32::MB_ICONEXCLAMATION | win32::MB_TASKMODAL) != win32::IDYES)
                    break;
            }
            if (app.state.idPlayer == iplrNone)
                break;
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
                if (!app.state.tutor.fTurnDone) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsTutorialTurnWillGeneratedHaveYetCompleted, {}).data(), win32::MB_ICONHAND);
                    break;
                }
                ShowTutor(app, 0);
                if (app.state.game.turn <= 35) {
                    Randomize(app, 1234567890);
                    FWriteHistFile(app, idCur);
                    if (!FWriteTutorialMFile(app, app.state.game.turn + 1)) {
                        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, static_cast<StringId>(app.state.idsFileError), {}).data(), win32::MB_ICONHAND);
                        break;
                    }
                    if (app.state.game.turn == 35 && !FWriteTutorialMFile(app, app.state.game.turn + 2)) {
                        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, static_cast<StringId>(app.state.idsFileError), {}).data(), win32::MB_ICONHAND);
                        break;
                    }
                    CchCopyUtf8(std::span{app.state.szWork}, app.state.szBase);
                    FAppendUtf8(std::span{app.state.szWork}, ".x1");
                    RemoveNativeFile(app.state.szWork);
                    RecordIO().pfnDirtyGame(app, 0);
                    ShowProgressGauge(app);
                    dwTickBase = win32::GetTickCount();
                    do {
                        UpdateProgressGauge(app, static_cast<ProgressStep>((LOWORD(dwTickCur) - LOWORD(dwTickBase)) * 2));
                        dwTickCur = win32::GetTickCount();
                    } while (dwTickCur >= dwTickBase && dwTickCur < dwTickBase + 500);
                    goto LTutorialFinishUp;
                }
            }
            if (!app.state.game.fSinglePlr)
                goto LWaitForTurn;
            app.state.idsFileError = -1;
            if (FCheckFile(app, dtHost, iplrNone, mdMarkInUse)) {
                if (FBadFileError(static_cast<StringId>(app.state.idsFileError))) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, static_cast<StringId>(app.state.idsFileError), {}).data(), win32::MB_ICONHAND);
                    break;
                }
                if (AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsGameAlreadyHostedAnotherInstanceStarsWould, {}).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) == win32::IDYES)
                    goto LWaitForTurn;
                break;
            }
            hcurSav = win32::SetCursor(win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32514)));
            FWriteLogFile(app, app.state.szBase, idCur);
            FWriteHistFile(app, idCur);
        RepGen:
            ShowProgressGauge(app);
            EnsureAis(app);
            hcurT = win32::SetCursor(win32::LoadCursorA(nullptr, win32::IDC_WAIT));
            FGenerateTurn(app);
            win32::SetCursor(hcurT);
            switch (LOWORD(wParam)) {
            case IDM_DEBUG_GEN_10_TURNS:
            case IDM_DEBUG_GEN_100_TURNS:
            case IDM_DEBUG_GEN_1000_TURNS:
                app.lpui->iPassCnt--;
                if (app.lpui->iPassCnt > 0) {
                    HideProgressGauge(app);
                    if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) >= 0 || ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) >= 0)
                        goto RepGen;
                }
            }
            app.lpui->iPassCnt = 0;
        LTutorialFinishUp:
            DestroyCurGame(app);
            win32::wsprintfA(szExt, app.state.MPCTD, idCur + 1);
            if (!FLoadGame(app, app.state.szBase, szExt)) {
                win32::SetCursor(hcurSav);
                HideProgressGauge(app);
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, {}).data(), win32::MB_ICONHAND);
                break;
            }
            HideProgressGauge(app);
            app.state.idPlayer = idCur;
            CreateChildWindows(app);
            ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            win32::SetCursor(hcurSav);
            if (!app.state.gd.fTutorial)
                break;
            app.state.tutor.fTurnDone = 0;
            app.state.tutor.fAutoComplete = 0;
            PresentationIO().pfnTutor(app);
            break;
        case IDM_VIEW_BROWSER_TOGGLE:
        case IDM_VIEW_BROWSER_TOGGLE2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            if (!app.lpui->hwndBrowser) {
                mf = 8;
                ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_BROWSER), app.lpui->hwndFrame, app.lpui->lpfnBrowserDlgProc);
            } else {
                mf = 0;
                ControlIO().pfnDestroyWindow(app.lpui->hwndBrowser);
            }
            hmenu = GetASubMenu(app, hwnd, menuHelp);
            win32::CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, mf);
            break;
        case IDM_REPORT_CYCLE:
            if (!app.lpui->hwndReportDlg) {
                wParam = IDM_REPORT_PLANET;
            } else if (app.lpui->vprptCur == &app.state.vrptPlanet) {
                wParam = IDM_REPORT_FLEET;
            } else if (app.lpui->vprptCur == &app.state.vrptFleet) {
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
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            hmenu = GetASubMenu(app, hwnd, menuReport);
        LRetryReport:
            if (!app.lpui->hwndReportDlg) {
                mf = 8;
                switch (LOWORD(wParam)) {
                case IDM_REPORT_FLEET:
                    ids = idsFleetSummaryReportDFleetC;
                    app.lpui->vprptCur = &app.state.vrptFleet;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->iPlayer == app.state.idPlayer) {
                            cObj++;
                        }
                    }
                    break;
                case IDM_REPORT_ENEMY_FLEET:
                    ids = idsOthersFleetsSummaryReportDFleetC;
                    app.lpui->vprptCur = &app.state.vrptEFleet;
                    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                        lpfl = app.state.rglpfl[ifl];
                        if (!app.state.rglpfl[ifl])
                            break;
                        if (lpfl->iPlayer != app.state.idPlayer) {
                            cObj++;
                        }
                    }
                    break;
                case IDM_REPORT_BATTLE:
                    app.lpui->vprptCur = &app.state.vrptBattle;
                    ids = idsBattleSummaryReportDBattleC;
                    cObj = CBattles(app);
                    break;
                default:
                    app.lpui->vprptCur = &app.state.vrptPlanet;
                    ids = idsPlanetSummaryReportDPlanetC;
                    lppl = app.state.lpPlanets;
                    lpplMac = app.state.lpPlanets + app.state.cPlanet;
                    for (; lppl < lpplMac; lppl++) {
                        if (lppl->iPlayer == app.state.idPlayer) {
                            cObj++;
                        }
                    }
                }
                szCatalogPsz = TextIO().pfnCompressed(app, ids);
                psz = szCatalogPsz.data();
                win32::wsprintfA(app.state.szWork, psz, cObj, cObj == 1 ? 32 : 115);
                app.lpui->hwndReportDlg = ui::CreateWindowEx(0, app.lpui->szReport, app.state.szWork, win32::WS_POPUP | win32::WS_CAPTION | win32::WS_SYSMENU | win32::WS_THICKFRAME | win32::WS_MINIMIZEBOX | win32::WS_MAXIMIZEBOX, 0, 0, 100,
                                             100, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
                ControlIO().pfnSetWindowPos(app.lpui->hwndReportDlg, nullptr, 0, 0, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
                win32::CheckMenuItem(hmenu, LOWORD(wParam), mf);
                break;
            } else {
                if (LOWORD(wParam) != IDM_REPORT_BATTLE || app.lpui->vprptCur != &app.state.vrptBattle) {
                    ids = idsPlayerLogFileAppearsCorruptUnableLoad;
                } else {
                    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
                }
                mf = 0;
                ControlIO().pfnDestroyWindow(app.lpui->hwndReportDlg);
                win32::CheckMenuItem(hmenu, LOWORD(wParam) == IDM_REPORT_FLEET ? IDM_REPORT_FLEET : IDM_REPORT_PLANET, mf);
                if (ids != idsPlayerLogFileAppearsCorruptUnableLoad) {
                    return;
                }
                goto LRetryReport;
            }
        case IDM_HELP_CONTEXT_13002:
        case IDM_HELP_INTRO:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<std::uint32_t>((LOWORD(wParam) == IDM_HELP_INTRO ? 4501 : 13002)));
            break;
        case IDM_HELP_CONTENTS:
        case IDM_HELP_CONTENTS2:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_INDEX, 0);
            break;
        case IDM_TURN_WAIT_NEW:
        case IDM_GAME_WAIT_FOR_TURN:
        LWaitForTurn:
            if (!app.state.lpPlanets || app.state.idPlayer == iplrNone || app.state.game.fSinglePlr)
                break;
            if (FNewTurnAvail(app, app.state.idPlayer)) {
            LNewTurnAvail:
                FWriteHistFile(app, app.state.idPlayer);
                if (app.state.game.fDirty) {
                    id = AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsSorryTurnHasAlreadyGeneratedAnyChanges, {}).data(), win32::MB_OKCANCEL | win32::MB_ICONEXCLAMATION);
                    if (id == 2)
                        break;
                } else {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsNewTurnAvailable, {}).data(), win32::MB_ICONASTERISK);
                }
                win32::wsprintfA(szExt, app.state.MPCTD, app.state.idPlayer + 1);
                app.state.game.fDirty = 0;
                DestroyCurGame(app);
                if (!FLoadGame(app, app.state.szBase, szExt)) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, {}).data(), win32::MB_ICONHAND);
                } else {
                    CreateChildWindows(app);
                    ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                }
            } else {
                app.state.gd.fSubmit = 1;
                FWriteLogFile(app, app.state.szBase, app.state.idPlayer);
                FWriteHistFile(app, app.state.idPlayer);
                ControlIO().pfnSetWindowText(app.lpui->hwndFrame, TextIO().pfnCompressed(app, idsWaitingNewTurn).data());
                ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOWMINIMIZED);
                app.lpui->uTimerId = win32::SetTimer(nullptr, hostTimerWaitTurn, 10000, app.lpui->lpfnHostTimerProc);
                app.lpui->uTimerType = hostTimerWaitTurn;
                HostTimerProc(nullptr, win32::WM_NULL, app.lpui->uTimerId, 0);
            }
            break;
        case IDM_TURN_SAVE_SUBMIT:
            wParam = IDM_TURN_END_B;
            /* fallthrough */
        case IDM_FILE_SAVE:
        case IDM_TURN_END_A:
        case IDM_TURN_END_B:
            if (!app.lpui->hwndScanner) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsGameCurrentlyLoaded, {}).data(), win32::MB_ICONHAND);
                break;
            }
            if (app.state.idPlayer == iplrNone) {
                FWriteDataFile(app, app.state.szBase, app.state.idPlayer, 0);
                break;
            }
            if (FNewTurnAvail(app, app.state.idPlayer))
                goto LNewTurnAvail;
            app.state.gd.fSubmit = LOWORD(wParam) == IDM_TURN_END_B;
            FWriteLogFile(app, app.state.szBase, app.state.idPlayer);
            FWriteHistFile(app, app.state.idPlayer);
            break;
        case IDM_LIBRARY_EXPORT_ORDERS:
            FExportLibraryOrders(app, hwnd, nullptr);
            break;
        case IDM_FILE_OPEN_GAME:
        case IDM_TOOL_OPEN_GAME:
            if (app.state.gd.fTutorial && !FAskKillTutor(app)) break;
            if (app.state.ini.fStartupFile && FIsManagedLibraryGameFile(app.state.szBase)) {
                if (FOpenGame(app, hwnd, 0) <= 0) break;
                InitializeMenu(app, nullptr);
                if (app.lpui->uTimerId == 0) win32::PostMessageA(hwnd, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                break;
            }
            fRet = FImportDesktopFile(app, hwnd, app.state.ini.fStartupFile ? app.state.szBase : nullptr);
            app.state.ini.fStartupFile = 0;
            if (fRet && app.lpui->hwndTitle) {
                RefreshLibraryGames(app, app.lpui->hwndTitle);
                RefreshLibraryRaces(app, app.lpui->hwndTitle);
            }
            break;
        case IDM_TITLE_NEW_GAME:
        case IDM_TITLE_OPEN_GAME:
        case IDM_TITLE_CONTINUE:
        case IDM_TITLE_EXIT:
            if (!app.lpui->hwndTitle)
                break;
            win32::PostMessageA(app.lpui->hwndTitle, win32::WM_COMMAND, LOWORD(wParam) - 250, 0);
            break;
        case IDM_FILE_RETURN_TO_TITLE:
            if (app.state.gd.fTutorial && !FAskKillTutor(app))
                break;
            if (!FCanLeaveCurrentGame(app))
                break;
            WriteIniSettings(app);
            DestroyCurGame(app);
            szWorkBase = app.state.szWork;
            ichWorkExtension = szWorkBase.rfind('.');
            ichWorkSeparator = szWorkBase.find_last_of("/\\");
            if (ichWorkExtension != std::string_view::npos &&
                (ichWorkSeparator == std::string_view::npos || ichWorkExtension > ichWorkSeparator)) {
                szWorkBase = szWorkBase.substr(0, ichWorkExtension);
            }
            CchCopyUtf8(std::span{app.state.szBase}, szWorkBase);
            app.state.ini.grobjSel = 0;
            app.state.ini.iObjSel = 0;
            app.state.ini.idPlayer = iplrNone;
            InitializeMenu(app, nullptr);
            ShowTitleScreen(app);
            ui::ShowWindow(app.lpui->hwndFrame, win32::SW_HIDE);
            break;
        case IDM_GAME_RESEARCH:
        case IDM_GAME_RESEARCH2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RESEARCH), app.lpui->hwndFrame, ResearchDlg, 0);
            if (!fRet || app.state.sel.grobj != grobjPlanet)
                break;
            if (app.state.sel.pl.lpplprod) {
                ControlIO().pfnFillPlanetProdLB(app, app.lpui->hwndPlanetProdLB, app.state.sel.pl.lpplprod, nullptr);
            }
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tilePlanetStats | tileProductionOrOrbit));
            break;
        case IDM_GAME_BATTLE_PLANS1:
        case IDM_GAME_BATTLE_PLANS2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_BATTLE_PLANS), app.lpui->hwndFrame, BattlePlansDlg, 0);
            break;
        case IDM_GAME_RELATIONS:
        case IDM_GAME_RELATIONS2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone || app.state.game.fSinglePlr)
                break;
            ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RELATIONS), app.lpui->hwndFrame, RelationsDlg, 0);
            break;
        case IDM_RACE_EDIT1:
        case IDM_RACE_EDIT2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            app.state.vplr = app.state.rgplr[app.state.idPlayer];
            RaceCreationWizard(app, hwnd, 1, 0);
            break;
        case IDM_VIEW_GAME_PARAMS:
        case IDM_VIEW_GAME_PARAMS2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            DesktopChoiceIO().pfnNewGameWizard(app, hwnd, 1);
            break;
        case IDM_GAME_SHIP_BUILDER:
        case IDM_GAME_SHIP_BUILDER2:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone)
                break;
            pt.x = 610;
            pt.y = 450;
            if (app.lpui->hwndPopup) {
                ControlIO().pfnSendMessage(app.lpui->hwndPopup, win32::WM_RBUTTONUP, 0, 0);
            }
            ShipBuilder(app, pt);
            break;
        case IDM_DEBUG_DUMP_UNIVERSE:
            DumpUniverse(app);
            break;
        case IDM_DEBUG_DUMP_PLANETS:
            DumpPlanets(app);
            break;
        case IDM_DEBUG_DUMP_FLEETS:
            DumpFleets(app);
            break;
        case IDM_CMD_CHANGE_PASSWORD:
            if (app.state.game.lid == 0 || app.state.idPlayer == iplrNone || ((app.state.game.fSinglePlr && app.state.lSaltCur <= 0) || !FCheckPassword(app)))
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), app.lpui->hwndFrame, NewPasswordDlg, 0);
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
            if (!app.lpui->hwndScanner) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCantChangeZoomFactorUntilGameOpen, {}).data(), win32::MB_ICONHAND);
                break;
            }
            hmenu = GetASubMenu(app, hwnd, menuView);
            hmenu = win32::GetSubMenu(hmenu, 3);
            win32::CheckMenuItem(hmenu, app.state.iScanZoom + 4, win32::MF_BYPOSITION);
            ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
            rc.right = ScanToPt(app, rc.right) >> 1;
            rc.bottom = ScanToPt(app, rc.bottom) >> 1;
            dx = app.lpui->xScanTop;
            dy = app.state.dGalInv - app.lpui->yScanTop;
            app.state.iScanZoom = static_cast<ScanZoom>(LOWORD(wParam) - 3905);
            win32::CheckMenuItem(hmenu, app.state.iScanZoom + 4, win32::MF_CHECKED | win32::MF_BYPOSITION);
            win32::DrawMenuBar(hwnd);
            SetScanScrollBars(app, app.lpui->hwndScanner);
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            if (app.state.sel.scan.grobj != grobjNone) {
                pt = app.state.sel.scan.pt;
            } else {
                pt.x = dx + rc.right;
                pt.y = dy - rc.bottom;
            }
            CtrPointScan(app, pt, 0);
            break;
        case IDM_FRAME_POST_OPEN:
            RestoreGameWindows(app);
            break;
        case IDM_FLEET_DELETE_WAYPOINT:
        case IDM_FLEET_INSERT_WAYPOINT:
            if (app.state.sel.grobj == grobjFleet && win32::GetFocus() != app.lpui->hwndOrderED) {
                DeleteCurWayPoint(app, LOWORD(wParam) == IDM_FLEET_DELETE_WAYPOINT);
            }
            goto Default;
        default:
        Default:
            ui::DefWindowProc(hwnd, win32::WM_COMMAND, wParam, 0);
        }
    }
}

void InitializeMenu(APPSESSION &app, HMENU hmenu) {
    std::int16_t cMenu;
    std::int16_t i;
    HMENU   hmenuSub;

    if (!hmenu) {
        hmenu = win32::GetMenu(app.lpui->hwndFrame);
    }
    hmenuSub = GetASubMenu(app, app.lpui->hwndFrame, menuFile);
    for (i = 4300; i <= 4308; i++) {
        win32::DeleteMenu(hmenuSub, i, win32::MF_BYCOMMAND);
    }
    for (i = 0; i < 9 && app.lpui->vrgszMRU[i * 256] != 0; i++) {
        app.state.szWork[0] = '&';
        app.state.szWork[1] = i + '1';
        app.state.szWork[2] = ' ';
        CchCopyUtf8(std::span{app.state.szWork}.subspan(3), app.lpui->vrgszMRU + 256 * i);
        win32::InsertMenuA(hmenuSub, i + 9, win32::MF_BYPOSITION, i + 4300, app.state.szWork);
    }
    win32::EnableMenuItem(hmenu, 106, app.state.szBase[0] != 0 && !app.state.game.fSinglePlr ? win32::MF_ENABLED : win32::MF_GRAYED | win32::MF_DISABLED);
    win32::EnableMenuItem(hmenu, 105, app.state.szBase[0] == 0 ? win32::MF_GRAYED | win32::MF_DISABLED : win32::MF_ENABLED);
    win32::EnableMenuItem(hmenu, 270, app.state.szBase[0] != 0 && (!app.state.game.fSinglePlr || app.state.lSaltCur > 0) ? win32::MF_ENABLED : win32::MF_GRAYED | win32::MF_DISABLED);
    win32::EnableMenuItem(hmenu, 2014, app.state.szBase[0] != 0 && !app.state.game.fSinglePlr ? win32::MF_ENABLED : win32::MF_GRAYED | win32::MF_DISABLED);
    win32::EnableMenuItem(hmenu, 3803, app.state.szBase[0] != 0 && !app.state.game.fSinglePlr ? win32::MF_ENABLED : win32::MF_GRAYED | win32::MF_DISABLED);
    hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
    win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !app.state.gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
    win32::CheckMenuItem(hmenu, IDM_VIEW_PLAYER_COLORS, !(app.lpui->grbitScan & grbitScanPlayerColors) ? win32::MF_UNCHECKED : win32::MF_CHECKED);
    if (!app.lpui->hwndScanner) {
        win32::EnableMenuItem(win32::GetMenu(app.lpui->hwndFrame), 1, win32::MF_GRAYED | win32::MF_DISABLED | win32::MF_BYPOSITION);
    } else {
        win32::EnableMenuItem(win32::GetMenu(app.lpui->hwndFrame), 1, win32::MF_BYPOSITION);
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 3);
        win32::CheckMenuItem(hmenu, app.state.iScanZoom + 4, win32::MF_CHECKED | win32::MF_BYPOSITION);
        cMenu = win32::GetMenuItemCount(hmenu);
        for (i = 0; i < cMenu; i++) {
            win32::EnableMenuItem(hmenu, i, win32::MF_BYPOSITION);
        }
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 4);
        win32::CheckMenuItem(hmenu, app.lpui->iWindowLayout, win32::MF_CHECKED | win32::MF_BYPOSITION);
    }
    win32::DrawMenuBar(app.lpui->hwndFrame);
    return;
}

HMENU GetASubMenu(APPSESSION &app, HWND hwnd, MainMenu iMenu) {
    std::int16_t fChildMenu;
    HMENU   hmenu;

    fChildMenu = app.lpui->hwndActive && win32::IsZoomed(app.lpui->hwndActive) != 0;
    hmenu = win32::GetMenu(hwnd);
    hmenu = win32::GetSubMenu(hmenu, iMenu + fChildMenu);
    return hmenu;
}

std::int16_t FOpenGame(APPSESSION &app, HWND hwnd, std::int16_t fRaceOnly) {
    win32::files::FILESELECTIONREQUEST request;
    win32::files::FILESELECTIONRESULT result;
    const win32::files::FILESELECTIONSELECTED *pSelected;
    std::array<char, cchCatalogText> rgchFilterText;
    char         szFile[256];
    char *pch, *pchBackslash;
    std::int16_t      fRet;
    std::size_t ichFileName, ichFileExtension, cchDirectory;

    if (app.state.ini.fStartupFile) {
        CchCopyUtf8(std::span{szFile}, app.state.szBase);
        pch = std::strrchr(app.state.szBase, chDirSep);
        if (pch)
            *pch = 0;
        pch = std::strrchr(szFile, 46);
        if (!pch) {
            SetSzWorkFromDt(app, dtHost, iplrNone);
            if (CchCopyUtf8(std::span{szFile}, app.state.szWork) < 0)
                return 0;
            pch = std::strrchr(szFile, 46);
        }
        ichFileExtension = static_cast<std::size_t>(pch - szFile + 1);
        ichFileName = 0;
        app.state.fFileErrSilent = 1;
        goto LGotFileName;
    }
    szFile[0] = 0;
    request.mode = win32::files::FILESELECTIONMODE::open;
    request.szInitialDirectory = app.lpui->szDirName;
    request.cbPathMax = sizeof(szFile);
    request.fMustExist = true;
    rgchFilterText = TextIO().pfnCompressed(app, !fRaceOnly ? idsStarsGameFilesMHstRStars : idsStarsGameFilesRFiles);
    request.rgFilters = ui::files::RgParseFilters(std::string_view{rgchFilterText.data()});
    result = ui::files::ChooseFile(hwnd, std::move(request));
    pSelected = std::get_if<win32::files::FILESELECTIONSELECTED>(&result);
    if (!pSelected || CchCopyUtf8(std::span{szFile}, pSelected->szPath) < 0) return 0;
    pch = std::strrchr(szFile, '/');
    pchBackslash = std::strrchr(szFile, '\\');
    if (pchBackslash && (!pch || pchBackslash > pch)) pch = pchBackslash;
    ichFileName = pch ? static_cast<std::size_t>(pch - szFile + 1) : 0;
    pch = std::strrchr(szFile + ichFileName, '.');
    ichFileExtension = pch ? static_cast<std::size_t>(pch - szFile + 1) : 0;
LGotFileName:
    app.lpui->szDirName[0] = 0;
    fRet = FWasRaceFile(app, &szFile[ichFileName], fRaceOnly == 0);
    if (fRaceOnly) {
        if (fRet > 0) {
            if (!FIsManagedLibraryRaceFile(szFile)) return FImportLibraryRace(app, hwnd, szFile);
            pch = szFile + std::strlen(szFile);
            while (pch > szFile && pch[-1] != '/' && pch[-1] != '\\') pch--;
            return FLoadLibraryRace(app, hwnd, pch);
        }
        return fRet;
    }
    if (fRet) {
        app.state.fFileErrSilent = 0;
        app.state.ini.fStartupFile = 0;
        if (fRet == -1) {
            return -1;
        }
        SetSzRaceFile(app, &szFile[ichFileName]);
        RaceCreationWizard(app, hwnd, 0, 0);
        return 0;
    }
    if (ichFileExtension == 0) return 0;
    szFile[ichFileExtension - 1] = 0;
    DestroyCurGame(app);
    CchCopyUtf8(std::span{app.state.szBase}, szFile);
    if (!FLoadGame(app, szFile, &szFile[ichFileExtension])) {
        if (app.state.ini.fStartupFile) {
            app.state.ini.SetWFlags(0);
            app.state.fFileErrSilent = 0;
        }
        return 0;
    }
    if (app.state.ini.fStartupFile) {
        app.state.fFileErrSilent = 0;
        app.state.ini.fStartupFile = 0;
        pch = std::strrchr(szFile, chDirSep);
        if (pch) {
            cchDirectory = static_cast<std::size_t>(pch - szFile);
            CchCopyUtf8(std::span{app.lpui->szDirName}, std::string_view{szFile, cchDirectory});
        }
        if (app.state.idPlayer == iplrNone && !app.state.ini.fGen) {
            app.state.gd.fClose = 1;
        }
        if (app.state.game.lid != app.state.ini.lid || app.state.game.turn > app.state.ini.turn) {
            app.state.ini.fTry = 0;
        }
    }
    app.state.sel.grobjFull = grobjNone;
    app.state.sel.grobj = grobjNone;
    app.state.sel.iwpAct = iwpNone;
    app.state.sel.id = -1;
    app.state.sel.scan.grobjFull = grobjNone;
    app.state.sel.scan.grobj = grobjNone;
    app.state.sel.scan.iwp = iwpNone;
    app.state.sel.scan.ifl = iflNone;
    app.state.sel.scan.idpl = idplNone;
    app.lpui->fOrdersVis = 0;
    CreateChildWindows(app);
    return FinishOpenGame(app);
}

void BringUpHostDlg(APPSESSION &app) {
    std::int16_t fRet;
    HCURSOR hcurSav;

    if (!app.state.gd.fHostMode) {
        if (!app.state.gd.fReadOnly) {
            FMarkFile(app, dtHost, iplrNone, mdMarkInUse, 1);
        }
        app.state.gd.fHostMode = 1;
    }
    ui::ShowWindow(app.lpui->hwndFrame, win32::SW_HIDE);
    if (app.state.ini.fWait) {
        app.state.ini.fWait = 0;
        goto LAutoMode;
    }
Top:
    fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_HOST_MODE), app.lpui->hwndFrame, HostModeDialog, 0);
    if (fRet == -1) {
    LAutoMode:
        ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOWMINIMIZED);
        app.lpui->uTimerId = win32::SetTimer(nullptr, hostTimerAutoGen, 10000, app.lpui->lpfnHostTimerProc);
        app.lpui->uTimerType = hostTimerAutoGen;
        HostTimerProc(nullptr, win32::WM_NULL, app.lpui->uTimerId, 0);
        return;
    }
    if (fRet) {
    LNextGen:
        if (app.state.gd.fProgressTxt) {
            ShowProgressGauge(app);
        }
        EnsureAis(app);
        hcurSav = win32::SetCursor(win32::LoadCursorA(nullptr, win32::IDC_WAIT));
        FGenerateTurn(app);
        win32::SetCursor(hcurSav);
        if (app.lpui->iPassCnt != 0) {
            app.lpui->iPassCnt--;
            if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) >= 0 && ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) >= 0)
                goto LNextGen;
        }
        app.lpui->iPassCnt = 0;
        HideProgressGauge(app);
        goto Top;
    }
    if (!app.state.gd.fReadOnly) {
        FMarkFile(app, dtHost, iplrNone, mdMarkInUse, 0);
    }
    app.state.gd.fHostMode = 0;
    DestroyCurGame(app);
    if (app.state.ini.fGen) {
        return;
    }
    ShowTitleScreen(app);
    return;
}

void DrawHostDialog2(APPSESSION &app, HWND hwnd, HDC hdcIn) {
    std::uint32_t dsec;
    HDC      hdc;
    std::uint16_t dhour;
    std::int16_t  bkMode;
    std::int16_t  yCur;
    std::int16_t  i;
    std::uint16_t dmin;
    std::int16_t  dday;
    std::int16_t  cch;
    RECT     rcDiamond;
    COLORREF crBackSav;
    std::int16_t  x;
    char     szStat[30];

    if (hdcIn) {
        hdc = hdcIn;
    } else {
        hdc = win32::GetDC(hwnd);
    }
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    crBackSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    x = app.state.dyArial8 + 10 + LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN16).data(), 4));
    yCur = 48;
    win32::SetRect(&rcDiamond, 6, yCur, app.state.dyArial8 + 7, yCur + app.state.dyArial8 + 1);
    for (i = 0; i < app.state.game.cPlayer; i++) {
        DrawDiamond(app, hdc, &rcDiamond, app.lpui->hbrBBlue);
        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsD2).data(), i + 1);
        RightTextOut(app, hdc, x, yCur, app.state.szWork, cch, 0);
        win32::SetTextColor(hdc, app.state.rgOut[i] <= 0 ? 32512 : 127);
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsTurned + app.state.rgOut[i]), std::span{szStat});
        if (app.state.gd.fNoHostNames) {
            cch = win32::wsprintfA(app.state.szWork, " %s", szStat);
        } else {
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSS).data(), TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr), szStat);
        }
        if (app.state.rgplr[i].fHacker) {
            CchAppendUtf8Prefix(std::span{app.state.szWork}, " - HACKER");
            cch += 9;
        }
        win32::TextOutA(hdc, x + 4, yCur, app.state.szWork, cch);
        win32::SetTextColor(hdc, app.lpui->crWindowText);
        win32::OffsetRect(&rcDiamond, 0, app.state.dyArial8 + 4);
        yCur += app.state.dyArial8 + 4;
    }
    cch = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.game.turn + 2401);
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_NEXT_YEAR_TEXT), app.state.szWork);
    dsec = static_cast<std::uint32_t>(((win32::GetTickCount() - app.state.ctickLast) / 1000));
    if (dsec < 60) {
        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDSeconds).data(), LOWORD(dsec));
    } else {
        dmin = LOWORD(static_cast<std::uint32_t>((dsec / 60)));
        dsec -= static_cast<std::uint32_t>((60 * dmin));
        if (dmin < 60) {
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsD02d).data(), dmin, LOWORD(dsec));
        } else {
            dhour = static_cast<std::uint32_t>(dmin) / 60;
            dmin -= 60 * dhour;
            if (dhour < 24) {
                cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsD02d02d).data(), dhour, dmin, LOWORD(dsec));
            } else {
                dday = static_cast<std::uint32_t>(dhour) / 24;
                dhour -= dday * 24;
                cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDDaysD02d02d).data(), dday, dhour, dmin, LOWORD(dsec));
            }
        }
    }
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_TIME_SINCE_TEXT), app.state.szWork);
    win32::SetBkMode(hdc, bkMode);
    win32::SetBkColor(hdc, crBackSav);
    if (!hdcIn) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

INT_PTR  HostModeDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     fRet;
    RECT        rc;
    std::int16_t     mf;
    POINT16     pt;
    std::int16_t     tpm;
    std::int16_t     i;
    std::int16_t     iRet;
    std::int16_t     iSel;
    std::int16_t     iDiamond;
    HMENU       hmenuPopup;
    win32::MSG         msg;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyHostModeDlg, 1);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_GAME_NAME_TEXT), app.state.game.szName);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_FILE_TEXT), app.state.szBase);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), app.state.gd.fReadOnly == 0 && (app.lpui->vtimer.fAutoGenWhenIn || app.lpui->vtimer.mdForce != 0));
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_GENERATE_NOW), app.state.gd.fReadOnly == 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_PASSWORD), app.state.gd.fReadOnly == 0);
        app.lpui->uTimerId = win32::SetTimer(hwnd, 13, 10000, nullptr);
        /* fallthrough */
    case win32::WM_TIMER:
        if (app.lpui->fProcessingTimer)
            goto Done;
        {
            app.lpui->fProcessingTimer = 1;
            CFindTurnsOutstanding(app);
            DrawHostDialog2(app, hwnd, nullptr);
            app.lpui->fProcessingTimer = 0;
        }
    Done:
        if (message != win32::WM_TIMER) {
            return 1;
        }
        return 0;
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
        win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
        return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
    case win32::WM_SETCURSOR:
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < 6 || pt.x >= app.state.dyArial8 + 7 || pt.y < 48)
            break;
        iDiamond = static_cast<std::int16_t>((pt.y - 48)) / (app.state.dyArial8 + 4);
        if (iDiamond >= app.state.game.cPlayer || static_cast<std::int16_t>((pt.y - 48)) % (app.state.dyArial8 + 4) >= app.state.dyArial8 + 1)
            break;
        if (message == win32::WM_SETCURSOR) {
            win32::SetCursor(app.lpui->hcurHand);
            return 1;
        }
        if (!app.state.rgplr[iDiamond].fAi) {
            iSel = 0;
        } else if (app.state.rgplr[iDiamond].idAi == idAiMaid) {
            iSel = 2;
        } else {
            iSel = 1;
        }
        hmenuPopup = win32::CreatePopupMenu();
        app.lpui->iPopMenuSel = -1;
        for (i = 0; i < 3; i++) {
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsHumanControlled + i), std::span{app.state.szWork});
            mf = i == 1                                                         ? app.state.rgplr[iDiamond].fAi != 0 && app.state.rgplr[iDiamond].idAi != idAiMaid ? 0 : 3
                 : app.state.rgplr[iDiamond].fAi == 0 || app.state.rgplr[iDiamond].idAi == idAiMaid ? 0
                                                                                : 3;
            win32::AppendMenuA(hmenuPopup, (i == iSel ? 8 : 0) | mf, i + 15000, app.state.szWork);
        }
        ClientToScreen16(hwnd, &pt);
        tpm = message == win32::WM_LBUTTONDOWN ? win32::TPM_LEFTBUTTON : win32::TPM_RIGHTBUTTON;
        ui::TrackPopupMenu(hmenuPopup, win32::TPM_CENTERALIGN | tpm, pt.x, pt.y, 0, hwnd, nullptr);
        win32::DestroyMenu(hmenuPopup);
        iRet = -1;
        if (ui::PeekMessage(&msg, hwnd, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15100) {
            iRet = msg.wParam - 15000;
        }
        if (iRet != -1 && iSel != iRet) {
            app.state.rgplr[iDiamond].fAi = iRet > 0;
            if (iRet == 2) {
                app.state.rgplr[iDiamond].idAi = idAiMaid;
            }
            app.state.rgplr[iDiamond].lSalt = ~app.state.rgplr[iDiamond].lSalt;
            FMarkFile(app, dtTurn, iDiamond, mdMarkAi, iRet != 0);
            FMarkFile(app, dtHost, iDiamond, mdMarkAi, iRet != 0);
            app.state.gd.fAisDone = 0;
            app.lpui->fProcessingTimer = 1;
            CFindTurnsOutstanding(app);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), app.state.gd.fAllAis == 0 && (app.lpui->vtimer.fAutoGenWhenIn || app.lpui->vtimer.mdForce != 0));
            DrawHostDialog2(app, hwnd, nullptr);
            app.lpui->fProcessingTimer = 0;
        }
        return 0;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        if (app.state.ctickLast == 0) {
            CFindTurnsOutstanding(app);
        }
        if (!app.state.gd.fReadOnly) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), app.state.gd.fAllAis == 0 && (app.lpui->vtimer.fAutoGenWhenIn || app.lpui->vtimer.mdForce != 0));
        }
        DrawHostDialog2(app, hwnd, hdc);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_HOST_GENERATE_NOW:
        case IDCANCEL:
        case IDC_HOST_AUTO_GENERATE:
            if (LOWORD(wParam) == IDC_HOST_GENERATE_NOW) {
                if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) < 0) {
                    if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) < 0) {
                        app.lpui->iPassCnt = 999;
                    } else {
                        app.lpui->iPassCnt = 9;
                    }
                } else if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) < 0) {
                    app.lpui->iPassCnt = 99;
                } else {
                    app.lpui->iPassCnt = 0;
                }
                if (app.lpui->iPassCnt != 0) {
                    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSureWantForceGenerateDTurnsRow).data(), app.lpui->iPassCnt + 1);
                    if (CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), app.state.szWork, "Stars!", win32::MB_YESNO | win32::MB_ICONEXCLAMATION | win32::MB_TASKMODAL) != win32::IDYES) {
                        app.lpui->iPassCnt = 0;
                        return 1;
                    }
                } else if (CFindTurnsOutstanding(app) != 0 &&
                           AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsSureWishGenerateOptionDoesGuaranteePlayers, {}).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_SYSTEMMODAL) != win32::IDYES) {
                    return 1;
                }
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyHostModeDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDCANCEL ? 0 : LOWORD(wParam) == IDC_HOST_AUTO_GENERATE ? -1 : 1);
            if (LOWORD(wParam) == IDC_HOST_AUTO_GENERATE) {
                EnsureAis(app);
            } else if (LOWORD(wParam) == IDCANCEL && app.state.gd.fClose) {
                win32::PostQuitMessage(app.state.vretExitValue);
            }
            return 1;
        case IDC_HOST_PASSWORD:
            if (!FCheckPassword(app))
                break;
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), hwnd, NewPasswordDlg, 0);
            ControlIO().pfnSetFocus(hwnd);
            return fRet;
        case 1029:
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_HOST_OPTIONS), hwnd, HostOptionsDialog, 0);
            ControlIO().pfnSetFocus(hwnd);
            if (fRet && !app.state.gd.fReadOnly) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), app.state.gd.fAllAis == 0 && (app.lpui->vtimer.fAutoGenWhenIn || app.lpui->vtimer.mdForce != 0));
            }
            return fRet;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhHostModeDialog);
            return 1;
        }
        break;
    case win32::WM_DESTROY:
        win32::KillTimer(hwnd, app.lpui->uTimerId);
        app.lpui->uTimerId = 0;
        break;
    }
    return 0;
}

INT_PTR  HostOptionsDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case win32::WM_INITDIALOG:
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
        win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
        return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawHostOptions(hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhHostModeDialog);
            return 1;
        }
        break;
    case win32::WM_DESTROY:
        break;
    }
    return 0;
}

void DrawHostOptions(HWND hwnd, HDC hdc, std::int16_t iDraw) { return; }

void  HostTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND    hwndT;
    char    szExt[4];
    std::int16_t cOut;
    std::int16_t fSav;
    std::int16_t idCur;
    HCURSOR hcurSav;

    if (!app.lpui->fProcessingTimer) {
        app.lpui->fProcessingTimer = 1;
        fSav = app.state.fFileErrSilent;
        if (app.lpui->uTimerType == hostTimerWaitTurn) {
            if (!FNewTurnAvail(app, app.state.idPlayer))
                goto Done;
            idCur = app.state.idPlayer;
            win32::KillTimer(hwnd, app.lpui->uTimerId);
            win32::wsprintfA(szExt, app.state.MPCTD, app.state.idPlayer + 1);
            DestroyCurGame(app);
            if (!FLoadGame(app, app.state.szBase, szExt)) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, {}).data(), win32::MB_ICONHAND);
                goto Done;
            }
            app.state.idPlayer = idCur;
            CreateChildWindows(app);
            app.lpui->uTimerId = win32::SetTimer(nullptr, hostTimerTurnReady, 1000, app.lpui->lpfnHostTimerProc);
            app.lpui->uTimerType = hostTimerTurnReady;
            win32::FlashWindow(app.lpui->hwndFrame, 1);
            ControlIO().pfnSetWindowText(app.lpui->hwndFrame, TextIO().pfnCompressed(app, idsNewTurnAvailable2).data());
            ControlIO().pfnMessageBeep(win32::MB_ICONEXCLAMATION);
            cOut = 1;
            goto RedrawText;
        }
        if (app.lpui->uTimerType == hostTimerTurnReady) {
            win32::FlashWindow(app.lpui->hwndFrame, 1);
            goto Done;
        }
    Loop:
        cOut = CFindTurnsOutstanding(app);
        if (app.state.gd.fAllAis) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsAutoGenerateDisabledBecauseHumanPlayersDead, {}).data(), win32::MB_ICONHAND);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_HOST_AUTO_GENERATE), 0);
            goto Done;
        }
        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsHostModeDPlayer).data(), cOut);
        if (cOut != 1) {
            CchAppendUtf8Prefix(std::span{app.state.szWork}, "s");
        }
        CchAppendUtf8Prefix(std::span{app.state.szWork}, TextIO().pfnCompressed(app, idsOut).data());
        ControlIO().pfnSetWindowText(app.lpui->hwndFrame, app.state.szWork);
    RedrawText:
        hwndT = win32::GetWindow(app.lpui->hwndFrame, win32::GW_HWNDPREV);
        if (win32::GetWindow(hwndT, win32::GW_OWNER) == app.lpui->hwndFrame) {
            ControlIO().pfnInvalidateRect(hwndT, nullptr, 1);
        }
        if (cOut == 0) {
            if (app.state.gd.fProgressTxt) {
                ShowProgressGauge(app);
            }
            EnsureAis(app);
            hcurSav = win32::SetCursor(win32::LoadCursorA(nullptr, win32::IDC_WAIT));
            FGenerateTurn(app);
            win32::SetCursor(hcurSav);
            HideProgressGauge(app);
            if (app.state.ini.fGen) {
                win32::PostQuitMessage(app.state.vretExitValue);
            } else {
                EnsureAis(app);
                goto Loop;
            }
        }
    Done:
        app.lpui->fProcessingTimer = 0;
        app.state.fFileErrSilent = fSav;
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

void SetWindowIniString(APPSESSION &app, char *sz, HWND hwnd) {
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
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsC04d04d04d04d).data(), ch, rc.left, rc.top, rc.right, rc.bottom);
    return;
}

void WriteIniSettings(APPSESSION &app) {
    std::int16_t  ctile;
    char     szPd[3];
    TILE    *rgtile;
    std::int16_t  i;
    std::int16_t  iPass;
    char     szEntry[16];
    char     szIniFile[16];
    char    *psz;
    char     szSection[16];
    std::uint16_t iCol;
    char     ch;

    szPd[0] = '%';
    szPd[1] = 'd';
    szPd[2] = 0;
    TextIO().pfnCchGetString(app, idsWindows, std::span{szSection});
    TextIO().pfnCchGetString(app, idsStarsIni, std::span{szIniFile});
    TextIO().pfnCchGetString(app, idsResolution, std::span{szEntry});
    i = 0;
    if (app.lpui->vcScreenColors <= 4) {
        i |= 1;
    }
    if (app.state.gd.mdScreenSize == 0) {
        i |= 2;
    }
    win32::wsprintfA(app.state.szWork, szPd, i);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsMain, std::span{szEntry});
    SetWindowIniString(app, app.state.szWork, app.lpui->hwndFrame);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsReportfleetwin, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsC04d04d04d04d).data(), 77, app.state.vrptFleet.ptDlg.x, app.state.vrptFleet.ptDlg.y, app.state.vrptFleet.ptDlg.x + app.state.vrptFleet.ptSize.x,
             app.state.vrptFleet.ptDlg.y + app.state.vrptFleet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsReportefleetwin, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsC04d04d04d04d).data(), 77, app.state.vrptEFleet.ptDlg.x, app.state.vrptEFleet.ptDlg.y, app.state.vrptEFleet.ptDlg.x + app.state.vrptEFleet.ptSize.x,
             app.state.vrptEFleet.ptDlg.y + app.state.vrptEFleet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsReportbtlwin, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsC04d04d04d04d).data(), 77, app.state.vrptBattle.ptDlg.x, app.state.vrptBattle.ptDlg.y, app.state.vrptBattle.ptDlg.x + app.state.vrptBattle.ptSize.x,
             app.state.vrptBattle.ptDlg.y + app.state.vrptBattle.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsReportplanwin, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsC04d04d04d04d).data(), 77, app.state.vrptPlanet.ptDlg.x, app.state.vrptPlanet.ptDlg.y, app.state.vrptPlanet.ptDlg.x + app.state.vrptPlanet.ptSize.x,
             app.state.vrptPlanet.ptDlg.y + app.state.vrptPlanet.ptSize.y);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsLayout, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->iWindowLayout);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle1width, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dxPlanWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle1height, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dyMsgWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle1height2, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dyMinWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle2width, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dx2PlanWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle2height, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dy2MsgWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsStyle2height2, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.lpui->vfs.dy2MinWant);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsToolbar, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, szPd, app.state.gd.fToolbar);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    iPass = 2;
    rgtile = app.lpui->rgtilePlanet;
    ctile = 6;
    TextIO().pfnCchGetString(app, idsPlanettiles, std::span{szEntry});
    while (iPass-- != 0) {
        psz = app.state.szWork;
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
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsShiptiles, std::span{szEntry});
        rgtile = app.lpui->rgtileShip;
        ctile = 7;
    }
    TextIO().pfnCchGetString(app, idsSelection, std::span{szEntry});
    switch (app.state.sel.grobj) {
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
    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCCD).data(), ch, static_cast<std::int16_t>(static_cast<std::int8_t>(LOBYTE(app.state.idPlayer + 66))), app.state.sel.id);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsMessage, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.iMsgCur);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsGameid, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, "%lx", app.state.game.lid);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsScanzoom, std::span{szEntry});
    app.state.szWork[0] = app.state.iScanZoom + '5';
    app.state.szWork[1] = 0;
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    if (app.state.gd.fChgScanner) {
        itoa(app.lpui->grbitScan, app.state.szWork, 10);
        TextIO().pfnCchGetString(app, idsScanmodev25, std::span{szEntry});
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        itoa(app.lpui->grbitScanShip, app.state.szWork, 10);
        TextIO().pfnCchGetString(app, idsScanfilterv25, std::span{szEntry});
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        itoa(app.lpui->grbitScanEShip, app.state.szWork, 10);
        TextIO().pfnCchGetString(app, idsScanefilterv25, std::span{szEntry});
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        itoa(app.lpui->grbitScanMines, app.state.szWork, 10);
        TextIO().pfnCchGetString(app, idsScanmines, std::span{szEntry});
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        itoa(app.lpui->vpctRadarView, app.state.szWork, 10);
        TextIO().pfnCchGetString(app, idsScanradar, std::span{szEntry});
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    }
    itoa(app.lpui->cMinGrafMax, app.state.szWork, 10);
    TextIO().pfnCchGetString(app, idsMineralscale, std::span{szEntry});
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    if (app.state.idPlayer != iplrNone) {
        TextIO().pfnCchGetString(app, idsFiles, std::span{szSection});
        TextIO().pfnCchGetString(app, idsWait2, std::span{szEntry});
        app.state.szWork[0] = (app.lpui->uTimerId != 0) + '0';
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        if (app.state.gd.fWriteTurnNum) {
            itoa(app.state.game.turn, app.state.szWork, 10);
            TextIO().pfnCchGetString(app, idsTurn, std::span{szEntry});
            ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
            app.state.gd.fWriteTurnNum = 0;
        }
        TextIO().pfnCchGetString(app, idsFile1, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, "%s.m%d", app.state.szBase, app.state.idPlayer + 1);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    }
    TextIO().pfnCchGetString(app, idsMisc, std::span{szSection});
    if (app.state.gd.fChgReports) {
        TextIO().pfnCchGetString(app, idsReportplanfld, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, app.state.PCTD, LOWORD(app.state.vrptPlanet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportplansort, std::span{szEntry});
        i = app.state.vrptPlanet.icolSort;
        if (app.state.vrptPlanet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintfA(app.state.szWork, app.state.PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportfleetfld, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, app.state.PCTD, LOWORD(app.state.vrptFleet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportfleetsort, std::span{szEntry});
        i = app.state.vrptFleet.icolSort;
        if (app.state.vrptFleet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintfA(app.state.szWork, app.state.PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportefleetfld, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, app.state.PCTD, LOWORD(app.state.vrptEFleet.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportefltsort, std::span{szEntry});
        i = app.state.vrptEFleet.icolSort;
        if (app.state.vrptEFleet.fAscending) {
            i |= 0x100;
        }
        win32::wsprintfA(app.state.szWork, app.state.PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportbtlfld, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, app.state.PCTD, LOWORD(app.state.vrptBattle.grbitVisible));
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportbtlsort, std::span{szEntry});
        i = app.state.vrptBattle.icolSort;
        if (app.state.vrptBattle.fAscending) {
            i |= 0x100;
        }
        win32::wsprintfA(app.state.szWork, app.state.PCTD, i);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
        TextIO().pfnCchGetString(app, idsReportdefgraph, std::span{szEntry});
        win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.gd.iCurGraph);
        ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    }
    TextIO().pfnCchGetString(app, idsHistoryinfo, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->uDateInstalled);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    TextIO().pfnCchGetString(app, idsVcrspeed, std::span{szEntry});
    win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->viSpeedVCR);
    ProfileTextIO().pfnWriteString(szSection, szEntry, app.state.szWork, szIniFile);
    WriteIniPresets(app, szIniFile);
    return;
}

void RefitFrameChildren(APPSESSION &app) {
    std::int16_t dyMsg;
    HMENU   hmenu;
    std::int16_t i;
    std::int16_t dyMinMin;
    std::int16_t dyMsgMin;
    std::int16_t dyMin;
    std::int16_t dyT;
    std::int16_t dyTot;
    std::int16_t yScanner;

    if (app.lpui->hwndFrame && win32::IsIconic(app.lpui->hwndFrame) == 0) {
        switch (app.lpui->iWindowLayout) {
        case layoutLarge:
        default:
            if (app.lpui->vfs.dx - app.lpui->vfs.dxPlanWant < 100) {
                app.lpui->vfs.xTop = app.lpui->vfs.dx - 100;
            } else {
                app.lpui->vfs.xTop = app.lpui->vfs.dxPlanWant;
            }
            app.lpui->vfs.xTop = (((app.lpui->vfs.xTop) > (198 + win32::GetSystemMetrics(win32::SM_CXVSCROLL))) ? (app.lpui->vfs.xTop) : (198 + win32::GetSystemMetrics(win32::SM_CXVSCROLL)));
            dyMsgMin = (0xd * app.state.dyArial8 >> 1) + 0xa;
            dyMinMin = 13 * app.state.dyArial8 - 36;
            dyMsg = app.lpui->vfs.dyMsgWant <= dyMsgMin ? dyMsgMin : app.lpui->vfs.dyMsgWant;
            app.lpui->vfs.dyMsgWant = dyMsg;
            dyMin = app.lpui->vfs.dyMinWant <= dyMinMin ? dyMinMin : app.lpui->vfs.dyMinWant;
            app.lpui->vfs.dyMinWant = dyMin;
            if (app.lpui->vfs.dy - (dyMsg + dyMin + 16) < 50) {
                dyT = app.lpui->vfs.dy - 66;
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
            app.lpui->vfs.y1 = app.lpui->vfs.dy - dyMin - dyMsg - 16;
            app.lpui->vfs.y2 = app.lpui->vfs.y1 + dyMsg + 8;
            if (!app.lpui->hwndScanner)
                break;
            if (app.state.gd.fToolbar) {
                ui::MoveWindow(app.lpui->hwndTb, app.lpui->vfs.xTop + 8, 0, app.lpui->vfs.dx - app.lpui->vfs.xTop - 8, 36, 1);
                yScanner = 36;
            } else {
                ui::MoveWindow(app.lpui->hwndTb, 0, -100, 50, 50, 1);
                yScanner = 0;
            }
            ui::MoveWindow(app.lpui->hwndScanner, app.lpui->vfs.xTop + 8, yScanner, app.lpui->vfs.dx - app.lpui->vfs.xTop - 8, app.lpui->vfs.dy - yScanner, 1);
            MovePlanetPane(app, 0, 0, app.lpui->vfs.xTop, app.lpui->vfs.y1);
            ui::MoveWindow(app.lpui->hwndMessage, 0, app.lpui->vfs.y1 + 8, app.lpui->vfs.xTop, dyMsg, 1);
            ui::MoveWindow(app.lpui->hwndMine, 0, app.lpui->vfs.y2 + 8, app.lpui->vfs.xTop, dyMin, 1);
            break;
        case layoutMedium:
        case layoutSmall:
            if (app.lpui->vfs.dx - app.lpui->vfs.dx2PlanWant < 200) {
                app.lpui->vfs.xTop = app.lpui->vfs.dx - 200;
            } else {
                app.lpui->vfs.xTop = app.lpui->vfs.dx2PlanWant;
            }
            app.lpui->vfs.xTop = (((app.lpui->vfs.xTop) > (198 + win32::GetSystemMetrics(win32::SM_CXVSCROLL))) ? (app.lpui->vfs.xTop) : (198 + win32::GetSystemMetrics(win32::SM_CXVSCROLL)));
            dyMsgMin = (0xd * app.state.dyArial8 >> 1) + 0xa;
            dyMinMin = 13 * app.state.dyArial8 - 36;
            dyMsg = app.lpui->vfs.dy2MsgWant <= dyMsgMin ? dyMsgMin : app.lpui->vfs.dy2MsgWant;
            app.lpui->vfs.dy2MsgWant = dyMsg;
            dyMin = app.lpui->vfs.dy2MinWant <= dyMinMin ? dyMinMin : app.lpui->vfs.dy2MinWant;
            app.lpui->vfs.dy2MinWant = dyMin;
            if (app.lpui->vfs.dy - (dyMsg + 8) < 100) {
                dyMsg = app.lpui->vfs.dy - 108;
            }
            if (app.lpui->vfs.dy - (dyMin + 8) < 100) {
                dyMin = app.lpui->vfs.dy - 108;
            }
            app.lpui->vfs.y1 = app.lpui->vfs.dy - dyMsg - 8;
            app.lpui->vfs.y2 = app.lpui->vfs.dy - dyMin - 8;
            if (app.lpui->hwndScanner) {
                if (app.state.gd.fToolbar) {
                    ui::MoveWindow(app.lpui->hwndTb, 0, 0, app.lpui->vfs.dx, 36, 1);
                    yScanner = 36;
                } else {
                    ui::MoveWindow(app.lpui->hwndTb, 0, -100, 50, 50, 1);
                    yScanner = 0;
                }
                ui::MoveWindow(app.lpui->hwndScanner, app.lpui->vfs.xTop + 8, yScanner, app.lpui->vfs.dx - app.lpui->vfs.xTop - 8, app.lpui->vfs.y2 - yScanner, 1);
                MovePlanetPane(app, 0, yScanner, app.lpui->vfs.xTop, app.lpui->vfs.y1 - yScanner);
                ui::MoveWindow(app.lpui->hwndMessage, 0, app.lpui->vfs.y1 + 8, app.lpui->vfs.xTop, dyMsg, 1);
                ui::MoveWindow(app.lpui->hwndMine, app.lpui->vfs.xTop + 8, app.lpui->vfs.y2 + 8, app.lpui->vfs.dx - app.lpui->vfs.xTop - 8, dyMin, 1);
            }
        }
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
        hmenu = win32::GetSubMenu(hmenu, 4);
        for (i = 130; i <= 132; i++) {
            win32::CheckMenuItem(hmenu, i, i - 130 == app.lpui->iWindowLayout ? win32::MF_CHECKED : win32::MF_UNCHECKED);
        }
    }
    return;
}

LRESULT  TitleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    std::array<char, 360> szVersion{};
    std::array<char, cchCatalogText> szCatalogPsz{};
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    HDC         hdc;
    std::int16_t     i;
    HPALETTE    hpalSav;
    RECT        rc;
    std::int16_t     dy;
    std::int16_t     dxGap;
    std::int16_t     dx;
    std::int16_t     xCur;
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
    std::int16_t     idButton;
    std::int16_t     fIdleSav;

    switch (msg) {
    case win32::WM_CREATE:
        if (std::getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            std::fprintf(stderr, "[DEBUG-library-startup] TitleWndProc WM_CREATE begin hwnd=%p visible=%d\n",
                    static_cast<void *>(hwnd), win32::IsWindowVisible(hwnd));
        if (app.lpui->vcScreenColors >= 8) {
            app.lpui->vhdibTitle = HdibLoadBigResource(app, IDDIB_SPLASH);
            if (!app.lpui->vhpalSplash && app.lpui->vhdibTitle) {
                app.lpui->vhpalSplash = HpalFromDib(app.lpui->vhdibTitle);
            }
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        dx = (((120) < ((rc.right - 48) / 5 - 12)) ? (120) : ((rc.right - 48) / 5 - 12));
        dxGap = (rc.right - 5 * dx) / 6;
        xCur = dxGap;
        dy = rc.bottom <= 500 ? app.state.dyArial8 * 2 : static_cast<std::int16_t>((5 * app.state.dyArial8)) / 2;
        for (i = 0; i < 5; i++) {
            idButton = i == 3 ? IDC_LIBRARY_TUTORIAL : i == 4 ? 3 : i;
            szCatalogPsz = TextIO().pfnCompressed(app, static_cast<StringId>(i == 3 ? idsLibraryStartTutorial : idsNewGame + idButton));
            psz = szCatalogPsz.data();
            hwndButton = ui::CreateWindowEx(0, "BUTTON", psz, win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP, xCur, rc.bottom - dy - static_cast<std::int16_t>((5 * app.state.dyArial8)) / 2, dx, dy, hwnd,
                                      reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(idButton)), app.lpui->hInst, nullptr);
            if (i != 3) app.lpui->rghwndBtnSplash[idButton] = hwndButton;
            if (i == 2 && (app.state.szBase[0] == 0 || !FFileExists(app.state.szBase))) {
                ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[2], 0);
            }
            if (rc.bottom < 500) {
                ControlIO().pfnSendMessage(hwndButton, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
            }
            xCur += dx + dxGap;
        }
        InitLibraryGames(app, hwnd, &rc);
        InitLibraryRaces(app, hwnd, &rc);
        if (std::getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            std::fprintf(stderr, "[DEBUG-library-startup] TitleWndProc library initialized visible=%d\n", win32::IsWindowVisible(hwnd));
        ControlIO().pfnSetWindowText(app.lpui->rghwndBtnSplash[1], TextIO().pfnCompressed(app, idsLibraryOpenGame).data());
        // Paint while the native window is hidden: startup cleanup can delay
        // the first queued WM_PAINT after CreateWindow makes it visible.
        ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
        ControlIO().pfnUpdateWindow(hwnd);
        for (i = 0; i < 4; i++) {
            ControlIO().pfnUpdateWindow(app.lpui->rghwndBtnSplash[i]);
        }
        ControlIO().pfnUpdateWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TUTORIAL));
        break;
    case win32::WM_QUERYNEWPALETTE:
    MapIt:
        if (app.lpui->vcScreenColors < 8)
            break;
        hdc = win32::GetDC(hwnd);
        hpalSav = win32::SelectPalette(hdc, app.lpui->vhpalSplash, 0);
        i = win32::RealizePalette(hdc);
        win32::SelectPalette(hdc, hpalSav, 0);
        win32::ReleaseDC(hwnd, hdc);
        if (i != 0) {
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
            return 1;
        }
        return 0;
    case win32::WM_PALETTECHANGED:
        if (reinterpret_cast<HWND>(wParam) != hwnd)
            goto MapIt;
        break;
    case win32::WM_DESTROY:
        if (app.lpui->vhdibTitle) {
            // HdibLoadBigResource returns an owned, writable heap copy.
            win32::GlobalFree(app.lpui->vhdibTitle);
            app.lpui->vhdibTitle = nullptr;
        }
        if (app.lpui->fFreeingTitle)
            goto Default;
        if (app.state.gd.fExitWindows) {
            ui::RequestQuit(app.state.vretExitValue);
            goto Default;
        }
        WriteIniSettings(app);
        win32::PostQuitMessage(app.state.vretExitValue);
        goto Default;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LIBRARY_TUTORIAL:
            fIdleSav = app.state.gd.fNoIdleChecks;
            app.state.gd.fNoIdleChecks = 1;
            StartTutor(app, 0);
            app.state.gd.fNoIdleChecks = fIdleSav;
            break;
        case 0:
            DesktopChoiceIO().pfnNewGameWizard(app, hwnd, 0);
            if (!app.state.lpPlanets && app.state.game.lid == 0) {
                ui::ShowWindow(app.lpui->hwndFrame, win32::SW_HIDE);
                ui::ShowWindow(app.lpui->hwndTitle, win32::SW_SHOW);
                ControlIO().pfnSetFocus(app.lpui->hwndTitle);
                break;
            }
            if (!app.lpui->fFreeingTitle) {
                app.lpui->fFreeingTitle = 1;
                ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
                app.lpui->hwndTitle = nullptr;
            }
            ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
            break;
        case IDC_LIBRARY_IMPORT:
            if (FImportLibraryGame(app, hwnd, nullptr)) {
                RefreshLibraryGames(app, hwnd);
                RefreshLibraryHostGames(app, hwnd);
            }
            break;
        case IDC_LIBRARY_IMPORT_FOLDER:
            ShowLibraryImportFolderPicker(hwnd);
            break;
        case IDC_LIBRARY_GAMES_VIEW:
            SelectLibraryView(app, hwnd, 0);
            break;
        case IDC_LIBRARY_HOST_VIEW:
            SelectLibraryView(app, hwnd, 1);
            break;
        case IDC_LIBRARY_IMPORT_ORDERS:
            if (FImportLibraryOrders(app, hwnd, nullptr)) RefreshLibraryHostGames(app, hwnd);
            break;
        case IDC_LIBRARY_GAME_EXPORT:
            FExportSelectedLibraryGame(app, hwnd, nullptr);
            break;
        case IDC_LIBRARY_DUPLICATE:
        case IDC_LIBRARY_RENAME:
            if (FActOnSelectedLibraryGame(app, hwnd, LOWORD(wParam) == IDC_LIBRARY_RENAME)) {
                RefreshLibraryGames(app, hwnd);
                RefreshLibraryHostGames(app, hwnd);
            }
            break;
        case IDC_LIBRARY_GAME_DELETE:
            if (FDeleteSelectedLibraryGame(app, hwnd)) {
                RefreshLibraryGames(app, hwnd);
                RefreshLibraryHostGames(app, hwnd);
            }
            break;
        case IDC_LIBRARY_RACES:
            if (HIWORD(wParam) != win32::LBN_DBLCLK || !FSelectLibraryRace(hwnd, szRaceName)) break;
            if (FEditLibraryRace(app, hwnd, szRaceName)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_RACE_NEW:
            if (FBeginLibraryRace(app, hwnd)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_RACE_EDIT:
            if (FSelectLibraryRace(hwnd, szRaceName) && FEditLibraryRace(app, hwnd, szRaceName)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_RACE_DUPLICATE:
            if (FSelectLibraryRace(hwnd, szRaceName) && FDuplicateLibraryRace(app, hwnd, szRaceName)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_RACE_EXPORT:
            if (FSelectLibraryRace(hwnd, szRaceName)) FExportLibraryRace(app, hwnd, szRaceName, nullptr);
            break;
        case IDC_LIBRARY_RACE_DELETE:
            if (FSelectLibraryRace(hwnd, szRaceName) && FDeleteLibraryRace(app, hwnd, szRaceName)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_RACE_IMPORT:
            if (FImportLibraryRaceAndEdit(app, hwnd, nullptr)) RefreshLibraryRaces(app, hwnd);
            break;
        case IDC_LIBRARY_GAMES:
            if (HIWORD(wParam) == win32::LBN_SELCHANGE) { RefreshLibraryPlayers(app, hwnd); break; }
            if (HIWORD(wParam) != win32::LBN_DBLCLK) break;
            if (!FSelectLibraryGame(app, hwnd)) break;
            goto LOpenGame;
        case IDC_LIBRARY_PLAYERS:
            if (HIWORD(wParam) == win32::CBN_SELCHANGE)
                ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[1], ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_GETCURSEL, 0, 0) != win32::CB_ERR);
            break;
        case IDC_LIBRARY_HOST_GAMES:
            if (HIWORD(wParam) != win32::LBN_DBLCLK) break;
            if (!FSelectLibraryHostGame(app, hwnd)) break;
            goto LOpenGame;
        case IDOK:
            if (!FSelectLibraryGame(app, hwnd)) break;
        LOpenGame:
            if (FOpenGame(app, hwnd, 0) > 0) {
                if (!app.lpui->fFreeingTitle) {
                    app.lpui->fFreeingTitle = 1;
                    ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
                    app.lpui->hwndTitle = nullptr;
                }
                if (app.state.idPlayer != iplrNone) {
                    ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
                }
                RememberLibraryGame(app);
                InitializeMenu(app, nullptr);
                win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
                if (app.state.game.fTutorial && app.state.idPlayer == 0) {
                    StartTutor(app, 0);
                }
            } else if (win32::IsWindow(app.lpui->hwndTitle)) {
                ControlIO().pfnSetFocus(hwnd);
            }
            app.state.ini.fStartupFile = 0;
            break;
        case IDCANCEL:
            if (!FContinueLibraryGame(app, hwnd)) break;
            goto LOpenGame;
        case 3:
            if (app.state.gd.fExitWindows) {
                ui::RequestQuit(app.state.vretExitValue);
            } else {
                WriteIniSettings(app);
                win32::PostQuitMessage(app.state.vretExitValue);
            }
        }
        break;
    case WM_STARS_IMPORT_FOLDER:
        if (auto *pszPath = static_cast<char *>(PvCompatMessagePayload(wParam))) {
            if (FImportLibraryGame(app, hwnd, pszPath)) {
                RefreshLibraryGames(app, hwnd);
                RefreshLibraryHostGames(app, hwnd);
            }
        }
        break;
    case WM_STARS_EXPORT_FOLDER:
        if (auto *pszPath = static_cast<char *>(PvCompatMessagePayload(wParam))) {
            FExportSelectedLibraryGame(app, hwnd, pszPath);
        }
        break;
    case win32::WM_PAINT:
        if (win32::IsIconic(hwnd) != 0) {
            hdc = win32::BeginPaint(hwnd, &ps);
            win32::DrawIcon(hdc, 2, 2, app.lpui->hiconStars);
            win32::EndPaint(hwnd, &ps);
            break;
        }
        plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
        plfOwner.reset(plf);
        hdc = win32::BeginPaint(hwnd, &ps);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonFace));
        ControlIO().pfnGetClientRect(hwnd, &rcWnd);
        if (app.lpui->vcScreenColors >= 8) {
            win32::SetTextColor(hdc, 50298879);
            if (!app.lpui->vhdibTitle)
                goto LTry16Color;
            win32::SelectPalette(hdc, app.lpui->vhpalSplash, 0);
            win32::RealizePalette(hdc);
            DibBlt(hdc, 0, 0, rcWnd.right, rcWnd.bottom, app.lpui->vhdibTitle, 0, 0, 800, 600, 13369376);
        } else {
        LTry16Color:
            rc.left = 0;
            rc.right = rcWnd.right;
            rc.bottom = 0;
            dx = rcWnd.right;
            DrawABunchOfStars(app, hdc, &rcWnd);
            plf->lfHeight = static_cast<std::int16_t>(-rcWnd.bottom) / 3;
            CchCopyUtf8Prefix(std::span{plf->lfFaceName}, app.lpui->rgszArial[3]);
            hfont = win32::CreateFontIndirectA(plf);
            win32::SetTextColor(hdc, 10158235);
            if (hfont) {
                hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, hfont));
                win32::SetBkMode(hdc, win32::TRANSPARENT);
                rcT = rcWnd;
                rcT.bottom = static_cast<std::int16_t>((3 * rcT.bottom)) / 4;
                RcCtrTextOut(app, hdc, &rcT, "Stars!", 6);
                win32::SelectObject(hdc, hfontSav);
                win32::DeleteObject(hfont);
            }
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial10[1]);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        szVersion = SzVersion(app);
        win32::GetWindowRect(app.lpui->rghwndBtnSplash[0], &rcT);
        win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rcT), 2);
        rcWnd.top = rcT.top - static_cast<std::int16_t>((9 * app.state.dyArial8)) / 2;
        rcWnd.bottom = static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + rcWnd.top;
        RcCtrTextOut(app, hdc, &rcWnd, szVersion.data(), std::strlen(szVersion.data()));
        win32::EndPaint(hwnd, &ps);
        plfOwner.reset();
        break;
    default:
    Default:
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void RestoreGameWindows(APPSESSION &app) {
    if (!app.lpui->hwndScanner)
        return;
    app.state.gd.fNoScannerDraw = 1;
    RestoreSelection(app);
    ControlIO().pfnRefitFrameChildren(app);
    app.state.gd.fNoScannerDraw = 0;
    ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
    ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
    return;
}

std::int16_t FinishOpenGame(APPSESSION &app) {
    GrobjClass grobjIni;
    if (app.state.idPlayer != iplrNone) {
        grobjIni = static_cast<GrobjClass>(app.state.ini.grobjSel);
        ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        if (grobjIni != grobjNone && app.state.ini.grobjSel == 0) {
            return 1;
        }
        app.state.ini.grobjSel = 0;
    } else {
        if (app.lpui->hwndTitle && !app.lpui->fFreeingTitle) {
            app.lpui->fFreeingTitle = 1;
            ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
            app.lpui->hwndTitle = nullptr;
        }
        ControlIO().pfnBringUpHostDlg(app);
        return 0;
    }
    if (app.state.ini.grobjSel == 0 && app.state.cPlanet != 0) {
        PresentationIO().pfnFindSelection(app);
    }
    return 1;
}
