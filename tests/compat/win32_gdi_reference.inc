/* Shared drawing/paint contracts, checked against the actual Windows SDK under
 * Wine as well as the Linux runtime. Font glyphs and metrics deliberately vary.
 */
#include <windows.h>
static const RECT rcTest1 = {0, 0, 100, 100};
static const RECT rcTest2 = {10, 10, 20, 20};
static const RECT rcTest3 = {10, 10, 20, 20};
static const RECT rcTest4 = {0, 0, 50, 50};
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static LRESULT CALLBACK ReferenceProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    HDC hdc;
    HBRUSH hbr;
    HPEN hpen;
    HGDIOBJ hOld;
    RECT rc;
    POINT pt;
    SIZE size;
    TEXTMETRIC tm;
    PAINTSTRUCT ps;

    wc.lpfnWndProc = ReferenceProc;
    wc.lpszClassName = "StarsGdiReference";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "GDI reference", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        20, 20, 200, 160, NULL, NULL, NULL, NULL);
    CHECK(hwnd && UpdateWindow(hwnd));
    hdc = GetDC(hwnd);
    hbr = CreateSolidBrush(RGB(15, 35, 55));
    hpen = CreatePen(PS_SOLID, 1, RGB(210, 170, 30));
    CHECK(hdc && hbr && hpen);
    CHECK(FillRect(hdc, &rcTest1, hbr));
    CHECK(GetPixel(hdc, 5, 5) == RGB(15, 35, 55));
    CHECK(GetPixel(hdc, 99, 99) == RGB(15, 35, 55));
    hOld = SelectObject(hdc, hpen);
    CHECK(hOld);
    CHECK(MoveToEx(hdc, 10, 10, NULL) && LineTo(hdc, 20, 10));
    CHECK(GetPixel(hdc, 10, 10) == RGB(210, 170, 30));
    CHECK(GetPixel(hdc, 19, 10) == RGB(210, 170, 30));
    CHECK(GetPixel(hdc, 20, 10) == RGB(15, 35, 55));
    CHECK(Rectangle(hdc, 30, 30, 40, 40));
    CHECK(GetPixel(hdc, 30, 30) == RGB(210, 170, 30));
    CHECK(GetPixel(hdc, 31, 31) == RGB(255, 255, 255));
    CHECK(GetPixel(hdc, 39, 39) == RGB(210, 170, 30));
    CHECK(GetPixel(hdc, 40, 40) == RGB(15, 35, 55));
    CHECK(MoveToEx(hdc, 0, 0, &pt) && pt.x == 20 && pt.y == 10);
    CHECK(SelectObject(hdc, hOld) == hpen && DeleteObject(hpen) && DeleteObject(hbr));
    CHECK(SetTextColor(hdc, RGB(1, 2, 3)) == RGB(0, 0, 0));
    CHECK(SetBkColor(hdc, RGB(4, 5, 6)) == RGB(255, 255, 255));
    CHECK(SetBkMode(hdc, TRANSPARENT) == OPAQUE);
    CHECK(GetTextExtentPoint32A(hdc, "Stars", 5, &size) && size.cx > 0 && size.cy > 0);
    CHECK(GetTextMetricsA(hdc, &tm) && tm.tmHeight > 0 && tm.tmAveCharWidth > 0);
    CHECK(TextOutA(hdc, 10, 60, "Stars", 5));
    CHECK(ReleaseDC(hwnd, hdc));
    CHECK(ValidateRect(hwnd, NULL));
    CHECK(InvalidateRect(hwnd, &rcTest2, 1));
    CHECK(GetUpdateRect(hwnd, &rc, 0) && EqualRect(&rc, &rcTest3));
    CHECK(GetUpdateRect(hwnd, NULL, 1));
    hdc = GetDC(hwnd);
    CHECK(GetPixel(hdc, 10, 10) == RGB(255, 255, 255));
    CHECK(GetPixel(hdc, 9, 9) == RGB(15, 35, 55));
    CHECK(ReleaseDC(hwnd, hdc));
    CHECK(BeginPaint(hwnd, &ps) && !ps.fErase && EqualRect(&ps.rcPaint, &rc));
    CHECK(!GetUpdateRect(hwnd, &rc, 0));
    CHECK(FillRect(ps.hdc, &rcTest4, (HBRUSH)GetStockObject(BLACK_BRUSH)));
    CHECK(EndPaint(hwnd, &ps));
    hdc = GetDC(hwnd);
    CHECK(GetPixel(hdc, 10, 10) == RGB(0, 0, 0));
    CHECK(GetPixel(hdc, 19, 19) == RGB(0, 0, 0));
    CHECK(GetPixel(hdc, 20, 20) == RGB(15, 35, 55));
    CHECK(ReleaseDC(hwnd, hdc) && DestroyWindow(hwnd));
    puts("Shared Windows/Linux GDI contracts: PASS");
    return 0;
}
