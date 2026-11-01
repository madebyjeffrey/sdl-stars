#ifndef _WIN32
#include "../../compat/win32/src/display_io.h"
#endif
/* Window states and normal placement, shared with the Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
static BOOL fFakeDisplay, fFailDisplay;
bool SDL_GetDisplayBounds(SDL_DisplayID id, SDL_Rect *rc);
bool SDL_GetDisplayUsableBounds(SDL_DisplayID id, SDL_Rect *rc);
extern "C" bool SDLCALL StarsTest_SDL_GetDisplayBounds(SDL_DisplayID id, SDL_Rect *rc) {
    if (!fFakeDisplay) return SDL_GetDisplayBounds(id, rc);
    if (fFailDisplay) return false;
    *rc = (SDL_Rect){-640, -80, 640, 480}; return true;
}
extern "C" bool SDLCALL StarsTest_SDL_GetDisplayUsableBounds(SDL_DisplayID id, SDL_Rect *rc) {
    if (!fFakeDisplay) return SDL_GetDisplayUsableBounds(id, rc);
    *rc = (SDL_Rect){-630, -50, 620, 440}; return true;
}
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static UINT sizeLast;
static BOOL fDestroySize;
static LRESULT CALLBACK StateProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_SIZE) {
        sizeLast = (UINT)w;
        if (fDestroySize) { fDestroySize = 0; DestroyWindow(hwnd); }
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static void Drain(void) {
    APPMSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
}
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndChild;
    WINDOWPLACEMENT wndpl = {0};
    RECT rc, rcNormal = {10, 20, 60, 80};
    HINSTANCE hInstance;
#ifndef _WIN32
    SDL_Event event = {0};
    WINDOWPLACEMENT wndplOld;
    SDL_Rect rcDisplay, rcWork;
    SDL_DisplayID idDisplay;
#endif
#ifndef _WIN32
    COMPATDISPLAYIO io_compatdisplayio;
    io_compatdisplayio.pfnGetDisplayBounds = StarsTest_SDL_GetDisplayBounds;
    io_compatdisplayio.pfnGetDisplayUsableBounds = StarsTest_SDL_GetDisplayUsableBounds;
    COMPATDISPLAYIOSCOPE scope_compatdisplayio(io_compatdisplayio);
#endif

#ifdef _WIN32
    hInstance = GetModuleHandleA(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.lpszClassName = "StarsStateContracts"; wc.lpfnWndProc = StateProc; wc.hInstance = hInstance;
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "state", WS_POPUP | WS_VISIBLE,
                        70, 80, 400, 300, NULL, NULL, hInstance, NULL);
    CHECK(hwnd);
    hwndChild = CreateWindowExA(0, wc.lpszClassName, "child", WS_CHILD | WS_VISIBLE,
                             10, 20, 50, 60, hwnd, (HMENU)(INT_PTR)101, hInstance, NULL);
    CHECK(hwndChild);
    Drain();
    wndpl.length = sizeof(wndpl);
    CHECK(GetWindowPlacement(hwndChild, &wndpl));
    CHECK(wndpl.length == sizeof(wndpl) && wndpl.showCmd == SW_SHOWNORMAL);
    CHECK(EqualRect(&wndpl.rcNormalPosition, &rcNormal));
    CHECK(!IsIconic(hwndChild) && !IsZoomed(hwndChild));
    ShowWindow(hwndChild, SW_SHOWMINIMIZED);
    CHECK(IsIconic(hwndChild) && !IsZoomed(hwndChild) && sizeLast == SIZE_MINIMIZED);
    CHECK(GetClientRect(hwndChild, &rc) && rc.right == 0 && rc.bottom == 0);
    CHECK(GetWindowLongPtrA(hwndChild, GWL_STYLE) & WS_MINIMIZE);
    CHECK(GetWindowPlacement(hwndChild, &wndpl) && wndpl.showCmd == SW_SHOWMINIMIZED);
    CHECK(EqualRect(&wndpl.rcNormalPosition, &rcNormal));
    ShowWindow(hwndChild, SW_SHOWMAXIMIZED);
    CHECK(!IsIconic(hwndChild) && IsZoomed(hwndChild) && sizeLast == SIZE_MAXIMIZED);
    CHECK(GetClientRect(hwndChild, &rc) && rc.right == 400 && rc.bottom == 300);
    CHECK(GetWindowPlacement(hwndChild, &wndpl) && EqualRect(&wndpl.rcNormalPosition, &rcNormal));
    ShowWindow(hwndChild, SW_SHOWMINIMIZED);
    CHECK(IsIconic(hwndChild));
    ShowWindow(hwndChild, SW_RESTORE);
    CHECK(IsZoomed(hwndChild) && !IsIconic(hwndChild));
    ShowWindow(hwndChild, SW_RESTORE);
    CHECK(!IsIconic(hwndChild) && !IsZoomed(hwndChild) && sizeLast == SIZE_RESTORED);
    CHECK(GetClientRect(hwndChild, &rc) && rc.right == 50 && rc.bottom == 60);
    CHECK(SetWindowPos(hwndChild, NULL, 30, 40, 70, 80, SWP_NOZORDER | SWP_NOACTIVATE));
    SetRect(&rcNormal, 30, 40, 100, 120);
    CHECK(GetWindowPlacement(hwndChild, &wndpl) && EqualRect(&wndpl.rcNormalPosition, &rcNormal));
    ShowWindow(hwndChild, SW_SHOWMINIMIZED);
    ShowWindow(hwndChild, SW_HIDE);
    CHECK(IsIconic(hwndChild));
    ShowWindow(hwndChild, SW_SHOW);
    CHECK(IsIconic(hwndChild));
    ShowWindow(hwndChild, SW_SHOWNORMAL);
    CHECK(!IsIconic(hwndChild) && GetClientRect(hwndChild, &rc) && rc.right == 70 && rc.bottom == 80);
#ifndef _WIN32
    wndpl.length = 22;
    CHECK(GetWindowPlacement(hwndChild, &wndpl) && wndpl.length == sizeof(wndpl));
    wndpl.length = 1; wndplOld = wndpl;
    CHECK(!GetWindowPlacement(hwndChild, &wndpl) && memcmp(&wndpl, &wndplOld, sizeof(wndpl)) == 0);
    CHECK(!GetWindowPlacement(hwndChild, NULL));
    wndpl.length = sizeof(wndpl);
    CHECK(GetWindowRect(hwnd, &rc));
    idDisplay = SDL_GetDisplayForWindow(hwnd->lpWindow.get());
    CHECK(SDL_GetDisplayBounds(idDisplay, &rcDisplay) && SDL_GetDisplayUsableBounds(idDisplay, &rcWork));
    OffsetRect(&rc, rcDisplay.x - rcWork.x, rcDisplay.y - rcWork.y);
    CHECK(GetWindowPlacement(hwnd, &wndpl) && EqualRect(&rc, &wndpl.rcNormalPosition));
    fFakeDisplay = 1;
    CHECK(GetWindowRect(hwnd, &rc));
    OffsetRect(&rc, -10, -30);
    CHECK(GetWindowPlacement(hwnd, &wndpl) && EqualRect(&rc, &wndpl.rcNormalPosition));
    wndplOld = wndpl; fFailDisplay = 1;
    CHECK(!GetWindowPlacement(hwnd, &wndpl) && memcmp(&wndpl, &wndplOld, sizeof(wndpl)) == 0);
    fFakeDisplay = fFailDisplay = 0;
    CHECK(GetWindowPlacement(hwnd, &wndpl));
    rc = wndpl.rcNormalPosition;
    event.window.windowID = hwnd->id;
    event.type = SDL_EVENT_WINDOW_MINIMIZED; CompatRouteEvent(&event); Drain();
    CHECK(IsIconic(hwnd) && sizeLast == SIZE_MINIMIZED);
    sizeLast = 99;
    CompatRouteEvent(&event); Drain();
    CHECK(sizeLast == SIZE_MINIMIZED);
    CHECK(GetWindowPlacement(hwnd, &wndpl) && EqualRect(&rc, &wndpl.rcNormalPosition));
    event.type = SDL_EVENT_WINDOW_MAXIMIZED; CompatRouteEvent(&event); Drain();
    CHECK(IsZoomed(hwnd) && !IsIconic(hwnd) && sizeLast == SIZE_MAXIMIZED);
    event.type = SDL_EVENT_WINDOW_RESTORED; CompatRouteEvent(&event); Drain();
    CHECK(!IsZoomed(hwnd) && !IsIconic(hwnd) && sizeLast == SIZE_RESTORED);
#endif
    fDestroySize = 1;
    ShowWindow(hwndChild, SW_SHOWMINIMIZED);
    CHECK(!IsWindow(hwndChild));
    CHECK(!IsIconic(hwndChild) && !IsZoomed(hwndChild));
    CHECK(DestroyWindow(hwnd));
#ifndef _WIN32
    CHECK(!GetWindowPlacement(hwndChild, &wndpl));
    StarsCompatShutdown();
#endif
    puts("Win32 window state contracts passed");
    return 0;
}
