#include "appstate.h"
#include "uistate.h"
/* Drive automatic library naming and the real wizard; inject a filesystem obstacle only
 * for the write-failure case, then retain the real game writer. */
static const char *pszNewGame;
static BOOL fCreatingGame;
static int cNewGameChoices, iNewGameStep;
static BOOL fRevisionRenamed;
static GAME gameNewSav;

static BOOL FSamePath(const char *pszLeft, const char *pszRight) {
    char chLeft, chRight;
    while (*pszLeft && *pszRight) {
        chLeft = *pszLeft++;
        chRight = *pszRight++;
        if (chLeft == '\\') chLeft = '/';
        if (chRight == '\\') chRight = '/';
        if (chLeft != chRight) return 0;
    }
    return *pszLeft == 0 && *pszRight == 0;
}

void StarsTest_NewGameWizard(APPSESSION &app, HWND hwnd, int16_t fReadOnly) {
    fCreatingGame = pszNewGame != NULL;
    NewGameWizard(app, hwnd, fReadOnly);
    fCreatingGame = 0;
}

int16_t StarsTest_FCreateFile(APPSESSION &app, DtFileType dt, int16_t iplr, char *pszBase) {
    const char *pszFailure = PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE");
    DtFileType dtFailure = pszFailure && !strcmp(pszFailure, "host") ? dtHost : dtXY;
    int16_t fCreated;
    if (fCreatingGame && dt == dtFailure && pszFailure) {
        SetSzWorkFromDt(app, dt, iplr);
        if (!SDL_CreateDirectory(app.state.szWork)) return 0;
    }
    fCreated = FCreateFile(app, dt, iplr, pszBase);
    if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE") && fCreated) {
        if (dt == dtLog) cSwitchLogWrites++;
        if (dt == dtHist) cSwitchHistoryWrites++;
    }
    return fCreated;
}

