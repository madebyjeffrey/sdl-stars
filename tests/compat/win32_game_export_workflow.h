/* Drive the export button and the real callback/message path. Only the native
 * folder chooser's response is supplied at the SDL boundary. */
static int cGameExportPickers, cGameExportSaves, cGameExportWait;
static BOOL fGameExportBusy;
static uint64_t idGameExportSaveDialog;
static int16_t fGameExportSubmitSav;

void SDLCALL StarsTest_SDL_ShowOpenFolderDialog(SDL_DialogFileCallback pfnCallback, void *lpData,
                                               SDL_Window *lpWindow, const char *pszLocation, bool fMany) {
    const char *pszGame = getenv("STARS_CLIENT_EXPORT_GAME");
    const char *pszAction = getenv("STARS_CLIENT_EXPORT_ACTION");
    const char *rgpszPath[2] = {NULL, NULL};
    if (!pszGame) {
        SDL_ShowOpenFolderDialog(pfnCallback, lpData, lpWindow, pszLocation, fMany);
        return;
    }
    cGameExportPickers++;
    if (!pszAction || strcmp(pszAction, "cancel")) rgpszPath[0] = getenv("STARS_CLIENT_EXPORT_PARENT");
    pfnCallback(lpData, rgpszPath, 0);
}

static BOOL FCheckGameExportWorkflow(UINT_PTR idTimer) {
    const char *pszGame = getenv("STARS_CLIENT_EXPORT_GAME");
    const char *pszParent = getenv("STARS_CLIENT_EXPORT_PARENT");
    const char *pszAction = getenv("STARS_CLIENT_EXPORT_ACTION");
    const char *pszPlayer = getenv("STARS_CLIENT_EXPORT_PLAYER");
    const char *pszResearch = getenv("STARS_CLIENT_EXPORT_RESEARCH");
    int iPlayer = pszPlayer ? atoi(pszPlayer) : 0;
    int iResearch = pszResearch ? atoi(pszResearch) : 4;
    HWND hwndSave = HwndWorkflowDialog(), hwndList;
    LRESULT i, c;
    char szDestination[512], szRow[320], szPrefix[280];
    int16_t fCancel = pszAction && !strcmp(pszAction, "cancel");
    int16_t fDiscard = pszAction && !strcmp(pszAction, "discard");
    int16_t fVerify = pszAction && !strcmp(pszAction, "verify");
    int16_t fFail = getenv("STARS_CLIENT_EXPORT_EXPECTED") != NULL;
    int cSaves;

    if (hwndSave && hwndSave->lpfnDlg == AskSaveDialog) {
        if (idGameExportSaveDialog == StarsCompatWindowId(hwndSave)) return TRUE;
        idGameExportSaveDialog = StarsCompatWindowId(hwndSave);
        cGameExportSaves++;
        return FPushClick(GetDlgItem(hwndSave, fDiscard ? IDC_NO_DON_T_SAVE : IDC_SAVE));
    }
    if (fGameExportBusy || fPassed) return TRUE;
    fGameExportBusy = TRUE;
    if (++cGameExportWait > 40) goto LFail;
    if (iStage == 0) {
        if (!IsWindowVisible(hwndTitle) || !pszParent) goto LFail;
        if (fVerify) {
            iStage = 1;
            if (!FPushClick(rghwndBtnSplash[2])) goto LFail;
        } else if (!FSelectClientGameName(hwndTitle, pszGame)) goto LFail;
        else if (pszAction) {
            /* Choose the requested player through the same Open Game action. */
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
            c = SendMessage(hwndList, LB_GETCOUNT, 0, 0);
            CchSnprintf(szPrefix, sizeof(szPrefix), "%s - Player %d (", pszGame, iPlayer + 1);
            for (i = 0; i < c; i++) {
                SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szRow);
                if (!strncmp(szRow, szPrefix, strlen(szPrefix))) break;
            }
            if (i == c) goto LFail;
            SendMessage(hwndList, LB_SETCURSEL, i, 0);
            iStage = 1;
            if (!FPushClick(rghwndBtnSplash[1])) goto LFail;
        } else {
            fGameExportSubmitSav = gd.fSubmit;
            iStage = 3;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
        }
    } else if (iStage == 1) {
        if (IsWindow(hwndTitle)) goto LWait;
        if (!lpPlanets || idPlayer != iPlayer || !IsWindow(hwndScanner)) goto LFail;
        if (fVerify) {
            if (rgplr[idPlayer].iTechNow != iResearch) goto LFail;
            goto LDone;
        }
        if (rgplr[idPlayer].iTechNow == iResearch) goto LFail;
        if (!FRunWorkflow(1, iResearch) || !game.fDirty || rgplr[idPlayer].iTechNow != iResearch) goto LFail;
        if (!strcmp(pszAction, "submit")) {
            SendMessage(hwndFrame, WM_COMMAND, IDM_TURN_SAVE_SUBMIT, 0);
            if (game.fDirty || !gd.fSubmit) goto LFail;
        }
        fGameExportSubmitSav = gd.fSubmit;
        /* Exercise the selected-loaded-game save path without discarding its
         * orders, as returning via File/Open normally tears that state down. */
        ShowTitleScreen();
        iStage = 2;
    } else if (iStage == 2) {
        if (!FSelectClientGameName(hwndTitle, pszGame) ||
            !FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
        iStage = 3;
    } else if (iStage == 3) {
        if (cGameExportPickers == 0) goto LWait;
        if (cGameExportPickers != 1) goto LFail;
        CchSnprintf(szDestination, sizeof(szDestination), "%s%s%s", pszParent, szDirSep, pszGame);
        if (fFail) {
            if (cAlerts == 0) goto LWait;
            if (cAlerts != 1) goto LFail;
        } else if (!fCancel && !FNativeDirectoryExists(szDestination)) goto LWait;
        if (fCancel && FNativeDirectoryExists(szDestination)) goto LFail;
        cSaves = pszAction && (!strcmp(pszAction, "save") || fDiscard) ? 1 : 0;
        if (cGameExportSaves != cSaves || gd.fSubmit != fGameExportSubmitSav) goto LFail;
        if (pszAction && (game.fDirty != (fCancel || fDiscard))) goto LFail;
        goto LDone;
    }
LWait:
    fGameExportBusy = FALSE;
    return TRUE;
LDone:
    fChecked = fPassed = TRUE;
    fGameExportBusy = FALSE;
    puts("Managed game export: button, folder callback, save choice and real files checked");
    KillTimer(NULL, idTimer);
    PostQuitMessage(0);
    return TRUE;
LFail:
    fprintf(stderr, "Game export failed: stage %d pickers %d saves %d alerts %d dirty %d\n",
            iStage, cGameExportPickers, cGameExportSaves, cAlerts, game.fDirty);
    fGameExportBusy = FALSE;
    return FALSE;
}
