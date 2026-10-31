/* Scroll state and child controls; standard nonclient bars use separate layout. */
#include "internal.h"
#include "cleanup.h"
#include <limits.h>

static HWND LpScrollWindow(HWND hwnd, int sb) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!lpwin || lpwin->fDestroying) { win32::SetLastError(ERROR_INVALID_WINDOW_HANDLE); return NULL; }
    if (sb != SB_HORZ && sb != SB_VERT && sb != SB_CTL) {
        win32::SetLastError(ERROR_INVALID_PARAMETER); return NULL;
    }
    return lpwin;
}

static int IScrollMax(const COMPATSCROLL *lpscr) {
    return (int)((((int64_t)lpscr->iMin) > ((int64_t)lpscr->iMax - (lpscr->cPage != 0 ? lpscr->cPage - 1 : 0))) ? ((int64_t)lpscr->iMin) : ((int64_t)lpscr->iMax - (lpscr->cPage != 0 ? lpscr->cPage - 1 : 0)));
}

void CompatLayoutScrollbars(HWND hwnd) {
    RECT rc, client;
    HWND hbar;
    int i, d;
    BOOL fShow;
    /* Combo scroll styles apply to the dropdown, not the closed field. */
    if (!win32::IsWindow(hwnd) || hwnd->fDestroying || hwnd->fScrollLayout || hwnd->fNcScroll || hwnd->lpcombo || !hwnd->fCreated) return;
    StarsRestore<BOOL> restoreLayout(hwnd->fScrollLayout);
    hwnd->fScrollLayout = TRUE;
    CompatGetSurfaceRect(hwnd, &rc); win32::GetClientRect(hwnd, &client);
    for (i = 0; i < 2; i++) {
        fShow = (hwnd->dwStyle & (i == SB_VERT ? WS_VSCROLL : WS_HSCROLL)) != 0;
        hbar = hwnd->rghwndScroll[i];
        if (fShow && !win32::IsWindow(hbar)) {
            hbar = win32::CreateWindowExA(WS_EX_NOPARENTNOTIFY, "ScrollBar", "", WS_CHILD | (i == SB_VERT ? SBS_VERT : SBS_HORZ),
                                  0, 0, 1, 1, hwnd, (HMENU)(INT_PTR)(-100 - i), NULL, NULL);
            if (!win32::IsWindow(hwnd)) return;
            hwnd->rghwndScroll[i] = hbar;
            if (hbar) hbar->fNcScroll = TRUE;
        }
        if (!win32::IsWindow(hbar)) continue;
        hbar->rgscr[0] = hwnd->rgscr[i];
        if (i == SB_VERT) {
            d = StarsMax(0, rc.right - client.right);
            if (hbar->rcChild.left != client.right || hbar->rcChild.top != 0 ||
                hbar->rcChild.right != rc.right || hbar->rcChild.bottom != client.bottom)
                win32::SetWindowPos(hbar, HWND_TOP, client.right, 0, d, client.bottom, SWP_NOACTIVATE);
        } else {
            d = StarsMax(0, rc.bottom - client.bottom);
            if (hbar->rcChild.left != 0 || hbar->rcChild.top != client.bottom ||
                hbar->rcChild.right != client.right || hbar->rcChild.bottom != rc.bottom)
                win32::SetWindowPos(hbar, HWND_TOP, 0, client.bottom, client.right, d, SWP_NOACTIVATE);
        }
        if (!win32::IsWindow(hwnd)) return;
        win32::EnableWindow(hbar, !hbar->rgscr[0].fDisable && IScrollMax(&hbar->rgscr[0]) > hbar->rgscr[0].iMin);
        if (hbar->fVisible != fShow) win32::ShowWindow(hbar, fShow ? SW_SHOW : SW_HIDE);
        if (!win32::IsWindow(hwnd)) return;
        win32::InvalidateRect(hbar, NULL, TRUE);
    }
}

