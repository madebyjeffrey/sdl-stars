/* The same assertions compile against MinGW and the native compatibility headers. */
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "win32_constant_values.h"

#define SIZE(type, cb) _Static_assert(sizeof(type) == (cb), #type " size")
#define OFFSET(type, field, ib) _Static_assert(offsetof(type, field) == (ib), #type "." #field " offset")
SIZE(BOOL, 4); SIZE(LONG, 4); SIZE(DWORD, 4); SIZE(WORD, 2); SIZE(WCHAR, 2);
SIZE(WPARAM, sizeof(void *)); SIZE(LPARAM, sizeof(void *)); SIZE(LRESULT, sizeof(void *));
SIZE(POINT, 8); SIZE(RECT, 16); SIZE(SIZE, 8); SIZE(POINT16, 4);
SIZE(BITMAPFILEHEADER, 14); OFFSET(BITMAPFILEHEADER, bfSize, 2); OFFSET(BITMAPFILEHEADER, bfOffBits, 10);
SIZE(BITMAPINFOHEADER, 40); SIZE(BITMAPCOREHEADER, 12); SIZE(RGBQUAD, 4); SIZE(RGBTRIPLE, 3);
SIZE(PALETTEENTRY, 4); SIZE(LOGPALETTE, 8); SIZE(BITMAPINFO, 44); SIZE(BITMAPCOREINFO, 16);
SIZE(LOGFONTA, 60); OFFSET(LOGFONTA, lfFaceName, 28);
SIZE(TEXTMETRICA, 56); OFFSET(TEXTMETRICA, tmFirstChar, 44);
SIZE(SCROLLINFO, 28); OFFSET(SCROLLINFO, nTrackPos, 24);
_Static_assert(SIF_ALL == 0x17 && SIF_DISABLENOSCROLL == 8 && SB_BOTH == 3, "scroll masks");
SIZE(ACCEL, 6); OFFSET(ACCEL, key, 2); OFFSET(ACCEL, cmd, 4);
SIZE(WINDOWPLACEMENT, 44); SIZE(MINMAXINFO, 40); SIZE(OFSTRUCT, 136); SIZE(DEVNAMES, 8);
_Static_assert(WM_USER == 0x400 && WM_COMMAND == 0x111, "message numbers");
_Static_assert(WS_POPUP == UINT32_C(0x80000000), "unsigned style bit");
_Static_assert(CB_ERR == -1 && LB_ERR == -1, "signed failure values");
_Static_assert(RGB(0x12, 0x34, 0x56) == 0x563412, "COLORREF byte order");
_Static_assert(_Generic((WNDPROC)0, LRESULT (CALLBACK *)(HWND, UINT, WPARAM, LPARAM): 1, default: 0), "WNDPROC signature");
_Static_assert(_Generic((DLGPROC)0, INT_PTR (CALLBACK *)(HWND, UINT, WPARAM, LPARAM): 1, default: 0), "DLGPROC signature");
_Static_assert(_Generic(&GetWindowLongPtrA, LONG_PTR (WINAPI *)(HWND, int): 1, default: 0), "pointer accessor signature");

#if UINTPTR_MAX == UINT64_MAX
SIZE(ICONINFO, 32); OFFSET(ICONINFO, xHotspot, 4); OFFSET(ICONINFO, hbmMask, 16);
SIZE(WINDOWPOS, 40); OFFSET(WINDOWPOS, x, 16); OFFSET(WINDOWPOS, flags, 32);
SIZE(WNDCLASSA, 72); OFFSET(WNDCLASSA, lpfnWndProc, 8); OFFSET(WNDCLASSA, lpszClassName, 64);
SIZE(MSG, 48); OFFSET(MSG, wParam, 16); OFFSET(MSG, lParam, 24); OFFSET(MSG, pt, 36);
SIZE(PAINTSTRUCT, 72); OFFSET(PAINTSTRUCT, rcPaint, 12);
SIZE(CREATESTRUCTA, 80); OFFSET(CREATESTRUCTA, style, 48); OFFSET(CREATESTRUCTA, lpszName, 56);
SIZE(DRAWITEMSTRUCT, 64); OFFSET(DRAWITEMSTRUCT, itemData, 56);
SIZE(MEASUREITEMSTRUCT, 32); OFFSET(MEASUREITEMSTRUCT, itemData, 24);
SIZE(DELETEITEMSTRUCT, 32); OFFSET(DELETEITEMSTRUCT, itemData, 24);
SIZE(BITMAP, 32); OFFSET(BITMAP, bmBits, 24);
SIZE(OPENFILENAMEA, 152); OFFSET(OPENFILENAMEA, Flags, 96); OFFSET(OPENFILENAMEA, lpfnHook, 120);
SIZE(PRINTDLGA, 120); OFFSET(PRINTDLGA, hDC, 32); OFFSET(PRINTDLGA, lpfnPrintHook, 72);
SIZE(DEVMODEA, 156); OFFSET(DEVMODEA, dmFields, 40); OFFSET(DEVMODEA, dmFormName, 70);
#else
#error The initial compatibility ABI is validated for 64-bit builds only
#endif

static LRESULT CALLBACK TestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)hwnd;
    (void)msg;
    return (LRESULT)((UINT_PTR)wParam ^ (UINT_PTR)lParam);
}

#ifndef _WIN32
int FTestIncludeOrder(void);
#endif

int main(void) {
    WNDPROC lpfnWndProc = TestWndProc;
    WPARAM wParam = (WPARAM)UINT64_C(0x123456789abcdef0);
    LPARAM lParam = (LPARAM)UINT64_C(0x76543210fedcba98);

    if ((UINT_PTR)lpfnWndProc(NULL, WM_USER, wParam, lParam) != ((UINT_PTR)wParam ^ (UINT_PTR)lParam))
        return 1;
    if (LOWORD(wParam) != 0xdef0 || HIWORD(wParam) != 0x9abc ||
        (UINT_PTR)MAKEINTRESOURCE(0xffff) != 0xffff ||
        MAKEWPARAM(0x1234, 0xabcd) != (WPARAM)UINT32_C(0xabcd1234) ||
        MAKELONG(0xffff, 0xffff) != -1 ||
        (int16_t)LOWORD(MAKELPARAM(-12, -34)) != -12 ||
        (int16_t)HIWORD(MAKELPARAM(-12, -34)) != -34)
        return 2;
#ifndef _WIN32
    if (!FTestIncludeOrder())
        return 3;
#endif
    puts("Win32 source ABI: sizes, offsets, constants, callbacks and pointer round trips passed");
    return 0;
}
