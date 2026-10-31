#include "win.h"
#include "control_io.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "record_io.h"
#include "desktop_choice_io.h"

INT_PTR CALLBACK AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
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
            win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, 1090);
            return 1;
        }
        /* fallthrough */
    case WM_DESTROY:
    default:
        return 0;
    }
}

void PromptSaveGame() {
    FARPROC lpProc;
    int16_t fRet;

    lpProc = MakeProcInstance(DesktopChoiceIO().pfnAskSaveDialog, hInst);
    fRet = ControlIO().pfnDialogBox(hInst, !game.fSinglePlr ? MAKEINTRESOURCE(IDD_SAVE_TURN1) : MAKEINTRESOURCE(IDD_SAVE_TURN2), hwndFrame, lpProc, 0);
    FreeProcInstance(lpProc);
    if (fRet == 1 || fRet == -1) {
        gd.fSubmit = fRet == -1;
        FWriteLogFile(szBase, idPlayer);
        FWriteHistFile(idPlayer);
    }
    return;
}

int16_t FCanLeaveCurrentGame(void) {
    FARPROC lpProc;
    int16_t fRet;
    int16_t fSubmitSav;

    if (idPlayer == iplrNone || !game.fDirty) return TRUE;
    lpProc = MakeProcInstance(DesktopChoiceIO().pfnAskSaveDialog, hInst);
    fRet = ControlIO().pfnDialogBox(hInst, !game.fSinglePlr ? MAKEINTRESOURCE(IDD_SAVE_TURN1) : MAKEINTRESOURCE(IDD_SAVE_TURN2),
                     hwndFrame, lpProc, 0);
    FreeProcInstance(lpProc);
    if (fRet == 0 || fRet == -2) return FALSE;
    if (fRet == 1 || fRet == -1) {
        fSubmitSav = gd.fSubmit;
        gd.fSubmit = fRet == -1;
        if (!FWriteLogFile(szBase, idPlayer) || !FWriteHistFile(idPlayer)) {
            gd.fSubmit = fSubmitSav;
            /* The orders writer clears dirty before the history save. */
            RecordIO().pfnDirtyGame(TRUE);
            return FALSE;
        }
    }
    game.fDirty = FALSE;
    return TRUE;
}

// ShowTitleScreen brings back the title screen when FLoadGame fails.
void ShowTitleScreen() {
    RECT rcWork;
    RECT rc;
    DWORD dwStyle;
    int dx, dy, x, y;

    if (!hwndTitle) {
        dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (!win32::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0)) {
            win32::SetRect(&rcWork, 0, 0, win32::GetSystemMetrics(SM_CXSCREEN), win32::GetSystemMetrics(SM_CYSCREEN));
        }
        win32::SetRect(&rc, 0, 0, 800, 600);
        dx = (((rc.right - rc.left) < (rcWork.right - rcWork.left - 2 * win32::GetSystemMetrics(SM_CXDLGFRAME))) ? (rc.right - rc.left) : (rcWork.right - rcWork.left - 2 * win32::GetSystemMetrics(SM_CXDLGFRAME)));
        dy = (((rc.bottom - rc.top) < (rcWork.bottom - rcWork.top -
                 win32::GetSystemMetrics(SM_CYCAPTION) - 2 * win32::GetSystemMetrics(SM_CYDLGFRAME))) ? (rc.bottom - rc.top) : (rcWork.bottom - rcWork.top -
                 win32::GetSystemMetrics(SM_CYCAPTION) - 2 * win32::GetSystemMetrics(SM_CYDLGFRAME)));
        x = rcWork.left + (rcWork.right - rcWork.left - dx) / 2;
        y = rcWork.top + (rcWork.bottom - rcWork.top - dy) / 2;
        hwndTitle = ui::CreateWindowEx(0, szTitle, "Stars!", dwStyle | WS_VISIBLE, x, y, dx, dy, hwndFrame, NULL, hInst, NULL);
        if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
            fprintf(stderr, "[DEBUG-library-startup] ShowTitleScreen CreateWindow returned title=%p valid=%d visible=%d\n",
                    (void *)hwndTitle, win32::IsWindow(hwndTitle), win32::IsWindowVisible(hwndTitle));
        fFreeingTitle = FALSE;
        ui::ShowWindow(hwndFrame, SW_HIDE);
    }
    return;
}

// AddMRUFile puts a game the player opened at the top of the File menu's
// recent files and saves the list to stars.ini.
void AddMRUFile(const char *pszFileName, const char *pszExt) {
    int16_t i;
    char    szT[256];
    char    szIniFile[16];
    char    szSection[16];
    char   *psz;
    char    szEntry[16];

    if (!vrgszMRU) {
        return;
    }
    strcpy(szT, pszFileName);
    strcat(szT, ".");
    strcat(szT, pszExt);
    if (_stricmp(szT, vrgszMRU) != 0) {
        for (i = 1; i < 8 && _stricmp(szT, vrgszMRU + 256 * i) != 0; i++) {
        }
        for (; i >= 1; i--) {
            strcpy(vrgszMRU + 256 * i, vrgszMRU + 256 * (i - 1));
        }
        strcpy(vrgszMRU, szT);
        TextIO().pfnCchGetString(idsStarsIni, szIniFile);
        TextIO().pfnCchGetString(idsFiles, szSection);
        TextIO().pfnCchGetString(idsFile1, szEntry);
        psz = &szEntry[strlen(szEntry) - 1];
        for (i = 0; i < 9; i++) {
            *psz = i + '1';
            strcpy(szT, vrgszMRU + 256 * i);
            ProfileTextIO().pfnWriteString(szSection, szEntry, szT, szIniFile);
        }
    }
    return;
}

// CloseGameWindows closes the windows that show the game DestroyCurGame
// is unloading and resets the tile layout.
void CloseGameWindows() {
    int16_t i;

    if (hwndBrowser) {
        ControlIO().pfnDestroyWindow(hwndBrowser);
    }
    if (hwndReportDlg) {
        ControlIO().pfnDestroyWindow(hwndReportDlg);
    }
    if (hwndPopup) {
        ControlIO().pfnDestroyWindow(hwndPopup);
        hwndPopup = 0;
    }
    hwndActive = 0;
    fOrdersVis = FALSE;
    dxPlanetProdLB = 0;
    dxOrderED = 0;
    dxFleetCompLB = 0;
    dxShipLB = 0;
    dxShipDD = 0;
    for (i = 0; i < 3; i++) {
        rgdxOrderDD[i] = 0;
    }
    return;
}
