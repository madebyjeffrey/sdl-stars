#ifndef STARS_WINCOMPAT_TYPES_H
#define STARS_WINCOMPAT_TYPES_H

#if defined(UNICODE) || defined(_UNICODE)
#error The Stars compatibility headers require narrow-character UI builds
#endif

#include <stddef.h>
#include <stdint.h>
#include "windowtypes.h"
#include "ui_values.h"
// Match Win32 LLP64 on Linux LP64; never use native long for LONG/DWORD.

typedef int32_t BOOL, WINBOOL, LONG;
typedef uint32_t DWORD, ULONG, UINT, COLORREF;
typedef int32_t INT;
typedef uint16_t WORD, ATOM, WCHAR;
typedef int16_t SHORT;
typedef uint8_t BYTE;
typedef char CHAR;
typedef void *LPVOID, *PVOID, *HANDLE, *HGDIOBJ;
typedef const void *LPCVOID;
typedef char *LPSTR;
typedef const char *LPCSTR, *LPCCH;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD;
typedef BOOL *LPBOOL;
typedef intptr_t INT_PTR, LONG_PTR, LPARAM, LRESULT;
typedef uintptr_t UINT_PTR, ULONG_PTR, DWORD_PTR, WPARAM;
typedef size_t SIZE_T;
typedef int HFILE;

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
typedef win32::HINSTANCE__ *HINSTANCE;
typedef win32::HBITMAP__ *HBITMAP;
typedef win32::HBRUSH__ *HBRUSH;
typedef win32::HPEN__ *HPEN;
typedef win32::HFONT__ *HFONT;
typedef win32::HPALETTE__ *HPALETTE;
typedef win32::HRGN__ *HRGN;
typedef win32::HICON__ *HICON;
typedef win32::HMENU__ *HMENU;
typedef win32::HACCEL__ *HACCEL;
typedef win32::HRSRC__ *HRSRC;

typedef HINSTANCE HMODULE;
typedef HICON HCURSOR;
typedef HANDLE HGLOBAL, HLOCAL;

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
// The remaining Win16 procedure instances all hold dialog callbacks.
typedef DLGPROC FARPROC;
typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef UINT_PTR (*LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (*LPPRINTHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (*LPSETUPHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

typedef RECT *LPRECT;
typedef const RECT *LPCRECT;

#endif
