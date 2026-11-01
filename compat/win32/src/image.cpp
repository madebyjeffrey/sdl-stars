/* Immutable module images and owned copies; resource reads stay bounded. */
#include "internal.h"
#include "input_io.h"
#include "render_io.h"
#include "resourcecodec.h"
#include "ownership.h"
#include <new>
#include <cstdlib>
#include <cstring>

void FreeCompatSurface(SDL_Surface *lp) noexcept { CompatRenderIO().pfnDestroySurface(lp); }
void FreeCompatCursor(SDL_Cursor *lp) noexcept { SDL_DestroyCursor(lp); }

static COMPATIMAGE *lpImages, *lpCursor;
static HWND hwndMouse;
static POINT ptMouse;
static WORD WImage(const BYTE *rgb) { return static_cast<WORD>((rgb[0] | (rgb[1] << 8))); }
COMPATIMAGE *LpCompatImage(HICON hicon) {
    COMPATIMAGE *lp;
    if (!SDL_IsMainThread()) return nullptr;
    for (lp = lpImages; lp; lp = lp->lpNext)
        if (reinterpret_cast<HICON>(lp) == hicon) return lp->fAlive ? lp : nullptr;
    return nullptr;
}
static COMPATIMAGE *LpNewImage(SDL_Surface *lpXor, BYTE *rgbAnd, BOOL fCursor) {
    COMPATIMAGE *lp = new (std::nothrow) COMPATIMAGE{};
    if (!lp) return nullptr;
    lp->lpXor.reset(lpXor); lp->rgbAnd.reset(rgbAnd); lp->fCursor = fCursor; lp->fAlive = 1;
    lp->lpNext = lpImages; lpImages = lp;
    return lp;
}
HICON HiconCompatResource(const BYTE *rgb, std::size_t cb, BOOL fCursor) {
    BITMAPINFOHEADER bi;
    COMPATDIB dib;
    COMPATIMAGE *lp;
    SDL_Surface *lpSurface;
    BYTE rgInfo[cbWireDibHeader + 256 * cbWireRgbQuad], *rgAnd;
    const BYTE *lpAnd;
    std::size_t cbInfo, cbMaskRow, cbMask, cColors;
    int x, y;
    UINT xHot = 0, yHot = 0;
    BYTE *lpPixel;
    BOOL fInvert = 0;
    NATIVEOWNER<BYTE, std::free> rgbAndOwner;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;

    if (!SDL_IsMainThread() || !rgb) return nullptr;
    if (fCursor) {
        if (cb < 4) return nullptr;
        xHot = WImage(rgb); yHot = WImage(rgb + 2); rgb += 4; cb -= 4;
    }
    if (cb < cbWireDibHeader) return nullptr;
    FDecodeDibHeader(std::as_bytes(std::span(rgb, cbWireDibHeader)), bi);
    if (bi.biSize != cbWireDibHeader || bi.biHeight <= 0 || bi.biHeight % 2 || bi.biWidth <= 0 || bi.biWidth > 256 || bi.biHeight > 512)
        return nullptr;
    bi.biHeight /= 2;
    if (fCursor && (xHot >= static_cast<UINT>(bi.biWidth) || yHot >= static_cast<UINT>(bi.biHeight))) return nullptr;
    cColors = bi.biClrUsed ? bi.biClrUsed : (bi.biBitCount <= 8 ? static_cast<std::size_t>(1) << bi.biBitCount : 0);
    if (cColors > 256) return nullptr;
    cbInfo = cbWireDibHeader + cColors * cbWireRgbQuad;
    if (cbInfo > cb) return nullptr;
    std::memcpy(rgInfo, rgb, cbInfo); FEncodeDibHeader(bi, std::as_writable_bytes(std::span(rgInfo)));
    if (!FCompatDib(rgInfo, cbInfo, rgb + cbInfo, cb - cbInfo, &dib)) return nullptr;
    cbMaskRow = ((static_cast<std::size_t>(dib.cx) + 31) / 32) * 4; cbMask = cbMaskRow * static_cast<std::size_t>(dib.cy);
    if (dib.cbBits > cb - cbInfo || cbMask > cb - cbInfo - dib.cbBits) return nullptr;
    lpAnd = rgb + cbInfo + dib.cbBits;
    lpsurfaceOwner.reset(LpCompatDibSurface(&dib)); lpSurface = lpsurfaceOwner.get();
    if (!lpSurface) return nullptr;
    rgbAndOwner.reset(static_cast<BYTE *>(std::malloc(static_cast<std::size_t>(dib.cx) * dib.cy))); rgAnd = rgbAndOwner.get();
    if (!rgAnd) return nullptr;
    for (y = 0; y < dib.cy; y++) for (x = 0; x < dib.cx; x++) {
        rgAnd[y * dib.cx + x] = static_cast<BYTE>(((lpAnd[static_cast<std::size_t>((dib.cy - 1 - y)) * cbMaskRow + x / 8] >> (7 - x % 8)) & 1));
        lpPixel = static_cast<BYTE *>(lpSurface->pixels) + static_cast<std::size_t>(y) * lpSurface->pitch + x * 4;
        if (rgAnd[y * dib.cx + x] && (lpPixel[0] || lpPixel[1] || lpPixel[2])) fInvert = 1;
    }
    lp = LpNewImage(lpSurface, rgAnd, fCursor);
    if (!lp) return nullptr;
    lpsurfaceOwner.release(); rgbAndOwner.release();
    lp->xHot = xHot; lp->yHot = yHot; lp->fMono = bi.biBitCount == 1; lp->fInvert = fInvert;
    return reinterpret_cast<HICON>(lp);
}
static HICON HimageLoad(HINSTANCE hInstance, LPCSTR psz, BOOL fCursor) {
    HRSRC hgroup, hrsrc;
    const BYTE *rgb;
    DWORD cb;
    WORD count, id;
    COMPATIMAGE *lp;
    HICON h;
    if (!hInstance || !psz) return nullptr;
    hgroup = win32::FindResourceA(hInstance, psz, fCursor ? win32::RT_GROUP_CURSOR : win32::RT_GROUP_ICON);
    rgb = static_cast<const BYTE *>((win32::LockResource(win32::LoadResource(hInstance, hgroup)))); cb = win32::SizeofResource(hInstance, hgroup);
    if (!rgb || cb < 20 || WImage(rgb) || WImage(rgb + 2) != (fCursor ? 2 : 1)) return nullptr;
    count = WImage(rgb + 4);
    if (count == 0 || static_cast<std::size_t>(count) * 14 > cb - 6) return nullptr;
    // All canonical images are 32x32. Group entries retain their original IDs.
    id = WImage(rgb + 18);
    hrsrc = win32::FindResourceA(hInstance, win32::MAKEINTRESOURCE(id), fCursor ? win32::RT_CURSOR : win32::RT_ICON);
    if (!hrsrc) return nullptr;
    for (lp = lpImages; lp; lp = lp->lpNext) if (lp->hrsrc == hrsrc && lp->fCursor == fCursor) return reinterpret_cast<HICON>(lp);
    rgb = static_cast<const BYTE *>((win32::LockResource(win32::LoadResource(hInstance, hrsrc)))); cb = win32::SizeofResource(hInstance, hrsrc);
    h = HiconCompatResource(rgb, cb, fCursor);
    if (h) {
        lp = LpCompatImage(h); lp->fShared = 1; lp->hrsrc = hrsrc;
        lp->fAppIcon = !fCursor && reinterpret_cast<UINT_PTR>(psz) > UINT16_MAX &&
            (!SDL_strcasecmp(psz, "StarsIco") || !SDL_strcasecmp(psz, "APPICON"));
    }
    return h;
}
HICON win32::LoadIconA(HINSTANCE hInstance, LPCSTR psz) { return HimageLoad(hInstance, psz, 0); }
static SDL_SystemCursor IdSystemCursor(UINT id) {
    switch (id) {
    case 32512: return SDL_SYSTEM_CURSOR_DEFAULT;
    case 32513: return SDL_SYSTEM_CURSOR_TEXT;
    case 32514: return SDL_SYSTEM_CURSOR_WAIT;
    case 32515: return SDL_SYSTEM_CURSOR_CROSSHAIR;
    case 32516: case 32646: return SDL_SYSTEM_CURSOR_MOVE;
    case 32642: return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
    case 32643: return SDL_SYSTEM_CURSOR_NESW_RESIZE;
    case 32644: return SDL_SYSTEM_CURSOR_EW_RESIZE;
    case 32645: return SDL_SYSTEM_CURSOR_NS_RESIZE;
    case 32648: return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
    case 32649: return SDL_SYSTEM_CURSOR_POINTER;
    case 32650: return SDL_SYSTEM_CURSOR_PROGRESS;
    default: return SDL_SYSTEM_CURSOR_COUNT;
    }
}
HCURSOR win32::LoadCursorA(HINSTANCE hInstance, LPCSTR psz) {
    COMPATIMAGE *lp;
    UINT_PTR id = reinterpret_cast<UINT_PTR>(psz);
    if (!FCompatVideo()) return nullptr;
    if (hInstance) return static_cast<HCURSOR>(HimageLoad(hInstance, psz, 1));
    if (id > UINT16_MAX || IdSystemCursor(static_cast<UINT>(id)) == SDL_SYSTEM_CURSOR_COUNT) return nullptr;
    for (lp = lpImages; lp; lp = lp->lpNext) if (lp->idSystem == id) return reinterpret_cast<HCURSOR>(lp);
    lp = LpNewImage(nullptr, nullptr, 1);
    if (!lp) return nullptr;
    lp->fShared = 1; lp->idSystem = static_cast<UINT>(id);
    return reinterpret_cast<HCURSOR>(lp);
}
HICON win32::CopyIcon(HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon), *lpNew;
    SDL_Surface *lpXor;
    BYTE *rgAnd;
    std::size_t cb;
    NATIVEOWNER<BYTE, std::free> rgbAndOwner;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    if (!lp || !lp->lpXor) return nullptr;
    cb = static_cast<std::size_t>(lp->lpXor->w) * lp->lpXor->h;
    lpsurfaceOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpXor = lpsurfaceOwner.get();
    rgbAndOwner.reset(static_cast<BYTE *>(std::malloc(cb))); rgAnd = rgbAndOwner.get();
    if (!lpXor || !rgAnd) return nullptr;
    std::memcpy(rgAnd, lp->rgbAnd.get(), cb);
    lpNew = LpNewImage(lpXor, rgAnd, lp->fCursor);
    if (!lpNew) return nullptr;
    lpsurfaceOwner.release(); rgbAndOwner.release();
    lpNew->fMono = lp->fMono; lpNew->fInvert = lp->fInvert; lpNew->xHot = lp->xHot; lpNew->yHot = lp->yHot;
    return reinterpret_cast<HICON>(lpNew);
}
static BOOL FDestroyImage(HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    if (!lp) return 0;
    if (lp->fShared) return 1; // LoadIcon/LoadCursor handles belong to the module.
    if (lpCursor == lp) win32::SetCursor(nullptr);
    lp->lpCursor.reset(); lp->lpXor.reset(); lp->rgbAnd.reset(); lp->fAlive = 0;
    return 1;
}
BOOL win32::DestroyIcon(HICON hicon) { return FDestroyImage(hicon); }
BOOL win32::DestroyCursor(HCURSOR hcursor) { return FDestroyImage(static_cast<HICON>(hcursor)); }
BOOL win32::GetIconInfo(HICON hicon, PICONINFO lpii) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    ICONINFO ii = {};
    SDL_Surface *lpMask;
    BYTE *rgb, *src;
    int x, y, dy;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpmaskOwner;
    if (!lp || !lp->lpXor || !lpii) return 0;
    dy = lp->lpXor->h;
    lpmaskOwner.reset(SDL_CreateSurface(lp->lpXor->w, dy * (lp->fMono ? 2 : 1), SDL_PIXELFORMAT_RGBA32)); lpMask = lpmaskOwner.get();
    if (!lpMask) return 0;
    for (y = 0; y < lpMask->h; y++) for (x = 0; x < lpMask->w; x++) {
        rgb = static_cast<BYTE *>(lpMask->pixels) + static_cast<std::size_t>(y) * lpMask->pitch + x * 4;
        src = static_cast<BYTE *>(lp->lpXor->pixels) + static_cast<std::size_t>((y % dy)) * lp->lpXor->pitch + x * 4;
        rgb[0] = rgb[1] = rgb[2] = y < dy ? (lp->rgbAnd[y * lpMask->w + x] ? 255 : 0) : src[0]; rgb[3] = 255;
    }
    ii.fIcon = !lp->fCursor; ii.xHotspot = lp->fCursor ? lp->xHot : static_cast<UINT>(lp->lpXor->w) / 2;
    ii.yHotspot = lp->fCursor ? lp->yHot : static_cast<UINT>(dy) / 2;
    ii.hbmMask = HbmpCompatSurface(lpMask, 1); lpmaskOwner.reset();
    if (!lp->fMono) ii.hbmColor = HbmpCompatSurface(lp->lpXor.get(), 0);
    if (!ii.hbmMask || (!lp->fMono && !ii.hbmColor)) {
        if (ii.hbmMask) win32::DeleteObject(ii.hbmMask);
        if (ii.hbmColor) win32::DeleteObject(ii.hbmColor);
        return 0;
    }
    *lpii = ii; return 1;
}

