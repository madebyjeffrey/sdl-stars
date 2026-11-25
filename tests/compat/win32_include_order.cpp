/* Core-first inclusion must retain native.h's RECT and restore caller packing. */
#include "common.h"
#pragma pack(push, 2)
#include <limits>
#include <windows.h>
#include "wincompat_constants.h"
#include "wincompat_constants.h"
static_assert(win32::ERROR_CANCELLED == 1223, "constant declarations survive repeated inclusion");
struct PACKCHECK { char ch; uint32_t dw; };
#pragma pack(pop)
static_assert(offsetof(struct PACKCHECK, dw) == 2, "compat headers restore caller packing");
static_assert(sizeof(RECT) == 16, "core RECT survives UI header inclusion");
static_assert(sizeof(WNDCLASSA) == 72, "Win32 structures ignore caller packing");
static_assert(std::numeric_limits<unsigned char>::digits == 8 && std::numeric_limits<int>::max() == std::numeric_limits<std::int32_t>::max() && std::numeric_limits<unsigned int>::max() == std::numeric_limits<std::uint32_t>::max() && std::numeric_limits<short>::max() == std::numeric_limits<std::int16_t>::max(),
    "native integer limits match the Win32 wrapper");

static INT_PTR  TestDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)hwnd;
    return (INT_PTR)((UINT_PTR)wParam ^ (UINT_PTR)lParam ^ msg);
}

int FTestIncludeOrder(void) {
    DLGPROC lpfnDlgProc = TestDlgProc;
    WPARAM wParam = (WPARAM)std::uint64_t{0x123456789abcdef0};
    LPARAM lParam = (LPARAM)std::uint64_t{0x76543210fedcba98};

    return sizeof(POINT16) == 4 &&
        (UINT_PTR)lpfnDlgProc(NULL, win32::WM_USER, wParam, lParam) ==
            ((UINT_PTR)wParam ^ (UINT_PTR)lParam ^ win32::WM_USER);
}
