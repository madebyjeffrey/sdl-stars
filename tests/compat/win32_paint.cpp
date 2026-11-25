/* The UI path uses Win32 APIs; SDL is only used for renderer observation and
 * native-event injection in the automated test.
 */
#include <limits>
#include <windows.h>
static const RECT rcTest1 = {4, 4, 8, 8};
static const RECT rcTest2 = {-10, -10, 10, 10};
static const RECT rcTest3 = {10, 10, 20, 20};
static const RECT rcTest4 = {15, 15, 30, 30};
static const RECT rcTest5 = {10, 10, 30, 30};
static const RECT rcTest6 = {15, 15, 20, 20};
static const RECT rcTest7 = {0, 0, 100, 100};
static const RECT rcTest8 = {0, 0, 0, 0};
static const RECT rcTest9 = {0, 0, 100, 100};
static const RECT rcTest10 = {0, 0, 0, 0};
#include <stars_compat.h>
#include <SDL3/SDL.h>
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

    win32::GetClientRect(hwnd, &rc);
    hdc = win32::CreateCompatibleDC(hdcDest);
    hbm = win32::CreateCompatibleBitmap(hdcDest, rc.right, rc.bottom);
    if (!hdc || !hbm) {
        win32::DeleteDC(hdc); win32::DeleteObject(hbm);
        return 0;
    }
    hbmOld = win32::SelectObject(hdc, hbm);
    hbr = win32::CreateSolidBrush(lpwin->color);
    hpen = win32::CreatePen(win32::PS_SOLID, 1, win32::RGB(255, 255, 255));
    if (!hbr || !hpen) {
        win32::DeleteObject(hbr); win32::DeleteObject(hpen);
        win32::DeleteDC(hdc); win32::DeleteObject(hbm);
        return 0;
    }
    fOk = win32::FillRect(hdc, &rc, hbr);
    hbrOld = win32::SelectObject(hdc, win32::GetStockObject(win32::NULL_BRUSH));
    hpenOld = win32::SelectObject(hdc, hpen);
    fOk &= win32::Rectangle(hdc, 12, 12, rc.right - 12, rc.bottom - 12);
    fOk &= win32::MoveToEx(hdc, 20, 70, NULL) && win32::LineTo(hdc, rc.right - 20, 70);
    win32::SetTextColor(hdc, win32::RGB(255, 255, 255));
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    cch = snprintf(sz, sizeof(sz), "Window %d - %ld x %ld", lpwin->i + 1, (long)rc.right, (long)rc.bottom);
    fOk &= win32::GetTextExtentPoint32A(hdc, sz, cch, &size);
    fOk &= win32::TextOutA(hdc, (rc.right - size.cx) / 2, 30, sz, cch);
    fOk &= win32::TextOutA(hdc, 24, 90, "Resize me; press a key to change color.", 38);
    fOk &= win32::TextOutA(hdc, 24, 110, "Close one window; the other keeps painting.", 42);
    // Draw an indexed image from the game's real resource atlas in each window.
    hrsrc = win32::FindResourceA(NULL, win32::MAKEINTRESOURCE(87), win32::RT_BITMAP);
    lpbi = (const BITMAPINFOHEADER*)(win32::LockResource(win32::LoadResource(NULL, hrsrc)));
    if (lpbi) {
        cColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1u << lpbi->biBitCount;
        rgb = (const BYTE *)(lpbi + 1) + cColors * sizeof(RGBQUAD);
        fOk &= win32::StretchDIBits(hdc, 24, 140, 64, 64, 0, 0, 64, 64, rgb,
                            (const BITMAPINFO *)lpbi, win32::DIB_RGB_COLORS, win32::SRCCOPY) > 0;
    } else {
        fOk = 0;
    }
    win32::SelectObject(hdc, hpenOld);
    win32::SelectObject(hdc, hbrOld);
    fOk &= win32::DeleteObject(hpen) && win32::DeleteObject(hbr);
    fOk &= win32::BitBlt(hdcDest, 0, 0, rc.right, rc.bottom, hdc, 0, 0, win32::SRCCOPY);
    fOk &= win32::SelectObject(hdc, hbmOld) == hbm;
    fOk &= win32::DeleteDC(hdc) && win32::DeleteObject(hbm);
    return fOk;
}

