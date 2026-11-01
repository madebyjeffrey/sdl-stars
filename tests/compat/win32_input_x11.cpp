/* Real X11 keyboard delivery: run only under a private Xvfb display. */
#include <windows.h>
#include <stars_compat.h>
#include "internal.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); iResult = 1; goto LExit; } } while (0)
int main(void) {
    WNDCLASS wc = {};
    HWND hwnd = NULL;
    Display *lpDisplay = NULL;
    Window window;
    KeyCode key = 0;
    BOOL fPressed = 0;
    win32::MSG msg;
    DWORD ms;
    int iResult = 0;
    wc.lpszClassName = "StarsPhysicalInput"; wc.lpfnWndProc = win32::DefWindowProcA;
    wc.hInstance = (HINSTANCE)(UINT_PTR)1;
    CHECK(win32::RegisterClassA(&wc));
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "physical input", win32::WS_POPUP | win32::WS_VISIBLE,
                        40, 50, 320, 240, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0);
    lpDisplay = XOpenDisplay(NULL);
    CHECK(lpDisplay);
    key = XKeysymToKeycode(lpDisplay, XK_Shift_L);
    CHECK(key != 0);
    window = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(hwnd->lpWindow.get()), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    CHECK(window != 0);
    XSetInputFocus(lpDisplay, window, RevertToParent, CurrentTime);
    XSync(lpDisplay, False);
    win32::SetFocus(hwnd);
    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) win32::DispatchMessageA(&msg);
    CHECK(!(win32::GetKeyState(win32::VK_SHIFT) & 0x8000));
    CHECK(XTestFakeKeyEvent(lpDisplay, key, True, CurrentTime));
    fPressed = 1;
    XSync(lpDisplay, False);
    ms = win32::GetTickCount();
    while (!(win32::GetAsyncKeyState(win32::VK_SHIFT) & 0x8000) && win32::GetTickCount() - ms < 2000) SDL_Delay(5);
    CHECK(win32::GetAsyncKeyState(win32::VK_SHIFT) < 0 && win32::GetAsyncKeyState(win32::VK_LSHIFT) < 0);
    CHECK(!(win32::GetKeyState(win32::VK_SHIFT) & 0x8000));
    CHECK(win32::PeekMessageA(&msg, hwnd, win32::WM_KEYDOWN, win32::WM_KEYDOWN, win32::PM_NOREMOVE));
    CHECK(!(win32::GetKeyState(win32::VK_SHIFT) & 0x8000));
    CHECK(win32::PeekMessageA(&msg, hwnd, win32::WM_KEYDOWN, win32::WM_KEYDOWN, win32::PM_REMOVE));
    CHECK(msg.wParam == win32::VK_SHIFT && win32::GetKeyState(win32::VK_SHIFT) < 0);
    CHECK(XTestFakeKeyEvent(lpDisplay, key, False, CurrentTime));
    fPressed = 0;
    XSync(lpDisplay, False);
    ms = win32::GetTickCount();
    while ((win32::GetAsyncKeyState(win32::VK_SHIFT) & 0x8000) && win32::GetTickCount() - ms < 2000) SDL_Delay(5);
    CHECK(!(win32::GetAsyncKeyState(win32::VK_SHIFT) & 0x8000) && win32::GetKeyState(win32::VK_SHIFT) < 0);
    CHECK(win32::PeekMessageA(&msg, hwnd, win32::WM_KEYUP, win32::WM_KEYUP, win32::PM_REMOVE));
    CHECK(!(win32::GetKeyState(win32::VK_SHIFT) & 0x8000));
    puts("X11 physical keyboard contracts passed");
LExit:
    if (lpDisplay) {
        if (fPressed) { XTestFakeKeyEvent(lpDisplay, key, False, CurrentTime); XSync(lpDisplay, False); }
        XCloseDisplay(lpDisplay);
    }
    if (hwnd) win32::DestroyWindow(hwnd);
    StarsCompatShutdown();
    return iResult;
}
