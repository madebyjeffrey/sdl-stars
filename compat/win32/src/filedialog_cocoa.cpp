#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <stdlib.h>
#include <string.h>

static char *PszCocoaLabelUtf8(LPCSTR psz) {
    return FUtf8Valid(psz, strlen(psz)) ? SDL_strdup(psz) : NULL;
}
static int FPumpCocoaChooser(void *lpv) {
    HWND hwndOwner = (HWND)(lpv);
    win32::MSG msg;
    while (win32::PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { win32::PostQuitMessage((int)msg.wParam); return 0; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return 0;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath) {
    COCOAFILEFILTER *rg = NULL;
    char *pszTitleUtf;
    void *lpWindow = NULL;
    int i, iResult = -1;
    pszTitleUtf = PszCocoaLabelUtf8(pszTitle);
    if (!pszTitleUtf) return -1;
    if (cFilter != 0) {
        rg = (COCOAFILEFILTER *)(calloc((size_t)cFilter, sizeof(*rg))); if (!rg) goto done;
        for (i = 0; i < cFilter; i++) {
            rg[i].pszName = PszCocoaLabelUtf8(rgfilter[i].pszName);
            rg[i].pszPattern = PszCocoaLabelUtf8(rgfilter[i].pszPattern);
            if (!rg[i].pszName || !rg[i].pszPattern) goto done;
        }
    }
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    iResult = IChooseCocoaFile(lpWindow, fSave, pszTitleUtf, pszFolder, pszName, rg, cFilter,
                             lpiFilter, pszPath, cbPath, FPumpCocoaChooser, hwndOwner);
done:
    if (rg) for (i = 0; i < cFilter; i++) { SDL_free((void *)rg[i].pszName); SDL_free((void *)rg[i].pszPattern); }
    free(rg); SDL_free(pszTitleUtf); return iResult;
}
