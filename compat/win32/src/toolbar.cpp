/* Narrow common-control toolbar contracts used by the native Stars UI. */
#include "internal.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>

struct COMPATTOOLBUTTON { TBBUTTON btn; RECT rc; char *psz; };
struct COMPATTOOLBITMAP { HBITMAP hbmp; int iFirst, c; };
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
        for (int i = 0; i < cbmp; i++) win32::DeleteObject(rgbmp[i].hbmp);
        free(rgbtn); free(rgbmp);
    }
};

BOOL win32::InitCommonControlsEx(const INITCOMMONCONTROLSEX *lpInit) {
    if (!lpInit || lpInit->dwSize != sizeof(*lpInit) || (lpInit->dwICC & ~win32::ICC_BAR_CLASSES)) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    return 1;
}

static void HideTip(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    if (lp->hwndTip) win32::ShowWindow(lp->hwndTip, win32::SW_HIDE);
    win32::KillTimer(hwnd, 1);
}

void CompatDestroyToolbar(HWND hwnd) {
    auto lptoolbarKeep = std::move(hwnd->lptoolbar);
    if (lptoolbarKeep && win32::IsWindow(lptoolbarKeep->hwndTip)) win32::DestroyWindow(lptoolbarKeep->hwndTip);
}

static int IndexButton(COMPATTOOLBAR *lp, int id) {
    int i;
    for (i = 0; i < lp->c; i++) if (lp->rgbtn[i].btn.idCommand == id) return i;
    return -1;
}

static BOOL FArrow(COMPATTOOLBAR *lp, TBBUTTON *lpbtn) {
    return (lpbtn->fsStyle & win32::BTNS_WHOLEDROPDOWN) ||
        ((lpbtn->fsStyle & win32::BTNS_DROPDOWN) && (lp->dwExStyle & win32::TBSTYLE_EX_DRAWDDARROWS));
}

static void LayoutToolbar(HWND hwnd) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBUTTON *lpbtn;
    int i, x = lp->xIndent, cx;
    for (i = 0; i < lp->c; i++) {
        lpbtn = &lp->rgbtn[i];
        cx = lpbtn->btn.fsStyle & win32::BTNS_SEP ? StarsMax(1, lpbtn->btn.iBitmap) : lp->cx + (FArrow(lp, &lpbtn->btn) ? 11 : 0);
        if (lpbtn->btn.fsState & win32::TBSTATE_HIDDEN) cx = 0;
        win32::SetRect(&lpbtn->rc, x, 2, x + cx, 2 + lp->cy);
        x += cx;
    }
    win32::InvalidateRect(hwnd, nullptr, 1);
}

static int HitToolbar(COMPATTOOLBAR *lp, POINT pt) {
    int i;
    for (i = 0; i < lp->c; i++) if (win32::PtInRect(&lp->rgbtn[i].rc, pt))
        return lp->rgbtn[i].btn.fsStyle & win32::BTNS_SEP ? -i - 1 : i;
    return -lp->c - 1;
}

