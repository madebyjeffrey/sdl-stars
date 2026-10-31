/* Window-position contracts compiled against both the SDK and local runtime. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#define flagsQuiet (SWP_NOZORDER | SWP_NOACTIVATE)
#define flagsZ (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)

typedef struct POSITIONTEST {
    int cChanging, cChanged, cMove, cSize, cShow;
    BOOL fAdjust, fSuppress, fDestroyChanging, fDestroyChanged, fNested, fMinmax;
    int cMinmax;
    WINDOWPOS wpChanging, wpChanged;
} POSITIONTEST;

static LRESULT CALLBACK PositionProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    POSITIONTEST *lp = (POSITIONTEST *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    WINDOWPOS *lpwp;
    CREATESTRUCT *lpcs;
    if (msg == WM_NCCREATE) {
        lpcs = (CREATESTRUCT *)l;
        lp = (POSITIONTEST*)(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lp);
    }
    if (!lp) return DefWindowProc(hwnd, msg, w, l);
    switch (msg) {
    case WM_WINDOWPOSCHANGING:
        lpwp = (WINDOWPOS *)l;
        lp->cChanging++; lp->wpChanging = *lpwp;
        if (lp->fAdjust) {
            lpwp->x = 31; lpwp->y = 41; lpwp->cx = 51; lpwp->cy = 61;
            lpwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
        }
        if (lp->fDestroyChanging) DestroyWindow(hwnd);
        return lp->fMinmax ? DefWindowProc(hwnd, msg, w, l) : 0;
    case WM_WINDOWPOSCHANGED:
        lp->cChanged++; lp->wpChanged = *(WINDOWPOS *)l;
        if (lp->fDestroyChanged) { DestroyWindow(hwnd); return 0; }
        if (lp->fNested) {
            lp->fNested = FALSE;
            SetWindowPos(hwnd, NULL, 80, 90, 0, 0, flagsQuiet | SWP_NOSIZE);
        }
        if (lp->fSuppress) return 0;
        return DefWindowProc(hwnd, msg, w, l);
    case WM_GETMINMAXINFO:
        if (lp->fMinmax) {
            MINMAXINFO *lpmmi = (MINMAXINFO *)l;
            lp->cMinmax++;
            lpmmi->ptMinTrackSize = (POINT){200, 210};
            lpmmi->ptMaxTrackSize = (POINT){240, 250};
            return 0;
        }
        return DefWindowProc(hwnd, msg, w, l);
    case WM_MOVE: lp->cMove++; return 0;
    case WM_SIZE: lp->cSize++; return 0;
    case WM_SHOWWINDOW: lp->cShow++; return 0;
    default: return DefWindowProc(hwnd, msg, w, l);
    }
}

static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}

static int TestRect(HWND hwnd, HWND hwndParent, int x, int y, int dx, int dy) {
    RECT rc;
    POINT origin = {0, 0};
    CHECK(GetWindowRect(hwnd, &rc));
    if (hwndParent) CHECK(ClientToScreen(hwndParent, &origin));
    CHECK(rc.left == origin.x + x && rc.top == origin.y + y && rc.right - rc.left == dx && rc.bottom - rc.top == dy);
    CHECK(GetClientRect(hwnd, &rc) && rc.right == dx && rc.bottom == dy);
    return 0;
}

#if !defined(_WIN32) || defined(STARS_UI_SDL)
static COLORREF CrComposite(HWND hwnd, int x, int y) {
    BYTE *rgb;
    // Window moves defer presentation until the whole visible tree finishes painting.
    if (!FCompatPaintTree(hwnd)) return CLR_INVALID;
    rgb = (BYTE *)hwnd->lpComposite->pixels + (size_t)y * hwnd->lpComposite->pitch + (size_t)x * 4;
    return RGB(rgb[0], rgb[1], rgb[2]);
}
static int FillWindow(HWND hwnd, COLORREF cr) {
    HDC hdc = GetDC(hwnd);
    HBRUSH hbr = CreateSolidBrush(cr);
    RECT rc;
    CHECK(hdc && hbr && GetClientRect(hwnd, &rc));
    CHECK(FillRect(hdc, &rc, hbr));
    CHECK(ReleaseDC(hwnd, hdc) && DeleteObject(hbr));
    return 0;
}
static int TestRedraw(HWND hwnd, HWND hwndA, HWND hwndB, HWND hwndC) {
    HDC hdc;
    RECT rc;
    ShowWindow(hwndC, SW_HIDE);
    CHECK(SetWindowPos(hwndA, HWND_TOP, 10, 10, 30, 30, SWP_NOACTIVATE));
    CHECK(SetWindowPos(hwndB, HWND_BOTTOM, 10, 10, 30, 30, SWP_NOACTIVATE));
    CHECK(FillWindow(hwnd, RGB(1, 2, 3)) == 0);
    CHECK(FillWindow(hwndA, RGB(10, 20, 30)) == 0 && FillWindow(hwndB, RGB(40, 50, 60)) == 0);
    CHECK(CrComposite(hwnd, 15, 15) == RGB(10, 20, 30));
    CHECK(SetWindowPos(hwndA, HWND_BOTTOM, 0, 0, 0, 0, flagsZ));
    CHECK(CrComposite(hwnd, 15, 15) == RGB(40, 50, 60));
    CHECK(ValidateRect(hwnd, NULL) && ValidateRect(hwndB, NULL));
    CHECK(SetWindowPos(hwndB, NULL, 60, 60, 0, 0, flagsQuiet | SWP_NOSIZE | SWP_NOREDRAW));
    CHECK(CrComposite(hwnd, 15, 15) == RGB(40, 50, 60));
    CHECK(!GetUpdateRect(hwnd, &rc, FALSE) && !GetUpdateRect(hwndB, &rc, FALSE));
    CHECK(InvalidateRect(hwnd, NULL, FALSE) && UpdateWindow(hwnd));
    CHECK(CrComposite(hwnd, 15, 15) == RGB(10, 20, 30));
    CHECK(CrComposite(hwnd, 65, 65) == RGB(40, 50, 60));
    CHECK(SetWindowPos(hwndA, NULL, 0, 0, 0, 0, flagsQuiet | SWP_NOMOVE));
    hdc = GetDC(hwndA);
    CHECK(hdc && GetPixel(hdc, 0, 0) == CLR_INVALID && ReleaseDC(hwndA, hdc));
    CHECK(CrComposite(hwnd, 15, 15) == RGB(1, 2, 3));
    CHECK(SetWindowPos(hwndA, NULL, 10, 10, 30, 30, flagsQuiet));
    CHECK(GetWindow(hwnd, GW_CHILD) == hwndC && GetWindow(hwndB, GW_HWNDNEXT) == hwndA);
    return 0;
}
#endif

int main(void) {
    WNDCLASS wc = {0};
    HINSTANCE hInstance;
    HWND hwnd, hwndA, hwndB, hwndC, hwndOther, hwndOwned, hwndZero, hwndFailure, hwndInherited, hwndMinmax;
    POSITIONTEST parent = {0}, a = {0}, b = {0}, c = {0}, other = {0}, owned = {0}, zero = {0}, failure = {0}, inherited = {0}, minmax = {0};
    int cChanging, cChanged, cMove, cSize, cShow;
    RECT rc;
    BOOL fWineOwnedMismatch = FALSE;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.lpfnWndProc = PositionProc; wc.hInstance = hInstance; wc.lpszClassName = "StarsPositionContracts";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "position root", WS_POPUP | WS_VISIBLE, 40, 50, 320, 240,
                        NULL, NULL, hInstance, &parent);
    CHECK(hwnd);
    hwndA = CreateWindow(wc.lpszClassName, "A", WS_CHILD | WS_VISIBLE, 10, 10, 30, 30,
                         hwnd, (HMENU)(INT_PTR)1, hInstance, &a);
    hwndB = CreateWindow(wc.lpszClassName, "B", WS_CHILD | WS_VISIBLE, 20, 20, 40, 40,
                         hwnd, (HMENU)(INT_PTR)2, hInstance, &b);
    hwndC = CreateWindow(wc.lpszClassName, "C", WS_CHILD | WS_VISIBLE, 30, 30, 50, 50,
                         hwnd, (HMENU)(INT_PTR)3, hInstance, &c);
    CHECK(hwndA && hwndB && hwndC);
    Drain();
    memset(&a, 0, sizeof(a));
    CHECK(SetWindowPos(hwndA, NULL, 20, 25, 35, 45, flagsQuiet));
    CHECK(TestRect(hwndA, hwnd, 20, 25, 35, 45) == 0);
    CHECK(a.cChanging == 1 && a.cChanged == 1 && a.cMove == 1 && a.cSize == 1);
    CHECK(a.wpChanging.hwnd == hwndA && a.wpChanging.x == 20 && a.wpChanged.cy == 45);
    cMove = a.cMove; cSize = a.cSize;
    CHECK(SetWindowPos(hwndA, NULL, -100, -100, 55, 65, flagsQuiet | SWP_NOMOVE));
    CHECK(TestRect(hwndA, hwnd, 20, 25, 55, 65) == 0 && a.cMove == cMove && a.cSize == cSize + 1);
    cSize = a.cSize;
    CHECK(SetWindowPos(hwndA, NULL, 22, 27, -100, -100, flagsQuiet | SWP_NOSIZE));
    CHECK(TestRect(hwndA, hwnd, 22, 27, 55, 65) == 0 && a.cSize == cSize);
    a.fAdjust = TRUE;
    CHECK(SetWindowPos(hwndA, NULL, 1, 2, 3, 4, flagsQuiet | SWP_NOMOVE | SWP_NOSIZE));
    CHECK(TestRect(hwndA, hwnd, 31, 41, 51, 61) == 0);
    a.fAdjust = FALSE; a.fSuppress = TRUE;
    cMove = a.cMove; cSize = a.cSize;
    CHECK(SetWindowPos(hwndA, NULL, 32, 42, 52, 62, flagsQuiet));
    CHECK(a.cMove == cMove && a.cSize == cSize && TestRect(hwndA, hwnd, 32, 42, 52, 62) == 0);
    a.fSuppress = FALSE;
    cChanging = a.cChanging;
    CHECK(SetWindowPos(hwndA, NULL, 33, 43, 53, 63, flagsQuiet | SWP_NOSENDCHANGING));
    CHECK(a.cChanging == cChanging && TestRect(hwndA, hwnd, 33, 43, 53, 63) == 0);
    cChanged = a.cChanged;
    CHECK(SetWindowPos(hwndA, NULL, 0, 0, 0, 0, flagsQuiet | SWP_NOMOVE | SWP_NOSIZE));
    CHECK(a.cChanged == cChanged);
    CHECK(GetWindow(hwnd, GW_CHILD) == hwndA);
    CHECK(SetWindowPos(hwndC, HWND_TOP, 0, 0, 0, 0, flagsZ));
    CHECK(GetWindow(hwnd, GW_CHILD) == hwndC && GetWindow(hwndC, GW_HWNDNEXT) == hwndA);
    CHECK(SetWindowPos(hwndC, HWND_BOTTOM, 0, 0, 0, 0, flagsZ));
    CHECK(GetWindow(hwndC, GW_HWNDPREV) == hwndB && GetWindow(hwndA, GW_HWNDLAST) == hwndC);
    CHECK(SetWindowPos(hwndA, hwndB, 0, 0, 0, 0, flagsZ));
    CHECK(GetWindow(hwnd, GW_CHILD) == hwndB && GetWindow(hwndB, GW_HWNDNEXT) == hwndA);
    CHECK(SetWindowPos(hwndA, hwndA, 0, 0, 0, 0, flagsZ));
    CHECK(GetWindow(hwndB, GW_HWNDNEXT) == hwndA);
    cShow = a.cShow;
    CHECK(SetWindowPos(hwndA, NULL, 0, 0, 0, 0, flagsQuiet | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW));
    CHECK(!IsWindowVisible(hwndA) && a.cShow == cShow);
    CHECK(SetWindowPos(hwndA, NULL, 0, 0, 0, 0, flagsQuiet | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));
    CHECK(IsWindowVisible(hwndA) && a.cShow == cShow);
    hwndZero = CreateWindow(wc.lpszClassName, "zero child", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                            hwnd, (HMENU)(INT_PTR)4, hInstance, &zero);
    CHECK(hwndZero && GetClientRect(hwndZero, &rc) && rc.right == 0 && rc.bottom == 0);
    CHECK(SetWindowPos(hwndZero, NULL, 5, 6, -1, -2, flagsQuiet));
    CHECK(TestRect(hwndZero, hwnd, 5, 6, 0, 0) == 0);
    CHECK(SetWindowPos(hwndZero, NULL, 5, 6, 10, 0, flagsQuiet));
    CHECK(TestRect(hwndZero, hwnd, 5, 6, 10, 0) == 0 && DestroyWindow(hwndZero));
    a.fNested = TRUE;
    CHECK(SetWindowPos(hwndA, NULL, 50, 60, 0, 0, flagsQuiet | SWP_NOSIZE));
    CHECK(TestRect(hwndA, hwnd, 80, 90, 53, 63) == 0);
    hwndFailure = CreateWindow(wc.lpszClassName, "destroy changing", WS_CHILD, 1, 1, 10, 10,
                               hwnd, NULL, hInstance, &failure);
    CHECK(hwndFailure); failure.fDestroyChanging = TRUE;
    SetWindowPos(hwndFailure, NULL, 2, 2, 20, 20, flagsQuiet);
    CHECK(!IsWindow(hwndFailure));
    memset(&failure, 0, sizeof(failure));
    hwndFailure = CreateWindow(wc.lpszClassName, "destroy changed", WS_CHILD, 1, 1, 10, 10,
                               hwnd, NULL, hInstance, &failure);
    CHECK(hwndFailure); failure.fDestroyChanged = TRUE;
    SetWindowPos(hwndFailure, NULL, 2, 2, 20, 20, flagsQuiet);
    CHECK(!IsWindow(hwndFailure));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestRedraw(hwnd, hwndA, hwndB, hwndC) == 0);
#endif
    hwndOther = CreateWindow(wc.lpszClassName, "other root", WS_POPUP, 0, 0, 50, 50, NULL, NULL, hInstance, &other);
    CHECK(hwndOther);
    CHECK(SetWindowPos(hwndOther, HWND_TOPMOST, 0, 0, 0, 0, flagsZ));
    CHECK(GetWindowLongPtr(hwndOther, GWL_EXSTYLE) & WS_EX_TOPMOST);
    hwndInherited = CreateWindow(wc.lpszClassName, "inherited band", WS_POPUP, 0, 0, 10, 10,
                                 hwndOther, NULL, hInstance, &inherited);
    CHECK(hwndInherited && (GetWindowLongPtr(hwndInherited, GWL_EXSTYLE) & WS_EX_TOPMOST));
    CHECK(GetWindow(hwndOther, GW_HWNDPREV) == hwndInherited);
    CHECK(DestroyWindow(hwndInherited));
    CHECK(SetWindowPos(hwndOther, HWND_NOTOPMOST, 0, 0, 0, 0, flagsZ));
    CHECK(!(GetWindowLongPtr(hwndOther, GWL_EXSTYLE) & WS_EX_TOPMOST));
    hwndOwned = CreateWindow(wc.lpszClassName, "owned root", WS_POPUP, 0, 0, 40, 40, hwnd, NULL, hInstance, &owned);
    CHECK(hwndOther && hwndOwned);
    CHECK(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, flagsZ));
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    if (!(GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
        CHECK(GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version"));
        CHECK(GetWindowLongPtr(hwndOwned, GWL_EXSTYLE) & WS_EX_TOPMOST);
        fWineOwnedMismatch = TRUE;
        fprintf(stderr, "Wine owner-family difference: owned popup becomes topmost while owner remains normal\n");
    }
#endif
    if (!fWineOwnedMismatch) CHECK(GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
    CHECK(GetWindowLongPtr(hwndOwned, GWL_EXSTYLE) & WS_EX_TOPMOST);
    CHECK(GetWindow(hwnd, GW_HWNDPREV) == hwndOwned);
    CHECK(SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, flagsZ));
    CHECK(!(GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST));
    if (!fWineOwnedMismatch) CHECK(!(GetWindowLongPtr(hwndOwned, GWL_EXSTYLE) & WS_EX_TOPMOST));
    else fprintf(stderr, "Wine owner-family demotion: owned topmost=%d\n", !!(GetWindowLongPtr(hwndOwned, GWL_EXSTYLE) & WS_EX_TOPMOST));
    CHECK(GetWindow(hwnd, GW_HWNDPREV) == hwndOwned);
    CHECK(SetWindowPos(hwndOther, NULL, 90, 100, 70, 80, flagsQuiet));
    CHECK(TestRect(hwndOther, NULL, 90, 100, 70, 80) == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    {
        SDL_Event event = {0};
        MSG msg;
        HDC hdc;
        CHECK(SetWindowPos(hwndOther, NULL, 90, 100, 0, 0, flagsQuiet));
        CHECK(GetClientRect(hwndOther, &rc) && rc.right == 0 && rc.bottom == 0);
        Drain();
        event.type = SDL_EVENT_WINDOW_RESIZED;
        event.window.windowID = hwndOther->id; event.window.data1 = 1; event.window.data2 = 1;
        CompatRouteEvent(&event);
        CHECK(PeekMessage(&msg, hwndOther, WM_SIZE, WM_SIZE, PM_REMOVE));
        CHECK(LOWORD(msg.lParam) == 0 && HIWORD(msg.lParam) == 0);
        event.window.data1 = 70; event.window.data2 = 80; /* An older queued physical resize. */
        CompatRouteEvent(&event);
        CHECK(PeekMessage(&msg, hwndOther, WM_SIZE, WM_SIZE, PM_REMOVE));
        CHECK(LOWORD(msg.lParam) == 0 && HIWORD(msg.lParam) == 0);
        hdc = GetDC(hwndOther);
        CHECK(hdc && GetPixel(hdc, 0, 0) == CLR_INVALID && ReleaseDC(hwndOther, hdc));
        event.type = SDL_EVENT_WINDOW_MOVED; event.window.data1 = -12; event.window.data2 = -34;
        CompatRouteEvent(&event);
        CHECK(PeekMessage(&msg, hwndOther, WM_MOVE, WM_MOVE, PM_REMOVE));
        CHECK((int16_t)LOWORD(msg.lParam) == -12 && (int16_t)HIWORD(msg.lParam) == -34);
    }
#endif
    CHECK(MoveWindow(hwndOther, 100, 110, 80, 90, FALSE));
    CHECK(TestRect(hwndOther, NULL, 100, 110, 80, 90) == 0);
    minmax.fMinmax = TRUE;
    hwndMinmax = CreateWindow(wc.lpszClassName, "bounded window", WS_POPUP | WS_THICKFRAME,
                              10, 10, 220, 230, NULL, NULL, hInstance, &minmax);
    CHECK(hwndMinmax);
    minmax.cMinmax = 0;
    CHECK(SetWindowPos(hwndMinmax, NULL, 10, 10, 50, 60, flagsQuiet));
    CHECK(minmax.cMinmax != 0 && GetWindowRect(hwndMinmax, &rc));
    CHECK(rc.right - rc.left == 200 && rc.bottom - rc.top == 210);
    CHECK(SetWindowPos(hwndMinmax, NULL, 10, 10, 500, 500, flagsQuiet));
    CHECK(GetWindowRect(hwndMinmax, &rc) && rc.right - rc.left == 240 && rc.bottom - rc.top == 250);
    CHECK(DestroyWindow(hwndMinmax));
    CHECK(DestroyWindow(hwndOther) && DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 positioning contracts passed");
    return 0;
}
