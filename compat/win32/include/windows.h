#ifndef STARS_WINCOMPAT_WINDOWS_H
#define STARS_WINCOMPAT_WINDOWS_H

#include "wincompat_types.h"

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
#include "value.h"
#define CreateWindow(c, t, s, x, y, w, h, p, m, i, d) CreateWindowExA(0, c, t, s, x, y, w, h, p, m, i, d)
#define CreateDialog(i, t, p, f) CreateDialogParamA(i, t, p, f, 0)
#define DialogBox(i, t, p, f) DialogBoxParamA(i, t, p, f, 0)
#define CreateDialogParam CreateDialogParamA
#define DialogBoxParam DialogBoxParamA
#define MakeProcInstance(f, i) (f)
#define FreeProcInstance(f) ((void)(f))
#define GetCurrentTime() GetTickCount()
#define Yield() ((void)0)
// The legacy game ignores logoff failure; keep its application exit usable.
BOOL StarsCompatExitWindows(int nExit);
#define ExitWindows(r, c) StarsCompatExitWindows(r)



#endif
