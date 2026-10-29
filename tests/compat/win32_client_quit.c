/* Application quit must unwind the real new-game modal loop. */
#include "win.h"
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
static BOOL fPlayers, fRace, fQuit, fCheckFocus, fFocusPassed, fSaveOwner, fOwnerPassed;
static BOOL fLaunchFocus, fLaunchFocusPassed;
static HWND hwndNewGame;
static int iStage, cFocusWait;
#ifdef __APPLE__
int FTestCocoaSaveOwner(void *lpWindow);
int FTestCocoaAppIcon(void);
static BOOL fNativeSaveOwner, fCheckIcon, fIconPassed;
#endif

static void CALLBACK CheckQuit(HWND hwndTimer, UINT msg, UINT_PTR id, DWORD time) {
    STARSCOMPATSNAPSHOT snap = {0};
    HWND hwnd;
    SDL_Event event = {0};
    size_t i;
    const char *pszRoot;
    char szCollision[512];

    (void)hwndTimer; (void)msg; (void)id; (void)time;
    if (iStage == 0 && IsWindowVisible(hwndTitle)) {
        if (fLaunchFocus) {
            hwnd = LpCompatWindow(hwndTitle);
            fLaunchFocusPassed = SDL_GetKeyboardFocus() == hwnd->lpWindow &&
                HwndCompatRoot(GetFocus()) == hwndTitle;
            if (!fLaunchFocusPassed && ++cFocusWait < 20) return;
        }
#ifdef __APPLE__
        if (fCheckIcon) fIconPassed = FTestCocoaAppIcon();
#endif
        iStage = 1;
        PostMessage(hwndTitle, WM_COMMAND, fRace ? IDC_LIBRARY_RACE_NEW : 0, 0);
        return;
    }
    if (iStage == 0 || fQuit || !StarsCompatCaptureHierarchy(&snap)) return;
#ifdef __APPLE__
    if (fNativeSaveOwner && iStage == 2 && IsWindow(hwndNewGame)) {
        hwnd = LpCompatWindow(hwndNewGame);
        if (FTestCocoaSaveOwner(SDL_GetPointerProperty(SDL_GetWindowProperties(hwnd->lpWindow),
                SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL))) {
            fOwnerPassed = IsWindowVisible(hwndNewGame) && !IsWindowEnabled(hwndNewGame) &&
                !IsWindowVisible(hwndFrame);
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
                if (!pszRoot) { StarsCompatFreeHierarchy(&snap); PostQuitMessage(1); return; }
                CchSnprintf(szCollision, sizeof(szCollision), "%s%sGames%sOwner check", pszRoot, szDirSep, szDirSep);
                if (!SDL_CreateDirectory(szCollision) || !SetDlgItemText(hwnd, IDC_NEW_GAME_NAME, "Owner check")) {
                    StarsCompatFreeHierarchy(&snap); PostQuitMessage(1); return;
                }
                iStage = 2;
                PostMessage(hwnd, WM_COMMAND, IDC_FINISH, 0);
                break;
            } else if (iStage == 2 && GetWindow(hwnd, GW_OWNER) == hwndNewGame) {
                fOwnerPassed = IsWindowVisible(hwndNewGame) && !IsWindowEnabled(hwndNewGame) &&
                    !IsWindowVisible(hwndFrame);
                iStage = 3;
                PostMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
                break;
            } else if (iStage == 3 && hwnd == hwndNewGame && IsWindowEnabled(hwndNewGame)) {
                event.type = SDL_EVENT_QUIT;
                fQuit = SDL_PushEvent(&event);
                break;
            }
            continue;
        }
        if (fPlayers && iStage == 1 && hwnd->lpfnDlg == NewGameDlg) {
            iStage = 2;
            PostMessage(hwnd, WM_COMMAND, IDC_NEXT, 0);
            break;
        }
        if ((!fPlayers && !fRace && hwnd->lpfnDlg == NewGameDlg) ||
            (fPlayers && iStage == 2 && hwnd->lpfnDlg == NewGameDlg2) ||
            (fRace && hwnd->lpfnDlg == RaceWizardDlg1)) {
            // Optional real-desktop check; dummy video cannot grant native keyboard focus.
            if (fCheckFocus) {
                fFocusPassed = SDL_GetKeyboardFocus() == hwnd->lpWindow && IsChild(hwnd, GetFocus());
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
    if (fNativeSaveOwner) fSaveOwner = TRUE;
    fCheckIcon = argc == 2 && !strcmp(argv[1], "icon");
#endif
    idTimer = SetTimer(NULL, 0, 25, CheckQuit);
    if (!idTimer) return 1;
    nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    KillTimer(NULL, idTimer);
    StarsCompatProcessExit();
    SDL_Quit();
    if (nResult != 0 || !fQuit || gd.fNoIdleChecks) return 1;
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
