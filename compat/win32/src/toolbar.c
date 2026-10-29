/* Narrow common-control toolbar contracts used by the native Stars UI. */
#include "internal.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>

typedef struct COMPATTOOLBUTTON { TBBUTTON btn; RECT rc; char *psz; } COMPATTOOLBUTTON;
typedef struct COMPATTOOLBITMAP { HBITMAP hbmp; int iFirst, c; } COMPATTOOLBITMAP;
struct COMPATTOOLBAR {
    COMPATTOOLBUTTON *rgbtn;
    COMPATTOOLBITMAP *rgbmp;
    int c, cbmp, cImage, cx, cy, cxBitmap, cyBitmap, xIndent;
    int iDown, iHot, cTextRows;
    int idDrop;
    BOOL fDropPressed;
    DWORD dwExStyle;
    UINT cbButton;
    HWND hwndTip;
    ~COMPATTOOLBAR() noexcept {
        for (int i = 0; i < c; i++) SDL_free(rgbtn[i].psz);
        for (int i = 0; i < cbmp; i++) DeleteObject(rgbmp[i].hbmp);
        free(rgbtn); free(rgbmp);
    }
};

BOOL WINAPI InitCommonControlsEx(const INITCOMMONCONTROLSEX *lpInit) {
    if (!lpInit || lpInit->dwSize != sizeof(*lpInit) || (lpInit->dwICC & ~ICC_BAR_CLASSES)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

static void HideTip(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    if (lp->hwndTip) ShowWindow(lp->hwndTip, SW_HIDE);
    KillTimer(hwnd, 1);
}

void CompatDestroyToolbar(HWND hwnd) {
    auto lptoolbarKeep = std::move(hwnd->lptoolbar);
    if (lptoolbarKeep && IsWindow(lptoolbarKeep->hwndTip)) DestroyWindow(lptoolbarKeep->hwndTip);
}

static int IndexButton(COMPATTOOLBAR *lp, int id) {
    int i;
    for (i = 0; i < lp->c; i++) if (lp->rgbtn[i].btn.idCommand == id) return i;
    return -1;
}

static BOOL FArrow(COMPATTOOLBAR *lp, TBBUTTON *lpbtn) {
    return (lpbtn->fsStyle & BTNS_WHOLEDROPDOWN) ||
        ((lpbtn->fsStyle & BTNS_DROPDOWN) && (lp->dwExStyle & TBSTYLE_EX_DRAWDDARROWS));
}

static void LayoutToolbar(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBUTTON *lpbtn;
    int i, x = lp->xIndent, cx;
    for (i = 0; i < lp->c; i++) {
        lpbtn = &lp->rgbtn[i];
        cx = lpbtn->btn.fsStyle & BTNS_SEP ? StarsMax(1, lpbtn->btn.iBitmap) : lp->cx + (FArrow(lp, &lpbtn->btn) ? 11 : 0);
        if (lpbtn->btn.fsState & TBSTATE_HIDDEN) cx = 0;
        SetRect(&lpbtn->rc, x, 2, x + cx, 2 + lp->cy);
        x += cx;
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

static int HitToolbar(COMPATTOOLBAR *lp, POINT pt) {
    int i;
    for (i = 0; i < lp->c; i++) if (PtInRect(&lp->rgbtn[i].rc, pt))
        return lp->rgbtn[i].btn.fsStyle & BTNS_SEP ? -i - 1 : i;
    return -lp->c - 1;
}

static BOOL AddButtons(HWND hwnd, int iInsert, size_t c, const TBBUTTON *rgbtn) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBUTTON *rgNew;
    size_t i;
    if (!rgbtn || iInsert < 0 || iInsert > lp->c || c > 4096 || c + lp->c > 4096 ||
        lp->cbButton != sizeof(TBBUTTON)) return FALSE;
    rgNew = (COMPATTOOLBUTTON *)(calloc(c + (size_t)lp->c + 1, sizeof(*rgNew)));
    if (!rgNew) return FALSE;
    for (i = 0; i < c; i++) {
        rgNew[iInsert + i].btn = rgbtn[i];
        // This subset accepts direct UTF-8 text, not string pool/callback indices.
        if (rgbtn[i].iString > 65535) {
            rgNew[iInsert + i].psz = SDL_strdup((const char *)rgbtn[i].iString);
            if (!rgNew[iInsert + i].psz) break;
            rgNew[iInsert + i].btn.iString = (INT_PTR)rgNew[iInsert + i].psz;
        } else if (rgbtn[i].iString > 0) break;
    }
    if (i != c) {
        for (i = 0; i < c; i++) SDL_free(rgNew[iInsert + i].psz);
        free(rgNew); return FALSE;
    }
    if (iInsert != 0) memcpy(rgNew, lp->rgbtn, (size_t)iInsert * sizeof(*rgNew));
    if (lp->c != iInsert) memcpy(rgNew + iInsert + c, lp->rgbtn + iInsert, (size_t)(lp->c - iInsert) * sizeof(*rgNew));
    free(lp->rgbtn); lp->rgbtn = rgNew; lp->c += (int)c;
    lp->iDown = lp->iHot = -1;
    HideTip(hwnd); LayoutToolbar(hwnd);
    return TRUE;
}

static int AddBitmap(HWND hwnd, int c, const TBADDBITMAP *lpadd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBITMAP *rgbmp;
    HBITMAP hbmp, hcopy, hOldSrc, hOldDst;
    HDC hsrc, hdst;
    BITMAP bm;
    int iFirst;
    BOOL fCopy;
    if (!lpadd || c <= 0 || c > 4096 || c > 4096 - lp->cImage) return -1;
    hbmp = lpadd->hInst ? LoadBitmapA(lpadd->hInst, (LPCSTR)lpadd->nID) : (HBITMAP)lpadd->nID;
    if (!hbmp || !GetObjectA(hbmp, sizeof(bm), &bm) || bm.bmHeight < lp->cyBitmap ||
        bm.bmWidth / lp->cxBitmap < c) {
        if (lpadd->hInst && hbmp) DeleteObject(hbmp);
        return -1;
    }
    hsrc = CreateCompatibleDC(NULL); hdst = CreateCompatibleDC(NULL);
    hOldSrc = (HBITMAP)SelectObject(hsrc, hbmp);
    // A fresh memory DC contains a monochrome bitmap. Match the selected
    // source instead, or the copied toolbar icons lose all their colors.
    hcopy = CreateCompatibleBitmap(hsrc, bm.bmWidth, bm.bmHeight);
    hOldDst = (HBITMAP)SelectObject(hdst, hcopy);
    fCopy = hsrc && hdst && hcopy && BitBlt(hdst, 0, 0, bm.bmWidth, bm.bmHeight, hsrc, 0, 0, SRCCOPY);
    if (hOldSrc) SelectObject(hsrc, hOldSrc);
    if (hOldDst) SelectObject(hdst, hOldDst);
    DeleteDC(hsrc); DeleteDC(hdst);
    if (lpadd->hInst) DeleteObject(hbmp);
    if (!fCopy) { if (hcopy) DeleteObject(hcopy); return -1; }
    rgbmp = (COMPATTOOLBITMAP *)(realloc(lp->rgbmp, (size_t)(lp->cbmp + 1) * sizeof(*rgbmp)));
    if (!rgbmp) { DeleteObject(hcopy); return -1; }
    lp->rgbmp = rgbmp; iFirst = lp->cImage;
    lp->rgbmp[lp->cbmp++] = (COMPATTOOLBITMAP){hcopy, iFirst, c}; lp->cImage += c;
    InvalidateRect(hwnd, NULL, TRUE);
    return iFirst;
}

static void PaintToolbar(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    PAINTSTRUCT ps;
    HDC hdc, hsrc;
    HBITMAP hOld;
    COMPATTOOLBUTTON *lpbtn;
    HBRUSH hbr;
    RECT rc, rcImage, rcAction, rcArrow;
    int i, j, x, y, d, cx, xArrow;
    BOOL fDown, fEnabled, fSplit, fArrowDown;
    hdc = BeginPaint(hwnd, &ps);
    if (!hdc) return;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    for (i = 0; i < lp->c; i++) {
        lpbtn = &lp->rgbtn[i]; rc = lpbtn->rc;
        if (lpbtn->btn.fsState & TBSTATE_HIDDEN) continue;
        if (lpbtn->btn.fsStyle & BTNS_SEP) continue;
        fDown = (lpbtn->btn.fsState & (TBSTATE_CHECKED | TBSTATE_PRESSED)) != 0;
        fSplit = FArrow(lp, &lpbtn->btn) && (lpbtn->btn.fsStyle & BTNS_DROPDOWN) &&
            !(lpbtn->btn.fsStyle & BTNS_WHOLEDROPDOWN);
        fArrowDown = lp->fDropPressed && lp->idDrop == lpbtn->btn.idCommand;
        if (!fSplit && fArrowDown) fDown = TRUE;
        fEnabled = IsWindowEnabled(hwnd) && (lpbtn->btn.fsState & TBSTATE_ENABLED);
        rcAction = rc;
        if (fSplit) {
            rcAction.right -= 11; rcArrow = rc; rcArrow.left = rcAction.right;
            CompatDrawClassicEdge(hdc, &rcArrow, fArrowDown, TRUE);
        }
        // Split controls have two complete button frames, as in the classic
        // toolbar: the arrow's frame changes independently during its menu.
        CompatDrawClassicEdge(hdc, &rcAction, fDown, TRUE);
        cx = rc.right - rc.left - (FArrow(lp, &lpbtn->btn) ? 11 : 0);
        x = rc.left + (cx - lp->cxBitmap) / 2 + fDown;
        y = rc.top + (lp->cy - lp->cyBitmap) / 2 + fDown;
        for (j = 0; j < lp->cbmp; j++) {
            if (lpbtn->btn.iBitmap < lp->rgbmp[j].iFirst || lpbtn->btn.iBitmap >= lp->rgbmp[j].iFirst + lp->rgbmp[j].c) continue;
            hsrc = CreateCompatibleDC(hdc); hOld = (HBITMAP)SelectObject(hsrc, lp->rgbmp[j].hbmp);
            BitBlt(hdc, x, y, lp->cxBitmap, lp->cyBitmap, hsrc,
                   (lpbtn->btn.iBitmap - lp->rgbmp[j].iFirst) * lp->cxBitmap, 0, SRCCOPY);
            SelectObject(hsrc, hOld); DeleteDC(hsrc); break;
        }
        if (lp->cTextRows && lpbtn->psz) {
            SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, GetSysColor(fEnabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
            DrawTextA(hdc, lpbtn->psz, -1, &rcAction, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        if (FArrow(lp, &lpbtn->btn)) {
            xArrow = rc.right - 8 + (fSplit ? fArrowDown : fDown);
            y = (rc.top + rc.bottom) / 2 - 1 + (fSplit ? fArrowDown : fDown);
            hbr = CreateSolidBrush(GetSysColor(fEnabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
            for (d = 0; d < 3; d++) {
                SetRect(&rcImage, xArrow + d, y + d, xArrow + 5 - d, y + d + 1);
                FillRect(hdc, &rcImage, hbr);
            }
            DeleteObject(hbr);
        }
    }
    EndPaint(hwnd, &ps);
}

static void ClickToolbar(HWND hwnd, int i, BOOL fDrop) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    NMTOOLBARA nm = {};
    TBBUTTON btn;
    HWND hwndParent = hwnd->hwndParent;
    int j;
    LRESULT result;
    if (i < 0 || i >= lp->c) return;
    HideTip(hwnd);
    btn = lp->rgbtn[i].btn;
    if (!IsWindowEnabled(hwnd) || !(btn.fsState & TBSTATE_ENABLED) || (btn.fsStyle & BTNS_SEP)) return;
    if (fDrop) {
        nm.hdr = (NMHDR){hwnd, (UINT_PTR)hwnd->idChild, TBN_DROPDOWN};
        nm.iItem = btn.idCommand; nm.tbButton = btn; nm.rcButton = lp->rgbtn[i].rc;
        // Menu activation is independent of the split button's action/check
        // state, and remains visible throughout the parent's modal menu loop.
        StarsRestore<BOOL> restorePressed(lp->fDropPressed);
        lp->idDrop = btn.idCommand; lp->fDropPressed = TRUE;
        InvalidateRect(hwnd, NULL, TRUE); UpdateWindow(hwnd);
        result = SendMessageA(hwndParent, WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm);
        if (!IsWindow(hwnd) || !(lp = hwnd->lptoolbar.get())) return;
        lp->fDropPressed = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        i = IndexButton(lp, btn.idCommand);
        if (i < 0) return;
        if (result != TBDDRET_TREATPRESSED) return;
    }
    if (btn.fsStyle & BTNS_CHECK) {
        if (btn.fsStyle & BTNS_GROUP) {
            for (j = i; j > 0 && (lp->rgbtn[j - 1].btn.fsStyle & BTNS_GROUP); j--);
            for (; j < lp->c && (lp->rgbtn[j].btn.fsStyle & BTNS_GROUP); j++)
                lp->rgbtn[j].btn.fsState &= ~TBSTATE_CHECKED;
            lp->rgbtn[i].btn.fsState |= TBSTATE_CHECKED;
        } else lp->rgbtn[i].btn.fsState ^= TBSTATE_CHECKED;
    }
    InvalidateRect(hwnd, NULL, TRUE);
    SendMessageA(hwndParent, WM_COMMAND, MAKEWPARAM(btn.idCommand, 0), (LPARAM)hwnd);
}

LRESULT CALLBACK CompatToolbarTipProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr;
    RECT rc;
    if (msg == WM_PAINT) {
        hdc = BeginPaint(hwnd, &ps); GetClientRect(hwnd, &rc);
        hbr = CreateSolidBrush(GetSysColor(COLOR_INFOBK)); FillRect(hdc, &rc, hbr); DeleteObject(hbr);
        FrameRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        InflateRect(&rc, -3, -3); SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, GetSysColor(COLOR_INFOTEXT));
        DrawTextA(hdc, hwnd->pszTitle, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}

static void ShowToolbarTip(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBUTTON *lpbtn;
    RECT rc, rcRoot;
    POINT pt;
    SIZE size;
    HDC hdc;
    HWND hwndRoot;
    int x, y;
    if (lp->iHot < 0 || lp->iHot >= lp->c) return;
    lpbtn = &lp->rgbtn[lp->iHot]; rc = lpbtn->rc;
    MapWindowPoints(hwnd, NULL, (POINT *)&rc, 2);
    if (!GetCursorPos(&pt) || !PtInRect(&rc, pt)) { HideTip(hwnd); lp->iHot = -1; return; }
    if (IsWindowVisible(lp->hwndTip) || !lpbtn->psz || !*lpbtn->psz) return;
    hdc = GetDC(hwnd); GetTextExtentPoint32A(hdc, lpbtn->psz, (int)strlen(lpbtn->psz), &size); ReleaseDC(hwnd, hdc);
    hwndRoot = HwndCompatRoot(hwnd); GetWindowRect(hwndRoot, &rcRoot);
    x = (((rcRoot.left) > (StarsMin(rc.left, rcRoot.right - size.cx - 6))) ? (rcRoot.left) : (StarsMin(rc.left, rcRoot.right - size.cx - 6))); y = rc.bottom + 2;
    if (y + size.cy + 6 > rcRoot.bottom) y = StarsMax(rcRoot.top, rc.top - size.cy - 8);
    if (lp->hwndTip) {
        SetWindowTextA(lp->hwndTip, lpbtn->psz);
        SetWindowPos(lp->hwndTip, HWND_TOP, x, y, size.cx + 6, size.cy + 6, SWP_NOACTIVATE);
        UpdateWindow(lp->hwndTip);
        SetWindowPos(lp->hwndTip, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        SetTimer(hwnd, 1, 50, NULL);
    }
}

LRESULT CALLBACK CompatToolbarProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
    int i, j, id, step;
    BYTE mask;
    BOOL fDrop;
    BYTE state;
    RECT rc;
    TBBUTTON *lpbtn;
    if (msg == WM_NCCREATE) {
        try { lptoolbarKeep = std::make_shared<COMPATTOOLBAR>(); }
        catch (const std::bad_alloc &) { return FALSE; }
        lp = lptoolbarKeep.get();
        hwnd->lptoolbar = lptoolbarKeep; hwnd->iControl = 7;
        lp->cx = 23; lp->cy = 22; lp->cxBitmap = 16; lp->cyBitmap = 15;
        lp->iDown = lp->iHot = -1; lp->cTextRows = 1;
        return TRUE;
    }
    if (!lp) return DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case WM_CREATE:
        if (hwnd->dwStyle & TBSTYLE_TOOLTIPS) {
            GetWindowRect(HwndCompatRoot(hwnd), &rc);
            lp->hwndTip = CreateWindowExA(WS_EX_NOACTIVATE, "StarsCompatToolbarTip", "", WS_POPUP,
                rc.left, rc.top, 1, 1, HwndCompatRoot(hwnd), NULL, hwnd->lpclass->wc.hInstance, NULL);
            if (!lp->hwndTip) return -1;
        }
        return 0;
    case WM_NCDESTROY: CompatDestroyToolbar(hwnd); return 0;
    case WM_PAINT: PaintToolbar(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case TB_BUTTONSTRUCTSIZE: lp->cbButton = (UINT)w; return 0;
    case TB_ADDBUTTONSA: return AddButtons(hwnd, lp->c, (size_t)w, (const TBBUTTON *)l);
    case TB_INSERTBUTTONA: return AddButtons(hwnd, ((((int)w) < (lp->c)) ? ((int)w) : (lp->c)), 1, (const TBBUTTON *)l);
    case TB_DELETEBUTTON:
        i = (int)w; if (i < 0 || i >= lp->c) return FALSE;
        SDL_free(lp->rgbtn[i].psz); memmove(lp->rgbtn + i, lp->rgbtn + i + 1, (size_t)(lp->c - i - 1) * sizeof(*lp->rgbtn));
        lp->c--; lp->iDown = lp->iHot = -1; HideTip(hwnd); LayoutToolbar(hwnd); return TRUE;
    case TB_BUTTONCOUNT: return lp->c;
    case TB_COMMANDTOINDEX: return IndexButton(lp, (int)w);
    case TB_GETBUTTON:
        if (!l || (int)w < 0 || (int)w >= lp->c) return FALSE;
        *(TBBUTTON *)l = lp->rgbtn[w].btn; return TRUE;
    case TB_GETITEMRECT: i = (int)w; goto LRect;
    case TB_GETRECT: i = IndexButton(lp, (int)w);
    LRect:
        if (!l || i < 0 || i >= lp->c) return FALSE;
        *(RECT *)l = lp->rgbtn[i].rc; return TRUE;
    case TB_HITTEST: return l ? HitToolbar(lp, *(POINT *)l) : -1;
    case TB_ADDBITMAP: return AddBitmap(hwnd, (int)w, (const TBADDBITMAP *)l);
    case TB_SETBUTTONSIZE: case TB_SETBITMAPSIZE:
        if (!LOWORD(l) || !HIWORD(l) || LOWORD(l) > 4096 || HIWORD(l) > 4096) return FALSE;
        if (msg == TB_SETBUTTONSIZE) { lp->cx = LOWORD(l); lp->cy = HIWORD(l); }
        else { lp->cxBitmap = LOWORD(l); lp->cyBitmap = HIWORD(l); }
        LayoutToolbar(hwnd); return TRUE;
    case TB_GETBUTTONSIZE: return MAKELONG(lp->cx, lp->cy);
    case TB_SETINDENT: if (w > 4096) return FALSE; lp->xIndent = (int)w; LayoutToolbar(hwnd); return TRUE;
    case TB_SETMAXTEXTROWS: lp->cTextRows = (int)w; InvalidateRect(hwnd, NULL, TRUE); return TRUE;
    case TB_GETTEXTROWS: return lp->cTextRows;
    case TB_SETEXTENDEDSTYLE: id = (int)lp->dwExStyle; lp->dwExStyle = (DWORD)l; LayoutToolbar(hwnd); return id;
    case TB_GETEXTENDEDSTYLE: return lp->dwExStyle;
    case TB_AUTOSIZE:
        if (!(hwnd->dwStyle & CCS_NORESIZE)) {
            GetClientRect(hwnd->hwndParent, &rc);
            SetWindowPos(hwnd, NULL, 0, 0, rc.right, lp->cy + 4, SWP_NOZORDER | SWP_NOACTIVATE |
                         (hwnd->dwStyle & CCS_NOPARENTALIGN ? SWP_NOMOVE : 0));
        }
        return 0;
    case TB_SETSTATE: case TB_GETSTATE: case TB_GETBITMAP: case TB_GETBUTTONTEXTA:
        i = IndexButton(lp, (int)w); if (i < 0) return -1;
        if (msg == TB_GETSTATE) return lp->rgbtn[i].btn.fsState;
        if (msg == TB_GETBITMAP) return lp->rgbtn[i].btn.iBitmap;
        if (msg == TB_GETBUTTONTEXTA) {
            if (!lp->rgbtn[i].psz) return -1;
            if (l) strcpy((char *)l, lp->rgbtn[i].psz);
            return (LRESULT)strlen(lp->rgbtn[i].psz);
        }
        if (lp->rgbtn[i].btn.fsState != (BYTE)l) {
            lp->rgbtn[i].btn.fsState = (BYTE)l; LayoutToolbar(hwnd);
        }
        return TRUE;
    case TB_CHECKBUTTON: case TB_ENABLEBUTTON: case TB_PRESSBUTTON: case TB_HIDEBUTTON:
    case TB_ISBUTTONCHECKED: case TB_ISBUTTONENABLED: case TB_ISBUTTONPRESSED: case TB_ISBUTTONHIDDEN:
        i = IndexButton(lp, (int)w); if (i < 0) return FALSE;
        mask = msg == TB_CHECKBUTTON || msg == TB_ISBUTTONCHECKED ? TBSTATE_CHECKED :
               msg == TB_ENABLEBUTTON || msg == TB_ISBUTTONENABLED ? TBSTATE_ENABLED :
               msg == TB_PRESSBUTTON || msg == TB_ISBUTTONPRESSED ? TBSTATE_PRESSED : TBSTATE_HIDDEN;
        if (msg >= TB_ISBUTTONENABLED) return (lp->rgbtn[i].btn.fsState & mask) != 0;
        state = lp->rgbtn[i].btn.fsState;
        if (l) lp->rgbtn[i].btn.fsState |= mask; else lp->rgbtn[i].btn.fsState &= ~mask;
        if (state != lp->rgbtn[i].btn.fsState) LayoutToolbar(hwnd);
        return TRUE;
    case TB_GETHOTITEM: return lp->iHot;
    case TB_SETHOTITEM: i = lp->iHot; lp->iHot = (int)w < lp->c ? (int)w : -1; return i;
    case TB_GETTOOLTIPS: return (LRESULT)lp->hwndTip;
    case WM_TIMER: if (w == 1) ShowToolbarTip(hwnd); return 0;
    case WM_MOUSEMOVE:
        i = HitToolbar(lp, pt);
        if (i != lp->iHot) {
            HideTip(hwnd); lp->iHot = i >= 0 ? i : -1;
            if ((hwnd->dwStyle & TBSTYLE_TOOLTIPS) && lp->iHot >= 0) SetTimer(hwnd, 1, 700, NULL);
        }
        if (lp->iDown >= 0 && lp->iDown < lp->c) {
            if (i == lp->iDown) lp->rgbtn[lp->iDown].btn.fsState |= TBSTATE_PRESSED;
            else lp->rgbtn[lp->iDown].btn.fsState &= ~TBSTATE_PRESSED;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        HideTip(hwnd); i = HitToolbar(lp, pt);
        if (i < 0 || !IsWindowEnabled(hwnd) || !(lp->rgbtn[i].btn.fsState & TBSTATE_ENABLED)) return 0;
        lpbtn = &lp->rgbtn[i].btn;
        fDrop = (lpbtn->fsStyle & BTNS_WHOLEDROPDOWN) || ((lpbtn->fsStyle & BTNS_DROPDOWN) &&
            (!(lp->dwExStyle & TBSTYLE_EX_DRAWDDARROWS) || pt.x >= lp->rgbtn[i].rc.right - 11));
        if (fDrop) { ClickToolbar(hwnd, i, TRUE); return 0; }
        lp->iDown = i; lpbtn->fsState |= TBSTATE_PRESSED; SetCapture(hwnd); InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_LBUTTONUP:
        i = lp->iDown; lp->iDown = -1;
        if (i >= 0 && i < lp->c) lp->rgbtn[i].btn.fsState &= ~TBSTATE_PRESSED;
        ReleaseCapture(); if (!IsWindow(hwnd)) return 0;
        InvalidateRect(hwnd, NULL, TRUE);
        if (i >= 0 && HitToolbar(lp, pt) == i) ClickToolbar(hwnd, i, FALSE);
        return 0;
    case WM_CANCELMODE: case WM_CAPTURECHANGED:
        if (lp->iDown >= 0 && lp->iDown < lp->c) lp->rgbtn[lp->iDown].btn.fsState &= ~TBSTATE_PRESSED;
        lp->iDown = -1; HideTip(hwnd); InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_KEYDOWN:
        i = lp->iHot;
        if (w == VK_RETURN || w == VK_SPACE || (w == VK_DOWN && i >= 0 &&
            (lp->rgbtn[i].btn.fsStyle & (BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN)))) {
            if (i >= 0) ClickToolbar(hwnd, i, (lp->rgbtn[i].btn.fsStyle & BTNS_WHOLEDROPDOWN) || w == VK_DOWN);
        } else if (w == VK_LEFT || w == VK_RIGHT) {
            step = w == VK_LEFT ? -1 : 1;
            for (j = 0; j < lp->c; j++) {
                i = (i + step + lp->c) % lp->c;
                if (!(lp->rgbtn[i].btn.fsStyle & BTNS_SEP) &&
                    (lp->rgbtn[i].btn.fsState & (TBSTATE_ENABLED | TBSTATE_HIDDEN)) == TBSTATE_ENABLED) { lp->iHot = i; break; }
            }
        }
        return 0;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
