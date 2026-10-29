/* Desktop geometry and coordinate mapping shared with the Windows SDK. */
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <unistd.h>
static BOOL fFake, fFailDisplay;
SDL_DisplayID *SDL_GetDisplays(int *c);
SDL_DisplayID SDL_GetPrimaryDisplay(void);
bool SDL_GetDisplayBounds(SDL_DisplayID id, SDL_Rect *rc);
bool SDL_GetDisplayUsableBounds(SDL_DisplayID id, SDL_Rect *rc);
extern "C" SDL_DisplayID *SDLCALL StarsTest_SDL_GetDisplays(int *c) {
    SDL_DisplayID *rg;
    if (!fFake) return SDL_GetDisplays(c);
    rg = (SDL_DisplayID*)(SDL_calloc(3, sizeof(*rg))); if (!rg) return NULL;
    rg[0] = 1; rg[1] = 2; *c = 2; return rg;
}
extern "C" SDL_DisplayID SDLCALL StarsTest_SDL_GetPrimaryDisplay(void) { return fFake ? 2 : SDL_GetPrimaryDisplay(); }
extern "C" bool SDLCALL StarsTest_SDL_GetDisplayBounds(SDL_DisplayID id, SDL_Rect *rc) {
    if (!fFake) return SDL_GetDisplayBounds(id, rc);
    if (id == 1) { *rc = (SDL_Rect){-640, -80, 640, 480}; return true; }
    if (id == 2 && !fFailDisplay) { *rc = (SDL_Rect){0, 0, 800, 600}; return true; }
    return false;
}
extern "C" bool SDLCALL StarsTest_SDL_GetDisplayUsableBounds(SDL_DisplayID id, SDL_Rect *rc) {
    if (!fFake) return SDL_GetDisplayUsableBounds(id, rc);
    if (id == 2 && !fFailDisplay) { *rc = (SDL_Rect){0, 20, 800, 560}; return true; }
    return false;
}
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { return DefWindowProc(hwnd, msg, w, l); }
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndA, hwndB;
    RECT rc, rcVirt;
    POINT rgpt[3] = {{1, 2}, {-3, 4}, {5, -6}}, rgOld[3], ptA = {0, 0}, ptB = {0, 0};
    UINT cLines = UINT_MAX;
    DWORD dwProcess = 0;
    int result, dx, dy, i;
    CHECK(GetSystemMetrics(SM_CXSCREEN) > 0 && GetSystemMetrics(SM_CYSCREEN) > 0);
    CHECK(GetSystemMetrics(SM_CMONITORS) > 0);
    SetRect(&rcVirt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), 0, 0);
    rcVirt.right = rcVirt.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rcVirt.bottom = rcVirt.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    CHECK(SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0));
    CHECK(rc.left >= rcVirt.left && rc.top >= rcVirt.top && rc.right <= rcVirt.right && rc.bottom <= rcVirt.bottom);
    CHECK(rc.right > rc.left && rc.bottom > rc.top);
    CHECK(SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &cLines, 0));
    CHECK(cLines <= INT_MAX || cLines == WHEEL_PAGESCROLL);
    CHECK(GetSystemMetrics(SM_CXFRAME) > 0 && GetSystemMetrics(SM_CYCAPTION) > 0);
    CHECK(GetSystemMetrics(-1) == 0);
    wc.lpfnWndProc = WndProc; wc.lpszClassName = "CompatDesktop";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Desktop", WS_POPUP, 70, 80, 400, 300, NULL, NULL, NULL, NULL);
    hwndA = CreateWindow(wc.lpszClassName, "A", WS_CHILD, -10, 20, 50, 60, hwnd, NULL, NULL, NULL);
    hwndB = CreateWindow(wc.lpszClassName, "B", WS_CHILD, 30, -15, 50, 60, hwnd, NULL, NULL, NULL);
    CHECK(hwnd && hwndA && hwndB);
    CHECK(GetWindowThreadProcessId(hwndA, &dwProcess) == GetCurrentThreadId());
#ifdef _WIN32
    CHECK(dwProcess == GetCurrentProcessId());
#else
    CHECK(dwProcess == (DWORD)getpid());
