/* Real game/race I/O through the native portal or a portable chooser adapter. */
#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "../../src/core/alert_io.h"
#include "../../src/core/player_choice_io.h"
#include "../../src/ui/desktop_choice_io.h"
#include "../../compat/win32/src/choice_io.h"

#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BOOL fPassed, fRunning;
static UINT_PTR idTimer;
static int cAlerts, cReplaceChoices;
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "file workflow line %d: %s\n", __LINE__, #f); goto done; } } while (0)
static int cFileChoices;

static const char *PszWorkflowFilename(const char *pszFile) {
    const char *pch = pszFile + strlen(pszFile);
    while (pch > pszFile && pch[-1] != '/' && pch[-1] != '\\') pch--;
    return pch;
}

static BOOL FChooseWorkflowFile(OPENFILENAME *lpof, BOOL fSave) {
    const char *pszFile, *pchExtension;
    cFileChoices++;
    if ((fSave && cFileChoices != 3 && cFileChoices != 4 && cFileChoices != 6 &&
                  cFileChoices != 7 && cFileChoices != 8 && cFileChoices != 9) ||
        (!fSave && cFileChoices != 1 && cFileChoices != 2 && cFileChoices != 5)) return 0;
    if (cFileChoices >= 7 && strcmp(lpof->lpstrFile, "café-Ω.x1")) return 0;
    if (cFileChoices == 1 || cFileChoices == 3 || cFileChoices == 7) return 0;
    pszFile = PszCompatEnvironment(cFileChoices == 2 ? "STARS_TEST_GAME_PATH" :
                     cFileChoices == 6 ? "STARS_TEST_RACE_REPEAT_PATH" :
                     cFileChoices >= 8 ? "STARS_TEST_EXPORT_CHOICE" : "STARS_TEST_RACE_PATH");
    if (!pszFile || strlen(pszFile) >= lpof->nMaxFile) return 0;
    strcpy(lpof->lpstrFile, pszFile);
    lpof->nFileOffset = (WORD)(PszWorkflowFilename(pszFile) - pszFile);
    pchExtension = strrchr(PszWorkflowFilename(pszFile), '.');
    lpof->nFileExtension = pchExtension ? (WORD)(pchExtension - pszFile + 1) : 0;
    return 1;
}

BOOL StarsTest_FNativeOpenFileName(OPENFILENAME *lpof) {
    return PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") ? FChooseWorkflowFile(lpof, 0) : FNativeOpenFileName(lpof);
}

BOOL StarsTest_FNativeSaveFileName(OPENFILENAME *lpof) {
    return PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") ? FChooseWorkflowFile(lpof, 1) : FNativeSaveFileName(lpof);
}

