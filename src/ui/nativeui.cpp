#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "control_io.h"
#include <stars_compat.h>

DWORD GetTextExtent(HDC hdc, LPCSTR str, int len) {
    SIZE size;

    if (!win32::GetTextExtentPoint32A(hdc, str, len, &size))
        return 0;

    return MAKELONG(static_cast<WORD>(size.cx), static_cast<WORD>(size.cy));
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

constexpr auto WM_STARS_SYSCOMMAND = (win32::WM_APP + 1);

// FrameWndProcDeferred defers FrameWndProc's restore and maximize commands
// to the message loop and passes every other message straight through.
LRESULT  FrameWndProcDeferred(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    switch (msg) {
    case win32::WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == win32::SC_RESTORE || (wParam & 0xfff0) == win32::SC_MAXIMIZE) {
            win32::PostMessageA(hwnd, WM_STARS_SYSCOMMAND, wParam, lParam);
            return 0;
        }
        break;
    case WM_STARS_SYSCOMMAND:
        if (win32::IsIconic(hwnd))
            ui::ShowWindow(hwnd, win32::SW_RESTORE);
        return FrameWndProc(app, hwnd, win32::WM_SYSCOMMAND, wParam, lParam);
    }
    return FrameWndProc(app, hwnd, msg, wParam, lParam);
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
static std::int16_t FIsComboBox(HWND hwnd) {
    char szClass[16];

    return hwnd && win32::GetClassNameA(hwnd, szClass, sizeof(szClass)) && win32::lstrcmpiA(szClass, "ComboBox") == 0;
}

// FRouteMouseWheel points a wheel message at the window under the cursor.
// It returns FALSE when the message should be dropped.
std::int16_t FRouteMouseWheel(win32::MSG *pmsg) {
    POINT pt;
    HWND  hwnd;

    // The message's own cursor position, not lParam: Wine sends the point
    // of WM_MOUSEHWHEEL in client coordinates rather than screen ones.
    pt = pmsg->pt;
    hwnd = win32::WindowFromPoint(pt);
    if (!hwnd || hwnd == pmsg->hwnd || win32::GetWindowThreadProcessId(hwnd, nullptr) != win32::GetCurrentThreadId())
        return 1;
    // A closed drop-down list changes its selection on the wheel. One the
    // player only points at must not change an order or a battle plan; the
    // edit of a CBS_DROPDOWN box is a child of the box.
    if (FIsComboBox(hwnd) || FIsComboBox(win32::GetParent(hwnd)))
        return 0;
    pmsg->hwnd = hwnd;
    return 1;
}

// CWheelNotches adds a wheel delta to *pdWheel and returns the whole notches
// (WHEEL_DELTA each) gathered, keeping the remainder. Touchpads and fine
// wheels send part notches. A change of direction starts over.
std::int16_t CWheelNotches(std::int16_t *pdWheel, std::int16_t dWheel) {
    std::int32_t d;
    std::int16_t c;

    d = *pdWheel;
    if ((d < 0 && dWheel > 0) || (d > 0 && dWheel < 0))
        d = 0;
    d += dWheel;
    c = static_cast<std::int16_t>((d / win32::WHEEL_DELTA));
    *pdWheel = static_cast<std::int16_t>((d - c * win32::WHEEL_DELTA));
    return c;
}

// CWheelLines returns how many lines a wheel notch scrolls, from the
// player's mouse settings, or cPage when they scroll a page a notch.
std::int16_t CWheelLines(std::int16_t cPage) {
    UINT cLines;

    if (cPage < 1) {
        cPage = 1;
    }
    if (!win32::SystemParametersInfoA(win32::SPI_GETWHEELSCROLLLINES, 0, &cLines, 0))
        cLines = 3;
    if (cLines == win32::WHEEL_PAGESCROLL || cLines > static_cast<UINT>(cPage))
        return cPage;
    return static_cast<std::int16_t>(cLines);
}


int CchGetUtf8WindowText(HWND hwnd, char *psz, int cchMax) {
    if (StarsCompatTextInputRejected(hwnd)) {
        if (cchMax > 0) psz[0] = 0;
        return -1;
    }
    return win32::GetWindowTextA(hwnd, psz, cchMax);
}

BOOL FSetUtf8WindowText(HWND hwnd, const char *psz) {
    return ControlIO().pfnSetWindowText(hwnd, psz);
}

BOOL FSetNativeWindowTitle(HWND hwnd, const char *psz) {
    return StarsCompatSetWindowTitleUtf8(hwnd, psz);
}
