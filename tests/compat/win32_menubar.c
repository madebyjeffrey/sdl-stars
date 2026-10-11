/* Shared SDK geometry/lifecycle/keyboard contracts; SDL pixels and input below. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static UINT idCommand;
static int cEnter, cExit, iAction, iStep;
static BOOL fWrongLoop, fDestroyInit, fDemo;
#ifndef _WIN32
static BOOL fPopupContract;
#endif
static LRESULT CALLBACK BarProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_DESTROY && fDemo) PostQuitMessage(0);
    if (msg == WM_COMMAND) { idCommand = LOWORD(w); return 0; }
    if (msg == WM_ENTERMENULOOP) { cEnter++; if (w) fWrongLoop = TRUE; }
    if (msg == WM_EXITMENULOOP) { cExit++; if (w) fWrongLoop = TRUE; }
    if (msg == WM_INITMENU && fDestroyInit) DestroyWindow(hwnd);
    return DefWindowProc(hwnd, msg, w, l);
}
static void CALLBACK InputTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) {
    UINT key = 0;
    (void)msg; (void)tick;
    if (iAction == 1) { key = iStep == 0 ? VK_HOME : VK_RETURN; }
    else if (iAction == 2) { key = iStep == 0 ? VK_RIGHT : iStep == 1 ? VK_HOME : VK_RETURN; }
    else if (iAction == 3) { EndMenu(); }
#ifndef _WIN32
    else if (iAction == 5) {
        HWND hp;
        RECT rcPopup, rcBar;
        SDL_Event event = {0};
        for (hp = GetWindow(hwnd, GW_HWNDFIRST); hp; hp = GetWindow(hp, GW_HWNDNEXT)) {
            if (GetParent(hp) != hwnd || !IsWindowVisible(hp) || !hp->lpWindow ||
                !(SDL_GetWindowFlags(hp->lpWindow) & SDL_WINDOW_POPUP_MENU)) continue;
            fPopupContract = SDL_GetWindowParent(hp->lpWindow) == hwnd->lpWindow &&
                GetWindowRect(hp, &rcPopup) && GetMenuItemRect(hwnd, GetMenu(hwnd), 0, &rcBar) &&
                rcPopup.left == rcBar.left && rcPopup.top == rcBar.bottom;
            event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hp->id;
            event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
            event.button.x = 8; event.button.y = 10;
            SDL_PushEvent(&event);
            event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
            SDL_PushEvent(&event);
            break;
        }
        if (!fPopupContract) EndMenu();
    }
    else if (iAction == 4) {
        RECT rc;
        POINT pt;
        if (iStep == 0 && GetMenuItemRect(hwnd, GetMenu(hwnd), 1, &rc)) {
            pt.x = (rc.left + rc.right) / 2; pt.y = (rc.top + rc.bottom) / 2;
            ScreenToClient(hwnd, &pt);
            PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
        } else key = iStep == 1 ? VK_HOME : VK_RETURN;
    }
#endif
    if (key) PostMessage(hwnd, WM_KEYDOWN, key, 0);
    iStep++;
    if ((iAction == 1 && iStep == 2) || (iAction == 2 && iStep == 3) || iAction == 3 || (iAction == 4 && iStep == 3) || iAction == 5) KillTimer(hwnd, id);
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
}
static int OpenBar(HWND hwnd, int action) {
    iAction = action; iStep = 0; idCommand = 0;
    CHECK(SetTimer(hwnd, 17, 20, InputTimer));
    SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 'f');
    Drain();
    CHECK(cEnter == cExit && !fWrongLoop);
    return 0;
}
int main(int argc, char **argv) {
    WNDCLASS wc = {0};
    HWND hwnd, hchild;
    HMENU hmenu, hfile, hedit;
    RECT rcBefore, rcAfter, rcFile, rcEdit, rcWindow, rcWindowAfter;
    POINT ptBefore = {0, 0}, ptAfter = {0, 0}, ptChild = {0, 0};
#ifndef _WIN32
    HDC hdc;
    SDL_Surface *lpComposite;
    SDL_Event event = {0};
    BYTE *rgb;
    BYTE r, g, b, a;
    Uint32 pixel;
    const char *pszCapture;
#endif
    wc.lpfnWndProc = BarProc; wc.lpszClassName = "MenuBarContract";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Menu bars", WS_OVERLAPPEDWINDOW, 80, 80, 420, 220, NULL, NULL, NULL, NULL);
    CHECK(hwnd); ShowWindow(hwnd, SW_SHOW); UpdateWindow(hwnd); Drain();
    CHECK(GetClientRect(hwnd, &rcBefore) && ClientToScreen(hwnd, &ptBefore));
    CHECK(GetWindowRect(hwnd, &rcWindow));
    hmenu = CreateMenu(); hfile = CreatePopupMenu(); hedit = CreatePopupMenu();
    CHECK(hmenu && hfile && hedit);
    CHECK(AppendMenu(hfile, MF_STRING, 701, "&Open"));
    CHECK(AppendMenu(hedit, MF_STRING, 702, "&Copy"));
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hfile, "&File"));
    CHECK(AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hedit, "&Edit"));
    CHECK(SetMenu(hwnd, hmenu) && DrawMenuBar(hwnd));
    ShowWindow(hwnd, SW_SHOW); UpdateWindow(hwnd); Drain();
    CHECK(GetClientRect(hwnd, &rcAfter) && ClientToScreen(hwnd, &ptAfter));
    CHECK(rcBefore.right == rcAfter.right && rcAfter.bottom < rcBefore.bottom);
    CHECK(ptAfter.x == ptBefore.x && ptAfter.y > ptBefore.y);
    CHECK(ptAfter.y - ptBefore.y == rcBefore.bottom - rcAfter.bottom);
    CHECK(GetWindowRect(hwnd, &rcWindowAfter) && EqualRect(&rcWindow, &rcWindowAfter));
    CHECK(GetMenuItemRect(hwnd, hmenu, 0, &rcFile) && GetMenuItemRect(hwnd, hmenu, 1, &rcEdit));
    CHECK(rcFile.left < rcEdit.left && rcFile.top == rcEdit.top && rcFile.bottom <= ptAfter.y);
    hchild = CreateWindow("STATIC", "Child", WS_CHILD | WS_VISIBLE, 0, 0, 20, 20, hwnd, (HMENU)(UINT_PTR)3, NULL, NULL);
    CHECK(hchild && ClientToScreen(hchild, &ptChild) && ptChild.x == ptAfter.x && ptChild.y == ptAfter.y);
    CHECK(!GetMenuItemRect(hwnd, hmenu, 9, &rcEdit));
#ifndef _WIN32
    hdc = GetDC(hwnd); CHECK(hdc);
    SetPixel(hdc, 30, 10, RGB(220, 30, 10)); CHECK(ReleaseDC(hwnd, hdc));
    lpComposite = hwnd->lpComposite; CHECK(lpComposite && lpComposite->h == 220);
    rgb = (BYTE *)lpComposite->pixels + (ptAfter.y - ptBefore.y + 10) * lpComposite->pitch + 30 * 4;
    memcpy(&pixel, rgb, sizeof(pixel)); SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(lpComposite->format), NULL, &r, &g, &b, &a);
    CHECK(r == 220 && g == 30 && b == 10);
    rgb = (BYTE *)lpComposite->pixels + 200 * 4;
    memcpy(&pixel, rgb, sizeof(pixel)); SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(lpComposite->format), NULL, &r, &g, &b, &a);
    CHECK(r == 192 && g == 192 && b == 192);
    pszCapture = SDL_getenv("STARS_MENUBAR_CAPTURE");
    if (pszCapture) CHECK(SDL_SaveBMP(lpComposite, pszCapture));
#endif
    CHECK(OpenBar(hwnd, 1) == 0 && idCommand == 701);
    CHECK(OpenBar(hwnd, 2) == 0 && idCommand == 702);
    CHECK(OpenBar(hwnd, 3) == 0 && idCommand == 0);
#ifndef _WIN32
    CHECK(OpenBar(hwnd, 4) == 0 && idCommand == 702);
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0)
        CHECK(OpenBar(hwnd, 5) == 0 && fPopupContract && idCommand == 701);
    SetFocus(hchild);
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwnd->id;
    event.key.scancode = SDL_SCANCODE_F10; event.key.mod = 0;
    CompatRouteEvent(&event); Drain(); CHECK(hwnd->iMenuHot == 0);
    SendMessage(hchild, WM_KEYDOWN, VK_ESCAPE, 0); CHECK(hwnd->iMenuHot == -1);
    event.key.scancode = SDL_SCANCODE_F; event.key.mod = SDL_KMOD_LALT;
    iAction = 1; iStep = 0; idCommand = 0;
    CHECK(SetTimer(hwnd, 17, 20, InputTimer));
    CompatRouteEvent(&event); CompatTextInput(hchild, "f"); Drain();
    CHECK(idCommand == 701 && cEnter == cExit && hwnd->iMenuHot == -1);
    event.type = SDL_EVENT_KEY_UP; event.key.scancode = SDL_SCANCODE_LALT; event.key.mod = 0;
    CompatRouteEvent(&event); Drain(); CHECK(hwnd->iMenuHot == -1);
    SetFocus(hwnd);
    CHECK(GetMenuItemRect(hwnd, hmenu, 0, &rcFile));
    CHECK(GetWindowRect(hwnd, &rcWindow));
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwnd->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.x = (float)(rcFile.left - rcWindow.left + 2);
    event.button.y = (float)(rcFile.top - rcWindow.top + 2);
    iAction = 1; iStep = 0; idCommand = 0;
    CHECK(SetTimer(hwnd, 17, 20, InputTimer)); CompatRouteEvent(&event); Drain();
    CHECK(idCommand == 701 && GetCapture() == NULL);
    CHECK(SetWindowPos(hwnd, NULL, 0, 0, 45, 220, SWP_NOMOVE | SWP_NOZORDER) && DrawMenuBar(hwnd));
    CHECK(GetMenuItemRect(hwnd, hmenu, 0, &rcFile) && GetMenuItemRect(hwnd, hmenu, 1, &rcEdit));
    CHECK(rcEdit.top >= rcFile.bottom);
    CHECK(SetWindowPos(hwnd, NULL, 0, 0, 420, 220, SWP_NOMOVE | SWP_NOZORDER) && DrawMenuBar(hwnd));
#endif
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        MSG msg;
        fDemo = TRUE;
        while (IsWindow(hwnd) && GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
    if (fDemo) {
#ifndef _WIN32
        StarsCompatShutdown();
#endif
        return 0;
    }
    CHECK(SetMenu(hwnd, NULL) && DrawMenuBar(hwnd));
    CHECK(GetClientRect(hwnd, &rcAfter) && rcAfter.bottom == rcBefore.bottom);
    CHECK(DestroyMenu(hmenu));
    CHECK(DestroyWindow(hwnd));
    hwnd = CreateWindow(wc.lpszClassName, "Destroy", WS_OVERLAPPEDWINDOW, 80, 80, 300, 180, NULL, NULL, NULL, NULL);
    hmenu = CreateMenu(); hfile = CreatePopupMenu();
    CHECK(hwnd && AppendMenu(hfile, MF_STRING, 701, "&Open") && AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hfile, "&File") && SetMenu(hwnd, hmenu));
    fDestroyInit = TRUE; SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 'f');
    CHECK(!IsWindow(hwnd));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Menu bar contracts passed");
    return 0;
}
