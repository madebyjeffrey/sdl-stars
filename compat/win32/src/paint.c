/* Basic GDI with logical-to-device translation. Rasterization stays deterministic on a CPU
 * canvas; SDL's renderer uploads/presents each window through its own texture.
 * No native-sized state in this module reaches game files.
 */
#include "internal.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define objPen 1
#define objBrush 2
#define objFont 3
#define objBitmap 4
#define objRegion 5
#define objPalette 6

typedef struct COMPATOBJECT {
    int iKind, cSelected, iPenStyle;
    COLORREF crColor;
    BOOL fNull, fStock, fAlive, fMono;
    SDL_Surface *lpSurface;
    COMPATFONT *lpFont;
    LOGFONTA lf;
    PALETTEENTRY *rgpe;
    UINT cEntries;
    RECT rcRegion;
    struct COMPATOBJECT *lpNext;
} COMPATOBJECT;

struct HDC__ {
    COMPATPRINT *lpprint;
    HDC hdcToken;
    HWND hwnd;
    RECT rcClip;
    POINT pt, ptOrg, ptBrush;
    RECT *rgrcRegion, *rgrcPaint;
    size_t crcPaint;
    size_t crcRegion;
    BOOL fRegion;
    COMPATOBJECT *lppen, *lpbrush, *lpfont, *lpbitmap, *lpbitmapDefault, *lppalette;
    COLORREF crText, crBk;
    int iBkMode, iRop2;
    BOOL fAlive, fPaint, fClip, fMemory, fScreen;
    struct HDC__ *lpNext;
};

static COMPATOBJECT *lpObjects;
static HDC lpDCs;
static uintptr_t idDCNext = 1;
static COMPATOBJECT rgStock[18];
static BOOL fStock;
static PALETTEENTRY rgpeDefault[] = {
    {0, 0, 0, 0}, {128, 0, 0, 0}, {0, 128, 0, 0}, {128, 128, 0, 0},
    {0, 0, 128, 0}, {128, 0, 128, 0}, {0, 128, 128, 0}, {192, 192, 192, 0},
    {192, 220, 192, 0}, {166, 202, 240, 0}, {255, 251, 240, 0}, {160, 160, 164, 0},
    {128, 128, 128, 0}, {255, 0, 0, 0}, {0, 255, 0, 0}, {255, 255, 0, 0},
    {0, 0, 255, 0}, {255, 0, 255, 0}, {0, 255, 255, 0}, {255, 255, 255, 0}
};

static BOOL FColor(COLORREF cr) {
    return (cr >> 24) <= 2;
}
static COLORREF CrPalette(HDC lpdc, COLORREF cr) {
    UINT i;
    PALETTEENTRY pe;
    if ((cr >> 24) != 1) return cr & UINT32_C(0x00ffffff);
    i = (WORD)cr;
    if (i >= lpdc->lppalette->cEntries) return RGB(0, 0, 0);
    pe = lpdc->lppalette->rgpe[i];
    return RGB(pe.peRed, pe.peGreen, pe.peBlue);
}

static COMPATOBJECT *LpObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj;
    int i;

    if (!SDL_IsMainThread())
        return NULL;
    for (i = 0; i < 18; i++)
        if (h == &rgStock[i] && rgStock[i].fAlive)
            return &rgStock[i];
    for (lpobj = lpObjects; lpobj; lpobj = lpobj->lpNext)
        if (h == lpobj && lpobj->fAlive)
            return lpobj;
    return NULL;
}

static HDC LpDC(HDC hdc) {
    HDC lpdc;

    if (!SDL_IsMainThread())
        return NULL;
    for (lpdc = lpDCs; lpdc; lpdc = lpdc->lpNext)
        if (lpdc->hdcToken == hdc && lpdc->fAlive && (lpdc->fMemory || lpdc->fScreen || LpCompatWindow(lpdc->hwnd)))
            return lpdc;
    return NULL;
}

HGDIOBJ WINAPI GetStockObject(int i) {
    int j;
    static const BYTE rgGray[] = {255, 192, 128, 64, 0, 0};

    if (!SDL_IsMainThread())
        return NULL;
    if (!fStock) {
        for (j = 0; j < 18; j++) {
            rgStock[j].fStock = TRUE;
            if (j <= NULL_BRUSH) {
                rgStock[j].iKind = objBrush;
                rgStock[j].crColor = RGB(rgGray[j], rgGray[j], rgGray[j]);
                rgStock[j].fNull = j == NULL_BRUSH;
                rgStock[j].fAlive = TRUE;
            } else if (j <= NULL_PEN) {
                rgStock[j].iKind = objPen;
                rgStock[j].crColor = j == WHITE_PEN ? RGB(255, 255, 255) : RGB(0, 0, 0);
                rgStock[j].fNull = j == NULL_PEN;
                rgStock[j].fAlive = TRUE;
            } else if (j == DEFAULT_PALETTE) {
                rgStock[j].iKind = objPalette;
                rgStock[j].rgpe = rgpeDefault;
                rgStock[j].cEntries = sizeof(rgpeDefault) / sizeof(rgpeDefault[0]);
                rgStock[j].fAlive = TRUE;
            } else if (j == SYSTEM_FONT || j == SYSTEM_FIXED_FONT ||
                       j == ANSI_FIXED_FONT || j == OEM_FIXED_FONT || j == ANSI_VAR_FONT ||
                       j == DEVICE_DEFAULT_FONT || j == DEFAULT_GUI_FONT) {
                rgStock[j].iKind = objFont;
                rgStock[j].fAlive = TRUE;
            }
        }
        fStock = TRUE;
    }
    if (i < 0 || i >= 18 || !rgStock[i].fAlive) return NULL;
    if (rgStock[i].iKind == objFont && !rgStock[i].lpFont) {
        rgStock[i].lf.lfHeight = -12;
        strcpy(rgStock[i].lf.lfFaceName, i == SYSTEM_FIXED_FONT || i == ANSI_FIXED_FONT || i == OEM_FIXED_FONT ?
               "monospace" : "sans-serif");
        rgStock[i].lpFont = LpNewCompatFont(&rgStock[i].lf);
        if (!rgStock[i].lpFont) return NULL;
    }
    return &rgStock[i];
}

static COMPATOBJECT *LpNewObject(int iKind, COLORREF crColor, BOOL fNull) {
    COMPATOBJECT *lpobj;

    if (!SDL_IsMainThread() || !FColor(crColor))
        return NULL;
    lpobj = calloc(1, sizeof(*lpobj));
    if (!lpobj)
        return NULL;
    lpobj->iKind = iKind;
    lpobj->crColor = crColor;
    lpobj->fNull = fNull;
    lpobj->fAlive = TRUE;
    lpobj->lpNext = lpObjects;
    lpObjects = lpobj;
    return lpobj;
}

HPEN WINAPI CreatePen(int iStyle, int cWidth, COLORREF crColor) {
    COMPATOBJECT *lpobj;
    if (iStyle < PS_SOLID || iStyle > PS_NULL || cWidth < 0 || cWidth > 1) {
        SDL_SetError("GDI supports cosmetic solid/dashed/dotted or null pens, width 0 or 1");
        return NULL;
    }
    lpobj = LpNewObject(objPen, crColor, iStyle == PS_NULL);
    if (lpobj) lpobj->iPenStyle = iStyle;
    return (HPEN)lpobj;
}

HBRUSH WINAPI CreateSolidBrush(COLORREF crColor) {
    return (HBRUSH)LpNewObject(objBrush, crColor, FALSE);
}

HPALETTE WINAPI CreatePalette(const LOGPALETTE *lppal) {
    COMPATOBJECT *lpobj;
    PALETTEENTRY *rgpe;
    if (!SDL_IsMainThread() || !lppal || lppal->palVersion != 0x300 || lppal->palNumEntries == 0)
        return NULL;
    rgpe = malloc((size_t)lppal->palNumEntries * sizeof(*rgpe));
    if (!rgpe) return NULL;
    memcpy(rgpe, lppal->palPalEntry, (size_t)lppal->palNumEntries * sizeof(*rgpe));
    lpobj = LpNewObject(objPalette, 0, FALSE);
    if (!lpobj) { free(rgpe); return NULL; }
    lpobj->rgpe = rgpe; lpobj->cEntries = lppal->palNumEntries;
    return (HPALETTE)lpobj;
}

HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE hpal, BOOL fBackground) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject((HGDIOBJ)hpal), *lpold;
    (void)fBackground;
    if (!lpdc || !lpobj || lpobj->iKind != objPalette) return NULL;
    lpold = lpdc->lppalette;
    lpold->cSelected--; lpobj->cSelected++;
    lpdc->lppalette = lpobj;
    return (HPALETTE)lpold;
}

UINT WINAPI RealizePalette(HDC hdc) {
    // RGBA display and memory canvases have no physical palette to remap.
    return LpDC(hdc) ? 0 : GDI_ERROR;
}

BOOL WINAPI UnrealizeObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj = LpObject(h);
    // True-color palettes need no remapping; brush origins belong to the DC.
    return lpobj && !lpobj->fStock && (lpobj->iKind == objPalette || lpobj->iKind == objBrush);
}

UINT WINAPI GetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, LPPALETTEENTRY rgpe) {
    COMPATOBJECT *lpobj = LpObject((HGDIOBJ)hpal);
    if (!lpobj || lpobj->iKind != objPalette) return 0;
    if (!rgpe) return lpobj->cEntries;
    if (iStart >= lpobj->cEntries) return 0;
    cEntries = min(cEntries, lpobj->cEntries - iStart);
    memcpy(rgpe, lpobj->rgpe + iStart, (size_t)cEntries * sizeof(*rgpe));
    return cEntries;
}

HFONT WINAPI CreateFontIndirectA(const LOGFONTA *lplf) {
    COMPATOBJECT *lpobj;
    COMPATFONT *lpfont;
    if (!SDL_IsMainThread() || !lplf) return NULL;
    lpfont = LpNewCompatFont(lplf);
    if (!lpfont) return NULL;
    lpobj = LpNewObject(objFont, 0, FALSE);
    if (!lpobj) { FreeCompatFont(lpfont); return NULL; }
    lpobj->lpFont = lpfont; lpobj->lf = *lplf;
    return (HFONT)lpobj;
}

BOOL WINAPI DeleteObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj = LpObject(h);

    if (!lpobj || lpobj->fStock || lpobj->cSelected != 0)
        return FALSE;
    // Tombstones prevent stale GDI handles from aliasing newly allocated objects.
    SDL_DestroySurface(lpobj->lpSurface);
    lpobj->lpSurface = NULL;
    FreeCompatFont(lpobj->lpFont); lpobj->lpFont = NULL;
    free(lpobj->rgpe); lpobj->rgpe = NULL; lpobj->cEntries = 0;
    lpobj->fAlive = FALSE;
    return TRUE;
}

HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject(h), *lpold, **lplp;

    if (!lpdc || !lpobj)
        return NULL;
    if (lpobj->iKind == objBitmap) {
        if (!lpdc->fMemory || lpdc->lpprint || (lpobj->fStock && lpobj != lpdc->lpbitmapDefault) ||
            (lpobj != lpdc->lpbitmap && lpobj->cSelected != 0))
            return NULL;
        lplp = &lpdc->lpbitmap;
    } else if (lpobj->iKind == objPen)
        lplp = &lpdc->lppen;
    else if (lpobj->iKind == objBrush)
        lplp = &lpdc->lpbrush;
    else if (lpobj->iKind == objFont)
        lplp = &lpdc->lpfont;
    else
        return NULL;
    lpold = *lplp;
    lpold->cSelected--;
    lpobj->cSelected++;
    *lplp = lpobj;
    return lpold;
}

