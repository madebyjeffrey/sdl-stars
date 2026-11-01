#include "filedialog_backend.h"
#include "windows_chooser_io.h"
#include "filedialog_windows_bridge.h"
#include <stdlib.h>

static int FPumpWindowsChooser(void *lpv) {
    HWND hwndOwner = (HWND)lpv;
    win32::MSG msg;

    while (win32::PeekMessageA(&msg, nullptr, 0, 0, win32::PM_REMOVE)) {
        if (msg.message == win32::WM_QUIT) { win32::PostQuitMessage((int)msg.wParam); return 0; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return 0;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}

int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath) {
    WINDOWSFILEFILTER *rg = nullptr;
    void *lpWindow = nullptr;
    int i, iResult;

    if (cFilter != 0) {
        rg = (WINDOWSFILEFILTER *)calloc((size_t)cFilter, sizeof(*rg));
        if (!rg) return -1;
        for (i = 0; i < cFilter; i++) {
            rg[i].pszName = rgfilter[i].pszName;
            rg[i].pszPattern = rgfilter[i].pszPattern;
        }
    }
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    iResult = WindowsChooserIO().pfnIChooseWindowsFile(lpWindow, fSave, pszTitle, pszFolder, pszName, rg, cFilter,
                                lpiFilter, pszPath, cbPath, FPumpWindowsChooser, hwndOwner);
    free(rg);
    return iResult;
}
