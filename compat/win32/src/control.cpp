/* Classic control behavior for the source-level SDL UI. Fonts remain caller-owned. */
#include "internal.h"
#include <cstdlib>

DWORD win32::GetSysColor(int i) {
    static const COLORREF rgcr[] = {
        win32::RGB(192,192,192), win32::RGB(0,128,128), win32::RGB(0,0,128), win32::RGB(128,128,128),
        win32::RGB(192,192,192), win32::RGB(255,255,255), win32::RGB(0,0,0), win32::RGB(0,0,0),
        win32::RGB(0,0,0), win32::RGB(255,255,255), win32::RGB(192,192,192), win32::RGB(192,192,192),
        win32::RGB(128,128,128), win32::RGB(0,0,128), win32::RGB(255,255,255), win32::RGB(192,192,192),
        win32::RGB(128,128,128), win32::RGB(128,128,128), win32::RGB(0,0,0), win32::RGB(192,192,192),
        win32::RGB(255,255,255), win32::RGB(0,0,0), win32::RGB(192,192,192), win32::RGB(0,0,0),
        win32::RGB(255,255,225), win32::RGB(0,0,0), win32::RGB(0,0,255), win32::RGB(16,132,208),
        win32::RGB(181,181,181), win32::RGB(0,0,128), win32::RGB(192,192,192)
    };
    return i >= 0 && i < static_cast<int>((sizeof(rgcr) / sizeof(rgcr[0]))) ? rgcr[i] : 0;
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
    return static_cast<UINT>(win32::SendDlgItemMessageA(hwnd, id, win32::WM_GETTEXT, static_cast<WPARAM>(cch), reinterpret_cast<LPARAM>(psz)));
}

BOOL win32::CheckDlgButton(HWND hwnd, int id, UINT check) {
    HWND hwndChild;
    if (!SDL_IsMainThread()) return 0;
    hwndChild = win32::GetDlgItem(hwnd, id);
    if (!hwndChild) return 0;
    // BM_SETCHECK returns zero even when the button update succeeds.
    win32::SendMessageA(hwndChild, win32::BM_SETCHECK, check, 0);
    return 1;
}

UINT win32::IsDlgButtonChecked(HWND hwnd, int id) {
    return static_cast<UINT>(win32::SendDlgItemMessageA(hwnd, id, win32::BM_GETCHECK, 0, 0));
}

BOOL win32::CheckRadioButton(HWND hwnd, int idFirst, int idLast, int idCheck) {
    HWND lp, *rghwnd;
    int c = 0, i = 0, id;
    if (!LpCompatWindow(hwnd) || idFirst > idLast) return 0;
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT)) c++;
    rghwnd = static_cast<HWND *>((std::calloc(c ? static_cast<std::size_t>(c) : 1, sizeof(*rghwnd))));
    NATIVEARRAYOWNER<HWND, std::free> rghwndOwner(rghwnd);
    if (!rghwnd) return 0;
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT)) rghwnd[i++] = lp;
    for (i = 0; i < c; i++) {
        lp = LpCompatWindow(rghwnd[i]);
        if (!lp || lp->hwndParent != hwnd) continue;
        id = win32::GetDlgCtrlID(lp);
        if (id >= idFirst && id <= idLast) win32::SendMessageA(lp, win32::BM_SETCHECK, id == idCheck, 0);
    }
    return 1;
}

static UINT CheckMax(UINT type) {
    if (type == win32::BS_3STATE || type == win32::BS_AUTO3STATE) return win32::BST_INDETERMINATE;
    return type == win32::BS_CHECKBOX || type == win32::BS_AUTOCHECKBOX ||
           type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON ? win32::BST_CHECKED : 0;
}

static void NotifyControl(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent) win32::SendMessageA(lp->hwndParent, win32::WM_COMMAND, win32::MAKEWPARAM(lp->idChild, code), reinterpret_cast<LPARAM>(hwnd));
}

static void AutoRadio(HWND hwnd) {
    HWND lp, hwndStart, hwndNext;
    UINT type;
    hwndStart = win32::GetWindow(win32::GetParent(hwnd), win32::GW_CHILD);
    for (lp = hwndStart; lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT)) {
        if (lp->dwStyle & win32::WS_GROUP) hwndStart = lp;
        if (lp == hwnd) break;
    }
    for (lp = hwndStart; lp; lp = hwndNext) {
        hwndNext = win32::GetWindow(lp, win32::GW_HWNDNEXT);
        if (lp != hwndStart && (lp->dwStyle & win32::WS_GROUP)) break;
        type = lp->dwStyle & win32::BS_TYPEMASK;
        if (lp->iControl == 1 && (type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON))
            win32::SendMessageA(lp, win32::BM_SETCHECK, lp == hwnd, 0);
        if (!win32::IsWindow(hwnd)) return;
    }
}

