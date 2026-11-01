/* Basic GDI with logical-to-device translation. Rasterization stays deterministic on a CPU
 * canvas; SDL's renderer uploads/presents each window through its own texture.
 * No native-sized state in this module reaches game files.
 */
#include "internal.h"
#include "encoding.h"
#include <algorithm>
#include "display_io.h"
#include "input_io.h"
#include "render_io.h"
#include "resourcecodec.h"
#include "ownership.h"
#include <climits>
#include <cmath>
#include <new>
#include <cstdlib>
#include <cstring>

constexpr auto objPen = 1;
constexpr auto objBrush = 2;
constexpr auto objFont = 3;
constexpr auto objBitmap = 4;
constexpr auto objRegion = 5;
constexpr auto objPalette = 6;

struct COMPATOBJECT {
    int iKind, cSelected, iPenStyle;
    COLORREF crColor;
    BOOL fNull, fStock, fAlive, fMono;
    NATIVEARRAYOWNER<PALETTEENTRY, std::free> rgpe;
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpFont;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpSurface;
    LOGFONTA lf;
    UINT cEntries;
    RECT rcRegion;
    struct COMPATOBJECT *lpNext;
};

struct win32::HDC__ {
    NATIVEOWNER<COMPATPRINT, CompatDeletePrinter> lpprint;
    HDC hdcToken;
    HWND hwnd;
    RECT rcClip;
    POINT pt, ptOrg, ptBrush;
    NATIVEARRAYOWNER<RECT, std::free> rgrcRegion, rgrcPaint;
    std::size_t crcPaint;
    std::size_t crcRegion;
    BOOL fRegion;
    COMPATOBJECT *lppen, *lpbrush, *lpfont, *lpbitmap, *lpbitmapDefault, *lppalette;
    COLORREF crText, crBk;
    int iBkMode, iRop2;
    BOOL fAlive, fPaint, fClip, fMemory, fScreen;
    int iScale;
    struct win32::HDC__ *lpNext;
};

static COMPATOBJECT *lpObjects;
static HDC lpDCs;
static std::uintptr_t idDCNext = 1;
static COMPATOBJECT rgStock[18];
static BOOL fStock;
static PALETTEENTRY rgpeDefault[] = {
    {0, 0, 0, 0}, {128, 0, 0, 0}, {0, 128, 0, 0}, {128, 128, 0, 0},
    {0, 0, 128, 0}, {128, 0, 128, 0}, {0, 128, 128, 0}, {192, 192, 192, 0},
    {192, 220, 192, 0}, {166, 202, 240, 0}, {255, 251, 240, 0}, {160, 160, 164, 0},
    {128, 128, 128, 0}, {255, 0, 0, 0}, {0, 255, 0, 0}, {255, 255, 0, 0},
    {0, 0, 255, 0}, {255, 0, 255, 0}, {0, 255, 255, 0}, {255, 255, 255, 0}
};

// The stock palette borrows static entries; custom palettes own their storage.
static PALETTEENTRY *RgpeCompatPalette(COMPATOBJECT *lpobj) {
    return lpobj->fStock ? rgpeDefault : lpobj->rgpe.get();
}

static BOOL FColor(COLORREF cr) {
    return (cr >> 24) <= 2;
}
static COLORREF CrPalette(HDC lpdc, COLORREF cr) {
    UINT i;
    PALETTEENTRY pe;
    if ((cr >> 24) != 1) return cr & UINT32_C(0x00ffffff);
    i = static_cast<WORD>(cr);
    if (i >= lpdc->lppalette->cEntries) return win32::RGB(0, 0, 0);
    pe = RgpeCompatPalette(lpdc->lppalette)[i];
    return win32::RGB(pe.peRed, pe.peGreen, pe.peBlue);
}

static COMPATOBJECT *LpObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj;
    int i;

    if (!SDL_IsMainThread())
        return nullptr;
    for (i = 0; i < 18; i++)
        if (h == &rgStock[i] && rgStock[i].fAlive)
            return &rgStock[i];
    for (lpobj = lpObjects; lpobj; lpobj = lpobj->lpNext)
        if (h == lpobj && lpobj->fAlive)
            return lpobj;
    return nullptr;
}

static HDC LpDC(HDC hdc) {
    HDC lpdc;

    if (!SDL_IsMainThread())
        return nullptr;
    for (lpdc = lpDCs; lpdc; lpdc = lpdc->lpNext)
        if (lpdc->hdcToken == hdc && lpdc->fAlive && (lpdc->fMemory || lpdc->fScreen || LpCompatWindow(lpdc->hwnd)))
            return lpdc;
    return nullptr;
}

HGDIOBJ win32::GetStockObject(int i) {
    int j;
    static const BYTE rgGray[] = {255, 192, 128, 64, 0, 0};

    if (!SDL_IsMainThread())
        return nullptr;
    if (!fStock) {
        for (j = 0; j < 18; j++) {
            rgStock[j].fStock = 1;
            if (j <= win32::NULL_BRUSH) {
                rgStock[j].iKind = objBrush;
                rgStock[j].crColor = win32::RGB(rgGray[j], rgGray[j], rgGray[j]);
                rgStock[j].fNull = j == win32::NULL_BRUSH;
                rgStock[j].fAlive = 1;
            } else if (j <= win32::NULL_PEN) {
                rgStock[j].iKind = objPen;
                rgStock[j].crColor = j == win32::WHITE_PEN ? win32::RGB(255, 255, 255) : win32::RGB(0, 0, 0);
                rgStock[j].fNull = j == win32::NULL_PEN;
                rgStock[j].fAlive = 1;
            } else if (j == win32::DEFAULT_PALETTE) {
                rgStock[j].iKind = objPalette;
                rgStock[j].cEntries = sizeof(rgpeDefault) / sizeof(rgpeDefault[0]);
                rgStock[j].fAlive = 1;
            } else if (j == win32::SYSTEM_FONT || j == win32::SYSTEM_FIXED_FONT ||
                       j == win32::ANSI_FIXED_FONT || j == win32::OEM_FIXED_FONT || j == win32::ANSI_VAR_FONT ||
                       j == win32::DEVICE_DEFAULT_FONT || j == win32::DEFAULT_GUI_FONT) {
                rgStock[j].iKind = objFont;
                rgStock[j].fAlive = 1;
            }
        }
        fStock = 1;
    }
    if (i < 0 || i >= 18 || !rgStock[i].fAlive) return nullptr;
    if (rgStock[i].iKind == objFont && !rgStock[i].lpFont.get()) {
        rgStock[i].lf.lfHeight = -12;
        CchCopyUtf8Prefix(std::span{rgStock[i].lf.lfFaceName}, i == win32::SYSTEM_FIXED_FONT || i == win32::ANSI_FIXED_FONT || i == win32::OEM_FIXED_FONT ?
               "monospace" : "sans-serif");
        rgStock[i].lpFont.reset(LpNewCompatFont(&rgStock[i].lf));
        if (!rgStock[i].lpFont.get()) return nullptr;
    }
    return &rgStock[i];
}

static COMPATOBJECT *LpNewObject(int iKind, COLORREF crColor, BOOL fNull) {
    COMPATOBJECT *lpobj;

    if (!SDL_IsMainThread() || !FColor(crColor))
        return nullptr;
    lpobj = new (std::nothrow) COMPATOBJECT{};
    if (!lpobj)
        return nullptr;
    lpobj->iKind = iKind;
    lpobj->crColor = crColor;
    lpobj->fNull = fNull;
    lpobj->fAlive = 1;
    lpobj->lpNext = lpObjects;
    lpObjects = lpobj;
    return lpobj;
}

HPEN win32::CreatePen(int iStyle, int cWidth, COLORREF crColor) {
    COMPATOBJECT *lpobj;
    if (iStyle < win32::PS_SOLID || iStyle > win32::PS_NULL || cWidth < 0 || cWidth > 1) {
        SDL_SetError("GDI supports cosmetic solid/dashed/dotted or null pens, width 0 or 1");
        return nullptr;
    }
    lpobj = LpNewObject(objPen, crColor, iStyle == win32::PS_NULL);
    if (lpobj) lpobj->iPenStyle = iStyle;
    return reinterpret_cast<HPEN>(lpobj);
}

HBRUSH win32::CreateSolidBrush(COLORREF crColor) {
    return reinterpret_cast<HBRUSH>(LpNewObject(objBrush, crColor, 0));
}

HPALETTE win32::CreatePalette(const LOGPALETTE *lppal) {
    COMPATOBJECT *lpobj;
    PALETTEENTRY *rgpe;
    NATIVEARRAYOWNER<PALETTEENTRY, std::free> rgpeOwner;
    if (!SDL_IsMainThread() || !lppal || lppal->palVersion != 0x300 || lppal->palNumEntries == 0)
        return nullptr;
    rgpeOwner.reset(static_cast<PALETTEENTRY *>(std::malloc(static_cast<std::size_t>(lppal->palNumEntries) * sizeof(*rgpe)))); rgpe = rgpeOwner.get();
    if (!rgpe) return nullptr;
    std::memcpy(rgpe, lppal->palPalEntry, static_cast<std::size_t>(lppal->palNumEntries) * sizeof(*rgpe));
    lpobj = LpNewObject(objPalette, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->rgpe.reset(rgpeOwner.release()); lpobj->cEntries = lppal->palNumEntries;
    return reinterpret_cast<HPALETTE>(lpobj);
}

HPALETTE win32::SelectPalette(HDC hdc, HPALETTE hpal, BOOL fBackground) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject(static_cast<HGDIOBJ>(hpal)), *lpold;
    static_cast<void>(fBackground);
    if (!lpdc || !lpobj || lpobj->iKind != objPalette) return nullptr;
    lpold = lpdc->lppalette;
    lpold->cSelected--; lpobj->cSelected++;
    lpdc->lppalette = lpobj;
    return reinterpret_cast<HPALETTE>(lpold);
}

UINT win32::RealizePalette(HDC hdc) {
    // RGBA display and memory canvases have no physical palette to remap.
    return LpDC(hdc) ? 0 : win32::GDI_ERROR;
}

BOOL win32::UnrealizeObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj = LpObject(h);
    // True-color palettes need no remapping; brush origins belong to the DC.
    return lpobj && !lpobj->fStock && (lpobj->iKind == objPalette || lpobj->iKind == objBrush);
}

UINT win32::GetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, LPPALETTEENTRY rgpe) {
    COMPATOBJECT *lpobj = LpObject(static_cast<HGDIOBJ>(hpal));
    if (!lpobj || lpobj->iKind != objPalette) return 0;
    if (!rgpe) return lpobj->cEntries;
    if (iStart >= lpobj->cEntries) return 0;
    cEntries = StarsMin(cEntries, lpobj->cEntries - iStart);
    std::memcpy(rgpe, RgpeCompatPalette(lpobj) + iStart, static_cast<std::size_t>(cEntries) * sizeof(*rgpe));
    return cEntries;
}

HFONT win32::CreateFontIndirectA(const LOGFONTA *lplf) {
    COMPATOBJECT *lpobj;
    COMPATFONT *lpfont;
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpfontOwner;
    if (!SDL_IsMainThread() || !lplf) return nullptr;
    lpfontOwner.reset(LpNewCompatFont(lplf)); lpfont = lpfontOwner.get();
    if (!lpfont) return nullptr;
    lpobj = LpNewObject(objFont, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->lpFont.reset(lpfontOwner.release()); lpobj->lf = *lplf;
    return reinterpret_cast<HFONT>(lpobj);
}

BOOL win32::DeleteObject(HGDIOBJ h) {
    COMPATOBJECT *lpobj = LpObject(h);

    if (!lpobj || lpobj->fStock || lpobj->cSelected != 0)
        return 0;
    // Tombstones prevent stale GDI handles from aliasing newly allocated objects.
    lpobj->lpSurface.reset();
    lpobj->lpFont.reset();
    lpobj->rgpe.reset(); lpobj->cEntries = 0;
    lpobj->fAlive = 0;
    return 1;
}

HGDIOBJ win32::SelectObject(HDC hdc, HGDIOBJ h) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject(h), *lpold, **lplp;

    if (!lpdc || !lpobj)
        return nullptr;
    if (lpobj->iKind == objBitmap) {
        if (!lpdc->fMemory || lpdc->lpprint || (lpobj->fStock && lpobj != lpdc->lpbitmapDefault) ||
            (lpobj != lpdc->lpbitmap && lpobj->cSelected != 0))
            return nullptr;
        lplp = &lpdc->lpbitmap;
    } else if (lpobj->iKind == objPen)
        lplp = &lpdc->lppen;
    else if (lpobj->iKind == objBrush)
        lplp = &lpdc->lpbrush;
    else if (lpobj->iKind == objFont)
        lplp = &lpdc->lpfont;
    else
        return nullptr;
    lpold = *lplp;
    lpold->cSelected--;
    lpobj->cSelected++;
    *lplp = lpobj;
    return lpold;
}

int ICompatCanvasScale(HWND hwnd) {
    HWND lpRoot = HwndCompatRoot(hwnd);
    int cx, cy, cxPixel, cyPixel, iScale;
    const char *pszDriver = CompatInputIO().pfnGetCurrentVideoDriver();

    if (!lpRoot || !pszDriver || std::strcmp(pszDriver, "cocoa")) return 1;
    if (hwnd != lpRoot && lpRoot->lpCanvas.get()) return StarsMax(1, lpRoot->iCanvasScale);
    if (!SDL_GetWindowSize(lpRoot->lpWindow.get(), &cx, &cy) || cx <= 0 || cy <= 0 ||
        !SDL_GetWindowSizeInPixels(lpRoot->lpWindow.get(), &cxPixel, &cyPixel)) return 1;
    iScale = cxPixel / cx;
    return iScale >= 1 && iScale <= 4 && cxPixel == cx * iScale && cyPixel == cy * iScale ? iScale : 1;
}

static int IDCScale(HDC lpdc) {
    return lpdc->fMemory ? StarsMax(1, lpdc->iScale) : lpdc->hwnd ? StarsMax(1, lpdc->hwnd->iCanvasScale) : 1;
}

static void InvalidateCompatCanvasTree(HWND hwnd) {
    HWND lp;
    win32::InvalidateRect(hwnd, nullptr, 1);
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT))
        if (lp->fVisible) InvalidateCompatCanvasTree(lp);
}

BOOL FCompatSetDCScale(HDC hdc, int iScale) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc || !lpdc->fMemory || iScale < 1 || iScale > 4) return 0;
    lpdc->iScale = iScale;
    return 1;
}