static LRESULT  PaintWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PAINTTEST *lpwin = (PAINTTEST *)win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA);
    PAINTSTRUCT ps;

    if (msg == win32::WM_NCCREATE) {
        lpwin = (PAINTTEST*)(((CREATESTRUCT *)lParam)->lpCreateParams);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, (LONG_PTR)lpwin);
        return 1;
    }
    switch (msg) {
    case win32::WM_CREATE:
        cLive++;
        return 0;
    case win32::WM_ERASEBKGND:
        if (lpwin->fDestroyOnErase) {
            win32::DestroyWindow(hwnd);
            return 1;
        }
        return win32::DefWindowProcA(hwnd, msg, wParam, lParam);
    case win32::WM_PAINT:
        if (!win32::BeginPaint(hwnd, &ps)) {
            lpwin->fOk = 0;
            return 0;
        }
        lpwin->rcLast = ps.rcPaint;
        lpwin->cPaint++;
        lpwin->fOk &= FDraw(hwnd, ps.hdc, lpwin);
        if (lpwin->fInvalidateInPaint) {
            lpwin->fInvalidateInPaint = 0;
            lpwin->fOk &= win32::InvalidateRect(hwnd, &rcTest1, 0);
        }
        lpwin->fOk &= win32::EndPaint(hwnd, &ps);
        return 0;
    case win32::WM_KEYDOWN:
        lpwin->color ^= win32::RGB(64, 32, 96);
        win32::InvalidateRect(hwnd, NULL, 0);
        return 0;
    case win32::WM_DESTROY:
        if (--cLive == 0 && fInteractive)
            win32::PostQuitMessage(0);
        return 0;
    default:
        return win32::DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

static void Drain(void) {
    win32::MSG msg;

    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE))
        win32::DispatchMessageA(&msg);
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
    win32::MSG msg;
    SDL_Event event = {};
    SDL_Window *lpSDL[2];
    SDL_Renderer *lpRenderer[2];
    int cPaint, x, y, cText;
    COLORREF color;
    PAINTTEST winDestroy = {};
    HWND hwndDestroy;

    winDestroy.fOk = 1;
    winDestroy.fDestroyOnErase = 1;
    Drain();
    CHECK(rgwin[0].fOk && rgwin[1].fOk && rgwin[0].cPaint > 0 && rgwin[1].cPaint > 0);
    lpSDL[0] = LpSDLWindow("Stars GDI first");
    lpSDL[1] = LpSDLWindow("Stars GDI second");
    CHECK(lpSDL[0] && lpSDL[1]);
    lpRenderer[0] = SDL_GetRenderer(lpSDL[0]);
    lpRenderer[1] = SDL_GetRenderer(lpSDL[1]);
    CHECK(lpRenderer[0] && lpRenderer[1] && lpRenderer[0] != lpRenderer[1]);
    printf("GDI renderers: %s / %s\n", SDL_GetRendererName(lpRenderer[0]), SDL_GetRendererName(lpRenderer[1]));
    hdc = win32::GetDC(rghwnd[0]); hdcOther = win32::GetDC(rghwnd[1]);
    CHECK(hdc && hdcOther && hdc != hdcOther);
    CHECK(win32::GetPixel(hdc, 0, 0) == rgwin[0].color && win32::GetPixel(hdcOther, 0, 0) == rgwin[1].color);
    CHECK(win32::GetPixel(hdc, -1, 0) == win32::CLR_INVALID);
    hbr = win32::CreateSolidBrush(win32::RGB(12, 34, 56));
    hpen = win32::CreatePen(win32::PS_SOLID, 1, win32::RGB(220, 180, 40));
    CHECK(hbr && hpen && !win32::CreatePen(99, 1, 0) && !win32::CreatePen(win32::PS_SOLID, 2, 0));
    CHECK(win32::FillRect(hdc, &rcTest2, hbr));
    CHECK(win32::GetPixel(hdc, 0, 0) == win32::RGB(12, 34, 56));
    CHECK(win32::GetPixel(hdc, 9, 9) == win32::RGB(12, 34, 56) && win32::GetPixel(hdc, 10, 10) == rgwin[0].color);
    CHECK(win32::GetPixel(hdcOther, 0, 0) == rgwin[1].color);
    hOld = win32::SelectObject(hdc, hpen); hbrOld = win32::SelectObject(hdc, hbr);
    CHECK(hOld && hbrOld && !win32::DeleteObject(hpen) && !win32::DeleteObject(hbr));
    CHECK(win32::MoveToEx(hdc, 20, 20, &pt) && pt.x == 0 && pt.y == 0);
    CHECK(win32::LineTo(hdc, 30, 20));
    CHECK(win32::GetPixel(hdc, 20, 20) == win32::RGB(220, 180, 40) && win32::GetPixel(hdc, 29, 20) == win32::RGB(220, 180, 40));
    CHECK(win32::GetPixel(hdc, 30, 20) == rgwin[0].color);
    CHECK(win32::MoveToEx(hdc, std::numeric_limits<int>::min(), 180, &pt) && pt.x == 30 && pt.y == 20);
    CHECK(win32::LineTo(hdc, std::numeric_limits<int>::max(), 180) && win32::GetPixel(hdc, 0, 180) == win32::RGB(220, 180, 40));
    CHECK(win32::Rectangle(hdc, 30, 30, 40, 40));
    CHECK(win32::GetPixel(hdc, 30, 30) == win32::RGB(220, 180, 40));
    CHECK(win32::GetPixel(hdc, 31, 31) == win32::RGB(12, 34, 56));
    CHECK(win32::GetPixel(hdc, 40, 40) == rgwin[0].color);
    CHECK(win32::Rectangle(hdc, std::numeric_limits<int>::min(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
    CHECK(win32::MoveToEx(hdc, 0, 0, &pt) && pt.x == std::numeric_limits<int>::max() && pt.y == 180);
    CHECK(win32::SelectObject(hdc, hOld) == hpen && win32::SelectObject(hdc, hbrOld) == hbr);
    CHECK(win32::DeleteObject(hpen) && !win32::DeleteObject(hpen) && win32::DeleteObject(hbr));
    CHECK(!win32::SelectObject(hdc, hpen) && !win32::DeleteObject(win32::GetStockObject(win32::BLACK_PEN)));
    CHECK(win32::GetTextExtentPoint32A(hdc, "Hello", 5, &size) && size.cx > 0 && size.cy > 0);
    CHECK(win32::GetTextExtentPoint32A(hdc, NULL, 0, &size) && size.cx == 0);
    CHECK(!win32::GetTextExtentPoint32A(hdc, "x", std::numeric_limits<int>::max(), &size));
    CHECK(win32::GetTextMetricsA(hdc, &tm) && tm.tmHeight > 0 && tm.tmAscent + tm.tmDescent == tm.tmHeight);
    CHECK(win32::SetTextColor(hdc, win32::RGB(255, 0, 0)) == win32::RGB(0, 0, 0));
    CHECK(win32::SetBkColor(hdc, win32::RGB(0, 0, 255)) == win32::RGB(255, 255, 255));
    CHECK(win32::SetBkMode(hdc, win32::OPAQUE) == win32::OPAQUE && !win32::SetBkMode(hdc, 3));
    CHECK(win32::GetTextExtentPoint32A(hdc, "! ", 2, &size));
    CHECK(win32::TextOutA(hdc, 50, 50, "! ", 2));
    CHECK(win32::GetPixel(hdc, 50 + size.cx - 1, 50 + tm.tmHeight - 1) == win32::RGB(0, 0, 255));
    CHECK(win32::GetPixel(hdc, 49, 50) == win32::RGB(12, 34, 56));
    CHECK(win32::SetBkMode(hdc, win32::TRANSPARENT) == win32::OPAQUE);
    CHECK(win32::TextOutA(hdc, 70, 50, "!", 1));
    CHECK(win32::GetPixel(hdc, 70 + size.cx + 2, 50) == win32::RGB(12, 34, 56));
    CHECK(!win32::TextOutA(hdc, 140, 50, "\xff", 1));
    CHECK(win32::TextOutA(hdc, 140, 50, "\xc3\xbf", 2));
    CHECK(win32::TextOutA(hdc, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), "!", 1));
    cText = 0;
    for (y = 50; y < 50 + tm.tmHeight; y++)
        for (x = 50; x < 50 + size.cx; x++)
            if (win32::GetPixel(hdc, x, y) != win32::RGB(0, 0, 255))
                cText++;
    CHECK(cText > 0 && cText < size.cx * tm.tmHeight);
    CHECK(!win32::ReleaseDC(rghwnd[1], hdc));
    CHECK(win32::ReleaseDC(rghwnd[0], hdc) && win32::ReleaseDC(rghwnd[1], hdcOther));
    CHECK(win32::GetPixel(hdc, 0, 0) == win32::CLR_INVALID && !win32::ReleaseDC(rghwnd[0], hdc));
    hdcNew = win32::GetDC(rghwnd[0]);
    CHECK(hdcNew && hdcNew != hdc && win32::GetPixel(hdc, 0, 0) == win32::CLR_INVALID);
    CHECK(win32::ReleaseDC(rghwnd[0], hdcNew));

    // Paint remains pending until BeginPaint validates; posted messages come first.
    CHECK(win32::InvalidateRect(rghwnd[0], &rcTest3, 0));
    CHECK(win32::InvalidateRect(rghwnd[0], &rcTest4, 1));
    CHECK(win32::GetUpdateRect(rghwnd[0], &rc, 0) && win32::EqualRect(&rc, &rcTest5));
    CHECK(win32::PostMessageA(rghwnd[1], win32::WM_APP, 0, 0));
    CHECK(win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE) && msg.message == win32::WM_APP);
    CHECK(win32::PeekMessageA(&msg, rghwnd[0], win32::WM_PAINT, win32::WM_PAINT, win32::PM_NOREMOVE));
    CHECK(!win32::PeekMessageA(&msg, rghwnd[1], win32::WM_PAINT, win32::WM_PAINT, win32::PM_NOREMOVE));
    CHECK(win32::ValidateRect(rghwnd[0], &rcTest6));
    hdc = win32::BeginPaint(rghwnd[0], &ps);
    CHECK(hdc && ps.fErase && win32::EqualRect(&ps.rcPaint, &rc));
    CHECK(!win32::BeginPaint(rghwnd[0], &psOther) && !win32::GetUpdateRect(rghwnd[0], NULL, 0));
    CHECK(!win32::ReleaseDC(rghwnd[0], hdc) && !win32::EndPaint(rghwnd[1], &ps));
    CHECK(win32::FillRect(hdc, &rcTest7, (HBRUSH)win32::GetStockObject(win32::WHITE_BRUSH)));
    CHECK(!win32::PeekMessageA(&msg, rghwnd[0], win32::WM_PAINT, win32::WM_PAINT, win32::PM_REMOVE));
    CHECK(win32::EndPaint(rghwnd[0], &ps) && !win32::EndPaint(rghwnd[0], &ps));
    hdc = win32::GetDC(rghwnd[0]);
    CHECK(win32::GetPixel(hdc, 10, 10) == win32::RGB(255, 255, 255));
    CHECK(win32::GetPixel(hdc, 9, 9) == win32::RGB(12, 34, 56) && win32::GetPixel(hdc, 30, 30) == win32::RGB(12, 34, 56));
    CHECK(win32::ReleaseDC(rghwnd[0], hdc));
    CHECK(win32::BeginPaint(rghwnd[0], &ps) && !ps.fErase && win32::EqualRect(&ps.rcPaint, &rcTest8));
    CHECK(win32::FillRect(ps.hdc, &rcTest9, (HBRUSH)win32::GetStockObject(win32::BLACK_BRUSH)));
    CHECK(win32::EndPaint(rghwnd[0], &ps));
    CHECK(win32::InvalidateRect(rghwnd[0], NULL, 0) && win32::ValidateRect(rghwnd[0], NULL));
    CHECK(!win32::GetUpdateRect(rghwnd[0], &rc, 0) && win32::EqualRect(&rc, &rcTest10));
    rgwin[0].fInvalidateInPaint = 1;
    cPaint = rgwin[0].cPaint;
    CHECK(win32::InvalidateRect(rghwnd[0], NULL, 0) && win32::UpdateWindow(rghwnd[0]));
    CHECK(rgwin[0].cPaint == cPaint + 1 && win32::GetUpdateRect(rghwnd[0], &rc, 0));
    CHECK(win32::UpdateWindow(rghwnd[0]) && rgwin[0].cPaint == cPaint + 2);
    CHECK(!win32::GetUpdateRect(rghwnd[0], NULL, 0));
    CHECK(win32::MoveWindow(rghwnd[0], 20, 20, 520, 320, 1));
    Drain();
    CHECK(win32::GetClientRect(rghwnd[0], &rc) && rc.right == 520 && rc.bottom == 320);
    hdc = win32::GetDC(rghwnd[0]);
    CHECK(win32::GetPixel(hdc, 519, 319) == rgwin[0].color && win32::ReleaseDC(rghwnd[0], hdc));
    CHECK(rgwin[0].fOk && rgwin[1].fOk);
    cPaint = rgwin[1].cPaint;
    event.type = SDL_EVENT_WINDOW_EXPOSED;
    event.window.windowID = SDL_GetWindowID(lpSDL[1]);
    CHECK(SDL_PushEvent(&event));
    Drain();
    CHECK(rgwin[1].cPaint == cPaint + 1 && rgwin[1].fOk);
    hdcOther = win32::GetDC(rghwnd[0]);
    CHECK(win32::DestroyWindow(rghwnd[0]) && win32::GetPixel(hdcOther, 0, 0) == win32::CLR_INVALID);
    CHECK(!win32::ReleaseDC(rghwnd[0], hdcOther));
    CHECK(cLive == 1 && !win32::UpdateWindow(rghwnd[0]));
    color = rgwin[1].color;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = SDL_GetWindowID(lpSDL[1]);
    event.key.scancode = SDL_SCANCODE_A;
    CHECK(SDL_PushEvent(&event));
    Drain();
    CHECK(rgwin[1].color != color && rgwin[1].cPaint > cPaint && rgwin[1].fOk);
    hdc = win32::GetDC(rghwnd[1]);
    CHECK(win32::GetPixel(hdc, 0, 0) == rgwin[1].color && win32::ReleaseDC(rghwnd[1], hdc));
    CHECK(win32::DestroyWindow(rghwnd[1]) && cLive == 0);
    hwndDestroy = win32::CreateWindowExA(0, "StarsGdiPaint", "Destroy during erase", win32::WS_OVERLAPPEDWINDOW,
                               0, 0, 100, 100, NULL, NULL, NULL, &winDestroy);
    CHECK(hwndDestroy && win32::InvalidateRect(hwndDestroy, NULL, 1));
    CHECK(!win32::BeginPaint(hwndDestroy, &ps) && cLive == 0);
    CHECK(win32::GetPixel(ps.hdc, 0, 0) == win32::CLR_INVALID && !win32::EndPaint(hwndDestroy, &ps));
    return 0;
}

