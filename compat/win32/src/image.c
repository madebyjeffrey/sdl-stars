/* Immutable module images and owned copies; resource reads stay bounded. */
#include "internal.h"
#include "ownership.h"
#include <new>
#include <stdlib.h>
#include <string.h>

static COMPATIMAGE *lpImages, *lpCursor;
static HWND hwndMouse;
static POINT ptMouse;
static WORD WImage(const BYTE *rgb) { return (WORD)(rgb[0] | (rgb[1] << 8)); }
COMPATIMAGE *LpCompatImage(HICON hicon) {
    COMPATIMAGE *lp;
    if (!SDL_IsMainThread()) return NULL;
    for (lp = lpImages; lp; lp = lp->lpNext)
        if ((HICON)lp == hicon) return lp->fAlive ? lp : NULL;
    return NULL;
}
static COMPATIMAGE *LpNewImage(SDL_Surface *lpXor, BYTE *rgbAnd, BOOL fCursor) {
    COMPATIMAGE *lp = new (std::nothrow) COMPATIMAGE{};
    if (!lp) return NULL;
    lp->lpXor.reset(lpXor); lp->rgbAnd.reset(rgbAnd); lp->fCursor = fCursor; lp->fAlive = TRUE;
    lp->lpNext = lpImages; lpImages = lp;
    return lp;
}
HICON HiconCompatResource(const BYTE *rgb, size_t cb, BOOL fCursor) {
    BITMAPINFOHEADER bi;
    COMPATDIB dib;
    COMPATIMAGE *lp;
    SDL_Surface *lpSurface;
    BYTE rgInfo[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)], *rgAnd;
    const BYTE *lpAnd;
    size_t cbInfo, cbMaskRow, cbMask, cColors;
    int x, y;
    UINT xHot = 0, yHot = 0;
    BYTE *lpPixel;
    BOOL fInvert = FALSE;
    NATIVEOWNER<BYTE, free> rgbAndOwner;
    NATIVEOWNER<SDL_Surface, SDL_DestroySurface> lpsurfaceOwner;

    if (!SDL_IsMainThread() || !rgb) return NULL;
    if (fCursor) {
        if (cb < 4) return NULL;
        xHot = WImage(rgb); yHot = WImage(rgb + 2); rgb += 4; cb -= 4;
    }
    if (cb < sizeof(bi)) return NULL;
    memcpy(&bi, rgb, sizeof(bi));
    if (bi.biSize != sizeof(bi) || bi.biHeight <= 0 || bi.biHeight % 2 || bi.biWidth <= 0 || bi.biWidth > 256 || bi.biHeight > 512)
        return NULL;
    bi.biHeight /= 2;
    if (fCursor && (xHot >= (UINT)bi.biWidth || yHot >= (UINT)bi.biHeight)) return NULL;
    cColors = bi.biClrUsed ? bi.biClrUsed : (bi.biBitCount <= 8 ? (size_t)1 << bi.biBitCount : 0);
    if (cColors > 256) return NULL;
    cbInfo = sizeof(bi) + cColors * sizeof(RGBQUAD);
    if (cbInfo > cb) return NULL;
    memcpy(rgInfo, rgb, cbInfo); memcpy(rgInfo, &bi, sizeof(bi));
    if (!FCompatDib(rgInfo, cbInfo, rgb + cbInfo, cb - cbInfo, &dib)) return NULL;
    cbMaskRow = (((size_t)dib.cx + 31) / 32) * 4; cbMask = cbMaskRow * (size_t)dib.cy;
    if (dib.cbBits > cb - cbInfo || cbMask > cb - cbInfo - dib.cbBits) return NULL;
    lpAnd = rgb + cbInfo + dib.cbBits;
    lpsurfaceOwner.reset(LpCompatDibSurface(&dib)); lpSurface = lpsurfaceOwner.get();
    if (!lpSurface) return NULL;
    rgbAndOwner.reset((BYTE *)malloc((size_t)dib.cx * dib.cy)); rgAnd = rgbAndOwner.get();
    if (!rgAnd) return NULL;
    for (y = 0; y < dib.cy; y++) for (x = 0; x < dib.cx; x++) {
        rgAnd[y * dib.cx + x] = (BYTE)((lpAnd[(size_t)(dib.cy - 1 - y) * cbMaskRow + x / 8] >> (7 - x % 8)) & 1);
        lpPixel = (BYTE *)lpSurface->pixels + (size_t)y * lpSurface->pitch + x * 4;
        if (rgAnd[y * dib.cx + x] && (lpPixel[0] || lpPixel[1] || lpPixel[2])) fInvert = TRUE;
    }
    lp = LpNewImage(lpSurface, rgAnd, fCursor);
    if (!lp) return NULL;
    lpsurfaceOwner.release(); rgbAndOwner.release();
    lp->xHot = xHot; lp->yHot = yHot; lp->fMono = bi.biBitCount == 1; lp->fInvert = fInvert;
    return (HICON)lp;
}
static HICON HimageLoad(HINSTANCE hInstance, LPCSTR psz, BOOL fCursor) {
    HRSRC hgroup, hrsrc;
    const BYTE *rgb;
    DWORD cb;
    WORD count, id;
    COMPATIMAGE *lp;
    HICON h;
    if (!hInstance || !psz) return NULL;
    hgroup = FindResourceA(hInstance, psz, fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON);
    rgb = (const BYTE *)(LockResource(LoadResource(hInstance, hgroup))); cb = SizeofResource(hInstance, hgroup);
    if (!rgb || cb < 20 || WImage(rgb) || WImage(rgb + 2) != (fCursor ? 2 : 1)) return NULL;
    count = WImage(rgb + 4);
    if (count == 0 || (size_t)count * 14 > cb - 6) return NULL;
    // All canonical images are 32x32. Group entries retain their original IDs.
    id = WImage(rgb + 18);
    hrsrc = FindResourceA(hInstance, MAKEINTRESOURCE(id), fCursor ? RT_CURSOR : RT_ICON);
    if (!hrsrc) return NULL;
    for (lp = lpImages; lp; lp = lp->lpNext) if (lp->hrsrc == hrsrc && lp->fCursor == fCursor) return (HICON)lp;
    rgb = (const BYTE *)(LockResource(LoadResource(hInstance, hrsrc))); cb = SizeofResource(hInstance, hrsrc);
    h = HiconCompatResource(rgb, cb, fCursor);
    if (h) {
        lp = LpCompatImage(h); lp->fShared = TRUE; lp->hrsrc = hrsrc;
        lp->fAppIcon = !fCursor && (UINT_PTR)psz > UINT16_MAX &&
            (!SDL_strcasecmp(psz, "StarsIco") || !SDL_strcasecmp(psz, "APPICON"));
    }
    return h;
}
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR psz) { return HimageLoad(hInstance, psz, FALSE); }
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
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR psz) {
    COMPATIMAGE *lp;
    UINT_PTR id = (UINT_PTR)psz;
    if (!FCompatVideo()) return NULL;
    if (hInstance) return (HCURSOR)HimageLoad(hInstance, psz, TRUE);
    if (id > UINT16_MAX || IdSystemCursor((UINT)id) == SDL_SYSTEM_CURSOR_COUNT) return NULL;
    for (lp = lpImages; lp; lp = lp->lpNext) if (lp->idSystem == id) return (HCURSOR)lp;
    lp = LpNewImage(NULL, NULL, TRUE);
    if (!lp) return NULL;
    lp->fShared = TRUE; lp->idSystem = (UINT)id;
    return (HCURSOR)lp;
}
HICON WINAPI CopyIcon(HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon), *lpNew;
    SDL_Surface *lpXor;
    BYTE *rgAnd;
    size_t cb;
    NATIVEOWNER<BYTE, free> rgbAndOwner;
    NATIVEOWNER<SDL_Surface, SDL_DestroySurface> lpsurfaceOwner;
    if (!lp || !lp->lpXor) return NULL;
    cb = (size_t)lp->lpXor->w * lp->lpXor->h;
    lpsurfaceOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpXor = lpsurfaceOwner.get();
    rgbAndOwner.reset((BYTE *)malloc(cb)); rgAnd = rgbAndOwner.get();
    if (!lpXor || !rgAnd) return NULL;
    memcpy(rgAnd, lp->rgbAnd.get(), cb);
    lpNew = LpNewImage(lpXor, rgAnd, lp->fCursor);
    if (!lpNew) return NULL;
    lpsurfaceOwner.release(); rgbAndOwner.release();
    lpNew->fMono = lp->fMono; lpNew->fInvert = lp->fInvert; lpNew->xHot = lp->xHot; lpNew->yHot = lp->yHot;
    return (HICON)lpNew;
}
static BOOL FDestroyImage(HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    if (!lp) return FALSE;
    if (lp->fShared) return TRUE; // LoadIcon/LoadCursor handles belong to the module.
    if (lpCursor == lp) SetCursor(NULL);
    lp->lpCursor.reset(); lp->lpXor.reset(); lp->rgbAnd.reset(); lp->fAlive = FALSE;
    return TRUE;
}
BOOL WINAPI DestroyIcon(HICON hicon) { return FDestroyImage(hicon); }
BOOL WINAPI DestroyCursor(HCURSOR hcursor) { return FDestroyImage((HICON)hcursor); }
BOOL WINAPI GetIconInfo(HICON hicon, PICONINFO lpii) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    ICONINFO ii = {};
    SDL_Surface *lpMask;
    BYTE *rgb, *src;
    int x, y, dy;
    NATIVEOWNER<SDL_Surface, SDL_DestroySurface> lpmaskOwner;
    if (!lp || !lp->lpXor || !lpii) return FALSE;
    dy = lp->lpXor->h;
    lpmaskOwner.reset(SDL_CreateSurface(lp->lpXor->w, dy * (lp->fMono ? 2 : 1), SDL_PIXELFORMAT_RGBA32)); lpMask = lpmaskOwner.get();
    if (!lpMask) return FALSE;
    for (y = 0; y < lpMask->h; y++) for (x = 0; x < lpMask->w; x++) {
        rgb = (BYTE *)lpMask->pixels + (size_t)y * lpMask->pitch + x * 4;
        src = (BYTE *)lp->lpXor->pixels + (size_t)(y % dy) * lp->lpXor->pitch + x * 4;
        rgb[0] = rgb[1] = rgb[2] = y < dy ? (lp->rgbAnd[y * lpMask->w + x] ? 255 : 0) : src[0]; rgb[3] = 255;
    }
    ii.fIcon = !lp->fCursor; ii.xHotspot = lp->fCursor ? lp->xHot : (UINT)lp->lpXor->w / 2;
    ii.yHotspot = lp->fCursor ? lp->yHot : (UINT)dy / 2;
    ii.hbmMask = HbmpCompatSurface(lpMask, TRUE); lpmaskOwner.reset();
    if (!lp->fMono) ii.hbmColor = HbmpCompatSurface(lp->lpXor.get(), FALSE);
    if (!ii.hbmMask || (!lp->fMono && !ii.hbmColor)) {
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        return FALSE;
    }
    *lpii = ii; return TRUE;
}

