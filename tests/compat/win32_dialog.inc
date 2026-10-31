/* Resource-backed modeless/modal lifecycle, checked against the Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../../res/resource.h"
#ifndef _WIN32
#include <stars_compat.h>
#include <SDL3/SDL.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#define EXPECT(f) do { if (!(f)) { fprintf(stderr, "callback line %d: %s\n", __LINE__, #f); cErrors++; } } while (0)
static int cErrors, cInit, cDestroy, cNcDestroy;
static HWND hwndOwner;
static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    INT_PTR mode;
    if (msg == WM_INITDIALOG) {
        cInit++; SetWindowLongPtr(hwnd, DWLP_USER, l);
        EXPECT(GetDlgItem(hwnd, IDC_EDIT1) && GetDlgItem(hwnd, IDOK) && GetDlgItem(hwnd, IDCANCEL));
        EXPECT((HWND)w == GetDlgItem(hwnd, IDC_EDIT1));
        EXPECT(SendMessageA(hwnd, WM_GETFONT, 0, 0) != 0);
        EXPECT(SendDlgItemMessageA(hwnd, IDC_EDIT1, WM_GETFONT, 0, 0) != 0);
        EXPECT(GetParent(hwnd) == hwndOwner);
        if (l) EXPECT(!IsWindowEnabled(hwndOwner));
        if (l == 1) { EXPECT(EndDialog(hwnd, 1234)); EXPECT(IsWindow(hwnd)); }
        else if (l == 2) EXPECT(PostMessage(hwnd, WM_APP + 1, 0, 0));
        else if (l == 7 || l == 8) {
            EXPECT(PostMessage(hwnd, WM_COMMAND, l == 7 ? IDCANCEL : IDOK, 0));
            EXPECT(PostMessage(hwnd, WM_APP + 1, 0, 0));
        }
        else if (l == 5 || l == 6) EXPECT(PostMessage(hwnd, WM_COMMAND, l == 5 ? IDCANCEL : IDOK, 0));
        else if (l == 3) {
            HWND hwndSave = hwndOwner;
            hwndOwner = hwnd;
            EXPECT(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 1) == 1234);
            hwndOwner = hwndSave;
            EXPECT(IsWindowEnabled(hwnd) && !IsWindowEnabled(hwndOwner));
            EXPECT(EndDialog(hwnd, 88));
        }
#ifndef _WIN32
        else if (l == 4) EXPECT(EndDialog(hwnd, 2));
#endif
        return TRUE;
    }
    if (msg == WM_APP + 1) {
        EXPECT(EndDialog(hwnd, 55)); EXPECT(IsWindow(hwnd)); return TRUE;
    }
    if (msg == WM_COMMAND) {
        mode = GetWindowLongPtr(hwnd, DWLP_USER);
        if ((mode == 5 || mode == 6) && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) {
            EXPECT(EndDialog(hwnd, mode == 5 ? 0 : 77));
            return FALSE;
        }
        if (mode == 7 || mode == 8) return FALSE;
        if (mode && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) EndDialog(hwnd, LOWORD(w));
        return TRUE;
    }
    if (msg == WM_DESTROY) { cDestroy++; EXPECT(GetDlgItem(hwnd, IDC_EDIT1) != NULL); }
    if (msg == WM_NCDESTROY) cNcDestroy++;
    return FALSE;
}
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { return DefWindowProcA(hwnd, msg, w, l); }
int main(void) {
    WNDCLASS wc = {0};
    HINSTANCE hinst = GetModuleHandle(NULL);
    HWND hwnd, hwndOther, hwndEdit;
    RECT rc, mapped;
    POINT pt;
    APPMSG msg = {0};
    char sz[64];
    int cBefore;
#ifndef _WIN32
    HFONT hfont;
    LOGFONT lf;
    SDL_Window **rglpWindow;
    int cWindowBefore, cWindowAfter;
#endif
    wc.lpfnWndProc = OwnerProc; wc.lpszClassName = "DialogOwner"; wc.hInstance = hinst;
    CHECK(RegisterClass(&wc));
    hwndOwner = CreateWindow(wc.lpszClassName, "Owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 300, NULL, NULL, hinst, NULL);
    CHECK(hwndOwner);
    hwnd = CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 0);
    hwndOther = CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 0);
    CHECK(hwnd && hwndOther && hwnd != hwndOther && cErrors == 0 && cInit == 2);
    CHECK(IsWindowVisible(hwnd) && IsWindowEnabled(hwndOwner));
    CHECK(GetWindowText(hwnd, sz, sizeof(sz)) && strcmp(sz, "Stars!") == 0);
    CHECK(SetDlgItemText(hwnd, IDC_EDIT1, "secret"));
#ifndef _WIN32
    hfont = (HFONT)SendMessageA(hwnd, WM_GETFONT, 0, 0);
    CHECK(GetObject(hfont, sizeof(lf), &lf) == sizeof(lf));
#endif
    CHECK(GetDlgItemText(hwndOther, IDC_EDIT1, sz, sizeof(sz)) == 0);
    hwndEdit = GetDlgItem(hwnd, IDC_EDIT1);
    CHECK(GetWindowRect(hwndEdit, &rc)); pt = (POINT){rc.left, rc.top}; CHECK(ScreenToClient(hwnd, &pt));
    mapped = (RECT){6, 22, 73, 14}; CHECK(MapDialogRect(hwnd, &mapped));
    CHECK(pt.x == mapped.left && pt.y == mapped.top && rc.right - rc.left == mapped.right);
    SetFocus(hwndEdit); msg.hwnd = hwndEdit; msg.message = WM_KEYDOWN; msg.wParam = VK_TAB;
    CHECK(IsDialogMessageA(hwnd, &msg) && GetFocus() == GetDlgItem(hwnd, IDOK));
    CHECK(DestroyWindow(hwndOther) && IsWindow(hwnd));
    CHECK(DestroyWindow(hwnd) && cDestroy == 2 && cNcDestroy == 2 && cErrors == 0);
#ifndef _WIN32
    CHECK(GetObject(hfont, sizeof(lf), &lf) == 0);
#endif
    cBefore = cDestroy;
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 1) == 1234);
    CHECK(IsWindowEnabled(hwndOwner) && cDestroy == cBefore + 1 && cErrors == 0);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 2) == 55);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 3) == 88);
    CHECK(IsWindowEnabled(hwndOwner) && cErrors == 0);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 5) == 0);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 6) == 77);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 7) == 55);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 8) == 55);
    CHECK(IsWindowEnabled(hwndOwner) && cErrors == 0);
#ifndef _WIN32
    rglpWindow = SDL_GetWindows(&cWindowBefore); SDL_free(rglpWindow);
    CHECK(StarsCompatBeginDialogSequence(hwndOwner));
    CHECK(!StarsCompatBeginDialogSequence(hwndOwner));
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 4) == 2);
    CHECK(!IsWindowEnabled(hwndOwner));
    rglpWindow = SDL_GetWindows(&cWindowAfter); SDL_free(rglpWindow);
    CHECK(cWindowAfter == cWindowBefore + 1);
    StarsCompatEndDialogSequence();
    CHECK(IsWindowEnabled(hwndOwner));
    rglpWindow = SDL_GetWindows(&cWindowAfter); SDL_free(rglpWindow);
    CHECK(cWindowAfter == cWindowBefore);
    CHECK(StarsCompatBeginDialogSequence(hwndOwner));
    CHECK(DialogBoxParam(hinst, "missing", hwndOwner, DialogProc, 4) == -1);
    StarsCompatEndDialogSequence();
    CHECK(IsWindowEnabled(hwndOwner));
#endif
    EnableWindow(hwndOwner, FALSE);
    CHECK(DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, DialogProc, 1) == 1234);
#ifndef _WIN32
    CHECK(!IsWindowEnabled(hwndOwner));
#else
    // Wine re-enables an initially disabled owner on immediate EndDialog.
    // Keep this result visible; the local runtime follows the documented rule.
    printf("SDK initially-disabled owner after modal: %s\n", IsWindowEnabled(hwndOwner) ? "enabled" : "disabled");
#endif
    EnableWindow(hwndOwner, TRUE);
    CHECK(!CreateDialogParam(hinst, "missing", hwndOwner, DialogProc, 0));
    CHECK(DialogBoxParam(hinst, "missing", hwndOwner, DialogProc, 0) == -1 && IsWindowEnabled(hwndOwner));
    hwnd = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_ABOUT), hwndOwner, NULL, 0);
    CHECK(hwnd && GetWindowText(hwnd, sz, sizeof(sz)) && strcmp(sz, "About Stars!") == 0);
    ShowWindow(hwnd, SW_SHOW); UpdateWindow(hwnd);
    for (hwndOther = GetWindow(hwnd, GW_CHILD); hwndOther; hwndOther = GetWindow(hwndOther, GW_HWNDNEXT)) UpdateWindow(hwndOther);
    CHECK(DestroyWindow(hwnd));
    CHECK(DestroyWindow(hwndOwner));
#ifndef _WIN32
    CHECK(!EndDialog(hwnd, 0));
    StarsCompatShutdown();
#endif
    puts("Dialog lifecycle and owner contracts passed"); return 0;
}