static void ClickControl(HWND hwnd) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    if (!lp) return;
    type = lp->dwStyle & win32::BS_TYPEMASK;
    if (type == win32::BS_AUTOCHECKBOX) win32::SendMessageA(hwnd, win32::BM_SETCHECK, !lp->bstCheck, 0);
    else if (type == win32::BS_AUTO3STATE) win32::SendMessageA(hwnd, win32::BM_SETCHECK, (lp->bstCheck + 1) % 3, 0);
    else if (type == win32::BS_AUTORADIOBUTTON) AutoRadio(hwnd);
    if (win32::IsWindow(hwnd)) NotifyControl(hwnd, win32::BN_CLICKED);
}

static BOOL FControlPoint(HWND hwnd, LPARAM l) {
    RECT rc;
    POINT pt = {static_cast<std::int16_t>(LOWORD(l)), static_cast<std::int16_t>(HIWORD(l))};
    return win32::GetClientRect(hwnd, &rc) && win32::PtInRect(&rc, pt);
}

static void PaintControl(HWND hwnd) {
    HWND lp = LpCompatWindow(hwnd);
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr, hbrOwn = nullptr;
    HPEN hpenOld;
    HFONT hfontOld = nullptr;
    HGDIOBJ hbrOld;
    RECT rc, rcText, rcBox;
    SIZE size;
    UINT type, format;
    int x, y, d, j;
    BOOL fButton, fCheck;
    if (!lp || !(hdc = win32::BeginPaint(hwnd, &ps))) return;
    win32::GetClientRect(hwnd, &rc);
    type = lp->dwStyle & (lp->iControl == 1 ? win32::BS_TYPEMASK : 0x1f);
    fButton = lp->iControl == 1;
    fCheck = fButton && CheckMax(type) != 0;
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    win32::SetTextColor(hdc, win32::GetSysColor(win32::IsWindowEnabled(hwnd) ? win32::COLOR_BTNTEXT : win32::COLOR_GRAYTEXT));
    if (lp->hfontControl) hfontOld = static_cast<HFONT>(win32::SelectObject(hdc, lp->hfontControl));
    hbr = reinterpret_cast<HBRUSH>(win32::SendMessageA(lp->hwndParent,
                            fButton && (type == win32::BS_PUSHBUTTON || type == win32::BS_DEFPUSHBUTTON) ? win32::WM_CTLCOLORBTN : win32::WM_CTLCOLORSTATIC,
                            reinterpret_cast<WPARAM>(hdc), reinterpret_cast<LPARAM>(hwnd)));
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
    win32::FillRect(hdc, &rc, hbr);
    rcText = rc;
    format = win32::DT_LEFT | win32::DT_WORDBREAK;
    if (fButton && (type == win32::BS_PUSHBUTTON || type == win32::BS_DEFPUSHBUTTON)) {
        if (type == win32::BS_DEFPUSHBUTTON) {
            win32::FrameRect(hdc, &rc, static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH))); win32::InflateRect(&rc, -1, -1);
        }
        CompatDrawClassicEdge(hdc, &rc, lp->fPressed, 1);
        win32::InflateRect(&rcText, -4, -3);
        if (lp->fPressed) win32::OffsetRect(&rcText, 1, 1);
        format = win32::DT_CENTER | win32::DT_VCENTER | win32::DT_SINGLELINE;
    } else if (fCheck) {
        d = (((13) < (StarsMin(rc.right, rc.bottom))) ? (13) : (StarsMin(rc.right, rc.bottom)));
        y = (rc.bottom - d) / 2;
        x = lp->dwStyle & win32::BS_LEFTTEXT ? rc.right - d : 0;
        win32::SetRect(&rcBox, x, y, x + d, y + d);
        hbrOld = win32::SelectObject(hdc, win32::GetStockObject(win32::WHITE_BRUSH));
        hpenOld = static_cast<HPEN>(win32::SelectObject(hdc, win32::GetStockObject(win32::BLACK_PEN)));
        if (type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON) win32::Ellipse(hdc, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
        else { win32::FillRect(hdc, &rcBox, static_cast<HBRUSH>(win32::GetStockObject(win32::WHITE_BRUSH))); CompatDrawClassicEdge(hdc, &rcBox, 1, 1); }
        if (lp->bstCheck) {
            win32::InflateRect(&rcBox, -3, -3);
            if (type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON) {
                win32::SelectObject(hdc, win32::GetStockObject(win32::BLACK_BRUSH));
                win32::Ellipse(hdc, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
            } else {
                if (lp->bstCheck == win32::BST_INDETERMINATE) win32::FillRect(hdc, &rcBox, static_cast<HBRUSH>(win32::GetStockObject(win32::GRAY_BRUSH)));
                else {
                    for (j = 0; j < 3; j++) {
                        win32::MoveToEx(hdc, rcBox.left, rcBox.top + 2 + j, nullptr);
                        win32::LineTo(hdc, rcBox.left + 2, rcBox.top + 4 + j);
                        win32::LineTo(hdc, rcBox.right - 1, rcBox.top + j);
                    }
                }
            }
        }
        win32::SelectObject(hdc, hpenOld); win32::SelectObject(hdc, hbrOld);
        if (lp->dwStyle & win32::BS_LEFTTEXT) rcText.right -= d + 4;
        else rcText.left += d + 4;
        format = win32::DT_LEFT | win32::DT_VCENTER | win32::DT_SINGLELINE;
    } else if (fButton && type == win32::BS_GROUPBOX) {
        rcBox = rc; rcBox.top += 7;
        CompatDrawClassicEdge(hdc, &rcBox, 1, 1);
        rcText.left += 8;
        if (win32::GetTextExtentPoint32A(hdc, lp->pszTitle, static_cast<int>(SDL_strlen(lp->pszTitle)), &size)) {
            rcBox = rcText; rcBox.right = rcBox.left + size.cx + 4; rcBox.bottom = rcBox.top + size.cy;
            win32::FillRect(hdc, &rcBox, hbr);
        }
        format = win32::DT_LEFT | win32::DT_SINGLELINE;
    } else if (!fButton) {
        if (lp->dwStyle & win32::SS_SUNKEN) CompatDrawClassicEdge(hdc, &rc, 1, 1);
        if (type == win32::SS_CENTER) format |= win32::DT_CENTER;
        else if (type == win32::SS_RIGHT) format |= win32::DT_RIGHT;
        else if (type == win32::SS_SIMPLE || type == win32::SS_LEFTNOWORDWRAP) format = win32::DT_LEFT | win32::DT_SINGLELINE;
        if (lp->dwStyle & win32::SS_NOPREFIX) format |= win32::DT_NOPREFIX;
        if (lp->dwStyle & win32::SS_CENTERIMAGE) format = (format & ~win32::DT_WORDBREAK) | win32::DT_VCENTER | win32::DT_SINGLELINE;
    }
    if (fButton && (lp->dwStyle & win32::BS_MULTILINE) && type != win32::BS_GROUPBOX) {
        format = (format & ~win32::DT_SINGLELINE) | win32::DT_WORDBREAK;
        rcBox = rcText;
        win32::DrawTextA(hdc, lp->pszTitle, -1, &rcBox, format | win32::DT_CALCRECT);
        rcText.top += (((0) > (((rcText.bottom - rcText.top) - (rcBox.bottom - rcBox.top)) / 2)) ? (0) : (((rcText.bottom - rcText.top) - (rcBox.bottom - rcBox.top)) / 2));
    }
    if (!fButton && type == win32::SS_ICON) win32::DrawIcon(hdc, 0, 0, win32::LoadIconA(lp->lpclass->wc.hInstance, lp->pszTitle));
    else win32::DrawTextA(hdc, lp->pszTitle, -1, &rcText, format);
    if (fButton && win32::GetFocus() == hwnd && type != win32::BS_GROUPBOX) {
        rcBox = rcText;
        if (fCheck && lp->pszTitle[0]) {
            /* An inset of the label area can cross glyphs; surround the
               measured text instead, leaving room for glyph overhang. */
            win32::DrawTextA(hdc, lp->pszTitle, -1, &rcBox, format | win32::DT_CALCRECT);
            if (format & win32::DT_SINGLELINE) win32::OffsetRect(&rcBox, 0, (rcText.bottom - rcBox.bottom) / 2);
            win32::InflateRect(&rcBox, 2, 2);
            /* Compact controls cannot fit the full vertical padding. */
            rcBox.top = StarsMax(rc.top, rcBox.top);
            rcBox.bottom = StarsMin(rc.bottom, rcBox.bottom);
        } else {
            win32::InflateRect(&rcBox, -1, -1);
        }
        win32::FrameRect(hdc, &rcBox, static_cast<HBRUSH>(win32::GetStockObject(win32::GRAY_BRUSH)));
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
    case win32::WM_GETFONT: return reinterpret_cast<LRESULT>(lp->hfontControl);
    case win32::WM_SETFONT: lp->hfontControl = reinterpret_cast<HFONT>(w); if (l) win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_PAINT: PaintControl(hwnd); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::WM_SETTEXT:
        result = win32::DefWindowProcA(hwnd, msg, w, l);
        if (result && win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 1);
        return result;
    case win32::WM_ENABLE: case win32::WM_SIZE: win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}

static LRESULT  ButtonProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    BOOL fClick;
    if (!lp) return 0;
    type = lp->dwStyle & win32::BS_TYPEMASK;
    switch (msg) {
    case win32::WM_NCCREATE: lp->iControl = 1; return 1;
    case win32::WM_CREATE:
        return type <= win32::BS_GROUPBOX || type == win32::BS_AUTORADIOBUTTON ?
               (lp->dwStyle & (win32::BS_ICON | win32::BS_BITMAP) ? -1 : 0) : -1;
    case win32::WM_GETDLGCODE:
        return win32::DLGC_BUTTON | (type == win32::BS_DEFPUSHBUTTON ? win32::DLGC_DEFPUSHBUTTON : type == win32::BS_PUSHBUTTON ? win32::DLGC_UNDEFPUSHBUTTON :
                             type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON ? win32::DLGC_RADIOBUTTON : 0);
    case win32::BM_GETCHECK: return CheckMax(type) ? lp->bstCheck : 0;
    case win32::BM_SETCHECK:
        lp->bstCheck = (((static_cast<UINT>(w)) < (CheckMax(type))) ? (static_cast<UINT>(w)) : (CheckMax(type)));
        if (type == win32::BS_RADIOBUTTON || type == win32::BS_AUTORADIOBUTTON) {
            if (lp->bstCheck) lp->dwStyle |= win32::WS_TABSTOP; else lp->dwStyle &= ~win32::WS_TABSTOP;
        }
        win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::BM_GETSTATE: return lp->bstCheck | (lp->fPressed ? win32::BST_PUSHED : 0) | (win32::GetFocus() == hwnd ? win32::BST_FOCUS : 0);
    case win32::BM_SETSTATE: lp->fPressed = !!w; win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::BM_SETSTYLE:
        lp->dwStyle = (lp->dwStyle & ~win32::BS_TYPEMASK) | (static_cast<DWORD>(w) & win32::BS_TYPEMASK);
        if (l) win32::InvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::BM_CLICK:
        if (type == win32::BS_GROUPBOX) return 0;
        win32::SendMessageA(hwnd, win32::WM_LBUTTONDOWN, win32::MK_LBUTTON, win32::MAKELPARAM(1, 1));
        if (win32::IsWindow(hwnd)) win32::SendMessageA(hwnd, win32::WM_LBUTTONUP, 0, win32::MAKELPARAM(1, 1));
        return 0;
    case win32::WM_LBUTTONDOWN: case win32::WM_LBUTTONDBLCLK:
        if (type == win32::BS_GROUPBOX) return 0;
        win32::SetFocus(hwnd);
        if (!win32::IsWindow(hwnd)) return 0;
        win32::SetCapture(hwnd);
        if (win32::IsWindow(hwnd) && win32::GetCapture() == hwnd) {
            lp->fTracking = 1; lp->fPressed = 1;
            win32::InvalidateRect(hwnd, nullptr, 1);
        }
        return 0;
    case win32::WM_MOUSEMOVE:
        if (lp->fTracking && !lp->fSpace) {
            lp->fPressed = FControlPoint(hwnd, l);
            win32::InvalidateRect(hwnd, nullptr, 1);
        }
        return 0;
    case win32::WM_LBUTTONUP:
        if (!lp->fTracking || lp->fSpace) return 0;
        fClick = lp->fPressed && FControlPoint(hwnd, l);
        lp->fTracking = lp->fPressed = 0;
        if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (!win32::IsWindow(hwnd)) return 0;
        win32::InvalidateRect(hwnd, nullptr, 1);
        if (fClick) ClickControl(hwnd);
        return 0;
    case win32::WM_KEYDOWN:
        if (w == win32::VK_SPACE && win32::IsWindowEnabled(hwnd) && type != win32::BS_GROUPBOX && !lp->fSpace) {
            win32::SetCapture(hwnd);
            if (win32::IsWindow(hwnd) && win32::GetCapture() == hwnd) {
                lp->fSpace = lp->fPressed = lp->fTracking = 1;
                win32::InvalidateRect(hwnd, nullptr, 1);
            }
        }
        return 0;
    case win32::WM_KEYUP:
        if (w == win32::VK_SPACE && lp->fSpace) {
            lp->fSpace = lp->fPressed = lp->fTracking = 0;
            if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
            if (win32::IsWindow(hwnd)) { win32::InvalidateRect(hwnd, nullptr, 1); ClickControl(hwnd); }
        }
        return 0;
    case win32::WM_CAPTURECHANGED: case win32::WM_CANCELMODE:
        lp->fTracking = lp->fPressed = lp->fSpace = 0;
        if (msg == win32::WM_CANCELMODE && win32::GetCapture() == hwnd) win32::ReleaseCapture();
        if (win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::WM_SETFOCUS: case win32::WM_KILLFOCUS:
        if (msg == win32::WM_KILLFOCUS && lp->fSpace) win32::SendMessageA(hwnd, win32::WM_CANCELMODE, 0, 0);
        if (win32::IsWindow(hwnd)) {
            win32::InvalidateRect(hwnd, nullptr, 1);
            if (lp->dwStyle & win32::BS_NOTIFY) NotifyControl(hwnd, msg == win32::WM_SETFOCUS ? win32::BN_SETFOCUS : win32::BN_KILLFOCUS);
        }
        return 0;
    default: return ControlCommon(hwnd, msg, w, l);
    }
}

static LRESULT  StaticProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT type;
    if (!lp) return 0;
    type = lp->dwStyle & 0x1f;
    if (msg == win32::WM_NCCREATE) { lp->iControl = 2; return 1; }
    if (msg == win32::WM_CREATE) return type <= win32::SS_ICON || type == win32::SS_SIMPLE || type == win32::SS_LEFTNOWORDWRAP ? 0 : -1;
    if (msg == win32::WM_GETDLGCODE) return win32::DLGC_STATIC;
    return ControlCommon(hwnd, msg, w, l);
}

BOOL FCompatControlClass(LPCSTR psz, HINSTANCE hInstance) {
    WNDCLASSA wc = {};
    if (reinterpret_cast<UINT_PTR>(psz) <= UINT16_MAX) return 0;
    if (SDL_strcasecmp(psz, "Button") == 0) { wc.lpszClassName = "Button"; wc.lpfnWndProc = ButtonProc; wc.style = win32::CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "Static") == 0) { wc.lpszClassName = "Static"; wc.lpfnWndProc = StaticProc; }
    else if (SDL_strcasecmp(psz, "StarsMessageText") == 0) { wc.lpszClassName = "StarsMessageText"; wc.lpfnWndProc = CompatMessageTextProc; }
    else if (SDL_strcasecmp(psz, "Edit") == 0) { wc.lpszClassName = "Edit"; wc.lpfnWndProc = CompatEditProc; wc.style = win32::CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "ListBox") == 0) { wc.lpszClassName = "ListBox"; wc.lpfnWndProc = CompatListProc; wc.style = win32::CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "ScrollBar") == 0) { wc.lpszClassName = "ScrollBar"; wc.lpfnWndProc = CompatScrollProc; }
    else if (SDL_strcasecmp(psz, "ComboBox") == 0) { wc.lpszClassName = "ComboBox"; wc.lpfnWndProc = CompatComboProc; wc.style = win32::CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "StarsComboPopup") == 0) { wc.lpszClassName = "StarsComboPopup"; wc.lpfnWndProc = CompatComboPopupProc; }
    else if (SDL_strcasecmp(psz, "ToolbarWindow32") == 0) { wc.lpszClassName = "ToolbarWindow32"; wc.lpfnWndProc = CompatToolbarProc; wc.style = win32::CS_DBLCLKS; }
    else if (SDL_strcasecmp(psz, "StarsCompatToolbarTip") == 0) { wc.lpszClassName = "StarsCompatToolbarTip"; wc.lpfnWndProc = CompatToolbarTipProc; }
    else if (SDL_strcasecmp(psz, "#32770") == 0) { wc.lpszClassName = "#32770"; wc.lpfnWndProc = CompatDialogProc; wc.cbWndExtra = 3 * sizeof(LONG_PTR); }
    else return 0;
    wc.hInstance = hInstance;
    wc.hCursor = win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512));
    return win32::RegisterClassA(&wc) != 0;
}
