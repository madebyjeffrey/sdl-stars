/* Windows declarations adapted from mingw-w64; see ../LICENSE.
 * Modified for the Stars Linux source compatibility layer, October 10, 2026.
 */
#ifndef STARS_WINCOMPAT_API_H
#define STARS_WINCOMPAT_API_H

#include "wincompat_types.h"
#include "wincompat_structs.h"

namespace win32 {
WINBOOL AppendMenuA(HMENU hMenu,UINT uFlags,UINT_PTR uIDNewItem,LPCSTR lpNewItem);
HDC BeginPaint(HWND hWnd,LPPAINTSTRUCT lpPaint);
WINBOOL BitBlt(HDC hdc,int x,int y,int cx,int cy,HDC hdcSrc,int x1,int y1,DWORD rop);
LRESULT CallWindowProcA (WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINBOOL CheckDlgButton(HWND hDlg,int nIDButton,UINT uCheck);
DWORD CheckMenuItem(HMENU hMenu,UINT uIDCheckItem,UINT uCheck);
WINBOOL CheckRadioButton(HWND hDlg,int nIDFirstButton,int nIDLastButton,int nIDCheckButton);
WINBOOL ClientToScreen(HWND hWnd,LPPOINT lpPoint);
WINBOOL CopyRect(LPRECT lprcDst,CONST RECT *lprcSrc);
HBITMAP CreateCompatibleBitmap(HDC hdc,int cx,int cy);
HDC CreateCompatibleDC(HDC hdc);
HWND CreateDialogParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
HFONT CreateFontIndirectA(CONST LOGFONTA *lplf);
HPALETTE CreatePalette(CONST LOGPALETTE *plpal);
HBRUSH CreatePatternBrush(HBITMAP hbm);
HPEN CreatePen(int iStyle,int cWidth,COLORREF color);
HMENU CreatePopupMenu(VOID);
HRGN CreateRectRgn(int x1,int y1,int x2,int y2);
HBRUSH CreateSolidBrush(COLORREF color);
HWND CreateWindowExA(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
LRESULT DefWindowProcA (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINBOOL DeleteDC(HDC hdc);
WINBOOL DeleteMenu(HMENU hMenu,UINT uPosition,UINT uFlags);
WINBOOL DeleteObject(HGDIOBJ ho);
WINBOOL DestroyCursor(HCURSOR hCursor);
WINBOOL DestroyIcon(HICON hIcon);
WINBOOL DestroyMenu(HMENU hMenu);
WINBOOL DestroyWindow(HWND hWnd);
INT_PTR DialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
LRESULT DispatchMessageA(CONST win32::MSG *lpMsg);
WINBOOL DrawIcon(HDC hDC,int X,int Y,HICON hIcon);
WINBOOL DrawMenuBar(HWND hWnd);
int DrawTextA(HDC hdc,LPCSTR lpchText,int cchText,LPRECT lprc,UINT format);
WINBOOL Ellipse(HDC hdc,int left,int top,int right,int bottom);
WINBOOL EnableMenuItem(HMENU hMenu,UINT uIDEnableItem,UINT uEnable);
WINBOOL EnableWindow(HWND hWnd,WINBOOL bEnable);
WINBOOL EndDialog(HWND hDlg,INT_PTR nResult);
WINBOOL EndPaint(HWND hWnd,CONST PAINTSTRUCT *lpPaint);
WINBOOL EqualRect(CONST RECT *lprc1,CONST RECT *lprc2);
int Escape(HDC hdc,int iEscape,int cjIn,LPCSTR pvIn,LPVOID pvOut);
int ExcludeClipRect(HDC hdc,int left,int top,int right,int bottom);
WINBOOL ExitWindowsEx(UINT uFlags,DWORD dwReason);
WINBOOL ExtTextOutA(HDC hdc,int x,int y,UINT options,CONST RECT *lprect,LPCSTR lpString,UINT c,CONST INT *lpDx);
int FillRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr);
HRSRC FindResourceA (HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
WINBOOL FlashWindow (HWND hWnd, WINBOOL bInvert);
int FrameRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr);
WINBOOL FreeResource (HGLOBAL hResData);
HWND GetActiveWindow(VOID);
SHORT GetAsyncKeyState(int vKey);
COLORREF GetBkColor(HDC hdc);
int GetClassNameA(HWND hWnd,LPSTR lpClassName,int nMaxCount);
WINBOOL GetClientRect(HWND hWnd,LPRECT lpRect);
DWORD GetCurrentThreadId (VOID);
WINBOOL GetCursorPos(LPPOINT lpPoint);
HDC GetDC(HWND hWnd);
COLORREF GetPixel(HDC hdc,int x,int y);
int GetDIBits(HDC hdc,HBITMAP hbm,UINT start,UINT cLines,LPVOID lpvBits,LPBITMAPINFO lpbmi,UINT usage);
int GetDeviceCaps(HDC hdc,int index);
HWND GetDlgItem(HWND hDlg,int nIDDlgItem);
UINT GetDlgItemTextA(HWND hDlg,int nIDDlgItem,LPSTR lpString,int cchMax);
DWORD GetFileSize (HANDLE hFile, LPDWORD lpFileSizeHigh);
HWND GetFocus(VOID);
SHORT GetKeyState(int nVirtKey);
HMENU GetMenu(HWND hWnd);
int GetMenuItemCount(HMENU hMenu);
WINBOOL GetMessageA(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax);
HMODULE GetModuleHandleA (LPCSTR lpModuleName);
int GetObjectA(HANDLE h,int c,LPVOID pv);
WINBOOL GetOpenFileNameA(LPOPENFILENAMEA);
DWORD CommDlgExtendedError(void);
HWND GetParent(HWND hWnd);
WINBOOL IsChild(HWND hWndParent,HWND hWnd);
WINBOOL IsWindow(HWND hWnd);
WINBOOL IsWindowEnabled(HWND hWnd);
int GetDlgCtrlID(HWND hWnd);
UINT GetPrivateProfileIntA (LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
DWORD GetPrivateProfileStringA (LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
int GetROP2(HDC hdc);
WINBOOL GetSaveFileNameA(LPOPENFILENAMEA);
int GetScrollPos(HWND hWnd,int nBar);
WINBOOL GetScrollRange(HWND hWnd,int nBar,int *lpMinPos,int *lpMaxPos);
HGDIOBJ GetStockObject(int i);
HMENU GetSubMenu(HMENU hMenu,int nPos);
DWORD GetSysColor(int nIndex);
int GetSystemMetrics(int nIndex);
WINBOOL GetTextExtentPoint32A(HDC hdc,LPCSTR lpString,int c,LPSIZE psizl);
WINBOOL GetTextMetricsA(HDC hdc,LPTEXTMETRICA lptm);
DWORD GetTickCount (VOID);
WINBOOL GetUpdateRect(HWND hWnd,LPRECT lpRect,WINBOOL bErase);
HWND GetWindow(HWND hWnd,UINT uCmd);
LONG_PTR GetWindowLongPtrA(HWND hWnd,int nIndex);
WINBOOL GetWindowPlacement (HWND hWnd, WINDOWPLACEMENT *lpwndpl);
WINBOOL GetWindowRect(HWND hWnd,LPRECT lpRect);
int GetWindowTextA(HWND hWnd,LPSTR lpString,int nMaxCount);
DWORD GetWindowThreadProcessId(HWND hWnd,LPDWORD lpdwProcessId);
HGLOBAL GlobalAlloc (UINT uFlags, SIZE_T dwBytes);
HGLOBAL GlobalFree (HGLOBAL hMem);
LPVOID GlobalLock (HGLOBAL hMem);
HGLOBAL GlobalReAlloc (HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags);
WINBOOL GlobalUnlock (HGLOBAL hMem);
SIZE_T GlobalSize(HGLOBAL hMem);
UINT GlobalFlags(HGLOBAL hMem);
HGLOBAL GlobalHandle(LPCVOID lpv);
DWORD GetLastError(void);
void SetLastError(DWORD dwError);
WINBOOL InflateRect(LPRECT lprc,int dx,int dy);
WINBOOL InsertMenuA(HMENU hMenu,UINT uPosition,UINT uFlags,UINT_PTR uIDNewItem,LPCSTR lpNewItem);
int IntersectClipRect(HDC hdc,int left,int top,int right,int bottom);
WINBOOL IntersectRect(LPRECT lprcDst,CONST RECT *lprcSrc1,CONST RECT *lprcSrc2);
WINBOOL InvalidateRect(HWND hWnd,CONST RECT *lpRect,WINBOOL bErase);
UINT IsDlgButtonChecked(HWND hDlg,int nIDButton);
WINBOOL IsIconic (HWND hWnd);
WINBOOL IsWindowVisible (HWND hWnd);
WINBOOL IsZoomed (HWND hWnd);
WINBOOL KillTimer(HWND hWnd,UINT_PTR uIDEvent);
WINBOOL LineTo(HDC hdc,int x,int y);
HACCEL LoadAcceleratorsA(HINSTANCE hInstance,LPCSTR lpTableName);
HBITMAP LoadBitmapA(HINSTANCE hInstance,LPCSTR lpBitmapName);
HCURSOR LoadCursorA(HINSTANCE hInstance,LPCSTR lpCursorName);
HICON LoadIconA(HINSTANCE hInstance,LPCSTR lpIconName);
HGLOBAL LoadResource (HMODULE hModule, HRSRC hResInfo);
HLOCAL LocalAlloc (UINT uFlags, SIZE_T uBytes);
HLOCAL LocalFree (HLOCAL hMem);
LPVOID LocalLock(HLOCAL hMem);
BOOL LocalUnlock(HLOCAL hMem);
SIZE_T LocalSize(HLOCAL hMem);
UINT LocalFlags(HLOCAL hMem);
HLOCAL LocalReAlloc(HLOCAL hMem, SIZE_T cb, UINT flags);
HLOCAL LocalHandle(LPCVOID lpv);
LPVOID LockResource (HGLOBAL hResData);
int MapWindowPoints(HWND hWndFrom,HWND hWndTo,LPPOINT lpPoints,UINT cPoints);
WINBOOL MessageBeep(UINT uType);
int MessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);
WINBOOL MoveToEx(HDC hdc,int x,int y,LPPOINT lppt);
WINBOOL MoveWindow (HWND hWnd, int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint);
int MulDiv (int nNumber, int nNumerator, int nDenominator);
WINBOOL OffsetRect(LPRECT lprc,int dx,int dy);
HFILE OpenFile (LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
WINBOOL PatBlt(HDC hdc,int x,int y,int w,int h,DWORD rop);
WINBOOL PeekMessageA(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax,UINT wRemoveMsg);
WINBOOL PostMessageA (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
VOID PostQuitMessage (int nExitCode);
WINBOOL PrintDlgA(LPPRINTDLGA);
WINBOOL PtInRect(CONST RECT *lprc,POINT pt);
UINT RealizePalette(HDC hdc);
UINT GetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, LPPALETTEENTRY rgpe);
WINBOOL Rectangle(HDC hdc,int left,int top,int right,int bottom);
ATOM RegisterClassA (CONST WNDCLASSA *lpWndClass);
WINBOOL ReleaseCapture(VOID);
int ReleaseDC(HWND hWnd,HDC hDC);
WINBOOL ScreenToClient(HWND hWnd,LPPOINT lpPoint);
WINBOOL ScrollWindow(HWND hWnd,int XAmount,int YAmount,CONST RECT *lpRect,CONST RECT *lpClipRect);
int SelectClipRgn(HDC hdc,HRGN hrgn);
HGDIOBJ SelectObject(HDC hdc,HGDIOBJ h);
HPALETTE SelectPalette(HDC hdc,HPALETTE hPal,WINBOOL bForceBkgd);
LRESULT SendDlgItemMessageA(HWND hDlg,int nIDDlgItem,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT SendMessageA(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
COLORREF SetBkColor(HDC hdc,COLORREF color);
int SetBkMode(HDC hdc,int mode);
WINBOOL SetBrushOrgEx(HDC hdc,int x,int y,LPPOINT lppt);
HWND SetCapture(HWND hWnd);
HWND GetCapture(VOID);
HCURSOR SetCursor(HCURSOR hCursor);
WINBOOL SetDlgItemTextA(HWND hDlg,int nIDDlgItem,LPCSTR lpString);
HWND SetFocus(HWND hWnd);
COLORREF SetPixel(HDC hdc,int x,int y,COLORREF color);
int SetROP2(HDC hdc,int rop2);
WINBOOL SetRect(LPRECT lprc,int xLeft,int yTop,int xRight,int yBottom);
int SetScrollPos(HWND hWnd,int nBar,int nPos,WINBOOL bRedraw);
WINBOOL SetScrollRange(HWND hWnd,int nBar,int nMinPos,int nMaxPos,WINBOOL bRedraw);
COLORREF SetTextColor(HDC hdc,COLORREF color);
UINT_PTR SetTimer(HWND hWnd,UINT_PTR nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
LONG_PTR SetWindowLongPtrA(HWND hWnd,int nIndex,LONG_PTR dwNewLong);
WINBOOL SetWindowOrgEx(HDC hdc,int x,int y,LPPOINT lppt);
WINBOOL SetWindowPos (HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
WINBOOL SetWindowTextA(HWND hWnd,LPCSTR lpString);
WINBOOL ShowWindow(HWND hWnd,int nCmdShow);
DWORD SizeofResource (HMODULE hModule, HRSRC hResInfo);
int StretchDIBits(HDC hdc,int xDest,int yDest,int DestWidth,int DestHeight,int xSrc,int ySrc,int SrcWidth,int SrcHeight,CONST VOID *lpBits,CONST BITMAPINFO *lpbmi,UINT iUsage,DWORD rop);
WINBOOL SystemParametersInfoA(UINT uiAction,UINT uiParam,PVOID pvParam,UINT fWinIni);
WINBOOL TextOutA(HDC hdc,int x,int y,LPCSTR lpString,int c);
WINBOOL TrackPopupMenu(HMENU hMenu,UINT uFlags,int x,int y,int nReserved,HWND hWnd,CONST RECT *prcRect);
int TranslateAcceleratorA(HWND hWnd,HACCEL hAccTable,LPMSG lpMsg);
WINBOOL TranslateMessage(CONST win32::MSG *lpMsg);
WINBOOL WaitMessage(VOID);
WINBOOL UnrealizeObject(HGDIOBJ h);
WINBOOL UpdateWindow(HWND hWnd);
WINBOOL ValidateRect(HWND hWnd,CONST RECT *lpRect);
WINBOOL WinHelpA(HWND hWndMain,LPCSTR lpszHelp,UINT uCommand,ULONG_PTR dwData);
HWND WindowFromPoint(POINT Point);
WINBOOL WritePrivateProfileStringA (LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
HFILE _lclose (HFILE hFile);
LONG _llseek (HFILE hFile, LONG lOffset, int iOrigin);
UINT _lread (HFILE hFile, LPVOID lpBuffer, UINT uBytes);
UINT _lwrite (HFILE hFile, LPCCH lpBuffer, UINT uBytes);
LPSTR lstrcatA (LPSTR lpString1, LPCSTR lpString2);
int lstrcmpiA (LPCSTR lpString1, LPCSTR lpString2);
LPSTR lstrcpyA (LPSTR lpString1, LPCSTR lpString2);
int lstrlenA (LPCSTR lpString);
int wsprintfA(LPSTR,LPCSTR,...);

#define AppendMenu AppendMenuA
#define CreateFontIndirect CreateFontIndirectA
#define DrawText DrawTextA
#define ExtTextOut ExtTextOutA
#define FindResource FindResourceA
#define GetDlgItemText GetDlgItemTextA
#define GetClassName GetClassNameA
#define GetModuleHandle GetModuleHandleA
#define GetObject GetObjectA
#define GetPrivateProfileInt GetPrivateProfileIntA
#define GetPrivateProfileString GetPrivateProfileStringA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextMetrics GetTextMetricsA
#define GetWindowLongPtr GetWindowLongPtrA
#define GetWindowText GetWindowTextA
#define InsertMenu InsertMenuA
#define LoadMenu LoadMenuA
#define LoadAccelerators LoadAcceleratorsA
#define LoadBitmap LoadBitmapA
#define LoadCursor LoadCursorA
#define LoadIcon LoadIconA
#define MessageBox MessageBoxA
#define PostMessage PostMessageA
#define PrintDlg PrintDlgA
#define RegisterClass RegisterClassA
#define SetDlgItemText SetDlgItemTextA
#define SetWindowLongPtr SetWindowLongPtrA
#define SystemParametersInfo SystemParametersInfoA
#define TextOut TextOutA
#define WinHelp WinHelpA
#define WritePrivateProfileString WritePrivateProfileStringA
#define lstrcat lstrcatA
#define lstrcpy lstrcpyA
#define lstrlen lstrlenA
#define wsprintf wsprintfA
BOOL SetKeyboardState(BYTE *rgb);
HACCEL CreateAcceleratorTableA(LPACCEL rgaccel, int c);
int CopyAcceleratorTableA(HACCEL haccel, LPACCEL rgaccel, int c);
BOOL DestroyAcceleratorTable(HACCEL haccel);
#define CreateAcceleratorTable CreateAcceleratorTableA
#define CopyAcceleratorTable CopyAcceleratorTableA
HMENU LoadMenuA(HINSTANCE hInstance, LPCSTR pszName);
BOOL GetMenuItemRect(HWND hwnd, HMENU hmenu, UINT i, LPRECT lprc);
HICON CopyIcon(HICON hicon);
BOOL GetIconInfo(HICON hicon, PICONINFO lpii);
HCURSOR GetCursor(void);
BOOL MapDialogRect(HWND hwnd, LPRECT lprc);
BOOL IsDialogMessageA(HWND hwnd, LPMSG lpmsg);
BOOL SetMenu(HWND hwnd, HMENU hmenu);
BOOL EndMenu(void);
HMENU CreateMenu(void);
BOOL IsMenu(HMENU hMenu);
BOOL RemoveMenu(HMENU hMenu, UINT pos, UINT flags);
UINT GetMenuState(HMENU hMenu, UINT id, UINT flags);
UINT GetMenuItemID(HMENU hMenu, int i);
int GetMenuStringA(HMENU hMenu, UINT id, LPSTR psz, int cch, UINT flags);
#define GetMenuString GetMenuStringA
int SetScrollInfo(HWND hwnd, int sb, LPCSCROLLINFO lpsi, BOOL fRedraw);
BOOL GetScrollInfo(HWND hwnd, int sb, LPSCROLLINFO lpsi);
BOOL ShowScrollBar(HWND hwnd, int sb, BOOL fShow);
} // namespace win32
#endif
