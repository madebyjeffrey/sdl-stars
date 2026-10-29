/* Source-level message boxes use the same controls and modal queue as dialogs. */
#include "internal.h"
#include <stars_compat.h>
#include <stdlib.h>
#include <string.h>

typedef struct COMPATMSGBOX {
    LPCSTR pszText;
    UINT type, format;
    int rgid[3], cButtons, iDefault, idCancel;
    int cxText, cyText, cyTotal, yScroll;
    HWND hwndBody, hwndScroll, hwndLastFocus;
    HFONT hfont, hfontIcon;
} COMPATMSGBOX;
static const int rgrid[7][3] = {
    {IDOK, 0, 0}, {IDOK, IDCANCEL, 0}, {IDABORT, IDRETRY, IDIGNORE},
    {IDYES, IDNO, IDCANCEL}, {IDYES, IDNO, 0}, {IDRETRY, IDCANCEL, 0},
    {IDCANCEL, IDTRYAGAIN, IDCONTINUE}
};
static LPCSTR PszMessageButton(int id) {
    switch (id) {
    case IDOK: return "&OK";
    case IDCANCEL: return "Cancel";
    case IDABORT: return "&Abort";
    case IDRETRY: return "&Retry";
    case IDIGNORE: return "&Ignore";
    case IDYES: return "&Yes";
    case IDNO: return "&No";
    case IDTRYAGAIN: return "&Try Again";
    default: return "&Continue";
    }
}
static void ScrollMessage(COMPATMSGBOX *lp, int y) {
    lp->yScroll = (((StarsMax(0, lp->cyTotal - lp->cyText)) < (StarsMax(0, y))) ? (StarsMax(0, lp->cyTotal - lp->cyText)) : (StarsMax(0, y)));
    if (IsWindow(lp->hwndScroll)) SetScrollPos(lp->hwndScroll, SB_CTL, lp->yScroll, TRUE);
    InvalidateRect(lp->hwndBody, NULL, TRUE);
}
LRESULT CALLBACK CompatMessageTextProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATMSGBOX *lp = (COMPATMSGBOX *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    HDC hdc;
    PAINTSTRUCT ps;
    HGDIOBJ hold;
    HBRUSH hbr;
    HRGN hrgn;
    RECT rc;
    int y;
    if (msg == WM_NCCREATE) {
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTA *)l)->lpCreateParams); return TRUE;
    }
    if (!lp) return DefWindowProcA(hwnd, msg, w, l);
    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps); if (!hdc) return 0;
        rc = (RECT){0, 0, lp->cxText, lp->cyText};
        hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        if (hbr) { FillRect(hdc, &rc, hbr); DeleteObject(hbr); }
        hrgn = CreateRectRgn(0, 0, lp->cxText, lp->cyText);
        if (hrgn) { SelectClipRgn(hdc, hrgn); DeleteObject(hrgn); }
        hold = SelectObject(hdc, lp->hfont); SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
        rc.top = -lp->yScroll; rc.bottom = lp->cyTotal - lp->yScroll;
        DrawTextA(hdc, lp->pszText, -1, &rc, lp->format);
        if (hold) SelectObject(hdc, hold);
        EndPaint(hwnd, &ps); return 0;
    case WM_GETDLGCODE: return DLGC_WANTARROWS;
    case WM_MOUSEWHEEL: ScrollMessage(lp, lp->yScroll - (SHORT)HIWORD(w) / WHEEL_DELTA * 48); return 0;
    case WM_KEYDOWN:
        y = lp->yScroll;
        if (w == VK_UP) y -= 16;
        else if (w == VK_DOWN) y += 16;
        else if (w == VK_PRIOR) y -= lp->cyText;
        else if (w == VK_NEXT) y += lp->cyText;
        else if (w == VK_HOME) y = 0;
        else if (w == VK_END) y = lp->cyTotal;
        else return 0;
        ScrollMessage(lp, y); return 0;
    case WM_VSCROLL:
        y = lp->yScroll;
        switch (LOWORD(w)) {
        case SB_LINEUP: y -= 16; break;
        case SB_LINEDOWN: y += 16; break;
        case SB_PAGEUP: y -= lp->cyText; break;
        case SB_PAGEDOWN: y += lp->cyText; break;
        case SB_TOP: y = 0; break;
        case SB_BOTTOM: y = lp->cyTotal; break;
        case SB_THUMBPOSITION: case SB_THUMBTRACK:
            y = IsWindow(lp->hwndScroll) ? lp->hwndScroll->iScrollTrack : HIWORD(w); break;
        default: return 0;
        }
        ScrollMessage(lp, y); return 0;
    default: return DefWindowProcA(hwnd, msg, w, l);
    }
}
static void PaintMessageIcon(HWND hwnd, COMPATMSGBOX *lp) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH hbr, holdBrush;
    HGDIOBJ holdFont;
    RECT rc;
    COLORREF cr;
    LPCSTR psz;
    UINT icon = lp->type & 0xf0;
    if (!hdc) return;
    GetClientRect(hwnd, &rc); hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    if (hbr) { FillRect(hdc, &rc, hbr); DeleteObject(hbr); }
    if (icon) {
        cr = icon == MB_ICONHAND ? RGB(200, 30, 30) : icon == MB_ICONEXCLAMATION ? RGB(255, 210, 0) : RGB(30, 90, 190);
        psz = icon == MB_ICONHAND ? "X" : icon == MB_ICONEXCLAMATION ? "!" : icon == MB_ICONQUESTION ? "?" : "i";
        hbr = CreateSolidBrush(cr);
        if (hbr) {
            holdBrush = (HBRUSH)SelectObject(hdc, hbr); Ellipse(hdc, 16, 16, 48, 48);
            SelectObject(hdc, holdBrush); DeleteObject(hbr);
        }
        holdFont = SelectObject(hdc, lp->hfontIcon);
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, icon == MB_ICONEXCLAMATION ? RGB(0, 0, 0) : RGB(255, 255, 255));
        rc = (RECT){16, 16, 48, 48}; DrawTextA(hdc, psz, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (holdFont) SelectObject(hdc, holdFont);
    }
    EndPaint(hwnd, &ps);
}
static INT_PTR CALLBACK MessageDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATMSGBOX *lp = (COMPATMSGBOX *)GetWindowLongPtrA(hwnd, DWLP_USER);
    int i;
    if (msg == WM_INITDIALOG) { SetWindowLongPtrA(hwnd, DWLP_USER, l); return TRUE; }
    if (!lp) return FALSE;
    if (msg == WM_PAINT) { PaintMessageIcon(hwnd, lp); return TRUE; }
    if (msg == WM_SETFOCUS) {
        SetFocus(IsWindow(lp->hwndLastFocus) ? lp->hwndLastFocus : GetDlgItem(hwnd, lp->rgid[lp->iDefault]));
        return TRUE;
    }
    if (msg == WM_MOUSEWHEEL) { ScrollMessage(lp, lp->yScroll - (SHORT)HIWORD(w) / WHEEL_DELTA * 48); return TRUE; }
    if (msg == WM_CLOSE) { if (lp->idCancel) EndDialog(hwnd, lp->idCancel); return TRUE; }
    if (msg == WM_COMMAND) {
        for (i = 0; i < lp->cButtons; i++)
            if (LOWORD(w) == lp->rgid[i]) { EndDialog(hwnd, lp->rgid[i]); break; }
        return TRUE;
    }
    return FALSE;
}
static BOOL FMessageKey(HWND hwnd, COMPATMSGBOX *lp, const MSG *lpmsg) {
    HWND hwndFocus = GetFocus();
    LPCSTR psz;
    int i, id = 0, iFocus = -1;
    if (lpmsg->hwnd != hwnd && !IsChild(hwnd, lpmsg->hwnd)) return FALSE;
    for (i = 0; i < lp->cButtons; i++) if (hwndFocus == GetDlgItem(hwnd, lp->rgid[i])) iFocus = i;
    if (lpmsg->message == WM_KEYDOWN) {
        if (lpmsg->wParam == VK_ESCAPE) id = lp->idCancel;
        else if (lpmsg->wParam == VK_RETURN) id = lp->rgid[iFocus >= 0 ? iFocus : lp->iDefault];
        else if ((lpmsg->wParam == VK_LEFT || lpmsg->wParam == VK_RIGHT) && iFocus >= 0) {
            i = (iFocus + (lpmsg->wParam == VK_LEFT ? lp->cButtons - 1 : 1)) % lp->cButtons;
            SetFocus(GetDlgItem(hwnd, lp->rgid[i])); return TRUE;
        } else return FALSE;
        if (id) EndDialog(hwnd, id);
        return TRUE;
    }
    if (lpmsg->message == WM_SYSCHAR) {
        for (i = 0; i < lp->cButtons; i++) {
            psz = SDL_strchr(PszMessageButton(lp->rgid[i]), '&');
            if (psz && SDL_tolower((int)lpmsg->wParam) == SDL_tolower((unsigned char)psz[1])) { EndDialog(hwnd, lp->rgid[i]); return TRUE; }
        }
    }
    return FALSE;
}
BOOL StarsCompatSetWindowAlertSounds(HWND hwnd, BOOL fEnable) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin) { SetLastError(ERROR_INVALID_WINDOW_HANDLE); return FALSE; }
    lpwin->fSilentAlerts = !fEnable;
    return TRUE;
}

