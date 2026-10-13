/* Window hierarchy contracts shared with the Windows SDK. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#define msgChild (WM_APP + 47)

typedef struct CHILDTEST {
    HWND hwndParent, hwndChild;
    int id, cDestroy, cNcDestroy, iDestroy, iNcDestroy, cMove, cSize, cEnable, cFocus, cBlur;
    BOOL fCreateData, fChildrenAlive, fReject, fDestroyParent, fDestroyOnMove;
} CHILDTEST;
static int iMessage;

static LRESULT CALLBACK ChildProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    CHILDTEST *lp = (CHILDTEST *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    CREATESTRUCT *lpcs;
    if (msg == WM_NCCREATE) {
        lpcs = (CREATESTRUCT *)l;
        lp = lpcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lp);
        lp->fCreateData = lpcs->hwndParent == lp->hwndParent && (INT_PTR)lpcs->hMenu == lp->id;
    }
    if (!lp) return DefWindowProc(hwnd, msg, w, l);
    switch (msg) {
    case WM_CREATE: return lp->fReject ? -1 : 0;
    case WM_MOVE:
        lp->cMove++;
        if (lp->fDestroyOnMove) DestroyWindow(hwnd);
        return 0;
    case WM_SIZE: lp->cSize++; return 0;
    case WM_ENABLE: lp->cEnable++; return 0;
    case WM_SETFOCUS: lp->cFocus++; return 0;
    case WM_KILLFOCUS: lp->cBlur++; return 0;
    case WM_DESTROY:
        lp->cDestroy++;
        lp->iDestroy = ++iMessage;
        lp->fChildrenAlive = !lp->hwndChild || IsWindow(lp->hwndChild);
        if (lp->fDestroyParent) DestroyWindow(lp->hwndParent);
        return 0;
    case WM_NCDESTROY: lp->cNcDestroy++; lp->iNcDestroy = ++iMessage; return 0;
    default: return DefWindowProc(hwnd, msg, w, l);
    }
}

static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}

static int FillWindow(HWND hwnd, COLORREF cr) {
    RECT rc;
    HDC hdc = GetDC(hwnd);
    HBRUSH hbr = CreateSolidBrush(cr);
    CHECK(hdc && hbr && GetClientRect(hwnd, &rc));
    CHECK(FillRect(hdc, &rc, hbr));
    CHECK(ReleaseDC(hwnd, hdc) && DeleteObject(hbr));
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

static int TestComposition(HWND hwnd, HWND hwndChild, HWND hwndNested, HWND hwndSibling) {
    SDL_Event event = {0};
    MSG msg;
    SDL_Window **rglp;
    int cWindows;
    CHECK(FillWindow(hwnd, RGB(1, 2, 3)) == 0);
    CHECK(FillWindow(hwndChild, RGB(10, 20, 30)) == 0);
    CHECK(FillWindow(hwndNested, RGB(40, 50, 60)) == 0);
    CHECK(FillWindow(hwndSibling, RGB(70, 80, 90)) == 0);
    rglp = SDL_GetWindows(&cWindows);
    SDL_free(rglp);
    CHECK(cWindows == 1 && !hwndChild->lpWindow && !hwndNested->lpRenderer);
    CHECK(CrComposite(hwnd, 5, 5) == RGB(1, 2, 3));
    CHECK(CrComposite(hwnd, 21, 31) == RGB(10, 20, 30));
    CHECK(CrComposite(hwnd, 28, 40) == RGB(40, 50, 60));
    CHECK(CrComposite(hwnd, 130, 115) == RGB(70, 80, 90));
    /* The nested window extends past its parent and must be clipped there. */
    CHECK(MoveWindow(hwndNested, 90, 60, 40, 40, TRUE));
    CHECK(FillWindow(hwndNested, RGB(40, 50, 60)) == 0);
    CHECK(CrComposite(hwnd, 115, 95) == RGB(40, 50, 60));
    CHECK(CrComposite(hwnd, 125, 95) == RGB(1, 2, 3));
    CHECK(ShowWindow(hwndChild, SW_HIDE));
    CHECK(CrComposite(hwnd, 21, 31) == RGB(1, 2, 3));
    CHECK(!ShowWindow(hwndChild, SW_SHOW));
    CHECK(MoveWindow(hwndNested, 7, 9, 40, 30, TRUE));
    CHECK(CrComposite(hwnd, 28, 40) == RGB(40, 50, 60));
    Drain();
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = hwnd->id;
    event.motion.x = 28; event.motion.y = 40;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(msg.hwnd == hwndNested && (int16_t)LOWORD(msg.lParam) == 1 && (int16_t)HIWORD(msg.lParam) == 1);
    EnableWindow(hwndNested, FALSE);
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && LOWORD(msg.lParam) == 8 && HIWORD(msg.lParam) == 10);
    EnableWindow(hwndNested, TRUE);
    SetFocus(hwndNested);
    event = (SDL_Event){0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = hwnd->id; event.key.scancode = SDL_SCANCODE_A;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE) && msg.hwnd == hwndNested);
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = hwnd->id; event.text.text = "a";
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE) && msg.hwnd == hwndNested && msg.wParam == 'a');
    CHECK(DestroyWindow(hwndSibling));
    CHECK(CrComposite(hwnd, 115, 105) == RGB(10, 20, 30));
    CHECK(CrComposite(hwnd, 130, 115) == RGB(1, 2, 3));
    return 0;
}
#endif