int main(int argc, char **argv) {
    WNDCLASS wc = {};
    HWND rghwnd[2];
    PAINTTEST rgwin[2] = {};
    win32::MSG msg;
    BOOL fMessage;
    int result = 0, i;

    rgwin[0].color = win32::RGB(32, 64, 112);
    rgwin[0].fOk = 1;
    rgwin[1].i = 1;
    rgwin[1].color = win32::RGB(96, 48, 64);
    rgwin[1].fOk = 1;
    fInteractive = argc == 1;
    if (!fInteractive && (argc != 2 || strcmp(argv[1], "--self-test") != 0)) {
        fprintf(stderr, "usage: %s [--self-test]\n", argv[0]);
        return 2;
    }
    wc.lpfnWndProc = PaintWndProc;
    wc.lpszClassName = "StarsGdiPaint";
    CHECK(win32::RegisterClassA(&wc));
    for (i = 0; i < 2; i++) {
        rghwnd[i] = win32::CreateWindowExA(0, wc.lpszClassName, i == 0 ? "Stars GDI first" : "Stars GDI second",
                                win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE, 40 + i * 480, 80, 460, 240,
                                NULL, NULL, NULL, &rgwin[i]);
        CHECK(rghwnd[i]);
    }
    if (fInteractive) {
        while ((fMessage = win32::GetMessageA(&msg, NULL, 0, 0)) > 0) {
            win32::TranslateMessage(&msg);
            win32::DispatchMessageA(&msg);
        }
        result = fMessage == -1 || !rgwin[0].fOk || !rgwin[1].fOk;
    } else
        result = TestPaint(rghwnd, rgwin);
    StarsCompatShutdown();
    if (result == 0 && !fInteractive)
        puts("Win32 painting: PASS (two windows, clipping, DCs, objects, text, resize and survivor repaint)");
    return result;
}