static BOOL AddButtons(HWND hwnd, int iInsert, size_t c, const TBBUTTON *rgbtn) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    COMPATTOOLBUTTON *rgNew;
    size_t i;
    if (!rgbtn || iInsert < 0 || iInsert > lp->c || c > 4096 || c + lp->c > 4096 ||
        lp->cbButton != sizeof(TBBUTTON)) return 0;
    rgNew = static_cast<COMPATTOOLBUTTON *>((calloc(c + static_cast<size_t>(lp->c) + 1, sizeof(*rgNew))));
    if (!rgNew) return 0;
    for (i = 0; i < c; i++) {
        rgNew[iInsert + i].btn = rgbtn[i];
        // This subset accepts direct UTF-8 text, not string pool/callback indices.
        if (rgbtn[i].iString > 65535) {
            rgNew[iInsert + i].psz = SDL_strdup(reinterpret_cast<const char *>(rgbtn[i].iString));
            if (!rgNew[iInsert + i].psz) break;
            rgNew[iInsert + i].btn.iString = reinterpret_cast<INT_PTR>(rgNew[iInsert + i].psz);
        } else if (rgbtn[i].iString > 0) break;
    }
    if (i != c) {
        for (i = 0; i < c; i++) SDL_free(rgNew[iInsert + i].psz);
        free(rgNew); return 0;
    }
    if (iInsert != 0) memcpy(rgNew, lp->rgbtn, static_cast<size_t>(iInsert) * sizeof(*rgNew));
    if (lp->c != iInsert) memcpy(rgNew + iInsert + c, lp->rgbtn + iInsert, static_cast<size_t>((lp->c - iInsert)) * sizeof(*rgNew));
    free(lp->rgbtn); lp->rgbtn = rgNew; lp->c += static_cast<int>(c);
    lp->iDown = lp->iHot = -1;
    HideTip(hwnd); LayoutToolbar(hwnd);
    return 1;
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
    hbmp = lpadd->hInst ? win32::LoadBitmapA(lpadd->hInst, reinterpret_cast<LPCSTR>(lpadd->nID)) : reinterpret_cast<HBITMAP>(lpadd->nID);
    if (!hbmp || !win32::GetObjectA(hbmp, sizeof(bm), &bm) || bm.bmHeight < lp->cyBitmap ||
        bm.bmWidth / lp->cxBitmap < c) {
        if (lpadd->hInst && hbmp) win32::DeleteObject(hbmp);
        return -1;
    }
    hsrc = win32::CreateCompatibleDC(nullptr); hdst = win32::CreateCompatibleDC(nullptr);
    hOldSrc = static_cast<HBITMAP>(win32::SelectObject(hsrc, hbmp));
    // A fresh memory DC contains a monochrome bitmap. Match the selected
    // source instead, or the copied toolbar icons lose all their colors.
    hcopy = win32::CreateCompatibleBitmap(hsrc, bm.bmWidth, bm.bmHeight);
    hOldDst = static_cast<HBITMAP>(win32::SelectObject(hdst, hcopy));
    fCopy = hsrc && hdst && hcopy && win32::BitBlt(hdst, 0, 0, bm.bmWidth, bm.bmHeight, hsrc, 0, 0, win32::SRCCOPY);
    if (hOldSrc) win32::SelectObject(hsrc, hOldSrc);
    if (hOldDst) win32::SelectObject(hdst, hOldDst);
    win32::DeleteDC(hsrc); win32::DeleteDC(hdst);
    if (lpadd->hInst) win32::DeleteObject(hbmp);
    if (!fCopy) { if (hcopy) win32::DeleteObject(hcopy); return -1; }
    rgbmp = static_cast<COMPATTOOLBITMAP *>((realloc(lp->rgbmp, static_cast<size_t>((lp->cbmp + 1)) * sizeof(*rgbmp))));
    if (!rgbmp) { win32::DeleteObject(hcopy); return -1; }
    lp->rgbmp = rgbmp; iFirst = lp->cImage;
    lp->rgbmp[lp->cbmp++] = COMPATTOOLBITMAP{hcopy, iFirst, c}; lp->cImage += c;
    win32::InvalidateRect(hwnd, nullptr, 1);
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
    hdc = win32::BeginPaint(hwnd, &ps);
    if (!hdc) return;
    win32::GetClientRect(hwnd, &rc);
    win32::FillRect(hdc, &rc, static_cast<HBRUSH>(win32::GetStockObject(win32::LTGRAY_BRUSH)));
    for (i = 0; i < lp->c; i++) {
        lpbtn = &lp->rgbtn[i]; rc = lpbtn->rc;
        if (lpbtn->btn.fsState & win32::TBSTATE_HIDDEN) continue;
        if (lpbtn->btn.fsStyle & win32::BTNS_SEP) continue;
        fDown = (lpbtn->btn.fsState & (win32::TBSTATE_CHECKED | win32::TBSTATE_PRESSED)) != 0;
        fSplit = FArrow(lp, &lpbtn->btn) && (lpbtn->btn.fsStyle & win32::BTNS_DROPDOWN) &&
            !(lpbtn->btn.fsStyle & win32::BTNS_WHOLEDROPDOWN);
        fArrowDown = lp->fDropPressed && lp->idDrop == lpbtn->btn.idCommand;
        if (!fSplit && fArrowDown) fDown = 1;
        fEnabled = win32::IsWindowEnabled(hwnd) && (lpbtn->btn.fsState & win32::TBSTATE_ENABLED);
        rcAction = rc;
        if (fSplit) {
            rcAction.right -= 11; rcArrow = rc; rcArrow.left = rcAction.right;
            CompatDrawClassicEdge(hdc, &rcArrow, fArrowDown, 1);
        }
        // Split controls have two complete button frames, as in the classic
        // toolbar: the arrow's frame changes independently during its menu.
        CompatDrawClassicEdge(hdc, &rcAction, fDown, 1);
        cx = rc.right - rc.left - (FArrow(lp, &lpbtn->btn) ? 11 : 0);
        x = rc.left + (cx - lp->cxBitmap) / 2 + fDown;
        y = rc.top + (lp->cy - lp->cyBitmap) / 2 + fDown;
        for (j = 0; j < lp->cbmp; j++) {
            if (lpbtn->btn.iBitmap < lp->rgbmp[j].iFirst || lpbtn->btn.iBitmap >= lp->rgbmp[j].iFirst + lp->rgbmp[j].c) continue;
            hsrc = win32::CreateCompatibleDC(hdc); hOld = static_cast<HBITMAP>(win32::SelectObject(hsrc, lp->rgbmp[j].hbmp));
            win32::BitBlt(hdc, x, y, lp->cxBitmap, lp->cyBitmap, hsrc,
                   (lpbtn->btn.iBitmap - lp->rgbmp[j].iFirst) * lp->cxBitmap, 0, win32::SRCCOPY);
            win32::SelectObject(hsrc, hOld); win32::DeleteDC(hsrc); break;
        }
        if (lp->cTextRows && lpbtn->psz) {
            win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, win32::GetSysColor(fEnabled ? win32::COLOR_BTNTEXT : win32::COLOR_GRAYTEXT));
            win32::DrawTextA(hdc, lpbtn->psz, -1, &rcAction, win32::DT_CENTER | win32::DT_VCENTER | win32::DT_SINGLELINE);
        }
        if (FArrow(lp, &lpbtn->btn)) {
            xArrow = rc.right - 8 + (fSplit ? fArrowDown : fDown);
            y = (rc.top + rc.bottom) / 2 - 1 + (fSplit ? fArrowDown : fDown);
            hbr = win32::CreateSolidBrush(win32::GetSysColor(fEnabled ? win32::COLOR_BTNTEXT : win32::COLOR_GRAYTEXT));
            for (d = 0; d < 3; d++) {
                win32::SetRect(&rcImage, xArrow + d, y + d, xArrow + 5 - d, y + d + 1);
                win32::FillRect(hdc, &rcImage, hbr);
            }
            win32::DeleteObject(hbr);
        }
    }
    win32::EndPaint(hwnd, &ps);
}