static BOOL FCanvas(HWND hwnd) {
    SDL_Surface *lpnew;
    SDL_Texture *lptex;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    NATIVEOWNER<SDL_Texture, SDL_DestroyTexture> lptextureOwner;
    int dx, dy, cyMenu, iScale, iOld = StarsMax(1, hwnd->iCanvasScale);
    float cxTexture, cyTexture;
    RECT rc;
    const char *pszMode;

    if (!CompatGetSurfaceRect(hwnd, &rc)) return 0;
    dx = rc.right; dy = rc.bottom;
    iScale = ICompatCanvasScale(hwnd);
    if (dx > INT_MAX / iScale || dy > INT_MAX / iScale) return 0;
    dx *= iScale; dy *= iScale;
    cyMenu = CyCompatMenuBar(hwnd) * iScale;
    dx = StarsMax(1, dx); dy = StarsMax(1, dy);
    if (hwnd->lpCanvas.get() && hwnd->iCanvasScale == iScale && hwnd->lpCanvas->w == dx && hwnd->lpCanvas->h == dy &&
        (hwnd->hwndParent || (SDL_GetTextureSize(hwnd->lpTexture.get(), &cxTexture, &cyTexture) &&
         cxTexture == dx && cyTexture == dy + cyMenu)))
        return 1;
    if (!hwnd->hwndParent && !hwnd->lpRenderer.get()) {
        pszMode = SDL_getenv("STARS_SDL_RENDERER");
        if (pszMode && std::strcmp(pszMode, "auto") != 0 && std::strcmp(pszMode, "software") != 0 && std::strcmp(pszMode, "opengl") != 0) {
            SDL_SetError("GDI STARS_SDL_RENDERER must be auto, software or opengl");
            return 0;
        }
        hwnd->lpRenderer.reset(SDL_CreateRenderer(hwnd->lpWindow.get(),
                                             pszMode && std::strcmp(pszMode, "auto") != 0 ? pszMode : nullptr));
        if (!hwnd->lpRenderer.get() && (!pszMode || std::strcmp(pszMode, "auto") == 0))
            hwnd->lpRenderer.reset(SDL_CreateRenderer(hwnd->lpWindow.get(), "software"));
        if (!hwnd->lpRenderer.get())
            return 0;
    }
    lpsurfaceOwner.reset(SDL_CreateSurface(dx, dy, SDL_PIXELFORMAT_RGBA32)); lpnew = lpsurfaceOwner.get();
    lptextureOwner.reset(hwnd->hwndParent ? nullptr :
        SDL_CreateTexture(hwnd->lpRenderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, dx, dy + cyMenu));
    lptex = lptextureOwner.get();
    if (!lpnew || (!hwnd->hwndParent && !lptex)) {
        lpsurfaceOwner.reset();
        lptextureOwner.reset();
        return 0;
    }
    SDL_FillSurfaceRect(lpnew, nullptr, SDL_MapSurfaceRGB(lpnew, 0, 0, 0));
    if (hwnd->lpCanvas.get()) {
        SDL_Rect src, dst;
        src = SDL_Rect{0, 0, StarsMin(hwnd->lpCanvas->w / iOld, dx / iScale) * iOld,
                              StarsMin(hwnd->lpCanvas->h / iOld, dy / iScale) * iOld};
        dst = SDL_Rect{0, 0, src.w / iOld * iScale, src.h / iOld * iScale};
        SDL_BlitSurfaceScaled(hwnd->lpCanvas.get(), &src, lpnew, &dst, SDL_SCALEMODE_NEAREST);
    }
    SDL_SetSurfaceBlendMode(lpnew, SDL_BLENDMODE_NONE);
    if (lptex) {
        SDL_SetTextureBlendMode(lptex, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(lptex, SDL_SCALEMODE_NEAREST);
    }
    hwnd->lpTexture.reset();
    hwnd->lpCanvas.reset();
    hwnd->lpComposite.reset();
    hwnd->lpTexture.reset(lptextureOwner.release());
    hwnd->lpCanvas.reset(lpsurfaceOwner.release());
    hwnd->iCanvasScale = iScale;
    if (iOld != iScale) InvalidateCompatCanvasTree(hwnd);
    return 1;
}

BOOL FCompatCanvas(HWND hwnd) { return FCanvas(hwnd); }

static BOOL FComposeChildren(HWND hwnd, SDL_Surface *lpSurface, std::int64_t x, std::int64_t y, const RECT *lprcClip, int iScale) {
    HWND lp;
    RECT rc, client, clip;
    SDL_Rect src, dst;
    std::int64_t left, top, right, bottom;
    COLORREF cr;

    win32::GetClientRect(hwnd, &client);
    if (!hwnd->lpcombo && (hwnd->dwStyle & (win32::WS_HSCROLL | win32::WS_VSCROLL)) == (win32::WS_HSCROLL | win32::WS_VSCROLL)) {
        CompatGetSurfaceRect(hwnd, &rc);
        left = (((static_cast<std::int64_t>(lprcClip->left)) > (x + client.right)) ? (static_cast<std::int64_t>(lprcClip->left)) : (x + client.right));
        top = (((static_cast<std::int64_t>(lprcClip->top)) > (y + client.bottom)) ? (static_cast<std::int64_t>(lprcClip->top)) : (y + client.bottom));
        right = (((static_cast<std::int64_t>(lprcClip->right)) < (x + rc.right)) ? (static_cast<std::int64_t>(lprcClip->right)) : (x + rc.right));
        bottom = (((static_cast<std::int64_t>(lprcClip->bottom)) < (y + rc.bottom)) ? (static_cast<std::int64_t>(lprcClip->bottom)) : (y + rc.bottom));
        if (right > left && bottom > top) {
            dst = SDL_Rect{static_cast<int>(left) * iScale, static_cast<int>(top) * iScale,
                             static_cast<int>((right - left)) * iScale, static_cast<int>((bottom - top)) * iScale};
            cr = win32::GetSysColor(win32::COLOR_BTNFACE);
            SDL_FillSurfaceRect(lpSurface, &dst, SDL_MapSurfaceRGB(lpSurface, cr & 255, (cr >> 8) & 255, (cr >> 16) & 255));
        }
    }
    // Nonclient scrollbar children occupy the reserved strips; ordinary children
    // must not paint into those strips even when they extend beyond the client.
    // Draw bottom to top; descendants are clipped by every ancestor.
    lp = win32::GetWindow(hwnd, win32::GW_CHILD);
    if (lp) lp = win32::GetWindow(lp, win32::GW_HWNDLAST);
    for (; lp; lp = win32::GetWindow(lp, win32::GW_HWNDPREV)) {
        if (!lp->fVisible || lp->fDestroying) continue;
        clip = *lprcClip;
        if (!lp->fNcScroll) {
            clip.right = static_cast<LONG>((((static_cast<std::int64_t>(clip.right)) < (x + client.right)) ? (static_cast<std::int64_t>(clip.right)) : (x + client.right)));
            clip.bottom = static_cast<LONG>((((static_cast<std::int64_t>(clip.bottom)) < (y + client.bottom)) ? (static_cast<std::int64_t>(clip.bottom)) : (y + client.bottom)));
        }
        left = x + lp->rcChild.left; top = y + lp->rcChild.top;
        right = x + lp->rcChild.right; bottom = y + lp->rcChild.bottom;
        if (left >= clip.right || top >= clip.bottom ||
            right <= clip.left || bottom <= clip.top) continue;
        rc.left = static_cast<LONG>((left > clip.left ? left : clip.left));
        rc.top = static_cast<LONG>((top > clip.top ? top : clip.top));
        rc.right = static_cast<LONG>((right < clip.right ? right : clip.right));
        rc.bottom = static_cast<LONG>((bottom < clip.bottom ? bottom : clip.bottom));
        if (!FCanvas(lp)) return 0;
        dst = SDL_Rect{rc.left * iScale, rc.top * iScale, (rc.right - rc.left) * iScale, (rc.bottom - rc.top) * iScale};
        src = SDL_Rect{static_cast<int>((rc.left - left)) * iScale, static_cast<int>((rc.top - top)) * iScale, dst.w, dst.h};
        if (!SDL_BlitSurface(lp->lpCanvas.get(), &src, lpSurface, &dst) ||
            !FComposeChildren(lp, lpSurface, left, top, &rc, iScale)) return 0;
    }
    return 1;
}

static BOOL FPendingPaintTree(HWND hwnd) {
    HWND lp;

    if (hwnd->hdcPaint || hwnd->fUpdate) return 1;
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT))
        if (lp->fVisible && FPendingPaintTree(lp)) return 1;
    return 0;
}

BOOL FCompatPaintTree(HWND hwnd) {
    HWND lp, lpNext;

    if (FStarsErrorPending() || !win32::UpdateWindow(hwnd) || !win32::IsWindow(hwnd) || FStarsErrorPending()) return 0;
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = lpNext) {
        lpNext = win32::GetWindow(lp, win32::GW_HWNDNEXT);
        if (lp->fVisible && !FCompatPaintTree(lp)) return 0;
        if (!win32::IsWindow(hwnd)) return 0;
    }
    return 1;
}

BOOL FCompatRefreshPresent(HWND hwnd) {
    HWND lpwin = HwndCompatRoot(hwnd);
    static BOOL fPresenting;
    BOOL fOk;

    if (!lpwin || lpwin->fDestroying || !lpwin->lpRenderer.get() || !lpwin->lpTexture.get() ||
        !lpwin->lpComposite.get() || fPresenting) return 0;
    // Cocoa can clear its view during exposure after the initial show. Reuse
    // the last completed texture even if a new paint is pending; never reenter
    // application drawing from a native exposure callback.
    StarsRestore<BOOL> restorePresenting(fPresenting);
    fPresenting = 1;
    fOk = SDL_SetRenderDrawColor(lpwin->lpRenderer.get(), 0, 0, 0,
                                SDL_GetWindowFlags(lpwin->lpWindow.get()) & SDL_WINDOW_TRANSPARENT ? 0 : 255) &&
          SDL_RenderClear(lpwin->lpRenderer.get()) &&
          SDL_RenderTexture(lpwin->lpRenderer.get(), lpwin->lpTexture.get(), nullptr, nullptr) &&
          CompatRenderIO().pfnRenderPresent(lpwin->lpRenderer.get());
    return fOk;
}

BOOL FCompatPresent(HWND hwnd) {
    HWND lpwin = HwndCompatRoot(hwnd);
    RECT rc;
    SDL_Surface *lpnew;
    SDL_Rect dst;
    int cyMenu, iScale;
    BOOL fOk;

    if (!lpwin || lpwin->fDestroying) return 0;
    lpwin->fPresentPending = 1;
    // Present the completed window tree, not intermediate icon/DC or pane paints.
    if (lpwin->fDeferPresent || lpwin->cPresentDefer != 0 || FPendingPaintTree(lpwin)) return 1;
    if (!FCanvas(lpwin)) return 0;
    iScale = lpwin->iCanvasScale;
    cyMenu = CyCompatMenuBar(lpwin);
    if (!lpwin->lpComposite.get() || lpwin->lpComposite->w != lpwin->lpCanvas->w ||
        lpwin->lpComposite->h != lpwin->lpCanvas->h + cyMenu * iScale) {
        lpnew = SDL_CreateSurface(lpwin->lpCanvas->w, lpwin->lpCanvas->h + cyMenu * iScale, SDL_PIXELFORMAT_RGBA32);
        if (!lpnew) return 0;
        SDL_SetSurfaceBlendMode(lpnew, SDL_BLENDMODE_NONE);
        lpwin->lpComposite.reset(lpnew);
    }
    dst = SDL_Rect{0, cyMenu * iScale, lpwin->lpCanvas->w, lpwin->lpCanvas->h};
    if (!SDL_BlitSurface(lpwin->lpCanvas.get(), nullptr, lpwin->lpComposite.get(), &dst)) return 0;
    CompatGetSurfaceRect(lpwin, &rc);
    win32::OffsetRect(&rc, 0, cyMenu);
    if (!FComposeChildren(lpwin, lpwin->lpComposite.get(), 0, cyMenu, &rc, iScale) ||
        !FCompatDrawMenuBar(lpwin, lpwin->lpComposite.get())) return 0;
    CompatDrawCursor(lpwin, lpwin->lpComposite.get());
    if (FPendingPaintTree(lpwin)) return 1;
    fOk = SDL_UpdateTexture(lpwin->lpTexture.get(), nullptr, lpwin->lpComposite->pixels, lpwin->lpComposite->pitch) &&
          FCompatRefreshPresent(lpwin);
    if (fOk) lpwin->fPresentPending = 0;
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
        return nullptr;
    lpdc = new (std::nothrow) win32::HDC__{};
    if (!lpdc)
        return nullptr;
    // Never dereference public HDC tokens. Monotonic IDs permit immediate release
    // of frequent paint DC allocations without making stale handles valid again.
    lpdc->hdcToken = reinterpret_cast<HDC>((idDCNext++ * 16));
    lpdc->hwnd = hwnd;
    if (hwnd)
        win32::GetClientRect(hwnd, &lpdc->rcClip);
    lpdc->lppen = static_cast<COMPATOBJECT *>((win32::GetStockObject(win32::BLACK_PEN)));
    lpdc->lpbrush = static_cast<COMPATOBJECT *>((win32::GetStockObject(win32::WHITE_BRUSH)));
    lpdc->lpfont = static_cast<COMPATOBJECT *>((win32::GetStockObject(win32::SYSTEM_FONT)));
    lpdc->lppalette = static_cast<COMPATOBJECT *>((win32::GetStockObject(win32::DEFAULT_PALETTE)));
    if (!lpdc->lpfont) { delete lpdc; return nullptr; }
    lpdc->lppen->cSelected++;
    lpdc->lpbrush->cSelected++;
    lpdc->lpfont->cSelected++;
    lpdc->lppalette->cSelected++;
    lpdc->crBk = win32::RGB(255, 255, 255);
    lpdc->iBkMode = win32::OPAQUE;
    lpdc->iRop2 = win32::R2_COPYPEN;
    lpdc->fPaint = lpdc->fClip = fPaint;
    lpdc->fAlive = 1;
    lpdc->lpNext = lpDCs;
    lpDCs = lpdc;
    return lpdc->hdcToken;
}

static COMPATOBJECT *LpNewBitmap(int cx, int cy, BOOL fMono) {
    COMPATOBJECT *lpobj;
    SDL_Surface *lpsurface;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;

    if (cx <= 0 || cy <= 0 || cx > INT_MAX / 4)
        return nullptr;
    lpsurfaceOwner.reset(SDL_CreateSurface(cx, cy, SDL_PIXELFORMAT_RGBA32)); lpsurface = lpsurfaceOwner.get();
    if (!lpsurface)
        return nullptr;
    lpobj = LpNewObject(objBitmap, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->lpSurface.reset(lpsurfaceOwner.release());
    lpobj->fMono = fMono;
    SDL_FillSurfaceRect(lpsurface, nullptr, SDL_MapSurfaceRGB(lpsurface, 0, 0, 0));
    return lpobj;
}

HBITMAP win32::CreateCompatibleBitmap(HDC hdc, int cx, int cy) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc || cx < 0 || cy < 0)
        return nullptr;
    if (cx == 0 || cy == 0)
        return reinterpret_cast<HBITMAP>(LpNewBitmap(1, 1, 1));
    return reinterpret_cast<HBITMAP>(LpNewBitmap(cx, cy, lpdc->fMemory && lpdc->lpbitmap->fMono));
}

