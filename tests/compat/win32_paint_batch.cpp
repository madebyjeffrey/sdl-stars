/* Completed panes must reach the renderer together, including native live resize. */
#include "internal.h"
#include "../../compat/win32/src/render_io.h"

#include <stars_compat.h>
#include <stdio.h>
static unsigned cPresent;
static BOOL fFailPresent, fFailSize, fDestroySize, fFailPaint, fFailShow, fDestroyShow;

extern "C" bool SDLCALL StarsTest_SDL_RenderPresent(SDL_Renderer *lpRenderer) {
    cPresent++;
    if (fFailPresent) { fFailPresent = 0; StarsThrowError(starsErrorFile, 731); }
    return SDL_RenderPresent(lpRenderer);
}


#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
static HWND rghwndPane[2];
static COLORREF rgcrPane[2] = {win32::RGB(20, 80, 120), win32::RGB(100, 140, 180)};
static HICON hicon;
static BOOL fShowLayout;
static unsigned cPaint;

static COLORREF CrComposite(HWND hwnd, int x, int y) {
    BYTE r, g, b, a;
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin || !lpwin->lpComposite.get() ||
        !SDL_ReadSurfacePixel(lpwin->lpComposite.get(), x * lpwin->iCanvasScale, y * lpwin->iCanvasScale, &r, &g, &b, &a))
        return win32::CLR_INVALID;
    return win32::RGB(r, g, b);
}

static LRESULT  PaintProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    PAINTSTRUCT ps;
    RECT rc;
    HBRUSH hbr;
    int i = (int)win32::GetWindowLongPtrA(hwnd, win32::GWLP_ID);
    SDL_Event event = {};

    if (msg == win32::WM_APP) {
        event.type = SDL_EVENT_WINDOW_EXPOSED;
        event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
        SDL_PushEvent(&event);
        return 0;
    }
    if (msg == win32::WM_SHOWWINDOW && w && !(hwnd->dwStyle & win32::WS_CHILD)) {
        if (fFailShow) StarsThrowError(starsErrorFile, 732);
        if (fDestroyShow) { win32::DestroyWindow(hwnd); return 0; }
    }
    if (msg == win32::WM_SHOWWINDOW && w && fShowLayout && !(hwnd->dwStyle & win32::WS_CHILD)) {
        rghwndPane[0] = win32::CreateWindowExA(0, "StarsPaintBatch", "Late pane", win32::WS_CHILD | win32::WS_VISIBLE,
                                    0, 0, 160, 160, hwnd, (HMENU)(INT_PTR)0, win32::GetModuleHandleA(NULL), NULL);
        return 0;
    }
    if (msg == win32::WM_SIZE && !(hwnd->dwStyle & win32::WS_CHILD)) {
        if (fFailSize) StarsThrowError(starsErrorFile, 730);
        if (fDestroySize) { win32::DestroyWindow(hwnd); return 0; }
        if (win32::IsWindow(rghwndPane[0])) {
            win32::MoveWindow(rghwndPane[0], 0, 0, LOWORD(l) / 2, HIWORD(l), 0);
            win32::MoveWindow(rghwndPane[1], LOWORD(l) / 2, 0, LOWORD(l) - LOWORD(l) / 2, HIWORD(l), 0);
            win32::InvalidateRect(rghwndPane[0], NULL, 1);
            win32::InvalidateRect(rghwndPane[1], NULL, 1);
        }
        return 0;
    }
    if (msg == win32::WM_PAINT) {
        if (fFailPaint) StarsThrowError(starsErrorFile, 733);
        cPaint++;
        win32::BeginPaint(hwnd, &ps);
        win32::GetClientRect(hwnd, &rc);
        hbr = win32::CreateSolidBrush((hwnd->dwStyle & win32::WS_CHILD) ? rgcrPane[i] : win32::RGB(200, 200, 200));
        win32::FillRect(ps.hdc, &rc, hbr);
        win32::DeleteObject(hbr);
        if (hwnd->dwStyle & win32::WS_CHILD) win32::DrawIcon(ps.hdc, 8, 8, hicon);
        win32::EndPaint(hwnd, &ps);
        return 0;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}

