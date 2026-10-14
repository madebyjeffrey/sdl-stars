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
static void Drain(void) { MSG msg; while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg); }
static HWND hwndPickerOwner;
static BOOL fPickerReady;
static int iPicker;
static const char *pszPickerName;
static void CALLBACK PickerTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD ms) {
    HWND hwndDlg, hwndEdit;
    char sz[256], *pszClip;
    (void)msg; (void)ms;
    for (hwndDlg = GetWindow(hwndPickerOwner, GW_HWNDFIRST); hwndDlg; hwndDlg = GetWindow(hwndDlg, GW_HWNDNEXT))
        if (hwndDlg->fDialog && hwndDlg->hwndOwner == hwndPickerOwner && GetDlgItem(hwndDlg, 0x480)) break;
    if (!hwndDlg) return;
    KillTimer(hwnd, id);
    hwndEdit = GetDlgItem(hwndDlg, 0x480);
    fPickerReady = hwndEdit && GetDlgItem(hwndDlg, 0x440) && GetDlgItem(hwndDlg, 0x460);
    SetWindowText(hwndEdit, "");
    if (iPicker == 0) { SDL_SetClipboardText(pszPickerName); SendMessage(hwndEdit, WM_PASTE, 0, 0); }
    else { CompatTextInput(hwndEdit, pszPickerName); Drain(); }
    GetWindowText(hwndEdit, sz, sizeof(sz)); fPickerReady = fPickerReady && !strcmp(sz, pszPickerName);
    SendMessage(hwndEdit, EM_SETSEL, 0, -1); SendMessage(hwndEdit, WM_COPY, 0, 0);
    pszClip = SDL_GetClipboardText(); fPickerReady = fPickerReady && pszClip && !strcmp(pszClip, pszPickerName);
    SDL_free(pszClip);
    SendMessage(hwndDlg, WM_COMMAND, fPickerReady ? IDOK : IDCANCEL, 0);
}
static int TestPicker(HWND hwnd) {
    char szTemp[4096], szDir[4096], szExpected[8192], szCanonical[4096];
    char szFile[8192];
    char szTitle[4];
    OPENFILENAMEA ofn;
    FILE *pf;
    hwndPickerOwner = hwnd;
    CHECK(FFilesystemTempDir(szTemp, sizeof(szTemp)));
    CHECK(snprintf(szDir, sizeof(szDir), "%s/caf\xc3\xa9-\xce\xa9\xf0\x9f\x98\x80", szTemp) < (int)sizeof(szDir));
    CHECK(FFilesystemCreateDirectory(szDir));
    CHECK(FFilesystemCanonicalPath(szDir, szCanonical, sizeof(szCanonical)));
    for (iPicker = 0; iPicker < 2; iPicker++) {
        pszPickerName = iPicker == 0 ? "caf\xc3\xa9-\xce\xa9\xf0\x9f\x98\x80.m1" : "\xe6\x9d\xb1\xe4\xba\xac-\xce\xa9.m1";
        snprintf(szExpected, sizeof(szExpected), "%s/%s", szCanonical, pszPickerName);
        if (iPicker == 0) { pf = fopen(szExpected, "wb"); CHECK(pf); fclose(pf); }
        memset(&ofn, 0, sizeof(ofn)); szFile[0] = 0;
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFileTitle = szTitle; ofn.nMaxFileTitle = sizeof(szTitle);
        ofn.lpstrInitialDir = szDir; ofn.lpstrFilter = "Stars\0*.m1\0All\0*.*\0";
        ofn.Flags = OFN_NOCHANGEDIR | (iPicker == 0 ? OFN_FILEMUSTEXIST : 0);
        fPickerReady = FALSE; CHECK(SetTimer(hwnd, 99, 10, PickerTimer));
        CHECK(iPicker == 0 ? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn));
        CHECK(fPickerReady && strcmp(szFile, szExpected) == 0 && IsWindowEnabled(hwnd));
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
    WNDCLASS wc = {0};
    HWND hwnd, hwndEdit, hwndOrdinary, hwndPassword, hwndList;
    HMENU hmenu;
    HDC hdc, hdcOrdinary;
    SIZE size, sizeOrdinary;
    RECT rc = {0, 0, 500, 30};
    char szText[256], szSmall[5], szJson[16384], *pszClipboard;
    char szSnapshotDir[4096], szSnapshotPath[8192];
    const char *psz = "\xc3\xa9\xce\xa9\xe6\x9d\xb1\xe4\xba\xac\xf0\x9f\x98\x80";
    FILE *pf;
    STARSCOMPATSNAPSHOT snap = {0};
    OFSTRUCT ofs;
    size_t cb, i;
    BOOL fFound = FALSE;
    wc.lpszClassName = "StarsUtf8Probe\xce\xa9"; wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandle(NULL); CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Path fields", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        0, 0, 520, 160, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwnd && StarsCompatSetWindowTitleUtf8(hwnd, psz));
    CHECK(GetClassNameA(hwnd, szText, 16) == 14 && !strcmp(szText, "StarsUtf8Probe"));
    hmenu = CreatePopupMenu(); CHECK(hmenu && AppendMenuA(hmenu, MF_STRING, 1, psz));
    CHECK(!AppendMenuA(hmenu, MF_STRING, 2, "\xff"));
    CHECK(GetMenuItemCount(hmenu) == 1);
    CHECK(GetMenuStringA(hmenu, 1, szSmall, 2, MF_BYCOMMAND) == 0 && szSmall[0] == 0);
    CHECK(GetMenuStringA(hmenu, 1, szSmall, 4, MF_BYCOMMAND) == 2 && !strcmp(szSmall, "\xc3\xa9"));
    CHECK(DestroyMenu(hmenu));
    hwndList = CreateWindow("LISTBOX", "", WS_CHILD | LBS_HASSTRINGS, 0, 0, 100, 20, hwnd,
                            (HMENU)(INT_PTR)103, wc.hInstance, NULL);
    CHECK(hwndList);
    CHECK(SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)"\xff") == LB_ERR);
    CHECK(SendMessage(hwndList, LB_GETCOUNT, 0, 0) == 0);
    CHECK(OpenFile("/tmp/stars-\xff", &ofs, OF_READ) == HFILE_ERROR);
    CHECK(GetLastError() == ERROR_INVALID_PARAMETER);
    CHECK(strcmp(SDL_GetWindowTitle(hwnd->lpWindow), psz) == 0);
    hwndEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                            8, 8, 500, 30, hwnd, (HMENU)(INT_PTR)100, wc.hInstance, NULL);
    hwndOrdinary = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE, 8, 48, 500, 30,
                              hwnd, (HMENU)(INT_PTR)101, wc.hInstance, NULL);
    CHECK(hwndEdit && hwndOrdinary);
    CHECK(SetWindowText(hwndOrdinary, psz));
    CHECK(GetWindowText(hwndOrdinary, szText, sizeof(szText)) == (int)strlen(psz) && strcmp(szText, psz) == 0);
    SetFocus(hwndEdit); CompatTextInput(hwndEdit, psz); Drain();
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == (int)strlen(psz) && strcmp(szText, psz) == 0);
    SendMessage(hwndEdit, EM_SETSEL, 0, -1); SendMessage(hwndEdit, WM_COPY, 0, 0);
    pszClipboard = SDL_GetClipboardText(); CHECK(pszClipboard && strcmp(pszClipboard, psz) == 0); SDL_free(pszClipboard);
    SendMessage(hwndEdit, EM_SETSEL, -1, 0); SendMessage(hwndEdit, WM_CHAR, '\b', 0);
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == 10);
    CHECK(SendMessage(hwndEdit, EM_UNDO, 0, 0));
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == 14 && strcmp(szText, psz) == 0);
    SendMessage(hwndEdit, EM_SETSEL, 0, 0); SendMessage(hwndEdit, WM_KEYDOWN, VK_DELETE, 0);
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == 12 && strcmp(szText, psz + 2) == 0);
    SendMessage(hwndEdit, EM_SETSEL, 1, 1); SendMessage(hwndEdit, WM_KEYDOWN, VK_RIGHT, 0);
    CHECK(LOWORD(SendMessage(hwndEdit, EM_GETSEL, 0, 0)) == 2);
    CompatTextInput(hwndEdit, "prefix\xff"); Drain();
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == 12 && strcmp(szText, psz + 2) == 0);
    CHECK(SetWindowText(hwndEdit, "")); SendMessage(hwndEdit, EM_LIMITTEXT, 3, 0);
    CompatTextInput(hwndEdit, "\xf0\x9f\x98\x80"); Drain(); CHECK(SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0) == 0);
    SendMessage(hwndEdit, EM_LIMITTEXT, 100, 0);
    CHECK(SDL_SetClipboardText(psz)); SendMessage(hwndEdit, WM_PASTE, 0, 0);
    CHECK(GetWindowText(hwndEdit, szText, sizeof(szText)) == 14 && strcmp(szText, psz) == 0);
    CHECK(GetWindowText(hwndEdit, szSmall, 2) == 0 && szSmall[0] == 0);
    CHECK(GetWindowText(hwndEdit, szSmall, 3) == 2 && strcmp(szSmall, "\xc3\xa9") == 0);
    hdc = GetDC(hwndEdit); hdcOrdinary = GetDC(hwndOrdinary); CHECK(hdc && hdcOrdinary);
    CHECK(GetTextExtentPoint32A(hdc, "\xc3\xa9", 2, &size) && GetTextExtentPoint32A(hdcOrdinary, "\xc3\xa9", 2, &sizeOrdinary));
    CHECK(size.cx == sizeOrdinary.cx && size.cy == sizeOrdinary.cy);
    CHECK(GetTextExtentPoint32A(hdc, psz, 14, &size) && size.cx > 0);
    CHECK(DrawTextA(hdc, psz, 14, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT) > 0 && rc.right == size.cx);
    CHECK(TextOutA(hdc, 0, 0, psz, 14) && ReleaseDC(hwndEdit, hdc) && ReleaseDC(hwndOrdinary, hdcOrdinary));
    CHECK(StarsCompatCaptureHierarchy(&snap));
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == StarsCompatWindowId(hwndEdit)) {
        CHECK(snap.rgWindow[i].fUtf8Text && strcmp(snap.rgWindow[i].pszText, psz) == 0); fFound = TRUE;
    }
    CHECK(fFound && FFilesystemTempDir(szSnapshotDir, sizeof(szSnapshotDir)));
    CHECK(snprintf(szSnapshotPath, sizeof(szSnapshotPath), "%s/hierarchy.json", szSnapshotDir) < (int)sizeof(szSnapshotPath));
    CHECK((pf = fopen(szSnapshotPath, "w+b")) && StarsCompatWriteHierarchy(&snap, pf));
    rewind(pf); cb = fread(szJson, 1, sizeof(szJson) - 1, pf); szJson[cb] = 0;
    CHECK(strstr(szJson, psz));
    CHECK(fclose(pf) == 0 && FFilesystemRemove(szSnapshotPath) && FFilesystemRemove(szSnapshotDir));
    StarsCompatFreeHierarchy(&snap);
    hwndPassword = CreateWindow("EDIT", "old", WS_CHILD | ES_PASSWORD, 0, 0, 100, 24,
                                hwnd, (HMENU)(INT_PTR)102, wc.hInstance, NULL);
    CHECK(hwndPassword);
    CompatTextInput(hwndPassword, "\xce\xa9"); Drain();
    CHECK(GetWindowText(hwndPassword, szText, sizeof(szText)) == 5 && !strcmp(szText, "\xce\xa9old") && !StarsCompatTextInputRejected(hwndPassword));
    CompatTextInput(hwndPassword, "\xc3\xa9"); Drain();
    CHECK(!StarsCompatTextInputRejected(hwndPassword));
    CHECK(SDL_SetClipboardText("\xe6\x9d\xb1\xe4\xba\xac")); SendMessage(hwndPassword, WM_PASTE, 0, 0);
    CHECK(!StarsCompatTextInputRejected(hwndPassword) && GetWindowText(hwndPassword, szText, sizeof(szText)) == 13 && FUtf8Valid(szText, strlen(szText)));
    SendMessage(hwndPassword, WM_CHAR, '\b', 0); CHECK(!StarsCompatTextInputRejected(hwndPassword));
    CHECK(TestPicker(hwnd) == 0);
    CHECK(DestroyWindow(hwnd)); StarsCompatProcessExit(); SDL_Quit();
    return 0;
}
