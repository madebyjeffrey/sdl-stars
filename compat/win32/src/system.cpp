/* SDL display coordinates plus classic UI metrics for the source wrapper. */
#include "internal.h"
#include "display_io.h"
#include <limits>
#include <unistd.h>

static std::optional<RECT> FDisplayRect(BOOL fUsable) {
    SDL_Rect rc;
    SDL_DisplayID id;
    RECT result;
    if (!FCompatVideo()) return std::nullopt;
    id = CompatDisplayIO().pfnGetPrimaryDisplay();
    if (!id || !(fUsable ? CompatDisplayIO().pfnGetDisplayUsableBounds(id, &rc) : CompatDisplayIO().pfnGetDisplayBounds(id, &rc)) ||
        rc.w <= 0 || rc.h <= 0 || static_cast<std::int64_t>(rc.x) + rc.w > std::numeric_limits<int>::max() || static_cast<std::int64_t>(rc.y) + rc.h > std::numeric_limits<int>::max()) return std::nullopt;
    win32::SetRect(&result, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h);
    return result;
}

struct VIRTUALDISPLAYRESULT { RECT rc; int c; };
static std::optional<VIRTUALDISPLAYRESULT> FVirtualRect() {
    SDL_DisplayID *rgid;
    SDL_Rect rc;
    int c, i;
    std::int64_t xMin = std::numeric_limits<int>::max(), yMin = std::numeric_limits<int>::max(), xMax = std::numeric_limits<int>::min(), yMax = std::numeric_limits<int>::min();
    BOOL fOk = 1;
    if (!FCompatVideo()) return std::nullopt;
    rgid = CompatDisplayIO().pfnGetDisplays(&c);
    NATIVEARRAYOWNER<SDL_DisplayID, SDL_free> rgidOwner(rgid);
    if (!rgid || c <= 0) return std::nullopt;
    for (i = 0; i < c; i++) {
        if (!CompatDisplayIO().pfnGetDisplayBounds(rgid[i], &rc) || rc.w <= 0 || rc.h <= 0) { fOk = 0; break; }
        xMin = StarsMin(xMin, rc.x); yMin = StarsMin(yMin, rc.y);
        xMax = (((xMax) > (static_cast<std::int64_t>(rc.x) + rc.w)) ? (xMax) : (static_cast<std::int64_t>(rc.x) + rc.w)); yMax = (((yMax) > (static_cast<std::int64_t>(rc.y) + rc.h)) ? (yMax) : (static_cast<std::int64_t>(rc.y) + rc.h));
    }
    rgidOwner.reset();
    if (!fOk || xMax > std::numeric_limits<int>::max() || yMax > std::numeric_limits<int>::max() || xMax - xMin > std::numeric_limits<int>::max() || yMax - yMin > std::numeric_limits<int>::max()) return std::nullopt;
    VIRTUALDISPLAYRESULT result{};
    win32::SetRect(&result.rc, static_cast<int>(xMin), static_cast<int>(yMin), static_cast<int>(xMax), static_cast<int>(yMax));
    result.c = c;
    return result;
}

int win32::GetSystemMetrics(int i) {
    std::optional<RECT> rc;
    std::optional<VIRTUALDISPLAYRESULT> virtualDisplay;
    if (!SDL_IsMainThread()) return 0;
    switch (i) {
    case win32::SM_CXSCREEN: case win32::SM_CYSCREEN:
        rc = FDisplayRect(0);
        if (!rc) return 0;
        return i == win32::SM_CXSCREEN ? rc->right - rc->left : rc->bottom - rc->top;
    case win32::SM_XVIRTUALSCREEN: case win32::SM_YVIRTUALSCREEN: case win32::SM_CXVIRTUALSCREEN: case win32::SM_CYVIRTUALSCREEN: case win32::SM_CMONITORS:
        virtualDisplay = FVirtualRect();
        if (!virtualDisplay) return 0;
        if (i == win32::SM_XVIRTUALSCREEN) return virtualDisplay->rc.left;
        if (i == win32::SM_YVIRTUALSCREEN) return virtualDisplay->rc.top;
        if (i == win32::SM_CXVIRTUALSCREEN) return virtualDisplay->rc.right - virtualDisplay->rc.left;
        if (i == win32::SM_CYVIRTUALSCREEN) return virtualDisplay->rc.bottom - virtualDisplay->rc.top;
        return virtualDisplay->c;
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
    std::optional<RECT> rc;
    static_cast<void>(param); static_cast<void>(flags);
    if (!SDL_IsMainThread() || !lpv) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    switch (action) {
    case win32::SPI_GETWORKAREA:
        rc = FDisplayRect(1);
        if (!rc) return 0;
        *static_cast<RECT *>(lpv) = *rc; return 1;
    case win32::SPI_GETWHEELSCROLLLINES: case win32::SPI_GETWHEELSCROLLCHARS:
        /* SDL exposes wheel deltas, but no portable desktop line/character preference. */
        *static_cast<UINT *>(lpv) = 3; return 1;
    default: win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
    }
}

DWORD win32::GetWindowThreadProcessId(HWND hwnd, LPDWORD lpdwProcess) {
    if (!LpCompatWindow(hwnd)) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0; }
    if (lpdwProcess) *lpdwProcess = static_cast<DWORD>(getpid());
    return win32::GetCurrentThreadId();
}
