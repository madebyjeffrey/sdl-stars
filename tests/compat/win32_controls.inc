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
        checkAtClick = (UINT)SendMessageA((HWND)l, BM_GETCHECK, 0, 0);
        if (fDestroyClick) { fDestroyClick = 0; DestroyWindow((HWND)l); }
        if (fDestroyParent) { fDestroyParent = 0; DestroyWindow(hwnd); }
        return 0;
    }
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN) {
        if (fDestroyColor) { fDestroyColor = 0; DestroyWindow((HWND)l); return 0; }
        SetTextColor((HDC)w, RGB(0, 0, 0));
        SetBkColor((HDC)w, RGB(9, 70, 90));
        return (LRESULT)hbrColor;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static void Drain(void) {
    APPMSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
}
static HWND Button(HWND hwnd, HINSTANCE hInstance, int id, DWORD style, int y) {
    return CreateWindowExA(0, "Button", "&Button", WS_CHILD | WS_VISIBLE | style,
                        20, y, 150, 24, hwnd, (HMENU)(INT_PTR)id, hInstance, NULL);
}
#if !defined(_WIN32) || defined(STARS_UI_SDL)
/* Focusing a check/radio control must preserve every painted label pixel,
   including mnemonics, wrapped labels and text clipped by a narrow control. */
static int TestFocusLabels(HWND hwnd, HINSTANCE hInstance) {
    static const DWORD rgdwStyle[] = {BS_AUTOCHECKBOX, BS_AUTORADIOBUTTON, BS_AUTO3STATE};
    static const struct {
        DWORD dwStyle;
        int dy;
    } rgfmt[] = {
        {0, 15}, {BS_LEFTTEXT, 15}, {0, 21}, {BS_LEFTTEXT, 21},
        {BS_MULTILINE, 38}, {BS_MULTILINE | BS_LEFTTEXT, 38}
    };
    static const char *rgpszLabel[] = {"&Humanoid", "&Caf\xc3\xa9 and a long label"};
    LOGFONT lf = {0};
    COLORREF rgcrBefore[100 * 38];
    COLORREF cr;
    HWND hwndButton;
    HDC hdc;
    HFONT hfont;
    int i, k, x, y, dy, xLeft, xRight, cChanged;
    size_t j;

    lf.lfHeight = -13;
    strcpy(lf.lfFaceName, "Arial");
    hfont = CreateFontIndirectA(&lf);
    CHECK(hfont);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < sizeof(rgfmt) / sizeof(rgfmt[0]); j++) {
            for (k = 0; k < 2; k++) {
                dy = rgfmt[j].dy;
                hwndButton = CreateWindowExA(0, "Button", rgpszLabel[k], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                          rgdwStyle[i] | rgfmt[j].dwStyle,
                                          200, 40, 100, dy, hwnd, (HMENU)(INT_PTR)500, hInstance, NULL);
                CHECK(hwndButton);
                SendMessageA(hwndButton, WM_SETFONT, (WPARAM)hfont, 1);
                SetFocus(hwnd);
                CHECK(UpdateWindow(hwndButton));
                hdc = GetDC(hwndButton);
                CHECK(hdc);
                for (y = 0; y < dy; y++) {
                    for (x = 0; x < 100; x++) rgcrBefore[y * 100 + x] = GetPixel(hdc, x, y);
                }
                CHECK(ReleaseDC(hwndButton, hdc));
                SetFocus(hwndButton);
                CHECK(UpdateWindow(hwndButton));
                hdc = GetDC(hwndButton);
                CHECK(hdc);
                xLeft = rgfmt[j].dwStyle & BS_LEFTTEXT ? 0 : 17;
                xRight = rgfmt[j].dwStyle & BS_LEFTTEXT ? 83 : 100;
                cChanged = 0;
                for (y = 0; y < dy; y++) {
                    for (x = 0; x < 100; x++) {
                        cr = GetPixel(hdc, x, y);
                        if (cr != rgcrBefore[y * 100 + x]) cChanged++;
                        if (x >= xLeft && x < xRight && rgcrBefore[y * 100 + x] != RGB(9, 70, 90) &&
                            cr != rgcrBefore[y * 100 + x]) {
                            fprintf(stderr, "Focus overwrites label: style %lu text style %lu label %d at %d,%d\n",
                                    (unsigned long)rgdwStyle[i], (unsigned long)rgfmt[j].dwStyle, k, x, y);
                            return 1;
                        }
                    }
                }
                CHECK(cChanged != 0);
                if (dy == 15) {
                    CHECK(GetPixel(hdc, xLeft + 3, 0) == GetSysColor(COLOR_GRAYTEXT));
                    CHECK(GetPixel(hdc, xLeft + 3, dy - 1) == GetSysColor(COLOR_GRAYTEXT));
                }
                CHECK(ReleaseDC(hwndButton, hdc));
                CHECK(DestroyWindow(hwndButton));
            }
        }
    }
    CHECK(DeleteObject(hfont));
    return 0;
}