HDC win32::CreateCompatibleDC(HDC hdc) {
    HDC hdcNew, lpdc;
    COMPATOBJECT *lpbitmap;

    if (hdc && !LpDC(hdc))
        return nullptr;
    // A memory DC starts with defaults, not a copy of its reference DC state.
    lpbitmap = LpNewBitmap(1, 1, 1);
    if (!lpbitmap)
        return nullptr;
    hdcNew = HdcNew(nullptr, 0);
    if (!hdcNew) {
        win32::DeleteObject(lpbitmap);
        return nullptr;
    }
    lpdc = lpDCs;
    lpdc->fMemory = 1;
    lpdc->lpbitmap = lpdc->lpbitmapDefault = lpbitmap;
    lpbitmap->fStock = 1;
    lpbitmap->cSelected = 1;
    return hdcNew;
}

int win32::GetObjectA(HANDLE h, int cb, LPVOID lpv) {
    COMPATOBJECT *lpobj = LpObject(h);
    BITMAP bm = {};

    if (lpobj && lpobj->iKind == objPalette) {
        if (!lpv) return sizeof(WORD);
        if (cb < static_cast<int>(sizeof(WORD))) return 0;
        *static_cast<WORD *>(lpv) = static_cast<WORD>(lpobj->cEntries);
        return sizeof(WORD);
    }
    if (lpobj && lpobj->iKind == objFont) {
        if (!lpv) return sizeof(LOGFONTA);
        if (cb < static_cast<int>(sizeof(LOGFONTA))) return 0;
        std::memcpy(lpv, &lpobj->lf, sizeof(lpobj->lf));
        return sizeof(LOGFONTA);
    }
    if (!lpobj || lpobj->iKind != objBitmap)
        return 0;
    if (!lpv)
        return sizeof(bm);
    if (cb < static_cast<int>(sizeof(bm)))
        return 0;
    bm.bmWidth = lpobj->lpSurface.get()->w;
    bm.bmHeight = lpobj->lpSurface.get()->h;
    bm.bmPlanes = 1;
    bm.bmBitsPixel = lpobj->fMono ? 1 : 32;
    bm.bmWidthBytes = lpobj->fMono ? ((bm.bmWidth + 15) / 16) * 2 : bm.bmWidth * 4;
    std::memcpy(lpv, &bm, sizeof(bm));
    return sizeof(bm);
}

static void CloseDC(HDC lpdc) {
    HDC *lplp = &lpDCs;

    lpdc->lpprint.reset();
    lpdc->lppen->cSelected--;
    lpdc->lpbrush->cSelected--;
    lpdc->lpfont->cSelected--;
    lpdc->lppalette->cSelected--;
    if (lpdc->lpbitmap)
        lpdc->lpbitmap->cSelected--;
    if (lpdc->lpbitmapDefault) {
        lpdc->lpbitmapDefault->lpSurface.reset();
        lpdc->lpbitmapDefault->fAlive = 0;
    }
    while (*lplp != lpdc)
        lplp = &(*lplp)->lpNext;
    *lplp = lpdc->lpNext;
    delete lpdc;
}

BOOL win32::DeleteDC(HDC hdc) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc || !lpdc->fMemory)
        return 0;
    if (lpdc->lpprint && FCompatPrinterBusy(lpdc->lpprint.get())) { win32::SetLastError(win32::ERROR_BUSY); return 0; }
    CloseDC(lpdc);
    return 1;
}

HDC HdcCompatPrinter(COMPATPRINT *lpprint, int cx, int cy) {
    COMPATOBJECT *lpbitmap = LpNewBitmap(cx, cy, 0);
    HDC hdc, lpdc;
    if (!lpbitmap) return nullptr;
    hdc = HdcNew(nullptr, 0);
    if (!hdc) { win32::DeleteObject(lpbitmap); return nullptr; }
    lpdc = lpDCs;
    lpdc->lpprint.reset(lpprint);
    lpdc->fMemory = 1;
    lpdc->lpbitmap = lpdc->lpbitmapDefault = lpbitmap;
    lpbitmap->fStock = 1; lpbitmap->cSelected = 1;
    SDL_FillSurfaceRect(lpbitmap->lpSurface.get(), nullptr, SDL_MapSurfaceRGB(lpbitmap->lpSurface.get(), 255, 255, 255));
    return hdc;
}
COMPATPRINT *LpCompatPrinter(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->lpprint.get() : nullptr;
}
SDL_Surface *LpCompatPrinterSurface(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc && lpdc->lpprint ? lpdc->lpbitmap->lpSurface.get() : nullptr;
}

HDC win32::GetDC(HWND hwnd) {
    HDC hdc = HdcNew(hwnd, 0);
    if (hdc && !hwnd) lpDCs->fScreen = 1;
    return hdc;
}

int win32::ReleaseDC(HWND hwnd, HDC hdc) {
    HDC lpdc = LpDC(hdc);
    BOOL fOk;

    if (!lpdc || lpdc->hwnd != hwnd || lpdc->fPaint || lpdc->fMemory)
        return 0;
    fOk = lpdc->fScreen ? 1 : FCompatPresent(hwnd);
    CloseDC(lpdc);
    return fOk;
}

static BOOL FAddUpdate(RECT **lprgrc, std::size_t *lpcrc, RECT rc) {
    RECT *rgrc;
    std::size_t i;
    if (rc.left >= rc.right || rc.top >= rc.bottom) return 1;
    for (i = 0; i < *lpcrc; i++)
        if (rc.left >= (*lprgrc)[i].left && rc.top >= (*lprgrc)[i].top &&
            rc.right <= (*lprgrc)[i].right && rc.bottom <= (*lprgrc)[i].bottom) return 1;
    if (*lpcrc >= SIZE_MAX / sizeof(*rgrc) - 1) return 0;
    rgrc = static_cast<RECT *>((std::realloc(*lprgrc, (*lpcrc + 1) * sizeof(*rgrc))));
    if (!rgrc) return 0;
    rgrc[(*lpcrc)++] = rc; *lprgrc = rgrc;
    return 1;
}

static BOOL FSubtractUpdate(RECT **lprgrc, std::size_t *lpcrc, RECT rc, const RECT *lprcCut) {
    RECT hit;
    if (!win32::IntersectRect(&hit, &rc, lprcCut)) return FAddUpdate(lprgrc, lpcrc, rc);
    return FAddUpdate(lprgrc, lpcrc, RECT{rc.left, rc.top, rc.right, hit.top}) &&
           FAddUpdate(lprgrc, lpcrc, RECT{rc.left, hit.bottom, rc.right, rc.bottom}) &&
           FAddUpdate(lprgrc, lpcrc, RECT{rc.left, hit.top, hit.left, hit.bottom}) &&
           FAddUpdate(lprgrc, lpcrc, RECT{hit.right, hit.top, rc.right, hit.bottom});
}

static void SetUpdate(HWND hwnd, RECT *rgrc, std::size_t crc) {
    std::size_t i;
    hwnd->rgrcUpdate.reset(rgrc); hwnd->crcUpdate = crc;
    hwnd->rcUpdate = RECT{}; hwnd->fUpdate = crc != 0;
    for (i = 0; i < crc; i++) {
        if (i == 0) hwnd->rcUpdate = rgrc[i];
        else {
            hwnd->rcUpdate.left = StarsMin(hwnd->rcUpdate.left, rgrc[i].left);
            hwnd->rcUpdate.top = StarsMin(hwnd->rcUpdate.top, rgrc[i].top);
            hwnd->rcUpdate.right = StarsMax(hwnd->rcUpdate.right, rgrc[i].right);
            hwnd->rcUpdate.bottom = StarsMax(hwnd->rcUpdate.bottom, rgrc[i].bottom);
        }
    }
    if (crc == 0) hwnd->fErase = 0;
}

BOOL win32::InvalidateRect(HWND hwnd, const RECT *lprc, BOOL fErase) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT rc, rcClient;
    RECT *rgrc;
    std::size_t crc;
    if (!lpwin || !win32::GetClientRect(hwnd, &rcClient)) return 0;
    if (!win32::IntersectRect(&rc, lprc ? lprc : &rcClient, &rcClient)) return 1;
    rgrc = lpwin->rgrcUpdate.get(); crc = lpwin->crcUpdate;
    if (!FAddUpdate(&rgrc, &crc, rc)) return 0;
    // FAddUpdate may have relocated the allocation; retain ownership before SetUpdate.
    static_cast<void>(lpwin->rgrcUpdate.release());
    SetUpdate(lpwin, rgrc, crc); lpwin->fErase |= fErase;
    return 1;
}

static BOOL FPaintClip(HDC lpdc, HWND hwnd) {
    RECT *rgrc = nullptr;
    NATIVEARRAYOWNER<RECT, std::free> rgrcOwner;
    if (hwnd->crcUpdate != 0) {
        rgrc = static_cast<RECT *>((std::malloc(hwnd->crcUpdate * sizeof(*rgrc))));
        rgrcOwner.reset(rgrc);
        if (!rgrc) return 0;
        std::memcpy(rgrc, hwnd->rgrcUpdate.get(), hwnd->crcUpdate * sizeof(*rgrc));
    }
    lpdc->rgrcPaint = std::move(rgrcOwner); lpdc->crcPaint = hwnd->crcUpdate;
    lpdc->fPaint = 1; lpdc->fClip = 1; lpdc->rcClip = hwnd->rcUpdate;
    return 1;
}

BOOL win32::GetUpdateRect(HWND hwnd, LPRECT lprc, BOOL fErase) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC hdc;

    if (!lpwin)
        return 0;
    if (lprc)
        *lprc = lpwin->fUpdate ? lpwin->rcUpdate : RECT{};
    if (fErase && lpwin->fUpdate && lpwin->fErase) {
        hdc = win32::GetDC(hwnd);
        if (hdc) {
            if (!FPaintClip(LpDC(hdc), lpwin)) { win32::ReleaseDC(hwnd, hdc); return 0; }
            if (win32::SendMessageA(hwnd, win32::WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0) && LpCompatWindow(hwnd))
                lpwin->fErase = 0;
            if (LpDC(hdc)) LpDC(hdc)->fPaint = 0;
            win32::ReleaseDC(hwnd, hdc);
        }
    }
    return lpwin->fAlive && lpwin->fUpdate;
}

HDC win32::BeginPaint(HWND hwnd, LPPAINTSTRUCT lpps) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC hdc;
    BOOL fErase;

    if (!lpwin || !lpps || lpwin->hdcPaint)
        return nullptr;
    hdc = HdcNew(hwnd, 1);
    if (!hdc)
        return nullptr;
    *lpps = {};
    lpps->hdc = hdc;
    lpps->rcPaint = lpwin->fUpdate ? lpwin->rcUpdate : RECT{};
    LpDC(hdc)->rcClip = lpps->rcPaint;
    LpDC(hdc)->rgrcPaint.reset(lpwin->rgrcUpdate.release()); LpDC(hdc)->crcPaint = lpwin->crcUpdate;
    lpwin->crcUpdate = 0;
    fErase = lpwin->fErase;
    lpwin->fUpdate = 0;
    lpwin->fErase = 0;
    lpwin->hdcPaint = hdc;
    if (fErase)
        lpps->fErase = !win32::SendMessageA(hwnd, win32::WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0);
    return LpDC(hdc) ? hdc : nullptr;
}

BOOL win32::EndPaint(HWND hwnd, const PAINTSTRUCT *lpps) {
    HWND lpwin = LpCompatWindow(hwnd);
    HDC lpdc = lpps ? LpDC(lpps->hdc) : nullptr;
    BOOL fOk;

    if (!lpwin || !lpdc || !lpdc->fPaint || lpdc->hwnd != hwnd || lpwin->hdcPaint != lpps->hdc)
        return 0;
    lpwin->hdcPaint = nullptr;
    CloseDC(lpdc);
    fOk = FCompatPresent(hwnd);
    return fOk;
}

BOOL win32::UpdateWindow(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin)
        return 0;
    if (lpwin->fUpdate && !lpwin->hdcPaint)
        win32::SendMessageA(hwnd, win32::WM_PAINT, 0, 0);
    return 1;
}

BOOL win32::ValidateRect(HWND hwnd, const RECT *lprc) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT *rgrc = nullptr;
    std::size_t crc = 0, i;
    NATIVECLEANUPSCOPE updateOwner([&]() noexcept { std::free(rgrc); });
    if (!lpwin) return 0;
    if (lprc) for (i = 0; i < lpwin->crcUpdate; i++)
        if (!FSubtractUpdate(&rgrc, &crc, lpwin->rgrcUpdate[i], lprc)) return 0;
    SetUpdate(lpwin, rgrc, crc); rgrc = nullptr;
    return 1;
}

static SDL_Surface *LpDCRasterSurface(HDC lpdc) {
    if (lpdc->fScreen) return nullptr;
    if (lpdc->fMemory)
        return lpdc->lpbitmap->lpSurface.get();
    return lpdc->hwnd->lpCanvas.get();
}

static SDL_Surface *LpDCSurface(HDC lpdc) {
    return lpdc->fScreen || lpdc->fMemory || FCanvas(lpdc->hwnd) ? LpDCRasterSurface(lpdc) : nullptr;
}

static BOOL FClip(HDC lpdc, RECT *lprc) {
    RECT rcClient;
    SDL_Surface *lpsurface = LpDCSurface(lpdc);

    if (!lpsurface)
        return 0;
    rcClient = RECT{0, 0, lpsurface->w / IDCScale(lpdc), lpsurface->h / IDCScale(lpdc)};
    if (lpdc->hwnd && !win32::GetClientRect(lpdc->hwnd, &rcClient)) return 0;
    if (lpdc->fClip)
        win32::IntersectRect(lprc, &lpdc->rcClip, &rcClient);
    else
        *lprc = rcClient;
    return 1;
}

/* Keep the rasterizer in device coordinates. Promote translations before
 * subtraction, and clip rectangles before narrowing, so extreme origins cannot
 * overflow a signed Win32 coordinate or address a pixel outside its surface. */
static LONG LCoord(std::int64_t l) {
    return static_cast<LONG>((((static_cast<std::int64_t>(INT_MIN)) > ((((static_cast<std::int64_t>(INT_MAX)) < (l)) ? (static_cast<std::int64_t>(INT_MAX)) : (l)))) ? (static_cast<std::int64_t>(INT_MIN)) : ((((static_cast<std::int64_t>(INT_MAX)) < (l)) ? (static_cast<std::int64_t>(INT_MAX)) : (l)))));
}

static RECT RcDevice(HDC lpdc, RECT rc) {
    return RECT{LCoord(static_cast<std::int64_t>(rc.left) - lpdc->ptOrg.x), LCoord(static_cast<std::int64_t>(rc.top) - lpdc->ptOrg.y),
                  LCoord(static_cast<std::int64_t>(rc.right) - lpdc->ptOrg.x), LCoord(static_cast<std::int64_t>(rc.bottom) - lpdc->ptOrg.y)};
}

