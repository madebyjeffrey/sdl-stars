#include <SDL3/SDL.h>
#pragma pack(push, 2)
#include <windows.h>
struct FORMATPACKCHECK { char ch; int value; };
static_assert(offsetof(FORMATPACKCHECK, value) == 2, "wrapper headers restore caller packing");
#pragma pack(pop)
#include <wincompat_calls.h>
#include <type_traits>

static_assert(alignof(NATIVEFORMATARG) >= alignof(long double), "typed format arguments retain native alignment");
static_assert(alignof(std::span<char>) >= alignof(char *), "format argument spans retain native alignment");

#ifdef _WIN32
static_assert(!std::is_same_v<win32::MSG, ::MSG>);
#endif
static_assert(std::is_same_v<decltype(&win32::GetWindowLongPtrA), LONG_PTR (*)(HWND, int)>);

static_assert(std::is_same_v<HWND, win32::HWND__ *>);
static_assert(std::is_same_v<HDC, win32::HDC__ *>);
static_assert(std::is_same_v<HINSTANCE, win32::HINSTANCE__ *>);
static_assert(std::is_same_v<HBITMAP, win32::HBITMAP__ *>);
static_assert(std::is_same_v<HBRUSH, win32::HBRUSH__ *>);
static_assert(std::is_same_v<HPEN, win32::HPEN__ *>);
static_assert(std::is_same_v<HFONT, win32::HFONT__ *>);
static_assert(std::is_same_v<HPALETTE, win32::HPALETTE__ *>);
static_assert(std::is_same_v<HRGN, win32::HRGN__ *>);
static_assert(std::is_same_v<HICON, win32::HICON__ *>);
static_assert(std::is_same_v<HMENU, win32::HMENU__ *>);
static_assert(std::is_same_v<HACCEL, win32::HACCEL__ *>);
static_assert(std::is_same_v<HRSRC, win32::HRSRC__ *>);
static_assert(std::is_same_v<RECT, win32::tagRECT>);
static_assert(std::is_same_v<POINT, win32::tagPOINT>);
static_assert(std::is_same_v<win32::MSG, win32::tagMSG>);
static_assert(std::is_same_v<WNDCLASSA, win32::tagWNDCLASSA>);
static_assert(std::is_same_v<OPENFILENAMEA, win32::tagOFNA>);
static_assert(std::is_same_v<PRINTDLGA, win32::tagPDA>);

int main() { return 0; }
