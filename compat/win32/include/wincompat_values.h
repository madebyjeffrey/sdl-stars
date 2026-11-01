#ifndef STARS_WINCOMPAT_VALUES_H
#define STARS_WINCOMPAT_VALUES_H
#include "wincompat_types.h"
#include "value.h"

namespace win32 {
template<class A, class B> constexpr WPARAM MAKEWPARAM(A lo, B hi) {
    return static_cast<WPARAM>(static_cast<DWORD>(MAKELONG(lo, hi)));
}
template<class A, class B> constexpr LPARAM MAKELPARAM(A lo, B hi) {
    return static_cast<LPARAM>(static_cast<DWORD>(MAKELONG(lo, hi)));
}
template<class T> inline LPSTR MAKEINTRESOURCE(T id) {
    return reinterpret_cast<LPSTR>(static_cast<std::uintptr_t>(static_cast<WORD>(id)));
}
template<class R, class G, class B> constexpr COLORREF RGB(R r, G g, B b) {
    return static_cast<COLORREF>(static_cast<BYTE>(r) |
                                 (static_cast<DWORD>(static_cast<BYTE>(g)) << 8) |
                                 (static_cast<DWORD>(static_cast<BYTE>(b)) << 16));
}
template<class T> constexpr COLORREF PALETTEINDEX(T id) {
    return static_cast<COLORREF>(UINT32_C(0x01000000) | static_cast<WORD>(id));
}
template<class R, class G, class B> constexpr COLORREF PALETTERGB(R r, G g, B b) {
    return RGB(r, g, b) | UINT32_C(0x02000000);
}
template<class T> constexpr WORD GET_KEYSTATE_WPARAM(T wParam) { return LOWORD(wParam); }
template<class T> constexpr short GET_WHEEL_DELTA_WPARAM(T wParam) { return static_cast<short>(HIWORD(wParam)); }
} // namespace win32
#endif