static BOOL FNativeCursor(COMPATIMAGE *lp) {
    BYTE *rg, *data, *mask, *src;
    SDL_Surface *lpColor;
    size_t cb;
    int x, y;
    BOOL fAnd, fWhite;
    NATIVEOWNER<BYTE, free> lpmaskOwner;
    NATIVEOWNER<SDL_Surface, SDL_DestroySurface> lpcolorOwner;
    if (lp->lpCursor) return TRUE;
    if (lp->idSystem) lp->lpCursor.reset(SDL_CreateSystemCursor(IdSystemCursor(lp->idSystem)));
    else if (lp->fMono && lp->lpXor->w % 8 == 0) {
        cb = (size_t)(lp->lpXor->w / 8) * lp->lpXor->h;
        lpmaskOwner.reset((BYTE *)calloc(2, cb)); rg = lpmaskOwner.get();
        if (!rg) return FALSE;
        data = rg; mask = rg + cb;
        for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
            fAnd = lp->rgbAnd[y * lp->lpXor->w + x] != 0;
            src = (BYTE *)lp->lpXor->pixels + (size_t)y * lp->lpXor->pitch + x * 4;
            fWhite = src[0] != 0;
            if (fAnd ? fWhite : !fWhite) data[y * (lp->lpXor->w / 8) + x / 8] |= (BYTE)(0x80 >> (x % 8));
            if (!fAnd) mask[y * (lp->lpXor->w / 8) + x / 8] |= (BYTE)(0x80 >> (x % 8));
        }
        lp->lpCursor.reset(SDL_CreateCursor(data, mask, lp->lpXor->w, lp->lpXor->h, lp->xHot, lp->yHot));
        lpmaskOwner.reset();
    } else {
        lpcolorOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpColor = lpcolorOwner.get();
        if (!lpColor) return FALSE;
        for (y = 0; y < lpColor->h; y++) for (x = 0; x < lpColor->w; x++)
            ((BYTE *)lpColor->pixels)[(size_t)y * lpColor->pitch + x * 4 + 3] = lp->rgbAnd[y * lpColor->w + x] ? 0 : 255;
        lp->lpCursor.reset(SDL_CreateColorCursor(lpColor, lp->xHot, lp->yHot));
        lpcolorOwner.reset();
    }
    return lp->lpCursor != NULL;
}
HCURSOR WINAPI GetCursor(void) { return SDL_IsMainThread() ? (HCURSOR)lpCursor : NULL; }
HCURSOR WINAPI SetCursor(HCURSOR hcursor) {
    COMPATIMAGE *lp = LpCompatImage((HICON)hcursor), *lpOld = lpCursor;
    const char *pszDriver;
    if (!SDL_IsMainThread() || (hcursor && (!lp || !lp->fCursor)) || !FCompatVideo()) return NULL;
    pszDriver = SDL_GetCurrentVideoDriver();
    if (!lp || lp->fInvert) SDL_HideCursor();
    else {
        if (!FNativeCursor(lp) && (!pszDriver || strcmp(pszDriver, "dummy") != 0)) return NULL;
        if (lp->lpCursor && !SDL_SetCursor(lp->lpCursor.get())) return NULL;
        SDL_ShowCursor();
    }
    lpCursor = lp;
    if (LpCompatWindow(hwndMouse)) FCompatPresent(hwndMouse);
    return (HCURSOR)lpOld;
}
void CompatCursorPoint(HWND hwnd, int x, int y) {
    HWND hwndOld = hwndMouse;
    hwndMouse = hwnd; ptMouse = (POINT){x, y};
    if (lpCursor && lpCursor->fInvert) {
        if (hwndOld != hwnd && LpCompatWindow(hwndOld)) FCompatPresent(hwndOld);
        FCompatPresent(hwnd);
    }
}
void CompatCursorLeave(HWND hwnd) {
    if (hwndMouse != hwnd) return;
    hwndMouse = NULL;
    if (lpCursor && lpCursor->fInvert) FCompatPresent(hwnd);
}
void CompatDrawCursor(HWND hwnd, SDL_Surface *lpSurface) {
    COMPATIMAGE *lp = lpCursor;
    BYTE *src, *dst;
    int x, y, i, ix, iy, iScale = StarsMax(1, hwnd->iCanvasScale);
    int64_t dx, dy;
    if (!lp || !lp->fInvert || hwnd != hwndMouse) return;
    for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
        dx = ((int64_t)ptMouse.x - lp->xHot + x) * iScale; dy = ((int64_t)ptMouse.y - lp->yHot + y) * iScale;
        if (dx < 0 || dy < 0 || dx + iScale > lpSurface->w || dy + iScale > lpSurface->h) continue;
        src = (BYTE *)lp->lpXor->pixels + (size_t)y * lp->lpXor->pitch + x * 4;
        for (iy = 0; iy < iScale; iy++) for (ix = 0; ix < iScale; ix++) {
            dst = (BYTE *)lpSurface->pixels + (size_t)(dy + iy) * lpSurface->pitch + (dx + ix) * 4;
            for (i = 0; i < 3; i++) dst[i] = (BYTE)((lp->rgbAnd[y * lp->lpXor->w + x] ? dst[i] : 0) ^ src[i]);
        }
    }
}
void CompatShutdownImages(void) {
    COMPATIMAGE *lp, *lpNext;
    if (SDL_WasInit(SDL_INIT_VIDEO)) { SDL_SetCursor(SDL_GetDefaultCursor()); SDL_ShowCursor(); }
    lpCursor = NULL; hwndMouse = NULL;
    for (lp = lpImages; lp; lp = lpNext) {
        lpNext = lp->lpNext; delete lp;
    }
    lpImages = NULL;
}