static void ClickToolbar(HWND hwnd, int i, BOOL fDrop, BOOL fMouse) {
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
    if (!win32::IsWindowEnabled(hwnd) || !(btn.fsState & win32::TBSTATE_ENABLED) || (btn.fsStyle & win32::BTNS_SEP)) return;
    if (fDrop) {
        nm.hdr = NMHDR{hwnd, static_cast<UINT_PTR>(hwnd->idChild), win32::TBN_DROPDOWN};
        nm.iItem = btn.idCommand; nm.tbButton = btn; nm.rcButton = lp->rgbtn[i].rc;
        // Menu activation is independent of the split button's action/check
        // state, and remains visible throughout the parent's modal menu loop.
        StarsRestore<BOOL> restorePressed(lp->fDropPressed);
        lp->idDrop = btn.idCommand; lp->fDropPressed = 1;
        win32::InvalidateRect(hwnd, nullptr, 1); win32::UpdateWindow(hwnd);
        result = win32::SendMessageA(hwndParent, win32::WM_NOTIFY, nm.hdr.idFrom, reinterpret_cast<LPARAM>(&nm));
        if (!win32::IsWindow(hwnd) || !(lp = hwnd->lptoolbar.get())) return;
        lp->fDropPressed = 0;
        win32::InvalidateRect(hwnd, nullptr, 1);
        i = IndexButton(lp, btn.idCommand);
        if (i < 0) return;
        if (result != win32::TBDDRET_NODEFAULT && result != win32::TBDDRET_TREATPRESSED) return;
        // An unhandled mouse dropdown becomes an ordinary button press;
        // Windows sends its command on release, not during TBN_DROPDOWN.
        if (fMouse) {
            lp->iDown = i; lp->rgbtn[i].btn.fsState |= win32::TBSTATE_PRESSED;
            win32::SetCapture(hwnd); win32::InvalidateRect(hwnd, nullptr, 1); return;
        }
    }
    if (btn.fsStyle & win32::BTNS_CHECK) {
        if (btn.fsStyle & win32::BTNS_GROUP) {
            for (j = i; j > 0 && (lp->rgbtn[j - 1].btn.fsStyle & win32::BTNS_GROUP); j--);
            for (; j < lp->c && (lp->rgbtn[j].btn.fsStyle & win32::BTNS_GROUP); j++)
                lp->rgbtn[j].btn.fsState &= ~win32::TBSTATE_CHECKED;
            lp->rgbtn[i].btn.fsState |= win32::TBSTATE_CHECKED;
        } else lp->rgbtn[i].btn.fsState ^= win32::TBSTATE_CHECKED;
    }
    win32::InvalidateRect(hwnd, nullptr, 1);
    win32::SendMessageA(hwndParent, win32::WM_COMMAND, win32::MAKEWPARAM(btn.idCommand, 0), reinterpret_cast<LPARAM>(hwnd));
}

