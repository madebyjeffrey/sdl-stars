/* SetRect and IntersectRect adapted from Free API include/winuser.h.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars C11 adaptation and remaining rectangle operations, October 10, 2026.
 */
#include <windows.h>

BOOL win32::SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
    if (!lprc)
        return 0;
    lprc->left = xLeft;
    lprc->top = yTop;
    lprc->right = xRight;
    lprc->bottom = yBottom;
    return 1;
}

BOOL win32::CopyRect(LPRECT lprcDst, const RECT *lprcSrc) {
    if (!lprcDst || !lprcSrc)
        return 0;
    *lprcDst = *lprcSrc;
    return 1;
}

BOOL win32::EqualRect(const RECT *lprc1, const RECT *lprc2) {
    return lprc1 && lprc2 && lprc1->left == lprc2->left && lprc1->top == lprc2->top &&
           lprc1->right == lprc2->right && lprc1->bottom == lprc2->bottom;
}

BOOL win32::IntersectRect(LPRECT lprcDst, const RECT *lprcSrc1, const RECT *lprcSrc2) {
    RECT rc;

    if (!lprcDst || !lprcSrc1 || !lprcSrc2)
        return 0;
    rc.left = StarsMax(lprcSrc1->left, lprcSrc2->left);
    rc.top = StarsMax(lprcSrc1->top, lprcSrc2->top);
    rc.right = StarsMin(lprcSrc1->right, lprcSrc2->right);
    rc.bottom = StarsMin(lprcSrc1->bottom, lprcSrc2->bottom);
    // Win32 clears empty intersections; upstream leaves inverted coordinates.
    if (rc.right <= rc.left || rc.bottom <= rc.top) {
        win32::SetRect(lprcDst, 0, 0, 0, 0);
        return 0;
    }
    *lprcDst = rc;
    return 1;
}

BOOL win32::OffsetRect(LPRECT lprc, int dx, int dy) {
    if (!lprc)
        return 0;
    lprc->left += dx;
    lprc->right += dx;
    lprc->top += dy;
    lprc->bottom += dy;
    return 1;
}

BOOL win32::InflateRect(LPRECT lprc, int dx, int dy) {
    if (!lprc)
        return 0;
    lprc->left -= dx;
    lprc->right += dx;
    lprc->top -= dy;
    lprc->bottom += dy;
    return 1;
}

BOOL win32::PtInRect(const RECT *lprc, POINT pt) {
    return lprc && pt.x >= lprc->left && pt.x < lprc->right &&
           pt.y >= lprc->top && pt.y < lprc->bottom;
}
