/* Capture contracts shared with the Windows SDK. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static HWND hwndLost, hwndGained, hwndObserved;
static int cChanged;
static BOOL fDestroyOnChange;

static LRESULT CALLBACK CaptureProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_CAPTURECHANGED) {
        hwndLost = hwnd; hwndGained = (HWND)l; hwndObserved = GetCapture();
        cChanged++;
        if (fDestroyOnChange) { fDestroyOnChange = FALSE; DestroyWindow((HWND)l); }
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}
#ifndef _WIN32
static int TestRouting(HWND hwnd, HWND hwndChild, HWND hwndOther) {
    SDL_Event event = {0};
    MSG msg;
    POINT pt;
    SetFocus(hwndChild);
    Drain();
    CHECK(SetCapture(hwndChild) == NULL);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = hwnd->id;
    event.motion.x = -12; event.motion.y = -14;
    event.motion.state = SDL_BUTTON_LMASK;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && (int16_t)LOWORD(msg.lParam) == -32 &&
          (int16_t)HIWORD(msg.lParam) == -44 && (msg.wParam & MK_LBUTTON));
    event.motion.windowID = hwndOther->id;
    event.motion.x = 5; event.motion.y = 6;
    pt.x = 5; pt.y = 6;
    CHECK(ClientToScreen(hwndOther, &pt) && ScreenToClient(hwndChild, &pt));
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && (int16_t)LOWORD(msg.lParam) == pt.x &&
          (int16_t)HIWORD(msg.lParam) == pt.y);
    event = (SDL_Event){0};
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = hwnd->id; event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 150; event.button.y = 160;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_LBUTTONUP, WM_LBUTTONUP, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && LOWORD(msg.lParam) == 130 && HIWORD(msg.lParam) == 130);
    CHECK(ReleaseCapture());
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = hwnd->id; event.motion.x = 150; event.motion.y = 160;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE) && msg.hwnd == hwnd);
    SetFocus(hwndOther);
    CHECK(SetCapture(hwndChild) == NULL);
    event.motion.x = 25; event.motion.y = 36;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && LOWORD(msg.lParam) == 5 && HIWORD(msg.lParam) == 6);
    event.motion.x = 150; event.motion.y = 160;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE) && msg.hwnd == hwnd);
    event.motion.windowID = hwndOther->id;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE) && msg.hwnd == hwndOther);
    CHECK(ReleaseCapture());
    SetFocus(hwndChild);
    CHECK(SetCapture(hwndChild) == NULL);
    event = (SDL_Event){0}; event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    event.window.windowID = hwnd->id;
    CompatRouteEvent(&event);
    CHECK(!GetCapture() && hwndLost == hwndChild && !hwndGained);
    CHECK(SetCapture((HWND)(UINT_PTR)0x1234) == NULL && !GetCapture());
    return 0;
}
#endif
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndChild, hwndOther;
    HINSTANCE hInstance;
    int c;
#ifdef _WIN32
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.hInstance = hInstance; wc.lpszClassName = "StarsCaptureContracts"; wc.lpfnWndProc = CaptureProc;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "capture", WS_POPUP | WS_VISIBLE, 40, 50, 320, 240,
                        NULL, NULL, hInstance, NULL);
    hwndChild = CreateWindow(wc.lpszClassName, "child", WS_CHILD | WS_VISIBLE, 20, 30, 100, 80,
                             hwnd, (HMENU)(INT_PTR)101, hInstance, NULL);
    hwndOther = CreateWindow(wc.lpszClassName, "other", WS_POPUP | WS_VISIBLE, 440, 50, 320, 240,
                             NULL, NULL, hInstance, NULL);
    CHECK(hwnd && hwndChild && hwndOther);
    Drain();
    CHECK(ReleaseCapture() && !GetCapture());
    CHECK(SetCapture(hwndChild) == NULL && GetCapture() == hwndChild);
    c = cChanged;
    CHECK(SetCapture(hwndChild) == hwndChild && cChanged == c + 1);
    CHECK(hwndLost == hwndChild && hwndGained == hwndChild && hwndObserved == hwndChild);
    c = cChanged;
    CHECK(SetCapture(hwndOther) == hwndChild);
    CHECK(cChanged == c + 1 && hwndLost == hwndChild && hwndGained == hwndOther && hwndObserved == hwndOther);
    CHECK(ReleaseCapture() && !GetCapture());
    CHECK(hwndLost == hwndOther && !hwndGained && !hwndObserved);
    CHECK(SetCapture(hwndChild) == NULL);
    SendMessage(hwndChild, WM_CANCELMODE, 0, 0);
    CHECK(!GetCapture() && hwndLost == hwndChild);
#ifndef _WIN32
    CHECK(TestRouting(hwnd, hwndChild, hwndOther) == 0);
#endif
    CHECK(SetCapture(hwndChild) == NULL);
    fDestroyOnChange = TRUE;
    CHECK(SetCapture(hwndOther) == hwndChild && !IsWindow(hwndOther) && !GetCapture());
    CHECK(SetCapture(hwndChild) == NULL);
    CHECK(DestroyWindow(hwnd) && !GetCapture());
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Win32 capture contracts passed");
    return 0;
}