#endif
    CHECK(GetWindowThreadProcessId(hwndB, NULL) == GetCurrentThreadId());
    CHECK(ClientToScreen(hwndA, &ptA) && ClientToScreen(hwndB, &ptB));
    dx = ptA.x - ptB.x; dy = ptA.y - ptB.y;
    memcpy(rgOld, rgpt, sizeof(rgpt));
    result = MapWindowPoints(hwndA, hwndB, rgpt, 3);
    CHECK((short)LOWORD(result) == dx && (short)HIWORD(result) == dy);
    for (i = 0; i < 3; i++) CHECK(rgpt[i].x == rgOld[i].x + dx && rgpt[i].y == rgOld[i].y + dy);
    MapWindowPoints(hwndB, hwndA, rgpt, 3); CHECK(memcmp(rgpt, rgOld, sizeof(rgpt)) == 0);
    SetLastError(0); CHECK(MapWindowPoints(hwndA, hwndA, rgpt, 3) == 0 && GetLastError() == 0);
    MapWindowPoints(hwndA, NULL, rgpt, 3);
    CHECK(rgpt[0].x == rgOld[0].x + ptA.x && rgpt[0].y == rgOld[0].y + ptA.y);
    MapWindowPoints(NULL, hwndA, rgpt, 3); CHECK(memcmp(rgpt, rgOld, sizeof(rgpt)) == 0);
    SetRect(&rc, 9, 8, 2, 1); MapWindowPoints(hwndA, hwndB, (POINT *)&rc, 2);
    CHECK(rc.left == 9 + dx && rc.right == 2 + dx && rc.top == 8 + dy && rc.bottom == 1 + dy);
    CHECK(DestroyWindow(hwndA)); SetLastError(0);
    CHECK(GetWindowThreadProcessId(hwndA, &dwProcess) == 0 && GetLastError() == ERROR_INVALID_WINDOW_HANDLE);
#ifndef _WIN32
    {
        SDL_Rect bounds;
        POINT rgOverflow[2] = {{0, 0}, {INT_MAX, INT_MAX}}, rgBefore[2];
        CHECK(SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &bounds));
        CHECK(GetSystemMetrics(SM_CXSCREEN) == bounds.w && GetSystemMetrics(SM_CYSCREEN) == bounds.h);
        CHECK(GetSystemMetrics(SM_CXFRAME) == 8 && GetSystemMetrics(SM_CYCAPTION) == 23);
        memcpy(rgBefore, rgOverflow, sizeof(rgBefore)); SetLastError(0);
        CHECK(MapWindowPoints(hwndB, NULL, rgOverflow, 2) == 0 && GetLastError() == ERROR_ARITHMETIC_OVERFLOW);
        CHECK(memcmp(rgOverflow, rgBefore, sizeof(rgBefore)) == 0);
        SetLastError(0); CHECK(!SystemParametersInfo(SPI_GETWORKAREA, 0, NULL, 0) && GetLastError() == ERROR_INVALID_PARAMETER);
        fFake = TRUE;
        CHECK(GetSystemMetrics(SM_CXSCREEN) == 800 && GetSystemMetrics(SM_CYSCREEN) == 600);
        CHECK(GetSystemMetrics(SM_CMONITORS) == 2);
        CHECK(GetSystemMetrics(SM_XVIRTUALSCREEN) == -640 && GetSystemMetrics(SM_YVIRTUALSCREEN) == -80);
        CHECK(GetSystemMetrics(SM_CXVIRTUALSCREEN) == 1440 && GetSystemMetrics(SM_CYVIRTUALSCREEN) == 680);
        CHECK(SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0) && rc.top == 20 && rc.bottom == 580);
        fFailDisplay = TRUE; CHECK(GetSystemMetrics(SM_CXSCREEN) == 0 && GetSystemMetrics(SM_CXVIRTUALSCREEN) == 0);
        SetRect(&rc, 1, 2, 3, 4); CHECK(!SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0));
        CHECK(rc.left == 1 && rc.top == 2 && rc.right == 3 && rc.bottom == 4);
        fFake = fFailDisplay = FALSE;
    }
#endif
    CHECK(DestroyWindow(hwnd));
#ifndef _WIN32
    StarsCompatShutdown();
    CHECK(!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO));
    CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO));
    CHECK(GetSystemMetrics(SM_CXSCREEN) > 0);
    StarsCompatShutdown();
    CHECK(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
    puts("Win32 desktop services passed");
    return 0;
}
