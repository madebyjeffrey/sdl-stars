/* String list boxes, including the fixed-height owner drawing used by Stars. */
#include "internal.h"
#include "time_io.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct COMPATLISTITEM { char *psz; LPARAM lData; BOOL fSelected; } COMPATLISTITEM;
struct COMPATLIST {
    COMPATLISTITEM *rgitem;
    int cItem, cAlloc, iSel, iCaret, iTop, cyItem, iAnchor, iDrag;
    DWORD grfDrag;
    int cxExtent, xOffset;
    BOOL fScrollSync, fScrollV, fScrollH;
    BYTE *rgSelectedDrag;
    int cSelectedDrag;
    UINT_PTR idDragTimer;
    LPARAM lDragPoint;
    char szSearch[128];
    Uint64 msSearch;
    unsigned nMutation;
    ~COMPATLIST() noexcept {
        for (int i = 0; i < cItem; i++) SDL_free(rgitem[i].psz);
        SDL_free(rgSelectedDrag);
        SDL_free(rgitem);
    }
};
unsigned NCompatListMutation(HWND hwnd) { return hwnd->lplist ? hwnd->lplist->nMutation : 0; }

void CompatDestroyList(HWND hwnd) { hwnd->lplist.reset(); }

static BOOL FMulti(HWND hwnd) { return !hwnd->lpcombo && (hwnd->dwStyle & (win32::LBS_MULTIPLESEL | win32::LBS_EXTENDEDSEL)) != 0; }
static BOOL FOwnerList(HWND hwnd) { return (hwnd->dwStyle & (hwnd->lpcombo ? win32::CBS_OWNERDRAWFIXED : win32::LBS_OWNERDRAWFIXED)) != 0; }
static void NotifyList(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent && (lp->dwStyle & win32::LBS_NOTIFY))
        win32::SendMessageA(lp->hwndParent, win32::WM_COMMAND, win32::MAKEWPARAM(lp->idChild, code), (LPARAM)hwnd);
}
static int CListPage(HWND hwnd) {
    RECT rc;
    win32::GetClientRect(hwnd, &rc);
    return (((1) > ((hwnd->lpcombo ? hwnd->lpcombo->cyDrop : rc.bottom) / hwnd->lplist->cyItem)) ? (1) : ((hwnd->lpcombo ? hwnd->lpcombo->cyDrop : rc.bottom) / hwnd->lplist->cyItem));
}
static void ClampList(HWND hwnd) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 0, 0, 0, 0, 0};
    RECT rc;
    lp->iTop = (((StarsMax(0, lp->iTop)) < ((((0) > (lp->cItem - CListPage(hwnd))) ? (0) : (lp->cItem - CListPage(hwnd))))) ? (StarsMax(0, lp->iTop)) : ((((0) > (lp->cItem - CListPage(hwnd))) ? (0) : (lp->cItem - CListPage(hwnd)))));
    lp->iCaret = (((StarsMax(0, lp->iCaret)) < (StarsMax(0, lp->cItem - 1))) ? (StarsMax(0, lp->iCaret)) : (StarsMax(0, lp->cItem - 1)));
    if (!hwnd->lpcombo && !lp->fScrollSync && !hwnd->fDestroying) {
        StarsRestore<BOOL> restoreSync(lp->fScrollSync);
        lp->fScrollSync = 1;
        if (hwnd->dwStyle & win32::LBS_DISABLENOSCROLL) si.fMask |= win32::SIF_DISABLENOSCROLL;
        si.nMax = StarsMax(0, lp->cItem - 1); si.nPage = (UINT)CListPage(hwnd); si.nPos = lp->iTop;
        if (lp->fScrollV) win32::SetScrollInfo(hwnd, win32::SB_VERT, &si, 1);
        if (!win32::IsWindow(hwnd)) return;
        win32::GetClientRect(hwnd, &rc);
        if (lp->fScrollH) {
            si.nMax = StarsMax(0, lp->cxExtent - 1); si.nPage = (UINT)StarsMax(1, rc.right); si.nPos = lp->xOffset;
            lp->xOffset = win32::SetScrollInfo(hwnd, win32::SB_HORZ, &si, 1);
            if (!win32::IsWindow(hwnd)) return;
        }
        lp->iTop = (((lp->iTop) < ((((0) > (lp->cItem - CListPage(hwnd))) ? (0) : (lp->cItem - CListPage(hwnd))))) ? (lp->iTop) : ((((0) > (lp->cItem - CListPage(hwnd))) ? (0) : (lp->cItem - CListPage(hwnd)))));
        if (lp->fScrollV) win32::SetScrollPos(hwnd, win32::SB_VERT, lp->iTop, 1);
    }
    win32::InvalidateRect(hwnd, NULL, 1);
}
static void RevealList(HWND hwnd, int i) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    int cPage = CListPage(hwnd);
    if (i < lp->iTop) lp->iTop = i;
    else if (i >= lp->iTop + cPage) lp->iTop = i - cPage + 1;
    ClampList(hwnd);
}
static void DeleteNotice(HWND hwnd, UINT i, LPARAM lData) {
    DELETEITEMSTRUCT ds;
    if (!FOwnerList(hwnd) || !win32::IsWindow(hwnd->hwndParent)) return;
    memset(&ds, 0, sizeof(ds)); ds.CtlType = hwnd->lpcombo ? win32::ODT_COMBOBOX : win32::ODT_LISTBOX; ds.CtlID = (UINT)hwnd->idChild;
    ds.itemID = i; ds.hwndItem = hwnd; ds.itemData = (ULONG_PTR)lData;
    win32::SendMessageA(hwnd->hwndParent, win32::WM_DELETEITEM, (WPARAM)hwnd->idChild, (LPARAM)&ds);
}
static void EndListDrag(HWND hwnd, BOOL fRelease);
static void ResetList(HWND hwnd) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    COMPATLISTITEM *rg = lp->rgitem;
    int c = lp->cItem, i;
    auto freeItems = [c](COMPATLISTITEM *rg) noexcept {
        for (int i = 0; i < c; i++) SDL_free(rg[i].psz);
        SDL_free(rg);
    };
    std::unique_ptr<COMPATLISTITEM[], decltype(freeItems)> rgKeep(rg, freeItems);
    lp->rgitem = NULL; lp->cItem = lp->cAlloc = lp->iTop = lp->iCaret = 0; lp->iSel = -1; lp->nMutation++;
    lp->iAnchor = -1; lp->szSearch[0] = 0;
    EndListDrag(hwnd, 1);
    win32::InvalidateRect(hwnd, NULL, 1);
    for (i = 0; i < c; i++) { SDL_free(rg[i].psz); rg[i].psz = NULL; DeleteNotice(hwnd, (UINT)i, rg[i].lData); }
    rgKeep.reset();
    if (win32::IsWindow(hwnd) && !hwnd->fDestroying) ClampList(hwnd);
}
static LRESULT AddList(HWND hwnd, int i, LPCSTR psz) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    COMPATLISTITEM *rg;
    char *pszNew;
    int cAlloc;
    if (!psz || !FUtf8Valid(psz, strlen(psz)) || lp->cItem == INT_MAX) return win32::LB_ERR;
    pszNew = SDL_strdup(psz);
    if (!pszNew) return win32::LB_ERRSPACE;
    if (lp->cItem == lp->cAlloc) {
        cAlloc = lp->cAlloc > (INT_MAX - 16) / 2 ? INT_MAX : lp->cAlloc * 2 + 16;
        if ((size_t)cAlloc > SIZE_MAX / sizeof(*rg)) { SDL_free(pszNew); return win32::LB_ERRSPACE; }
        rg = (COMPATLISTITEM *)(SDL_realloc(lp->rgitem, (size_t)cAlloc * sizeof(*rg)));
        if (!rg) { SDL_free(pszNew); return win32::LB_ERRSPACE; }
        lp->rgitem = rg; lp->cAlloc = cAlloc;
    }
    if (i < 0 || i > lp->cItem) i = lp->cItem;
    memmove(lp->rgitem + i + 1, lp->rgitem + i, (size_t)(lp->cItem - i) * sizeof(*lp->rgitem));
    lp->rgitem[i] = COMPATLISTITEM{pszNew, 0, 0}; lp->cItem++; lp->nMutation++;
    if (lp->iSel >= i) lp->iSel++;
    if (lp->cItem > 1 && lp->iCaret >= i) lp->iCaret++;
    if (lp->iAnchor >= i) lp->iAnchor++;
    ClampList(hwnd);
    return i;
}
static void DrawListSelection(HWND hwnd, int iOld);
static void UserSelectList(HWND hwnd, int i, BOOL fToggle, BOOL fNotify) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    int j, iOld = lp->iSel;
    if (i < 0 || i >= lp->cItem) return;
    lp->iCaret = i;
    if (FMulti(hwnd)) {
        if (!fToggle && !(hwnd->dwStyle & win32::LBS_MULTIPLESEL))
            for (j = 0; j < lp->cItem; j++) lp->rgitem[j].fSelected = 0;
        lp->rgitem[i].fSelected = fToggle ? !lp->rgitem[i].fSelected : 1;
    } else lp->iSel = i;
    RevealList(hwnd, i);
    if (win32::IsWindow(hwnd)) DrawListSelection(hwnd, iOld);
    if (fNotify && win32::IsWindow(hwnd)) NotifyList(hwnd, win32::LBN_SELCHANGE);
}
static void ExtendedSelectList(HWND hwnd, int i, BOOL fCtrl, BOOL fShift, BOOL fNotify) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    int j, iFirst, iLast;
    if (i < 0 || i >= lp->cItem) return;
    if (!(hwnd->dwStyle & win32::LBS_EXTENDEDSEL)) {
        UserSelectList(hwnd, i, FMulti(hwnd), fNotify); return;
    }
    if (fShift) {
        if (lp->iAnchor < 0 || lp->iAnchor >= lp->cItem) lp->iAnchor = lp->iCaret;
        iFirst = StarsMin(lp->iAnchor, i); iLast = StarsMax(lp->iAnchor, i);
        for (j = 0; j < lp->cItem; j++)
            if (!fCtrl || (j >= iFirst && j <= iLast)) lp->rgitem[j].fSelected = j >= iFirst && j <= iLast;
    } else if (fCtrl) {
        lp->rgitem[i].fSelected = !lp->rgitem[i].fSelected; lp->iAnchor = i;
    } else {
        for (j = 0; j < lp->cItem; j++) lp->rgitem[j].fSelected = j == i;
        lp->iAnchor = i;
    }
    lp->iCaret = i;
    RevealList(hwnd, i);
    if (fNotify) NotifyList(hwnd, win32::LBN_SELCHANGE);
}

