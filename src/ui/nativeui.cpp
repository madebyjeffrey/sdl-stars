#include "win.h"
#include <stars_compat.h>

DWORD GetTextExtent(HDC hdc, LPCSTR str, int len) {
    SIZE size;

    if (!GetTextExtentPoint32A(hdc, str, len, &size))
        return 0;

    return MAKELONG((WORD)size.cx, (WORD)size.cy);
}

/*
 * Frame restore under Wine
 *
 * FrameWndProc handles SC_RESTORE and SC_MAXIMIZE before DefWindowProc
 * restores the window, and while a submitted turn waits it asks, in a task
 * modal MessageBox, whether to load the new turn or unsubmit. Win16 delivered
 * those commands from the message loop. Wine's macOS driver sends SC_RESTORE
 * from inside its handler for a Dock un-minimize and takes no more input
 * events until that send returns, so the MessageBox can never be answered.
 * FrameWndProcDeferred, the frame class's procedure, posts those commands
 * back to the message loop and restores the window before FrameWndProc sees
 * them, so the box shows over the restored frame and gets input.
 */

#define WM_STARS_SYSCOMMAND (WM_APP + 1)

// FrameWndProcDeferred defers FrameWndProc's restore and maximize commands
// to the message loop and passes every other message straight through.
LRESULT CALLBACK FrameWndProcDeferred(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_RESTORE || (wParam & 0xfff0) == SC_MAXIMIZE) {
            PostMessage(hwnd, WM_STARS_SYSCOMMAND, wParam, lParam);
            return 0;
        }
        break;
    case WM_STARS_SYSCOMMAND:
        if (IsIconic(hwnd))
            FUiShowWindow(hwnd, SW_RESTORE);
        return FrameWndProc(hwnd, WM_SYSCOMMAND, wParam, lParam);
    }
    return FrameWndProc(hwnd, msg, wParam, lParam);
}

/*
 * Mouse wheel routing
 *
 * Windows sends wheel messages to the focus window, and the scanner, planet
 * and message panes hand focus back to the frame when clicked, so the wheel
 * would reach none of them. Windows 10's "scroll inactive windows" setting
 * sends the wheel to the window under the cursor instead; Wine and older
 * Windows do not. The message loop calls FRouteMouseWheel to do the same.
 */

// FIsComboBox tells whether hwnd is a combo box.
static int16_t FIsComboBox(HWND hwnd) {
    char szClass[16];

    return hwnd && GetClassNameA(hwnd, szClass, sizeof(szClass)) && lstrcmpiA(szClass, "ComboBox") == 0;
}

// FRouteMouseWheel points a wheel message at the window under the cursor.
// It returns FALSE when the message should be dropped.
int16_t FRouteMouseWheel(MSG *pmsg) {
    POINT pt;
    HWND  hwnd;

    // The message's own cursor position, not lParam: Wine sends the point
    // of WM_MOUSEHWHEEL in client coordinates rather than screen ones.
    pt = pmsg->pt;
    hwnd = WindowFromPoint(pt);
    if (!hwnd || hwnd == pmsg->hwnd || GetWindowThreadProcessId(hwnd, NULL) != GetCurrentThreadId())
        return TRUE;
    // A closed drop-down list changes its selection on the wheel. One the
    // player only points at must not change an order or a battle plan; the
    // edit of a CBS_DROPDOWN box is a child of the box.
    if (FIsComboBox(hwnd) || FIsComboBox(GetParent(hwnd)))
        return FALSE;
    pmsg->hwnd = hwnd;
    return TRUE;
}

// CWheelNotches adds a wheel delta to *pdWheel and returns the whole notches
// (WHEEL_DELTA each) gathered, keeping the remainder. Touchpads and fine
// wheels send part notches. A change of direction starts over.
int16_t CWheelNotches(int16_t *pdWheel, int16_t dWheel) {
    int32_t d;
    int16_t c;

    d = *pdWheel;
    if ((d < 0 && dWheel > 0) || (d > 0 && dWheel < 0))
        d = 0;
    d += dWheel;
    c = (int16_t)(d / WHEEL_DELTA);
    *pdWheel = (int16_t)(d - c * WHEEL_DELTA);
    return c;
}

// CWheelLines returns how many lines a wheel notch scrolls, from the
// player's mouse settings, or cPage when they scroll a page a notch.
int16_t CWheelLines(int16_t cPage) {
    UINT cLines;

    if (cPage < 1) {
        cPage = 1;
    }
    if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &cLines, 0))
        cLines = 3;
    if (cLines == WHEEL_PAGESCROLL || cLines > (UINT)cPage)
        return cPage;
    return (int16_t)cLines;
}


BOOL STARS_TEST_IMPL(FNativeOpenFileName)(OPENFILENAMEA *lpof) { return FUiGetOpenFileName(lpof); }
BOOL STARS_TEST_IMPL(FNativeSaveFileName)(OPENFILENAMEA *lpof) { return FUiGetSaveFileName(lpof); }
DWORD DwNativeFileDialogError(void) { return CommDlgExtendedError(); }

int CchGetUtf8WindowText(HWND hwnd, char *psz, int cchMax) {
    if (StarsCompatTextInputRejected(hwnd)) {
        if (cchMax > 0) psz[0] = 0;
        return -1;
    }
    return GetWindowTextA(hwnd, psz, cchMax);
}

BOOL FSetUtf8WindowText(HWND hwnd, const char *psz) {
    return FUiSetWindowText(hwnd, psz);
}

BOOL FSetNativeWindowTitle(HWND hwnd, const char *psz) {
    return StarsCompatSetWindowTitleUtf8(hwnd, psz);
}
