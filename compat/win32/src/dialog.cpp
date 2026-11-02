/* Resource-backed dialogs share the existing window, control and message paths. */
#include "cleanup.h"
#include <limits>
#include <stars_compat.h>
#include <cstring>

struct COMPATDLGCREATE { DLGPROC lpfn; HFONT hfont; int cxUnit, cyUnit; BOOL fModal; };

void CompatDestroyDialog(HWND hwnd) {
    if (hwnd->hfontDialog) win32::DeleteObject(hwnd->hfontDialog);
    hwnd->hfontDialog = nullptr;
}
static HWND HwndDialogTab(HWND hwnd, HWND hwndCurrent, BOOL fBack) {
    HWND lp, hwndFirst = nullptr, hwndLast = nullptr, hwndPrevious = nullptr;
    BOOL fFound;
    while (hwndCurrent && win32::IsChild(hwnd, hwndCurrent) && win32::GetParent(hwndCurrent) != hwnd)
        hwndCurrent = win32::GetParent(hwndCurrent);
    fFound = hwndCurrent == nullptr;
    for (lp = win32::GetWindow(hwnd, win32::GW_CHILD); lp; lp = win32::GetWindow(lp, win32::GW_HWNDNEXT)) {
        if (!lp->fVisible || !win32::IsWindowEnabled(lp) || !(lp->dwStyle & win32::WS_TABSTOP)) continue;
        if (!hwndFirst) hwndFirst = lp;
        hwndLast = lp;
        if (lp == hwndCurrent) {
            if (fBack && hwndPrevious) return hwndPrevious;
            fFound = 1;
        } else if (fFound && !fBack) return lp;
        hwndPrevious = lp;
    }
    return fBack ? hwndLast : hwndFirst;
}
LRESULT  CompatDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    COMPATDLGCREATE *lpc;
    INT_PTR result;
    HBRUSH hbr;
    if (!lp) return 0;
    if (msg == win32::WM_NCCREATE) {
        lpc = static_cast<COMPATDLGCREATE *>(((reinterpret_cast<const CREATESTRUCTA *>(l))->lpCreateParams));
        if (!lpc) return 0;
        lp->fDialog = 1; lp->lpfnDlg = lpc->lpfn; lp->hfontDialog = lpc->hfont;
        lpc->hfont = nullptr;
        lp->cxDlgUnit = lpc->cxUnit; lp->cyDlgUnit = lpc->cyUnit; lp->fModal = lpc->fModal;
        return 1;
    }
    if (msg == win32::WM_CREATE) return 0;
    lp->lDlgMsgResult = 0;
    result = lp->lpfnDlg ? CompatInvokeWindowProc(lp->lpfnDlg, hwnd, msg, w, l) : 0;
    // EndDialog defers destruction; default handling must not replace its result.
    if (!win32::IsWindow(hwnd) || lp->fEndDialog) return result;
    if (msg == win32::WM_INITDIALOG) return result;
    if (result) {
        if (msg >= win32::WM_CTLCOLORMSGBOX && msg <= win32::WM_CTLCOLORSTATIC) return result;
        return lp->lDlgMsgResult;
    }
    switch (msg) {
    case win32::WM_GETFONT: return reinterpret_cast<LRESULT>(lp->hfontDialog);
    case win32::WM_ERASEBKGND: {
        RECT rc;
        win32::GetClientRect(hwnd, &rc); hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE));
        if (!hbr) return 0;
        win32::FillRect(reinterpret_cast<HDC>(w), &rc, hbr); win32::DeleteObject(hbr); return 1;
    }
    case win32::WM_CLOSE:
        win32::SendMessageA(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(win32::IDCANCEL, 0), reinterpret_cast<LPARAM>(win32::GetDlgItem(hwnd, win32::IDCANCEL))); return 0;
    case win32::WM_COMMAND:
        // The application decides whether OK/Cancel ends a modal or just an edit.
        return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}
