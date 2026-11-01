/* Source-level message boxes use the same controls and modal queue as dialogs. */
#include "cleanup.h"
#include <new>
#include <stars_compat.h>
#include <stdlib.h>
#include <string.h>

struct COMPATMSGBOX {
    LPCSTR pszText;
    UINT type, format;
    int rgid[3], cButtons, iDefault, idCancel;
    int cxText, cyText, cyTotal, yScroll;
    HWND hwndBody, hwndScroll, hwndLastFocus;
    HFONT hfont, hfontIcon;
};
static const int rgrid[7][3] = {
    {win32::IDOK, 0, 0}, {win32::IDOK, win32::IDCANCEL, 0}, {win32::IDABORT, win32::IDRETRY, win32::IDIGNORE},
    {win32::IDYES, win32::IDNO, win32::IDCANCEL}, {win32::IDYES, win32::IDNO, 0}, {win32::IDRETRY, win32::IDCANCEL, 0},
    {win32::IDCANCEL, win32::IDTRYAGAIN, win32::IDCONTINUE}
};
static LPCSTR PszMessageButton(int id) {
    switch (id) {
    case win32::IDOK: return "&OK";
    case win32::IDCANCEL: return "Cancel";
    case win32::IDABORT: return "&Abort";
    case win32::IDRETRY: return "&Retry";
    case win32::IDIGNORE: return "&Ignore";
    case win32::IDYES: return "&Yes";
    case win32::IDNO: return "&No";
    case win32::IDTRYAGAIN: return "&Try Again";
    default: return "&Continue";
    }
}
static void ScrollMessage(COMPATMSGBOX *lp, int y) {
    lp->yScroll = (((StarsMax(0, lp->cyTotal - lp->cyText)) < (StarsMax(0, y))) ? (StarsMax(0, lp->cyTotal - lp->cyText)) : (StarsMax(0, y)));
    if (win32::IsWindow(lp->hwndScroll)) win32::SetScrollPos(lp->hwndScroll, win32::SB_CTL, lp->yScroll, 1);
    win32::InvalidateRect(lp->hwndBody, nullptr, 1);
}
LRESULT  CompatMessageTextProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATMSGBOX *lp = reinterpret_cast<COMPATMSGBOX *>(win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA));
    HDC hdc;
    PAINTSTRUCT ps;
    HGDIOBJ hold;
    HBRUSH hbr;
    HRGN hrgn;
    RECT rc;
    int y;
    if (msg == win32::WM_NCCREATE) {
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, reinterpret_cast<LONG_PTR>((reinterpret_cast<CREATESTRUCTA *>(l))->lpCreateParams)); return 1;
    }
    if (!lp) return win32::DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps); if (!hdc) return 0;
        rc = RECT{0, 0, lp->cxText, lp->cyText};
        hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
        if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
        hrgn = win32::CreateRectRgn(0, 0, lp->cxText, lp->cyText);
        if (hrgn) { win32::SelectClipRgn(hdc, hrgn); win32::DeleteObject(hrgn); }
        hold = win32::SelectObject(hdc, lp->hfont); win32::SetBkMode(hdc, win32::TRANSPARENT);
        win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_BTNTEXT));
        rc.top = -lp->yScroll; rc.bottom = lp->cyTotal - lp->yScroll;
        win32::DrawTextA(hdc, lp->pszText, -1, &rc, lp->format);
        if (hold) win32::SelectObject(hdc, hold);
        win32::EndPaint(hwnd, &ps); return 0;
    case win32::WM_GETDLGCODE: return win32::DLGC_WANTARROWS;
    case win32::WM_MOUSEWHEEL: ScrollMessage(lp, lp->yScroll - static_cast<SHORT>(HIWORD(w)) / win32::WHEEL_DELTA * 48); return 0;
    case win32::WM_KEYDOWN:
        y = lp->yScroll;
        if (w == win32::VK_UP) y -= 16;
        else if (w == win32::VK_DOWN) y += 16;
        else if (w == win32::VK_PRIOR) y -= lp->cyText;
        else if (w == win32::VK_NEXT) y += lp->cyText;
        else if (w == win32::VK_HOME) y = 0;
        else if (w == win32::VK_END) y = lp->cyTotal;
        else return 0;
        ScrollMessage(lp, y); return 0;
    case win32::WM_VSCROLL:
        y = lp->yScroll;
        switch (LOWORD(w)) {
        case win32::SB_LINEUP: y -= 16; break;
        case win32::SB_LINEDOWN: y += 16; break;
        case win32::SB_PAGEUP: y -= lp->cyText; break;
        case win32::SB_PAGEDOWN: y += lp->cyText; break;
        case win32::SB_TOP: y = 0; break;
        case win32::SB_BOTTOM: y = lp->cyTotal; break;
        case win32::SB_THUMBPOSITION: case win32::SB_THUMBTRACK:
            y = win32::IsWindow(lp->hwndScroll) ? lp->hwndScroll->iScrollTrack : HIWORD(w); break;
        default: return 0;
        }
        ScrollMessage(lp, y); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