static BOOL FCanvas(HWND hwnd) {
    SDL_Surface *lpnew;
    SDL_Texture *lptex;
    int dx, dy, cyMenu;
    float cxTexture, cyTexture;
    RECT rc;
    const char *pszMode;

    if (!CompatGetSurfaceRect(hwnd, &rc)) return FALSE;
    dx = rc.right; dy = rc.bottom;
    cyMenu = CyCompatMenuBar(hwnd);
    dx = max(1, dx); dy = max(1, dy);
    if (hwnd->lpCanvas && hwnd->lpCanvas->w == dx && hwnd->lpCanvas->h == dy &&
        (hwnd->hwndParent || (SDL_GetTextureSize(hwnd->lpTexture, &cxTexture, &cyTexture) &&
         cxTexture == dx && cyTexture == dy + cyMenu)))
        return TRUE;
    if (!hwnd->hwndParent && !hwnd->lpRenderer) {
        pszMode = SDL_getenv("STARS_SDL_RENDERER");
        if (pszMode && strcmp(pszMode, "auto") != 0 && strcmp(pszMode, "software") != 0 && strcmp(pszMode, "opengl") != 0) {
            SDL_SetError("GDI STARS_SDL_RENDERER must be auto, software or opengl");
            return FALSE;
        }
        hwnd->lpRenderer = SDL_CreateRenderer(hwnd->lpWindow,
                                             pszMode && strcmp(pszMode, "auto") != 0 ? pszMode : NULL);
        if (!hwnd->lpRenderer && (!pszMode || strcmp(pszMode, "auto") == 0))
            hwnd->lpRenderer = SDL_CreateRenderer(hwnd->lpWindow, "software");
        if (!hwnd->lpRenderer)
            return FALSE;
    }
    lpnew = SDL_CreateSurface(dx, dy, SDL_PIXELFORMAT_RGBA32);
    lptex = hwnd->hwndParent ? NULL :
        SDL_CreateTexture(hwnd->lpRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, dx, dy + cyMenu);
    if (!lpnew || (!hwnd->hwndParent && !lptex)) {
        SDL_DestroySurface(lpnew);
        SDL_DestroyTexture(lptex);
        return FALSE;
    }
    SDL_FillSurfaceRect(lpnew, NULL, SDL_MapSurfaceRGB(lpnew, 0, 0, 0));
    if (hwnd->lpCanvas)
        SDL_BlitSurface(hwnd->lpCanvas, NULL, lpnew, NULL);
    SDL_SetSurfaceBlendMode(lpnew, SDL_BLENDMODE_NONE);
    if (lptex) {
        SDL_SetTextureBlendMode(lptex, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(lptex, SDL_SCALEMODE_NEAREST);
    }
    SDL_DestroyTexture(hwnd->lpTexture);
    SDL_DestroySurface(hwnd->lpCanvas);
    SDL_DestroySurface(hwnd->lpComposite);
    hwnd->lpComposite = NULL;
    hwnd->lpTexture = lptex;
    hwnd->lpCanvas = lpnew;
    return TRUE;
}

BOOL FCompatCanvas(HWND hwnd) { return FCanvas(hwnd); }

static BOOL FComposeChildren(HWND hwnd, SDL_Surface *lpSurface, int64_t x, int64_t y, const RECT *lprcClip) {
    HWND lp;
    RECT rc, client, clip;
    SDL_Rect src, dst;
    int64_t left, top, right, bottom;
    COLORREF cr;

    GetClientRect(hwnd, &client);
    if (!hwnd->lpcombo && (hwnd->dwStyle & (WS_HSCROLL | WS_VSCROLL)) == (WS_HSCROLL | WS_VSCROLL)) {
        CompatGetSurfaceRect(hwnd, &rc);
        left = max((int64_t)lprcClip->left, x + client.right);
        top = max((int64_t)lprcClip->top, y + client.bottom);
        right = min((int64_t)lprcClip->right, x + rc.right);
        bottom = min((int64_t)lprcClip->bottom, y + rc.bottom);
        if (right > left && bottom > top) {
            dst = (SDL_Rect){(int)left, (int)top, (int)(right - left), (int)(bottom - top)};
            cr = GetSysColor(COLOR_BTNFACE);
            SDL_FillSurfaceRect(lpSurface, &dst, SDL_MapSurfaceRGB(lpSurface, cr & 255, (cr >> 8) & 255, (cr >> 16) & 255));
        }
    }
    // Nonclient scrollbar children occupy the reserved strips; ordinary children
    // must not paint into those strips even when they extend beyond the client.
    // Draw bottom to top; descendants are clipped by every ancestor.
    lp = GetWindow(hwnd, GW_CHILD);
    if (lp) lp = GetWindow(lp, GW_HWNDLAST);
    for (; lp; lp = GetWindow(lp, GW_HWNDPREV)) {
        if (!lp->fVisible || lp->fDestroying) continue;
        clip = *lprcClip;
        if (!lp->fNcScroll) {
            clip.right = (LONG)min((int64_t)clip.right, x + client.right);
            clip.bottom = (LONG)min((int64_t)clip.bottom, y + client.bottom);
        }
        left = x + lp->rcChild.left; top = y + lp->rcChild.top;
        right = x + lp->rcChild.right; bottom = y + lp->rcChild.bottom;
        if (left >= clip.right || top >= clip.bottom ||
            right <= clip.left || bottom <= clip.top) continue;
        rc.left = (LONG)(left > clip.left ? left : clip.left);
        rc.top = (LONG)(top > clip.top ? top : clip.top);
        rc.right = (LONG)(right < clip.right ? right : clip.right);
        rc.bottom = (LONG)(bottom < clip.bottom ? bottom : clip.bottom);
        if (!FCanvas(lp)) return FALSE;
        dst = (SDL_Rect){rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top};
        src = (SDL_Rect){(int)(rc.left - left), (int)(rc.top - top), dst.w, dst.h};
        if (!SDL_BlitSurface(lp->lpCanvas, &src, lpSurface, &dst) ||
            !FComposeChildren(lp, lpSurface, left, top, &rc)) return FALSE;
    }
    return TRUE;
}

static BOOL FPendingPaintTree(HWND hwnd) {
    HWND lp;

    if (hwnd->hdcPaint || hwnd->fUpdate) return TRUE;
    for (lp = GetWindow(hwnd, GW_CHILD); lp; lp = GetWindow(lp, GW_HWNDNEXT))
        if (lp->fVisible && FPendingPaintTree(lp)) return TRUE;
    return FALSE;
}

BOOL FCompatPaintTree(HWND hwnd) {
    HWND lp, lpNext;

    if (!UpdateWindow(hwnd) || !IsWindow(hwnd)) return FALSE;
    for (lp = GetWindow(hwnd, GW_CHILD); lp; lp = lpNext) {
        lpNext = GetWindow(lp, GW_HWNDNEXT);
        if (lp->fVisible && !FCompatPaintTree(lp)) return FALSE;
        if (!IsWindow(hwnd)) return FALSE;
    }
    return TRUE;
}

BOOL FCompatRefreshPresent(HWND hwnd) {
    HWND lpwin = HwndCompatRoot(hwnd);
    static BOOL fPresenting;
    BOOL fOk;

    if (!lpwin || lpwin->fDestroying || !lpwin->lpRenderer || !lpwin->lpTexture ||
        !lpwin->lpComposite || fPresenting) return FALSE;
    // Cocoa can clear its view during exposure after the initial show. Reuse
    // the last completed texture even if a new paint is pending; never reenter
    // application drawing from a native exposure callback.
    fPresenting = TRUE;
    fOk = SDL_SetRenderDrawColor(lpwin->lpRenderer, 0, 0, 0,
                                SDL_GetWindowFlags(lpwin->lpWindow) & SDL_WINDOW_TRANSPARENT ? 0 : 255) &&
          SDL_RenderClear(lpwin->lpRenderer) &&
          SDL_RenderTexture(lpwin->lpRenderer, lpwin->lpTexture, NULL, NULL) &&
          SDL_RenderPresent(lpwin->lpRenderer);
    fPresenting = FALSE;
    return fOk;
}

BOOL FCompatPresent(HWND hwnd) {
    HWND lpwin = HwndCompatRoot(hwnd);
    RECT rc;
    SDL_Surface *lpnew;
    SDL_Rect dst;
    int cyMenu;
    BOOL fOk;

    if (!lpwin || lpwin->fDestroying) return FALSE;
    lpwin->fPresentPending = TRUE;
    // Present the completed window tree, not intermediate icon/DC or pane paints.
    if (lpwin->fDeferPresent || lpwin->cPresentDefer != 0 || FPendingPaintTree(lpwin)) return TRUE;
    if (!FCanvas(lpwin)) return FALSE;
    cyMenu = CyCompatMenuBar(lpwin);
    if (!lpwin->lpComposite || lpwin->lpComposite->w != lpwin->lpCanvas->w ||
        lpwin->lpComposite->h != lpwin->lpCanvas->h + cyMenu) {
        lpnew = SDL_CreateSurface(lpwin->lpCanvas->w, lpwin->lpCanvas->h + cyMenu, SDL_PIXELFORMAT_RGBA32);
        if (!lpnew) return FALSE;
        SDL_SetSurfaceBlendMode(lpnew, SDL_BLENDMODE_NONE);
        SDL_DestroySurface(lpwin->lpComposite);
        lpwin->lpComposite = lpnew;
    }
    dst = (SDL_Rect){0, cyMenu, lpwin->lpCanvas->w, lpwin->lpCanvas->h};
    if (!SDL_BlitSurface(lpwin->lpCanvas, NULL, lpwin->lpComposite, &dst)) return FALSE;
    CompatGetSurfaceRect(lpwin, &rc);
    OffsetRect(&rc, 0, cyMenu);
    if (!FComposeChildren(lpwin, lpwin->lpComposite, 0, cyMenu, &rc) ||
        !FCompatDrawMenuBar(lpwin, lpwin->lpComposite)) return FALSE;
    CompatDrawCursor(lpwin, lpwin->lpComposite);
    if (FPendingPaintTree(lpwin)) return TRUE;
    fOk = SDL_UpdateTexture(lpwin->lpTexture, NULL, lpwin->lpComposite->pixels, lpwin->lpComposite->pitch) &&
          FCompatRefreshPresent(lpwin);
    if (fOk) lpwin->fPresentPending = FALSE;
    return fOk;
}

void CompatFlushPresent(void) {
    HWND lp;

    for (lp = HwndCompatRegistry(); lp; lp = lp->lpNext)
        if (lp->fAlive && !lp->hwndParent && lp->fVisible && lp->fPresentPending)
            FCompatPresent(lp);
}

static HDC HdcNew(HWND hwnd, BOOL fPaint) {
    HDC lpdc;

    if (idDCNext > UINTPTR_MAX / 16 || !SDL_IsMainThread() ||
        (hwnd && (!LpCompatWindow(hwnd) || !FCanvas(hwnd))))
        return NULL;
    lpdc = calloc(1, sizeof(*lpdc));
    if (!lpdc)
        return NULL;
    // Never dereference public HDC tokens. Monotonic IDs permit immediate release
    // of frequent paint DC allocations without making stale handles valid again.
    lpdc->hdcToken = (HDC)(idDCNext++ * 16);
    lpdc->hwnd = hwnd;
    if (hwnd)
        GetClientRect(hwnd, &lpdc->rcClip);
    lpdc->lppen = GetStockObject(BLACK_PEN);
    lpdc->lpbrush = GetStockObject(WHITE_BRUSH);
    lpdc->lpfont = GetStockObject(SYSTEM_FONT);
    lpdc->lppalette = GetStockObject(DEFAULT_PALETTE);
    if (!lpdc->lpfont) { free(lpdc); return NULL; }
    lpdc->lppen->cSelected++;
    lpdc->lpbrush->cSelected++;
    lpdc->lpfont->cSelected++;
    lpdc->lppalette->cSelected++;
    lpdc->crBk = RGB(255, 255, 255);
    lpdc->iBkMode = OPAQUE;
    lpdc->iRop2 = R2_COPYPEN;
    lpdc->fPaint = lpdc->fClip = fPaint;
    lpdc->fAlive = TRUE;
    lpdc->lpNext = lpDCs;
    lpDCs = lpdc;
    return lpdc->hdcToken;
}

static COMPATOBJECT *LpNewBitmap(int cx, int cy, BOOL fMono) {
    COMPATOBJECT *lpobj;
    SDL_Surface *lpsurface;

    if (cx <= 0 || cy <= 0 || cx > INT_MAX / 4)
        return NULL;
    lpsurface = SDL_CreateSurface(cx, cy, SDL_PIXELFORMAT_RGBA32);
    if (!lpsurface)
        return NULL;
    lpobj = LpNewObject(objBitmap, 0, FALSE);
    if (!lpobj) {
        SDL_DestroySurface(lpsurface);
        return NULL;
    }
    lpobj->lpSurface = lpsurface;
    lpobj->fMono = fMono;
    SDL_FillSurfaceRect(lpsurface, NULL, SDL_MapSurfaceRGB(lpsurface, 0, 0, 0));
    return lpobj;
}

HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int cx, int cy) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc || cx < 0 || cy < 0)
        return NULL;
    if (cx == 0 || cy == 0)
        return (HBITMAP)LpNewBitmap(1, 1, TRUE);
    return (HBITMAP)LpNewBitmap(cx, cy, lpdc->fMemory && lpdc->lpbitmap->fMono);
}

HDC WINAPI CreateCompatibleDC(HDC hdc) {
    HDC hdcNew, lpdc;
    COMPATOBJECT *lpbitmap;

    if (hdc && !LpDC(hdc))
        return NULL;
    // A memory DC starts with defaults, not a copy of its reference DC state.
    lpbitmap = LpNewBitmap(1, 1, TRUE);
    if (!lpbitmap)
        return NULL;
    hdcNew = HdcNew(NULL, FALSE);
    if (!hdcNew) {
        DeleteObject(lpbitmap);
        return NULL;
    }
    lpdc = lpDCs;
    lpdc->fMemory = TRUE;
    lpdc->lpbitmap = lpdc->lpbitmapDefault = lpbitmap;
    lpbitmap->fStock = TRUE;
    lpbitmap->cSelected = 1;
    return hdcNew;
}

int WINAPI GetObjectA(HANDLE h, int cb, LPVOID lpv) {
    COMPATOBJECT *lpobj = LpObject(h);
    BITMAP bm = {0};

    if (lpobj && lpobj->iKind == objPalette) {
        if (!lpv) return sizeof(WORD);
        if (cb < (int)sizeof(WORD)) return 0;
        *(WORD *)lpv = (WORD)lpobj->cEntries;
        return sizeof(WORD);
    }
    if (lpobj && lpobj->iKind == objFont) {
        if (!lpv) return sizeof(LOGFONTA);
        if (cb < (int)sizeof(LOGFONTA)) return 0;
        memcpy(lpv, &lpobj->lf, sizeof(lpobj->lf));
        return sizeof(LOGFONTA);
    }
    if (!lpobj || lpobj->iKind != objBitmap)
        return 0;
    if (!lpv)
        return sizeof(bm);
    if (cb < (int)sizeof(bm))
        return 0;
    bm.bmWidth = lpobj->lpSurface->w;
    bm.bmHeight = lpobj->lpSurface->h;
    bm.bmPlanes = 1;
    bm.bmBitsPixel = lpobj->fMono ? 1 : 32;
    bm.bmWidthBytes = lpobj->fMono ? ((bm.bmWidth + 15) / 16) * 2 : bm.bmWidth * 4;
    memcpy(lpv, &bm, sizeof(bm));
    return sizeof(bm);
}

