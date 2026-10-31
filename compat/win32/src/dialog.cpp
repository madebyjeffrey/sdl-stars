/* Resource-backed dialogs share the existing window, control and message paths. */
#include "cleanup.h"
#include <stars_compat.h>
#include <string.h>

typedef struct COMPATDLGCREATE { DLGPROC lpfn; HFONT hfont; int cxUnit, cyUnit; BOOL fModal; } COMPATDLGCREATE;

void CompatDestroyDialog(HWND hwnd) {
    if (hwnd->hfontDialog) win32::DeleteObject(hwnd->hfontDialog);
    hwnd->hfontDialog = NULL;
}
static HWND HwndDialogTab(HWND hwnd, HWND hwndCurrent, BOOL fBack) {
    HWND lp, hwndFirst = NULL, hwndLast = NULL, hwndPrevious = NULL;
    BOOL fFound;
    while (hwndCurrent && win32::IsChild(hwnd, hwndCurrent) && win32::GetParent(hwndCurrent) != hwnd)
        hwndCurrent = win32::GetParent(hwndCurrent);
    fFound = hwndCurrent == NULL;
    for (lp = win32::GetWindow(hwnd, GW_CHILD); lp; lp = win32::GetWindow(lp, GW_HWNDNEXT)) {
        if (!lp->fVisible || !win32::IsWindowEnabled(lp) || !(lp->dwStyle & WS_TABSTOP)) continue;
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
    if (!win32::IsWindow(hwnd) || lp->fEndDialog) return result;
    if (msg == WM_INITDIALOG) return result;
    if (result) {
        if (msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) return result;
        return lp->lDlgMsgResult;
    }
    switch (msg) {
    case WM_GETFONT: return (LRESULT)lp->hfontDialog;
    case WM_ERASEBKGND: {
        RECT rc;
        win32::GetClientRect(hwnd, &rc); hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNFACE));
        if (!hbr) return FALSE;
        win32::FillRect((HDC)w, &rc, hbr); win32::DeleteObject(hbr); return TRUE;
    }
    case WM_CLOSE:
        win32::SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM)win32::GetDlgItem(hwnd, IDCANCEL)); return 0;
    case WM_COMMAND:
        // The application decides whether OK/Cancel ends a modal or just an edit.
        return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
