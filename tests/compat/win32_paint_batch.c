/* Completed panes must reach the renderer together, including native live resize. */
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
static unsigned cPresent;

extern "C" bool SDLCALL StarsTest_SDL_RenderPresent(SDL_Renderer *lpRenderer) {
    cPresent++;
    return SDL_RenderPresent(lpRenderer);
}


#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
static HWND rghwndPane[2];
static COLORREF rgcrPane[2] = {RGB(20, 80, 120), RGB(100, 140, 180)};
static HICON hicon;
static BOOL fShowLayout;
static unsigned cPaint;

static COLORREF CrComposite(HWND hwnd, int x, int y) {
    BYTE r, g, b, a;
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin || !lpwin->lpComposite.get() ||
        !SDL_ReadSurfacePixel(lpwin->lpComposite.get(), x * lpwin->iCanvasScale, y * lpwin->iCanvasScale, &r, &g, &b, &a))
        return CLR_INVALID;
    return RGB(r, g, b);
}

static LRESULT CALLBACK PaintProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    PAINTSTRUCT ps;
    RECT rc;
    HBRUSH hbr;
    int i = (int)GetWindowLongPtr(hwnd, GWLP_ID);
    SDL_Event event = {0};

    if (msg == WM_APP) {
        event.type = SDL_EVENT_WINDOW_EXPOSED;
        event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
        SDL_PushEvent(&event);
        return 0;
    }
    if (msg == WM_SHOWWINDOW && w && fShowLayout && !(hwnd->dwStyle & WS_CHILD)) {
        rghwndPane[0] = CreateWindow("StarsPaintBatch", "Late pane", WS_CHILD | WS_VISIBLE,
                                    0, 0, 160, 160, hwnd, (HMENU)(INT_PTR)0, GetModuleHandle(NULL), NULL);
        return 0;
    }
    if (msg == WM_SIZE && !(hwnd->dwStyle & WS_CHILD)) {
        if (IsWindow(rghwndPane[0])) {
            MoveWindow(rghwndPane[0], 0, 0, LOWORD(l) / 2, HIWORD(l), FALSE);
            MoveWindow(rghwndPane[1], LOWORD(l) / 2, 0, LOWORD(l) - LOWORD(l) / 2, HIWORD(l), FALSE);
            InvalidateRect(rghwndPane[0], NULL, TRUE);
            InvalidateRect(rghwndPane[1], NULL, TRUE);
        }
        return 0;
    }
    if (msg == WM_PAINT) {
        cPaint++;
        BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        hbr = CreateSolidBrush((hwnd->dwStyle & WS_CHILD) ? rgcrPane[i] : RGB(200, 200, 200));
        FillRect(ps.hdc, &rc, hbr);
        DeleteObject(hbr);
        if (hwnd->dwStyle & WS_CHILD) DrawIcon(ps.hdc, 8, 8, hicon);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd;
    MSG msg;
    SDL_Event event = {0};
    HDC hdc;
    HBRUSH hbr;
    RECT rc;
    unsigned cPaintSav;
    unsigned cPresentSav;

    wc.lpfnWndProc = PaintProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "StarsPaintBatch";
    CHECK(RegisterClass(&wc));
    hicon = LoadIcon(wc.hInstance, "StarsIco");
    CHECK(hicon);
    hwnd = CreateWindow(wc.lpszClassName, "Paint batch", WS_OVERLAPPEDWINDOW, 0, 0, 320, 160,
                        NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    rghwndPane[0] = CreateWindow(wc.lpszClassName, "Left", WS_CHILD | WS_VISIBLE, 0, 0, 160, 160,
                                hwnd, (HMENU)(INT_PTR)0, wc.hInstance, NULL);
    rghwndPane[1] = CreateWindow(wc.lpszClassName, "Right", WS_CHILD | WS_VISIBLE, 160, 0, 160, 160,
                                hwnd, (HMENU)(INT_PTR)1, wc.hInstance, NULL);
    CHECK(rghwndPane[0] && rghwndPane[1]);
    ShowWindow(hwnd, SW_SHOW);
    // The first frame is ready before any queued paint message is dispatched.
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0] && CrComposite(hwnd, 240, 80) == rgcrPane[1]);
    CHECK(InvalidateRect(rghwndPane[0], NULL, TRUE));
    cPaintSav = cPaint;
    cPresentSav = cPresent;
    // Native exposure can arrive inside menu/window dispatch, while new paint
    // is pending. It must resubmit the completed texture without application paint.
    SendMessage(hwnd, WM_APP, 0, 0);
    CHECK(cPaint == cPaintSav && rghwndPane[0]->fUpdate);
    CHECK(cPresent == cPresentSav + 1);
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0]);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    rgcrPane[0] = RGB(180, 20, 30); rgcrPane[1] = RGB(30, 180, 20);
    CHECK(InvalidateRect(rghwndPane[0], NULL, TRUE) && InvalidateRect(rghwndPane[1], NULL, TRUE));
    CHECK(UpdateWindow(rghwndPane[0]));
    CHECK(CrComposite(hwnd, 80, 80) == RGB(20, 80, 120) && CrComposite(hwnd, 240, 80) == RGB(100, 140, 180));
    CHECK(UpdateWindow(rghwndPane[1]));
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0] && CrComposite(hwnd, 240, 80) == rgcrPane[1]);
    CHECK(InvalidateRect(rghwndPane[0], NULL, FALSE));
    hdc = GetDC(rghwndPane[0]);
    hbr = CreateSolidBrush(RGB(50, 60, 200));
    CHECK(hdc && hbr && GetClientRect(rghwndPane[0], &rc) && FillRect(hdc, &rc, hbr));
    CHECK(DeleteObject(hbr) && ReleaseDC(rghwndPane[0], hdc));
    CHECK(ValidateRect(rghwndPane[0], NULL));
    PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    rgcrPane[0] = RGB(50, 60, 200);
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0]);
    CHECK(SDL_SetWindowSize(hwnd->lpWindow.get(), 400, 200) && SDL_SyncWindow(hwnd->lpWindow.get()));
    event.type = SDL_EVENT_WINDOW_EXPOSED;
    event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
    event.window.data1 = 1;
    CHECK(SDL_PushEvent(&event));
    // The live-resize watcher lays out and paints at native size before the main loop resumes.
    CHECK(hwnd->lpComposite->w == 400 * hwnd->iCanvasScale && hwnd->lpComposite->h == 200 * hwnd->iCanvasScale);
    CHECK(rghwndPane[1]->rcChild.left == 200 && CrComposite(hwnd, 199, 100) == rgcrPane[0] &&
          CrComposite(hwnd, 200, 100) == rgcrPane[1]);
    CHECK(DestroyWindow(hwnd));
    rghwndPane[0] = rghwndPane[1] = NULL;
    fShowLayout = TRUE;
    hwnd = CreateWindow(wc.lpszClassName, "Show-time layout", WS_OVERLAPPEDWINDOW,
                        0, 0, 320, 160, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    CHECK(IsWindow(rghwndPane[0]));
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0]);
    CHECK(!hwnd->fPresentPending && !rghwndPane[0]->fUpdate);
    printf("First frame, show-time controls, pane batching and native live-resize layout passed (density %d)\n",
           hwnd->iCanvasScale);
    StarsCompatShutdown();
    return 0;
}
