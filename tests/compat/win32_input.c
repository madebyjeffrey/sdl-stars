/* Keyboard queue and ANSI input contracts; SDK checks use real SendInput. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
static BOOL fMock;
static bool rgfKeys[SDL_SCANCODE_COUNT];
static SDL_Keymod modMock;
static SDL_MouseButtonFlags buttonsMock;
static float xMock, yMock;
static const char *pszDriverMock;
const char *SDL_GetCurrentVideoDriver(void);
const char *SDLCALL StarsTest_SDL_GetCurrentVideoDriver(void) { return pszDriverMock ? pszDriverMock : SDL_GetCurrentVideoDriver(); }
const bool *SDL_GetKeyboardState(int *c);
SDL_Keymod SDL_GetModState(void);
SDL_MouseButtonFlags SDL_GetGlobalMouseState(float *x, float *y);
const bool *SDLCALL StarsTest_SDL_GetKeyboardState(int *c) {
    if (!fMock) return SDL_GetKeyboardState(c);
    if (c) *c = SDL_SCANCODE_COUNT;
    return rgfKeys;
}
SDL_Keymod SDLCALL StarsTest_SDL_GetModState(void) { return fMock ? modMock : SDL_GetModState(); }
SDL_MouseButtonFlags SDLCALL StarsTest_SDL_GetGlobalMouseState(float *x, float *y) {
    if (!fMock) return SDL_GetGlobalMouseState(x, y);
    if (x) *x = xMock;
    if (y) *y = yMock;
    return buttonsMock;
}
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cWheel;
static HWND hwndWheelLast;
static LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_MOUSEWHEEL && !GetParent(hwnd)) { cWheel++; hwndWheelLast = hwnd; return 0; }
    return DefWindowProc(hwnd, msg, w, l);
}
static int TestPoints(HWND hwnd, HINSTANCE hInstance) {
    HWND hwndChild, hwndSibling;
    POINT pt = {5, 6};
    hwndChild = CreateWindow("StarsInputContracts", "child", WS_CHILD | WS_VISIBLE,
                            20, 30, 100, 80, hwnd, (HMENU)(INT_PTR)101, hInstance, NULL);
    hwndSibling = CreateWindow("StarsInputContracts", "sibling", WS_CHILD | WS_VISIBLE,
                              20, 30, 100, 80, hwnd, (HMENU)(INT_PTR)102, hInstance, NULL);
    CHECK(hwndChild && hwndSibling && ClientToScreen(hwndChild, &pt));
    CHECK(SetWindowPos(hwndChild, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
    CHECK(WindowFromPoint(pt) == hwndChild);
    SetFocus(hwndChild);
    CHECK(GetFocus() == hwndChild && GetActiveWindow() == hwnd);
    CHECK(SetFocus(NULL) == hwndChild);
    CHECK(GetFocus() == NULL && GetActiveWindow() == hwnd);
    SetFocus(hwndSibling);
    CHECK(GetFocus() == hwndSibling && GetActiveWindow() == hwnd);
    ShowWindow(hwndChild, SW_HIDE);
    CHECK(WindowFromPoint(pt) == hwndSibling);
    ShowWindow(hwndChild, SW_SHOW);
    EnableWindow(hwndChild, FALSE);
    CHECK(WindowFromPoint(pt) == hwndSibling);
    EnableWindow(hwndChild, TRUE);
    CHECK(WindowFromPoint(pt) == hwndChild);
    cWheel = 0;
    SendMessage(hwndChild, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), MAKELPARAM(pt.x, pt.y));
    CHECK(cWheel == 1 && hwndWheelLast == hwnd);
    CHECK(DestroyWindow(hwndSibling) && DestroyWindow(hwndChild));
    CHECK(GetActiveWindow() == hwnd);
    return 0;
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}
static int QueueShift(HWND hwnd) {
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    INPUT rg[2] = {{0}};
    rg[0].type = INPUT_KEYBOARD; rg[0].ki.wVk = VK_LSHIFT;
    rg[1] = rg[0]; rg[1].ki.dwFlags = KEYEVENTF_KEYUP;
    CHECK(SendInput(2, rg, sizeof(INPUT)) == 2);
#else
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwnd->id;
    event.key.scancode = SDL_SCANCODE_LSHIFT; event.key.mod = SDL_KMOD_LSHIFT;
    CHECK(SDL_PushEvent(&event));
    event.type = SDL_EVENT_KEY_UP; event.key.mod = SDL_KMOD_NONE;
    CHECK(SDL_PushEvent(&event));
#endif
    return 0;
}
static int WaitKey(HWND hwnd, MSG *lpmsg, UINT flags) {
    DWORD ms = GetTickCount();
    do {
        if (PeekMessage(lpmsg, hwnd, WM_KEYDOWN, WM_KEYUP, flags)) return 0;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
        Sleep(5);
#else
        SDL_Delay(5);
#endif
    } while (GetTickCount() - ms < 2000);
    CHECK(FALSE);
    return 1;
}
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int TestWheelClicks(HWND hwnd, HINSTANCE hInstance) {
    SDL_Event event = {0};
    HWND hwndChild;
    MSG msg;
    POINT pt = {28, 40};
    int i, total = 0;
    hwndChild = CreateWindow("StarsInputContracts", "wheel child", WS_CHILD | WS_VISIBLE,
                            20, 30, 100, 80, hwnd, (HMENU)(INT_PTR)103, hInstance, NULL);
    CHECK(hwndChild);
    SetFocus(hwndChild); Drain();
    CHECK(ClientToScreen(hwnd, &pt));
    event.type = SDL_EVENT_MOUSE_WHEEL; event.wheel.windowID = hwnd->id;
    event.wheel.mouse_x = 28; event.wheel.mouse_y = 40;
    event.wheel.y = 0.5f; event.wheel.x = 1;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE));
    CHECK(msg.hwnd == hwndChild && GET_WHEEL_DELTA_WPARAM(msg.wParam) == 60);
    CHECK(msg.pt.x == pt.x && msg.pt.y == pt.y && (int16_t)LOWORD(msg.lParam) == pt.x &&
          (int16_t)HIWORD(msg.lParam) == pt.y);
    CHECK(WindowFromPoint(msg.pt) == hwndChild);
    cWheel = 0; DispatchMessage(&msg);
    CHECK(cWheel == 1 && hwndWheelLast == hwnd);
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEHWHEEL, WM_MOUSEHWHEEL, PM_REMOVE));
    CHECK(GET_WHEEL_DELTA_WPARAM(msg.wParam) == 120);
    event.wheel.x = 0; event.wheel.y = 1; event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE));
    CHECK(GET_WHEEL_DELTA_WPARAM(msg.wParam) == -120);
    event.wheel.direction = SDL_MOUSEWHEEL_NORMAL; event.wheel.y = 0.001f;
    for (i = 0; i < 10; i++) CHECK(SDL_PushEvent(&event));
    while (PeekMessage(&msg, NULL, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE))
        total += GET_WHEEL_DELTA_WPARAM(msg.wParam);
    CHECK(total == 1);
    event.wheel.y = 400;
    CHECK(SDL_PushEvent(&event));
    total = 0;
    while (PeekMessage(&msg, NULL, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE))
        total += GET_WHEEL_DELTA_WPARAM(msg.wParam);
    CHECK(total == 48000);
    event = (SDL_Event){0}; event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = hwnd->id; event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 28; event.button.y = 40; event.button.clicks = 2;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, PM_REMOVE) && msg.message == WM_LBUTTONDOWN);
    event.button.clicks = 3;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, NULL, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, PM_REMOVE));
    CHECK(msg.message == WM_LBUTTONDBLCLK && msg.hwnd == hwndChild && (msg.wParam & MK_LBUTTON));
    CHECK(msg.pt.x == pt.x && msg.pt.y == pt.y);
    CHECK(DestroyWindow(hwndChild));
    SetFocus(hwnd); Drain();
    return 0;
}
static int TestPhysical(void) {
    SHORT w;
    POINT pt;
    const char *pszDriver = SDL_GetCurrentVideoDriver();
    fMock = TRUE;
    rgfKeys[SDL_SCANCODE_A] = true;
    rgfKeys[SDL_SCANCODE_LSHIFT] = true;
    modMock = SDL_KMOD_RCTRL | SDL_KMOD_LSHIFT;
    buttonsMock = SDL_BUTTON_RMASK;
    w = GetAsyncKeyState('A');
    CHECK(w < 0 && (w & 1));
    CHECK(GetAsyncKeyState('A') < 0 && !(GetAsyncKeyState('A') & 1));
    CHECK(GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_RCONTROL) < 0);
    CHECK(!(GetAsyncKeyState(VK_LCONTROL) & 0x8000));
    CHECK(GetAsyncKeyState(VK_SHIFT) < 0 && GetAsyncKeyState(VK_LSHIFT) < 0);
    CHECK(!(GetAsyncKeyState(VK_RSHIFT) & 0x8000));
    CHECK(GetAsyncKeyState(VK_RBUTTON) < 0 && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000));
    rgfKeys[SDL_SCANCODE_A] = false; modMock = 0; buttonsMock = 0;
    CHECK(GetAsyncKeyState(VK_LSHIFT) < 0 && GetAsyncKeyState(VK_SHIFT) < 0);
    rgfKeys[SDL_SCANCODE_LSHIFT] = false;
    CHECK(!(GetAsyncKeyState('A') & 0x8000) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000));
    CHECK(!(GetAsyncKeyState(VK_RBUTTON) & 0x8000));
    CHECK(GetAsyncKeyState(-1) == 0 && GetAsyncKeyState(256) == 0);
    if (pszDriver && strcmp(pszDriver, "x11") == 0) {
        xMock = -123.75f; yMock = 456.75f;
        CHECK(GetCursorPos(&pt) && pt.x == -123 && pt.y == 456);
    }
    pszDriverMock = "windows"; xMock = -123.75f; yMock = 456.75f;
    CHECK(GetCursorPos(&pt) && pt.x == -123 && pt.y == 456);
    xMock = NAN;
    CHECK(!GetCursorPos(&pt));
    pszDriverMock = "unsupported";
    CHECK(!GetCursorPos(&pt) && GetLastError() == ERROR_NOT_SUPPORTED);
    pszDriverMock = NULL;
    CHECK(!GetCursorPos(NULL));
    fMock = FALSE;
    return 0;
}
static int TestSnapshots(HWND hwnd) {
    SDL_Event event = {0};
    MSG msg;
    POINT pt, expected = {-7, 13};
    int i;
    static const SDL_Scancode rgsc[] = {SDL_SCANCODE_COMMA, SDL_SCANCODE_PERIOD,
        SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET};
    static const UINT rgvk[] = {VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_4, VK_OEM_6};
    CompatResetInput();
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwnd->id;
    event.key.scancode = SDL_SCANCODE_A; event.key.mod = SDL_KMOD_LSHIFT;
    CHECK(SDL_PushEvent(&event));
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwnd->id;
    event.motion.x = -7; event.motion.y = 13;
    CHECK(SDL_PushEvent(&event));
    event = (SDL_Event){0}; event.type = SDL_EVENT_KEY_UP;
    event.key.windowID = hwnd->id; event.key.scancode = SDL_SCANCODE_A;
    CHECK(SDL_PushEvent(&event));
    SDL_SetModState(SDL_KMOD_NONE);
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(GetKeyState('A') < 0 && GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_LSHIFT) < 0);
    CHECK(PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    CHECK((msg.wParam & MK_SHIFT) && GetKeyState('A') < 0 && GetKeyState(VK_SHIFT) < 0);
    CHECK(PeekMessage(&msg, hwnd, WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(!(GetKeyState('A') & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(ClientToScreen(hwnd, &expected) && GetCursorPos(&pt));
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") == 0)
        CHECK(pt.x == expected.x && pt.y == expected.y);
    for (i = 0; i < 4; i++) {
        event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = rgsc[i];
        CHECK(SDL_PushEvent(&event));
        CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
        CHECK(msg.wParam == rgvk[i] && (msg.lParam & 0x00ff0000));
    }
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_LSHIFT;
    event.key.mod = SDL_KMOD_LSHIFT;
    CHECK(SDL_PushEvent(&event));
    event.key.scancode = SDL_SCANCODE_RSHIFT; event.key.mod = SDL_KMOD_SHIFT;
    CHECK(SDL_PushEvent(&event));
    event.type = SDL_EVENT_KEY_UP; event.key.scancode = SDL_SCANCODE_LSHIFT;
    event.key.mod = SDL_KMOD_RSHIFT;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_RSHIFT) < 0 && !(GetKeyState(VK_LSHIFT) & 0x8000));
    event.key.scancode = SDL_SCANCODE_RSHIFT; event.key.mod = 0;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_LALT;
    event.key.mod = SDL_KMOD_LALT;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE));
    CHECK(msg.wParam == VK_MENU && !(msg.lParam & (1u << 29)) && GetKeyState(VK_MENU) < 0);
    event.type = SDL_EVENT_KEY_UP; event.key.mod = 0;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_SYSKEYUP, WM_SYSKEYUP, PM_REMOVE));
    CHECK(msg.wParam == VK_MENU && (msg.lParam & (1u << 29)) && !(GetKeyState(VK_MENU) & 0x8000));
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_CAPSLOCK;
    event.key.mod = SDL_KMOD_CAPS;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(GetKeyState(VK_CAPITAL) < 0 && (GetKeyState(VK_CAPITAL) & 1));
    event.type = SDL_EVENT_KEY_UP;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(!(GetKeyState(VK_CAPITAL) & 0x8000) && (GetKeyState(VK_CAPITAL) & 1));
    event = (SDL_Event){0}; event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = hwnd->id;
    event.text.text = "A\xc3\xa9\xe2\x82\xac\xe2\x80\x94\xf0\x9f\x98\x80";
    CHECK(SDL_PushEvent(&event));
    { const BYTE rgch[] = {'A', 0xe9, 0x80, 0x97, '?'};
      for (i = 0; i < 5; i++) {
          CHECK(PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE));
          CHECK(msg.wParam == rgch[i]);
      }
    }
    event.text.text = "prefix\xe2\x82";
    CHECK(SDL_PushEvent(&event));
    CHECK(!PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE));
    event.text.text = "\xed\xa0\x80";
    CHECK(SDL_PushEvent(&event));
    CHECK(!PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE));
    event = (SDL_Event){0}; event.type = SDL_EVENT_WINDOW_FOCUS_LOST; event.window.windowID = hwnd->id;
    CompatRouteEvent(&event); Drain();
    CHECK(!(GetKeyState(VK_OEM_COMMA) & 0x8000));
    return 0;
}
#endif
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    HINSTANCE hInstance;
    MSG msg;
    POINT pt;
    SHORT w;
    int i;
    static const UINT rgControl[] = {VK_BACK, VK_TAB, VK_RETURN, VK_ESCAPE};
    static const UINT rgScan[] = {0x0e, 0x0f, 0x1c, 0x01};
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.hInstance = hInstance; wc.lpszClassName = "StarsInputContracts"; wc.lpfnWndProc = InputProc; wc.style = CS_DBLCLKS;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "input", WS_POPUP | WS_VISIBLE, 40, 50, 320, 240,
                        NULL, NULL, hInstance, NULL);
    CHECK(hwnd);
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    CHECK(SetForegroundWindow(hwnd));
#endif
    CHECK(TestPoints(hwnd, hInstance) == 0);
    SetFocus(hwnd); Drain();
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(QueueShift(hwnd) == 0);
    CHECK(WaitKey(hwnd, &msg, PM_NOREMOVE) == 0 && msg.message == WM_KEYDOWN);
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
    CHECK(WaitKey(hwnd, &msg, PM_REMOVE) == 0 && msg.wParam == VK_SHIFT);
    CHECK(GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_LSHIFT) < 0);
    CHECK(WaitKey(hwnd, &msg, PM_REMOVE) == 0 && msg.message == WM_KEYUP);
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    w = GetKeyState('Z');
    CHECK(PostMessage(hwnd, WM_KEYDOWN, 'Z', 1));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(GetKeyState('Z') == w);
    for (i = 0; i < 4; i++) {
        CHECK(PostMessage(hwnd, WM_KEYDOWN, rgControl[i], (rgScan[i] << 16) | 1));
        CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
        CHECK(TranslateMessage(&msg));
        CHECK(PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE) && msg.wParam == rgControl[i]);
    }
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    CHECK(SetCursorPos(120, 130) && GetCursorPos(&pt) && pt.x == 120 && pt.y == 130);
#else
    (void)pt;
    if (strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        CHECK(SDL_WarpMouseGlobal(120, 130));
        CHECK(GetCursorPos(&pt) && pt.x == 120 && pt.y == 130);
        Drain();
    }
    CHECK(TestSnapshots(hwnd) == 0 && TestPhysical() == 0 && TestWheelClicks(hwnd, hInstance) == 0);
#endif
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
    CHECK(GetKeyState('A') == 0 && GetAsyncKeyState('A') == 0);
#endif
    puts("Win32 input state contracts passed");
    return 0;
}
