/* The UI path uses Win32 APIs; SDL is only used for renderer observation and
 * native-event injection in the automated test.
 */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)

typedef struct PAINTTEST {
    int i, cPaint;
    COLORREF color;
    BOOL fOk, fInvalidateInPaint, fDestroyOnErase;
    RECT rcLast;
} PAINTTEST;

static int cLive;
static BOOL fInteractive;

static BOOL FDraw(HWND hwnd, HDC hdcDest, PAINTTEST *lpwin) {
    RECT rc;
    HBRUSH hbr;
    HPEN hpen;
    HDC hdc;
    HBITMAP hbm;
    HRSRC hrsrc;
    const BITMAPINFOHEADER *lpbi;
    const BYTE *rgb;
    DWORD cColors;
    HGDIOBJ hbmOld;
    HGDIOBJ hbrOld, hpenOld;
    SIZE size;
    char sz[120];
    int cch;
    BOOL fOk;

    GetClientRect(hwnd, &rc);
    hdc = CreateCompatibleDC(hdcDest);
    hbm = CreateCompatibleBitmap(hdcDest, rc.right, rc.bottom);
    if (!hdc || !hbm) {
        DeleteDC(hdc); DeleteObject(hbm);
        return FALSE;
    }
    hbmOld = SelectObject(hdc, hbm);
    hbr = CreateSolidBrush(lpwin->color);
    hpen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    if (!hbr || !hpen) {
        DeleteObject(hbr); DeleteObject(hpen);
        DeleteDC(hdc); DeleteObject(hbm);
        return FALSE;
    }
    fOk = FillRect(hdc, &rc, hbr);
    hbrOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    hpenOld = SelectObject(hdc, hpen);
    fOk &= Rectangle(hdc, 12, 12, rc.right - 12, rc.bottom - 12);
    fOk &= MoveToEx(hdc, 20, 70, NULL) && LineTo(hdc, rc.right - 20, 70);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    cch = snprintf(sz, sizeof(sz), "Window %d - %ld x %ld", lpwin->i + 1, (long)rc.right, (long)rc.bottom);
    fOk &= GetTextExtentPoint32(hdc, sz, cch, &size);
    fOk &= TextOut(hdc, (rc.right - size.cx) / 2, 30, sz, cch);
    fOk &= TextOut(hdc, 24, 90, "Resize me; press a key to change color.", 38);
    fOk &= TextOut(hdc, 24, 110, "Close one window; the other keeps painting.", 42);
    // Draw an indexed image from the game's real resource atlas in each window.
    hrsrc = FindResource(NULL, MAKEINTRESOURCE(87), RT_BITMAP);
    lpbi = LockResource(LoadResource(NULL, hrsrc));
    if (lpbi) {
        cColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1u << lpbi->biBitCount;
        rgb = (const BYTE *)(lpbi + 1) + cColors * sizeof(RGBQUAD);
        fOk &= StretchDIBits(hdc, 24, 140, 64, 64, 0, 0, 64, 64, rgb,
                            (const BITMAPINFO *)lpbi, DIB_RGB_COLORS, SRCCOPY) > 0;
    } else {
        fOk = FALSE;
    }
    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hbrOld);
    fOk &= DeleteObject(hpen) && DeleteObject(hbr);
    fOk &= BitBlt(hdcDest, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
    fOk &= SelectObject(hdc, hbmOld) == hbm;
    fOk &= DeleteDC(hdc) && DeleteObject(hbm);
    return fOk;
}

