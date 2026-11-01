#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <cstdlib>
#include <cstring>

static char *PszCocoaLabelUtf8(LPCSTR psz) {
    return FUtf8Valid(psz, std::strlen(psz)) ? SDL_strdup(psz) : nullptr;
}
static int FPumpCocoaChooser(void *lpv) {
    HWND hwndOwner = static_cast<HWND>((lpv));
    win32::MSG msg;
    while (win32::PeekMessageA(&msg, nullptr, 0, 0, win32::PM_REMOVE)) {
        if (msg.message == win32::WM_QUIT) { win32::PostQuitMessage(static_cast<int>(msg.wParam)); return 0; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return 0;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, std::size_t cbPath) {
    COCOAFILEFILTER *rg = nullptr;
    char *pszTitleUtf;
    void *lpWindow = nullptr;
    int i, iResult = -1;
    pszTitleUtf = PszCocoaLabelUtf8(pszTitle);
    if (!pszTitleUtf) return -1;
    if (cFilter != 0) {
        rg = static_cast<COCOAFILEFILTER *>((std::calloc(static_cast<std::size_t>(cFilter), sizeof(*rg)))); if (!rg) goto done;
        for (i = 0; i < cFilter; i++) {
            rg[i].pszName = PszCocoaLabelUtf8(rgfilter[i].pszName);
            rg[i].pszPattern = PszCocoaLabelUtf8(rgfilter[i].pszPattern);
            if (!rg[i].pszName || !rg[i].pszPattern) goto done;
        }
    }
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    iResult = IChooseCocoaFile(lpWindow, fSave, pszTitleUtf, pszFolder, pszName, rg, cFilter,
                             lpiFilter, pszPath, cbPath, FPumpCocoaChooser, hwndOwner);
done:
    if (rg) for (i = 0; i < cFilter; i++) { SDL_free(const_cast<void *>(reinterpret_cast<const void*>(rg[i].pszName))); SDL_free(const_cast<void *>(reinterpret_cast<const void*>(rg[i].pszPattern))); }
    std::free(rg); SDL_free(pszTitleUtf); return iResult;
}
