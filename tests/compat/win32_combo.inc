/* String/edit/owner-draw combo behavior and canonical dialog creation vs SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../../res/resource.h"
#include "ui_manifest.h"
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cSel, cDrop, cClose, cDraw, cDelete, cErrors;
static BOOL fDestroyDrop, fResetDraw, fDestroyDraw, fFocusOnSelect;
static LRESULT CALLBACK ComboOwner(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    DRAWITEMSTRUCT *lpds;
    MEASUREITEMSTRUCT *lpms;
    char szItem[64];
    if (msg == WM_COMMAND && l) {
        if (HIWORD(w) == CBN_SELCHANGE) { cSel++; if (fFocusOnSelect) SetFocus(hwnd); }
        if (HIWORD(w) == CBN_CLOSEUP) cClose++;
        if (HIWORD(w) == CBN_DROPDOWN) { cDrop++; if (fDestroyDrop) DestroyWindow((HWND)l); }
    }
    if (msg == WM_MEASUREITEM) { lpms = (MEASUREITEMSTRUCT *)l; if (lpms->CtlType == ODT_COMBOBOX) lpms->itemHeight = 18; return TRUE; }
    if (msg == WM_DELETEITEM) { if (((DELETEITEMSTRUCT *)l)->CtlType == ODT_COMBOBOX) cDelete++; return TRUE; }
    if (msg == WM_DRAWITEM) {
        lpds = (DRAWITEMSTRUCT *)l; cDraw++;
        if (lpds->CtlType != ODT_COMBOBOX) cErrors++;
        if (lpds->itemID != (UINT)-1 && lpds->itemID >= (UINT)SendMessageA(lpds->hwndItem, CB_GETCOUNT, 0, 0)) cErrors++;
        if (lpds->itemID != (UINT)-1 &&
            SendMessageA(lpds->hwndItem, CB_GETLBTEXT, lpds->itemID, (LPARAM)szItem) < 0) cErrors++;
        FillRect(lpds->hDC, &lpds->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
        if (fResetDraw) { fResetDraw = FALSE; SendMessageA(lpds->hwndItem, CB_RESETCONTENT, 0, 0); }
        if (fDestroyDraw) { fDestroyDraw = FALSE; DestroyWindow(lpds->hwndItem); }
        return TRUE;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static void Drain(void) { APPMSG msg; while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); } }
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static BOOL FComboButtonPainted(HWND hwnd, BOOL fDown) {
    RECT rc;
    HDC hdc;
    BOOL fPassed;
    if (!UpdateWindow(hwnd) || !GetClientRect(hwnd, &rc) || !(hdc = GetDC(hwnd))) return FALSE;
    fPassed = GetPixel(hdc, rc.right - 10, 2) == GetSysColor(fDown ? COLOR_BTNSHADOW : COLOR_BTNHIGHLIGHT) &&
              GetPixel(hdc, rc.right - 10, 3) == GetSysColor(fDown ? COLOR_3DDKSHADOW : COLOR_3DLIGHT) &&
              GetPixel(hdc, rc.right - 10, rc.bottom - 3) == GetSysColor(fDown ? COLOR_BTNHIGHLIGHT : COLOR_3DDKSHADOW) &&
              GetPixel(hdc, 5, 0) == GetSysColor(COLOR_BTNSHADOW) &&
              GetPixel(hdc, 5, 1) == GetSysColor(COLOR_3DDKSHADOW);
    ReleaseDC(hwnd, hdc); return fPassed;
}
static BOOL FPopupTextPainted(HWND hwndPopup, int cyItem) {
    HDC hdc = GetDC(hwndPopup);
    COLORREF cr;
    RECT rc;
    int x, y;
    BOOL fText = FALSE;
    if (!hdc) return FALSE;
    GetClientRect(hwndPopup, &rc); cr = GetPixel(hdc, 3, 3);
    for (y = 3; y < (((cyItem) < (rc.bottom - 2)) ? (cyItem) : (rc.bottom - 2)) && !fText; y++)
        for (x = 3; x < rc.right - 3; x++)
            if (GetPixel(hdc, x, y) != cr) { fText = TRUE; break; }
    ReleaseDC(hwndPopup, hdc);
    return fText;
}
#endif
int main(void) {
    WNDCLASS wc = {0};
    HINSTANCE hinst = GetModuleHandle(NULL);
    HWND hwnd, hcb, hother, hedit, howner, hdlg, hchild;
    RECT rc;
    char sz[64], szBefore[64];
    int i, cBefore, cDialogs = 0;
    BYTE rgKeys[256] = {0};
    APPMSG msg = {0};
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    RECT rcField;
    SDL_Event event = {0};
    Uint64 msPopup;
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    wc.lpfnWndProc = ComboOwner; wc.hInstance = hinst; wc.lpszClassName = "ComboContract";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Combo contracts", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 80, 80, 400, 260, NULL, NULL, hinst, NULL);
    CHECK(hwnd);
    hcb = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 10, 10, 150, 100, hwnd, (HMENU)101, hinst, NULL);
    hother = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 190, 10, 150, 100, hwnd, (HMENU)102, hinst, NULL);
    hedit = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 10, 60, 150, 100, hwnd, (HMENU)103, hinst, NULL);
    howner = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 190, 60, 150, 100, hwnd, (HMENU)104, hinst, NULL);
    CHECK(hcb && hother && hedit && howner); Drain();
    CHECK(GetClientRect(hcb, &rc) && rc.right == 150);
    CHECK(GetClientRect(hedit, &rc) && rc.right == 150);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!hcb->rghwndScroll[SB_VERT] && !hedit->rghwndScroll[SB_VERT]);
    CHECK(FComboButtonPainted(hedit, FALSE));
#endif
    CHECK(GetWindowRect(hcb, &rc) && rc.bottom - rc.top < 100);
    cBefore = cSel;
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == CB_ERR);
    CHECK(SendMessageA(hcb, CB_ADDSTRING, 0, (LPARAM)"Alpha") == 0);
    CHECK(SendMessageA(hcb, CB_ADDSTRING, 0, (LPARAM)"Beta") == 1);
    CHECK(SendMessageA(hcb, CB_INSERTSTRING, 1, (LPARAM)"Azure") == 1);
    CHECK(SendMessageA(hcb, CB_GETCOUNT, 0, 0) == 3 && SendMessageA(hother, CB_GETCOUNT, 0, 0) == 0);
    CHECK(SendMessageA(hcb, CB_SETITEMDATA, 2, 123456) != CB_ERR && SendMessageA(hcb, CB_GETITEMDATA, 2, 0) == 123456);
    CHECK(SendMessageA(hcb, CB_GETLBTEXTLEN, 1, 0) == 5 && SendMessageA(hcb, CB_GETLBTEXT, 1, (LPARAM)sz) == 5 && strcmp(sz, "Azure") == 0);
    CHECK(SendMessageA(hcb, CB_FINDSTRING, -1, (LPARAM)"az") == 1);
    CHECK(SendMessageA(hcb, CB_FINDSTRINGEXACT, -1, (LPARAM)"beta") == 2);
    CHECK(SendMessageA(hcb, CB_SETCURSEL, 2, 0) == 2 && GetWindowText(hcb, sz, sizeof(sz)) == 4 && strcmp(sz, "Beta") == 0);
    CHECK(cSel == cBefore);
    CHECK(SendMessageA(hcb, CB_DELETESTRING, 0, 0) == 2 && SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1);
    CHECK(SendMessageA(hcb, CB_SETCURSEL, -1, 0) == CB_ERR && GetWindowText(hcb, sz, sizeof(sz)) == 0);
    CHECK(SendMessageA(hcb, CB_SELECTSTRING, -1, (LPARAM)"Az") == 0);
    SetFocus(hcb); SendMessageA(hcb, WM_KEYDOWN, VK_DOWN, 0);
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1 && cSel > cBefore);
    CHECK(SendMessageA(hedit, CB_ADDSTRING, 0, (LPARAM)"Ten") == 0 && SendMessageA(hedit, CB_ADDSTRING, 0, (LPARAM)"Twenty") == 1);
    CHECK(SendMessageA(hedit, CB_SETCURSEL, 1, 0) == 1 && GetWindowText(hedit, sz, sizeof(sz)) == 6 && strcmp(sz, "Twenty") == 0);
    CHECK(SetWindowTextA(hedit, "custom") && GetWindowText(hedit, sz, sizeof(sz)) == 6 && strcmp(sz, "custom") == 0);
    hchild = GetWindow(hedit, GW_CHILD); CHECK(hchild);
    SetFocus(hchild); SendMessageA(hchild, WM_CHAR, 'x', 0);
    CHECK(SendMessageA(hedit, CB_GETCURSEL, 0, 0) == CB_ERR);
    CHECK(GetWindowText(hedit, szBefore, sizeof(szBefore)) > 0);
    CHECK(SendMessageA(hedit, CB_ADDSTRING, 0, (LPARAM)"Thirty") == 2);
    CHECK(GetWindowText(hedit, sz, sizeof(sz)) > 0 && strcmp(sz, szBefore) == 0);
    SetFocus(hchild); SendMessageA(hchild, WM_KEYDOWN, VK_UP, 0);
    CHECK(SendMessageA(hedit, CB_GETCURSEL, 0, 0) >= 0);
    CHECK(SendMessageA(howner, CB_GETITEMHEIGHT, 0, 0) == 18);
    CHECK(SendMessageA(howner, CB_ADDSTRING, 0, (LPARAM)"Ship") == 0 && SendMessageA(howner, CB_SETCURSEL, 0, 0) == 0);
    InvalidateRect(howner, NULL, TRUE); UpdateWindow(howner); CHECK(cDraw > 0 && cErrors == 0);
    puts("Combo data, edit and owner-draw contracts passed");
    SetFocus(hcb); cBefore = cDrop;
    SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0); CHECK(SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0) && cDrop == cBefore + 1);
    SendMessageA(hcb, WM_KEYDOWN, VK_ESCAPE, 0); CHECK(!SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0) && cClose > 0);
    CHECK(SendMessageA(hcb, CB_SETEXTENDEDUI, TRUE, 0) == CB_OKAY && SendMessageA(hcb, CB_GETEXTENDEDUI, 0, 0));
    SendMessageA(hcb, WM_KEYDOWN, VK_F4, 0); CHECK(!SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0));
    SendMessageA(hcb, WM_KEYDOWN, VK_DOWN, 0); CHECK(SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0));
    SendMessageA(hcb, WM_KEYDOWN, VK_ESCAPE, 0); CHECK(SendMessageA(hcb, CB_SETEXTENDEDUI, FALSE, 0) == CB_OKAY);
    // Stars' selection handler returns focus to the frame during CBN_SELCHANGE.
    CHECK(SendMessageA(hcb, CB_SETCURSEL, 0, 0) == 0);
    SetFocus(hcb); SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0);
    SendMessageA(hcb, WM_KEYDOWN, VK_DOWN, 0);
    fFocusOnSelect = TRUE;
    SendMessageA(hcb, WM_KEYDOWN, VK_RETURN, 0);
    fFocusOnSelect = FALSE;
    CHECK(!SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0) && SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1);
    puts("Combo popup open/close contracts passed");
    for (i = 0; i < (int)(sizeof(rgUIManifest) / sizeof(rgUIManifest[0])); i++) {
        if ((UINT_PTR)rgUIManifest[i].pszType != 5) continue;
        printf("Create dialog %lu\n", (unsigned long)(UINT_PTR)rgUIManifest[i].pszName);
        hdlg = CreateDialogParam(NULL, rgUIManifest[i].pszName, hwnd, NULL, 0);
        CHECK(hdlg); cDialogs++; CHECK(DestroyWindow(hdlg));
    }
    CHECK(cDialogs == 38);
    hdlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, NULL, 0); CHECK(hdlg);
    hchild = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN,
                          10, 70, 100, 80, hdlg, (HMENU)999, hinst, NULL); CHECK(hchild);
    SetFocus(GetWindow(hchild, GW_CHILD));
    msg.hwnd = GetFocus(); msg.message = WM_KEYDOWN; msg.wParam = VK_TAB;
    rgKeys[VK_SHIFT] = 0x80; CHECK(SetKeyboardState(rgKeys));
    CHECK(IsDialogMessageA(hdlg, &msg) && GetFocus() == GetDlgItem(hdlg, IDC_HELP));
    rgKeys[VK_SHIFT] = 0; CHECK(SetKeyboardState(rgKeys)); CHECK(DestroyWindow(hdlg));
    Drain();
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    // Real SDL events arriving on the popup route through capture to the combo.
    CHECK(FComboButtonPainted(hcb, FALSE));
    SetFocus(hcb); SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0);
    CHECK(hcb->lpcombo->hwndPopup && GetCapture() == hcb);
    CHECK(FComboButtonPainted(hcb, TRUE));
    CHECK(FPopupTextPainted(hcb->lpcombo->hwndPopup, (int)SendMessageA(hcb, CB_GETITEMHEIGHT, 0, 0)));
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0) {
        CHECK(SDL_GetWindowFlags(hcb->lpcombo->hwndPopup->lpWindow.get()) & SDL_WINDOW_POPUP_MENU);
        CHECK(SDL_GetWindowParent(hcb->lpcombo->hwndPopup->lpWindow.get()) == hwnd->lpWindow.get());
    }
    // X11 maps a shown popup asynchronously. Route synthetic mouse input only
    // after the window manager has established its native screen coordinates.
    CHECK(SDL_SyncWindow(hcb->lpcombo->hwndPopup->lpWindow.get()));
    msPopup = SDL_GetTicks();
    do {
        SDL_PumpEvents(); CHECK(GetWindowRect(hcb->lpcombo->hwndPopup, &rc));
        // This fixture's popup is away from the origin. The X11 backend reports
        // zero until its initial ConfigureNotify arrives after mapping.
        if (strcmp(SDL_GetCurrentVideoDriver(), "x11") != 0 || rc.left != 0 || rc.top != 0) break;
        SDL_Delay(1);
    } while (SDL_GetTicks() - msPopup < 1000);
    CHECK(strcmp(SDL_GetCurrentVideoDriver(), "x11") != 0 || rc.left != 0 || rc.top != 0);
    CHECK(GetWindowRect(hcb, &rcField) && rc.left == rcField.left && rc.top == rcField.bottom);
    Drain();
    CHECK(hcb->lpcombo->hwndPopup && GetCapture() == hcb);
    CHECK(GetFocus() == hcb && hcb->lpcombo->hwndPopup->lpCanvas.get());
    cBefore = cSel;
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hcb->lpcombo->hwndPopup->id;
    event.motion.x = 4; event.motion.y = 4;
    CompatRouteEvent(&event); Drain();
    CHECK(hcb->lpcombo->iHot == 0 && SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1 && cSel == cBefore);
    SendMessageA(hcb, WM_KEYDOWN, VK_ESCAPE, 0);
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1 && cSel == cBefore);
    CHECK(FComboButtonPainted(hcb, FALSE));
    SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0);
    SendMessageA(hcb, WM_KEYDOWN, VK_HOME, 0);
    SendMessageA(hcb, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(-1, -1));
    CHECK(!SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0) && SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1 && cSel == cBefore);
    SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0);
    SendMessageA(hcb, WM_KEYDOWN, VK_HOME, 0);
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 1 && cSel == cBefore);
    SendMessageA(hcb, WM_KEYDOWN, VK_RETURN, 0);
    CHECK(SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 0 && cSel == cBefore + 1);
    SendMessageA(hcb, CB_SETCURSEL, 1, 0);
    SendMessageA(hcb, CB_SHOWDROPDOWN, TRUE, 0);
    cBefore = cSel;
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hcb->lpcombo->hwndPopup->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.x = 4; event.button.y = 4;
    CompatRouteEvent(&event); Drain(); CHECK(!SendMessageA(hcb, CB_GETDROPPEDSTATE, 0, 0) && SendMessageA(hcb, CB_GETCURSEL, 0, 0) == 0 && cSel == cBefore + 1);
    SendMessageA(howner, CB_ADDSTRING, 0, (LPARAM)"Scout");
    SetFocus(howner); SendMessageA(howner, CB_SHOWDROPDOWN, TRUE, 0); CHECK(howner->lpcombo->hwndPopup);
    InvalidateRect(howner->lpcombo->hwndPopup, NULL, TRUE); UpdateWindow(howner->lpcombo->hwndPopup);
    CHECK(!GetUpdateRect(howner->lpcombo->hwndPopup, NULL, FALSE));
    CHECK(SendMessageA(howner, CB_GETCOUNT, 0, 0) > 0);
    CHECK(SendMessageA(howner, CB_GETLBTEXTLEN, 0, 0) >= 0);
    CHECK(SendMessageA(howner, CB_GETITEMDATA, 0, 0) != CB_ERR);
    CHECK(!GetUpdateRect(howner->lpcombo->hwndPopup, NULL, FALSE));
    fResetDraw = TRUE; InvalidateRect(howner->lpcombo->hwndPopup, NULL, TRUE); UpdateWindow(howner->lpcombo->hwndPopup);
    CHECK(SendMessageA(howner, CB_GETCOUNT, 0, 0) == 0 && cErrors == 0); SendMessageA(howner, CB_SHOWDROPDOWN, FALSE, 0);
    SendMessageA(howner, CB_ADDSTRING, 0, (LPARAM)"Scout"); SendMessageA(howner, CB_SETCURSEL, 0, 0);
    fDestroyDraw = TRUE; SendMessageA(howner, CB_SHOWDROPDOWN, TRUE, 0);
    CHECK(!IsWindow(howner) && GetCapture() != howner);
#endif
    fDestroyDrop = TRUE; SendMessageA(hother, CB_SHOWDROPDOWN, TRUE, 0); fDestroyDrop = FALSE;
    CHECK(!IsWindow(hother));
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Combo data, interaction and all 34 dialog templates passed"); return 0;
}
