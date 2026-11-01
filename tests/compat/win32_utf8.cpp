/* UTF-8 path controls keep byte identity while editing whole code points. */
#include "internal.h"
#include "filesystem.h"
#include <stars_compat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "UTF-8 line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
static void Drain(void) { win32::MSG msg; while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) win32::DispatchMessageA(&msg); }
static HWND hwndPickerOwner;
static BOOL fPickerReady;
static int iPicker;
static const char *pszPickerName;
static void  PickerTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD ms) {
    HWND hwndDlg, hwndEdit;
    char sz[256], *pszClip;
    (void)msg; (void)ms;
    for (hwndDlg = win32::GetWindow(hwndPickerOwner, win32::GW_HWNDFIRST); hwndDlg; hwndDlg = win32::GetWindow(hwndDlg, win32::GW_HWNDNEXT))
        if (hwndDlg->fDialog && hwndDlg->hwndOwner == hwndPickerOwner && win32::GetDlgItem(hwndDlg, 0x480)) break;
    if (!hwndDlg) return;
    win32::KillTimer(hwnd, id);
    hwndEdit = win32::GetDlgItem(hwndDlg, 0x480);
    fPickerReady = hwndEdit && win32::GetDlgItem(hwndDlg, 0x440) && win32::GetDlgItem(hwndDlg, 0x460);
    win32::SetWindowTextA(hwndEdit, "");
    if (iPicker == 0) { SDL_SetClipboardText(pszPickerName); win32::SendMessageA(hwndEdit, win32::WM_PASTE, 0, 0); }
    else { CompatTextInput(hwndEdit, pszPickerName); Drain(); }
    win32::GetWindowTextA(hwndEdit, sz, sizeof(sz)); fPickerReady = fPickerReady && !strcmp(sz, pszPickerName);
    win32::SendMessageA(hwndEdit, win32::EM_SETSEL, 0, -1); win32::SendMessageA(hwndEdit, win32::WM_COPY, 0, 0);
    pszClip = SDL_GetClipboardText(); fPickerReady = fPickerReady && pszClip && !strcmp(pszClip, pszPickerName);
    SDL_free(pszClip);
    win32::SendMessageA(hwndDlg, win32::WM_COMMAND, fPickerReady ? win32::IDOK : win32::IDCANCEL, 0);
}
static int TestPicker(HWND hwnd) {
    char szTemp[4096], szDir[4096], szExpected[8192], szCanonical[4096];
    char szFile[8192];
    char szTitle[4];
    OPENFILENAMEA ofn;
    NATIVEFILE *pf;
    hwndPickerOwner = hwnd;
    CHECK(FFilesystemTempDir(szTemp, sizeof(szTemp)));
    CHECK(snprintf(szDir, sizeof(szDir), "%s/caf\xc3\xa9-\xce\xa9\xf0\x9f\x98\x80", szTemp) < (int)sizeof(szDir));
    CHECK(FFilesystemCreateDirectory(szDir));
    CHECK(FFilesystemCanonicalPath(szDir, szCanonical, sizeof(szCanonical)));
    for (iPicker = 0; iPicker < 2; iPicker++) {
        pszPickerName = iPicker == 0 ? "caf\xc3\xa9-\xce\xa9\xf0\x9f\x98\x80.m1" : "\xe6\x9d\xb1\xe4\xba\xac-\xce\xa9.m1";
        snprintf(szExpected, sizeof(szExpected), "%s/%s", szCanonical, pszPickerName);
        if (iPicker == 0) { pf = PfNativeOpen(szExpected, NATIVEFILEMODE::writeTruncate); CHECK(pf); ICloseNativeFile(pf); }
        memset(&ofn, 0, sizeof(ofn)); szFile[0] = 0;
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFileTitle = szTitle; ofn.nMaxFileTitle = sizeof(szTitle);
        ofn.lpstrInitialDir = szDir; ofn.lpstrFilter = "Stars\0*.m1\0All\0*.*\0";
        ofn.Flags = win32::OFN_NOCHANGEDIR | (iPicker == 0 ? win32::OFN_FILEMUSTEXIST : 0);
        fPickerReady = 0; CHECK(win32::SetTimer(hwnd, 99, 10, PickerTimer));
        CHECK(iPicker == 0 ? win32::GetOpenFileNameA(&ofn) : win32::GetSaveFileNameA(&ofn));
        CHECK(fPickerReady && strcmp(szFile, szExpected) == 0 && win32::IsWindowEnabled(hwnd));
        CHECK(FUtf8Valid(szTitle, strlen(szTitle)));
        CHECK(strcmp(szTitle, iPicker == 0 ? "caf" : "\xe6\x9d\xb1") == 0);
        if (iPicker == 0) {
            CHECK(FFilesystemRemove(szExpected));
        }
    }
    CHECK(FFilesystemRemove(szDir));
    CHECK(FFilesystemRemove(szTemp));
    return 0;
}

