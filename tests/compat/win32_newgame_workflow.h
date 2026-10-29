/* Drive automatic library naming and the real wizard; inject a filesystem obstacle only
 * for the write-failure case, then retain the real game writer. */
static const char *pszNewGame;
static BOOL fCreatingGame;
static int cNewGameChoices, iNewGameStep;
static BOOL fRevisionRenamed;
static GAME gameNewSav;
void StarsTestReal_NewGameWizard(HWND hwnd, int16_t fReadOnly);

static BOOL FSamePath(const char *pszLeft, const char *pszRight) {
    char chLeft, chRight;
    while (*pszLeft && *pszRight) {
        chLeft = *pszLeft++;
        chRight = *pszRight++;
        if (chLeft == '\\') chLeft = '/';
        if (chRight == '\\') chRight = '/';
        if (chLeft != chRight) return FALSE;
    }
    return *pszLeft == 0 && *pszRight == 0;
}

void StarsTest_NewGameWizard(HWND hwnd, int16_t fReadOnly) {
    fCreatingGame = pszNewGame != NULL;
    StarsTestReal_NewGameWizard(hwnd, fReadOnly);
    fCreatingGame = FALSE;
}

int16_t StarsTestReal_FCreateFile(DtFileType dt, int16_t iplr, char *pszBase);
int16_t StarsTest_FCreateFile(DtFileType dt, int16_t iplr, char *pszBase) {
    const char *pszFailure = PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE");
    DtFileType dtFailure = pszFailure && !strcmp(pszFailure, "host") ? dtHost : dtXY;
    int16_t fCreated;
    if (fCreatingGame && dt == dtFailure && pszFailure) {
        SetSzWorkFromDt(dt, iplr);
        if (!SDL_CreateDirectory(szWork)) return FALSE;
    }
    fCreated = StarsTestReal_FCreateFile(dt, iplr, pszBase);
    if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE") && fCreated) {
        if (dt == dtLog) cSwitchLogWrites++;
        if (dt == dtHist) cSwitchHistoryWrites++;
    }
    return fCreated;
}

