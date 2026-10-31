/* Shared native/SDK contracts for DIB conversion and the canonical bitmap resources. */
#include <windows.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
int TestInvalidDibs(void);
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

typedef struct TESTRESOURCE { LPCSTR psz; DWORD cb; } TESTRESOURCE;
static const TESTRESOURCE rgResources[] = {
#include "bitmap_manifest.h"
};
typedef struct TESTINFO { BITMAPINFOHEADER bi; RGBQUAD rgColors[256]; } TESTINFO;

static COLORREF CrDib(const BITMAPINFOHEADER *lpbi, const BYTE *rgb, int x, int y) {
    const RGBQUAD *rgColors = (const RGBQUAD *)(lpbi + 1);
    int cbRow = ((lpbi->biWidth * lpbi->biBitCount + 31) / 32) * 4;
    int iColor;

    rgb += (lpbi->biHeight < 0 ? y : lpbi->biHeight - 1 - y) * cbRow;
    if (lpbi->biBitCount <= 8) {
        iColor = lpbi->biBitCount == 1 ? (rgb[x / 8] >> (7 - x % 8)) & 1 :
                 lpbi->biBitCount == 4 ? (rgb[x / 2] >> (x % 2 == 0 ? 4 : 0)) & 15 : rgb[x];
        return RGB(rgColors[iColor].rgbRed, rgColors[iColor].rgbGreen, rgColors[iColor].rgbBlue);
    }
    rgb += x * (lpbi->biBitCount / 8);
    return RGB(rgb[2], rgb[1], rgb[0]);
}

static int TestResources(HDC hdc) {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hrsrc;
    HGLOBAL hres;
    HBITMAP hbm, hbmOther;
    HDC hmem = CreateCompatibleDC(hdc);
    HGDIOBJ hOld;
    BITMAP bm;
    TESTINFO out;
    BYTE *rgbOut;
    const BITMAPINFOHEADER *lpbi;
    const BYTE *rgb;
    DWORD cColors;
    size_t i;
    int x, y, ix, iy;

    CHECK(hModule && hmem && sizeof(rgResources) / sizeof(rgResources[0]) == 38);
    CHECK(!FindResource(hModule, "missing-bitmap", RT_BITMAP));
    CHECK(!LoadBitmap(hModule, MAKEINTRESOURCE(65535)));
    CHECK(FindResource(hModule, "Screen50Bmp", RT_BITMAP) == FindResource(hModule, "SCREEN50BMP", RT_BITMAP));
    for (i = 0; i < sizeof(rgResources) / sizeof(rgResources[0]); i++) {
        hrsrc = FindResource(hModule, rgResources[i].psz, RT_BITMAP);
        CHECK(hrsrc && SizeofResource(hModule, hrsrc) == rgResources[i].cb);
        hres = LoadResource(hModule, hrsrc);
        lpbi = (const BITMAPINFOHEADER*)(LockResource(hres));
        CHECK(lpbi && lpbi->biSize == sizeof(*lpbi) && lpbi->biCompression == BI_RGB);
        cColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1u << lpbi->biBitCount;
        rgb = (const BYTE *)(lpbi + 1) + cColors * sizeof(RGBQUAD);
        hbm = LoadBitmap(hModule, rgResources[i].psz);
        hbmOther = LoadBitmap(hModule, rgResources[i].psz);
        CHECK(hbm && hbmOther && hbm != hbmOther);
        CHECK(GetObject(hbm, sizeof(bm), &bm) == sizeof(bm));
        CHECK(bm.bmWidth == lpbi->biWidth && bm.bmHeight == lpbi->biHeight);
        hOld = SelectObject(hmem, hbm);
        CHECK(hOld);
        for (iy = 0; iy < 3; iy++) {
            y = iy * (bm.bmHeight - 1) / 2;
            for (ix = 0; ix < 3; ix++) {
                x = ix * (bm.bmWidth - 1) / 2;
                CHECK(GetPixel(hmem, x, y) == CrDib(lpbi, rgb, x, y));
            }
        }
        CHECK(SelectObject(hmem, hOld) == hbm);
        memset(&out, 0, sizeof(out));
        out.bi.biSize = sizeof(out.bi); out.bi.biPlanes = 1; out.bi.biBitCount = 32;
        out.bi.biWidth = bm.bmWidth; out.bi.biHeight = -bm.bmHeight;
        rgbOut = (BYTE*)(malloc((size_t)bm.bmWidth * bm.bmHeight * 4));
        CHECK(rgbOut && GetDIBits(hdc, hbm, 0, bm.bmHeight, rgbOut, (BITMAPINFO *)&out, DIB_RGB_COLORS) == bm.bmHeight);
        for (y = 0; y < bm.bmHeight; y++) for (x = 0; x < bm.bmWidth; x++)
            CHECK(CrDib(&out.bi, rgbOut, x, y) == CrDib(lpbi, rgb, x, y));
        free(rgbOut);
        CHECK(DeleteObject(hbm) && DeleteObject(hbmOther));
        CHECK(!FreeResource(hres) && LockResource(hres) == lpbi);
    }
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!FindResource(NULL, MAKEINTRESOURCE(249), MAKEINTRESOURCE(3)));
    CHECK(!SizeofResource(NULL, (HRSRC)(UINT_PTR)123));
    CHECK(!LoadResource(NULL, (HRSRC)(UINT_PTR)123) && !LockResource((HGLOBAL)(UINT_PTR)123));
    CHECK(!GetModuleHandle("other-module") && !LoadBitmap((HINSTANCE)(UINT_PTR)123, "CargoBmp"));