static void PaintMessageIcon(HWND hwnd, COMPATMSGBOX *lp) {
    PAINTSTRUCT ps;
    HDC hdc = win32::BeginPaint(hwnd, &ps);
    HBRUSH hbr, holdBrush;
    HGDIOBJ holdFont;
    RECT rc;
    COLORREF cr;
    LPCSTR psz;
    UINT icon = lp->type & 0xf0;
    if (!hdc) return;
    win32::GetClientRect(hwnd, &rc); hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    if (icon) {
        cr = icon == win32::MB_ICONHAND ? win32::RGB(200, 30, 30) : icon == win32::MB_ICONEXCLAMATION ? win32::RGB(255, 210, 0) : win32::RGB(30, 90, 190);
        psz = icon == win32::MB_ICONHAND ? "X" : icon == win32::MB_ICONEXCLAMATION ? "!" : icon == win32::MB_ICONQUESTION ? "?" : "i";
        hbr = win32::CreateSolidBrush(cr);
        if (hbr) {
            holdBrush = static_cast<HBRUSH>(win32::SelectObject(hdc, hbr)); win32::Ellipse(hdc, 16, 16, 48, 48);
            win32::SelectObject(hdc, holdBrush); win32::DeleteObject(hbr);
        }
        holdFont = win32::SelectObject(hdc, lp->hfontIcon);
        win32::SetBkMode(hdc, win32::TRANSPARENT); win32::SetTextColor(hdc, icon == win32::MB_ICONEXCLAMATION ? win32::RGB(0, 0, 0) : win32::RGB(255, 255, 255));
        rc = RECT{16, 16, 48, 48}; win32::DrawTextA(hdc, psz, -1, &rc, win32::DT_CENTER | win32::DT_VCENTER | win32::DT_SINGLELINE | win32::DT_NOPREFIX);
        if (holdFont) win32::SelectObject(hdc, holdFont);
    }
    win32::EndPaint(hwnd, &ps);
}
static INT_PTR  MessageDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATMSGBOX *lp = reinterpret_cast<COMPATMSGBOX *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));
    int i;
    if (msg == win32::WM_INITDIALOG) { win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, l); return 1; }
    if (!lp) return 0;
    if (msg == win32::WM_PAINT) { PaintMessageIcon(hwnd, lp); return 1; }
    if (msg == win32::WM_SETFOCUS) {
        win32::SetFocus(win32::IsWindow(lp->hwndLastFocus) ? lp->hwndLastFocus : win32::GetDlgItem(hwnd, lp->rgid[lp->iDefault]));
        return 1;
    }
    if (msg == win32::WM_MOUSEWHEEL) { ScrollMessage(lp, lp->yScroll - static_cast<SHORT>(HIWORD(w)) / win32::WHEEL_DELTA * 48); return 1; }
    if (msg == win32::WM_CLOSE) { if (lp->idCancel) win32::EndDialog(hwnd, lp->idCancel); return 1; }
    if (msg == win32::WM_COMMAND) {
        for (i = 0; i < lp->cButtons; i++)
            if (LOWORD(w) == lp->rgid[i]) { win32::EndDialog(hwnd, lp->rgid[i]); break; }
        return 1;
    }
    return 0;
}
static BOOL FMessageKey(HWND hwnd, COMPATMSGBOX *lp, const win32::MSG *lpmsg) {
    HWND hwndFocus = win32::GetFocus();
    LPCSTR psz;
    int i, id = 0, iFocus = -1;
    if (lpmsg->hwnd != hwnd && !win32::IsChild(hwnd, lpmsg->hwnd)) return 0;
    for (i = 0; i < lp->cButtons; i++) if (hwndFocus == win32::GetDlgItem(hwnd, lp->rgid[i])) iFocus = i;
    if (lpmsg->message == win32::WM_KEYDOWN) {
        if (lpmsg->wParam == win32::VK_ESCAPE) id = lp->idCancel;
        else if (lpmsg->wParam == win32::VK_RETURN) id = lp->rgid[iFocus >= 0 ? iFocus : lp->iDefault];
        else if ((lpmsg->wParam == win32::VK_LEFT || lpmsg->wParam == win32::VK_RIGHT) && iFocus >= 0) {
            i = (iFocus + (lpmsg->wParam == win32::VK_LEFT ? lp->cButtons - 1 : 1)) % lp->cButtons;
            win32::SetFocus(win32::GetDlgItem(hwnd, lp->rgid[i])); return 1;
        } else return 0;
        if (id) win32::EndDialog(hwnd, id);
        return 1;
    }
    if (lpmsg->message == win32::WM_SYSCHAR) {
        for (i = 0; i < lp->cButtons; i++) {
            psz = SDL_strchr(PszMessageButton(lp->rgid[i]), '&');
            if (psz && SDL_tolower(static_cast<int>(lpmsg->wParam)) == SDL_tolower(static_cast<unsigned char>(psz[1]))) { win32::EndDialog(hwnd, lp->rgid[i]); return 1; }
        }
    }
    return 0;
}
BOOL StarsCompatSetWindowAlertSounds(HWND hwnd, BOOL fEnable) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0; }
    lpwin->fSilentAlerts = !fEnable;
    return 1;
}

