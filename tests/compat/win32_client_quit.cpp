/* Application quit must unwind the real new-game modal loop. */
#include "win.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>

namespace ui { int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow); }
static BOOL fPlayers, fRace, fQuit, fCheckFocus, fFocusPassed, fSaveOwner, fOwnerPassed;
static BOOL fLaunchFocus, fLaunchFocusPassed;
static HWND hwndNewGame;
static int iStage, cFocusWait;
#ifdef __APPLE__
int FTestCocoaSaveOwner(void *lpWindow);
int FTestCocoaAppIcon(void);
static BOOL fNativeSaveOwner, fCheckIcon, fIconPassed;
#endif

static void  CheckQuit(HWND hwndTimer, UINT msg, UINT_PTR id, DWORD time) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    STARSCOMPATSNAPSHOT snap = {};
    HWND hwnd;
    SDL_Event event = {};
    size_t i;
    const char *pszRoot;
    char szCollision[512];

    (void)hwndTimer; (void)msg; (void)id; (void)time;
    if (iStage == 0 && win32::IsWindowVisible(app.lpui->hwndTitle)) {
        if (fLaunchFocus) {
            hwnd = LpCompatWindow(app.lpui->hwndTitle);
            fLaunchFocusPassed = SDL_GetKeyboardFocus() == hwnd->lpWindow.get() &&
                HwndCompatRoot(win32::GetFocus()) == app.lpui->hwndTitle;
            if (!fLaunchFocusPassed && ++cFocusWait < 20) return;
        }
#ifdef __APPLE__
        if (fCheckIcon) fIconPassed = FTestCocoaAppIcon();
#endif
        iStage = 1;
        win32::PostMessageA(app.lpui->hwndTitle, win32::WM_COMMAND, fRace ? IDC_LIBRARY_RACE_NEW : 0, 0);
        return;
    }
    if (iStage == 0 || fQuit || !StarsCompatCaptureHierarchy(&snap)) return;
#ifdef __APPLE__
    if (fNativeSaveOwner && iStage == 2 && win32::IsWindow(hwndNewGame)) {
        hwnd = LpCompatWindow(hwndNewGame);
        if (FTestCocoaSaveOwner(SDL_GetPointerProperty(SDL_GetWindowProperties(hwnd->lpWindow.get()),
                SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL))) {
            fOwnerPassed = win32::IsWindowVisible(hwndNewGame) && !win32::IsWindowEnabled(hwndNewGame) &&
                !win32::IsWindowVisible(app.lpui->hwndFrame);
            iStage = 3;
        }
    }
#endif
    for (i = 0; i < snap.cWindow; i++) {
        hwnd = LpCompatWindow(StarsCompatResolveWindow(snap.rgWindow[i].id));
        if (!hwnd || !hwnd->fVisible || !hwnd->fDialog) continue;
        if (fSaveOwner) {
            if (iStage == 1 && hwnd->lpfnDlg == NewGameDlg) {
                hwndNewGame = hwnd;
                pszRoot = PszCompatEnvironment("STARS_LIBRARY_DIR");
                if (!pszRoot) pszRoot = PszCompatEnvironment("STARS_SETTINGS_DIR");
                if (!pszRoot) { StarsCompatFreeHierarchy(&snap); win32::PostQuitMessage(1); return; }
                CchSnprintf(szCollision, sizeof(szCollision), "%s%sGames%sOwner check", pszRoot, szDirSep, szDirSep);
                if (!SDL_CreateDirectory(szCollision) || !win32::SetDlgItemTextA(hwnd, IDC_NEW_GAME_NAME, "Owner check")) {
                    StarsCompatFreeHierarchy(&snap); win32::PostQuitMessage(1); return;
                }
                iStage = 2;
                win32::PostMessageA(hwnd, win32::WM_COMMAND, IDC_FINISH, 0);
                break;
            } else if (iStage == 2 && win32::GetWindow(hwnd, win32::GW_OWNER) == hwndNewGame) {
                fOwnerPassed = win32::IsWindowVisible(hwndNewGame) && !win32::IsWindowEnabled(hwndNewGame) &&
                    !win32::IsWindowVisible(app.lpui->hwndFrame);
                iStage = 3;
                win32::PostMessageA(hwnd, win32::WM_COMMAND, IDCANCEL, 0);
                break;
            } else if (iStage == 3 && hwnd == hwndNewGame && win32::IsWindowEnabled(hwndNewGame)) {
                event.type = SDL_EVENT_QUIT;
                fQuit = SDL_PushEvent(&event);
                break;
            }
            continue;
        }
        if (fPlayers && iStage == 1 && hwnd->lpfnDlg == NewGameDlg) {
            iStage = 2;
            win32::PostMessageA(hwnd, win32::WM_COMMAND, IDC_NEXT, 0);
            break;
        }
        if ((!fPlayers && !fRace && hwnd->lpfnDlg == NewGameDlg) ||
            (fPlayers && iStage == 2 && hwnd->lpfnDlg == NewGameDlg2) ||
            (fRace && hwnd->lpfnDlg == RaceWizardDlg1)) {
            // Optional real-desktop check; dummy video cannot grant native keyboard focus.
            if (fCheckFocus) {
                fFocusPassed = SDL_GetKeyboardFocus() == hwnd->lpWindow.get() && win32::IsChild(hwnd, win32::GetFocus());
                if (!fFocusPassed && ++cFocusWait < 20) break;
            }
            event.type = SDL_EVENT_QUIT;
            fQuit = SDL_PushEvent(&event);
            break;
        }
    }
    StarsCompatFreeHierarchy(&snap);
}

int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szCommand[] = "";
    UINT_PTR idTimer;
    int nResult;

    fPlayers = argc == 2 && !strcmp(argv[1], "players");
    fRace = argc == 2 && !strcmp(argv[1], "race");
    fCheckFocus = argc == 2 && !strcmp(argv[1], "focus");
    fLaunchFocus = argc == 2 && !strcmp(argv[1], "launch-focus");
    fSaveOwner = argc == 2 && !strcmp(argv[1], "save-owner");
#ifdef __APPLE__
    fNativeSaveOwner = argc == 2 && !strcmp(argv[1], "native-save-owner");
    if (fNativeSaveOwner) fSaveOwner = 1;
    fCheckIcon = argc == 2 && !strcmp(argv[1], "icon");
#endif
    idTimer = win32::SetTimer(NULL, 0, 25, CheckQuit);
    if (!idTimer) return 1;
    nResult = ui::WinMain(win32::GetModuleHandleA(NULL), NULL, szCommand, win32::SW_SHOWNORMAL);
    win32::KillTimer(NULL, idTimer);
    StarsCompatProcessExit();
    SDL_Quit();
    if (nResult != 0 || !fQuit || app.state.gd.fNoIdleChecks) return 1;
    if (fCheckFocus && !fFocusPassed) {
        fprintf(stderr, "New-game dialog did not acquire native and control focus\n");
        return 1;
    }
    if (fLaunchFocus && !fLaunchFocusPassed) {
        fprintf(stderr, "Title screen did not acquire native and logical focus at launch\n");
        return 1;
    }
    if (fSaveOwner && !fOwnerPassed) return 1;
#ifdef __APPLE__
    if (fCheckIcon && !fIconPassed) {
        fprintf(stderr, "Dock icon did not use the transparent 512-pixel artwork\n");
        return 1;
    }
#endif
    puts("Application quit unwinds the new-game wizard and exits");
    return 0;
}
