/* Shared Win32/Linux memory DC and bitmap contracts. */
#include <windows.h>
static const RECT rcTest1 = {0, 0, 16, 16};
static const RECT rcTest2 = {0, 0, 1, 1};
static const RECT rcTest3 = {4, 4, 8, 8};
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static LRESULT CALLBACK BitmapProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int TestBitmaps(HWND hwnd) {
    HDC hdc = GetDC(hwnd), hsrc, hdst, hmono, hother;
    HBITMAP hbmSrc, hbmDst, hbmMono, hbmZero, hbmColor;
    HGDIOBJ hSrcOld, hDstOld, hMonoOld, hbrOld;
    HBRUSH hbr;
    BITMAP bm;
    COLORREF cr;
    PAINTSTRUCT ps;
    int i, j;
    BYTE rop;

    CHECK(hdc);
    hsrc = CreateCompatibleDC(hdc); hdst = CreateCompatibleDC(hdc);
    hmono = CreateCompatibleDC(NULL); hother = CreateCompatibleDC(hsrc);
    CHECK(hsrc && hdst && hmono && hother);
    CHECK(SetTextColor(hdc, RGB(11, 22, 33)) == RGB(0, 0, 0));
    CHECK(SetTextColor(hsrc, RGB(44, 55, 66)) == RGB(0, 0, 0));
    hbmMono = CreateCompatibleBitmap(hmono, 8, 8);
    hbmZero = CreateCompatibleBitmap(hdc, 0, 12);
    hbmSrc = CreateCompatibleBitmap(hdc, 16, 16);
    hbmDst = CreateCompatibleBitmap(hdc, 16, 16);
    CHECK(hbmMono && hbmZero && hbmSrc && hbmDst);
    CHECK(GetObjectA(hbmMono, sizeof(bm), &bm) == sizeof(bm) && bm.bmBitsPixel == 1 && bm.bmWidthBytes == 2);
    CHECK(GetObjectA(hbmZero, sizeof(bm), &bm) == sizeof(bm) && bm.bmWidth == 1 && bm.bmHeight == 1 && bm.bmBitsPixel == 1);
    CHECK(DeleteObject(hbmZero));
    CHECK(GetObjectA(hbmSrc, 0, NULL) == sizeof(bm));
    CHECK(GetObjectA(hbmSrc, sizeof(bm), &bm) == sizeof(bm) && bm.bmWidth == 16 && bm.bmHeight == 16 && bm.bmBitsPixel >= 24);
    hSrcOld = SelectObject(hsrc, hbmSrc); hDstOld = SelectObject(hdst, hbmDst);
    hMonoOld = SelectObject(hmono, hbmMono);
    CHECK(hSrcOld && hDstOld && hMonoOld);
    CHECK(GetObjectA(hSrcOld, sizeof(bm), &bm) == sizeof(bm) && bm.bmWidth == 1 && bm.bmBitsPixel == 1);
    hbmColor = CreateCompatibleBitmap(hsrc, 4, 4);
    CHECK(hbmColor && GetObjectA(hbmColor, sizeof(bm), &bm) == sizeof(bm) && bm.bmBitsPixel >= 24);
    CHECK(DeleteObject(hbmColor));
    CHECK(!SelectObject(hother, hbmSrc));
    // Deleting selected bitmaps has driver-dependent/deferred semantics on Windows.
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!DeleteObject(hbmSrc) && !DeleteDC(hdc) && !ReleaseDC(hwnd, hsrc));
#endif
    hbr = CreateSolidBrush(RGB(15, 35, 55));
    CHECK(hbr && FillRect(hsrc, &rcTest1, hbr));
    CHECK(GetPixel(hsrc, 0, 0) == RGB(15, 35, 55));
    CHECK(PatBlt(hdst, 0, 0, 16, 16, WHITENESS));
    CHECK(BitBlt(hdst, 2, 3, 5, 6, hsrc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hdst, 2, 3) == RGB(15, 35, 55));
    CHECK(GetPixel(hdst, 6, 8) == RGB(15, 35, 55) && GetPixel(hdst, 7, 8) == RGB(255, 255, 255));
    CHECK(BitBlt(hdst, -2, -3, 5, 6, hsrc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hdst, 0, 0) == RGB(15, 35, 55));
    CHECK(BitBlt(hdst, 0, 0, 1, 1, hsrc, 0, 0, SRCAND));
    CHECK(GetPixel(hdst, 0, 0) == RGB(15, 35, 55));
    CHECK(PatBlt(hdst, 0, 0, 1, 1, BLACKNESS));
    CHECK(BitBlt(hdst, 0, 0, 1, 1, hsrc, 0, 0, SRCPAINT));
    CHECK(GetPixel(hdst, 0, 0) == RGB(15, 35, 55));
    CHECK(BitBlt(hdst, 0, 0, 1, 1, hsrc, 0, 0, UINT32_C(0x00220326)));
    CHECK(GetPixel(hdst, 0, 0) == 0);
    hbrOld = SelectObject(hdst, hbr);
    CHECK(hbrOld && PatBlt(hdst, 0, 0, 1, 1, PATCOPY));
    CHECK(GetPixel(hdst, 0, 0) == RGB(15, 35, 55));
    CHECK(PatBlt(hdst, 0, 0, 1, 1, DSTINVERT));
    CHECK(GetPixel(hdst, 0, 0) == (RGB(15, 35, 55) ^ RGB(255, 255, 255)));
    CHECK(!PatBlt(hdst, 0, 0, 1, 1, SRCCOPY));

    // All 256 truth tables, each tested with every binary P/S/D input.
    for (i = 0; i < 256; i++) {
        rop = (BYTE)i;
        for (j = 0; j < 8; j++) {
            SelectObject(hdst, GetStockObject(j & 4 ? WHITE_BRUSH : BLACK_BRUSH));
            CHECK(PatBlt(hsrc, 0, 0, 1, 1, j & 2 ? WHITENESS : BLACKNESS));
            CHECK(PatBlt(hdst, 0, 0, 1, 1, j & 1 ? WHITENESS : BLACKNESS));
            CHECK(BitBlt(hdst, 0, 0, 1, 1, hsrc, 0, 0, (DWORD)rop << 16));
            CHECK(GetPixel(hdst, 0, 0) == (rop & (1 << j) ? RGB(255, 255, 255) : 0));
        }
    }
    SelectObject(hdst, hbrOld);
    CHECK(DeleteObject(hbr));
    // In-place horizontal and vertical moves capture the source before writes.
    CHECK(PatBlt(hsrc, 0, 0, 16, 16, BLACKNESS));
    CHECK(PatBlt(hsrc, 0, 0, 1, 1, WHITENESS));
    CHECK(BitBlt(hsrc, 1, 0, 8, 1, hsrc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hsrc, 1, 0) == RGB(255, 255, 255) && GetPixel(hsrc, 2, 0) == 0);
    CHECK(BitBlt(hsrc, 0, 1, 2, 8, hsrc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hsrc, 1, 1) == RGB(255, 255, 255) && GetPixel(hsrc, 1, 2) == 0);
    // Color-to-mono uses the source background; mono-to-color uses destination colors.
    CHECK(PatBlt(hsrc, 0, 0, 8, 8, BLACKNESS));
    CHECK(FillRect(hsrc, &rcTest2, (HBRUSH)GetStockObject(WHITE_BRUSH)));
    SetTextColor(hsrc, RGB(0, 0, 0));
    SetBkColor(hsrc, RGB(255, 255, 255));
    CHECK(BitBlt(hmono, 0, 0, 8, 8, hsrc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hmono, 0, 0) == RGB(255, 255, 255) && GetPixel(hmono, 1, 0) == 0);
    SetTextColor(hdst, RGB(200, 30, 40)); SetBkColor(hdst, RGB(10, 220, 60));
    CHECK(BitBlt(hdst, 0, 0, 8, 8, hmono, 0, 0, SRCCOPY));
    CHECK(GetPixel(hdst, 0, 0) == RGB(10, 220, 60) && GetPixel(hdst, 1, 0) == RGB(200, 30, 40));
    // Window-to-memory capture and memory-to-window painting obey paint clipping.
    CHECK(PatBlt(hdc, 0, 0, 20, 20, WHITENESS));
    CHECK(BitBlt(hdst, 0, 0, 4, 4, hdc, 0, 0, SRCCOPY));
    CHECK(GetPixel(hdst, 0, 0) == RGB(255, 255, 255));
    CHECK(PatBlt(hdst, 0, 0, 16, 16, BLACKNESS));
    CHECK(ValidateRect(hwnd, NULL) && InvalidateRect(hwnd, &rcTest3, 0));
    CHECK(BeginPaint(hwnd, &ps));
    CHECK(BitBlt(ps.hdc, 0, 0, 16, 16, hdst, 0, 0, SRCCOPY));
    CHECK(EndPaint(hwnd, &ps));
    CHECK(GetPixel(hdc, 4, 4) == 0 && GetPixel(hdc, 7, 7) == 0);
    CHECK(GetPixel(hdc, 3, 3) == RGB(255, 255, 255) && GetPixel(hdc, 8, 8) == RGB(255, 255, 255));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!BitBlt(hdst, 0, 0, 4, 4, NULL, 0, 0, SRCCOPY));
    CHECK(BitBlt(hdst, INT_MIN, INT_MIN, INT_MAX, INT_MAX, hsrc, 0, 0, SRCCOPY));
    CHECK(BitBlt(hdst, 0, 0, INT_MAX, INT_MAX, hsrc, 0, 0, SRCCOPY));
    CHECK(!BitBlt(hdst, 0, 0, 4, 4, hsrc, 0, 0, SRCCOPY | UINT32_C(0x40000000)));
