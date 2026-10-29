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
static int mdControl, cRetained;
static BOOL fTrigger, fThrow;

static void DestroyFromCallback(HWND hwnd) {
    BOOL fRetained;
    fTrigger = FALSE;
    DestroyWindow(hwnd);
    fRetained = !IsWindow(hwnd) && !hwnd->lplist && !hwnd->lpcombo && !hwnd->lptoolbar;
    if (mdControl == 0) fRetained = fRetained && !lplistWeak.expired();
    if (mdControl == 1) fRetained = fRetained && !lpcomboWeak.expired();
    if (mdControl == 2) fRetained = fRetained && !lptoolbarWeak.expired();
    if (fRetained) cRetained++;
    if (fThrow) StarsThrowError(starsErrorFile, 834);
}

static LRESULT CALLBACK Owner(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_MEASUREITEM) {
        reinterpret_cast<MEASUREITEMSTRUCT *>(l)->itemHeight = 18;
        return TRUE;
    }
    if (fTrigger && mdControl == 0 && msg == WM_DRAWITEM) {
        DestroyFromCallback(reinterpret_cast<DRAWITEMSTRUCT *>(l)->hwndItem);
        return TRUE;
    }
    if (fTrigger && mdControl == 1 && msg == WM_COMMAND && HIWORD(w) == CBN_DROPDOWN) {
        DestroyFromCallback(reinterpret_cast<HWND>(l));
        return 0;
    }
    if (fTrigger && mdControl == 2 && msg == WM_NOTIFY && reinterpret_cast<NMHDR *>(l)->code == TBN_DROPDOWN) {
        DestroyFromCallback(reinterpret_cast<NMHDR *>(l)->hwndFrom);
        return TBDDRET_NODEFAULT;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}

static int TestControl(HWND hwndOwner, int md, BOOL fFail) {
    HWND hwnd;
    TBBUTTON btn = {};
    int cSaved = cRetained;

    mdControl = md; fThrow = fFail; fTrigger = FALSE;
    if (md == 0) {
        hwnd = CreateWindow("ListBox", "", WS_CHILD | WS_VISIBLE | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                             0, 0, 100, 80, hwndOwner, (HMENU)1, NULL, NULL);
        CHECK(hwnd && SendMessageA(hwnd, LB_ADDSTRING, 0, (LPARAM)"row") == 0);
    } else if (md == 1) {
        hwnd = CreateWindow("ComboBox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                             0, 0, 100, 100, hwndOwner, (HMENU)2, NULL, NULL);
        CHECK(hwnd && SendMessageA(hwnd, CB_ADDSTRING, 0, (LPARAM)"row") == 0);
    } else {
        hwnd = CreateWindow(TOOLBARCLASSNAMEA, "", WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
                             0, 0, 100, 30, hwndOwner, (HMENU)3, NULL, NULL);
        CHECK(hwnd);
        SendMessageA(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(btn), 0);
        btn.idCommand = 73; btn.fsState = TBSTATE_ENABLED; btn.fsStyle = BTNS_WHOLEDROPDOWN;
        CHECK(SendMessageA(hwnd, TB_ADDBUTTONSA, 1, (LPARAM)&btn));
    }
    lplistWeak = hwnd->lplist; lpcomboWeak = hwnd->lpcombo; lptoolbarWeak = hwnd->lptoolbar;
    fTrigger = TRUE;
    if (md == 0) { InvalidateRect(hwnd, NULL, TRUE); UpdateWindow(hwnd); }
    else if (md == 1) SendMessageA(hwnd, CB_SHOWDROPDOWN, TRUE, 0);
    else {
        SendMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(5, 5));
        SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(5, 5));
    }
    if (IsWindow(hwnd) || cRetained != cSaved + 1)
        std::fprintf(stderr, "control %d, failure %d: alive=%d retained=%d\n", md, fFail, IsWindow(hwnd), cRetained - cSaved);
    CHECK(!IsWindow(hwnd) && cRetained == cSaved + 1);
    CHECK(lplistWeak.expired() && lpcomboWeak.expired() && lptoolbarWeak.expired());
    CHECK(FStarsErrorPending() == fFail && CCompatDispatch() == 0);
    if (fFail) {
        try { StarsRethrowPendingError(); CHECK(FALSE); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 834); }
    }
    CHECK(!FStarsErrorPending() && IsWindow(hwndOwner));
    return 0;
}

int main() {
    WNDCLASSA wc = {};
    HWND hwnd;
    wc.hInstance = GetModuleHandleA(NULL); wc.lpfnWndProc = Owner; wc.lpszClassName = "ControlOwnership";
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Control ownership", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         0, 0, 320, 180, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd);
    for (int i = 0; i < 3; i++) {
        CHECK(TestControl(hwnd, i, FALSE) == 0);
        CHECK(TestControl(hwnd, i, TRUE) == 0);
    }
    CHECK(DestroyWindow(hwnd));
    StarsCompatShutdown();
    std::puts("List, combo and toolbar callbacks retain storage until operation exit; pending errors survive teardown");
    return 0;
}
