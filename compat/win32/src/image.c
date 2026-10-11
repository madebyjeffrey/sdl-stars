/* Immutable module images and owned copies; resource reads stay bounded. */
#include "internal.h"
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
    COMPATIMAGE *lp = calloc(1, sizeof(*lp));
    if (!lp) return NULL;
    lp->lpXor = lpXor; lp->rgbAnd = rgbAnd; lp->fCursor = fCursor; lp->fAlive = TRUE;
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
    lpSurface = LpCompatDibSurface(&dib);
    if (!lpSurface) return NULL;
    rgAnd = malloc((size_t)dib.cx * dib.cy);
    if (!rgAnd) { SDL_DestroySurface(lpSurface); return NULL; }
    for (y = 0; y < dib.cy; y++) for (x = 0; x < dib.cx; x++) {
        rgAnd[y * dib.cx + x] = (BYTE)((lpAnd[(size_t)(dib.cy - 1 - y) * cbMaskRow + x / 8] >> (7 - x % 8)) & 1);
        lpPixel = (BYTE *)lpSurface->pixels + (size_t)y * lpSurface->pitch + x * 4;
        if (rgAnd[y * dib.cx + x] && (lpPixel[0] || lpPixel[1] || lpPixel[2])) fInvert = TRUE;
    }
    lp = LpNewImage(lpSurface, rgAnd, fCursor);
    if (!lp) { SDL_DestroySurface(lpSurface); free(rgAnd); return NULL; }
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
    rgb = LockResource(LoadResource(hInstance, hgroup)); cb = SizeofResource(hInstance, hgroup);
    if (!rgb || cb < 20 || WImage(rgb) || WImage(rgb + 2) != (fCursor ? 2 : 1)) return NULL;
    count = WImage(rgb + 4);
    if (count == 0 || (size_t)count * 14 > cb - 6) return NULL;
    // All canonical images are 32x32. Group entries retain their original IDs.
    id = WImage(rgb + 18);
    hrsrc = FindResourceA(hInstance, MAKEINTRESOURCE(id), fCursor ? RT_CURSOR : RT_ICON);
    if (!hrsrc) return NULL;
    for (lp = lpImages; lp; lp = lp->lpNext) if (lp->hrsrc == hrsrc && lp->fCursor == fCursor) return (HICON)lp;
    rgb = LockResource(LoadResource(hInstance, hrsrc)); cb = SizeofResource(hInstance, hrsrc);
    h = HiconCompatResource(rgb, cb, fCursor);
    if (h) { lp = LpCompatImage(h); lp->fShared = TRUE; lp->hrsrc = hrsrc; }
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
    if (!lp || !lp->lpXor) return NULL;
    cb = (size_t)lp->lpXor->w * lp->lpXor->h;
    lpXor = SDL_DuplicateSurface(lp->lpXor); rgAnd = malloc(cb);
    if (!lpXor || !rgAnd) { SDL_DestroySurface(lpXor); free(rgAnd); return NULL; }
    memcpy(rgAnd, lp->rgbAnd, cb);
    lpNew = LpNewImage(lpXor, rgAnd, lp->fCursor);
    if (!lpNew) { SDL_DestroySurface(lpXor); free(rgAnd); return NULL; }
    lpNew->fMono = lp->fMono; lpNew->fInvert = lp->fInvert; lpNew->xHot = lp->xHot; lpNew->yHot = lp->yHot;
    return (HICON)lpNew;
}
static BOOL FDestroyImage(HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    if (!lp) return FALSE;
    if (lp->fShared) return TRUE; // LoadIcon/LoadCursor handles belong to the module.
    if (lpCursor == lp) SetCursor(NULL);
    SDL_DestroyCursor(lp->lpCursor); lp->lpCursor = NULL;
    SDL_DestroySurface(lp->lpXor); lp->lpXor = NULL;
    free(lp->rgbAnd); lp->rgbAnd = NULL; lp->fAlive = FALSE;
    return TRUE;
}
BOOL WINAPI DestroyIcon(HICON hicon) { return FDestroyImage(hicon); }
BOOL WINAPI DestroyCursor(HCURSOR hcursor) { return FDestroyImage((HICON)hcursor); }
BOOL WINAPI GetIconInfo(HICON hicon, PICONINFO lpii) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    ICONINFO ii = {0};
    SDL_Surface *lpMask;
    BYTE *rgb, *src;
    int x, y, dy;
    if (!lp || !lp->lpXor || !lpii) return FALSE;
    dy = lp->lpXor->h;
    lpMask = SDL_CreateSurface(lp->lpXor->w, dy * (lp->fMono ? 2 : 1), SDL_PIXELFORMAT_RGBA32);
    if (!lpMask) return FALSE;
    for (y = 0; y < lpMask->h; y++) for (x = 0; x < lpMask->w; x++) {
        rgb = (BYTE *)lpMask->pixels + (size_t)y * lpMask->pitch + x * 4;
        src = (BYTE *)lp->lpXor->pixels + (size_t)(y % dy) * lp->lpXor->pitch + x * 4;
        rgb[0] = rgb[1] = rgb[2] = y < dy ? (lp->rgbAnd[y * lpMask->w + x] ? 255 : 0) : src[0]; rgb[3] = 255;
    }
    ii.fIcon = !lp->fCursor; ii.xHotspot = lp->fCursor ? lp->xHot : (UINT)lp->lpXor->w / 2;
    ii.yHotspot = lp->fCursor ? lp->yHot : (UINT)dy / 2;
    ii.hbmMask = HbmpCompatSurface(lpMask, TRUE); SDL_DestroySurface(lpMask);
    if (!lp->fMono) ii.hbmColor = HbmpCompatSurface(lp->lpXor, FALSE);
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
    if (lp->lpCursor) return TRUE;
    if (lp->idSystem) lp->lpCursor = SDL_CreateSystemCursor(IdSystemCursor(lp->idSystem));
    else if (lp->fMono && lp->lpXor->w % 8 == 0) {
        cb = (size_t)(lp->lpXor->w / 8) * lp->lpXor->h;
        rg = calloc(2, cb);
        if (!rg) return FALSE;
        data = rg; mask = rg + cb;
        for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
            fAnd = lp->rgbAnd[y * lp->lpXor->w + x] != 0;
            src = (BYTE *)lp->lpXor->pixels + (size_t)y * lp->lpXor->pitch + x * 4;
            fWhite = src[0] != 0;
            if (fAnd ? fWhite : !fWhite) data[y * (lp->lpXor->w / 8) + x / 8] |= (BYTE)(0x80 >> (x % 8));
            if (!fAnd) mask[y * (lp->lpXor->w / 8) + x / 8] |= (BYTE)(0x80 >> (x % 8));
        }
        lp->lpCursor = SDL_CreateCursor(data, mask, lp->lpXor->w, lp->lpXor->h, lp->xHot, lp->yHot);
        free(rg);
    } else {
        lpColor = SDL_DuplicateSurface(lp->lpXor);
        if (!lpColor) return FALSE;
        for (y = 0; y < lpColor->h; y++) for (x = 0; x < lpColor->w; x++)
            ((BYTE *)lpColor->pixels)[(size_t)y * lpColor->pitch + x * 4 + 3] = lp->rgbAnd[y * lpColor->w + x] ? 0 : 255;
        lp->lpCursor = SDL_CreateColorCursor(lpColor, lp->xHot, lp->yHot);
        SDL_DestroySurface(lpColor);
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
        if (lp->lpCursor && !SDL_SetCursor(lp->lpCursor)) return NULL;
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
    int x, y, dx, dy, i;
    if (!lp || !lp->fInvert || hwnd != hwndMouse) return;
    for (y = 0; y < lp->lpXor->h; y++) for (x = 0; x < lp->lpXor->w; x++) {
        dx = ptMouse.x - (int)lp->xHot + x; dy = ptMouse.y - (int)lp->yHot + y;
        if (dx < 0 || dy < 0 || dx >= lpSurface->w || dy >= lpSurface->h) continue;
        src = (BYTE *)lp->lpXor->pixels + (size_t)y * lp->lpXor->pitch + x * 4;
        dst = (BYTE *)lpSurface->pixels + (size_t)dy * lpSurface->pitch + dx * 4;
        for (i = 0; i < 3; i++) dst[i] = (BYTE)((lp->rgbAnd[y * lp->lpXor->w + x] ? dst[i] : 0) ^ src[i]);
    }
}
void CompatShutdownImages(void) {
    COMPATIMAGE *lp, *lpNext;
    if (SDL_WasInit(SDL_INIT_VIDEO)) { SDL_SetCursor(SDL_GetDefaultCursor()); SDL_ShowCursor(); }
    lpCursor = NULL; hwndMouse = NULL;
    for (lp = lpImages; lp; lp = lpNext) {
        lpNext = lp->lpNext; SDL_DestroyCursor(lp->lpCursor); SDL_DestroySurface(lp->lpXor); free(lp->rgbAnd); free(lp);
    }
    lpImages = NULL;
}

void CompatSetWindowIcon(SDL_Window *lpWindow, HICON hicon) {
    COMPATIMAGE *lp = LpCompatImage(hicon);
    SDL_Surface *lpIcon;
    int x, y;
    if (!lp || !lp->lpXor) return;
    lpIcon = SDL_DuplicateSurface(lp->lpXor);
    if (!lpIcon) return;
    for (y = 0; y < lpIcon->h; y++) for (x = 0; x < lpIcon->w; x++)
        ((BYTE *)lpIcon->pixels)[(size_t)y * lpIcon->pitch + x * 4 + 3] = lp->rgbAnd[y * lpIcon->w + x] ? 0 : 255;
    SDL_SetWindowIcon(lpWindow, lpIcon); SDL_DestroySurface(lpIcon);
}
