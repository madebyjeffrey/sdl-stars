/* Core-first inclusion must retain native.h's RECT and restore caller packing. */
#include "common.h"
#pragma pack(push, 2)
#include <windows.h>
#include "wincompat_constants.h"
#undef ERROR_CANCELLED
#include "wincompat_constants.h"
#ifdef ERROR_CANCELLED
#error Constant declarations must remain inside the include guard
#endif
struct PACKCHECK { char ch; uint32_t dw; };
#pragma pack(pop)
_Static_assert(offsetof(struct PACKCHECK, dw) == 2, "compat headers restore caller packing");
_Static_assert(sizeof(RECT) == 16, "core RECT survives UI header inclusion");
_Static_assert(sizeof(WNDCLASSA) == 72, "Win32 structures ignore caller packing");
int FTestIncludeOrder(void) { return sizeof(POINT16) == 4; }