BOOL win32::MapDialogRect(HWND hwnd, LPRECT lprc) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lprc) return 0;
    lprc->left = win32::MulDiv(lprc->left, lp->cxDlgUnit, 4); lprc->right = win32::MulDiv(lprc->right, lp->cxDlgUnit, 4);
    lprc->top = win32::MulDiv(lprc->top, lp->cyDlgUnit, 8); lprc->bottom = win32::MulDiv(lprc->bottom, lp->cyDlgUnit, 8);
    return 1;
}
static BOOL FDialogUnits(COMPATDLGTEMPLATE *lpdlg, COMPATDLGCREATE *lpc) {
    LOGFONTA lf = {};
    HDC hdc;
    HGDIOBJ hold;
    SIZE size;
    TEXTMETRICA tm;
    BOOL fOk;
    const char *pszFont;
    lf.lfHeight = lpdlg->cPoints ? -win32::MulDiv(lpdlg->cPoints, 96, 72) : -11;
    lf.lfWeight = win32::FW_NORMAL;
    pszFont = lpdlg->pszFont ? lpdlg->pszFont : "MS Sans Serif";
    CchCopyUtf8(std::span{lf.lfFaceName}, std::string_view{pszFont, CchUtf8Prefix(pszFont, sizeof(lf.lfFaceName) - 1)});
    lpc->hfont = win32::CreateFontIndirectA(&lf);
    if (!lpc->hfont || !(hdc = win32::CreateCompatibleDC(nullptr))) return 0;
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
    dc.lpfn = lpfn; dc.fModal = 1;
    if (!FDialogUnits(&dlg, &dc)) return nullptr;
    hwnd = win32::CreateWindowExA(dwExStyle, "#32770", pszTitle, dwStyle & ~win32::WS_VISIBLE,
                         lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
                         hwndOwner, nullptr, win32::GetModuleHandleA(nullptr), &dc);
    deleteFont.Close();
    return hwnd;
}
static LPCSTR PszDialogClass(COMPATDLGITEM *lpitem) {
    static const char *rgpsz[] = {"Button", "Edit", "Static", "ListBox", "ScrollBar", "ComboBox"};
    if (lpitem->pszClass && *lpitem->pszClass) return lpitem->pszClass;
    return lpitem->idClass >= 0x80 && lpitem->idClass <= 0x85 ? rgpsz[lpitem->idClass - 0x80] : nullptr;
}
static HWND HwndCreateDialog(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param, BOOL fModal) {
    HRSRC hrsrc;
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGCREATE dc = {};
    COMPATDLGITEM *lpitem;
    HWND hwnd = nullptr, hwndChild, hwndFocus;
    RECT rc;
    WORD i;
    LPCSTR pszClass, pszTitle;
    BOOL fFocus;
    COMPATCLEANUPSCOPE freeTemplate([&] { CompatFreeDialogTemplate(&dlg); });
    COMPATCLEANUPSCOPE deleteFont([&] { if (dc.hfont) win32::DeleteObject(dc.hfont); });
    COMPATCLEANUPSCOPE destroyDialog([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); }, hwndOwner);
    // A checked caller cannot take ownership of a modeless window on failure.
    if (!fModal && FStarsErrorPending()) return nullptr;
    if (!SDL_IsMainThread() || (hwndOwner && !win32::IsWindow(hwndOwner))) return nullptr;
    if (!hinst) hinst = win32::GetModuleHandleA(nullptr);
    hrsrc = win32::FindResourceA(hinst, psz, win32::RT_DIALOG);
    if (!FCompatDialogTemplate(static_cast<const BYTE *>((win32::LockResource(win32::LoadResource(hinst, hrsrc)))), win32::SizeofResource(hinst, hrsrc), &dlg)) return nullptr;
    if (dlg.idClass || (dlg.pszClass && *dlg.pszClass) || dlg.idMenu || (dlg.pszMenu && *dlg.pszMenu)) goto done;
    dc.lpfn = lpfn; dc.fModal = fModal;
    if (!FDialogUnits(&dlg, &dc)) goto done;
    hwnd = win32::CreateWindowExA(dlg.dwExStyle, "#32770", dlg.pszTitle, dlg.dwStyle & ~win32::WS_VISIBLE,
                         win32::MulDiv(dlg.x, dc.cxUnit, 4), win32::MulDiv(dlg.y, dc.cyUnit, 8),
                         win32::MulDiv(dlg.cx, dc.cxUnit, 4), win32::MulDiv(dlg.cy, dc.cyUnit, 8), hwndOwner, nullptr, hinst, &dc);
    // The dialog record owns the font after WM_NCCREATE, including failure cleanup.
    if (!hwnd) goto done;
    dc.hfont = nullptr;
    if (!fModal && FStarsErrorPending()) goto fail;
    win32::SendMessageA(hwnd, win32::WM_SETFONT, reinterpret_cast<WPARAM>(hwnd->hfontDialog), 0);
    if (!win32::IsWindow(hwnd)) { hwnd = nullptr; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    for (i = 0; i < dlg.cItems; i++) {
        lpitem = &dlg.rgItems[i]; pszClass = PszDialogClass(lpitem);
        pszTitle = lpitem->pszTitle ? lpitem->pszTitle : win32::MAKEINTRESOURCE(lpitem->idTitle);
        if (!pszClass || lpitem->cbCreate || reinterpret_cast<UINT_PTR>(pszTitle) <= std::numeric_limits<std::uint16_t>::max()) goto fail;
        rc = RECT{lpitem->x, lpitem->y, lpitem->cx, lpitem->cy};
        win32::MapDialogRect(hwnd, &rc);
        hwndChild = win32::CreateWindowExA(lpitem->dwExStyle, pszClass, pszTitle, lpitem->dwStyle,
                                   rc.left, rc.top, rc.right, rc.bottom,
                                   hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(static_cast<std::int16_t>(lpitem->id))), hinst, nullptr);
        if (!hwndChild || !win32::IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
        win32::SendMessageA(hwndChild, win32::WM_SETFONT, reinterpret_cast<WPARAM>(hwnd->hfontDialog), 0);
        if (!win32::IsWindow(hwnd) || (!fModal && FStarsErrorPending())) goto fail;
    }
    hwndFocus = HwndDialogTab(hwnd, nullptr, 0);
    fFocus = win32::SendMessageA(hwnd, win32::WM_INITDIALOG, reinterpret_cast<WPARAM>(hwndFocus), param) != 0;
    if (!win32::IsWindow(hwnd)) { hwnd = nullptr; goto done; }
    if (!fModal && FStarsErrorPending()) goto fail;
    if (!hwnd->fEndDialog) {
        if (fModal || (dlg.dwStyle & win32::WS_VISIBLE)) {
            // Prepare the initialized dialog before exposing its native window;
            // otherwise the compositor first displays the empty black canvas.
            win32::InvalidateRect(hwnd, nullptr, 1);
            win32::UpdateWindow(hwnd);
            if (!win32::IsWindow(hwnd)) { hwnd = nullptr; goto done; }
            if (!fModal && FStarsErrorPending()) goto fail;
            for (hwndChild = win32::GetWindow(hwnd, win32::GW_CHILD); hwndChild; hwndChild = win32::GetWindow(hwndChild, win32::GW_HWNDNEXT)) {
                win32::UpdateWindow(hwndChild);
                if (!win32::IsWindow(hwnd)) { hwnd = nullptr; goto done; }
                if (!fModal && FStarsErrorPending()) goto fail;
            }
            hwnd->fDeferPresent = 0;
            if (!FCompatPresent(hwnd)) goto fail;
            if (!hwnd->fEndDialog) {
                win32::ShowWindow(hwnd, win32::SW_SHOW);
            }
        }
        if (fFocus && win32::IsWindow(hwndFocus) && win32::IsWindowEnabled(hwndFocus)) win32::SetFocus(hwndFocus);
    }
    if (!win32::IsWindow(hwnd)) hwnd = nullptr;
    if (!fModal && FStarsErrorPending()) goto fail;
    goto done;
fail:
    destroyDialog.Close();
    hwnd = nullptr;
done:
    deleteFont.Close();
    freeTemplate.Close();
    destroyDialog.Release();
    return hwnd;
}
HWND win32::CreateDialogParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    return HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, 0);
}
BOOL win32::EndDialog(HWND hwnd, INT_PTR result) {
    HWND lp = LpCompatWindow(hwnd);
    if (!lp || !lp->fDialog || !lp->fModal) return 0;
    lp->lDlgResult = result; lp->fEndDialog = 1;
    return 1;
}
BOOL win32::IsDialogMessageA(HWND hwnd, LPMSG lpmsg) {
    HWND lp = LpCompatWindow(hwnd), hwndFocus, hwndNext;
    LRESULT code;
    UINT id;
    if (!lp || !lp->fDialog || !lpmsg || (lpmsg->hwnd != hwnd && !win32::IsChild(hwnd, lpmsg->hwnd))) return 0;
    if (lpmsg->message != win32::WM_KEYDOWN && lpmsg->message != win32::WM_SYSKEYDOWN) return 0;
    code = win32::SendMessageA(lpmsg->hwnd, win32::WM_GETDLGCODE, lpmsg->wParam, reinterpret_cast<LPARAM>(lpmsg));
    if (!win32::IsWindow(hwnd)) return 1;
    if (code & win32::DLGC_WANTALLKEYS) return 0;
    hwndFocus = win32::GetFocus();
    if (lpmsg->wParam == win32::VK_TAB && !(code & win32::DLGC_WANTTAB)) {
        hwndNext = HwndDialogTab(hwnd, hwndFocus, (win32::GetKeyState(win32::VK_SHIFT) & 0x8000) != 0);
        if (hwndNext) win32::SetFocus(hwndNext);
        return 1;
    }
    if (lpmsg->wParam == win32::VK_RETURN || lpmsg->wParam == win32::VK_ESCAPE) {
        id = lpmsg->wParam == win32::VK_ESCAPE ? win32::IDCANCEL : win32::IDOK;
        if (lpmsg->wParam == win32::VK_RETURN) {
            for (hwndNext = win32::GetWindow(hwnd, win32::GW_CHILD); hwndNext; hwndNext = win32::GetWindow(hwndNext, win32::GW_HWNDNEXT))
                if (hwndNext->iControl == 1 && (hwndNext->dwStyle & win32::BS_TYPEMASK) == win32::BS_DEFPUSHBUTTON) { id = static_cast<UINT>(win32::GetDlgCtrlID(hwndNext)); break; }
        }
        hwndNext = win32::GetDlgItem(hwnd, id);
        if (!hwndNext || win32::IsWindowEnabled(hwndNext)) win32::SendMessageA(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(id, win32::BN_CLICKED), reinterpret_cast<LPARAM>(hwndNext));
        return 1;
    }
    return 0;
}
INT_PTR win32::DialogBoxParamA(HINSTANCE hinst, LPCSTR psz, HWND hwndOwner, DLGPROC lpfn, LPARAM param) {
    HWND hwnd = nullptr, hwndFocus = win32::GetFocus(), hwndRoot;
    BOOL fEnable;
    win32::MSG msg;
    int result;
    INT_PTR value = -1;
    if (!SDL_IsMainThread() || FStarsErrorPending()) return -1;
    // Nested wizard loops must unwind without exposing another page after application quit.
    if (win32::PeekMessageA(&msg, nullptr, win32::WM_QUIT, win32::WM_QUIT, win32::PM_NOREMOVE)) return -1;
    if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : nullptr;
    fEnable = hwndRoot && win32::IsWindowEnabled(hwndRoot);
    COMPATCLEANUPSCOPE restoreOwner([&] {
        if (FCompatPendingDialogSequence(hwndRoot)) return;
        if (fEnable && win32::IsWindow(hwndRoot)) win32::EnableWindow(hwndRoot, 1);
        CompatRestoreOwnerFocus(hwndRoot, hwndFocus);
    }, hwndRoot);
    COMPATCLEANUPSCOPE destroyDialog([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); }, hwndRoot);
    if (fEnable) win32::EnableWindow(hwndRoot, 0);
    if (FStarsErrorPending()) return -1;
    hwnd = HwndCreateDialog(hinst, psz, hwndOwner, lpfn, param, 1);
    if (hwnd) {
        while (win32::IsWindow(hwnd) && !hwnd->fEndDialog) {
            result = win32::GetMessageA(&msg, nullptr, 0, 0);
            if (result <= 0) { if (result == 0) win32::PostQuitMessage(static_cast<int>(msg.wParam)); break; }
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
    while ((lp = LpCompatWindow(hwnd)) != nullptr) {
        if (lp->fDialog && lp->fModal) { win32::EndDialog(hwnd, 0); return; }
        hwnd = lp->hwndParent ? lp->hwndParent : lp->hwndOwner;
    }
    win32::PostMessageA(nullptr, win32::WM_NULL, 0, 0);
}