BOOL win32::SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc) return 0;
    if (lppt) *lppt = lpdc->ptOrg;
    lpdc->ptOrg = POINT{x, y};
    return 1;
}

BOOL win32::SetBrushOrgEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc) return 0;
    if (lppt) *lppt = lpdc->ptBrush;
    lpdc->ptBrush = POINT{x, y};
    return 1;
}

int win32::GetROP2(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->iRop2 : 0;
}

int win32::SetROP2(HDC hdc, int iRop) {
    HDC lpdc = LpDC(hdc);
    int iOld;
    if (!lpdc || iRop < 1 || iRop > 16) return 0;
    iOld = lpdc->iRop2; lpdc->iRop2 = iRop;
    return iOld;
}

/* Application clipping is independent of BeginPaint's visible/update clip.
 * Rectangles remain disjoint, so exclusions preserve holes without a mask the
 * size of the display. A selected region is copied, never retained by handle. */
static int IRegionKind(std::size_t crc) {
    return crc == 0 ? win32::NULLREGION : crc == 1 ? win32::SIMPLEREGION : win32::COMPLEXREGION;
}

HRGN win32::CreateRectRgn(int left, int top, int right, int bottom) {
    COMPATOBJECT *lpobj;
    // GDI regions use signed 27-bit coordinates, even on a 64-bit host.
    if (left < -67108864 || top < -67108864 || right < -67108864 || bottom < -67108864 ||
        left > 67108863 || top > 67108863 || right > 67108863 || bottom > 67108863)
        return nullptr;
    lpobj = LpNewObject(objRegion, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->rcRegion = RECT{StarsMin(left, right), StarsMin(top, bottom), StarsMax(left, right), StarsMax(top, bottom)};
    return reinterpret_cast<HRGN>(lpobj);
}

int win32::SelectClipRgn(HDC hdc, HRGN hrgn) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = hrgn ? LpObject(static_cast<HGDIOBJ>(hrgn)) : nullptr;
    RECT *rgrc = nullptr;
    std::size_t crc = 0;
    NATIVEARRAYOWNER<RECT, std::free> rgrcOwner;

    if (!lpdc || (hrgn && (!lpobj || lpobj->iKind != objRegion))) return win32::ERROR;
    if (lpobj && lpobj->rcRegion.left < lpobj->rcRegion.right && lpobj->rcRegion.top < lpobj->rcRegion.bottom) {
        rgrc = static_cast<RECT *>((std::malloc(sizeof(*rgrc))));
        rgrcOwner.reset(rgrc);
        if (!rgrc) return win32::ERROR;
        *rgrc = lpobj->rcRegion; crc = 1;
    }
    lpdc->rgrcRegion = std::move(rgrcOwner); lpdc->crcRegion = crc;
    lpdc->fRegion = hrgn != nullptr;
    return hrgn ? IRegionKind(crc) : win32::SIMPLEREGION;
}

static void MergeRegion(RECT *rgrc, std::size_t *lpcrc) {
    std::size_t i, j;
    RECT *a, *b;
    BOOL fAgain = 1;

    while (fAgain) {
        fAgain = 0;
        for (i = 0; i < *lpcrc && !fAgain; i++) for (j = i + 1; j < *lpcrc; j++) {
            a = &rgrc[i]; b = &rgrc[j];
            if (a->left == b->left && a->right == b->right && (a->bottom == b->top || b->bottom == a->top)) {
                a->top = StarsMin(a->top, b->top); a->bottom = StarsMax(a->bottom, b->bottom);
            } else if (a->top == b->top && a->bottom == b->bottom && (a->right == b->left || b->right == a->left)) {
                a->left = StarsMin(a->left, b->left); a->right = StarsMax(a->right, b->right);
            } else continue;
            rgrc[j] = rgrc[--*lpcrc]; fAgain = 1; break;
        }
    }
}

static int IClipRect(HDC hdc, int left, int top, int right, int bottom, BOOL fExclude) {
    HDC lpdc = LpDC(hdc);
    RECT rcCut = {StarsMin(left, right), StarsMin(top, bottom), StarsMax(left, right), StarsMax(top, bottom)};
    RECT rcBase, rc, rcHit, *rgrc, *rgsrc;
    std::size_t i, crc = 0, crcSrc;

    if (!lpdc) return win32::ERROR;
    rcCut = RcDevice(lpdc, rcCut);
    if (!lpdc->fRegion) {
        if (fExclude) {
            if (!FClip(lpdc, &rcBase)) return win32::ERROR;
        } else rcBase = rcCut;
        rgsrc = &rcBase; crcSrc = 1;
    } else { rgsrc = lpdc->rgrcRegion.get(); crcSrc = lpdc->crcRegion; }
    if (crcSrc > SIZE_MAX / (4 * sizeof(*rgrc))) return win32::ERROR;
    rgrc = static_cast<RECT *>((crcSrc ? std::malloc(crcSrc * 4 * sizeof(*rgrc)) : nullptr));
    NATIVEARRAYOWNER<RECT, std::free> rgrcOwner(rgrc);
    if (crcSrc != 0 && !rgrc) return win32::ERROR;
    for (i = 0; i < crcSrc; i++) {
        rc = rgsrc[i];
        if (rc.left >= rc.right || rc.top >= rc.bottom) continue;
        if (!win32::IntersectRect(&rcHit, &rc, &rcCut)) {
            if (fExclude) rgrc[crc++] = rc;
        } else if (!fExclude) rgrc[crc++] = rcHit;
        else {
            if (rc.top < rcHit.top) rgrc[crc++] = RECT{rc.left, rc.top, rc.right, rcHit.top};
            if (rcHit.bottom < rc.bottom) rgrc[crc++] = RECT{rc.left, rcHit.bottom, rc.right, rc.bottom};
            if (rc.left < rcHit.left) rgrc[crc++] = RECT{rc.left, rcHit.top, rcHit.left, rcHit.bottom};
            if (rcHit.right < rc.right) rgrc[crc++] = RECT{rcHit.right, rcHit.top, rc.right, rcHit.bottom};
        }
    }
    MergeRegion(rgrc, &crc);
    lpdc->rgrcRegion = std::move(rgrcOwner); lpdc->crcRegion = crc; lpdc->fRegion = 1;
    return IRegionKind(crc);
}

int win32::IntersectClipRect(HDC hdc, int left, int top, int right, int bottom) {
    return IClipRect(hdc, left, top, right, bottom, 0);
}

int win32::ExcludeClipRect(HDC hdc, int left, int top, int right, int bottom) {
    return IClipRect(hdc, left, top, right, bottom, 1);
}

static BOOL FRectPixel(const RECT *rgrc, std::size_t crc, std::int64_t x, std::int64_t y) {
    std::size_t i;
    for (i = 0; i < crc; i++)
        if (x >= rgrc[i].left && x < rgrc[i].right && y >= rgrc[i].top && y < rgrc[i].bottom) return 1;
    return 0;
}

static BOOL FRegionPixel(HDC lpdc, std::int64_t x, std::int64_t y) {
    return (!lpdc->fPaint || FRectPixel(lpdc->rgrcPaint.get(), lpdc->crcPaint, x, y)) &&
           (!lpdc->fRegion || FRectPixel(lpdc->rgrcRegion.get(), lpdc->crcRegion, x, y));
}

static void DevicePixel(HDC lpdc, std::int64_t x, std::int64_t y, COLORREF crColor) {
    BYTE *rgb;

    crColor = CrPalette(lpdc, crColor);
    rgb = static_cast<BYTE *>(LpDCRasterSurface(lpdc)->pixels) + static_cast<std::size_t>(y) * LpDCRasterSurface(lpdc)->pitch + static_cast<std::size_t>(x) * 4;
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        crColor = (static_cast<BYTE>(crColor) + static_cast<BYTE>((crColor >> 8)) + static_cast<BYTE>((crColor >> 16)) >= 384) ?
                  win32::RGB(255, 255, 255) : win32::RGB(0, 0, 0);
    rgb[0] = static_cast<BYTE>(crColor);
    rgb[1] = static_cast<BYTE>((crColor >> 8));
    rgb[2] = static_cast<BYTE>((crColor >> 16));
    rgb[3] = 255;
}

static void Pixel(HDC lpdc, const RECT *lprc, std::int64_t x, std::int64_t y, COLORREF crColor) {
    int iScale = IDCScale(lpdc), ix, iy;
    if (x < lprc->left || x >= lprc->right || y < lprc->top || y >= lprc->bottom || !FRegionPixel(lpdc, x, y)) return;
    for (iy = 0; iy < iScale; iy++) for (ix = 0; ix < iScale; ix++)
        DevicePixel(lpdc, x * iScale + ix, y * iScale + iy, crColor);
}

COLORREF win32::GetPixel(HDC hdc, int x, int y) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    BYTE *rgb;
    std::int64_t px, py;

    if (!lpdc || !FClip(lpdc, &rc)) return win32::CLR_INVALID;
    px = static_cast<std::int64_t>(x) - lpdc->ptOrg.x; py = static_cast<std::int64_t>(y) - lpdc->ptOrg.y;
    if (px < rc.left || px >= rc.right || py < rc.top || py >= rc.bottom || !FRegionPixel(lpdc, px, py))
        return win32::CLR_INVALID;
    rgb = static_cast<BYTE *>(LpDCSurface(lpdc)->pixels) + static_cast<std::size_t>((py * IDCScale(lpdc))) * LpDCSurface(lpdc)->pitch +
          static_cast<std::size_t>((px * IDCScale(lpdc))) * 4;
    return win32::RGB(rgb[0], rgb[1], rgb[2]);
}

static void ShapePixel(HDC lpdc, const RECT *lprc, std::int64_t x, std::int64_t y, COLORREF crPen);

COLORREF win32::SetPixel(HDC hdc, int x, int y, COLORREF cr) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    std::int64_t px, py;

    if (!lpdc || !FColor(cr) || !FClip(lpdc, &rc)) return win32::CLR_INVALID;
    px = static_cast<std::int64_t>(x) - lpdc->ptOrg.x; py = static_cast<std::int64_t>(y) - lpdc->ptOrg.y;
    if (px < rc.left || px >= rc.right || py < rc.top || py >= rc.bottom || !FRegionPixel(lpdc, px, py))
        return win32::CLR_INVALID;
    cr = CrPalette(lpdc, cr);
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        cr = (static_cast<BYTE>(cr) + static_cast<BYTE>((cr >> 8)) + static_cast<BYTE>((cr >> 16)) >= 384) ? win32::RGB(255, 255, 255) : 0;
    ShapePixel(lpdc, &rc, px, py, cr);
    return cr;
}

static COLORREF CrBrush(HDC lpdc, COMPATOBJECT *lpbrush, std::int64_t x, std::int64_t y) {
    SDL_Surface *lpsurface = lpbrush->lpSurface.get();
    BYTE *rgb;
    if (!lpsurface) return CrPalette(lpdc, lpbrush->crColor);
    x = (x - lpdc->ptBrush.x) % lpsurface->w; y = (y - lpdc->ptBrush.y) % lpsurface->h;
    if (x < 0) x += lpsurface->w;
    if (y < 0) y += lpsurface->h;
    rgb = static_cast<BYTE *>(lpsurface->pixels) + static_cast<std::size_t>(y) * lpsurface->pitch + static_cast<std::size_t>(x) * 4;
    // A monochrome pattern uses the destination DC's text/background colors.
    if (lpbrush->fMono) return CrPalette(lpdc, rgb[0] ? lpdc->crBk : lpdc->crText);
    return win32::RGB(rgb[0], rgb[1], rgb[2]);
}

HBRUSH win32::CreatePatternBrush(HBITMAP hbmp) {
    COMPATOBJECT *lpbm = LpObject(static_cast<HGDIOBJ>(hbmp)), *lpobj;
    SDL_Surface *lpsurface;
    if (!lpbm || lpbm->iKind != objBitmap) return nullptr;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner(SDL_DuplicateSurface(lpbm->lpSurface.get()));
    lpsurface = lpsurfaceOwner.get();
    if (!lpsurface) return nullptr;
    lpobj = LpNewObject(objBrush, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->lpSurface.reset(lpsurfaceOwner.release()); lpobj->fMono = lpbm->fMono;
    return reinterpret_cast<HBRUSH>(lpobj);
}

static BOOL FFillDevice(HDC lpdc, RECT rcFill, COMPATOBJECT *lpbrush, BOOL fRop);

int win32::FillRect(HDC hdc, const RECT *lprc, HBRUSH hbr) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT brush = {}, *lpbrush = LpObject(hbr);
    if (reinterpret_cast<UINT_PTR>(hbr) > 0 && reinterpret_cast<UINT_PTR>(hbr) <= win32::COLOR_MENUBAR + 1) {
        brush.iKind = objBrush; brush.crColor = win32::GetSysColor(static_cast<int>(reinterpret_cast<UINT_PTR>(hbr)) - 1); lpbrush = &brush;
    }
    if (!lpdc || !lprc || !lpbrush || lpbrush->iKind != objBrush) return 0;
    return FFillDevice(lpdc, RcDevice(lpdc, *lprc), lpbrush, 0);
}

void CompatDrawClassicEdge(HDC hdc, const RECT *lprc, BOOL fSunken, BOOL fRight) {
    RECT rc = *lprc, edge;
    HBRUSH hbr;
    int i;
    for (i = 0; i < 2 && rc.right > rc.left && rc.bottom > rc.top; i++) {
        hbr = win32::CreateSolidBrush(win32::GetSysColor(fSunken ? (i ? win32::COLOR_3DDKSHADOW : win32::COLOR_BTNSHADOW) :
                                          (i ? win32::COLOR_3DLIGHT : win32::COLOR_BTNHIGHLIGHT)));
        edge = rc; edge.bottom = rc.top + 1; win32::FillRect(hdc, &edge, hbr);
        edge = rc; edge.right = rc.left + 1; win32::FillRect(hdc, &edge, hbr); win32::DeleteObject(hbr);
        hbr = win32::CreateSolidBrush(win32::GetSysColor(fSunken ? (i ? win32::COLOR_3DLIGHT : win32::COLOR_BTNHIGHLIGHT) :
                                          (i ? win32::COLOR_BTNSHADOW : win32::COLOR_3DDKSHADOW)));
        edge = rc; edge.top = rc.bottom - 1; win32::FillRect(hdc, &edge, hbr);
        if (fRight) { edge = rc; edge.left = rc.right - 1; win32::FillRect(hdc, &edge, hbr); }
        win32::DeleteObject(hbr); win32::InflateRect(&rc, -1, -1);
    }
}