BOOL WINAPI win32::ShowScrollBar(HWND hwnd, int sb, BOOL fShow) {
    DWORD style, styleOld;
    RECT rc;
    if (!LpCompatWindow(hwnd) || (sb != SB_HORZ && sb != SB_VERT && sb != SB_BOTH && sb != SB_CTL)) return FALSE;
    if (sb == SB_CTL) { win32::ShowWindow(hwnd, fShow ? SW_SHOW : SW_HIDE); return TRUE; }
    style = sb == SB_BOTH ? WS_HSCROLL | WS_VSCROLL : sb == SB_VERT ? WS_VSCROLL : WS_HSCROLL;
    styleOld = hwnd->dwStyle;
    if (fShow) hwnd->dwStyle |= style; else hwnd->dwStyle &= ~style;
    if (styleOld == hwnd->dwStyle) return TRUE;
    CompatLayoutScrollbars(hwnd);
    if (!win32::GetClientRect(hwnd, &rc)) return FALSE;
    win32::InvalidateRect(hwnd, NULL, TRUE);
    win32::SendMessageA(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
    return TRUE;
}

int WINAPI win32::SetScrollInfo(HWND hwnd, int sb, LPCSCROLLINFO lpsi, BOOL fRedraw) {
    COMPATSCROLL *lpscr;
    int64_t cRange;
    BOOL fShow;
    DWORD styleOld;
    RECT rc;
    if (!LpScrollWindow(hwnd, sb)) return 0;
    if (!lpsi || lpsi->cbSize != sizeof(*lpsi) || (lpsi->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL))) { win32::SetLastError(ERROR_INVALID_PARAMETER); return 0; }
    if (sb == SB_CTL && hwnd->iControl != 5) return (int)win32::SendMessageA(hwnd, SBM_SETSCROLLINFO, fRedraw, (LPARAM)lpsi);
    styleOld = hwnd->dwStyle;
    lpscr = &hwnd->rgscr[sb == SB_CTL ? 0 : sb];
    if (lpsi->fMask & SIF_RANGE) {
        lpscr->iMin = lpsi->nMin; lpscr->iMax = StarsMax(lpsi->nMin, lpsi->nMax);
    }
    cRange = (int64_t)lpscr->iMax - lpscr->iMin + 1;
    if (lpsi->fMask & SIF_PAGE) lpscr->cPage = (UINT)((((uint64_t)lpsi->nPage) < ((uint64_t)cRange)) ? ((uint64_t)lpsi->nPage) : ((uint64_t)cRange));
    else lpscr->cPage = (UINT)((((uint64_t)lpscr->cPage) < ((uint64_t)cRange)) ? ((uint64_t)lpscr->cPage) : ((uint64_t)cRange));
    if (lpsi->fMask & SIF_POS) lpscr->iPos = lpsi->nPos;
    lpscr->iPos = (((lpscr->iMin) > ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr)))));
    fShow = IScrollMax(lpscr) > lpscr->iMin;
    lpscr->fDisable = !fShow;
    if (sb != SB_CTL) {
        if (fShow || (lpsi->fMask & SIF_DISABLENOSCROLL)) hwnd->dwStyle |= sb == SB_VERT ? WS_VSCROLL : WS_HSCROLL;
        else hwnd->dwStyle &= ~(sb == SB_VERT ? WS_VSCROLL : WS_HSCROLL);
        CompatLayoutScrollbars(hwnd);
    }
    if (!win32::IsWindow(hwnd)) return 0;
    if (styleOld != hwnd->dwStyle && win32::GetClientRect(hwnd, &rc))
        win32::SendMessageA(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
    if (!win32::IsWindow(hwnd)) return 0;
    if (fRedraw) win32::InvalidateRect(hwnd, NULL, TRUE);
    return lpscr->iPos;
}

