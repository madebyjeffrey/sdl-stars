/* String, multi-selection and owner-draw list contracts shared with the SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cMeasure, cDraw, cDelete, cChange;
static BOOL fDestroyDraw, fDestroyChange, fResetDraw;
static ULONG_PTR dataDraw, dataDelete;
static UINT stateDraw;
static int cFocusDraw, cEmptyDraw;
static UINT stateFocus, actionEmpty;
static int cFocusNotify, cDrawAtFocusNotify;
static int cSelectDraw;
static UINT grSelectedDraw, grDeselectedDraw;
static LRESULT CALLBACK ParentProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    MEASUREITEMSTRUCT *lpms;
    DRAWITEMSTRUCT *lpds;
    DELETEITEMSTRUCT *lpdel;
    if (msg == WM_MEASUREITEM) {
        lpms = (MEASUREITEMSTRUCT *)l; cMeasure++;
        if (lpms->CtlType == ODT_LISTBOX) lpms->itemHeight = 20;
        return TRUE;
    }
    if (msg == WM_DRAWITEM) {
        lpds = (DRAWITEMSTRUCT *)l; cDraw++;
        if ((lpds->itemAction & ODA_SELECT) && lpds->itemID < 8) {
            cSelectDraw++;
            if (lpds->itemState & ODS_SELECTED) grSelectedDraw |= 1u << lpds->itemID;
            else grDeselectedDraw |= 1u << lpds->itemID;
        }
        if (lpds->itemAction & ODA_FOCUS) { cFocusDraw++; stateFocus = lpds->itemState; }
        if (lpds->itemID == (UINT)-1) { cEmptyDraw++; actionEmpty = lpds->itemAction; }
        if (lpds->itemID == 0) { dataDraw = lpds->itemData; stateDraw = lpds->itemState; }
        if (fDestroyDraw) { fDestroyDraw = FALSE; DestroyWindow(lpds->hwndItem); return TRUE; }
        if (fResetDraw) { fResetDraw = FALSE; SendMessage(lpds->hwndItem, LB_RESETCONTENT, 0, 0); return TRUE; }
        FillRect(lpds->hDC, &lpds->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
        return TRUE;
    }
    if (msg == WM_DELETEITEM) {
        lpdel = (DELETEITEMSTRUCT *)l; cDelete++; dataDelete = lpdel->itemData; return TRUE;
    }
    if (msg == WM_COMMAND && HIWORD(w) == LBN_SELCHANGE) {
        cChange++;
        if (fDestroyChange) { fDestroyChange = FALSE; DestroyWindow((HWND)l); }
    }
    if (msg == WM_COMMAND && (HIWORD(w) == LBN_SETFOCUS || HIWORD(w) == LBN_KILLFOCUS)) {
        cFocusNotify++; cDrawAtFocusNotify = cFocusDraw;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static HWND NewList(HWND hwnd, int id, DWORD style) {
    return CreateWindow("ListBox", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOINTEGRALHEIGHT | style,
                        10 + (id % 3) * 130, 10, 120, 80, hwnd, (HMENU)(UINT_PTR)id, NULL, NULL);
}
static int TestOwnerSelection(HWND hwndParent) {
    HWND hwnd = NewList(hwndParent, 111, LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
    int i, c;
    CHECK(hwnd);
    for (i = 0; i < 3; i++) CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"selection") == i);
    UpdateWindow(hwnd); c = cChange;
    cSelectDraw = 0; grSelectedDraw = grDeselectedDraw = 0;
    CHECK(SendMessage(hwnd, LB_SETCURSEL, 1, 0) == 1);
    CHECK(cSelectDraw != 0 && grSelectedDraw == 2 && grDeselectedDraw == 0 && cChange == c);
    grSelectedDraw = grDeselectedDraw = 0;
    CHECK(SendMessage(hwnd, LB_SETCURSEL, 2, 0) == 2);
    CHECK(grSelectedDraw == 4 && grDeselectedDraw == 2 && cChange == c);
    c = cSelectDraw; SendMessage(hwnd, LB_SETCURSEL, 2, 0);
    CHECK(cSelectDraw == c);
    grSelectedDraw = grDeselectedDraw = 0;
    CHECK(SendMessage(hwnd, LB_SETCURSEL, -1, 0) == LB_ERR && grDeselectedDraw == 4);
    SetFocus(hwnd); CHECK(SendMessage(hwnd, LB_SETCURSEL, 0, 0) == 0);
    grSelectedDraw = grDeselectedDraw = 0; c = cChange;
    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1);
    CHECK(grSelectedDraw == 2 && grDeselectedDraw == 1 && cChange == c + 1);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    fResetDraw = TRUE; SendMessage(hwnd, LB_SETCURSEL, 0, 0);
    CHECK(IsWindow(hwnd) && SendMessage(hwnd, LB_GETCOUNT, 0, 0) == 0);
    CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"destroy") == 0);
    fDestroyDraw = TRUE; SendMessage(hwnd, LB_SETCURSEL, 0, 0);
    CHECK(!IsWindow(hwnd)); return 0;
#endif
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestOwnerFocus(HWND hwndParent) {
    HWND hwnd = NewList(hwndParent, 110, LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
    int c;
    CHECK(hwnd && SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"focus") == 0);
    SetFocus(hwndParent); UpdateWindow(hwnd);
    cFocusDraw = cFocusNotify = 0; SetFocus(hwnd);
    CHECK(cFocusDraw != 0 && (stateFocus & ODS_FOCUS));
    CHECK(cFocusNotify == 1 && cDrawAtFocusNotify == cFocusDraw);
    c = cFocusDraw; SetFocus(hwndParent);
    CHECK(cFocusDraw > c && !(stateFocus & ODS_FOCUS));
    CHECK(cFocusNotify == 2 && cDrawAtFocusNotify == cFocusDraw);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    fResetDraw = TRUE; SetFocus(hwnd);
    CHECK(IsWindow(hwnd) && SendMessage(hwnd, LB_GETCOUNT, 0, 0) == 0);
    SetFocus(hwndParent);
#endif
    SendMessage(hwnd, LB_RESETCONTENT, 0, 0); UpdateWindow(hwnd);
    cEmptyDraw = 0; SetFocus(hwnd); UpdateWindow(hwnd);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    // The SDK documents itemID == -1 for empty owner-drawn focus. Wine draws
    // the empty focus rectangle itself instead of notifying the owner.
    CHECK(cEmptyDraw != 0 && (actionEmpty & ODA_FOCUS));
    {
        HDC hdc = GetDC(hwnd);
        CHECK(hdc && GetPixel(hdc, 5, 5) == RGB(0, 0, 0) && GetPixel(hdc, 5, 30) == GetSysColor(COLOR_WINDOW));
        CHECK(ReleaseDC(hwnd, hdc));
    }
    fDestroyDraw = TRUE; SetFocus(hwndParent);
    CHECK(!IsWindow(hwnd));
#else
    printf("Wine empty owner-draw callbacks: %d (documented native case checked separately)\n", cEmptyDraw);
    CHECK(DestroyWindow(hwnd));
#endif
    return 0;
}
static int TestExtendedList(HWND hwndParent) {
    HWND hwnd = NewList(hwndParent, 106, LBS_NOTIFY | LBS_EXTENDEDSEL);
    BYTE rgbKeys[256] = {0};
    CHECK(hwnd);
    CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"alpha") == 0);
    CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"beta") == 1);
    CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"bravo") == 2);
    CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"gamma") == 3);
    SetFocus(hwnd);
    CHECK(SetKeyboardState(rgbKeys));
    SendMessage(hwnd, WM_KEYDOWN, VK_HOME, 1);
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 1 && SendMessage(hwnd, LB_GETSEL, 0, 0));
    rgbKeys[VK_SHIFT] = 0x80; CHECK(SetKeyboardState(rgbKeys));
    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1);
    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1);
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 3);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    // Wine's list implementation does not handle Ctrl keyboard selection or
    // multi-character type-ahead; qualify these native behaviors separately.
    rgbKeys[VK_SHIFT] = 0; rgbKeys[VK_CONTROL] = 0x80; CHECK(SetKeyboardState(rgbKeys));
    SendMessage(hwnd, WM_KEYDOWN, VK_END, 1);
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 3 && SendMessage(hwnd, LB_GETCARETINDEX, 0, 0) == 3);
    SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 1);
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 4);
    rgbKeys[VK_CONTROL] = 0; CHECK(SetKeyboardState(rgbKeys));
    SendMessage(hwnd, WM_CHAR, 'b', 1);
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 1 && SendMessage(hwnd, LB_GETSEL, 1, 0));
    SendMessage(hwnd, WM_CHAR, 'r', 1);
    CHECK(SendMessage(hwnd, LB_GETSEL, 2, 0));
#endif
    memset(rgbKeys, 0, sizeof(rgbKeys)); CHECK(SetKeyboardState(rgbKeys));
    DestroyWindow(hwnd);
    return 0;
}
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int TestCtrlDrag(HWND hwndParent) {
    HWND hwnd = NewList(hwndParent, 107, LBS_NOTIFY | LBS_EXTENDEDSEL);
    int i, cy, c;
    CHECK(hwnd);
    for (i = 0; i < 4; i++) CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"item") == i);
    SendMessage(hwnd, LB_SETSEL, TRUE, 0); SendMessage(hwnd, LB_SETSEL, TRUE, 3);
    cy = (int)SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0); c = cChange;
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_CONTROL, MAKELPARAM(3, cy + 1));
    SendMessage(hwnd, WM_MOUSEMOVE, MK_CONTROL | MK_LBUTTON, MAKELPARAM(3, 3 * cy + 1));
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 3 && !SendMessage(hwnd, LB_GETSEL, 3, 0));
    SendMessage(hwnd, WM_MOUSEMOVE, MK_CONTROL | MK_LBUTTON, MAKELPARAM(3, 2 * cy + 1));
    CHECK(SendMessage(hwnd, LB_GETSELCOUNT, 0, 0) == 4);
    CHECK(cChange == c);
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(3, 2 * cy + 1));
    CHECK(cChange == c + 1 && !GetCapture());
    SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(3, 1));
    SendMessage(hwnd, WM_CANCELMODE, 0, 0);
    CHECK(!GetCapture() && !hwnd->fTracking);
    CHECK(DestroyWindow(hwnd)); return 0;
}
#endif

static int TestListScroll(HWND hwndParent) {
    HWND hwnd = CreateWindow("ListBox", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
                             0, 110, 100, 64, hwndParent, NULL, NULL, NULL);
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    int i;
    CHECK(hwnd);
    for (i = 0; i < 30; i++) CHECK(SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"long list entry") == i);
    SendMessage(hwnd, LB_SETHORIZONTALEXTENT, 400, 0);
    CHECK(SendMessage(hwnd, LB_GETHORIZONTALEXTENT, 0, 0) == 400);
    CHECK(GetScrollInfo(hwnd, SB_VERT, &si) && si.nMax == 29 && si.nPage > 0 && si.nPage < 30);
    SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
    CHECK(SendMessage(hwnd, LB_GETTOPINDEX, 0, 0) > 0);
    SendMessage(hwnd, WM_HSCROLL, SB_PAGERIGHT, 0);
    CHECK(GetScrollPos(hwnd, SB_HORZ) > 0);
    SendMessage(hwnd, LB_RESETCONTENT, 0, 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!(GetWindowLongPtr(hwnd, GWL_STYLE) & WS_VSCROLL));
#endif
    CHECK(DestroyWindow(hwnd)); return 0;
}
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndList, hwndMulti, hwndOwner, hwndKill, hwndSorted;
    char sz[64];
    int rgi[3], c, i;
    RECT rc;
    wc.lpfnWndProc = ParentProc; wc.lpszClassName = "CompatListParent";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "List test", WS_POPUP | WS_VISIBLE, 30, 30, 420, 200, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    CHECK(TestOwnerSelection(hwnd) == 0);
    CHECK(TestOwnerFocus(hwnd) == 0);
    CHECK(TestExtendedList(hwnd) == 0);
    CHECK(TestListScroll(hwnd) == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestCtrlDrag(hwnd) == 0);
#endif
    hwndList = NewList(hwnd, 100, LBS_NOTIFY);
    hwndMulti = NewList(hwnd, 101, LBS_NOTIFY | LBS_MULTIPLESEL);
    c = cMeasure; hwndOwner = NewList(hwnd, 102, LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
    CHECK(hwndList && hwndMulti && hwndOwner && cMeasure == c + 1);
    CHECK(SendMessage(hwndOwner, LB_GETITEMHEIGHT, 0, 0) == 20);
    CHECK(SendMessage(hwndList, LB_GETCURSEL, 0, 0) == LB_ERR);
    CHECK(SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"Alpha") == 0);
    CHECK(SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"Bravo") == 1);
    CHECK(SendMessage(hwndList, LB_INSERTSTRING, 1, (LPARAM)"Beta") == 1);
    CHECK(SendMessage(hwndList, LB_GETCOUNT, 0, 0) == 3);
    CHECK(SendMessage(hwndList, LB_GETTEXT, 1, (LPARAM)sz) == 4 && strcmp(sz, "Beta") == 0);
    CHECK(SendMessage(hwndList, LB_GETTEXTLEN, 2, 0) == 5);
    CHECK(SendMessage(hwndList, LB_GETTEXTLEN, 3, 0) == LB_ERR);
    CHECK(SendMessage(hwndList, LB_FINDSTRING, -1, (LPARAM)"br") == 2);
    CHECK(SendMessage(hwndList, LB_FINDSTRINGEXACT, 1, (LPARAM)"ALPHA") == 0);
    c = cChange; CHECK(SendMessage(hwndList, LB_SETCURSEL, 1, 0) == 1 && cChange == c);
    CHECK(SendMessage(hwndList, LB_GETSEL, 1, 0) == 1);
    CHECK(SendMessage(hwndList, LB_DELETESTRING, 0, 0) == 2);
    CHECK(SendMessage(hwndList, LB_GETCURSEL, 0, 0) == 0);
    CHECK(SendMessage(hwndList, LB_SETITEMDATA, 0, (LPARAM)(UINT_PTR)0x12345678) == 1);
    CHECK(SendMessage(hwndList, LB_GETITEMDATA, 0, 0) == (LPARAM)(UINT_PTR)0x12345678);
    CHECK(SendMessage(hwndList, LB_GETSELCOUNT, 0, 0) == LB_ERR);
    for (i = 0; i < 3; i++) CHECK(SendMessage(hwndMulti, LB_ADDSTRING, 0, (LPARAM)"Item") == i);
    CHECK(SendMessage(hwndMulti, LB_SETCURSEL, 1, 0) == LB_ERR);
    CHECK(SendMessage(hwndMulti, LB_SETSEL, TRUE, -1) == 0);
    CHECK(SendMessage(hwndMulti, LB_SETSEL, FALSE, 1) == 0);
    CHECK(SendMessage(hwndMulti, LB_GETSELCOUNT, 0, 0) == 2);
    CHECK(SendMessage(hwndMulti, LB_GETSELITEMS, 3, (LPARAM)rgi) == 2 && rgi[0] == 0 && rgi[1] == 2);
    CHECK(SendMessage(hwndMulti, LB_GETSEL, 1, 0) == 0);
    CHECK(SendMessage(hwndList, LB_SETITEMHEIGHT, 0, 20) == 0);
    CHECK(SendMessage(hwndList, LB_GETITEMRECT, 1, (LPARAM)&rc) == 1 && rc.top == 20 && rc.bottom == 40);
    hwndSorted = NewList(hwnd, 106, LBS_SORT);
    CHECK(hwndSorted);
    CHECK(SendMessage(hwndSorted, LB_ADDSTRING, 0, (LPARAM)"Zulu") == 0);
    CHECK(SendMessage(hwndSorted, LB_ADDSTRING, 0, (LPARAM)"alpha") == 0);
    CHECK(SendMessage(hwndSorted, LB_GETTEXT, 0, (LPARAM)sz) == 5 && strcmp(sz, "alpha") == 0);
    CHECK(DestroyWindow(hwndSorted));
    SetFocus(hwndList); c = cChange;
    SendMessage(hwndList, WM_KEYDOWN, VK_DOWN, 1);
    CHECK(SendMessage(hwndList, LB_GETCURSEL, 0, 0) == 1 && cChange == c + 1);
    CHECK(SendMessage(hwndOwner, LB_ADDSTRING, 0, (LPARAM)"Drawn") == 0);
    CHECK(SendMessage(hwndOwner, LB_SETITEMDATA, 0, 0x42) == 1);
    CHECK(SendMessage(hwndOwner, LB_SETCURSEL, 0, 0) == 0);
    InvalidateRect(hwndOwner, NULL, TRUE); CHECK(UpdateWindow(hwndOwner));
    CHECK(cDraw != 0 && dataDraw == 0x42 && (stateDraw & ODS_SELECTED));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    {
        HDC hdc = GetDC(hwndOwner);
        CHECK(hdc && GetPixel(hdc, 5, 5) == RGB(0, 0, 0));
        CHECK(ReleaseDC(hwndOwner, hdc));
    }
#endif
    c = cDelete; SendMessage(hwndOwner, LB_RESETCONTENT, 0, 0);
    CHECK(cDelete == c + 1 && dataDelete == 0x42 && SendMessage(hwndOwner, LB_GETCOUNT, 0, 0) == 0);
    for (i = 0; i < 10; i++) CHECK(SendMessage(hwndOwner, LB_ADDSTRING, 0, (LPARAM)"Scroll") == i);
    CHECK(SendMessage(hwndOwner, LB_SETTOPINDEX, 4, 0) == 0);
    CHECK(SendMessage(hwndOwner, LB_GETTOPINDEX, 0, 0) == 4);
    CHECK(SendMessage(hwndOwner, LB_GETITEMRECT, 4, (LPARAM)&rc) == 1 && rc.top == 0);
    SendMessage(hwndOwner, LB_RESETCONTENT, 0, 0);
    hwndKill = NewList(hwnd, 103, LBS_NOTIFY);
    CHECK(hwndKill && SendMessage(hwndKill, LB_ADDSTRING, 0, (LPARAM)"Destroy") == 0);
    fDestroyChange = TRUE; SendMessage(hwndKill, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(5, 5));
    CHECK(IsWindow(hwndKill));
    SendMessage(hwndKill, WM_LBUTTONUP, 0, MAKELPARAM(5, 5));
    CHECK(!IsWindow(hwndKill));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    {
        SDL_Event event = {0};
        MSG msg;
        int cBefore = cChange;
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = HwndCompatRoot(hwndList)->id;
        event.button.button = SDL_BUTTON_LEFT; event.button.clicks = 1;
        event.button.x = hwndList->rcChild.left + 5; event.button.y = hwndList->rcChild.top + 5;
        CompatRouteEvent(&event);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        CHECK(GetCapture() == hwndList && cChange == cBefore);
        event.type = SDL_EVENT_MOUSE_BUTTON_UP; CompatRouteEvent(&event);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
        CHECK(!GetCapture() && cChange == cBefore + 1 && SendMessage(hwndList, LB_GETCURSEL, 0, 0) == 0);
    }
    CHECK(SendMessage(hwndOwner, LB_ADDSTRING, 0, (LPARAM)"Reset in draw") == 0);
    fResetDraw = TRUE; InvalidateRect(hwndOwner, NULL, TRUE); CHECK(UpdateWindow(hwndOwner));
    CHECK(SendMessage(hwndOwner, LB_GETCOUNT, 0, 0) == 0);
    CHECK(SendMessage(hwndOwner, LB_ADDSTRING, 0, (LPARAM)"Destroy in draw") == 0);
    fDestroyDraw = TRUE; InvalidateRect(hwndOwner, NULL, TRUE); CHECK(UpdateWindow(hwndOwner));
    CHECK(!IsWindow(hwndOwner));
#endif
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 list boxes passed");
    return 0;
}