static void CloseDC(HDC lpdc) {
    HDC *lplp = &lpDCs;

    CompatDeletePrinter(lpdc->lpprint);
    lpdc->lppen->cSelected--;
    lpdc->lpbrush->cSelected--;
    lpdc->lpfont->cSelected--;
    lpdc->lppalette->cSelected--;
    if (lpdc->lpbitmap)
        lpdc->lpbitmap->cSelected--;
    if (lpdc->lpbitmapDefault) {
        SDL_DestroySurface(lpdc->lpbitmapDefault->lpSurface);
        lpdc->lpbitmapDefault->lpSurface = NULL;
        lpdc->lpbitmapDefault->fAlive = FALSE;
    }
    while (*lplp != lpdc)
        lplp = &(*lplp)->lpNext;
    *lplp = lpdc->lpNext;
    free(lpdc->rgrcRegion);
    free(lpdc->rgrcPaint);
    free(lpdc);
}

BOOL WINAPI DeleteDC(HDC hdc) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc || !lpdc->fMemory)
        return FALSE;
    if (lpdc->lpprint && FCompatPrinterBusy(lpdc->lpprint)) { SetLastError(ERROR_BUSY); return FALSE; }
    CloseDC(lpdc);
    return TRUE;
}

HDC HdcCompatPrinter(COMPATPRINT *lpprint, int cx, int cy) {
    COMPATOBJECT *lpbitmap = LpNewBitmap(cx, cy, FALSE);
    HDC hdc, lpdc;
    if (!lpbitmap) return NULL;
    hdc = HdcNew(NULL, FALSE);
    if (!hdc) { DeleteObject(lpbitmap); return NULL; }
    lpdc = lpDCs;
    lpdc->lpprint = lpprint;
    lpdc->fMemory = TRUE;
    lpdc->lpbitmap = lpdc->lpbitmapDefault = lpbitmap;
    lpbitmap->fStock = TRUE; lpbitmap->cSelected = 1;
    SDL_FillSurfaceRect(lpbitmap->lpSurface, NULL, SDL_MapSurfaceRGB(lpbitmap->lpSurface, 255, 255, 255));
    return hdc;
}
COMPATPRINT *LpCompatPrinter(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->lpprint : NULL;
}
SDL_Surface *LpCompatPrinterSurface(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc && lpdc->lpprint ? lpdc->lpbitmap->lpSurface : NULL;
}

HDC WINAPI GetDC(HWND hwnd) {
    HDC hdc = HdcNew(hwnd, FALSE);
    if (hdc && !hwnd) lpDCs->fScreen = TRUE;
    return hdc;
}

int WINAPI ReleaseDC(HWND hwnd, HDC hdc) {
    HDC lpdc = LpDC(hdc);
    BOOL fOk;

    if (!lpdc || lpdc->hwnd != hwnd || lpdc->fPaint || lpdc->fMemory)
        return 0;
    fOk = lpdc->fScreen ? TRUE : FCompatPresent(hwnd);
    CloseDC(lpdc);
    return fOk;
}

static BOOL FAddUpdate(RECT **lprgrc, size_t *lpcrc, RECT rc) {
    RECT *rgrc;
    size_t i;
    if (rc.left >= rc.right || rc.top >= rc.bottom) return TRUE;
    for (i = 0; i < *lpcrc; i++)
        if (rc.left >= (*lprgrc)[i].left && rc.top >= (*lprgrc)[i].top &&
            rc.right <= (*lprgrc)[i].right && rc.bottom <= (*lprgrc)[i].bottom) return TRUE;
    if (*lpcrc >= SIZE_MAX / sizeof(*rgrc) - 1) return FALSE;
    rgrc = realloc(*lprgrc, (*lpcrc + 1) * sizeof(*rgrc));
    if (!rgrc) return FALSE;
    rgrc[(*lpcrc)++] = rc; *lprgrc = rgrc;
    return TRUE;
}

static BOOL FSubtractUpdate(RECT **lprgrc, size_t *lpcrc, RECT rc, const RECT *lprcCut) {
    RECT hit;
    if (!IntersectRect(&hit, &rc, lprcCut)) return FAddUpdate(lprgrc, lpcrc, rc);
    return FAddUpdate(lprgrc, lpcrc, (RECT){rc.left, rc.top, rc.right, hit.top}) &&
           FAddUpdate(lprgrc, lpcrc, (RECT){rc.left, hit.bottom, rc.right, rc.bottom}) &&
           FAddUpdate(lprgrc, lpcrc, (RECT){rc.left, hit.top, hit.left, hit.bottom}) &&
           FAddUpdate(lprgrc, lpcrc, (RECT){hit.right, hit.top, rc.right, hit.bottom});
}

static void SetUpdate(HWND hwnd, RECT *rgrc, size_t crc) {
    size_t i;
    free(hwnd->rgrcUpdate); hwnd->rgrcUpdate = rgrc; hwnd->crcUpdate = crc;
    hwnd->rcUpdate = (RECT){0}; hwnd->fUpdate = crc != 0;
    for (i = 0; i < crc; i++) {
        if (i == 0) hwnd->rcUpdate = rgrc[i];
        else {
            hwnd->rcUpdate.left = min(hwnd->rcUpdate.left, rgrc[i].left);
            hwnd->rcUpdate.top = min(hwnd->rcUpdate.top, rgrc[i].top);
            hwnd->rcUpdate.right = max(hwnd->rcUpdate.right, rgrc[i].right);
            hwnd->rcUpdate.bottom = max(hwnd->rcUpdate.bottom, rgrc[i].bottom);
        }
    }
    if (crc == 0) hwnd->fErase = FALSE;
}

BOOL WINAPI InvalidateRect(HWND hwnd, const RECT *lprc, BOOL fErase) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT rc, rcClient;
    RECT *rgrc;
    size_t crc;
    if (!lpwin || !GetClientRect(hwnd, &rcClient)) return FALSE;
    if (!IntersectRect(&rc, lprc ? lprc : &rcClient, &rcClient)) return TRUE;
    rgrc = lpwin->rgrcUpdate; crc = lpwin->crcUpdate;
    if (!FAddUpdate(&rgrc, &crc, rc)) return FALSE;
    // FAddUpdate may have relocated the allocation; retain ownership before SetUpdate.
    lpwin->rgrcUpdate = NULL;
    SetUpdate(lpwin, rgrc, crc); lpwin->fErase |= fErase;
    return TRUE;
}

static BOOL FPaintClip(HDC lpdc, HWND hwnd) {
    RECT *rgrc = NULL;
    if (hwnd->crcUpdate != 0) {
        rgrc = malloc(hwnd->crcUpdate * sizeof(*rgrc));
        if (!rgrc) return FALSE;
        memcpy(rgrc, hwnd->rgrcUpdate, hwnd->crcUpdate * sizeof(*rgrc));
    }
    lpdc->rgrcPaint = rgrc; lpdc->crcPaint = hwnd->crcUpdate;
    lpdc->fPaint = TRUE; lpdc->fClip = TRUE; lpdc->rcClip = hwnd->rcUpdate;
    return TRUE;
}

BOOL WINAPI GetUpdateRect(HWND hwnd, LPRECT lprc, BOOL fErase) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC hdc;

    if (!lpwin)
        return FALSE;
    if (lprc)
        *lprc = lpwin->fUpdate ? lpwin->rcUpdate : (RECT){0};
    if (fErase && lpwin->fUpdate && lpwin->fErase) {
        hdc = GetDC(hwnd);
        if (hdc) {
            if (!FPaintClip(LpDC(hdc), lpwin)) { ReleaseDC(hwnd, hdc); return FALSE; }
            if (SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0) && LpCompatWindow(hwnd))
                lpwin->fErase = FALSE;
            if (LpDC(hdc)) LpDC(hdc)->fPaint = FALSE;
            ReleaseDC(hwnd, hdc);
        }
    }
    return lpwin->fAlive && lpwin->fUpdate;
}

HDC WINAPI BeginPaint(HWND hwnd, LPPAINTSTRUCT lpps) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC hdc;
    BOOL fErase;

    if (!lpwin || !lpps || lpwin->hdcPaint)
        return NULL;
    hdc = HdcNew(hwnd, TRUE);
    if (!hdc)
        return NULL;
    memset(lpps, 0, sizeof(*lpps));
    lpps->hdc = hdc;
    lpps->rcPaint = lpwin->fUpdate ? lpwin->rcUpdate : (RECT){0};
    LpDC(hdc)->rcClip = lpps->rcPaint;
    LpDC(hdc)->rgrcPaint = lpwin->rgrcUpdate; LpDC(hdc)->crcPaint = lpwin->crcUpdate;
    lpwin->rgrcUpdate = NULL; lpwin->crcUpdate = 0;
    fErase = lpwin->fErase;
    lpwin->fUpdate = FALSE;
    lpwin->fErase = FALSE;
    lpwin->hdcPaint = hdc;
    if (fErase)
        lpps->fErase = !SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    return LpDC(hdc) ? hdc : NULL;
}

BOOL WINAPI EndPaint(HWND hwnd, const PAINTSTRUCT *lpps) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC lpdc = lpps ? LpDC(lpps->hdc) : NULL;
    BOOL fOk;

    if (!lpwin || !lpdc || !lpdc->fPaint || lpdc->hwnd != hwnd || lpwin->hdcPaint != lpps->hdc)
        return FALSE;
    lpwin->hdcPaint = NULL;
    CloseDC(lpdc);
    fOk = FCompatPresent(hwnd);
    return fOk;
}

BOOL WINAPI UpdateWindow(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin)
        return FALSE;
    if (lpwin->fUpdate && !lpwin->hdcPaint)
        SendMessageA(hwnd, WM_PAINT, 0, 0);
    return TRUE;
}

BOOL WINAPI ValidateRect(HWND hwnd, const RECT *lprc) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT *rgrc = NULL;
    size_t crc = 0, i;
    if (!lpwin) return FALSE;
    if (lprc) for (i = 0; i < lpwin->crcUpdate; i++)
        if (!FSubtractUpdate(&rgrc, &crc, lpwin->rgrcUpdate[i], lprc)) { free(rgrc); return FALSE; }
    SetUpdate(lpwin, rgrc, crc);
    return TRUE;
}

static SDL_Surface *LpDCSurface(HDC lpdc) {
    if (lpdc->fScreen) return NULL;
    if (lpdc->fMemory)
        return lpdc->lpbitmap->lpSurface;
    return FCanvas(lpdc->hwnd) ? lpdc->hwnd->lpCanvas : NULL;
}

static BOOL FClip(HDC lpdc, RECT *lprc) {
    RECT rcClient;
    SDL_Surface *lpsurface = LpDCSurface(lpdc);

    if (!lpsurface)
        return FALSE;
    rcClient = (RECT){0, 0, lpsurface->w, lpsurface->h};
    if (lpdc->hwnd && !GetClientRect(lpdc->hwnd, &rcClient)) return FALSE;
    if (lpdc->fClip)
        IntersectRect(lprc, &lpdc->rcClip, &rcClient);
    else
        *lprc = rcClient;
    return TRUE;
}

/* Keep the rasterizer in device coordinates. Promote translations before
 * subtraction, and clip rectangles before narrowing, so extreme origins cannot
 * overflow a signed Win32 coordinate or address a pixel outside its surface. */
static LONG LCoord(int64_t l) {
    return (LONG)max((int64_t)INT_MIN, min((int64_t)INT_MAX, l));
}

static RECT RcDevice(HDC lpdc, RECT rc) {
    return (RECT){LCoord((int64_t)rc.left - lpdc->ptOrg.x), LCoord((int64_t)rc.top - lpdc->ptOrg.y),
                  LCoord((int64_t)rc.right - lpdc->ptOrg.x), LCoord((int64_t)rc.bottom - lpdc->ptOrg.y)};
}

BOOL WINAPI SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc) return FALSE;
    if (lppt) *lppt = lpdc->ptOrg;
    lpdc->ptOrg = (POINT){x, y};
    return TRUE;
}

BOOL WINAPI SetBrushOrgEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc) return FALSE;
    if (lppt) *lppt = lpdc->ptBrush;
    lpdc->ptBrush = (POINT){x, y};
    return TRUE;
}

int WINAPI GetROP2(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->iRop2 : 0;
}

int WINAPI SetROP2(HDC hdc, int iRop) {
    HDC lpdc = LpDC(hdc);
    int iOld;
    if (!lpdc || iRop < 1 || iRop > 16) return 0;
    iOld = lpdc->iRop2; lpdc->iRop2 = iRop;
    return iOld;
}

/* Application clipping is independent of BeginPaint's visible/update clip.
 * Rectangles remain disjoint, so exclusions preserve holes without a mask the
 * size of the display. A selected region is copied, never retained by handle. */
static int IRegionKind(size_t crc) {
    return crc == 0 ? NULLREGION : crc == 1 ? SIMPLEREGION : COMPLEXREGION;
}

