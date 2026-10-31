/* Classic control behavior for the source-level SDL UI. Fonts remain caller-owned. */
#include "internal.h"
#include <stdlib.h>

DWORD win32::GetSysColor(int i) {
    static const COLORREF rgcr[] = {
        RGB(192,192,192), RGB(0,128,128), RGB(0,0,128), RGB(128,128,128),
        RGB(192,192,192), RGB(255,255,255), RGB(0,0,0), RGB(0,0,0),
        RGB(0,0,0), RGB(255,255,255), RGB(192,192,192), RGB(192,192,192),
        RGB(128,128,128), RGB(0,0,128), RGB(255,255,255), RGB(192,192,192),
        RGB(128,128,128), RGB(128,128,128), RGB(0,0,0), RGB(192,192,192),
        RGB(255,255,255), RGB(0,0,0), RGB(192,192,192), RGB(0,0,0),
        RGB(255,255,225), RGB(0,0,0), RGB(0,0,255), RGB(16,132,208),
        RGB(181,181,181), RGB(0,0,128), RGB(192,192,192)
    };
    return i >= 0 && i < (int)(sizeof(rgcr) / sizeof(rgcr[0])) ? rgcr[i] : 0;
}

LRESULT win32::SendDlgItemMessageA(HWND hwnd, int id, UINT msg, WPARAM w, LPARAM l) {
    HWND hwndChild = win32::GetDlgItem(hwnd, id);
    return hwndChild ? win32::SendMessageA(hwndChild, msg, w, l) : 0;
}

BOOL win32::SetDlgItemTextA(HWND hwnd, int id, LPCSTR psz) {
    HWND hwndChild = win32::GetDlgItem(hwnd, id);
    return hwndChild && win32::SetWindowTextA(hwndChild, psz ? psz : "");
}

UINT win32::GetDlgItemTextA(HWND hwnd, int id, LPSTR psz, int cch) {
    if (!psz || cch <= 0) return 0;
    psz[0] = 0;
    return (UINT)win32::SendDlgItemMessageA(hwnd, id, WM_GETTEXT, (WPARAM)cch, (LPARAM)psz);
}

BOOL win32::CheckDlgButton(HWND hwnd, int id, UINT check) {
    if (!SDL_IsMainThread()) return FALSE;
    win32::SendDlgItemMessageA(hwnd, id, BM_SETCHECK, check, 0);
    return TRUE;
}

UINT win32::IsDlgButtonChecked(HWND hwnd, int id) {
    return (UINT)win32::SendDlgItemMessageA(hwnd, id, BM_GETCHECK, 0, 0);
}

BOOL win32::CheckRadioButton(HWND hwnd, int idFirst, int idLast, int idCheck) {
    HWND lp, *rghwnd;
    int c = 0, i = 0, id;
    if (!LpCompatWindow(hwnd) || idFirst > idLast) return FALSE;
    for (lp = win32::GetWindow(hwnd, GW_CHILD); lp; lp = win32::GetWindow(lp, GW_HWNDNEXT)) c++;
    rghwnd = (HWND *)(calloc(c ? (size_t)c : 1, sizeof(*rghwnd)));
    if (!rghwnd) return FALSE;
    for (lp = win32::GetWindow(hwnd, GW_CHILD); lp; lp = win32::GetWindow(lp, GW_HWNDNEXT)) rghwnd[i++] = lp;
    for (i = 0; i < c; i++) {
        lp = LpCompatWindow(rghwnd[i]);
        if (!lp || lp->hwndParent != hwnd) continue;
        id = win32::GetDlgCtrlID(lp);
        if (id >= idFirst && id <= idLast) win32::SendMessageA(lp, BM_SETCHECK, id == idCheck, 0);
    }
    free(rghwnd);
    return TRUE;
}

static UINT CheckMax(UINT type) {
    if (type == BS_3STATE || type == BS_AUTO3STATE) return BST_INDETERMINATE;
    return type == BS_CHECKBOX || type == BS_AUTOCHECKBOX ||
           type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON ? BST_CHECKED : 0;
}

static void NotifyControl(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent) win32::SendMessageA(lp->hwndParent, WM_COMMAND, MAKEWPARAM(lp->idChild, code), (LPARAM)hwnd);
}