static BOOL FCheckNewGameWorkflow(void) {
    HWND hwndDialog = NULL, hwnd;
    DLGPROC lpfn;
    char szFile[512];
    int i;
    SDL_Event event = {0};

    if (fCreatingGame) {
        if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
            for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                char szCaption[256];
                GetWindowText(hwnd, szCaption, sizeof(szCaption));
                if (!strcmp(szCaption, "Games") && IsWindowVisible(hwnd) && IsWindowEnabled(hwnd) && GetDlgItem(hwnd, IDOK))
                    return FPushClick(GetDlgItem(hwnd, IDOK));
            }
        }
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
            if (hwnd->fDialog && hwnd->lpfnDlg == LibraryNameDlg) return FALSE;
            if (hwnd->fDialog && hwnd->lpfnDlg == LibraryGameRevisionDlg) hwndDialog = hwnd;
        }
        if (hwndDialog && IsWindowVisible(hwndDialog)) {
            const char *pszSuggested = PszCompatEnvironment("STARS_CLIENT_LIBRARY_SUGGESTED");
            const char *pszCapture = PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_CAPTURE");
            char szPrompt[512];
            GetDlgItemText(hwndDialog, IDC_LIBRARY_GAME_REVISION_TEXT, szPrompt, sizeof(szPrompt));
            if (pszSuggested && !strstr(szPrompt, pszSuggested)) return FALSE;
            if (pszCapture && (!FCompatPaintTree(hwndDialog) || !FCompatPresent(hwndDialog) ||
                               !SDL_SaveBMP(hwndDialog->lpComposite, pszCapture))) return FALSE;
            cNewGameChoices++;
            return FPushClick(GetDlgItem(hwndDialog,
                PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_BACK") && cNewGameChoices == 1 ? IDCANCEL : IDOK));
        }
        if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_BACK") && cNewGameChoices == 1 && !fRevisionRenamed) {
            for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
                if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg && IsWindowEnabled(hwnd)) hwndDialog = hwnd;
            if (hwndDialog) {
                if (IsDlgButtonChecked(hwndDialog, IDC_NEW_GAME_TINY) != BST_CHECKED ||
                    FFileExists((char *)pszNewGame) || !SetDlgItemText(hwndDialog, IDC_NEW_GAME_NAME,
                        PszCompatEnvironment("STARS_CLIENT_LIBRARY_RENAME_TITLE"))) return FALSE;
                fRevisionRenamed = TRUE;
                return FPushClick(GetDlgItem(hwndDialog, IDC_FINISH));
            }
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
            if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        {
            const char *pszTitle = PszCompatEnvironment("STARS_CLIENT_GAME_TITLE");
            if (!pszTitle) pszTitle = PszCompatEnvironment("STARS_CLIENT_LIBRARY_NAME");
            if (pszTitle && !SetDlgItemText(hwndDialog, IDC_NEW_GAME_NAME, pszTitle)) return FALSE;
        }
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, IDC_NEW_GAME_TINY));
    }
    if (iNewGameStep == 2 || iNewGameStep == 5) {
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog || IsDlgButtonChecked(hwndDialog, IDC_NEW_GAME_TINY) != BST_CHECKED) return FALSE;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, iNewGameStep == 3 ? IDCANCEL :
                                    iNewGameStep == 6 && PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") ? IDC_NEXT : IDC_FINISH));
    }
    if (PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") && iNewGameStep >= 6 && iNewGameStep <= 9) {
        lpfn = iNewGameStep == 6 || iNewGameStep == 8 ? NewGameDlg2 : NewGameDlg3;
        for (hwnd = GetWindow(hwndTitle ? hwndTitle : hwndFrame, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == lpfn) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        i = iNewGameStep == 7 ? IDC_BACK : iNewGameStep == 9 ? IDC_FINISH : IDC_NEXT;
        iNewGameStep++;
        return FPushClick(GetDlgItem(hwndDialog, i));
    }
    if (fCreatingGame) return TRUE;
    if (iNewGameStep == 3) {
        if (cNewGameChoices != 0 || memcmp(&game, &gameNewSav, sizeof(GAME)) ||
            !IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle) || FFileExists((char *)pszNewGame)) return FALSE;
        iNewGameStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
        // Showing the restored title can pump a timer before ShowWindow completes.
        if (!IsWindowVisible(hwndTitle)) return ++cWait < 20;
        if (cAlerts != 2 || game.lid != 0 || !IsWindowVisible(hwndTitle)) return FALSE;
        fPassed = TRUE;
        puts("Managed game: real game-file write failure leaves no playable entry");
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = hwndTitle->id;
        return SDL_PushEvent(&event);
    }
    if (iNewGameStep != (PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") ? 10 : 6) || game.lid == 0 || game.turn != 0 ||
        !game.fSinglePlr || idPlayer == iplrNone || !lpPlanets || game.cPlanMax == 0 ||
        IsWindow(hwndTitle) || !IsWindowVisible(hwndFrame) || !IsWindow(hwndScanner)) return FALSE;
    {
        const char *pszTitle = fRevisionRenamed ? PszCompatEnvironment("STARS_CLIENT_LIBRARY_RENAME_TITLE") : PszCompatEnvironment("STARS_CLIENT_GAME_TITLE");
        if (pszTitle && strcmp(game.szName, pszTitle)) {
            fprintf(stderr, "Generated title [%s], expected [%s]\n", game.szName, pszTitle);
            return FALSE;
        }
        snprintf(szFile, sizeof(szFile), "%s.xy", szBase);
        if (!FSamePath(szFile, pszNewGame)) {
            fprintf(stderr, "Generated path [%s], expected [%s]\n", szFile, pszNewGame);
            return FALSE;
        }
    }
    for (i = -1; i <= 1; i++) {
        snprintf(szFile, sizeof(szFile), i == -1 ? "%s.xy" : i == 0 ? "%s.hst" : "%s.m1", szBase);
        if (!FFileExists(szFile)) return FALSE;
    }
    UpdateWindow(hwndFrame);
    UpdateWindow(hwndScanner);
    if (!FCompatPresent(hwndFrame)) return FALSE;
    fPassed = TRUE;
    puts("New Game: cancellation preserves state; title-derived library naming creates and opens a tiny game");
    PostQuitMessage(0);
    return TRUE;
}
