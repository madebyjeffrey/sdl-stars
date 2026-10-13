/* Windows declarations adapted from mingw-w64; see ../LICENSE.
 * Modified for the Stars Linux source compatibility layer, October 10, 2026.
 */
#ifndef STARS_WINCOMPAT_API_H
#define STARS_WINCOMPAT_API_H

WINBOOL WINAPI AppendMenuA(HMENU hMenu,UINT uFlags,UINT_PTR uIDNewItem,LPCSTR lpNewItem);
HDC WINAPI BeginPaint(HWND hWnd,LPPAINTSTRUCT lpPaint);
WINBOOL WINAPI BitBlt(HDC hdc,int x,int y,int cx,int cy,HDC hdcSrc,int x1,int y1,DWORD rop);
LRESULT WINAPI CallWindowProcA (WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINBOOL WINAPI CheckDlgButton(HWND hDlg,int nIDButton,UINT uCheck);
DWORD WINAPI CheckMenuItem(HMENU hMenu,UINT uIDCheckItem,UINT uCheck);
WINBOOL WINAPI CheckRadioButton(HWND hDlg,int nIDFirstButton,int nIDLastButton,int nIDCheckButton);
WINBOOL WINAPI ClientToScreen(HWND hWnd,LPPOINT lpPoint);
WINBOOL WINAPI CopyRect(LPRECT lprcDst,CONST RECT *lprcSrc);
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc,int cx,int cy);
HDC WINAPI CreateCompatibleDC(HDC hdc);
HWND WINAPI CreateDialogParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
HFONT WINAPI CreateFontIndirectA(CONST LOGFONTA *lplf);
HPALETTE WINAPI CreatePalette(CONST LOGPALETTE *plpal);
HBRUSH WINAPI CreatePatternBrush(HBITMAP hbm);
HPEN WINAPI CreatePen(int iStyle,int cWidth,COLORREF color);
HMENU WINAPI CreatePopupMenu(VOID);
HRGN WINAPI CreateRectRgn(int x1,int y1,int x2,int y2);
HBRUSH WINAPI CreateSolidBrush(COLORREF color);
HWND WINAPI CreateWindowExA(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
LRESULT WINAPI DefWindowProcA (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
WINBOOL WINAPI DeleteDC(HDC hdc);
WINBOOL WINAPI DeleteMenu(HMENU hMenu,UINT uPosition,UINT uFlags);
WINBOOL WINAPI DeleteObject(HGDIOBJ ho);
WINBOOL WINAPI DestroyCursor(HCURSOR hCursor);
WINBOOL WINAPI DestroyIcon(HICON hIcon);
WINBOOL WINAPI DestroyMenu(HMENU hMenu);
WINBOOL WINAPI DestroyWindow(HWND hWnd);
INT_PTR WINAPI DialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
LRESULT WINAPI DispatchMessageA(CONST MSG *lpMsg);
WINBOOL WINAPI DrawIcon(HDC hDC,int X,int Y,HICON hIcon);
WINBOOL WINAPI DrawMenuBar(HWND hWnd);
int WINAPI DrawTextA(HDC hdc,LPCSTR lpchText,int cchText,LPRECT lprc,UINT format);
WINBOOL WINAPI Ellipse(HDC hdc,int left,int top,int right,int bottom);
WINBOOL WINAPI EnableMenuItem(HMENU hMenu,UINT uIDEnableItem,UINT uEnable);
WINBOOL WINAPI EnableWindow(HWND hWnd,WINBOOL bEnable);
WINBOOL WINAPI EndDialog(HWND hDlg,INT_PTR nResult);
WINBOOL WINAPI EndPaint(HWND hWnd,CONST PAINTSTRUCT *lpPaint);
WINBOOL WINAPI EqualRect(CONST RECT *lprc1,CONST RECT *lprc2);
int WINAPI Escape(HDC hdc,int iEscape,int cjIn,LPCSTR pvIn,LPVOID pvOut);
int WINAPI ExcludeClipRect(HDC hdc,int left,int top,int right,int bottom);
WINBOOL WINAPI ExitWindowsEx(UINT uFlags,DWORD dwReason);
WINBOOL WINAPI ExtTextOutA(HDC hdc,int x,int y,UINT options,CONST RECT *lprect,LPCSTR lpString,UINT c,CONST INT *lpDx);
int WINAPI FillRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr);
HRSRC WINAPI FindResourceA (HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
WINBOOL WINAPI FlashWindow (HWND hWnd, WINBOOL bInvert);
int WINAPI FrameRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr);
WINBOOL WINAPI FreeResource (HGLOBAL hResData);
HWND WINAPI GetActiveWindow(VOID);
SHORT WINAPI GetAsyncKeyState(int vKey);
COLORREF WINAPI GetBkColor(HDC hdc);
int WINAPI GetClassNameA(HWND hWnd,LPSTR lpClassName,int nMaxCount);
WINBOOL WINAPI GetClientRect(HWND hWnd,LPRECT lpRect);
DWORD WINAPI GetCurrentThreadId (VOID);
WINBOOL WINAPI GetCursorPos(LPPOINT lpPoint);
HDC WINAPI GetDC(HWND hWnd);
COLORREF WINAPI GetPixel(HDC hdc,int x,int y);
int WINAPI GetDIBits(HDC hdc,HBITMAP hbm,UINT start,UINT cLines,LPVOID lpvBits,LPBITMAPINFO lpbmi,UINT usage);
int WINAPI GetDeviceCaps(HDC hdc,int index);
HWND WINAPI GetDlgItem(HWND hDlg,int nIDDlgItem);
UINT WINAPI GetDlgItemTextA(HWND hDlg,int nIDDlgItem,LPSTR lpString,int cchMax);
DWORD WINAPI GetFileSize (HANDLE hFile, LPDWORD lpFileSizeHigh);
HWND WINAPI GetFocus(VOID);
SHORT WINAPI GetKeyState(int nVirtKey);
HMENU WINAPI GetMenu(HWND hWnd);
int WINAPI GetMenuItemCount(HMENU hMenu);
WINBOOL WINAPI GetMessageA(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax);
HMODULE WINAPI GetModuleHandleA (LPCSTR lpModuleName);
int WINAPI GetObjectA(HANDLE h,int c,LPVOID pv);
WINBOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA);
DWORD WINAPI CommDlgExtendedError(void);
HWND WINAPI GetParent(HWND hWnd);
WINBOOL WINAPI IsChild(HWND hWndParent,HWND hWnd);
WINBOOL WINAPI IsWindow(HWND hWnd);
WINBOOL WINAPI IsWindowEnabled(HWND hWnd);
int WINAPI GetDlgCtrlID(HWND hWnd);
UINT WINAPI GetPrivateProfileIntA (LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
DWORD WINAPI GetPrivateProfileStringA (LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
int WINAPI GetROP2(HDC hdc);
WINBOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA);
int WINAPI GetScrollPos(HWND hWnd,int nBar);
WINBOOL WINAPI GetScrollRange(HWND hWnd,int nBar,int *lpMinPos,int *lpMaxPos);
HGDIOBJ WINAPI GetStockObject(int i);
HMENU WINAPI GetSubMenu(HMENU hMenu,int nPos);
DWORD WINAPI GetSysColor(int nIndex);
int WINAPI GetSystemMetrics(int nIndex);
WINBOOL WINAPI GetTextExtentPoint32A(HDC hdc,LPCSTR lpString,int c,LPSIZE psizl);
WINBOOL WINAPI GetTextMetricsA(HDC hdc,LPTEXTMETRICA lptm);
DWORD WINAPI GetTickCount (VOID);
WINBOOL WINAPI GetUpdateRect(HWND hWnd,LPRECT lpRect,WINBOOL bErase);
HWND WINAPI GetWindow(HWND hWnd,UINT uCmd);
LONG_PTR WINAPI GetWindowLongPtrA(HWND hWnd,int nIndex);
WINBOOL WINAPI GetWindowPlacement (HWND hWnd, WINDOWPLACEMENT *lpwndpl);
WINBOOL WINAPI GetWindowRect(HWND hWnd,LPRECT lpRect);
int WINAPI GetWindowTextA(HWND hWnd,LPSTR lpString,int nMaxCount);
DWORD WINAPI GetWindowThreadProcessId(HWND hWnd,LPDWORD lpdwProcessId);
HGLOBAL WINAPI GlobalAlloc (UINT uFlags, SIZE_T dwBytes);
HGLOBAL WINAPI GlobalFree (HGLOBAL hMem);
LPVOID WINAPI GlobalLock (HGLOBAL hMem);
HGLOBAL WINAPI GlobalReAlloc (HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags);
WINBOOL WINAPI GlobalUnlock (HGLOBAL hMem);
SIZE_T WINAPI GlobalSize(HGLOBAL hMem);
UINT WINAPI GlobalFlags(HGLOBAL hMem);
HGLOBAL WINAPI GlobalHandle(LPCVOID lpv);
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD dwError);
WINBOOL WINAPI InflateRect(LPRECT lprc,int dx,int dy);
WINBOOL WINAPI InsertMenuA(HMENU hMenu,UINT uPosition,UINT uFlags,UINT_PTR uIDNewItem,LPCSTR lpNewItem);
int WINAPI IntersectClipRect(HDC hdc,int left,int top,int right,int bottom);
WINBOOL WINAPI IntersectRect(LPRECT lprcDst,CONST RECT *lprcSrc1,CONST RECT *lprcSrc2);
WINBOOL WINAPI InvalidateRect(HWND hWnd,CONST RECT *lpRect,WINBOOL bErase);
UINT WINAPI IsDlgButtonChecked(HWND hDlg,int nIDButton);
WINBOOL WINAPI IsIconic (HWND hWnd);
WINBOOL WINAPI IsWindowVisible (HWND hWnd);
WINBOOL WINAPI IsZoomed (HWND hWnd);
WINBOOL WINAPI KillTimer(HWND hWnd,UINT_PTR uIDEvent);
WINBOOL WINAPI LineTo(HDC hdc,int x,int y);
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance,LPCSTR lpTableName);
HBITMAP WINAPI LoadBitmapA(HINSTANCE hInstance,LPCSTR lpBitmapName);
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance,LPCSTR lpCursorName);
HICON WINAPI LoadIconA(HINSTANCE hInstance,LPCSTR lpIconName);
HGLOBAL WINAPI LoadResource (HMODULE hModule, HRSRC hResInfo);
HLOCAL WINAPI LocalAlloc (UINT uFlags, SIZE_T uBytes);
HLOCAL WINAPI LocalFree (HLOCAL hMem);
LPVOID WINAPI LocalLock(HLOCAL hMem);
BOOL WINAPI LocalUnlock(HLOCAL hMem);
SIZE_T WINAPI LocalSize(HLOCAL hMem);
UINT WINAPI LocalFlags(HLOCAL hMem);
HLOCAL WINAPI LocalReAlloc(HLOCAL hMem, SIZE_T cb, UINT flags);
HLOCAL WINAPI LocalHandle(LPCVOID lpv);
LPVOID WINAPI LockResource (HGLOBAL hResData);
int WINAPI MapWindowPoints(HWND hWndFrom,HWND hWndTo,LPPOINT lpPoints,UINT cPoints);
WINBOOL WINAPI MessageBeep(UINT uType);
int WINAPI MessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);
WINBOOL WINAPI MoveToEx(HDC hdc,int x,int y,LPPOINT lppt);
WINBOOL WINAPI MoveWindow (HWND hWnd, int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint);
int WINAPI MulDiv (int nNumber, int nNumerator, int nDenominator);
WINBOOL WINAPI OffsetRect(LPRECT lprc,int dx,int dy);
HFILE WINAPI OpenFile (LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
WINBOOL WINAPI PatBlt(HDC hdc,int x,int y,int w,int h,DWORD rop);
WINBOOL WINAPI PeekMessageA(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax,UINT wRemoveMsg);
WINBOOL WINAPI PostMessageA (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
VOID WINAPI PostQuitMessage (int nExitCode);
WINBOOL WINAPI PrintDlgA(LPPRINTDLGA);
WINBOOL WINAPI PtInRect(CONST RECT *lprc,POINT pt);
UINT WINAPI RealizePalette(HDC hdc);
UINT WINAPI GetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, LPPALETTEENTRY rgpe);
WINBOOL WINAPI Rectangle(HDC hdc,int left,int top,int right,int bottom);
ATOM WINAPI RegisterClassA (CONST WNDCLASSA *lpWndClass);
WINBOOL WINAPI ReleaseCapture(VOID);
int WINAPI ReleaseDC(HWND hWnd,HDC hDC);
WINBOOL WINAPI ScreenToClient(HWND hWnd,LPPOINT lpPoint);
WINBOOL WINAPI ScrollWindow(HWND hWnd,int XAmount,int YAmount,CONST RECT *lpRect,CONST RECT *lpClipRect);
int WINAPI SelectClipRgn(HDC hdc,HRGN hrgn);
HGDIOBJ WINAPI SelectObject(HDC hdc,HGDIOBJ h);
HPALETTE WINAPI SelectPalette(HDC hdc,HPALETTE hPal,WINBOOL bForceBkgd);
LRESULT WINAPI SendDlgItemMessageA(HWND hDlg,int nIDDlgItem,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI SendMessageA(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
COLORREF WINAPI SetBkColor(HDC hdc,COLORREF color);
int WINAPI SetBkMode(HDC hdc,int mode);
WINBOOL WINAPI SetBrushOrgEx(HDC hdc,int x,int y,LPPOINT lppt);
HWND WINAPI SetCapture(HWND hWnd);
HWND WINAPI GetCapture(VOID);
HCURSOR WINAPI SetCursor(HCURSOR hCursor);
WINBOOL WINAPI SetDlgItemTextA(HWND hDlg,int nIDDlgItem,LPCSTR lpString);
HWND WINAPI SetFocus(HWND hWnd);
COLORREF WINAPI SetPixel(HDC hdc,int x,int y,COLORREF color);
int WINAPI SetROP2(HDC hdc,int rop2);
WINBOOL WINAPI SetRect(LPRECT lprc,int xLeft,int yTop,int xRight,int yBottom);
int WINAPI SetScrollPos(HWND hWnd,int nBar,int nPos,WINBOOL bRedraw);
WINBOOL WINAPI SetScrollRange(HWND hWnd,int nBar,int nMinPos,int nMaxPos,WINBOOL bRedraw);
COLORREF WINAPI SetTextColor(HDC hdc,COLORREF color);
UINT_PTR WINAPI SetTimer(HWND hWnd,UINT_PTR nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
LONG_PTR WINAPI SetWindowLongPtrA(HWND hWnd,int nIndex,LONG_PTR dwNewLong);
WINBOOL WINAPI SetWindowOrgEx(HDC hdc,int x,int y,LPPOINT lppt);
WINBOOL WINAPI SetWindowPos (HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
WINBOOL WINAPI SetWindowTextA(HWND hWnd,LPCSTR lpString);
WINBOOL WINAPI ShowWindow(HWND hWnd,int nCmdShow);
DWORD WINAPI SizeofResource (HMODULE hModule, HRSRC hResInfo);
int WINAPI StretchDIBits(HDC hdc,int xDest,int yDest,int DestWidth,int DestHeight,int xSrc,int ySrc,int SrcWidth,int SrcHeight,CONST VOID *lpBits,CONST BITMAPINFO *lpbmi,UINT iUsage,DWORD rop);
WINBOOL WINAPI SystemParametersInfoA(UINT uiAction,UINT uiParam,PVOID pvParam,UINT fWinIni);
WINBOOL WINAPI TextOutA(HDC hdc,int x,int y,LPCSTR lpString,int c);
WINBOOL WINAPI TrackPopupMenu(HMENU hMenu,UINT uFlags,int x,int y,int nReserved,HWND hWnd,CONST RECT *prcRect);
int WINAPI TranslateAcceleratorA(HWND hWnd,HACCEL hAccTable,LPMSG lpMsg);
WINBOOL WINAPI TranslateMessage(CONST MSG *lpMsg);
WINBOOL WINAPI WaitMessage(VOID);
WINBOOL WINAPI UnrealizeObject(HGDIOBJ h);
WINBOOL WINAPI UpdateWindow(HWND hWnd);
WINBOOL WINAPI ValidateRect(HWND hWnd,CONST RECT *lpRect);
WINBOOL WINAPI WinHelpA(HWND hWndMain,LPCSTR lpszHelp,UINT uCommand,ULONG_PTR dwData);
HWND WINAPI WindowFromPoint(POINT Point);
WINBOOL WINAPI WritePrivateProfileStringA (LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
HFILE WINAPI _lclose (HFILE hFile);
LONG WINAPI _llseek (HFILE hFile, LONG lOffset, int iOrigin);
UINT WINAPI _lread (HFILE hFile, LPVOID lpBuffer, UINT uBytes);
UINT WINAPI _lwrite (HFILE hFile, LPCCH lpBuffer, UINT uBytes);
LPSTR WINAPI lstrcatA (LPSTR lpString1, LPCSTR lpString2);
int WINAPI lstrcmpiA (LPCSTR lpString1, LPCSTR lpString2);
LPSTR WINAPI lstrcpyA (LPSTR lpString1, LPCSTR lpString2);
int WINAPI lstrlenA (LPCSTR lpString);
int WINAPIV wsprintfA(LPSTR,LPCSTR,...);

#define AppendMenu AppendMenuA
#define CallWindowProc CallWindowProcA
#define CreateFontIndirect CreateFontIndirectA
#define DefWindowProc DefWindowProcA
#define DispatchMessage DispatchMessageA
#define DrawText DrawTextA
#define ExtTextOut ExtTextOutA
#define FindResource FindResourceA
#define GetDlgItemText GetDlgItemTextA
#define GetMessage GetMessageA
#define GetClassName GetClassNameA
#define GetModuleHandle GetModuleHandleA
#define GetObject GetObjectA
#define GetOpenFileName GetOpenFileNameA
#define GetPrivateProfileInt GetPrivateProfileIntA
#define GetPrivateProfileString GetPrivateProfileStringA
#define GetSaveFileName GetSaveFileNameA
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
#define PeekMessage PeekMessageA
#define PostMessage PostMessageA
#define PrintDlg PrintDlgA
#define RegisterClass RegisterClassA
#define SendDlgItemMessage SendDlgItemMessageA
#define SendMessage SendMessageA
#define SetDlgItemText SetDlgItemTextA
#define SetWindowLongPtr SetWindowLongPtrA
#define SetWindowText SetWindowTextA
#define SystemParametersInfo SystemParametersInfoA
#define TextOut TextOutA
#define TranslateAccelerator TranslateAcceleratorA
#define WinHelp WinHelpA
#define WritePrivateProfileString WritePrivateProfileStringA
#define lstrcat lstrcatA
#define lstrcpy lstrcpyA
#define lstrlen lstrlenA
#define wsprintf wsprintfA
BOOL WINAPI SetKeyboardState(BYTE *rgb);
HACCEL WINAPI CreateAcceleratorTableA(LPACCEL rgaccel, int c);
int WINAPI CopyAcceleratorTableA(HACCEL haccel, LPACCEL rgaccel, int c);
BOOL WINAPI DestroyAcceleratorTable(HACCEL haccel);
#define CreateAcceleratorTable CreateAcceleratorTableA
#define CopyAcceleratorTable CopyAcceleratorTableA
HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR pszName);
BOOL WINAPI GetMenuItemRect(HWND hwnd, HMENU hmenu, UINT i, LPRECT lprc);
HICON WINAPI CopyIcon(HICON hicon);
BOOL WINAPI GetIconInfo(HICON hicon, PICONINFO lpii);
HCURSOR WINAPI GetCursor(void);
BOOL WINAPI MapDialogRect(HWND hwnd, LPRECT lprc);
BOOL WINAPI IsDialogMessageA(HWND hwnd, LPMSG lpmsg);
#define IsDialogMessage IsDialogMessageA
BOOL WINAPI SetMenu(HWND hwnd, HMENU hmenu);
BOOL WINAPI EndMenu(void);
HMENU WINAPI CreateMenu(void);
BOOL WINAPI IsMenu(HMENU hMenu);
BOOL WINAPI RemoveMenu(HMENU hMenu, UINT pos, UINT flags);
UINT WINAPI GetMenuState(HMENU hMenu, UINT id, UINT flags);
UINT WINAPI GetMenuItemID(HMENU hMenu, int i);
int WINAPI GetMenuStringA(HMENU hMenu, UINT id, LPSTR psz, int cch, UINT flags);
#define GetMenuString GetMenuStringA
int WINAPI SetScrollInfo(HWND hwnd, int sb, LPCSCROLLINFO lpsi, BOOL fRedraw);
BOOL WINAPI GetScrollInfo(HWND hwnd, int sb, LPSCROLLINFO lpsi);
BOOL WINAPI ShowScrollBar(HWND hwnd, int sb, BOOL fShow);
#endif
