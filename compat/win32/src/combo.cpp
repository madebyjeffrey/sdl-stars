/* Editable and fixed-height string combos, with independent owned popup windows. */
#include "internal.h"
#include "cleanup.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static BOOL FCombo(HWND hwnd, COMPATCOMBO *lp) { return win32::IsWindow(hwnd) && hwnd->lpcombo.get() == lp; }
static void SelectCombo(HWND hwnd, int i, BOOL fUser);
static void HighlightCombo(HWND hwnd, int i) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    int iTop, cVisible, cyItem;
    if (i < 0 || i >= CompatListProc(hwnd, LB_GETCOUNT, 0, 0) || i == lp->iHot) return;
    lp->iHot = i;
    cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
    cVisible = (((1) > ((lp->cyDrop - 2) / StarsMax(1, cyItem))) ? (1) : ((lp->cyDrop - 2) / StarsMax(1, cyItem)));
    iTop = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0);
    if (i < iTop) CompatListProc(hwnd, LB_SETTOPINDEX, i, 0);
    else if (i >= iTop + cVisible) CompatListProc(hwnd, LB_SETTOPINDEX, i - cVisible + 1, 0);
    if (lp->hwndPopup) win32::InvalidateRect(lp->hwndPopup, NULL, TRUE);
}
static BOOL FOwnerCombo(HWND hwnd) { return (hwnd->dwStyle & CBS_OWNERDRAWFIXED) != 0; }
static void NotifyCombo(HWND hwnd, UINT code) {
    if (win32::IsWindow(hwnd) && win32::IsWindow(hwnd->hwndParent))
        win32::SendMessageA(hwnd->hwndParent, WM_COMMAND, MAKEWPARAM(hwnd->idChild, code), (LPARAM)hwnd);
}
static char *PszComboItem(HWND hwnd, int i) {
    LRESULT cch = CompatListProc(hwnd, LB_GETTEXTLEN, i, 0);
    char *psz;
    if (cch < 0 || (UINT_PTR)cch >= SIZE_MAX || !(psz = (char *)(SDL_malloc((size_t)cch + 1)))) return NULL;
    if (CompatListProc(hwnd, LB_GETTEXT, i, (LPARAM)psz) < 0) { SDL_free(psz); return NULL; }
    return psz;
}
static void SyncCombo(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    char *psz;
    int i = (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
    if (lp->hwndEdit && !lp->fSync) {
        StarsRestore<BOOL> restoreSync(lp->fSync);
        psz = PszComboItem(hwnd, i); lp->fSync = TRUE;
        win32::SetWindowTextA(lp->hwndEdit, psz ? psz : ""); SDL_free(psz);
        if (!FCombo(hwnd, lp)) return;
        if (i >= 0) win32::SendMessageA(lp->hwndEdit, EM_SETSEL, 0, -1);
        if (!FCombo(hwnd, lp)) return;
    }
    win32::InvalidateRect(hwnd, NULL, TRUE);
    if (lp->hwndPopup) win32::InvalidateRect(lp->hwndPopup, NULL, TRUE);
}
static void CloseCombo(HWND hwnd, BOOL fAccept, BOOL fNotify) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    HWND hwndPopup, hwndCapture;
    if (!lp || !lp->fDropped || lp->fClosing) return;
    // CBN_SELCHANGE can move focus to the frame. Its WM_KILLFOCUS must not
    // reenter cancellation and restore the old row while accepting this one.
    {
        StarsRestore<BOOL> restoreClosing(lp->fClosing);
        lp->fClosing = TRUE;
        if (fAccept && lp->iHot >= 0) {
            SelectCombo(hwnd, lp->iHot, TRUE);
            if (!FCombo(hwnd, lp) || !lp->fDropped) return;
        }
        lp->fDropped = FALSE; hwndPopup = lp->hwndPopup; lp->hwndPopup = NULL;
        hwndCapture = lp->hwndCapture; lp->hwndCapture = NULL;
        if (!fAccept && !hwnd->fDestroying) {
            CompatListProc(hwnd, LB_SETCURSEL, lp->iSave, 0); SyncCombo(hwnd);
        }
        if (win32::GetCapture() == hwnd) { win32::ReleaseCapture(); if (!win32::GetCapture() && win32::IsWindow(hwndCapture)) win32::SetCapture(hwndCapture); }
        if (win32::IsWindow(hwndPopup)) win32::DestroyWindow(hwndPopup);
        if (!FCombo(hwnd, lp)) return;
        win32::InvalidateRect(hwnd, NULL, TRUE);
    }
    if (fNotify) {
        NotifyCombo(hwnd, fAccept ? CBN_SELENDOK : CBN_SELENDCANCEL);
        if (FCombo(hwnd, lp)) NotifyCombo(hwnd, CBN_CLOSEUP);
    }
}
static BOOL FComboDropRect(HWND hwnd, RECT *lprc) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    RECT rc;
    int cyItem, c, cxScreen, cyScreen, cy;
    if (!win32::GetWindowRect(hwnd, &rc)) return FALSE;
    c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0); cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
    cy = (int)((((int64_t)lp->cyDrop) < ((int64_t)StarsMax(1, c) * cyItem + 2)) ? ((int64_t)lp->cyDrop) : ((int64_t)StarsMax(1, c) * cyItem + 2));
    cxScreen = win32::GetSystemMetrics(SM_CXSCREEN); cyScreen = win32::GetSystemMetrics(SM_CYSCREEN);
    lprc->left = rc.left; lprc->right = rc.left + StarsMax(rc.right - rc.left, lp->cxDrop);
    if (cxScreen > 0 && lprc->right > cxScreen) win32::OffsetRect(lprc, cxScreen - lprc->right, 0);
    lprc->top = cyScreen > 0 && (int64_t)rc.bottom + cy > cyScreen ? rc.top - cy : rc.bottom;
    lprc->bottom = lprc->top + cy;
    return TRUE;
}
static void OpenCombo(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    RECT rc;
    HWND hwndPopup;
    if (FStarsErrorPending() || !lp || lp->fDropped || lp->fOpening || lp->fClosing || hwnd->fDestroying || !win32::IsWindowEnabled(hwnd)) return;
    {
        StarsRestore<BOOL> restoreOpening(lp->fOpening);
        lp->fOpening = TRUE; NotifyCombo(hwnd, CBN_DROPDOWN);
        if (!FCombo(hwnd, lp) || FStarsErrorPending()) return;
    }
    if (!FComboDropRect(hwnd, &rc)) return;
    lp->iSave = (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
    lp->iHot = lp->iSave;
    hwndPopup = win32::CreateWindowExA(WS_EX_NOACTIVATE, "StarsComboPopup", "", WS_POPUP,
                               rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                               HwndCompatRoot(hwnd), NULL, hwnd->lpclass->wc.hInstance, hwnd);
    if (!hwndPopup || !FCombo(hwnd, lp) || FStarsErrorPending()) { if (win32::IsWindow(hwndPopup)) win32::DestroyWindow(hwndPopup); return; }
    lp->hwndPopup = hwndPopup; lp->fDropped = TRUE; lp->hwndCapture = win32::GetCapture();
    COMPATCLEANUPSCOPE closeFailedPopup([&] {
        if (FCombo(hwnd, lp) && lp->hwndPopup == hwndPopup) CloseCombo(hwnd, FALSE, FALSE);
    }, hwnd);
    win32::InvalidateRect(hwnd, NULL, TRUE);
    win32::SetCapture(hwnd);
    if (!FCombo(hwnd, lp) || !lp->fDropped || FStarsErrorPending()) return;
    // Populate the hidden surface before the compositor maps the list. Owner
    // drawing can destroy or close the combo, so revalidate after painting.
    win32::InvalidateRect(hwndPopup, NULL, TRUE); win32::UpdateWindow(hwndPopup);
    if (!FCombo(hwnd, lp) || !lp->fDropped || lp->hwndPopup != hwndPopup || FStarsErrorPending()) return;
    if (!win32::SetWindowPos(hwndPopup, HWND_TOP, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW) || FStarsErrorPending()) return;
    closeFailedPopup.Release();
}
static void SelectCombo(HWND hwnd, int i, BOOL fUser) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    int iOld = (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
    CompatListProc(hwnd, LB_SETCURSEL, i, 0); SyncCombo(hwnd);
    if (!FCombo(hwnd, lp) || !fUser || i == iOld || i < 0) return;
    if (!lp->fDropped) NotifyCombo(hwnd, CBN_SELENDOK);
    if (FCombo(hwnd, lp)) NotifyCombo(hwnd, CBN_SELCHANGE);
}
static void LayoutCombo(HWND hwnd, BOOL fRequested) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    RECT rc;
    POINT pt;
    int cy;
    if (lp->fSizing || !win32::GetWindowRect(hwnd, &rc)) return;
    cy = lp->cyField + 6;
    if (fRequested && rc.bottom - rc.top > cy) lp->cyDrop = StarsMax(3 * cy, rc.bottom - rc.top - cy);
    pt = POINT{rc.left, rc.top};
    if (hwnd->hwndParent) win32::ScreenToClient(hwnd->hwndParent, &pt);
    {
        StarsRestore<BOOL> restoreSizing(lp->fSizing);
        lp->fSizing = TRUE;
        if (rc.bottom - rc.top != cy) win32::MoveWindow(hwnd, pt.x, pt.y, rc.right - rc.left, cy, TRUE);
        if (!FCombo(hwnd, lp)) return;
    }
    if (lp->hwndEdit) win32::MoveWindow(lp->hwndEdit, 2, 2, StarsMax(0, rc.right - rc.left - 22), StarsMax(1, cy - 4), TRUE);
}
static void ComboFont(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    HDC hdc = win32::CreateCompatibleDC(NULL);
    HGDIOBJ hold;
    TEXTMETRICA tm;
    if (!hdc) return;
    hold = hwnd->hfontControl ? win32::SelectObject(hdc, hwnd->hfontControl) : NULL;
    if (win32::GetTextMetricsA(hdc, &tm) && !FOwnerCombo(hwnd)) {
        lp->cyField = (((1) > (StarsMin(255, tm.tmHeight))) ? (1) : (StarsMin(255, tm.tmHeight))); CompatListProc(hwnd, LB_SETITEMHEIGHT, 0, lp->cyField);
    }
    if (hold) win32::SelectObject(hdc, hold);
    win32::DeleteDC(hdc);
    if (lp->hwndEdit) win32::SendMessageA(lp->hwndEdit, WM_SETFONT, (WPARAM)hwnd->hfontControl, 0);
    if (FCombo(hwnd, lp)) LayoutCombo(hwnd, FALSE);
}
static BOOL DrawComboItem(HWND hwnd, HDC hdc, const RECT *lprc, int i, BOOL fSelection, BOOL fSelected) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    DRAWITEMSTRUCT ds = {};
    RECT rc = *lprc;
    char *psz;
    HBRUSH hbr;
    if (FOwnerCombo(hwnd)) {
        ds.CtlType = ODT_COMBOBOX; ds.CtlID = (UINT)hwnd->idChild; ds.itemID = (UINT)i;
        ds.itemAction = ODA_DRAWENTIRE; ds.hwndItem = hwnd; ds.hDC = hdc; ds.rcItem = rc;
        ds.itemState = (fSelected ? ODS_SELECTED : 0) | (fSelection ? ODS_COMBOBOXEDIT : 0) |
                       (win32::IsWindowEnabled(hwnd) ? 0 : ODS_DISABLED) | (win32::GetFocus() == hwnd && fSelection ? ODS_FOCUS : 0);
        ds.itemData = i >= 0 ? (ULONG_PTR)CompatListProc(hwnd, LB_GETITEMDATA, i, 0) : 0;
        win32::SendMessageA(hwnd->hwndParent, WM_DRAWITEM, (WPARAM)hwnd->idChild, (LPARAM)&ds);
        return FCombo(hwnd, lp);
    }
    hbr = win32::CreateSolidBrush(win32::GetSysColor(fSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    win32::SetTextColor(hdc, win32::GetSysColor(!win32::IsWindowEnabled(hwnd) ? COLOR_GRAYTEXT : fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
    win32::SetBkMode(hdc, TRANSPARENT); rc.left += 2;
    psz = PszComboItem(hwnd, i);
    if (psz) { win32::DrawTextA(hdc, psz, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX); SDL_free(psz); }
    return TRUE;
}
static void PaintCombo(HWND hwnd, HWND hwndPaint, BOOL fPopup) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    PAINTSTRUCT ps;
    HDC hdc = win32::BeginPaint(hwndPaint, &ps);
    HGDIOBJ hold = NULL;
    HBRUSH hbr;
    RECT rc, item, arrow;
    int i, iSel, iTop, cyItem, c, y, j;
    unsigned n;
    if (!hdc) return;
    win32::GetClientRect(hwndPaint, &rc);
    if (hwnd->hfontControl) hold = win32::SelectObject(hdc, hwnd->hfontControl);
    hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_WINDOW));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    iSel = fPopup ? lp->iHot : (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
    if (fPopup) {
        iTop = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0);
        cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0); c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0);
        n = NCompatListMutation(hwnd);
        for (i = iTop, y = 1; i < c && y < rc.bottom - 1; i++, y += cyItem) {
            item = RECT{1, y, rc.right - 1, StarsMin(rc.bottom - 1, y + cyItem)};
            if (!DrawComboItem(hwnd, hdc, &item, i, FALSE, i == iSel) || !win32::IsWindow(hwndPaint)) return;
            if (NCompatListMutation(hwnd) != n) { win32::InvalidateRect(hwndPaint, NULL, TRUE); break; }
        }
    } else {
        // Keep the complete raised button inside the field's two-pixel sunken
        // frame; painting on that frame would overwrite its top/right bevel.
        arrow = RECT{StarsMax(2, rc.right - 19), 2, rc.right - 2, rc.bottom - 2};
        item = RECT{2, 2, arrow.left, rc.bottom - 2};
        if (!lp->hwndEdit && !DrawComboItem(hwnd, hdc, &item, iSel, TRUE, win32::GetFocus() == hwnd)) return;
        hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNFACE));
        if (hbr) { win32::FillRect(hdc, &arrow, hbr); win32::DeleteObject(hbr); }
        CompatDrawClassicEdge(hdc, &arrow, lp->fDropped, TRUE);
        for (j = 0; j < 4; j++) {
            item = RECT{arrow.left + 5 + j, (rc.bottom - 4) / 2 + j, arrow.right - 5 - j, (rc.bottom - 4) / 2 + j + 1};
            if (lp->fDropped) win32::OffsetRect(&item, 1, 1);
            win32::FillRect(hdc, &item, (HBRUSH)win32::GetStockObject(win32::IsWindowEnabled(hwnd) ? BLACK_BRUSH : GRAY_BRUSH));
        }
    }
    CompatDrawClassicEdge(hdc, &rc, !fPopup, TRUE);
    if (hold) win32::SelectObject(hdc, hold);
    win32::EndPaint(hwndPaint, &ps);
}
LRESULT CALLBACK CompatComboPopupProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndCombo = (HWND)win32::GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE) { win32::SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTA *)l)->lpCreateParams); return TRUE; }
    if (msg == WM_CREATE) return 0;
    if (msg == WM_PAINT && win32::IsWindow(hwndCombo) && hwndCombo->lpcombo) { PaintCombo(hwndCombo, hwnd, TRUE); return 0; }
    if (msg == WM_ERASEBKGND) return TRUE;
    if (msg == WM_NCDESTROY && win32::IsWindow(hwndCombo) && hwndCombo->lpcombo && hwndCombo->lpcombo->hwndPopup == hwnd) {
        hwndCombo->lpcombo->hwndPopup = NULL; CloseCombo(hwndCombo, FALSE, !hwndCombo->fDestroying);
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
static LRESULT CALLBACK ComboEditProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndCombo = win32::GetParent(hwnd);
    auto lpcomboKeep = win32::IsWindow(hwndCombo) ? hwndCombo->lpcombo : nullptr;
    COMPATCOMBO *lp = lpcomboKeep.get();
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    if (msg == WM_GETDLGCODE && lp->fDropped) return DLGC_WANTALLKEYS | DLGC_WANTCHARS;
    if (((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && (w == VK_UP || w == VK_DOWN || w == VK_F4 ||
         (lp->fDropped && (w == VK_RETURN || w == VK_ESCAPE)))) || msg == WM_MOUSEWHEEL)
        return win32::SendMessageA(hwndCombo, msg, w, l);
    return win32::CallWindowProcA(lp->lpfnEdit, hwnd, msg, w, l);
}
static UINT ListMessage(UINT msg) {
    switch (msg) {
    case CB_ADDSTRING: return LB_ADDSTRING;
    case CB_INSERTSTRING: return LB_INSERTSTRING;
    case CB_DELETESTRING: return LB_DELETESTRING;
    case CB_GETCOUNT: return LB_GETCOUNT;
    case CB_GETCURSEL: return LB_GETCURSEL;
    case CB_GETLBTEXT: return LB_GETTEXT;
    case CB_GETLBTEXTLEN: return LB_GETTEXTLEN;
    case CB_RESETCONTENT: return LB_RESETCONTENT;
    case CB_SETCURSEL: return LB_SETCURSEL;
    case CB_GETITEMDATA: return LB_GETITEMDATA;
    case CB_SETITEMDATA: return LB_SETITEMDATA;
    case CB_FINDSTRING: return LB_FINDSTRING;
    case CB_FINDSTRINGEXACT: return LB_FINDSTRINGEXACT;
    default: return 0;
    }
}
LRESULT CALLBACK CompatComboProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lpwin = LpCompatWindow(hwnd);
    std::shared_ptr<COMPATCOMBO> lpcomboKeep;
    COMPATCOMBO *lp;
    UINT mapped;
    int i, c, cyItem, y, iTop;
    POINT pt;
    RECT rc;
    LRESULT result;
    LONG_PTR fn;
    WNDPROC lpfn;
    char *psz;
    MEASUREITEMSTRUCT ms = {};
    HWND hwndEdit;
    if (!lpwin) return 0;
    lpcomboKeep = lpwin->lpcombo;
    lp = lpcomboKeep.get();
    if (msg == WM_NCCREATE) {
        if (!CompatListProc(hwnd, msg, w, l)) return FALSE;
        try { lpcomboKeep = std::make_shared<COMPATCOMBO>(); }
        catch (const std::bad_alloc &) { CompatListProc(hwnd, WM_NCDESTROY, 0, 0); return FALSE; }
        lp = lpcomboKeep.get();
        lp->cyField = 16; lp->cyDrop = 100; lp->iSave = -1;
        lpwin->lpcombo = lpcomboKeep; lpwin->iControl = 6; return TRUE;
    }
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    mapped = ListMessage(msg);
    if (mapped) {
        if (msg == CB_SETITEMDATA && (INT_PTR)w < 0) return CB_ERR;
        i = (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
        result = CompatListProc(hwnd, mapped, w, l);
        if (!FCombo(hwnd, lp)) return result;
        if (msg == CB_RESETCONTENT || msg == CB_SETCURSEL ||
            (i >= 0 && (msg == CB_ADDSTRING || msg == CB_INSERTSTRING || msg == CB_DELETESTRING))) SyncCombo(hwnd);
        /* Owner-draw callbacks query item text while painting. Invalidating on
         * getters perpetually repaints the popup and starves WM_TIMER. */
        if (FCombo(hwnd, lp) && lp->hwndPopup &&
            (msg == CB_ADDSTRING || msg == CB_INSERTSTRING || msg == CB_DELETESTRING ||
             msg == CB_RESETCONTENT || msg == CB_SETCURSEL || msg == CB_SETITEMDATA))
            win32::InvalidateRect(lp->hwndPopup, NULL, TRUE);
        return result;
    }
    switch (msg) {
    case WM_CREATE:
        if ((lpwin->dwStyle & 3) < CBS_DROPDOWN || (lpwin->dwStyle & CBS_OWNERDRAWVARIABLE) ||
            (FOwnerCombo(hwnd) && !(lpwin->dwStyle & CBS_HASSTRINGS))) return -1;
        ComboFont(hwnd);
        if (FOwnerCombo(hwnd)) {
            ms.CtlType = ODT_COMBOBOX; ms.CtlID = (UINT)lpwin->idChild; ms.itemHeight = lp->cyField;
            win32::SendMessageA(lpwin->hwndParent, WM_MEASUREITEM, lpwin->idChild, (LPARAM)&ms);
            if (!FCombo(hwnd, lp)) return -1;
            if (ms.itemHeight > 0 && ms.itemHeight <= 255) { lp->cyField = (int)ms.itemHeight; CompatListProc(hwnd, LB_SETITEMHEIGHT, 0, ms.itemHeight); }
        }
        lp->cyDrop = (((3 * (lp->cyField + 6)) > (((CREATESTRUCTA *)l)->cy - lp->cyField - 6)) ? (3 * (lp->cyField + 6)) : (((CREATESTRUCTA *)l)->cy - lp->cyField - 6));
        LayoutCombo(hwnd, TRUE);
        if ((lpwin->dwStyle & 3) == CBS_DROPDOWN) {
            win32::GetClientRect(hwnd, &rc);
            hwndEdit = win32::CreateWindowExA(0, "Edit", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                          2, 2, StarsMax(0, rc.right - 22), StarsMax(1, rc.bottom - 4), hwnd, (HMENU)1, lpwin->lpclass->wc.hInstance, NULL);
            if (!hwndEdit || !FCombo(hwnd, lp)) return -1;
            lp->hwndEdit = hwndEdit;
            fn = win32::GetWindowLongPtrA(lp->hwndEdit, GWLP_WNDPROC); memcpy(&lp->lpfnEdit, &fn, sizeof(fn));
            lpfn = ComboEditProc; memcpy(&fn, &lpfn, sizeof(fn)); win32::SetWindowLongPtrA(lp->hwndEdit, GWLP_WNDPROC, fn);
        }
        return 0;
    case WM_DESTROY: CloseCombo(hwnd, FALSE, FALSE); return CompatListProc(hwnd, WM_DESTROY, w, l);
    case WM_NCDESTROY:
        CompatListProc(hwnd, WM_NCDESTROY, w, l); lpwin->lpcombo.reset(); return 0;
    case WM_GETFONT: return (LRESULT)lpwin->hfontControl;
    case WM_SETFONT: lpwin->hfontControl = (HFONT)w; ComboFont(hwnd); if (l && win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_SIZE: LayoutCombo(hwnd, TRUE); return 0;
    case WM_PAINT: PaintCombo(hwnd, hwnd, FALSE); return 0;
    case WM_ERASEBKGND: return TRUE;
    case WM_GETDLGCODE: return DLGC_WANTARROWS | DLGC_WANTCHARS | (lp->fDropped ? DLGC_WANTALLKEYS : 0);
    case WM_GETTEXT: case WM_GETTEXTLENGTH:
        if (lp->hwndEdit) return win32::SendMessageA(lp->hwndEdit, msg, w, l);
        psz = PszComboItem(hwnd, (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0));
        result = psz ? (LRESULT)SDL_strlen(psz) : 0;
        if (msg == WM_GETTEXT) { result = w && l && psz ? (LRESULT)CchUtf8Prefix(psz, (size_t)w - 1) : 0; if (w && l) { if (result) memcpy((char *)l, psz, (size_t)result); ((char *)l)[result] = 0; } }
        SDL_free(psz); return result;
    case WM_SETTEXT: {
        if (!lp->hwndEdit) return FALSE;
        StarsRestore<BOOL> restoreSync(lp->fSync);
        lp->fSync = TRUE; result = win32::SetWindowTextA(lp->hwndEdit, (LPCSTR)l);
        return result;
    }
    case WM_COMMAND:
        if ((HWND)l != lp->hwndEdit) return 0;
        if (HIWORD(w) == EN_SETFOCUS) NotifyCombo(hwnd, CBN_SETFOCUS);
        else if (HIWORD(w) == EN_KILLFOCUS) { CloseCombo(hwnd, FALSE, TRUE); if (FCombo(hwnd, lp)) NotifyCombo(hwnd, CBN_KILLFOCUS); }
        else if (!lp->fSync && (HIWORD(w) == EN_UPDATE || HIWORD(w) == EN_CHANGE)) {
            CompatListProc(hwnd, LB_SETCURSEL, -1, 0); NotifyCombo(hwnd, HIWORD(w) == EN_UPDATE ? CBN_EDITUPDATE : CBN_EDITCHANGE);
        }
        return 0;
    case WM_SETFOCUS:
        if (lp->hwndEdit) win32::SetFocus(lp->hwndEdit); else NotifyCombo(hwnd, CBN_SETFOCUS);
        if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_KILLFOCUS:
        if ((HWND)w != lp->hwndEdit) { CloseCombo(hwnd, FALSE, TRUE); if (FCombo(hwnd, lp)) NotifyCombo(hwnd, CBN_KILLFOCUS); }
        if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_ENABLE: if (!w) CloseCombo(hwnd, FALSE, TRUE); if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    case CB_SELECTSTRING:
        i = (int)CompatListProc(hwnd, LB_FINDSTRING, w, l); if (i >= 0) SelectCombo(hwnd, i, FALSE); return i;
    case CB_GETITEMHEIGHT: return (INT_PTR)w == -1 ? lp->cyField : CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
    case CB_SETITEMHEIGHT:
        if (l <= 0 || l > 255 || ((INT_PTR)w != -1 && w != 0)) return CB_ERR;
        if ((INT_PTR)w == -1) { lp->cyField = (int)l; LayoutCombo(hwnd, FALSE); return CB_OKAY; }
        return CompatListProc(hwnd, LB_SETITEMHEIGHT, 0, l);
    case CB_SETEXTENDEDUI: lp->fExtended = w != 0; return CB_OKAY;
    case CB_GETEXTENDEDUI: return lp->fExtended;
    case CB_GETDROPPEDSTATE: return lp->fDropped;
    case CB_SHOWDROPDOWN: if (w) OpenCombo(hwnd); else CloseCombo(hwnd, FALSE, TRUE); return TRUE;
    case CB_GETDROPPEDWIDTH: win32::GetClientRect(hwnd, &rc); return StarsMax(rc.right, lp->cxDrop);
    case CB_SETDROPPEDWIDTH: if ((INT_PTR)w < 0 || w > INT_MAX) return CB_ERR; lp->cxDrop = (int)w; return win32::SendMessageA(hwnd, CB_GETDROPPEDWIDTH, 0, 0);
    case CB_GETDROPPEDCONTROLRECT:
        if (!l || !FComboDropRect(hwnd, &rc)) return FALSE;
        win32::GetWindowRect(hwnd, (LPRECT)l);
        ((LPRECT)l)->left = (((((LPRECT)l)->left) < (rc.left)) ? (((LPRECT)l)->left) : (rc.left)); ((LPRECT)l)->top = (((((LPRECT)l)->top) < (rc.top)) ? (((LPRECT)l)->top) : (rc.top));
        ((LPRECT)l)->right = (((((LPRECT)l)->right) > (rc.right)) ? (((LPRECT)l)->right) : (rc.right)); ((LPRECT)l)->bottom = (((((LPRECT)l)->bottom) > (rc.bottom)) ? (((LPRECT)l)->bottom) : (rc.bottom));
        return TRUE;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (!win32::IsWindowEnabled(hwnd)) return 0;
        win32::SetFocus(lp->hwndEdit ? lp->hwndEdit : hwnd);
        if (!FCombo(hwnd, lp)) return 0;
        if (!lp->fDropped) OpenCombo(hwnd);
        else {
            pt = POINT{(SHORT)LOWORD(l), (SHORT)HIWORD(l)}; win32::ClientToScreen(hwnd, &pt);
            if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
                cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
                i = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0) + (pt.y - rc.top - 1) / cyItem;
                c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0);
                if (i >= 0 && i < c) HighlightCombo(hwnd, i);
            } else CloseCombo(hwnd, FALSE, TRUE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (!lp->fDropped) return 0;
        pt = POINT{(SHORT)LOWORD(l), (SHORT)HIWORD(l)}; win32::ClientToScreen(hwnd, &pt);
        if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
            cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
            i = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0) + (pt.y - rc.top - 1) / cyItem;
            c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0);
            if (i >= 0 && i < c) HighlightCombo(hwnd, i);
            if (FCombo(hwnd, lp)) CloseCombo(hwnd, TRUE, TRUE);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (!lp->fDropped) return 0;
        pt = POINT{(SHORT)LOWORD(l), (SHORT)HIWORD(l)}; win32::ClientToScreen(hwnd, &pt);
        if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
            cyItem = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
            iTop = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0); i = iTop + (pt.y - rc.top - 1) / cyItem;
            if (i >= 0 && i < CompatListProc(hwnd, LB_GETCOUNT, 0, 0)) HighlightCombo(hwnd, i);
        }
        return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN:
        if (w == VK_F4 && lp->fExtended) return 0;
        if (w == VK_DOWN && lp->fExtended && !lp->fDropped) { OpenCombo(hwnd); return 0; }
        if (w == VK_F4 || ((w == VK_UP || w == VK_DOWN) && (win32::GetKeyState(VK_MENU) & 0x8000))) {
            if (lp->fDropped) CloseCombo(hwnd, TRUE, TRUE); else OpenCombo(hwnd); return 0;
        }
        if (lp->fDropped && (w == VK_RETURN || w == VK_ESCAPE)) { CloseCombo(hwnd, w == VK_RETURN, TRUE); return 0; }
        c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0); i = lp->fDropped ? lp->iHot : (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0);
        if (w == VK_DOWN) i++;
        else if (w == VK_UP) i--;
        else if (w == VK_HOME) i = 0;
        else if (w == VK_END) i = c - 1;
        else return 0;
        if (c > 0) {
            i = (((c - 1) < (StarsMax(0, i))) ? (c - 1) : (StarsMax(0, i)));
            if (lp->fDropped) HighlightCombo(hwnd, i); else SelectCombo(hwnd, i, TRUE);
        }
        return 0;
    case WM_CHAR:
        if (w > 32 && w < 256) { char sz[2] = {(char)w, 0}; i = (int)CompatListProc(hwnd, LB_FINDSTRING, lp->fDropped ? lp->iHot : CompatListProc(hwnd, LB_GETCURSEL, 0, 0), (LPARAM)sz); if (i >= 0) { if (lp->fDropped) HighlightCombo(hwnd, i); else SelectCombo(hwnd, i, TRUE); } }
        return 0;
    case WM_MOUSEWHEEL:
        if (lp->fExtended) return 0;
        y = (SHORT)HIWORD(w) / WHEEL_DELTA;
        if (lp->fDropped) { i = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0); CompatListProc(hwnd, LB_SETTOPINDEX, StarsMax(0, i - y * 3), 0); win32::InvalidateRect(lp->hwndPopup, NULL, TRUE); }
        else { i = (int)CompatListProc(hwnd, LB_GETCURSEL, 0, 0); c = (int)CompatListProc(hwnd, LB_GETCOUNT, 0, 0); if (c > 0) SelectCombo(hwnd, (((c - 1) < (StarsMax(0, i - y))) ? (c - 1) : (StarsMax(0, i - y))), TRUE); }
        return 0;
    case WM_CAPTURECHANGED: case WM_CANCELMODE:
        if (lp->fDropped) CloseCombo(hwnd, FALSE, TRUE);
        return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
