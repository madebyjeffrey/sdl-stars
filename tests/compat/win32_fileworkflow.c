/* Real game/race I/O through the native portal on a private test bus. */
#include "win.h"
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int16_t StarsTestReal_InitInstance(int16_t nCmdShow);
static BOOL fPassed, fRunning;
static UINT_PTR idTimer;
static int cAlerts;
#ifdef STARS_TEST_FILE_WORKFLOW_COMPAT_CHOOSER
static int cOpenChoices, cSaveChoices;
BOOL StarsTest_FNativeOpenFileName(OPENFILENAMEA *lpof) {
    const char *pszPath;
    cOpenChoices++;
    if (cOpenChoices == 1) return FALSE;
    pszPath = getenv(cOpenChoices == 3 ? "STARS_TEST_RACE_PATH" : "STARS_TEST_GAME_PATH");
    if (!pszPath || strlen(pszPath) >= lpof->nMaxFile) return FALSE;
    strcpy(lpof->lpstrFile, pszPath);
    lpof->nFileOffset = (WORD)(strrchr(pszPath, '/') ? strrchr(pszPath, '/') - pszPath + 1 : 0);
    lpof->nFileExtension = (WORD)(strrchr(pszPath, '.') ? strrchr(pszPath, '.') - pszPath + 1 : 0);
    return TRUE;
}
BOOL StarsTest_FNativeSaveFileName(OPENFILENAMEA *lpof) {
    const char *pszPath;
    if (++cSaveChoices == 1 || cSaveChoices == 4) return FALSE;
    pszPath = getenv(cSaveChoices == 2 ? "STARS_TEST_RACE_PATH" :
                     cSaveChoices == 3 ? "STARS_TEST_RACE_REPEAT_PATH" : "STARS_TEST_EXPORT_CHOICE");
    if (cSaveChoices >= 4 && strcmp(lpof->lpstrFile, "café-Ω.x1") != 0) {
        fprintf(stderr, "orders chooser suggested unexpected name: %s\n", lpof->lpstrFile);
        return FALSE;
    }
    if (!pszPath || strlen(pszPath) >= lpof->nMaxFile) return FALSE;
    strcpy(lpof->lpstrFile, pszPath);
    lpof->nFileOffset = (WORD)(strrchr(pszPath, '/') ? strrchr(pszPath, '/') - pszPath + 1 : 0);
    return TRUE;
}
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "file workflow line %d: %s\n", __LINE__, #f); goto done; } } while (0)
static BOOL FEqualFiles(const char *pszLeft, const char *pszRight) {
    FILE *pfLeft = fopen(pszLeft, "rb"), *pfRight = fopen(pszRight, "rb");
    unsigned char rgbLeft[4096], rgbRight[4096];
    size_t cbLeft, cbRight;
    BOOL fEqual = pfLeft && pfRight;
    if (!fEqual) goto LDone;
    do {
        cbLeft = fread(rgbLeft, 1, sizeof(rgbLeft), pfLeft);
        cbRight = fread(rgbRight, 1, sizeof(rgbRight), pfRight);
        if (cbLeft != cbRight || memcmp(rgbLeft, rgbRight, cbLeft)) { fEqual = FALSE; break; }
    } while (cbLeft != 0);
LDone:
    if (pfLeft) fclose(pfLeft);
    if (pfRight) fclose(pfRight);
    return fEqual;
}
int16_t StarsTest_PromptPassword(void) { return TRUE; }
int16_t StarsTest_AlertSz(char *sz, int16_t mbType) {
    (void)mbType; fprintf(stderr, "File workflow alert: %s\n", sz); cAlerts++;
    return IDOK;
}
static void CALLBACK FileWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND hwndParentSav;
    PLAYER plrSav;
    PLAYER plrManaged;
    char szRaceSav[sizeof(szRaceFile)];
    char szManagedRace[512];
    char szRaceLibrary[256];
    const char *pszExport = getenv("STARS_TEST_EXPORT_EXPECTED");
    const char *pszManagedOrders = getenv("STARS_TEST_MANAGED_ORDERS");
    const char *pszManagedTurn = getenv("STARS_TEST_MANAGED_TURN");
    const char *pszExportFailure = getenv("STARS_TEST_EXPORT_FAILURE_TARGET");
    int64_t cbRaceLibrary;
    int64_t cbExport;
    int16_t fSubmitSav;
    const char *pszRace = getenv("STARS_TEST_RACE_PATH");
    const char *pszRaceRepeat = getenv("STARS_TEST_RACE_REPEAT_PATH");
    int32_t lid;
    (void)hwnd; (void)msg; (void)dwTime;
    if (fRunning) return;
    fRunning = TRUE; KillTimer(NULL, id);
    CHECK(pszRace && pszRaceRepeat && IsWindow(hwndTitle));
    CHECK(FOpenGame(hwndTitle, FALSE) == 0 && IsWindowVisible(hwndTitle) && IsWindowEnabled(hwndTitle));
    CHECK(!(SDL_GetWindowFlags(hwndTitle->lpWindow) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_NOT_FOCUSABLE)));
    CHECK(FOpenGame(hwndTitle, FALSE) > 0 && game.lid != 0 && lpPlanets && idPlayer != iplrNone);
    lid = game.lid; plrSav = rgplr[idPlayer]; memcpy(szRaceSav, szRaceFile, sizeof(szRaceSav));
    hwndParentSav = hwndRaceParent; hwndRaceParent = hwndTitle;
    CHECK(!FSaveRace(NULL, &plrSav) && memcmp(szRaceSav, szRaceFile, sizeof(szRaceSav)) == 0);
    CHECK(FSaveRace(NULL, &plrSav));
    CHECK(szRaceFile[sizeof(szRaceFile) - 1] == 0 &&
          strncmp(szRaceFile, strrchr(pszRace, '/') + 1, strlen(szRaceFile)) == 0);
    CHECK(FWasRaceFile((char *)pszRace, FALSE) > 0);
    CHECK(FOpenGame(hwndTitle, TRUE) > 0 && szRaceFile[sizeof(szRaceFile) - 1] == 0);
    CHECK(FSaveRace(szRaceFile, &plrSav) && FWasRaceFile((char *)pszRaceRepeat, FALSE) > 0);
    CHECK(IRaceChecksum(&vplr) == IRaceChecksum(&plrSav));
    CHECK(FSaveLibraryRace(hwndTitle, "Managed.r1", &plrSav));
    CHECK(CchSnprintf(szRaceLibrary, sizeof(szRaceLibrary), "%s%sRace Designs", getenv("STARS_SETTINGS_DIR"), szDirSep) > 0);
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
    CHECK(FExportLibraryOrders(hwndTitle, NULL));
    CHECK(gd.fSubmit == fSubmitSav && game.lid == lid);
    CHECK(FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0);
    CHECK(FEqualFiles(pszExport, pszManagedOrders));
    CHECK(!FExportLibraryOrders(hwndTitle, pszExportFailure));
    CHECK(gd.fSubmit == fSubmitSav && game.lid == lid &&
          FNativeFileSize((char *)pszExport, &cbExport) && cbExport > 0 &&
          FNativeFileSize((char *)pszManagedOrders, &cbExport) && cbExport > 0);
    CHECK(game.lid == lid && memcmp(&plrSav, &rgplr[idPlayer], sizeof(plrSav)) == 0);
    hwndRaceParent = hwndParentSav;
    fPassed = cAlerts == 1;
done:
    PostQuitMessage(fPassed ? 0 : 92);
}
int16_t StarsTest_InitInstance(int16_t nCmdShow) {
    int16_t fResult = StarsTestReal_InitInstance(nCmdShow);
    if (fResult) idTimer = SetTimer(NULL, 0, 50, FileWorkflow);
    return fResult && idTimer ? fResult : FALSE;
}
int main(void) {
    char szCommand[] = "";
    int nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (idTimer) KillTimer(NULL, idTimer);
    StarsCompatProcessExit(); SDL_Quit();
    return nResult == 0 && fPassed ? 0 : 1;
}
