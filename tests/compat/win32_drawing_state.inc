/* Shared SDK/local tests: translation and boolean operations use exact pixels. */
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static COLORREF CrExpected(int iRop, COLORREF crPen, COLORREF crDest) {
    COLORREF cr = 0;
    int i;
    for (i = 0; i < 4; i++)
        if ((iRop - 1) & (1 << i)) cr |= (i & 2 ? crPen : ~crPen) & (i & 1 ? crDest : ~crDest);
    return cr & 0xffffff;
}

int main(void) {
    HDC hscreen, hdc, hsrc, hmonoDC;
    HBITMAP hbm, hbmOld, hsrcbm, hsrcOld, hpatbm;
    HBRUSH hbrush, hbrushOld, hpattern, hmono;
    HPEN hpen, hpenOld;
    HRGN hrgn;
    RECT rc = {0, 0, 40, 40}, rcFrame;
    POINT pt;
    COLORREF crPen = RGB(0x35, 0xa7, 0x4c), crDest = RGB(0xf2, 0x59, 0x81), cr;
    struct { BITMAPINFOHEADER header; RGBQUAD colors[2]; } bi;
    BYTE rgbDib[16] = {0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0};
    int i, x, y;

    hscreen = GetDC(NULL); CHECK(hscreen);
    hdc = CreateCompatibleDC(hscreen); hsrc = CreateCompatibleDC(hscreen);
    hbm = CreateCompatibleBitmap(hscreen, 40, 40); hsrcbm = CreateCompatibleBitmap(hscreen, 40, 40);
    CHECK(hdc && hsrc && hbm && hsrcbm);
    hbmOld = (HBITMAP)(SelectObject(hdc, hbm)); hsrcOld = (HBITMAP)(SelectObject(hsrc, hsrcbm));
    hpen = CreatePen(PS_SOLID, 1, crPen); hbrush = CreateSolidBrush(crPen);
    CHECK(hpen && hbrush);
    hpenOld = (HPEN)(SelectObject(hdc, hpen)); hbrushOld = (HBRUSH)(SelectObject(hdc, hbrush));
    CHECK(GetROP2(hdc) == R2_COPYPEN && GetROP2(hsrc) == R2_COPYPEN);
    for (i = 1; i <= 16; i++) {
        CHECK(SetROP2(hdc, R2_COPYPEN));
        CHECK(SetPixel(hdc, 5, 5, crDest) == crDest && SetPixel(hdc, 6, 6, crDest) == crDest);
        CHECK(SetPixel(hdc, 7, 6, crDest) == crDest);
        CHECK(SetROP2(hdc, i) && GetROP2(hdc) == i && GetROP2(hsrc) == R2_COPYPEN);
        CHECK(MoveToEx(hdc, 5, 5, NULL) && LineTo(hdc, 6, 5));
        cr = GetPixel(hdc, 5, 5);
        if (cr != CrExpected(i, crPen, crDest)) fprintf(stderr, "ROP2 %d: got %06lx, expected %06lx\n", i, (unsigned long)cr, (unsigned long)CrExpected(i, crPen, crDest));
        CHECK(cr == CrExpected(i, crPen, crDest));
        CHECK(Rectangle(hdc, 6, 6, 9, 9));
        CHECK(GetPixel(hdc, 6, 6) == CrExpected(i, crPen, crDest));
        CHECK(GetPixel(hdc, 7, 6) == CrExpected(i, crPen, crDest));
        CHECK(SetROP2(hdc, R2_COPYPEN) == i && SetPixel(hdc, 1, 1, crDest) == crDest);
        CHECK(SetROP2(hdc, i) == R2_COPYPEN && SetPixel(hdc, 1, 1, crPen) == crPen);
        CHECK(GetPixel(hdc, 1, 1) == CrExpected(i, crPen, crDest));
    }
    CHECK(!SetROP2(hdc, 0) && !SetROP2(hdc, 17) && GetROP2(hdc) == R2_WHITE);
    CHECK(SetROP2(hdc, R2_XORPEN) == R2_WHITE);
    CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS));
    CHECK(Rectangle(hdc, 2, 2, 12, 12) && Rectangle(hdc, 2, 2, 12, 12));
    for (y = 0; y < 16; y++) for (x = 0; x < 16; x++) CHECK(GetPixel(hdc, x, y) == 0);
    CHECK(SetROP2(hdc, R2_COPYPEN) == R2_XORPEN);
    CHECK(SetWindowOrgEx(hdc, 100, -200, &pt) && pt.x == 0 && pt.y == 0);
    CHECK(SetWindowOrgEx(hdc, 10, 20, &pt) && pt.x == 100 && pt.y == -200);
    rcFrame = (RECT){12, 22, 18, 28};
    CHECK(FrameRect(hdc, &rcFrame, hbrush));
    CHECK(SetPixel(hdc, 20, 30, crDest) == crDest);
    CHECK(MoveToEx(hdc, 10, 20, &pt) && MoveToEx(hdc, 11, 21, &pt) && pt.x == 10 && pt.y == 20);
    CHECK(LineTo(hdc, 14, 21));
    CHECK(SetWindowOrgEx(hdc, 0, 0, NULL));
    CHECK(GetPixel(hdc, 10, 10) == crDest && GetPixel(hdc, 2, 2) == crPen);
    CHECK(GetPixel(hdc, 3, 3) == 0 && GetPixel(hdc, 7, 7) == crPen);
    CHECK(GetPixel(hdc, 1, 1) == crPen && GetPixel(hdc, 4, 1) == 0);
    // Region handles remain in device units; rectangle clipping uses logical units.
    hrgn = CreateRectRgn(0, 0, 4, 4); CHECK(hrgn);
    CHECK(SelectClipRgn(hdc, hrgn) == SIMPLEREGION && DeleteObject(hrgn));
    CHECK(SetWindowOrgEx(hdc, 10, 20, NULL));
    CHECK(SetPixel(hdc, 12, 22, crDest) == crDest);
