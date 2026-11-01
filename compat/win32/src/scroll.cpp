/* Scroll state and child controls; standard nonclient bars use separate layout. */
#include "internal.h"
#include "cleanup.h"
#include <limits.h>

static HWND LpScrollWindow(HWND hwnd, int sb) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!lpwin || lpwin->fDestroying) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return nullptr; }
    if (sb != win32::SB_HORZ && sb != win32::SB_VERT && sb != win32::SB_CTL) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return nullptr;
    }
    return lpwin;
}

static int IScrollMax(const COMPATSCROLL *lpscr) {
    return static_cast<int>((((static_cast<int64_t>(lpscr->iMin)) > (static_cast<int64_t>(lpscr->iMax) - (lpscr->cPage != 0 ? lpscr->cPage - 1 : 0))) ? (static_cast<int64_t>(lpscr->iMin)) : (static_cast<int64_t>(lpscr->iMax) - (lpscr->cPage != 0 ? lpscr->cPage - 1 : 0))));
}

void CompatLayoutScrollbars(HWND hwnd) {
    RECT rc, client;
    HWND hbar;
    int i, d;
    BOOL fShow;
    /* Combo scroll styles apply to the dropdown, not the closed field. */
    if (!win32::IsWindow(hwnd) || hwnd->fDestroying || hwnd->fScrollLayout || hwnd->fNcScroll || hwnd->lpcombo || !hwnd->fCreated) return;
    StarsRestore<BOOL> restoreLayout(hwnd->fScrollLayout);
    hwnd->fScrollLayout = 1;
    CompatGetSurfaceRect(hwnd, &rc); win32::GetClientRect(hwnd, &client);
    for (i = 0; i < 2; i++) {
        fShow = (hwnd->dwStyle & (i == win32::SB_VERT ? win32::WS_VSCROLL : win32::WS_HSCROLL)) != 0;
        hbar = hwnd->rghwndScroll[i];
        if (fShow && !win32::IsWindow(hbar)) {
            hbar = win32::CreateWindowExA(win32::WS_EX_NOPARENTNOTIFY, "ScrollBar", "", win32::WS_CHILD | (i == win32::SB_VERT ? win32::SBS_VERT : win32::SBS_HORZ),
                                  0, 0, 1, 1, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>((-100 - i))), nullptr, nullptr);
            if (!win32::IsWindow(hwnd)) return;
            hwnd->rghwndScroll[i] = hbar;
            if (hbar) hbar->fNcScroll = 1;
        }
        if (!win32::IsWindow(hbar)) continue;
        hbar->rgscr[0] = hwnd->rgscr[i];
        if (i == win32::SB_VERT) {
            d = StarsMax(0, rc.right - client.right);
            if (hbar->rcChild.left != client.right || hbar->rcChild.top != 0 ||
                hbar->rcChild.right != rc.right || hbar->rcChild.bottom != client.bottom)
                win32::SetWindowPos(hbar, win32::HWND_TOP, client.right, 0, d, client.bottom, win32::SWP_NOACTIVATE);
        } else {
            d = StarsMax(0, rc.bottom - client.bottom);
            if (hbar->rcChild.left != 0 || hbar->rcChild.top != client.bottom ||
                hbar->rcChild.right != client.right || hbar->rcChild.bottom != rc.bottom)
                win32::SetWindowPos(hbar, win32::HWND_TOP, 0, client.bottom, client.right, d, win32::SWP_NOACTIVATE);
        }
        if (!win32::IsWindow(hwnd)) return;
        win32::EnableWindow(hbar, !hbar->rgscr[0].fDisable && IScrollMax(&hbar->rgscr[0]) > hbar->rgscr[0].iMin);
        if (hbar->fVisible != fShow) win32::ShowWindow(hbar, fShow ? win32::SW_SHOW : win32::SW_HIDE);
        if (!win32::IsWindow(hwnd)) return;
        win32::InvalidateRect(hbar, nullptr, 1);
    }
}

