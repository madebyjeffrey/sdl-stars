/* The same ownership and pixel contracts run against the SDK and local GDI. */
#include <windows.h>
#include <stdio.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    HDC hscreen, hdc, hcopy;
    HBITMAP hbm, hbmOld, hcopybm, hcopyOld;
    HRGN hrgn;
    HBRUSH hred, hblue;
    RECT rc = {0, 0, 32, 32};
    int x, y;
    COLORREF cr;

    hscreen = GetDC(NULL); CHECK(hscreen);
    hdc = CreateCompatibleDC(hscreen); hcopy = CreateCompatibleDC(hscreen);
    hbm = CreateCompatibleBitmap(hscreen, 32, 32); hcopybm = CreateCompatibleBitmap(hscreen, 32, 32);
    CHECK(hdc && hcopy && hbm && hcopybm);
    hbmOld = SelectObject(hdc, hbm); hcopyOld = SelectObject(hcopy, hcopybm);
    hred = CreateSolidBrush(RGB(255, 0, 0)); hblue = CreateSolidBrush(RGB(0, 0, 255));
    CHECK(hred && hblue && FillRect(hdc, &rc, hblue));
    hrgn = CreateRectRgn(2, 2, 30, 30); CHECK(hrgn);
    CHECK(SelectClipRgn(hdc, hrgn) == SIMPLEREGION);
    CHECK(SelectClipRgn(hcopy, hrgn) == SIMPLEREGION && DeleteObject(hrgn));
    CHECK(ExcludeClipRect(hdc, 8, 8, 24, 24) == COMPLEXREGION);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    // Wine reads memory-bitmap pixels outside the application clip; the native
    // implementation follows Microsoft's documented GetPixel clip rejection.
    CHECK(GetPixel(hdc, 12, 12) == CLR_INVALID);
#endif
    CHECK(FillRect(hdc, &rc, hred));
    CHECK(SelectClipRgn(hdc, NULL) == SIMPLEREGION);
    for (y = 0; y < 32; y++) for (x = 0; x < 32; x++) {
        cr = GetPixel(hdc, x, y);
        CHECK(cr == ((x >= 2 && x < 30 && y >= 2 && y < 30 &&
                      !(x >= 8 && x < 24 && y >= 8 && y < 24)) ? RGB(255, 0, 0) : RGB(0, 0, 255)));
    }
    CHECK(FillRect(hcopy, &rc, hblue));
    CHECK(SelectClipRgn(hcopy, NULL) == SIMPLEREGION);
    CHECK(GetPixel(hcopy, 3, 3) == RGB(0, 0, 255));
    CHECK(IntersectClipRect(hdc, 4, 4, 28, 28) == SIMPLEREGION);
    CHECK(ExcludeClipRect(hdc, 10, 10, 22, 22) == COMPLEXREGION);
    CHECK(IntersectClipRect(hdc, 4, 4, 28, 8) == SIMPLEREGION);
    CHECK(ExcludeClipRect(hdc, 4, 4, 28, 8) == NULLREGION);
    CHECK(FillRect(hdc, &rc, hblue) && SelectClipRgn(hdc, NULL) == SIMPLEREGION);
    CHECK(GetPixel(hdc, 5, 5) == RGB(255, 0, 0));
    hrgn = CreateRectRgn(20, 20, 4, 4); CHECK(hrgn);
    CHECK(SelectClipRgn(hdc, hrgn) == SIMPLEREGION && DeleteObject(hrgn));
    CHECK(PatBlt(hdc, 0, 0, 32, 32, BLACKNESS));
    CHECK(ExcludeClipRect(hdc, 8, 8, 16, 16) == COMPLEXREGION);
    CHECK(BitBlt(hdc, 0, 0, 32, 32, hcopy, 0, 0, SRCCOPY));
    CHECK(MoveToEx(hdc, 0, 12, NULL) && LineTo(hdc, 32, 12));
    CHECK(SetBkMode(hdc, OPAQUE) && TextOut(hdc, 4, 4, "WW", 2));
    CHECK(SelectClipRgn(hdc, NULL) == SIMPLEREGION && GetPixel(hdc, 12, 12) == RGB(0, 0, 0));
    hrgn = CreateRectRgn(0, 0, 0, 10); CHECK(hrgn);
    CHECK(SelectClipRgn(hdc, hrgn) == NULLREGION && DeleteObject(hrgn));
    CHECK(SelectClipRgn(hdc, (HRGN)hred) == ERROR);
    CHECK(SelectClipRgn(hdc, NULL) == SIMPLEREGION);
    CHECK(FillRect(hdc, &rc, hblue));
    CHECK(IntersectClipRect(hdc, 0, 0, 32, 32) == SIMPLEREGION);
    // Repeated isolated exclusions exercise region growth beyond four strips.
    for (y = 2; y < 30; y += 4) for (x = 2; x < 30; x += 4)
        CHECK(ExcludeClipRect(hdc, x, y, x + 1, y + 1) == COMPLEXREGION);
    CHECK(FillRect(hdc, &rc, hred) && SelectClipRgn(hdc, NULL) == SIMPLEREGION);
    for (y = 0; y < 32; y++) for (x = 0; x < 32; x++)
        CHECK(GetPixel(hdc, x, y) == ((x >= 2 && x < 30 && y >= 2 && y < 30 &&
                                     x % 4 == 2 && y % 4 == 2) ? RGB(0, 0, 255) : RGB(255, 0, 0)));
    CHECK(SelectObject(hdc, hbmOld) == hbm && SelectObject(hcopy, hcopyOld) == hcopybm);
    CHECK(DeleteObject(hbm) && DeleteObject(hcopybm) && DeleteObject(hred) && DeleteObject(hblue));
    CHECK(DeleteDC(hdc) && DeleteDC(hcopy) && ReleaseDC(NULL, hscreen));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 region contracts passed");
    return 0;
}
