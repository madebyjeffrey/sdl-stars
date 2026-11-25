// Notification callbacks can destroy a control while its outer operation owns
// borrowed state. Logical destruction is immediate; storage lasts to frame exit.
#include "internal.h"
#include <stars_compat.h>
#include <commctrl.h>
#include <cstdio>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static std::weak_ptr<COMPATLIST> lplistWeak;
static std::weak_ptr<COMPATCOMBO> lpcomboWeak;
static std::weak_ptr<COMPATTOOLBAR> lptoolbarWeak;
static int mdControl, cRetained, mdOpenFailure;
static BOOL fTrigger, fThrow, fReplacePopup;

static void DestroyFromCallback(HWND hwnd) {
    BOOL fRetained;
    fTrigger = 0;
    win32::DestroyWindow(hwnd);
    fRetained = !win32::IsWindow(hwnd) && !hwnd->lplist && !hwnd->lpcombo && !hwnd->lptoolbar;
    if (mdControl == 0) fRetained = fRetained && !lplistWeak.expired();
    if (mdControl == 1) fRetained = fRetained && !lpcomboWeak.expired();
    if (mdControl == 2) fRetained = fRetained && !lptoolbarWeak.expired();
    if (fRetained) cRetained++;
    if (fThrow) StarsThrowError(starsErrorFile, 834);
}

static LRESULT  Owner(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if ((mdOpenFailure == 1 && msg == win32::WM_COMMAND && HIWORD(w) == win32::CBN_DROPDOWN) ||
        (mdOpenFailure == 2 && msg == win32::WM_DRAWITEM)) StarsThrowError(starsErrorFile, 835);
    if (msg == win32::WM_DRAWITEM && fReplacePopup) {
        HWND hwndCombo = reinterpret_cast<DRAWITEMSTRUCT *>(l)->hwndItem;
        fReplacePopup = 0;
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 0, 0);
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 1, 0);
        return 1;
    }
    if (msg == win32::WM_MEASUREITEM) {
        reinterpret_cast<MEASUREITEMSTRUCT *>(l)->itemHeight = 18;
        return 1;
    }
    if (fTrigger && mdControl == 0 && msg == win32::WM_DRAWITEM) {
        DestroyFromCallback(reinterpret_cast<DRAWITEMSTRUCT *>(l)->hwndItem);
        return 1;
    }
    if (fTrigger && mdControl == 1 && msg == win32::WM_COMMAND && HIWORD(w) == win32::CBN_DROPDOWN) {
        DestroyFromCallback(reinterpret_cast<HWND>(l));
        return 0;
    }
    if (fTrigger && mdControl == 2 && msg == win32::WM_NOTIFY && reinterpret_cast<NMHDR *>(l)->code == win32::TBN_DROPDOWN) {
        DestroyFromCallback(reinterpret_cast<NMHDR *>(l)->hwndFrom);
        return win32::TBDDRET_NODEFAULT;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}

static int TestControl(HWND hwndOwner, int md, BOOL fFail) {
    HWND hwnd;
    TBBUTTON btn = {};
    int cSaved = cRetained;

    mdControl = md; fThrow = fFail; fTrigger = 0;
    if (md == 0) {
        hwnd = win32::CreateWindowExA(0, "ListBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::LBS_OWNERDRAWFIXED | win32::LBS_HASSTRINGS,
                             0, 0, 100, 80, hwndOwner, (HMENU)1, NULL, NULL);
        CHECK(hwnd && win32::SendMessageA(hwnd, win32::LB_ADDSTRING, 0, (LPARAM)"row") == 0);
    } else if (md == 1) {
        hwnd = win32::CreateWindowExA(0, "ComboBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::CBS_DROPDOWNLIST,
                             0, 0, 100, 100, hwndOwner, (HMENU)2, NULL, NULL);
        CHECK(hwnd && win32::SendMessageA(hwnd, win32::CB_ADDSTRING, 0, (LPARAM)"row") == 0);
    } else {
        hwnd = win32::CreateWindowExA(0, win32::TOOLBARCLASSNAMEA, "", win32::WS_CHILD | win32::WS_VISIBLE | win32::TBSTYLE_TOOLTIPS,
                             0, 0, 100, 30, hwndOwner, (HMENU)3, NULL, NULL);
        CHECK(hwnd);
        win32::SendMessageA(hwnd, win32::TB_BUTTONSTRUCTSIZE, sizeof(btn), 0);
        btn.idCommand = 73; btn.fsState = win32::TBSTATE_ENABLED; btn.fsStyle = win32::BTNS_WHOLEDROPDOWN;
        CHECK(win32::SendMessageA(hwnd, win32::TB_ADDBUTTONSA, 1, (LPARAM)&btn));
    }
    lplistWeak = hwnd->lplist; lpcomboWeak = hwnd->lpcombo; lptoolbarWeak = hwnd->lptoolbar;
    fTrigger = 1;
    if (md == 0) { win32::InvalidateRect(hwnd, NULL, 1); win32::UpdateWindow(hwnd); }
    else if (md == 1) win32::SendMessageA(hwnd, win32::CB_SHOWDROPDOWN, 1, 0);
    else {
        win32::SendMessageA(hwnd, win32::WM_LBUTTONDOWN, win32::MK_LBUTTON, win32::MAKELPARAM(5, 5));
        win32::SendMessageA(hwnd, win32::WM_LBUTTONUP, 0, win32::MAKELPARAM(5, 5));
    }
    if (win32::IsWindow(hwnd) || cRetained != cSaved + 1)
        std::fprintf(stderr, "control %d, failure %d: alive=%d retained=%d\n", md, fFail, win32::IsWindow(hwnd), cRetained - cSaved);
    CHECK(!win32::IsWindow(hwnd) && cRetained == cSaved + 1);
    CHECK(lplistWeak.expired() && lpcomboWeak.expired() && lptoolbarWeak.expired());
    CHECK(FStarsErrorPending() == fFail && CCompatDispatch() == 0);
    if (fFail) {
        try { StarsRethrowPendingError(); CHECK(0); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 834); }
    }
    CHECK(!FStarsErrorPending() && win32::IsWindow(hwndOwner));
    return 0;
}

