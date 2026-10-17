/* Compare generated resources and accelerator contracts with the Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../../res/resource.h"
#include "ui_manifest.h"
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cCommand, cInit, cPopup;
static WPARAM wCommand;
static BOOL fDisableInit, fDestroyInit, fInvalidCommand;
static LRESULT CALLBACK ResourceProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_COMMAND) { cCommand++; wCommand = w; if (l) fInvalidCommand = TRUE; return 0; }
    if (msg == WM_INITMENU) {
        cInit++;
        if (fDisableInit) EnableMenuItem(GetMenu(hwnd), IDM_HELP_CONTENTS2, MF_BYCOMMAND | MF_GRAYED);
        if (fDestroyInit) DestroyWindow(hwnd);
    }
    if (msg == WM_INITMENUPOPUP) cPopup++;
    return DefWindowProc(hwnd, msg, w, l);
}
static int TestTranslate(HWND hwnd, HACCEL haccel, BYTE mod, UINT msg, WORD key, WORD cmd) {
    BYTE rgb[256] = {0};
    MSG message = {0};
    int cBefore = cCommand;
    rgb[VK_SHIFT] = mod & FSHIFT ? 0x80 : 0;
    rgb[VK_CONTROL] = mod & FCONTROL ? 0x80 : 0;
    rgb[VK_MENU] = mod & FALT ? 0x80 : 0;
    CHECK(SetKeyboardState(rgb));
    message.hwnd = hwnd; message.message = msg; message.wParam = key;
    CHECK(TranslateAccelerator(hwnd, haccel, &message) == (cmd != 0));
    CHECK(cCommand == cBefore + (cmd != 0));
    if (cmd != 0) CHECK(wCommand == MAKEWPARAM(cmd, 1));
    return 0;
}
int main(void) {
    HINSTANCE hinst = GetModuleHandle(NULL);
    HRSRC hrsrc;
    const BYTE *rgb;
    DWORD cb, ib;
    HMENU hmenu, hsecond, hfile;
    HACCEL hmain, htitle, hcustom, hmenuaccel;
    ACCEL menuaccel = {FVIRTKEY, VK_F1, IDM_HELP_CONTENTS2};
    ACCEL rg[32], custom = {0, 'q', 501};
    WNDCLASS wc = {0};
    HWND hwnd, hwndExplicit, hwndChild;
    MSG msg = {0};
    BYTE rgkey[256] = {0};
    char sz[80];
    int i, cBefore, cTutorial = 0;
#ifndef _WIN32
    COMPATDLGTEMPLATE dlg = {0};
    WORD j;
    int cDialogs = 0, cControls = 0;
    const BYTE rgbExtended[26] = {1, 0, 255, 255};
#endif

    CHECK(hinst);
    for (i = 0; i < (int)(sizeof(rgUIManifest) / sizeof(rgUIManifest[0])); i++) {
        hrsrc = FindResource(hinst, rgUIManifest[i].pszName, rgUIManifest[i].pszType);
        CHECK(hrsrc);
        cb = SizeofResource(hinst, hrsrc);
        rgb = LockResource(LoadResource(hinst, hrsrc));
        if (cb != rgUIManifest[i].cb)
            fprintf(stderr, "resource %d size %lu / %lu\n", i, (unsigned long)cb, (unsigned long)rgUIManifest[i].cb);
        CHECK(rgb && cb == rgUIManifest[i].cb);
        if ((UINT_PTR)rgUIManifest[i].pszType == RT_TUTORIAL_HST ||
            (UINT_PTR)rgUIManifest[i].pszType == RT_TUTORIAL_M1 ||
            (UINT_PTR)rgUIManifest[i].pszType == RT_TUTORIAL_M2) cTutorial++;
        if (memcmp(rgb, rgUIManifest[i].rgb, cb)) {
            for (ib = 0; ib < cb; ib++) if (rgb[ib] != rgUIManifest[i].rgb[ib]) {
                fprintf(stderr, "resource %d first byte difference at %lu: %02x / %02x\n", i, (unsigned long)ib, rgb[ib], rgUIManifest[i].rgb[ib]); break;
            }
            return 1;
        }
#ifndef _WIN32
        if ((UINT_PTR)rgUIManifest[i].pszType == 4) {
            for (ib = 0; ib < cb; ib++) CHECK(!HmenuCompatTemplate(rgb, ib));
        } else if ((UINT_PTR)rgUIManifest[i].pszType == 5) {
            CHECK(FCompatDialogTemplate(rgb, cb, &dlg));
            CHECK(dlg.pszTitle && dlg.pszFont && dlg.cPoints == 8);
            for (j = 0; j < dlg.cItems; j++) {
                CHECK(dlg.rgItems[j].pszClass && dlg.rgItems[j].pszTitle);
                CHECK(dlg.rgItems[j].cbCreate == 0);
            }
            cDialogs++; cControls += dlg.cItems;
            CompatFreeDialogTemplate(&dlg);
            for (ib = 0; ib < cb; ib++) CHECK(!FCompatDialogTemplate(rgb, ib, &dlg));
        }
#endif
    }
    CHECK(cTutorial == 3);
#ifndef _WIN32
    CHECK(cDialogs == 37 && cControls == 347);
    CHECK(!FCompatDialogTemplate(rgbExtended, sizeof(rgbExtended), &dlg));
#endif
    CHECK(!LoadMenu(hinst, "missing"));
    CHECK(!LoadAccelerators(hinst, MAKEINTRESOURCE(65000)));
    hmenu = LoadMenu(hinst, "StarsMenu"); hsecond = LoadMenu(hinst, "STARSMENU");
    CHECK(hmenu && hsecond && hmenu != hsecond);
    CHECK(GetMenuItemCount(hmenu) == 6);
    hfile = GetSubMenu(hmenu, 0);
    CHECK(hfile && GetMenuItemID(hfile, 0) == IDM_TOOL_NEW_GAME);
    CHECK(GetMenuString(hfile, 0, sz, sizeof(sz), MF_BYPOSITION) > 0);
    CHECK(strstr(sz, "New") != NULL);
    CHECK(EnableMenuItem(hmenu, IDM_TOOL_NEW_GAME, MF_BYCOMMAND | MF_GRAYED) != -1);
    CHECK(!(GetMenuState(hsecond, IDM_TOOL_NEW_GAME, MF_BYCOMMAND) & MF_GRAYED));
    CHECK(DestroyMenu(hmenu) && IsMenu(hsecond));
    hmain = LoadAccelerators(hinst, MAKEINTRESOURCE(IDA_MAIN));
    htitle = LoadAccelerators(hinst, MAKEINTRESOURCE(IDA_TITLE));
    CHECK(hmain && htitle);
    CHECK(CopyAcceleratorTable(hmain, NULL, 0) == 18);
    CHECK(CopyAcceleratorTable(hmain, rg, 32) == 18);
    CHECK(rg[0].fVirt == (FVIRTKEY | FNOINVERT | FCONTROL) && rg[0].key == 'A' && rg[0].cmd == IDM_TURN_SAVE_SUBMIT);
    CHECK(rg[17].key == VK_F12 && rg[17].fVirt == (FVIRTKEY | FNOINVERT | FSHIFT));
    CHECK(CopyAcceleratorTable(htitle, rg, 2) == 2 && rg[0].cmd == IDM_TITLE_NEW_GAME);
    wc.lpfnWndProc = ResourceProc; wc.hInstance = hinst; wc.lpszClassName = "ResourceContract"; wc.lpszMenuName = "StarsMenu";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Resources", WS_OVERLAPPEDWINDOW, 10, 10, 300, 180, NULL, NULL, hinst, NULL);
    CHECK(hwnd && GetMenu(hwnd) && GetMenuItemCount(GetMenu(hwnd)) == 6);
    hwndExplicit = CreateWindow(wc.lpszClassName, "Explicit", WS_OVERLAPPEDWINDOW, 10, 10, 300, 180, NULL, hsecond, hinst, NULL);
    CHECK(hwndExplicit && GetMenu(hwndExplicit) == hsecond);
    hwndChild = CreateWindow(wc.lpszClassName, "Child", WS_CHILD, 0, 0, 20, 20, hwnd, (HMENU)(UINT_PTR)42, hinst, NULL);
    CHECK(hwndChild && GetDlgCtrlID(hwndChild) == 42);
    CHECK(CopyAcceleratorTable(hmain, rg, 32) == 18);
    for (i = 0; i < 18; i++) CHECK(TestTranslate(hwnd, hmain, rg[i].fVirt & (FSHIFT | FCONTROL | FALT), WM_KEYDOWN, rg[i].key, rg[i].cmd) == 0);
    CHECK(CopyAcceleratorTable(htitle, rg, 32) == 4);
    for (i = 0; i < 4; i++) CHECK(TestTranslate(hwnd, htitle, FALT, WM_SYSKEYDOWN, rg[i].key, rg[i].cmd) == 0);
    CHECK(TestTranslate(hwnd, hmain, FCONTROL, WM_KEYDOWN, 'A', IDM_TURN_SAVE_SUBMIT) == 0);
    CHECK(TestTranslate(hwnd, hmain, 0, WM_KEYDOWN, 'A', 0) == 0);
    CHECK(TestTranslate(hwnd, hmain, FCONTROL, WM_KEYUP, 'A', 0) == 0);
    CHECK(TestTranslate(hwnd, hmain, 0, WM_KEYDOWN, VK_F12, IDM_DEBUG_GEN_10_TURNS) == 0);
    CHECK(TestTranslate(hwnd, hmain, FCONTROL, WM_KEYDOWN, VK_F12, IDM_DEBUG_GEN_100_TURNS) == 0);
    CHECK(TestTranslate(hwnd, hmain, FSHIFT, WM_KEYDOWN, VK_F12, IDM_DEBUG_GEN_1000_TURNS) == 0);
    CHECK(TestTranslate(hwnd, htitle, FALT, WM_SYSKEYDOWN, 'N', IDM_TITLE_NEW_GAME) == 0);
    hcustom = CreateAcceleratorTable(&custom, 1); custom.cmd = 502;
    CHECK(hcustom && CopyAcceleratorTable(hcustom, rg, 1) == 1 && rg[0].cmd == 501);
    CHECK(TestTranslate(hwnd, hcustom, 0, WM_CHAR, 'q', 501) == 0);
    CHECK(TestTranslate(hwnd, hcustom, 0, WM_KEYDOWN, 'q', 0) == 0);
    CHECK(SetKeyboardState(rgkey));
    msg.hwnd = hwnd; msg.message = WM_KEYDOWN; msg.wParam = VK_F1;
    hmenuaccel = CreateAcceleratorTable(&menuaccel, 1);
    CHECK(hmenuaccel);
    cInit = cPopup = 0; cBefore = cCommand;
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg));
    CHECK(cCommand == cBefore + 1 && cInit == 1 && cPopup == 1);
    fDisableInit = TRUE; cBefore = cCommand;
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg) && cCommand == cBefore);
    fDisableInit = FALSE;
    EnableMenuItem(GetMenu(hwnd), IDM_HELP_CONTENTS2, MF_BYCOMMAND | MF_ENABLED);
    EnableWindow(hwnd, FALSE);
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg) && cCommand == cBefore);
    EnableWindow(hwnd, TRUE);
    SetCapture(hwnd);
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg) && cCommand == cBefore);
    CHECK(ReleaseCapture());
    ShowWindow(hwnd, SW_SHOWMINIMIZED);
#ifndef _WIN32
    // The dummy SDL driver has no native minimization; exercise logical state.
    if (!IsIconic(hwnd)) CompatWindowState(hwnd, SW_SHOWMINIMIZED, FALSE);
#endif
    CHECK(IsIconic(hwnd));
    cInit = cPopup = 0;
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg) && cCommand == cBefore);
    CHECK(cInit == 1 && cPopup == 1);
    CHECK(TestTranslate(hwnd, hmain, 0, WM_KEYDOWN, VK_F12, IDM_DEBUG_GEN_10_TURNS) == 0);
    ShowWindow(hwnd, SW_RESTORE);
#ifndef _WIN32
    if (IsIconic(hwnd)) CompatWindowState(hwnd, SW_SHOWNORMAL, FALSE);
#endif
    fDestroyInit = TRUE;
    CHECK(TranslateAccelerator(hwnd, hmenuaccel, &msg) && !IsWindow(hwnd));
    CHECK(DestroyWindow(hwndExplicit));
    CHECK(!IsMenu(hsecond));
    CHECK(DestroyAcceleratorTable(hcustom));
    CHECK(DestroyAcceleratorTable(hmenuaccel));
    CHECK(DestroyAcceleratorTable(hmain));
    CHECK(!fInvalidCommand);
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("UI resource and accelerator contracts passed");
    return 0;
}
