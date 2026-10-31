/* Ordinary windows activate; real detail cards retain their owner's focus. */
#include "win.h"
#include "../../src/core/player_choice_io.h"

#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
#ifdef __APPLE__
int FTestCocoaPopupBackground(void *lpWindow);
#endif

static void  CloseVisualMenu(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    (void)hwnd; (void)msg; (void)time;
    win32::EndMenu();
    win32::KillTimer(NULL, id);
}

static BOOL FNativeFocus(HWND hwnd) {
    win32::MSG msg;
    DWORD timeEnd = win32::GetTickCount() + 1000;

    if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy")) return 1;
    do {
        while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
        if (SDL_GetKeyboardFocus() == hwnd->lpWindow.get()) return 1;
        SDL_Delay(10);
    } while (win32::GetTickCount() < timeEnd);
    return 0;
}

// The saved fixture is password-protected; this check exercises window focus.
int16_t StarsTest_PromptPassword(void) { return 1; }

static BOOL fDesignerSeen, fDesignerFocus;
static int cDesignerWait;

static int TestBrowserAvailableTechnology(void) {
    HWND hwnd;
    int iCategory, c;
    HS hs;

    hwnd = ui::CreateDialog(hInst, win32::MAKEINTRESOURCE(IDD_BROWSER), hwndFrame, lpfnBrowserDlgProc);
    CHECK(hwnd && hwnd == hwndBrowser);
    for (iCategory = 0; iCategory <= 1; iCategory++) {
        win32::CheckDlgButton(hwnd, IDC_BROWSER_AVAILABLE_ONLY, win32::BST_UNCHECKED);
        ui::SendDlgItemMessage(hwnd, IDC_BROWSER_COMPONENT_CATEGORY, win32::CB_SETCURSEL, iCategory, 0);
        ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_BROWSER_COMPONENT_CATEGORY, win32::CBN_SELCHANGE), 0);
        for (c = 0; c < iarmorCount && vpartBrowser.hs.iItem != iarmorSuperlatanium; c++)
            ui::SendMessage(hwnd, win32::WM_COMMAND, IDC_NEXT, 0);
        CHECK(vpartBrowser.hs.grhst == hstArmor && vpartBrowser.hs.iItem == iarmorSuperlatanium);
        CHECK(FLookupPart(&vpartBrowser) != mdPartAvailAvailable);
        ui::SendMessage(win32::GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), win32::BM_CLICK, 0, 0);
        CHECK(win32::IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY));
        CHECK(vpartBrowser.pcom() && FLookupPart(&vpartBrowser) == mdPartAvailAvailable);
        CHECK(ui::SendDlgItemMessage(hwnd, IDC_BROWSER_COMPONENT_CATEGORY, win32::CB_GETCURSEL, 0, 0) == iCategory);
        ui::SendMessage(hwnd, win32::WM_COMMAND, IDC_NEXT, 0);
        CHECK(vpartBrowser.pcom() && FLookupPart(&vpartBrowser) == mdPartAvailAvailable);
        hs = vpartBrowser.hs;
        ui::SendMessage(win32::GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), win32::BM_CLICK, 0, 0);
        CHECK(vpartBrowser.hs.grhst == hs.grhst && vpartBrowser.hs.iItem == hs.iItem);
        ui::SendMessage(win32::GetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), win32::BM_CLICK, 0, 0);
        CHECK(vpartBrowser.hs.grhst == hs.grhst && vpartBrowser.hs.iItem == hs.iItem);
    }
    CHECK(ui::DestroyWindow(hwnd) && !hwndBrowser);
    return 0;
}

static void  CloseDesigner(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    (void)hwnd; (void)msg; (void)time;
    if (!win32::IsWindowVisible(hwndSlotDlg)) return;
    fDesignerSeen = 1;
    fDesignerFocus = !strcmp(SDL_GetCurrentVideoDriver(), "dummy") ||
        SDL_GetKeyboardFocus() == hwndSlotDlg->lpWindow.get();
    if (!fDesignerFocus && ++cDesignerWait < 40) return;
    win32::KillTimer(NULL, id);
    win32::PostMessageA(hwndSlotDlg, win32::WM_COMMAND, IDCANCEL, 0);
}