int win32::FrameRect(HDC hdc, const RECT *lprc, HBRUSH hbr) {
    RECT rc;
    if (!lprc || lprc->left >= lprc->right || lprc->top >= lprc->bottom) return 0;
    rc = RECT{lprc->left, lprc->top, lprc->right, lprc->top + 1};
    if (!win32::FillRect(hdc, &rc, hbr)) return 0;
    rc = RECT{lprc->left, lprc->bottom - 1, lprc->right, lprc->bottom};
    if (!win32::FillRect(hdc, &rc, hbr)) return 0;
    rc = RECT{lprc->left, lprc->top + 1, lprc->left + 1, lprc->bottom - 1};
    if (!win32::FillRect(hdc, &rc, hbr)) return 0;
    rc = RECT{lprc->right - 1, lprc->top + 1, lprc->right, lprc->bottom - 1};
    return win32::FillRect(hdc, &rc, hbr);
}

static BOOL FUsesSource(BYTE rop) {
    return ((rop ^ (rop >> 2)) & 0x33) != 0;
}

static BOOL FUsesPattern(BYTE rop) {
    return ((rop ^ (rop >> 4)) & 0x0f) != 0;
}

static COLORREF CrSurfacePixel(SDL_Surface *lpsurface, int x, int y) {
    BYTE *rgb = static_cast<BYTE *>(lpsurface->pixels) + static_cast<std::size_t>(y) * lpsurface->pitch + static_cast<std::size_t>(x) * 4;
    return win32::RGB(rgb[0], rgb[1], rgb[2]);
}

static COLORREF CrDCPixel(HDC lpdc, int x, int y) {
    int iScale = IDCScale(lpdc);
    return CrSurfacePixel(LpDCRasterSurface(lpdc), x * iScale, y * iScale);
}

static void ShapePixel(HDC lpdc, const RECT *lprc, std::int64_t x, std::int64_t y, COLORREF crPen) {
    COLORREF crDest, cr = 0;
    int i, ix, iy, iScale = IDCScale(lpdc);
    if (x < lprc->left || x >= lprc->right || y < lprc->top || y >= lprc->bottom || !FRegionPixel(lpdc, x, y)) return;
    crPen = CrPalette(lpdc, crPen);
    if (lpdc->fMemory && lpdc->lpbitmap->fMono)
        crPen = (static_cast<BYTE>(crPen) + static_cast<BYTE>((crPen >> 8)) + static_cast<BYTE>((crPen >> 16)) >= 384) ? win32::RGB(255, 255, 255) : 0;
    for (iy = 0; iy < iScale; iy++) for (ix = 0; ix < iScale; ix++) {
        cr = 0;
        crDest = CrSurfacePixel(LpDCRasterSurface(lpdc), static_cast<int>((x * iScale + ix)), static_cast<int>((y * iScale + iy)));
        for (i = 0; i < 4; i++)
            if ((lpdc->iRop2 - 1) & (1 << i))
                cr |= (i & 2 ? crPen : ~crPen) & (i & 1 ? crDest : ~crDest);
        DevicePixel(lpdc, x * iScale + ix, y * iScale + iy, cr & UINT32_C(0x00ffffff));
    }
}

static BOOL FFillDevice(HDC lpdc, RECT rcFill, COMPATOBJECT *lpbrush, BOOL fRop) {
    RECT rc, rcClip, rcPart, rcPaint;
    SDL_Rect sdlrc;
    SDL_Surface *lpsurface;
    int x, y, iScale = IDCScale(lpdc);
    std::size_t i, j, crc, crcPaint;
    COLORREF cr;
    if (!FClip(lpdc, &rcClip)) return 0;
    if (lpbrush->fNull || !win32::IntersectRect(&rc, &rcFill, &rcClip)) return 1;
    // Solid copies are common for erase/paint and can fill entire clipped spans.
    if (!lpbrush->lpSurface.get() && (!fRop || lpdc->iRop2 == win32::R2_COPYPEN)) {
        lpsurface = LpDCSurface(lpdc); cr = CrPalette(lpdc, lpbrush->crColor);
        if (lpdc->fMemory && lpdc->lpbitmap->fMono)
            cr = (static_cast<BYTE>(cr) + static_cast<BYTE>((cr >> 8)) + static_cast<BYTE>((cr >> 16)) >= 384) ? win32::RGB(255, 255, 255) : 0;
        crc = lpdc->fRegion ? lpdc->crcRegion : 1;
        for (i = 0; i < crc; i++) {
            if (lpdc->fRegion) {
                if (!win32::IntersectRect(&rcPart, &rc, &lpdc->rgrcRegion[i])) continue;
            } else rcPart = rc;
            crcPaint = lpdc->fPaint ? lpdc->crcPaint : 1;
            for (j = 0; j < crcPaint; j++) {
                if (lpdc->fPaint) {
                    if (!win32::IntersectRect(&rcPaint, &rcPart, &lpdc->rgrcPaint[j])) continue;
                } else rcPaint = rcPart;
                sdlrc = SDL_Rect{rcPaint.left * iScale, rcPaint.top * iScale,
                                   (rcPaint.right - rcPaint.left) * iScale, (rcPaint.bottom - rcPaint.top) * iScale};
                if (!SDL_FillSurfaceRect(lpsurface, &sdlrc, SDL_MapSurfaceRGB(lpsurface,
                                        static_cast<BYTE>(cr), static_cast<BYTE>((cr >> 8)), static_cast<BYTE>((cr >> 16))))) return 0;
            }
        }
        return 1;
    }
    for (y = rc.top; y < rc.bottom; y++) for (x = rc.left; x < rc.right; x++) {
        cr = CrBrush(lpdc, lpbrush, x, y);
        if (fRop) ShapePixel(lpdc, &rc, x, y, cr);
        else Pixel(lpdc, &rc, x, y, cr);
    }
    return 1;
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

BOOL win32::BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int xSrc, int ySrc, DWORD dwRop) {
    HDC lpdc = LpDC(hdc), lpdcSrc = nullptr;
    SDL_Surface *lpsrc = nullptr;
    RECT rcClip;
    BYTE rop = static_cast<BYTE>((dwRop >> 16));
    BOOL fSource = FUsesSource(rop), fMonoSrc = 0, fMonoDst;
    std::int64_t px, py, pxSrc = 0, pySrc = 0;
    std::int64_t x0, y0, x1, y1, ix, iy, cxCopy, cyCopy;
    std::size_t cPixels, i;
    int iScale, iScaleSrc = 1, ixPixel, iyPixel;
    COLORREF *rgcr = nullptr, crSource, crPattern, crDest;
    NATIVEARRAYOWNER<COLORREF, std::free> rgcrOwner;

    if (!lpdc || cx <= 0 || cy <= 0 || (dwRop & UINT32_C(0xff000000)) || !FClip(lpdc, &rcClip))
        return 0;
    iScale = IDCScale(lpdc);
    if (FUsesPattern(rop) && lpdc->lpbrush->fNull) {
        SDL_SetError("Pattern-dependent blits require a non-null brush");
        return 0;
    }
    if (fSource) {
        lpdcSrc = LpDC(hdcSrc);
        if (!lpdcSrc || !(lpsrc = LpDCSurface(lpdcSrc)))
            return 0;
        fMonoSrc = lpdcSrc->fMemory && lpdcSrc->lpbitmap->fMono;
        iScaleSrc = IDCScale(lpdcSrc);
    }
    px = static_cast<std::int64_t>(x) - lpdc->ptOrg.x; py = static_cast<std::int64_t>(y) - lpdc->ptOrg.y;
    if (fSource) { pxSrc = static_cast<std::int64_t>(xSrc) - lpdcSrc->ptOrg.x; pySrc = static_cast<std::int64_t>(ySrc) - lpdcSrc->ptOrg.y; }
    fMonoDst = lpdc->fMemory && lpdc->lpbitmap->fMono;
    x0 = (((static_cast<std::int64_t>(0)) > (static_cast<std::int64_t>(rcClip.left) - px)) ? (static_cast<std::int64_t>(0)) : (static_cast<std::int64_t>(rcClip.left) - px));
    y0 = (((static_cast<std::int64_t>(0)) > (static_cast<std::int64_t>(rcClip.top) - py)) ? (static_cast<std::int64_t>(0)) : (static_cast<std::int64_t>(rcClip.top) - py));
    x1 = (((static_cast<std::int64_t>(cx)) < (static_cast<std::int64_t>(rcClip.right) - px)) ? (static_cast<std::int64_t>(cx)) : (static_cast<std::int64_t>(rcClip.right) - px));
    y1 = (((static_cast<std::int64_t>(cy)) < (static_cast<std::int64_t>(rcClip.bottom) - py)) ? (static_cast<std::int64_t>(cy)) : (static_cast<std::int64_t>(rcClip.bottom) - py));
    if (fSource) {
        x0 = StarsMax(x0, -pxSrc); y0 = StarsMax(y0, -pySrc);
        x1 = (((x1) < (static_cast<std::int64_t>(lpsrc->w) / IDCScale(lpdcSrc) - pxSrc)) ? (x1) : (static_cast<std::int64_t>(lpsrc->w) / IDCScale(lpdcSrc) - pxSrc));
        y1 = (((y1) < (static_cast<std::int64_t>(lpsrc->h) / IDCScale(lpdcSrc) - pySrc)) ? (y1) : (static_cast<std::int64_t>(lpsrc->h) / IDCScale(lpdcSrc) - pySrc));
    }
    if (x0 >= x1 || y0 >= y1)
        return 1;
    cxCopy = x1 - x0; cyCopy = y1 - y0;
    if (static_cast<std::uint64_t>(cxCopy) > SIZE_MAX / sizeof(*rgcr) / static_cast<std::uint64_t>(cyCopy) / iScale / iScale)
        return 0;
    cPixels = static_cast<std::size_t>(cxCopy) * static_cast<std::size_t>(cyCopy) * iScale * iScale;
    if (fSource) {
        // Capture before writing: overlapping blits must not feed updated pixels back.
        rgcr = static_cast<COLORREF *>((std::malloc(cPixels * sizeof(*rgcr))));
        rgcrOwner.reset(rgcr);
        if (!rgcr)
            return 0;
        i = 0;
        for (iy = y0; iy < y1; iy++)
            for (ix = x0; ix < x1; ix++)
                for (iyPixel = 0; iyPixel < iScale; iyPixel++) for (ixPixel = 0; ixPixel < iScale; ixPixel++)
                    rgcr[i++] = CrSurfacePixel(lpsrc, static_cast<int>(((pxSrc + ix) * iScaleSrc + ixPixel * iScaleSrc / iScale)),
                                              static_cast<int>(((pySrc + iy) * iScaleSrc + iyPixel * iScaleSrc / iScale)));
    }
    i = 0;
    for (iy = y0; iy < y1; iy++) {
        for (ix = x0; ix < x1; ix++) for (iyPixel = 0; iyPixel < iScale; iyPixel++) for (ixPixel = 0; ixPixel < iScale; ixPixel++) {
            crSource = fSource ? rgcr[i++] : 0;
            if (fSource && fMonoSrc && !fMonoDst)
                crSource = CrPalette(lpdc, crSource ? lpdc->crBk : lpdc->crText);
            else if (fSource && !fMonoSrc && fMonoDst)
                crSource = crSource == CrPalette(lpdcSrc, lpdcSrc->crBk) ? win32::RGB(255, 255, 255) : 0;
            crPattern = CrBrush(lpdc, lpdc->lpbrush, px + ix, py + iy);
            if (fMonoDst) crPattern = (static_cast<BYTE>(crPattern) + static_cast<BYTE>((crPattern >> 8)) + static_cast<BYTE>((crPattern >> 16)) >= 384) ? win32::RGB(255, 255, 255) : 0;
            if (!FRegionPixel(lpdc, px + ix, py + iy)) continue;
            crDest = CrSurfacePixel(LpDCRasterSurface(lpdc), static_cast<int>(((px + ix) * iScale + ixPixel)),
                                   static_cast<int>(((py + iy) * iScale + iyPixel)));
            DevicePixel(lpdc, (px + ix) * iScale + ixPixel, (py + iy) * iScale + iyPixel,
                        CrRop(rop, crPattern, crSource, crDest));
        }
    }
    return 1;
}

BOOL win32::ScrollWindow(HWND hwnd, int dx, int dy, const RECT *lprc, const RECT *lprcClip) {
    HWND lpwin = LpCompatWindow(hwnd), hwndChild, hwndNext;
    RECT rcClient, rcScroll, rcClip, rcArea, rcMoved, rcDest, rc;
    RECT *rgrc = nullptr;
    std::size_t crc = 0, i;
    SDL_Surface *lpsurface;
    BYTE *lpSrc, *lpDst;
    int y, yFirst, yLast, dRow, iScale;
    std::size_t cbRow;
    BOOL fErase, fCopy, fArea;
    NATIVECLEANUPSCOPE updateOwner([&]() noexcept { std::free(rgrc); });
    if (!lpwin || lpwin->fDestroying || !win32::GetClientRect(hwnd, &rcClient)) {
        win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0;
    }
    if (dx == 0 && dy == 0) return 1;
    fArea = win32::IntersectRect(&rcScroll, lprc ? lprc : &rcClient, &rcClient) &&
            win32::IntersectRect(&rcClip, lprcClip ? lprcClip : &rcClient, &rcClient) &&
            win32::IntersectRect(&rcArea, &rcScroll, &rcClip);
    if (!lprc) for (hwndChild = lpwin->hwndFirstChild; hwndChild; hwndChild = hwndChild->hwndZNext) {
        if (static_cast<std::int64_t>(hwndChild->rcChild.left) + dx < INT_MIN ||
            static_cast<std::int64_t>(hwndChild->rcChild.right) + dx > INT_MAX ||
            static_cast<std::int64_t>(hwndChild->rcChild.top) + dy < INT_MIN ||
            static_cast<std::int64_t>(hwndChild->rcChild.bottom) + dy > INT_MAX) {
            win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
        }
    }
    if (!fArea) goto MoveChildren;
    rcMoved = RECT{LCoord(static_cast<std::int64_t>(rcArea.left) + dx), LCoord(static_cast<std::int64_t>(rcArea.top) + dy),
                     LCoord(static_cast<std::int64_t>(rcArea.right) + dx), LCoord(static_cast<std::int64_t>(rcArea.bottom) + dy)};
    fCopy = win32::IntersectRect(&rcDest, &rcArea, &rcMoved);
    fErase = lpwin->fErase;
    for (i = 0; i < lpwin->crcUpdate; i++) {
        rc = lpwin->rgrcUpdate[i];
        if (!lprc) {
            // The whole-client variant moves pending damage with the pixels.
            if (!FSubtractUpdate(&rgrc, &crc, rc, &rcClip)) goto Fail;
            if (!win32::IntersectRect(&rc, &rc, &rcClip)) continue;
            rc = RECT{LCoord(static_cast<std::int64_t>(rc.left) + dx), LCoord(static_cast<std::int64_t>(rc.top) + dy),
                        LCoord(static_cast<std::int64_t>(rc.right) + dx), LCoord(static_cast<std::int64_t>(rc.bottom) + dy)};
            if (!win32::IntersectRect(&rc, &rc, &rcClip)) continue;
        }
        if (!FAddUpdate(&rgrc, &crc, rc)) goto Fail;
    }
    if (!FSubtractUpdate(&rgrc, &crc, rcArea, &rcDest)) goto Fail;
    if (fCopy) {
        if (!FCanvas(hwnd)) goto Fail;
        lpsurface = lpwin->lpCanvas.get();
        iScale = lpwin->iCanvasScale;
        cbRow = static_cast<std::size_t>((rcDest.right - rcDest.left)) * iScale * 4;
        /* Window canvases are RGBA32. Copy overlapping rows in movement order,
           avoiding the generic raster-op path's per-pixel conversions. */
        dRow = dy > 0 ? -1 : 1;
        yFirst = dy > 0 ? rcDest.bottom * iScale - 1 : rcDest.top * iScale;
        yLast = dy > 0 ? rcDest.top * iScale - 1 : rcDest.bottom * iScale;
        for (y = yFirst; y != yLast; y += dRow) {
            lpDst = static_cast<BYTE *>(lpsurface->pixels) + static_cast<std::size_t>(y) * lpsurface->pitch + static_cast<std::size_t>(rcDest.left) * iScale * 4;
            lpSrc = static_cast<BYTE *>(lpsurface->pixels) + static_cast<std::size_t>((static_cast<std::int64_t>(y) - static_cast<std::int64_t>(dy) * iScale)) * lpsurface->pitch +
                    static_cast<std::size_t>((static_cast<std::int64_t>(rcDest.left) - dx)) * iScale * 4;
            std::memmove(lpDst, lpSrc, cbRow);
        }
    }
    SetUpdate(lpwin, rgrc, crc); rgrc = nullptr;
    lpwin->fErase = crc != 0 && (fErase || !win32::EqualRect(&rcArea, &rcDest));
MoveChildren:
    if (!lprc) for (hwndChild = lpwin->hwndFirstChild; hwndChild; hwndChild = hwndNext) {
        hwndNext = hwndChild->hwndZNext;
        if (!win32::IsWindow(hwndChild)) continue;
        if (!win32::SetWindowPos(hwndChild, nullptr, static_cast<int>((static_cast<std::int64_t>(hwndChild->rcChild.left) + dx)),
                          static_cast<int>((static_cast<std::int64_t>(hwndChild->rcChild.top) + dy)), 0, 0,
                          win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_NOREDRAW)) return 0;
        if (!win32::IsWindow(hwnd)) return 0;
    }
    if (fArea) FCompatPresent(hwnd);
    return 1;
Fail:
    updateOwner.Close(); win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); return 0;
}


