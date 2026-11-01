#include "win.h"
#include "callbackcontext.h"
#include "appstate.h"
#include "uistate.h"
#include "control_io.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "record_io.h"
#include "desktop_choice_io.h"

INT_PTR  AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    switch (message) {
    case win32::WM_INITDIALOG:
        return 1;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SAVE:
        case IDC_NO_DON_T_SAVE:
        case IDC_SAVESUBMIT:
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDC_NO_DON_T_SAVE ? 2 : LOWORD(wParam) == IDC_SAVESUBMIT ? -1 : 1);
            return 1;
        case IDCANCEL:
            ControlIO().pfnEndDialog(hwnd, -2);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, 1090);
            return 1;
        }
        /* fallthrough */
    case win32::WM_DESTROY:
    default:
        return 0;
    }
}

void PromptSaveGame(APPSESSION &app) {
    std::int16_t fRet;

    fRet = ControlIO().pfnDialogBox(app.lpui->hInst, !app.state.game.fSinglePlr ? win32::MAKEINTRESOURCE(IDD_SAVE_TURN1) : win32::MAKEINTRESOURCE(IDD_SAVE_TURN2), app.lpui->hwndFrame, DesktopChoiceIO().pfnAskSaveDialog, 0);
    if (fRet == 1 || fRet == -1) {
        app.state.gd.fSubmit = fRet == -1;
        FWriteLogFile(app, app.state.szBase, app.state.idPlayer);
        FWriteHistFile(app, app.state.idPlayer);
    }
    return;
}

std::int16_t FCanLeaveCurrentGame(APPSESSION &app) {
    std::int16_t fRet;
    std::int16_t fSubmitSav;

    if (app.state.idPlayer == iplrNone || !app.state.game.fDirty) return 1;
    fRet = ControlIO().pfnDialogBox(app.lpui->hInst, !app.state.game.fSinglePlr ? win32::MAKEINTRESOURCE(IDD_SAVE_TURN1) : win32::MAKEINTRESOURCE(IDD_SAVE_TURN2),
                     app.lpui->hwndFrame, DesktopChoiceIO().pfnAskSaveDialog, 0);
    if (fRet == 0 || fRet == -2) return 0;
    if (fRet == 1 || fRet == -1) {
        fSubmitSav = app.state.gd.fSubmit;
        app.state.gd.fSubmit = fRet == -1;
        if (!FWriteLogFile(app, app.state.szBase, app.state.idPlayer) || !FWriteHistFile(app, app.state.idPlayer)) {
            app.state.gd.fSubmit = fSubmitSav;
            /* The orders writer clears dirty before the history save. */
            RecordIO().pfnDirtyGame(app, 1);
            return 0;
        }
    }
    app.state.game.fDirty = 0;
    return 1;
}