static int TestDesignerReturnFocus(HWND hwndOwnerFocus) {
    UINT_PTR idTimer;
    POINT16 ptSize = {736, 500};
    char szExtension[] = "m1";

    strcpy(szBase, STARS_TEST_FOCUS_GAME);
    idPlayer = 0;
    CHECK(FLoadGame(szBase, szExtension));
    ui::SetFocus(hwndOwnerFocus);
    idTimer = win32::SetTimer(NULL, 0, 25, CloseDesigner);
    CHECK(idTimer);
    ShipBuilder(ptSize);
    win32::KillTimer(NULL, idTimer);
    CHECK(fDesignerSeen && fDesignerFocus && !win32::IsWindow(hwndSlotDlg));
    CHECK(win32::IsWindowEnabled(hwndFrame) && FNativeFocus(hwndFrame));
    CHECK(win32::GetFocus() == hwndOwnerFocus);
    CHECK(TestBrowserAvailableTechnology() == 0);
    DestroyCurGame();
    return 0;
}

static HWND hwndRedirectFocus;

static LRESULT  ReturnFocusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == win32::WM_DESTROY && hwndRedirectFocus && hwndRedirectFocus != hwnd)
        ui::ShowWindow(hwndRedirectFocus, win32::SW_SHOW);
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