HRGN WINAPI CreateRectRgn(int left, int top, int right, int bottom) {
    COMPATOBJECT *lpobj;
    // GDI regions use signed 27-bit coordinates, even on a 64-bit host.
    if (left < -67108864 || top < -67108864 || right < -67108864 || bottom < -67108864 ||
        left > 67108863 || top > 67108863 || right > 67108863 || bottom > 67108863)
        return NULL;
    lpobj = LpNewObject(objRegion, 0, FALSE);
    if (!lpobj) return NULL;
    lpobj->rcRegion = (RECT){min(left, right), min(top, bottom), max(left, right), max(top, bottom)};
    return (HRGN)lpobj;
}

int WINAPI SelectClipRgn(HDC hdc, HRGN hrgn) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = hrgn ? LpObject((HGDIOBJ)hrgn) : NULL;
    RECT *rgrc = NULL;
    size_t crc = 0;

    if (!lpdc || (hrgn && (!lpobj || lpobj->iKind != objRegion))) return ERROR;
    if (lpobj && lpobj->rcRegion.left < lpobj->rcRegion.right && lpobj->rcRegion.top < lpobj->rcRegion.bottom) {
        rgrc = malloc(sizeof(*rgrc));
        if (!rgrc) return ERROR;
        *rgrc = lpobj->rcRegion; crc = 1;
    }
    free(lpdc->rgrcRegion); lpdc->rgrcRegion = rgrc; lpdc->crcRegion = crc;
    lpdc->fRegion = hrgn != NULL;
    return hrgn ? IRegionKind(crc) : SIMPLEREGION;
}

static void MergeRegion(RECT *rgrc, size_t *lpcrc) {
    size_t i, j;
    RECT *a, *b;
    BOOL fAgain = TRUE;

    while (fAgain) {
        fAgain = FALSE;
        for (i = 0; i < *lpcrc && !fAgain; i++) for (j = i + 1; j < *lpcrc; j++) {
            a = &rgrc[i]; b = &rgrc[j];
            if (a->left == b->left && a->right == b->right && (a->bottom == b->top || b->bottom == a->top)) {
                a->top = min(a->top, b->top); a->bottom = max(a->bottom, b->bottom);
            } else if (a->top == b->top && a->bottom == b->bottom && (a->right == b->left || b->right == a->left)) {
                a->left = min(a->left, b->left); a->right = max(a->right, b->right);
            } else continue;
            rgrc[j] = rgrc[--*lpcrc]; fAgain = TRUE; break;
        }
    }
}

static int IClipRect(HDC hdc, int left, int top, int right, int bottom, BOOL fExclude) {
    HDC lpdc = LpDC(hdc);
    RECT rcCut = {min(left, right), min(top, bottom), max(left, right), max(top, bottom)};
    RECT rcBase, rc, rcHit, *rgrc, *rgsrc;
    size_t i, crc = 0, crcSrc;

    if (!lpdc) return ERROR;
    rcCut = RcDevice(lpdc, rcCut);
    if (!lpdc->fRegion) {
        if (fExclude) {
            if (!FClip(lpdc, &rcBase)) return ERROR;
        } else rcBase = rcCut;
        rgsrc = &rcBase; crcSrc = 1;
    } else { rgsrc = lpdc->rgrcRegion; crcSrc = lpdc->crcRegion; }
    if (crcSrc > SIZE_MAX / (4 * sizeof(*rgrc))) return ERROR;
    rgrc = crcSrc ? malloc(crcSrc * 4 * sizeof(*rgrc)) : NULL;
    if (crcSrc != 0 && !rgrc) return ERROR;
    for (i = 0; i < crcSrc; i++) {
        rc = rgsrc[i];
        if (rc.left >= rc.right || rc.top >= rc.bottom) continue;
        if (!IntersectRect(&rcHit, &rc, &rcCut)) {
            if (fExclude) rgrc[crc++] = rc;
        } else if (!fExclude) rgrc[crc++] = rcHit;
        else {
            if (rc.top < rcHit.top) rgrc[crc++] = (RECT){rc.left, rc.top, rc.right, rcHit.top};
            if (rcHit.bottom < rc.bottom) rgrc[crc++] = (RECT){rc.left, rcHit.bottom, rc.right, rc.bottom};
            if (rc.left < rcHit.left) rgrc[crc++] = (RECT){rc.left, rcHit.top, rcHit.left, rcHit.bottom};
            if (rcHit.right < rc.right) rgrc[crc++] = (RECT){rcHit.right, rcHit.top, rc.right, rcHit.bottom};
        }
    }
    MergeRegion(rgrc, &crc);
    free(lpdc->rgrcRegion); lpdc->rgrcRegion = rgrc; lpdc->crcRegion = crc; lpdc->fRegion = TRUE;
    return IRegionKind(crc);
}

int WINAPI IntersectClipRect(HDC hdc, int left, int top, int right, int bottom) {
    return IClipRect(hdc, left, top, right, bottom, FALSE);
}

int WINAPI ExcludeClipRect(HDC hdc, int left, int top, int right, int bottom) {
    return IClipRect(hdc, left, top, right, bottom, TRUE);
}

static BOOL FRectPixel(const RECT *rgrc, size_t crc, int64_t x, int64_t y) {
    size_t i;
    for (i = 0; i < crc; i++)
        if (x >= rgrc[i].left && x < rgrc[i].right && y >= rgrc[i].top && y < rgrc[i].bottom) return TRUE;
    return FALSE;
}

static BOOL FRegionPixel(HDC lpdc, int64_t x, int64_t y) {
    return (!lpdc->fPaint || FRectPixel(lpdc->rgrcPaint, lpdc->crcPaint, x, y)) &&
           (!lpdc->fRegion || FRectPixel(lpdc->rgrcRegion, lpdc->crcRegion, x, y));
}

static void Pixel(HDC lpdc, const RECT *lprc, int64_t x, int64_t y, COLORREF crColor) {
    BYTE *rgb;

    if (x < lprc->left || x >= lprc->right || y < lprc->top || y >= lprc->bottom || !FRegionPixel(lpdc, x, y))
        return;
    crColor = CrPalette(lpdc, crColor);
    rgb = (BYTE *)LpDCSurface(lpdc)->pixels + (size_t)y * LpDCSurface(lpdc)->pitch + (size_t)x * 4;
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        crColor = ((BYTE)crColor + (BYTE)(crColor >> 8) + (BYTE)(crColor >> 16) >= 384) ?
                  RGB(255, 255, 255) : RGB(0, 0, 0);
    rgb[0] = (BYTE)crColor;
    rgb[1] = (BYTE)(crColor >> 8);
    rgb[2] = (BYTE)(crColor >> 16);
    rgb[3] = 255;
}

COLORREF WINAPI GetPixel(HDC hdc, int x, int y) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    BYTE *rgb;
    int64_t px, py;

    if (!lpdc || !FClip(lpdc, &rc)) return CLR_INVALID;
    px = (int64_t)x - lpdc->ptOrg.x; py = (int64_t)y - lpdc->ptOrg.y;
    if (px < rc.left || px >= rc.right || py < rc.top || py >= rc.bottom || !FRegionPixel(lpdc, px, py))
        return CLR_INVALID;
    rgb = (BYTE *)LpDCSurface(lpdc)->pixels + (size_t)py * LpDCSurface(lpdc)->pitch + (size_t)px * 4;
    return RGB(rgb[0], rgb[1], rgb[2]);
}

static void ShapePixel(HDC lpdc, const RECT *lprc, int64_t x, int64_t y, COLORREF crPen);

COLORREF WINAPI SetPixel(HDC hdc, int x, int y, COLORREF cr) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    int64_t px, py;

    if (!lpdc || !FColor(cr) || !FClip(lpdc, &rc)) return CLR_INVALID;
    px = (int64_t)x - lpdc->ptOrg.x; py = (int64_t)y - lpdc->ptOrg.y;
    if (px < rc.left || px >= rc.right || py < rc.top || py >= rc.bottom || !FRegionPixel(lpdc, px, py))
        return CLR_INVALID;
    cr = CrPalette(lpdc, cr);
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        cr = ((BYTE)cr + (BYTE)(cr >> 8) + (BYTE)(cr >> 16) >= 384) ? RGB(255, 255, 255) : 0;
    ShapePixel(lpdc, &rc, px, py, cr);
    return cr;
}

static COLORREF CrBrush(HDC lpdc, COMPATOBJECT *lpbrush, int64_t x, int64_t y) {
    SDL_Surface *lpsurface = lpbrush->lpSurface;
    BYTE *rgb;
    if (!lpsurface) return CrPalette(lpdc, lpbrush->crColor);
    x = (x - lpdc->ptBrush.x) % lpsurface->w; y = (y - lpdc->ptBrush.y) % lpsurface->h;
    if (x < 0) x += lpsurface->w;
    if (y < 0) y += lpsurface->h;
    rgb = (BYTE *)lpsurface->pixels + (size_t)y * lpsurface->pitch + (size_t)x * 4;
    // A monochrome pattern uses the destination DC's text/background colors.
    if (lpbrush->fMono) return CrPalette(lpdc, rgb[0] ? lpdc->crBk : lpdc->crText);
    return RGB(rgb[0], rgb[1], rgb[2]);
}

HBRUSH WINAPI CreatePatternBrush(HBITMAP hbmp) {
    COMPATOBJECT *lpbm = LpObject((HGDIOBJ)hbmp), *lpobj;
    SDL_Surface *lpsurface;
    if (!lpbm || lpbm->iKind != objBitmap) return NULL;
    lpsurface = SDL_DuplicateSurface(lpbm->lpSurface);
    if (!lpsurface) return NULL;
    lpobj = LpNewObject(objBrush, 0, FALSE);
    if (!lpobj) { SDL_DestroySurface(lpsurface); return NULL; }
    lpobj->lpSurface = lpsurface; lpobj->fMono = lpbm->fMono;
    return (HBRUSH)lpobj;
}

static BOOL FFillDevice(HDC lpdc, RECT rcFill, COMPATOBJECT *lpbrush, BOOL fRop);

int WINAPI FillRect(HDC hdc, const RECT *lprc, HBRUSH hbr) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT brush = {0}, *lpbrush = LpObject(hbr);
    if ((UINT_PTR)hbr > 0 && (UINT_PTR)hbr <= COLOR_MENUBAR + 1) {
        brush.iKind = objBrush; brush.crColor = GetSysColor((int)(UINT_PTR)hbr - 1); lpbrush = &brush;
    }
    if (!lpdc || !lprc || !lpbrush || lpbrush->iKind != objBrush) return 0;
    return FFillDevice(lpdc, RcDevice(lpdc, *lprc), lpbrush, FALSE);
}

void CompatDrawClassicEdge(HDC hdc, const RECT *lprc, BOOL fSunken, BOOL fRight) {
    RECT rc = *lprc, edge;
    HBRUSH hbr;
    int i;
    for (i = 0; i < 2 && rc.right > rc.left && rc.bottom > rc.top; i++) {
        hbr = CreateSolidBrush(GetSysColor(fSunken ? (i ? COLOR_3DDKSHADOW : COLOR_BTNSHADOW) :
                                          (i ? COLOR_3DLIGHT : COLOR_BTNHIGHLIGHT)));
        edge = rc; edge.bottom = rc.top + 1; FillRect(hdc, &edge, hbr);
        edge = rc; edge.right = rc.left + 1; FillRect(hdc, &edge, hbr); DeleteObject(hbr);
        hbr = CreateSolidBrush(GetSysColor(fSunken ? (i ? COLOR_3DLIGHT : COLOR_BTNHIGHLIGHT) :
                                          (i ? COLOR_BTNSHADOW : COLOR_3DDKSHADOW)));
        edge = rc; edge.top = rc.bottom - 1; FillRect(hdc, &edge, hbr);
        if (fRight) { edge = rc; edge.left = rc.right - 1; FillRect(hdc, &edge, hbr); }
        DeleteObject(hbr); InflateRect(&rc, -1, -1);
    }
}

int WINAPI FrameRect(HDC hdc, const RECT *lprc, HBRUSH hbr) {
    RECT rc;
    if (!lprc || lprc->left >= lprc->right || lprc->top >= lprc->bottom) return 0;
    rc = (RECT){lprc->left, lprc->top, lprc->right, lprc->top + 1};
    if (!FillRect(hdc, &rc, hbr)) return 0;
    rc = (RECT){lprc->left, lprc->bottom - 1, lprc->right, lprc->bottom};
    if (!FillRect(hdc, &rc, hbr)) return 0;
    rc = (RECT){lprc->left, lprc->top + 1, lprc->left + 1, lprc->bottom - 1};
    if (!FillRect(hdc, &rc, hbr)) return 0;
    rc = (RECT){lprc->right - 1, lprc->top + 1, lprc->right, lprc->bottom - 1};
    return FillRect(hdc, &rc, hbr);
}

static BOOL FUsesSource(BYTE rop) {
    return ((rop ^ (rop >> 2)) & 0x33) != 0;
}

static BOOL FUsesPattern(BYTE rop) {
    return ((rop ^ (rop >> 4)) & 0x0f) != 0;
}

static COLORREF CrSurfacePixel(SDL_Surface *lpsurface, int x, int y) {
    BYTE *rgb = (BYTE *)lpsurface->pixels + (size_t)y * lpsurface->pitch + (size_t)x * 4;
    return RGB(rgb[0], rgb[1], rgb[2]);
}

