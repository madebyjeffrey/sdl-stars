/* Ordinary windows activate; real detail cards retain their owner's focus. */
#include "win.h"
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
#ifdef __APPLE__
int FTestCocoaPopupBackground(void *lpWindow);
#endif

static void CALLBACK CloseVisualMenu(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    (void)hwnd; (void)msg; (void)time;
    EndMenu();
    KillTimer(NULL, id);
}

static BOOL FNativeFocus(HWND hwnd) {
    MSG msg;
    DWORD timeEnd = GetTickCount() + 1000;

    if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy")) return TRUE;
    do {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        if (SDL_GetKeyboardFocus() == hwnd->lpWindow.get()) return TRUE;
        SDL_Delay(10);
    } while (GetTickCount() < timeEnd);
    return FALSE;
}

// The saved fixture is password-protected; this check exercises window focus.
int16_t StarsTest_PromptPassword(void) { return TRUE; }

static BOOL fDesignerSeen, fDesignerFocus;
static int cDesignerWait;

static int TestBrowserAvailableTechnology(void) {
    HWND hwnd;
    int iCategory, c;
    HS hs;

    hwnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BROWSER), hwndFrame, lpfnBrowserDlgProc);
    CHECK(hwnd && hwnd == hwndBrowser);
    for (iCategory = 0; iCategory <= 1; iCategory++) {
        CheckDlgButton(hwnd, IDC_BROWSER_AVAILABLE_ONLY, BST_UNCHECKED);
        SendDlgItemMessage(hwnd, IDC_BROWSER_COMPONENT_CATEGORY, CB_SETCURSEL, iCategory, 0);
        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_BROWSER_COMPONENT_CATEGORY, CBN_SELCHANGE), 0);
        for (c = 0; c < iarmorCount && vpartBrowser.hs.iItem != iarmorSuperlatanium; c++)
            SendMessage(hwnd, WM_COMMAND, IDC_NEXT, 0);
        CHECK(vpartBrowser.hs.grhst == hstArmor && vpartBrowser.hs.iItem == iarmorSuperlatanium);
        CHECK(FLookupPart(&vpartBrowser) != mdPartAvailAvailable);
        SendMessage(GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), BM_CLICK, 0, 0);
        CHECK(IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY));
        CHECK(vpartBrowser.pcom && FLookupPart(&vpartBrowser) == mdPartAvailAvailable);
        CHECK(SendDlgItemMessage(hwnd, IDC_BROWSER_COMPONENT_CATEGORY, CB_GETCURSEL, 0, 0) == iCategory);
        SendMessage(hwnd, WM_COMMAND, IDC_NEXT, 0);
        CHECK(vpartBrowser.pcom && FLookupPart(&vpartBrowser) == mdPartAvailAvailable);
        hs = vpartBrowser.hs;
        SendMessage(GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), BM_CLICK, 0, 0);
        CHECK(vpartBrowser.hs.grhst == hs.grhst && vpartBrowser.hs.iItem == hs.iItem);
        SendMessage(GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), BM_CLICK, 0, 0);
        CHECK(vpartBrowser.hs.grhst == hs.grhst && vpartBrowser.hs.iItem == hs.iItem);
    }
    CHECK(DestroyWindow(hwnd) && !hwndBrowser);
    return 0;
}

static void CALLBACK CloseDesigner(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    (void)hwnd; (void)msg; (void)time;
    if (!IsWindowVisible(hwndSlotDlg)) return;
    fDesignerSeen = TRUE;
    fDesignerFocus = !strcmp(SDL_GetCurrentVideoDriver(), "dummy") ||
        SDL_GetKeyboardFocus() == hwndSlotDlg->lpWindow.get();
    if (!fDesignerFocus && ++cDesignerWait < 40) return;
    KillTimer(NULL, id);
    PostMessage(hwndSlotDlg, WM_COMMAND, IDCANCEL, 0);
}

static int TestDesignerReturnFocus(HWND hwndOwnerFocus) {
    UINT_PTR idTimer;
    POINT16 ptSize = {736, 500};
    char szExtension[] = "m1";

    strcpy(szBase, STARS_TEST_FOCUS_GAME);
    idPlayer = 0;
    CHECK(FLoadGame(szBase, szExtension));
    SetFocus(hwndOwnerFocus);
    idTimer = SetTimer(NULL, 0, 25, CloseDesigner);
    CHECK(idTimer);
    ShipBuilder(ptSize);
    KillTimer(NULL, idTimer);
    CHECK(fDesignerSeen && fDesignerFocus && !IsWindow(hwndSlotDlg));
    CHECK(IsWindowEnabled(hwndFrame) && FNativeFocus(hwndFrame));
    CHECK(GetFocus() == hwndOwnerFocus);
    CHECK(TestBrowserAvailableTechnology() == 0);
    DestroyCurGame();
    return 0;
}

static HWND hwndRedirectFocus;

