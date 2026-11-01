/* Exercise the actual map dialog and printing command on a copied game. */
#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "../../src/core/alert_io.h"
#include "../../src/core/player_choice_io.h"
#include "../../src/ui/desktop_choice_io.h"
#include "../../compat/win32/src/external_io.h"

#include "ownership.h"
#include <SDL3/SDL.h>
#include <stars_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BOOL fRunning, fPassed, fCancel;
static int cLaunch, cAlert;
static UINT_PTR idDriver, idDialog;
static int cPageX = 1, cPageY = 1;
static const char *pszTarget;
static char szInstalledRace[] = "installed.r1";

int16_t StarsTest_PromptPassword(APPSESSION &) { return 1; }
int16_t StarsTest_AlertSz(APPSESSION &, const char *sz, int16_t mb) { (void)mb; fprintf(stderr, "Print alert: %s\n", sz); cAlert++; return IDOK; }
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
static void  ChoosePages(HWND hwnd, UINT msg, UINT_PTR id, DWORD ticks) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND hwndDialog, hwndEdit;
    char sz[2];
    (void)hwnd; (void)msg; (void)id; (void)ticks;
    for (hwndDialog = win32::GetWindow(app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
        hwndEdit = win32::GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_X);
        if (!hwndEdit || !win32::GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_Y)) continue;
        sz[0] = '0' + cPageX; sz[1] = 0; ui::SetWindowText(hwndEdit, sz);
        sz[0] = '0' + cPageY; ui::SetWindowText(win32::GetDlgItem(hwndDialog, IDC_PRINT_MAP_PAGES_Y), sz);
        win32::PostMessageA(hwndDialog, win32::WM_COMMAND, fCancel ? IDCANCEL : IDOK, 0);
        return;
    }
}
static BOOL FTextViewContracts(APPSESSION &app) {
    NATIVEOWNER<std::remove_pointer_t<HDC>, win32::DeleteDC> hdc(win32::CreateCompatibleDC(NULL));
    char sz[sizeof(app.state.szLastStrGet)], szExpected[sizeof(app.state.szLastStrGet)], rgch[7];
    const char *psz = sz;

    if (!hdc) return 0;
    CchGetString(app, idsNone2, szExpected);
    strcpy(sz, "mutable");
    RightTextOut(app, hdc.get(), 80, 0, sz, -1, 0);
    if (strcmp(sz, szExpected)) return 0;
    strcpy(sz, "borrowed");
    RightTextOut(app, hdc.get(), 80, 0, psz, -1, 0);
    if (strcmp(sz, "borrowed")) return 0;
    memset(rgch, '!', sizeof(rgch));
    RightTextOut(app, hdc.get(), 80, 0, std::span{rgch}.subspan(1, 3), -1, 0);
    if (memcmp(rgch + 1, szExpected, 2) || rgch[3] || rgch[0] != '!' || rgch[4] != '!') return 0;
    RightTextOut(app, hdc.get(), 80, 0, "literal", 0, 0);
    return 1;
}
static void  PrintGame(HWND hwnd, UINT msg, UINT_PTR id, DWORD ticks) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    GAME gameSav;
    const char *pszOutput = getenv("STARS_PRINTMAP_OUTPUT");
    char szTarget[4096], szManifest[4096];
    PLAYER plr;
    uint16_t icksum;
    FILE *lpFile;
    int i;
    (void)hwnd; (void)msg; (void)ticks;
    if (fRunning || !win32::IsWindow(app.lpui->hwndScanner) || app.state.idPlayer == iplrNone) return;
    fRunning = 1; win32::KillTimer(NULL, id);
    if (!pszOutput || !FTextViewContracts(app)) goto done;
    gameSav = app.state.game;
    snprintf(szManifest, sizeof(szManifest), "%s/map.json", pszOutput);
    lpFile = fopen(szManifest, "wb"); if (!lpFile) goto done;
    fprintf(lpFile, "{\"galaxy\":%d,\"inverse\":%d,\"planets\":[", app.state.dGal, app.state.dGalInv);
    for (i = 0; i < app.state.game.cPlanMax; i++) fprintf(lpFile, "%s[%d,%d]", i ? "," : "", app.state.rgptPlan[i].x, app.state.rgptPlan[i].y);
    fputs("]}\n", lpFile); fclose(lpFile);
    idDialog = win32::SetTimer(NULL, 0, 30, ChoosePages);
    if (!idDialog) goto done;
    app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | grbitScanPlanetNames);
    snprintf(szTarget, sizeof(szTarget), "%s/single.pdf", pszOutput); pszTarget = szTarget;
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 1 || cAlert != 0) goto done;
    cPageX = cPageY = 2;
    snprintf(szTarget, sizeof(szTarget), "%s/tiled.pdf", pszOutput);
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 2 || cAlert != 0) goto done;
    cPageX = cPageY = 1;
    SDL_setenv_unsafe("STARS_SDL_PRINT_PAPER", "a4", 1);
    SDL_setenv_unsafe("STARS_SDL_PRINT_ORIENTATION", "landscape", 1);
    snprintf(szTarget, sizeof(szTarget), "%s/a4.pdf", pszOutput);
    SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", szTarget, 1);
    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    if (cLaunch != 3 || cAlert != 0) goto done;
    fCancel = 1;
    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_PRINT_MAP, 0);
    fPassed = cLaunch == 3 && cAlert == 0 && !memcmp(&app.state.game, &gameSav, sizeof(app.state.game));
    if (fPassed) {
        // Use the normal race serializer to prepare an unprotected predefined
        // race for a separate installed-product game; frozen inputs stay intact.
        plr = vrgplrDef[0]; plr.lSalt = 0;
        fPassed = FCreateFile(app, dtRace, iplrNone, szInstalledRace);
        if (fPassed) {
            WriteRtPlr(app, &plr); icksum = IRaceChecksum(&plr);
            WriteRtEof(app, icksum); StreamClose(app);
        }
    }
