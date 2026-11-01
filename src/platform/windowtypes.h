#ifndef STARS_WINDOW_TYPES_H
#define STARS_WINDOW_TYPES_H
#include <stdint.h>

// Game/UI records borrow SDL runtime handles, never native SDK handles.
namespace win32 {
struct HWND__;
struct HDC__;
struct tagRECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};
} // namespace win32
using HWND = win32::HWND__*;
using HDC = win32::HDC__*;
using RECT = win32::tagRECT;
#endif
