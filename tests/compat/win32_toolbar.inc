/* Shared common-control contracts checked against the real Windows SDK. */
#include <windows.h>
#include <commctrl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static_assert(TB_ADDBUTTONSA == WM_USER + 20, "toolbar message ABI");
static_assert(TBN_DROPDOWN == (UINT)-710, "toolbar notification ABI");
static_assert(sizeof(INITCOMMONCONTROLSEX) == 8, "common-control ABI");
static_assert(sizeof(TBBUTTON) == (sizeof(void *) == 8 ? 32 : 20), "button ABI");
static_assert(offsetof(TBBUTTON, dwData) == (sizeof(void *) == 8 ? 16 : 12), "button data ABI");
static_assert(sizeof(NMHDR) == (sizeof(void *) == 8 ? 24 : 12), "notification ABI");
static_assert(sizeof(NMTOOLBARA) == (sizeof(void *) == 8 ? 96 : 60), "toolbar notification ABI");
static int cCommand, idCommand, cDrop, idDrop;
static RECT rcDrop;
static BOOL fDestroy, fDropRect;
static LRESULT resultDrop = TBDDRET_DEFAULT;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static BOOL fCheckPaint, fPaintPassed;
static COLORREF rgcrAction[2048];
static BOOL FMenuPaint(HWND hwnd, const RECT *lprc, BOOL fCapture) {
    HDC hdc = GetDC(hwnd);
    int x, y, i = 0, xArrow = lprc->right - 8, yArrow = (lprc->top + lprc->bottom) / 2 - 1;
    BOOL fPassed = 1;
    if (!hdc) return 0;
    for (y = lprc->top; y < lprc->bottom; y++)
        for (x = lprc->left; x < lprc->right - 11; x++) {
            if (i >= (int)(sizeof(rgcrAction) / sizeof(rgcrAction[0]))) { ReleaseDC(hwnd, hdc); return 0; }
            if (fCapture) rgcrAction[i] = GetPixel(hdc, x, y);
            else if (rgcrAction[i] != GetPixel(hdc, x, y)) fPassed = 0;
            i++;
        }
    if (!fCapture && (GetPixel(hdc, lprc->right - 5, lprc->top) != GetSysColor(COLOR_BTNSHADOW) ||
        GetPixel(hdc, lprc->right - 5, lprc->bottom - 1) != GetSysColor(COLOR_BTNHIGHLIGHT) ||
        GetPixel(hdc, xArrow + 2, yArrow) == GetSysColor(COLOR_BTNTEXT) ||
        GetPixel(hdc, xArrow + 3, yArrow + 3) != GetSysColor(COLOR_BTNTEXT))) fPassed = 0;
    ReleaseDC(hwnd, hdc); return fPassed;
}
#endif
static LRESULT CALLBACK Parent(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    NMTOOLBAR *lpnm;
    if (msg == WM_COMMAND && l) { cCommand++; idCommand = LOWORD(w); return 0; }
    if (msg == WM_NOTIFY) {
        lpnm = (NMTOOLBAR *)l;
        if (lpnm->hdr.code == TBN_DROPDOWN) {
            cDrop++; idDrop = lpnm->iItem;
            // Only hdr and iItem are valid for TBN_DROPDOWN.
            fDropRect = (BOOL)SendMessageA(lpnm->hdr.hwndFrom, TB_GETRECT, idDrop, (LPARAM)&rcDrop);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
            if (fCheckPaint) fPaintPassed = FMenuPaint(lpnm->hdr.hwndFrom, &rcDrop, 0);
#endif
            if (fDestroy) DestroyWindow(lpnm->hdr.hwndFrom);
            return resultDrop;
        }
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static void Click(HWND hwnd, int x, int y) {
    SendMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}
int main(void) {
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    WNDCLASS wc = {0};
    HWND hwnd, htb;
    HINSTANCE hinst;
    TBBUTTON rgbtn[6] = {0}, btn;
    TBADDBITMAP add;
    HBITMAP hbmp;
    HDC hdc, hmem;
    HGDIOBJ hOld;
    HBRUSH hbr;
    RECT rc, rcSplit, rcWhole;
    POINT pt;
    char sz[64];
    int i, j, c, cDropBefore;
    BOOL fChecked;
    APPMSG msg;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    HWND hwndTip, hwndFocus;
    RECT rcTip;
    POINT ptScreen;
    SDL_Event event = {0};
#endif
#ifdef _WIN32
    hinst = GetModuleHandleA(NULL);
#else
    hinst = (HINSTANCE)(UINT_PTR)1;
#endif
    CHECK(InitCommonControlsEx(&icc));
    wc.hInstance = hinst; wc.lpfnWndProc = Parent; wc.lpszClassName = "ToolbarContracts";
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "toolbar", WS_POPUP | WS_VISIBLE, 40, 50, 400, 180, NULL, NULL, hinst, NULL);
    CHECK(hwnd);
    htb = CreateWindowExA(0, TOOLBARCLASSNAMEA, NULL, WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS,
                       0, 0, 390, 40, hwnd, (HMENU)(INT_PTR)55, hinst, NULL);
    CHECK(htb && GetClassNameA(htb, sz, sizeof(sz)) && !strcmp(sz, TOOLBARCLASSNAMEA));
    SendMessageA(htb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    CHECK(SendMessageA(htb, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 15)));
    CHECK(SendMessageA(htb, TB_SETBUTTONSIZE, 0, MAKELPARAM(30, 28)));
    SendMessageA(htb, TB_SETMAXTEXTROWS, 0, 0);
    SendMessageA(htb, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
    CHECK(SendMessageA(htb, TB_GETEXTENDEDSTYLE, 0, 0) == TBSTYLE_EX_DRAWDDARROWS);
    for (i = 0; i < 6; i++) {
        rgbtn[i].idCommand = 100 + i; rgbtn[i].fsState = TBSTATE_ENABLED;
        rgbtn[i].iBitmap = 0; rgbtn[i].iString = (INT_PTR)"Toolbar tip";
    }
    rgbtn[0].fsStyle = rgbtn[1].fsStyle = BTNS_CHECKGROUP;
    rgbtn[2].fsStyle = BTNS_SEP; rgbtn[2].iBitmap = 20; rgbtn[2].iString = -1;
    rgbtn[3].fsStyle = BTNS_CHECK | BTNS_DROPDOWN;
    rgbtn[4].fsStyle = BTNS_WHOLEDROPDOWN;
    CHECK(SendMessageA(htb, TB_ADDBUTTONS, 6, (LPARAM)rgbtn));
    CHECK(SendMessageA(htb, TB_BUTTONCOUNT, 0, 0) == 6);
    CHECK(SendMessageA(htb, TB_COMMANDTOINDEX, 103, 0) == 3);
    CHECK(SendMessageA(htb, TB_GETBUTTON, 3, (LPARAM)&btn) && btn.idCommand == 103 && btn.fsStyle == rgbtn[3].fsStyle);
    CHECK(SendMessageA(htb, TB_GETBUTTONTEXT, 103, (LPARAM)sz) == 11 && !strcmp(sz, "Toolbar tip"));
    CHECK(SendMessageA(htb, TB_GETITEMRECT, 3, (LPARAM)&rcSplit));
    CHECK(SendMessageA(htb, TB_GETRECT, 103, (LPARAM)&rc) && EqualRect(&rc, &rcSplit));
    CHECK(SendMessageA(htb, TB_GETRECT, 104, (LPARAM)&rcWhole));
    pt = (POINT){rc.left + 2, rc.top + 2};
    CHECK(SendMessageA(htb, TB_HITTEST, 0, (LPARAM)&pt) == 3);
    CHECK(SendMessageA(htb, TB_CHECKBUTTON, 100, 1));
    CHECK(SendMessageA(htb, TB_GETRECT, 101, (LPARAM)&rc));
    Click(htb, rc.left + 5, rc.top + 5);
    CHECK(cCommand == 1 && idCommand == 101 && SendMessageA(htb, TB_ISBUTTONCHECKED, 101, 0));
    CHECK(!SendMessageA(htb, TB_ISBUTTONCHECKED, 100, 0));
    Click(htb, rcSplit.left + 3, rcSplit.top + 3);
    CHECK(cCommand == 2 && idCommand == 103 && cDrop == 0);
    Click(htb, rcSplit.right - 3, rcSplit.top + 3);
    CHECK(cDrop == 1 && idDrop == 103 && fDropRect && EqualRect(&rcDrop, &rcSplit) && cCommand == 2);
    Click(htb, rcWhole.left + 3, rcWhole.top + 3);
    CHECK(cDrop == 2 && idDrop == 104 && cCommand == 2 && fDropRect && EqualRect(&rcDrop, &rcWhole));
    CHECK(SendMessageA(htb, TB_ENABLEBUTTON, 104, 0));
    Click(htb, rcWhole.left + 3, rcWhole.top + 3);
    CHECK(cDrop == 2 && !SendMessageA(htb, TB_ISBUTTONENABLED, 104, 0));
    CHECK(SendMessageA(htb, TB_ENABLEBUTTON, 104, 1));
    // Unhandled dropdowns and TREATPRESSED complete an ordinary click on release.
    for (i = TBDDRET_NODEFAULT; i <= TBDDRET_TREATPRESSED; i++) {
        resultDrop = i;
        for (j = 103; j <= 104; j++) {
            CHECK(SendMessageA(htb, TB_GETRECT, j, (LPARAM)&rc));
            c = cCommand; cDropBefore = cDrop;
            fChecked = (BOOL)SendMessageA(htb, TB_ISBUTTONCHECKED, j, 0);
            SendMessageA(htb, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(rc.right - 3, rc.top + 3));
            CHECK(cDrop == cDropBefore + 1 && idDrop == j && cCommand == c && GetCapture() == htb);
            CHECK(SendMessageA(htb, TB_ISBUTTONCHECKED, j, 0) == fChecked);
            SendMessageA(htb, WM_LBUTTONUP, 0, MAKELPARAM(rc.right - 3, rc.top + 3));
            CHECK(cCommand == c + 1 && idCommand == j && !GetCapture());
            if (j == 103) CHECK(SendMessageA(htb, TB_ISBUTTONCHECKED, j, 0) == !fChecked);
        }
    }
    resultDrop = TBDDRET_DEFAULT;
    c = cCommand;
    SendMessageA(htb, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(rcSplit.left + 3, rcSplit.top + 3));
    SendMessageA(htb, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(-2, -2));
    SendMessageA(htb, WM_LBUTTONUP, 0, MAKELPARAM(-2, -2));
    CHECK(cCommand == c && !GetCapture());
    hdc = GetDC(hwnd); hmem = CreateCompatibleDC(hdc);
    hbmp = CreateCompatibleBitmap(hdc, 16, 15); CHECK(hdc && hmem && hbmp);
    hOld = SelectObject(hmem, hbmp);
    hbr = CreateSolidBrush(RGB(47, 80, 123)); SetRect(&rc, 0, 0, 16, 15);
    CHECK(hbr && FillRect(hmem, &rc, hbr)); DeleteObject(hbr); SelectObject(hmem, hOld);
    DeleteDC(hmem); ReleaseDC(hwnd, hdc);
    add.hInst = NULL; add.nID = (UINT_PTR)hbmp;
    CHECK(SendMessageA(htb, TB_ADDBITMAP, 1, (LPARAM)&add) == 0);
    CHECK(UpdateWindow(htb));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    DeleteObject(hbmp); // Native adapter owns a copy, even for caller bitmaps.
    CHECK(SendMessageA(htb, TB_GETRECT, 101, (LPARAM)&rc));
    hdc = GetDC(htb);
    CHECK(hdc && GetPixel(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2) == RGB(47, 80, 123));
    CHECK(GetPixel(hdc, rcSplit.right - 11, rcSplit.top + 3) == GetSysColor(COLOR_BTNHIGHLIGHT));
    CHECK(GetPixel(hdc, rcSplit.right - 10, rcSplit.top + 3) == GetSysColor(COLOR_3DLIGHT));
    CHECK(GetPixel(hdc, rcSplit.right - 5, rcSplit.bottom - 1) == GetSysColor(COLOR_3DDKSHADOW));
    CHECK(GetPixel(hdc, rcSplit.right - 5, rcSplit.bottom - 2) == GetSysColor(COLOR_BTNSHADOW));
    ReleaseDC(htb, hdc);
    for (i = 0; i < 2; i++) {
        CHECK(SendMessageA(htb, TB_CHECKBUTTON, 103, i));
        CHECK(UpdateWindow(htb) && FMenuPaint(htb, &rcSplit, 1));
        hdc = GetDC(htb);
        CHECK(hdc && GetPixel(hdc, rcSplit.right - 12, rcSplit.top + 3) ==
              GetSysColor(i ? COLOR_BTNHIGHLIGHT : COLOR_3DDKSHADOW));
        ReleaseDC(htb, hdc);
        fCheckPaint = 1; fPaintPassed = 0;
        Click(htb, rcSplit.right - 3, rcSplit.top + 3);
        fCheckPaint = 0;
        CHECK(fPaintPassed && SendMessageA(htb, TB_ISBUTTONCHECKED, 103, 0) == i);
        CHECK(UpdateWindow(htb) && FMenuPaint(htb, &rcSplit, 1));
        hdc = GetDC(htb);
        CHECK(hdc && GetPixel(hdc, rcSplit.right - 5, rcSplit.top) == GetSysColor(COLOR_BTNHIGHLIGHT));
        ReleaseDC(htb, hdc);
    }
    hwndTip = (HWND)SendMessageA(htb, TB_GETTOOLTIPS, 0, 0);
    CHECK(IsWindow(hwndTip));
    hwndFocus = GetFocus();
    ptScreen = (POINT){rcSplit.left + 3, rcSplit.top + 3};
    CHECK(ClientToScreen(htb, &ptScreen));
    if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
        pt = ptScreen; CHECK(ScreenToClient(hwnd, &pt));
        event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwnd->id;
        event.motion.x = (float)pt.x; event.motion.y = (float)pt.y;
        CompatRouteEvent(&event);
    } else CHECK(SDL_WarpMouseGlobal((float)ptScreen.x, (float)ptScreen.y));
    SendMessageA(htb, WM_MOUSEMOVE, 0, MAKELPARAM(rcSplit.left + 3, rcSplit.top + 3));
    SendMessageA(htb, WM_TIMER, 1, 0);
    CHECK(IsWindowVisible(hwndTip) && GetWindowRect(hwndTip, &rcTip));
    rc = rcSplit; MapWindowPoints(htb, NULL, (POINT *)&rc, 2);
    CHECK(rcTip.left == rc.left && rcTip.top == rc.bottom + 2 && GetFocus() == hwndFocus);
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
        CHECK(SDL_GetWindowFlags(hwndTip->lpWindow.get()) & SDL_WINDOW_TOOLTIP);
        CHECK(SDL_GetWindowParent(hwndTip->lpWindow.get()) == hwnd->lpWindow.get());
    }
    SendMessageA(htb, TB_SETHOTITEM, 3, 0);
    CHECK(SendMessageA(htb, TB_GETHOTITEM, 0, 0) == 3);
    c = cDrop;
    SendMessageA(htb, WM_KEYDOWN, VK_DOWN, 0);
    CHECK(cDrop == c + 1 && idDrop == 103);
#endif
    CHECK(SendMessageA(htb, TB_HIDEBUTTON, 105, 1) && SendMessageA(htb, TB_ISBUTTONHIDDEN, 105, 0));
    CHECK(SendMessageA(htb, TB_DELETEBUTTON, 5, 0) && SendMessageA(htb, TB_BUTTONCOUNT, 0, 0) == 5);
    rgbtn[5].idCommand = 200;
    CHECK(SendMessageA(htb, TB_INSERTBUTTON, 5, (LPARAM)&rgbtn[5]));
    CHECK(SendMessageA(htb, TB_COMMANDTOINDEX, 200, 0) == 5);
    fDestroy = 1;
    Click(htb, rcWhole.left + 3, rcWhole.top + 3);
    CHECK(!IsWindow(htb));
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    return 0;
}