BOOL win32::ShowScrollBar(HWND hwnd, int sb, BOOL fShow) {
    DWORD style, styleOld;
    RECT rc;
    if (!LpCompatWindow(hwnd) || (sb != win32::SB_HORZ && sb != win32::SB_VERT && sb != win32::SB_BOTH && sb != win32::SB_CTL)) return 0;
    if (sb == win32::SB_CTL) { win32::ShowWindow(hwnd, fShow ? win32::SW_SHOW : win32::SW_HIDE); return 1; }
    style = sb == win32::SB_BOTH ? win32::WS_HSCROLL | win32::WS_VSCROLL : sb == win32::SB_VERT ? win32::WS_VSCROLL : win32::WS_HSCROLL;
    styleOld = hwnd->dwStyle;
    if (fShow) hwnd->dwStyle |= style; else hwnd->dwStyle &= ~style;
    if (styleOld == hwnd->dwStyle) return 1;
    CompatLayoutScrollbars(hwnd);
    if (!win32::GetClientRect(hwnd, &rc)) return 0;
    win32::InvalidateRect(hwnd, nullptr, 1);
    win32::SendMessageA(hwnd, win32::WM_SIZE, win32::SIZE_RESTORED, win32::MAKELPARAM(rc.right, rc.bottom));
    return 1;
}

int win32::SetScrollInfo(HWND hwnd, int sb, LPCSCROLLINFO lpsi, BOOL fRedraw) {
    COMPATSCROLL *lpscr;
    int64_t cRange;
    BOOL fShow;
    DWORD styleOld;
    RECT rc;
    if (!LpScrollWindow(hwnd, sb)) return 0;
    if (!lpsi || lpsi->cbSize != sizeof(*lpsi) || (lpsi->fMask & ~(win32::SIF_ALL | win32::SIF_DISABLENOSCROLL))) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    if (sb == win32::SB_CTL && hwnd->iControl != 5) return static_cast<int>(win32::SendMessageA(hwnd, win32::SBM_SETSCROLLINFO, fRedraw, reinterpret_cast<LPARAM>(lpsi)));
    styleOld = hwnd->dwStyle;
    lpscr = &hwnd->rgscr[sb == win32::SB_CTL ? 0 : sb];
    if (lpsi->fMask & win32::SIF_RANGE) {
        lpscr->iMin = lpsi->nMin; lpscr->iMax = StarsMax(lpsi->nMin, lpsi->nMax);
    }
    cRange = static_cast<int64_t>(lpscr->iMax) - lpscr->iMin + 1;
    if (lpsi->fMask & win32::SIF_PAGE) lpscr->cPage = static_cast<UINT>((((static_cast<uint64_t>(lpsi->nPage)) < (static_cast<uint64_t>(cRange))) ? (static_cast<uint64_t>(lpsi->nPage)) : (static_cast<uint64_t>(cRange))));
    else lpscr->cPage = static_cast<UINT>((((static_cast<uint64_t>(lpscr->cPage)) < (static_cast<uint64_t>(cRange))) ? (static_cast<uint64_t>(lpscr->cPage)) : (static_cast<uint64_t>(cRange))));
    if (lpsi->fMask & win32::SIF_POS) lpscr->iPos = lpsi->nPos;
    lpscr->iPos = (((lpscr->iMin) > ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr)))));
    fShow = IScrollMax(lpscr) > lpscr->iMin;
    lpscr->fDisable = !fShow;
    if (sb != win32::SB_CTL) {
        if (fShow || (lpsi->fMask & win32::SIF_DISABLENOSCROLL)) hwnd->dwStyle |= sb == win32::SB_VERT ? win32::WS_VSCROLL : win32::WS_HSCROLL;
        else hwnd->dwStyle &= ~(sb == win32::SB_VERT ? win32::WS_VSCROLL : win32::WS_HSCROLL);
        CompatLayoutScrollbars(hwnd);
    }
    if (!win32::IsWindow(hwnd)) return 0;
    if (styleOld != hwnd->dwStyle && win32::GetClientRect(hwnd, &rc))
        win32::SendMessageA(hwnd, win32::WM_SIZE, win32::SIZE_RESTORED, win32::MAKELPARAM(rc.right, rc.bottom));
    if (!win32::IsWindow(hwnd)) return 0;
    if (fRedraw) win32::InvalidateRect(hwnd, nullptr, 1);
    return lpscr->iPos;
}

