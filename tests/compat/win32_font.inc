/* Font contracts compare layout/ownership, not platform-specific raster shapes. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static LRESULT CALLBACK FontWindow(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return DefWindowProc(hwnd, msg, wp, lp);
}

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    HDC hdc, hmem;
    HBITMAP hbm, hbmOld;
    HFONT hfont, hfontBig, hfontOld, hfontRotate, hfontStyled;
    LOGFONT lf = {0}, lfRead;
    TEXTMETRIC tm, tmBig, tmStyled;
    SIZE size, sizeBig, sizeI, sizeW, sizeWord;
    RECT rc, rcBefore;
    int x, y, cPixels = 0, height;
    INT rgdx[3] = {20, 20, 20};
    COLORREF cr;
#ifndef _WIN32
    const char *pszSnapshot;
    SDL_Surface *lpSnapshot;
    BITMAPINFO bi = {0};
#endif

    wc.lpfnWndProc = FontWindow; wc.lpszClassName = "StarsFontReference";
    hdc = GetDC(NULL);
    CHECK(hdc && GetDeviceCaps(hdc, BITSPIXEL) >= 16 && GetDeviceCaps(hdc, LOGPIXELSY) > 0);
#ifndef _WIN32
    CHECK(!TextOut(hdc, 0, 0, "desktop", 7) && GetPixel(hdc, 0, 0) == CLR_INVALID);
#endif
    CHECK(GetTextMetrics(hdc, &tm) && ReleaseDC(NULL, hdc));
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Fonts", WS_OVERLAPPEDWINDOW, 0, 0, 320, 240, NULL, NULL, NULL, NULL);
    CHECK(hwnd && (hdc = GetDC(hwnd)));
    hmem = CreateCompatibleDC(hdc); hbm = CreateCompatibleBitmap(hdc, 320, 240);
    CHECK(hmem && hbm && (hbmOld = (HBITMAP)(SelectObject(hmem, hbm))));
    CHECK(GetDeviceCaps(hdc, LOGPIXELSX) > 0 && GetDeviceCaps(hmem, LOGPIXELSY) > 0);
    CHECK(GetDeviceCaps(hmem, PLANES) == 1 && GetDeviceCaps(hmem, BITSPIXEL) >= 16);
    lf.lfHeight = -12; lf.lfWeight = FW_NORMAL; strcpy(lf.lfFaceName, "Arial");
    hfont = CreateFontIndirect(&lf); CHECK(hfont);
    CHECK(GetObject(hfont, sizeof(lfRead), &lfRead) == sizeof(lfRead));
    CHECK(lfRead.lfHeight == lf.lfHeight && !strcmp(lfRead.lfFaceName, "Arial"));
    hfontOld = (HFONT)(SelectObject(hmem, hfont)); CHECK(hfontOld);
    CHECK(GetTextMetrics(hmem, &tm) && tm.tmHeight == tm.tmAscent + tm.tmDescent);
    CHECK(GetTextExtentPoint32(hmem, "iiii", 4, &sizeI) && GetTextExtentPoint32(hmem, "WWWW", 4, &sizeW));
    CHECK(sizeW.cx > sizeI.cx && sizeW.cy == tm.tmHeight && sizeI.cy == tm.tmHeight);
    CHECK(GetTextExtentPoint32(hmem, "Stars!", 6, &size));
    lf.lfHeight = -24; hfontBig = CreateFontIndirect(&lf); CHECK(hfontBig && SelectObject(hmem, hfontBig) == hfont);
    CHECK(GetTextMetrics(hmem, &tmBig) && GetTextExtentPoint32(hmem, "Stars!", 6, &sizeBig));
    CHECK(tmBig.tmHeight > tm.tmHeight && sizeBig.cx > size.cx);
    CHECK(SelectObject(hmem, hfont) == hfontBig && DeleteObject(hfontBig));
    lf.lfHeight = -12; strcpy(lf.lfFaceName, "Arial Bold Italic"); lf.lfUnderline = TRUE;
    hfontStyled = CreateFontIndirect(&lf); CHECK(hfontStyled && SelectObject(hmem, hfontStyled) == hfont);
    CHECK(GetTextMetrics(hmem, &tmStyled) && tmStyled.tmUnderlined);
#ifndef _WIN32
    CHECK(tmStyled.tmItalic && tmStyled.tmWeight >= FW_BOLD);
#endif
    CHECK(SelectObject(hmem, hfont) == hfontStyled && DeleteObject(hfontStyled));
    strcpy(lf.lfFaceName, "Stars Missing Font Family"); lf.lfUnderline = FALSE;
    hfontStyled = CreateFontIndirect(&lf); CHECK(hfontStyled && SelectObject(hmem, hfontStyled) == hfont);
    CHECK(GetTextMetrics(hmem, &tmStyled) && tmStyled.tmHeight > 0);
    CHECK(SelectObject(hmem, hfont) == hfontStyled && DeleteObject(hfontStyled));
    CHECK(SetBkColor(hmem, RGB(10, 20, 30)) != CLR_INVALID && GetBkColor(hmem) == RGB(10, 20, 30));
    SetTextColor(hmem, RGB(255, 255, 255)); SetBkMode(hmem, TRANSPARENT);
    CHECK(PatBlt(hmem, 0, 0, 320, 240, BLACKNESS));
    rc = (RECT){30, 20, 90, 60};
    CHECK(ExtTextOut(hmem, 25, 20, ETO_CLIPPED | ETO_OPAQUE, &rc, "WWW", 3, rgdx));
    CHECK(GetPixel(hmem, 29, 20) == RGB(0, 0, 0) && GetPixel(hmem, 90, 20) == RGB(0, 0, 0));
    CHECK(GetPixel(hmem, 89, 59) == RGB(10, 20, 30));
    rc = (RECT){100, 100, 110, 110};
    CHECK(ExtTextOut(hmem, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL));
    CHECK(GetPixel(hmem, 105, 105) == RGB(10, 20, 30));
    CHECK(TextOut(hmem, 10, 70, "Caf\xc3\xa9 \xe2\x82\xac\xe2\x80\x94", 12));
    CHECK(GetTextExtentPoint32(hmem, "A B", 3, &sizeWord));
    rc = (RECT){10, 120, 11 + sizeWord.cx, 200}; rcBefore = rc;
    height = DrawText(hmem, "A B C", -1, &rc, DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
    CHECK(height == tm.tmHeight * 2 && rc.bottom == rc.top + height && rc.right <= rcBefore.right);
    CHECK(DrawText(hmem, "A B C", -1, &rcBefore, DT_WORDBREAK | DT_NOPREFIX) == height);
    rc = (RECT){10, 160, 200, 200};
    CHECK(DrawText(hmem, "&Fleet && cargo", -1, &rc, DT_SINGLELINE | DT_CALCRECT) == tm.tmHeight);
    CHECK(GetTextExtentPoint32(hmem, "Fleet & cargo", 13, &sizeWord) && rc.right - rc.left == sizeWord.cx);
    rc = (RECT){10, 160, 200, 200};
    CHECK(DrawText(hmem, "&Fleet && cargo", -1, &rc, DT_SINGLELINE) == tm.tmHeight);
#ifndef _WIN32
    CHECK(GetTextExtentPoint32(hmem, "F", 1, &size));
    for (x = 10; x < 10 + size.cx; x++) {
        CHECK(GetPixel(hmem, x, 160 + tm.tmAscent) == RGB(0, 0, 0));
        CHECK(GetPixel(hmem, x, 160 + tm.tmAscent + 1) == RGB(255, 255, 255));
    }
#endif
    rc = (RECT){10, 0, 200, 200};
    CHECK(DrawText(hmem, "one\r\ntwo", -1, &rc, DT_CALCRECT | DT_NOPREFIX) == tm.tmHeight * 2);
    lf.lfUnderline = FALSE; strcpy(lf.lfFaceName, "Arial"); lf.lfEscapement = 3150;
    hfontRotate = CreateFontIndirect(&lf); CHECK(hfontRotate && SelectObject(hmem, hfontRotate) == hfont);
    CHECK(TextOut(hmem, 200, 40, "Stars", 5));
    for (y = 20; y < 100; y++) for (x = 180; x < 280; x++) {
        cr = GetPixel(hmem, x, y);
        if (cr != RGB(0, 0, 0)) cPixels++;
    }
    CHECK(cPixels > 0);
#ifndef _WIN32
    pszSnapshot = getenv("STARS_FONT_SNAPSHOT");
    if (pszSnapshot) {
        CHECK(SelectObject(hmem, hbmOld) == hbm);
        lpSnapshot = SDL_CreateSurface(320, 240, SDL_PIXELFORMAT_BGRA32);
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = 320; bi.bmiHeader.biHeight = -240;
        bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
        CHECK(lpSnapshot && GetDIBits(hmem, hbm, 0, 240, lpSnapshot->pixels, &bi, DIB_RGB_COLORS) == 240);
        CHECK(SDL_SaveBMP(lpSnapshot, pszSnapshot));
        SDL_DestroySurface(lpSnapshot);
        CHECK(SelectObject(hmem, hbm) == hbmOld);
    }
#endif
    CHECK(SelectObject(hmem, hfontOld) == hfontRotate && DeleteObject(hfontRotate) && DeleteObject(hfont));
    CHECK(SelectObject(hmem, hbmOld) == hbm && DeleteObject(hbm) && DeleteDC(hmem));
    CHECK(ReleaseDC(hwnd, hdc) && DestroyWindow(hwnd));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Fonts: PASS (selection, metrics, styles, UTF-8, clipping, spacing, wrapping, rotation)");
    return 0;
}