static BOOL FCheckNewGameWorkflow(APPSESSION &app) {
    HWND hwndDialog = NULL, hwnd;
    DLGPROC lpfn;
    char szFile[512];
    int i;
    SDL_Event event = {0};

    if (fCreatingGame) {
        if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
            for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
                char szCaption[256];
                win32::GetWindowTextA(hwnd, szCaption, sizeof(szCaption));
                if (!strcmp(szCaption, "Games") && win32::IsWindowVisible(hwnd) && win32::IsWindowEnabled(hwnd) && win32::GetDlgItem(hwnd, IDOK))
                    return FPushClick(app, win32::GetDlgItem(hwnd, IDOK));
            }
        }
        for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
            if (hwnd->fDialog && hwnd->lpfnDlg == LibraryNameDlg) return 0;
            if (hwnd->fDialog && hwnd->lpfnDlg == LibraryGameRevisionDlg) hwndDialog = hwnd;
        }
        if (hwndDialog && win32::IsWindowVisible(hwndDialog)) {
            const char *pszSuggested = PszCompatEnvironment("STARS_CLIENT_LIBRARY_SUGGESTED");
            const char *pszCapture = PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_CAPTURE");
            char szPrompt[512];
            win32::GetDlgItemTextA(hwndDialog, IDC_LIBRARY_GAME_REVISION_TEXT, szPrompt, sizeof(szPrompt));
            if (pszSuggested && !strstr(szPrompt, pszSuggested)) return 0;
            if (pszCapture && (!FCompatPaintTree(hwndDialog) || !FCompatPresent(hwndDialog) ||
                               !SDL_SaveBMP(hwndDialog->lpComposite.get(), pszCapture))) return 0;
            cNewGameChoices++;
            return FPushClick(app, win32::GetDlgItem(hwndDialog,
                PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_BACK") && cNewGameChoices == 1 ? IDCANCEL : IDOK));
        }
        if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_REVISION_BACK") && cNewGameChoices == 1 && !fRevisionRenamed) {
            for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
                if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg && win32::IsWindowEnabled(hwnd)) hwndDialog = hwnd;
            if (hwndDialog) {
                if (win32::IsDlgButtonChecked(hwndDialog, IDC_NEW_GAME_TINY) != win32::BST_CHECKED ||
                    FFileExists((char *)pszNewGame) || !win32::SetDlgItemTextA(hwndDialog, IDC_NEW_GAME_NAME,
                        PszCompatEnvironment("STARS_CLIENT_LIBRARY_RENAME_TITLE"))) return 0;
                fRevisionRenamed = 1;
                return FPushClick(app, win32::GetDlgItem(hwndDialog, IDC_FINISH));
            }
        }
    }

    if (iNewGameStep == 0) {
        if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::IsWindowEnabled(app.lpui->hwndTitle)) {
            if (++cWait < 20) return 1;
            fprintf(stderr, "[DEBUG-library-startup] new-game start timed out title=%p valid=%d visible=%d enabled=%d\n",
                    (void *)app.lpui->hwndTitle, win32::IsWindow(app.lpui->hwndTitle), win32::IsWindowVisible(app.lpui->hwndTitle), win32::IsWindowEnabled(app.lpui->hwndTitle));
            return 0;
        }
        cWait = 0;
        gameNewSav = app.state.game; fChecked = 1; iNewGameStep++;
        return FPushClick(app, app.lpui->rghwndBtnSplash[0]);
    }
    if (iNewGameStep == 1 || iNewGameStep == 4) {
        for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        {
            const char *pszTitle = PszCompatEnvironment("STARS_CLIENT_GAME_TITLE");
            if (!pszTitle) pszTitle = PszCompatEnvironment("STARS_CLIENT_LIBRARY_NAME");
            if (pszTitle && !win32::SetDlgItemTextA(hwndDialog, IDC_NEW_GAME_NAME, pszTitle)) return 0;
        }
        iNewGameStep++;
        return FPushClick(app, win32::GetDlgItem(hwndDialog, IDC_NEW_GAME_TINY));
    }
    if (iNewGameStep == 2 || iNewGameStep == 5) {
        for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == NewGameDlg) { hwndDialog = hwnd; break; }
        if (!hwndDialog || win32::IsDlgButtonChecked(hwndDialog, IDC_NEW_GAME_TINY) != win32::BST_CHECKED) return 0;
        iNewGameStep++;
        return FPushClick(app, win32::GetDlgItem(hwndDialog, iNewGameStep == 3 ? IDCANCEL :
                                    iNewGameStep == 6 && PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") ? IDC_NEXT : IDC_FINISH));
    }
    if (PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") && iNewGameStep >= 6 && iNewGameStep <= 9) {
        lpfn = iNewGameStep == 6 || iNewGameStep == 8 ? NewGameDlg2 : NewGameDlg3;
        for (hwnd = win32::GetWindow(app.lpui->hwndTitle ? app.lpui->hwndTitle : app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
            if (hwnd->fDialog && hwnd->lpfnDlg == lpfn) { hwndDialog = hwnd; break; }
        if (!hwndDialog) return fCreatingGame;
        i = iNewGameStep == 7 ? IDC_BACK : iNewGameStep == 9 ? IDC_FINISH : IDC_NEXT;
        iNewGameStep++;
        return FPushClick(app, win32::GetDlgItem(hwndDialog, i));
    }
    if (fCreatingGame) return 1;
    if (iNewGameStep == 3) {
        if (cNewGameChoices != 0 || memcmp(&app.state.game, &gameNewSav, sizeof(GAME)) ||
            !win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::IsWindowEnabled(app.lpui->hwndTitle) || FFileExists((char *)pszNewGame)) return 0;
        iNewGameStep++;
        return FPushClick(app, app.lpui->rghwndBtnSplash[0]);
    }
    if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE")) {
        // Showing the restored title can pump a timer before ShowWindow completes.
        if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return ++cWait < 20;
        if (cAlerts != 2 || app.state.game.lid != 0 || !win32::IsWindowVisible(app.lpui->hwndTitle)) return 0;
        fPassed = 1;
        puts("Managed game: real game-file write failure leaves no playable entry");
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = app.lpui->hwndTitle->id;
        return SDL_PushEvent(&event);
    }
    if (iNewGameStep != (PszCompatEnvironment("STARS_CLIENT_NEW_PAGES") ? 10 : 6) || app.state.game.lid == 0 || app.state.game.turn != 0 ||
        !app.state.game.fSinglePlr || app.state.idPlayer == iplrNone || !app.state.lpPlanets || app.state.game.cPlanMax == 0 ||
        win32::IsWindow(app.lpui->hwndTitle) || !win32::IsWindowVisible(app.lpui->hwndFrame) || !win32::IsWindow(app.lpui->hwndScanner)) return 0;
    {
        const char *pszTitle = fRevisionRenamed ? PszCompatEnvironment("STARS_CLIENT_LIBRARY_RENAME_TITLE") : PszCompatEnvironment("STARS_CLIENT_GAME_TITLE");
        if (pszTitle && strcmp(app.state.game.szName, pszTitle)) {
            fprintf(stderr, "Generated title [%s], expected [%s]\n", app.state.game.szName, pszTitle);
            return 0;
        }
        snprintf(szFile, sizeof(szFile), "%s.xy", app.state.szBase);
        if (!FSamePath(szFile, pszNewGame)) {
            fprintf(stderr, "Generated path [%s], expected [%s]\n", szFile, pszNewGame);
            return 0;
        }
    }
    for (i = -1; i <= 1; i++) {
        snprintf(szFile, sizeof(szFile), i == -1 ? "%s.xy" : i == 0 ? "%s.hst" : "%s.m1", app.state.szBase);
        if (!FFileExists(szFile)) return 0;
    }
    win32::UpdateWindow(app.lpui->hwndFrame);
    win32::UpdateWindow(app.lpui->hwndScanner);
    if (!FCompatPresent(app.lpui->hwndFrame)) return 0;
    fPassed = 1;
    puts("New Game: cancellation preserves state; title-derived library naming creates and opens a tiny game");
    win32::PostQuitMessage(0);
    return 1;
}
