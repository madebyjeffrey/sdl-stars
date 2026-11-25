/* The same assertions compile against MinGW and the native compatibility headers. */
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#ifdef STARS_UI_SDL
#include <type_traits>
#include "common.h"
#else
#include "../reference/cpp-a/common.h"
#endif
#include "win32_constant_values.h"

#define SIZE(type, cb) static_assert(sizeof(type) == (cb), #type " size")
#define OFFSET(type, field, ib) static_assert(offsetof(type, field) == (ib), #type "." #field " offset")
SIZE(BOOL, 4); SIZE(LONG, 4); SIZE(DWORD, 4); SIZE(WORD, 2); SIZE(WCHAR, 2);
SIZE(WPARAM, sizeof(void *)); SIZE(LPARAM, sizeof(void *)); SIZE(LRESULT, sizeof(void *));
SIZE(POINT, 8); SIZE(RECT, 16); SIZE(SIZE, 8); SIZE(POINT16, 4);
SIZE(BITMAPFILEHEADER, 14); OFFSET(BITMAPFILEHEADER, bfSize, 2); OFFSET(BITMAPFILEHEADER, bfOffBits, 10);
SIZE(BITMAPINFOHEADER, 40); SIZE(BITMAPCOREHEADER, 12); SIZE(RGBQUAD, 4); SIZE(RGBTRIPLE, 3);
SIZE(PALETTEENTRY, 4); SIZE(LOGPALETTE, 8); SIZE(BITMAPINFO, 44); SIZE(BITMAPCOREINFO, 16);
SIZE(LOGFONTA, 60); OFFSET(LOGFONTA, lfFaceName, 28);
SIZE(TEXTMETRICA, 56); OFFSET(TEXTMETRICA, tmFirstChar, 44);
SIZE(SCROLLINFO, 28); OFFSET(SCROLLINFO, nTrackPos, 24);
static_assert(SIF_ALL == 0x17 && SIF_DISABLENOSCROLL == 8 && SB_BOTH == 3, "scroll masks");
SIZE(ACCEL, 6); OFFSET(ACCEL, key, 2); OFFSET(ACCEL, cmd, 4);
SIZE(WINDOWPLACEMENT, 44); SIZE(MINMAXINFO, 40); SIZE(OFSTRUCT, 136); SIZE(DEVNAMES, 8);
static_assert(WM_USER == 0x400 && WM_COMMAND == 0x111, "message numbers");
static_assert(WS_POPUP == UINT32_C(0x80000000), "unsigned style bit");
static_assert(CB_ERR == -1 && LB_ERR == -1, "signed failure values");
static_assert(RGB(0x12, 0x34, 0x56) == 0x563412, "COLORREF byte order");
#ifdef STARS_UI_SDL
enum class WordProbe : uint32_t { value = 0xabcd1234 };
static_assert(LOWORD(WordProbe::value) == 0x1234 && HIWORD(WordProbe::value) == 0xabcd);
static_assert(std::is_same_v<decltype(LOWORD(0)), WORD> && std::is_same_v<decltype(HIBYTE(0)), BYTE>);
static_assert(std::is_same_v<decltype(MAKELONG(0, 0)), LONG>);
static_assert(std::is_same_v<decltype(MAKEWPARAM(0, 0)), WPARAM>);
static_assert(std::is_same_v<decltype(MAKELPARAM(0, 0)), LPARAM>);
static_assert(std::is_same_v<decltype(RGB(0, 0, 0)), COLORREF>);
static_assert(std::is_same_v<WNDPROC, LRESULT (CALLBACK *)(HWND, UINT, WPARAM, LPARAM)>, "WNDPROC signature");
static_assert(std::is_same_v<DLGPROC, INT_PTR (CALLBACK *)(HWND, UINT, WPARAM, LPARAM)>, "DLGPROC signature");
static_assert(std::is_same_v<decltype(&GetWindowLongPtrA), LONG_PTR (*)(HWND, int)>, "pointer accessor signature");
#else
static_assert(_Generic((WNDPROC)0, LRESULT (CALLBACK *)(HWND, UINT, WPARAM, LPARAM): 1, default: 0), "WNDPROC signature");
static_assert(_Generic((DLGPROC)0, INT_PTR (CALLBACK *)(HWND, UINT, WPARAM, LPARAM): 1, default: 0), "DLGPROC signature");
static_assert(_Generic(&GetWindowLongPtrA, LONG_PTR (WINAPI *)(HWND, int): 1, default: 0), "pointer accessor signature");
#endif