BOOL WINAPI win32::GetScrollInfo(HWND hwnd, int sb, LPSCROLLINFO lpsi) {
    COMPATSCROLL *lpscr;
    HWND hbar;
    if (!LpScrollWindow(hwnd, sb)) return FALSE;
    if (!lpsi || lpsi->cbSize != sizeof(*lpsi) || (lpsi->fMask & ~SIF_ALL)) { win32::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (sb == SB_CTL && hwnd->iControl != 5) return (BOOL)win32::SendMessageA(hwnd, SBM_GETSCROLLINFO, 0, (LPARAM)lpsi);
    lpscr = &hwnd->rgscr[sb == SB_CTL ? 0 : sb];
    if (lpsi->fMask & SIF_RANGE) { lpsi->nMin = lpscr->iMin; lpsi->nMax = lpscr->iMax; }
    if (lpsi->fMask & SIF_PAGE) lpsi->nPage = lpscr->cPage;
    if (lpsi->fMask & SIF_POS) lpsi->nPos = lpscr->iPos;
    if (lpsi->fMask & SIF_TRACKPOS) {
        hbar = sb == SB_CTL ? hwnd : hwnd->rghwndScroll[sb];
        lpsi->nTrackPos = win32::IsWindow(hbar) && hbar->fTracking ? hbar->iScrollTrack : lpscr->iPos;
    }
    return TRUE;
}

static int ISetScrollPos(HWND hwnd, COMPATSCROLL *lpscr, int iPos, BOOL fRedraw) {
    int iOld = lpscr->iPos;
    lpscr->iPos = (((lpscr->iMin) > ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr)))));
    if (fRedraw && iOld != lpscr->iPos) win32::InvalidateRect(hwnd, NULL, FALSE);
    return iOld;
}

static BOOL FSetScrollRange(HWND hwnd, COMPATSCROLL *lpscr, int iMin, int iMax, BOOL fRedraw) {
    if (iMin > iMax) iMin = iMax = 0;
    if ((int64_t)iMax - iMin > INT32_MAX) { win32::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    lpscr->iMin = iMin; lpscr->iMax = iMax;
    lpscr->cPage = (UINT)((((uint64_t)lpscr->cPage) < ((uint64_t)((int64_t)iMax - iMin + 1))) ? ((uint64_t)lpscr->cPage) : ((uint64_t)((int64_t)iMax - iMin + 1)));
    lpscr->fDisable = IScrollMax(lpscr) == iMin;
    lpscr->iPos = (((iMin) > ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr))))) ? (iMin) : ((((lpscr->iPos) < (IScrollMax(lpscr))) ? (lpscr->iPos) : (IScrollMax(lpscr)))));
    if (fRedraw) win32::InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
}

int WINAPI win32::GetScrollPos(HWND hwnd, int sb) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    if (!lpwin) return 0;
    if (sb == SB_CTL) return (int)win32::SendMessageA(hwnd, SBM_GETPOS, 0, 0);
    return lpwin->rgscr[sb].iPos;
}

BOOL WINAPI win32::GetScrollRange(HWND hwnd, int sb, int *lpiMin, int *lpiMax) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    LRESULT lr;
    if (!lpwin) return FALSE;
    if (!lpiMin || !lpiMax) { win32::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (sb == SB_CTL) {
        lr = win32::SendMessageA(hwnd, SBM_GETRANGE, (WPARAM)lpiMin, (LPARAM)lpiMax);
        return lpwin->iControl == 5 || lr != 0;
    }
    *lpiMin = lpwin->rgscr[sb].iMin; *lpiMax = lpwin->rgscr[sb].iMax;
    return TRUE;
}

int WINAPI win32::SetScrollPos(HWND hwnd, int sb, int iPos, BOOL fRedraw) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    HWND hbar;
    if (!lpwin) return 0;
    if (sb == SB_CTL) return (int)win32::SendMessageA(hwnd, SBM_SETPOS, (WPARAM)(INT_PTR)iPos, (LPARAM)fRedraw);
    /* Only the scrollbar changes here. Invalidating its owner's client made
       ScrollScanner abandon pixel scrolling and redraw the entire map. */
    iPos = ISetScrollPos(hwnd, &lpwin->rgscr[sb], iPos, FALSE);
    hbar = lpwin->rghwndScroll[sb];
    if (win32::IsWindow(hbar)) {
        hbar->rgscr[0] = lpwin->rgscr[sb];
        if (fRedraw && iPos != lpwin->rgscr[sb].iPos) win32::InvalidateRect(hbar, NULL, FALSE);
    }
    return iPos;
}