int WINAPI STARS_TEST_IMPL(MessageBoxA)(HWND hwndOwner, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    COMPATMSGBOX box = {};
    HWND hwnd = NULL, hwndRoot, hwndFocus, lpwin, hwndButton;
    HWND *rghwnd = NULL;
    size_t cDisabled = 0, iDisabled, cTop = 0;
    HDC hdc;
    HGDIOBJ hold;
    LOGFONTA lf = {};
    RECT rcWork, rc, rcText;
    MSG msg;
    int i, result, value = 0, cx, cy, cxButtons, xText, x, y, cyMax;
    DWORD ex;
    BOOL fSound = TRUE;
    UINT allowed = 0x000f | 0x00f0 | 0x0300 | MB_TASKMODAL | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_RIGHT;
    if (!SDL_IsMainThread() || !FCompatVideo()) { SetLastError(ERROR_NOT_SUPPORTED); return 0; }
    if ((type & ~allowed) || (type & 0xf) > 6 || (type & 0xf0) > MB_ICONASTERISK ||
        (type & 0x3000) == 0x3000) { SetLastError(ERROR_NOT_SUPPORTED); return 0; }
    if (hwndOwner && !IsWindow(hwndOwner)) { SetLastError(ERROR_INVALID_WINDOW_HANDLE); return 0; }
    for (lpwin = LpCompatWindow(hwndOwner); lpwin; lpwin = LpCompatWindow(lpwin->hwndParent ? lpwin->hwndParent : lpwin->hwndOwner)) {
        if (lpwin->fSilentAlerts) { fSound = FALSE; break; }
    }
    if (!pszText) pszText = "";
    if (SDL_strnlen(pszText, 65537) > 65536) { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
    box.pszText = pszText; box.type = type; box.format = DT_WORDBREAK | DT_NOPREFIX | (type & MB_RIGHT ? DT_RIGHT : DT_LEFT);
    memcpy(box.rgid, rgrid[type & 0xf], sizeof(box.rgid));
    for (i = 0; i < 3 && box.rgid[i] != 0; i++) { box.cButtons++; if (box.rgid[i] == IDCANCEL) box.idCancel = IDCANCEL; }
    if (box.cButtons == 1) box.idCancel = IDOK;
    box.iDefault = (type >> 8) & 3;
    if (box.iDefault >= box.cButtons) box.iDefault = 0;
    hwndFocus = GetFocus(); hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : NULL;
    if (!hwndRoot && (type & MB_TASKMODAL)) {
        for (lpwin = HwndCompatFirstTop(); lpwin; lpwin = GetWindow(lpwin, GW_HWNDNEXT)) cTop++;
    } else if (hwndRoot) cTop = 1;
    if (cTop) {
        rghwnd = (HWND *)(SDL_calloc(cTop, sizeof(*rghwnd)));
        if (!rghwnd) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return 0; }
        if (hwndRoot) { if (IsWindowEnabled(hwndRoot)) rghwnd[cDisabled++] = hwndRoot; }
        else for (lpwin = HwndCompatFirstTop(); lpwin; lpwin = GetWindow(lpwin, GW_HWNDNEXT))
            if (IsWindowEnabled(lpwin)) rghwnd[cDisabled++] = lpwin;
        for (iDisabled = 0; iDisabled < cDisabled; iDisabled++) if (IsWindow(rghwnd[iDisabled])) EnableWindow(rghwnd[iDisabled], FALSE);
    }
    if (hwndRoot && !IsWindow(hwndRoot)) { SetLastError(ERROR_INVALID_WINDOW_HANDLE); goto done; }
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWork, 0)) goto done;
    xText = type & 0xf0 ? 64 : 16;
    cxButtons = box.cButtons * 88 + (box.cButtons - 1) * 8;
    cx = (((StarsMax(320, cxButtons + 32)) < (StarsMax(160, rcWork.right - rcWork.left - 32))) ? (StarsMax(320, cxButtons + 32)) : (StarsMax(160, rcWork.right - rcWork.left - 32)));
    cx = (((StarsMax(cx, 480)) < (StarsMax(160, rcWork.right - rcWork.left - 32))) ? (StarsMax(cx, 480)) : (StarsMax(160, rcWork.right - rcWork.left - 32)));
    rc = (RECT){rcWork.left, rcWork.top, rcWork.left + cx, rcWork.top + 160};
    ex = (type & (MB_TOPMOST | MB_SYSTEMMODAL)) ? WS_EX_TOPMOST : 0;
    hwnd = HwndCompatDynamicDialog(pszCaption ? pszCaption : "Error", WS_POPUP | WS_CAPTION | WS_SYSMENU, ex, &rc, hwndRoot, MessageDialogProc);
    if (!hwnd) goto done;
    box.hfont = hwnd->hfontDialog;
    hdc = CreateCompatibleDC(NULL); if (!hdc) goto done;
    hold = SelectObject(hdc, box.hfont);
    box.cxText = StarsMax(32, cx - xText - 16);
    rcText = (RECT){0, 0, box.cxText, 0};
    result = DrawTextA(hdc, pszText, -1, &rcText, box.format | DT_CALCRECT);
    cyMax = StarsMax(32, rcWork.bottom - rcWork.top - 128);
    if (rcText.bottom > cyMax) {
        box.cxText -= 20; rcText = (RECT){0, 0, box.cxText, 0};
        result = DrawTextA(hdc, pszText, -1, &rcText, box.format | DT_CALCRECT);
    }
    SelectObject(hdc, hold); DeleteDC(hdc);
    if (result == 0 && *pszText) goto done;
    box.cyTotal = StarsMax(32, rcText.bottom);
    box.cyText = StarsMin(box.cyTotal, cyMax);
    cy = box.cyText + 80;
    x = rcWork.left + (rcWork.right - rcWork.left - cx) / 2;
    y = rcWork.top + (rcWork.bottom - rcWork.top - cy) / 2;
    if (hwndRoot && GetWindowRect(hwndRoot, &rc)) {
        x = (((rcWork.right - cx) < ((((rcWork.left) > (rc.left + (rc.right - rc.left - cx) / 2)) ? (rcWork.left) : (rc.left + (rc.right - rc.left - cx) / 2)))) ? (rcWork.right - cx) : ((((rcWork.left) > (rc.left + (rc.right - rc.left - cx) / 2)) ? (rcWork.left) : (rc.left + (rc.right - rc.left - cx) / 2))));
        y = (((rcWork.bottom - cy) < ((((rcWork.top) > (rc.top + (rc.bottom - rc.top - cy) / 2)) ? (rcWork.top) : (rc.top + (rc.bottom - rc.top - cy) / 2)))) ? (rcWork.bottom - cy) : ((((rcWork.top) > (rc.top + (rc.bottom - rc.top - cy) / 2)) ? (rcWork.top) : (rc.top + (rc.bottom - rc.top - cy) / 2))));
    }
    if (!MoveWindow(hwnd, x, y, cx, cy, FALSE)) goto done;
    box.hwndBody = CreateWindowExA(0, "StarsMessageText", "", WS_CHILD | WS_VISIBLE | (box.cyText < box.cyTotal ? WS_TABSTOP : 0),
                                  xText, 16, cx - xText - 16, box.cyText, hwnd, (HMENU)0xffff, GetModuleHandleA(NULL), &box);
    if (!box.hwndBody) goto done;
    if (box.cyText < box.cyTotal) {
        box.hwndScroll = CreateWindowExA(0, "ScrollBar", "", WS_CHILD | WS_VISIBLE | SBS_VERT,
                                        box.cxText + 3, 0, 17, box.cyText, box.hwndBody, (HMENU)1, GetModuleHandleA(NULL), NULL);
        if (!box.hwndScroll) goto done;
        SetScrollRange(box.hwndScroll, SB_CTL, 0, box.cyTotal - box.cyText, TRUE);
    }
    for (i = 0; i < box.cButtons; i++) {
        hwndButton = CreateWindowExA(0, "Button", PszMessageButton(box.rgid[i]), WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                     (i == box.iDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                                     (cx - cxButtons) / 2 + i * 96, box.cyText + 32, 88, 26,
                                     hwnd, (HMENU)(INT_PTR)box.rgid[i], GetModuleHandleA(NULL), NULL);
        if (!hwndButton) goto done;
        SendMessageA(hwndButton, WM_SETFONT, (WPARAM)box.hfont, 0);
    }
    if (type & 0xf0) { lf.lfHeight = -26; lf.lfWeight = FW_BOLD; SDL_strlcpy(lf.lfFaceName, "sans-serif", sizeof(lf.lfFaceName)); box.hfontIcon = CreateFontIndirectA(&lf); }
    SendMessageA(hwnd, WM_INITDIALOG, (WPARAM)GetDlgItem(hwnd, box.rgid[box.iDefault]), (LPARAM)&box);
    box.hwndLastFocus = GetDlgItem(hwnd, box.rgid[box.iDefault]);
    ShowWindow(hwnd, SW_SHOW);
    SDL_RaiseWindow(hwnd->lpWindow); SDL_SyncWindow(hwnd->lpWindow);
    SetFocus(GetDlgItem(hwnd, box.rgid[box.iDefault]));
    if (fSound) MessageBeep(type & 0xf0);
    while (IsWindow(hwnd) && !hwnd->fEndDialog) {
        if (IsChild(hwnd, GetFocus())) box.hwndLastFocus = GetFocus();
        result = GetMessageA(&msg, NULL, 0, 0);
        if (result <= 0) { if (result == 0) PostQuitMessage((int)msg.wParam); break; }
        // SDL root activation may be routed before its queued WM_SETFOCUS.
        if (GetFocus() == hwnd && IsWindow(box.hwndLastFocus)) SetFocus(box.hwndLastFocus);
        if (!FMessageKey(hwnd, &box, &msg) && !IsDialogMessageA(hwnd, &msg)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    }
    if (IsWindow(hwnd) && hwnd->fEndDialog) value = (int)hwnd->lDlgResult;
    if (value == 0) SetLastError(ERROR_CANCELLED);
done:
    if (IsWindow(hwnd)) DestroyWindow(hwnd);
    if (box.hfontIcon) DeleteObject(box.hfontIcon);
    for (iDisabled = 0; iDisabled < cDisabled; iDisabled++) if (IsWindow(rghwnd[iDisabled])) EnableWindow(rghwnd[iDisabled], TRUE);
    SDL_free(rghwnd);
    if (IsWindow(hwndFocus) && IsWindowEnabled(hwndFocus)) SetFocus(hwndFocus);
    return value;
}