#if UINTPTR_MAX == UINT64_MAX
SIZE(ICONINFO, 32); OFFSET(ICONINFO, xHotspot, 4); OFFSET(ICONINFO, hbmMask, 16);
SIZE(WINDOWPOS, 40); OFFSET(WINDOWPOS, x, 16); OFFSET(WINDOWPOS, flags, 32);
SIZE(WNDCLASSA, 72); OFFSET(WNDCLASSA, lpfnWndProc, 8); OFFSET(WNDCLASSA, lpszClassName, 64);
SIZE(APPMSG, 48); OFFSET(APPMSG, wParam, 16); OFFSET(APPMSG, lParam, 24); OFFSET(APPMSG, pt, 36);
SIZE(PAINTSTRUCT, 72); OFFSET(PAINTSTRUCT, rcPaint, 12);
SIZE(CREATESTRUCTA, 80); OFFSET(CREATESTRUCTA, style, 48); OFFSET(CREATESTRUCTA, lpszName, 56);
SIZE(DRAWITEMSTRUCT, 64); OFFSET(DRAWITEMSTRUCT, itemData, 56);
SIZE(MEASUREITEMSTRUCT, 32); OFFSET(MEASUREITEMSTRUCT, itemData, 24);
SIZE(DELETEITEMSTRUCT, 32); OFFSET(DELETEITEMSTRUCT, itemData, 24);
SIZE(BITMAP, 32); OFFSET(BITMAP, bmBits, 24);
SIZE(OPENFILENAMEA, 152); OFFSET(OPENFILENAMEA, Flags, 96); OFFSET(OPENFILENAMEA, lpfnHook, 120);
SIZE(PRINTDLGA, 120); OFFSET(PRINTDLGA, hDC, 32); OFFSET(PRINTDLGA, lpfnPrintHook, 72);
SIZE(DEVMODEA, 156); OFFSET(DEVMODEA, dmFields, 40); OFFSET(DEVMODEA, dmFormName, 70);
#ifdef STARS_UI_SDL
OFFSET(DEVMODEA, mode.printer.dmOrientation, 44); OFFSET(DEVMODEA, mode.display.dmPosition, 44);
OFFSET(DEVMODEA, displayFlags.dmDisplayFlags, 116); OFFSET(DEVMODEA, displayFlags.dmNup, 116);
#else
OFFSET(DEVMODEA, dmOrientation, 44); OFFSET(DEVMODEA, dmPosition, 44);
OFFSET(DEVMODEA, dmDisplayFlags, 116); OFFSET(DEVMODEA, dmNup, 116);
#endif
#else
#error The initial compatibility ABI is validated for 64-bit builds only
#endif

static LRESULT CALLBACK TestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)hwnd;
    (void)msg;
    return (LRESULT)((UINT_PTR)wParam ^ (UINT_PTR)lParam);
}

#if !defined(_WIN32) || defined(STARS_UI_SDL)
int FTestIncludeOrder(void);
#endif

int main(void) {
    struct { unsigned lo : 16; unsigned hi : 16; } bits = {0x1234, 0xabcd};
    unsigned cLo = 0, cHi = 0, cR = 0, cG = 0, cB = 0;
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
    if ((DWORD)MAKELONG(bits.lo, bits.hi) != UINT32_C(0xabcd1234) ||
        (UINT_PTR)MAKELPARAM(0xffff, 0xffff) != (UINT_PTR)UINT32_C(0xffffffff) ||
        (UINT_PTR)MAKEWPARAM(0xffff, 0xffff) != (UINT_PTR)UINT32_C(0xffffffff) ||
        LOBYTE(UINT32_C(0x12345678)) != 0x78 || HIBYTE(UINT32_C(0x12345678)) != 0x56 ||
        GET_KEYSTATE_WPARAM(UINT32_C(0xfffe1234)) != 0x1234 ||
        GET_WHEEL_DELTA_WPARAM(UINT32_C(0xfffe1234)) != -2 ||
        PALETTEINDEX(0x12345) != UINT32_C(0x01002345) ||
        PALETTERGB(0x123, 0x234, 0x345) != UINT32_C(0x02453423) ||
        (UINT_PTR)MAKEINTRESOURCE(-1) != 0xffff ||
        (DWORD)MAKELONG(++cLo, ++cHi) != UINT32_C(0x00010001) || cLo != 1 || cHi != 1 ||
        RGB(++cR, ++cG, ++cB) != UINT32_C(0x010101) || cR != 1 || cG != 1 || cB != 1)
        return 4;
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    if (!FTestIncludeOrder())
        return 3;
#endif
    puts("Win32 source ABI: sizes, offsets, constants, callbacks and pointer round trips passed");
    return 0;
}
