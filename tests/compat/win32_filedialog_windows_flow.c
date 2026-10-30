/* Test native backend selection and shared wrapper contracts without desktop input. */
#include "internal.h"
#include <stars_compat.h>
#include "filedialog_windows_bridge.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static HWND hwndOwner;
static char szChosen[PATH_MAX];
static int iAction, fSaveExpected, cCalls, fBad;

int StarsTest_IChooseWindowsFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                              const char *pszName, const WINDOWSFILEFILTER *rgfilter, int cFilter,
                              int *lpiFilter, char *pszPath, size_t cbPath,
                              int (*pfnPump)(void *), void *lpv) {
    (void)lpWindow; (void)pszFolder; (void)pszName;
    cCalls++;
    if (fSave != fSaveExpected || strcmp(pszTitle, "Picker \xc3\xa9") != 0 || cFilter != 2 ||
        strcmp(rgfilter[0].pszPattern, "*.m?;*.hst") != 0 || *lpiFilter != 0 ||
        IsWindowEnabled(hwndOwner) || hwndOwner->cNativeFileDialogs != 1) fBad = 1;
    if (iAction == 1) return 0;
    if (iAction == 2) {
        PostQuitMessage(73);
        if (pfnPump(lpv) != 0) fBad = 1;
        return 0;
    }
    if (iAction == 3) {
        DestroyWindow(hwndOwner);
        if (pfnPump(lpv) != 0) fBad = 1;
        return 0;
    }
    if (SDL_strlcpy(pszPath, szChosen, cbPath) >= cbPath) return -1;
    *lpiFilter = 1;
    return 1;
}

int main(void) {
    WNDCLASSA wc = {};
    OPENFILENAMEA of = {};
    char szCwd[PATH_MAX], szAfter[PATH_MAX], szFile[PATH_MAX], szTitle[128];
    NATIVEFILE *lpfile;
    MSG msg;
    int i;

    CHECK(SDL_setenv_unsafe("STARS_FILE_DIALOG_BACKEND", "native", 1) == 0);
    CHECK(PszNativeCwd(szCwd, sizeof(szCwd)));
    CHECK(SDL_strlcpy(szChosen, szCwd, sizeof(szChosen)) < sizeof(szChosen));
    CHECK(SDL_strlcat(szChosen, "/\xce\xa9.m1", sizeof(szChosen)) < sizeof(szChosen));
    lpfile = PfNativeOpen(szChosen, "wb"); CHECK(lpfile && ICloseNativeFile(lpfile) == 0);
    wc.lpfnWndProc = DefWindowProcA; wc.lpszClassName = "WindowsNativePickerFlow";
    CHECK(RegisterClassA(&wc));
    hwndOwner = CreateWindow(wc.lpszClassName, "Owner", WS_OVERLAPPEDWINDOW, 0, 0, 400, 300, NULL, NULL, NULL, NULL);
    CHECK(hwndOwner);
    of.lStructSize = sizeof(of); of.hwndOwner = hwndOwner;
    of.lpstrFile = szFile; of.nMaxFile = sizeof(szFile);
    of.lpstrFileTitle = szTitle; of.nMaxFileTitle = sizeof(szTitle);
    of.lpstrInitialDir = szCwd; of.lpstrTitle = "Picker \xc3\xa9";
    of.lpstrFilter = "Game\0*.m?;*.hst\0All\0*.*\0";
    of.Flags = OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
    for (i = 0; i < 2; i++) {
        fSaveExpected = i; szFile[0] = 0; of.nFilterIndex = 1;
        CHECK(i ? GetSaveFileNameA(&of) : GetOpenFileNameA(&of));
        CHECK(strcmp(szFile, szChosen) == 0 && strcmp(szTitle, "\xce\xa9.m1") == 0 && of.nFilterIndex == 2);
        CHECK(of.nFileOffset == strlen(szCwd) + 1 && of.nFileExtension == strlen(szChosen) - 2);
        CHECK(IsWindowEnabled(hwndOwner) && hwndOwner->cNativeFileDialogs == 0);
    }
    fSaveExpected = 0;
    for (iAction = 1; iAction <= 3; iAction++) {
        strcpy(szFile, "unchanged.m1"); of.nFilterIndex = 1;
        CHECK(!GetOpenFileNameA(&of) && CommDlgExtendedError() == 0);
        CHECK(strcmp(szFile, "unchanged.m1") == 0 && of.nFilterIndex == 1);
        if (iAction < 3) CHECK(IsWindowEnabled(hwndOwner) && hwndOwner->cNativeFileDialogs == 0);
        if (iAction == 2) CHECK(PeekMessageA(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE) && msg.wParam == 73);
    }
    CHECK(!IsWindow(hwndOwner) && !fBad && cCalls == 5);
    CHECK(PszNativeCwd(szAfter, sizeof(szAfter)) && strcmp(szAfter, szCwd) == 0);
    CHECK(IUnlinkNativePath(szChosen) == 0);
    StarsCompatShutdown();
    puts("Windows native wrapper file flow passed");
    return 0;
}
