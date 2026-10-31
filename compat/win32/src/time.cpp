/* GetTickCount's 32-bit monotonic millisecond conversion adapted from
 * Free API src/winbase.cpp. Copyright (c) 2026 Robert Vokac.
 * MIT license: ../LICENSE.free-api. Stars C11/SDL3 adaptation, October 10, 2026.
 */
#include <windows.h>
#include <SDL3/SDL.h>
#include "time_io.h"

DWORD WINAPI win32::GetTickCount(void) {
    // SDL owns the monotonic clock; truncation preserves Win32's 49.7-day wrap.
    return (DWORD)CompatTimeIO().pfnGetTicks();
}