static LRESULT CALLBACK PaintWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PAINTTEST *lpwin = (PAINTTEST *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    PAINTSTRUCT ps;

    if (msg == WM_NCCREATE) {
        lpwin = ((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpwin);
        return TRUE;
    }
    switch (msg) {
    case WM_CREATE:
        cLive++;
        return 0;
    case WM_ERASEBKGND:
        if (lpwin->fDestroyOnErase) {
            DestroyWindow(hwnd);
            return TRUE;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    case WM_PAINT:
        if (!BeginPaint(hwnd, &ps)) {
            lpwin->fOk = FALSE;
            return 0;
        }
        lpwin->rcLast = ps.rcPaint;
        lpwin->cPaint++;
        lpwin->fOk &= FDraw(hwnd, ps.hdc, lpwin);
        if (lpwin->fInvalidateInPaint) {
            lpwin->fInvalidateInPaint = FALSE;
            lpwin->fOk &= InvalidateRect(hwnd, &(RECT){4, 4, 8, 8}, FALSE);
        }
        lpwin->fOk &= EndPaint(hwnd, &ps);
        return 0;
    case WM_KEYDOWN:
        lpwin->color ^= RGB(64, 32, 96);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_DESTROY:
        if (--cLive == 0 && fInteractive)
            PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

static void Drain(void) {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessage(&msg);
}

static SDL_Window *LpSDLWindow(const char *psz) {
    SDL_Window **rgwin, *lpwin = NULL;
    int cwin, i;

    rgwin = SDL_GetWindows(&cwin);
    for (i = 0; i < cwin; i++)
        if (strcmp(SDL_GetWindowTitle(rgwin[i]), psz) == 0)
            lpwin = rgwin[i];
    SDL_free(rgwin);
    return lpwin;
}

static int TestPaint(HWND *rghwnd, PAINTTEST *rgwin) {
    HDC hdc, hdcOther, hdcNew;
    HBRUSH hbr;
    HPEN hpen;
    HGDIOBJ hOld, hbrOld;
    RECT rc;
    POINT pt;
    SIZE size;
    TEXTMETRIC tm;
    PAINTSTRUCT ps, psOther;
    MSG msg;
    SDL_Event event = {0};
    SDL_Window *lpSDL[2];
    SDL_Renderer *lpRenderer[2];
    int cPaint, x, y, cText;
    COLORREF color;
    PAINTTEST winDestroy = {.fOk = TRUE, .fDestroyOnErase = TRUE};
    HWND hwndDestroy;

    Drain();
    CHECK(rgwin[0].fOk && rgwin[1].fOk && rgwin[0].cPaint > 0 && rgwin[1].cPaint > 0);
    lpSDL[0] = LpSDLWindow("Stars GDI first");
    lpSDL[1] = LpSDLWindow("Stars GDI second");
    CHECK(lpSDL[0] && lpSDL[1]);
    lpRenderer[0] = SDL_GetRenderer(lpSDL[0]);
    lpRenderer[1] = SDL_GetRenderer(lpSDL[1]);
    CHECK(lpRenderer[0] && lpRenderer[1] && lpRenderer[0] != lpRenderer[1]);
    printf("GDI renderers: %s / %s\n", SDL_GetRendererName(lpRenderer[0]), SDL_GetRendererName(lpRenderer[1]));
    hdc = GetDC(rghwnd[0]); hdcOther = GetDC(rghwnd[1]);
    CHECK(hdc && hdcOther && hdc != hdcOther);
    CHECK(GetPixel(hdc, 0, 0) == rgwin[0].color && GetPixel(hdcOther, 0, 0) == rgwin[1].color);
    CHECK(GetPixel(hdc, -1, 0) == CLR_INVALID);
    hbr = CreateSolidBrush(RGB(12, 34, 56));
    hpen = CreatePen(PS_SOLID, 1, RGB(220, 180, 40));
    CHECK(hbr && hpen && !CreatePen(99, 1, 0) && !CreatePen(PS_SOLID, 2, 0));
    CHECK(FillRect(hdc, &(RECT){-10, -10, 10, 10}, hbr));
    CHECK(GetPixel(hdc, 0, 0) == RGB(12, 34, 56));
    CHECK(GetPixel(hdc, 9, 9) == RGB(12, 34, 56) && GetPixel(hdc, 10, 10) == rgwin[0].color);
    CHECK(GetPixel(hdcOther, 0, 0) == rgwin[1].color);
    hOld = SelectObject(hdc, hpen); hbrOld = SelectObject(hdc, hbr);
    CHECK(hOld && hbrOld && !DeleteObject(hpen) && !DeleteObject(hbr));
    CHECK(MoveToEx(hdc, 20, 20, &pt) && pt.x == 0 && pt.y == 0);
    CHECK(LineTo(hdc, 30, 20));
    CHECK(GetPixel(hdc, 20, 20) == RGB(220, 180, 40) && GetPixel(hdc, 29, 20) == RGB(220, 180, 40));
    CHECK(GetPixel(hdc, 30, 20) == rgwin[0].color);
    CHECK(MoveToEx(hdc, INT_MIN, 180, &pt) && pt.x == 30 && pt.y == 20);
    CHECK(LineTo(hdc, INT_MAX, 180) && GetPixel(hdc, 0, 180) == RGB(220, 180, 40));
    CHECK(Rectangle(hdc, 30, 30, 40, 40));
    CHECK(GetPixel(hdc, 30, 30) == RGB(220, 180, 40));
    CHECK(GetPixel(hdc, 31, 31) == RGB(12, 34, 56));
    CHECK(GetPixel(hdc, 40, 40) == rgwin[0].color);
    CHECK(Rectangle(hdc, INT_MIN, INT_MIN, INT_MAX, INT_MAX));
    CHECK(MoveToEx(hdc, 0, 0, &pt) && pt.x == INT_MAX && pt.y == 180);
    CHECK(SelectObject(hdc, hOld) == hpen && SelectObject(hdc, hbrOld) == hbr);
    CHECK(DeleteObject(hpen) && !DeleteObject(hpen) && DeleteObject(hbr));
    CHECK(!SelectObject(hdc, hpen) && !DeleteObject(GetStockObject(BLACK_PEN)));
    CHECK(GetTextExtentPoint32(hdc, "Hello", 5, &size) && size.cx > 0 && size.cy > 0);
    CHECK(GetTextExtentPoint32(hdc, NULL, 0, &size) && size.cx == 0);
    CHECK(!GetTextExtentPoint32(hdc, "x", INT_MAX, &size));
    CHECK(GetTextMetrics(hdc, &tm) && tm.tmHeight > 0 && tm.tmAscent + tm.tmDescent == tm.tmHeight);
    CHECK(SetTextColor(hdc, RGB(255, 0, 0)) == RGB(0, 0, 0));
    CHECK(SetBkColor(hdc, RGB(0, 0, 255)) == RGB(255, 255, 255));
    CHECK(SetBkMode(hdc, OPAQUE) == OPAQUE && !SetBkMode(hdc, 3));
    CHECK(GetTextExtentPoint32(hdc, "! ", 2, &size));
    CHECK(TextOut(hdc, 50, 50, "! ", 2));
    CHECK(GetPixel(hdc, 50 + size.cx - 1, 50 + tm.tmHeight - 1) == RGB(0, 0, 255));
    CHECK(GetPixel(hdc, 49, 50) == RGB(12, 34, 56));
    CHECK(SetBkMode(hdc, TRANSPARENT) == OPAQUE);
    CHECK(TextOut(hdc, 70, 50, "!", 1));
    CHECK(GetPixel(hdc, 70 + size.cx + 2, 50) == RGB(12, 34, 56));
    CHECK(TextOut(hdc, 140, 50, "\xff", 1));
    CHECK(TextOut(hdc, INT_MAX, INT_MAX, "!", 1));
    cText = 0;
    for (y = 50; y < 50 + tm.tmHeight; y++)
        for (x = 50; x < 50 + size.cx; x++)
            if (GetPixel(hdc, x, y) != RGB(0, 0, 255))
                cText++;
    CHECK(cText > 0 && cText < size.cx * tm.tmHeight);
    CHECK(!ReleaseDC(rghwnd[1], hdc));
    CHECK(ReleaseDC(rghwnd[0], hdc) && ReleaseDC(rghwnd[1], hdcOther));
    CHECK(GetPixel(hdc, 0, 0) == CLR_INVALID && !ReleaseDC(rghwnd[0], hdc));
    hdcNew = GetDC(rghwnd[0]);
    CHECK(hdcNew && hdcNew != hdc && GetPixel(hdc, 0, 0) == CLR_INVALID);
    CHECK(ReleaseDC(rghwnd[0], hdcNew));

    // Paint remains pending until BeginPaint validates; posted messages come first.
    CHECK(InvalidateRect(rghwnd[0], &(RECT){10, 10, 20, 20}, FALSE));
    CHECK(InvalidateRect(rghwnd[0], &(RECT){15, 15, 30, 30}, TRUE));
    CHECK(GetUpdateRect(rghwnd[0], &rc, FALSE) && EqualRect(&rc, &(RECT){10, 10, 30, 30}));
    CHECK(PostMessage(rghwnd[1], WM_APP, 0, 0));
    CHECK(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_APP);
    CHECK(PeekMessage(&msg, rghwnd[0], WM_PAINT, WM_PAINT, PM_NOREMOVE));
    CHECK(!PeekMessage(&msg, rghwnd[1], WM_PAINT, WM_PAINT, PM_NOREMOVE));
    CHECK(ValidateRect(rghwnd[0], &(RECT){15, 15, 20, 20}));
    hdc = BeginPaint(rghwnd[0], &ps);
    CHECK(hdc && ps.fErase && EqualRect(&ps.rcPaint, &rc));
    CHECK(!BeginPaint(rghwnd[0], &psOther) && !GetUpdateRect(rghwnd[0], NULL, FALSE));
    CHECK(!ReleaseDC(rghwnd[0], hdc) && !EndPaint(rghwnd[1], &ps));
    CHECK(FillRect(hdc, &(RECT){0, 0, 100, 100}, (HBRUSH)GetStockObject(WHITE_BRUSH)));
    CHECK(!PeekMessage(&msg, rghwnd[0], WM_PAINT, WM_PAINT, PM_REMOVE));
    CHECK(EndPaint(rghwnd[0], &ps) && !EndPaint(rghwnd[0], &ps));
    hdc = GetDC(rghwnd[0]);
    CHECK(GetPixel(hdc, 10, 10) == RGB(255, 255, 255));
    CHECK(GetPixel(hdc, 9, 9) == RGB(12, 34, 56) && GetPixel(hdc, 30, 30) == RGB(12, 34, 56));
    CHECK(ReleaseDC(rghwnd[0], hdc));
    CHECK(BeginPaint(rghwnd[0], &ps) && !ps.fErase && EqualRect(&ps.rcPaint, &(RECT){0}));
    CHECK(FillRect(ps.hdc, &(RECT){0, 0, 100, 100}, (HBRUSH)GetStockObject(BLACK_BRUSH)));
    CHECK(EndPaint(rghwnd[0], &ps));
    CHECK(InvalidateRect(rghwnd[0], NULL, FALSE) && ValidateRect(rghwnd[0], NULL));
    CHECK(!GetUpdateRect(rghwnd[0], &rc, FALSE) && EqualRect(&rc, &(RECT){0}));
    rgwin[0].fInvalidateInPaint = TRUE;
    cPaint = rgwin[0].cPaint;
    CHECK(InvalidateRect(rghwnd[0], NULL, FALSE) && UpdateWindow(rghwnd[0]));
    CHECK(rgwin[0].cPaint == cPaint + 1 && GetUpdateRect(rghwnd[0], &rc, FALSE));
    CHECK(UpdateWindow(rghwnd[0]) && rgwin[0].cPaint == cPaint + 2);
    CHECK(!GetUpdateRect(rghwnd[0], NULL, FALSE));
    CHECK(MoveWindow(rghwnd[0], 20, 20, 520, 320, TRUE));
    Drain();
    CHECK(GetClientRect(rghwnd[0], &rc) && rc.right == 520 && rc.bottom == 320);
    hdc = GetDC(rghwnd[0]);
    CHECK(GetPixel(hdc, 519, 319) == rgwin[0].color && ReleaseDC(rghwnd[0], hdc));
    CHECK(rgwin[0].fOk && rgwin[1].fOk);
    cPaint = rgwin[1].cPaint;
    event.type = SDL_EVENT_WINDOW_EXPOSED;
    event.window.windowID = SDL_GetWindowID(lpSDL[1]);
    CHECK(SDL_PushEvent(&event));
    Drain();
    CHECK(rgwin[1].cPaint == cPaint + 1 && rgwin[1].fOk);
    hdcOther = GetDC(rghwnd[0]);
    CHECK(DestroyWindow(rghwnd[0]) && GetPixel(hdcOther, 0, 0) == CLR_INVALID);
    CHECK(!ReleaseDC(rghwnd[0], hdcOther));
    CHECK(cLive == 1 && !UpdateWindow(rghwnd[0]));
    color = rgwin[1].color;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = SDL_GetWindowID(lpSDL[1]);
    event.key.scancode = SDL_SCANCODE_A;
    CHECK(SDL_PushEvent(&event));
    Drain();
    CHECK(rgwin[1].color != color && rgwin[1].cPaint > cPaint && rgwin[1].fOk);
    hdc = GetDC(rghwnd[1]);
    CHECK(GetPixel(hdc, 0, 0) == rgwin[1].color && ReleaseDC(rghwnd[1], hdc));
    CHECK(DestroyWindow(rghwnd[1]) && cLive == 0);
    hwndDestroy = CreateWindow("StarsGdiPaint", "Destroy during erase", WS_OVERLAPPEDWINDOW,
                               0, 0, 100, 100, NULL, NULL, NULL, &winDestroy);
    CHECK(hwndDestroy && InvalidateRect(hwndDestroy, NULL, TRUE));
    CHECK(!BeginPaint(hwndDestroy, &ps) && cLive == 0);
    CHECK(GetPixel(ps.hdc, 0, 0) == CLR_INVALID && !EndPaint(hwndDestroy, &ps));
    return 0;
}

int main(int argc, char **argv) {
    WNDCLASS wc = {0};
    HWND rghwnd[2];
    PAINTTEST rgwin[2] = {{.i = 0, .color = RGB(32, 64, 112), .fOk = TRUE},
                         {.i = 1, .color = RGB(96, 48, 64), .fOk = TRUE}};
    MSG msg;
    BOOL fMessage;
    int result = 0, i;

    fInteractive = argc == 1;
    if (!fInteractive && (argc != 2 || strcmp(argv[1], "--self-test") != 0)) {
        fprintf(stderr, "usage: %s [--self-test]\n", argv[0]);
        return 2;
    }
    wc.lpfnWndProc = PaintWndProc;
    wc.lpszClassName = "StarsGdiPaint";
    CHECK(RegisterClass(&wc));
    for (i = 0; i < 2; i++) {
        rghwnd[i] = CreateWindow(wc.lpszClassName, i == 0 ? "Stars GDI first" : "Stars GDI second",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, 40 + i * 480, 80, 460, 240,
                                NULL, NULL, NULL, &rgwin[i]);
        CHECK(rghwnd[i]);
    }
    if (fInteractive) {
        while ((fMessage = GetMessage(&msg, NULL, 0, 0)) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        result = fMessage == -1 || !rgwin[0].fOk || !rgwin[1].fOk;
    } else
        result = TestPaint(rghwnd, rgwin);
    StarsCompatShutdown();
    if (result == 0 && !fInteractive)
        puts("Win32 painting: PASS (two windows, clipping, DCs, objects, text, resize and survivor repaint)");
    return result;
}
