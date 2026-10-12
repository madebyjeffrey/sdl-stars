/* Classic button and static-control contracts shared with the Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cClick, idClick;
static HWND hwndClick;
static UINT checkAtClick;
static BOOL fDestroyClick, fDestroyParent, fDestroyColor;
static HBRUSH hbrColor;
static LRESULT CALLBACK ParentProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_COMMAND && HIWORD(w) == BN_CLICKED) {
        cClick++; idClick = LOWORD(w); hwndClick = (HWND)l;
        checkAtClick = (UINT)SendMessage((HWND)l, BM_GETCHECK, 0, 0);
        if (fDestroyClick) { fDestroyClick = FALSE; DestroyWindow((HWND)l); }
        if (fDestroyParent) { fDestroyParent = FALSE; DestroyWindow(hwnd); }
        return 0;
    }
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN) {
        if (fDestroyColor) { fDestroyColor = FALSE; DestroyWindow((HWND)l); return 0; }
        SetTextColor((HDC)w, RGB(0, 0, 0));
        SetBkColor((HDC)w, RGB(9, 70, 90));
        return (LRESULT)hbrColor;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static void Drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}
static HWND Button(HWND hwnd, HINSTANCE hInstance, int id, DWORD style, int y) {
    return CreateWindow("Button", "&Button", WS_CHILD | WS_VISIBLE | style,
                        20, y, 150, 24, hwnd, (HMENU)(INT_PTR)id, hInstance, NULL);
}
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int TestPaint(HWND hwnd, HWND hwndCheck, HWND hwndRadio, HWND hwndStatic) {
    HDC hdc;
    BYTE *rgb;
    size_t cb;
    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CHECK(SendMessage(hwndStatic, WM_GETFONT, 0, 0) == 0);
    SendMessage(hwndStatic, WM_SETFONT, (WPARAM)hfont, TRUE);
    CHECK((HFONT)SendMessage(hwndStatic, WM_GETFONT, 0, 0) == hfont);
    CHECK(UpdateWindow(hwndStatic));
    hdc = GetDC(hwndStatic);
    CHECK(hdc && GetPixel(hdc, 149, 23) == RGB(9, 70, 90));
    CHECK(ReleaseDC(hwndStatic, hdc));
    CHECK(CheckDlgButton(hwnd, 101, BST_UNCHECKED) && UpdateWindow(hwndCheck));
    cb = (size_t)hwndCheck->lpCanvas->pitch * hwndCheck->lpCanvas->h;
    rgb = malloc(cb);
    CHECK(rgb);
    memcpy(rgb, hwndCheck->lpCanvas->pixels, cb);
    CHECK(CheckDlgButton(hwnd, 101, BST_CHECKED) && UpdateWindow(hwndCheck));
    CHECK(memcmp(rgb, hwndCheck->lpCanvas->pixels, cb) != 0);
    free(rgb);
    SendMessage(hwndRadio, BM_SETCHECK, BST_CHECKED, 0);
    CHECK(UpdateWindow(hwndRadio));
    hdc = GetDC(hwndRadio);
    // A round seven-pixel dot has white corners and a black center.
    CHECK(hdc && GetPixel(hdc, 3, 8) == RGB(255, 255, 255));
    CHECK(GetPixel(hdc, 6, 11) == RGB(0, 0, 0));
    CHECK(ReleaseDC(hwndRadio, hdc));
    fDestroyColor = TRUE;
    InvalidateRect(hwndStatic, NULL, TRUE);
    CHECK(UpdateWindow(hwndStatic) && !IsWindow(hwndStatic));
    return 0;
}
#endif
int main(void) {
    WNDCLASS wc = {0};
    HINSTANCE hInstance;
    HWND hwnd, hwndPush, hwndCheck, hwndTri, hwndRadio1, hwndRadio2, hwndRadio3, hwndStatic, hwndGroup, hwndKill;
    char sz[32];
    HFONT hfont;
    POINT pt = {2, 2};
    int c, i;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    hInstance = GetModuleHandle(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    hbrColor = CreateSolidBrush(RGB(9, 70, 90));
    CHECK(hbrColor);
    wc.hInstance = hInstance; wc.lpszClassName = "StarsControlContracts"; wc.lpfnWndProc = ParentProc;
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "controls", WS_POPUP | WS_VISIBLE, 40, 50, 400, 360,
                        NULL, NULL, hInstance, NULL);
    CHECK(hwnd);
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    CHECK(SetForegroundWindow(hwnd));
#endif
    hwndPush = Button(hwnd, hInstance, 100, BS_DEFPUSHBUTTON | WS_TABSTOP, 10);
    hwndCheck = Button(hwnd, hInstance, 101, BS_AUTOCHECKBOX | WS_TABSTOP, 40);
    hwndTri = Button(hwnd, hInstance, 102, BS_AUTO3STATE, 70);
    hwndRadio1 = Button(hwnd, hInstance, 201, BS_AUTORADIOBUTTON | WS_GROUP, 100);
    hwndRadio2 = Button(hwnd, hInstance, 202, BS_AUTORADIOBUTTON, 130);
    hwndRadio3 = Button(hwnd, hInstance, 205, BS_AUTORADIOBUTTON | WS_GROUP, 160);
    hwndGroup = Button(hwnd, hInstance, 301, BS_GROUPBOX, 200);
    hwndStatic = CreateWindow("Static", "Label", WS_CHILD | WS_VISIBLE | SS_LEFT,
                             200, 10, 150, 24, hwnd, (HMENU)(INT_PTR)300, hInstance, NULL);
    CHECK(hwndPush && hwndCheck && hwndTri && hwndRadio1 && hwndRadio2 && hwndRadio3 && hwndStatic && hwndGroup);
    Drain();
    CHECK(GetClassName(hwndCheck, sz, sizeof(sz)) && strcmp(sz, "Button") == 0);
    CHECK(SendMessage(hwndPush, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON);
    CHECK(SendMessage(hwndRadio1, WM_GETDLGCODE, 0, 0) & DLGC_RADIOBUTTON);
    CHECK(SendMessage(hwndStatic, WM_GETDLGCODE, 0, 0) & DLGC_STATIC);
    CHECK(GetDlgItemText(hwnd, 300, sz, sizeof(sz)) == 5 && strcmp(sz, "Label") == 0);
    CHECK(SetDlgItemText(hwnd, 300, "Updated label"));
    CHECK(GetDlgItemText(hwnd, 300, sz, 4) == 3 && strcmp(sz, "Upd") == 0);
    CHECK(!GetDlgItemText(hwnd, 999, sz, sizeof(sz)));
    CHECK(!SetDlgItemText(hwnd, 999, "missing"));
    CHECK(CheckDlgButton(hwnd, 999, BST_CHECKED));
    CHECK(SendDlgItemMessage(hwnd, 999, BM_GETCHECK, 0, 0) == 0);
    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwndPush, WM_SETFONT, (WPARAM)hfont, TRUE);
    CHECK((HFONT)SendMessage(hwndPush, WM_GETFONT, 0, 0) == hfont);
    CHECK(CheckDlgButton(hwnd, 101, BST_CHECKED) && IsDlgButtonChecked(hwnd, 101) == BST_CHECKED);
    CHECK(CheckDlgButton(hwnd, 101, BST_UNCHECKED));
    c = cClick;
    SendMessage(hwndCheck, BM_CLICK, 0, 0);
    CHECK(cClick == c + 1 && idClick == 101 && hwndClick == hwndCheck && checkAtClick == BST_CHECKED);
    CHECK(IsDlgButtonChecked(hwnd, 101) == BST_CHECKED && !GetCapture());
    SendMessage(hwndCheck, BM_CLICK, 0, 0);
    CHECK(IsDlgButtonChecked(hwnd, 101) == BST_UNCHECKED);
    for (i = 1; i <= 3; i++) {
        SendMessage(hwndTri, BM_CLICK, 0, 0);
        CHECK(IsDlgButtonChecked(hwnd, 102) == (UINT)(i % 3));
    }
    CHECK(CheckRadioButton(hwnd, 201, 202, 201));
    CHECK(IsDlgButtonChecked(hwnd, 201) == BST_CHECKED && IsDlgButtonChecked(hwnd, 202) == BST_UNCHECKED);
    CHECK(CheckDlgButton(hwnd, 205, BST_CHECKED));
    SendMessage(hwndRadio2, BM_CLICK, 0, 0);
    CHECK(IsDlgButtonChecked(hwnd, 201) == BST_UNCHECKED && IsDlgButtonChecked(hwnd, 202) == BST_CHECKED);
    CHECK(IsDlgButtonChecked(hwnd, 205) == BST_CHECKED && (GetWindowLongPtr(hwndRadio2, GWL_STYLE) & WS_TABSTOP));
    CHECK(CheckRadioButton(hwnd, 201, 202, 201));
    CHECK(IsDlgButtonChecked(hwnd, 205) == BST_CHECKED);
    c = cClick;
    EnableWindow(hwndPush, FALSE); SendMessage(hwndPush, BM_CLICK, 0, 0);
    CHECK(cClick == c + 1 && idClick == 100);
    c = cClick;
    EnableWindow(hwndPush, TRUE);
    SendMessage(hwndPush, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 2));
    CHECK(GetCapture() == hwndPush && (SendMessage(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED));
    SendMessage(hwndPush, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(-5, -5));
    CHECK(!(SendMessage(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED));
    SendMessage(hwndPush, WM_LBUTTONUP, 0, MAKELPARAM(-5, -5));
    CHECK(cClick == c && !GetCapture());
    SendMessage(hwndPush, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 2));
    ReleaseCapture();
    SendMessage(hwndPush, WM_LBUTTONUP, 0, MAKELPARAM(2, 2));
    CHECK(cClick == c);
    SetFocus(hwndPush);
    SendMessage(hwndPush, WM_KEYDOWN, VK_SPACE, 1);
    CHECK(SendMessage(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED);
    SendMessage(hwndPush, WM_KEYUP, VK_SPACE, 0xc0000001);
    CHECK(cClick == c + 1 && idClick == 100);
    CHECK(ClientToScreen(hwndStatic, &pt));
    CHECK(WindowFromPoint(pt) == hwnd);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestPaint(hwnd, hwndCheck, hwndRadio1, hwndStatic) == 0);
    CHECK(!CreateWindow("Button", "unsupported", WS_CHILD | BS_ICON, 0, 0, 30, 30, hwnd, NULL, hInstance, NULL));
#endif
    hwndKill = Button(hwnd, hInstance, 400, BS_PUSHBUTTON, 240);
    CHECK(hwndKill);
    fDestroyClick = TRUE;
    SendMessage(hwndKill, BM_CLICK, 0, 0);
    CHECK(!IsWindow(hwndKill));
    hwndKill = Button(hwnd, hInstance, 401, BS_PUSHBUTTON, 240);
    CHECK(hwndKill);
    fDestroyParent = TRUE;
    SendMessage(hwndKill, BM_CLICK, 0, 0);
    CHECK(!IsWindow(hwnd));
    CHECK(DeleteObject(hbrColor));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    CHECK((GetSysColor(COLOR_WINDOW) & 0xff000000) == 0);
    puts("Win32 standard controls passed");
    return 0;
}
