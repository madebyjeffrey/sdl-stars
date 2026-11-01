/* Editable and fixed-height string combos, with independent owned popup windows. */
#include "internal.h"
#include "cleanup.h"
#include <climits>
#include <cstdlib>
#include <cstring>

static BOOL FCombo(HWND hwnd, COMPATCOMBO *lp) { return win32::IsWindow(hwnd) && hwnd->lpcombo.get() == lp; }
static void SelectCombo(HWND hwnd, int i, BOOL fUser);
static void HighlightCombo(HWND hwnd, int i) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    int iTop, cVisible, cyItem;
    if (i < 0 || i >= CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0) || i == lp->iHot) return;
    lp->iHot = i;
    cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0));
    cVisible = (((1) > ((lp->cyDrop - 2) / StarsMax(1, cyItem))) ? (1) : ((lp->cyDrop - 2) / StarsMax(1, cyItem)));
    iTop = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0));
    if (i < iTop) CompatListProc(hwnd, win32::LB_SETTOPINDEX, i, 0);
    else if (i >= iTop + cVisible) CompatListProc(hwnd, win32::LB_SETTOPINDEX, i - cVisible + 1, 0);
    if (lp->hwndPopup) win32::InvalidateRect(lp->hwndPopup, nullptr, 1);
}
static BOOL FOwnerCombo(HWND hwnd) { return (hwnd->dwStyle & win32::CBS_OWNERDRAWFIXED) != 0; }
static void NotifyCombo(HWND hwnd, UINT code) {
    if (win32::IsWindow(hwnd) && win32::IsWindow(hwnd->hwndParent))
        win32::SendMessageA(hwnd->hwndParent, win32::WM_COMMAND, win32::MAKEWPARAM(hwnd->idChild, code), reinterpret_cast<LPARAM>(hwnd));
}
static char *PszComboItem(HWND hwnd, int i) {
    LRESULT cch = CompatListProc(hwnd, win32::LB_GETTEXTLEN, i, 0);
    char *psz;
    if (cch < 0 || static_cast<UINT_PTR>(cch) >= SIZE_MAX || !(psz = static_cast<char *>((SDL_malloc(static_cast<std::size_t>(cch) + 1))))) return nullptr;
    NATIVEOWNER<char, SDL_free> pszOwner(psz);
    if (CompatListProc(hwnd, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(psz)) < 0) return nullptr;
    return pszOwner.release();
}
static void SyncCombo(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    char *psz;
    int i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
    if (lp->hwndEdit && !lp->fSync) {
        StarsRestore<BOOL> restoreSync(lp->fSync);
        psz = PszComboItem(hwnd, i); lp->fSync = 1;
        NATIVEOWNER<char, SDL_free> pszOwner(psz);
        win32::SetWindowTextA(lp->hwndEdit, psz ? psz : ""); pszOwner.reset();
        if (!FCombo(hwnd, lp)) return;
        if (i >= 0) win32::SendMessageA(lp->hwndEdit, win32::EM_SETSEL, 0, -1);
        if (!FCombo(hwnd, lp)) return;
    }
    win32::InvalidateRect(hwnd, nullptr, 1);
    if (lp->hwndPopup) win32::InvalidateRect(lp->hwndPopup, nullptr, 1);
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
        lp->fClosing = 1;
        if (fAccept && lp->iHot >= 0) {
            SelectCombo(hwnd, lp->iHot, 1);
            if (!FCombo(hwnd, lp) || !lp->fDropped) return;
        }
        lp->fDropped = 0; hwndPopup = lp->hwndPopup; lp->hwndPopup = nullptr;
        hwndCapture = lp->hwndCapture; lp->hwndCapture = nullptr;
        if (!fAccept && !hwnd->fDestroying) {
            CompatListProc(hwnd, win32::LB_SETCURSEL, lp->iSave, 0); SyncCombo(hwnd);
        }
        if (win32::GetCapture() == hwnd) { win32::ReleaseCapture(); if (!win32::GetCapture() && win32::IsWindow(hwndCapture)) win32::SetCapture(hwndCapture); }
        if (win32::IsWindow(hwndPopup)) win32::DestroyWindow(hwndPopup);
        if (!FCombo(hwnd, lp)) return;
        win32::InvalidateRect(hwnd, nullptr, 1);
    }
    if (fNotify) {
        NotifyCombo(hwnd, fAccept ? win32::CBN_SELENDOK : win32::CBN_SELENDCANCEL);
        if (FCombo(hwnd, lp)) NotifyCombo(hwnd, win32::CBN_CLOSEUP);
    }
}
static BOOL FComboDropRect(HWND hwnd, RECT *lprc) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    RECT rc;
    int cyItem, c, cxScreen, cyScreen, cy;
    if (!win32::GetWindowRect(hwnd, &rc)) return 0;
    c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0)); cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0));
    cy = static_cast<int>((((static_cast<std::int64_t>(lp->cyDrop)) < (static_cast<std::int64_t>(StarsMax(1, c)) * cyItem + 2)) ? (static_cast<std::int64_t>(lp->cyDrop)) : (static_cast<std::int64_t>(StarsMax(1, c)) * cyItem + 2)));
    cxScreen = win32::GetSystemMetrics(win32::SM_CXSCREEN); cyScreen = win32::GetSystemMetrics(win32::SM_CYSCREEN);
    lprc->left = rc.left; lprc->right = rc.left + StarsMax(rc.right - rc.left, lp->cxDrop);
    if (cxScreen > 0 && lprc->right > cxScreen) win32::OffsetRect(lprc, cxScreen - lprc->right, 0);
    lprc->top = cyScreen > 0 && static_cast<std::int64_t>(rc.bottom) + cy > cyScreen ? rc.top - cy : rc.bottom;
    lprc->bottom = lprc->top + cy;
    return 1;
}
static void OpenCombo(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    RECT rc;
    HWND hwndPopup;
    if (FStarsErrorPending() || !lp || lp->fDropped || lp->fOpening || lp->fClosing || hwnd->fDestroying || !win32::IsWindowEnabled(hwnd)) return;
    {
        StarsRestore<BOOL> restoreOpening(lp->fOpening);
        lp->fOpening = 1; NotifyCombo(hwnd, win32::CBN_DROPDOWN);
        if (!FCombo(hwnd, lp) || FStarsErrorPending()) return;
    }
    if (!FComboDropRect(hwnd, &rc)) return;
    lp->iSave = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
    lp->iHot = lp->iSave;
    hwndPopup = win32::CreateWindowExA(win32::WS_EX_NOACTIVATE, "StarsComboPopup", "", win32::WS_POPUP,
                               rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                               HwndCompatRoot(hwnd), nullptr, hwnd->lpclass->wc.hInstance, hwnd);
    if (!hwndPopup || !FCombo(hwnd, lp) || FStarsErrorPending()) { if (win32::IsWindow(hwndPopup)) win32::DestroyWindow(hwndPopup); return; }
    lp->hwndPopup = hwndPopup; lp->fDropped = 1; lp->hwndCapture = win32::GetCapture();
    COMPATCLEANUPSCOPE closeFailedPopup([&] {
        if (FCombo(hwnd, lp) && lp->hwndPopup == hwndPopup) CloseCombo(hwnd, 0, 0);
    }, hwnd);
    win32::InvalidateRect(hwnd, nullptr, 1);
    win32::SetCapture(hwnd);
    if (!FCombo(hwnd, lp) || !lp->fDropped || FStarsErrorPending()) return;
    // Populate the hidden surface before the compositor maps the list. Owner
    // drawing can destroy or close the combo, so revalidate after painting.
    win32::InvalidateRect(hwndPopup, nullptr, 1); win32::UpdateWindow(hwndPopup);
    if (!FCombo(hwnd, lp) || !lp->fDropped || lp->hwndPopup != hwndPopup || FStarsErrorPending()) return;
    if (!win32::SetWindowPos(hwndPopup, win32::HWND_TOP, 0, 0, 0, 0,
                      win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW) || FStarsErrorPending()) return;
    closeFailedPopup.Release();
}
static void SelectCombo(HWND hwnd, int i, BOOL fUser) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    int iOld = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
    CompatListProc(hwnd, win32::LB_SETCURSEL, i, 0); SyncCombo(hwnd);
    if (!FCombo(hwnd, lp) || !fUser || i == iOld || i < 0) return;
    if (!lp->fDropped) NotifyCombo(hwnd, win32::CBN_SELENDOK);
    if (FCombo(hwnd, lp)) NotifyCombo(hwnd, win32::CBN_SELCHANGE);
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
        lp->fSizing = 1;
        if (rc.bottom - rc.top != cy) win32::MoveWindow(hwnd, pt.x, pt.y, rc.right - rc.left, cy, 1);
        if (!FCombo(hwnd, lp)) return;
    }
    if (lp->hwndEdit) win32::MoveWindow(lp->hwndEdit, 2, 2, StarsMax(0, rc.right - rc.left - 22), StarsMax(1, cy - 4), 1);
}
static void ComboFont(HWND hwnd) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    HDC hdc = win32::CreateCompatibleDC(nullptr);
    HGDIOBJ hold;
    TEXTMETRICA tm;
    if (!hdc) return;
    hold = hwnd->hfontControl ? win32::SelectObject(hdc, hwnd->hfontControl) : nullptr;
    if (win32::GetTextMetricsA(hdc, &tm) && !FOwnerCombo(hwnd)) {
        lp->cyField = (((1) > (StarsMin(255, tm.tmHeight))) ? (1) : (StarsMin(255, tm.tmHeight))); CompatListProc(hwnd, win32::LB_SETITEMHEIGHT, 0, lp->cyField);
    }
    if (hold) win32::SelectObject(hdc, hold);
    win32::DeleteDC(hdc);
    if (lp->hwndEdit) win32::SendMessageA(lp->hwndEdit, win32::WM_SETFONT, reinterpret_cast<WPARAM>(hwnd->hfontControl), 0);
    if (FCombo(hwnd, lp)) LayoutCombo(hwnd, 0);
}
static BOOL DrawComboItem(HWND hwnd, HDC hdc, const RECT *lprc, int i, BOOL fSelection, BOOL fSelected) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    DRAWITEMSTRUCT ds = {};
    RECT rc = *lprc;
    char *psz;
    HBRUSH hbr;
    if (FOwnerCombo(hwnd)) {
        ds.CtlType = win32::ODT_COMBOBOX; ds.CtlID = static_cast<UINT>(hwnd->idChild); ds.itemID = static_cast<UINT>(i);
        ds.itemAction = win32::ODA_DRAWENTIRE; ds.hwndItem = hwnd; ds.hDC = hdc; ds.rcItem = rc;
        ds.itemState = (fSelected ? win32::ODS_SELECTED : 0) | (fSelection ? win32::ODS_COMBOBOXEDIT : 0) |
                       (win32::IsWindowEnabled(hwnd) ? 0 : win32::ODS_DISABLED) | (win32::GetFocus() == hwnd && fSelection ? win32::ODS_FOCUS : 0);
        ds.itemData = i >= 0 ? static_cast<ULONG_PTR>(CompatListProc(hwnd, win32::LB_GETITEMDATA, i, 0)) : 0;
        win32::SendMessageA(hwnd->hwndParent, win32::WM_DRAWITEM, static_cast<WPARAM>(hwnd->idChild), reinterpret_cast<LPARAM>(&ds));
        return FCombo(hwnd, lp);
    }
    hbr = win32::CreateSolidBrush(win32::GetSysColor(fSelected ? win32::COLOR_HIGHLIGHT : win32::COLOR_WINDOW));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    win32::SetTextColor(hdc, win32::GetSysColor(!win32::IsWindowEnabled(hwnd) ? win32::COLOR_GRAYTEXT : fSelected ? win32::COLOR_HIGHLIGHTTEXT : win32::COLOR_WINDOWTEXT));
    win32::SetBkMode(hdc, win32::TRANSPARENT); rc.left += 2;
    psz = PszComboItem(hwnd, i);
    NATIVEOWNER<char, SDL_free> pszOwner(psz);
    if (psz) win32::DrawTextA(hdc, psz, -1, &rc, win32::DT_SINGLELINE | win32::DT_VCENTER | win32::DT_NOPREFIX);
    return 1;
}
static void PaintCombo(HWND hwnd, HWND hwndPaint, BOOL fPopup) {
    auto lpcomboKeep = hwnd->lpcombo;
    COMPATCOMBO *lp = lpcomboKeep.get();
    PAINTSTRUCT ps;
    HDC hdc = win32::BeginPaint(hwndPaint, &ps);
    HGDIOBJ hold = nullptr;
    HBRUSH hbr;
    RECT rc, item, arrow;
    int i, iSel, iTop, cyItem, c, y, j;
    unsigned n;
    if (!hdc) return;
    win32::GetClientRect(hwndPaint, &rc);
    if (hwnd->hfontControl) hold = win32::SelectObject(hdc, hwnd->hfontControl);
    hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_WINDOW));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    iSel = fPopup ? lp->iHot : static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
    if (fPopup) {
        iTop = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0));
        cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0)); c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0));
        n = NCompatListMutation(hwnd);
        for (i = iTop, y = 1; i < c && y < rc.bottom - 1; i++, y += cyItem) {
            item = RECT{1, y, rc.right - 1, StarsMin(rc.bottom - 1, y + cyItem)};
            if (!DrawComboItem(hwnd, hdc, &item, i, 0, i == iSel) || !win32::IsWindow(hwndPaint)) return;
            if (NCompatListMutation(hwnd) != n) { win32::InvalidateRect(hwndPaint, nullptr, 1); break; }
        }
    } else {
        // Keep the complete raised button inside the field's two-pixel sunken
        // frame; painting on that frame would overwrite its top/right bevel.
        arrow = RECT{StarsMax(2, rc.right - 19), 2, rc.right - 2, rc.bottom - 2};
        item = RECT{2, 2, arrow.left, rc.bottom - 2};
        if (!lp->hwndEdit && !DrawComboItem(hwnd, hdc, &item, iSel, 1, win32::GetFocus() == hwnd)) return;
        hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
        if (hbr) { win32::FillRect(hdc, &arrow, hbr); win32::DeleteObject(hbr); }
        CompatDrawClassicEdge(hdc, &arrow, lp->fDropped, 1);
        for (j = 0; j < 4; j++) {
            item = RECT{arrow.left + 5 + j, (rc.bottom - 4) / 2 + j, arrow.right - 5 - j, (rc.bottom - 4) / 2 + j + 1};
            if (lp->fDropped) win32::OffsetRect(&item, 1, 1);
            win32::FillRect(hdc, &item, static_cast<HBRUSH>(win32::GetStockObject(win32::IsWindowEnabled(hwnd) ? win32::BLACK_BRUSH : win32::GRAY_BRUSH)));
        }
    }
    CompatDrawClassicEdge(hdc, &rc, !fPopup, 1);
    if (hold) win32::SelectObject(hdc, hold);
    win32::EndPaint(hwndPaint, &ps);
}
LRESULT  CompatComboPopupProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndCombo = reinterpret_cast<HWND>(win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA));
    if (msg == win32::WM_NCCREATE) { win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, reinterpret_cast<LONG_PTR>((reinterpret_cast<CREATESTRUCTA *>(l))->lpCreateParams)); return 1; }
    if (msg == win32::WM_CREATE) return 0;
    if (msg == win32::WM_PAINT && win32::IsWindow(hwndCombo) && hwndCombo->lpcombo) { PaintCombo(hwndCombo, hwnd, 1); return 0; }
    if (msg == win32::WM_ERASEBKGND) return 1;
    if (msg == win32::WM_NCDESTROY && win32::IsWindow(hwndCombo) && hwndCombo->lpcombo && hwndCombo->lpcombo->hwndPopup == hwnd) {
        hwndCombo->lpcombo->hwndPopup = nullptr; CloseCombo(hwndCombo, 0, !hwndCombo->fDestroying);
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
static LRESULT  ComboEditProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndCombo = win32::GetParent(hwnd);
    auto lpcomboKeep = win32::IsWindow(hwndCombo) ? hwndCombo->lpcombo : nullptr;
    COMPATCOMBO *lp = lpcomboKeep.get();
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    if (msg == win32::WM_GETDLGCODE && lp->fDropped) return win32::DLGC_WANTALLKEYS | win32::DLGC_WANTCHARS;
    if (((msg == win32::WM_KEYDOWN || msg == win32::WM_SYSKEYDOWN) && (w == win32::VK_UP || w == win32::VK_DOWN || w == win32::VK_F4 ||
         (lp->fDropped && (w == win32::VK_RETURN || w == win32::VK_ESCAPE)))) || msg == win32::WM_MOUSEWHEEL)
        return win32::SendMessageA(hwndCombo, msg, w, l);
    return win32::CallWindowProcA(lp->lpfnEdit, hwnd, msg, w, l);
}
static UINT ListMessage(UINT msg) {
    switch (msg) {
    case win32::CB_ADDSTRING: return win32::LB_ADDSTRING;
    case win32::CB_INSERTSTRING: return win32::LB_INSERTSTRING;
    case win32::CB_DELETESTRING: return win32::LB_DELETESTRING;
    case win32::CB_GETCOUNT: return win32::LB_GETCOUNT;
    case win32::CB_GETCURSEL: return win32::LB_GETCURSEL;
    case win32::CB_GETLBTEXT: return win32::LB_GETTEXT;
    case win32::CB_GETLBTEXTLEN: return win32::LB_GETTEXTLEN;
    case win32::CB_RESETCONTENT: return win32::LB_RESETCONTENT;
    case win32::CB_SETCURSEL: return win32::LB_SETCURSEL;
    case win32::CB_GETITEMDATA: return win32::LB_GETITEMDATA;
    case win32::CB_SETITEMDATA: return win32::LB_SETITEMDATA;
    case win32::CB_FINDSTRING: return win32::LB_FINDSTRING;
    case win32::CB_FINDSTRINGEXACT: return win32::LB_FINDSTRINGEXACT;
    default: return 0;
    }
}
LRESULT  CompatComboProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
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
    if (msg == win32::WM_NCCREATE) {
        if (!CompatListProc(hwnd, msg, w, l)) return 0;
        try { lpcomboKeep = std::make_shared<COMPATCOMBO>(); }
        catch (const std::bad_alloc &) { CompatListProc(hwnd, win32::WM_NCDESTROY, 0, 0); return 0; }
        lp = lpcomboKeep.get();
        lp->cyField = 16; lp->cyDrop = 100; lp->iSave = -1;
        lpwin->lpcombo = lpcomboKeep; lpwin->iControl = 6; return 1;
    }
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    mapped = ListMessage(msg);
    if (mapped) {
        if (msg == win32::CB_SETITEMDATA && static_cast<INT_PTR>(w) < 0) return win32::CB_ERR;
        i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
        result = CompatListProc(hwnd, mapped, w, l);
        if (!FCombo(hwnd, lp)) return result;
        if (msg == win32::CB_RESETCONTENT || msg == win32::CB_SETCURSEL ||
            (i >= 0 && (msg == win32::CB_ADDSTRING || msg == win32::CB_INSERTSTRING || msg == win32::CB_DELETESTRING))) SyncCombo(hwnd);
        /* Owner-draw callbacks query item text while painting. Invalidating on
         * getters perpetually repaints the popup and starves WM_TIMER. */
        if (FCombo(hwnd, lp) && lp->hwndPopup &&
            (msg == win32::CB_ADDSTRING || msg == win32::CB_INSERTSTRING || msg == win32::CB_DELETESTRING ||
             msg == win32::CB_RESETCONTENT || msg == win32::CB_SETCURSEL || msg == win32::CB_SETITEMDATA))
            win32::InvalidateRect(lp->hwndPopup, nullptr, 1);
        return result;
    }
    switch (msg) {
    case win32::WM_CREATE:
        if ((lpwin->dwStyle & 3) < win32::CBS_DROPDOWN || (lpwin->dwStyle & win32::CBS_OWNERDRAWVARIABLE) ||
            (FOwnerCombo(hwnd) && !(lpwin->dwStyle & win32::CBS_HASSTRINGS))) return -1;
        ComboFont(hwnd);
        if (FOwnerCombo(hwnd)) {
            ms.CtlType = win32::ODT_COMBOBOX; ms.CtlID = static_cast<UINT>(lpwin->idChild); ms.itemHeight = lp->cyField;
            win32::SendMessageA(lpwin->hwndParent, win32::WM_MEASUREITEM, lpwin->idChild, reinterpret_cast<LPARAM>(&ms));
            if (!FCombo(hwnd, lp)) return -1;
            if (ms.itemHeight > 0 && ms.itemHeight <= 255) { lp->cyField = static_cast<int>(ms.itemHeight); CompatListProc(hwnd, win32::LB_SETITEMHEIGHT, 0, ms.itemHeight); }
        }
        lp->cyDrop = (((3 * (lp->cyField + 6)) > ((reinterpret_cast<CREATESTRUCTA *>(l))->cy - lp->cyField - 6)) ? (3 * (lp->cyField + 6)) : ((reinterpret_cast<CREATESTRUCTA *>(l))->cy - lp->cyField - 6));
        LayoutCombo(hwnd, 1);
        if ((lpwin->dwStyle & 3) == win32::CBS_DROPDOWN) {
            win32::GetClientRect(hwnd, &rc);
            hwndEdit = win32::CreateWindowExA(0, "Edit", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::ES_AUTOHSCROLL,
                                          2, 2, StarsMax(0, rc.right - 22), StarsMax(1, rc.bottom - 4), hwnd, reinterpret_cast<HMENU>(1), lpwin->lpclass->wc.hInstance, nullptr);
            if (!hwndEdit || !FCombo(hwnd, lp)) return -1;
            lp->hwndEdit = hwndEdit;
            fn = win32::GetWindowLongPtrA(lp->hwndEdit, win32::GWLP_WNDPROC); std::memcpy(&lp->lpfnEdit, &fn, sizeof(fn));
            lpfn = ComboEditProc; std::memcpy(&fn, &lpfn, sizeof(fn)); win32::SetWindowLongPtrA(lp->hwndEdit, win32::GWLP_WNDPROC, fn);
        }
        return 0;
    case win32::WM_DESTROY: CloseCombo(hwnd, 0, 0); return CompatListProc(hwnd, win32::WM_DESTROY, w, l);
    case win32::WM_NCDESTROY:
        CompatListProc(hwnd, win32::WM_NCDESTROY, w, l); lpwin->lpcombo.reset(); return 0;
    case win32::WM_GETFONT: return reinterpret_cast<LRESULT>(lpwin->hfontControl);
    case win32::WM_SETFONT: lpwin->hfontControl = reinterpret_cast<HFONT>(w); ComboFont(hwnd); if (l && win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_SIZE: LayoutCombo(hwnd, 1); return 0;
    case win32::WM_PAINT: PaintCombo(hwnd, hwnd, 0); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::WM_GETDLGCODE: return win32::DLGC_WANTARROWS | win32::DLGC_WANTCHARS | (lp->fDropped ? win32::DLGC_WANTALLKEYS : 0);
    case win32::WM_GETTEXT: case win32::WM_GETTEXTLENGTH: {
        if (lp->hwndEdit) return win32::SendMessageA(lp->hwndEdit, msg, w, l);
        psz = PszComboItem(hwnd, static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0)));
        NATIVEOWNER<char, SDL_free> pszOwner(psz);
        result = psz ? static_cast<LRESULT>(SDL_strlen(psz)) : 0;
        if (msg == win32::WM_GETTEXT) { result = w && l && psz ? static_cast<LRESULT>(CchUtf8Prefix(psz, static_cast<std::size_t>(w) - 1)) : 0; if (w && l) { if (result) std::memcpy(reinterpret_cast<char *>(l), psz, static_cast<std::size_t>(result)); (reinterpret_cast<char *>(l))[result] = 0; } }
        return result;
    }
    case win32::WM_SETTEXT: {
        if (!lp->hwndEdit) return 0;
        StarsRestore<BOOL> restoreSync(lp->fSync);
        lp->fSync = 1; result = win32::SetWindowTextA(lp->hwndEdit, reinterpret_cast<LPCSTR>(l));
        return result;
    }
    case win32::WM_COMMAND:
        if (reinterpret_cast<HWND>(l) != lp->hwndEdit) return 0;
        if (HIWORD(w) == win32::EN_SETFOCUS) NotifyCombo(hwnd, win32::CBN_SETFOCUS);
        else if (HIWORD(w) == win32::EN_KILLFOCUS) { CloseCombo(hwnd, 0, 1); if (FCombo(hwnd, lp)) NotifyCombo(hwnd, win32::CBN_KILLFOCUS); }
        else if (!lp->fSync && (HIWORD(w) == win32::EN_UPDATE || HIWORD(w) == win32::EN_CHANGE)) {
            CompatListProc(hwnd, win32::LB_SETCURSEL, -1, 0); NotifyCombo(hwnd, HIWORD(w) == win32::EN_UPDATE ? win32::CBN_EDITUPDATE : win32::CBN_EDITCHANGE);
        }
        return 0;
    case win32::WM_SETFOCUS:
        if (lp->hwndEdit) win32::SetFocus(lp->hwndEdit); else NotifyCombo(hwnd, win32::CBN_SETFOCUS);
        if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::WM_KILLFOCUS:
        if (reinterpret_cast<HWND>(w) != lp->hwndEdit) { CloseCombo(hwnd, 0, 1); if (FCombo(hwnd, lp)) NotifyCombo(hwnd, win32::CBN_KILLFOCUS); }
        if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::WM_ENABLE: if (!w) CloseCombo(hwnd, 0, 1); if (FCombo(hwnd, lp)) win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::CB_SELECTSTRING:
        i = static_cast<int>(CompatListProc(hwnd, win32::LB_FINDSTRING, w, l)); if (i >= 0) SelectCombo(hwnd, i, 0); return i;
    case win32::CB_GETITEMHEIGHT: return static_cast<INT_PTR>(w) == -1 ? lp->cyField : CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0);
    case win32::CB_SETITEMHEIGHT:
        if (l <= 0 || l > 255 || (static_cast<INT_PTR>(w) != -1 && w != 0)) return win32::CB_ERR;
        if (static_cast<INT_PTR>(w) == -1) { lp->cyField = static_cast<int>(l); LayoutCombo(hwnd, 0); return win32::CB_OKAY; }
        return CompatListProc(hwnd, win32::LB_SETITEMHEIGHT, 0, l);
    case win32::CB_SETEXTENDEDUI: lp->fExtended = w != 0; return win32::CB_OKAY;
    case win32::CB_GETEXTENDEDUI: return lp->fExtended;
    case win32::CB_GETDROPPEDSTATE: return lp->fDropped;
    case win32::CB_SHOWDROPDOWN: if (w) OpenCombo(hwnd); else CloseCombo(hwnd, 0, 1); return 1;
    case win32::CB_GETDROPPEDWIDTH: win32::GetClientRect(hwnd, &rc); return StarsMax(rc.right, lp->cxDrop);
    case win32::CB_SETDROPPEDWIDTH: if (static_cast<INT_PTR>(w) < 0 || w > INT_MAX) return win32::CB_ERR; lp->cxDrop = static_cast<int>(w); return win32::SendMessageA(hwnd, win32::CB_GETDROPPEDWIDTH, 0, 0);
    case win32::CB_GETDROPPEDCONTROLRECT:
        if (!l || !FComboDropRect(hwnd, &rc)) return 0;
        win32::GetWindowRect(hwnd, reinterpret_cast<LPRECT>(l));
        (reinterpret_cast<LPRECT>(l))->left = ((((reinterpret_cast<LPRECT>(l))->left) < (rc.left)) ? ((reinterpret_cast<LPRECT>(l))->left) : (rc.left)); (reinterpret_cast<LPRECT>(l))->top = ((((reinterpret_cast<LPRECT>(l))->top) < (rc.top)) ? ((reinterpret_cast<LPRECT>(l))->top) : (rc.top));
        (reinterpret_cast<LPRECT>(l))->right = ((((reinterpret_cast<LPRECT>(l))->right) > (rc.right)) ? ((reinterpret_cast<LPRECT>(l))->right) : (rc.right)); (reinterpret_cast<LPRECT>(l))->bottom = ((((reinterpret_cast<LPRECT>(l))->bottom) > (rc.bottom)) ? ((reinterpret_cast<LPRECT>(l))->bottom) : (rc.bottom));
        return 1;
    case win32::WM_LBUTTONDOWN: case win32::WM_LBUTTONDBLCLK:
        if (!win32::IsWindowEnabled(hwnd)) return 0;
        win32::SetFocus(lp->hwndEdit ? lp->hwndEdit : hwnd);
        if (!FCombo(hwnd, lp)) return 0;
        if (!lp->fDropped) OpenCombo(hwnd);
        else {
            pt = POINT{static_cast<SHORT>(LOWORD(l)), static_cast<SHORT>(HIWORD(l))}; win32::ClientToScreen(hwnd, &pt);
            if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
                cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0));
                i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0)) + (pt.y - rc.top - 1) / cyItem;
                c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0));
                if (i >= 0 && i < c) HighlightCombo(hwnd, i);
            } else CloseCombo(hwnd, 0, 1);
        }
        return 0;
    case win32::WM_LBUTTONUP:
        if (!lp->fDropped) return 0;
        pt = POINT{static_cast<SHORT>(LOWORD(l)), static_cast<SHORT>(HIWORD(l))}; win32::ClientToScreen(hwnd, &pt);
        if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
            cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0));
            i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0)) + (pt.y - rc.top - 1) / cyItem;
            c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0));
            if (i >= 0 && i < c) HighlightCombo(hwnd, i);
            if (FCombo(hwnd, lp)) CloseCombo(hwnd, 1, 1);
        }
        return 0;
    case win32::WM_MOUSEMOVE:
        if (!lp->fDropped) return 0;
        pt = POINT{static_cast<SHORT>(LOWORD(l)), static_cast<SHORT>(HIWORD(l))}; win32::ClientToScreen(hwnd, &pt);
        if (win32::GetWindowRect(lp->hwndPopup, &rc) && win32::PtInRect(&rc, pt)) {
            cyItem = static_cast<int>(CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0));
            iTop = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0)); i = iTop + (pt.y - rc.top - 1) / cyItem;
            if (i >= 0 && i < CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0)) HighlightCombo(hwnd, i);
        }
        return 0;
    case win32::WM_KEYDOWN: case win32::WM_SYSKEYDOWN:
        if (w == win32::VK_F4 && lp->fExtended) return 0;
        if (w == win32::VK_DOWN && lp->fExtended && !lp->fDropped) { OpenCombo(hwnd); return 0; }
        if (w == win32::VK_F4 || ((w == win32::VK_UP || w == win32::VK_DOWN) && (win32::GetKeyState(win32::VK_MENU) & 0x8000))) {
            if (lp->fDropped) CloseCombo(hwnd, 1, 1); else OpenCombo(hwnd); return 0;
        }
        if (lp->fDropped && (w == win32::VK_RETURN || w == win32::VK_ESCAPE)) { CloseCombo(hwnd, w == win32::VK_RETURN, 1); return 0; }
        c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0)); i = lp->fDropped ? lp->iHot : static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0));
        if (w == win32::VK_DOWN) i++;
        else if (w == win32::VK_UP) i--;
        else if (w == win32::VK_HOME) i = 0;
        else if (w == win32::VK_END) i = c - 1;
        else return 0;
        if (c > 0) {
            i = (((c - 1) < (StarsMax(0, i))) ? (c - 1) : (StarsMax(0, i)));
            if (lp->fDropped) HighlightCombo(hwnd, i); else SelectCombo(hwnd, i, 1);
        }
        return 0;
    case win32::WM_CHAR:
        if (w > 32 && w < 256) { char sz[2] = {static_cast<char>(w), 0}; i = static_cast<int>(CompatListProc(hwnd, win32::LB_FINDSTRING, lp->fDropped ? lp->iHot : CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0), reinterpret_cast<LPARAM>(sz))); if (i >= 0) { if (lp->fDropped) HighlightCombo(hwnd, i); else SelectCombo(hwnd, i, 1); } }
        return 0;
    case win32::WM_MOUSEWHEEL:
        if (lp->fExtended) return 0;
        y = static_cast<SHORT>(HIWORD(w)) / win32::WHEEL_DELTA;
        if (lp->fDropped) { i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0)); CompatListProc(hwnd, win32::LB_SETTOPINDEX, StarsMax(0, i - y * 3), 0); win32::InvalidateRect(lp->hwndPopup, nullptr, 1); }
        else { i = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCURSEL, 0, 0)); c = static_cast<int>(CompatListProc(hwnd, win32::LB_GETCOUNT, 0, 0)); if (c > 0) SelectCombo(hwnd, (((c - 1) < (StarsMax(0, i - y))) ? (c - 1) : (StarsMax(0, i - y))), 1); }
        return 0;
    case win32::WM_CAPTURECHANGED: case win32::WM_CANCELMODE:
        if (lp->fDropped) CloseCombo(hwnd, 0, 1);
        return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