static void ShapePixel(HDC lpdc, const RECT *lprc, int64_t x, int64_t y, COLORREF crPen) {
    COLORREF crDest, cr = 0;
    int i;
    if (x < lprc->left || x >= lprc->right || y < lprc->top || y >= lprc->bottom || !FRegionPixel(lpdc, x, y)) return;
    crPen = CrPalette(lpdc, crPen);
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        crPen = ((BYTE)crPen + (BYTE)(crPen >> 8) + (BYTE)(crPen >> 16) >= 384) ? RGB(255, 255, 255) : 0;
    crDest = CrSurfacePixel(LpDCSurface(lpdc), (int)x, (int)y);
    for (i = 0; i < 4; i++)
        if ((lpdc->iRop2 - 1) & (1 << i))
            cr |= (i & 2 ? crPen : ~crPen) & (i & 1 ? crDest : ~crDest);
    Pixel(lpdc, lprc, x, y, cr & UINT32_C(0x00ffffff));
}

static BOOL FFillDevice(HDC lpdc, RECT rcFill, COMPATOBJECT *lpbrush, BOOL fRop) {
    RECT rc, rcClip, rcPart, rcPaint;
    SDL_Rect sdlrc;
    SDL_Surface *lpsurface;
    int x, y;
    size_t i, j, crc, crcPaint;
    COLORREF cr;
    if (!FClip(lpdc, &rcClip)) return FALSE;
    if (lpbrush->fNull || !IntersectRect(&rc, &rcFill, &rcClip)) return TRUE;
    // Solid copies are common for erase/paint and can fill entire clipped spans.
    if (!lpbrush->lpSurface && (!fRop || lpdc->iRop2 == R2_COPYPEN)) {
        lpsurface = LpDCSurface(lpdc); cr = CrPalette(lpdc, lpbrush->crColor);
        if (lpdc->fMemory && lpdc->lpbitmap->fMono)
            cr = ((BYTE)cr + (BYTE)(cr >> 8) + (BYTE)(cr >> 16) >= 384) ? RGB(255, 255, 255) : 0;
        crc = lpdc->fRegion ? lpdc->crcRegion : 1;
        for (i = 0; i < crc; i++) {
            if (lpdc->fRegion) {
                if (!IntersectRect(&rcPart, &rc, &lpdc->rgrcRegion[i])) continue;
            } else rcPart = rc;
            crcPaint = lpdc->fPaint ? lpdc->crcPaint : 1;
            for (j = 0; j < crcPaint; j++) {
                if (lpdc->fPaint) {
                    if (!IntersectRect(&rcPaint, &rcPart, &lpdc->rgrcPaint[j])) continue;
                } else rcPaint = rcPart;
                sdlrc = (SDL_Rect){rcPaint.left, rcPaint.top, rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top};
                if (!SDL_FillSurfaceRect(lpsurface, &sdlrc, SDL_MapSurfaceRGB(lpsurface,
                                        (BYTE)cr, (BYTE)(cr >> 8), (BYTE)(cr >> 16)))) return FALSE;
            }
        }
        return TRUE;
    }
    for (y = rc.top; y < rc.bottom; y++) for (x = rc.left; x < rc.right; x++) {
        cr = CrBrush(lpdc, lpbrush, x, y);
        if (fRop) ShapePixel(lpdc, &rc, x, y, cr);
        else Pixel(lpdc, &rc, x, y, cr);
    }
    return TRUE;
}

static COLORREF CrRop(BYTE rop, COLORREF crPattern, COLORREF crSource, COLORREF crDest) {
    COLORREF cr = 0;
    int i;

    // The high ROP byte is a truth table indexed by pattern/source/destination.
    for (i = 0; i < 8; i++)
        if (rop & (1 << i))
            cr |= (i & 4 ? crPattern : ~crPattern) &
                  (i & 2 ? crSource : ~crSource) & (i & 1 ? crDest : ~crDest);
    return cr & UINT32_C(0x00ffffff);
}

BOOL WINAPI BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int xSrc, int ySrc, DWORD dwRop) {
    HDC lpdc = LpDC(hdc), lpdcSrc = NULL;
    SDL_Surface *lpsrc = NULL, *lpdst;
    RECT rcClip;
    BYTE rop = (BYTE)(dwRop >> 16);
    BOOL fSource = FUsesSource(rop), fMonoSrc = FALSE, fMonoDst;
    int64_t px, py, pxSrc = 0, pySrc = 0;
    int64_t x0, y0, x1, y1, ix, iy, cxCopy, cyCopy;
    size_t cPixels, i;
    COLORREF *rgcr = NULL, crSource, crPattern, crDest;

    if (!lpdc || cx <= 0 || cy <= 0 || (dwRop & UINT32_C(0xff000000)) || !FClip(lpdc, &rcClip))
        return FALSE;
    if (FUsesPattern(rop) && lpdc->lpbrush->fNull) {
        SDL_SetError("Pattern-dependent blits require a non-null brush");
        return FALSE;
    }
    if (fSource) {
        lpdcSrc = LpDC(hdcSrc);
        if (!lpdcSrc || !(lpsrc = LpDCSurface(lpdcSrc)))
            return FALSE;
        fMonoSrc = lpdcSrc->fMemory && lpdcSrc->lpbitmap->fMono;
    }
    px = (int64_t)x - lpdc->ptOrg.x; py = (int64_t)y - lpdc->ptOrg.y;
    if (fSource) { pxSrc = (int64_t)xSrc - lpdcSrc->ptOrg.x; pySrc = (int64_t)ySrc - lpdcSrc->ptOrg.y; }
    lpdst = LpDCSurface(lpdc);
    fMonoDst = lpdc->fMemory && lpdc->lpbitmap->fMono;
    x0 = max((int64_t)0, (int64_t)rcClip.left - px);
    y0 = max((int64_t)0, (int64_t)rcClip.top - py);
    x1 = min((int64_t)cx, (int64_t)rcClip.right - px);
    y1 = min((int64_t)cy, (int64_t)rcClip.bottom - py);
    if (fSource) {
        x0 = max(x0, -pxSrc); y0 = max(y0, -pySrc);
        x1 = min(x1, (int64_t)lpsrc->w - pxSrc); y1 = min(y1, (int64_t)lpsrc->h - pySrc);
    }
    if (x0 >= x1 || y0 >= y1)
        return TRUE;
    cxCopy = x1 - x0; cyCopy = y1 - y0;
    if ((uint64_t)cxCopy > SIZE_MAX / sizeof(*rgcr) / (uint64_t)cyCopy)
        return FALSE;
    cPixels = (size_t)cxCopy * (size_t)cyCopy;
    if (fSource) {
        // Capture before writing: overlapping blits must not feed updated pixels back.
        rgcr = malloc(cPixels * sizeof(*rgcr));
        if (!rgcr)
            return FALSE;
        i = 0;
        for (iy = y0; iy < y1; iy++)
            for (ix = x0; ix < x1; ix++)
                rgcr[i++] = CrSurfacePixel(lpsrc, (int)(pxSrc + ix), (int)(pySrc + iy));
    }
    i = 0;
    for (iy = y0; iy < y1; iy++) {
        for (ix = x0; ix < x1; ix++) {
            crSource = fSource ? rgcr[i++] : 0;
            if (fSource && fMonoSrc && !fMonoDst)
                crSource = CrPalette(lpdc, crSource ? lpdc->crBk : lpdc->crText);
            else if (fSource && !fMonoSrc && fMonoDst)
                crSource = crSource == CrPalette(lpdcSrc, lpdcSrc->crBk) ? RGB(255, 255, 255) : 0;
            crPattern = CrBrush(lpdc, lpdc->lpbrush, px + ix, py + iy);
            if (fMonoDst) crPattern = ((BYTE)crPattern + (BYTE)(crPattern >> 8) + (BYTE)(crPattern >> 16) >= 384) ? RGB(255, 255, 255) : 0;
            crDest = CrSurfacePixel(lpdst, (int)(px + ix), (int)(py + iy));
            Pixel(lpdc, &rcClip, px + ix, py + iy,
                  CrRop(rop, crPattern, crSource, crDest));
        }
    }
    free(rgcr);
    return TRUE;
}

BOOL WINAPI ScrollWindow(HWND hwnd, int dx, int dy, const RECT *lprc, const RECT *lprcClip) {
    HWND lpwin = LpCompatWindow(hwnd), hwndChild, hwndNext;
    RECT rcClient, rcScroll, rcClip, rcArea, rcMoved, rcDest, rc;
    RECT *rgrc = NULL;
    size_t crc = 0, i;
    SDL_Surface *lpsurface;
    BYTE *lpSrc, *lpDst;
    int y, yFirst, yLast, dRow;
    size_t cbRow;
    BOOL fErase, fCopy, fArea;
    if (!lpwin || lpwin->fDestroying || !GetClientRect(hwnd, &rcClient)) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE); return FALSE;
    }
    if (dx == 0 && dy == 0) return TRUE;
    fArea = IntersectRect(&rcScroll, lprc ? lprc : &rcClient, &rcClient) &&
            IntersectRect(&rcClip, lprcClip ? lprcClip : &rcClient, &rcClient) &&
            IntersectRect(&rcArea, &rcScroll, &rcClip);
    if (!lprc) for (hwndChild = lpwin->hwndFirstChild; hwndChild; hwndChild = hwndChild->hwndZNext) {
        if ((int64_t)hwndChild->rcChild.left + dx < INT_MIN ||
            (int64_t)hwndChild->rcChild.right + dx > INT_MAX ||
            (int64_t)hwndChild->rcChild.top + dy < INT_MIN ||
            (int64_t)hwndChild->rcChild.bottom + dy > INT_MAX) {
            SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
        }
    }
    if (!fArea) goto MoveChildren;
    rcMoved = (RECT){LCoord((int64_t)rcArea.left + dx), LCoord((int64_t)rcArea.top + dy),
                     LCoord((int64_t)rcArea.right + dx), LCoord((int64_t)rcArea.bottom + dy)};
    fCopy = IntersectRect(&rcDest, &rcArea, &rcMoved);
    fErase = lpwin->fErase;
    for (i = 0; i < lpwin->crcUpdate; i++) {
        rc = lpwin->rgrcUpdate[i];
        if (!lprc) {
            // The whole-client variant moves pending damage with the pixels.
            if (!FSubtractUpdate(&rgrc, &crc, rc, &rcClip)) goto Fail;
            if (!IntersectRect(&rc, &rc, &rcClip)) continue;
            rc = (RECT){LCoord((int64_t)rc.left + dx), LCoord((int64_t)rc.top + dy),
                        LCoord((int64_t)rc.right + dx), LCoord((int64_t)rc.bottom + dy)};
            if (!IntersectRect(&rc, &rc, &rcClip)) continue;
        }
        if (!FAddUpdate(&rgrc, &crc, rc)) goto Fail;
    }
    if (!FSubtractUpdate(&rgrc, &crc, rcArea, &rcDest)) goto Fail;
    if (fCopy) {
        if (!FCanvas(hwnd)) goto Fail;
        lpsurface = lpwin->lpCanvas;
        cbRow = (size_t)(rcDest.right - rcDest.left) * 4;
        /* Window canvases are RGBA32. Copy overlapping rows in movement order,
           avoiding the generic raster-op path's per-pixel conversions. */
        dRow = dy > 0 ? -1 : 1;
        yFirst = dy > 0 ? rcDest.bottom - 1 : rcDest.top;
        yLast = dy > 0 ? rcDest.top - 1 : rcDest.bottom;
        for (y = yFirst; y != yLast; y += dRow) {
            lpDst = (BYTE *)lpsurface->pixels + (size_t)y * lpsurface->pitch + (size_t)rcDest.left * 4;
            lpSrc = (BYTE *)lpsurface->pixels + (size_t)((int64_t)y - dy) * lpsurface->pitch +
                    (size_t)((int64_t)rcDest.left - dx) * 4;
            memmove(lpDst, lpSrc, cbRow);
        }
    }
    SetUpdate(lpwin, rgrc, crc); rgrc = NULL;
    lpwin->fErase = crc != 0 && (fErase || !EqualRect(&rcArea, &rcDest));
MoveChildren:
    if (!lprc) for (hwndChild = lpwin->hwndFirstChild; hwndChild; hwndChild = hwndNext) {
        hwndNext = hwndChild->hwndZNext;
        if (!IsWindow(hwndChild)) continue;
        if (!SetWindowPos(hwndChild, NULL, (int)((int64_t)hwndChild->rcChild.left + dx),
                          (int)((int64_t)hwndChild->rcChild.top + dy), 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)) return FALSE;
        if (!IsWindow(hwnd)) return FALSE;
    }
    if (fArea) FCompatPresent(hwnd);
    return TRUE;
Fail:
    free(rgrc); SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE;
}


BOOL WINAPI PatBlt(HDC hdc, int x, int y, int cx, int cy, DWORD dwRop) {
    if (FUsesSource((BYTE)(dwRop >> 16)))
        return FALSE;
    return BitBlt(hdc, x, y, cx, cy, NULL, 0, 0, dwRop);
}

BOOL WINAPI MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc)
        return FALSE;
    if (lppt)
        *lppt = lpdc->pt;
    lpdc->pt = (POINT){x, y};
    return TRUE;
}

