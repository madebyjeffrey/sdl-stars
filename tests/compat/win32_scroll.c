/* Scroll positions/ranges are checked against the Windows SDK too. */
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static BOOL fDestroy;
static UINT msgLast;
static UINT rgsb[32], rgmsg[32];
static int rgpos[32], cScroll;
static HWND hwndNotify;
#ifndef _WIN32
static HBRUSH hbrTrack;
static BOOL fDestroyNotify, fDestroyColor;
#endif
static WPARAM wLast;
static LPARAM lLast;
static LRESULT CALLBACK ScrollProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_HSCROLL || msg == WM_VSCROLL) {
        if (cScroll < 32) {
            rgsb[cScroll] = LOWORD(w); rgpos[cScroll] = HIWORD(w); rgmsg[cScroll++] = msg;
        }
        hwndNotify = (HWND)l;
#ifndef _WIN32
        if (fDestroyNotify) { fDestroyNotify = FALSE; DestroyWindow((HWND)l); }
#endif
        return 0;
    }
#ifndef _WIN32
    if (msg == WM_CTLCOLORSCROLLBAR) {
        if (fDestroyColor) { fDestroyColor = FALSE; DestroyWindow((HWND)l); }
        return (LRESULT)hbrTrack;
    }
#endif
    if (msg >= SBM_SETPOS && msg <= SBM_SETRANGEREDRAW) {
        msgLast = msg; wLast = w; lLast = l;
        if (fDestroy) { fDestroy = FALSE; DestroyWindow(hwnd); }
        return 777;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static int TestAxis(HWND hwnd, int sb) {
    int iMin, iMax;
    CHECK(GetScrollPos(hwnd, sb) == 0);
    CHECK(GetScrollRange(hwnd, sb, &iMin, &iMax));
    CHECK(iMin == 0 && iMax == (sb == SB_CTL ? 0 : 100));
    CHECK(SetScrollRange(hwnd, sb, -10, 10, FALSE));
    CHECK(SetScrollPos(hwnd, sb, 8, FALSE) == 0 && GetScrollPos(hwnd, sb) == 8);
    CHECK(SetScrollPos(hwnd, sb, 100, TRUE) == 8 && GetScrollPos(hwnd, sb) == 10);
    CHECK(SetScrollPos(hwnd, sb, -100, FALSE) == 10 && GetScrollPos(hwnd, sb) == -10);
    CHECK(SetScrollRange(hwnd, sb, 3, 7, FALSE) && GetScrollPos(hwnd, sb) == 3);
    CHECK(SetScrollRange(hwnd, sb, 12, 12, TRUE) && GetScrollPos(hwnd, sb) == 12);
    CHECK(SetScrollRange(hwnd, sb, 10, -10, FALSE));
    CHECK(GetScrollRange(hwnd, sb, &iMin, &iMax) && iMin == 0 && iMax == 0 && GetScrollPos(hwnd, sb) == 0);
    CHECK(SetScrollRange(hwnd, sb, 0, 100000, FALSE));
    CHECK(SetScrollPos(hwnd, sb, 65537, FALSE) == 0 && GetScrollPos(hwnd, sb) == 65537);
#ifndef _WIN32
    CHECK(!SetScrollRange(hwnd, sb, INT_MIN, INT_MAX, FALSE));
    CHECK(GetScrollRange(hwnd, sb, &iMin, &iMax) && iMin == 0 && iMax == 100000);
    CHECK(GetScrollPos(hwnd, sb) == 65537);
#endif
    return 0;
}
static int TestPage(HWND hwnd) {
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, -10, 100, 20, 100, 0};
    RECT rc, rcBefore;
    ShowWindow(hwnd, SW_SHOW);
    CHECK(ShowScrollBar(hwnd, SB_VERT, TRUE));
    GetClientRect(hwnd, &rcBefore);
    CHECK(SetScrollInfo(hwnd, SB_VERT, &si, TRUE) == 81);
    si.fMask = SIF_ALL; CHECK(GetScrollInfo(hwnd, SB_VERT, &si));
    CHECK(si.nMin == -10 && si.nMax == 100 && si.nPage == 20 && si.nPos == 81 && si.nTrackPos == 81);
    CHECK(SetScrollPos(hwnd, SB_VERT, 200, TRUE) == 81 && GetScrollPos(hwnd, SB_VERT) == 81);
    CHECK(ShowScrollBar(hwnd, SB_VERT, FALSE));
    CHECK(GetClientRect(hwnd, &rc) && rc.right > rcBefore.right && rc.bottom == rcBefore.bottom);
    CHECK(ShowScrollBar(hwnd, SB_VERT, TRUE));
    CHECK(GetClientRect(hwnd, &rc) && rc.right == rcBefore.right);
#ifndef _WIN32
    CHECK(IsWindow(hwnd->rghwndScroll[SB_VERT]) && IsWindowVisible(hwnd->rghwndScroll[SB_VERT]));
    ValidateRect(hwnd, NULL);
    CHECK(SetScrollPos(hwnd, SB_VERT, 20, TRUE) == 81);
    CHECK(!GetUpdateRect(hwnd, NULL, FALSE) && GetUpdateRect(hwnd->rghwndScroll[SB_VERT], NULL, FALSE));
    ValidateRect(hwnd->rghwndScroll[SB_VERT], NULL);
    CHECK(SetScrollPos(hwnd, SB_VERT, 21, FALSE) == 20);
    CHECK(!GetUpdateRect(hwnd, NULL, FALSE) && !GetUpdateRect(hwnd->rghwndScroll[SB_VERT], NULL, FALSE));
    CHECK(SetScrollPos(hwnd, SB_VERT, 81, TRUE) == 21);
    cScroll = 0;
    SendMessage(hwnd->rghwndScroll[SB_VERT], WM_LBUTTONDOWN, 0, MAKELPARAM(8, 5));
    CHECK(cScroll == 1 && rgmsg[0] == WM_VSCROLL && hwndNotify == NULL);
    SendMessage(hwnd->rghwndScroll[SB_VERT], WM_LBUTTONUP, 0, MAKELPARAM(8, 5));
#endif
    si.fMask = SIF_PAGE | SIF_DISABLENOSCROLL; si.nPage = 1000;
    CHECK(SetScrollInfo(hwnd, SB_VERT, &si, TRUE) == -10);
    si.fMask = SIF_ALL; CHECK(GetScrollInfo(hwnd, SB_VERT, &si) && si.nPage == 111 && si.nPos == -10);
#ifndef _WIN32
    CHECK(IsWindowVisible(hwnd->rghwndScroll[SB_VERT]) && !IsWindowEnabled(hwnd->rghwndScroll[SB_VERT]));
#endif
    return 0;
}
#ifndef _WIN32
static int TestCorner(void) {
    HWND hwnd, hwndChild, hwndTest;
    RECT rc;
    HDC hdc;
    COLORREF cr = GetSysColor(COLOR_BTNFACE);
    Uint8 r, g, b;
    int i, x, y;

    hwnd = CreateWindow("StarsScrollContracts", "corner", WS_POPUP | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                        40, 50, 200, 160, NULL, NULL, (HINSTANCE)(UINT_PTR)1, NULL);
    CHECK(hwnd);
    hwndChild = CreateWindow("StarsScrollContracts", "", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                             20, 20, 100, 80, hwnd, NULL, (HINSTANCE)(UINT_PTR)1, NULL);
    CHECK(hwndChild);
    for (i = 0; i < 2; i++) {
        hwndTest = i == 0 ? hwnd : hwndChild;
        hdc = GetDC(hwndTest); CHECK(hdc);
        GetClientRect(hwndTest, &rc);
        CHECK(PatBlt(hdc, 0, 0, rc.right, rc.bottom, BLACKNESS));
        ReleaseDC(hwndTest, hdc);
        CHECK(FCompatPresent(hwnd));
        x = rc.right + 8; y = rc.bottom + 8;
        if (i != 0) { x += hwndChild->rcChild.left; y += hwndChild->rcChild.top; }
        CHECK(SDL_ReadSurfacePixel(hwnd->lpComposite, x, y, &r, &g, &b, NULL));
        CHECK(RGB(r, g, b) == cr);
    }
    CHECK(DestroyWindow(hwnd));
    return 0;
}
static int TestThumbProportion(HWND hwndParent) {
    HWND hwnd;
    HDC hdc;
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 99, 20, 0, 0};
    int i, y, cThumb;
    MSG msg;
    SDL_Event event = {0};

    hwnd = CreateWindow("ScrollBar", "", WS_CHILD | WS_VISIBLE | SBS_VERT,
                        0, 0, 17, 120, hwndParent, NULL, NULL, NULL);
    CHECK(hwnd);
    hbrTrack = CreateSolidBrush(RGB(23, 45, 67)); CHECK(hbrTrack);
    for (i = 0; i < 2; i++) {
        si.nPage = i == 0 ? 20 : 50;
        CHECK(SetScrollInfo(hwnd, SB_CTL, &si, TRUE) == 0 && UpdateWindow(hwnd));
        hdc = GetDC(hwnd); CHECK(hdc); cThumb = 0;
        for (y = 17; y < 103; y++)
            if (GetPixel(hdc, 8, y) != RGB(23, 45, 67)) cThumb++;
        ReleaseDC(hwnd, hdc);
        CHECK(cThumb == (int)(86 * si.nPage / 100));
    }
    /* Inspect the canvas before release or message draining: tracking must
       repaint immediately even while more mouse messages are pending. */
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(8, 30));
    SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(8, 80));
    CHECK(GetCapture() == hwnd && GetScrollPos(hwnd, SB_CTL) == 0);
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(GetPixel(hdc, 8, 30) == RGB(23, 45, 67));
    CHECK(GetPixel(hdc, 8, 80) != RGB(23, 45, 67)); ReleaseDC(hwnd, hdc);
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(8, 80));
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(8, 30));
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = HwndCompatRoot(hwnd)->id;
    event.motion.state = SDL_BUTTON_LMASK; event.motion.x = 8;
    for (i = 31; i <= 80; i++) {
        event.motion.y = i; CHECK(SDL_PushEvent(&event));
    }
    CHECK(PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK((short)HIWORD(msg.lParam) == 80);
    DispatchMessage(&msg);
    CHECK(!PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(GetCapture() == hwnd && hwnd->iScrollTrack == 50);
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(8, 80));
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(8, 30));
    fDestroyColor = TRUE;
    SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(8, 80));
    CHECK(!IsWindow(hwnd) && HwndCompatRoot(hwndParent)->cPresentDefer == 0);
    CHECK(DeleteObject(hbrTrack)); hbrTrack = NULL;
    return 0;
}
#endif
static int TestKeys(HWND hwnd, UINT msg) {
    static const UINT rgvk[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_PRIOR, VK_NEXT, VK_HOME, VK_END};
    static const UINT rgsbExpect[] = {SB_LINEUP, SB_LINEDOWN, SB_LINEUP, SB_LINEDOWN,
                                    SB_PAGEUP, SB_PAGEDOWN, SB_TOP, SB_BOTTOM};
    unsigned i;
    ShowWindow(GetParent(hwnd), SW_SHOW); ShowWindow(hwnd, SW_SHOW); SetFocus(hwnd);
    CHECK(SetScrollRange(hwnd, SB_CTL, 0, 100, FALSE));
    SetScrollPos(hwnd, SB_CTL, 50, FALSE);
    CHECK(SendMessage(hwnd, WM_GETDLGCODE, 0, 0) & DLGC_WANTARROWS);
    for (i = 0; i < sizeof(rgvk) / sizeof(rgvk[0]); i++) {
        cScroll = 0;
        SendMessage(hwnd, WM_KEYDOWN, rgvk[i], 0);
        SendMessage(hwnd, WM_KEYUP, rgvk[i], 0);
        CHECK(cScroll == 1 && rgsb[0] == rgsbExpect[i]);
        CHECK(rgmsg[0] == msg && hwndNotify == hwnd && GetScrollPos(hwnd, SB_CTL) == 50);
    }
    return 0;
}
#ifndef _WIN32
static int TestInput(HWND hwnd) {
    HWND hwndParent = GetParent(hwnd), hwndOther;
    HDC hdc;
    SDL_Event event = {0};
    MSG msg;
    cScroll = 0;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 5));
    CHECK(GetCapture() == hwnd && cScroll == 1 && rgsb[0] == SB_LINEUP);
    CHECK(hwnd->idScrollTimer != 0);
    CHECK(GetMessage(&msg, hwnd, WM_TIMER, WM_TIMER) > 0);
    DispatchMessage(&msg);
    CHECK(cScroll == 2 && rgsb[1] == SB_LINEUP);
    SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(30, 50));
    SendMessage(hwnd, WM_TIMER, hwnd->idScrollTimer, 0);
    CHECK(cScroll == 2);
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(8, 5));
    CHECK(!GetCapture() && cScroll == 3 && rgsb[2] == SB_ENDSCROLL && !hwnd->idScrollTimer);
    cScroll = 0;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 35));
    SendMessage(hwnd, WM_CANCELMODE, 0, 0);
    CHECK(cScroll == 2 && rgsb[0] == SB_PAGEUP && rgsb[1] == SB_ENDSCROLL && !GetCapture());
    cScroll = 0;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 55));
    SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(8, 118));
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(8, 118));
    CHECK(cScroll == 4 && rgsb[0] == SB_THUMBTRACK && rgpos[0] == 50);
    CHECK(rgsb[1] == SB_THUMBTRACK && rgpos[1] == 100);
    CHECK(rgsb[2] == SB_THUMBPOSITION && rgpos[2] == 100 && rgsb[3] == SB_ENDSCROLL);
    CHECK(GetScrollPos(hwnd, SB_CTL) == 50 && !GetCapture());
    hwndOther = CreateWindow("SCROLLBAR", "", WS_CHILD, 20, 0, 120, 17,
                             hwndParent, NULL, (HINSTANCE)(UINT_PTR)1, NULL);
    CHECK(hwndOther);
    cScroll = 0;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 5));
    SetCapture(hwndOther);
    CHECK(cScroll == 2 && rgsb[1] == SB_ENDSCROLL && GetCapture() == hwndOther);
    ReleaseCapture(); DestroyWindow(hwndOther);
    EnableWindow(hwnd, FALSE); cScroll = 0;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 5));
    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0);
    CHECK(cScroll == 0 && !GetCapture());
    EnableWindow(hwnd, TRUE);
    hbrTrack = CreateSolidBrush(RGB(23, 45, 67)); CHECK(hbrTrack);
    ShowWindow(hwndParent, SW_SHOW); ShowWindow(hwnd, SW_SHOW);
    InvalidateRect(hwnd, NULL, FALSE); CHECK(UpdateWindow(hwnd));
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(GetPixel(hdc, 8, 30) == RGB(23, 45, 67)); ReleaseDC(hwnd, hdc);
    SetScrollPos(hwnd, SB_CTL, 0, TRUE); CHECK(UpdateWindow(hwnd));
    hdc = GetDC(hwnd); CHECK(hdc);
    CHECK(GetPixel(hdc, 8, 30) != RGB(23, 45, 67)); ReleaseDC(hwnd, hdc);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    cScroll = 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = hwndParent->id; event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 8; event.button.y = 5;
    CHECK(SDL_PushEvent(&event));
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    CHECK(cScroll == 1 && rgsb[0] == SB_LINEUP && hwndNotify == hwnd && GetCapture() == hwnd);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; CHECK(SDL_PushEvent(&event));
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    CHECK(cScroll == 2 && rgsb[1] == SB_ENDSCROLL && !GetCapture());
    fDestroyColor = TRUE;
    InvalidateRect(hwnd, NULL, FALSE); UpdateWindow(hwnd);
    CHECK(!IsWindow(hwnd));
    CHECK(DeleteObject(hbrTrack)); hbrTrack = NULL;
    hwnd = CreateWindow("SCROLLBAR", "", WS_CHILD | SBS_VERT, 0, 0, 17, 120,
                        hwndParent, NULL, (HINSTANCE)(UINT_PTR)1, NULL);
    CHECK(hwnd && SetScrollRange(hwnd, SB_CTL, 0, 100, FALSE));
    fDestroyNotify = TRUE;
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(8, 5));
    CHECK(!IsWindow(hwnd) && !GetCapture());
    return 0;
}
#endif
static int TestEmptyPaint(HWND hwndParent, HINSTANCE hInstance) {
    HWND hwnd;
    int i;
    hwnd = CreateWindow("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_VERT,
                        20, 20, 17, 120, hwndParent, NULL, hInstance, NULL);
    CHECK(hwnd);
    for (i = 0; i < 2; i++) {
        CHECK(SetWindowPos(hwnd, NULL, 0, 0, 17, 120, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
        CHECK(InvalidateRect(hwnd, NULL, FALSE) && GetUpdateRect(hwnd, NULL, FALSE));
        CHECK(SetWindowPos(hwnd, NULL, 0, 0, i == 0 ? 0 : 17, i == 0 ? 120 : 0,
                           SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
        SendMessage(hwnd, WM_PAINT, 0, 0);
        CHECK(!GetUpdateRect(hwnd, NULL, FALSE));
    }
    CHECK(SetWindowPos(hwnd, NULL, 0, 0, 17, 120, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    CHECK(InvalidateRect(hwnd, NULL, FALSE) && UpdateWindow(hwnd) && !GetUpdateRect(hwnd, NULL, FALSE));
    CHECK(DestroyWindow(hwnd));
    return 0;
}
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndControl, hwndPeer, hwndHorizontal;
    HINSTANCE hInstance;
#ifndef _WIN32
    int iMin = 91, iMax = 92;
#endif
#ifdef _WIN32
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.hbrBackground = (HBRUSH)(INT_PTR)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "StarsScrollContracts"; wc.hInstance = hInstance; wc.lpfnWndProc = ScrollProc;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "scroll", WS_POPUP | WS_HSCROLL | WS_VSCROLL,
                        40, 50, 320, 240, NULL, NULL, hInstance, NULL);
    hwndPeer = CreateWindow(wc.lpszClassName, "peer", WS_POPUP | WS_HSCROLL,
                            400, 50, 320, 240, NULL, NULL, hInstance, NULL);
    CHECK(hwnd && hwndPeer);
    hwndControl = CreateWindow("SCROLLBAR", "", WS_CHILD | SBS_VERT,
                               0, 0, 17, 120, hwnd, (HMENU)(INT_PTR)101, hInstance, NULL);
    CHECK(hwndControl);
    CHECK(TestAxis(hwnd, SB_HORZ) == 0 && TestAxis(hwnd, SB_VERT) == 0 && TestAxis(hwndControl, SB_CTL) == 0);
    CHECK(TestKeys(hwndControl, WM_VSCROLL) == 0);
    hwndHorizontal = CreateWindow("SCROLLBAR", "", WS_CHILD, 20, 0, 120, 17,
                                  hwnd, NULL, hInstance, NULL);
    CHECK(hwndHorizontal && TestKeys(hwndHorizontal, WM_HSCROLL) == 0);
#ifndef _WIN32
    CHECK(TestInput(hwndControl) == 0);
    CHECK(TestThumbProportion(hwnd) == 0);
    CHECK(TestCorner() == 0);
#endif
    CHECK(TestEmptyPaint(hwnd, hInstance) == 0);
    CHECK(TestPage(hwndPeer) == 0);
    CHECK(GetScrollPos(hwndPeer, SB_HORZ) == 0);
    CHECK(GetScrollPos(hwnd, SB_CTL) == 777 && msgLast == SBM_GETPOS);
#ifndef _WIN32
    CHECK(SetScrollPos(hwnd, SB_CTL, -34, TRUE) == 777 && msgLast == SBM_SETPOS);
    CHECK((INT_PTR)wLast == -34 && lLast == TRUE);
    CHECK(SetScrollRange(hwnd, SB_CTL, -12, 56, TRUE) && msgLast == SBM_SETRANGEREDRAW);
    CHECK((INT_PTR)wLast == -12 && lLast == 56);
    CHECK(!GetScrollRange(hwnd, 99, &iMin, &iMax) && iMin == 91 && iMax == 92);
    CHECK(!GetScrollRange(hwnd, SB_HORZ, NULL, &iMax));
    CHECK(!SetScrollRange(hwnd, 99, 0, 10, FALSE));
    CHECK(SetScrollPos(hwnd, 99, 10, FALSE) == 0 && GetLastError() == ERROR_INVALID_PARAMETER);
    fDestroy = TRUE;
    CHECK(SetScrollPos(hwndPeer, SB_CTL, 7, FALSE) == 777 && !IsWindow(hwndPeer));
    CHECK(GetScrollPos(hwndPeer, SB_HORZ) == 0 && GetLastError() == ERROR_INVALID_WINDOW_HANDLE);
#else
    CHECK(DestroyWindow(hwndPeer));
#endif
    CHECK(DestroyWindow(hwnd));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Win32 scroll state contracts passed");
    return 0;
}
