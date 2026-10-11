/* Pixel movement, pending damage and child relocation share SDK contracts. */
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#ifndef _WIN32
#include <stars_compat.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cErase;
#ifndef _WIN32
static HWND hwndDestroyMove;
#endif
static LRESULT CALLBACK ScrollProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
#ifndef _WIN32
    if (msg == WM_WINDOWPOSCHANGED && hwndDestroyMove) {
        HWND hwndParent = hwndDestroyMove;
        hwndDestroyMove = NULL; DestroyWindow(hwndParent);
        return 0;
    }
#endif
    if (msg == WM_ERASEBKGND) { cErase++; return 1; }
    return DefWindowProc(hwnd, msg, w, l);
}
static int TestPixels(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    RECT rc = {8, 8, 48, 32}, rcClip = {16, 12, 40, 28}, rcUpdate;
    int x, y;
    CHECK(hdc);
    for (y = 0; y < 48; y++) for (x = 0; x < 64; x++)
        CHECK(SetPixel(hdc, x, y, RGB(x, y, 123)) != CLR_INVALID);
    CHECK(ReleaseDC(hwnd, hdc));
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(ScrollWindow(hwnd, -4, 0, &rc, &rcClip));
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(GetPixel(hdc, 16, 12) == RGB(20, 12, 123));
    CHECK(GetPixel(hdc, 35, 27) == RGB(39, 27, 123));
    CHECK(GetPixel(hdc, 15, 12) == RGB(15, 12, 123));
    CHECK(GetPixel(hdc, 40, 12) == RGB(40, 12, 123));
    CHECK(ReleaseDC(hwnd, hdc));
    CHECK(GetUpdateRect(hwnd, &rcUpdate, FALSE) && EqualRect(&rcUpdate, &(RECT){36, 12, 40, 28}));
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(ScrollWindow(hwnd, 0, -5, &rc, &rc));
    CHECK(GetUpdateRect(hwnd, &rcUpdate, FALSE));
    CHECK(EqualRect(&rcUpdate, &(RECT){8, 27, 48, 32}));
    cErase = 0; CHECK(GetUpdateRect(hwnd, NULL, TRUE) && cErase == 1);
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(ScrollWindow(hwnd, INT_MAX, INT_MIN, &rc, NULL));
    CHECK(GetUpdateRect(hwnd, &rcUpdate, FALSE) && EqualRect(&rcUpdate, &rc));
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(ScrollWindow(hwnd, 0, 0, &rc, NULL) && !GetUpdateRect(hwnd, NULL, FALSE));
    return 0;
}
static int TestOverlap(HWND hwnd) {
    static const POINT rgDelta[] = {{4, 5}, {-4, 5}, {4, -5}, {-4, -5}};
    RECT rc = {8, 8, 48, 32}, rcClip = {16, 12, 40, 28};
    HDC hdc;
    int i, x, y, xSrc, ySrc;

    for (i = 0; i < 4; i++) {
        hdc = GetDC(hwnd); CHECK(hdc);
        for (y = 0; y < 48; y++) for (x = 0; x < 64; x++)
            CHECK(SetPixel(hdc, x, y, RGB(x, y, 123)) != CLR_INVALID);
        CHECK(ReleaseDC(hwnd, hdc) && ValidateRect(hwnd, NULL));
        CHECK(ScrollWindow(hwnd, rgDelta[i].x, rgDelta[i].y, &rc, &rcClip));
        hdc = GetDC(hwnd); CHECK(hdc);
        for (y = 0; y < 48; y++) for (x = 0; x < 64; x++) {
            if (x < rcClip.left || x >= rcClip.right || y < rcClip.top || y >= rcClip.bottom) {
                CHECK(GetPixel(hdc, x, y) == RGB(x, y, 123));
            } else {
                xSrc = x - rgDelta[i].x; ySrc = y - rgDelta[i].y;
                if (xSrc >= rcClip.left && xSrc < rcClip.right && ySrc >= rcClip.top && ySrc < rcClip.bottom)
                    CHECK(GetPixel(hdc, x, y) == RGB(xSrc, ySrc, 123));
            }
        }
        CHECK(ReleaseDC(hwnd, hdc));
    }
    CHECK(ValidateRect(hwnd, NULL));
    return 0;
}
static int TestDamage(HWND hwnd, HWND hwndChild) {
    RECT rc, rcClient;
    POINT pt = {0, 0};
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr;
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(InvalidateRect(hwnd, &(RECT){20, 20, 22, 22}, FALSE));
    CHECK(ScrollWindow(hwnd, 3, 0, NULL, NULL));
    CHECK(ClientToScreen(hwndChild, &pt) && ScreenToClient(hwnd, &pt));
    CHECK(pt.x == 13 && pt.y == 10);
    CHECK(ValidateRect(hwnd, &(RECT){0, 0, 3, 48}));
    CHECK(GetUpdateRect(hwnd, &rc, FALSE));
    CHECK((rc.left == 20 || rc.left == 23) && rc.top == 20 && rc.right == 25 && rc.bottom == 22);
#ifndef _WIN32
    CHECK(rc.left == 23);
#endif
    CHECK(ValidateRect(hwnd, NULL) && GetClientRect(hwnd, &rcClient));
    CHECK(InvalidateRect(hwnd, &(RECT){20, 20, 22, 22}, FALSE));
    CHECK(ScrollWindow(hwnd, 3, 0, &rcClient, NULL));
    pt = (POINT){0, 0}; CHECK(ClientToScreen(hwndChild, &pt) && ScreenToClient(hwnd, &pt));
    CHECK(pt.x == 13 && pt.y == 10);
    CHECK(ValidateRect(hwnd, &(RECT){0, 0, 3, 48}));
    CHECK(GetUpdateRect(hwnd, &rc, FALSE));
    CHECK(rc.left == 20 && rc.top == 20 && (rc.right == 22 || rc.right == 25) && rc.bottom == 22);
#ifndef _WIN32
    CHECK(rc.right == 22);
#endif
    CHECK(ValidateRect(hwnd, NULL));
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH))); CHECK(ReleaseDC(hwnd, hdc));
    CHECK(InvalidateRect(hwnd, &(RECT){30, 10, 40, 20}, FALSE));
    CHECK(ValidateRect(hwnd, &(RECT){33, 13, 37, 17}));
    CHECK(InvalidateRect(hwnd, &(RECT){45, 10, 48, 13}, FALSE));
    hdc = BeginPaint(hwnd, &ps); CHECK(hdc);
    CHECK(SelectClipRgn(hdc, NULL) != ERROR);
    hbr = CreateSolidBrush(RGB(250, 10, 20)); CHECK(hbr);
    CHECK(FillRect(hdc, &rcClient, hbr)); CHECK(DeleteObject(hbr));
    CHECK(EndPaint(hwnd, &ps) && !GetUpdateRect(hwnd, NULL, FALSE));
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(GetPixel(hdc, 30, 10) == RGB(250, 10, 20));
    CHECK(GetPixel(hdc, 35, 15) == RGB(0, 0, 0));
    CHECK(GetPixel(hdc, 42, 11) == RGB(0, 0, 0));
    CHECK(GetPixel(hdc, 46, 11) == RGB(250, 10, 20));
    CHECK(ReleaseDC(hwnd, hdc));
    CHECK(ScrollWindow(hwnd, 2, 3, NULL, &(RECT){0}));
    pt = (POINT){0, 0}; CHECK(ClientToScreen(hwndChild, &pt) && ScreenToClient(hwnd, &pt));
    CHECK(pt.x == 15 && pt.y == 13 && !GetUpdateRect(hwnd, NULL, FALSE));
    return 0;
}
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndChild;
    HINSTANCE hInstance;
