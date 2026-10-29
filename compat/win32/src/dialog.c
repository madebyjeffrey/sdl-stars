/* Resource-backed dialogs share the existing window, control and message paths. */
#include "cleanup.h"
#include <stars_compat.h>
#include <string.h>

typedef struct COMPATDLGCREATE { DLGPROC lpfn; HFONT hfont; int cxUnit, cyUnit; BOOL fModal; } COMPATDLGCREATE;

void CompatDestroyDialog(HWND hwnd) {
    if (hwnd->hfontDialog) DeleteObject(hwnd->hfontDialog);
    hwnd->hfontDialog = NULL;
}
static HWND HwndDialogTab(HWND hwnd, HWND hwndCurrent, BOOL fBack) {
    HWND lp, hwndFirst = NULL, hwndLast = NULL, hwndPrevious = NULL;
    BOOL fFound;
    while (hwndCurrent && IsChild(hwnd, hwndCurrent) && GetParent(hwndCurrent) != hwnd)
        hwndCurrent = GetParent(hwndCurrent);
    fFound = hwndCurrent == NULL;
    for (lp = GetWindow(hwnd, GW_CHILD); lp; lp = GetWindow(lp, GW_HWNDNEXT)) {
        if (!lp->fVisible || !IsWindowEnabled(lp) || !(lp->dwStyle & WS_TABSTOP)) continue;
        if (!hwndFirst) hwndFirst = lp;
        hwndLast = lp;
        if (lp == hwndCurrent) {
            if (fBack && hwndPrevious) return hwndPrevious;
            fFound = TRUE;
        } else if (fFound && !fBack) return lp;
        hwndPrevious = lp;
    }
    return fBack ? hwndLast : hwndFirst;
}
LRESULT CALLBACK CompatDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    COMPATDLGCREATE *lpc;
    INT_PTR result;
    HBRUSH hbr;
    if (!lp) return 0;
    if (msg == WM_NCCREATE) {
        lpc = (COMPATDLGCREATE *)(((const CREATESTRUCTA *)l)->lpCreateParams);
        if (!lpc) return FALSE;
        lp->fDialog = TRUE; lp->lpfnDlg = lpc->lpfn; lp->hfontDialog = lpc->hfont;
        lpc->hfont = NULL;
        lp->cxDlgUnit = lpc->cxUnit; lp->cyDlgUnit = lpc->cyUnit; lp->fModal = lpc->fModal;
        return TRUE;
    }
    if (msg == WM_CREATE) return 0;
    lp->lDlgMsgResult = 0;
    result = lp->lpfnDlg ? CompatInvokeWindowProc(lp->lpfnDlg, hwnd, msg, w, l) : 0;
    // EndDialog defers destruction; default handling must not replace its result.
    if (!IsWindow(hwnd) || lp->fEndDialog) return result;
    if (msg == WM_INITDIALOG) return result;
    if (result) {
        if (msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) return result;
        return lp->lDlgMsgResult;
    }
    switch (msg) {
    case WM_GETFONT: return (LRESULT)lp->hfontDialog;
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc); hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        if (!hbr) return FALSE;
        FillRect((HDC)w, &rc, hbr); DeleteObject(hbr); return TRUE;
    }
    case WM_CLOSE:
        SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM)GetDlgItem(hwnd, IDCANCEL)); return 0;
    case WM_COMMAND:
        // The application decides whether OK/Cancel ends a modal or just an edit.
        return 0;
    default: return DefWindowProcA(hwnd, msg, w, l);
    }
}
BOOL WINAPI MapDialogRect(HWND hwnd, LPRECT lprc) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lprc) return FALSE;
    lprc->left = MulDiv(lprc->left, lp->cxDlgUnit, 4); lprc->right = MulDiv(lprc->right, lp->cxDlgUnit, 4);
    lprc->top = MulDiv(lprc->top, lp->cyDlgUnit, 8); lprc->bottom = MulDiv(lprc->bottom, lp->cyDlgUnit, 8);
    return TRUE;
}
static BOOL FDialogUnits(COMPATDLGTEMPLATE *lpdlg, COMPATDLGCREATE *lpc) {
    LOGFONTA lf = {};
    HDC hdc;
    HGDIOBJ hold;
    SIZE size;
    TEXTMETRICA tm;
    BOOL fOk;
    lf.lfHeight = lpdlg->cPoints ? -MulDiv(lpdlg->cPoints, 96, 72) : -11;
    lf.lfWeight = FW_NORMAL;
    CchCopyUtf8(lf.lfFaceName, sizeof(lf.lfFaceName), lpdlg->pszFont ? lpdlg->pszFont : "MS Sans Serif",
                CchUtf8Prefix(lpdlg->pszFont ? lpdlg->pszFont : "MS Sans Serif", sizeof(lf.lfFaceName) - 1));
    lpc->hfont = CreateFontIndirectA(&lf);
    if (!lpc->hfont || !(hdc = CreateCompatibleDC(NULL))) return FALSE;
    hold = SelectObject(hdc, lpc->hfont);
    fOk = GetTextExtentPoint32A(hdc, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size) && GetTextMetricsA(hdc, &tm);
    if (fOk) { lpc->cxUnit = (size.cx / 26 + 1) / 2; lpc->cyUnit = tm.tmHeight; fOk = lpc->cxUnit > 0 && lpc->cyUnit > 0; }
    SelectObject(hdc, hold); DeleteDC(hdc); return fOk;
}
HWND HwndCompatDynamicDialog(LPCSTR pszTitle, DWORD dwStyle, DWORD dwExStyle, const RECT *lprc,
                             HWND hwndOwner, DLGPROC lpfn) {
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGCREATE dc = {};
    HWND hwnd;
    COMPATCLEANUPSCOPE deleteFont([&] { if (dc.hfont) DeleteObject(dc.hfont); });
    dlg.cPoints = 8;
    dc.lpfn = lpfn; dc.fModal = TRUE;
    if (!FDialogUnits(&dlg, &dc)) return NULL;
    hwnd = CreateWindowExA(dwExStyle, "#32770", pszTitle, dwStyle & ~WS_VISIBLE,
                         lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
                         hwndOwner, NULL, GetModuleHandleA(NULL), &dc);
    deleteFont.Close();
    return hwnd;
}
static LPCSTR PszDialogClass(COMPATDLGITEM *lpitem) {
    static const char *rgpsz[] = {"Button", "Edit", "Static", "ListBox", "ScrollBar", "ComboBox"};
    if (lpitem->pszClass && *lpitem->pszClass) return lpitem->pszClass;
    return lpitem->idClass >= 0x80 && lpitem->idClass <= 0x85 ? rgpsz[lpitem->idClass - 0x80] : NULL;
}
static HWND HwndCreateDialog(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param, BOOL fModal) {
    HRSRC hrsrc;
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGCREATE dc = {};
    COMPATDLGITEM *lpitem;
    HWND hwnd = NULL, hwndChild, hwndFocus;
    RECT rc;
    WORD i;
    LPCSTR pszClass, pszTitle;
    BOOL fFocus;
    COMPATCLEANUPSCOPE freeTemplate([&] { CompatFreeDialogTemplate(&dlg); });
    COMPATCLEANUPSCOPE deleteFont([&] { if (dc.hfont) DeleteObject(dc.hfont); });
    COMPATCLEANUPSCOPE destroyDialog([&] { if (IsWindow(hwnd)) DestroyWindow(hwnd); }, hwndOwner);
    // A checked caller cannot take ownership of a modeless window on failure.
    if (!fModal && FStarsErrorPending()) return NULL;
    if (!SDL_IsMainThread() || (hwndOwner && !IsWindow(hwndOwner))) return NULL;
    if (!hinst) hinst = GetModuleHandleA(NULL);
    hrsrc = FindResourceA(hinst, psz, RT_DIALOG);
    if (!FCompatDialogTemplate((const BYTE *)(LockResource(LoadResource(hinst, hrsrc))), SizeofResource(hinst, hrsrc), &dlg)) return NULL;
    if (dlg.idClass || (dlg.pszClass && *dlg.pszClass) || dlg.idMenu || (dlg.pszMenu && *dlg.pszMenu)) goto done;
    dc.lpfn = lpfn; dc.fModal = fModal;
    if (!FDialogUnits(&dlg, &dc)) goto done;
    hwnd = CreateWindowExA(dlg.dwExStyle, "#32770", dlg.pszTitle, dlg.dwStyle & ~WS_VISIBLE,
                         MulDiv(dlg.x, dc.cxUnit, 4), MulDiv(dlg.y, dc.cyUnit, 8),
                         MulDiv(dlg.cx, dc.cxUnit, 4), MulDiv(dlg.cy, dc.cyUnit, 8), hwndOwner, NULL, hinst, &dc);
    // The dialog record owns the font after WM_NCCREATE, including failure cleanup.
    if (!hwnd) goto done;
    dc.hfont = NULL;
    if (!fModal && FStarsErrorPending()) goto fail;
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hwnd->hfontDialog, 0);
    if (!IsWindow(hwnd)) { hwnd = NULL; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    for (i = 0; i < dlg.cItems; i++) {
        lpitem = &dlg.rgItems[i]; pszClass = PszDialogClass(lpitem);
        pszTitle = lpitem->pszTitle ? lpitem->pszTitle : MAKEINTRESOURCE(lpitem->idTitle);
        if (!pszClass || lpitem->cbCreate || (UINT_PTR)pszTitle <= UINT16_MAX) goto fail;
        rc = (RECT){lpitem->x, lpitem->y, lpitem->cx, lpitem->cy};
        MapDialogRect(hwnd, &rc);
        hwndChild = CreateWindowExA(lpitem->dwExStyle, pszClass, pszTitle, lpitem->dwStyle,
                                   rc.left, rc.top, rc.right, rc.bottom,
                                   hwnd, (HMENU)(INT_PTR)(int16_t)lpitem->id, hinst, NULL);
        if (!hwndChild || !IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
        SendMessageA(hwndChild, WM_SETFONT, (WPARAM)hwnd->hfontDialog, 0);
        if (!IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
    }
    hwndFocus = HwndDialogTab(hwnd, NULL, FALSE);
    fFocus = SendMessageA(hwnd, WM_INITDIALOG, (WPARAM)hwndFocus, param) != 0;
    if (!IsWindow(hwnd)) { hwnd = NULL; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    if (!hwnd->fEndDialog) {
        if (fModal || (dlg.dwStyle & WS_VISIBLE)) {
            // Prepare the initialized dialog before exposing its native window;
            // otherwise the compositor first displays the empty black canvas.
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            if (!IsWindow(hwnd)) { hwnd = NULL; goto done; }
            if (!fModal && FStarsErrorPending()) goto fail;
            for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT)) {
                UpdateWindow(hwndChild);
                if (!IsWindow(hwnd)) { hwnd = NULL; goto done; }
                if (!fModal && FStarsErrorPending()) goto fail;
            }
            hwnd->fDeferPresent = FALSE;
            if (!FCompatPresent(hwnd)) goto fail;
            if (!hwnd->fEndDialog) {
                ShowWindow(hwnd, SW_SHOW);
            }
        }
        if (fFocus && IsWindow(hwndFocus) && IsWindowEnabled(hwndFocus)) SetFocus(hwndFocus);
    }
    if (!IsWindow(hwnd)) hwnd = NULL;
    if (!fModal && FStarsErrorPending()) goto fail;
    goto done;
fail:
    destroyDialog.Close();
    hwnd = NULL;
done:
    deleteFont.Close();
    freeTemplate.Close();
    destroyDialog.Release();
    return hwnd;
}
HWND WINAPI CreateDialogParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    return HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, FALSE);
}
BOOL WINAPI EndDialog(HWND hwnd, INT_PTR result) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lp->fModal) return FALSE;
    lp->lDlgResult = result; lp->fEndDialog = TRUE;
    return TRUE;
}
BOOL WINAPI IsDialogMessageA(HWND hwnd, LPMSG lpmsg) {
    HWND lp = LpCompatWindow(hwnd), hwndFocus, hwndNext;
    LRESULT code;
    UINT id;
    if (!lp || !lp->fDialog || !lpmsg || (lpmsg->hwnd != hwnd && !IsChild(hwnd, lpmsg->hwnd))) return FALSE;
    if (lpmsg->message != WM_KEYDOWN && lpmsg->message != WM_SYSKEYDOWN) return FALSE;
    code = SendMessageA(lpmsg->hwnd, WM_GETDLGCODE, lpmsg->wParam, (LPARAM)lpmsg);
    if (!IsWindow(hwnd)) return TRUE;
    if (code & DLGC_WANTALLKEYS) return FALSE;
    hwndFocus = GetFocus();
    if (lpmsg->wParam == VK_TAB && !(code & DLGC_WANTTAB)) {
        hwndNext = HwndDialogTab(hwnd, hwndFocus, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
        if (hwndNext) SetFocus(hwndNext);
        return TRUE;
    }
    if (lpmsg->wParam == VK_RETURN || lpmsg->wParam == VK_ESCAPE) {
        id = lpmsg->wParam == VK_ESCAPE ? IDCANCEL : IDOK;
        if (lpmsg->wParam == VK_RETURN) {
            for (hwndNext = GetWindow(hwnd, GW_CHILD); hwndNext; hwndNext = GetWindow(hwndNext, GW_HWNDNEXT))
                if (hwndNext->iControl == 1 && (hwndNext->dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON) { id = (UINT)GetDlgCtrlID(hwndNext); break; }
        }
        hwndNext = GetDlgItem(hwnd, id);
        if (!hwndNext || IsWindowEnabled(hwndNext)) SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)hwndNext);
        return TRUE;
    }
    return FALSE;
}
INT_PTR WINAPI DialogBoxParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    HWND hwnd = NULL, hwndFocus = GetFocus(), hwndRoot;
    BOOL fEnable;
    MSG msg;
    int result;
    INT_PTR value = -1;
    if (!SDL_IsMainThread()) return -1;
    // Nested wizard loops must unwind without exposing another page after application quit.
    if (PeekMessageA(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE)) return -1;
    if (hwndOwner && !IsWindow(hwndOwner)) return 0;
    hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : NULL;
    fEnable = hwndRoot && IsWindowEnabled(hwndRoot);
    COMPATCLEANUPSCOPE restoreOwner([&] {
        if (FCompatPendingDialogSequence(hwndRoot)) return;
        if (fEnable && IsWindow(hwndRoot)) EnableWindow(hwndRoot, TRUE);
        CompatRestoreOwnerFocus(hwndRoot, hwndFocus);
    }, hwndRoot);
    COMPATCLEANUPSCOPE destroyDialog([&] { if (IsWindow(hwnd)) DestroyWindow(hwnd); }, hwndRoot);
    if (fEnable) EnableWindow(hwndRoot, FALSE);
    hwnd = HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, TRUE);
    if (hwnd) {
        while (IsWindow(hwnd) && !hwnd->fEndDialog) {
            result = GetMessageA(&msg, NULL, 0, 0);
            if (result <= 0) { if (result == 0) PostQuitMessage((int)msg.wParam); break; }
            if (!IsDialogMessageA(hwnd, &msg)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
        }
        value = hwnd->fEndDialog ? hwnd->lDlgResult : -1;
        if (IsWindow(hwnd)) {
            // A wizard's Back/Next results retain only native presentation;
            // callbacks, child HWNDs and page-local state still get destroyed.
            hwnd->fKeepNative = hwnd->fEndDialog && (value == 1 || value == 2);
        }
    }
    destroyDialog.Close();
    restoreOwner.Close();
    return value;
}

// Abort the enclosing modal operation, completing its cleanup before the
// exception is delivered at the game boundary. GetMessage stops modal and outer pumps
// for unowned failures, including thread timers without an HWND.
void CompatAbortCallback(HWND hwnd) noexcept {
    HWND lp;
    while ((lp = LpCompatWindow(hwnd)) != NULL) {
        if (lp->fDialog && lp->fModal) { EndDialog(hwnd, 0); return; }
        hwnd = lp->hwndParent ? lp->hwndParent : lp->hwndOwner;
    }
    PostMessageA(NULL, WM_NULL, 0, 0);
}
