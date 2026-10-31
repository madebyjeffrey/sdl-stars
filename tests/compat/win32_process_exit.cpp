/* WinMain has already released game state: final cleanup must not call it. */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include "../../res/resource.h"

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static BOOL fReleased;
static int cLate;
static INT_PTR  DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)w; (void)l;
    if (fReleased) cLate++;
    if (msg == win32::WM_INITDIALOG) { win32::EndDialog(hwnd, 2); return 1; }
    return 0;
}
static LRESULT  WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (fReleased) cLate++;
    if (msg == win32::WM_APP) {
        StarsCompatProcessExit();
        return win32::IsWindow(hwnd);
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
int main(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndChild, hwndOwned, hwndList, hwndCombo;
    HDC hdc;
    int i;

    wc.lpfnWndProc = WndProc;
    wc.hInstance = win32::GetModuleHandleA(NULL);
    wc.lpszClassName = "StarsProcessExit";
    for (i = 0; i < 2; i++) {
        CHECK(win32::RegisterClassA(&wc));
        hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Frame", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                            0, 0, 200, 150, NULL, NULL, wc.hInstance, NULL);
        CHECK(hwnd);
        hwndChild = win32::CreateWindowExA(0, wc.lpszClassName, "Child", win32::WS_CHILD | win32::WS_VISIBLE,
                                 5, 5, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndOwned = win32::CreateWindowExA(0, wc.lpszClassName, "Owned", win32::WS_POPUP | win32::WS_VISIBLE,
                                 20, 20, 100, 80, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndChild && hwndOwned);
        hwndList = win32::CreateWindowExA(0, "ListBox", "", win32::WS_CHILD | win32::LBS_HASSTRINGS,
                                0, 0, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndCombo = win32::CreateWindowExA(0, "ComboBox", "", win32::WS_CHILD | win32::CBS_DROPDOWNLIST,
                                 0, 0, 80, 100, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndList && hwndCombo);
        CHECK(win32::SendMessageA(hwndList, win32::LB_ADDSTRING, 0, (LPARAM)"List allocation") == 0);
        CHECK(win32::SendMessageA(hwndCombo, win32::CB_ADDSTRING, 0, (LPARAM)"Combo allocation") == 0);
        CHECK(win32::SendMessageA(hwnd, win32::WM_APP, 0, 0));
        win32::SetFocus(hwndChild);
        win32::SetCapture(hwndChild);
        hdc = win32::GetDC(hwndChild);
        CHECK(hdc);
        CHECK(StarsCompatBeginDialogSequence(hwnd));
        CHECK(DialogBoxParamA(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
        CHECK(!win32::IsWindowEnabled(hwnd));
        fReleased = 1;
        StarsCompatProcessExit();
        CHECK(cLate == 0);
        CHECK(!win32::IsWindow(hwnd) && !win32::IsWindow(hwndChild) && !win32::IsWindow(hwndOwned));
        CHECK(!win32::IsWindow(hwndList) && !win32::IsWindow(hwndCombo));
        CHECK(!win32::GetFocus() && !win32::GetCapture());
        CHECK(SDL_WasInit(SDL_INIT_VIDEO) == 0);
        CHECK(!win32::ReleaseDC(hwndChild, hdc));
        fReleased = 0;
    }
    SDL_Quit();
    return 0;
}
