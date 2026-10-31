/* WinMain has already released game state: final cleanup must not call it. */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include "../../res/resource.h"

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static BOOL fReleased;
static int cLate;
static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)w; (void)l;
    if (fReleased) cLate++;
    if (msg == WM_INITDIALOG) { win32::EndDialog(hwnd, 2); return TRUE; }
    return FALSE;
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (fReleased) cLate++;
    if (msg == WM_APP) {
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
    wc.hInstance = win32::GetModuleHandle(NULL);
    wc.lpszClassName = "StarsProcessExit";
    for (i = 0; i < 2; i++) {
        CHECK(win32::RegisterClass(&wc));
        hwnd = win32::CreateWindow(wc.lpszClassName, "Frame", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            0, 0, 200, 150, NULL, NULL, wc.hInstance, NULL);
        CHECK(hwnd);
        hwndChild = win32::CreateWindow(wc.lpszClassName, "Child", WS_CHILD | WS_VISIBLE,
                                 5, 5, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndOwned = win32::CreateWindow(wc.lpszClassName, "Owned", WS_POPUP | WS_VISIBLE,
                                 20, 20, 100, 80, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndChild && hwndOwned);
        hwndList = win32::CreateWindow("ListBox", "", WS_CHILD | LBS_HASSTRINGS,
                                0, 0, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndCombo = win32::CreateWindow("ComboBox", "", WS_CHILD | CBS_DROPDOWNLIST,
                                 0, 0, 80, 100, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndList && hwndCombo);
        CHECK(win32::SendMessageA(hwndList, LB_ADDSTRING, 0, (LPARAM)"List allocation") == 0);
        CHECK(win32::SendMessageA(hwndCombo, CB_ADDSTRING, 0, (LPARAM)"Combo allocation") == 0);
        CHECK(win32::SendMessageA(hwnd, WM_APP, 0, 0));
        win32::SetFocus(hwndChild);
        win32::SetCapture(hwndChild);
        hdc = win32::GetDC(hwndChild);
        CHECK(hdc);
        CHECK(StarsCompatBeginDialogSequence(hwnd));
        CHECK(DialogBoxParam(wc.hInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
        CHECK(!win32::IsWindowEnabled(hwnd));
        fReleased = TRUE;
        StarsCompatProcessExit();
        CHECK(cLate == 0);
        CHECK(!win32::IsWindow(hwnd) && !win32::IsWindow(hwndChild) && !win32::IsWindow(hwndOwned));
        CHECK(!win32::IsWindow(hwndList) && !win32::IsWindow(hwndCombo));
        CHECK(!win32::GetFocus() && !win32::GetCapture());
        CHECK(SDL_WasInit(SDL_INIT_VIDEO) == 0);
        CHECK(!win32::ReleaseDC(hwndChild, hdc));
        fReleased = FALSE;
    }
    SDL_Quit();
    return 0;
}