static void PenPixel(HDC lpdc, const RECT *lprc, int64_t x, int64_t y, uint64_t iPixel) {
    static const BYTE rgDash[4][6] = {{18, 6}, {3, 3}, {9, 6, 3, 6}, {9, 3, 3, 3, 3, 3}};
    static const BYTE rgc[] = {2, 2, 4, 6};
    int iStyle = lpdc->lppen->iPenStyle, i;
    unsigned cCycle = 0, iPhase;
    if (lpdc->lppen->fNull) return;
    if (iStyle == PS_SOLID) { ShapePixel(lpdc, lprc, x, y, lpdc->lppen->crColor); return; }
    for (i = 0; i < rgc[iStyle - 1]; i++) cCycle += rgDash[iStyle - 1][i];
    iPhase = (unsigned)(iPixel % cCycle);
    for (i = 0; i < rgc[iStyle - 1]; i++) {
        if (iPhase < rgDash[iStyle - 1][i]) {
            if (!(i & 1)) ShapePixel(lpdc, lprc, x, y, lpdc->lppen->crColor);
            else if (lpdc->iBkMode == OPAQUE) ShapePixel(lpdc, lprc, x, y, lpdc->crBk);
            return;
        }
        iPhase -= rgDash[iStyle - 1][i];
    }
}

// Liang-Barsky clipping bounds work by client size even with INT_MIN/MAX endpoints.
static BOOL FLineBound(double p, double q, double *lpt0, double *lpt1) {
    double t;

    if (p == 0)
        return q >= 0;
    t = q / p;
    if (p < 0) {
        if (t > *lpt1) return FALSE;
        if (t > *lpt0) *lpt0 = t;
    } else {
        if (t < *lpt0) return FALSE;
        if (t < *lpt1) *lpt1 = t;
    }
    return TRUE;
}

static void Line(HDC lpdc, RECT rc, POINT pt0, POINT pt1, BOOL fExcludeEnd, uint64_t *lpiPhase) {
    double xStart = (double)pt0.x - lpdc->ptOrg.x, yStart = (double)pt0.y - lpdc->ptOrg.y;
    double xEnd = (double)pt1.x - lpdc->ptOrg.x, yEnd = (double)pt1.y - lpdc->ptOrg.y;
    double dx = xEnd - xStart, dy = yEnd - yStart, t0 = 0, t1 = 1;
    int64_t x, y, x1, y1, cx, cy, sx, sy, err, e2;
    uint64_t iPixel, iPhase = lpiPhase ? *lpiPhase : 0;
    if (lpiPhase) *lpiPhase += (uint64_t)max(fabs(dx), fabs(dy)) + !fExcludeEnd;

    if (rc.left >= rc.right || rc.top >= rc.bottom || lpdc->lppen->fNull ||
        !FLineBound(-dx, xStart - rc.left, &t0, &t1) ||
        !FLineBound(dx, (double)rc.right - 1 - xStart, &t0, &t1) ||
        !FLineBound(-dy, yStart - rc.top, &t0, &t1) ||
        !FLineBound(dy, (double)rc.bottom - 1 - yStart, &t0, &t1))
        return;
    x = (int64_t)(xStart + t0 * dx);
    y = (int64_t)(yStart + t0 * dy);
    x1 = (int64_t)(xStart + t1 * dx);
    y1 = (int64_t)(yStart + t1 * dy);
    cx = llabs(x1 - x); cy = -llabs(y1 - y);
    sx = x < x1 ? 1 : -1; sy = y < y1 ? 1 : -1;
    err = cx + cy;
    iPixel = iPhase + (uint64_t)max(fabs((double)x - xStart), fabs((double)y - yStart));
    for (;;) {
        if (!(fExcludeEnd && x == xEnd && y == yEnd))
            PenPixel(lpdc, &rc, x, y, iPixel);
        if (x == x1 && y == y1)
            break;
        iPixel++;
        e2 = 2 * err;
        if (e2 >= cy) { err += cy; x += sx; }
        if (e2 <= cx) { err += cx; y += sy; }
    }
}

BOOL WINAPI LineTo(HDC hdc, int x, int y) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    POINT pt;

    if (!lpdc)
        return FALSE;
    pt = lpdc->pt;
    lpdc->pt = (POINT){x, y};
    if (!FClip(lpdc, &rc))
        return FALSE;
    Line(lpdc, rc, pt, lpdc->pt, TRUE, NULL);
    return TRUE;
}

BOOL WINAPI Rectangle(HDC hdc, int left, int top, int right, int bottom) {
    HDC lpdc = LpDC(hdc);
    RECT rc, rcFill;
    uint64_t iPhase = 0;
    int l = min(left, right), r = max(left, right), t = min(top, bottom), b = max(top, bottom);

    if (!lpdc)
        return FALSE;
    if (!FClip(lpdc, &rc))
        return FALSE;
    if (l == r || t == b)
        return TRUE;
    rcFill = lpdc->lppen->fNull ? (RECT){l, t, r, b} : (RECT){l + 1, t + 1, r - 1, b - 1};
    FFillDevice(lpdc, RcDevice(lpdc, rcFill), lpdc->lpbrush, TRUE);
    // Counterclockwise from top-right, sharing one dash phase and painting
    // each corner once, matches the default Win32 rectangle traversal.
    if (r - (int64_t)l == 1) Line(lpdc, rc, (POINT){l, t}, (POINT){l, b - 1}, FALSE, &iPhase);
    else if (b - (int64_t)t == 1) Line(lpdc, rc, (POINT){r - 1, t}, (POINT){l, t}, FALSE, &iPhase);
    else {
        Line(lpdc, rc, (POINT){r - 1, t}, (POINT){l, t}, TRUE, &iPhase);
        Line(lpdc, rc, (POINT){l, t}, (POINT){l, b - 1}, TRUE, &iPhase);
        Line(lpdc, rc, (POINT){l, b - 1}, (POINT){r - 1, b - 1}, TRUE, &iPhase);
        Line(lpdc, rc, (POINT){r - 1, b - 1}, (POINT){r - 1, t}, TRUE, &iPhase);
    }
    return TRUE;
}

typedef struct COMPATELLIPSE {
    RECT rc;
    int cx;
    BYTE *rgb;
    int64_t *rglLeft, *rglRight;
} COMPATELLIPSE;

static void EllipsePoint(COMPATELLIPSE *lp, int64_t x, int64_t y) {
    size_t iRow;
    if (y < lp->rc.top || y >= lp->rc.bottom) return;
    iRow = (size_t)(y - lp->rc.top);
    lp->rglLeft[iRow] = min(lp->rglLeft[iRow], x);
    lp->rglRight[iRow] = max(lp->rglRight[iRow], x);
    if (x >= lp->rc.left && x < lp->rc.right)
        lp->rgb[iRow * lp->cx + (size_t)(x - lp->rc.left)] = 1;
}

__extension__ typedef __int128 COMPATELLIPSEERROR;

/* plotEllipseRect adapted from Alois Zingl's Bresenham V20.15 (April 2020),
 * https://github.com/zingl/Bresenham/blob/master/bresenham.c .
 * MIT license: ../LICENSE.zingl. Large ellipses need wider error accumulators
 * than their coordinates; these private integers never reach the Win32 ABI.
 * Accumulate each border pixel once so mirrored tips cannot cancel under XOR. */
static void EllipseOutline(COMPATELLIPSE *lp, int64_t x0, int64_t y0, int64_t x1, int64_t y1) {
    int64_t a = x1 - x0, b = y1 - y0, b1 = b & 1;
    COMPATELLIPSEERROR dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a;
    COMPATELLIPSEERROR err = dx + dy + b1 * a * a, e2;
    BOOL fThin = a < 2;

    y0 += (b + 1) / 2; y1 = y0 - b1;
    a = 8 * a * a; b1 = 8 * b * b;
    do {
        EllipsePoint(lp, x1, y0); EllipsePoint(lp, x0, y0);
        EllipsePoint(lp, x0, y1); EllipsePoint(lp, x1, y1);
        e2 = 2 * err;
        if (e2 <= dy) { y0++; y1--; err += dy += a; }
        if (e2 >= dx) { x0++; x1--; err += dx += b1; }
    } while (x0 <= x1);
    while (fThin && y0 - y1 <= b) {
        EllipsePoint(lp, x0 - 1, y0); EllipsePoint(lp, x1 + 1, y0++);
        EllipsePoint(lp, x0 - 1, y1); EllipsePoint(lp, x1 + 1, y1--);
    }
}

BOOL WINAPI Ellipse(HDC hdc, int left, int top, int right, int bottom) {
    HDC lpdc = LpDC(hdc);
    COMPATELLIPSE el = {0};
    RECT rcClip;
    int64_t l, t, r, b;
    size_t cb, i;
    int x, y;
    BOOL fBorder;

    if (!lpdc || !FClip(lpdc, &rcClip)) return FALSE;
    l = (int64_t)min(left, right) - lpdc->ptOrg.x; r = (int64_t)max(left, right) - lpdc->ptOrg.x;
    t = (int64_t)min(top, bottom) - lpdc->ptOrg.y; b = (int64_t)max(top, bottom) - lpdc->ptOrg.y;
    if (l == r || t == b || (r - l == 1 && b - t == 1) || r <= rcClip.left || l >= rcClip.right || b <= rcClip.top || t >= rcClip.bottom) return TRUE;
    // Cap iteration work independently of the clipped mask size.
    if (r - l > 1048576 || b - t > 1048576) return FALSE;
    if (!lpdc->lppen->fNull && lpdc->lppen->iPenStyle != PS_SOLID) {
        SDL_SetError("Ellipse currently supports solid or null cosmetic pens"); return FALSE;
    }
    if (lpdc->lppen->fNull && (r - l <= 2 || b - t <= 2))
        return FFillDevice(lpdc, (RECT){LCoord(l), LCoord(t), LCoord(r - 1), LCoord(b - 1)}, lpdc->lpbrush, TRUE);
    el.rc = (RECT){(LONG)max(l, rcClip.left), (LONG)max(t, rcClip.top),
                   (LONG)min(r, rcClip.right), (LONG)min(b, rcClip.bottom)};
    el.cx = el.rc.right - el.rc.left;
    if ((size_t)el.cx > SIZE_MAX / (size_t)(el.rc.bottom - el.rc.top)) return FALSE;
    cb = (size_t)el.cx * (size_t)(el.rc.bottom - el.rc.top);
    el.rgb = calloc(cb, 1);
    el.rglLeft = malloc((size_t)(el.rc.bottom - el.rc.top) * sizeof(*el.rglLeft));
    el.rglRight = malloc((size_t)(el.rc.bottom - el.rc.top) * sizeof(*el.rglRight));
    if (!el.rgb || !el.rglLeft || !el.rglRight) {
        free(el.rgb); free(el.rglLeft); free(el.rglRight); return FALSE;
    }
    for (y = el.rc.top; y < el.rc.bottom; y++) {
        el.rglLeft[y - el.rc.top] = INT64_MAX; el.rglRight[y - el.rc.top] = INT64_MIN;
    }
    EllipseOutline(&el, l, t, r - 1, b - 1);
    for (y = el.rc.top; y < el.rc.bottom; y++) for (x = el.rc.left; x < el.rc.right; x++) {
        i = (size_t)(y - el.rc.top); fBorder = el.rgb[i * el.cx + (size_t)(x - el.rc.left)] != 0;
        if (fBorder && !lpdc->lppen->fNull) ShapePixel(lpdc, &el.rc, x, y, lpdc->lppen->crColor);
        else if (!lpdc->lpbrush->fNull && x >= el.rglLeft[i] && x <= el.rglRight[i])
            ShapePixel(lpdc, &el.rc, x, y, CrBrush(lpdc, lpdc->lpbrush, x, y));
    }
    free(el.rgb); free(el.rglLeft); free(el.rglRight);
    return TRUE;
}

COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor) {
    HDC lpdc = LpDC(hdc);
    COLORREF crOld;

    if (!lpdc || !FColor(crColor))
        return CLR_INVALID;
    crOld = lpdc->crText; lpdc->crText = crColor;
    return crOld;
}

COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor) {
    HDC lpdc = LpDC(hdc);
    COLORREF crOld;

    if (!lpdc || !FColor(crColor))
        return CLR_INVALID;
    crOld = lpdc->crBk; lpdc->crBk = crColor;
    return crOld;
}

int WINAPI SetBkMode(HDC hdc, int iMode) {
    HDC lpdc = LpDC(hdc);
    int lOld;

    if (!lpdc || (iMode != OPAQUE && iMode != TRANSPARENT))
        return 0;
    lOld = lpdc->iBkMode; lpdc->iBkMode = iMode;
    return lOld;
}

typedef struct COMPATTEXTDRAW {
    HDC lpdc;
    RECT rc;
    int64_t x, y;
    double cs, sn;
} COMPATTEXTDRAW;

static BOOL FTextUtf8(HDC lpdc) { return lpdc->hwnd && lpdc->hwnd->fUtf8Text; }

static BOOL FTextSize(HDC lpdc, LPCSTR psz, int cch, LPSIZE lpsize) {
    return FTextUtf8(lpdc) ? FCompatFontSizeUtf8(lpdc->lpfont->lpFont, psz, cch, lpsize) :
                           FCompatFontSize(lpdc->lpfont->lpFont, psz, cch, lpsize);
}

BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR psz, int cch, LPSIZE lpsize) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FTextSize(lpdc, psz, cch, lpsize);
}

BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FCompatFontMetrics(lpdc->lpfont->lpFont, lptm);
}

COLORREF WINAPI GetBkColor(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->crBk : CLR_INVALID;
}

