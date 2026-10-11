/* Timer generation and callbacks are UI-thread message contracts, not worker callbacks. */
#include <windows.h>
#ifndef _WIN32
#include <stars_compat.h>
#include <SDL3/SDL.h>
#define Pause(ms) SDL_Delay(ms)
#else
#define Pause(ms) Sleep(ms)
#endif
#include <stdio.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cTimers, cCallbacks;
static UINT_PTR idCallback;

static LRESULT CALLBACK TimerProcWindow(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_TIMER) { cTimers++; return 0; }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static void CALLBACK TimerCallback(HWND hwnd, UINT msg, UINT_PTR id, DWORD ms) {
    (void)hwnd; (void)ms;
    if (msg == WM_TIMER) { cCallbacks++; idCallback = id; }
}

int main(void) {
    WNDCLASS wc = {0};
    HWND h1, h2;
    MSG msg, again;
    UINT_PTR id;

    wc.lpfnWndProc = TimerProcWindow; wc.lpszClassName = "StarsTimerReference";
    CHECK(RegisterClass(&wc));
    h1 = CreateWindow(wc.lpszClassName, "Timer 1", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    h2 = CreateWindow(wc.lpszClassName, "Timer 2", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    CHECK(h1 && h2);
    CHECK(SetTimer(h1, 55, 10, NULL) == 55 && SetTimer(h2, 55, 10, NULL) == 55);
    CHECK(GetMessage(&msg, h1, WM_TIMER, WM_TIMER) > 0 && msg.hwnd == h1 && msg.wParam == 55);
    DispatchMessage(&msg);
    CHECK(GetMessage(&msg, h2, WM_TIMER, WM_TIMER) > 0 && msg.hwnd == h2 && msg.wParam == 55);
    DispatchMessage(&msg);
    CHECK(cTimers == 2 && KillTimer(h1, 55) && KillTimer(h2, 55));
    CHECK(!KillTimer(h1, 55));
    CHECK(SetTimer(h1, 17, 1000, NULL) == 17 && SetTimer(h1, 17, 10, NULL) == 17);
    CHECK(GetMessage(&msg, h1, WM_TIMER, WM_TIMER) > 0 && msg.wParam == 17);
    CHECK(KillTimer(h1, 17));

    id = SetTimer(NULL, 0, 10, TimerCallback);
    CHECK(id);
    Pause(30);
    CHECK(cCallbacks == 0); // A sleeping thread never invokes the callback.
    CHECK(PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_NOREMOVE));
    CHECK(msg.hwnd == NULL && msg.wParam == id && msg.lParam);
    CHECK(PeekMessage(&again, NULL, WM_TIMER, WM_TIMER, PM_NOREMOVE));
    CHECK(again.hwnd == msg.hwnd && again.wParam == msg.wParam && again.lParam == msg.lParam);
#ifndef _WIN32
    // Wine can refresh a synthesized timer's timestamp between non-removing peeks.
    CHECK(again.time == msg.time);
#endif
    CHECK(PeekMessage(&again, NULL, WM_TIMER, WM_TIMER, PM_REMOVE) && again.wParam == id);
    CHECK(KillTimer(NULL, id));
    DispatchMessage(&again);
    CHECK(cCallbacks == 1 && idCallback == id);
    CHECK(!PeekMessage(&again, NULL, WM_TIMER, WM_TIMER, PM_REMOVE));
    id = SetTimer(NULL, 0, 1000, TimerCallback);
    CHECK(id && PostMessage(NULL, WM_TIMER, id, (LPARAM)TimerCallback));
    CHECK(KillTimer(NULL, id));
    CHECK(PeekMessage(&again, NULL, WM_TIMER, WM_TIMER, PM_REMOVE) && again.wParam == id);
    DispatchMessage(&again);
    CHECK(cCallbacks == 2 && idCallback == id);

    CHECK(SetTimer(h2, 19, 10, NULL) == 19);
    Pause(30);
    CHECK(PostMessage(h2, WM_APP, 77, 0));
    CHECK(PeekMessage(&msg, h2, WM_APP, WM_APP, PM_REMOVE) && msg.wParam == 77);
    CHECK(PeekMessage(&msg, h2, WM_TIMER, WM_TIMER, PM_REMOVE) && msg.wParam == 19);
    CHECK(DestroyWindow(h2) && !KillTimer(h2, 19));
    CHECK(DestroyWindow(h1));
#ifndef _WIN32
    CHECK(!SetTimer(h1, 1, 10, NULL) && GetLastError() == ERROR_INVALID_HANDLE);
    StarsCompatShutdown();
#endif
    puts("Timers: PASS (per-window IDs, filtering, replacement, callbacks, pending messages, teardown)");
    return 0;
}
