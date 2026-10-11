#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <stdlib.h>
#include <string.h>
static int FPumpCocoaChooser(void *lpv) {
    HWND hwndOwner = lpv;
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { PostQuitMessage((int)msg.wParam); return 0; }
        TranslateMessage(&msg); DispatchMessageA(&msg);
        if (hwndOwner && !IsWindow(hwndOwner)) return 0;
    }
    return !hwndOwner || IsWindow(hwndOwner);
}
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath) {
    COCOAFILEFILTER *rg = NULL;
    char *pszTitleUtf;
    void *lpWindow = NULL;
    int i, iResult = -1;
    pszTitleUtf = SDL_iconv_string("UTF-8", "WINDOWS-1252", pszTitle, strlen(pszTitle) + 1);
    if (!pszTitleUtf) return -1;
    if (cFilter != 0) {
        rg = calloc((size_t)cFilter, sizeof(*rg)); if (!rg) goto done;
        for (i = 0; i < cFilter; i++) {
            rg[i].pszName = SDL_iconv_string("UTF-8", "WINDOWS-1252", rgfilter[i].pszName, strlen(rgfilter[i].pszName) + 1);
            rg[i].pszPattern = SDL_iconv_string("UTF-8", "WINDOWS-1252", rgfilter[i].pszPattern, strlen(rgfilter[i].pszPattern) + 1);
            if (!rg[i].pszName || !rg[i].pszPattern) goto done;
        }
    }
    if (hwndOwner && hwndOwner->lpWindow)
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    iResult = IChooseCocoaFile(lpWindow, fSave, pszTitleUtf, pszFolder, pszName, rg, cFilter,
                             lpiFilter, pszPath, cbPath, FPumpCocoaChooser, hwndOwner);
done:
    if (rg) for (i = 0; i < cFilter; i++) { SDL_free((void *)rg[i].pszName); SDL_free((void *)rg[i].pszPattern); }
    free(rg); SDL_free(pszTitleUtf); return iResult;
}
