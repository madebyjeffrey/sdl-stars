/* Native surface coordinates and presentation at compositor-selected scale. */
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "desktop line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
static int cMouse;
static POINT ptMouse;
static LRESULT  DesktopProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == win32::WM_MOUSEMOVE) {
        cMouse++;
        ptMouse = (POINT){(SHORT)LOWORD(l), (SHORT)HIWORD(l)};
        return 0;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
static void Drain(void) {
    win32::MSG msg;
    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) win32::DispatchMessageA(&msg);
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
    wc.hInstance = win32::GetModuleHandleA(NULL);
    CHECK(win32::RegisterClassA(&wc));
    wc.lpszClassName = "StarsCompatToolbarTip";
    CHECK(win32::RegisterClassA(&wc));
    hmenu = win32::CreateMenu(); CHECK(hmenu && win32::AppendMenuA(hmenu, win32::MF_STRING, 1, "&File"));
    hwnd = win32::CreateWindowExA(0, "StarsDesktopProbe", "Desktop probe", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                        150, 170, 400, 300, NULL, hmenu, wc.hInstance, NULL);
    hwndSecond = win32::CreateWindowExA(0, "StarsDesktopProbe", "Second surface", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                              600, 200, 320, 240, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && hwndSecond && hwnd->lpWindow.get() != hwndSecond->lpWindow.get());
    CHECK(win32::GetWindowRect(hwnd, &rcOwner) && rcOwner.left == 0 && rcOwner.top == 0);
    // Toplevel movement is advisory on Wayland, while size and child placement work.
    CHECK(win32::SetWindowPos(hwnd, NULL, 230, 270, 400, 300, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE));
    hwndChild = win32::CreateWindowExA(0, "StarsDesktopProbe", "Child", win32::WS_CHILD | win32::WS_VISIBLE,
                             30, 40, 100, 80, hwnd, (HMENU)(INT_PTR)10, wc.hInstance, NULL);
    CHECK(hwndChild && win32::GetWindowRect(hwndChild, &rc));
    pt = (POINT){0, 0}; CHECK(win32::ClientToScreen(hwndChild, &pt));
    CHECK(pt.x == rc.left && pt.y == rc.top && win32::ScreenToClient(hwndChild, &pt) && pt.x == 0 && pt.y == 0);
    pt = (POINT){5, 6}; CHECK(win32::MapWindowPoints(hwndChild, hwnd, &pt, 1) != 0 && pt.x == 35 && pt.y == 46);
    hwndPopup = win32::CreateWindowExA(win32::WS_EX_NOACTIVATE, "StarsCompatToolbarTip", "Tip", win32::WS_POPUP | win32::WS_VISIBLE,
                               60, 70, 80, 30, hwnd, NULL, wc.hInstance, NULL);
    CHECK(hwndPopup && FCompatPresent(hwndPopup) && SDL_SyncWindow(hwndPopup->lpWindow.get()));
    CHECK(SDL_GetWindowParent(hwndPopup->lpWindow.get()) == hwnd->lpWindow.get());
    CHECK(win32::GetWindowRect(hwndPopup, &rc) && rc.left == 60 && rc.top == 70);
    CHECK(win32::SetWindowPos(hwndPopup, NULL, 90, 100, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE));
    CHECK(win32::GetWindowRect(hwndPopup, &rc) && rc.left == 90 && rc.top == 100);
    CHECK(win32::DestroyWindow(hwndPopup));
    win32::SetFocus(hwndChild); win32::SetCapture(hwndChild);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwnd->id;
    event.motion.x = 37; event.motion.y = 49 + CyCompatMenuBar(hwnd);
    CHECK(SDL_PushEvent(&event)); Drain();
    CHECK(cMouse != 0 && ptMouse.x == 7 && ptMouse.y == 9 && win32::GetCursorPos(&ptCursor));
    pt = (POINT){7, 9}; CHECK(win32::ClientToScreen(hwndChild, &pt) && pt.x == ptCursor.x && pt.y == ptCursor.y);
    CHECK(win32::GetFocus() == hwndChild && win32::GetCapture() == hwndChild && win32::ReleaseCapture());
    win32::SetFocus(hwndSecond);
    CHECK(SDL_PushEvent(&event)); Drain();
    CHECK(win32::GetFocus() == hwndSecond && win32::GetCursorPos(&ptCursor) && win32::WindowFromPoint(ptCursor) == hwndChild);
    CHECK(win32::GetClientRect(hwnd, &rc) && rc.right == 400 && rc.bottom == 300 - CyCompatMenuBar(hwnd));
    hdc = win32::GetDC(hwnd); hbr = win32::CreateSolidBrush(win32::RGB(30, 90, 150));
    CHECK(hdc && hbr && win32::FillRect(hdc, &rc, hbr) && win32::ReleaseDC(hwnd, hdc) && win32::DeleteObject(hbr));
    CHECK(FCompatPresent(hwnd) && FCompatPresent(hwndSecond));
    CHECK(SDL_GetWindowSizeInPixels(hwnd->lpWindow.get(), &cxPixels, &cyPixels));
    CHECK(cxPixels >= 400 && cyPixels >= 300 && hwnd->lpComposite->w == 400 && hwnd->lpComposite->h == 300);
    CHECK(!pszRenderer || strcmp(SDL_GetRendererName(hwnd->lpRenderer.get()), pszRenderer) == 0);
    printf("Wayland logical 400x300, pixels %dx%d, scale %.2f, renderer %s\n", cxPixels, cyPixels,
           SDL_GetWindowPixelDensity(hwnd->lpWindow.get()), SDL_GetRendererName(hwnd->lpRenderer.get()));
    CHECK(win32::DestroyWindow(hwndSecond) && win32::DestroyWindow(hwnd) && !win32::IsMenu(hmenu));
    StarsCompatProcessExit(); SDL_Quit();
    return 0;
}
