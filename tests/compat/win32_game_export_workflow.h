#include "appstate.h"
#include "uistate.h"
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

static BOOL FCheckGameExportWorkflow(APPSESSION &app, UINT_PTR idTimer) {
    const char *pszGame = PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME");
    const char *pszParent = PszCompatEnvironment("STARS_CLIENT_EXPORT_PARENT");
    const char *pszZip = PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP");
    const char *pszAction = PszCompatEnvironment("STARS_CLIENT_EXPORT_ACTION");
    const char *pszPlayer = PszCompatEnvironment("STARS_CLIENT_EXPORT_PLAYER");
    const char *pszResearch = PszCompatEnvironment("STARS_CLIENT_EXPORT_RESEARCH");
    int iPlayer = pszPlayer ? atoi(pszPlayer) : 0;
    int iResearch = pszResearch ? atoi(pszResearch) : 4;
    HWND hwndSave = HwndWorkflowDialog(app);
    char szDestination[512];
    int16_t fCancel = (pszAction && !strcmp(pszAction, "cancel")) || PszCompatEnvironment("STARS_CLIENT_EXPORT_ZIP_CANCEL") != NULL;
    int16_t fDiscard = pszAction && !strcmp(pszAction, "discard");
    int16_t fVerify = pszAction && !strcmp(pszAction, "verify");
    int16_t fFail = PszCompatEnvironment("STARS_CLIENT_EXPORT_EXPECTED") != NULL;
    int cSaves;

    if (hwndSave && hwndSave->lpfnDlg == DesktopChoiceIO().pfnAskSaveDialog) {
        if (idGameExportSaveDialog == StarsCompatWindowId(hwndSave)) return 1;
        idGameExportSaveDialog = StarsCompatWindowId(hwndSave);
        cGameExportSaves++;
        return FPushClick(app, win32::GetDlgItem(hwndSave, fDiscard ? IDC_NO_DON_T_SAVE : IDC_SAVE));
    }
    if (fGameExportBusy || fPassed) return 1;
    fGameExportBusy = 1;
    if (++cGameExportWait > 40) goto LFail;
    if (iStage == 0) {
        if (!win32::IsWindowVisible(app.lpui->hwndTitle) || (!pszParent && !pszZip)) goto LFail;
        if (fVerify) {
            iStage = 1;
            /* Verify this exported file through the real loader; Continue now
             * deliberately resumes the managed session instead of an MRU path. */
            CchSnprintf(app.state.szBase, sizeof(app.state.szBase), "%s", PszCompatEnvironment("STARS_CLIENT_CONTINUE"));
            app.state.ini.fStartupFile = 1;
            if (FOpenGame(app, app.lpui->hwndTitle, 0) <= 0) goto LFail;
            app.lpui->fFreeingTitle = 1;
            win32::DestroyWindow(app.lpui->hwndTitle);
            app.lpui->hwndTitle = 0;
            app.state.ini.fStartupFile = 0;
            win32::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
            InitializeMenu(app, NULL);
            win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
        } else if (!FSelectClientGameName(app.lpui->hwndTitle, pszGame)) goto LFail;
        else if (pszAction) {
            /* Choose the requested player through the same Open Game action. */
            if (!FSelectClientLibraryPlayer(app.lpui->hwndTitle, iPlayer)) goto LFail;
            iStage = 1;
            if (!FPushClick(app, app.lpui->rghwndBtnSplash[1])) goto LFail;
        } else {
            fGameExportSubmitSav = app.state.gd.fSubmit;
            iStage = 3;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
        }
    } else if (iStage == 1) {
        if (win32::IsWindow(app.lpui->hwndTitle)) goto LWait;
        if (!app.state.lpPlanets || app.state.idPlayer != iPlayer || !win32::IsWindow(app.lpui->hwndScanner)) goto LFail;
        if (fVerify) {
            if (app.state.rgplr[app.state.idPlayer].iTechNow() != iResearch) goto LFail;
            goto LDone;
        }
        if (app.state.rgplr[app.state.idPlayer].iTechNow() == iResearch) goto LFail;
        if (!FRunWorkflow(app, 1, iResearch) || !app.state.game.fDirty || app.state.rgplr[app.state.idPlayer].iTechNow() != iResearch) goto LFail;
        if (!strcmp(pszAction, "submit")) {
            win32::SendMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_TURN_SAVE_SUBMIT, 0);
            if (app.state.game.fDirty || !app.state.gd.fSubmit) goto LFail;
        }
        fGameExportSubmitSav = app.state.gd.fSubmit;
        /* Exercise the selected-loaded-game save path without discarding its
         * orders, as returning via File/Open normally tears that state down. */
        ShowTitleScreen(app);
        iStage = 2;
    } else if (iStage == 2) {
        if (!FSelectClientGameName(app.lpui->hwndTitle, pszGame) ||
            !FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAME_EXPORT))) goto LFail;
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
        if (cGameExportSaves != cSaves || app.state.gd.fSubmit != fGameExportSubmitSav) goto LFail;
        if (pszAction && (app.state.game.fDirty != (fCancel || fDiscard))) goto LFail;
        goto LDone;
    }
LWait:
    fGameExportBusy = 0;
    return 1;
LDone:
    fChecked = fPassed = 1;
    fGameExportBusy = 0;
    puts("Managed game export: button, destination callback, save choice and real files checked");
    win32::KillTimer(NULL, idTimer);
    win32::PostQuitMessage(0);
    return 1;
LFail:
    fprintf(stderr, "Game export failed: stage %d pickers %d saves %d alerts %d dirty %d\n",
            iStage, cGameExportPickers, cGameExportSaves, cAlerts, app.state.game.fDirty);
    fGameExportBusy = 0;
    return 0;
}