static BOOL FEqualFiles(const char *pszLeft, const char *pszRight) {
    NATIVEFILE *pfLeft = PfOpenNativeFile(pszLeft, NATIVEFILEMODE::read), *pfRight = PfOpenNativeFile(pszRight, NATIVEFILEMODE::read);
    unsigned char rgbLeft[4096], rgbRight[4096];
    size_t cbLeft, cbRight;
    BOOL fEqual = pfLeft && pfRight;
    if (!fEqual) goto LDone;
    do {
        cbLeft = CReadNative(std::span{rgbLeft}, pfLeft);
        cbRight = CReadNative(std::span{rgbRight}, pfRight);
        if (cbLeft != cbRight || memcmp(rgbLeft, rgbRight, cbLeft)) { fEqual = 0; break; }
    } while (cbLeft != 0);
LDone:
    if (pfLeft) ICloseNativeFile(pfLeft);
    if (pfRight) ICloseNativeFile(pfRight);
    return fEqual;
}
static BOOL FCopyFile(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource = PfOpenNativeFile(pszSource, NATIVEFILEMODE::read), *pfTarget = PfOpenNativeFile(pszTarget, NATIVEFILEMODE::writeTruncate);
    unsigned char rgb[4096];
    size_t cb;
    BOOL fCopied = pfSource && pfTarget;
    if (!fCopied) goto LDone;
    while ((cb = CReadNative(std::span{rgb}, pfSource)) != 0) {
        if (CWriteNative(std::span{rgb}.first(cb), pfTarget) != cb) { fCopied = 0; break; }
    }
    if (FNativeStreamError(pfSource)) fCopied = 0;
LDone:
    if (pfSource) ICloseNativeFile(pfSource);
    if (pfTarget && ICloseNativeFile(pfTarget) != 0) fCopied = 0;
    return fCopied;
}
int StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    const char *pszExport = PszCompatEnvironment("STARS_TEST_EXPORT_EXPECTED");
    if ((type & win32::MB_YESNO) && pszExport && strstr(pszText, pszExport)) {
        if (!(type & win32::MB_DEFBUTTON2)) return win32::IDNO;
        return ++cReplaceChoices == 1 ? win32::IDNO : win32::IDYES;
    }
    return win32::MessageBoxA(hwnd, pszText, pszCaption, type);
}
int16_t StarsTest_PromptPassword(APPSESSION &) { return 1; }
int16_t StarsTest_AlertSz(APPSESSION &, const char *sz, int16_t mbType) {
    (void)mbType; fprintf(stderr, "File workflow alert: %s\n", sz); cAlerts++;
    return IDOK;
}
static void  FileWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND hwndDialog;
    HWND hwndParentSav;
    PLAYER plrSav;
    PLAYER plrManaged;
    char szRaceSav[sizeof(app.state.szRaceFile)];
    char szManagedRace[512];
    char szImportedRace[512];
    char szRaceOriginal[512];
    char szRaceLibrary[256];
    const char *pszExport = PszCompatEnvironment("STARS_TEST_EXPORT_EXPECTED");
    const char *pszPreviousExport = PszCompatEnvironment("STARS_TEST_EXPORT_PREVIOUS");
    const char *pszManagedOrders = PszCompatEnvironment("STARS_TEST_MANAGED_ORDERS");
    const char *pszManagedTurn = PszCompatEnvironment("STARS_TEST_MANAGED_TURN");
    const char *pszExportFailure = PszCompatEnvironment("STARS_TEST_EXPORT_FAILURE_TARGET");
    int64_t cbRaceLibrary;
    int64_t cbExport;
    int16_t fSubmitSav;
    const char *pszRace = PszCompatEnvironment("STARS_TEST_RACE_PATH");
    const char *pszRaceRepeat = PszCompatEnvironment("STARS_TEST_RACE_REPEAT_PATH");
    int32_t lid;
    (void)hwnd; (void)msg; (void)dwTime;
    if (fRunning) {
        for (hwndDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
            if (hwndDialog->fDialog && hwndDialog->lpfnDlg == LibraryNameDlg && win32::IsWindowVisible(hwndDialog)) {
                ui::SendMessage(hwndDialog, win32::WM_COMMAND, IDOK, 0);
                return;
            }
        }
        return;
    }
    fRunning = 1;
    CHECK(pszRace && pszRaceRepeat && win32::IsWindow(app.lpui->hwndTitle));
    CHECK(FOpenGame(app, app.lpui->hwndTitle, 0) == 0 && win32::IsWindowVisible(app.lpui->hwndTitle) && win32::IsWindowEnabled(app.lpui->hwndTitle));
    CHECK(!(SDL_GetWindowFlags(app.lpui->hwndTitle->lpWindow.get()) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_NOT_FOCUSABLE)));
    CHECK(FOpenGame(app, app.lpui->hwndTitle, 0) > 0 && app.state.game.lid != 0 && app.state.lpPlanets && app.state.idPlayer != iplrNone);
    lid = app.state.game.lid; plrSav = app.state.rgplr[app.state.idPlayer]; memcpy(szRaceSav, app.state.szRaceFile, sizeof(szRaceSav));
    hwndParentSav = app.lpui->hwndRaceParent; app.lpui->hwndRaceParent = app.lpui->hwndTitle;
    CHECK(!FSaveRace(app, NULL, &plrSav) && memcmp(szRaceSav, app.state.szRaceFile, sizeof(szRaceSav)) == 0);
    CHECK(FSaveRace(app, NULL, &plrSav));
    CHECK(app.state.szRaceFile[sizeof(app.state.szRaceFile) - 1] == 0 &&
          strncmp(app.state.szRaceFile, PszWorkflowFilename(pszRace), strlen(app.state.szRaceFile)) == 0);
    CHECK(FWasRaceFile(app, (char *)pszRace, 0) > 0);
    CHECK(CchSnprintf(std::span{szRaceOriginal, sizeof(szRaceOriginal)}, "%s.before-import", pszRace) > 0 &&
          FCopyFile(pszRace, szRaceOriginal));
    CHECK(FOpenGame(app, app.lpui->hwndTitle, 1) > 0 && app.state.szRaceFile[sizeof(app.state.szRaceFile) - 1] == 0);
    CHECK(CchSnprintf(std::span{szImportedRace, sizeof(szImportedRace)}, "%s%sRace Designs%srace-long-ΩΩΩΩ-native-filename.r1",
                      PszCompatEnvironment("STARS_SETTINGS_DIR"), szDirSep, szDirSep) > 0);
    CHECK(FWasRaceFile(app, szImportedRace, 0) > 0 && FEqualFiles(pszRace, szImportedRace) &&
          FEqualFiles(pszRace, szRaceOriginal));
    CHECK(FSaveRace(app, app.state.szRaceFile, &plrSav) && FWasRaceFile(app, (char *)pszRaceRepeat, 0) > 0);
    CHECK(IRaceChecksum(&app.state.vplr) == IRaceChecksum(&plrSav));
    CHECK(FSaveLibraryRace(app, app.lpui->hwndTitle, "Managed.r1", &plrSav));
    CHECK(CchSnprintf(std::span{szRaceLibrary, sizeof(szRaceLibrary)}, "%s%sRace Designs", PszCompatEnvironment("STARS_SETTINGS_DIR"), szDirSep) > 0);
    CHECK(CchSnprintf(std::span{szManagedRace, sizeof(szManagedRace)}, "%s%sManaged.r1", szRaceLibrary, szDirSep) > 0);
    CHECK(FWasRaceFile(app, szManagedRace, 0) > 0);
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0);
    strcpy(app.state.szPassLast, "managed-pass");
    app.state.lSaltLast = LSaltFromSz(app.state.szPassLast);
    app.state.lSaltCur = app.state.lSaltLast;
    plrManaged = plrSav;
    plrManaged.lSalt = app.state.lSaltLast;
    CHECK(FSaveLibraryRace(app, app.lpui->hwndTitle, "Managed.r1", &plrManaged));
    CHECK(FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1"));
    CHECK(app.state.vplr.lSalt == app.state.lSaltLast && IRaceChecksum(&app.state.vplr) == IRaceChecksum(&plrManaged));
    CHECK(!FSaveLibraryRace(app, app.lpui->hwndTitle, "bad/name.r1", &plrManaged));
    CHECK(FWasRaceFile(app, szManagedRace, 0) > 0 && app.state.vplr.lSalt == app.state.lSaltLast);
    CHECK(FImportLibraryRaceAs(app, app.lpui->hwndTitle, pszRace, "Imported.r2"));
    CHECK(CchSnprintf(std::span{szManagedRace, sizeof(szManagedRace)}, "%s%sImported.r2", szRaceLibrary, szDirSep) > 0);
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0 && FEqualFiles(pszRace, szManagedRace));
    CHECK(!FImportLibraryRaceAs(app, app.lpui->hwndTitle, pszRace, "imported.R2"));
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0);
    CHECK(FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1"));
    CHECK(IRaceChecksum(&app.state.vplr) == IRaceChecksum(&plrManaged));
    fSubmitSav = app.state.gd.fSubmit;
    CHECK(pszExport && pszManagedOrders && pszManagedTurn && pszExportFailure && !FExportLibraryOrders(app, app.lpui->hwndTitle, NULL));
    CHECK(app.state.gd.fSubmit == fSubmitSav && app.state.game.lid == lid);
    CHECK(!FNativeFileSize((char *)pszManagedOrders, &cbExport) && FNativeFileSize((char *)pszManagedTurn, &cbExport));
    DirtyGame(app, 1);
    CHECK(pszPreviousExport && !FExportLibraryOrders(app, app.lpui->hwndTitle, NULL));
    CHECK(cReplaceChoices == 1 && app.state.game.fDirty && app.state.gd.fSubmit == fSubmitSav && app.state.game.lid == lid);
    CHECK(FEqualFiles(pszExport, pszPreviousExport));
    CHECK(!FNativeFileSize((char *)pszManagedOrders, &cbExport));
    CHECK(FExportLibraryOrders(app, app.lpui->hwndTitle, NULL));
    CHECK(cReplaceChoices == 2 && !app.state.game.fDirty);
    CHECK(app.state.gd.fSubmit == fSubmitSav && app.state.game.lid == lid);
    CHECK(FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0);
    CHECK(FEqualFiles(pszExport, pszManagedOrders));
    CHECK(!FExportLibraryOrders(app, app.lpui->hwndTitle, pszExportFailure));
    CHECK(app.state.gd.fSubmit == fSubmitSav && app.state.game.lid == lid &&
          FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0 &&
          FNativeFileSize((char *)pszManagedOrders, &cbExport) && cbExport > 0);
    CHECK(app.state.game.lid == lid && memcmp(&plrSav, &app.state.rgplr[app.state.idPlayer], sizeof(plrSav)) == 0);
    app.lpui->hwndRaceParent = hwndParentSav;
    fPassed = cAlerts == 1 && (!PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") || cFileChoices == 9);
done:
    win32::KillTimer(NULL, id);
    win32::PostQuitMessage(fPassed ? 0 : 92);
}
int16_t StarsTest_InitInstance(APPSESSION &app, int16_t nCmdShow) {
    int16_t fResult = InitInstance(app, nCmdShow);
    if (fResult) idTimer = win32::SetTimer(NULL, 0, 50, FileWorkflow);
    return fResult && idTimer ? fResult : 0;
}
int main(void) {
    char szCommand[] = "";
    int nResult;
    ALERTIO io_alertio;
    io_alertio.pfnAlertSz = StarsTest_AlertSz;
    ALERTIOSCOPE scope_alertio(io_alertio);
    PLAYERCHOICEIO io_playerchoiceio;
    io_playerchoiceio.pfnPromptPassword = StarsTest_PromptPassword;
    PLAYERCHOICEIOSCOPE scope_playerchoiceio(io_playerchoiceio);
    DESKTOPCHOICEIO io_desktopchoiceio;
    io_desktopchoiceio.pfnInitInstance = StarsTest_InitInstance;
    io_desktopchoiceio.pfnFNativeSaveFileName = StarsTest_FNativeSaveFileName;
    io_desktopchoiceio.pfnFNativeOpenFileName = StarsTest_FNativeOpenFileName;
    DESKTOPCHOICEIOSCOPE scope_desktopchoiceio(io_desktopchoiceio);
    COMPATCHOICEIO io_compatchoiceio;
    io_compatchoiceio.pfnMessageBoxA = StarsTest_MessageBoxA;
    COMPATCHOICEIOSCOPE scope_compatchoiceio(io_compatchoiceio);

    nResult = ui::WinMain(win32::GetModuleHandleA(NULL), NULL, szCommand, win32::SW_SHOWNORMAL);
    if (idTimer) win32::KillTimer(NULL, idTimer);
    StarsCompatProcessExit(); SDL_Quit();
    return nResult == 0 && fPassed ? 0 : 1;
}