BOOL win32::MapDialogRect(HWND hwnd, LPRECT lprc) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lprc) return FALSE;
    lprc->left = win32::MulDiv(lprc->left, lp->cxDlgUnit, 4); lprc->right = win32::MulDiv(lprc->right, lp->cxDlgUnit, 4);
    lprc->top = win32::MulDiv(lprc->top, lp->cyDlgUnit, 8); lprc->bottom = win32::MulDiv(lprc->bottom, lp->cyDlgUnit, 8);
    return TRUE;
}
static BOOL FDialogUnits(COMPATDLGTEMPLATE *lpdlg, COMPATDLGCREATE *lpc) {
    LOGFONTA lf = {};
    HDC hdc;
    HGDIOBJ hold;
    SIZE size;
    TEXTMETRICA tm;
    BOOL fOk;
    lf.lfHeight = lpdlg->cPoints ? -win32::MulDiv(lpdlg->cPoints, 96, 72) : -11;
    lf.lfWeight = FW_NORMAL;
    CchCopyUtf8(lf.lfFaceName, sizeof(lf.lfFaceName), lpdlg->pszFont ? lpdlg->pszFont : "MS Sans Serif",
                CchUtf8Prefix(lpdlg->pszFont ? lpdlg->pszFont : "MS Sans Serif", sizeof(lf.lfFaceName) - 1));
    lpc->hfont = win32::CreateFontIndirectA(&lf);
    if (!lpc->hfont || !(hdc = win32::CreateCompatibleDC(NULL))) return FALSE;
    hold = win32::SelectObject(hdc, lpc->hfont);
    fOk = win32::GetTextExtentPoint32A(hdc, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size) && win32::GetTextMetricsA(hdc, &tm);
    if (fOk) { lpc->cxUnit = (size.cx / 26 + 1) / 2; lpc->cyUnit = tm.tmHeight; fOk = lpc->cxUnit > 0 && lpc->cyUnit > 0; }
    win32::SelectObject(hdc, hold); win32::DeleteDC(hdc); return fOk;
}
HWND HwndCompatDynamicDialog(LPCSTR pszTitle, DWORD dwStyle, DWORD dwExStyle, const RECT *lprc,
                             HWND hwndOwner, DLGPROC lpfn) {
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGCREATE dc = {};
    HWND hwnd;
    COMPATCLEANUPSCOPE deleteFont([&] { if (dc.hfont) win32::DeleteObject(dc.hfont); });
    dlg.cPoints = 8;
    dc.lpfn = lpfn; dc.fModal = TRUE;
    if (!FDialogUnits(&dlg, &dc)) return NULL;
    hwnd = win32::CreateWindowExA(dwExStyle, "#32770", pszTitle, dwStyle & ~WS_VISIBLE,
                         lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
                         hwndOwner, NULL, win32::GetModuleHandleA(NULL), &dc);
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
    COMPATCLEANUPSCOPE deleteFont([&] { if (dc.hfont) win32::DeleteObject(dc.hfont); });
    COMPATCLEANUPSCOPE destroyDialog([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); }, hwndOwner);
    // A checked caller cannot take ownership of a modeless window on failure.
    if (!fModal && FStarsErrorPending()) return NULL;
    if (!SDL_IsMainThread() || (hwndOwner && !win32::IsWindow(hwndOwner))) return NULL;
    if (!hinst) hinst = win32::GetModuleHandleA(NULL);
    hrsrc = win32::FindResourceA(hinst, psz, RT_DIALOG);
    if (!FCompatDialogTemplate((const BYTE *)(win32::LockResource(win32::LoadResource(hinst, hrsrc))), win32::SizeofResource(hinst, hrsrc), &dlg)) return NULL;
    if (dlg.idClass || (dlg.pszClass && *dlg.pszClass) || dlg.idMenu || (dlg.pszMenu && *dlg.pszMenu)) goto done;
    dc.lpfn = lpfn; dc.fModal = fModal;
    if (!FDialogUnits(&dlg, &dc)) goto done;
    hwnd = win32::CreateWindowExA(dlg.dwExStyle, "#32770", dlg.pszTitle, dlg.dwStyle & ~WS_VISIBLE,
                         win32::MulDiv(dlg.x, dc.cxUnit, 4), win32::MulDiv(dlg.y, dc.cyUnit, 8),
                         win32::MulDiv(dlg.cx, dc.cxUnit, 4), win32::MulDiv(dlg.cy, dc.cyUnit, 8), hwndOwner, NULL, hinst, &dc);
    // The dialog record owns the font after WM_NCCREATE, including failure cleanup.
    if (!hwnd) goto done;
    dc.hfont = NULL;
    if (!fModal && FStarsErrorPending()) goto fail;
    win32::SendMessageA(hwnd, WM_SETFONT, (WPARAM)hwnd->hfontDialog, 0);
    if (!win32::IsWindow(hwnd)) { hwnd = NULL; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    for (i = 0; i < dlg.cItems; i++) {
        lpitem = &dlg.rgItems[i]; pszClass = PszDialogClass(lpitem);
        pszTitle = lpitem->pszTitle ? lpitem->pszTitle : MAKEINTRESOURCE(lpitem->idTitle);
        if (!pszClass || lpitem->cbCreate || (UINT_PTR)pszTitle <= UINT16_MAX) goto fail;
        rc = RECT{lpitem->x, lpitem->y, lpitem->cx, lpitem->cy};
        win32::MapDialogRect(hwnd, &rc);
        hwndChild = win32::CreateWindowExA(lpitem->dwExStyle, pszClass, pszTitle, lpitem->dwStyle,
                                   rc.left, rc.top, rc.right, rc.bottom,
                                   hwnd, (HMENU)(INT_PTR)(int16_t)lpitem->id, hinst, NULL);
        if (!hwndChild || !win32::IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
        win32::SendMessageA(hwndChild, WM_SETFONT, (WPARAM)hwnd->hfontDialog, 0);
        if (!win32::IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
    }
    hwndFocus = HwndDialogTab(hwnd, NULL, FALSE);
    fFocus = win32::SendMessageA(hwnd, WM_INITDIALOG, (WPARAM)hwndFocus, param) != 0;
    if (!win32::IsWindow(hwnd)) { hwnd = NULL; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    if (!hwnd->fEndDialog) {
        if (fModal || (dlg.dwStyle & WS_VISIBLE)) {
            // Prepare the initialized dialog before exposing its native window;
            // otherwise the compositor first displays the empty black canvas.
            win32::InvalidateRect(hwnd, NULL, TRUE);
            win32::UpdateWindow(hwnd);
            if (!win32::IsWindow(hwnd)) { hwnd = NULL; goto done; }
            if (!fModal && FStarsErrorPending()) goto fail;
            for (hwndChild = win32::GetWindow(hwnd, GW_CHILD); hwndChild; hwndChild = win32::GetWindow(hwndChild, GW_HWNDNEXT)) {
                win32::UpdateWindow(hwndChild);
                if (!win32::IsWindow(hwnd)) { hwnd = NULL; goto done; }
                if (!fModal && FStarsErrorPending()) goto fail;
            }
            hwnd->fDeferPresent = FALSE;
            if (!FCompatPresent(hwnd)) goto fail;
            if (!hwnd->fEndDialog) {
                win32::ShowWindow(hwnd, SW_SHOW);
            }
        }
        if (fFocus && win32::IsWindow(hwndFocus) && win32::IsWindowEnabled(hwndFocus)) win32::SetFocus(hwndFocus);
    }
    if (!win32::IsWindow(hwnd)) hwnd = NULL;
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
HWND win32::CreateDialogParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    return HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, FALSE);
}
BOOL win32::EndDialog(HWND hwnd, INT_PTR result) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lp->fModal) return FALSE;
    lp->lDlgResult = result; lp->fEndDialog = TRUE;
    return TRUE;
}
BOOL win32::IsDialogMessageA(HWND hwnd, LPMSG lpmsg) {
    HWND lp = LpCompatWindow(hwnd), hwndFocus, hwndNext;
    LRESULT code;
    UINT id;
    if (!lp || !lp->fDialog || !lpmsg || (lpmsg->hwnd != hwnd && !win32::IsChild(hwnd, lpmsg->hwnd))) return FALSE;
    if (lpmsg->message != WM_KEYDOWN && lpmsg->message != WM_SYSKEYDOWN) return FALSE;
    code = win32::SendMessageA(lpmsg->hwnd, WM_GETDLGCODE, lpmsg->wParam, (LPARAM)lpmsg);
    if (!win32::IsWindow(hwnd)) return TRUE;
    if (code & DLGC_WANTALLKEYS) return FALSE;
    hwndFocus = win32::GetFocus();
    if (lpmsg->wParam == VK_TAB && !(code & DLGC_WANTTAB)) {
        hwndNext = HwndDialogTab(hwnd, hwndFocus, (win32::GetKeyState(VK_SHIFT) & 0x8000) != 0);
        if (hwndNext) win32::SetFocus(hwndNext);
        return TRUE;
    }
    if (lpmsg->wParam == VK_RETURN || lpmsg->wParam == VK_ESCAPE) {
        id = lpmsg->wParam == VK_ESCAPE ? IDCANCEL : IDOK;
        if (lpmsg->wParam == VK_RETURN) {
            for (hwndNext = win32::GetWindow(hwnd, GW_CHILD); hwndNext; hwndNext = win32::GetWindow(hwndNext, GW_HWNDNEXT))
                if (hwndNext->iControl == 1 && (hwndNext->dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON) { id = (UINT)win32::GetDlgCtrlID(hwndNext); break; }
        }
        hwndNext = win32::GetDlgItem(hwnd, id);
        if (!hwndNext || win32::IsWindowEnabled(hwndNext)) win32::SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)hwndNext);
        return TRUE;
    }
    return FALSE;
}
INT_PTR win32::DialogBoxParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    HWND hwnd = NULL, hwndFocus = win32::GetFocus(), hwndRoot;
    BOOL fEnable;
    win32::MSG msg;
    int result;
    INT_PTR value = -1;
    if (!SDL_IsMainThread() || FStarsErrorPending()) return -1;
    // Nested wizard loops must unwind without exposing another page after application quit.
    if (win32::PeekMessageA(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE)) return -1;
    if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : NULL;
    fEnable = hwndRoot && win32::IsWindowEnabled(hwndRoot);
    COMPATCLEANUPSCOPE restoreOwner([&] {
        if (FCompatPendingDialogSequence(hwndRoot)) return;
        if (fEnable && win32::IsWindow(hwndRoot)) win32::EnableWindow(hwndRoot, TRUE);
        CompatRestoreOwnerFocus(hwndRoot, hwndFocus);
    }, hwndRoot);
    COMPATCLEANUPSCOPE destroyDialog([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); }, hwndRoot);
    if (fEnable) win32::EnableWindow(hwndRoot, FALSE);
    if (FStarsErrorPending()) return -1;
    hwnd = HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, TRUE);
    if (hwnd) {
        while (win32::IsWindow(hwnd) && !hwnd->fEndDialog) {
            result = win32::GetMessageA(&msg, NULL, 0, 0);
            if (result <= 0) { if (result == 0) win32::PostQuitMessage((int)msg.wParam); break; }
            if (!win32::IsDialogMessageA(hwnd, &msg)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
        }
        value = hwnd->fEndDialog ? hwnd->lDlgResult : -1;
        if (win32::IsWindow(hwnd)) {
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
        if (lp->fDialog && lp->fModal) { win32::EndDialog(hwnd, 0); return; }
        hwnd = lp->hwndParent ? lp->hwndParent : lp->hwndOwner;
    }
    win32::PostMessageA(NULL, WM_NULL, 0, 0);
}