void CompatSetWindowIcon(SDL_Window *lpWindow, HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    SDL_Surface *lpIcon;
    SDL_IOStream *lpStream;
    int x, y;
    NATIVEOWNER<SDL_Surface, SDL_DestroySurface> lpiconOwner;
    if (!lp || !lp->lpXor) return;
    // The desktop uses the high-resolution artwork; DrawIcon keeps the original game pixels.
    if (lp->fAppIcon) {
        lpStream = SDL_IOFromConstMem(rgbCompatAppIcon, cbCompatAppIcon);
        lpiconOwner.reset(lpStream ? SDL_LoadPNG_IO(lpStream, true) : NULL); lpIcon = lpiconOwner.get();
        if (lpIcon) {
            SDL_SetWindowIcon(lpWindow, lpIcon);
            return;
        }
    }
    lpiconOwner.reset(SDL_DuplicateSurface(lp->lpXor.get())); lpIcon = lpiconOwner.get();
    if (!lpIcon) return;
    for (y = 0; y < lpIcon->h; y++) for (x = 0; x < lpIcon->w; x++)
        ((BYTE *)lpIcon->pixels)[(size_t)y * lpIcon->pitch + x * 4 + 3] = lp->rgbAnd[y * lpIcon->w + x] ? 0 : 255;
    SDL_SetWindowIcon(lpWindow, lpIcon);
}
