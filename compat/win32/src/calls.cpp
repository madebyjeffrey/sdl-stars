#include "choice_io.h"
#include <wincompat_calls.h>
#include "errors.h"
#include <type_traits>

LRESULT ui::CallWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = win32::CallWindowProcA(lpfn, hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

HWND ui::CreateWindowEx(DWORD grbitEx, LPCSTR pszClass, LPCSTR pszWindow, DWORD grbitStyle, int x, int y, int dx, int dy, HWND hwndParent, HMENU hmenu, HINSTANCE hinst, LPVOID lpvParam) {
    HWND hwndResult = win32::CreateWindowExA(grbitEx, pszClass, pszWindow, grbitStyle, x, y, dx, dy, hwndParent, hmenu, hinst, lpvParam);
    StarsRethrowPendingError();
    return hwndResult;
}

HWND ui::CreateDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    HWND hwndResult = win32::CreateDialogParamA(hinst, pszTemplate, hwndParent, lpfn, lParam);
    StarsRethrowPendingError();
    return hwndResult;
}

INT_PTR ui::DialogBox(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    INT_PTR lResult = win32::DialogBoxParamA(hinst, pszTemplate, hwndParent, lpfn, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL ui::DestroyWindow(HWND hwnd) {
    BOOL fResult = win32::DestroyWindow(hwnd);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT ui::DispatchMessage(const win32::MSG *lpmsg) {
    LRESULT lResult = win32::DispatchMessageA(lpmsg);
    StarsRethrowPendingError();
    return lResult;
}

BOOL ui::GetMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    BOOL fResult = win32::GetMessageA(lpmsg, hwnd, msgMin, msgMax);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::PeekMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax, UINT grbitRemove) {
    BOOL fResult = win32::PeekMessageA(lpmsg, hwnd, msgMin, msgMax, grbitRemove);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT ui::SendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = win32::SendMessageA(hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

LRESULT ui::SendDlgItemMessage(HWND hwnd, int id, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = win32::SendDlgItemMessageA(hwnd, id, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL ui::ShowWindow(HWND hwnd, int mdShow) {
    BOOL fResult = win32::ShowWindow(hwnd, mdShow);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::EnableWindow(HWND hwnd, BOOL fEnable) {
    BOOL fResult = win32::EnableWindow(hwnd, fEnable);
    StarsRethrowPendingError();
    return fResult;
}

HWND ui::SetFocus(HWND hwnd) {
    HWND hwndResult = win32::SetFocus(hwnd);
    StarsRethrowPendingError();
    return hwndResult;
}

BOOL ui::SetWindowPos(HWND hwnd, HWND hwndAfter, int x, int y, int dx, int dy, UINT grbit) {
    BOOL fResult = win32::SetWindowPos(hwnd, hwndAfter, x, y, dx, dy, grbit);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::MoveWindow(HWND hwnd, int x, int y, int dx, int dy, BOOL fRepaint) {
    BOOL fResult = win32::MoveWindow(hwnd, x, y, dx, dy, fRepaint);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::UpdateWindow(HWND hwnd) {
    BOOL fResult = win32::UpdateWindow(hwnd);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::SetWindowText(HWND hwnd, LPCSTR psz) {
    BOOL fResult = win32::SetWindowTextA(hwnd, psz);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::IsDialogMessage(HWND hwnd, LPMSG lpmsg) {
    BOOL fResult = win32::IsDialogMessageA(hwnd, lpmsg);
    StarsRethrowPendingError();
    return fResult;
}

int ui::TranslateAccelerator(HWND hwnd, HACCEL haccel, LPMSG lpmsg) {
    int iResult = win32::TranslateAcceleratorA(hwnd, haccel, lpmsg);
    StarsRethrowPendingError();
    return iResult;
}

BOOL ui::TrackPopupMenu(HMENU hmenu, UINT grbit, int x, int y, int nReserved, HWND hwnd, const RECT *lprc) {
    BOOL fResult = CompatChoiceIO().pfnTrackPopupMenu(hmenu, grbit, x, y, nReserved, hwnd, lprc);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT ui::DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = win32::DefWindowProcA(hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL ui::GetOpenFileName(LPOPENFILENAMEA lpof) {
    BOOL fResult = win32::GetOpenFileNameA(lpof);
    StarsRethrowPendingError();
    return fResult;
}

BOOL ui::GetSaveFileName(LPOPENFILENAMEA lpof) {
    BOOL fResult = win32::GetSaveFileNameA(lpof);
    StarsRethrowPendingError();
    return fResult;
}

static_assert(std::is_same_v<decltype(&ui::CallWindowProc), decltype(&win32::CallWindowProcA)>);
static_assert(std::is_same_v<decltype(&ui::CreateWindowEx), decltype(&win32::CreateWindowExA)>);
static_assert(std::is_same_v<decltype(&ui::CreateDialog), decltype(&win32::CreateDialogParamA)>);
static_assert(std::is_same_v<decltype(&ui::DialogBox), decltype(&win32::DialogBoxParamA)>);
static_assert(std::is_same_v<decltype(&ui::DestroyWindow), decltype(&win32::DestroyWindow)>);
static_assert(std::is_same_v<decltype(&ui::DispatchMessage), decltype(&win32::DispatchMessageA)>);
static_assert(std::is_same_v<decltype(&ui::GetMessage), decltype(&win32::GetMessageA)>);
static_assert(std::is_same_v<decltype(&ui::PeekMessage), decltype(&win32::PeekMessageA)>);
static_assert(std::is_same_v<decltype(&ui::SendMessage), decltype(&win32::SendMessageA)>);
static_assert(std::is_same_v<decltype(&ui::SendDlgItemMessage), decltype(&win32::SendDlgItemMessageA)>);
static_assert(std::is_same_v<decltype(&ui::ShowWindow), decltype(&win32::ShowWindow)>);
static_assert(std::is_same_v<decltype(&ui::EnableWindow), decltype(&win32::EnableWindow)>);
static_assert(std::is_same_v<decltype(&ui::SetFocus), decltype(&win32::SetFocus)>);
static_assert(std::is_same_v<decltype(&ui::SetWindowPos), decltype(&win32::SetWindowPos)>);
static_assert(std::is_same_v<decltype(&ui::MoveWindow), decltype(&win32::MoveWindow)>);
static_assert(std::is_same_v<decltype(&ui::UpdateWindow), decltype(&win32::UpdateWindow)>);
static_assert(std::is_same_v<decltype(&ui::SetWindowText), decltype(&win32::SetWindowTextA)>);
static_assert(std::is_same_v<decltype(&ui::IsDialogMessage), decltype(&win32::IsDialogMessageA)>);
static_assert(std::is_same_v<decltype(&ui::TranslateAccelerator), decltype(&win32::TranslateAcceleratorA)>);
static_assert(std::is_same_v<decltype(&ui::TrackPopupMenu), decltype(&win32::TrackPopupMenu)>);
static_assert(std::is_same_v<decltype(&ui::DefWindowProc), decltype(&win32::DefWindowProcA)>);
static_assert(std::is_same_v<decltype(&ui::GetOpenFileName), decltype(&win32::GetOpenFileNameA)>);
static_assert(std::is_same_v<decltype(&ui::GetSaveFileName), decltype(&win32::GetSaveFileNameA)>);