// ShowTitleScreen brings back the title screen when FLoadGame fails.
void ShowTitleScreen(APPSESSION &app) {
    RECT rcWork;
    RECT rc;
    DWORD dwStyle;
    int dx, dy, x, y;

    if (!app.lpui->hwndTitle) {
        dwStyle = win32::WS_OVERLAPPED | win32::WS_CAPTION | win32::WS_SYSMENU | win32::WS_MINIMIZEBOX;
        if (!win32::SystemParametersInfoA(win32::SPI_GETWORKAREA, 0, &rcWork, 0)) {
            win32::SetRect(&rcWork, 0, 0, win32::GetSystemMetrics(win32::SM_CXSCREEN), win32::GetSystemMetrics(win32::SM_CYSCREEN));
        }
        win32::SetRect(&rc, 0, 0, 800, 600);
        dx = (((rc.right - rc.left) < (rcWork.right - rcWork.left - 2 * win32::GetSystemMetrics(win32::SM_CXDLGFRAME))) ? (rc.right - rc.left) : (rcWork.right - rcWork.left - 2 * win32::GetSystemMetrics(win32::SM_CXDLGFRAME)));
        dy = (((rc.bottom - rc.top) < (rcWork.bottom - rcWork.top -
                 win32::GetSystemMetrics(win32::SM_CYCAPTION) - 2 * win32::GetSystemMetrics(win32::SM_CYDLGFRAME))) ? (rc.bottom - rc.top) : (rcWork.bottom - rcWork.top -
                 win32::GetSystemMetrics(win32::SM_CYCAPTION) - 2 * win32::GetSystemMetrics(win32::SM_CYDLGFRAME)));
        x = rcWork.left + (rcWork.right - rcWork.left - dx) / 2;
        y = rcWork.top + (rcWork.bottom - rcWork.top - dy) / 2;
        app.lpui->hwndTitle = ui::CreateWindowEx(0, app.lpui->szTitle, "Stars!", dwStyle | win32::WS_VISIBLE, x, y, dx, dy, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        if (std::getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            std::fprintf(stderr, "[DEBUG-library-startup] ShowTitleScreen CreateWindow returned title=%p valid=%d visible=%d\n",
                    static_cast<void *>(app.lpui->hwndTitle), win32::IsWindow(app.lpui->hwndTitle), win32::IsWindowVisible(app.lpui->hwndTitle));
        app.lpui->fFreeingTitle = 0;
        ui::ShowWindow(app.lpui->hwndFrame, win32::SW_HIDE);
    }
    return;
}

// AddMRUFile puts a game the player opened at the top of the File menu's
// recent files and saves the list to stars.ini.
void AddMRUFile(APPSESSION &app, const char *pszFileName, const char *pszExt) {
    std::int16_t i;
    char    szT[256];
    char    szIniFile[16];
    char    szSection[16];
    char   *psz;
    char    szEntry[16];

    if (!app.lpui->vrgszMRU) {
        return;
    }
    std::strcpy(szT, pszFileName);
    std::strcat(szT, ".");
    std::strcat(szT, pszExt);
    if (_stricmp(szT, app.lpui->vrgszMRU) != 0) {
        for (i = 1; i < 8 && _stricmp(szT, app.lpui->vrgszMRU + 256 * i) != 0; i++) {
        }
        for (; i >= 1; i--) {
            std::strcpy(app.lpui->vrgszMRU + 256 * i, app.lpui->vrgszMRU + 256 * (i - 1));
        }
        std::strcpy(app.lpui->vrgszMRU, szT);
        TextIO().pfnCchGetString(app, idsStarsIni, std::span{szIniFile});
        TextIO().pfnCchGetString(app, idsFiles, std::span{szSection});
        TextIO().pfnCchGetString(app, idsFile1, std::span{szEntry});
        psz = &szEntry[std::strlen(szEntry) - 1];
        for (i = 0; i < 9; i++) {
            *psz = i + '1';
            std::strcpy(szT, app.lpui->vrgszMRU + 256 * i);
            ProfileTextIO().pfnWriteString(szSection, szEntry, szT, szIniFile);
        }
    }
    return;
}

// CloseGameWindows closes the windows that show the game DestroyCurGame
// is unloading and resets the tile layout.
void CloseGameWindows(APPSESSION &app) {
    std::int16_t i;

    if (app.lpui->hwndBrowser) {
        ControlIO().pfnDestroyWindow(app.lpui->hwndBrowser);
    }
    if (app.lpui->hwndReportDlg) {
        ControlIO().pfnDestroyWindow(app.lpui->hwndReportDlg);
    }
    if (app.lpui->hwndPopup) {
        ControlIO().pfnDestroyWindow(app.lpui->hwndPopup);
        app.lpui->hwndPopup = nullptr;
    }
    app.lpui->hwndActive = nullptr;
    app.lpui->fOrdersVis = 0;
    app.lpui->dxPlanetProdLB = 0;
    app.lpui->dxOrderED = 0;
    app.lpui->dxFleetCompLB = 0;
    app.lpui->dxShipLB = 0;
    app.lpui->dxShipDD = 0;
    for (i = 0; i < 3; i++) {
        app.lpui->rgdxOrderDD[i] = 0;
    }
    return;
}