int main(void) {
    WNDCLASS wc = {};
    HWND hwnd, hwndEdit, hwndOrdinary, hwndPassword, hwndList;
    HMENU hmenu;
    HDC hdc, hdcOrdinary;
    SIZE size, sizeOrdinary;
    RECT rc = {0, 0, 500, 30};
    char szText[256], szSmall[5], szJson[16384], *pszClipboard;
    char szSnapshotDir[4096], szSnapshotPath[8192];
    const char *psz = "\xc3\xa9\xce\xa9\xe6\x9d\xb1\xe4\xba\xac\xf0\x9f\x98\x80";
    NATIVEFILE *pf;
    STARSCOMPATSNAPSHOT snap = {};
    OFSTRUCT ofs;
    size_t cb, i;
    BOOL fFound = 0;
    wc.lpszClassName = "StarsUtf8Probe\xce\xa9"; wc.lpfnWndProc = win32::DefWindowProcA;
    wc.hInstance = win32::GetModuleHandleA(NULL); CHECK(win32::RegisterClassA(&wc));
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Path fields", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                        0, 0, 520, 160, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && StarsCompatSetWindowTitleUtf8(hwnd, psz));
    CHECK(win32::GetClassNameA(hwnd, szText, 16) == 14 && !strcmp(szText, "StarsUtf8Probe"));
    hmenu = win32::CreatePopupMenu(); CHECK(hmenu && win32::AppendMenuA(hmenu, win32::MF_STRING, 1, psz));
    CHECK(!win32::AppendMenuA(hmenu, win32::MF_STRING, 2, "\xff"));
    CHECK(win32::GetMenuItemCount(hmenu) == 1);
    CHECK(win32::GetMenuStringA(hmenu, 1, szSmall, 2, win32::MF_BYCOMMAND) == 0 && szSmall[0] == 0);
    CHECK(win32::GetMenuStringA(hmenu, 1, szSmall, 4, win32::MF_BYCOMMAND) == 2 && !strcmp(szSmall, "\xc3\xa9"));
    CHECK(win32::DestroyMenu(hmenu));
    hwndList = win32::CreateWindowExA(0, "LISTBOX", "", win32::WS_CHILD | win32::LBS_HASSTRINGS, 0, 0, 100, 20, hwnd,
                            (HMENU)(INT_PTR)103, wc.hInstance, NULL);
    CHECK(hwndList);
    CHECK(win32::SendMessageA(hwndList, win32::LB_ADDSTRING, 0, (LPARAM)"\xff") == win32::LB_ERR);
    CHECK(win32::SendMessageA(hwndList, win32::LB_GETCOUNT, 0, 0) == 0);
    CHECK(win32::OpenFile("/tmp/stars-\xff", &ofs, win32::OF_READ) == win32::HFILE_ERROR);
    CHECK(win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    CHECK(strcmp(SDL_GetWindowTitle(hwnd->lpWindow.get()), psz) == 0);
    hwndEdit = win32::CreateWindowExA(0, "EDIT", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_BORDER | win32::ES_AUTOHSCROLL,
                            8, 8, 500, 30, hwnd, (HMENU)(INT_PTR)100, wc.hInstance, NULL);
    hwndOrdinary = win32::CreateWindowExA(0, "EDIT", "", win32::WS_CHILD | win32::WS_VISIBLE, 8, 48, 500, 30,
                              hwnd, (HMENU)(INT_PTR)101, wc.hInstance, NULL);
    CHECK(hwndEdit && hwndOrdinary);
    CHECK(win32::SetWindowTextA(hwndOrdinary, psz));
    CHECK(win32::GetWindowTextA(hwndOrdinary, szText, sizeof(szText)) == (int)strlen(psz) && strcmp(szText, psz) == 0);
    win32::SetFocus(hwndEdit); CompatTextInput(hwndEdit, psz); Drain();
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == (int)strlen(psz) && strcmp(szText, psz) == 0);
    win32::SendMessageA(hwndEdit, win32::EM_SETSEL, 0, -1); win32::SendMessageA(hwndEdit, win32::WM_COPY, 0, 0);
    pszClipboard = SDL_GetClipboardText(); CHECK(pszClipboard && strcmp(pszClipboard, psz) == 0); SDL_free(pszClipboard);
    win32::SendMessageA(hwndEdit, win32::EM_SETSEL, -1, 0); win32::SendMessageA(hwndEdit, win32::WM_CHAR, '\b', 0);
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == 10);
    CHECK(win32::SendMessageA(hwndEdit, win32::EM_UNDO, 0, 0));
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == 14 && strcmp(szText, psz) == 0);
    win32::SendMessageA(hwndEdit, win32::EM_SETSEL, 0, 0); win32::SendMessageA(hwndEdit, win32::WM_KEYDOWN, win32::VK_DELETE, 0);
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == 12 && strcmp(szText, psz + 2) == 0);
    win32::SendMessageA(hwndEdit, win32::EM_SETSEL, 1, 1); win32::SendMessageA(hwndEdit, win32::WM_KEYDOWN, win32::VK_RIGHT, 0);
    CHECK(LOWORD(win32::SendMessageA(hwndEdit, win32::EM_GETSEL, 0, 0)) == 2);
    CompatTextInput(hwndEdit, "prefix\xff"); Drain();
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == 12 && strcmp(szText, psz + 2) == 0);
    CHECK(win32::SetWindowTextA(hwndEdit, "")); win32::SendMessageA(hwndEdit, win32::EM_LIMITTEXT, 3, 0);
    CompatTextInput(hwndEdit, "\xf0\x9f\x98\x80"); Drain(); CHECK(win32::SendMessageA(hwndEdit, win32::WM_GETTEXTLENGTH, 0, 0) == 0);
    win32::SendMessageA(hwndEdit, win32::EM_LIMITTEXT, 100, 0);
    CHECK(SDL_SetClipboardText(psz)); win32::SendMessageA(hwndEdit, win32::WM_PASTE, 0, 0);
    CHECK(win32::GetWindowTextA(hwndEdit, szText, sizeof(szText)) == 14 && strcmp(szText, psz) == 0);
    CHECK(win32::GetWindowTextA(hwndEdit, szSmall, 2) == 0 && szSmall[0] == 0);
    CHECK(win32::GetWindowTextA(hwndEdit, szSmall, 3) == 2 && strcmp(szSmall, "\xc3\xa9") == 0);
    hdc = win32::GetDC(hwndEdit); hdcOrdinary = win32::GetDC(hwndOrdinary); CHECK(hdc && hdcOrdinary);
    CHECK(win32::GetTextExtentPoint32A(hdc, "\xc3\xa9", 2, &size) && win32::GetTextExtentPoint32A(hdcOrdinary, "\xc3\xa9", 2, &sizeOrdinary));
    CHECK(size.cx == sizeOrdinary.cx && size.cy == sizeOrdinary.cy);
    CHECK(win32::GetTextExtentPoint32A(hdc, psz, 14, &size) && size.cx > 0);
    CHECK(win32::DrawTextA(hdc, psz, 14, &rc, win32::DT_SINGLELINE | win32::DT_NOPREFIX | win32::DT_CALCRECT) > 0 && rc.right == size.cx);
    CHECK(win32::TextOutA(hdc, 0, 0, psz, 14) && win32::ReleaseDC(hwndEdit, hdc) && win32::ReleaseDC(hwndOrdinary, hdcOrdinary));
    CHECK(StarsCompatCaptureHierarchy(&snap));
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == StarsCompatWindowId(hwndEdit)) {
        CHECK(snap.rgWindow[i].fUtf8Text && strcmp(snap.rgWindow[i].pszText, psz) == 0); fFound = 1;
    }
    CHECK(fFound && FFilesystemTempDir(szSnapshotDir, sizeof(szSnapshotDir)));
    CHECK(snprintf(szSnapshotPath, sizeof(szSnapshotPath), "%s/hierarchy.json", szSnapshotDir) < (int)sizeof(szSnapshotPath));
    CHECK((pf = PfNativeOpen(szSnapshotPath, NATIVEFILEMODE::readWriteTruncate)) && StarsCompatWriteHierarchy(&snap, pf));
    ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin); cb = CReadNative(std::span{szJson}.first(sizeof(szJson) - 1), pf); szJson[cb] = 0;
    CHECK(strstr(szJson, psz));
    CHECK(ICloseNativeFile(pf) == 0 && FFilesystemRemove(szSnapshotPath) && FFilesystemRemove(szSnapshotDir));
    StarsCompatFreeHierarchy(&snap);
    hwndPassword = win32::CreateWindowExA(0, "EDIT", "old", win32::WS_CHILD | win32::ES_PASSWORD, 0, 0, 100, 24,
                                hwnd, (HMENU)(INT_PTR)102, wc.hInstance, NULL);
    CHECK(hwndPassword);
    CompatTextInput(hwndPassword, "\xce\xa9"); Drain();
    CHECK(win32::GetWindowTextA(hwndPassword, szText, sizeof(szText)) == 5 && !strcmp(szText, "\xce\xa9old") && !StarsCompatTextInputRejected(hwndPassword));
    CompatTextInput(hwndPassword, "\xc3\xa9"); Drain();
    CHECK(!StarsCompatTextInputRejected(hwndPassword));
    CHECK(SDL_SetClipboardText("\xe6\x9d\xb1\xe4\xba\xac")); win32::SendMessageA(hwndPassword, win32::WM_PASTE, 0, 0);
    CHECK(!StarsCompatTextInputRejected(hwndPassword) && win32::GetWindowTextA(hwndPassword, szText, sizeof(szText)) == 13 && FUtf8Valid(szText, strlen(szText)));
    win32::SendMessageA(hwndPassword, win32::WM_CHAR, '\b', 0); CHECK(!StarsCompatTextInputRejected(hwndPassword));
    CHECK(TestPicker(hwnd) == 0);
    CHECK(win32::DestroyWindow(hwnd)); StarsCompatProcessExit(); SDL_Quit();
    return 0;
}
