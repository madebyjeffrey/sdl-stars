/* SDL display coordinates plus classic UI metrics for the source wrapper. */
#include "internal.h"
#include "display_io.h"
#include <limits.h>
#include <unistd.h>

static BOOL FDisplayRect(BOOL fUsable, LPRECT lprc) {
    SDL_Rect rc;
    SDL_DisplayID id;
    if (!FCompatVideo()) return 0;
    id = CompatDisplayIO().pfnGetPrimaryDisplay();
    if (!id || !(fUsable ? CompatDisplayIO().pfnGetDisplayUsableBounds(id, &rc) : CompatDisplayIO().pfnGetDisplayBounds(id, &rc)) ||
        rc.w <= 0 || rc.h <= 0 || (int64_t)rc.x + rc.w > INT_MAX || (int64_t)rc.y + rc.h > INT_MAX) return 0;
    win32::SetRect(lprc, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h);
    return 1;
}

static BOOL FVirtualRect(LPRECT lprc, int *lpc) {
    SDL_DisplayID *rgid;
    SDL_Rect rc;
    int c, i;
    int64_t xMin = INT_MAX, yMin = INT_MAX, xMax = INT_MIN, yMax = INT_MIN;
    BOOL fOk = 1;
    if (!FCompatVideo()) return 0;
    rgid = CompatDisplayIO().pfnGetDisplays(&c);
    if (!rgid || c <= 0) { SDL_free(rgid); return 0; }
    for (i = 0; i < c; i++) {
        if (!CompatDisplayIO().pfnGetDisplayBounds(rgid[i], &rc) || rc.w <= 0 || rc.h <= 0) { fOk = 0; break; }
        xMin = StarsMin(xMin, rc.x); yMin = StarsMin(yMin, rc.y);
        xMax = (((xMax) > ((int64_t)rc.x + rc.w)) ? (xMax) : ((int64_t)rc.x + rc.w)); yMax = (((yMax) > ((int64_t)rc.y + rc.h)) ? (yMax) : ((int64_t)rc.y + rc.h));
    }
    SDL_free(rgid);
    if (!fOk || xMax > INT_MAX || yMax > INT_MAX || xMax - xMin > INT_MAX || yMax - yMin > INT_MAX) return 0;
    win32::SetRect(lprc, (int)xMin, (int)yMin, (int)xMax, (int)yMax); *lpc = c;
    return 1;
}

int win32::GetSystemMetrics(int i) {
    RECT rc;
    int c;
    if (!SDL_IsMainThread()) return 0;
    switch (i) {
    case win32::SM_CXSCREEN: case win32::SM_CYSCREEN:
        if (!FDisplayRect(0, &rc)) return 0;
        return i == win32::SM_CXSCREEN ? rc.right - rc.left : rc.bottom - rc.top;
    case win32::SM_XVIRTUALSCREEN: case win32::SM_YVIRTUALSCREEN: case win32::SM_CXVIRTUALSCREEN: case win32::SM_CYVIRTUALSCREEN: case win32::SM_CMONITORS:
        if (!FVirtualRect(&rc, &c)) return 0;
        if (i == win32::SM_XVIRTUALSCREEN) return rc.left;
        if (i == win32::SM_YVIRTUALSCREEN) return rc.top;
        if (i == win32::SM_CXVIRTUALSCREEN) return rc.right - rc.left;
        if (i == win32::SM_CYVIRTUALSCREEN) return rc.bottom - rc.top;
        return c;
    case win32::SM_CXBORDER: case win32::SM_CYBORDER: return 1;
    case win32::SM_CXDLGFRAME: case win32::SM_CYDLGFRAME: return 3;
    case win32::SM_CXFRAME: case win32::SM_CYFRAME: return 8;
    case win32::SM_CYCAPTION: return 23;
    case win32::SM_CXVSCROLL: case win32::SM_CYHSCROLL: return 17;
    case win32::SM_CXICON: case win32::SM_CYICON: case win32::SM_CXCURSOR: case win32::SM_CYCURSOR: return 32;
    case win32::SM_CXDRAG: case win32::SM_CYDRAG: return 4;
    default: return 0;
    }
}

BOOL win32::SystemParametersInfoA(UINT action, UINT param, PVOID lpv, UINT flags) {
    RECT rc;
    (void)param; (void)flags;
    if (!SDL_IsMainThread() || !lpv) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    switch (action) {
    case win32::SPI_GETWORKAREA:
        if (!FDisplayRect(1, &rc)) return 0;
        *(RECT *)lpv = rc; return 1;
    case win32::SPI_GETWHEELSCROLLLINES: case win32::SPI_GETWHEELSCROLLCHARS:
        /* SDL exposes wheel deltas, but no portable desktop line/character preference. */
        *(UINT *)lpv = 3; return 1;
    default: win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
    }
}

DWORD win32::GetWindowThreadProcessId(HWND hwnd, LPDWORD lpdwProcess) {
    if (!LpCompatWindow(hwnd)) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0; }
    if (lpdwProcess) *lpdwProcess = (DWORD)getpid();
    return win32::GetCurrentThreadId();
}