static LRESULT CALLBACK ReturnFocusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY && hwndRedirectFocus && hwndRedirectFocus != hwnd)
        ShowWindow(hwndRedirectFocus, SW_SHOW);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int TestWindowFocus(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndChild, hwndCard, hwndOwnerFocus, hwndOther;

    hwndOwnerFocus = CreateWindow("BUTTON", "Owner focus", WS_CHILD | WS_VISIBLE,
                                  10, 10, 50, 20, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndOwnerFocus);
    SetFocus(hwndOwnerFocus);
    CHECK(TestDesignerReturnFocus(hwndOwnerFocus) == 0);
    idPlayer = 0;
    hwnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BROWSER), hwndFrame, lpfnBrowserDlgProc);
    CHECK(hwnd && hwnd == hwndBrowser && FNativeFocus(hwnd));
    CHECK(HwndCompatRoot(GetFocus()) == hwnd);
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwndFrame, SW_SHOW);
    CHECK(FNativeFocus(hwndFrame));
    SetFocus(hwndOwnerFocus);
    ShowWindow(hwnd, SW_SHOW);
    CHECK(FNativeFocus(hwnd) && HwndCompatRoot(GetFocus()) == hwnd);
    CHECK(DestroyWindow(hwnd) && !hwndBrowser);
    CHECK(FNativeFocus(hwndFrame) && GetFocus() == hwndOwnerFocus);
    CHECK(DestroyWindow(hwndOwnerFocus));
    SetFocus(hwndFrame);
    wc.lpszClassName = "FocusContract";
    wc.lpfnWndProc = ReturnFocusWndProc;
    wc.hInstance = hInst;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Focus contract", WS_OVERLAPPEDWINDOW,
                        100, 100, 320, 240, hwndFrame, NULL, hInst, NULL);
    CHECK(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    CHECK(FNativeFocus(hwnd) && GetFocus() == hwnd);
    hwndChild = CreateWindow("BUTTON", "Child", WS_CHILD | WS_VISIBLE,
                             10, 10, 50, 20, hwnd, NULL, hInst, NULL);
    CHECK(hwndChild && GetFocus() == hwnd);
    SetFocus(hwndChild);
    ShowWindow(hwnd, SW_SHOW);
    CHECK(GetFocus() == hwndChild && FNativeFocus(hwnd));
    ShowWindow(hwndFrame, SW_SHOW);
    CHECK(FNativeFocus(hwndFrame));
    CHECK(SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW));
    CHECK(GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    EnableWindow(hwnd, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    CHECK(GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    EnableWindow(hwnd, TRUE);
    ShowWindow(hwnd, SW_SHOWMINIMIZED);
    CHECK(GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    hwndCard = CreateWindowExA(WS_EX_NOACTIVATE, wc.lpszClassName, "Detail card", WS_POPUP,
                             150, 150, 100, 100, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndCard);
    ShowWindow(hwndCard, SW_SHOW);
    CHECK(SetWindowPos(hwndCard, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER));
    CHECK(GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    CHECK(DestroyWindow(hwndCard));
    ShowWindow(hwnd, SW_RESTORE);
    CHECK(FNativeFocus(hwnd));
    hwndOther = CreateWindow(wc.lpszClassName, "Teardown focus choice", WS_OVERLAPPEDWINDOW,
                             100, 100, 320, 240, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndOther);
    hwndRedirectFocus = hwndOther;
    CHECK(DestroyWindow(hwnd));
    CHECK(FNativeFocus(hwndOther) && GetFocus() == hwndOther);
    hwndRedirectFocus = NULL;
    CHECK(DestroyWindow(hwndOther) && FNativeFocus(hwndFrame));
    return 0;
}

int main(void) {
    HWND hwnd, hwndFocus;
    MSG msg;
    SDL_WindowFlags flags;
    SDL_Event event = {};
    SHDEF shdef = {};
    BYTE r, g, b, a;
    SDL_Surface *lpFirst;
    int i;

    hInst = GetModuleHandle(NULL);
    CHECK(InitMDIApp() && FCreateStuff() && FGetSystemColors() && InitInstance(SW_SHOWNORMAL));
    ShowWindow(hwndFrame, SW_SHOW);
    SetFocus(hwndFrame);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
    CHECK(TestWindowFocus() == 0);
    hwndFocus = GetFocus();
    // The recorded scanner-combobox tip uses SetWindowPos to reveal a hidden
    // starstt, rather than Popup's direct ShowWindow path.
    vidsTooltip = idsScannerEffective;
    hwnd = CreateWindowExA(WS_EX_NOACTIVATE, szTooltip, NULL, WS_POPUP,
                          200, 180, 180, dyArial8 + 6, hwndFrame, NULL, hInst, NULL);
    CHECK(hwnd && hwndTooltip == hwnd && !IsWindowVisible(hwnd));
#ifdef __APPLE__
    if (!strcmp(SDL_GetCurrentVideoDriver(), "cocoa")) {
        CHECK(SDL_GetWindowFlags(hwnd->lpWindow.get()) & SDL_WINDOW_TRANSPARENT);
        CHECK(FTestCocoaPopupBackground(SDL_GetPointerProperty(SDL_GetWindowProperties(hwnd->lpWindow.get()),
                  SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL)));
    }
#endif
    CHECK(SetWindowPos(hwnd, HWND_TOPMOST, 200, 180, 0, 0,
                       SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER));
    CHECK(UpdateWindow(hwnd) && hwnd->lpComposite.get());
    CHECK(SDL_ReadSurfacePixel(hwnd->lpComposite.get(), 5 * hwnd->iCanvasScale, (dyArial8 + 3) * hwnd->iCanvasScale, &r, &g, &b, &a));
    CHECK(RGB(r, g, b) == RGB(255, 255, 159) && a == 255);
    CHECK(GetFocus() == hwndFocus && DestroyWindow(hwnd) && !hwndTooltip);
    for (i = 0; i < 3; i++) {
        GlobalPD.grPopup = grPopupMineral;
        GlobalPD.rgi[0] = 0; GlobalPD.rgi[1] = 0;
        GlobalPD.rgi[2] = 2000; GlobalPD.rgi[3] = 68; GlobalPD.rgi[4] = 11;
        Popup(hwndFrame, 200, 180);
        hwnd = LpCompatWindow(hwndPopup);
        CHECK(hwnd && hwnd->hwndOwner == hwndFrame && IsWindowVisible(hwnd));
        CHECK(hwnd->dwExStyle & WS_EX_NOACTIVATE);
        flags = SDL_GetWindowFlags(hwnd->lpWindow.get());
        CHECK(flags & SDL_WINDOW_NOT_FOCUSABLE);
        CHECK(flags & SDL_WINDOW_BORDERLESS);
        if (strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
            CHECK(flags & SDL_WINDOW_TOOLTIP);
            CHECK(SDL_GetWindowParent(hwnd->lpWindow.get()) == hwndFrame->lpWindow.get());
        }
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        CHECK(GetFocus() == hwndFocus && GetCapture() == hwnd);
        if (i == 0) {
            SDL_zero(event);
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
            CHECK(SDL_PushEvent(&event));
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        } else if (i == 1) {
            SDL_zero(event);
            event.type = SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.windowID = SDL_GetWindowID(hwndFrame->lpWindow.get());
            event.button.button = SDL_BUTTON_RIGHT;
            event.button.x = 200; event.button.y = 180;
            CHECK(SDL_PushEvent(&event));
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        } else DestroyWindow(hwnd);
        CHECK(!IsWindow(hwnd) && !hwndPopup && GlobalPD.grPopup == 0 && !GetCapture());
    }
    idPlayer = 0;
    shdef.hul = rghuldefSB[0].hul;
    for (i = 0; i < shdef.hul.chs; i++) shdef.hul.rghs[i].cItem = 0;
    memset(&GlobalPD, 0, sizeof(GlobalPD));
    GlobalPD.grPopup = grPopupShdef;
    GlobalPD.lpshdef = &shdef;
    Popup(hwndFrame, 200, 180);
    hwnd = LpCompatWindow(hwndPopup);
    CHECK(hwnd && hwnd->lpComposite.get());
    CHECK(SDL_ReadSurfacePixel(hwnd->lpComposite.get(), 330 * hwnd->iCanvasScale, 300 * hwnd->iCanvasScale, &r, &g, &b, &a));
    CHECK(RGB(r, g, b) == crButtonFace);
    CHECK(!hwnd->fUpdate && !hwnd->fPresentPending);
    lpFirst = SDL_DuplicateSurface(hwnd->lpCanvas.get());
    CHECK(lpFirst);
    CHECK(InvalidateRect(hwnd, NULL, TRUE) && UpdateWindow(hwnd));
    CHECK(lpFirst->pitch == hwnd->lpCanvas->pitch &&
          !memcmp(lpFirst->pixels, hwnd->lpCanvas->pixels, (size_t)lpFirst->pitch * lpFirst->h));
    SDL_DestroySurface(lpFirst);
    if (SDL_getenv("STARS_TEST_POPUP_HOLD")) {
        DWORD timeEnd = GetTickCount() + 60000;
        while (GetTickCount() < timeEnd && IsWindow(hwnd)) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
            SDL_Delay(10);
        }
    }
    CHECK(DestroyWindow(hwnd));
    if (SDL_getenv("STARS_TEST_MENU_HOLD")) {
        CHECK(SetTimer(NULL, 0, 60000, CloseVisualMenu));
        TrackPopupMenu(GetSubMenu(GetMenu(hwndFrame), 0), TPM_RETURNCMD, 200, 200, 0, hwndFrame, NULL);
    }
    FreeStuff();
    StarsCompatProcessExit();
    SDL_Quit();
    puts("Designer/browser return focus, hidden tooltip backing and mineral/starbase popup contracts passed");
    return 0;
}
