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
    WNDCLASS wc = {0};
    HWND hwnd = NULL;
    Display *lpDisplay = NULL;
    Window window;
    KeyCode key = 0;
    BOOL fPressed = FALSE;
    MSG msg;
    DWORD ms;
    int iResult = 0;
    wc.lpszClassName = "StarsPhysicalInput"; wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = (HINSTANCE)(UINT_PTR)1;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "physical input", WS_POPUP | WS_VISIBLE,
                        40, 50, 320, 240, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0);
    lpDisplay = XOpenDisplay(NULL);
    CHECK(lpDisplay);
    key = XKeysymToKeycode(lpDisplay, XK_Shift_L);
    CHECK(key != 0);
    window = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(hwnd->lpWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    CHECK(window != 0);
    XSetInputFocus(lpDisplay, window, RevertToParent, CurrentTime);
    XSync(lpDisplay, False);
    SetFocus(hwnd);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(XTestFakeKeyEvent(lpDisplay, key, True, CurrentTime));
    fPressed = TRUE;
    XSync(lpDisplay, False);
    ms = GetTickCount();
    while (!(GetAsyncKeyState(VK_SHIFT) & 0x8000) && GetTickCount() - ms < 2000) SDL_Delay(5);
    CHECK(GetAsyncKeyState(VK_SHIFT) < 0 && GetAsyncKeyState(VK_LSHIFT) < 0);
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE));
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    CHECK(PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
    CHECK(msg.wParam == VK_SHIFT && GetKeyState(VK_SHIFT) < 0);
    CHECK(XTestFakeKeyEvent(lpDisplay, key, False, CurrentTime));
    fPressed = FALSE;
    XSync(lpDisplay, False);
    ms = GetTickCount();
    while ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && GetTickCount() - ms < 2000) SDL_Delay(5);
    CHECK(!(GetAsyncKeyState(VK_SHIFT) & 0x8000) && GetKeyState(VK_SHIFT) < 0);
    CHECK(PeekMessage(&msg, hwnd, WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(!(GetKeyState(VK_SHIFT) & 0x8000));
    puts("X11 physical keyboard contracts passed");
LExit:
    if (lpDisplay) {
        if (fPressed) { XTestFakeKeyEvent(lpDisplay, key, False, CurrentTime); XSync(lpDisplay, False); }
        XCloseDisplay(lpDisplay);
    }
    if (hwnd) DestroyWindow(hwnd);
    StarsCompatShutdown();
    return iResult;
}