static void AutoRadio(HWND hwnd) {
    HWND lp, hwndStart, hwndNext;
    UINT type;
    hwndStart = win32::GetWindow(win32::GetParent(hwnd), GW_CHILD);
    for (lp = hwndStart; lp; lp = win32::GetWindow(lp, GW_HWNDNEXT)) {
        if (lp->dwStyle & WS_GROUP) hwndStart = lp;
        if (lp == hwnd) break;
    }
    for (lp = hwndStart; lp; lp = hwndNext) {
        hwndNext = win32::GetWindow(lp, GW_HWNDNEXT);
        if (lp != hwndStart && (lp->dwStyle & WS_GROUP)) break;
        type = lp->dwStyle & BS_TYPEMASK;
        if (lp->iControl == 1 && (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON))
            win32::SendMessageA(lp, BM_SETCHECK, lp == hwnd, 0);
        if (!win32::IsWindow(hwnd)) return;
    }
}

static void ClickControl(HWND hwnd) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    if (!lp) return;
    type = lp->dwStyle & BS_TYPEMASK;
    if (type == BS_AUTOCHECKBOX) win32::SendMessageA(hwnd, BM_SETCHECK, !lp->bstCheck, 0);
    else if (type == BS_AUTO3STATE) win32::SendMessageA(hwnd, BM_SETCHECK, (lp->bstCheck + 1) % 3, 0);
    else if (type == BS_AUTORADIOBUTTON) AutoRadio(hwnd);
    if (win32::IsWindow(hwnd)) NotifyControl(hwnd, BN_CLICKED);
}

static BOOL FControlPoint(HWND hwnd, LPARAM l) {
    RECT rc;
    POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
    return win32::GetClientRect(hwnd, &rc) && win32::PtInRect(&rc, pt);
}