int win32::MessageBoxA(HWND hwndOwner, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    COMPATMSGBOX box = {};
    HWND hwnd = nullptr, hwndRoot = nullptr, hwndFocus = nullptr, lpwin, hwndButton;
    HWND *rghwnd = nullptr;
    size_t cDisabled = 0, iDisabled, cTop = 0;
    HDC hdc;
    HGDIOBJ hold;
    LOGFONTA lf = {};
    RECT rcWork, rc, rcText;
    win32::MSG msg;
    int i, result, value = 0, cx, cy, cxButtons, xText, x, y, cyMax;
    DWORD ex;
    BOOL fSound = 1, fRestoreFocus = 0;
    UINT allowed = 0x000f | 0x00f0 | 0x0300 | win32::MB_TASKMODAL | win32::MB_SYSTEMMODAL | win32::MB_SETFOREGROUND | win32::MB_TOPMOST | win32::MB_RIGHT;
    COMPATCLEANUPSCOPE restoreFocus([&] { if (fRestoreFocus && win32::IsWindow(hwndFocus) && win32::IsWindowEnabled(hwndFocus)) win32::SetFocus(hwndFocus); }, hwndOwner);
    std::unique_ptr<HWND[]> rghwndOwner;
    COMPATCLEANUPSCOPE restoreOwners([&] {
        for (iDisabled = 0; iDisabled < cDisabled; iDisabled++) if (win32::IsWindow(rghwnd[iDisabled])) win32::EnableWindow(rghwnd[iDisabled], 1);
    }, hwndOwner);
    COMPATCLEANUPSCOPE deleteIconFont([&] { if (box.hfontIcon) win32::DeleteObject(box.hfontIcon); });
    COMPATCLEANUPSCOPE destroyBox([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); }, hwndOwner);
    if (FStarsErrorPending()) return 0;
    if (!SDL_IsMainThread() || !FCompatVideo()) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    if ((type & ~allowed) || (type & 0xf) > 6 || (type & 0xf0) > win32::MB_ICONASTERISK ||
        (type & 0x3000) == 0x3000) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    if (hwndOwner && !win32::IsWindow(hwndOwner)) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0; }
    for (lpwin = LpCompatWindow(hwndOwner); lpwin; lpwin = LpCompatWindow(lpwin->hwndParent ? lpwin->hwndParent : lpwin->hwndOwner)) {
        if (lpwin->fSilentAlerts) { fSound = 0; break; }
    }
    if (!pszText) pszText = "";
    if (SDL_strnlen(pszText, 65537) > 65536) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    box.pszText = pszText; box.type = type; box.format = win32::DT_WORDBREAK | win32::DT_NOPREFIX | (type & win32::MB_RIGHT ? win32::DT_RIGHT : win32::DT_LEFT);
    memcpy(box.rgid, rgrid[type & 0xf], sizeof(box.rgid));
    for (i = 0; i < 3 && box.rgid[i] != 0; i++) { box.cButtons++; if (box.rgid[i] == win32::IDCANCEL) box.idCancel = win32::IDCANCEL; }
    if (box.cButtons == 1) box.idCancel = win32::IDOK;
    box.iDefault = (type >> 8) & 3;
    if (box.iDefault >= box.cButtons) box.iDefault = 0;
    hwndFocus = win32::GetFocus(); hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : nullptr; fRestoreFocus = 1;
    if (!hwndRoot && (type & win32::MB_TASKMODAL)) {
        for (lpwin = HwndCompatFirstTop(); lpwin; lpwin = win32::GetWindow(lpwin, win32::GW_HWNDNEXT)) cTop++;
    } else if (hwndRoot) cTop = 1;
    if (cTop) {
        rghwndOwner.reset(new (std::nothrow) HWND[cTop]{}); rghwnd = rghwndOwner.get();
        if (!rghwnd) { win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); return 0; }
        if (hwndRoot) { if (win32::IsWindowEnabled(hwndRoot)) rghwnd[cDisabled++] = hwndRoot; }
        else for (lpwin = HwndCompatFirstTop(); lpwin; lpwin = win32::GetWindow(lpwin, win32::GW_HWNDNEXT))
            if (win32::IsWindowEnabled(lpwin)) rghwnd[cDisabled++] = lpwin;
        for (iDisabled = 0; iDisabled < cDisabled; iDisabled++) {
            if (win32::IsWindow(rghwnd[iDisabled])) win32::EnableWindow(rghwnd[iDisabled], 0);
            if (FStarsErrorPending()) goto done;
        }
    }
    if (hwndRoot && !win32::IsWindow(hwndRoot)) { win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); goto done; }
    if (!win32::SystemParametersInfoA(win32::SPI_GETWORKAREA, 0, &rcWork, 0)) goto done;
    xText = type & 0xf0 ? 64 : 16;
    cxButtons = box.cButtons * 88 + (box.cButtons - 1) * 8;
    cx = (((StarsMax(320, cxButtons + 32)) < (StarsMax(160, rcWork.right - rcWork.left - 32))) ? (StarsMax(320, cxButtons + 32)) : (StarsMax(160, rcWork.right - rcWork.left - 32)));
    cx = (((StarsMax(cx, 480)) < (StarsMax(160, rcWork.right - rcWork.left - 32))) ? (StarsMax(cx, 480)) : (StarsMax(160, rcWork.right - rcWork.left - 32)));
    rc = RECT{rcWork.left, rcWork.top, rcWork.left + cx, rcWork.top + 160};
    ex = (type & (win32::MB_TOPMOST | win32::MB_SYSTEMMODAL)) ? win32::WS_EX_TOPMOST : 0;
    hwnd = HwndCompatDynamicDialog(pszCaption ? pszCaption : "Error", win32::WS_POPUP | win32::WS_CAPTION | win32::WS_SYSMENU, ex, &rc, hwndRoot, MessageDialogProc);
    if (!hwnd) goto done;
    box.hfont = hwnd->hfontDialog;
    hdc = win32::CreateCompatibleDC(nullptr); if (!hdc) goto done;
    hold = win32::SelectObject(hdc, box.hfont);
    box.cxText = StarsMax(32, cx - xText - 16);
    rcText = RECT{0, 0, box.cxText, 0};
    result = win32::DrawTextA(hdc, pszText, -1, &rcText, box.format | win32::DT_CALCRECT);
    cyMax = StarsMax(32, rcWork.bottom - rcWork.top - 128);
    if (rcText.bottom > cyMax) {
        box.cxText -= 20; rcText = RECT{0, 0, box.cxText, 0};
        result = win32::DrawTextA(hdc, pszText, -1, &rcText, box.format | win32::DT_CALCRECT);
    }
    win32::SelectObject(hdc, hold); win32::DeleteDC(hdc);
    if (result == 0 && *pszText) goto done;
    box.cyTotal = StarsMax(32, rcText.bottom);
    box.cyText = StarsMin(box.cyTotal, cyMax);
    cy = box.cyText + 80;
    x = rcWork.left + (rcWork.right - rcWork.left - cx) / 2;
    y = rcWork.top + (rcWork.bottom - rcWork.top - cy) / 2;
    if (hwndRoot && win32::GetWindowRect(hwndRoot, &rc)) {
        x = (((rcWork.right - cx) < ((((rcWork.left) > (rc.left + (rc.right - rc.left - cx) / 2)) ? (rcWork.left) : (rc.left + (rc.right - rc.left - cx) / 2)))) ? (rcWork.right - cx) : ((((rcWork.left) > (rc.left + (rc.right - rc.left - cx) / 2)) ? (rcWork.left) : (rc.left + (rc.right - rc.left - cx) / 2))));
        y = (((rcWork.bottom - cy) < ((((rcWork.top) > (rc.top + (rc.bottom - rc.top - cy) / 2)) ? (rcWork.top) : (rc.top + (rc.bottom - rc.top - cy) / 2)))) ? (rcWork.bottom - cy) : ((((rcWork.top) > (rc.top + (rc.bottom - rc.top - cy) / 2)) ? (rcWork.top) : (rc.top + (rc.bottom - rc.top - cy) / 2))));
    }
    if (!win32::MoveWindow(hwnd, x, y, cx, cy, 0)) goto done;
    box.hwndBody = win32::CreateWindowExA(0, "StarsMessageText", "", win32::WS_CHILD | win32::WS_VISIBLE | (box.cyText < box.cyTotal ? win32::WS_TABSTOP : 0),
                                  xText, 16, cx - xText - 16, box.cyText, hwnd, reinterpret_cast<HMENU>(0xffff), win32::GetModuleHandleA(nullptr), &box);
    if (!box.hwndBody) goto done;
    if (box.cyText < box.cyTotal) {
        box.hwndScroll = win32::CreateWindowExA(0, "ScrollBar", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::SBS_VERT,
                                        box.cxText + 3, 0, 17, box.cyText, box.hwndBody, reinterpret_cast<HMENU>(1), win32::GetModuleHandleA(nullptr), nullptr);
        if (!box.hwndScroll) goto done;
        win32::SetScrollRange(box.hwndScroll, win32::SB_CTL, 0, box.cyTotal - box.cyText, 1);
    }
    for (i = 0; i < box.cButtons; i++) {
        hwndButton = win32::CreateWindowExA(0, "Button", PszMessageButton(box.rgid[i]), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP |
                                     (i == box.iDefault ? win32::BS_DEFPUSHBUTTON : win32::BS_PUSHBUTTON),
                                     (cx - cxButtons) / 2 + i * 96, box.cyText + 32, 88, 26,
                                     hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(box.rgid[i])), win32::GetModuleHandleA(nullptr), nullptr);
        if (!hwndButton) goto done;
        win32::SendMessageA(hwndButton, win32::WM_SETFONT, reinterpret_cast<WPARAM>(box.hfont), 0);
    }
    if (type & 0xf0) { lf.lfHeight = -26; lf.lfWeight = win32::FW_BOLD; SDL_strlcpy(lf.lfFaceName, "sans-serif", sizeof(lf.lfFaceName)); box.hfontIcon = win32::CreateFontIndirectA(&lf); }
    win32::SendMessageA(hwnd, win32::WM_INITDIALOG, reinterpret_cast<WPARAM>(win32::GetDlgItem(hwnd, box.rgid[box.iDefault])), reinterpret_cast<LPARAM>(&box));
    box.hwndLastFocus = win32::GetDlgItem(hwnd, box.rgid[box.iDefault]);
    win32::ShowWindow(hwnd, win32::SW_SHOW);
    SDL_RaiseWindow(hwnd->lpWindow.get()); SDL_SyncWindow(hwnd->lpWindow.get());
    win32::SetFocus(win32::GetDlgItem(hwnd, box.rgid[box.iDefault]));
    if (fSound) win32::MessageBeep(type & 0xf0);
    while (win32::IsWindow(hwnd) && !hwnd->fEndDialog) {
        if (win32::IsChild(hwnd, win32::GetFocus())) box.hwndLastFocus = win32::GetFocus();
        result = win32::GetMessageA(&msg, nullptr, 0, 0);
        if (result <= 0) { if (result == 0) win32::PostQuitMessage(static_cast<int>(msg.wParam)); break; }
        // SDL root activation may be routed before its queued WM_SETFOCUS.
        if (win32::GetFocus() == hwnd && win32::IsWindow(box.hwndLastFocus)) win32::SetFocus(box.hwndLastFocus);
        if (!FMessageKey(hwnd, &box, &msg) && !win32::IsDialogMessageA(hwnd, &msg)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
    }
    if (win32::IsWindow(hwnd) && hwnd->fEndDialog) value = static_cast<int>(hwnd->lDlgResult);
    if (value == 0) win32::SetLastError(win32::ERROR_CANCELLED);
done:
    destroyBox.Close();
    deleteIconFont.Close();
    restoreOwners.Close();
    rghwndOwner.reset(); rghwnd = nullptr;
    restoreFocus.Close();
    return value;
}