static int TestPaint(HWND hwnd, HWND hwndCheck, HWND hwndRadio, HWND hwndStatic) {
    HDC hdc;
    BYTE *rgb;
    size_t cb;
    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    CHECK(SendMessageA(hwndStatic, WM_GETFONT, 0, 0) == 0);
    SendMessageA(hwndStatic, WM_SETFONT, (WPARAM)hfont, 1);
    CHECK((HFONT)SendMessageA(hwndStatic, WM_GETFONT, 0, 0) == hfont);
    CHECK(UpdateWindow(hwndStatic));
    hdc = GetDC(hwndStatic);
    CHECK(hdc && GetPixel(hdc, 149, 23) == RGB(9, 70, 90));
    CHECK(ReleaseDC(hwndStatic, hdc));
    CHECK(CheckDlgButton(hwnd, 101, BST_UNCHECKED) && UpdateWindow(hwndCheck));
    cb = (size_t)hwndCheck->lpCanvas->pitch * hwndCheck->lpCanvas->h;
    rgb = (BYTE *)(malloc(cb));
    CHECK(rgb);
    memcpy(rgb, hwndCheck->lpCanvas->pixels, cb);
    CHECK(CheckDlgButton(hwnd, 101, BST_CHECKED) && UpdateWindow(hwndCheck));
    CHECK(memcmp(rgb, hwndCheck->lpCanvas->pixels, cb) != 0);
    free(rgb);
    SendMessageA(hwndRadio, BM_SETCHECK, BST_CHECKED, 0);
    CHECK(UpdateWindow(hwndRadio));
    hdc = GetDC(hwndRadio);
    // A round seven-pixel dot has white corners and a black center.
    CHECK(hdc && GetPixel(hdc, 3, 8) == RGB(255, 255, 255));
    CHECK(GetPixel(hdc, 6, 11) == RGB(0, 0, 0));
    CHECK(ReleaseDC(hwndRadio, hdc));
    fDestroyColor = 1;
    InvalidateRect(hwndStatic, NULL, 1);
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
    hInstance = GetModuleHandleA(NULL);
#else
    hInstance = (HINSTANCE)(UINT_PTR)1;
#endif
    hbrColor = CreateSolidBrush(RGB(9, 70, 90));
    CHECK(hbrColor);
    wc.hInstance = hInstance; wc.lpszClassName = "StarsControlContracts"; wc.lpfnWndProc = ParentProc;
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "controls", WS_POPUP | WS_VISIBLE, 40, 50, 400, 360,
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
    hwndStatic = CreateWindowExA(0, "Static", "Label", WS_CHILD | WS_VISIBLE | SS_LEFT,
                             200, 10, 150, 24, hwnd, (HMENU)(INT_PTR)300, hInstance, NULL);
    CHECK(hwndPush && hwndCheck && hwndTri && hwndRadio1 && hwndRadio2 && hwndRadio3 && hwndStatic && hwndGroup);
    Drain();
    CHECK(GetClassNameA(hwndCheck, sz, sizeof(sz)) && strcmp(sz, "Button") == 0);
    CHECK(SendMessageA(hwndPush, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON);
    CHECK(SendMessageA(hwndRadio1, WM_GETDLGCODE, 0, 0) & DLGC_RADIOBUTTON);
    CHECK(SendMessageA(hwndStatic, WM_GETDLGCODE, 0, 0) & DLGC_STATIC);
    CHECK(GetDlgItemTextA(hwnd, 300, sz, sizeof(sz)) == 5 && strcmp(sz, "Label") == 0);
    CHECK(SetDlgItemTextA(hwnd, 300, "Updated label"));
    CHECK(GetDlgItemTextA(hwnd, 300, sz, 4) == 3 && strcmp(sz, "Upd") == 0);
    CHECK(!GetDlgItemTextA(hwnd, 999, sz, sizeof(sz)));
    CHECK(!SetDlgItemTextA(hwnd, 999, "missing"));
    CHECK(!GetDlgItem(hwnd, 999));
    CHECK(!CheckDlgButton(hwnd, 999, BST_CHECKED));
    CHECK(!CheckDlgButton(NULL, 101, BST_CHECKED));
    CHECK(SendDlgItemMessageA(hwnd, 999, BM_GETCHECK, 0, 0) == 0);
    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessageA(hwndPush, WM_SETFONT, (WPARAM)hfont, 1);
    CHECK((HFONT)SendMessageA(hwndPush, WM_GETFONT, 0, 0) == hfont);
    CHECK(CheckDlgButton(hwnd, 101, BST_CHECKED) && IsDlgButtonChecked(hwnd, 101) == BST_CHECKED);
    CHECK(CheckDlgButton(hwnd, 101, BST_UNCHECKED));
    c = cClick;
    SendMessageA(hwndCheck, BM_CLICK, 0, 0);
    CHECK(cClick == c + 1 && idClick == 101 && hwndClick == hwndCheck && checkAtClick == BST_CHECKED);
    CHECK(IsDlgButtonChecked(hwnd, 101) == BST_CHECKED && !GetCapture());
    SendMessageA(hwndCheck, BM_CLICK, 0, 0);
    CHECK(IsDlgButtonChecked(hwnd, 101) == BST_UNCHECKED);
    for (i = 1; i <= 3; i++) {
        SendMessageA(hwndTri, BM_CLICK, 0, 0);
        CHECK(IsDlgButtonChecked(hwnd, 102) == (UINT)(i % 3));
    }
    CHECK(CheckRadioButton(hwnd, 201, 202, 201));
    CHECK(IsDlgButtonChecked(hwnd, 201) == BST_CHECKED && IsDlgButtonChecked(hwnd, 202) == BST_UNCHECKED);
    CHECK(CheckDlgButton(hwnd, 205, BST_CHECKED));
    SendMessageA(hwndRadio2, BM_CLICK, 0, 0);
    CHECK(IsDlgButtonChecked(hwnd, 201) == BST_UNCHECKED && IsDlgButtonChecked(hwnd, 202) == BST_CHECKED);
    CHECK(IsDlgButtonChecked(hwnd, 205) == BST_CHECKED && (GetWindowLongPtrA(hwndRadio2, GWL_STYLE) & WS_TABSTOP));
    CHECK(CheckRadioButton(hwnd, 201, 202, 201));
    CHECK(IsDlgButtonChecked(hwnd, 205) == BST_CHECKED);
    c = cClick;
    EnableWindow(hwndPush, 0); SendMessageA(hwndPush, BM_CLICK, 0, 0);
    CHECK(cClick == c + 1 && idClick == 100);
    c = cClick;
    EnableWindow(hwndPush, 1);
    SendMessageA(hwndPush, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 2));
    CHECK(GetCapture() == hwndPush && (SendMessageA(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED));
    SendMessageA(hwndPush, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(-5, -5));
    CHECK(!(SendMessageA(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED));
    SendMessageA(hwndPush, WM_LBUTTONUP, 0, MAKELPARAM(-5, -5));
    CHECK(cClick == c && !GetCapture());
    SendMessageA(hwndPush, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 2));
    ReleaseCapture();
    SendMessageA(hwndPush, WM_LBUTTONUP, 0, MAKELPARAM(2, 2));
    CHECK(cClick == c);
    SetFocus(hwndPush);
    SendMessageA(hwndPush, WM_KEYDOWN, VK_SPACE, 1);
    CHECK(SendMessageA(hwndPush, BM_GETSTATE, 0, 0) & BST_PUSHED);
    SendMessageA(hwndPush, WM_KEYUP, VK_SPACE, 0xc0000001);
    CHECK(cClick == c + 1 && idClick == 100);
    CHECK(ClientToScreen(hwndStatic, &pt));
    CHECK(WindowFromPoint(pt) == hwnd);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestFocusLabels(hwnd, hInstance) == 0);
    CHECK(TestPaint(hwnd, hwndCheck, hwndRadio1, hwndStatic) == 0);
    CHECK(!CreateWindowExA(0, "Button", "unsupported", WS_CHILD | BS_ICON, 0, 0, 30, 30, hwnd, NULL, hInstance, NULL));
#endif
    hwndKill = Button(hwnd, hInstance, 400, BS_PUSHBUTTON, 240);
    CHECK(hwndKill);
    fDestroyClick = 1;
    SendMessageA(hwndKill, BM_CLICK, 0, 0);
    CHECK(!IsWindow(hwndKill));
    hwndKill = Button(hwnd, hInstance, 401, BS_PUSHBUTTON, 240);
    CHECK(hwndKill);
    fDestroyParent = 1;
    SendMessageA(hwndKill, BM_CLICK, 0, 0);
    CHECK(!IsWindow(hwnd));
    CHECK(DeleteObject(hbrColor));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    CHECK((GetSysColor(COLOR_WINDOW) & 0xff000000) == 0);
    puts("Win32 standard controls passed");
    return 0;
}