static int TestWindowFocus(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndChild, hwndCard, hwndOwnerFocus, hwndOther;

    hwndOwnerFocus = ui::CreateWindowEx(0, "BUTTON", "Owner focus", win32::WS_CHILD | win32::WS_VISIBLE,
                                  10, 10, 50, 20, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndOwnerFocus);
    ui::SetFocus(hwndOwnerFocus);
    CHECK(TestDesignerReturnFocus(hwndOwnerFocus) == 0);
    idPlayer = 0;
    hwnd = ui::CreateDialog(hInst, win32::MAKEINTRESOURCE(IDD_BROWSER), hwndFrame, lpfnBrowserDlgProc);
    CHECK(hwnd && hwnd == hwndBrowser && FNativeFocus(hwnd));
    CHECK(HwndCompatRoot(win32::GetFocus()) == hwnd);
    ui::ShowWindow(hwnd, win32::SW_HIDE);
    ui::ShowWindow(hwndFrame, win32::SW_SHOW);
    CHECK(FNativeFocus(hwndFrame));
    ui::SetFocus(hwndOwnerFocus);
    ui::ShowWindow(hwnd, win32::SW_SHOW);
    CHECK(FNativeFocus(hwnd) && HwndCompatRoot(win32::GetFocus()) == hwnd);
    CHECK(ui::DestroyWindow(hwnd) && !hwndBrowser);
    CHECK(FNativeFocus(hwndFrame) && win32::GetFocus() == hwndOwnerFocus);
    CHECK(ui::DestroyWindow(hwndOwnerFocus));
    ui::SetFocus(hwndFrame);
    wc.lpszClassName = "FocusContract";
    wc.lpfnWndProc = ReturnFocusWndProc;
    wc.hInstance = hInst;
    CHECK(win32::RegisterClassA(&wc));
    hwnd = ui::CreateWindowEx(0, wc.lpszClassName, "Focus contract", win32::WS_OVERLAPPEDWINDOW,
                        100, 100, 320, 240, hwndFrame, NULL, hInst, NULL);
    CHECK(hwnd);
    ui::ShowWindow(hwnd, win32::SW_SHOW);
    CHECK(FNativeFocus(hwnd) && win32::GetFocus() == hwnd);
    hwndChild = ui::CreateWindowEx(0, "BUTTON", "Child", win32::WS_CHILD | win32::WS_VISIBLE,
                             10, 10, 50, 20, hwnd, NULL, hInst, NULL);
    CHECK(hwndChild && win32::GetFocus() == hwnd);
    ui::SetFocus(hwndChild);
    ui::ShowWindow(hwnd, win32::SW_SHOW);
    CHECK(win32::GetFocus() == hwndChild && FNativeFocus(hwnd));
    ui::ShowWindow(hwndFrame, win32::SW_SHOW);
    CHECK(FNativeFocus(hwndFrame));
    CHECK(ui::SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                       win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW));
    CHECK(win32::GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    ui::EnableWindow(hwnd, 0);
    ui::ShowWindow(hwnd, win32::SW_SHOW);
    CHECK(win32::GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    ui::EnableWindow(hwnd, 1);
    ui::ShowWindow(hwnd, win32::SW_SHOWMINIMIZED);
    CHECK(win32::GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    hwndCard = ui::CreateWindowEx(win32::WS_EX_NOACTIVATE, wc.lpszClassName, "Detail card", win32::WS_POPUP,
                             150, 150, 100, 100, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndCard);
    ui::ShowWindow(hwndCard, win32::SW_SHOW);
    CHECK(ui::SetWindowPos(hwndCard, NULL, 0, 0, 0, 0, win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOZORDER));
    CHECK(win32::GetFocus() == hwndFrame && FNativeFocus(hwndFrame));
    CHECK(ui::DestroyWindow(hwndCard));
    ui::ShowWindow(hwnd, win32::SW_RESTORE);
    CHECK(FNativeFocus(hwnd));
    hwndOther = ui::CreateWindowEx(0, wc.lpszClassName, "Teardown focus choice", win32::WS_OVERLAPPEDWINDOW,
                             100, 100, 320, 240, hwndFrame, NULL, hInst, NULL);
    CHECK(hwndOther);
    hwndRedirectFocus = hwndOther;
    CHECK(ui::DestroyWindow(hwnd));
    CHECK(FNativeFocus(hwndOther) && win32::GetFocus() == hwndOther);
    hwndRedirectFocus = NULL;
    CHECK(ui::DestroyWindow(hwndOther) && FNativeFocus(hwndFrame));
    return 0;
}

int main(void) {
    HWND hwnd, hwndFocus;
    win32::MSG msg;
    SDL_WindowFlags flags;
    SDL_Event event = {};
    SHDEF shdef = {};
    BYTE r, g, b, a;
    SDL_Surface *lpFirst;
    int i;

    PLAYERCHOICEIO io_playerchoiceio;
    io_playerchoiceio.pfnPromptPassword = StarsTest_PromptPassword;
    PLAYERCHOICEIOSCOPE scope_playerchoiceio(io_playerchoiceio);

    hInst = win32::GetModuleHandleA(NULL);
    CHECK(InitMDIApp() && FCreateStuff() && FGetSystemColors() && InitInstance(win32::SW_SHOWNORMAL));
    ui::ShowWindow(hwndFrame, win32::SW_SHOW);
    ui::SetFocus(hwndFrame);
    while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
    CHECK(TestWindowFocus() == 0);
    hwndFocus = win32::GetFocus();
    // The recorded scanner-combobox tip uses SetWindowPos to reveal a hidden
    // starstt, rather than Popup's direct ShowWindow path.
    vidsTooltip = idsScannerEffective;
    hwnd = ui::CreateWindowEx(win32::WS_EX_NOACTIVATE, szTooltip, NULL, win32::WS_POPUP,
                          200, 180, 180, dyArial8 + 6, hwndFrame, NULL, hInst, NULL);
    CHECK(hwnd && hwndTooltip == hwnd && !win32::IsWindowVisible(hwnd));
#ifdef __APPLE__
    if (!strcmp(SDL_GetCurrentVideoDriver(), "cocoa")) {
        CHECK(SDL_GetWindowFlags(hwnd->lpWindow.get()) & SDL_WINDOW_TRANSPARENT);
        CHECK(FTestCocoaPopupBackground(SDL_GetPointerProperty(SDL_GetWindowProperties(hwnd->lpWindow.get()),
                  SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL)));
    }
#endif
    CHECK(ui::SetWindowPos(hwnd, win32::HWND_TOPMOST, 200, 180, 0, 0,
                       win32::SWP_NOSIZE | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW | win32::SWP_NOOWNERZORDER));
    CHECK(ui::UpdateWindow(hwnd) && hwnd->lpComposite.get());
    CHECK(SDL_ReadSurfacePixel(hwnd->lpComposite.get(), 5 * hwnd->iCanvasScale, (dyArial8 + 3) * hwnd->iCanvasScale, &r, &g, &b, &a));
    CHECK(win32::RGB(r, g, b) == win32::RGB(255, 255, 159) && a == 255);
    CHECK(win32::GetFocus() == hwndFocus && ui::DestroyWindow(hwnd) && !hwndTooltip);
    for (i = 0; i < 3; i++) {
        GlobalPD.grPopup = grPopupMineral;
        GlobalPD.SetRgi(0, 0); GlobalPD.SetRgi(1, 0);
        GlobalPD.SetRgi(2, 2000); GlobalPD.SetRgi(3, 68); GlobalPD.SetRgi(4, 11);
        Popup(hwndFrame, 200, 180);
        hwnd = LpCompatWindow(hwndPopup);
        CHECK(hwnd && hwnd->hwndOwner == hwndFrame && win32::IsWindowVisible(hwnd));
        CHECK(hwnd->dwExStyle & win32::WS_EX_NOACTIVATE);
        flags = SDL_GetWindowFlags(hwnd->lpWindow.get());
        CHECK(flags & SDL_WINDOW_NOT_FOCUSABLE);
        CHECK(flags & SDL_WINDOW_BORDERLESS);
        if (strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
            CHECK(flags & SDL_WINDOW_TOOLTIP);
            CHECK(SDL_GetWindowParent(hwnd->lpWindow.get()) == hwndFrame->lpWindow.get());
        }
        while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
        CHECK(win32::GetFocus() == hwndFocus && win32::GetCapture() == hwnd);
        if (i == 0) {
            SDL_zero(event);
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = SDL_GetWindowID(hwnd->lpWindow.get());
            CHECK(SDL_PushEvent(&event));
            while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
        } else if (i == 1) {
            SDL_zero(event);
            event.type = SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.windowID = SDL_GetWindowID(hwndFrame->lpWindow.get());
            event.button.button = SDL_BUTTON_RIGHT;
            event.button.x = 200; event.button.y = 180;
            CHECK(SDL_PushEvent(&event));
            while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
        } else ui::DestroyWindow(hwnd);
        CHECK(!win32::IsWindow(hwnd) && !hwndPopup && GlobalPD.grPopup == 0 && !win32::GetCapture());
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
    CHECK(win32::RGB(r, g, b) == crButtonFace);
    CHECK(!hwnd->fUpdate && !hwnd->fPresentPending);
    lpFirst = SDL_DuplicateSurface(hwnd->lpCanvas.get());
    CHECK(lpFirst);
    CHECK(win32::InvalidateRect(hwnd, NULL, 1) && ui::UpdateWindow(hwnd));
    CHECK(lpFirst->pitch == hwnd->lpCanvas->pitch &&
          !memcmp(lpFirst->pixels, hwnd->lpCanvas->pixels, (size_t)lpFirst->pitch * lpFirst->h));
    SDL_DestroySurface(lpFirst);
    if (SDL_getenv("STARS_TEST_POPUP_HOLD")) {
        DWORD timeEnd = win32::GetTickCount() + 60000;
        while (win32::GetTickCount() < timeEnd && win32::IsWindow(hwnd)) {
            while (ui::PeekMessage(&msg, NULL, 0, 0, win32::PM_REMOVE)) ui::DispatchMessage(&msg);
            SDL_Delay(10);
        }
    }
    CHECK(ui::DestroyWindow(hwnd));
    if (SDL_getenv("STARS_TEST_MENU_HOLD")) {
        CHECK(win32::SetTimer(NULL, 0, 60000, CloseVisualMenu));
        ui::TrackPopupMenu(win32::GetSubMenu(win32::GetMenu(hwndFrame), 0), win32::TPM_RETURNCMD, 200, 200, 0, hwndFrame, NULL);
    }
    FreeStuff();
    StarsCompatProcessExit();
    SDL_Quit();
    puts("Designer/browser return focus, hidden tooltip backing and mineral/starbase popup contracts passed");
    return 0;
}
