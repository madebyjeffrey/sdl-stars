/* Popup commands/navigation and window menu ownership share SDK checks. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static HWND hwndOwner;
static UINT rgkey[16], cKey, iKey, idCommand;
static int cEnter, cExit, cInit, cCommand;
static BOOL fTracking, fCommandDuring, fDestroyOwner, fDestroyMenu;
static HMENU hmenuCurrent;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int iMouse;
static BOOL fPopupPixels, fNested, fNestedSafe, fEnd, fEndWorked, fQuit;
static UINT idNested;
#endif
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_ENTERMENULOOP) cEnter++;
    if (msg == WM_EXITMENULOOP) cExit++;
    if (msg == WM_INITMENUPOPUP) cInit++;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    if (msg == WM_INITMENUPOPUP && fNested) {
        HMENU hnested = CreatePopupMenu();
        fNested = FALSE; fNestedSafe = TRUE;
        AppendMenu(hnested, MF_STRING, 303, "Nested loop");
        if (TrackPopupMenu(hnested, TPM_RETURNCMD, 180, 180, 0, hwnd, NULL)) fNestedSafe = FALSE;
        if (TrackPopupMenu(hmenuCurrent, TPM_RETURNCMD | TPM_RECURSE, 180, 180, 0, hwnd, NULL)) fNestedSafe = FALSE;
        PostMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1); PostMessage(hwnd, WM_KEYDOWN, VK_RETURN, 1);
        idNested = (UINT)TrackPopupMenu(hnested, TPM_RETURNCMD | TPM_RECURSE | TPM_NONOTIFY, 180, 180, 0, hwnd, NULL);
        DestroyMenu(hnested);
    }
#endif
    if (msg == WM_MENUSELECT && fDestroyOwner) { fDestroyOwner = FALSE; DestroyWindow(hwnd); }
    if (msg == WM_INITMENUPOPUP && fDestroyMenu) { fDestroyMenu = FALSE; DestroyMenu((HMENU)w); }
    if (msg == WM_COMMAND) { cCommand++; idCommand = LOWORD(w); if (fTracking) fCommandDuring = TRUE; }
    return DefWindowProc(hwnd, msg, w, l);
}
static void CALLBACK InputTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD ms) {
    UINT i;
    (void)msg; (void)ms;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    if (fEnd || fQuit) {
        KillTimer(hwnd, id);
        if (fEnd) fEndWorked = EndMenu(); else PostQuitMessage(17);
        return;
    }
    if (iMouse) {
        HWND hp;
        char sz[64];
        SDL_Event event = {0};
        HDC hdc;
        const char *pszCapture = SDL_getenv("STARS_POPUP_CAPTURE");
        for (hp = GetWindow(hwndOwner, GW_HWNDFIRST); hp; hp = GetWindow(hp, GW_HWNDNEXT)) {
            GetClassNameA(hp, sz, sizeof(sz));
            if (strcmp(sz, "StarsCompatPopup") == 0) break;
        }
        KillTimer(hwnd, id);
        if (!hp) { PostMessage(hwndOwner, WM_KEYDOWN, VK_ESCAPE, 1); return; }
        if (pszCapture && iMouse == 1) SDL_SaveBMP(hp->lpCanvas.get(), pszCapture);
        hdc = GetDC(hp); fPopupPixels = GetPixel(hdc, 3, 3) == GetSysColor(COLOR_MENU) &&
            GetPixel(hdc, 8, 0) == GetSysColor(COLOR_BTNHIGHLIGHT) &&
            GetPixel(hdc, 8, hp->lpCanvas->h - 1) == GetSysColor(COLOR_3DDKSHADOW); ReleaseDC(hp, hdc);
        event.motion.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hp->id; event.motion.x = 8; event.motion.y = 8;
        SDL_PushEvent(&event);
        event.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hp->id;
        event.button.x = iMouse == 3 ? -10 : 8; event.button.y = 8;
        event.button.button = iMouse == 2 ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        SDL_PushEvent(&event); event.button.type = SDL_EVENT_MOUSE_BUTTON_UP; SDL_PushEvent(&event);
        return;
    }
#endif
    if (iKey >= cKey) { KillTimer(hwnd, id); return; }
    i = iKey++;
    if (iKey == cKey) KillTimer(hwnd, id);
    if (rgkey[i] >= 'A' && rgkey[i] <= 'Z') {
#if defined(_WIN32) && !defined(STARS_UI_SDL)
        INPUT rginput[2] = {{0}};
        rginput[0].type = rginput[1].type = INPUT_KEYBOARD;
        rginput[0].ki.wVk = rginput[1].ki.wVk = (WORD)rgkey[i];
        rginput[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, rginput, sizeof(INPUT));
#else
        PostMessage(hwndOwner, WM_CHAR, rgkey[i], 1);
#endif
    } else {
        PostMessage(hwndOwner, WM_KEYDOWN, rgkey[i], 1);
        PostMessage(hwndOwner, WM_KEYUP, rgkey[i], (LPARAM)0xc0000001);
    }
}
static void Drain(void) { MSG msg; while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg); }
static BOOL Track(UINT flags, const UINT *rg, UINT c) {
    BOOL result;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    SDL_WarpMouseInWindow(hwndOwner->lpWindow.get(), 2, 2);
#else
    SetCursorPos(42, 42);
#endif
    Drain();
    memcpy(rgkey, rg, c * sizeof(*rg)); cKey = c; iKey = 0;
    SetTimer(hwndOwner, 1, 20, InputTimer);
    fTracking = TRUE;
    result = TrackPopupMenu(hmenuCurrent, flags, 140, 140, 0, hwndOwner, NULL);
    fTracking = FALSE; KillTimer(hwndOwner, 1);
    return result;
}
int main(int argc, char **argv) {
    static const UINT rgChoose[] = {VK_HOME, VK_RETURN};
    static const UINT rgNested[] = {VK_HOME, VK_DOWN, VK_DOWN, VK_RIGHT, VK_HOME, VK_RETURN};
    static const UINT rgMnemonic[] = {VK_HOME, 'C', VK_RETURN};
    static const UINT rgLeafRight[] = {VK_HOME, VK_RIGHT, VK_RETURN};
    static const UINT rgCancel[] = {VK_ESCAPE};
    static const UINT rgBack[] = {VK_HOME, VK_DOWN, VK_DOWN, VK_RIGHT, VK_ESCAPE, VK_HOME, VK_RETURN};
    WNDCLASS wc = {0};
    HWND hwndChild;
    HMENU hbar, hother, hsub;
    BOOL result;
    wc.lpfnWndProc = OwnerProc; wc.lpszClassName = "PopupContractOwner";
    CHECK(RegisterClass(&wc));
    hbar = CreateMenu(); CHECK(AppendMenu(hbar, MF_STRING, 1, "One"));
    hwndOwner = CreateWindow(wc.lpszClassName, "Menu compatibility", WS_OVERLAPPEDWINDOW,
                             40, 40, 400, 300, NULL, hbar, NULL, NULL);
    CHECK(hwndOwner && GetMenu(hwndOwner) == hbar);
    hwndChild = CreateWindow(wc.lpszClassName, "", WS_CHILD, 0, 0, 10, 10, hwndOwner, (HMENU)(UINT_PTR)123, NULL, NULL);
    CHECK(hwndChild && GetDlgCtrlID(hwndChild) == 123);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!GetMenu(hwndChild));
#endif
    hother = CreateMenu(); CHECK(SetMenu(hwndOwner, hother));
    CHECK(GetMenu(hwndOwner) == hother && IsMenu(hbar));
    CHECK(SetMenu(hwndOwner, NULL) && IsMenu(hother) && !GetMenu(hwndOwner));
    CHECK(DestroyMenu(hother) && SetMenu(hwndOwner, hbar));
    ShowWindow(hwndOwner, SW_SHOW); SetFocus(hwndOwner); Drain();
    hmenuCurrent = CreatePopupMenu(); hsub = CreatePopupMenu();
    CHECK(AppendMenu(hmenuCurrent, MF_CHECKED, 101, "&Alpha\tCtrl+A"));
    CHECK(AppendMenu(hmenuCurrent, MF_SEPARATOR, 0, NULL));
    CHECK(AppendMenu(hmenuCurrent, MF_GRAYED | MF_DISABLED, 102, "Disabled"));
    CHECK(AppendMenu(hsub, MF_STRING, 201, "&Beta"));
    CHECK(AppendMenu(hmenuCurrent, MF_POPUP, (UINT_PTR)hsub, "&More"));
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        TrackPopupMenu(hmenuCurrent, 0, 160, 160, 0, hwndOwner, NULL); Drain();
        printf("Selected command: %u\n", idCommand);
    } else {
        result = Track(TPM_RETURNCMD, rgChoose, 2);
        CHECK(result == 101 && cCommand == 0 && cEnter == 1 && cExit == 1 && cInit == 1);
        CHECK(GetFocus() == hwndOwner && !GetCapture());
        CHECK(Track(TPM_RETURNCMD, rgNested, 6) == 201 && cCommand == 0);
        CHECK(Track(TPM_RETURNCMD, rgBack, 7) == 101);
        CHECK(Track(TPM_RETURNCMD, rgLeafRight, 3) == 101);
        CHECK(Track(TPM_RETURNCMD, rgCancel, 1) == 0);
        CHECK(Track(0, rgChoose, 2)); Drain();
        CHECK(cCommand == 1 && idCommand == 101 && !fCommandDuring);
        CHECK(Track(TPM_RETURNCMD | TPM_NONOTIFY, rgChoose, 2) == 101);
        CHECK(cCommand == 1);
        {
            HMENU hsaved = hmenuCurrent;
            hmenuCurrent = CreatePopupMenu();
            CHECK(AppendMenu(hmenuCurrent, MF_STRING, 501, "&Choice one"));
            CHECK(AppendMenu(hmenuCurrent, MF_STRING, 502, "&Choice two"));
            result = Track(TPM_RETURNCMD, rgMnemonic, 3);
#if defined(_WIN32) && !defined(STARS_UI_SDL)
            // Wine 9 executes its first text match; native Windows cycles duplicates.
            CHECK(result == 502 || (result == 501 && GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version")));
#else
            CHECK(result == 502);
#endif
            CHECK(DestroyMenu(hmenuCurrent)); hmenuCurrent = hsaved;
        }
#if !defined(_WIN32) || defined(STARS_UI_SDL)
        fEnd = TRUE; CHECK(Track(TPM_RETURNCMD, rgChoose, 0) == 0 && fEndWorked); fEnd = FALSE;
        fQuit = TRUE; CHECK(Track(TPM_RETURNCMD, rgChoose, 0) == 0); fQuit = FALSE;
        {
            MSG msg;
            CHECK(PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE) && msg.wParam == 17);
        }
        SetCapture(hwndChild);
        CHECK(Track(TPM_RETURNCMD, rgChoose, 2) == 101 && GetCapture() == hwndChild);
        ReleaseCapture();
        iMouse = 1; CHECK(Track(TPM_RETURNCMD, rgChoose, 0) == 101 && fPopupPixels);
        iMouse = 2; CHECK(Track(TPM_RETURNCMD | TPM_RIGHTBUTTON, rgChoose, 0) == 101);
        iMouse = 3; CHECK(Track(TPM_RETURNCMD, rgChoose, 0) == 0);
        iMouse = 0;
        fNested = TRUE; CHECK(Track(TPM_RETURNCMD, rgChoose, 2) == 101 && idNested == 303 && fNestedSafe);
        CHECK(!TrackPopupMenu(hmenuCurrent, 0, 0, 0, 1, hwndOwner, NULL));
        CHECK(!EndMenu());
        fDestroyMenu = TRUE;
        CHECK(!TrackPopupMenu(hmenuCurrent, TPM_RETURNCMD, 0, 0, 0, hwndOwner, NULL));
        CHECK(!IsMenu(hmenuCurrent));
        hmenuCurrent = CreatePopupMenu(); CHECK(AppendMenu(hmenuCurrent, MF_STRING, 1, "Destroy owner"));
        fDestroyOwner = TRUE;
        CHECK(Track(TPM_RETURNCMD, rgChoose, 2) == 0 && !IsWindow(hwndOwner));
        CHECK(!IsMenu(hbar));
#endif
    }
    if (IsWindow(hwndOwner)) CHECK(DestroyWindow(hwndOwner));
    CHECK(!IsMenu(hbar));
    if (IsMenu(hmenuCurrent)) CHECK(DestroyMenu(hmenuCurrent));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Popup menu contracts passed");
    return 0;
}
