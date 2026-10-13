#ifndef STARS_DECOMPILED_NATIVEUI_H
#define STARS_DECOMPILED_NATIVEUI_H

// The Windows interface's native port helpers. See docs/NATIVE-PORT.md.

#include <stdint.h>
#include <windows.h>

// PointFrom16 widens a Stars point to a Win32 point.
static inline POINT PointFrom16(POINT16 pt) {
    POINT out = {pt.x, pt.y};
    return out;
}

// PointTo16 narrows a Win32 point to a Stars point.
static inline POINT16 PointTo16(POINT pt) {
    POINT16 out = {(int16_t)pt.x, (int16_t)pt.y};
    return out;
}

// GetCursorPos16 stores the cursor position in a Stars point.
static inline BOOL GetCursorPos16(POINT16 *ppt) {
    // Callers still use the point when a backend cannot report its position.
    POINT pt = {0, 0};
    BOOL  ret = GetCursorPos(&pt);
    *ppt = PointTo16(pt);
    return ret;
}

// ScreenToClient16 converts a Stars point from screen to client coordinates.
static inline BOOL ScreenToClient16(HWND hwnd, POINT16 *ppt) {
    POINT pt = PointFrom16(*ppt);
    BOOL  ret = ScreenToClient(hwnd, &pt);
    *ppt = PointTo16(pt);
    return ret;
}

// ClientToScreen16 converts a Stars point from client to screen coordinates.
static inline BOOL ClientToScreen16(HWND hwnd, POINT16 *ppt) {
    POINT pt = PointFrom16(*ppt);
    BOOL  ret = ClientToScreen(hwnd, &pt);
    *ppt = PointTo16(pt);
    return ret;
}

// MapWindowPoints16 maps c Stars points from one window's coordinates to
// another's.
static inline int MapWindowPoints16(HWND hwndFrom, HWND hwndTo, POINT16 *ppt, UINT c) {
    int ret = 0;
    for (UINT i = 0; i < c; i++) {
        POINT pt = PointFrom16(ppt[i]);
        ret = MapWindowPoints(hwndFrom, hwndTo, &pt, 1);
        ppt[i] = PointTo16(pt);
    }
    return ret;
}

// GetTextExtent measures text like Win16 GetTextExtent, which Win32 dropped:
// the width in the low word and the height in the high word.
DWORD GetTextExtent(HDC hdc, LPCSTR str, int len);

// FrameWndProcDeferred is the frame window class's procedure; it wraps
// FrameWndProc (see nativeui.c).
LRESULT CALLBACK FrameWndProcDeferred(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// FRouteMouseWheel sends wheel messages to the window under the cursor,
// CWheelNotches gathers wheel deltas into notches and CWheelLines gives the
// lines a notch scrolls (see nativeui.c).
int16_t FRouteMouseWheel(MSG *pmsg);
int16_t CWheelNotches(int16_t *pdWheel, int16_t dWheel);
int16_t CWheelLines(int16_t cPage);

#endif