BOOL win32::GetScrollInfo(HWND hwnd, int sb, LPSCROLLINFO lpsi) {
    COMPATSCROLL *lpscr;
    HWND hbar;
    if (!LpScrollWindow(hwnd, sb)) return 0;
    if (!lpsi || lpsi->cbSize != sizeof(*lpsi) || (lpsi->fMask & ~win32::SIF_ALL)) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    if (sb == win32::SB_CTL && hwnd->iControl != 5) return static_cast<BOOL>(win32::SendMessageA(hwnd, win32::SBM_GETSCROLLINFO, 0, reinterpret_cast<LPARAM>(lpsi)));
    lpscr = &hwnd->rgscr[sb == win32::SB_CTL ? 0 : sb];
    if (lpsi->fMask & win32::SIF_RANGE) { lpsi->nMin = lpscr->iMin; lpsi->nMax = lpscr->iMax; }
    if (lpsi->fMask & win32::SIF_PAGE) lpsi->nPage = lpscr->cPage;
    if (lpsi->fMask & win32::SIF_POS) lpsi->nPos = lpscr->iPos;
    if (lpsi->fMask & win32::SIF_TRACKPOS) {
        hbar = sb == win32::SB_CTL ? hwnd : hwnd->rghwndScroll[sb];
        lpsi->nTrackPos = win32::IsWindow(hbar) && hbar->fTracking ? hbar->iScrollTrack : lpscr->iPos;
    }
    return 1;
}

static int ISetScrollPos(HWND hwnd, COMPATSCROLL *lpscr, int iPos, BOOL fRedraw) {
    int iOld = lpscr->iPos;
    lpscr->iPos = (((lpscr->iMin) > ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr)))));
    if (fRedraw && iOld != lpscr->iPos) win32::InvalidateRect(hwnd, nullptr, 0);
    return iOld;
}

static BOOL FSetScrollRange(HWND hwnd, COMPATSCROLL *lpscr, int iMin, int iMax, BOOL fRedraw) {
    if (iMin > iMax) iMin = iMax = 0;
    if (static_cast<int64_t>(iMax) - iMin > INT32_MAX) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    lpscr->iMin = iMin; lpscr->iMax = iMax;
    lpscr->cPage = static_cast<UINT>((((static_cast<uint64_t>(lpscr->cPage)) < (static_cast<uint64_t>((static_cast<int64_t>(iMax) - iMin + 1)))) ? (static_cast<uint64_t>(lpscr->cPage)) : (static_cast<uint64_t>((static_cast<int64_t>(iMax) - iMin + 1)))));
    lpscr->fDisable = IScrollMax(lpscr) == iMin;
    lpscr->iPos = (((iMin) > ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr))))) ? (iMin) : ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr)))));
    if (fRedraw) win32::InvalidateRect(hwnd, nullptr, 0);
    return 1;
}

int win32::GetScrollPos(HWND hwnd, int sb) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    if (!lpwin) return 0;
    if (sb == win32::SB_CTL) return static_cast<int>(win32::SendMessageA(hwnd, win32::SBM_GETPOS, 0, 0));
    return lpwin->rgscr[sb].iPos;
}

