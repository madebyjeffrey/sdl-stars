/* Real modal file-picker actions and returned OPENFILENAME fields. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <commdlg.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#define getcwd _getcwd
#define chdir _chdir
#define getpid _getpid
#define rmdir _rmdir
#define chmod _chmod
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#include <stars_compat.h>
#include "internal.h"
#define MKDIR(p) mkdir(p, 0700)
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
#define EXPECT(f) do { if (!(f)) { fprintf(stderr, "callback line %d mode %d: %s\n", __LINE__, mode, #f); cErrors++; } } while (0)
static int cErrors, mode, phase, cTicks, cPrompts;
static HWND hwndOwner, hwndOther;
static char szDir[2048], szCwd[4096];
static const char szFilter[] = "Stars files\0*.M*;*.HST;*.R*\0Host files\0*.HST\0All files\0*.*\0";
static HWND FindCaption(LPCSTR psz) {
    HWND hwnd;
    char sz[128];
    for (hwnd = GetWindow(hwndOwner, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        GetWindowText(hwnd, sz, sizeof(sz));
        if (strcmp(sz, psz) == 0 && IsWindowVisible(hwnd)) return hwnd;
    }
    return NULL;
}
#ifdef _WIN32
static void DumpWindow(HWND hwnd, int depth) {
    HWND child;
    char szClass[64], szText[128];
    GetClassName(hwnd, szClass, sizeof(szClass)); GetWindowText(hwnd, szText, sizeof(szText));
    fprintf(stderr, "%*s%s id=%x text=%s\n", depth * 2, "", szClass, GetDlgCtrlID(hwnd), szText);
    if (depth < 4) for (child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) DumpWindow(child, depth + 1);
}
#endif
static HWND HwndNameEdit(HWND hwnd) {
    HWND edit = GetDlgItem(hwnd, 0x480);
#ifdef _WIN32
    HWND combo;
    char szClass[64];
    if (!edit) {
        combo = GetDlgItem(hwnd, 0x47c);
        while (combo) {
            GetClassName(combo, szClass, sizeof(szClass));
            if (strcmp(szClass, "Edit") == 0) { edit = combo; break; }
            combo = GetWindow(combo, GW_CHILD);
        }
    }
#endif
    return edit;
}
static void Choose(HWND hwnd, LPCSTR psz) {
    EXPECT(SetWindowText(HwndNameEdit(hwnd), psz));
    EXPECT(PostMessage(HwndNameEdit(hwnd), WM_KEYDOWN, VK_RETURN, 0));
}
#ifndef _WIN32
static int FindEntry(HWND hwnd, LPCSTR psz) {
    HWND list = GetDlgItem(hwnd, 0x460);
    int i, c = (int)SendMessage(list, LB_GETCOUNT, 0, 0);
    char sz[4096];
    for (i = 0; i < c; i++) {
        SendMessage(list, LB_GETTEXT, i, (LPARAM)sz);
        if (strcmp(sz, psz) == 0) return i;
    }
    return -1;
}
#endif
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND dlg, prompt;
#ifndef _WIN32
    int i;
#endif
    if (msg != WM_TIMER) return DefWindowProc(hwnd, msg, w, l);
    if (++cTicks > 1000) { fprintf(stderr, "file-picker timer deadline mode %d phase %d\n", mode, phase); exit(2); }
    dlg = FindCaption("Stars File Contract");
#ifdef _WIN32
    if (cTicks == 100) {
        HWND top;
        for (top = GetWindow(hwndOwner, GW_HWNDFIRST); top; top = GetWindow(top, GW_HWNDNEXT)) if (IsWindowVisible(top)) DumpWindow(top, 0);
    }
#endif
#ifndef _WIN32
    prompt = FindCaption("New folder");
    if (mode == 12 && prompt) {
        EXPECT(!IsWindowEnabled(dlg));
        if (phase == 1) { phase = 2; PostMessage(prompt, WM_COMMAND, IDCANCEL, 0); }
        else if (phase == 3) {
            EXPECT(SetWindowText(GetDlgItem(prompt, 0x480), "created"));
            phase = 4; PostMessage(prompt, WM_COMMAND, IDOK, 0);
        }
        return 0;
    }
    prompt = FindCaption("Confirm save");
    if (prompt) {
        cPrompts++;
        EXPECT(!IsWindowEnabled(dlg));
        EXPECT(PostMessage(prompt, WM_COMMAND, phase == 1 ? IDNO : IDYES, 0));
        phase++; return 0;
    }
    prompt = FindCaption("File selection");
    if (prompt) { cPrompts++; EXPECT(PostMessage(prompt, WM_COMMAND, IDOK, 0)); phase = 2; return 0; }
#else
    prompt = NULL; (void)prompt;
#endif
    if (!dlg || !HwndNameEdit(dlg)) return 0;
    EXPECT(!IsWindowEnabled(hwndOwner)); EXPECT(IsWindowEnabled(hwndOther));
    if (phase == 0) {
        phase = 1;
        if (mode == 1) { EXPECT(PostMessage(dlg, WM_KEYDOWN, VK_ESCAPE, 0)); return 0; }
#ifndef _WIN32
        if (mode == 12) { PostMessage(dlg, WM_COMMAND, 0x404, 0); return 0; }
        if (mode == 11) {
            RECT rc, rcList;
            POINT pt;
            EXPECT(!IsWindowEnabled(GetDlgItem(dlg, 0x401)));
            EXPECT(FindEntry(dlg, ".hidden.M1") < 0);
            SendDlgItemMessage(dlg, 0x403, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(dlg, WM_COMMAND, MAKEWPARAM(0x403, BN_CLICKED), (LPARAM)GetDlgItem(dlg, 0x403));
            EXPECT(FindEntry(dlg, ".hidden.M1") >= 0);
            SendMessage(dlg, WM_COMMAND, 0x402, 0);
            EXPECT(IsWindowEnabled(GetDlgItem(dlg, 0x401)));
            SendMessage(dlg, WM_COMMAND, 0x401, 0);
            EXPECT(FindEntry(dlg, "sample.M1") >= 0 && !IsWindowEnabled(GetDlgItem(dlg, 0x401)));
            EXPECT(SetWindowPos(dlg, NULL, 0, 0, 800, 560, SWP_NOMOVE | SWP_NOZORDER));
            EXPECT(GetClientRect(dlg, &rc));
            EXPECT(GetWindowRect(GetDlgItem(dlg, 0x460), &rcList));
            pt = (POINT){rcList.right, rcList.bottom}; EXPECT(ScreenToClient(dlg, &pt));
            EXPECT(pt.x == rc.right - 12 && pt.y == rc.bottom - 144);
            if (getenv("STARS_FILE_DIALOG_CAPTURE")) {
                HWND child;
                for (child = GetWindow(dlg, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) UpdateWindow(child);
                EXPECT(UpdateWindow(dlg) && FCompatPresent(dlg));
                EXPECT(SDL_SaveBMP(dlg->lpComposite, getenv("STARS_FILE_DIALOG_CAPTURE")));
            }
            EXPECT(SetWindowText(GetDlgItem(dlg, 0x440), "sub"));
            SetFocus(GetDlgItem(dlg, 0x440));
            EXPECT(PostMessage(GetDlgItem(dlg, 0x440), WM_KEYDOWN, VK_RETURN, 0));
            return 0;
        }
        if (mode == 4) {
            EXPECT(FindEntry(dlg, "sample.M1") >= 0 && FindEntry(dlg, "race.r16") >= 0 && FindEntry(dlg, "host.HST") >= 0);
            EXPECT(FindEntry(dlg, "other.txt") < 0 && FindEntry(dlg, "[sub]") >= 0);
            i = FindEntry(dlg, "[sub]");
            SendDlgItemMessage(dlg, 0x460, LB_SETCURSEL, i, 0);
            PostMessage(dlg, WM_COMMAND, MAKEWPARAM(0x460, LBN_DBLCLK), (LPARAM)GetDlgItem(dlg, 0x460)); return 0;
        }
        if (mode == 5) {
            SendDlgItemMessage(dlg, 0x470, CB_SETCURSEL, 2, 0);
            SendMessage(dlg, WM_COMMAND, MAKEWPARAM(0x470, CBN_SELCHANGE), (LPARAM)GetDlgItem(dlg, 0x470));
            EXPECT(FindEntry(dlg, "other.txt") >= 0 && FindEntry(dlg, "README") >= 0); Choose(dlg, "other.txt"); return 0;
        }
        if (mode == 6) { Choose(dlg, "missing.M2"); return 0; }
        if (mode == 8) { Choose(dlg, "readonly.r1"); return 0; }
        if (mode == 9) { DestroyWindow(hwndOwner); return 0; }
        if (mode == 10) { PostQuitMessage(73); return 0; }
        if (mode == 3) { Choose(dlg, "race.r16"); return 0; }
#endif
        Choose(dlg, mode == 2 ? "newrace" : "sample.M1");
    }
#ifndef _WIN32
    else if (mode == 12 && phase == 2 && IsWindowEnabled(dlg)) {
        EXPECT(FindEntry(dlg, "[created]") < 0);
        phase = 3; PostMessage(dlg, WM_COMMAND, 0x404, 0);
    } else if (mode == 12 && phase == 4 && IsWindowEnabled(dlg)) {
        char szPath[4096];
        GetWindowText(GetDlgItem(dlg, 0x440), szPath, sizeof(szPath));
        EXPECT(strstr(szPath, "/created") != NULL && FindEntry(dlg, "sample.M1") < 0);
        phase = 5; SetFocus(HwndNameEdit(dlg)); Choose(dlg, "../sample.M1");
    }
    else if (mode == 11 && phase == 1 && FindEntry(dlg, "child.R2") >= 0) {
        EXPECT(IsWindowEnabled(GetDlgItem(dlg, 0x401)));
        SendMessage(dlg, WM_COMMAND, 0x400, 0);
        EXPECT(FindEntry(dlg, "sample.M1") >= 0);
        SendMessage(dlg, WM_COMMAND, 0x401, 0);
        EXPECT(FindEntry(dlg, "child.R2") >= 0);
        phase++; SetFocus(HwndNameEdit(dlg)); Choose(dlg, "child.R2");
    }
    else if (mode == 4 && phase == 1 && FindEntry(dlg, "child.R2") >= 0) { phase++; Choose(dlg, "child.R2"); }
    else if (mode == 3 && phase == 2 && IsWindowEnabled(dlg)) { phase++; Choose(dlg, "race.r16"); }
    else if ((mode == 6 || mode == 8) && phase == 2 && IsWindowEnabled(dlg)) { phase++; Choose(dlg, mode == 6 ? "sample.M1" : "writable"); }
#endif
    return 0;
}
static int FWriteFileFixture(LPCSTR psz) {
    char sz[4096];
    FILE *file;
    snprintf(sz, sizeof(sz), "%s/%s", szDir, psz);
    file = fopen(sz, "wb"); if (!file) return 0;
    fputs("fixture\n", file); return fclose(file) == 0;
}
static void InitOFN(OPENFILENAME *lp, char *psz, char *pszTitle) {
    memset(lp, 0, sizeof(*lp)); lp->lStructSize = sizeof(*lp); lp->hwndOwner = hwndOwner;
    lp->lpstrFilter = szFilter; lp->nFilterIndex = 1; lp->lpstrInitialDir = szDir;
    lp->lpstrFile = psz; lp->nMaxFile = 4096; lp->lpstrFileTitle = pszTitle; lp->nMaxFileTitle = 128;
    lp->lpstrTitle = "Stars File Contract"; lp->Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    mode = phase = cTicks = cPrompts = 0;
}
int main(void) {
    WNDCLASS wc = {0};
    OPENFILENAME ofn;
    char szFile[4096] = "", szTitle[128] = "", sz[4096], szAfter[4096];
    HWND hwndFocus;
    MSG msg;
    WORD needed;
    int i;
    const char *rgFiles[] = {"sample.M1", "race.r16", "host.HST", "other.txt", "README", "sub/child.R2", "readonly.r1", ".hidden.M1"};
    CHECK(getcwd(szCwd, sizeof(szCwd)));
    snprintf(szDir, sizeof(szDir), "stars-filedialog-test-%d-%lu", getpid(), (unsigned long)GetTickCount()); CHECK(MKDIR(szDir) == 0);
    CHECK(chdir(szDir) == 0); CHECK(getcwd(szDir, sizeof(szDir))); CHECK(chdir(szCwd) == 0);
    snprintf(sz, sizeof(sz), "%s/sub", szDir); CHECK(MKDIR(sz) == 0);
    for (i = 0; i < 8; i++) CHECK(FWriteFileFixture(rgFiles[i]));
    wc.lpszClassName = "FileDialogOwner"; wc.lpfnWndProc = OwnerProc; wc.hInstance = GetModuleHandle(NULL);
    CHECK(RegisterClass(&wc));
    hwndOwner = CreateWindow(wc.lpszClassName, "File owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 260, 180, NULL, NULL, wc.hInstance, NULL);
    hwndOther = CreateWindow(wc.lpszClassName, "Other file owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 300, 0, 200, 150, NULL, NULL, wc.hInstance, NULL);
    CHECK(hwndOwner && hwndOther);
    hwndFocus = CreateWindow("Edit", "prior", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 150, 30, hwndOwner, NULL, wc.hInstance, NULL); CHECK(hwndFocus);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    SetFocus(hwndFocus); CHECK(SetTimer(hwndOwner, 1, 20, NULL));
    InitOFN(&ofn, szFile, szTitle); ofn.Flags |= OFN_FILEMUSTEXIST;
    CHECK(GetOpenFileName(&ofn)); CHECK(CommDlgExtendedError() == 0);
    CHECK(strcmp(szFile + ofn.nFileOffset, "sample.M1") == 0 && strcmp(szTitle, "sample.M1") == 0);
    CHECK(strcmp(szFile + ofn.nFileExtension, "M1") == 0 && ofn.nFilterIndex == 1);
    CHECK(IsWindowEnabled(hwndOwner)); CHECK(getcwd(szAfter, sizeof(szAfter)) && strcmp(szCwd, szAfter) == 0);
#ifndef _WIN32
    CHECK(GetFocus() == hwndFocus);
#endif
    InitOFN(&ofn, szFile, szTitle); strcpy(szFile, "unchanged"); mode = 1;
    CHECK(!GetOpenFileName(&ofn) && CommDlgExtendedError() == 0); CHECK(strcmp(szFile, "unchanged") == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 2; ofn.lpstrDefExt = "r1";
    CHECK(GetSaveFileName(&ofn)); CHECK(strcmp(szFile + ofn.nFileOffset, "newrace.r1") == 0);
    CHECK(strcmp(szFile + ofn.nFileExtension, "r1") == 0); CHECK(fopen(szFile, "rb") == NULL);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; ofn.nMaxFile = 8; mode = 7;
    CHECK(!GetOpenFileName(&ofn)); CHECK(CommDlgExtendedError() == FNERR_BUFFERTOOSMALL);
    memcpy(&needed, szFile, sizeof(needed)); CHECK(needed > ofn.nMaxFile);
#ifndef _WIN32
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 3; ofn.Flags |= OFN_OVERWRITEPROMPT;
    CHECK(GetSaveFileName(&ofn)); CHECK(cPrompts == 2 && phase == 4); CHECK(strcmp(szFile + ofn.nFileOffset, "race.r16") == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 4;
    CHECK(GetOpenFileName(&ofn)); CHECK(strcmp(szFile + ofn.nFileOffset, "child.R2") == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 5;
    CHECK(GetOpenFileName(&ofn)); CHECK(ofn.nFilterIndex == 3 && strcmp(szTitle, "other.txt") == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 6; ofn.Flags |= OFN_FILEMUSTEXIST;
    CHECK(GetOpenFileName(&ofn)); CHECK(cPrompts > 0 && strcmp(szTitle, "sample.M1") == 0);
    snprintf(sz, sizeof(sz), "%s/readonly.r1", szDir); CHECK(chmod(sz, 0400) == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 8; ofn.Flags |= OFN_NOREADONLYRETURN;
    CHECK(GetSaveFileName(&ofn)); CHECK(cPrompts > 0 && strcmp(szTitle, "writable") == 0);
    CHECK(chmod(sz, 0600) == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; ofn.Flags &= ~OFN_NOCHANGEDIR;
    CHECK(GetOpenFileName(&ofn)); CHECK(getcwd(szAfter, sizeof(szAfter)) && strcmp(szDir, szAfter) == 0); CHECK(chdir(szCwd) == 0);
    InitOFN(&ofn, szFile, szTitle); ofn.lStructSize--; CHECK(!GetOpenFileName(&ofn) && CommDlgExtendedError() == CDERR_STRUCTSIZE);
    InitOFN(&ofn, szFile, szTitle); ofn.Flags |= 0x20; CHECK(!GetSaveFileName(&ofn) && CommDlgExtendedError() == CDERR_INITIALIZATION);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 11;
    CHECK(GetOpenFileName(&ofn)); CHECK(strcmp(szTitle, "child.R2") == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 12;
    CHECK(GetOpenFileName(&ofn)); CHECK(phase == 5 && strcmp(szTitle, "sample.M1") == 0);
    snprintf(sz, sizeof(sz), "%s/created", szDir); CHECK(rmdir(sz) == 0);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 10;
    CHECK(!GetOpenFileName(&ofn) && CommDlgExtendedError() == 0);
    CHECK(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_QUIT && msg.wParam == 73);
    InitOFN(&ofn, szFile, szTitle); szFile[0] = 0; mode = 9;
    CHECK(!GetOpenFileName(&ofn)); CHECK(!IsWindow(hwndOwner));
#else
    KillTimer(hwndOwner, 1); DestroyWindow(hwndOwner);
#endif
    DestroyWindow(hwndOther);
    for (i = 0; i < 8; i++) { snprintf(sz, sizeof(sz), "%s/%s", szDir, rgFiles[i]); CHECK(remove(sz) == 0); }
    snprintf(sz, sizeof(sz), "%s/sub", szDir); CHECK(rmdir(sz) == 0); CHECK(rmdir(szDir) == 0);
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    CHECK(cErrors == 0); puts("File-picker selection, cancellation, defaults, offsets and buffers passed"); return 0;
}