#endif
    CHECK(DeleteDC(hmem));
    return 0;
}

static int TestDibs(HDC hdc) {
    TESTINFO info = {0}, out = {0};
    BYTE rgb[24], rgbOut[80];
    HDC hmem = CreateCompatibleDC(hdc);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, 6, 4);
    HGDIOBJ hOld;
    int cBits, i, y, x, cbRow;
    static const int rgBits[] = {1, 4, 8, 24, 32};
    static const COLORREF rgcr[] = {0, RGB(255, 255, 255)};

    CHECK(hmem && hbm);
    hOld = SelectObject(hmem, hbm);
    CHECK(hOld);
    for (i = 0; i < 5; i++) {
        cBits = rgBits[i];
        memset(&info, 0, sizeof(info));
        info.bi.biSize = sizeof(info.bi); info.bi.biWidth = 3; info.bi.biHeight = 2;
        info.bi.biPlanes = 1; info.bi.biBitCount = (WORD)cBits;
        info.rgColors[1] = (RGBQUAD){255, 255, 255, 0};
        cbRow = ((3 * cBits + 31) / 32) * 4;
        memset(rgb, 0, sizeof(rgb));
        // Bottom row black/white/black, top row white/black/white, with DWORD padding.
        for (y = 0; y < 2; y++) for (x = 0; x < 3; x++) {
            if ((x + y) % 2 == 0) continue;
            if (cBits == 1) rgb[y * cbRow] |= (BYTE)(1 << (7 - x));
            else if (cBits == 4) rgb[y * cbRow + x / 2] |= (BYTE)(1 << (x % 2 == 0 ? 4 : 0));
            else if (cBits == 8) rgb[y * cbRow + x] = 1;
            else memset(rgb + y * cbRow + x * (cBits / 8), 255, 3);
        }
        CHECK(StretchDIBits(hmem, 0, 0, 6, 4, 0, 0, 3, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY) > 0);
        for (y = 0; y < 4; y++) for (x = 0; x < 6; x++)
            CHECK(GetPixel(hmem, x, y) == rgcr[(x / 2 + 1 - y / 2) % 2]);
        info.bi.biHeight = -2;
        CHECK(StretchDIBits(hmem, 0, 0, 3, 2, 0, 0, 3, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY) > 0);
        CHECK(GetPixel(hmem, 0, 0) == rgcr[0] && GetPixel(hmem, 0, 1) == rgcr[1]);
        info.bi.biHeight = 2;
        CHECK(StretchDIBits(hmem, 0, 0, 3, 1, 0, 0, 3, 1, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY) > 0);
        CHECK(GetPixel(hmem, 0, 0) == rgcr[0]);
    }
    info.bi.biBitCount = 8; info.bi.biClrUsed = 2;
    info.rgColors[1] = (RGBQUAD){33, 22, 11, 0};
    memset(rgb, 1, sizeof(rgb));
    CHECK(StretchDIBits(hmem, 0, 0, 3, 2, 0, 0, 3, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY) > 0);
    CHECK(GetPixel(hmem, 1, 1) == RGB(11, 22, 33));
    info.bi.biClrUsed = 0;
    // Distinct RGB components catch BGR decoding; BI_RGB's fourth byte is unused.
    info.bi.biBitCount = 32; info.bi.biHeight = 2;
    memset(rgb, 0, sizeof(rgb)); rgb[0] = 33; rgb[1] = 22; rgb[2] = 11; rgb[3] = 1;
    CHECK(PatBlt(hmem, 0, 0, 6, 4, WHITENESS));
    CHECK(StretchDIBits(hmem, -1, -1, 6, 4, 0, 0, 3, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY) > 0);
    CHECK(GetPixel(hmem, 0, 1) == RGB(11, 22, 33) && GetPixel(hmem, 5, 3) == RGB(255, 255, 255));
    CHECK(StretchDIBits(hmem, 0, 0, 3, 1, 0, 0, 3, 1, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCINVERT) > 0);
    CHECK(GetPixel(hmem, 0, 0) == RGB(11, 22, 33));
    CHECK(SelectObject(hmem, hOld) == hbm);
    out.bi.biSize = sizeof(out.bi);
    CHECK(GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *)&out, DIB_RGB_COLORS) > 0);
    CHECK(out.bi.biWidth == 6 && out.bi.biHeight == 4 && out.bi.biBitCount >= 24);
    out.bi.biBitCount = 24; out.bi.biHeight = -4; out.bi.biCompression = BI_RGB; out.bi.biSizeImage = 0;
    memset(rgbOut, 0xa5, sizeof(rgbOut));
    CHECK(GetDIBits(hdc, hbm, 0, 2, rgbOut, (BITMAPINFO *)&out, DIB_RGB_COLORS) == 2);
    CHECK(CrDib(&out.bi, rgbOut, 0, 0) == RGB(11, 22, 33));
    CHECK(rgbOut[40] == 0xa5); // Only two padded 20-byte rows were requested.
    out.bi.biHeight = 4;
    CHECK(GetDIBits(hdc, hbm, 2, 1, rgbOut, (BITMAPINFO *)&out, DIB_RGB_COLORS) == 1);
    CHECK(rgbOut[0] == 33 && rgbOut[1] == 22 && rgbOut[2] == 11);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(GetDIBits(hdc, hbm, 4, 1, rgbOut, (BITMAPINFO *)&out, DIB_RGB_COLORS) == 0);
    info.bi.biCompression = BI_RLE8;
    CHECK(!StretchDIBits(hmem, 0, 0, 3, 2, 0, 0, 3, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS, SRCCOPY));
