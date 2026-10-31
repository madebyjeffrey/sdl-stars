#ifndef STARS_WINCOMPAT_TYPES_H
#define STARS_WINCOMPAT_TYPES_H

#if defined(UNICODE) || defined(_UNICODE)
#error The Stars compatibility headers require narrow-character UI builds
#endif

#include <stddef.h>
#include <stdint.h>
#include "windowtypes.h"
// Match Win32 LLP64 on Linux LP64; never use native long for LONG/DWORD.
#define _WINDEF_
#define CALLBACK
#define CONST const
#define TRUE 1
#define FALSE 0
#define EWX_LOGOFF 0

typedef int32_t BOOL, WINBOOL, LONG;
typedef uint32_t DWORD, ULONG, UINT, COLORREF;
typedef int32_t INT;
typedef uint16_t WORD, ATOM, WCHAR;
typedef int16_t SHORT;
typedef uint8_t BYTE;
typedef char CHAR;
typedef void VOID;
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

#define STARS_DECLARE_HANDLE(name) namespace win32 { struct name##__; } typedef win32::name##__ *name
STARS_DECLARE_HANDLE(HINSTANCE);
STARS_DECLARE_HANDLE(HBITMAP);
STARS_DECLARE_HANDLE(HBRUSH);
STARS_DECLARE_HANDLE(HPEN);
STARS_DECLARE_HANDLE(HFONT);
STARS_DECLARE_HANDLE(HPALETTE);
STARS_DECLARE_HANDLE(HRGN);
STARS_DECLARE_HANDLE(HICON);
STARS_DECLARE_HANDLE(HMENU);
STARS_DECLARE_HANDLE(HACCEL);
STARS_DECLARE_HANDLE(HRSRC);
#undef STARS_DECLARE_HANDLE
typedef HINSTANCE HMODULE;
typedef HICON HCURSOR;
typedef HANDLE HGLOBAL, HLOCAL;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
// The remaining Win16 procedure instances all hold dialog callbacks.
typedef DLGPROC FARPROC;
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef UINT_PTR (CALLBACK *LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK *LPPRINTHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK *LPSETUPHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

typedef RECT *LPRECT;
typedef const RECT *LPCRECT;

#endif