#endif
    CHECK(SelectObject(hsrc, hSrcOld) == hbmSrc && DeleteObject(hbmSrc));
    CHECK(SelectObject(hmono, hMonoOld) == hbmMono && DeleteObject(hbmMono));
    CHECK(DeleteDC(hsrc) && DeleteDC(hmono) && DeleteDC(hother));
    // DeleteDC releases the selected bitmap reference, without deleting the bitmap.
    CHECK(DeleteDC(hdst) && DeleteObject(hbmDst));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!DeleteDC(hdst) && GetPixel(hdst, 0, 0) == CLR_INVALID && !SelectObject(hdst, hbmDst));
#endif
    CHECK(ReleaseDC(hwnd, hdc));
    hsrc = CreateCompatibleDC(NULL);
    CHECK(hsrc && DestroyWindow(hwnd));
    cr = GetPixel(hsrc, 0, 0);
    CHECK(cr != CLR_INVALID && DeleteDC(hsrc));
    return 0;
}

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    int result;

    wc.lpfnWndProc = BitmapProc;
    wc.lpszClassName = "StarsBitmapReference";
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Stars bitmap contracts", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        20, 20, 200, 160, NULL, NULL, NULL, NULL);
    CHECK(hwnd && UpdateWindow(hwnd));
    result = TestBitmaps(hwnd);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    if (result == 0)
        puts("Memory DCs and bitmap blits: PASS (ownership, all ROP3 truth tables, overlap, mono and window transfers)");
    return result;
}
