/* Exercise the actual map dialog and printing command on a copied game. */
#include "win.h"
#include <SDL3/SDL.h>
#include <stars_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int16_t StarsTestReal_InitInstance(int16_t);
static BOOL fRunning, fPassed, fCancel;
static int cLaunch, cAlert;
static UINT_PTR idDriver, idDialog;
static int cPageX = 1, cPageY = 1;
static const char *pszTarget;
static char szInstalledRace[] = "installed.r1";

int16_t StarsTest_PromptPassword(void) { return TRUE; }
int16_t StarsTest_AlertSz(char *sz, int16_t mb) { (void)mb; fprintf(stderr, "Print alert: %s\n", sz); cAlert++; return IDOK; }
extern "C" bool SDLCALL StarsTest_SDL_OpenURL(const char *pszUri) {
    FILE *lpFile;
    char rgb[5];
    (void)pszUri;
    cLaunch++;
    lpFile = fopen(pszTarget, "rb");
    if (!lpFile) { cAlert++; return false; }
    if (fread(rgb, 1, sizeof(rgb), lpFile) != sizeof(rgb) || memcmp(rgb, "%PDF-", sizeof(rgb))) cAlert++;
    fclose(lpFile);
    return true;
}
static void CALLBACK ChoosePages(HWND hwnd, UINT msg, UINT_PTR id, DWORD ticks) {
    HWND hwndDialog, hwndEdit;
    char sz[2];
    (void)hwnd; (void)msg; (void)id; (void)ticks;
    for (hwndDialog = GetWindow(hwndFrame, GW_HWNDFIRST); hwndDialog; hwndDialog = GetWindow(hwndDialog, GW_HWNDNEXT)) {
        hwndEdit = GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_X);
        if (!hwndEdit || !GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_Y)) continue;
        sz[0] = '0' + cPageX; sz[1] = 0; SetWindowText(hwndEdit, sz);
        sz[0] = '0' + cPageY; SetWindowText(GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_Y), sz);
        PostMessage(hwndDialog, WM_COMMAND, fCancel ? IDCANCEL : IDOK, 0);
        return;
    }
}
static void CALLBACK PrintGame(HWND hwnd, UINT msg, UINT_PTR id, DWORD ticks) {
    GAME gameSav;
    const char *pszOutput = getenv("STARS_PRINTMAP_OUTPUT");
    char szTarget[4096], szManifest[4096];
    PLAYER plr;
    uint16_t icksum;
    FILE *lpFile;
    int i;
    (void)hwnd; (void)msg; (void)ticks;
    if (fRunning || !IsWindow(hwndScanner) || idPlayer == iplrNone) return;
    fRunning = TRUE; KillTimer(NULL, id);
    if (!pszOutput) goto done;
    gameSav = game;
    snprintf(szManifest, sizeof(szManifest), "%s/map.json", pszOutput);
    lpFile = fopen(szManifest, "wb"); if (!lpFile) goto done;
    fprintf(lpFile, "{\"galaxy\":%d,\"inverse\":%d,\"planets\":[", dGal, dGalInv);
    for (i = 0; i < game.cPlanMax; i++) fprintf(lpFile, "%s[%d,%d]", i ? "," : "", rgptPlan[i].x, rgptPlan[i].y);
    fputs("]}\n", lpFile); fclose(lpFile);
    idDialog = SetTimer(NULL, 0, 30, ChoosePages);
    if (!idDialog) goto done;
    grbitScan = static_cast<GrbitScan>(grbitScan | grbitScanPlanetNames);
    snprintf(szTarget, sizeof(szTarget), "%s/single.pdf", pszOutput); pszTarget = szTarget;
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 1 || cAlert != 0) goto done;
    cPageX = cPageY = 2;
    snprintf(szTarget, sizeof(szTarget), "%s/tiled.pdf", pszOutput);
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 2 || cAlert != 0) goto done;
    cPageX = cPageY = 1;
    SDL_setenv_unsafe("STARS_SDL_PRINT_PAPER", "a4", 1);
    SDL_setenv_unsafe("STARS_SDL_PRINT_ORIENTATION", "landscape", 1);
    snprintf(szTarget, sizeof(szTarget), "%s/a4.pdf", pszOutput);
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 3 || cAlert != 0) goto done;
    fCancel = TRUE;
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    fPassed = cLaunch == 3 && cAlert == 0 && !memcmp(&game, &gameSav, sizeof(game));
    if (fPassed) {
        // Use the normal race serializer to prepare an unprotected predefined
        // race for a separate installed-product game; frozen inputs stay intact.
        plr = vrgplrDef[0]; plr.lSalt = 0;
        fPassed = FCreateFile(dtRace, iplrNone, szInstalledRace);
        if (fPassed) {
            WriteRtPlr(&plr, NULL); icksum = IRaceChecksum(&plr);
            WriteRt(rtEOF, 2, &icksum); StreamClose();
        }
    }
done:
    if (idDialog) KillTimer(NULL, idDialog);
    printf("Actual map printing: passed=%d launches=%d alerts=%d\n", fPassed, cLaunch, cAlert);
    PostQuitMessage(fPassed ? 0 : 92);
}
int16_t StarsTest_InitInstance(int16_t nCmdShow) {
    int16_t fResult = StarsTestReal_InitInstance(nCmdShow);
    if (fResult) idDriver = SetTimer(NULL, 0, 50, PrintGame);
    return fResult && idDriver ? fResult : FALSE;
}
int main(int argc, char **argv) {
    char szCommand[1024];
    int nResult;
    if (argc != 2 || strlen(argv[1]) >= sizeof(szCommand)) return 2;
    strcpy(szCommand, argv[1]);
    nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (idDriver) KillTimer(NULL, idDriver);
    StarsCompatProcessExit(); SDL_Quit();
    return nResult == 0 && fPassed ? 0 : 1;
}