int WINAPI GetDeviceCaps(HDC hdc, int index) {
    HDC lpdc = LpDC(hdc);
    const SDL_DisplayMode *lpMode;
    SDL_DisplayID id;
    if (!lpdc) return 0;
    if (lpdc->lpprint) return ICompatPrinterCaps(lpdc->lpprint, index);
    if (index == LOGPIXELSX || index == LOGPIXELSY) return 96;
    if (index == PLANES) return 1;
    if (index == BITSPIXEL) return 32;
    if (index != HORZRES && index != VERTRES) return 0;
    id = lpdc->hwnd ? SDL_GetDisplayForWindow(HwndCompatRoot(lpdc->hwnd)->lpWindow) : SDL_GetPrimaryDisplay();
    lpMode = SDL_GetCurrentDisplayMode(id);
    return lpMode ? index == HORZRES ? lpMode->w : lpMode->h : 0;
}

static void DrawFontPixel(void *lpv, int64_t x, int64_t y, BYTE alpha) {
    COMPATTEXTDRAW *lpdraw = lpv;
    int64_t px = (int64_t)lpdraw->x + llround(lpdraw->cs * x + lpdraw->sn * y);
    int64_t py = (int64_t)lpdraw->y + llround(-lpdraw->sn * x + lpdraw->cs * y);
    COLORREF cr;
    BYTE *rgb;
    unsigned r, g, b;

    if (px < lpdraw->rc.left || px >= lpdraw->rc.right || py < lpdraw->rc.top || py >= lpdraw->rc.bottom) return;
    cr = CrPalette(lpdraw->lpdc, lpdraw->lpdc->crText);
    rgb = (BYTE *)LpDCSurface(lpdraw->lpdc)->pixels + (size_t)py * LpDCSurface(lpdraw->lpdc)->pitch + (size_t)px * 4;
    r = ((BYTE)cr * alpha + rgb[0] * (255 - alpha) + 127) / 255;
    g = ((BYTE)(cr >> 8) * alpha + rgb[1] * (255 - alpha) + 127) / 255;
    b = ((BYTE)(cr >> 16) * alpha + rgb[2] * (255 - alpha) + 127) / 255;
    Pixel(lpdraw->lpdc, &lpdraw->rc, px, py, RGB(r, g, b));
}

static void DrawFontRect(COMPATTEXTDRAW *lpdraw, int64_t dx, int yTop, int yBottom, COLORREF cr) {
    double u, v, px, py, xMin = 1e30, yMin = 1e30, xMax = -1e30, yMax = -1e30;
    int i, x, y, xLeft, xRight, yFirst, yLast;
    for (i = 0; i < 4; i++) {
        u = i & 1 ? dx : 0; v = i & 2 ? yBottom : yTop;
        px = lpdraw->x + lpdraw->cs * u + lpdraw->sn * v;
        py = lpdraw->y - lpdraw->sn * u + lpdraw->cs * v;
        if (px < xMin) xMin = px;
        if (px > xMax) xMax = px;
        if (py < yMin) yMin = py;
        if (py > yMax) yMax = py;
    }
    xLeft = (int)fmax(lpdraw->rc.left, fmin(lpdraw->rc.right, floor(xMin)));
    xRight = (int)fmax(lpdraw->rc.left, fmin(lpdraw->rc.right, ceil(xMax)));
    yFirst = (int)fmax(lpdraw->rc.top, fmin(lpdraw->rc.bottom, floor(yMin)));
    yLast = (int)fmax(lpdraw->rc.top, fmin(lpdraw->rc.bottom, ceil(yMax)));
    for (y = yFirst; y < yLast; y++)
        for (x = xLeft; x < xRight; x++) {
            u = lpdraw->cs * ((int64_t)x - lpdraw->x) - lpdraw->sn * ((int64_t)y - lpdraw->y);
            v = lpdraw->sn * ((int64_t)x - lpdraw->x) + lpdraw->cs * ((int64_t)y - lpdraw->y);
            if (u >= fmin(0, dx) && u < fmax(0, dx) && v >= yTop && v < yBottom)
                Pixel(lpdraw->lpdc, &lpdraw->rc, x, y, cr);
        }
}

static BOOL FDrawText(HDC lpdc, int x, int y, LPCSTR psz, int cch, UINT options,
                      const RECT *lprc, const INT *rgdx) {
    COMPATTEXTDRAW draw;
    SIZE size;
    TEXTMETRICA tm;
    double angle;
    int i;
    int64_t dx;
    RECT rcDevice;
    if (!FTextSize(lpdc, psz, cch, &size) || !FClip(lpdc, &draw.rc) ||
        ((options & (ETO_OPAQUE | ETO_CLIPPED)) && !lprc)) return FALSE;
    if (lprc) { rcDevice = RcDevice(lpdc, *lprc); lprc = &rcDevice; }
    // ETO_OPAQUE uses the DC background color, independent of its selected brush.
    if (options & ETO_OPAQUE) {
        RECT rc;
        int ix, iy;
        if (IntersectRect(&rc, &draw.rc, lprc))
            for (iy = rc.top; iy < rc.bottom; iy++) for (ix = rc.left; ix < rc.right; ix++) Pixel(lpdc, &rc, ix, iy, lpdc->crBk);
    }
    if (options & ETO_CLIPPED) IntersectRect(&draw.rc, &draw.rc, lprc);
    draw.lpdc = lpdc; draw.x = (int64_t)x - lpdc->ptOrg.x; draw.y = (int64_t)y - lpdc->ptOrg.y;
    angle = (lpdc->lpfont->lf.lfEscapement % 3600) * (3.14159265358979323846 / 1800.0);
    draw.cs = cos(angle); draw.sn = sin(angle);
    dx = size.cx;
    if (rgdx) { dx = 0; for (i = 0; i < cch; i++) dx += rgdx[i]; }
    FCompatFontMetrics(lpdc->lpfont->lpFont, &tm);
    if (lpdc->iBkMode == OPAQUE) DrawFontRect(&draw, dx, 0, tm.tmHeight, lpdc->crBk);
    if (FTextUtf8(lpdc)) {
        if (rgdx || !FCompatFontDrawUtf8(lpdc->lpfont->lpFont, psz, cch, DrawFontPixel, &draw)) return FALSE;
    } else if (!FCompatFontDraw(lpdc->lpfont->lpFont, psz, cch, rgdx, DrawFontPixel, &draw)) return FALSE;
    if (tm.tmUnderlined) DrawFontRect(&draw, dx, tm.tmAscent + 1, tm.tmAscent + 2, lpdc->crText);
    if (tm.tmStruckOut) DrawFontRect(&draw, dx, tm.tmAscent * 2 / 3, tm.tmAscent * 2 / 3 + 1, lpdc->crText);
    return TRUE;
}

BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR psz, int cch) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FDrawText(lpdc, x, y, psz, cch, 0, NULL, NULL);
}

BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT *lprc,
                       LPCSTR psz, UINT cch, const INT *rgdx) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc || cch > INT_MAX || (options & ~(ETO_OPAQUE | ETO_CLIPPED))) return FALSE;
    return FDrawText(lpdc, x, y, psz, (int)cch, options, lprc, rgdx);
}

typedef struct COMPATTEXTLINE { int i, cch, dx; } COMPATTEXTLINE;

int WINAPI DrawTextA(HDC hdc, LPCSTR psz, int cch, LPRECT lprc, UINT flags) {
    HDC lpdc = LpDC(hdc);
    char *sz = NULL;
    BYTE *rgUnderline = NULL;
    COMPATTEXTLINE *rgline = NULL;
    TEXTMETRICA tm;
    SIZE size;
    RECT rcOld;
    BOOL fOldClip, fUnderline = FALSE;
    int i, j, c = 0, cLines = 0, iStart, iBreak, dxBreak, dxMax = 0, dyLine, dy, nReturn = 0, cStep;
    uint32_t ch;
    int64_t dx, dxAvail, x, y;
    UINT supported = DT_CENTER | DT_RIGHT | DT_VCENTER | DT_BOTTOM | DT_WORDBREAK | DT_SINGLELINE |
                     DT_EXPANDTABS | DT_NOCLIP | DT_EXTERNALLEADING | DT_CALCRECT | DT_NOPREFIX;

    if (!lpdc || !lprc || !psz || cch < -1 || (flags & ~supported)) return 0;
    if (cch == -1) { cch = (int)strnlen(psz, 1024 * 1024 + 1); }
    if (cch > 1024 * 1024) return 0;
    if (cch == 0) { if (flags & DT_CALCRECT) lprc->bottom = lprc->top; return 0; }
    sz = malloc((size_t)cch * 8 + 1); rgUnderline = calloc((size_t)cch * 8 + 1, 1);
    rgline = malloc(((size_t)cch * 8 + 1) * sizeof(*rgline));
    if (!sz || !rgUnderline || !rgline) goto LReturn;
    for (i = 0; i < cch; i++) {
        if (!(flags & DT_NOPREFIX) && psz[i] == '&') {
            if (i + 1 < cch && psz[i + 1] == '&') i++;
            else { fUnderline = TRUE; continue; }
        }
        if ((flags & DT_EXPANDTABS) && psz[i] == '\t') {
            do { sz[c++] = ' '; } while (c % 8);
        } else if ((flags & DT_SINGLELINE) && (psz[i] == '\r' || psz[i] == '\n')) {
            if (psz[i] == '\r' && i + 1 < cch && psz[i + 1] == '\n') i++;
            sz[c++] = ' ';
        } else {
            sz[c] = psz[i]; rgUnderline[c++] = fUnderline;
        }
        fUnderline = FALSE;
    }
    sz[c] = 0;
    FCompatFontMetrics(lpdc->lpfont->lpFont, &tm);
    dyLine = tm.tmHeight + (flags & DT_EXTERNALLEADING ? tm.tmExternalLeading : 0);
    dxAvail = (int64_t)lprc->right - lprc->left;
    i = 0;
    while (i < c) {
        iStart = i; iBreak = -1; dxBreak = 0; dx = 0;
        while (i < c && sz[i] != '\r' && sz[i] != '\n') {
            cStep = FTextUtf8(lpdc) ? CbUtf8Char(sz + i, (size_t)(c - i), &ch) : 1;
            if (cStep == 0) cStep = 1;
            if (!GetTextExtentPoint32A(hdc, sz + i, cStep, &size)) goto LReturn;
            if (sz[i] == ' ' || sz[i] == '\t') { iBreak = i; dxBreak = (int)dx; }
            dx += size.cx;
            if (dx > INT_MAX) goto LReturn;
            if ((flags & DT_WORDBREAK) && !(flags & DT_SINGLELINE) && dx > dxAvail && iBreak > iStart) {
                rgline[cLines++] = (COMPATTEXTLINE){iStart, iBreak - iStart, dxBreak};
                i = iBreak + 1;
                while (i < c && sz[i] == ' ') i++;
                goto LNextLine;
            }
            i += cStep;
        }
        rgline[cLines++] = (COMPATTEXTLINE){iStart, i - iStart, (int)dx};
        if (i < c && sz[i++] == '\r' && i < c && sz[i] == '\n') i++;
LNextLine:
        if (rgline[cLines - 1].dx > dxMax) dxMax = rgline[cLines - 1].dx;
    }
    if ((int64_t)cLines * dyLine > INT_MAX) goto LReturn;
    dy = cLines * dyLine;
    if (flags & DT_CALCRECT) {
        if ((int64_t)lprc->left + dxMax > INT_MAX || (int64_t)lprc->top + dy > INT_MAX) goto LReturn;
        lprc->right = lprc->left + dxMax; lprc->bottom = lprc->top + dy;
        nReturn = dy; goto LReturn;
    }
    rcOld = lpdc->rcClip; fOldClip = lpdc->fClip;
    if (!(flags & DT_NOCLIP)) {
        lpdc->rcClip = RcDevice(lpdc, *lprc);
        if (fOldClip) IntersectRect(&lpdc->rcClip, &rcOld, &lpdc->rcClip);
        lpdc->fClip = TRUE;
    }
    y = lprc->top;
    if (flags & DT_SINGLELINE) {
        if (flags & DT_VCENTER) y += ((int64_t)lprc->bottom - lprc->top - dy) / 2;
        else if (flags & DT_BOTTOM) y = (int64_t)lprc->bottom - dy;
    }
    nReturn = dy;
    for (i = 0; i < cLines; i++, y += dyLine) {
        x = lprc->left;
        if (flags & DT_CENTER) x += ((int64_t)lprc->right - lprc->left - rgline[i].dx) / 2;
        else if (flags & DT_RIGHT) x = (int64_t)lprc->right - rgline[i].dx;
        if (x < INT_MIN || x > INT_MAX || y < INT_MIN || y > INT_MAX ||
            !TextOutA(hdc, (int)x, (int)y, sz + rgline[i].i, rgline[i].cch)) { nReturn = 0; break; }
        for (j = 0; j < rgline[i].cch; j++) {
            if (!rgUnderline[rgline[i].i + j]) continue;
            GetTextExtentPoint32A(hdc, sz + rgline[i].i, j, &size);
            dx = x + size.cx;
            GetTextExtentPoint32A(hdc, sz + rgline[i].i + j, 1, &size);
            // Leave a pixel below the baseline, matching ordinary font
            // underlines, so mnemonic marks do not merge into the glyph.
            if (dx >= INT_MIN && dx + size.cx <= INT_MAX && y + tm.tmAscent + 2 <= INT_MAX) {
                RECT rc = {(LONG)dx, (LONG)(y + tm.tmAscent + 1), (LONG)(dx + size.cx), (LONG)(y + tm.tmAscent + 2)};
                COMPATTEXTDRAW draw = {lpdc, {0}, dx - lpdc->ptOrg.x, -(int64_t)lpdc->ptOrg.y, 1, 0};
                if (FClip(lpdc, &draw.rc)) DrawFontRect(&draw, rc.right - rc.left, rc.top, rc.bottom, lpdc->crText);
            }
        }
    }
    lpdc->rcClip = rcOld; lpdc->fClip = fOldClip;
LReturn:
    free(sz); free(rgUnderline); free(rgline);
    return nReturn;
}

