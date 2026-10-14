/* Drive the real naming dialog and wizard; inject a filesystem obstacle only
 * for the write-failure case, then retain the real game writer. */
static const char *pszNewGame;
static BOOL fCreatingGame;
static int cNewGameChoices, iNewGameStep, iLibraryRejected;
static const char *rgszLibraryRejected[] = {"CON", "../escape", "Trailing.", "gAmE",
    "A very long library name that would overflow the game and backup path buffers when combined with its managed folder and basename repeated twice in the full path on every supported platform with the existing 256 byte storage limit"};
static GAME gameNewSav;
void StarsTestReal_NewGameWizard(HWND hwnd, int16_t fReadOnly);

void StarsTest_NewGameWizard(HWND hwnd, int16_t fReadOnly) {
    fCreatingGame = pszNewGame != NULL;
    StarsTestReal_NewGameWizard(hwnd, fReadOnly);
    fCreatingGame = FALSE;
}

int16_t StarsTestReal_FCreateFile(DtFileType dt, int16_t iplr, char *pszBase);
int16_t StarsTest_FCreateFile(DtFileType dt, int16_t iplr, char *pszBase) {
    const char *pszFailure = getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE");
    DtFileType dtFailure = pszFailure && !strcmp(pszFailure, "host") ? dtHost : dtXY;
    if (fCreatingGame && dt == dtFailure && pszFailure) {
        SetSzWorkFromDt(dt, iplr);
        if (!SDL_CreateDirectory(szWork)) return FALSE;
    }
    return StarsTestReal_FCreateFile(dt, iplr, pszBase);
}

static BOOL FCheckNewGameWorkflow(void) {
    HWND hwndDialog = NULL, hwnd;
    DLGPROC lpfn;
    char szFile[512];
    int i;
    SDL_Event event = {0};

    if (fCreatingGame) {
        if (getenv("STARS_CLIENT_LIBRARY_REJECT") || getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
            for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                char szCaption[256];
                GetWindowText(hwnd, szCaption, sizeof(szCaption));
                if (!strcmp(szCaption, "Games") && IsWindowVisible(hwnd) && IsWindowEnabled(hwnd) && GetDlgItem(hwnd, IDOK))
                    return FPushClick(GetDlgItem(hwnd, IDOK));
            }
        }
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
            if (hwnd->fDialog && hwnd->lpfnDlg == LibraryNameDlg) { hwndDialog = hwnd; break; }
        }
        if (hwndDialog && IsWindowVisible(hwndDialog)) {
            if (cNewGameChoices == 0) {
                cNewGameChoices++;
                return FPushClick(GetDlgItem(hwndDialog, IDCANCEL));
            }
            if (cNewGameChoices == 1 && iNewGameStep >= 6) {
                const char *pszName = getenv("STARS_CLIENT_LIBRARY_NAME");
                const char *pszSuggested = getenv("STARS_CLIENT_LIBRARY_SUGGESTED");
                char szName[256];
                GetDlgItemText(hwndDialog, IDC_EDIT1, szName, sizeof(szName));
                if (pszSuggested && strcmp(szName, pszSuggested)) return FALSE;
                cNewGameChoices++;
                if (getenv("STARS_CLIENT_LIBRARY_REJECT")) return TRUE;
                if (!SetWindowTextA(GetDlgItem(hwndDialog, IDC_EDIT1), pszName ? pszName : "fresh-game")) return FALSE;
                return FPushClick(GetDlgItem(hwndDialog, IDOK));
            }
            if (cNewGameChoices == 2 && getenv("STARS_CLIENT_LIBRARY_REJECT")) {
                const char *pszName;
                if (iLibraryRejected < (int)(sizeof(rgszLibraryRejected) / sizeof(rgszLibraryRejected[0])))
                    pszName = rgszLibraryRejected[iLibraryRejected++];
                else {
                    pszName = getenv("STARS_CLIENT_LIBRARY_NAME");
                    cNewGameChoices++;
                }
                SetDlgItemText(hwndDialog, IDC_EDIT1, pszName);
                return FPushClick(GetDlgItem(hwndDialog, IDOK));
            }
            return FALSE;
        }
    }

    if (iNewGameStep == 0) {
        if (!IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle)) {
            if (++cWait < 20) return TRUE;
            fprintf(stderr, "[DEBUG-library-startup] new-game start timed out title=%p valid=%d visible=%d enabled=%d\n",
                    (void *)hwndTitle, IsWindow(hwndTitle), IsWindowVisible(hwndTitle), IsWindowEnabled(hwndTitle));
            return FALSE;
        }
        cWait = 0;
        gameNewSav = game; fChecked = TRUE; iNewGameStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (iNewGameStep == 1 || iNewGameStep == 4) {
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == SimpleNewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, IDC_SIMPLE_NEW_GAME_TINY));
    }
    if (iNewGameStep == 2 || iNewGameStep == 5) {
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == SimpleNewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog || IsDlgButtonChecked(hwndDialog, IDC_SIMPLE_NEW_GAME_TINY) != BST_CHECKED) return FALSE;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, iNewGameStep == 6 && getenv("STARS_CLIENT_NEW_ADVANCED") ?
                                    IDC_SIMPLE_NEW_GAME_ADVANCED : IDC_FINISH));
    }
    if (getenv("STARS_CLIENT_NEW_ADVANCED") && iNewGameStep >= 6 && iNewGameStep <= 10) {
        lpfn = iNewGameStep == 6 ? NewGameDlg :
               iNewGameStep == 7 || iNewGameStep == 9 ? NewGameDlg2 : NewGameDlg3;
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == lpfn) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        i = iNewGameStep == 8 ? IDC_BACK : iNewGameStep == 10 ? IDC_FINISH : IDC_NEXT;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, i));
    }
    if (fCreatingGame && iNewGameStep == 3 && cNewGameChoices == 1) {
        // Cancelling naming returns to the live wizard page.
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == SimpleNewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog || !IsWindowEnabled(hwndDialog)) return TRUE;
        if (IsDlgButtonChecked(hwndDialog, IDC_SIMPLE_NEW_GAME_TINY) != BST_CHECKED ||
            FFileExists((char *)pszNewGame)) return FALSE;
        return FPushClick(GetDlgItem(hwndDialog, IDCANCEL));
    }
    if (fCreatingGame) return TRUE;
    if (iNewGameStep == 3) {
        if (cNewGameChoices != 1 || memcmp(&game, &gameNewSav, sizeof(GAME)) ||
            !IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle) || FFileExists((char *)pszNewGame)) return FALSE;
        iNewGameStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
        // Showing the restored title can pump a timer before ShowWindow completes.
        if (!IsWindowVisible(hwndTitle)) return ++cWait < 20;
        if (cNewGameChoices != 2 || cAlerts != 2 || game.lid != 0 || !IsWindowVisible(hwndTitle)) return FALSE;
        fPassed = TRUE;
        puts("Managed game: real game-file write failure leaves no playable entry");
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = hwndTitle->id;
        return SDL_PushEvent(&event);
    }
    if (iNewGameStep != (getenv("STARS_CLIENT_NEW_ADVANCED") ? 11 : 6) || cNewGameChoices != (getenv("STARS_CLIENT_LIBRARY_REJECT") ? 3 : 2) || game.lid == 0 || game.turn != 0 ||
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
    puts("New Game: name cancellation preserves title/state; Basic/Advanced Finish creates and opens a tiny game");
    PostQuitMessage(0);
    return TRUE;
}
