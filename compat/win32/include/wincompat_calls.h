#ifndef STARS_WINCOMPAT_CALLS_H
#define STARS_WINCOMPAT_CALLS_H

#include <windows.h>

namespace ui {

// Owned callers deliver captured callback errors after the raw operation has
// returned and finished its dispatch/modal cleanup. Runtime internals use the
// raw APIs so an error cannot escape before their outer guards have unwound.
LRESULT CallWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND CreateWindowEx(DWORD grbitEx, LPCSTR pszClass, LPCSTR pszWindow, DWORD grbitStyle, int x, int y, int dx, int dy, HWND hwndParent, HMENU hmenu, HINSTANCE hinst, LPVOID lpvParam);
HWND CreateDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam = 0);
INT_PTR DialogBox(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam = 0);
BOOL DestroyWindow(HWND hwnd);
LRESULT DispatchMessage(const win32::MSG *lpmsg);
BOOL GetMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax);
BOOL PeekMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax, UINT grbitRemove);
LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT SendDlgItemMessage(HWND hwnd, int id, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL ShowWindow(HWND hwnd, int mdShow);
BOOL EnableWindow(HWND hwnd, BOOL fEnable);
HWND SetFocus(HWND hwnd);
BOOL SetWindowPos(HWND hwnd, HWND hwndAfter, int x, int y, int dx, int dy, UINT grbit);
BOOL MoveWindow(HWND hwnd, int x, int y, int dx, int dy, BOOL fRepaint);
BOOL UpdateWindow(HWND hwnd);
BOOL SetWindowText(HWND hwnd, LPCSTR psz);
BOOL IsDialogMessage(HWND hwnd, LPMSG lpmsg);
int TranslateAccelerator(HWND hwnd, HACCEL haccel, LPMSG lpmsg);
BOOL TrackPopupMenu(HMENU hmenu, UINT grbit, int x, int y, int nReserved, HWND hwnd, const RECT *lprc);
LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL GetOpenFileName(LPOPENFILENAMEA lpof);
BOOL GetSaveFileName(LPOPENFILENAMEA lpof);

} // namespace ui

#endif
