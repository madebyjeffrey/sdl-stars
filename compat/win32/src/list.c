/* String list boxes, including the fixed-height owner drawing used by Stars. */
#include "internal.h"
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
};
unsigned NCompatListMutation(HWND hwnd) { return hwnd->lplist ? hwnd->lplist->nMutation : 0; }

void CompatDestroyList(HWND hwnd) {
    COMPATLIST *lp = hwnd->lplist;
    int i;
    if (!lp) return;
    for (i = 0; i < lp->cItem; i++) SDL_free(lp->rgitem[i].psz);
    SDL_free(lp->rgSelectedDrag);
    SDL_free(lp->rgitem);
    SDL_free(lp);
    hwnd->lplist = NULL;
}

static BOOL FMulti(HWND hwnd) { return !hwnd->lpcombo && (hwnd->dwStyle & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0; }
static BOOL FOwnerList(HWND hwnd) { return (hwnd->dwStyle & (hwnd->lpcombo ? CBS_OWNERDRAWFIXED : LBS_OWNERDRAWFIXED)) != 0; }
static void NotifyList(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent && (lp->dwStyle & LBS_NOTIFY))
        SendMessageA(lp->hwndParent, WM_COMMAND, MAKEWPARAM(lp->idChild, code), (LPARAM)hwnd);
}
static int CListPage(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    return max(1, (hwnd->lpcombo ? hwnd->lpcombo->cyDrop : rc.bottom) / hwnd->lplist->cyItem);
}
static void ClampList(HWND hwnd) {
    COMPATLIST *lp = hwnd->lplist;
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 0, 0, 0, 0};
    RECT rc;
    lp->iTop = min(max(0, lp->iTop), max(0, lp->cItem - CListPage(hwnd)));
    lp->iCaret = min(max(0, lp->iCaret), max(0, lp->cItem - 1));
    if (!hwnd->lpcombo && !lp->fScrollSync && !hwnd->fDestroying) {
        lp->fScrollSync = TRUE;
        if (hwnd->dwStyle & LBS_DISABLENOSCROLL) si.fMask |= SIF_DISABLENOSCROLL;
        si.nMax = max(0, lp->cItem - 1); si.nPage = (UINT)CListPage(hwnd); si.nPos = lp->iTop;
        if (lp->fScrollV) SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        if (!IsWindow(hwnd)) return;
        GetClientRect(hwnd, &rc);
        if (lp->fScrollH) {
            si.nMax = max(0, lp->cxExtent - 1); si.nPage = (UINT)max(1, rc.right); si.nPos = lp->xOffset;
            lp->xOffset = SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            if (!IsWindow(hwnd)) return;
        }
        lp->iTop = min(lp->iTop, max(0, lp->cItem - CListPage(hwnd)));
        if (lp->fScrollV) SetScrollPos(hwnd, SB_VERT, lp->iTop, TRUE);
        lp->fScrollSync = FALSE;
    }
    InvalidateRect(hwnd, NULL, TRUE);
}
static void RevealList(HWND hwnd, int i) {
    COMPATLIST *lp = hwnd->lplist;
    int cPage = CListPage(hwnd);
    if (i < lp->iTop) lp->iTop = i;
    else if (i >= lp->iTop + cPage) lp->iTop = i - cPage + 1;
    ClampList(hwnd);
}
static void DeleteNotice(HWND hwnd, UINT i, LPARAM lData) {
    DELETEITEMSTRUCT ds;
    if (!FOwnerList(hwnd) || !IsWindow(hwnd->hwndParent)) return;
    memset(&ds, 0, sizeof(ds)); ds.CtlType = hwnd->lpcombo ? ODT_COMBOBOX : ODT_LISTBOX; ds.CtlID = (UINT)hwnd->idChild;
    ds.itemID = i; ds.hwndItem = hwnd; ds.itemData = (ULONG_PTR)lData;
    SendMessageA(hwnd->hwndParent, WM_DELETEITEM, (WPARAM)hwnd->idChild, (LPARAM)&ds);
}
static void EndListDrag(HWND hwnd, BOOL fRelease);
static void ResetList(HWND hwnd) {
    COMPATLIST *lp = hwnd->lplist;
    COMPATLISTITEM *rg = lp->rgitem;
    int c = lp->cItem, i;
    lp->rgitem = NULL; lp->cItem = lp->cAlloc = lp->iTop = lp->iCaret = 0; lp->iSel = -1; lp->nMutation++;
    lp->iAnchor = -1; lp->szSearch[0] = 0;
    EndListDrag(hwnd, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
    for (i = 0; i < c; i++) { SDL_free(rg[i].psz); DeleteNotice(hwnd, (UINT)i, rg[i].lData); }
    SDL_free(rg);
    if (IsWindow(hwnd) && !hwnd->fDestroying) ClampList(hwnd);
}
static LRESULT AddList(HWND hwnd, int i, LPCSTR psz) {
    COMPATLIST *lp = hwnd->lplist;
    COMPATLISTITEM *rg;
    char *pszNew;
    int cAlloc;
    if (!psz || !FUtf8Valid(psz, strlen(psz)) || lp->cItem == INT_MAX) return LB_ERR;
    pszNew = SDL_strdup(psz);
    if (!pszNew) return LB_ERRSPACE;
    if (lp->cItem == lp->cAlloc) {
        cAlloc = lp->cAlloc > (INT_MAX - 16) / 2 ? INT_MAX : lp->cAlloc * 2 + 16;
        if ((size_t)cAlloc > SIZE_MAX / sizeof(*rg)) { SDL_free(pszNew); return LB_ERRSPACE; }
        rg = (COMPATLISTITEM *)(SDL_realloc(lp->rgitem, (size_t)cAlloc * sizeof(*rg)));
        if (!rg) { SDL_free(pszNew); return LB_ERRSPACE; }
        lp->rgitem = rg; lp->cAlloc = cAlloc;
    }
    if (i < 0 || i > lp->cItem) i = lp->cItem;
    memmove(lp->rgitem + i + 1, lp->rgitem + i, (size_t)(lp->cItem - i) * sizeof(*lp->rgitem));
    lp->rgitem[i] = (COMPATLISTITEM){pszNew, 0, FALSE}; lp->cItem++; lp->nMutation++;
    if (lp->iSel >= i) lp->iSel++;
    if (lp->cItem > 1 && lp->iCaret >= i) lp->iCaret++;
    if (lp->iAnchor >= i) lp->iAnchor++;
    ClampList(hwnd);
    return i;
}
static void DrawListSelection(HWND hwnd, int iOld);
static void UserSelectList(HWND hwnd, int i, BOOL fToggle, BOOL fNotify) {
    COMPATLIST *lp = hwnd->lplist;
    int j, iOld = lp->iSel;
    if (i < 0 || i >= lp->cItem) return;
    lp->iCaret = i;
    if (FMulti(hwnd)) {
        if (!fToggle && !(hwnd->dwStyle & LBS_MULTIPLESEL))
            for (j = 0; j < lp->cItem; j++) lp->rgitem[j].fSelected = FALSE;
        lp->rgitem[i].fSelected = fToggle ? !lp->rgitem[i].fSelected : TRUE;
    } else lp->iSel = i;
    RevealList(hwnd, i);
    if (IsWindow(hwnd)) DrawListSelection(hwnd, iOld);
    if (fNotify && IsWindow(hwnd)) NotifyList(hwnd, LBN_SELCHANGE);
}
static void ExtendedSelectList(HWND hwnd, int i, BOOL fCtrl, BOOL fShift, BOOL fNotify) {
    COMPATLIST *lp = hwnd->lplist;
    int j, iFirst, iLast;
    if (i < 0 || i >= lp->cItem) return;
    if (!(hwnd->dwStyle & LBS_EXTENDEDSEL)) {
        UserSelectList(hwnd, i, FMulti(hwnd), fNotify); return;
    }
    if (fShift) {
        if (lp->iAnchor < 0 || lp->iAnchor >= lp->cItem) lp->iAnchor = lp->iCaret;
        iFirst = min(lp->iAnchor, i); iLast = max(lp->iAnchor, i);
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
    if (fNotify) NotifyList(hwnd, LBN_SELCHANGE);
}

static void EndListDrag(HWND hwnd, BOOL fRelease) {
    COMPATLIST *lp = hwnd->lplist;
    hwnd->fTracking = FALSE;
    if (lp->idDragTimer) KillTimer(hwnd, lp->idDragTimer);
    lp->idDragTimer = 0;
    SDL_free(lp->rgSelectedDrag); lp->rgSelectedDrag = NULL; lp->cSelectedDrag = 0;
    if (fRelease && GetCapture() == hwnd) ReleaseCapture();
}

static void DragList(HWND hwnd, LPARAM l) {
    COMPATLIST *lp = hwnd->lplist;
    RECT rc;
    int y = (int16_t)HIWORD(l), i, j, iFirst, iLast;
    if (lp->cItem == 0 || !GetClientRect(hwnd, &rc)) return;
    if (y < 0) lp->iTop = max(0, lp->iTop - 1);
    else if (y >= rc.bottom) lp->iTop++;
    ClampList(hwnd);
    i = min(lp->cItem - 1, lp->iTop + max(0, min(y, rc.bottom - 1)) / lp->cyItem);
    if (i == lp->iDrag) return;
    lp->iDrag = i;
    if ((hwnd->dwStyle & LBS_EXTENDEDSEL) && (lp->grfDrag & MK_CONTROL) && lp->cSelectedDrag == lp->cItem) {
        iFirst = min(lp->iAnchor, i); iLast = max(lp->iAnchor, i);
        for (j = 0; j < lp->cItem; j++)
            lp->rgitem[j].fSelected = lp->rgSelectedDrag[j] ^ (j >= iFirst && j <= iLast);
        lp->iCaret = i; RevealList(hwnd, i);
    } else if (hwnd->dwStyle & LBS_EXTENDEDSEL) ExtendedSelectList(hwnd, i, (lp->grfDrag & MK_CONTROL) != 0, TRUE, FALSE);
    else UserSelectList(hwnd, i, FMulti(hwnd), FALSE);
}
static BOOL FDrawOwnerList(HWND hwnd, HDC hdc, int i, UINT action, BOOL fFocus, const RECT *lprcItem) {
    COMPATLIST *lp = hwnd->lplist;
    DRAWITEMSTRUCT ds = {};
    RECT rc;
    BOOL fItem = i >= 0 && i < lp->cItem;
    ds.CtlType = ODT_LISTBOX; ds.CtlID = (UINT)hwnd->idChild;
    ds.itemID = (UINT)i; ds.itemAction = action;
    ds.itemState = (fItem && (FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel) ? ODS_SELECTED : 0) |
                   (fFocus ? ODS_FOCUS : 0) | (IsWindowEnabled(hwnd) ? 0 : ODS_DISABLED);
    ds.hwndItem = hwnd; ds.hDC = hdc; ds.rcItem = *lprcItem;
    ds.itemData = fItem ? (ULONG_PTR)lp->rgitem[i].lData : 0;
    GetClientRect(hwnd, &rc); IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SendMessageA(hwnd->hwndParent, WM_DRAWITEM, (WPARAM)hwnd->idChild, (LPARAM)&ds);
    SelectClipRgn(hdc, NULL);
    return IsWindow(hwnd);
}
static void DrawListSelection(HWND hwnd, int iOld) {
    COMPATLIST *lp = hwnd->lplist;
    HDC hdc;
    RECT rc, rcItem;
    int rgi[2] = {iOld, lp->iSel}, j, i;
    int64_t y;
    unsigned n = lp->nMutation;
    if (FMulti(hwnd) || !FOwnerList(hwnd) || hwnd->lpcombo || iOld == lp->iSel ||
        !IsWindowVisible(hwnd) || !GetClientRect(hwnd, &rc)) return;
    hdc = GetDC(hwnd); if (!hdc) return;
    if (hwnd->hfontControl) SelectObject(hdc, hwnd->hfontControl);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    for (j = 0; j < 2; j++) {
        i = rgi[j];
        if (i < 0 || i >= lp->cItem) continue;
        y = (int64_t)(i - lp->iTop) * lp->cyItem;
        if (y >= rc.bottom || y + lp->cyItem <= 0) continue;
        SetRect(&rcItem, -lp->xOffset, (int)y, max(rc.right, lp->cxExtent) - lp->xOffset,
                (int)min(y + lp->cyItem, INT_MAX));
        if (!FDrawOwnerList(hwnd, hdc, i, ODA_SELECT, GetFocus() == hwnd && i == lp->iCaret, &rcItem)) break;
        // The owner may replace the items or select another row during drawing.
        if (lp->nMutation != n || lp->iSel != rgi[1]) { InvalidateRect(hwnd, NULL, TRUE); break; }
    }
    ReleaseDC(hwnd, hdc);
}
static void DrawListFocus(HWND hwnd, BOOL fFocus) {
    COMPATLIST *lp = hwnd->lplist;
    HDC hdc;
    RECT rc, rcItem;
    int i = lp->cItem != 0 ? lp->iCaret : -1;
    int64_t y = i >= 0 ? (int64_t)(i - lp->iTop) * lp->cyItem : 0;
    if (!FOwnerList(hwnd) || hwnd->lpcombo || !IsWindowVisible(hwnd) || !GetClientRect(hwnd, &rc) ||
        y >= rc.bottom || y + lp->cyItem <= 0) return;
    hdc = GetDC(hwnd); if (!hdc) return;
    if (hwnd->hfontControl) SelectObject(hdc, hwnd->hfontControl);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    SetRect(&rcItem, -lp->xOffset, (int)y, max(rc.right, lp->cxExtent) - lp->xOffset, (int)min(y + lp->cyItem, INT_MAX));
    FDrawOwnerList(hwnd, hdc, i, ODA_FOCUS, fFocus, &rcItem);
    ReleaseDC(hwnd, hdc);
}
static void PaintList(HWND hwnd) {
    COMPATLIST *lp = hwnd->lplist;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH hbr, hbrOwn = NULL, hbrSel;
    HFONT hfontOld = NULL;
    RECT rc, rcItem;
    int i, y;
    unsigned n;
    BOOL fSel, fFocus = GetFocus() == hwnd;
    if (!hdc) return;
    GetClientRect(hwnd, &rc);
    if (hwnd->hfontControl) hfontOld = (HFONT)SelectObject(hdc, hwnd->hfontControl);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    hbr = (HBRUSH)SendMessageA(hwnd->hwndParent, WM_CTLCOLORLISTBOX, (WPARAM)hdc, (LPARAM)hwnd);
    if (!IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = CreateSolidBrush(GetBkColor(hdc));
    FillRect(hdc, &rc, hbr);
    n = lp->nMutation;
    if (lp->cItem == 0 && FOwnerList(hwnd) && fFocus) {
        SetRect(&rcItem, 0, 0, rc.right, lp->cyItem);
        // Stars handles itemID == -1 by drawing just the empty focus indicator.
        if (!FDrawOwnerList(hwnd, hdc, -1, ODA_FOCUS, TRUE, &rcItem)) {
            if (hbrOwn) DeleteObject(hbrOwn);
            return;
        }
        if (lp->nMutation != n) InvalidateRect(hwnd, NULL, TRUE);
    }
    for (i = lp->iTop, y = 0; i < lp->cItem && y < rc.bottom; i++, y += lp->cyItem) {
        SetRect(&rcItem, -lp->xOffset, y, max(rc.right, lp->cxExtent) - lp->xOffset, y + lp->cyItem);
        fSel = FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel;
        if (hwnd->dwStyle & LBS_OWNERDRAWFIXED) {
            if (!FDrawOwnerList(hwnd, hdc, i, ODA_DRAWENTIRE, fFocus && i == lp->iCaret, &rcItem)) {
                if (hbrOwn) DeleteObject(hbrOwn);
                return;
            }
            if (lp->nMutation != n) { InvalidateRect(hwnd, NULL, TRUE); break; }
        } else {
            if (fSel) {
                hbrSel = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT)); FillRect(hdc, &rcItem, hbrSel); DeleteObject(hbrSel);
            }
            SetTextColor(hdc, GetSysColor(fSel ? COLOR_HIGHLIGHTTEXT : IsWindowEnabled(hwnd) ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
            rcItem.left += 2;
            DrawTextA(hdc, lp->rgitem[i].psz, -1, &rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            if (fFocus && i == lp->iCaret) FrameRect(hdc, &rcItem, (HBRUSH)GetStockObject(GRAY_BRUSH));
        }
    }
    if ((hwnd->dwStyle & WS_BORDER) || (hwnd->dwExStyle & WS_EX_CLIENTEDGE)) CompatDrawClassicEdge(hdc, &rc, TRUE, TRUE);
    if (hfontOld) SelectObject(hdc, hfontOld);
    if (hbrOwn) DeleteObject(hbrOwn);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK CompatListProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lpwin = LpCompatWindow(hwnd);
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
    lp = lpwin->lplist;
    if (msg == WM_NCCREATE) {
        lp = (COMPATLIST *)(SDL_calloc(1, sizeof(*lp))); if (!lp) return FALSE;
        lp->iSel = lp->iAnchor = -1; lp->cyItem = 16; lpwin->lplist = lp; lpwin->iControl = 4;
        lp->fScrollV = (lpwin->dwStyle & WS_VSCROLL) != 0; lp->fScrollH = (lpwin->dwStyle & WS_HSCROLL) != 0;
        return TRUE;
    }
    if (!lp) return DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case WM_CREATE:
        if ((lpwin->dwStyle & (LBS_OWNERDRAWVARIABLE | LBS_MULTICOLUMN | LBS_NODATA)) ||
            ((lpwin->dwStyle & LBS_OWNERDRAWFIXED) && !(lpwin->dwStyle & LBS_HASSTRINGS))) return -1;
        hdc = GetDC(hwnd);
        if (hdc) { if (GetTextMetricsA(hdc, &tm)) lp->cyItem = max(1, tm.tmHeight); ReleaseDC(hwnd, hdc); }
        if (lpwin->dwStyle & LBS_OWNERDRAWFIXED) {
            memset(&ms, 0, sizeof(ms)); ms.CtlType = ODT_LISTBOX; ms.CtlID = (UINT)lpwin->idChild; ms.itemHeight = (UINT)lp->cyItem;
            SendMessageA(lpwin->hwndParent, WM_MEASUREITEM, (WPARAM)lpwin->idChild, (LPARAM)&ms);
            if (!IsWindow(hwnd)) return -1;
            if (ms.itemHeight != 0 && ms.itemHeight <= INT_MAX) lp->cyItem = (int)ms.itemHeight;
        }
        if (!(lpwin->dwStyle & LBS_NOINTEGRALHEIGHT) && lpwin->hwndParent && !lpwin->lpcombo) {
            cyBar = lpwin->dwStyle & WS_HSCROLL ? GetSystemMetrics(SM_CYHSCROLL) : 0;
            cy = lpwin->rcChild.bottom - lpwin->rcChild.top;
            if (cy > cyBar) lpwin->rcChild.bottom = lpwin->rcChild.top + cyBar + (cy - cyBar) / lp->cyItem * lp->cyItem;
        }
        return 0;
    case WM_WINDOWPOSCHANGING:
        if (l && !(lpwin->dwStyle & LBS_NOINTEGRALHEIGHT) && !lpwin->lpcombo) {
            lpwp = (WINDOWPOS *)l;
            cyBar = lpwin->dwStyle & WS_HSCROLL ? GetSystemMetrics(SM_CYHSCROLL) : 0;
            if (!(lpwp->flags & SWP_NOSIZE) && lpwp->cy > cyBar)
                lpwp->cy = cyBar + (lpwp->cy - cyBar) / lp->cyItem * lp->cyItem;
        }
        return 0;
    case WM_DESTROY: ResetList(hwnd); return 0;
    case WM_NCDESTROY:
        CompatDestroyList(hwnd); return 0;
    case LB_ADDSTRING:
        i = lp->cItem;
        if (l && (lpwin->dwStyle & (lpwin->lpcombo ? CBS_SORT : LBS_SORT))) for (i = 0; i < lp->cItem; i++) if (SDL_strcasecmp((LPCSTR)l, lp->rgitem[i].psz) < 0) break;
        return AddList(hwnd, i, (LPCSTR)l);
    case LB_INSERTSTRING: return AddList(hwnd, i, (LPCSTR)l);
    case LB_DELETESTRING:
        if (i < 0 || i >= lp->cItem) return LB_ERR;
        lData = lp->rgitem[i].lData; SDL_free(lp->rgitem[i].psz);
        memmove(lp->rgitem + i, lp->rgitem + i + 1, (size_t)(lp->cItem - i - 1) * sizeof(*lp->rgitem));
        lp->cItem--; lp->nMutation++;
        if (lp->iSel == i) lp->iSel = -1; else if (lp->iSel > i) lp->iSel--;
        if (lp->iAnchor == i) lp->iAnchor = -1; else if (lp->iAnchor > i) lp->iAnchor--;
        if (lp->iCaret > i) lp->iCaret--;
        ClampList(hwnd); c = lp->cItem; DeleteNotice(hwnd, (UINT)i, lData); return c;
    case LB_RESETCONTENT: ResetList(hwnd); return 0;
    case LB_GETCOUNT: return lp->cItem;
    case LB_GETTEXTLEN: return i >= 0 && i < lp->cItem ? (LRESULT)strlen(lp->rgitem[i].psz) : LB_ERR;
    case LB_GETTEXT:
        if (i < 0 || i >= lp->cItem || !l) return LB_ERR;
        strcpy((LPSTR)l, lp->rgitem[i].psz); return (LRESULT)strlen(lp->rgitem[i].psz);
    case LB_GETITEMDATA: return i >= 0 && i < lp->cItem ? lp->rgitem[i].lData : LB_ERR;
    case LB_SETITEMDATA:
        if (i < -1 || i >= lp->cItem) return LB_ERR;
        for (j = 0; j < lp->cItem; j++) if (i == -1 || j == i) lp->rgitem[j].lData = l;
        lp->nMutation++; InvalidateRect(hwnd, NULL, TRUE); return 1;
    case LB_SETCURSEL:
        if (FMulti(hwnd)) return LB_ERR;
        j = lp->iSel;
        lp->iSel = i >= 0 && i < lp->cItem ? i : -1;
        if (lp->iSel >= 0) { lp->iCaret = i; RevealList(hwnd, i); } else InvalidateRect(hwnd, NULL, TRUE);
        if (IsWindow(hwnd)) DrawListSelection(hwnd, j);
        return IsWindow(hwnd) ? lp->iSel : LB_ERR;
    case LB_GETCURSEL: return FMulti(hwnd) ? (lp->cItem ? lp->iCaret : LB_ERR) : lp->iSel;
    case LB_SETSEL:
        if (!FMulti(hwnd) || l < -1 || l >= lp->cItem) return LB_ERR;
        for (j = 0; j < lp->cItem; j++) if (l == -1 || j == l) lp->rgitem[j].fSelected = w != 0;
        InvalidateRect(hwnd, NULL, TRUE); return 0;
    case LB_GETSEL:
        if (i < 0 || i >= lp->cItem) return LB_ERR;
        return FMulti(hwnd) ? lp->rgitem[i].fSelected : i == lp->iSel;
    case LB_GETSELCOUNT:
        if (!FMulti(hwnd)) return LB_ERR;
        for (j = c = 0; j < lp->cItem; j++) if (lp->rgitem[j].fSelected) c++;
        return c;
    case LB_GETSELITEMS:
        if (!FMulti(hwnd) || !l || i < 0) return LB_ERR;
        for (j = c = 0; j < lp->cItem && c < i; j++) if (lp->rgitem[j].fSelected) ((int *)l)[c++] = j;
        return c;
    case LB_SETTOPINDEX: lp->iTop = i; ClampList(hwnd); return 0;
    case LB_GETTOPINDEX: return lp->iTop;
    case LB_GETHORIZONTALEXTENT: return lp->cxExtent;
    case LB_SETHORIZONTALEXTENT:
        lp->cxExtent = max(0, (int)w); ClampList(hwnd); return 0;
    case LB_GETITEMHEIGHT: return lp->cyItem;
    case LB_SETITEMHEIGHT:
        if (l <= 0 || l > 255) return LB_ERR;
        lp->cyItem = (int)l; ClampList(hwnd); return 0;
    case LB_GETITEMRECT:
        if (i < 0 || i >= lp->cItem || !l) return LB_ERR;
        GetClientRect(hwnd, &rc);
        yItem = (int64_t)(i - lp->iTop) * lp->cyItem;
        if (yItem < INT_MIN || yItem + lp->cyItem > INT_MAX) return LB_ERR;
        SetRect((LPRECT)l, 0, (int)yItem, rc.right, (int)(yItem + lp->cyItem)); return 1;
    case LB_SETCARETINDEX:
        if (i < 0 || i >= lp->cItem) return LB_ERR;
        lp->iCaret = i; RevealList(hwnd, i); return 0;
    case LB_GETCARETINDEX: return lp->iCaret;
    case LB_FINDSTRING: case LB_FINDSTRINGEXACT:
        if (!l || lp->cItem == 0) return LB_ERR;
        if (i < -1 || i >= lp->cItem) i = -1;
        for (j = 1; j <= lp->cItem; j++) {
            iNext = (int)(((int64_t)i + j) % lp->cItem);
            if ((msg == LB_FINDSTRINGEXACT ? SDL_strcasecmp(lp->rgitem[iNext].psz, (LPCSTR)l) :
                 SDL_strncasecmp(lp->rgitem[iNext].psz, (LPCSTR)l, strlen((LPCSTR)l))) == 0) return iNext;
        }
        return LB_ERR;
    case WM_GETDLGCODE: return DLGC_WANTARROWS | DLGC_WANTCHARS;
    case WM_GETFONT: return (LRESULT)lpwin->hfontControl;
    case WM_SETFONT:
        lpwin->hfontControl = (HFONT)w;
        if (!FOwnerList(hwnd) && (hdc = GetDC(hwnd))) {
            if (lpwin->hfontControl) SelectObject(hdc, lpwin->hfontControl);
            if (GetTextMetricsA(hdc, &tm)) lp->cyItem = max(1, tm.tmHeight);
            ReleaseDC(hwnd, hdc);
        }
        ClampList(hwnd); if (l) InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_PAINT: PaintList(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_ENABLE: case WM_SIZE: ClampList(hwnd); return 0;
    case WM_SETFOCUS: case WM_KILLFOCUS:
        DrawListFocus(hwnd, msg == WM_SETFOCUS);
        if (IsWindow(hwnd)) {
            InvalidateRect(hwnd, NULL, TRUE); NotifyList(hwnd, msg == WM_SETFOCUS ? LBN_SETFOCUS : LBN_KILLFOCUS);
        }
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        SetFocus(hwnd); if (!IsWindow(hwnd)) return 0;
        i = lp->iTop + (int16_t)HIWORD(l) / lp->cyItem;
        if ((int16_t)HIWORD(l) < 0) return 0;
        SDL_free(lp->rgSelectedDrag); lp->rgSelectedDrag = NULL; lp->cSelectedDrag = 0;
        if ((lpwin->dwStyle & LBS_EXTENDEDSEL) && (w & MK_CONTROL) && lp->cItem != 0) {
            lp->rgSelectedDrag = (BYTE *)(SDL_malloc((size_t)lp->cItem));
            if (lp->rgSelectedDrag) {
                lp->cSelectedDrag = lp->cItem;
                for (j = 0; j < lp->cItem; j++) lp->rgSelectedDrag[j] = (BYTE)lp->rgitem[j].fSelected;
            }
        }
        ExtendedSelectList(hwnd, i, (w & MK_CONTROL) != 0, (w & MK_SHIFT) != 0, FALSE);
        if (!IsWindow(hwnd)) return 0;
        if (msg == WM_LBUTTONDBLCLK) NotifyList(hwnd, LBN_DBLCLK);
        else {
            SetCapture(hwnd);
            if (IsWindow(hwnd) && GetCapture() == hwnd) {
                lpwin->fTracking = TRUE; lp->iDrag = i; lp->grfDrag = (DWORD)w; lp->lDragPoint = l;
                lp->idDragTimer = SetTimer(hwnd, UINTPTR_MAX, 100, NULL);
            }
        }
        return 0;
    case WM_MOUSEMOVE:
        if (lpwin->fTracking && GetCapture() == hwnd) { lp->lDragPoint = l; DragList(hwnd, l); }
        return 0;
    case WM_TIMER:
        if (w == lp->idDragTimer && lpwin->fTracking && GetCapture() == hwnd) DragList(hwnd, lp->lDragPoint);
        return 0;
    case WM_LBUTTONUP:
        c = lpwin->fTracking; EndListDrag(hwnd, TRUE);
        if (c != 0 && IsWindow(hwnd)) NotifyList(hwnd, LBN_SELCHANGE);
        return 0;
    case WM_CAPTURECHANGED: case WM_CANCELMODE:
        EndListDrag(hwnd, msg == WM_CANCELMODE);
        return 0;
    case WM_KEYDOWN:
        i = lp->iCaret;
        if (w == VK_UP) i--;
        else if (w == VK_DOWN) i++;
        else if (w == VK_HOME) i = 0;
        else if (w == VK_END) i = lp->cItem - 1;
        else if (w == VK_PRIOR) i -= CListPage(hwnd);
        else if (w == VK_NEXT) i += CListPage(hwnd);
        else if (w == VK_SPACE && FMulti(hwnd)) {
            ExtendedSelectList(hwnd, i, (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                               (GetKeyState(VK_SHIFT) & 0x8000) != 0, TRUE); return 0;
        }
        else return 0;
        i = min(max(0, i), lp->cItem - 1);
        if ((lpwin->dwStyle & LBS_EXTENDEDSEL) && !(GetKeyState(VK_CONTROL) & 0x8000))
            ExtendedSelectList(hwnd, i, FALSE, (GetKeyState(VK_SHIFT) & 0x8000) != 0, TRUE);
        else if (FMulti(hwnd)) { lp->iCaret = max(0, i); RevealList(hwnd, max(0, i)); }
        else UserSelectList(hwnd, i, FALSE, TRUE);
        return 0;
    case WM_CHAR:
        if (w < 32 || w > 255 || lp->cItem == 0) return 0;
        c = (int)strlen(lp->szSearch);
        if (SDL_GetTicks() - lp->msSearch > 1000 || c >= (int)sizeof(lp->szSearch) - 1 ||
            (c == 1 && (BYTE)lp->szSearch[0] == (BYTE)w)) c = 0;
        lp->szSearch[c] = (char)w; lp->szSearch[c + 1] = 0; lp->msSearch = SDL_GetTicks();
        i = (int)CompatListProc(hwnd, LB_FINDSTRING, c != 0 ? lp->iCaret - 1 : lp->iCaret, (LPARAM)lp->szSearch);
        if (i != LB_ERR) ExtendedSelectList(hwnd, i, FALSE, FALSE, TRUE);
        return 0;
    case WM_HSCROLL:
        GetClientRect(hwnd, &rc);
        if (LOWORD(w) == SB_LINELEFT) lp->xOffset -= 8;
        else if (LOWORD(w) == SB_LINERIGHT) lp->xOffset += 8;
        else if (LOWORD(w) == SB_PAGELEFT) lp->xOffset -= rc.right;
        else if (LOWORD(w) == SB_PAGERIGHT) lp->xOffset += rc.right;
        else if (LOWORD(w) == SB_LEFT) lp->xOffset = 0;
        else if (LOWORD(w) == SB_RIGHT) lp->xOffset = lp->cxExtent;
        else if (LOWORD(w) == SB_THUMBPOSITION || LOWORD(w) == SB_THUMBTRACK) {
            if (l == 0 && IsWindow(lpwin->rghwndScroll[SB_HORZ]) && lpwin->rghwndScroll[SB_HORZ]->fTracking)
                lp->xOffset = lpwin->rghwndScroll[SB_HORZ]->iScrollTrack;
            else lp->xOffset = HIWORD(w);
        }
        lp->xOffset = max(0, min(lp->xOffset, max(0, lp->cxExtent - rc.right)));
        ClampList(hwnd); return 0;
    case WM_VSCROLL:
        if (LOWORD(w) == SB_LINEUP) lp->iTop--;
        else if (LOWORD(w) == SB_LINEDOWN) lp->iTop++;
        else if (LOWORD(w) == SB_PAGEUP) lp->iTop -= CListPage(hwnd);
        else if (LOWORD(w) == SB_PAGEDOWN) lp->iTop += CListPage(hwnd);
        else if (LOWORD(w) == SB_TOP) lp->iTop = 0;
        else if (LOWORD(w) == SB_BOTTOM) lp->iTop = lp->cItem;
        else if (LOWORD(w) == SB_THUMBPOSITION || LOWORD(w) == SB_THUMBTRACK) {
            if (l == 0 && IsWindow(lpwin->rghwndScroll[SB_VERT]) && lpwin->rghwndScroll[SB_VERT]->fTracking)
                lp->iTop = lpwin->rghwndScroll[SB_VERT]->iScrollTrack;
            else lp->iTop = HIWORD(w);
        }
        ClampList(hwnd); return 0;
    case WM_MOUSEWHEEL:
        lp->iTop = (int)min((int64_t)INT_MAX, max((int64_t)0, (int64_t)lp->iTop - (int16_t)HIWORD(w) / WHEEL_DELTA * 3));
        ClampList(hwnd); return 0;
    default: return DefWindowProcA(hwnd, msg, w, l);
    }
}