BOOL WINAPI win32::SetScrollRange(HWND hwnd, int sb, int iMin, int iMax, BOOL fRedraw) {
    HWND lpwin = LpScrollWindow(hwnd, sb);
    DWORD dwStyle;
    if (!lpwin) return FALSE;
    if (iMin <= iMax && (int64_t)iMax - iMin > INT32_MAX) {
        win32::SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
    if (sb == SB_CTL) {
        if (lpwin->iControl == 5) {
            win32::SendMessageA(hwnd, fRedraw ? SBM_SETRANGEREDRAW : SBM_SETRANGE,
                         (WPARAM)(INT_PTR)iMin, (LPARAM)iMax);
            return TRUE;
        }
        return (BOOL)win32::SendMessageA(hwnd, fRedraw ? SBM_SETRANGEREDRAW : SBM_SETRANGE,
                                  (WPARAM)(INT_PTR)iMin, (LPARAM)iMax);
    }
    if (!FSetScrollRange(hwnd, &lpwin->rgscr[sb], iMin, iMax, fRedraw)) return FALSE;
    dwStyle = sb == SB_HORZ ? WS_HSCROLL : WS_VSCROLL;
    if (lpwin->rgscr[sb].iMin != lpwin->rgscr[sb].iMax) lpwin->dwStyle |= dwStyle;
    else lpwin->dwStyle &= ~dwStyle;
    CompatLayoutScrollbars(hwnd);
    return TRUE;
}

typedef struct SCROLLGEOM {
    RECT rc;
    BOOL fVert;
    int cAxis, cCross, dArrow, dThumb, dTravel, iThumb;
} SCROLLGEOM;

static BOOL FScrollGeom(HWND hwnd, SCROLLGEOM *lpg) {
    HWND lpwin = LpCompatWindow(hwnd);
    COMPATSCROLL *lpscr;
    int iPos;
    int64_t cRange;
    if (!lpwin || !win32::GetClientRect(hwnd, &lpg->rc)) return FALSE;
    lpscr = &lpwin->rgscr[0];
    lpg->fVert = !!(lpwin->dwStyle & SBS_VERT);
    lpg->cAxis = lpg->fVert ? lpg->rc.bottom : lpg->rc.right;
    lpg->cCross = lpg->fVert ? lpg->rc.right : lpg->rc.bottom;
    lpg->dArrow = StarsMin(17, lpg->cAxis / 2);
    cRange = (int64_t)lpscr->iMax - lpscr->iMin + 1;
    lpg->dThumb = (((lpg->cAxis - 2 * lpg->dArrow) < (lpscr->cPage != 0 ?
        (((8) > ((int)((int64_t)(lpg->cAxis - 2 * lpg->dArrow) * lpscr->cPage / StarsMax(1, cRange)))) ? (8) : ((int)((int64_t)(lpg->cAxis - 2 * lpg->dArrow) * lpscr->cPage / StarsMax(1, cRange)))) : 17)) ? (lpg->cAxis - 2 * lpg->dArrow) : (lpscr->cPage != 0 ?
        (((8) > ((int)((int64_t)(lpg->cAxis - 2 * lpg->dArrow) * lpscr->cPage / StarsMax(1, cRange)))) ? (8) : ((int)((int64_t)(lpg->cAxis - 2 * lpg->dArrow) * lpscr->cPage / StarsMax(1, cRange)))) : 17));
    lpg->dTravel = lpg->cAxis - 2 * lpg->dArrow - lpg->dThumb;
    iPos = lpwin->fTracking && lpwin->sbScrollTrack == SB_THUMBTRACK ? lpwin->iScrollTrack : lpscr->iPos;
    iPos = (((lpscr->iMin) > ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr))))) ? (lpscr->iMin) : ((((iPos) < (IScrollMax(lpscr))) ? (iPos) : (IScrollMax(lpscr)))));
    cRange = (int64_t)IScrollMax(lpscr) - lpscr->iMin;
    lpg->iThumb = lpg->dArrow + (cRange ? (int)(((int64_t)iPos - lpscr->iMin) * lpg->dTravel / cRange) : 0);
    return lpg->cAxis > 0 && lpg->cCross > 0;
}