BOOL win32::PatBlt(HDC hdc, int x, int y, int cx, int cy, DWORD dwRop) {
    if (FUsesSource(static_cast<BYTE>((dwRop >> 16))))
        return 0;
    return win32::BitBlt(hdc, x, y, cx, cy, nullptr, 0, 0, dwRop);
}

BOOL win32::MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) {
    HDC lpdc = LpDC(hdc);

    if (!lpdc)
        return 0;
    if (lppt)
        *lppt = lpdc->pt;
    lpdc->pt = POINT{x, y};
    return 1;
}

static void PenPixel(HDC lpdc, const RECT *lprc, std::int64_t x, std::int64_t y, std::uint64_t iPixel) {
    static const BYTE rgDash[4][6] = {{18, 6}, {3, 3}, {9, 6, 3, 6}, {9, 3, 3, 3, 3, 3}};
    static const BYTE rgc[] = {2, 2, 4, 6};
    int iStyle = lpdc->lppen->iPenStyle, i;
    unsigned cCycle = 0, iPhase;
    if (lpdc->lppen->fNull) return;
    if (iStyle == win32::PS_SOLID) { ShapePixel(lpdc, lprc, x, y, lpdc->lppen->crColor); return; }
    for (i = 0; i < rgc[iStyle - 1]; i++) cCycle += rgDash[iStyle - 1][i];
    iPhase = static_cast<unsigned>((iPixel % cCycle));
    for (i = 0; i < rgc[iStyle - 1]; i++) {
        if (iPhase < rgDash[iStyle - 1][i]) {
            if (!(i & 1)) ShapePixel(lpdc, lprc, x, y, lpdc->lppen->crColor);
            else if (lpdc->iBkMode == win32::OPAQUE) ShapePixel(lpdc, lprc, x, y, lpdc->crBk);
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
        if (t > *lpt1) return 0;
        if (t > *lpt0) *lpt0 = t;
    } else {
        if (t < *lpt0) return 0;
        if (t < *lpt1) *lpt1 = t;
    }
    return 1;
}

static void Line(HDC lpdc, RECT rc, POINT pt0, POINT pt1, BOOL fExcludeEnd, std::uint64_t *lpiPhase) {
    double xStart = static_cast<double>(pt0.x) - lpdc->ptOrg.x, yStart = static_cast<double>(pt0.y) - lpdc->ptOrg.y;
    double xEnd = static_cast<double>(pt1.x) - lpdc->ptOrg.x, yEnd = static_cast<double>(pt1.y) - lpdc->ptOrg.y;
    double dx = xEnd - xStart, dy = yEnd - yStart, t0 = 0, t1 = 1;
    std::int64_t x, y, x1, y1, cx, cy, sx, sy, err, e2;
    std::uint64_t iPixel, iPhase = lpiPhase ? *lpiPhase : 0;
    if (lpiPhase) *lpiPhase += static_cast<std::uint64_t>((((std::fabs(dx)) > (std::fabs(dy))) ? (std::fabs(dx)) : (std::fabs(dy)))) + !fExcludeEnd;

    if (rc.left >= rc.right || rc.top >= rc.bottom || lpdc->lppen->fNull ||
        !FLineBound(-dx, xStart - rc.left, &t0, &t1) ||
        !FLineBound(dx, static_cast<double>(rc.right) - 1 - xStart, &t0, &t1) ||
        !FLineBound(-dy, yStart - rc.top, &t0, &t1) ||
        !FLineBound(dy, static_cast<double>(rc.bottom) - 1 - yStart, &t0, &t1))
        return;
    x = static_cast<std::int64_t>((xStart + t0 * dx));
    y = static_cast<std::int64_t>((yStart + t0 * dy));
    x1 = static_cast<std::int64_t>((xStart + t1 * dx));
    y1 = static_cast<std::int64_t>((yStart + t1 * dy));
    cx = std::llabs(x1 - x); cy = -std::llabs(y1 - y);
    sx = x < x1 ? 1 : -1; sy = y < y1 ? 1 : -1;
    err = cx + cy;
    iPixel = iPhase + static_cast<std::uint64_t>((((std::fabs(static_cast<double>(x) - xStart)) > (std::fabs(static_cast<double>(y) - yStart))) ? (std::fabs(static_cast<double>(x) - xStart)) : (std::fabs(static_cast<double>(y) - yStart))));
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

BOOL win32::LineTo(HDC hdc, int x, int y) {
    HDC lpdc = LpDC(hdc);
    RECT rc;
    POINT pt;

    if (!lpdc)
        return 0;
    pt = lpdc->pt;
    lpdc->pt = POINT{x, y};
    if (!FClip(lpdc, &rc))
        return 0;
    Line(lpdc, rc, pt, lpdc->pt, 1, nullptr);
    return 1;
}

BOOL win32::Rectangle(HDC hdc, int left, int top, int right, int bottom) {
    HDC lpdc = LpDC(hdc);
    RECT rc, rcFill;
    std::uint64_t iPhase = 0;
    int l = StarsMin(left, right), r = StarsMax(left, right), t = StarsMin(top, bottom), b = StarsMax(top, bottom);

    if (!lpdc)
        return 0;
    if (!FClip(lpdc, &rc))
        return 0;
    if (l == r || t == b)
        return 1;
    rcFill = lpdc->lppen->fNull ? RECT{l, t, r, b} : RECT{l + 1, t + 1, r - 1, b - 1};
    FFillDevice(lpdc, RcDevice(lpdc, rcFill), lpdc->lpbrush, 1);
    // Counterclockwise from top-right, sharing one dash phase and painting
    // each corner once, matches the default Win32 rectangle traversal.
    if (r - static_cast<std::int64_t>(l) == 1) Line(lpdc, rc, POINT{l, t}, POINT{l, b - 1}, 0, &iPhase);
    else if (b - static_cast<std::int64_t>(t) == 1) Line(lpdc, rc, POINT{r - 1, t}, POINT{l, t}, 0, &iPhase);
    else {
        Line(lpdc, rc, POINT{r - 1, t}, POINT{l, t}, 1, &iPhase);
        Line(lpdc, rc, POINT{l, t}, POINT{l, b - 1}, 1, &iPhase);
        Line(lpdc, rc, POINT{l, b - 1}, POINT{r - 1, b - 1}, 1, &iPhase);
        Line(lpdc, rc, POINT{r - 1, b - 1}, POINT{r - 1, t}, 1, &iPhase);
    }
    return 1;
}

struct COMPATELLIPSE {
    RECT rc;
    int cx;
    NATIVEARRAYOWNER<BYTE, std::free> rgb;
    NATIVEARRAYOWNER<std::int64_t, std::free> rglLeft, rglRight;
};

static void EllipsePoint(COMPATELLIPSE *lp, std::int64_t x, std::int64_t y) {
    std::size_t iRow;
    if (y < lp->rc.top || y >= lp->rc.bottom) return;
    iRow = static_cast<std::size_t>((y - lp->rc.top));
    lp->rglLeft[iRow] = StarsMin(lp->rglLeft[iRow], x);
    lp->rglRight[iRow] = StarsMax(lp->rglRight[iRow], x);
    if (x >= lp->rc.left && x < lp->rc.right)
        lp->rgb[iRow * lp->cx + static_cast<std::size_t>((x - lp->rc.left))] = 1;
}

__extension__ using COMPATELLIPSEERROR = __int128;

/* plotEllipseRect adapted from Alois Zingl's Bresenham V20.15 (April 2020),
 * https://github.com/zingl/Bresenham/blob/master/bresenham.c .
 * MIT license: ../LICENSE.zingl. Large ellipses need wider error accumulators
 * than their coordinates; these private integers never reach the Win32 ABI.
 * Accumulate each border pixel once so mirrored tips cannot cancel under XOR. */
static void EllipseOutline(COMPATELLIPSE *lp, std::int64_t x0, std::int64_t y0, std::int64_t x1, std::int64_t y1) {
    std::int64_t a = x1 - x0, b = y1 - y0, b1 = b & 1;
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

BOOL win32::Ellipse(HDC hdc, int left, int top, int right, int bottom) {
    HDC lpdc = LpDC(hdc);
    COMPATELLIPSE el = {};
    RECT rcClip;
    std::int64_t l, t, r, b;
    std::size_t cb, i;
    int x, y;
    BOOL fBorder;

    if (!lpdc || !FClip(lpdc, &rcClip)) return 0;
    l = static_cast<std::int64_t>(StarsMin(left, right)) - lpdc->ptOrg.x; r = static_cast<std::int64_t>(StarsMax(left, right)) - lpdc->ptOrg.x;
    t = static_cast<std::int64_t>(StarsMin(top, bottom)) - lpdc->ptOrg.y; b = static_cast<std::int64_t>(StarsMax(top, bottom)) - lpdc->ptOrg.y;
    if (l == r || t == b || (r - l == 1 && b - t == 1) || r <= rcClip.left || l >= rcClip.right || b <= rcClip.top || t >= rcClip.bottom) return 1;
    // Cap iteration work independently of the clipped mask size.
    if (r - l > 1048576 || b - t > 1048576) return 0;
    if (!lpdc->lppen->fNull && lpdc->lppen->iPenStyle != win32::PS_SOLID) {
        SDL_SetError("Ellipse currently supports solid or null cosmetic pens"); return 0;
    }
    if (lpdc->lppen->fNull && (r - l <= 2 || b - t <= 2))
        return FFillDevice(lpdc, RECT{LCoord(l), LCoord(t), LCoord(r - 1), LCoord(b - 1)}, lpdc->lpbrush, 1);
    el.rc = RECT{static_cast<LONG>(StarsMax(l, rcClip.left)), static_cast<LONG>(StarsMax(t, rcClip.top)),
                   static_cast<LONG>(StarsMin(r, rcClip.right)), static_cast<LONG>(StarsMin(b, rcClip.bottom))};
    el.cx = el.rc.right - el.rc.left;
    if (static_cast<std::size_t>(el.cx) > SIZE_MAX / static_cast<std::size_t>((el.rc.bottom - el.rc.top))) return 0;
    cb = static_cast<std::size_t>(el.cx) * static_cast<std::size_t>((el.rc.bottom - el.rc.top));
    el.rgb.reset(static_cast<BYTE *>(std::calloc(cb, 1)));
    el.rglLeft.reset(static_cast<std::int64_t *>(std::malloc(static_cast<std::size_t>(el.rc.bottom - el.rc.top) * sizeof(std::int64_t))));
    el.rglRight.reset(static_cast<std::int64_t *>(std::malloc(static_cast<std::size_t>(el.rc.bottom - el.rc.top) * sizeof(std::int64_t))));
    if (!el.rgb || !el.rglLeft || !el.rglRight) return 0;
    for (y = el.rc.top; y < el.rc.bottom; y++) {
        el.rglLeft[y - el.rc.top] = INT64_MAX; el.rglRight[y - el.rc.top] = INT64_MIN;
    }
    EllipseOutline(&el, l, t, r - 1, b - 1);
    for (y = el.rc.top; y < el.rc.bottom; y++) for (x = el.rc.left; x < el.rc.right; x++) {
        i = static_cast<std::size_t>((y - el.rc.top)); fBorder = el.rgb[i * el.cx + static_cast<std::size_t>((x - el.rc.left))] != 0;
        if (fBorder && !lpdc->lppen->fNull) ShapePixel(lpdc, &el.rc, x, y, lpdc->lppen->crColor);
        else if (!lpdc->lpbrush->fNull && x >= el.rglLeft[i] && x <= el.rglRight[i])
            ShapePixel(lpdc, &el.rc, x, y, CrBrush(lpdc, lpdc->lpbrush, x, y));
    }
    return 1;
}

COLORREF win32::SetTextColor(HDC hdc, COLORREF crColor) {
    HDC lpdc = LpDC(hdc);
    COLORREF crOld;

    if (!lpdc || !FColor(crColor))
        return win32::CLR_INVALID;
    crOld = lpdc->crText; lpdc->crText = crColor;
    return crOld;
}

COLORREF win32::SetBkColor(HDC hdc, COLORREF crColor) {
    HDC lpdc = LpDC(hdc);
    COLORREF crOld;

    if (!lpdc || !FColor(crColor))
        return win32::CLR_INVALID;
    crOld = lpdc->crBk; lpdc->crBk = crColor;
    return crOld;
}

int win32::SetBkMode(HDC hdc, int iMode) {
    HDC lpdc = LpDC(hdc);
    int lOld;

    if (!lpdc || (iMode != win32::OPAQUE && iMode != win32::TRANSPARENT))
        return 0;
    lOld = lpdc->iBkMode; lpdc->iBkMode = iMode;
    return lOld;
}

struct COMPATTEXTDRAW {
    HDC lpdc;
    RECT rc;
    std::int64_t x, y;
    double cs, sn;
};

static BOOL FTextSize(HDC lpdc, LPCSTR psz, int cch, LPSIZE lpsize) {
    return FCompatFontSizeUtf8(lpdc->lpfont->lpFont.get(), psz, cch, lpsize);
}

BOOL win32::GetTextExtentPoint32A(HDC hdc, LPCSTR psz, int cch, LPSIZE lpsize) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FTextSize(lpdc, psz, cch, lpsize);
}

BOOL win32::GetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FCompatFontMetrics(lpdc->lpfont->lpFont.get(), lptm);
}

COLORREF win32::GetBkColor(HDC hdc) {
    HDC lpdc = LpDC(hdc);
    return lpdc ? lpdc->crBk : win32::CLR_INVALID;
}

int win32::GetDeviceCaps(HDC hdc, int index) {
    HDC lpdc = LpDC(hdc);
    const SDL_DisplayMode *lpMode;
    SDL_DisplayID id;
    if (!lpdc) return 0;
    if (lpdc->lpprint) return ICompatPrinterCaps(lpdc->lpprint.get(), index);
    if (index == win32::LOGPIXELSX || index == win32::LOGPIXELSY) return 96;
    if (index == win32::PLANES) return 1;
    if (index == win32::BITSPIXEL) return 32;
    if (index != win32::HORZRES && index != win32::VERTRES) return 0;
    id = lpdc->hwnd ? SDL_GetDisplayForWindow(HwndCompatRoot(lpdc->hwnd)->lpWindow.get()) : CompatDisplayIO().pfnGetPrimaryDisplay();
    lpMode = SDL_GetCurrentDisplayMode(id);
    return lpMode ? index == win32::HORZRES ? lpMode->w : lpMode->h : 0;
}

static void DrawFontPixel(void *lpv, std::int64_t x, std::int64_t y, BYTE alpha) {
    COMPATTEXTDRAW *lpdraw = static_cast<COMPATTEXTDRAW *>((lpv));
    int iScale = IDCScale(lpdraw->lpdc);
    std::int64_t px = static_cast<std::int64_t>(lpdraw->x) * iScale + std::llround(lpdraw->cs * x + lpdraw->sn * y);
    std::int64_t py = static_cast<std::int64_t>(lpdraw->y) * iScale + std::llround(-lpdraw->sn * x + lpdraw->cs * y);
    COLORREF cr;
    BYTE *rgb;
    unsigned r, g, b;

    if (px < static_cast<std::int64_t>(lpdraw->rc.left) * iScale || px >= static_cast<std::int64_t>(lpdraw->rc.right) * iScale ||
        py < static_cast<std::int64_t>(lpdraw->rc.top) * iScale || py >= static_cast<std::int64_t>(lpdraw->rc.bottom) * iScale ||
        !FRegionPixel(lpdraw->lpdc, px / iScale, py / iScale)) return;
    cr = CrPalette(lpdraw->lpdc, lpdraw->lpdc->crText);
    rgb = static_cast<BYTE *>(LpDCRasterSurface(lpdraw->lpdc)->pixels) + static_cast<std::size_t>(py) * LpDCRasterSurface(lpdraw->lpdc)->pitch + static_cast<std::size_t>(px) * 4;
    r = (static_cast<BYTE>(cr) * alpha + rgb[0] * (255 - alpha) + 127) / 255;
    g = (static_cast<BYTE>((cr >> 8)) * alpha + rgb[1] * (255 - alpha) + 127) / 255;
    b = (static_cast<BYTE>((cr >> 16)) * alpha + rgb[2] * (255 - alpha) + 127) / 255;
    DevicePixel(lpdraw->lpdc, px, py, win32::RGB(r, g, b));
}

static void DrawFontRect(COMPATTEXTDRAW *lpdraw, std::int64_t dx, int yTop, int yBottom, COLORREF cr) {
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
    xLeft = static_cast<int>(std::fmax(lpdraw->rc.left, std::fmin(lpdraw->rc.right, std::floor(xMin))));
    xRight = static_cast<int>(std::fmax(lpdraw->rc.left, std::fmin(lpdraw->rc.right, std::ceil(xMax))));
    yFirst = static_cast<int>(std::fmax(lpdraw->rc.top, std::fmin(lpdraw->rc.bottom, std::floor(yMin))));
    yLast = static_cast<int>(std::fmax(lpdraw->rc.top, std::fmin(lpdraw->rc.bottom, std::ceil(yMax))));
    for (y = yFirst; y < yLast; y++)
        for (x = xLeft; x < xRight; x++) {
            u = lpdraw->cs * (static_cast<std::int64_t>(x) - lpdraw->x) - lpdraw->sn * (static_cast<std::int64_t>(y) - lpdraw->y);
            v = lpdraw->sn * (static_cast<std::int64_t>(x) - lpdraw->x) + lpdraw->cs * (static_cast<std::int64_t>(y) - lpdraw->y);
            if (u >= std::fmin(0, dx) && u < std::fmax(0, dx) && v >= yTop && v < yBottom)
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
    std::int64_t dx;
    RECT rcDevice;
    if (!FTextSize(lpdc, psz, cch, &size) || !FClip(lpdc, &draw.rc) ||
        ((options & (win32::ETO_OPAQUE | win32::ETO_CLIPPED)) && !lprc)) return 0;
    if (lprc) { rcDevice = RcDevice(lpdc, *lprc); lprc = &rcDevice; }
    // ETO_OPAQUE uses the DC background color, independent of its selected brush.
    if (options & win32::ETO_OPAQUE) {
        RECT rc;
        int ix, iy;
        if (win32::IntersectRect(&rc, &draw.rc, lprc))
            for (iy = rc.top; iy < rc.bottom; iy++) for (ix = rc.left; ix < rc.right; ix++) Pixel(lpdc, &rc, ix, iy, lpdc->crBk);
    }
    if (options & win32::ETO_CLIPPED) win32::IntersectRect(&draw.rc, &draw.rc, lprc);
    draw.lpdc = lpdc; draw.x = static_cast<std::int64_t>(x) - lpdc->ptOrg.x; draw.y = static_cast<std::int64_t>(y) - lpdc->ptOrg.y;
    angle = (lpdc->lpfont->lf.lfEscapement % 3600) * (3.14159265358979323846 / 1800.0);
    draw.cs = std::cos(angle); draw.sn = std::sin(angle);
    dx = size.cx;
    if (rgdx) { dx = 0; for (i = 0; i < cch; i++) dx += rgdx[i]; }
    FCompatFontMetrics(lpdc->lpfont->lpFont.get(), &tm);
    if (lpdc->iBkMode == win32::OPAQUE) DrawFontRect(&draw, dx, 0, tm.tmHeight, lpdc->crBk);
    if (!FCompatFontDrawScaled(lpdc->lpfont->lpFont.get(), psz, cch, rgdx, IDCScale(lpdc),
                               DrawFontPixel, &draw)) return 0;
    if (tm.tmUnderlined) DrawFontRect(&draw, dx, tm.tmAscent + 1, tm.tmAscent + 2, lpdc->crText);
    if (tm.tmStruckOut) DrawFontRect(&draw, dx, tm.tmAscent * 2 / 3, tm.tmAscent * 2 / 3 + 1, lpdc->crText);
    return 1;
}

BOOL win32::TextOutA(HDC hdc, int x, int y, LPCSTR psz, int cch) {
    HDC lpdc = LpDC(hdc);
    return lpdc && FDrawText(lpdc, x, y, psz, cch, 0, nullptr, nullptr);
}

BOOL win32::ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT *lprc,
                       LPCSTR psz, UINT cch, const INT *rgdx) {
    HDC lpdc = LpDC(hdc);
    if (!lpdc || cch > INT_MAX || (options & ~(win32::ETO_OPAQUE | win32::ETO_CLIPPED))) return 0;
    return FDrawText(lpdc, x, y, psz, static_cast<int>(cch), options, lprc, rgdx);
}

struct COMPATTEXTLINE { int i, cch, dx; };

int win32::DrawTextA(HDC hdc, LPCSTR psz, int cch, LPRECT lprc, UINT flags) {
    HDC lpdc = LpDC(hdc);
    char *sz = nullptr;
    BYTE *rgUnderline = nullptr;
    COMPATTEXTLINE *rgline = nullptr;
    NATIVEARRAYOWNER<char, std::free> szOwner;
    NATIVEARRAYOWNER<BYTE, std::free> rgUnderlineOwner;
    NATIVEARRAYOWNER<COMPATTEXTLINE, std::free> rglineOwner;
    TEXTMETRICA tm;
    SIZE size;
    RECT rcOld;
    BOOL fOldClip, fUnderline = 0;
    int i, j, c = 0, cLines = 0, iStart, iBreak, dxBreak, dxMax = 0, dyLine, dy, nReturn = 0, cStep;
    std::uint32_t ch;
    std::int64_t dx, dxAvail, x, y;
    UINT supported = win32::DT_CENTER | win32::DT_RIGHT | win32::DT_VCENTER | win32::DT_BOTTOM | win32::DT_WORDBREAK | win32::DT_SINGLELINE |
                     win32::DT_EXPANDTABS | win32::DT_NOCLIP | win32::DT_EXTERNALLEADING | win32::DT_CALCRECT | win32::DT_NOPREFIX;

    if (!lpdc || !lprc || !psz || cch < -1 || (flags & ~supported)) return 0;
    if (cch == -1) { cch = static_cast<int>(strnlen(psz, 1024 * 1024 + 1)); }
    if (cch > 1024 * 1024) return 0;
    if (cch == 0) { if (flags & win32::DT_CALCRECT) lprc->bottom = lprc->top; return 0; }
    sz = static_cast<char *>((std::malloc(static_cast<std::size_t>(cch) * 8 + 1))); rgUnderline = static_cast<BYTE *>((std::calloc(static_cast<std::size_t>(cch) * 8 + 1, 1)));
    rgline = static_cast<COMPATTEXTLINE *>((std::malloc((static_cast<std::size_t>(cch) * 8 + 1) * sizeof(*rgline))));
    szOwner.reset(sz); rgUnderlineOwner.reset(rgUnderline); rglineOwner.reset(rgline);
    if (!sz || !rgUnderline || !rgline) goto LReturn;
    for (i = 0; i < cch; i++) {
        if (!(flags & win32::DT_NOPREFIX) && psz[i] == '&') {
            if (i + 1 < cch && psz[i + 1] == '&') i++;
            else { fUnderline = 1; continue; }
        }
        if ((flags & win32::DT_EXPANDTABS) && psz[i] == '\t') {
            do { sz[c++] = ' '; } while (c % 8);
        } else if ((flags & win32::DT_SINGLELINE) && (psz[i] == '\r' || psz[i] == '\n')) {
            if (psz[i] == '\r' && i + 1 < cch && psz[i + 1] == '\n') i++;
            sz[c++] = ' ';
        } else {
            sz[c] = psz[i]; rgUnderline[c++] = fUnderline;
        }
        fUnderline = 0;
    }
    sz[c] = 0;
    FCompatFontMetrics(lpdc->lpfont->lpFont.get(), &tm);
    dyLine = tm.tmHeight + (flags & win32::DT_EXTERNALLEADING ? tm.tmExternalLeading : 0);
    dxAvail = static_cast<std::int64_t>(lprc->right) - lprc->left;
    i = 0;
    while (i < c) {
        iStart = i; iBreak = -1; dxBreak = 0; dx = 0;
        while (i < c && sz[i] != '\r' && sz[i] != '\n') {
            cStep = CbUtf8Char(sz + i, static_cast<std::size_t>((c - i)), &ch);
            if (cStep == 0) goto LReturn;
            if (!win32::GetTextExtentPoint32A(hdc, sz + i, cStep, &size)) goto LReturn;
            if (sz[i] == ' ' || sz[i] == '\t') { iBreak = i; dxBreak = static_cast<int>(dx); }
            dx += size.cx;
            if (dx > INT_MAX) goto LReturn;
            if ((flags & win32::DT_WORDBREAK) && !(flags & win32::DT_SINGLELINE) && dx > dxAvail && iBreak > iStart) {
                rgline[cLines++] = COMPATTEXTLINE{iStart, iBreak - iStart, dxBreak};
                i = iBreak + 1;
                while (i < c && sz[i] == ' ') i++;
                goto LNextLine;
            }
            i += cStep;
        }
        rgline[cLines++] = COMPATTEXTLINE{iStart, i - iStart, static_cast<int>(dx)};
        if (i < c && sz[i++] == '\r' && i < c && sz[i] == '\n') i++;
LNextLine:
        if (rgline[cLines - 1].dx > dxMax) dxMax = rgline[cLines - 1].dx;
    }
    if (static_cast<std::int64_t>(cLines) * dyLine > INT_MAX) goto LReturn;
    dy = cLines * dyLine;
    if (flags & win32::DT_CALCRECT) {
        if (static_cast<std::int64_t>(lprc->left) + dxMax > INT_MAX || static_cast<std::int64_t>(lprc->top) + dy > INT_MAX) goto LReturn;
        lprc->right = lprc->left + dxMax; lprc->bottom = lprc->top + dy;
        nReturn = dy; goto LReturn;
    }
    rcOld = lpdc->rcClip; fOldClip = lpdc->fClip;
    if (!(flags & win32::DT_NOCLIP)) {
        lpdc->rcClip = RcDevice(lpdc, *lprc);
        if (fOldClip) win32::IntersectRect(&lpdc->rcClip, &rcOld, &lpdc->rcClip);
        lpdc->fClip = 1;
    }
    y = lprc->top;
    if (flags & win32::DT_SINGLELINE) {
        if (flags & win32::DT_VCENTER) y += (static_cast<std::int64_t>(lprc->bottom) - lprc->top - dy) / 2;
        else if (flags & win32::DT_BOTTOM) y = static_cast<std::int64_t>(lprc->bottom) - dy;
    }
    nReturn = dy;
    for (i = 0; i < cLines; i++, y += dyLine) {
        x = lprc->left;
        if (flags & win32::DT_CENTER) x += (static_cast<std::int64_t>(lprc->right) - lprc->left - rgline[i].dx) / 2;
        else if (flags & win32::DT_RIGHT) x = static_cast<std::int64_t>(lprc->right) - rgline[i].dx;
        if (x < INT_MIN || x > INT_MAX || y < INT_MIN || y > INT_MAX ||
            !win32::TextOutA(hdc, static_cast<int>(x), static_cast<int>(y), sz + rgline[i].i, rgline[i].cch)) { nReturn = 0; break; }
        for (j = 0; j < rgline[i].cch; j++) {
            if (!rgUnderline[rgline[i].i + j]) continue;
            win32::GetTextExtentPoint32A(hdc, sz + rgline[i].i, j, &size);
            dx = x + size.cx;
            cStep = CbUtf8Char(sz + rgline[i].i + j, static_cast<std::size_t>((rgline[i].cch - j)), &ch);
            if (cStep == 0) continue;
            win32::GetTextExtentPoint32A(hdc, sz + rgline[i].i + j, cStep, &size);
            // Leave a pixel below the baseline, matching ordinary font
            // underlines, so mnemonic marks do not merge into the glyph.
            if (dx >= INT_MIN && dx + size.cx <= INT_MAX && y + tm.tmAscent + 2 <= INT_MAX) {
                RECT rc = {static_cast<LONG>(dx), static_cast<LONG>((y + tm.tmAscent + 1)), static_cast<LONG>((dx + size.cx)), static_cast<LONG>((y + tm.tmAscent + 2))};
                COMPATTEXTDRAW draw = {lpdc, {}, dx - lpdc->ptOrg.x, -static_cast<std::int64_t>(lpdc->ptOrg.y), 1, 0};
                if (FClip(lpdc, &draw.rc)) DrawFontRect(&draw, rc.right - rc.left, rc.top, rc.bottom, lpdc->crText);
            }
        }
    }
    lpdc->rcClip = rcOld; lpdc->fClip = fOldClip;
LReturn:
    return nReturn;
}

HBITMAP HbmpCompatResource(const BYTE *rgb, std::size_t cb) {
    BITMAPINFOHEADER bi;
    COMPATDIB dib;
    COMPATOBJECT *lpobj;
    SDL_Surface *lpsurface;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    std::size_t cbInfo, cColors;

    if (!rgb || cb < cbWireDibHeader)
        return nullptr;
    FDecodeDibHeader(std::as_bytes(std::span(rgb, cbWireDibHeader)), bi);
    cColors = bi.biClrUsed ? bi.biClrUsed : (bi.biBitCount <= 8 ? 1u << bi.biBitCount : 0);
    if (cColors > 256)
        return nullptr;
    cbInfo = cbWireDibHeader + cColors * cbWireRgbQuad;
    if (cbInfo > cb || !FCompatDib(rgb, cbInfo, rgb + cbInfo, cb - cbInfo, &dib))
        return nullptr;
    lpsurfaceOwner.reset(LpCompatDibSurface(&dib)); lpsurface = lpsurfaceOwner.get();
    if (!lpsurface)
        return nullptr;
    lpobj = LpNewObject(objBitmap, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->lpSurface.reset(lpsurfaceOwner.release());
    lpobj->fMono = dib.cBits == 1;
    return reinterpret_cast<HBITMAP>(lpobj);
}

int win32::GetDIBits(HDC hdc, HBITMAP hbm, UINT iStart, UINT cLines, LPVOID lpBits, LPBITMAPINFO lpbmi, UINT usage) {
    HDC lpdc = LpDC(hdc);
    COMPATOBJECT *lpobj = LpObject(hbm);
    BITMAPINFOHEADER bi;
    SDL_Surface *lpsurface;
    std::size_t cbRow;
    UINT i, cCopy;
    int x, y;
    COLORREF cr;
    BYTE *rgb;
    BOOL fTopDown, fQuery;

    if (!lpdc || !lpobj || lpobj->iKind != objBitmap || lpobj->cSelected != 0 ||
        !lpbmi || usage != win32::DIB_RGB_COLORS || lpbmi->bmiHeader.biSize != sizeof(bi))
        return 0;
    lpsurface = lpobj->lpSurface.get();
    bi = lpbmi->bmiHeader;
    fQuery = bi.biBitCount == 0;
    if (fQuery && lpBits)
        return 0;
    if (fQuery)
        bi.biBitCount = lpobj->fMono ? 1 : 32;
    if ((bi.biBitCount != 1 && bi.biBitCount != 24 && bi.biBitCount != 32) || bi.biCompression != win32::BI_RGB ||
        (bi.biBitCount == 1 && !lpobj->fMono) ||
        (bi.biWidth != 0 && bi.biWidth != lpsurface->w) ||
        (bi.biHeight != 0 && bi.biHeight != lpsurface->h && bi.biHeight != -lpsurface->h))
        return 0;
    fTopDown = bi.biHeight < 0;
    bi.biWidth = lpsurface->w;
    bi.biHeight = fTopDown ? -lpsurface->h : lpsurface->h;
    bi.biPlanes = 1;
    bi.biClrUsed = bi.biBitCount == 1 ? 2 : 0;
    cbRow = ((static_cast<std::size_t>(lpsurface->w) * bi.biBitCount + 31) / 32) * 4;
    if (cbRow > UINT32_MAX / static_cast<std::size_t>(lpsurface->h))
        return 0;
    bi.biSizeImage = static_cast<DWORD>((cbRow * static_cast<std::size_t>(lpsurface->h)));
    lpbmi->bmiHeader = bi;
    // A format query may supply only the 40-byte header, with no palette storage.
    if (bi.biBitCount == 1 && !fQuery) {
        lpbmi->bmiColors[0] = RGBQUAD{0, 0, 0, 0};
        lpbmi->bmiColors[1] = RGBQUAD{255, 255, 255, 0};
    }
    if (!lpBits)
        return lpsurface->h;
    if (iStart >= static_cast<UINT>(lpsurface->h))
        return 0;
    cCopy = (((cLines) < (static_cast<UINT>(lpsurface->h) - iStart)) ? (cLines) : (static_cast<UINT>(lpsurface->h) - iStart));
    for (i = 0; i < cCopy; i++) {
        rgb = static_cast<BYTE *>(lpBits) + static_cast<std::size_t>(i) * cbRow;
        std::fill_n(rgb, cbRow, 0);
        y = fTopDown ? static_cast<int>((iStart + i)) : lpsurface->h - 1 - static_cast<int>((iStart + i));
        for (x = 0; x < lpsurface->w; x++) {
            cr = CrSurfacePixel(lpsurface, x, y);
            if (bi.biBitCount == 1) {
                if (static_cast<BYTE>(cr) + static_cast<BYTE>((cr >> 8)) + static_cast<BYTE>((cr >> 16)) >= 384)
                    rgb[x / 8] |= static_cast<BYTE>((1 << (7 - x % 8)));
            } else {
                rgb[x * (bi.biBitCount / 8)] = static_cast<BYTE>((cr >> 16));
                rgb[x * (bi.biBitCount / 8) + 1] = static_cast<BYTE>((cr >> 8));
                rgb[x * (bi.biBitCount / 8) + 2] = static_cast<BYTE>(cr);
            }
        }
    }
    return static_cast<int>(cCopy);
}

static BOOL FPaletteDib(HDC lpdc, const BITMAPINFO *lpbmi, const void *lpBits, UINT usage, COMPATDIB *lpdib) {
    struct { BITMAPINFOHEADER bi; RGBQUAD rgq[256]; } info;
    UINT c, i;
    WORD index;
    PALETTEENTRY pe;
    const BYTE *rgb = reinterpret_cast<const BYTE *>(lpbmi);
    if (usage == win32::DIB_RGB_COLORS) return FCompatNativeDib(lpbmi, SIZE_MAX, lpBits, SIZE_MAX, lpdib);
    if (usage != win32::DIB_PAL_COLORS || !lpbmi || lpbmi->bmiHeader.biSize != sizeof(info.bi)) return 0;
    info.bi = lpbmi->bmiHeader;
    if (info.bi.biBitCount != 1 && info.bi.biBitCount != 4 && info.bi.biBitCount != 8) return 0;
    c = info.bi.biClrUsed ? info.bi.biClrUsed : 1u << info.bi.biBitCount;
    if (c > (1u << info.bi.biBitCount)) return 0;
    for (i = 0; i < c; i++) {
        std::memcpy(&index, rgb + sizeof(info.bi) + i * sizeof(index), sizeof(index));
        if (index >= lpdc->lppalette->cEntries) return 0;
        pe = RgpeCompatPalette(lpdc->lppalette)[index];
        info.rgq[i] = RGBQUAD{pe.peBlue, pe.peGreen, pe.peRed, 0};
    }
    // The decoder copies the color table; only the caller's bits remain borrowed.
    return FCompatNativeDib(&info, sizeof(info), lpBits, SIZE_MAX, lpdib);
}

int win32::StretchDIBits(HDC hdc, int xDest, int yDest, int cxDest, int cyDest,
                         int xSrc, int ySrc, int cxSrc, int cySrc, const void *lpBits,
                         const BITMAPINFO *lpbmi, UINT usage, DWORD dwRop) {
    HDC lpdc = LpDC(hdc);
    COMPATDIB dib;
    SDL_Surface *lpsrc;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    RECT rcClip;
    BYTE rop = static_cast<BYTE>((dwRop >> 16));
    std::int64_t px, py;
    std::int64_t x0, y0, x1, y1, ix, iy, sx, sy, yTop;
    COLORREF crSource, crPattern, crDest;

    if (!lpdc || (lpdc->fMemory && lpdc->lpbitmap->fMono) ||
        cxDest <= 0 || cyDest <= 0 || cxSrc <= 0 || cySrc <= 0 ||
        (dwRop & UINT32_C(0xff000000)) || !FPaletteDib(lpdc, lpbmi, lpBits, usage, &dib))
        return 0;
    yTop = dib.fTopDown ? ySrc : static_cast<std::int64_t>(dib.cy) - ySrc - cySrc;
    if (xSrc < 0 || yTop < 0 || static_cast<std::int64_t>(xSrc) + cxSrc > dib.cx || yTop + cySrc > dib.cy ||
        (FUsesPattern(rop) && lpdc->lpbrush->fNull) || !FClip(lpdc, &rcClip))
        return 0;
    lpsurfaceOwner.reset(LpCompatDibSurface(&dib)); lpsrc = lpsurfaceOwner.get();
    if (!lpsrc)
        return 0;
    px = static_cast<std::int64_t>(xDest) - lpdc->ptOrg.x; py = static_cast<std::int64_t>(yDest) - lpdc->ptOrg.y;
    x0 = (((static_cast<std::int64_t>(0)) > (static_cast<std::int64_t>(rcClip.left) - px)) ? (static_cast<std::int64_t>(0)) : (static_cast<std::int64_t>(rcClip.left) - px));
    y0 = (((static_cast<std::int64_t>(0)) > (static_cast<std::int64_t>(rcClip.top) - py)) ? (static_cast<std::int64_t>(0)) : (static_cast<std::int64_t>(rcClip.top) - py));
    x1 = (((static_cast<std::int64_t>(cxDest)) < (static_cast<std::int64_t>(rcClip.right) - px)) ? (static_cast<std::int64_t>(cxDest)) : (static_cast<std::int64_t>(rcClip.right) - px));
    y1 = (((static_cast<std::int64_t>(cyDest)) < (static_cast<std::int64_t>(rcClip.bottom) - py)) ? (static_cast<std::int64_t>(cyDest)) : (static_cast<std::int64_t>(rcClip.bottom) - py));
    for (iy = y0; iy < y1; iy++) {
        sy = yTop + iy * cySrc / cyDest;
        for (ix = x0; ix < x1; ix++) {
            sx = static_cast<std::int64_t>(xSrc) + ix * cxSrc / cxDest;
            crPattern = CrBrush(lpdc, lpdc->lpbrush, px + ix, py + iy);
            crSource = CrSurfacePixel(lpsrc, static_cast<int>(sx), static_cast<int>(sy));
            crDest = CrDCPixel(lpdc, static_cast<int>((px + ix)), static_cast<int>((py + iy)));
            Pixel(lpdc, &rcClip, px + ix, py + iy,
                  CrRop(rop, crPattern, crSource, crDest));
        }
    }
    return cySrc;
}

void CompatDestroyPaint(HWND hwnd) {
    HDC lpdc, lpnext;

    for (lpdc = lpDCs; lpdc; lpdc = lpnext) {
        lpnext = lpdc->lpNext;
        if (lpdc->hwnd == hwnd)
            CloseDC(lpdc);
    }
    hwnd->lpTexture.reset();
    hwnd->lpRenderer.reset();
    hwnd->lpCanvas.reset();
    hwnd->lpComposite.reset();
    hwnd->hdcPaint = nullptr;
    SetUpdate(hwnd, nullptr, 0);
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
        delete lpobj;
    }
    for (i = 0; i < 18; i++) rgStock[i] = COMPATOBJECT{};
    CompatShutdownFonts();
    fStock = 0;
}

SDL_Surface *LpCompatBitmapSurface(HBITMAP hbmp) {
    COMPATOBJECT *lpobj = LpObject(hbmp);
    return lpobj && lpobj->iKind == objBitmap ? lpobj->lpSurface.get() : nullptr;
}

HBITMAP HbmpCompatSurface(SDL_Surface *lpSurface, BOOL fMono) {
    COMPATOBJECT *lpobj;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    if (!lpSurface) return nullptr;
    lpsurfaceOwner.reset(SDL_DuplicateSurface(lpSurface));
    if (!lpsurfaceOwner) return nullptr;
    lpobj = LpNewObject(objBitmap, 0, 0);
    if (!lpobj) return nullptr;
    lpobj->lpSurface.reset(lpsurfaceOwner.release()); lpobj->fMono = fMono;
    return reinterpret_cast<HBITMAP>(lpobj);
}
BOOL win32::DrawIcon(HDC hdc, int x, int y, HICON hicon) {
    HDC lpdc = LpDC(hdc);
    COMPATIMAGE *lp = LpCompatImage(hicon);
    RECT rc;
    int ix, iy, sx, sy;
    std::int64_t dx, dy;
    COLORREF crDst, crSrc;
    if (!lpdc || !lp || !lp->lpXor || !FClip(lpdc, &rc)) return 0;
    for (iy = 0; iy < 32; iy++) for (ix = 0; ix < 32; ix++) {
        dx = static_cast<std::int64_t>(x) - lpdc->ptOrg.x + ix; dy = static_cast<std::int64_t>(y) - lpdc->ptOrg.y + iy;
        if (dx < rc.left || dy < rc.top || dx >= rc.right || dy >= rc.bottom || !FRegionPixel(lpdc, dx, dy)) continue;
        sx = ix * lp->lpXor->w / 32; sy = iy * lp->lpXor->h / 32;
        crDst = CrDCPixel(lpdc, static_cast<int>(dx), static_cast<int>(dy)); crSrc = CrSurfacePixel(lp->lpXor.get(), sx, sy);
        Pixel(lpdc, &rc, dx, dy, (lp->rgbAnd[sy * lp->lpXor->w + sx] ? crDst : 0) ^ crSrc);
    }
    return lpdc->fMemory || lpdc->fScreen || FCompatPresent(lpdc->hwnd);
}