BOOL win32::GetScrollRange(HWND hwnd, int sb, int *lpiMin, int *lpiMax) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    LRESULT lr;
    if (!lpwin) return 0;
    if (!lpiMin || !lpiMax) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    if (sb == win32::SB_CTL) {
        lr = win32::SendMessageA(hwnd, win32::SBM_GETRANGE, reinterpret_cast<WPARAM>(lpiMin), reinterpret_cast<LPARAM>(lpiMax));
        return lpwin->iControl == 5 || lr != 0;
    }
    *lpiMin = lpwin->rgscr[sb].iMin; *lpiMax = lpwin->rgscr[sb].iMax;
    return 1;
}

int win32::SetScrollPos(HWND hwnd, int sb, int iPos, BOOL fRedraw) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    HWND hbar;
    if (!lpwin) return 0;
    if (sb == win32::SB_CTL) return static_cast<int>(win32::SendMessageA(hwnd, win32::SBM_SETPOS, static_cast<WPARAM>(static_cast<INT_PTR>(iPos)), static_cast<LPARAM>(fRedraw)));
    /* Only the scrollbar changes here. Invalidating its owner's client made
       ScrollScanner abandon pixel scrolling and redraw the entire map. */
    iPos = ISetScrollPos(hwnd, &lpwin->rgscr[sb], iPos, 0);
    hbar = lpwin->rghwndScroll[sb];
    if (win32::IsWindow(hbar)) {
        hbar->rgscr[0] = lpwin->rgscr[sb];
        if (fRedraw && iPos != lpwin->rgscr[sb].iPos) win32::InvalidateRect(hbar, nullptr, 0);
    }
    return iPos;
}

BOOL win32::SetScrollRange(HWND hwnd, int sb, int iMin, int iMax, BOOL fRedraw) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    DWORD dwStyle;
    if (!lpwin) return 0;
    if (iMin <= iMax && static_cast<int64_t>(iMax) - iMin > INT32_MAX) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
    }
    if (sb == win32::SB_CTL) {
        if (lpwin->iControl == 5) {
            win32::SendMessageA(hwnd, fRedraw ? win32::SBM_SETRANGEREDRAW : win32::SBM_SETRANGE,
                         static_cast<WPARAM>(static_cast<INT_PTR>(iMin)), static_cast<LPARAM>(iMax));
            return 1;
        }
        return static_cast<BOOL>(win32::SendMessageA(hwnd, fRedraw ? win32::SBM_SETRANGEREDRAW : win32::SBM_SETRANGE,
                                  static_cast<WPARAM>(static_cast<INT_PTR>(iMin)), static_cast<LPARAM>(iMax)));
    }
    if (!FSetScrollRange(hwnd, &lpwin->rgscr[sb], iMin, iMax, fRedraw)) return 0;
    dwStyle = sb == win32::SB_HORZ ? win32::WS_HSCROLL : win32::WS_VSCROLL;
    if (lpwin->rgscr[sb].iMin != lpwin->rgscr[sb].iMax) lpwin->dwStyle |= dwStyle;
    else lpwin->dwStyle &= ~dwStyle;
    CompatLayoutScrollbars(hwnd);
    return 1;
}

struct SCROLLGEOM {
    RECT rc;
    BOOL fVert;
    int cAxis, cCross, dArrow, dThumb, dTravel, iThumb;
};