#endif
    CHECK(DeleteObject(hbm) && DeleteDC(hmem));
    return 0;
}

static int TestMonoReadback(HDC hdc) {
    HDC hmem = CreateCompatibleDC(NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hmem, 3, 2);
    HGDIOBJ hOld;
    BITMAPINFOHEADER bi = {0};
    TESTINFO info = {0};
    BYTE rgb[8] = {0};

    CHECK(hmem && hbm);
    hOld = SelectObject(hmem, hbm);
    CHECK(hOld && PatBlt(hmem, 0, 0, 3, 2, BLACKNESS));
    CHECK(PatBlt(hmem, 0, 0, 1, 1, WHITENESS) && PatBlt(hmem, 2, 0, 1, 1, WHITENESS));
    CHECK(PatBlt(hmem, 1, 1, 1, 1, WHITENESS));
    CHECK(SelectObject(hmem, hOld) == hbm);
    bi.biSize = sizeof(bi);
    CHECK(GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *)&bi, DIB_RGB_COLORS) > 0);
    CHECK(bi.biWidth == 3 && bi.biHeight == 2 && bi.biBitCount == 1);
    info.bi = bi; info.bi.biHeight = -2;
    CHECK(GetDIBits(hdc, hbm, 0, 2, rgb, (BITMAPINFO *)&info, DIB_RGB_COLORS) == 2);
    CHECK(rgb[0] == 0xa0 && rgb[4] == 0x40);
    CHECK(info.rgColors[0].rgbRed == 0 && info.rgColors[1].rgbRed == 255);
    CHECK(DeleteObject(hbm) && DeleteDC(hmem));
    return 0;
}

static LRESULT CALLBACK DibProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    HDC hdc;
    int result;

    wc.lpfnWndProc = DibProc; wc.lpszClassName = "StarsDibReference";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Stars DIB contracts", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        20, 20, 200, 160, NULL, NULL, NULL, NULL);
    CHECK(hwnd && (hdc = GetDC(hwnd)));
    result = TestResources(hdc);
    if (result == 0) result = TestDibs(hdc);
    if (result == 0) result = TestMonoReadback(hdc);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    if (result == 0) result = TestInvalidDibs();
#endif
    CHECK(ReleaseDC(hwnd, hdc) && DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    if (result == 0) puts("DIBs/resources: PASS (38 assets, palettes, orientation, stretch, clipping, readback)");
    return result;
}