#ifdef _WIN32
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.lpszClassName = "StarsScrollWindow"; wc.hInstance = hInstance; wc.lpfnWndProc = ScrollProc;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "scroll pixels", WS_POPUP | WS_VISIBLE,
                        40, 50, 64, 48, NULL, NULL, hInstance, NULL); CHECK(hwnd);
    CHECK(UpdateWindow(hwnd)); CHECK(TestPixels(hwnd) == 0);
    CHECK(TestOverlap(hwnd) == 0);
    hwndChild = CreateWindow(wc.lpszClassName, "child", WS_CHILD,
                             10, 10, 4, 4, hwnd, NULL, hInstance, NULL); CHECK(hwndChild);
    CHECK(TestDamage(hwnd, hwndChild) == 0);
#ifndef _WIN32
    hwndDestroyMove = hwnd;
    CHECK(!ScrollWindow(hwnd, 1, 0, NULL, NULL) && !IsWindow(hwnd) && !IsWindow(hwndChild));
#else
    CHECK(DestroyWindow(hwnd));
#endif
#ifndef _WIN32
    CHECK(!ScrollWindow(hwnd, 1, 0, NULL, NULL) && GetLastError() == ERROR_INVALID_WINDOW_HANDLE);
    StarsCompatShutdown();
#endif
    puts("Win32 ScrollWindow contracts passed");
    return 0;
}