int main(void) {
    WNDCLASS wc = {};
    HWND hwnd;
    win32::MSG msg;
    SDL_Event event = {};
    HDC hdc;
    HBRUSH hbr;
    RECT rc;
    unsigned cPaintSav;
    unsigned cPresentSav;

    COMPATRENDERIO io_compatrenderio;
    io_compatrenderio.pfnRenderPresent = StarsTest_SDL_RenderPresent;
    COMPATRENDERIOSCOPE scope_compatrenderio(io_compatrenderio);

    wc.lpfnWndProc = PaintProc;
    wc.hInstance = win32::GetModuleHandleA(NULL);
    wc.lpszClassName = "StarsPaintBatch";
    CHECK(win32::RegisterClassA(&wc));
    hicon = win32::LoadIconA(wc.hInstance, "StarsIco");
    CHECK(hicon);
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Paint batch", win32::WS_OVERLAPPEDWINDOW, 0, 0, 320, 160,
                        NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    rghwndPane[0] = win32::CreateWindowExA(0, wc.lpszClassName, "Left", win32::WS_CHILD | win32::WS_VISIBLE, 0, 0, 160, 160,
                                hwnd, (HMENU)(INT_PTR)0, wc.hInstance, NULL);
    rghwndPane[1] = win32::CreateWindowExA(0, wc.lpszClassName, "Right", win32::WS_CHILD | win32::WS_VISIBLE, 160, 0, 160, 160,
                                hwnd, (HMENU)(INT_PTR)1, wc.hInstance, NULL);
    CHECK(rghwndPane[0] && rghwndPane[1]);
    win32::ShowWindow(hwnd, win32::SW_SHOW);
    // The first frame is ready before any queued paint message is dispatched.
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0] && CrComposite(hwnd, 240, 80) == rgcrPane[1]);
    CHECK(win32::InvalidateRect(rghwndPane[0], NULL, 1));
    cPaintSav = cPaint;
    cPresentSav = cPresent;
    // Native exposure can arrive inside menu/window dispatch, while new paint
    // is pending. It must resubmit the completed texture without application paint.
    win32::SendMessageA(hwnd, win32::WM_APP, 0, 0);
    CHECK(cPaint == cPaintSav && rghwndPane[0]->fUpdate);
    CHECK(cPresent == cPresentSav + 1);
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0]);
    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) win32::DispatchMessageA(&msg);
    rgcrPane[0] = win32::RGB(180, 20, 30); rgcrPane[1] = win32::RGB(30, 180, 20);
    CHECK(win32::InvalidateRect(rghwndPane[0], NULL, 1) && win32::InvalidateRect(rghwndPane[1], NULL, 1));
    CHECK(win32::UpdateWindow(rghwndPane[0]));
    CHECK(CrComposite(hwnd, 80, 80) == win32::RGB(20, 80, 120) && CrComposite(hwnd, 240, 80) == win32::RGB(100, 140, 180));
    CHECK(win32::UpdateWindow(rghwndPane[1]));
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0] && CrComposite(hwnd, 240, 80) == rgcrPane[1]);
    CHECK(win32::InvalidateRect(rghwndPane[0], NULL, 0));
    hdc = win32::GetDC(rghwndPane[0]);
    hbr = win32::CreateSolidBrush(win32::RGB(50, 60, 200));
    CHECK(hdc && hbr && win32::GetClientRect(rghwndPane[0], &rc) && win32::FillRect(hdc, &rc, hbr));
    CHECK(win32::DeleteObject(hbr) && win32::ReleaseDC(rghwndPane[0], hdc));
    CHECK(win32::ValidateRect(rghwndPane[0], NULL));
    win32::PeekMessageA(&msg, NULL, win32::WM_QUIT, win32::WM_QUIT, win32::PM_NOREMOVE);
    rgcrPane[0] = win32::RGB(50, 60, 200);
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
    CHECK(win32::DestroyWindow(hwnd));
    rghwndPane[0] = rghwndPane[1] = NULL;
    fShowLayout = 1;
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Show-time layout", win32::WS_OVERLAPPEDWINDOW,
                        0, 0, 320, 160, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    win32::ShowWindow(hwnd, win32::SW_SHOW);
    CHECK(win32::IsWindow(rghwndPane[0]));
    CHECK(CrComposite(hwnd, 80, 80) == rgcrPane[0]);
    CHECK(!hwnd->fPresentPending && !rghwndPane[0]->fUpdate);
    // A failed layout cannot paint or publish a subsequent frame. Retrying the
    // same native exposure must work after its pending error is delivered.
    event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
    cPaintSav = cPaint; cPresentSav = cPresent;
    fFailSize = 1;
    CHECK(SDL_PushEvent(&event));
    CHECK(FStarsErrorPending() && cPaint == cPaintSav && cPresent == cPresentSav && hwnd->cPresentDefer == 0);
    try { StarsRethrowPendingError(); CHECK(0); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 730); }
    fFailSize = 0;
    CHECK(SDL_PushEvent(&event) && !FStarsErrorPending() && cPaint > cPaintSav && hwnd->cPresentDefer == 0);
    fFailPaint = 1;
    cPaintSav = cPaint; cPresentSav = cPresent;
    CHECK(SDL_PushEvent(&event));
    CHECK(FStarsErrorPending() && cPaint == cPaintSav && cPresent == cPresentSav && hwnd->cPresentDefer == 0);
    try { StarsRethrowPendingError(); CHECK(0); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 733); }
    fFailPaint = 0;
    CHECK(SDL_PushEvent(&event) && !FStarsErrorPending() && cPaint > cPaintSav);
    // A renderer failure must not cross SDL's event-watcher boundary or leave
    // refresh/live-resize state latched for the next exposure.
    for (int i = 0; i < 2; i++) {
        event.window.data1 = i;
        fFailPresent = 1;
        try { CHECK(SDL_PushEvent(&event)); }
        catch (...) { CHECK(0); }
        CHECK(FStarsErrorPending() && !fFailPresent && hwnd->cPresentDefer == 0);
        try { StarsRethrowPendingError(); CHECK(0); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 731); }
        cPresentSav = cPresent;
        CHECK(SDL_PushEvent(&event) && !FStarsErrorPending() && cPresent > cPresentSav);
    }
    printf("First frame, show-time controls, pane batching and native live-resize layout passed (density %d)\n",
           hwnd->iCanvasScale);
    fDestroySize = 1;
    CHECK(SDL_PushEvent(&event));
    CHECK(!win32::IsWindow(hwnd) && hwnd->cPresentDefer == 0 && !FStarsErrorPending());
    fDestroySize = 0;
    fShowLayout = 0;
    for (int i = 0; i < 2; i++) {
        hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Interrupted show", win32::WS_OVERLAPPEDWINDOW,
                            0, 0, 320, 160, NULL, NULL, wc.hInstance, NULL);
        CHECK(hwnd);
        fFailShow = i == 0; fDestroyShow = i == 1;
        cPaintSav = cPaint;
        win32::ShowWindow(hwnd, win32::SW_SHOW);
        CHECK(cPaint == cPaintSav && hwnd->cPresentDefer == 0);
        if (fFailShow) {
            CHECK(FStarsErrorPending() && win32::IsWindow(hwnd));
            try { StarsRethrowPendingError(); CHECK(0); }
            catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 732); }
            CHECK(win32::DestroyWindow(hwnd));
        } else CHECK(!win32::IsWindow(hwnd) && !FStarsErrorPending());
    }
    StarsCompatShutdown();
    return 0;
}