int main() {
    WNDCLASSA wc = {};
    HWND hwnd;
    wc.hInstance = win32::GetModuleHandleA(NULL); wc.lpfnWndProc = Owner; wc.lpszClassName = "ControlOwnership";
    CHECK(win32::RegisterClassA(&wc));
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Control ownership", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                         0, 0, 320, 180, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    for (int i = 0; i < 3; i++) {
        CHECK(TestControl(hwnd, i, 0) == 0);
        CHECK(TestControl(hwnd, i, 1) == 0);
    }
    for (int i = 1; i < 3; i++) {
        HWND hwndCombo = win32::CreateWindowExA(0, "ComboBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::CBS_DROPDOWNLIST | win32::CBS_OWNERDRAWFIXED | win32::CBS_HASSTRINGS,
                                      0, 0, 100, 100, hwnd, (HMENU)4, NULL, NULL);
        if (i == 2) win32::SetCapture(hwnd);
        HWND hwndCapture = win32::GetCapture();
        CHECK(hwndCombo && win32::SendMessageA(hwndCombo, win32::CB_ADDSTRING, 0, (LPARAM)"row") == 0);
        mdOpenFailure = i;
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 1, 0);
        CHECK(FStarsErrorPending() && !hwndCombo->lpcombo->fOpening && !hwndCombo->lpcombo->fDropped);
        CHECK(!hwndCombo->lpcombo->hwndPopup && win32::GetCapture() == hwndCapture);
        try { StarsRethrowPendingError(); CHECK(0); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 835); }
        mdOpenFailure = 0;
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 1, 0);
        CHECK(!FStarsErrorPending() && hwndCombo->lpcombo->fDropped && win32::GetCapture() == hwndCombo);
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 0, 0);
        CHECK(!hwndCombo->lpcombo->fDropped && win32::GetCapture() == hwndCapture);
        CHECK(win32::DestroyWindow(hwndCombo));
        win32::ReleaseCapture();
    }
    {
        HWND hwndCombo = win32::CreateWindowExA(0, "ComboBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::CBS_DROPDOWNLIST | win32::CBS_OWNERDRAWFIXED | win32::CBS_HASSTRINGS,
                                      0, 0, 100, 100, hwnd, (HMENU)4, NULL, NULL);
        CHECK(hwndCombo && win32::SendMessageA(hwndCombo, win32::CB_ADDSTRING, 0, (LPARAM)"row") == 0);
        fReplacePopup = 1;
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 1, 0);
        CHECK(!fReplacePopup && !FStarsErrorPending() && hwndCombo->lpcombo->fDropped);
        CHECK(win32::IsWindow(hwndCombo->lpcombo->hwndPopup) && win32::GetCapture() == hwndCombo);
        win32::SendMessageA(hwndCombo, win32::CB_SHOWDROPDOWN, 0, 0);
        CHECK(win32::DestroyWindow(hwndCombo));
    }
    CHECK(win32::DestroyWindow(hwnd));
    StarsCompatShutdown();
    std::puts("List, combo and toolbar callbacks retain storage until operation exit; pending errors survive teardown");
    return 0;
}