static void EndListDrag(HWND hwnd, BOOL fRelease) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    hwnd->fTracking = 0;
    if (lp->idDragTimer) win32::KillTimer(hwnd, lp->idDragTimer);
    lp->idDragTimer = 0;
    SDL_free(lp->rgSelectedDrag); lp->rgSelectedDrag = NULL; lp->cSelectedDrag = 0;
    if (fRelease && win32::GetCapture() == hwnd) win32::ReleaseCapture();
}

static void DragList(HWND hwnd, LPARAM l) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    RECT rc;
    int y = (int16_t)HIWORD(l), i, j, iFirst, iLast;
    if (lp->cItem == 0 || !win32::GetClientRect(hwnd, &rc)) return;
    if (y < 0) lp->iTop = StarsMax(0, lp->iTop - 1);
    else if (y >= rc.bottom) lp->iTop++;
    ClampList(hwnd);
    i = (((lp->cItem - 1) < (lp->iTop + (((0) > (StarsMin(y, rc.bottom - 1))) ? (0) : (StarsMin(y, rc.bottom - 1))) / lp->cyItem)) ? (lp->cItem - 1) : (lp->iTop + (((0) > (StarsMin(y, rc.bottom - 1))) ? (0) : (StarsMin(y, rc.bottom - 1))) / lp->cyItem));
    if (i == lp->iDrag) return;
    lp->iDrag = i;
    if ((hwnd->dwStyle & win32::LBS_EXTENDEDSEL) && (lp->grfDrag & win32::MK_CONTROL) && lp->cSelectedDrag == lp->cItem) {
        iFirst = StarsMin(lp->iAnchor, i); iLast = StarsMax(lp->iAnchor, i);
        for (j = 0; j < lp->cItem; j++)
            lp->rgitem[j].fSelected = lp->rgSelectedDrag[j] ^ (j >= iFirst && j <= iLast);
        lp->iCaret = i; RevealList(hwnd, i);
    } else if (hwnd->dwStyle & win32::LBS_EXTENDEDSEL) ExtendedSelectList(hwnd, i, (lp->grfDrag & win32::MK_CONTROL) != 0, 1, 0);
    else UserSelectList(hwnd, i, FMulti(hwnd), 0);
}
static BOOL FDrawOwnerList(HWND hwnd, HDC hdc, int i, UINT action, BOOL fFocus, const RECT *lprcItem) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    DRAWITEMSTRUCT ds = {};
    RECT rc;
    BOOL fItem = i >= 0 && i < lp->cItem;
    ds.CtlType = win32::ODT_LISTBOX; ds.CtlID = (UINT)hwnd->idChild;
    ds.itemID = (UINT)i; ds.itemAction = action;
    ds.itemState = (fItem && (FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel) ? win32::ODS_SELECTED : 0) |
                   (fFocus ? win32::ODS_FOCUS : 0) | (win32::IsWindowEnabled(hwnd) ? 0 : win32::ODS_DISABLED);
    ds.hwndItem = hwnd; ds.hDC = hdc; ds.rcItem = *lprcItem;
    ds.itemData = fItem ? (ULONG_PTR)lp->rgitem[i].lData : 0;
    win32::GetClientRect(hwnd, &rc); win32::IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    win32::SendMessageA(hwnd->hwndParent, win32::WM_DRAWITEM, (WPARAM)hwnd->idChild, (LPARAM)&ds);
    win32::SelectClipRgn(hdc, NULL);
    return win32::IsWindow(hwnd);
}
static void DrawListSelection(HWND hwnd, int iOld) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    HDC hdc;
    RECT rc, rcItem;
    int rgi[2] = {iOld, lp->iSel}, j, i;
    int64_t y;
    unsigned n = lp->nMutation;
    if (FMulti(hwnd) || !FOwnerList(hwnd) || hwnd->lpcombo || iOld == lp->iSel ||
        !win32::IsWindowVisible(hwnd) || !win32::GetClientRect(hwnd, &rc)) return;
    hdc = win32::GetDC(hwnd); if (!hdc) return;
    if (hwnd->hfontControl) win32::SelectObject(hdc, hwnd->hfontControl);
    win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_WINDOWTEXT));
    win32::SetBkColor(hdc, win32::GetSysColor(win32::COLOR_WINDOW));
    for (j = 0; j < 2; j++) {
        i = rgi[j];
        if (i < 0 || i >= lp->cItem) continue;
        y = (int64_t)(i - lp->iTop) * lp->cyItem;
        if (y >= rc.bottom || y + lp->cyItem <= 0) continue;
        win32::SetRect(&rcItem, -lp->xOffset, (int)y, StarsMax(rc.right, lp->cxExtent) - lp->xOffset,
                (int)StarsMin(y + lp->cyItem, INT_MAX));
        if (!FDrawOwnerList(hwnd, hdc, i, win32::ODA_SELECT, win32::GetFocus() == hwnd && i == lp->iCaret, &rcItem)) break;
        // The owner may replace the items or select another row during drawing.
        if (lp->nMutation != n || lp->iSel != rgi[1]) { win32::InvalidateRect(hwnd, NULL, 1); break; }
    }
    win32::ReleaseDC(hwnd, hdc);
}
static void DrawListFocus(HWND hwnd, BOOL fFocus) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    HDC hdc;
    RECT rc, rcItem;
    int i = lp->cItem != 0 ? lp->iCaret : -1;
    int64_t y = i >= 0 ? (int64_t)(i - lp->iTop) * lp->cyItem : 0;
    if (!FOwnerList(hwnd) || hwnd->lpcombo || !win32::IsWindowVisible(hwnd) || !win32::GetClientRect(hwnd, &rc) ||
        y >= rc.bottom || y + lp->cyItem <= 0) return;
    hdc = win32::GetDC(hwnd); if (!hdc) return;
    if (hwnd->hfontControl) win32::SelectObject(hdc, hwnd->hfontControl);
    win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_WINDOWTEXT));
    win32::SetBkColor(hdc, win32::GetSysColor(win32::COLOR_WINDOW));
    win32::SetRect(&rcItem, -lp->xOffset, (int)y, StarsMax(rc.right, lp->cxExtent) - lp->xOffset, (int)StarsMin(y + lp->cyItem, INT_MAX));
    FDrawOwnerList(hwnd, hdc, i, win32::ODA_FOCUS, fFocus, &rcItem);
    win32::ReleaseDC(hwnd, hdc);
}
static void PaintList(HWND hwnd) {
    auto lplistKeep = hwnd->lplist;
    COMPATLIST *lp = lplistKeep.get();
    PAINTSTRUCT ps;
    HDC hdc = win32::BeginPaint(hwnd, &ps);
    HBRUSH hbr, hbrOwn = NULL, hbrSel;
    HFONT hfontOld = NULL;
    RECT rc, rcItem;
    int i, y;
    unsigned n;
    BOOL fSel, fFocus = win32::GetFocus() == hwnd;
    if (!hdc) return;
    win32::GetClientRect(hwnd, &rc);
    if (hwnd->hfontControl) hfontOld = (HFONT)win32::SelectObject(hdc, hwnd->hfontControl);
    win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_WINDOWTEXT));
    win32::SetBkColor(hdc, win32::GetSysColor(win32::COLOR_WINDOW));
    hbr = (HBRUSH)win32::SendMessageA(hwnd->hwndParent, win32::WM_CTLCOLORLISTBOX, (WPARAM)hdc, (LPARAM)hwnd);
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetBkColor(hdc));
    win32::FillRect(hdc, &rc, hbr);
    n = lp->nMutation;
    if (lp->cItem == 0 && FOwnerList(hwnd) && fFocus) {
        win32::SetRect(&rcItem, 0, 0, rc.right, lp->cyItem);
        // Stars handles itemID == -1 by drawing just the empty focus indicator.
        if (!FDrawOwnerList(hwnd, hdc, -1, win32::ODA_FOCUS, 1, &rcItem)) {
            if (hbrOwn) win32::DeleteObject(hbrOwn);
            return;
        }
        if (lp->nMutation != n) win32::InvalidateRect(hwnd, NULL, 1);
    }
    for (i = lp->iTop, y = 0; i < lp->cItem && y < rc.bottom; i++, y += lp->cyItem) {
        win32::SetRect(&rcItem, -lp->xOffset, y, StarsMax(rc.right, lp->cxExtent) - lp->xOffset, y + lp->cyItem);
        fSel = FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel;
        if (hwnd->dwStyle & win32::LBS_OWNERDRAWFIXED) {
            if (!FDrawOwnerList(hwnd, hdc, i, win32::ODA_DRAWENTIRE, fFocus && i == lp->iCaret, &rcItem)) {
                if (hbrOwn) win32::DeleteObject(hbrOwn);
                return;
            }
            if (lp->nMutation != n) { win32::InvalidateRect(hwnd, NULL, 1); break; }
        } else {
            if (fSel) {
                hbrSel = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_HIGHLIGHT)); win32::FillRect(hdc, &rcItem, hbrSel); win32::DeleteObject(hbrSel);
            }
            win32::SetTextColor(hdc, win32::GetSysColor(fSel ? win32::COLOR_HIGHLIGHTTEXT : win32::IsWindowEnabled(hwnd) ? win32::COLOR_WINDOWTEXT : win32::COLOR_GRAYTEXT));
            rcItem.left += 2;
            win32::DrawTextA(hdc, lp->rgitem[i].psz, -1, &rcItem, win32::DT_LEFT | win32::DT_SINGLELINE | win32::DT_VCENTER | win32::DT_NOPREFIX);
            if (fFocus && i == lp->iCaret) win32::FrameRect(hdc, &rcItem, (HBRUSH)win32::GetStockObject(win32::GRAY_BRUSH));
        }
    }
    if ((hwnd->dwStyle & win32::WS_BORDER) || (hwnd->dwExStyle & win32::WS_EX_CLIENTEDGE)) CompatDrawClassicEdge(hdc, &rc, 1, 1);
    if (hfontOld) win32::SelectObject(hdc, hfontOld);
    if (hbrOwn) win32::DeleteObject(hbrOwn);
    win32::EndPaint(hwnd, &ps);
}