LRESULT  CompatToolbarTipProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr;
    RECT rc;
    if (msg == win32::WM_PAINT) {
        hdc = win32::BeginPaint(hwnd, &ps); win32::GetClientRect(hwnd, &rc);
        hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_INFOBK)); win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr);
        win32::FrameRect(hdc, &rc, static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH)));
        win32::InflateRect(&rc, -3, -3); win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_INFOTEXT));
        win32::DrawTextA(hdc, hwnd->pszTitle, -1, &rc, win32::DT_LEFT | win32::DT_SINGLELINE | win32::DT_NOPREFIX);
        win32::EndPaint(hwnd, &ps); return 0;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
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
    win32::MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT *>(&rc), 2);
    if (!win32::GetCursorPos(&pt) || !win32::PtInRect(&rc, pt)) { HideTip(hwnd); lp->iHot = -1; return; }
    if (win32::IsWindowVisible(lp->hwndTip) || !lpbtn->psz || !*lpbtn->psz) return;
    hdc = win32::GetDC(hwnd); win32::GetTextExtentPoint32A(hdc, lpbtn->psz, static_cast<int>(strlen(lpbtn->psz)), &size); win32::ReleaseDC(hwnd, hdc);
    hwndRoot = HwndCompatRoot(hwnd); win32::GetWindowRect(hwndRoot, &rcRoot);
    x = (((rcRoot.left) > (StarsMin(rc.left, rcRoot.right - size.cx - 6))) ? (rcRoot.left) : (StarsMin(rc.left, rcRoot.right - size.cx - 6))); y = rc.bottom + 2;
    if (y + size.cy + 6 > rcRoot.bottom) y = StarsMax(rcRoot.top, rc.top - size.cy - 8);
    if (lp->hwndTip) {
        win32::SetWindowTextA(lp->hwndTip, lpbtn->psz);
        win32::SetWindowPos(lp->hwndTip, win32::HWND_TOP, x, y, size.cx + 6, size.cy + 6, win32::SWP_NOACTIVATE);
        win32::UpdateWindow(lp->hwndTip);
        win32::SetWindowPos(lp->hwndTip, nullptr, 0, 0, 0, 0,
                     win32::SWP_NOMOVE | win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW);
        win32::SetTimer(hwnd, 1, 50, nullptr);
    }
}

