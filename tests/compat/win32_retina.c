/* Higher-density glyphs must preserve logical metrics, spacing and clipping. */
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static BOOL FDetail(SDL_Surface *lp, int x, int y, int cx, int cy) {
    int ix, iy;
    BYTE *rgb;
    for (iy = y; iy < y + cy; iy += 2) for (ix = x; ix < x + cx; ix += 2) {
        rgb = (BYTE *)lp->pixels + (size_t)iy * lp->pitch + ix * 4;
        if (memcmp(rgb, rgb + 4, 4) || memcmp(rgb, rgb + lp->pitch, 4) || memcmp(rgb, rgb + lp->pitch + 4, 4)) return TRUE;
    }
    return FALSE;
}

int main(void) {
    HDC hdc;
    HBITMAP hbmp, hbmpOld;
    HFONT hfont, hfontOld;
    LOGFONTA lf = {0};
    TEXTMETRICA tmBefore, tmAfter;
    SIZE sizeBefore, sizeAfter;
    RECT rc = {0, 0, 200, 60}, rcClip = {5, 30, 20, 50};
    SDL_Surface *lp;
    INT rgdx[2] = {30, 30};
    BYTE *rgb;
    int x, y;

    hdc = CreateCompatibleDC(NULL);
    hbmp = CreateCompatibleBitmap(hdc, 400, 120);
    CHECK(hdc && hbmp);
    hbmpOld = SelectObject(hdc, hbmp);
    lf.lfHeight = -14; strcpy(lf.lfFaceName, "Arial");
    hfont = CreateFontIndirect(&lf); CHECK(hfont);
    hfontOld = SelectObject(hdc, hfont);
    CHECK(GetTextMetrics(hdc, &tmBefore) && GetTextExtentPoint32(hdc, "Retina 2", 8, &sizeBefore));
    CHECK(FCompatSetDCScale(hdc, 2) && FillRect(hdc, &rc, GetStockObject(WHITE_BRUSH)));
    CHECK(TextOut(hdc, 5, 3, "Retina 2", 8));
    CHECK(GetTextMetrics(hdc, &tmAfter) && GetTextExtentPoint32(hdc, "Retina 2", 8, &sizeAfter));
    CHECK(!memcmp(&tmBefore, &tmAfter, sizeof(tmBefore)) && !memcmp(&sizeBefore, &sizeAfter, sizeof(sizeBefore)));
    lp = LpCompatBitmapSurface(hbmp);
    CHECK(lp && FDetail(lp, 10, 6, sizeBefore.cx * 2, tmBefore.tmHeight * 2));
    CHECK(BitBlt(hdc, 100, 3, sizeBefore.cx, tmBefore.tmHeight, hdc, 5, 3, SRCCOPY));
    CHECK(PatBlt(hdc, 5, 3, sizeBefore.cx, tmBefore.tmHeight, DSTINVERT) &&
          PatBlt(hdc, 5, 3, sizeBefore.cx, tmBefore.tmHeight, DSTINVERT));
    for (y = 6; y < 6 + tmBefore.tmHeight * 2; y++)
        CHECK(!memcmp((BYTE *)lp->pixels + (size_t)y * lp->pitch + 10 * 4,
                      (BYTE *)lp->pixels + (size_t)y * lp->pitch + 200 * 4, (size_t)sizeBefore.cx * 2 * 4));
    CHECK(ExtTextOut(hdc, 5, 30, ETO_CLIPPED, &rcClip, "WW", 2, rgdx));
    for (y = 60; y < 100; y++) for (x = 40; x < 400; x++) {
        rgb = (BYTE *)lp->pixels + (size_t)y * lp->pitch + x * 4;
        CHECK(rgb[0] == 255 && rgb[1] == 255 && rgb[2] == 255);
    }
    CHECK(ExtTextOut(hdc, 40, 30, 0, NULL, "AA", 2, rgdx));
    for (y = 60; y < 60 + tmBefore.tmHeight * 2; y++)
        CHECK(!memcmp((BYTE *)lp->pixels + (size_t)y * lp->pitch + 80 * 4,
                      (BYTE *)lp->pixels + (size_t)y * lp->pitch + 140 * 4, 28 * 4));
    SelectObject(hdc, hfontOld); SelectObject(hdc, hbmpOld);
    CHECK(DeleteObject(hfont) && DeleteObject(hbmp) && DeleteDC(hdc));
    StarsCompatShutdown();
    puts("Retina glyph detail, unchanged logical metrics, clipping and custom advances passed");
    return 0;
}
