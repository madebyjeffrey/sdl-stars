/* SDL display coordinates plus classic UI metrics for the source wrapper. */
#include "internal.h"
#include <limits.h>
#include <unistd.h>

static BOOL FDisplayRect(BOOL fUsable, LPRECT lprc) {
    SDL_Rect rc;
    SDL_DisplayID id;
    if (!FCompatVideo()) return FALSE;
    id = SDL_GetPrimaryDisplay();
    if (!id || !(fUsable ? SDL_GetDisplayUsableBounds(id, &rc) : SDL_GetDisplayBounds(id, &rc)) ||
        rc.w <= 0 || rc.h <= 0 || (int64_t)rc.x + rc.w > INT_MAX || (int64_t)rc.y + rc.h > INT_MAX) return FALSE;
    SetRect(lprc, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h);
    return TRUE;
}

static BOOL FVirtualRect(LPRECT lprc, int *lpc) {
    SDL_DisplayID *rgid;
    SDL_Rect rc;
    int c, i;
    int64_t xMin = INT_MAX, yMin = INT_MAX, xMax = INT_MIN, yMax = INT_MIN;
    BOOL fOk = TRUE;
    if (!FCompatVideo()) return FALSE;
    rgid = SDL_GetDisplays(&c);
    if (!rgid || c <= 0) { SDL_free(rgid); return FALSE; }
    for (i = 0; i < c; i++) {
        if (!SDL_GetDisplayBounds(rgid[i], &rc) || rc.w <= 0 || rc.h <= 0) { fOk = FALSE; break; }
        xMin = min(xMin, rc.x); yMin = min(yMin, rc.y);
        xMax = max(xMax, (int64_t)rc.x + rc.w); yMax = max(yMax, (int64_t)rc.y + rc.h);
    }
    SDL_free(rgid);
    if (!fOk || xMax > INT_MAX || yMax > INT_MAX || xMax - xMin > INT_MAX || yMax - yMin > INT_MAX) return FALSE;
    SetRect(lprc, (int)xMin, (int)yMin, (int)xMax, (int)yMax); *lpc = c;
    return TRUE;
}

int WINAPI GetSystemMetrics(int i) {
    RECT rc;
    int c;
    if (!SDL_IsMainThread()) return 0;
    switch (i) {
    case SM_CXSCREEN: case SM_CYSCREEN:
        if (!FDisplayRect(FALSE, &rc)) return 0;
        return i == SM_CXSCREEN ? rc.right - rc.left : rc.bottom - rc.top;
    case SM_XVIRTUALSCREEN: case SM_YVIRTUALSCREEN: case SM_CXVIRTUALSCREEN: case SM_CYVIRTUALSCREEN: case SM_CMONITORS:
        if (!FVirtualRect(&rc, &c)) return 0;
        if (i == SM_XVIRTUALSCREEN) return rc.left;
        if (i == SM_YVIRTUALSCREEN) return rc.top;
        if (i == SM_CXVIRTUALSCREEN) return rc.right - rc.left;
        if (i == SM_CYVIRTUALSCREEN) return rc.bottom - rc.top;
        return c;
    case SM_CXBORDER: case SM_CYBORDER: return 1;
    case SM_CXDLGFRAME: case SM_CYDLGFRAME: return 3;
    case SM_CXFRAME: case SM_CYFRAME: return 8;
    case SM_CYCAPTION: return 23;
    case SM_CXVSCROLL: case SM_CYHSCROLL: return 17;
    case SM_CXICON: case SM_CYICON: case SM_CXCURSOR: case SM_CYCURSOR: return 32;
    case SM_CXDRAG: case SM_CYDRAG: return 4;
    default: return 0;
    }
}

BOOL WINAPI SystemParametersInfoA(UINT action, UINT param, PVOID lpv, UINT flags) {
    RECT rc;
    (void)param; (void)flags;
    if (!SDL_IsMainThread() || !lpv) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    switch (action) {
    case SPI_GETWORKAREA:
        if (!FDisplayRect(TRUE, &rc)) return FALSE;
        *(RECT *)lpv = rc; return TRUE;
    case SPI_GETWHEELSCROLLLINES: case SPI_GETWHEELSCROLLCHARS:
        /* SDL exposes wheel deltas, but no portable desktop line/character preference. */
        *(UINT *)lpv = 3; return TRUE;
    default: SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
}

DWORD WINAPI GetWindowThreadProcessId(HWND hwnd, LPDWORD lpdwProcess) {
    if (!LpCompatWindow(hwnd)) { SetLastError(ERROR_INVALID_WINDOW_HANDLE); return 0; }
    if (lpdwProcess) *lpdwProcess = (DWORD)getpid();
    return GetCurrentThreadId();
}