int main(void) {
    WNDCLASS wc = {0};
    HINSTANCE hInstance;
    HWND hwnd, hwndChild, hwndNested, hwndSibling, hwndOwned, hwndFailed, hwndReentrant, hwndTimerFilter;
    CHILDTEST parent = {0}, child = {0}, nested = {0}, sibling = {0}, owned = {0}, failure = {0}, reentrant = {0};
    RECT rc;
    POINT pt, origin = {0, 0};
    MSG msg;
    int cFocus, cBlur, cEnable;
    DWORD msStart;
    BOOL fTimer;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.hInstance = hInstance; wc.lpszClassName = "StarsChildContracts"; wc.lpfnWndProc = ChildProc;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "parent", WS_POPUP | WS_VISIBLE, 40, 50, 320, 240,
                        NULL, NULL, hInstance, &parent);
    CHECK(hwnd && parent.fCreateData);
    child.hwndParent = hwnd; child.id = 101;
    hwndChild = CreateWindow(wc.lpszClassName, "child", WS_CHILD | WS_VISIBLE, 20, 30, 100, 80,
                            hwnd, (HMENU)(INT_PTR)child.id, hInstance, &child);
    CHECK(hwndChild && child.fCreateData);
    parent.hwndChild = hwndChild;
    nested.hwndParent = hwndChild; nested.id = 202;
    hwndNested = CreateWindow(wc.lpszClassName, "nested", WS_CHILD | WS_VISIBLE, 7, 9, 40, 30,
                             hwndChild, (HMENU)(INT_PTR)nested.id, hInstance, &nested);
    CHECK(hwndNested && nested.fCreateData);
    child.hwndChild = hwndNested;
    sibling.hwndParent = hwnd; sibling.id = 303;
    hwndSibling = CreateWindow(wc.lpszClassName, "sibling", WS_CHILD | WS_VISIBLE, 110, 100, 30, 20,
                              hwnd, (HMENU)(INT_PTR)sibling.id, hInstance, &sibling);
    CHECK(hwndSibling && GetParent(hwndChild) == hwnd && GetParent(hwndNested) == hwndChild);
    CHECK(IsChild(hwnd, hwndNested) && !IsChild(hwndChild, hwnd) && !IsChild(hwnd, hwnd));
    CHECK(GetDlgItem(hwnd, 101) == hwndChild && GetDlgItem(hwnd, 202) == NULL);
    CHECK(GetDlgCtrlID(hwndNested) == 202 && GetWindowLongPtr(hwndNested, GWLP_ID) == 202);
    CHECK(SetWindowLongPtr(hwndNested, GWLP_ID, 204) == 202);
    CHECK(GetDlgItem(hwndChild, 204) == hwndNested);
    CHECK(GetWindow(hwnd, GW_CHILD) == hwndChild);
    CHECK(GetWindow(hwndChild, GW_HWNDNEXT) == hwndSibling && GetWindow(hwndSibling, GW_HWNDPREV) == hwndChild);
    CHECK(GetWindow(hwndSibling, GW_HWNDFIRST) == hwndChild && GetWindow(hwndChild, GW_HWNDLAST) == hwndSibling);
    CHECK(GetWindow(hwndNested, GW_OWNER) == NULL);
    CHECK(ClientToScreen(hwnd, &origin));
    CHECK(GetWindowRect(hwndNested, &rc));
    CHECK(rc.left == origin.x + 27 && rc.top == origin.y + 39 && rc.right - rc.left == 40 && rc.bottom - rc.top == 30);
    pt = (POINT){-4, 6};
    CHECK(ClientToScreen(hwndNested, &pt) && pt.x == origin.x + 23 && pt.y == origin.y + 45);
    CHECK(ScreenToClient(hwndNested, &pt) && pt.x == -4 && pt.y == 6);
    CHECK(GetClientRect(hwndNested, &rc) && rc.left == 0 && rc.top == 0 && rc.right == 40 && rc.bottom == 30);
    CHECK(SetWindowText(hwndNested, "retained child text"));
    {
        char sz[80];
        CHECK(GetWindowText(hwndNested, sz, sizeof(sz)) == 19 && strcmp(sz, "retained child text") == 0);
    }
    CHECK(ShowWindow(hwnd, SW_HIDE) && !IsWindowVisible(hwndNested));
    CHECK(!ShowWindow(hwnd, SW_SHOW) && IsWindowVisible(hwndNested));
    Drain();
    cFocus = nested.cFocus; cBlur = nested.cBlur; cEnable = child.cEnable;
    SetFocus(hwndNested);
    CHECK(GetFocus() == hwndNested && nested.cFocus == cFocus + 1);
    CHECK(!EnableWindow(hwndChild, FALSE) && !IsWindowEnabled(hwndChild));
    CHECK(IsWindowEnabled(hwndNested));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    /* Microsoft documents descendant focus loss; Wine 9 retains it in this case. */
    CHECK(GetFocus() == NULL && nested.cBlur == cBlur + 1);
    CHECK(SetFocus(hwndNested) == NULL);
