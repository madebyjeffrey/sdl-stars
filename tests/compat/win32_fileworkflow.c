/* Real game/race I/O through the native portal on a private test bus. */
#include "win.h"
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int16_t __real_InitInstance(int16_t nCmdShow);
static BOOL fPassed, fRunning;
static UINT_PTR idTimer;
static int cAlerts;
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "file workflow line %d: %s\n", __LINE__, #f); goto done; } } while (0)
int16_t __wrap_PromptPassword(void) { return TRUE; }
int16_t __wrap_AlertSz(char *sz, int16_t mbType) {
    (void)mbType; fprintf(stderr, "File workflow alert: %s\n", sz); cAlerts++;
    return IDOK;
}
static void CALLBACK FileWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND hwndParentSav;
    PLAYER plrSav;
    char szRaceSav[sizeof(szRaceFile)];
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
    CHECK(game.lid == lid && memcmp(&plrSav, &rgplr[idPlayer], sizeof(plrSav)) == 0);
    hwndRaceParent = hwndParentSav;
    fPassed = cAlerts == 0;
done:
    PostQuitMessage(fPassed ? 0 : 92);
}
int16_t __wrap_InitInstance(int16_t nCmdShow) {
    int16_t fResult = __real_InitInstance(nCmdShow);
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
