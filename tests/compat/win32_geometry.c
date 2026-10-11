/* Geometry contracts shared by the SDK and the local rasterizer. */
#include <windows.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <stars_compat.h>
#endif
#include "fixtures/ellipse_sdk.h"
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static uint32_t DwPixels(HDC hdc) {
    uint32_t dw = 2166136261u;
    int x, y;
    for (y = 0; y < 40; y++) for (x = 0; x < 40; x++) dw = (dw ^ GetPixel(hdc, x, y)) * 16777619u;
    return dw;
}

int main(int argc, char **argv) {
    HDC hs, hdc;
    HBITMAP hbm, hbmOld;
    HPEN hpen, hpenOld, hdot;
    HBRUSH hbrush, hbrushOld;
    BOOL fDump = argc == 2 && strcmp(argv[1], "--dump") == 0;
    int x, y, cx, cy, i, iStyle, iMode;
    size_t j;
    uint32_t dw;
    COLORREF cr;
    POINT pt;

    hs = GetDC(NULL); hdc = CreateCompatibleDC(hs); hbm = CreateCompatibleBitmap(hs, 40, 40);
    CHECK(hs && hdc && hbm && (hbmOld = SelectObject(hdc, hbm)));
    hpen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); hbrush = CreateSolidBrush(RGB(0, 0, 255));
    CHECK(hpen && hbrush && (hpenOld = SelectObject(hdc, hpen)) && (hbrushOld = SelectObject(hdc, hbrush)));
    if (argc == 5 && strcmp(argv[1], "--pixels") == 0) {
        cx = atoi(argv[2]); cy = atoi(argv[3]); i = atoi(argv[4]);
        CHECK(SelectObject(hdc, i == 1 ? GetStockObject(NULL_PEN) : hpen));
        CHECK(SelectObject(hdc, i == 2 ? GetStockObject(NULL_BRUSH) : hbrush));
        CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS) && Ellipse(hdc, 3, 3, 3 + cx, 3 + cy));
        for (y = 2; y < 4 + cy; y++) {
            for (x = 2; x < 4 + cx; x++) { cr = GetPixel(hdc, x, y); putchar(cr == RGB(255, 0, 0) ? 'R' : cr == RGB(0, 0, 255) ? 'B' : '.'); }
            putchar('\n');
        }
        fDump = TRUE;
    } else if (fDump) {
        for (i = 0; i < 3; i++) {
            CHECK(SelectObject(hdc, i == 1 ? GetStockObject(NULL_PEN) : hpen));
            CHECK(SelectObject(hdc, i == 2 ? GetStockObject(NULL_BRUSH) : hbrush));
            for (cy = 1; cy <= 24; cy++) for (cx = 1; cx <= 24; cx++) {
                CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
                CHECK(Ellipse(hdc, 3, 3, 3 + cx, 3 + cy));
                printf("%d,%d,%d,%08x\n", i, cx, cy, DwPixels(hdc));

            }
        }
    } else {
        for (iStyle = PS_DASH; iStyle <= PS_DASHDOTDOT; iStyle++) {
            hdot = CreatePen(iStyle, 1, RGB(255, 0, 0)); CHECK(hdot && SelectObject(hdc, hdot));
            CHECK(SetBkMode(hdc, TRANSPARENT));
            CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
            CHECK(MoveToEx(hdc, 2, 2, NULL) && LineTo(hdc, 38, 2));
            // DOT is the mass-driver path pen: 3 foreground, 3 gap pixels.
            if (iStyle == PS_DOT) for (x = 2; x < 38; x++)
                CHECK(GetPixel(hdc, x, 2) == ((x - 2) % 6 < 3 ? RGB(255, 0, 0) : 0));
            CHECK(SelectObject(hdc, hpen) == hdot && DeleteObject(hdot));
        }
        hdot = CreatePen(PS_DOT, 0, RGB(255, 0, 0)); CHECK(hdot && SelectObject(hdc, hdot));
        CHECK(SetBkMode(hdc, OPAQUE) && SetBkColor(hdc, RGB(0, 0, 255)) != CLR_INVALID);
        CHECK(MoveToEx(hdc, 2, 4, NULL) && LineTo(hdc, 38, 4));
        for (x = 2; x < 38; x++) CHECK(GetPixel(hdc, x, 4) == ((x - 2) % 6 < 3 ? RGB(255, 0, 0) : RGB(0, 0, 255)));
        CHECK(SetBkMode(hdc, TRANSPARENT));
        CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
        CHECK(IntersectClipRect(hdc, 8, 0, 40, 40) == SIMPLEREGION);
        CHECK(MoveToEx(hdc, 1, 6, NULL) && LineTo(hdc, 38, 6));
        CHECK(SelectClipRgn(hdc, NULL) == SIMPLEREGION);
        for (x = 0; x < 38; x++) CHECK(GetPixel(hdc, x, 6) == (x >= 8 && (x - 1) % 6 < 3 ? RGB(255, 0, 0) : 0));
        CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
        CHECK(MoveToEx(hdc, -5, 8, NULL) && LineTo(hdc, 35, 8));
        for (x = 0; x < 35; x++) CHECK(GetPixel(hdc, x, 8) == ((x + 5) % 6 < 3 ? RGB(255, 0, 0) : 0));
        CHECK(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
        CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS) && Rectangle(hdc, 3, 3, 16, 18));
        for (i = 0; i < 52; i++) {
            if (i < 12) { x = 15 - i; y = 3; }
            else if (i < 26) { x = 3; y = 3 + i - 12; }
            else if (i < 38) { x = 3 + i - 26; y = 17; }
            else { x = 15; y = 17 - (i - 38); }
            CHECK(GetPixel(hdc, x, y) == (i % 6 < 3 ? RGB(255, 0, 0) : 0));
        }
        CHECK(SelectObject(hdc, hpen) == hdot && DeleteObject(hdot));
        for (iMode = 0; iMode < 2; iMode++) {
            CHECK(SelectObject(hdc, iMode ? GetStockObject(NULL_BRUSH) : hbrush));
            CHECK(SetROP2(hdc, R2_COPYPEN));
            CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
            CHECK(MoveToEx(hdc, 1, 1, NULL));
            CHECK(Ellipse(hdc, 4, 6, 28, 26));
            CHECK(MoveToEx(hdc, 2, 2, &pt) && pt.x == 1 && pt.y == 1);
            CHECK(GetPixel(hdc, 16, 16) == (iMode ? 0 : RGB(0, 0, 255)));
            for (y = 0; y < 40; y++) for (x = 0; x < 40; x++) {
                cr = GetPixel(hdc, x, y);
                CHECK(cr == 0 || cr == RGB(255, 0, 0) || (!iMode && cr == RGB(0, 0, 255)));
                if (x < 4 || x >= 28 || y < 6 || y >= 26) CHECK(cr == 0);
            }
            dw = DwPixels(hdc);
            CHECK(SetWindowOrgEx(hdc, 10, 20, NULL));
            CHECK(PatBlt(hdc, 10, 20, 40, 40, BLACKNESS));
            CHECK(Ellipse(hdc, 14, 26, 38, 46));
            CHECK(SetWindowOrgEx(hdc, 0, 0, NULL) && DwPixels(hdc) == dw);
            CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS) && SetROP2(hdc, R2_XORPEN));
            CHECK(Ellipse(hdc, 4, 6, 28, 26) && Ellipse(hdc, 4, 6, 28, 26));
            for (y = 0; y < 40; y++) for (x = 0; x < 40; x++) CHECK(GetPixel(hdc, x, y) == 0);
        }
        CHECK(SetROP2(hdc, R2_COPYPEN) && SelectObject(hdc, hbrush));
        CHECK(Ellipse(hdc, 0, 0, 0, 10));
        CHECK(Ellipse(hdc, -20, -10, 20, 30));
        for (j = 0; j < sizeof(rgEllipseSDK) / sizeof(rgEllipseSDK[0]); j++) {
            i = rgEllipseSDK[j].iMode; cx = rgEllipseSDK[j].cx; cy = rgEllipseSDK[j].cy;
            CHECK(SelectObject(hdc, i == 1 ? GetStockObject(NULL_PEN) : hpen));
            CHECK(SelectObject(hdc, i == 2 ? GetStockObject(NULL_BRUSH) : hbrush));
            CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS) && Ellipse(hdc, 3, 3, 3 + cx, 3 + cy));
            if (!rgEllipseSDK[j].fDriverArtifact) {
                dw = DwPixels(hdc);
                if (dw != rgEllipseSDK[j].dw) fprintf(stderr, "ellipse mode %d, %dx%d: %08x != %08x\n", i, cx, cy, dw, rgEllipseSDK[j].dw);
                CHECK(dw == rgEllipseSDK[j].dw);
            }
#ifndef _WIN32
            // Wine's narrow-fill artifacts may extend beyond the bounding box.
            // They are deliberately not the native rasterization contract.
            for (y = 0; y < 40; y++) for (x = 0; x < 40; x++)
                if (x < 3 || x >= 3 + cx || y < 3 || y >= 3 + cy) CHECK(GetPixel(hdc, x, y) == 0);
#endif
        }
    }
#ifndef _WIN32
    CHECK(SelectObject(hdc, hbrush));
    CHECK(!Ellipse(hdc, INT_MIN, INT_MIN, INT_MAX, INT_MAX));
    CHECK(Ellipse(hdc, -524288, -524288, 524288, 524288));
    CHECK(GetPixel(hdc, 0, 0) == RGB(0, 0, 255) && GetPixel(hdc, 39, 39) == RGB(0, 0, 255));
#endif
    CHECK(SelectObject(hdc, hpenOld) && SelectObject(hdc, hbrushOld));
    CHECK(DeleteObject(hpen) && DeleteObject(hbrush));
    CHECK(SelectObject(hdc, hbmOld) && DeleteObject(hbm) && DeleteDC(hdc) && ReleaseDC(NULL, hs));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    if (!fDump) puts("Win32 geometry contracts passed");
    return 0;
}
