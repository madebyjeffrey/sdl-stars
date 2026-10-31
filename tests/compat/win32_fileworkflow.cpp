/* Real game/race I/O through the native portal or a portable chooser adapter. */
#include "win.h"
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
        (!fSave && cFileChoices != 1 && cFileChoices != 2 && cFileChoices != 5)) return FALSE;
    if (cFileChoices >= 7 && strcmp(lpof->lpstrFile, "café-Ω.x1")) return FALSE;
    if (cFileChoices == 1 || cFileChoices == 3 || cFileChoices == 7) return FALSE;
    pszFile = PszCompatEnvironment(cFileChoices == 2 ? "STARS_TEST_GAME_PATH" :
                     cFileChoices == 6 ? "STARS_TEST_RACE_REPEAT_PATH" :
                     cFileChoices >= 8 ? "STARS_TEST_EXPORT_CHOICE" : "STARS_TEST_RACE_PATH");
    if (!pszFile || strlen(pszFile) >= lpof->nMaxFile) return FALSE;
    strcpy(lpof->lpstrFile, pszFile);
    lpof->nFileOffset = (WORD)(PszWorkflowFilename(pszFile) - pszFile);
    pchExtension = strrchr(PszWorkflowFilename(pszFile), '.');
    lpof->nFileExtension = pchExtension ? (WORD)(pchExtension - pszFile + 1) : 0;
    return TRUE;
}

BOOL StarsTest_FNativeOpenFileName(OPENFILENAME *lpof) {
    return PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") ? FChooseWorkflowFile(lpof, FALSE) : FNativeOpenFileName(lpof);
}

BOOL StarsTest_FNativeSaveFileName(OPENFILENAME *lpof) {
    return PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") ? FChooseWorkflowFile(lpof, TRUE) : FNativeSaveFileName(lpof);
}