LRESULT  CompatToolbarProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto lptoolbarKeep = hwnd->lptoolbar;
    COMPATTOOLBAR *lp = lptoolbarKeep.get();
    POINT pt = {static_cast<int16_t>(LOWORD(l)), static_cast<int16_t>(HIWORD(l))};
    int i, j, id, step;
    BYTE mask;
    BOOL fDrop;
    BYTE state;
    RECT rc;
    TBBUTTON *lpbtn;
    if (msg == win32::WM_NCCREATE) {
        try { lptoolbarKeep = std::make_shared<COMPATTOOLBAR>(); }
        catch (const std::bad_alloc &) { return 0; }
        lp = lptoolbarKeep.get();
        hwnd->lptoolbar = lptoolbarKeep; hwnd->iControl = 7;
        lp->cx = 23; lp->cy = 22; lp->cxBitmap = 16; lp->cyBitmap = 15;
        lp->iDown = lp->iHot = -1; lp->cTextRows = 1;
        return 1;
    }
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case win32::WM_CREATE:
        if (hwnd->dwStyle & win32::TBSTYLE_TOOLTIPS) {
            win32::GetWindowRect(HwndCompatRoot(hwnd), &rc);
            lp->hwndTip = win32::CreateWindowExA(win32::WS_EX_NOACTIVATE, "StarsCompatToolbarTip", "", win32::WS_POPUP,
                rc.left, rc.top, 1, 1, HwndCompatRoot(hwnd), nullptr, hwnd->lpclass->wc.hInstance, nullptr);
            if (!lp->hwndTip) return -1;
        }
        return 0;
    case win32::WM_NCDESTROY: CompatDestroyToolbar(hwnd); return 0;
    case win32::WM_PAINT: PaintToolbar(hwnd); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::TB_BUTTONSTRUCTSIZE: lp->cbButton = static_cast<UINT>(w); return 0;
    case win32::TB_ADDBUTTONSA: return AddButtons(hwnd, lp->c, static_cast<size_t>(w), reinterpret_cast<const TBBUTTON *>(l));
    case win32::TB_INSERTBUTTONA: return AddButtons(hwnd, (((static_cast<int>(w)) < (lp->c)) ? (static_cast<int>(w)) : (lp->c)), 1, reinterpret_cast<const TBBUTTON *>(l));
    case win32::TB_DELETEBUTTON:
        i = static_cast<int>(w); if (i < 0 || i >= lp->c) return 0;
        SDL_free(lp->rgbtn[i].psz); memmove(lp->rgbtn + i, lp->rgbtn + i + 1, static_cast<size_t>((lp->c - i - 1)) * sizeof(*lp->rgbtn));
        lp->c--; lp->iDown = lp->iHot = -1; HideTip(hwnd); LayoutToolbar(hwnd); return 1;
    case win32::TB_BUTTONCOUNT: return lp->c;
    case win32::TB_COMMANDTOINDEX: return IndexButton(lp, static_cast<int>(w));
    case win32::TB_GETBUTTON:
        if (!l || static_cast<int>(w) < 0 || static_cast<int>(w) >= lp->c) return 0;
        *reinterpret_cast<TBBUTTON *>(l) = lp->rgbtn[w].btn; return 1;
    case win32::TB_GETITEMRECT: i = static_cast<int>(w); goto LRect;
    case win32::TB_GETRECT: i = IndexButton(lp, static_cast<int>(w));
    LRect:
        if (!l || i < 0 || i >= lp->c) return 0;
        *reinterpret_cast<RECT *>(l) = lp->rgbtn[i].rc; return 1;
    case win32::TB_HITTEST: return l ? HitToolbar(lp, *reinterpret_cast<POINT *>(l)) : -1;
    case win32::TB_ADDBITMAP: return AddBitmap(hwnd, static_cast<int>(w), reinterpret_cast<const TBADDBITMAP *>(l));
    case win32::TB_SETBUTTONSIZE: case win32::TB_SETBITMAPSIZE:
        if (!LOWORD(l) || !HIWORD(l) || LOWORD(l) > 4096 || HIWORD(l) > 4096) return 0;
        if (msg == win32::TB_SETBUTTONSIZE) { lp->cx = LOWORD(l); lp->cy = HIWORD(l); }
        else { lp->cxBitmap = LOWORD(l); lp->cyBitmap = HIWORD(l); }
        LayoutToolbar(hwnd); return 1;
    case win32::TB_GETBUTTONSIZE: return MAKELONG(lp->cx, lp->cy);
    case win32::TB_SETINDENT: if (w > 4096) return 0; lp->xIndent = static_cast<int>(w); LayoutToolbar(hwnd); return 1;
    case win32::TB_SETMAXTEXTROWS: lp->cTextRows = static_cast<int>(w); win32::InvalidateRect(hwnd, nullptr, 1); return 1;
    case win32::TB_GETTEXTROWS: return lp->cTextRows;
    case win32::TB_SETEXTENDEDSTYLE: id = static_cast<int>(lp->dwExStyle); lp->dwExStyle = static_cast<DWORD>(l); LayoutToolbar(hwnd); return id;
    case win32::TB_GETEXTENDEDSTYLE: return lp->dwExStyle;
    case win32::TB_AUTOSIZE:
        if (!(hwnd->dwStyle & win32::CCS_NORESIZE)) {
            win32::GetClientRect(hwnd->hwndParent, &rc);
            win32::SetWindowPos(hwnd, nullptr, 0, 0, rc.right, lp->cy + 4, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE |
                         (hwnd->dwStyle & win32::CCS_NOPARENTALIGN ? win32::SWP_NOMOVE : 0));
        }
        return 0;
    case win32::TB_SETSTATE: case win32::TB_GETSTATE: case win32::TB_GETBITMAP: case win32::TB_GETBUTTONTEXTA:
        i = IndexButton(lp, static_cast<int>(w)); if (i < 0) return -1;
        if (msg == win32::TB_GETSTATE) return lp->rgbtn[i].btn.fsState;
        if (msg == win32::TB_GETBITMAP) return lp->rgbtn[i].btn.iBitmap;
        if (msg == win32::TB_GETBUTTONTEXTA) {
            if (!lp->rgbtn[i].psz) return -1;
            if (l) strcpy(reinterpret_cast<char *>(l), lp->rgbtn[i].psz);
            return static_cast<LRESULT>(strlen(lp->rgbtn[i].psz));
        }
        if (lp->rgbtn[i].btn.fsState != static_cast<BYTE>(l)) {
            lp->rgbtn[i].btn.fsState = static_cast<BYTE>(l); LayoutToolbar(hwnd);
        }
        return 1;
    case win32::TB_CHECKBUTTON: case win32::TB_ENABLEBUTTON: case win32::TB_PRESSBUTTON: case win32::TB_HIDEBUTTON:
    case win32::TB_ISBUTTONCHECKED: case win32::TB_ISBUTTONENABLED: case win32::TB_ISBUTTONPRESSED: case win32::TB_ISBUTTONHIDDEN:
        i = IndexButton(lp, static_cast<int>(w)); if (i < 0) return 0;
        mask = msg == win32::TB_CHECKBUTTON || msg == win32::TB_ISBUTTONCHECKED ? win32::TBSTATE_CHECKED :
               msg == win32::TB_ENABLEBUTTON || msg == win32::TB_ISBUTTONENABLED ? win32::TBSTATE_ENABLED :
               msg == win32::TB_PRESSBUTTON || msg == win32::TB_ISBUTTONPRESSED ? win32::TBSTATE_PRESSED : win32::TBSTATE_HIDDEN;
        if (msg >= win32::TB_ISBUTTONENABLED) return (lp->rgbtn[i].btn.fsState & mask) != 0;
        state = lp->rgbtn[i].btn.fsState;
        if (l) lp->rgbtn[i].btn.fsState |= mask; else lp->rgbtn[i].btn.fsState &= ~mask;
        if (state != lp->rgbtn[i].btn.fsState) LayoutToolbar(hwnd);
        return 1;
    case win32::TB_GETHOTITEM: return lp->iHot;
    case win32::TB_SETHOTITEM: i = lp->iHot; lp->iHot = static_cast<int>(w) < lp->c ? static_cast<int>(w) : -1; return i;
    case win32::TB_GETTOOLTIPS: return reinterpret_cast<LRESULT>(lp->hwndTip);
    case win32::WM_TIMER: if (w == 1) ShowToolbarTip(hwnd); return 0;
    case win32::WM_MOUSEMOVE:
        i = HitToolbar(lp, pt);
        if (i != lp->iHot) {
            HideTip(hwnd); lp->iHot = i >= 0 ? i : -1;
            if ((hwnd->dwStyle & win32::TBSTYLE_TOOLTIPS) && lp->iHot >= 0) win32::SetTimer(hwnd, 1, 700, nullptr);
        }
        if (lp->iDown >= 0 && lp->iDown < lp->c) {
            if (i == lp->iDown) lp->rgbtn[lp->iDown].btn.fsState |= win32::TBSTATE_PRESSED;
            else lp->rgbtn[lp->iDown].btn.fsState &= ~win32::TBSTATE_PRESSED;
            win32::InvalidateRect(hwnd, nullptr, 1);
        }
        return 0;
    case win32::WM_LBUTTONDOWN: case win32::WM_LBUTTONDBLCLK:
        HideTip(hwnd); i = HitToolbar(lp, pt);
        if (i < 0 || !win32::IsWindowEnabled(hwnd) || !(lp->rgbtn[i].btn.fsState & win32::TBSTATE_ENABLED)) return 0;
        lpbtn = &lp->rgbtn[i].btn;
        fDrop = (lpbtn->fsStyle & win32::BTNS_WHOLEDROPDOWN) || ((lpbtn->fsStyle & win32::BTNS_DROPDOWN) &&
            (!(lp->dwExStyle & win32::TBSTYLE_EX_DRAWDDARROWS) || pt.x >= lp->rgbtn[i].rc.right - 11));
        if (fDrop) { ClickToolbar(hwnd, i, 1, 1); return 0; }
        lp->iDown = i; lpbtn->fsState |= win32::TBSTATE_PRESSED; win32::SetCapture(hwnd); win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_LBUTTONUP:
        i = lp->iDown; lp->iDown = -1;
        if (i >= 0 && i < lp->c) lp->rgbtn[i].btn.fsState &= ~win32::TBSTATE_PRESSED;
        win32::ReleaseCapture(); if (!win32::IsWindow(hwnd)) return 0;
        win32::InvalidateRect(hwnd, nullptr, 1);
        if (i >= 0 && HitToolbar(lp, pt) == i) ClickToolbar(hwnd, i, 0, 1);
        return 0;
    case win32::WM_CANCELMODE: case win32::WM_CAPTURECHANGED:
        if (lp->iDown >= 0 && lp->iDown < lp->c) lp->rgbtn[lp->iDown].btn.fsState &= ~win32::TBSTATE_PRESSED;
        lp->iDown = -1; HideTip(hwnd); win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_KEYDOWN:
        i = lp->iHot;
        if (w == win32::VK_RETURN || w == win32::VK_SPACE || (w == win32::VK_DOWN && i >= 0 &&
            (lp->rgbtn[i].btn.fsStyle & (win32::BTNS_DROPDOWN | win32::BTNS_WHOLEDROPDOWN)))) {
            if (i >= 0) ClickToolbar(hwnd, i, (lp->rgbtn[i].btn.fsStyle & win32::BTNS_WHOLEDROPDOWN) || w == win32::VK_DOWN, 0);
        } else if (w == win32::VK_LEFT || w == win32::VK_RIGHT) {
            step = w == win32::VK_LEFT ? -1 : 1;
            for (j = 0; j < lp->c; j++) {
                i = (i + step + lp->c) % lp->c;
                if (!(lp->rgbtn[i].btn.fsStyle & win32::BTNS_SEP) &&
                    (lp->rgbtn[i].btn.fsState & (win32::TBSTATE_ENABLED | win32::TBSTATE_HIDDEN)) == win32::TBSTATE_ENABLED) { lp->iHot = i; break; }
            }
        }
        return 0;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
