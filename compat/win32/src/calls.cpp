#include "choice_io.h"
#include <wincompat_calls.h>
#include "errors.h"
#include <type_traits>

LRESULT LUiCallWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = CallWindowProcA(lpfn, hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

HWND HwndUiCreateWindowEx(DWORD grbitEx, LPCSTR pszClass, LPCSTR pszWindow, DWORD grbitStyle, int x, int y, int dx, int dy, HWND hwndParent, HMENU hmenu, HINSTANCE hinst, LPVOID lpvParam) {
    HWND hwndResult = CreateWindowExA(grbitEx, pszClass, pszWindow, grbitStyle, x, y, dx, dy, hwndParent, hmenu, hinst, lpvParam);
    StarsRethrowPendingError();
    return hwndResult;
}

HWND HwndUiCreateDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    HWND hwndResult = CreateDialogParamA(hinst, pszTemplate, hwndParent, lpfn, lParam);
    StarsRethrowPendingError();
    return hwndResult;
}

INT_PTR LUiDialogBox(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    INT_PTR lResult = DialogBoxParamA(hinst, pszTemplate, hwndParent, lpfn, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL FUiDestroyWindow(HWND hwnd) {
    BOOL fResult = DestroyWindow(hwnd);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT LUiDispatchMessage(const MSG *lpmsg) {
    LRESULT lResult = DispatchMessageA(lpmsg);
    StarsRethrowPendingError();
    return lResult;
}

BOOL FUiGetMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    BOOL fResult = GetMessageA(lpmsg, hwnd, msgMin, msgMax);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiPeekMessage(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax, UINT grbitRemove) {
    BOOL fResult = PeekMessageA(lpmsg, hwnd, msgMin, msgMax, grbitRemove);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT LUiSendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = SendMessageA(hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

LRESULT LUiSendDlgItemMessage(HWND hwnd, int id, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = SendDlgItemMessageA(hwnd, id, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL FUiShowWindow(HWND hwnd, int mdShow) {
    BOOL fResult = ShowWindow(hwnd, mdShow);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiEnableWindow(HWND hwnd, BOOL fEnable) {
    BOOL fResult = EnableWindow(hwnd, fEnable);
    StarsRethrowPendingError();
    return fResult;
}

HWND HwndUiSetFocus(HWND hwnd) {
    HWND hwndResult = SetFocus(hwnd);
    StarsRethrowPendingError();
    return hwndResult;
}

BOOL FUiSetWindowPos(HWND hwnd, HWND hwndAfter, int x, int y, int dx, int dy, UINT grbit) {
    BOOL fResult = SetWindowPos(hwnd, hwndAfter, x, y, dx, dy, grbit);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiMoveWindow(HWND hwnd, int x, int y, int dx, int dy, BOOL fRepaint) {
    BOOL fResult = MoveWindow(hwnd, x, y, dx, dy, fRepaint);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiUpdateWindow(HWND hwnd) {
    BOOL fResult = UpdateWindow(hwnd);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiSetWindowText(HWND hwnd, LPCSTR psz) {
    BOOL fResult = SetWindowTextA(hwnd, psz);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiIsDialogMessage(HWND hwnd, LPMSG lpmsg) {
    BOOL fResult = IsDialogMessageA(hwnd, lpmsg);
    StarsRethrowPendingError();
    return fResult;
}

int IUiTranslateAccelerator(HWND hwnd, HACCEL haccel, LPMSG lpmsg) {
    int iResult = TranslateAcceleratorA(hwnd, haccel, lpmsg);
    StarsRethrowPendingError();
    return iResult;
}

BOOL FUiTrackPopupMenu(HMENU hmenu, UINT grbit, int x, int y, int nReserved, HWND hwnd, const RECT *lprc) {
    BOOL fResult = CompatChoiceIO().pfnTrackPopupMenu(hmenu, grbit, x, y, nReserved, hwnd, lprc);
    StarsRethrowPendingError();
    return fResult;
}

LRESULT LUiDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = DefWindowProcA(hwnd, msg, wParam, lParam);
    StarsRethrowPendingError();
    return lResult;
}

BOOL FUiGetOpenFileName(LPOPENFILENAMEA lpof) {
    BOOL fResult = GetOpenFileNameA(lpof);
    StarsRethrowPendingError();
    return fResult;
}

BOOL FUiGetSaveFileName(LPOPENFILENAMEA lpof) {
    BOOL fResult = GetSaveFileNameA(lpof);
    StarsRethrowPendingError();
    return fResult;
}

static_assert(std::is_same_v<decltype(&LUiCallWindowProc), decltype(&CallWindowProcA)>);
static_assert(std::is_same_v<decltype(&HwndUiCreateWindowEx), decltype(&CreateWindowExA)>);
static_assert(std::is_same_v<decltype(&HwndUiCreateDialog), decltype(&CreateDialogParamA)>);
static_assert(std::is_same_v<decltype(&LUiDialogBox), decltype(&DialogBoxParamA)>);
static_assert(std::is_same_v<decltype(&FUiDestroyWindow), decltype(&DestroyWindow)>);
static_assert(std::is_same_v<decltype(&LUiDispatchMessage), decltype(&DispatchMessageA)>);
static_assert(std::is_same_v<decltype(&FUiGetMessage), decltype(&GetMessageA)>);
static_assert(std::is_same_v<decltype(&FUiPeekMessage), decltype(&PeekMessageA)>);
static_assert(std::is_same_v<decltype(&LUiSendMessage), decltype(&SendMessageA)>);
static_assert(std::is_same_v<decltype(&LUiSendDlgItemMessage), decltype(&SendDlgItemMessageA)>);
static_assert(std::is_same_v<decltype(&FUiShowWindow), decltype(&ShowWindow)>);
static_assert(std::is_same_v<decltype(&FUiEnableWindow), decltype(&EnableWindow)>);
static_assert(std::is_same_v<decltype(&HwndUiSetFocus), decltype(&SetFocus)>);
static_assert(std::is_same_v<decltype(&FUiSetWindowPos), decltype(&SetWindowPos)>);
static_assert(std::is_same_v<decltype(&FUiMoveWindow), decltype(&MoveWindow)>);
static_assert(std::is_same_v<decltype(&FUiUpdateWindow), decltype(&UpdateWindow)>);
static_assert(std::is_same_v<decltype(&FUiSetWindowText), decltype(&SetWindowTextA)>);
static_assert(std::is_same_v<decltype(&FUiIsDialogMessage), decltype(&IsDialogMessageA)>);
static_assert(std::is_same_v<decltype(&IUiTranslateAccelerator), decltype(&TranslateAcceleratorA)>);
static_assert(std::is_same_v<decltype(&FUiTrackPopupMenu), decltype(&TrackPopupMenu)>);
static_assert(std::is_same_v<decltype(&LUiDefWindowProc), decltype(&DefWindowProcA)>);
static_assert(std::is_same_v<decltype(&FUiGetOpenFileName), decltype(&GetOpenFileNameA)>);
static_assert(std::is_same_v<decltype(&FUiGetSaveFileName), decltype(&GetSaveFileNameA)>);
