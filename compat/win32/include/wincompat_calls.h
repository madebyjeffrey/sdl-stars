#ifndef STARS_WINCOMPAT_CALLS_H
#define STARS_WINCOMPAT_CALLS_H

#include <windows.h>

// Owned callers deliver captured callback errors after the raw operation has
// returned and finished its dispatch/modal cleanup. Runtime internals use the
// raw APIs so an error cannot escape before their outer guards have unwound.
LRESULT LUiCallWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND HwndUiCreateWindowEx(DWORD grbitEx, LPCSTR pszClass, LPCSTR pszWindow, DWORD grbitStyle, int x, int y, int dx, int dy, HWND hwndParent, HMENU hmenu, HINSTANCE hinst, LPVOID lpvParam);
HWND HwndUiCreateDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam = 0);
INT_PTR LUiDialogBox(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam = 0);
BOOL FUiDestroyWindow(HWND hwnd);
LRESULT LUiDispatchMessage(const MSG *lpmsg);
BOOL FUiGetMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax);
BOOL FUiPeekMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax, UINT grbitRemove);
LRESULT LUiSendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT LUiSendDlgItemMessage(HWND hwnd, int id, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FUiShowWindow(HWND hwnd, int mdShow);
BOOL FUiEnableWindow(HWND hwnd, BOOL fEnable);
HWND HwndUiSetFocus(HWND hwnd);
BOOL FUiSetWindowPos(HWND hwnd, HWND hwndAfter, int x, int y, int dx, int dy, UINT grbit);
BOOL FUiMoveWindow(HWND hwnd, int x, int y, int dx, int dy, BOOL fRepaint);
BOOL FUiUpdateWindow(HWND hwnd);
BOOL FUiSetWindowText(HWND hwnd, LPCSTR psz);
BOOL FUiIsDialogMessage(HWND hwnd, LPMSG lpmsg);
int IUiTranslateAccelerator(HWND hwnd, HACCEL haccel, LPMSG lpmsg);
BOOL FUiTrackPopupMenu(HMENU hmenu, UINT grbit, int x, int y, int nReserved, HWND hwnd, const RECT *lprc);
LRESULT LUiDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FUiGetOpenFileName(LPOPENFILENAMEA lpof);
BOOL FUiGetSaveFileName(LPOPENFILENAMEA lpof);

#endif