static void ScrollRect(const SCROLLGEOM *lpg, RECT *lprc, int iFirst, int iLast) {
    if (lpg->fVert) win32::SetRect(lprc, 0, iFirst, lpg->cCross, iLast);
    else win32::SetRect(lprc, iFirst, 0, iLast, lpg->cCross);
}

static UINT SbScrollHit(const SCROLLGEOM *lpg, LPARAM l) {
    POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
    int iAxis = lpg->fVert ? pt.y : pt.x;
    if (!win32::PtInRect(&lpg->rc, pt)) return SB_ENDSCROLL;
    if (iAxis < lpg->dArrow) return SB_LINEUP;
    if (iAxis >= lpg->cAxis - lpg->dArrow) return SB_LINEDOWN;
    if (iAxis < lpg->iThumb) return SB_PAGEUP;
    if (iAxis >= lpg->iThumb + lpg->dThumb) return SB_PAGEDOWN;
    return SB_THUMBTRACK;
}

static void NotifyScroll(HWND hwnd, UINT sb, int iPos) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (lpwin && !lpwin->fDestroying && lpwin->hwndParent)
        win32::SendMessageA(lpwin->hwndParent, lpwin->dwStyle & SBS_VERT ? WM_VSCROLL : WM_HSCROLL,
                     MAKEWPARAM(sb, sb == SB_THUMBTRACK || sb == SB_THUMBPOSITION ? iPos : 0), lpwin->fNcScroll ? 0 : (LPARAM)hwnd);
}

static void PaintScroll(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    SCROLLGEOM g;
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr, hbrOwn = NULL, hbrFace = NULL, hbrGlyph;
    RECT rc, rcGlyph;
    int i, j, iCenter, iCross, iAxis;
    BOOL fEnabled;
    if (!lpwin || !(hdc = win32::BeginPaint(hwnd, &ps))) return;
    // A resized bar can retain dirty pixels even when its client size is zero.
    // Begin/EndPaint must validate them so paint does not starve other messages.
    if (!FScrollGeom(hwnd, &g)) { win32::EndPaint(hwnd, &ps); return; }
    hbr = (HBRUSH)win32::SendMessageA(lpwin->hwndParent, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)hwnd);
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetSysColor(COLOR_SCROLLBAR));
    hbrFace = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNFACE));
    win32::FillRect(hdc, &g.rc, hbr);
    fEnabled = win32::IsWindowEnabled(hwnd) && lpwin->rgscr[0].iMin != lpwin->rgscr[0].iMax;
    hbrGlyph = (HBRUSH)win32::GetStockObject(fEnabled ? BLACK_BRUSH : GRAY_BRUSH);
    for (i = 0; i < 2; i++) {
        ScrollRect(&g, &rc, i ? g.cAxis - g.dArrow : 0, i ? g.cAxis : g.dArrow);
        win32::FillRect(hdc, &rc, hbrFace);
        CompatDrawClassicEdge(hdc, &rc, lpwin->fTracking && lpwin->fPressed && lpwin->sbScrollTrack == (UINT)i, TRUE);
        iCenter = i ? g.cAxis - g.dArrow / 2 - 1 : g.dArrow / 2;
        iCross = g.cCross / 2;
        if (lpwin->fTracking && lpwin->fPressed && lpwin->sbScrollTrack == (UINT)i) { iCenter++; iCross++; }
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
        CompatDrawClassicEdge(hdc, &rc, FALSE, TRUE);
        if (win32::GetFocus() == hwnd) {
            win32::InflateRect(&rc, -2, -2);
            win32::FrameRect(hdc, &rc, (HBRUSH)win32::GetStockObject(GRAY_BRUSH));
        }
    }
    if (hbrFace) win32::DeleteObject(hbrFace);
    if (hbrOwn) win32::DeleteObject(hbrOwn);
    win32::EndPaint(hwnd, &ps);
}

