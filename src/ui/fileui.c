#include "win.h"

INT_PTR CALLBACK AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SAVE:
        case IDC_NO_DON_T_SAVE:
        case IDC_SAVESUBMIT:
            EndDialog(hwnd, LOWORD(wParam) == IDC_NO_DON_T_SAVE ? 0 : LOWORD(wParam) == IDC_SAVESUBMIT ? -1 : 1);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, 1090);
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

    lpProc = MakeProcInstance(AskSaveDialog, hInst);
    fRet = DialogBox(hInst, !game.fSinglePlr ? MAKEINTRESOURCE(IDD_SAVE_TURN1) : MAKEINTRESOURCE(IDD_SAVE_TURN2), hwndFrame, lpProc);
    FreeProcInstance(lpProc);
    if (fRet) {
        gd.fSubmit = fRet == -1;
        FWriteLogFile(szBase, idPlayer);
        FWriteHistFile(idPlayer);
    }
    return;
}

// ShowTitleScreen brings back the title screen when FLoadGame fails.
void ShowTitleScreen() {
    RECT rcWork;
    RECT rc;
    DWORD dwStyle;
    int dx, dy, x, y;

    if (!hwndTitle) {
        dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0)) {
            SetRect(&rcWork, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        }
        SetRect(&rc, 0, 0, 800, 600);
        dx = min(rc.right - rc.left, rcWork.right - rcWork.left - 2 * GetSystemMetrics(SM_CXDLGFRAME));
        dy = min(rc.bottom - rc.top, rcWork.bottom - rcWork.top -
                 GetSystemMetrics(SM_CYCAPTION) - 2 * GetSystemMetrics(SM_CYDLGFRAME));
        x = rcWork.left + (rcWork.right - rcWork.left - dx) / 2;
        y = rcWork.top + (rcWork.bottom - rcWork.top - dy) / 2;
        hwndTitle = CreateWindow(szTitle, "Stars!", dwStyle | WS_VISIBLE, x, y, dx, dy, hwndFrame, NULL, hInst, NULL);
        fFreeingTitle = FALSE;
        ShowWindow(hwndFrame, SW_HIDE);
    }
    return;
}

// AddMRUFile puts a game the player opened at the top of the File menu's
// recent files and saves the list to stars.ini.
void AddMRUFile(char *pszFileName, char *pszExt) {
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
        CchGetString(idsStarsIni, szIniFile);
        CchGetString(idsFiles, szSection);
        CchGetString(idsFile1, szEntry);
        psz = &szEntry[strlen(szEntry) - 1];
        for (i = 0; i < 9; i++) {
            *psz = i + '1';
            strcpy(szT, vrgszMRU + 256 * i);
            WritePrivateProfileString(szSection, szEntry, szT, szIniFile);
        }
    }
    return;
}

// CloseGameWindows closes the windows that show the game DestroyCurGame
// is unloading and resets the tile layout.
void CloseGameWindows() {
    int16_t i;

    if (hwndBrowser) {
        DestroyWindow(hwndBrowser);
    }
    if (hwndReportDlg) {
        DestroyWindow(hwndReportDlg);
    }
    if (hwndPopup) {
        DestroyWindow(hwndPopup);
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