#ifndef _WIN32
    CHECK(SetPixel(hdc, 16, 26, crDest) == CLR_INVALID);
#endif
    CHECK(IntersectClipRect(hdc, 11, 21, 13, 23) == SIMPLEREGION);
    CHECK(SelectClipRgn(hdc, NULL) == SIMPLEREGION);
    // Source and destination origins differ; overlap still uses captured source.
    CHECK(SetWindowOrgEx(hsrc, -50, -70, NULL));
    CHECK(SetPixel(hsrc, -48, -67, crPen) == crPen);
    CHECK(BitBlt(hdc, 14, 25, 1, 1, hsrc, -48, -67, SRCCOPY));
    CHECK(GetPixel(hdc, 14, 25) == crPen);
    CHECK(SetWindowOrgEx(hsrc, 0, 0, NULL) && SetWindowOrgEx(hdc, 0, 0, NULL));
    memset(&bi, 0, sizeof(bi)); bi.header.biSize = sizeof(bi.header); bi.header.biWidth = 2;
    bi.header.biHeight = -2; bi.header.biPlanes = 1; bi.header.biBitCount = 32;
    CHECK(SetWindowOrgEx(hdc, 10, 20, NULL));
    CHECK(StretchDIBits(hdc, 30, 40, 2, 2, 0, 0, 2, 2, rgbDib, (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY) == 2);
    CHECK(GetPixel(hdc, 30, 40) == RGB(255, 0, 0) && GetPixel(hdc, 31, 41) == RGB(255, 255, 255));
    CHECK(SetWindowOrgEx(hdc, 0, 0, NULL));
    // Opaque text rectangles and DrawText clipping follow the origin too.
    CHECK(SetBkColor(hdc, crDest) != CLR_INVALID && SetWindowOrgEx(hdc, 10, 20, NULL));
    rcFrame = (RECT){30, 40, 34, 44};
    CHECK(ExtTextOutA(hdc, 30, 40, ETO_OPAQUE, &rcFrame, "", 0, NULL));
    CHECK(SetWindowOrgEx(hdc, 0, 0, NULL) && GetPixel(hdc, 20, 20) == crDest);
    CHECK(PatBlt(hdc, 0, 0, 40, 40, BLACKNESS) && PatBlt(hsrc, 0, 0, 40, 40, BLACKNESS));
    CHECK(SetBkColor(hsrc, crDest) != CLR_INVALID && SetTextColor(hdc, crPen) != CLR_INVALID);
    CHECK(SetTextColor(hsrc, crPen) != CLR_INVALID && SetWindowOrgEx(hdc, 10, 20, NULL));
    CHECK(TextOutA(hsrc, 2, 2, "WW", 2) && TextOutA(hdc, 12, 22, "WW", 2));
    rcFrame = (RECT){2, 20, 20, 38};
    CHECK(DrawTextA(hsrc, "W", 1, &rcFrame, DT_SINGLELINE));
    rcFrame = (RECT){12, 40, 30, 58};
    CHECK(DrawTextA(hdc, "W", 1, &rcFrame, DT_SINGLELINE));
    CHECK(SetWindowOrgEx(hdc, 0, 0, NULL));
    for (y = 0; y < 40; y++) for (x = 0; x < 40; x++) CHECK(GetPixel(hdc, x, y) == GetPixel(hsrc, x, y));
    // Pattern brush owns a copy, so deleting the source bitmap is safe.
    hpatbm = CreateCompatibleBitmap(hscreen, 2, 2); CHECK(hpatbm);
    CHECK(SelectObject(hsrc, hpatbm) == hsrcbm);
    CHECK(StretchDIBits(hsrc, 0, 0, 2, 2, 0, 0, 2, 2, rgbDib, (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY) == 2);
    hpattern = CreatePatternBrush(hpatbm); CHECK(hpattern);
    CHECK(SelectObject(hsrc, hsrcbm) == hpatbm && DeleteObject(hpatbm));
    CHECK(SelectObject(hdc, hpattern) == hbrush);
    CHECK(SetBrushOrgEx(hdc, -1, 1, &pt) && pt.x == 0 && pt.y == 0);
    CHECK(PatBlt(hdc, 0, 0, 8, 8, PATCOPY));
    for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
        i = ((y + 1) % 2) * 2 + (x + 1) % 2;
        cr = i == 0 ? RGB(255, 0, 0) : i == 1 ? RGB(0, 255, 0) : i == 2 ? RGB(0, 0, 255) : RGB(255, 255, 255);
        CHECK(GetPixel(hdc, x, y) == cr);
    }
    CHECK(SetBrushOrgEx(hdc, 0, 0, &pt) && pt.x == -1 && pt.y == 1);
    hmonoDC = CreateCompatibleDC(NULL); CHECK(hmonoDC);
    hpatbm = CreateCompatibleBitmap(hmonoDC, 2, 2); CHECK(hpatbm);
    CHECK(SelectObject(hmonoDC, hpatbm));
    CHECK(PatBlt(hmonoDC, 0, 0, 2, 2, BLACKNESS));
    CHECK(SetPixel(hmonoDC, 0, 0, RGB(255, 255, 255)) != CLR_INVALID);
    CHECK(SetPixel(hmonoDC, 1, 1, RGB(255, 255, 255)) != CLR_INVALID);
    hmono = CreatePatternBrush(hpatbm); CHECK(hmono);
    CHECK(DeleteDC(hmonoDC) && DeleteObject(hpatbm));
    CHECK(SelectObject(hdc, hmono) == hpattern);
    CHECK(SetTextColor(hdc, crPen) != CLR_INVALID && SetBkColor(hdc, crDest) != CLR_INVALID);
    CHECK(FillRect(hdc, &rc, hmono));
    CHECK(GetPixel(hdc, 0, 0) == crDest && GetPixel(hdc, 1, 0) == crPen);
    CHECK(GetPixel(hdc, 0, 1) == crPen && GetPixel(hdc, 1, 1) == crDest);
#ifndef _WIN32
    CHECK(SetWindowOrgEx(hdc, INT_MIN, INT_MAX, NULL));
    CHECK(SetPixel(hdc, INT_MIN, INT_MAX, crPen) == crPen);
    CHECK(SetPixel(hdc, INT_MAX, INT_MIN, crPen) == CLR_INVALID);
#endif
    CHECK(SelectObject(hdc, hpenOld) == hpen && SelectObject(hdc, hbrushOld) == hmono);
    CHECK(DeleteObject(hpen) && DeleteObject(hbrush) && DeleteObject(hpattern) && DeleteObject(hmono));
    CHECK(SelectObject(hdc, hbmOld) == hbm && SelectObject(hsrc, hsrcOld) == hsrcbm);
    CHECK(DeleteObject(hbm) && DeleteObject(hsrcbm) && DeleteDC(hdc) && DeleteDC(hsrc));
    CHECK(ReleaseDC(NULL, hscreen));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 drawing-state contracts passed");
    return 0;
}