#else
    (void)cBlur;
#endif
    CHECK(EnableWindow(hwndChild, FALSE)); /* Already disabled: returns TRUE. */
    CHECK(EnableWindow(hwndChild, TRUE) && child.cEnable == cEnable + 2);
    SetFocus(NULL);
    SetFocus(hwndNested);
    cBlur = nested.cBlur;
    CHECK(!EnableWindow(hwndNested, FALSE));
    CHECK(GetFocus() == NULL && nested.cBlur == cBlur + 1);
    CHECK(SetFocus(hwndNested) == NULL);
    CHECK(EnableWindow(hwndNested, TRUE));
    CHECK(PostMessage(hwndNested, msgChild, 47, 0));
    CHECK(PeekMessage(&msg, hwnd, msgChild, msgChild, PM_REMOVE) && msg.hwnd == hwndNested && msg.wParam == 47);
    CHECK(SetTimer(hwndNested, 18, 10, NULL));
    /* Wine 9 excludes descendant timers from an ancestor filter, even after generation. */
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    hwndTimerFilter = hwndNested;
#else
    hwndTimerFilter = hwnd;
#endif
    msStart = GetTickCount();
    do {
        fTimer = PeekMessage(&msg, hwndTimerFilter, WM_TIMER, WM_TIMER, PM_REMOVE);
        if (fTimer) break;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
        Sleep(10);
#else
        SDL_Delay(10);
#endif
    } while ((DWORD)(GetTickCount() - msStart) < 1000);
    CHECK(fTimer && msg.hwnd == hwndNested && msg.wParam == 18);
    CHECK(KillTimer(hwndNested, 18));
    failure.hwndParent = hwnd; failure.id = 404; failure.fReject = TRUE;
    hwndFailed = CreateWindow(wc.lpszClassName, "reject", WS_CHILD, 0, 0, 10, 10,
                             hwnd, (HMENU)(INT_PTR)404, hInstance, &failure);
    CHECK(!hwndFailed && failure.cDestroy == 0 && failure.cNcDestroy == 1 && !GetDlgItem(hwnd, 404));
    reentrant.hwndParent = hwnd; reentrant.id = 505; reentrant.fDestroyOnMove = TRUE;
    hwndReentrant = CreateWindow(wc.lpszClassName, "move destroy", WS_CHILD, 1, 1, 10, 10,
                                hwnd, (HMENU)(INT_PTR)505, hInstance, &reentrant);
    /* Windows also sends initial WM_MOVE during creation; allow either path. */
    if (hwndReentrant) MoveWindow(hwndReentrant, 2, 2, 15, 15, TRUE);
    CHECK(reentrant.cDestroy == 1 && reentrant.cNcDestroy == 1);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestComposition(hwnd, hwndChild, hwndNested, hwndSibling) == 0);
#else
    CHECK(FillWindow(hwndChild, RGB(10, 20, 30)) == 0);
#endif
    owned.hwndParent = hwnd;
    hwndOwned = CreateWindow(wc.lpszClassName, "owned popup", WS_POPUP, 0, 0, 20, 20, hwnd, NULL, hInstance, &owned);
    CHECK(hwndOwned && GetParent(hwndOwned) == hwnd && GetWindow(hwndOwned, GW_OWNER) == hwnd && !IsChild(hwnd, hwndOwned));
    CHECK(DestroyWindow(hwnd));
    CHECK(!IsWindow(hwndNested) && !IsWindow(hwndChild) && !IsWindow(hwndOwned));
    CHECK(parent.cDestroy == 1 && child.cDestroy == 1 && nested.cDestroy == 1 && owned.cDestroy == 1);
    CHECK(parent.fChildrenAlive && child.fChildrenAlive);
    CHECK(parent.iDestroy < child.iDestroy && child.iDestroy < nested.iDestroy);
    CHECK(nested.iNcDestroy < child.iNcDestroy && child.iNcDestroy < parent.iNcDestroy);
    CHECK(!PostMessage(hwndNested, msgChild, 1, 0));
    CHECK(!ClientToScreen(hwndNested, &pt) && !GetWindowRect(hwndChild, &rc));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 child-window contracts passed");
    return 0;
}