static void PaintControl(HWND hwnd) {
    HWND lp = LpCompatWindow(hwnd);
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr, hbrOwn = NULL;
    HPEN hpenOld;
    HFONT hfontOld = NULL;
    HGDIOBJ hbrOld;
    RECT rc, rcText, rcBox;
    SIZE size;
    UINT type, format;
    int x, y, d, j;
    BOOL fButton, fCheck;
    if (!lp || !(hdc = win32::BeginPaint(hwnd, &ps))) return;
    win32::GetClientRect(hwnd, &rc);
    type = lp->dwStyle & (lp->iControl == 1 ? BS_TYPEMASK : 0x1f);
    fButton = lp->iControl == 1;
    fCheck = fButton && CheckMax(type) != 0;
    win32::SetBkMode(hdc, TRANSPARENT);
    win32::SetTextColor(hdc, win32::GetSysColor(win32::IsWindowEnabled(hwnd) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    if (lp->hfontControl) hfontOld = (HFONT)win32::SelectObject(hdc, lp->hfontControl);
    hbr = (HBRUSH)win32::SendMessageA(lp->hwndParent,
                            fButton && (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) ? WM_CTLCOLORBTN : WM_CTLCOLORSTATIC,
                            (WPARAM)hdc, (LPARAM)hwnd);
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNFACE));
    win32::FillRect(hdc, &rc, hbr);
    rcText = rc;
    format = DT_LEFT | DT_WORDBREAK;
    if (fButton && (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON)) {
        if (type == BS_DEFPUSHBUTTON) {
            win32::FrameRect(hdc, &rc, (HBRUSH)win32::GetStockObject(BLACK_BRUSH)); win32::InflateRect(&rc, -1, -1);
        }
        CompatDrawClassicEdge(hdc, &rc, lp->fPressed, TRUE);
        win32::InflateRect(&rcText, -4, -3);
        if (lp->fPressed) win32::OffsetRect(&rcText, 1, 1);
        format = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    } else if (fCheck) {
        d = (((13) < (StarsMin(rc.right, rc.bottom))) ? (13) : (StarsMin(rc.right, rc.bottom)));
        y = (rc.bottom - d) / 2;
        x = lp->dwStyle & BS_LEFTTEXT ? rc.right - d : 0;
        win32::SetRect(&rcBox, x, y, x + d, y + d);
        hbrOld = win32::SelectObject(hdc, win32::GetStockObject(WHITE_BRUSH));
        hpenOld = (HPEN)win32::SelectObject(hdc, win32::GetStockObject(BLACK_PEN));
        if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) win32::Ellipse(hdc, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
        else { win32::FillRect(hdc, &rcBox, (HBRUSH)win32::GetStockObject(WHITE_BRUSH)); CompatDrawClassicEdge(hdc, &rcBox, TRUE, TRUE); }
        if (lp->bstCheck) {
            win32::InflateRect(&rcBox, -3, -3);
            if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) {
                win32::SelectObject(hdc, win32::GetStockObject(BLACK_BRUSH));
                win32::Ellipse(hdc, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
            } else {
                if (lp->bstCheck == BST_INDETERMINATE) win32::FillRect(hdc, &rcBox, (HBRUSH)win32::GetStockObject(GRAY_BRUSH));
                else {
                    for (j = 0; j < 3; j++) {
                        win32::MoveToEx(hdc, rcBox.left, rcBox.top + 2 + j, NULL);
                        win32::LineTo(hdc, rcBox.left + 2, rcBox.top + 4 + j);
                        win32::LineTo(hdc, rcBox.right - 1, rcBox.top + j);
                    }
                }
            }
        }
        win32::SelectObject(hdc, hpenOld); win32::SelectObject(hdc, hbrOld);
        if (lp->dwStyle & BS_LEFTTEXT) rcText.right -= d + 4;
        else rcText.left += d + 4;
        format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
    } else if (fButton && type == BS_GROUPBOX) {
        rcBox = rc; rcBox.top += 7;
        CompatDrawClassicEdge(hdc, &rcBox, TRUE, TRUE);
        rcText.left += 8;
        if (win32::GetTextExtentPoint32A(hdc, lp->pszTitle, (int)SDL_strlen(lp->pszTitle), &size)) {
            rcBox = rcText; rcBox.right = rcBox.left + size.cx + 4; rcBox.bottom = rcBox.top + size.cy;
            win32::FillRect(hdc, &rcBox, hbr);
        }
        format = DT_LEFT | DT_SINGLELINE;
    } else if (!fButton) {
        if (lp->dwStyle & SS_SUNKEN) CompatDrawClassicEdge(hdc, &rc, TRUE, TRUE);
        if (type == SS_CENTER) format |= DT_CENTER;
        else if (type == SS_RIGHT) format |= DT_RIGHT;
        else if (type == SS_SIMPLE || type == SS_LEFTNOWORDWRAP) format = DT_LEFT | DT_SINGLELINE;
        if (lp->dwStyle & SS_NOPREFIX) format |= DT_NOPREFIX;
        if (lp->dwStyle & SS_CENTERIMAGE) format = (format & ~DT_WORDBREAK) | DT_VCENTER | DT_SINGLELINE;
    }
    if (fButton && (lp->dwStyle & BS_MULTILINE) && type != BS_GROUPBOX) {
        format = (format & ~DT_SINGLELINE) | DT_WORDBREAK;
        rcBox = rcText;
        win32::DrawTextA(hdc, lp->pszTitle, -1, &rcBox, format | DT_CALCRECT);
        rcText.top += (((0) > (((rcText.bottom - rcText.top) - (rcBox.bottom - rcBox.top)) / 2)) ? (0) : (((rcText.bottom - rcText.top) - (rcBox.bottom - rcBox.top)) / 2));
    }
    if (!fButton && type == SS_ICON) win32::DrawIcon(hdc, 0, 0, win32::LoadIconA(lp->lpclass->wc.hInstance, lp->pszTitle));
    else win32::DrawTextA(hdc, lp->pszTitle, -1, &rcText, format);
    if (fButton && win32::GetFocus() == hwnd && type != BS_GROUPBOX) {
        rcBox = rcText;
        if (fCheck && lp->pszTitle[0]) {
            /* An inset of the label area can cross glyphs; surround the
               measured text instead, leaving room for glyph overhang. */
            win32::DrawTextA(hdc, lp->pszTitle, -1, &rcBox, format | DT_CALCRECT);
            if (format & DT_SINGLELINE) win32::OffsetRect(&rcBox, 0, (rcText.bottom - rcBox.bottom) / 2);
            win32::InflateRect(&rcBox, 2, 2);
            /* Compact controls cannot fit the full vertical padding. */
            rcBox.top = StarsMax(rc.top, rcBox.top);
            rcBox.bottom = StarsMin(rc.bottom, rcBox.bottom);
        } else {
            win32::InflateRect(&rcBox, -1, -1);
        }
        win32::FrameRect(hdc, &rcBox, (HBRUSH)win32::GetStockObject(GRAY_BRUSH));
    }
    if (hfontOld) win32::SelectObject(hdc, hfontOld);
    if (hbrOwn) win32::DeleteObject(hbrOwn);
    win32::EndPaint(hwnd, &ps);
}