static BOOL FScrollGeom(HWND hwnd, SCROLLGEOM *lpg) {
    HWND lpwin = LpCompatWindow(hwnd);
    COMPATSCROLL *lpscr;
    int iPos;
    int64_t cRange;
    if (!lpwin || !win32::GetClientRect(hwnd, &lpg->rc)) return 0;
    lpscr = &lpwin->rgscr[0];
    lpg->fVert = !!(lpwin->dwStyle & win32::SBS_VERT);
    lpg->cAxis = lpg->fVert ? lpg->rc.bottom : lpg->rc.right;
    lpg->cCross = lpg->fVert ? lpg->rc.right : lpg->rc.bottom;
    lpg->dArrow = StarsMin(17, lpg->cAxis / 2);
    cRange = static_cast<int64_t>(lpscr->iMax) - lpscr->iMin + 1;
    lpg->dThumb = (((lpg->cAxis - 2 * lpg->dArrow) < (lpscr->cPage != 0 ?
        (((8) > (static_cast<int>((static_cast<int64_t>((lpg->cAxis - 2 * lpg->dArrow)) * lpscr->cPage / StarsMax(1, cRange))))) ? (8) : (static_cast<int>((static_cast<int64_t>((lpg->cAxis - 2 * lpg->dArrow)) * lpscr->cPage / StarsMax(1, cRange))))) : 17)) ? (lpg->cAxis - 2 * lpg->dArrow) : (lpscr->cPage != 0 ?
        (((8) > (static_cast<int>((static_cast<int64_t>((lpg->cAxis - 2 * lpg->dArrow)) * lpscr->cPage / StarsMax(1, cRange))))) ? (8) : (static_cast<int>((static_cast<int64_t>((lpg->cAxis - 2 * lpg->dArrow)) * lpscr->cPage / StarsMax(1, cRange))))) : 17));
    lpg->dTravel = lpg->cAxis - 2 * lpg->dArrow - lpg->dThumb;
    iPos = lpwin->fTracking && lpwin->sbScrollTrack == win32::SB_THUMBTRACK ? lpwin->iScrollTrack : lpscr->iPos;
    iPos = (((lpscr->iMin) > ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr)))));
    cRange = static_cast<int64_t>(IScrollMax(lpscr)) - lpscr->iMin;
    lpg->iThumb = lpg->dArrow + (cRange ? static_cast<int>(((static_cast<int64_t>(iPos) - lpscr->iMin) * lpg->dTravel / cRange)) : 0);
    return lpg->cAxis > 0 && lpg->cCross > 0;
}

static void ScrollRect(const SCROLLGEOM *lpg, RECT *lprc, int iFirst, int iLast) {
    if (lpg->fVert) win32::SetRect(lprc, 0, iFirst, lpg->cCross, iLast);
    else win32::SetRect(lprc, iFirst, 0, iLast, lpg->cCross);
}

static UINT SbScrollHit(const SCROLLGEOM *lpg, LPARAM l) {
    POINT pt = {static_cast<int16_t>(LOWORD(l)), static_cast<int16_t>(HIWORD(l))};
    int iAxis = lpg->fVert ? pt.y : pt.x;
    if (!win32::PtInRect(&lpg->rc, pt)) return win32::SB_ENDSCROLL;
    if (iAxis < lpg->dArrow) return win32::SB_LINEUP;
    if (iAxis >= lpg->cAxis - lpg->dArrow) return win32::SB_LINEDOWN;
    if (iAxis < lpg->iThumb) return win32::SB_PAGEUP;
    if (iAxis >= lpg->iThumb + lpg->dThumb) return win32::SB_PAGEDOWN;
    return win32::SB_THUMBTRACK;
}

static void NotifyScroll(HWND hwnd, UINT sb, int iPos) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (lpwin && !lpwin->fDestroying && lpwin->hwndParent)
        win32::SendMessageA(lpwin->hwndParent, lpwin->dwStyle & win32::SBS_VERT ? win32::WM_VSCROLL : win32::WM_HSCROLL,
                     win32::MAKEWPARAM(sb, sb == win32::SB_THUMBTRACK || sb == win32::SB_THUMBPOSITION ? iPos : 0), lpwin->fNcScroll ? 0 : reinterpret_cast<LPARAM>(hwnd));
}

