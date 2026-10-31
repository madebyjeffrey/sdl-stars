/* Native surface coordinates and presentation at compositor-selected scale. */
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "desktop line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
static int cMouse;
static POINT ptMouse;
static LRESULT CALLBACK DesktopProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_MOUSEMOVE) {
        cMouse++;
        ptMouse = (POINT){(SHORT)LOWORD(l), (SHORT)HIWORD(l)};
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}
int main(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndSecond, hwndChild, hwndPopup;
    RECT rc, rcOwner;
    POINT pt, ptCursor;
    SDL_Event event = {};
    HMENU hmenu;
    HDC hdc;
    HBRUSH hbr;
    int cxPixels, cyPixels;
    const char *pszRenderer = SDL_getenv("STARS_SDL_RENDERER");
    wc.lpszClassName = "StarsDesktopProbe"; wc.lpfnWndProc = DesktopProc;
    wc.hInstance = GetModuleHandle(NULL);
    CHECK(RegisterClass(&wc));
    wc.lpszClassName = "StarsCompatToolbarTip";
    CHECK(RegisterClass(&wc));
    hmenu = CreateMenu(); CHECK(hmenu && AppendMenu(hmenu, MF_STRING, 1, "&File"));
    hwnd = CreateWindow("StarsDesktopProbe", "Desktop probe", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        150, 170, 400, 300, NULL, hmenu, wc.hInstance, NULL);
    hwndSecond = CreateWindow("StarsDesktopProbe", "Second surface", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              600, 200, 320, 240, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && hwndSecond && hwnd->lpWindow.get() != hwndSecond->lpWindow.get());
    CHECK(GetWindowRect(hwnd, &rcOwner) && rcOwner.left == 0 && rcOwner.top == 0);
    // Toplevel movement is advisory on Wayland, while size and child placement work.
    CHECK(SetWindowPos(hwnd, NULL, 230, 270, 400, 300, SWP_NOZORDER | SWP_NOACTIVATE));
    hwndChild = CreateWindow("StarsDesktopProbe", "Child", WS_CHILD | WS_VISIBLE,
                             30, 40, 100, 80, hwnd, (HMENU)(INT_PTR)10, wc.hInstance, NULL);
    CHECK(hwndChild && GetWindowRect(hwndChild, &rc));
    pt = (POINT){0, 0}; CHECK(ClientToScreen(hwndChild, &pt));
    CHECK(pt.x == rc.left && pt.y == rc.top && ScreenToClient(hwndChild, &pt) && pt.x == 0 && pt.y == 0);
    pt = (POINT){5, 6}; CHECK(MapWindowPoints(hwndChild, hwnd, &pt, 1) != 0 && pt.x == 35 && pt.y == 46);
    hwndPopup = CreateWindowExA(WS_EX_NOACTIVATE, "StarsCompatToolbarTip", "Tip", WS_POPUP | WS_VISIBLE,
                               60, 70, 80, 30, hwnd, NULL, wc.hInstance, NULL);
    CHECK(hwndPopup && FCompatPresent(hwndPopup) && SDL_SyncWindow(hwndPopup->lpWindow.get()));
    CHECK(SDL_GetWindowParent(hwndPopup->lpWindow.get()) == hwnd->lpWindow.get());
    CHECK(GetWindowRect(hwndPopup, &rc) && rc.left == 60 && rc.top == 70);
    CHECK(SetWindowPos(hwndPopup, NULL, 90, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
    CHECK(GetWindowRect(hwndPopup, &rc) && rc.left == 90 && rc.top == 100);
    CHECK(DestroyWindow(hwndPopup));
    SetFocus(hwndChild); SetCapture(hwndChild);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwnd->id;
    event.motion.x = 37; event.motion.y = 49 + CyCompatMenuBar(hwnd);
    CHECK(SDL_PushEvent(&event)); Drain();
    CHECK(cMouse != 0 && ptMouse.x == 7 && ptMouse.y == 9 && GetCursorPos(&ptCursor));
    pt = (POINT){7, 9}; CHECK(ClientToScreen(hwndChild, &pt) && pt.x == ptCursor.x && pt.y == ptCursor.y);
    CHECK(GetFocus() == hwndChild && GetCapture() == hwndChild && ReleaseCapture());
    SetFocus(hwndSecond);
    CHECK(SDL_PushEvent(&event)); Drain();
    CHECK(GetFocus() == hwndSecond && GetCursorPos(&ptCursor) && WindowFromPoint(ptCursor) == hwndChild);
    CHECK(GetClientRect(hwnd, &rc) && rc.right == 400 && rc.bottom == 300 - CyCompatMenuBar(hwnd));
    hdc = GetDC(hwnd); hbr = CreateSolidBrush(RGB(30, 90, 150));
    CHECK(hdc && hbr && FillRect(hdc, &rc, hbr) && ReleaseDC(hwnd, hdc) && DeleteObject(hbr));
    CHECK(FCompatPresent(hwnd) && FCompatPresent(hwndSecond));
    CHECK(SDL_GetWindowSizeInPixels(hwnd->lpWindow.get(), &cxPixels, &cyPixels));
    CHECK(cxPixels >= 400 && cyPixels >= 300 && hwnd->lpComposite->w == 400 && hwnd->lpComposite->h == 300);
    CHECK(!pszRenderer || strcmp(SDL_GetRendererName(hwnd->lpRenderer.get()), pszRenderer) == 0);
    printf("Wayland logical 400x300, pixels %dx%d, scale %.2f, renderer %s\n", cxPixels, cyPixels,
           SDL_GetWindowPixelDensity(hwnd->lpWindow.get()), SDL_GetRendererName(hwnd->lpRenderer.get()));
    CHECK(DestroyWindow(hwndSecond) && DestroyWindow(hwnd) && !IsMenu(hmenu));
    StarsCompatProcessExit(); SDL_Quit();
    return 0;
}
