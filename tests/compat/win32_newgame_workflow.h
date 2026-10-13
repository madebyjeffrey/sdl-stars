/* Exercise Basic/Advanced New Game and cancellation. Windows drives the real
 * compatibility picker; native portal results remain qualified by fileworkflow. */
static const char *pszNewGame;
static BOOL fCreatingGame;
static int cNewGameChoices, iNewGameStep;
static GAME gameNewSav;
void StarsTestReal_NewGameWizard(HWND hwnd, int16_t fReadOnly);
BOOL StarsTestReal_FNativeSaveFileName(OPENFILENAMEA *lpof);

void StarsTest_NewGameWizard(HWND hwnd, int16_t fReadOnly) {
    fCreatingGame = pszNewGame != NULL;
    StarsTestReal_NewGameWizard(hwnd, fReadOnly);
    fCreatingGame = FALSE;
}

BOOL StarsTest_FNativeSaveFileName(OPENFILENAMEA *lpof) {
    const char *pszTitle, *pszExtension;
    size_t cb;
    BOOL fResult;

    if (!pszNewGame) return StarsTestReal_FNativeSaveFileName(lpof);
    cNewGameChoices++;
    if (getenv("STARS_CLIENT_NEW_GAME_REAL_DIALOG")) {
        fResult = StarsTestReal_FNativeSaveFileName(lpof);
        fprintf(stderr, "New Game chooser %d: result %d, error %lu\n", cNewGameChoices, fResult, (unsigned long)DwNativeFileDialogError());
        return fResult;
    }
    if (cNewGameChoices == 1) { SetLastError(0); return FALSE; }
    cb = strlen(pszNewGame) + 1;
    if (cNewGameChoices != 2 || cb > lpof->nMaxFile || !lpof->lpstrFile ||
        !(lpof->Flags & OFN_PATHMUSTEXIST) || strcmp(lpof->lpstrDefExt, "xy")) return FALSE;
    pszTitle = strrchr(pszNewGame, '/'); pszTitle = pszTitle ? pszTitle + 1 : pszNewGame;
    pszExtension = strrchr(pszNewGame, '.');
    memcpy(lpof->lpstrFile, pszNewGame, cb);
    lpof->nFileOffset = (WORD)(pszTitle - pszNewGame);
    lpof->nFileExtension = (WORD)(pszExtension - pszNewGame + 1);
    if (lpof->lpstrFileTitle && strlen(pszTitle) + 1 <= lpof->nMaxFileTitle)
        strcpy(lpof->lpstrFileTitle, pszTitle);
    return TRUE;
}

static BOOL FCheckNewGameWorkflow(void) {
    HWND hwndDialog = NULL, hwnd;
    DLGPROC lpfn;
    char szFile[512];
    int i;

    if (fCreatingGame && getenv("STARS_CLIENT_NEW_GAME_REAL_DIALOG")) {
        for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
            if (hwnd->fDialog && GetDlgItem(hwnd, 0x480)) { hwndDialog = hwnd; break; }
        }
        if (hwndDialog && IsWindowVisible(hwndDialog)) {
            if (cNewGameChoices == 1) return FPushClick(GetDlgItem(hwndDialog, IDCANCEL));
            if (cNewGameChoices == 2) {
                if (!SetWindowTextA(GetDlgItem(hwndDialog, 0x480), pszNewGame)) return FALSE;
                return FPushClick(GetDlgItem(hwndDialog, IDOK));
            }
            return FALSE;
        }
    }

    if (iNewGameStep == 0) {
        if (!IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle)) return FALSE;
        gameNewSav = game; fChecked = TRUE; iNewGameStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (iNewGameStep == 1 || iNewGameStep == 4) {
        for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == SimpleNewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, IDC_SIMPLE_NEW_GAME_TINY));
    }
    if (iNewGameStep == 2 || iNewGameStep == 5) {
        for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == SimpleNewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog || IsDlgButtonChecked(hwndDialog, IDC_SIMPLE_NEW_GAME_TINY) != BST_CHECKED) return FALSE;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, iNewGameStep == 6 && getenv("STARS_CLIENT_NEW_ADVANCED") ?
                                    IDC_SIMPLE_NEW_GAME_ADVANCED : IDC_FINISH));
    }
    if (getenv("STARS_CLIENT_NEW_ADVANCED") && iNewGameStep >= 6 && iNewGameStep <= 10) {
        lpfn = iNewGameStep == 6 ? NewGameDlg :
               iNewGameStep == 7 || iNewGameStep == 9 ? NewGameDlg2 : NewGameDlg3;
        for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == lpfn) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        i = iNewGameStep == 8 ? IDC_BACK : iNewGameStep == 10 ? IDC_FINISH : IDC_NEXT;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, i));
    }
    if (fCreatingGame) return TRUE;
    if (iNewGameStep == 3) {
        if (cNewGameChoices != 1 || memcmp(&game, &gameNewSav, sizeof(GAME)) ||
            !IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle) || FFileExists((char *)pszNewGame)) return FALSE;
        iNewGameStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (iNewGameStep != (getenv("STARS_CLIENT_NEW_ADVANCED") ? 11 : 6) || cNewGameChoices != 2 || game.lid == 0 || game.turn != 0 ||
        !game.fSinglePlr || idPlayer == iplrNone || !lpPlanets || game.cPlanMax == 0 ||
        IsWindow(hwndTitle) || !IsWindowVisible(hwndFrame) || !IsWindow(hwndScanner)) return FALSE;
    for (i = -1; i <= 1; i++) {
        snprintf(szFile, sizeof(szFile), i == -1 ? "%s.xy" : i == 0 ? "%s.hst" : "%s.m1", szBase);
        if (!FFileExists(szFile)) return FALSE;
    }
    UpdateWindow(hwndFrame);
    UpdateWindow(hwndScanner);
    if (!FCompatPresent(hwndFrame)) return FALSE;
    fPassed = TRUE;
    puts("New Game: chooser cancel preserves title/state; Basic/Advanced Finish creates and opens a tiny game");
    PostQuitMessage(0);
    return TRUE;
}
