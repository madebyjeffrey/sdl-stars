/* Drive the export button and the real callback/message path. Only the native
 * folder chooser's response is supplied at the SDL boundary. */
static int64_t cbGameExportFile;
static int cGameExportPickers, cGameExportSaves, cGameExportWait;
static BOOL fGameExportBusy;
static uint64_t idGameExportSaveDialog;
static int16_t fGameExportSubmitSav;

extern "C" void SDLCALL StarsTest_SDL_ShowOpenFolderDialog(SDL_DialogFileCallback pfnCallback, void *lpData,
                                               SDL_Window *lpWindow, const char *pszLocation, bool fMany) {
    const char *pszGame = PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME");
    const char *pszAction = PszCompatEnvironment("STARS_CLIENT_EXPORT_ACTION");
    const char *rgpszPath[2] = {NULL, NULL};
    if (!pszGame) {
        SDL_ShowOpenFolderDialog(pfnCallback, lpData, lpWindow, pszLocation, fMany);
        return;
    }
    cGameExportPickers++;
    if ((!pszAction || strcmp(pszAction, "cancel")) && !PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP_CANCEL"))
        rgpszPath[0] = PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP") ? PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP") : PszCompatEnvironment("STARS_CLIENT_EXPORT_PARENT");
    pfnCallback(lpData, rgpszPath, 0);
}

static BOOL FCheckGameExportWorkflow(UINT_PTR idTimer) {
    const char *pszGame = PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME");
    const char *pszParent = PszCompatEnvironment("STARS_CLIENT_EXPORT_PARENT");
    const char *pszZip = PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP");
    const char *pszAction = PszCompatEnvironment("STARS_CLIENT_EXPORT_ACTION");
    const char *pszPlayer = PszCompatEnvironment("STARS_CLIENT_EXPORT_PLAYER");
    const char *pszResearch = PszCompatEnvironment("STARS_CLIENT_EXPORT_RESEARCH");
    int iPlayer = pszPlayer ? atoi(pszPlayer) : 0;
    int iResearch = pszResearch ? atoi(pszResearch) : 4;
    HWND hwndSave = HwndWorkflowDialog();
    char szDestination[512];
    int16_t fCancel = (pszAction && !strcmp(pszAction, "cancel")) || PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP_CANCEL") != NULL;
    int16_t fDiscard = pszAction && !strcmp(pszAction, "discard");
    int16_t fVerify = pszAction && !strcmp(pszAction, "verify");
    int16_t fFail = PszCompatEnvironment("STARS_CLIENT_EXPORT_EXPECTED") != NULL;
    int cSaves;

    if (hwndSave && hwndSave->lpfnDlg == DesktopChoiceIO().pfnAskSaveDialog) {
        if (idGameExportSaveDialog == StarsCompatWindowId(hwndSave)) return TRUE;
        idGameExportSaveDialog = StarsCompatWindowId(hwndSave);
        cGameExportSaves++;
        return FPushClick(win32::GetDlgItem(hwndSave, fDiscard ? IDC_NO_DON_T_SAVE : IDC_SAVE));
    }
    if (fGameExportBusy || fPassed) return TRUE;
    fGameExportBusy = TRUE;
    if (++cGameExportWait > 40) goto LFail;
    if (iStage == 0) {
        if (!win32::IsWindowVisible(hwndTitle) || (!pszParent && !pszZip)) goto LFail;
        if (fVerify) {
            iStage = 1;
            /* Verify this exported file through the real loader; Continue now
             * deliberately resumes the managed session instead of an MRU path. */
            CchSnprintf(szBase, sizeof(szBase), "%s", PszCompatEnvironment("STARS_CLIENT_CONTINUE"));
            ini.fStartupFile = TRUE;
            if (FOpenGame(hwndTitle, FALSE) <= 0) goto LFail;
            fFreeingTitle = TRUE;
            win32::DestroyWindow(hwndTitle);
            hwndTitle = 0;
            ini.fStartupFile = FALSE;
            win32::ShowWindow(hwndFrame, SW_SHOW);
            InitializeMenu(NULL);
            win32::PostMessage(hwndFrame, WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        } else if (!FSelectClientGameName(hwndTitle, pszGame)) goto LFail;
        else if (pszAction) {
            /* Choose the requested player through the same Open Game action. */
            if (!FSelectClientLibraryPlayer(hwndTitle, iPlayer)) goto LFail;
            iStage = 1;
            if (!FPushClick(rghwndBtnSplash[1])) goto LFail;
        } else {
            fGameExportSubmitSav = gd.fSubmit;
            iStage = 3;
            if (!FPushClick(win32::GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
        }
    } else if (iStage == 1) {
        if (win32::IsWindow(hwndTitle)) goto LWait;
        if (!lpPlanets || idPlayer != iPlayer || !win32::IsWindow(hwndScanner)) goto LFail;
        if (fVerify) {
            if (rgplr[idPlayer].iTechNow() != iResearch) goto LFail;
            goto LDone;
        }
        if (rgplr[idPlayer].iTechNow() == iResearch) goto LFail;
        if (!FRunWorkflow(1, iResearch) || !game.fDirty || rgplr[idPlayer].iTechNow() != iResearch) goto LFail;
        if (!strcmp(pszAction, "submit")) {
            win32::SendMessageA(hwndFrame, WM_COMMAND, IDM_TURN_SAVE_SUBMIT, 0);
            if (game.fDirty || !gd.fSubmit) goto LFail;
        }
        fGameExportSubmitSav = gd.fSubmit;
        /* Exercise the selected-loaded-game save path without discarding its
         * orders, as returning via File/Open normally tears that state down. */
        ShowTitleScreen();
        iStage = 2;
    } else if (iStage == 2) {
        if (!FSelectClientGameName(hwndTitle, pszGame) ||
            !FPushClick(win32::GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
        iStage = 3;
    } else if (iStage == 3) {
        if (cGameExportPickers == 0) goto LWait;
        if (cGameExportPickers != 1) goto LFail;
        if (pszZip) CchSnprintf(szDestination, sizeof(szDestination), "%s", pszZip);
        else CchSnprintf(szDestination, sizeof(szDestination), "%s%s%s", pszParent, szDirSep, pszGame);
        if (fFail) {
            if (cAlerts == 0) goto LWait;
            if (cAlerts != 1) goto LFail;
        } else if (!fCancel && (pszZip ? !FNativeFileSize(szDestination, &cbGameExportFile) : !FNativeDirectoryExists(szDestination))) goto LWait;
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
    puts("Managed game export: button, destination callback, save choice and real files checked");
    win32::KillTimer(NULL, idTimer);
    win32::PostQuitMessage(0);
    return TRUE;
LFail:
    fprintf(stderr, "Game export failed: stage %d pickers %d saves %d alerts %d dirty %d\n",
            iStage, cGameExportPickers, cGameExportSaves, cAlerts, game.fDirty);
    fGameExportBusy = FALSE;
    return FALSE;
}