static BOOL FEqualFiles(const char *pszLeft, const char *pszRight) {
    NATIVEFILE *pfLeft = PfOpenNativeFile(pszLeft, "rb"), *pfRight = PfOpenNativeFile(pszRight, "rb");
    unsigned char rgbLeft[4096], rgbRight[4096];
    size_t cbLeft, cbRight;
    BOOL fEqual = pfLeft && pfRight;
    if (!fEqual) goto LDone;
    do {
        cbLeft = CReadNative(rgbLeft, 1, sizeof(rgbLeft), pfLeft);
        cbRight = CReadNative(rgbRight, 1, sizeof(rgbRight), pfRight);
        if (cbLeft != cbRight || memcmp(rgbLeft, rgbRight, cbLeft)) { fEqual = FALSE; break; }
    } while (cbLeft != 0);
LDone:
    if (pfLeft) ICloseNativeFile(pfLeft);
    if (pfRight) ICloseNativeFile(pfRight);
    return fEqual;
}
static BOOL FCopyFile(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource = PfOpenNativeFile(pszSource, "rb"), *pfTarget = PfOpenNativeFile(pszTarget, "wb");
    unsigned char rgb[4096];
    size_t cb;
    BOOL fCopied = pfSource && pfTarget;
    if (!fCopied) goto LDone;
    while ((cb = CReadNative(rgb, 1, sizeof(rgb), pfSource)) != 0) {
        if (CWriteNative(rgb, 1, cb, pfTarget) != cb) { fCopied = FALSE; break; }
    }
    if (FNativeStreamError(pfSource)) fCopied = FALSE;
LDone:
    if (pfSource) ICloseNativeFile(pfSource);
    if (pfTarget && ICloseNativeFile(pfTarget) != 0) fCopied = FALSE;
    return fCopied;
}
int WINAPI StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    const char *pszExport = PszCompatEnvironment("STARS_TEST_EXPORT_EXPECTED");
    if ((type & MB_YESNO) && pszExport && strstr(pszText, pszExport)) {
        if (!(type & MB_DEFBUTTON2)) return IDNO;
        return ++cReplaceChoices == 1 ? IDNO : IDYES;
    }
    return win32::MessageBoxA(hwnd, pszText, pszCaption, type);
}
int16_t StarsTest_PromptPassword(void) { return TRUE; }
int16_t StarsTest_AlertSz(const char *sz, int16_t mbType) {
    (void)mbType; fprintf(stderr, "File workflow alert: %s\n", sz); cAlerts++;
    return IDOK;
}
static void CALLBACK FileWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND hwndDialog;
    HWND hwndParentSav;
    PLAYER plrSav;
    PLAYER plrManaged;
    char szRaceSav[sizeof(szRaceFile)];
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
        for (hwndDialog = win32::GetWindow(hwndTitle, GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, GW_HWNDNEXT)) {
            if (hwndDialog->fDialog && hwndDialog->lpfnDlg == LibraryNameDlg && win32::IsWindowVisible(hwndDialog)) {
                ui::SendMessage(hwndDialog, WM_COMMAND, IDOK, 0);
                return;
            }
        }
        return;
    }
    fRunning = TRUE;
    CHECK(pszRace && pszRaceRepeat && win32::IsWindow(hwndTitle));
    CHECK(FOpenGame(hwndTitle, FALSE) == 0 && win32::IsWindowVisible(hwndTitle) && win32::IsWindowEnabled(hwndTitle));
    CHECK(!(SDL_GetWindowFlags(hwndTitle->lpWindow.get()) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_NOT_FOCUSABLE)));
    CHECK(FOpenGame(hwndTitle, FALSE) > 0 && game.lid != 0 && lpPlanets && idPlayer != iplrNone);
    lid = game.lid; plrSav = rgplr[idPlayer]; memcpy(szRaceSav, szRaceFile, sizeof(szRaceSav));
    hwndParentSav = hwndRaceParent; hwndRaceParent = hwndTitle;
    CHECK(!FSaveRace(NULL, &plrSav) && memcmp(szRaceSav, szRaceFile, sizeof(szRaceSav)) == 0);
    CHECK(FSaveRace(NULL, &plrSav));
    CHECK(szRaceFile[sizeof(szRaceFile) - 1] == 0 &&
          strncmp(szRaceFile, PszWorkflowFilename(pszRace), strlen(szRaceFile)) == 0);
    CHECK(FWasRaceFile((char *)pszRace, FALSE) > 0);
    CHECK(CchSnprintf(szRaceOriginal, sizeof(szRaceOriginal), "%s.before-import", pszRace) > 0 &&
          FCopyFile(pszRace, szRaceOriginal));
    CHECK(FOpenGame(hwndTitle, TRUE) > 0 && szRaceFile[sizeof(szRaceFile) - 1] == 0);
    CHECK(CchSnprintf(szImportedRace, sizeof(szImportedRace), "%s%sRace Designs%srace-long-ΩΩΩΩ-native-filename.r1",
                      PszCompatEnvironment("STARS_SETTINGS_DIR"), szDirSep, szDirSep) > 0);
    CHECK(FWasRaceFile(szImportedRace, FALSE) > 0 && FEqualFiles(pszRace, szImportedRace) &&
          FEqualFiles(pszRace, szRaceOriginal));
    CHECK(FSaveRace(szRaceFile, &plrSav) && FWasRaceFile((char *)pszRaceRepeat, FALSE) > 0);
    CHECK(IRaceChecksum(&vplr) == IRaceChecksum(&plrSav));
    CHECK(FSaveLibraryRace(hwndTitle, "Managed.r1", &plrSav));
    CHECK(CchSnprintf(szRaceLibrary, sizeof(szRaceLibrary), "%s%sRace Designs", PszCompatEnvironment("STARS_SETTINGS_DIR"), szDirSep) > 0);
    CHECK(CchSnprintf(szManagedRace, sizeof(szManagedRace), "%s%sManaged.r1", szRaceLibrary, szDirSep) > 0);
    CHECK(FWasRaceFile(szManagedRace, FALSE) > 0);
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0);
    strcpy(szPassLast, "managed-pass");
    lSaltLast = LSaltFromSz(szPassLast);
    lSaltCur = lSaltLast;
    plrManaged = plrSav;
    plrManaged.lSalt = lSaltLast;
    CHECK(FSaveLibraryRace(hwndTitle, "Managed.r1", &plrManaged));
    CHECK(FLoadLibraryRace(hwndTitle, "Managed.r1"));
    CHECK(vplr.lSalt == lSaltLast && IRaceChecksum(&vplr) == IRaceChecksum(&plrManaged));
    CHECK(!FSaveLibraryRace(hwndTitle, "bad/name.r1", &plrManaged));
    CHECK(FWasRaceFile(szManagedRace, FALSE) > 0 && vplr.lSalt == lSaltLast);
    CHECK(FImportLibraryRaceAs(hwndTitle, pszRace, "Imported.r2"));
    CHECK(CchSnprintf(szManagedRace, sizeof(szManagedRace), "%s%sImported.r2", szRaceLibrary, szDirSep) > 0);
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0 && FEqualFiles(pszRace, szManagedRace));
    CHECK(!FImportLibraryRaceAs(hwndTitle, pszRace, "imported.R2"));
    CHECK(FNativeFileSize(szManagedRace, &cbRaceLibrary) && cbRaceLibrary > 0);
    CHECK(FLoadLibraryRace(hwndTitle, "Managed.r1"));
    CHECK(IRaceChecksum(&vplr) == IRaceChecksum(&plrManaged));
    fSubmitSav = gd.fSubmit;
    CHECK(pszExport && pszManagedOrders && pszManagedTurn && pszExportFailure && !FExportLibraryOrders(hwndTitle, NULL));
    CHECK(gd.fSubmit == fSubmitSav && game.lid == lid);
    CHECK(!FNativeFileSize((char *)pszManagedOrders, &cbExport) && FNativeFileSize((char *)pszManagedTurn, &cbExport));
    DirtyGame(TRUE);
    CHECK(pszPreviousExport && !FExportLibraryOrders(hwndTitle, NULL));
    CHECK(cReplaceChoices == 1 && game.fDirty && gd.fSubmit == fSubmitSav && game.lid == lid);
    CHECK(FEqualFiles(pszExport, pszPreviousExport));
    CHECK(!FNativeFileSize((char *)pszManagedOrders, &cbExport));
    CHECK(FExportLibraryOrders(hwndTitle, NULL));
    CHECK(cReplaceChoices == 2 && !game.fDirty);
    CHECK(gd.fSubmit == fSubmitSav && game.lid == lid);
    CHECK(FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0);
    CHECK(FEqualFiles(pszExport, pszManagedOrders));
    CHECK(!FExportLibraryOrders(hwndTitle, pszExportFailure));
    CHECK(gd.fSubmit == fSubmitSav && game.lid == lid &&
          FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0 &&
          FNativeFileSize((char *)pszManagedOrders, &cbExport) && cbExport > 0);
    CHECK(game.lid == lid && memcmp(&plrSav, &rgplr[idPlayer], sizeof(plrSav)) == 0);
    hwndRaceParent = hwndParentSav;
    fPassed = cAlerts == 1 && (!PszCompatEnvironment("STARS_TEST_FILE_WORKFLOW_COMPAT") || cFileChoices == 9);
done:
    win32::KillTimer(NULL, id);
    win32::PostQuitMessage(fPassed ? 0 : 92);
}
int16_t StarsTest_InitInstance(int16_t nCmdShow) {
    int16_t fResult = InitInstance(nCmdShow);
    if (fResult) idTimer = win32::SetTimer(NULL, 0, 50, FileWorkflow);
    return fResult && idTimer ? fResult : FALSE;
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

    nResult = ui::WinMain(win32::GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (idTimer) win32::KillTimer(NULL, idTimer);
    StarsCompatProcessExit(); SDL_Quit();
    return nResult == 0 && fPassed ? 0 : 1;
}
