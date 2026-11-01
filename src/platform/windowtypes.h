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
typedef win32::HWND__ *HWND;
typedef win32::HDC__ *HDC;
typedef win32::tagRECT RECT;
#endif