static UINT SbScrollKey(WPARAM vk) {
    switch (vk) {
    case VK_LEFT: case VK_UP: return SB_LINEUP;
    case VK_RIGHT: case VK_DOWN: return SB_LINEDOWN;
    case VK_PRIOR: return SB_PAGEUP;
    case VK_NEXT: return SB_PAGEDOWN;
    case VK_HOME: return SB_TOP;
    case VK_END: return SB_BOTTOM;
    default: return SB_ENDSCROLL;
    }
}

static void CancelScroll(HWND hwnd, BOOL fRelease) {
    HWND lpwin = LpCompatWindow(hwnd);
    BOOL fTracking;
    if (!lpwin) return;
    fTracking = lpwin->fTracking;
    if (lpwin->idScrollTimer) win32::KillTimer(hwnd, lpwin->idScrollTimer);
    lpwin->idScrollTimer = 0;
    lpwin->fTracking = lpwin->fPressed = FALSE;
    if (fRelease && win32::GetCapture() == hwnd) win32::ReleaseCapture();
    if (!win32::IsWindow(hwnd)) return;
    win32::InvalidateRect(hwnd, NULL, FALSE);
    if (fTracking) NotifyScroll(hwnd, SB_ENDSCROLL, 0);
}

LRESULT CALLBACK CompatScrollProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
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
    case WM_NCCREATE:
        lpwin->iControl = 5;
        lpscr->iMin = lpscr->iMax = lpscr->iPos = 0;
        return TRUE;
    case SBM_SETSCROLLINFO: return win32::SetScrollInfo(hwnd, SB_CTL, (LPCSCROLLINFO)l, (BOOL)w);
    case SBM_GETSCROLLINFO: return win32::GetScrollInfo(hwnd, SB_CTL, (LPSCROLLINFO)l);
    case SBM_GETPOS: return lpscr->iPos;
    case SBM_SETPOS: return ISetScrollPos(hwnd, lpscr, (int)(INT_PTR)w, !!l);
    case SBM_SETRANGE:
    case SBM_SETRANGEREDRAW:
        iOld = lpscr->iPos;
        return FSetScrollRange(hwnd, lpscr, (int)(INT_PTR)w, (int)l, msg == SBM_SETRANGEREDRAW) ? iOld : 0;
    case SBM_GETRANGE:
        if (!w || !l) return 0;
        *(int *)w = lpscr->iMin; *(int *)l = lpscr->iMax;
        return 0;
    case WM_GETDLGCODE: return DLGC_WANTARROWS;
    case WM_PAINT: PaintScroll(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SETFOCUS: win32::InvalidateRect(hwnd, NULL, FALSE); return 0;
    case WM_KILLFOCUS: win32::InvalidateRect(hwnd, NULL, FALSE); return 0;
    case WM_ENABLE:
        if (!w) CancelScroll(hwnd, TRUE);
        if (win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_KEYDOWN:
        sb = SbScrollKey(w);
        if (sb != SB_ENDSCROLL && win32::IsWindowEnabled(hwnd) && lpscr->iMin != lpscr->iMax) {
            NotifyScroll(hwnd, sb, 0);
        }
        return 0;
    case WM_KEYUP: return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (!win32::IsWindowEnabled(hwnd) || lpscr->iMin == lpscr->iMax || !FScrollGeom(hwnd, &g)) return 0;
        sb = SbScrollHit(&g, l);
        if (sb == SB_ENDSCROLL) return 0;
        if (lpwin->dwStyle & WS_TABSTOP) win32::SetFocus(hwnd);
        if (!win32::IsWindow(hwnd)) return 0;
        win32::SetCapture(hwnd);
        if (!win32::IsWindow(hwnd) || win32::GetCapture() != hwnd) return 0;
        lpwin->fTracking = lpwin->fPressed = TRUE;
        lpwin->sbScrollTrack = sb; lpwin->iScrollTrack = lpscr->iPos;
        lpwin->lScrollPoint = l;
        if (sb != SB_THUMBTRACK) lpwin->idScrollTimer = win32::SetTimer(hwnd, UINTPTR_MAX, 400, NULL);
        lpwin->dScrollGrab = (g.fVert ? (int16_t)HIWORD(l) : (int16_t)LOWORD(l)) - g.iThumb;
        win32::InvalidateRect(hwnd, NULL, FALSE);
        NotifyScroll(hwnd, sb, lpscr->iPos);
        return 0;
    case WM_MOUSEMOVE: {
        if (!lpwin->fTracking || win32::GetCapture() != hwnd || !FScrollGeom(hwnd, &g)) return 0;
        /* Owner drawing may release several DCs and scroll exposed pixels.
           Present their completed map and the new thumb together once. */
        hwndRoot = HwndCompatRoot(hwnd);
        COMPATPRESENTSCOPE deferPresent(hwndRoot);
        lpwin->lScrollPoint = l;
        if (lpwin->sbScrollTrack == SB_THUMBTRACK) {
            iAxis = g.fVert ? (int16_t)HIWORD(l) : (int16_t)LOWORD(l);
            iOffset = (((0) > (StarsMin(iAxis - lpwin->dScrollGrab - g.dArrow, g.dTravel))) ? (0) : (StarsMin(iAxis - lpwin->dScrollGrab - g.dArrow, g.dTravel)));
            cRange = (int64_t)IScrollMax(lpscr) - lpscr->iMin;
            lpwin->iScrollTrack = g.dTravel ? (int)(lpscr->iMin + (iOffset * cRange + g.dTravel / 2) / g.dTravel) : lpscr->iMin;
            win32::InvalidateRect(hwnd, NULL, FALSE);
            NotifyScroll(hwnd, SB_THUMBTRACK, lpwin->iScrollTrack);
        } else {
            lpwin->fPressed = SbScrollHit(&g, l) == lpwin->sbScrollTrack;
            win32::InvalidateRect(hwnd, NULL, FALSE);
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
    case WM_TIMER:
        if (!lpwin->idScrollTimer || w != lpwin->idScrollTimer ||
            !lpwin->fTracking || win32::GetCapture() != hwnd) return 0;
        win32::SetTimer(hwnd, lpwin->idScrollTimer, 50, NULL);
        if (FScrollGeom(hwnd, &g) && SbScrollHit(&g, lpwin->lScrollPoint) == lpwin->sbScrollTrack)
            NotifyScroll(hwnd, lpwin->sbScrollTrack, lpscr->iPos);
        return 0;
    case WM_LBUTTONUP:
        if (!lpwin->fTracking) return 0;
        fThumb = lpwin->sbScrollTrack == SB_THUMBTRACK;
        iPos = lpwin->iScrollTrack;
        if (lpwin->idScrollTimer) win32::KillTimer(hwnd, lpwin->idScrollTimer);
        lpwin->idScrollTimer = 0;
        lpwin->fTracking = lpwin->fPressed = FALSE;
        if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (!win32::IsWindow(hwnd)) return 0;
        win32::InvalidateRect(hwnd, NULL, FALSE);
        if (fThumb) NotifyScroll(hwnd, SB_THUMBPOSITION, iPos);
        if (win32::IsWindow(hwnd)) NotifyScroll(hwnd, SB_ENDSCROLL, 0);
        return 0;
    case WM_CANCELMODE: CancelScroll(hwnd, TRUE); return 0;
    case WM_CAPTURECHANGED: CancelScroll(hwnd, FALSE); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
