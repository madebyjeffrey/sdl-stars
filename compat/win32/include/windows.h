#ifndef STARS_WINCOMPAT_WINDOWS_H
#define STARS_WINCOMPAT_WINDOWS_H

#if defined(_WIN32) || defined(UNICODE) || defined(_UNICODE)
#error The Stars compatibility headers are for native Linux narrow-character UI builds only
#endif

#include <stddef.h>
#include <stdint.h>

// Match Win32 LLP64 on Linux LP64; never use native long for LONG/DWORD.
#define _WINDEF_
#define WINAPI
#define CALLBACK
#define APIENTRY WINAPI
#define WINAPIV
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

#define STARS_DECLARE_HANDLE(name) typedef struct name##__ *name
STARS_DECLARE_HANDLE(HWND);
STARS_DECLARE_HANDLE(HDC);
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
typedef INT_PTR (WINAPI *FARPROC)();
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef UINT_PTR (CALLBACK *LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK *LPPRINTHOOKPROC)(HWND, UINT, WPARAM, LPARAM);
typedef UINT_PTR (CALLBACK *LPSETUPHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

// common.h can precede windows.h in ABI tests and portable consumers.
#ifndef STARS_NATIVE_H
typedef struct tagRECT { LONG left, top, right, bottom; } RECT;
#endif
typedef RECT *LPRECT;
typedef const RECT *LPCRECT;

#include "wincompat_structs.h"
#include "wincompat_constants.h"
#include "wincompat_api.h"

#ifndef LOWORD
#define LOWORD(l) ((WORD)((uintptr_t)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((uintptr_t)(l) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)((uintptr_t)(w) & 0xff))
#endif
#define HIBYTE(w) ((BYTE)(((uintptr_t)(w) >> 8) & 0xff))
#ifndef MAKELONG
#define MAKELONG(l, h) ((LONG)((DWORD)(WORD)(l) | ((DWORD)(WORD)(h) << 16)))
#endif
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEINTRESOURCE(i) ((LPSTR)(uintptr_t)(WORD)(i))
#define RGB(r, g, b) ((COLORREF)((BYTE)(r) | ((DWORD)(BYTE)(g) << 8) | ((DWORD)(BYTE)(b) << 16)))
#define PALETTEINDEX(i) ((COLORREF)(UINT32_C(0x01000000) | (WORD)(i)))
#define PALETTERGB(r, g, b) (RGB(r, g, b) | UINT32_C(0x02000000))
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define CreateWindow(c, t, s, x, y, w, h, p, m, i, d) CreateWindowExA(0, c, t, s, x, y, w, h, p, m, i, d)
#define CreateDialog(i, t, p, f) CreateDialogParamA(i, t, p, f, 0)
#define DialogBox(i, t, p, f) DialogBoxParamA(i, t, p, f, 0)
#define CreateDialogParam CreateDialogParamA
#define DialogBoxParam DialogBoxParamA
#define MakeProcInstance(f, i) (f)
#define FreeProcInstance(f) (f)
#define GetCurrentTime() GetTickCount()
#define Yield() ((void)0)
// The legacy game ignores logoff failure; keep its application exit usable.
BOOL StarsCompatExitWindows(int nExit);
#define ExitWindows(r, c) StarsCompatExitWindows(r)

#endif
