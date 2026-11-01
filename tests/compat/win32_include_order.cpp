/* Core-first inclusion must retain native.h's RECT and restore caller packing. */
#include "common.h"
#pragma pack(push, 2)
#include <windows.h>
#include "wincompat_constants.h"
#include "wincompat_constants.h"
static_assert(win32::ERROR_CANCELLED == 1223, "constant declarations survive repeated inclusion");
struct PACKCHECK { char ch; uint32_t dw; };
#pragma pack(pop)
static_assert(offsetof(struct PACKCHECK, dw) == 2, "compat headers restore caller packing");
static_assert(sizeof(RECT) == 16, "core RECT survives UI header inclusion");
static_assert(sizeof(WNDCLASSA) == 72, "Win32 structures ignore caller packing");
static_assert(CHAR_BIT == 8 && INT_MAX == INT32_MAX && UINT_MAX == UINT32_MAX && SHRT_MAX == INT16_MAX,
    "native integer limits match the Win32 wrapper");

static INT_PTR  TestDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)hwnd;
    return (INT_PTR)((UINT_PTR)wParam ^ (UINT_PTR)lParam ^ msg);
}

int FTestIncludeOrder(void) {
    FARPROC lpfnDlgProc = TestDlgProc;
    WPARAM wParam = (WPARAM)UINT64_C(0x123456789abcdef0);
    LPARAM lParam = (LPARAM)UINT64_C(0x76543210fedcba98);

    return sizeof(POINT16) == 4 &&
        (UINT_PTR)lpfnDlgProc(NULL, win32::WM_USER, wParam, lParam) ==
            ((UINT_PTR)wParam ^ (UINT_PTR)lParam ^ win32::WM_USER);
}