HBITMAP HbmpCompatResource(const BYTE *rgb, size_t cb) {
    BITMAPINFOHEADER bi;
    COMPATDIB dib;
    COMPATOBJECT *lpobj;
    SDL_Surface *lpsurface;
    size_t cbInfo, cColors;

    if (!rgb || cb < sizeof(bi))
        return NULL;
    memcpy(&bi, rgb, sizeof(bi));
    cColors = bi.biClrUsed ? bi.biClrUsed : (bi.biBitCount <= 8 ? 1u << bi.biBitCount : 0);
    if (cColors > 256)
        return NULL;
    cbInfo = sizeof(bi) + cColors * sizeof(RGBQUAD);
    if (cbInfo > cb || !FCompatDib(rgb, cbInfo, rgb + cbInfo, cb - cbInfo, &dib))
        return NULL;
    lpsurface = LpCompatDibSurface(&dib);
    if (!lpsurface)
        return NULL;
    lpobj = LpNewObject(objBitmap, 0, FALSE);
    if (!lpobj) {
        SDL_DestroySurface(lpsurface);
        return NULL;
    }
    lpobj->lpSurface = lpsurface;
    lpobj->fMono = dib.cBits == 1;
    return (HBITMAP)lpobj;
}

int WINAPI GetDIBits(HDC hdc, HBITMAP hbm, UINT iStart, UINT cLines, LPVOID lpBits, LPBITMAPINFO lpbmi, UINT usage) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject(hbm);
    BITMAPINFOHEADER bi;
    SDL_Surface *lpsurface;
    size_t cbRow;
    UINT i, cCopy;
    int x, y;
    COLORREF cr;
    BYTE *rgb;
    BOOL fTopDown, fQuery;

    if (!lpdc || !lpobj || lpobj->iKind != objBitmap || lpobj->cSelected != 0 ||
        !lpbmi || usage != DIB_RGB_COLORS || lpbmi->bmiHeader.biSize != sizeof(bi))
        return 0;
    lpsurface = lpobj->lpSurface;
    bi = lpbmi->bmiHeader;
    fQuery = bi.biBitCount == 0;
    if (fQuery && lpBits)
        return 0;
    if (fQuery)
        bi.biBitCount = lpobj->fMono ? 1 : 32;
    if ((bi.biBitCount != 1 && bi.biBitCount != 24 && bi.biBitCount != 32) || bi.biCompression != BI_RGB ||
        (bi.biBitCount == 1 && !lpobj->fMono) ||
        (bi.biWidth != 0 && bi.biWidth != lpsurface->w) ||
        (bi.biHeight != 0 && bi.biHeight != lpsurface->h && bi.biHeight != -lpsurface->h))
        return 0;
    fTopDown = bi.biHeight < 0;
    bi.biWidth = lpsurface->w;
    bi.biHeight = fTopDown ? -lpsurface->h : lpsurface->h;
    bi.biPlanes = 1;
    bi.biClrUsed = bi.biBitCount == 1 ? 2 : 0;
    cbRow = (((size_t)lpsurface->w * bi.biBitCount + 31) / 32) * 4;
    if (cbRow > UINT32_MAX / (size_t)lpsurface->h)
        return 0;
    bi.biSizeImage = (DWORD)(cbRow * (size_t)lpsurface->h);
    lpbmi->bmiHeader = bi;
    // A format query may supply only the 40-byte header, with no palette storage.
    if (bi.biBitCount == 1 && !fQuery) {
        lpbmi->bmiColors[0] = (RGBQUAD){0, 0, 0, 0};
        lpbmi->bmiColors[1] = (RGBQUAD){255, 255, 255, 0};
    }
    if (!lpBits)
        return lpsurface->h;
    if (iStart >= (UINT)lpsurface->h)
        return 0;
    cCopy = min(cLines, (UINT)lpsurface->h - iStart);
    for (i = 0; i < cCopy; i++) {
        rgb = (BYTE *)lpBits + (size_t)i * cbRow;
        memset(rgb, 0, cbRow);
        y = fTopDown ? (int)(iStart + i) : lpsurface->h - 1 - (int)(iStart + i);
        for (x = 0; x < lpsurface->w; x++) {
            cr = CrSurfacePixel(lpsurface, x, y);
            if (bi.biBitCount == 1) {
                if ((BYTE)cr + (BYTE)(cr >> 8) + (BYTE)(cr >> 16) >= 384)
                    rgb[x / 8] |= (BYTE)(1 << (7 - x % 8));
            } else {
                rgb[x * (bi.biBitCount / 8)] = (BYTE)(cr >> 16);
                rgb[x * (bi.biBitCount / 8) + 1] = (BYTE)(cr >> 8);
                rgb[x * (bi.biBitCount / 8) + 2] = (BYTE)cr;
            }
        }
    }
    return (int)cCopy;
}

static BOOL FPaletteDib(HDC lpdc, const BITMAPINFO *lpbmi, const void *lpBits, UINT usage, COMPATDIB *lpdib) {
    struct { BITMAPINFOHEADER bi; RGBQUAD rgq[256]; } info;
    UINT c, i;
    WORD index;
    PALETTEENTRY pe;
    const BYTE *rgb = (const BYTE *)lpbmi;
    if (usage == DIB_RGB_COLORS) return FCompatDib(lpbmi, SIZE_MAX, lpBits, SIZE_MAX, lpdib);
    if (usage != DIB_PAL_COLORS || !lpbmi || lpbmi->bmiHeader.biSize != sizeof(info.bi)) return FALSE;
    info.bi = lpbmi->bmiHeader;
    if (info.bi.biBitCount != 1 && info.bi.biBitCount != 4 && info.bi.biBitCount != 8) return FALSE;
    c = info.bi.biClrUsed ? info.bi.biClrUsed : 1u << info.bi.biBitCount;
    if (c > (1u << info.bi.biBitCount)) return FALSE;
    for (i = 0; i < c; i++) {
        memcpy(&index, rgb + sizeof(info.bi) + i * sizeof(index), sizeof(index));
        if (index >= lpdc->lppalette->cEntries) return FALSE;
        pe = lpdc->lppalette->rgpe[index];
        info.rgq[i] = (RGBQUAD){pe.peBlue, pe.peGreen, pe.peRed, 0};
    }
    // The decoder copies the color table; only the caller's bits remain borrowed.
    return FCompatDib(&info, sizeof(info), lpBits, SIZE_MAX, lpdib);
}

int WINAPI StretchDIBits(HDC hdc, int xDest, int yDest, int cxDest, int cyDest,
                         int xSrc, int ySrc, int cxSrc, int cySrc, const VOID *lpBits,
                         const BITMAPINFO *lpbmi, UINT usage, DWORD dwRop) {
    HDC lpdc = LpDC(hdc);
    COMPATDIB dib;
    SDL_Surface *lpsrc, *lpdst;
    RECT rcClip;
    BYTE rop = (BYTE)(dwRop >> 16);
    int64_t px, py;
    int64_t x0, y0, x1, y1, ix, iy, sx, sy, yTop;
    COLORREF crSource, crPattern, crDest;

    if (!lpdc || (lpdc->fMemory && lpdc->lpbitmap->fMono) ||
        cxDest <= 0 || cyDest <= 0 || cxSrc <= 0 || cySrc <= 0 ||
        (dwRop & UINT32_C(0xff000000)) || !FPaletteDib(lpdc, lpbmi, lpBits, usage, &dib))
        return 0;
    yTop = dib.fTopDown ? ySrc : (int64_t)dib.cy - ySrc - cySrc;
    if (xSrc < 0 || yTop < 0 || (int64_t)xSrc + cxSrc > dib.cx || yTop + cySrc > dib.cy ||
        (FUsesPattern(rop) && lpdc->lpbrush->fNull) || !FClip(lpdc, &rcClip))
        return 0;
    lpsrc = LpCompatDibSurface(&dib);
    if (!lpsrc)
        return 0;
    px = (int64_t)xDest - lpdc->ptOrg.x; py = (int64_t)yDest - lpdc->ptOrg.y;
    lpdst = LpDCSurface(lpdc);
    x0 = max((int64_t)0, (int64_t)rcClip.left - px);
    y0 = max((int64_t)0, (int64_t)rcClip.top - py);
    x1 = min((int64_t)cxDest, (int64_t)rcClip.right - px);
    y1 = min((int64_t)cyDest, (int64_t)rcClip.bottom - py);
    for (iy = y0; iy < y1; iy++) {
        sy = yTop + iy * cySrc / cyDest;
        for (ix = x0; ix < x1; ix++) {
            sx = (int64_t)xSrc + ix * cxSrc / cxDest;
            crPattern = CrBrush(lpdc, lpdc->lpbrush, px + ix, py + iy);
            crSource = CrSurfacePixel(lpsrc, (int)sx, (int)sy);
            crDest = CrSurfacePixel(lpdst, (int)(px + ix), (int)(py + iy));
            Pixel(lpdc, &rcClip, px + ix, py + iy,
                  CrRop(rop, crPattern, crSource, crDest));
        }
    }
    SDL_DestroySurface(lpsrc);
    return cySrc;
}

void CompatDestroyPaint(HWND hwnd) {
    HDC lpdc, lpnext;

    for (lpdc = lpDCs; lpdc; lpdc = lpnext) {
        lpnext = lpdc->lpNext;
        if (lpdc->hwnd == hwnd)
            CloseDC(lpdc);
    }
    SDL_DestroyTexture(hwnd->lpTexture);
    SDL_DestroyRenderer(hwnd->lpRenderer);
    SDL_DestroySurface(hwnd->lpCanvas);
    SDL_DestroySurface(hwnd->lpComposite);
    hwnd->lpComposite = NULL;
    hwnd->lpTexture = NULL;
    hwnd->lpRenderer = NULL;
    hwnd->lpCanvas = NULL;
    hwnd->hdcPaint = NULL;
    SetUpdate(hwnd, NULL, 0);
}

void CompatShutdownGdi(void) {
    HDC lpdc;
    COMPATOBJECT *lpobj;
    int i;

    while (lpDCs) {
        lpdc = lpDCs; CloseDC(lpdc);
    }
    while (lpObjects) {
        lpobj = lpObjects; lpObjects = lpobj->lpNext;
        SDL_DestroySurface(lpobj->lpSurface);
        FreeCompatFont(lpobj->lpFont);
        free(lpobj->rgpe);
        free(lpobj);
    }
    for (i = 0; i < 18; i++) FreeCompatFont(rgStock[i].lpFont);
    CompatShutdownFonts();
    memset(rgStock, 0, sizeof(rgStock));
    fStock = FALSE;
}

SDL_Surface *LpCompatBitmapSurface(HBITMAP hbmp) {
    COMPATOBJECT *lpobj = LpObject(hbmp);
    return lpobj && lpobj->iKind == objBitmap ? lpobj->lpSurface : NULL;
}

HBITMAP HbmpCompatSurface(SDL_Surface *lpSurface, BOOL fMono) {
    COMPATOBJECT *lpobj;
    SDL_Surface *lpCopy;
    if (!lpSurface || !(lpCopy = SDL_DuplicateSurface(lpSurface))) return NULL;
    lpobj = LpNewObject(objBitmap, 0, FALSE);
    if (!lpobj) { SDL_DestroySurface(lpCopy); return NULL; }
    lpobj->lpSurface = lpCopy; lpobj->fMono = fMono;
    return (HBITMAP)lpobj;
}
BOOL WINAPI DrawIcon(HDC hdc, int x, int y, HICON hicon) {
    HDC lpdc = LpDC(hdc);
    COMPATIMAGE *lp = LpCompatImage(hicon);
    RECT rc;
    int ix, iy, sx, sy;
    int64_t dx, dy;
    COLORREF crDst, crSrc;
    if (!lpdc || !lp || !lp->lpXor || !FClip(lpdc, &rc)) return FALSE;
    for (iy = 0; iy < 32; iy++) for (ix = 0; ix < 32; ix++) {
        dx = (int64_t)x - lpdc->ptOrg.x + ix; dy = (int64_t)y - lpdc->ptOrg.y + iy;
        if (dx < rc.left || dy < rc.top || dx >= rc.right || dy >= rc.bottom || !FRegionPixel(lpdc, dx, dy)) continue;
        sx = ix * lp->lpXor->w / 32; sy = iy * lp->lpXor->h / 32;
        crDst = CrSurfacePixel(LpDCSurface(lpdc), (int)dx, (int)dy); crSrc = CrSurfacePixel(lp->lpXor, sx, sy);
        Pixel(lpdc, &rc, dx, dy, (lp->rgbAnd[sy * lp->lpXor->w + sx] ? crDst : 0) ^ crSrc);
    }
    return lpdc->fMemory || lpdc->fScreen || FCompatPresent(lpdc->hwnd);
}
