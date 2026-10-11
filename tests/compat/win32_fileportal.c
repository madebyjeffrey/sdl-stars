/* The private-bus fixture serves real portal methods and asynchronous signals. */
#include <windows.h>
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static HWND hwndOwner;
static int cTicks, iAction, cPrompts;
static BOOL fPromptPending;
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndPrompt;
    char szCaption[64];
    if (msg == WM_TIMER) {
        cTicks++;
        if (IsWindowEnabled(hwnd)) { fputs("Portal owner was not modal\n", stderr); exit(2); }
        if (SDL_GetWindowFlags(hwnd->lpWindow) & (SDL_WINDOW_NOT_FOCUSABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
            fputs("Portal changed native owner visibility/focusability\n", stderr); exit(2);
        }
        if (iAction == 1) { iAction = 0; PostQuitMessage(73); }
        else if (iAction == 2) { iAction = 0; DestroyWindow(hwnd); }
        else if (!fPromptPending) {
            for (hwndPrompt = GetWindow(hwnd, GW_HWNDFIRST); hwndPrompt; hwndPrompt = GetWindow(hwndPrompt, GW_HWNDNEXT)) {
                GetWindowTextA(hwndPrompt, szCaption, sizeof(szCaption));
                if (IsWindowVisible(hwndPrompt) && strcmp(szCaption, iAction == 4 ? "Confirm save" : "File selection") == 0) {
                    cPrompts++; PostMessageA(hwndPrompt, WM_COMMAND, iAction == 4 ? IDNO : IDOK, 0);
                    fPromptPending = TRUE;
                    iAction = 0; break;
                }
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static void Init(OPENFILENAMEA *lp, char *pszFile, const char *pszTitle, const char *pszDir) {
    static const char szFilter[] = "Stars files\0*.M*;*.HST;*.R*\0Host files\0*.HST\0All files\0*.*\0";
    memset(lp, 0, sizeof(*lp));
    lp->lStructSize = sizeof(*lp); lp->hwndOwner = hwndOwner;
    lp->lpstrFile = pszFile; lp->nMaxFile = 4096; lp->lpstrTitle = pszTitle;
    lp->lpstrInitialDir = pszDir; lp->lpstrFilter = szFilter; lp->nFilterIndex = 1;
    lp->Flags = OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
    pszFile[0] = 0; cTicks = 0; cPrompts = 0; fPromptPending = FALSE;
}
int main(int argc, char **argv) {
    WNDCLASSA wc = {0};
    OPENFILENAMEA ofn;
    char szFile[4096], szCwd[4096], szAfter[4096];
    const char *rgInvalid[] = {"Portal bad URI", "Portal remote", "Portal malformed", "Portal empty", "Portal multiple", "Portal unavailable", "Portal failed"};
    size_t i;
    WORD cbRequired;
    MSG msg;
    CHECK(argc == 2 && getcwd(szCwd, sizeof(szCwd)));
    wc.lpszClassName = "PortalOwner"; wc.lpfnWndProc = OwnerProc; wc.hInstance = GetModuleHandleA(NULL);
    CHECK(RegisterClassA(&wc));
    hwndOwner = CreateWindow(wc.lpszClassName, "Portal owner", WS_POPUP | WS_VISIBLE, 10, 10, 200, 150, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwndOwner && SetTimer(hwndOwner, 1, 10, NULL));
    CHECK(SDL_SetWindowFullscreen(hwndOwner->lpWindow, true));
    Init(&ofn, szFile, "Portal unicode", argv[1]);
    CHECK(GetOpenFileNameA(&ofn) && cTicks != 0 && IsWindowEnabled(hwndOwner));
    CHECK(strcmp(szFile + ofn.nFileOffset, "caf\xc3\xa9-\xce\xa9.m1") == 0 && strcmp(szFile + ofn.nFileExtension, "m1") == 0);
    CHECK(ofn.nFilterIndex == 2 && CommDlgExtendedError() == 0);
    CHECK(getcwd(szAfter, sizeof(szAfter)) && strcmp(szCwd, szAfter) == 0);
    CHECK(hwndOwner->cNativeFileDialogs == 0);
    /* Exercise SDL's exported-handle contract without requiring a compositor
       or using the user's portal bus. Include a handle longer than an XID. */
    CHECK(SDL_SetStringProperty(SDL_GetWindowProperties(hwndOwner->lpWindow),
          SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING,
          "stars-export-abcdefghijklmnopqrstuvwxyz-abcdefghijklmnopqrstuvwxyz-abcdefghijklmnopqrstuvwxyz"));
    Init(&ofn, szFile, "Portal Wayland parent", argv[1]); CHECK(GetOpenFileNameA(&ofn));
    CHECK(SDL_ClearProperty(SDL_GetWindowProperties(hwndOwner->lpWindow),
          SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING));
    CHECK(hwndOwner->cNativeFileDialogs == 0);
    Init(&ofn, szFile, "Portal early", argv[1]); CHECK(GetOpenFileNameA(&ofn));
    Init(&ofn, szFile, "Portal cancel", argv[1]); strcpy(szFile, "unchanged");
    CHECK(!GetOpenFileNameA(&ofn) && CommDlgExtendedError() == 0 && strcmp(szFile, "unchanged") == 0 && cPrompts == 0);
    CHECK(hwndOwner->cNativeFileDialogs == 0 && IsWindowEnabled(hwndOwner) &&
          (SDL_GetWindowFlags(hwndOwner->lpWindow) & SDL_WINDOW_FULLSCREEN));
    Init(&ofn, szFile, "Portal save", argv[1]); ofn.Flags = OFN_NOCHANGEDIR; ofn.lpstrDefExt = "r1";
    CHECK(GetSaveFileNameA(&ofn) && strcmp(szFile + ofn.nFileOffset, "new race.r1") == 0 && access(szFile, F_OK) != 0);
    Init(&ofn, szFile, "Portal retry", argv[1]); iAction = 3; cPrompts = 0;
    CHECK(GetOpenFileNameA(&ofn) && cPrompts == 1 && strstr(szFile, "/retry/caf") && ofn.nFilterIndex == 2);
    Init(&ofn, szFile, "Portal directory", argv[1]); iAction = 3; cPrompts = 0;
    CHECK(GetOpenFileNameA(&ofn) && cPrompts == 1 && strstr(szFile, "/retry/caf"));
    Init(&ofn, szFile, "Portal overwrite", argv[1]); ofn.Flags = OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    iAction = 4; cPrompts = 0;
    CHECK(GetSaveFileNameA(&ofn) && cPrompts == 1 && strcmp(szFile + ofn.nFileOffset, "another.r1") == 0);
    Init(&ofn, szFile, "Portal buffer", argv[1]); ofn.nMaxFile = 8;
    CHECK(!GetOpenFileNameA(&ofn) && CommDlgExtendedError() == FNERR_BUFFERTOOSMALL && cPrompts == 1);
    memcpy(&cbRequired, szFile, sizeof(cbRequired)); CHECK(cbRequired > 8);
    for (i = 0; i < sizeof(rgInvalid) / sizeof(rgInvalid[0]); i++) {
        Init(&ofn, szFile, rgInvalid[i], argv[1]);
        CHECK(!GetOpenFileNameA(&ofn) && CommDlgExtendedError() == CDERR_INITIALIZATION && szFile[0] == 0 && IsWindowEnabled(hwndOwner) && cPrompts == 1);
    }
    Init(&ofn, szFile, "Portal quit", argv[1]); iAction = 1;
    CHECK(!GetOpenFileNameA(&ofn) && CommDlgExtendedError() == 0);
    CHECK(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_QUIT && msg.wParam == 73);
    Init(&ofn, szFile, "Portal destroy", argv[1]); iAction = 2;
    CHECK(!GetOpenFileNameA(&ofn) && CommDlgExtendedError() == 0 && !IsWindow(hwndOwner));
    StarsCompatShutdown();
    puts("Native portal selection, UTF-8, filters, cancellation, early response and lifetime checks passed");
    return 0;
}