static LRESULT ControlCommon(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    LRESULT result;
    if (!lp) return 0;
    switch (msg) {
    case WM_GETFONT: return (LRESULT)lp->hfontControl;
    case WM_SETFONT: lp->hfontControl = (HFONT)w; if (l) win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_PAINT: PaintControl(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SETTEXT:
        result = win32::DefWindowProcA(hwnd, msg, w, l);
        if (result && win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, NULL, TRUE);
        return result;
    case WM_ENABLE: case WM_SIZE: win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}

static LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    BOOL fClick;
    if (!lp) return 0;
    type = lp->dwStyle & BS_TYPEMASK;
    switch (msg) {
    case WM_NCCREATE: lp->iControl = 1; return TRUE;
    case WM_CREATE:
        return type <= BS_GROUPBOX || type == BS_AUTORADIOBUTTON ?
               (lp->dwStyle & (BS_ICON | BS_BITMAP) ? -1 : 0) : -1;
    case WM_GETDLGCODE:
        return DLGC_BUTTON | (type == BS_DEFPUSHBUTTON ? DLGC_DEFPUSHBUTTON : type == BS_PUSHBUTTON ? DLGC_UNDEFPUSHBUTTON :
                             type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON ? DLGC_RADIOBUTTON : 0);
    case BM_GETCHECK: return CheckMax(type) ? lp->bstCheck : 0;
    case BM_SETCHECK:
        lp->bstCheck = ((((UINT)w) < (CheckMax(type))) ? ((UINT)w) : (CheckMax(type)));
        if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) {
            if (lp->bstCheck) lp->dwStyle |= WS_TABSTOP; else lp->dwStyle &= ~WS_TABSTOP;
        }
        win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    case BM_GETSTATE: return lp->bstCheck | (lp->fPressed ? BST_PUSHED : 0) | (win32::GetFocus() == hwnd ? BST_FOCUS : 0);
    case BM_SETSTATE: lp->fPressed = !!w; win32::InvalidateRect(hwnd, NULL, TRUE); return 0;
    case BM_SETSTYLE:
        lp->dwStyle = (lp->dwStyle & ~BS_TYPEMASK) | ((DWORD)w & BS_TYPEMASK);
        if (l) win32::InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case BM_CLICK:
        if (type == BS_GROUPBOX) return 0;
        win32::SendMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(1, 1));
        if (win32::IsWindow(hwnd)) win32::SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(1, 1));
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (type == BS_GROUPBOX) return 0;
        win32::SetFocus(hwnd);
        if (!win32::IsWindow(hwnd)) return 0;
        win32::SetCapture(hwnd);
        if (win32::IsWindow(hwnd) && win32::GetCapture() == hwnd) {
            lp->fTracking = TRUE; lp->fPressed = TRUE;
            win32::InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (lp->fTracking && !lp->fSpace) {
            lp->fPressed = FControlPoint(hwnd, l);
            win32::InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (!lp->fTracking || lp->fSpace) return 0;
        fClick = lp->fPressed && FControlPoint(hwnd, l);
        lp->fTracking = lp->fPressed = FALSE;
        if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (!win32::IsWindow(hwnd)) return 0;
        win32::InvalidateRect(hwnd, NULL, TRUE);
        if (fClick) ClickControl(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (w == VK_SPACE && win32::IsWindowEnabled(hwnd) && type != BS_GROUPBOX && !lp->fSpace) {
            win32::SetCapture(hwnd);
            if (win32::IsWindow(hwnd) && win32::GetCapture() == hwnd) {
                lp->fSpace = lp->fPressed = lp->fTracking = TRUE;
                win32::InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return 0;
    case WM_KEYUP:
        if (w == VK_SPACE && lp->fSpace) {
            lp->fSpace = lp->fPressed = lp->fTracking = FALSE;
            if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
            if (win32::IsWindow(hwnd)) { win32::InvalidateRect(hwnd, NULL, TRUE); ClickControl(hwnd); }
        }
        return 0;
    case WM_CAPTURECHANGED: case WM_CANCELMODE:
        lp->fTracking = lp->fPressed = lp->fSpace = FALSE;
        if (msg == WM_CANCELMODE && win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_SETFOCUS: case WM_KILLFOCUS:
        if (msg == WM_KILLFOCUS && lp->fSpace) win32::SendMessageA(hwnd, WM_CANCELMODE, 0, 0);
        if (win32::IsWindow(hwnd)) {
            win32::InvalidateRect(hwnd, NULL, TRUE);
            if (lp->dwStyle & BS_NOTIFY) NotifyControl(hwnd, msg == WM_SETFOCUS ? BN_SETFOCUS : BN_KILLFOCUS);
        }
        return 0;
    default: return ControlCommon(hwnd, msg, w, l);
    }
}

static LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    if (!lp) return 0;
    type = lp->dwStyle & 0x1f;
    if (msg == WM_NCCREATE) { lp->iControl = 2; return TRUE; }
    if (msg == WM_CREATE) return type <= SS_ICON || type == SS_SIMPLE || type == SS_LEFTNOWORDWRAP ? 0 : -1;
    if (msg == WM_GETDLGCODE) return DLGC_STATIC;
    return ControlCommon(hwnd, msg, w, l);
}

BOOL FCompatControlClass(LPCSTR psz, HINSTANCE hInstance) {
    WNDCLASSA wc = {};
    if ((UINT_PTR)psz <= UINT16_MAX) return FALSE;
    if (SDL_strcasecmp(psz, "Button") == 0) { wc.lpszClassName = "Button"; wc.lpfnWndProc = ButtonProc; wc.style = CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "Static") == 0) { wc.lpszClassName = "Static"; wc.lpfnWndProc = StaticProc; }
    else if (SDL_strcasecmp(psz, "StarsMessageText") == 0) { wc.lpszClassName = "StarsMessageText"; wc.lpfnWndProc = CompatMessageTextProc; }
    else if (SDL_strcasecmp(psz, "Edit") == 0) { wc.lpszClassName = "Edit"; wc.lpfnWndProc = CompatEditProc; wc.style = CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "ListBox") == 0) { wc.lpszClassName = "ListBox"; wc.lpfnWndProc = CompatListProc; wc.style = CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "ScrollBar") == 0) { wc.lpszClassName = "ScrollBar"; wc.lpfnWndProc = CompatScrollProc; }
    else if (SDL_strcasecmp(psz, "ComboBox") == 0) { wc.lpszClassName = "ComboBox"; wc.lpfnWndProc = CompatComboProc; wc.style = CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "StarsComboPopup") == 0) { wc.lpszClassName = "StarsComboPopup"; wc.lpfnWndProc = CompatComboPopupProc; }
    else if (SDL_strcasecmp(psz, "ToolbarWindow32") == 0) { wc.lpszClassName = "ToolbarWindow32"; wc.lpfnWndProc = CompatToolbarProc; wc.style = CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "StarsCompatToolbarTip") == 0) { wc.lpszClassName = "StarsCompatToolbarTip"; wc.lpfnWndProc = CompatToolbarTipProc; }
    else if (SDL_strcasecmp(psz, "#32770") == 0) { wc.lpszClassName = "#32770"; wc.lpfnWndProc = CompatDialogProc; wc.cbWndExtra = 3 * sizeof(LONG_PTR); }
    else return FALSE;
    wc.hInstance = hInstance;
    wc.hCursor = win32::LoadCursorA(NULL, MAKEINTRESOURCE(32512));
    return win32::RegisterClassA(&wc) != 0;
}
