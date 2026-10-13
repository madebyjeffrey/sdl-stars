/* Caption-state messaging and nonactivating attention requests. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#ifdef STARS_TEST_ATTENTION_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
static BOOL fMock = TRUE, fFail;
static int cFlash;
static SDL_Window *lpWindowLast;
static SDL_FlashOperation operationLast;
bool SDL_FlashWindow(SDL_Window *lpWindow, SDL_FlashOperation operation);
bool SDLCALL StarsTest_SDL_FlashWindow(SDL_Window *lpWindow, SDL_FlashOperation operation) {
    cFlash++; lpWindowLast = lpWindow; operationLast = operation;
    if (!fMock) return SDL_FlashWindow(lpWindow, operation);
    return !fFail;
}
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
typedef struct ATTENTIONTEST { BOOL fCaption, fDestroy; int cCaption; } ATTENTIONTEST;
static LRESULT CALLBACK AttentionProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    ATTENTIONTEST *lptest = (ATTENTIONTEST *)GetWindowLongPtr(hwnd, 0);
    if (msg == WM_NCCREATE) {
        lptest = (ATTENTIONTEST *)((CREATESTRUCT *)l)->lpCreateParams;
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)lptest);
    }
    if (msg == WM_NCACTIVATE && lptest) {
        lptest->fCaption = !!w; lptest->cCaption++;
        if (lptest->fDestroy) { lptest->fDestroy = FALSE; DestroyWindow(hwnd); return 0; }
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}
#if defined(STARS_TEST_ATTENTION_X11) && !defined(_WIN32)
static int TestX11Focus(HWND hwndPeer) {
    Display *lpDisplay;
    Window windowPeer, windowFocus;
    int revert;
    DWORD ms;
    // The first paint can recreate SDL's X window for framebuffer acceleration.
    Drain();
    lpDisplay = XOpenDisplay(NULL); CHECK(lpDisplay);
    windowPeer = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(hwndPeer->lpWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    CHECK(windowPeer);
    XSetInputFocus(lpDisplay, windowPeer, RevertToParent, CurrentTime); XSync(lpDisplay, False);
    ms = GetTickCount();
    do {
        Drain();
        if (SDL_GetWindowFlags(hwndPeer->lpWindow) & SDL_WINDOW_INPUT_FOCUS) break;
        SDL_Delay(5);
    } while (GetTickCount() - ms < 2000);
    XGetInputFocus(lpDisplay, &windowFocus, &revert);
    if (!(SDL_GetWindowFlags(hwndPeer->lpWindow) & SDL_WINDOW_INPUT_FOCUS))
        fprintf(stderr, "X11 target=%lu focus=%lu flags=%llu\n", (unsigned long)windowPeer,
                (unsigned long)windowFocus, (unsigned long long)SDL_GetWindowFlags(hwndPeer->lpWindow));
    XCloseDisplay(lpDisplay);
    CHECK(SDL_GetWindowFlags(hwndPeer->lpWindow) & SDL_WINDOW_INPUT_FOCUS);
    return 0;
}

static int TestX11(HWND hwnd, HWND hwndPeer) {
    Display *lpDisplay, *lpSDLDisplay;
    Window window, windowPeer, windowFocus;
    XWMHints *lpHints;
    int revert;
    BOOL fUrgent;
    CHECK(strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0);
    lpDisplay = XOpenDisplay(NULL); CHECK(lpDisplay);
    lpSDLDisplay = SDL_GetPointerProperty(SDL_GetWindowProperties(hwnd->lpWindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    CHECK(lpSDLDisplay);
    window = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(hwnd->lpWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    windowPeer = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(hwndPeer->lpWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    CHECK(window && windowPeer);
    XSetInputFocus(lpDisplay, windowPeer, RevertToParent, CurrentTime); XSync(lpDisplay, False);
    SDL_Delay(20); Drain(); SetFocus(hwndPeer);
    SendMessage(hwnd, WM_NCACTIVATE, FALSE, 0);
    fMock = FALSE;
    SetLastError(0);
    CHECK(!FlashWindow(hwnd, TRUE) && GetLastError() == 0);
    XSync(lpSDLDisplay, False);
    XSync(lpDisplay, False); lpHints = XGetWMHints(lpDisplay, window); CHECK(lpHints);
    fUrgent = !!(lpHints->flags & XUrgencyHint); XFree(lpHints); CHECK(fUrgent);
    CHECK(FlashWindow(hwnd, TRUE));
    XSync(lpSDLDisplay, False);
    XSync(lpDisplay, False); lpHints = XGetWMHints(lpDisplay, window); CHECK(lpHints);
    fUrgent = !!(lpHints->flags & XUrgencyHint); XFree(lpHints); CHECK(!fUrgent);
    XGetInputFocus(lpDisplay, &windowFocus, &revert);
    CHECK(windowFocus == windowPeer && GetActiveWindow() == hwndPeer && GetFocus() == hwndPeer);
    XCloseDisplay(lpDisplay); fMock = TRUE;
    return 0;
}
#endif
int main(int argc, char **argv) {
    WNDCLASS wc = {0};
    ATTENTIONTEST test = {0}, peer = {0};
    HWND hwnd, hwndPeer, hwndChild = NULL;
    HINSTANCE hInstance;
    BOOL fResult;
    int cCaption;
#ifndef _WIN32
    int cOld;
#endif
#ifdef _WIN32
    hInstance = GetModuleHandle(NULL);
    (void)argc; (void)argv;
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    wc.hInstance = hInstance; wc.lpszClassName = "StarsAttentionContracts";
    wc.lpfnWndProc = AttentionProc; wc.cbWndExtra = sizeof(LONG_PTR);
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "attention", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        40, 50, 320, 240, NULL, NULL, hInstance, &test);
    hwndPeer = CreateWindow(wc.lpszClassName, "peer", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            400, 50, 320, 240, NULL, NULL, hInstance, &peer);
    CHECK(hwnd && hwndPeer);
#if defined(STARS_TEST_ATTENTION_X11) && !defined(_WIN32)
    if (argc == 2 && strcmp(argv[1], "--x11") == 0) CHECK(TestX11Focus(hwndPeer) == 0);
#endif
#ifdef _WIN32
    CHECK(SetForegroundWindow(hwndPeer));
#endif
    SetFocus(hwndPeer); Drain();
    SendMessage(hwnd, WM_NCACTIVATE, FALSE, 0);
    cCaption = test.cCaption;
    fResult = FlashWindow(hwnd, TRUE);
    CHECK(test.fCaption && test.cCaption == cCaption + 1);
    CHECK(GetActiveWindow() == hwndPeer && GetFocus() == hwndPeer);
#ifndef _WIN32
    CHECK(!fResult && operationLast == SDL_FLASH_BRIEFLY && lpWindowLast == hwnd->lpWindow);
#else
    (void)fResult; // Wine 9 returns the new state; the documented prior-state return is checked natively.
#endif
    fResult = FlashWindow(hwnd, TRUE);
    CHECK(!test.fCaption && GetActiveWindow() == hwndPeer && GetFocus() == hwndPeer);
#ifndef _WIN32
    CHECK(fResult && operationLast == SDL_FLASH_CANCEL);
#endif
    FlashWindow(hwnd, FALSE); CHECK(!test.fCaption);
    SendMessage(hwndPeer, WM_NCACTIVATE, TRUE, 0);
    fResult = FlashWindow(hwndPeer, TRUE); CHECK(!peer.fCaption);
#ifndef _WIN32
    CHECK(fResult && operationLast == SDL_FLASH_CANCEL);
#endif
    fResult = FlashWindow(hwndPeer, FALSE); CHECK(peer.fCaption);
#ifndef _WIN32
    CHECK(!fResult);
    SendMessage(hwnd, WM_NCACTIVATE, TRUE, 0);
    cCaption = test.cCaption; fFail = TRUE;
    CHECK(!FlashWindow(hwnd, TRUE) && GetLastError() == ERROR_NOT_SUPPORTED);
    CHECK(test.fCaption && test.cCaption == cCaption);
    fFail = FALSE;
    CHECK(FlashWindow(hwnd, FALSE) && !test.fCaption);
    hwndChild = CreateWindow("STATIC", "child", WS_CHILD, 0, 0, 20, 20,
                             hwndPeer, (HMENU)(INT_PTR)101, hInstance, NULL);
    CHECK(hwndChild); cOld = cFlash;
    CHECK(!FlashWindow(hwndChild, TRUE) && GetLastError() == ERROR_NOT_SUPPORTED && cFlash == cOld);
    CHECK(DestroyWindow(hwndChild));
    if (argc == 2 && strcmp(argv[1], "--x11") == 0) {
#ifdef STARS_TEST_ATTENTION_X11
        CHECK(TestX11(hwnd, hwndPeer) == 0);
#else
        CHECK(FALSE);
#endif
    } else CHECK(argc == 1);
#else
    (void)hwndChild;
#endif
    SendMessage(hwnd, WM_NCACTIVATE, FALSE, 0);
    test.fDestroy = TRUE;
    FlashWindow(hwnd, TRUE); CHECK(!IsWindow(hwnd));
#ifndef _WIN32
    cOld = cFlash;
    CHECK(!FlashWindow(hwnd, TRUE) && GetLastError() == ERROR_INVALID_WINDOW_HANDLE && cFlash == cOld);
#endif
    CHECK(DestroyWindow(hwndPeer));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Win32 attention contracts passed");
    return 0;
}