static BOOL FNativeCursor(COMPATIMAGE *lp) {
    BYTE *rg, *data, *mask, *src;
    SDL_Surface *lpColor;
    std::size_t cb;
    int x, y;
    BOOL fAnd, fWhite;
    NATIVEOWNER<BYTE, std::free> lpmaskOwner;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpcolorOwner;
    if (lp->lpCursor) return 1;
    if (lp->idSystem) lp->lpCursor.reset(SDL_CreateSystemCursor(IdSystemCursor(lp->idSystem)));
    else if (lp->fMono && lp->lpXor->w % 8 == 0) {
        cb = static_cast<std::size_t>((lp->lpXor->w / 8)) * lp->lpXor->h;
        lpmaskOwner.reset(static_cast<BYTE *>(std::calloc(2, cb))); rg = lpmaskOwner.get();
        if (!rg) return 0;
        data = rg; mask = rg + cb;
        for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
            fAnd = lp->rgbAnd[y * lp->lpXor->w + x] != 0;
            src = static_cast<BYTE *>(lp->lpXor->pixels) + static_cast<std::size_t>(y) * lp->lpXor->pitch + x * 4;
            fWhite = src[0] != 0;
            if (fAnd ? fWhite : !fWhite) data[y * (lp->lpXor->w / 8) + x / 8] |= static_cast<BYTE>((0x80 >> (x % 8)));
            if (!fAnd) mask[y * (lp->lpXor->w / 8) + x / 8] |= static_cast<BYTE>((0x80 >> (x % 8)));
        }
        lp->lpCursor.reset(SDL_CreateCursor(data, mask, lp->lpXor->w, lp->lpXor->h, lp->xHot, lp->yHot));
        lpmaskOwner.reset();
    } else {
        lpcolorOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpColor = lpcolorOwner.get();
        if (!lpColor) return 0;
        for (y = 0; y < lpColor->h; y++) for (x = 0; x < lpColor->w; x++)
            (static_cast<BYTE *>(lpColor->pixels))[static_cast<std::size_t>(y) * lpColor->pitch + x * 4 + 3] = lp->rgbAnd[y * lpColor->w + x] ? 0 : 255;
        lp->lpCursor.reset(SDL_CreateColorCursor(lpColor, lp->xHot, lp->yHot));
        lpcolorOwner.reset();
    }
    return lp->lpCursor != nullptr;
}
HCURSOR win32::GetCursor(void) { return SDL_IsMainThread() ? reinterpret_cast<HCURSOR>(lpCursor) : nullptr; }
HCURSOR win32::SetCursor(HCURSOR hcursor) {
    COMPATIMAGE *lp = LpCompatImage(static_cast<HICON>(hcursor)), *lpOld = lpCursor;
    const char *pszDriver;
    if (!SDL_IsMainThread() || (hcursor && (!lp || !lp->fCursor)) || !FCompatVideo()) return nullptr;
    pszDriver = CompatInputIO().pfnGetCurrentVideoDriver();
    if (!lp || lp->fInvert) SDL_HideCursor();
    else {
        if (!FNativeCursor(lp) && (!pszDriver || std::strcmp(pszDriver, "dummy") != 0)) return nullptr;
        if (lp->lpCursor && !SDL_SetCursor(lp->lpCursor.get())) return nullptr;
        SDL_ShowCursor();
    }
    lpCursor = lp;
    if (LpCompatWindow(hwndMouse)) FCompatPresent(hwndMouse);
    return reinterpret_cast<HCURSOR>(lpOld);
}
void CompatCursorPoint(HWND hwnd, int x, int y) {
    HWND hwndOld = hwndMouse;
    hwndMouse = hwnd; ptMouse = POINT{x, y};
    if (lpCursor && lpCursor->fInvert) {
        if (hwndOld != hwnd && LpCompatWindow(hwndOld)) FCompatPresent(hwndOld);
        FCompatPresent(hwnd);
    }
}
void CompatCursorLeave(HWND hwnd) {
    if (hwndMouse != hwnd) return;
    hwndMouse = nullptr;
    if (lpCursor && lpCursor->fInvert) FCompatPresent(hwnd);
}
void CompatDrawCursor(HWND hwnd, SDL_Surface *lpSurface) {
    COMPATIMAGE *lp = lpCursor;
    BYTE *src, *dst;
    int x, y, i, ix, iy, iScale = StarsMax(1, hwnd->iCanvasScale);
    std::int64_t dx, dy;
    if (!lp || !lp->fInvert || hwnd != hwndMouse) return;
    for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
        dx = (static_cast<std::int64_t>(ptMouse.x) - lp->xHot + x) * iScale; dy = (static_cast<std::int64_t>(ptMouse.y) - lp->yHot + y) * iScale;
        if (dx < 0 || dy < 0 || dx + iScale > lpSurface->w || dy + iScale > lpSurface->h) continue;
        src = static_cast<BYTE *>(lp->lpXor->pixels) + static_cast<std::size_t>(y) * lp->lpXor->pitch + x * 4;
        for (iy = 0; iy < iScale; iy++) for (ix = 0; ix < iScale; ix++) {
            dst = static_cast<BYTE *>(lpSurface->pixels) + static_cast<std::size_t>((dy + iy)) * lpSurface->pitch + (dx + ix) * 4;
            for (i = 0; i < 3; i++) dst[i] = static_cast<BYTE>(((lp->rgbAnd[y * lp->lpXor->w + x] ? dst[i] : 0) ^ src[i]));
        }
    }
}
void CompatShutdownImages(void) {
    COMPATIMAGE *lp, *lpNext;
    if (SDL_WasInit(SDL_INIT_VIDEO)) { SDL_SetCursor(SDL_GetDefaultCursor()); SDL_ShowCursor(); }
    lpCursor = nullptr; hwndMouse = nullptr;
    for (lp = lpImages; lp; lp = lpNext) {
        lpNext = lp->lpNext; delete lp;
    }
    lpImages = nullptr;
}

void CompatSetWindowIcon(SDL_Window *lpWindow, HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    SDL_Surface *lpIcon;
    SDL_IOStream *lpStream;
    int x, y;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpiconOwner;
    if (!lp || !lp->lpXor) return;
    // The desktop uses the high-resolution artwork; DrawIcon keeps the original game pixels.
    if (lp->fAppIcon) {
        lpStream = SDL_IOFromConstMem(rgbCompatAppIcon, cbCompatAppIcon);
        lpiconOwner.reset(lpStream ? SDL_LoadPNG_IO(lpStream, true) : nullptr); lpIcon = lpiconOwner.get();
        if (lpIcon) {
            SDL_SetWindowIcon(lpWindow, lpIcon);
            return;
        }
    }
    lpiconOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpIcon = lpiconOwner.get();
    if (!lpIcon) return;
    for (y = 0; y < lpIcon->h; y++) for (x = 0; x < lpIcon->w; x++)
        (static_cast<BYTE *>(lpIcon->pixels))[static_cast<std::size_t>(y) * lpIcon->pitch + x * 4 + 3] = lp->rgbAnd[y * lpIcon->w + x] ? 0 : 255;
    SDL_SetWindowIcon(lpWindow, lpIcon);
}