static void PaintScroll(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    SCROLLGEOM g;
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr, hbrOwn = nullptr, hbrFace = nullptr, hbrGlyph;
    RECT rc, rcGlyph;
    int i, j, iCenter, iCross, iAxis;
    BOOL fEnabled;
    if (!lpwin || !(hdc = win32::BeginPaint(hwnd, &ps))) return;
    // A resized bar can retain dirty pixels even when its client size is zero.
    // Begin/EndPaint must validate them so paint does not starve other messages.
    if (!FScrollGeom(hwnd, &g)) { win32::EndPaint(hwnd, &ps); return; }
    hbr = reinterpret_cast<HBRUSH>(win32::SendMessageA(lpwin->hwndParent, win32::WM_CTLCOLORSCROLLBAR, reinterpret_cast<WPARAM>(hdc), reinterpret_cast<LPARAM>(hwnd)));
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_SCROLLBAR));
    hbrFace = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
    win32::FillRect(hdc, &g.rc, hbr);
    fEnabled = win32::IsWindowEnabled(hwnd) && lpwin->rgscr[0].iMin != lpwin->rgscr[0].iMax;
    hbrGlyph = static_cast<HBRUSH>(win32::GetStockObject(fEnabled ? win32::BLACK_BRUSH : win32::GRAY_BRUSH));
    for (i = 0; i < 2; i++) {
        ScrollRect(&g, &rc, i ? g.cAxis - g.dArrow : 0, i ? g.cAxis : g.dArrow);
        win32::FillRect(hdc, &rc, hbrFace);
        CompatDrawClassicEdge(hdc, &rc, lpwin->fTracking && lpwin->fPressed && lpwin->sbScrollTrack == static_cast<UINT>(i), 1);
        iCenter = i ? g.cAxis - g.dArrow / 2 - 1 : g.dArrow / 2;
        iCross = g.cCross / 2;
        if (lpwin->fTracking && lpwin->fPressed && lpwin->sbScrollTrack == static_cast<UINT>(i)) { iCenter++; iCross++; }
        if (g.dArrow >= 9 && g.cCross >= 9)
            for (j = 0; j < 4; j++) {
                iAxis = iCenter + (i ? 2 - j : j - 2);
                if (g.fVert) win32::SetRect(&rcGlyph, iCross - j, iAxis, iCross + j + 1, iAxis + 1);
                else win32::SetRect(&rcGlyph, iAxis, iCross - j, iAxis + 1, iCross + j + 1);
                win32::FillRect(hdc, &rcGlyph, hbrGlyph);
            }
    }
    if (fEnabled && g.dThumb != 0) {
        ScrollRect(&g, &rc, g.iThumb, g.iThumb + g.dThumb);
        win32::FillRect(hdc, &rc, hbrFace);
        CompatDrawClassicEdge(hdc, &rc, 0, 1);
        if (win32::GetFocus() == hwnd) {
            win32::InflateRect(&rc, -2, -2);
            win32::FrameRect(hdc, &rc, static_cast<HBRUSH>(win32::GetStockObject(win32::GRAY_BRUSH)));
        }
    }
    if (hbrFace) win32::DeleteObject(hbrFace);
    if (hbrOwn) win32::DeleteObject(hbrOwn);
    win32::EndPaint(hwnd, &ps);
}

static UINT SbScrollKey(WPARAM vk) {
    switch (vk) {
    case win32::VK_LEFT: case win32::VK_UP: return win32::SB_LINEUP;
    case win32::VK_RIGHT: case win32::VK_DOWN: return win32::SB_LINEDOWN;
    case win32::VK_PRIOR: return win32::SB_PAGEUP;
    case win32::VK_NEXT: return win32::SB_PAGEDOWN;
    case win32::VK_HOME: return win32::SB_TOP;
    case win32::VK_END: return win32::SB_BOTTOM;
    default: return win32::SB_ENDSCROLL;
    }
}

static void CancelScroll(HWND hwnd, BOOL fRelease) {
    HWND lpwin = LpCompatWindow(hwnd);
    BOOL fTracking;
    if (!lpwin) return;
    fTracking = lpwin->fTracking;
    if (lpwin->idScrollTimer) win32::KillTimer(hwnd, lpwin->idScrollTimer);
    lpwin->idScrollTimer = 0;
    lpwin->fTracking = lpwin->fPressed = 0;
    if (fRelease && win32::GetCapture() == hwnd) win32::ReleaseCapture();
    if (!win32::IsWindow(hwnd)) return;
    win32::InvalidateRect(hwnd, nullptr, 0);
    if (fTracking) NotifyScroll(hwnd, win32::SB_ENDSCROLL, 0);
}

