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
    if (msg == WM_INITDIALOG) { EndDialog(hwnd, 2); return TRUE; }
    return FALSE;
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (fReleased) cLate++;
    if (msg == WM_APP) {
        StarsCompatProcessExit();
        return IsWindow(hwnd);
    }
    return DefWindowProc(hwnd, msg, w, l);
}
int main(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndChild, hwndOwned, hwndList, hwndCombo;
    HDC hdc;
    int i;

    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "StarsProcessExit";
    for (i = 0; i < 2; i++) {
        CHECK(RegisterClass(&wc));
        hwnd = CreateWindow(wc.lpszClassName, "Frame", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            0, 0, 200, 150, NULL, NULL, wc.hInstance, NULL);
        CHECK(hwnd);
        hwndChild = CreateWindow(wc.lpszClassName, "Child", WS_CHILD | WS_VISIBLE,
                                 5, 5, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndOwned = CreateWindow(wc.lpszClassName, "Owned", WS_POPUP | WS_VISIBLE,
                                 20, 20, 100, 80, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndChild && hwndOwned);
        hwndList = CreateWindow("ListBox", "", WS_CHILD | LBS_HASSTRINGS,
                                0, 0, 50, 40, hwnd, NULL, wc.hInstance, NULL);
        hwndCombo = CreateWindow("ComboBox", "", WS_CHILD | CBS_DROPDOWNLIST,
                                 0, 0, 80, 100, hwnd, NULL, wc.hInstance, NULL);
        CHECK(hwndList && hwndCombo);
        CHECK(SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"List allocation") == 0);
        CHECK(SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)"Combo allocation") == 0);
        CHECK(SendMessage(hwnd, WM_APP, 0, 0));
        SetFocus(hwndChild);
        SetCapture(hwndChild);
        hdc = GetDC(hwndChild);
        CHECK(hdc);
        CHECK(StarsCompatBeginDialogSequence(hwnd));
        CHECK(DialogBoxParam(wc.hInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
        CHECK(!IsWindowEnabled(hwnd));
        fReleased = TRUE;
        StarsCompatProcessExit();
        CHECK(cLate == 0);
        CHECK(!IsWindow(hwnd) && !IsWindow(hwndChild) && !IsWindow(hwndOwned));
        CHECK(!IsWindow(hwndList) && !IsWindow(hwndCombo));
        CHECK(!GetFocus() && !GetCapture());
        CHECK(SDL_WasInit(SDL_INIT_VIDEO) == 0);
        CHECK(!ReleaseDC(hwndChild, hdc));
        fReleased = FALSE;
    }
    SDL_Quit();
    return 0;
}