LRESULT  CompatListProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lpwin = LpCompatWindow(hwnd);
    std::shared_ptr<COMPATLIST> lplistKeep;
    COMPATLIST *lp;
    HDC hdc;
    TEXTMETRICA tm;
    MEASUREITEMSTRUCT ms;
    RECT rc;
    int i = (int)w, j, c, iNext, cyBar, cy;
    WINDOWPOS *lpwp;
    int64_t yItem;
    LPARAM lData;
    if (!lpwin) return 0;
    lplistKeep = lpwin->lplist;
    lp = lplistKeep.get();
    if (msg == win32::WM_NCCREATE) {
        try { lplistKeep = std::make_shared<COMPATLIST>(); }
        catch (const std::bad_alloc &) { return 0; }
        lp = lplistKeep.get();
        lp->iSel = lp->iAnchor = -1; lp->cyItem = 16; lpwin->lplist = lplistKeep; lpwin->iControl = 4;
        lp->fScrollV = (lpwin->dwStyle & win32::WS_VSCROLL) != 0; lp->fScrollH = (lpwin->dwStyle & win32::WS_HSCROLL) != 0;
        return 1;
    }
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case win32::WM_CREATE:
        if ((lpwin->dwStyle & (win32::LBS_OWNERDRAWVARIABLE | win32::LBS_MULTICOLUMN | win32::LBS_NODATA)) ||
            ((lpwin->dwStyle & win32::LBS_OWNERDRAWFIXED) && !(lpwin->dwStyle & win32::LBS_HASSTRINGS))) return -1;
        hdc = win32::GetDC(hwnd);
        if (hdc) { if (win32::GetTextMetricsA(hdc, &tm)) lp->cyItem = StarsMax(1, tm.tmHeight); win32::ReleaseDC(hwnd, hdc); }
        if (lpwin->dwStyle & win32::LBS_OWNERDRAWFIXED) {
            memset(&ms, 0, sizeof(ms)); ms.CtlType = win32::ODT_LISTBOX; ms.CtlID = (UINT)lpwin->idChild; ms.itemHeight = (UINT)lp->cyItem;
            win32::SendMessageA(lpwin->hwndParent, win32::WM_MEASUREITEM, (WPARAM)lpwin->idChild, (LPARAM)&ms);
            if (!win32::IsWindow(hwnd)) return -1;
            if (ms.itemHeight != 0 && ms.itemHeight <= INT_MAX) lp->cyItem = (int)ms.itemHeight;
        }
        if (!(lpwin->dwStyle & win32::LBS_NOINTEGRALHEIGHT) && lpwin->hwndParent && !lpwin->lpcombo) {
            cyBar = lpwin->dwStyle & win32::WS_HSCROLL ? win32::GetSystemMetrics(win32::SM_CYHSCROLL) : 0;
            cy = lpwin->rcChild.bottom - lpwin->rcChild.top;
            if (cy > cyBar) lpwin->rcChild.bottom = lpwin->rcChild.top + cyBar + (cy - cyBar) / lp->cyItem * lp->cyItem;
        }
        return 0;
    case win32::WM_WINDOWPOSCHANGING:
        if (l && !(lpwin->dwStyle & win32::LBS_NOINTEGRALHEIGHT) && !lpwin->lpcombo) {
            lpwp = (WINDOWPOS *)l;
            cyBar = lpwin->dwStyle & win32::WS_HSCROLL ? win32::GetSystemMetrics(win32::SM_CYHSCROLL) : 0;
            if (!(lpwp->flags & win32::SWP_NOSIZE) && lpwp->cy > cyBar)
                lpwp->cy = cyBar + (lpwp->cy - cyBar) / lp->cyItem * lp->cyItem;
        }
        return 0;
    case win32::WM_DESTROY: ResetList(hwnd); return 0;
    case win32::WM_NCDESTROY:
        CompatDestroyList(hwnd); return 0;
    case win32::LB_ADDSTRING:
        i = lp->cItem;
        if (l && (lpwin->dwStyle & (lpwin->lpcombo ? win32::CBS_SORT : win32::LBS_SORT))) for (i = 0; i < lp->cItem; i++) if (SDL_strcasecmp((LPCSTR)l, lp->rgitem[i].psz) < 0) break;
        return AddList(hwnd, i, (LPCSTR)l);
    case win32::LB_INSERTSTRING: return AddList(hwnd, i, (LPCSTR)l);
    case win32::LB_DELETESTRING:
        if (i < 0 || i >= lp->cItem) return win32::LB_ERR;
        lData = lp->rgitem[i].lData; SDL_free(lp->rgitem[i].psz);
        memmove(lp->rgitem + i, lp->rgitem + i + 1, (size_t)(lp->cItem - i - 1) * sizeof(*lp->rgitem));
        lp->cItem--; lp->nMutation++;
        if (lp->iSel == i) lp->iSel = -1; else if (lp->iSel > i) lp->iSel--;
        if (lp->iAnchor == i) lp->iAnchor = -1; else if (lp->iAnchor > i) lp->iAnchor--;
        if (lp->iCaret > i) lp->iCaret--;
        ClampList(hwnd); c = lp->cItem; DeleteNotice(hwnd, (UINT)i, lData); return c;
    case win32::LB_RESETCONTENT: ResetList(hwnd); return 0;
    case win32::LB_GETCOUNT: return lp->cItem;
    case win32::LB_GETTEXTLEN: return i >= 0 && i < lp->cItem ? (LRESULT)strlen(lp->rgitem[i].psz) : win32::LB_ERR;
    case win32::LB_GETTEXT:
        if (i < 0 || i >= lp->cItem || !l) return win32::LB_ERR;
        strcpy((LPSTR)l, lp->rgitem[i].psz); return (LRESULT)strlen(lp->rgitem[i].psz);
    case win32::LB_GETITEMDATA: return i >= 0 && i < lp->cItem ? lp->rgitem[i].lData : win32::LB_ERR;
    case win32::LB_SETITEMDATA:
        if (i < -1 || i >= lp->cItem) return win32::LB_ERR;
        for (j = 0; j < lp->cItem; j++) if (i == -1 || j == i) lp->rgitem[j].lData = l;
        lp->nMutation++; win32::InvalidateRect(hwnd, NULL, 1); return 1;
    case win32::LB_SETCURSEL:
        if (FMulti(hwnd)) return win32::LB_ERR;
        j = lp->iSel;
        lp->iSel = i >= 0 && i < lp->cItem ? i : -1;
        if (lp->iSel >= 0) { lp->iCaret = i; RevealList(hwnd, i); } else win32::InvalidateRect(hwnd, NULL, 1);
        if (win32::IsWindow(hwnd)) DrawListSelection(hwnd, j);
        return win32::IsWindow(hwnd) ? lp->iSel : win32::LB_ERR;
    case win32::LB_GETCURSEL: return FMulti(hwnd) ? (lp->cItem ? lp->iCaret : win32::LB_ERR) : lp->iSel;
    case win32::LB_SETSEL:
        if (!FMulti(hwnd) || l < -1 || l >= lp->cItem) return win32::LB_ERR;
        for (j = 0; j < lp->cItem; j++) if (l == -1 || j == l) lp->rgitem[j].fSelected = w != 0;
        win32::InvalidateRect(hwnd, NULL, 1); return 0;
    case win32::LB_GETSEL:
        if (i < 0 || i >= lp->cItem) return win32::LB_ERR;
        return FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel;
    case win32::LB_GETSELCOUNT:
        if (!FMulti(hwnd)) return win32::LB_ERR;
        for (j = c = 0; j < lp->cItem; j++) if (lp->rgitem[j].fSelected) c++;
        return c;
    case win32::LB_GETSELITEMS:
        if (!FMulti(hwnd) || !l || i < 0) return win32::LB_ERR;
        for (j = c = 0; j < lp->cItem && c < i; j++) if (lp->rgitem[j].fSelected) ((int *)l)[c++] = j;
        return c;
    case win32::LB_SETTOPINDEX: lp->iTop = i; ClampList(hwnd); return 0;
    case win32::LB_GETTOPINDEX: return lp->iTop;
    case win32::LB_GETHORIZONTALEXTENT: return lp->cxExtent;
    case win32::LB_SETHORIZONTALEXTENT:
        lp->cxExtent = (((0) > ((int)w)) ? (0) : ((int)w)); ClampList(hwnd); return 0;
    case win32::LB_GETITEMHEIGHT: return lp->cyItem;
    case win32::LB_SETITEMHEIGHT:
        if (l <= 0 || l > 255) return win32::LB_ERR;
        lp->cyItem = (int)l; ClampList(hwnd); return 0;
    case win32::LB_GETITEMRECT:
        if (i < 0 || i >= lp->cItem || !l) return win32::LB_ERR;
        win32::GetClientRect(hwnd, &rc);
        yItem = (int64_t)(i - lp->iTop) * lp->cyItem;
        if (yItem < INT_MIN || yItem + lp->cyItem > INT_MAX) return win32::LB_ERR;
        win32::SetRect((LPRECT)l, 0, (int)yItem, rc.right, (int)(yItem + lp->cyItem)); return 1;
    case win32::LB_SETCARETINDEX:
        if (i < 0 || i >= lp->cItem) return win32::LB_ERR;
        lp->iCaret = i; RevealList(hwnd, i); return 0;
    case win32::LB_GETCARETINDEX: return lp->iCaret;
    case win32::LB_FINDSTRING: case win32::LB_FINDSTRINGEXACT:
        if (!l || lp->cItem == 0) return win32::LB_ERR;
        if (i < -1 || i >= lp->cItem) i = -1;
        for (j = 1; j <= lp->cItem; j++) {
            iNext = (int)(((int64_t)i + j) % lp->cItem);
            if ((msg == win32::LB_FINDSTRINGEXACT ? SDL_strcasecmp(lp->rgitem[iNext].psz, (LPCSTR)l) :
                 SDL_strncasecmp(lp->rgitem[iNext].psz, (LPCSTR)l, strlen((LPCSTR)l))) == 0) return iNext;
        }
        return win32::LB_ERR;
    case win32::WM_GETDLGCODE: return win32::DLGC_WANTARROWS | win32::DLGC_WANTCHARS;
    case win32::WM_GETFONT: return (LRESULT)lpwin->hfontControl;
    case win32::WM_SETFONT:
        lpwin->hfontControl = (HFONT)w;
        if (!FOwnerList(hwnd) && (hdc = win32::GetDC(hwnd))) {
            if (lpwin->hfontControl) win32::SelectObject(hdc, lpwin->hfontControl);
            if (win32::GetTextMetricsA(hdc, &tm)) lp->cyItem = StarsMax(1, tm.tmHeight);
            win32::ReleaseDC(hwnd, hdc);
        }
        ClampList(hwnd); if (l) win32::InvalidateRect(hwnd, NULL, 1); return 0;
    case win32::WM_PAINT: PaintList(hwnd); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::WM_ENABLE: case win32::WM_SIZE: ClampList(hwnd); return 0;
    case win32::WM_SETFOCUS: case win32::WM_KILLFOCUS:
        DrawListFocus(hwnd, msg == win32::WM_SETFOCUS);
        if (win32::IsWindow(hwnd)) {
            win32::InvalidateRect(hwnd, NULL, 1); NotifyList(hwnd, msg == win32::WM_SETFOCUS ? win32::LBN_SETFOCUS : win32::LBN_KILLFOCUS);
        }
        return 0;
    case win32::WM_LBUTTONDOWN: case win32::WM_LBUTTONDBLCLK:
        win32::SetFocus(hwnd); if (!win32::IsWindow(hwnd)) return 0;
        i = lp->iTop + (int16_t)HIWORD(l) / lp->cyItem;
        if ((int16_t)HIWORD(l) < 0) return 0;
        SDL_free(lp->rgSelectedDrag); lp->rgSelectedDrag = NULL; lp->cSelectedDrag = 0;
        if ((lpwin->dwStyle & win32::LBS_EXTENDEDSEL) && (w & win32::MK_CONTROL) && lp->cItem != 0) {
            lp->rgSelectedDrag = (BYTE *)(SDL_malloc((size_t)lp->cItem));
            if (lp->rgSelectedDrag) {
                lp->cSelectedDrag = lp->cItem;
                for (j = 0; j < lp->cItem; j++) lp->rgSelectedDrag[j] = (BYTE)lp->rgitem[j].fSelected;
            }
        }
        ExtendedSelectList(hwnd, i, (w & win32::MK_CONTROL) != 0, (w & win32::MK_SHIFT) != 0, 0);
        if (!win32::IsWindow(hwnd)) return 0;
        if (msg == win32::WM_LBUTTONDBLCLK) NotifyList(hwnd, win32::LBN_DBLCLK);
        else {
            win32::SetCapture(hwnd);
            if (win32::IsWindow(hwnd) && win32::GetCapture() == hwnd) {
                lpwin->fTracking = 1; lp->iDrag = i; lp->grfDrag = (DWORD)w; lp->lDragPoint = l;
                lp->idDragTimer = win32::SetTimer(hwnd, UINTPTR_MAX, 100, NULL);
            }
        }
        return 0;
    case win32::WM_MOUSEMOVE:
        if (lpwin->fTracking && win32::GetCapture() == hwnd) { lp->lDragPoint = l; DragList(hwnd, l); }
        return 0;
    case win32::WM_TIMER:
        if (w == lp->idDragTimer && lpwin->fTracking && win32::GetCapture() == hwnd) DragList(hwnd, lp->lDragPoint);
        return 0;
    case win32::WM_LBUTTONUP:
        c = lpwin->fTracking; EndListDrag(hwnd, 1);
        if (c != 0 && win32::IsWindow(hwnd)) NotifyList(hwnd, win32::LBN_SELCHANGE);
        return 0;
    case win32::WM_CAPTURECHANGED: case win32::WM_CANCELMODE:
        EndListDrag(hwnd, msg == win32::WM_CANCELMODE);
        return 0;
    case win32::WM_KEYDOWN:
        i = lp->iCaret;
        if (w == win32::VK_UP) i--;
        else if (w == win32::VK_DOWN) i++;
        else if (w == win32::VK_HOME) i = 0;
        else if (w == win32::VK_END) i = lp->cItem - 1;
        else if (w == win32::VK_PRIOR) i -= CListPage(hwnd);
        else if (w == win32::VK_NEXT) i += CListPage(hwnd);
        else if (w == win32::VK_SPACE && FMulti(hwnd)) {
            ExtendedSelectList(hwnd, i, (win32::GetKeyState(win32::VK_CONTROL) & 0x8000) != 0,
                               (win32::GetKeyState(win32::VK_SHIFT) & 0x8000) != 0, 1); return 0;
        }
        else return 0;
        i = (((StarsMax(0, i)) < (lp->cItem - 1)) ? (StarsMax(0, i)) : (lp->cItem - 1));
        if ((lpwin->dwStyle & win32::LBS_EXTENDEDSEL) && !(win32::GetKeyState(win32::VK_CONTROL) & 0x8000))
            ExtendedSelectList(hwnd, i, 0, (win32::GetKeyState(win32::VK_SHIFT) & 0x8000) != 0, 1);
        else if (FMulti(hwnd)) { lp->iCaret = StarsMax(0, i); RevealList(hwnd, StarsMax(0, i)); }
        else UserSelectList(hwnd, i, 0, 1);
        return 0;
    case win32::WM_CHAR:
        if (w < 32 || w > 255 || lp->cItem == 0) return 0;
        c = (int)strlen(lp->szSearch);
        if (CompatTimeIO().pfnGetTicks() - lp->msSearch > 1000 || c >= (int)sizeof(lp->szSearch) - 1 ||
            (c == 1 && (BYTE)lp->szSearch[0] == (BYTE)w)) c = 0;
        lp->szSearch[c] = (char)w; lp->szSearch[c + 1] = 0; lp->msSearch = CompatTimeIO().pfnGetTicks();
        i = (int)CompatListProc(hwnd, win32::LB_FINDSTRING, c != 0 ? lp->iCaret - 1 : lp->iCaret, (LPARAM)lp->szSearch);
        if (i != win32::LB_ERR) ExtendedSelectList(hwnd, i, 0, 0, 1);
        return 0;
    case win32::WM_HSCROLL:
        win32::GetClientRect(hwnd, &rc);
        if (LOWORD(w) == win32::SB_LINELEFT) lp->xOffset -= 8;
        else if (LOWORD(w) == win32::SB_LINERIGHT) lp->xOffset += 8;
        else if (LOWORD(w) == win32::SB_PAGELEFT) lp->xOffset -= rc.right;
        else if (LOWORD(w) == win32::SB_PAGERIGHT) lp->xOffset += rc.right;
        else if (LOWORD(w) == win32::SB_LEFT) lp->xOffset = 0;
        else if (LOWORD(w) == win32::SB_RIGHT) lp->xOffset = lp->cxExtent;
        else if (LOWORD(w) == win32::SB_THUMBPOSITION || LOWORD(w) == win32::SB_THUMBTRACK) {
            if (l == 0 && win32::IsWindow(lpwin->rghwndScroll[win32::SB_HORZ]) && lpwin->rghwndScroll[win32::SB_HORZ]->fTracking)
                lp->xOffset = lpwin->rghwndScroll[win32::SB_HORZ]->iScrollTrack;
            else lp->xOffset = HIWORD(w);
        }
        lp->xOffset = (((0) > ((((lp->xOffset) < (StarsMax(0, lp->cxExtent - rc.right))) ? (lp->xOffset) : (StarsMax(0, lp->cxExtent - rc.right))))) ? (0) : ((((lp->xOffset) < (StarsMax(0, lp->cxExtent - rc.right))) ? (lp->xOffset) : (StarsMax(0, lp->cxExtent - rc.right)))));
        ClampList(hwnd); return 0;
    case win32::WM_VSCROLL:
        if (LOWORD(w) == win32::SB_LINEUP) lp->iTop--;
        else if (LOWORD(w) == win32::SB_LINEDOWN) lp->iTop++;
        else if (LOWORD(w) == win32::SB_PAGEUP) lp->iTop -= CListPage(hwnd);
        else if (LOWORD(w) == win32::SB_PAGEDOWN) lp->iTop += CListPage(hwnd);
        else if (LOWORD(w) == win32::SB_TOP) lp->iTop = 0;
        else if (LOWORD(w) == win32::SB_BOTTOM) lp->iTop = lp->cItem;
        else if (LOWORD(w) == win32::SB_THUMBPOSITION || LOWORD(w) == win32::SB_THUMBTRACK) {
            if (l == 0 && win32::IsWindow(lpwin->rghwndScroll[win32::SB_VERT]) && lpwin->rghwndScroll[win32::SB_VERT]->fTracking)
                lp->iTop = lpwin->rghwndScroll[win32::SB_VERT]->iScrollTrack;
            else lp->iTop = HIWORD(w);
        }
        ClampList(hwnd); return 0;
    case win32::WM_MOUSEWHEEL:
        lp->iTop = (int)((((int64_t)INT_MAX) < (((((int64_t)0) > ((int64_t)lp->iTop - (int16_t)HIWORD(w) / win32::WHEEL_DELTA * 3)) ? ((int64_t)0) : ((int64_t)lp->iTop - (int16_t)HIWORD(w) / win32::WHEEL_DELTA * 3)))) ? ((int64_t)INT_MAX) : (((((int64_t)0) > ((int64_t)lp->iTop - (int16_t)HIWORD(w) / win32::WHEEL_DELTA * 3)) ? ((int64_t)0) : ((int64_t)lp->iTop - (int16_t)HIWORD(w) / win32::WHEEL_DELTA * 3))));
        ClampList(hwnd); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
