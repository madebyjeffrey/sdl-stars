#ifndef STARS_WINCOMPAT_TYPES_H
#define STARS_WINCOMPAT_TYPES_H

#if defined(UNICODE) || defined(_UNICODE)
#error The Stars compatibility headers require narrow-character UI builds
#endif

#include <cstddef>
#include <cstdint>
#include "windowtypes.h"
#include "ui_values.h"
// Match Win32 LLP64 on Linux LP64; never use native long for LONG/DWORD.

using BOOL = std::int32_t;
using WINBOOL = std::int32_t;
using LONG = std::int32_t;
using DWORD = std::uint32_t;
using ULONG = std::uint32_t;
using UINT = std::uint32_t;
using COLORREF = std::uint32_t;
using INT = std::int32_t;
using WORD = std::uint16_t;
using ATOM = std::uint16_t;
using WCHAR = std::uint16_t;
using SHORT = std::int16_t;
using BYTE = std::uint8_t;
using CHAR = char;
using LPVOID = void*;
using PVOID = void*;
using HANDLE = void*;
using HGDIOBJ = void*;
using LPCVOID = const void*;
using LPSTR = char*;
using LPCSTR = const char*;
using LPCCH = const char*;
using LPWORD = WORD*;
using LPDWORD = DWORD*;
using LPBOOL = BOOL*;
using INT_PTR = std::intptr_t;
using LONG_PTR = std::intptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
using UINT_PTR = std::uintptr_t;
using ULONG_PTR = std::uintptr_t;
using DWORD_PTR = std::uintptr_t;
using WPARAM = std::uintptr_t;
using SIZE_T = std::size_t;
using HFILE = int;

namespace win32 {
struct HINSTANCE__;
struct HBITMAP__;
struct HBRUSH__;
struct HPEN__;
struct HFONT__;
struct HPALETTE__;
struct HRGN__;
struct HICON__;
struct HMENU__;
struct HACCEL__;
struct HRSRC__;
}
using HINSTANCE = win32::HINSTANCE__*;
using HBITMAP = win32::HBITMAP__*;
using HBRUSH = win32::HBRUSH__*;
using HPEN = win32::HPEN__*;
using HFONT = win32::HFONT__*;
using HPALETTE = win32::HPALETTE__*;
using HRGN = win32::HRGN__*;
using HICON = win32::HICON__*;
using HMENU = win32::HMENU__*;
using HACCEL = win32::HACCEL__*;
using HRSRC = win32::HRSRC__*;

using HMODULE = HINSTANCE;
using HCURSOR = HICON;
using HGLOBAL = HANDLE;
using HLOCAL = HANDLE;

using WNDPROC = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);
using DLGPROC = INT_PTR (*)(HWND, UINT, WPARAM, LPARAM);
// The remaining Win16 procedure instances all hold dialog callbacks.
using FARPROC = DLGPROC;
using TIMERPROC = void (*)(HWND, UINT, UINT_PTR, DWORD);
using LPOFNHOOKPROC = UINT_PTR (*)(HWND, UINT, WPARAM, LPARAM);
using LPPRINTHOOKPROC = UINT_PTR (*)(HWND, UINT, WPARAM, LPARAM);
using LPSETUPHOOKPROC = UINT_PTR (*)(HWND, UINT, WPARAM, LPARAM);

using LPRECT = RECT*;
using LPCRECT = const RECT*;

#endif
