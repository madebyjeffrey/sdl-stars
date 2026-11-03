/* Test native backend selection and shared wrapper contracts without desktop input. */
#include "internal.h"
#include "../../compat/win32/src/windows_chooser_io.h"

#include <stars_compat.h>
#include "filedialog_windows_bridge.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static HWND hwndOwner;
static char szChosen[PATH_MAX];
static int iAction, fSaveExpected, cCalls, fBad;

win32::files::FILESELECTIONRESULT StarsTest_IChooseWindowsFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump) {
    (void)lpWindow;
    cCalls++;
    if (static_cast<int>(request.mode == win32::files::FILESELECTIONMODE::save) != fSaveExpected ||
        request.szTitle != "Picker \xc3\xa9" || request.rgFilters.size() != 2 ||
        request.rgFilters[0].szPattern != "*.m?;*.hst" || request.iFilter != 0 ||
        win32::IsWindowEnabled(hwndOwner) || hwndOwner->cNativeFileDialogs != 1) fBad = 1;
    if (iAction == 1) return win32::files::FILESELECTIONCANCELLED{};
    if (iAction == 2) {
        win32::PostQuitMessage(73);
        if (pfnPump()) fBad = 1;
        return win32::files::FILESELECTIONCANCELLED{};
    }
    if (iAction == 3) {
        win32::DestroyWindow(hwndOwner);
        if (pfnPump()) fBad = 1;
        return win32::files::FILESELECTIONCANCELLED{};
    }
    win32::files::FILESELECTIONSELECTED selected;
    selected.szPath = szChosen;
    selected.iFilter = 1;
    return selected;
}

int main(void) {
    WNDCLASSA wc = {};
    OPENFILENAMEA of = {};
    char szCwd[PATH_MAX], szAfter[PATH_MAX], szFile[PATH_MAX], szTitle[128];
    NATIVEFILE *lpfile;
    win32::MSG msg;
    int i;

    WINDOWSCHOOSERIO io_windowschooserio;
    io_windowschooserio.pfnIChooseWindowsFile = StarsTest_IChooseWindowsFile;
    WINDOWSCHOOSERIOSCOPE scope_windowschooserio(io_windowschooserio);

    CHECK(SDL_setenv_unsafe("STARS_FILE_DIALOG_BACKEND", "native", 1) == 0);
    CHECK(CchNativeCwd(std::span{szCwd}));
    CHECK(SDL_strlcpy(szChosen, szCwd, sizeof(szChosen)) < sizeof(szChosen));
    CHECK(SDL_strlcat(szChosen, "/\xce\xa9.m1", sizeof(szChosen)) < sizeof(szChosen));
    lpfile = PfNativeOpen(szChosen, NATIVEFILEMODE::writeTruncate); CHECK(lpfile && ICloseNativeFile(lpfile) == 0);
    wc.lpfnWndProc = win32::DefWindowProcA; wc.lpszClassName = "WindowsNativePickerFlow";
    CHECK(win32::RegisterClassA(&wc));
    hwndOwner = win32::CreateWindowExA(0, wc.lpszClassName, "Owner", win32::WS_OVERLAPPEDWINDOW, 0, 0, 400, 300, NULL, NULL, NULL, NULL);
    CHECK(hwndOwner);
    of.lStructSize = sizeof(of); of.hwndOwner = hwndOwner;
    of.lpstrFile = szFile; of.nMaxFile = sizeof(szFile);
    of.lpstrFileTitle = szTitle; of.nMaxFileTitle = sizeof(szTitle);
    of.lpstrInitialDir = szCwd; of.lpstrTitle = "Picker \xc3\xa9";
    of.lpstrFilter = "Game\0*.m?;*.hst\0All\0*.*\0";
    of.Flags = win32::OFN_NOCHANGEDIR | win32::OFN_FILEMUSTEXIST;
    for (i = 0; i < 2; i++) {
        fSaveExpected = i; szFile[0] = 0; of.nFilterIndex = 1;
        CHECK(i ? win32::GetSaveFileNameA(&of) : win32::GetOpenFileNameA(&of));
        CHECK(strcmp(szFile, szChosen) == 0 && strcmp(szTitle, "\xce\xa9.m1") == 0 && of.nFilterIndex == 2);
        CHECK(of.nFileOffset == strlen(szCwd) + 1 && of.nFileExtension == strlen(szChosen) - 2);
        CHECK(win32::IsWindowEnabled(hwndOwner) && hwndOwner->cNativeFileDialogs == 0);
    }
    fSaveExpected = 0;
    for (iAction = 1; iAction <= 3; iAction++) {
        strcpy(szFile, "unchanged.m1"); of.nFilterIndex = 1;
        CHECK(!win32::GetOpenFileNameA(&of) && win32::CommDlgExtendedError() == 0);
        CHECK(strcmp(szFile, "unchanged.m1") == 0 && of.nFilterIndex == 1);
        if (iAction < 3) CHECK(win32::IsWindowEnabled(hwndOwner) && hwndOwner->cNativeFileDialogs == 0);
        if (iAction == 2) CHECK(win32::PeekMessageA(&msg, NULL, win32::WM_QUIT, win32::WM_QUIT, win32::PM_REMOVE) && msg.wParam == 73);
    }
    CHECK(!win32::IsWindow(hwndOwner) && !fBad && cCalls == 5);
    CHECK(CchNativeCwd(std::span{szAfter}) && strcmp(szAfter, szCwd) == 0);
    CHECK(IUnlinkNativePath(szChosen) == 0);
    StarsCompatShutdown();
    puts("Windows native wrapper file flow passed");
    return 0;
}