LRESULT  CompatScrollProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lpwin = LpCompatWindow(hwnd);
    HWND hwndRoot;
    COMPATSCROLL *lpscr;
    int iOld;
    SCROLLGEOM g;
    UINT sb;
    int iAxis, iOffset, iPos;
    int64_t cRange;
    BOOL fThumb;
    if (!lpwin) return 0;
    lpscr = &lpwin->rgscr[0];
    switch (msg) {
    case win32::WM_NCCREATE:
        lpwin->iControl = 5;
        lpscr->iMin = lpscr->iMax = lpscr->iPos = 0;
        return 1;
    case win32::SBM_SETSCROLLINFO: return win32::SetScrollInfo(hwnd, win32::SB_CTL, reinterpret_cast<LPCSCROLLINFO>(l), static_cast<BOOL>(w));
    case win32::SBM_GETSCROLLINFO: return win32::GetScrollInfo(hwnd, win32::SB_CTL, reinterpret_cast<LPSCROLLINFO>(l));
    case win32::SBM_GETPOS: return lpscr->iPos;
    case win32::SBM_SETPOS: return ISetScrollPos(hwnd, lpscr, static_cast<int>(static_cast<INT_PTR>(w)), !!l);
    case win32::SBM_SETRANGE:
    case win32::SBM_SETRANGEREDRAW:
        iOld = lpscr->iPos;
        return FSetScrollRange(hwnd, lpscr, static_cast<int>(static_cast<INT_PTR>(w)), static_cast<int>(l), msg == win32::SBM_SETRANGEREDRAW) ? iOld : 0;
    case win32::SBM_GETRANGE:
        if (!w || !l) return 0;
        *reinterpret_cast<int *>(w) = lpscr->iMin; *reinterpret_cast<int *>(l) = lpscr->iMax;
        return 0;
    case win32::WM_GETDLGCODE: return win32::DLGC_WANTARROWS;
    case win32::WM_PAINT: PaintScroll(hwnd); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::WM_SETFOCUS: win32::InvalidateRect(hwnd, nullptr, 0); return 0;
    case win32::WM_KILLFOCUS: win32::InvalidateRect(hwnd, nullptr, 0); return 0;
    case win32::WM_ENABLE:
        if (!w) CancelScroll(hwnd, 1);
        if (win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 0);
        return 0;
    case win32::WM_KEYDOWN:
        sb = SbScrollKey(w);
        if (sb != win32::SB_ENDSCROLL && win32::IsWindowEnabled(hwnd) && lpscr->iMin != lpscr->iMax) {
            NotifyScroll(hwnd, sb, 0);
        }
        return 0;
    case win32::WM_KEYUP: return 0;
    case win32::WM_LBUTTONDOWN: case win32::WM_LBUTTONDBLCLK:
        if (!win32::IsWindowEnabled(hwnd) || lpscr->iMin == lpscr->iMax || !FScrollGeom(hwnd, &g)) return 0;
        sb = SbScrollHit(&g, l);
        if (sb == win32::SB_ENDSCROLL) return 0;
        if (lpwin->dwStyle & win32::WS_TABSTOP) win32::SetFocus(hwnd);
        if (!win32::IsWindow(hwnd)) return 0;
        win32::SetCapture(hwnd);
        if (!win32::IsWindow(hwnd) || win32::GetCapture() != hwnd) return 0;
        lpwin->fTracking = lpwin->fPressed = 1;
        lpwin->sbScrollTrack = sb; lpwin->iScrollTrack = lpscr->iPos;
        lpwin->lScrollPoint = l;
        if (sb != win32::SB_THUMBTRACK) lpwin->idScrollTimer = win32::SetTimer(hwnd, UINTPTR_MAX, 400, nullptr);
        lpwin->dScrollGrab = (g.fVert ? static_cast<int16_t>(HIWORD(l)) : static_cast<int16_t>(LOWORD(l))) - g.iThumb;
        win32::InvalidateRect(hwnd, nullptr, 0);
        NotifyScroll(hwnd, sb, lpscr->iPos);
        return 0;
    case win32::WM_MOUSEMOVE: {
        if (!lpwin->fTracking || win32::GetCapture() != hwnd || !FScrollGeom(hwnd, &g)) return 0;
        /* Owner drawing may release several DCs and scroll exposed pixels.
           Present their completed map and the new thumb together once. */
        hwndRoot = HwndCompatRoot(hwnd);
        COMPATPRESENTSCOPE deferPresent(hwndRoot);
        lpwin->lScrollPoint = l;
        if (lpwin->sbScrollTrack == win32::SB_THUMBTRACK) {
            iAxis = g.fVert ? static_cast<int16_t>(HIWORD(l)) : static_cast<int16_t>(LOWORD(l));
            iOffset = (((0) > (StarsMin(iAxis - lpwin->dScrollGrab - g.dArrow, g.dTravel))) ? (0) : (StarsMin(iAxis - lpwin->dScrollGrab - g.dArrow, g.dTravel)));
            cRange = static_cast<int64_t>(IScrollMax(lpscr)) - lpscr->iMin;
            lpwin->iScrollTrack = g.dTravel ? static_cast<int>((lpscr->iMin + (iOffset * cRange + g.dTravel / 2) / g.dTravel)) : lpscr->iMin;
            win32::InvalidateRect(hwnd, nullptr, 0);
            NotifyScroll(hwnd, win32::SB_THUMBTRACK, lpwin->iScrollTrack);
        } else {
            lpwin->fPressed = SbScrollHit(&g, l) == lpwin->sbScrollTrack;
            win32::InvalidateRect(hwnd, nullptr, 0);
        }
        /* WM_PAINT is lower priority than mouse input; continuous dragging
           must not wait for the input queue to empty before moving the thumb. */
        if (win32::IsWindow(hwnd) && !FStarsErrorPending()) win32::UpdateWindow(hwnd);
        deferPresent.Close();
        if (win32::IsWindow(hwndRoot) && !FStarsErrorPending()) {
            if (hwndRoot->cPresentDefer == 0) FCompatPresent(hwndRoot);
        }
        return 0;
    }
    case win32::WM_TIMER:
        if (!lpwin->idScrollTimer || w != lpwin->idScrollTimer ||
            !lpwin->fTracking || win32::GetCapture() != hwnd) return 0;
        win32::SetTimer(hwnd, lpwin->idScrollTimer, 50, nullptr);
        if (FScrollGeom(hwnd, &g) && SbScrollHit(&g, lpwin->lScrollPoint) == lpwin->sbScrollTrack)
            NotifyScroll(hwnd, lpwin->sbScrollTrack, lpscr->iPos);
        return 0;
    case win32::WM_LBUTTONUP:
        if (!lpwin->fTracking) return 0;
        fThumb = lpwin->sbScrollTrack == win32::SB_THUMBTRACK;
        iPos = lpwin->iScrollTrack;
        if (lpwin->idScrollTimer) win32::KillTimer(hwnd, lpwin->idScrollTimer);
        lpwin->idScrollTimer = 0;
        lpwin->fTracking = lpwin->fPressed = 0;
        if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (!win32::IsWindow(hwnd)) return 0;
        win32::InvalidateRect(hwnd, nullptr, 0);
        if (fThumb) NotifyScroll(hwnd, win32::SB_THUMBPOSITION, iPos);
        if (win32::IsWindow(hwnd)) NotifyScroll(hwnd, win32::SB_ENDSCROLL, 0);
        return 0;
    case win32::WM_CANCELMODE: CancelScroll(hwnd, 1); return 0;
    case win32::WM_CAPTURECHANGED: CancelScroll(hwnd, 0); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