done:
    if (idDialog) win32::KillTimer(NULL, idDialog);
    printf("Actual map printing: passed=%d launches=%d alerts=%d\n", fPassed, cLaunch, cAlert);
    win32::PostQuitMessage(fPassed ? 0 : 92);
}
int16_t StarsTest_InitInstance(APPSESSION &app, int16_t nCmdShow) {
    int16_t fResult = InitInstance(app, nCmdShow);
    if (fResult) idDriver = win32::SetTimer(NULL, 0, 50, PrintGame);
    return fResult && idDriver ? fResult : 0;
}
int main(int argc, char **argv) {
    char szCommand[1024];
    int nResult;
    ALERTIO io_alertio;
    io_alertio.pfnAlertSz = StarsTest_AlertSz;
    ALERTIOSCOPE scope_alertio(io_alertio);
    PLAYERCHOICEIO io_playerchoiceio;
    io_playerchoiceio.pfnPromptPassword = StarsTest_PromptPassword;
    PLAYERCHOICEIOSCOPE scope_playerchoiceio(io_playerchoiceio);
    DESKTOPCHOICEIO io_desktopchoiceio;
    io_desktopchoiceio.pfnInitInstance = StarsTest_InitInstance;
    DESKTOPCHOICEIOSCOPE scope_desktopchoiceio(io_desktopchoiceio);
    COMPATEXTERNALIO io_compatexternalio;
    io_compatexternalio.pfnOpenURL = StarsTest_SDL_OpenURL;
    COMPATEXTERNALIOSCOPE scope_compatexternalio(io_compatexternalio);

    if (argc != 2 || strlen(argv[1]) >= sizeof(szCommand)) return 2;
    strcpy(szCommand, argv[1]);
    nResult = ui::WinMain(win32::GetModuleHandleA(NULL), NULL, szCommand, win32::SW_SHOWNORMAL);
    if (idDriver) win32::KillTimer(NULL, idDriver);
    StarsCompatProcessExit(); SDL_Quit();
    return nResult == 0 && fPassed ? 0 : 1;
}
