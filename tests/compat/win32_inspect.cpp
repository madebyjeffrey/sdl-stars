/* Snapshot lifetime, logical IDs and a built-in viewer without desktop automation. */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <iterator>
#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "../../res/resource.h"
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static int cMessages, cDialogs, cDialogErrors, cCommands;
static uint64_t rgidDialog[2];
static SDL_WindowID rgidNative[2];
static INT_PTR  DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    STARSCOMPATSNAPSHOT snap = {};
    size_t i;
    BOOL fFound = 0;
    (void)w; (void)l;
    if (msg != win32::WM_INITDIALOG) return 0;
    rgidDialog[cDialogs] = StarsCompatWindowId(hwnd);
    rgidNative[cDialogs] = LpCompatWindow(hwnd)->id;
    if (!StarsCompatCaptureHierarchy(&snap)) cDialogErrors++;
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == rgidDialog[cDialogs]) {
        fFound = snap.rgWindow[i].fDialog && snap.rgWindow[i].idOwner != 0;
    }
    if (!fFound) cDialogErrors++;
    StarsCompatFreeHierarchy(&snap); cDialogs++;
    win32::EndDialog(hwnd, 2);
    return 1;
}
static LRESULT  WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    cMessages++;
    if (msg == win32::WM_COMMAND && LOWORD(w) == 7 && HIWORD(w) == win32::BN_CLICKED) cCommands++;
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
static const STARSCOMPATWINDOW *LpItem(const STARSCOMPATSNAPSHOT *lpsnap, uint64_t id) {
    size_t i;
    for (i = 0; i < lpsnap->cWindow; i++) if (lpsnap->rgWindow[i].id == id) return &lpsnap->rgWindow[i];
    return NULL;
}
static int SDLCALL Worker(void *lpv) {
    STARSCOMPATSNAPSHOT snap = {};
    uint64_t id = *(uint64_t *)lpv;
    if (StarsCompatCaptureHierarchy(&snap) || StarsCompatResolveWindow(id) || StarsCompatShowInspector()) return 1;
    return 0;
}
static int TestStyles(void) {
    struct STYLECASE { UINT iControl; BOOL fDialog; DWORD dw; const char *psz, *pszAbsent; };
    static const struct STYLECASE rg[] = {
        { 0, 0, win32::WS_OVERLAPPEDWINDOW, "WS_CAPTION", "WS_GROUP" },
        { 1, 0, win32::WS_CHILD | win32::WS_GROUP | win32::BS_AUTOCHECKBOX | win32::BS_CENTER, "BS_AUTOCHECKBOX | BS_CENTER", "BS_PUSHBUTTON" },
        { 2, 0, win32::WS_CHILD | win32::SS_CENTER | win32::SS_ENDELLIPSIS, "SS_CENTER | SS_ENDELLIPSIS", "SS_LEFT" },
        { 3, 0, win32::WS_CHILD | win32::ES_CENTER | win32::ES_PASSWORD | win32::ES_NUMBER, "ES_CENTER | ES_PASSWORD | ES_NUMBER", "ES_LEFT" },
        { 4, 0, win32::WS_CHILD | win32::LBS_EXTENDEDSEL | win32::LBS_HASSTRINGS, "LBS_HASSTRINGS | LBS_EXTENDEDSEL", "CBS_" },
        { 5, 0, win32::WS_CHILD | win32::SBS_VERT | win32::SBS_LEFTALIGN, "SBS_VERT | SBS_LEFTALIGN", "SBS_TOPALIGN" },
        { 5, 0, win32::WS_CHILD | win32::SBS_SIZEBOX | win32::SBS_SIZEBOXBOTTOMRIGHTALIGN, "SBS_SIZEBOXBOTTOMRIGHTALIGN", "SBS_BOTTOMALIGN" },
        { 6, 0, win32::WS_CHILD | win32::CBS_DROPDOWNLIST | win32::CBS_SORT, "CBS_DROPDOWNLIST | CBS_SORT", "CBS_DROPDOWN |" },
        { 0, 1, win32::WS_POPUP | win32::DS_SETFONT | win32::DS_MODALFRAME, "DS_SETFONT | DS_MODALFRAME", "BS_" },
        { 1, 0, win32::WS_CHILD | 0x000c, "0x0000000c", "BS_PUSHBUTTON" },
    };
    STARSCOMPATWINDOW item = {};
    STARSCOMPATSNAPSHOT snap = { &item, 1 };
    size_t i, cb;
    NATIVEFILE *lpfile;
    char sz[8192], szClass[] = "StyleTest", szText[] = "";
    item.pszClass = szClass; item.pszText = szText;
    item.dwExStyle = win32::WS_EX_TOOLWINDOW | win32::WS_EX_NOACTIVATE | UINT32_C(0x40000000);
    for (i = 0; i < std::size(rg); i++) {
        item.iControl = rg[i].iControl; item.fDialog = rg[i].fDialog; item.dwStyle = rg[i].dw;
        lpfile = PfNativeTemporary(); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile));
        ISeekNativeFile(lpfile, 0, NATIVESEEKORIGIN::begin); cb = CReadNative(std::span{sz}.first(sizeof(sz) - 1), lpfile); sz[cb] = 0; ICloseNativeFile(lpfile);
        CHECK(strstr(sz, rg[i].psz) && !strstr(sz, rg[i].pszAbsent));
        CHECK(strstr(sz, "WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | 0x40000000"));
    }
    return 0;
}
int main(int argc, char **argv) {
    WNDCLASS wc = {};
    HWND hwnd, hwndChild, hwndNested, hwndHidden, hwndOwned, hwndDuplicate, hwndViewer, hwndNew, hwndPaint;
    uint64_t id, idChild, idNested, idViewer;
    STARSCOMPATSNAPSHOT snap = {}, snapAfter = {};
    const STARSCOMPATWINDOW *lp;
    NATIVEFILE *lpfile;
    char sz[8192];
    size_t cb, i;
    int cBefore, iStatus;
    SDL_Thread *lpthread;
    SDL_Event event = {};
    BYTE r, g, b, a;

    CHECK(TestStyles() == 0);
    wc.lpfnWndProc = WndProc; wc.lpszClassName = "InspectTest";
    CHECK(win32::RegisterClassA(&wc));
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Frame \"quote\" \\ \342\202\254\r\n", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                        10, 20, 320, 240, NULL, NULL, NULL, NULL);
    hwndChild = win32::CreateWindowExA(0, "Button", "Action", win32::WS_CHILD | win32::WS_VISIBLE | win32::BS_PUSHBUTTON,
                             4, 5, 90, 30, hwnd, (HMENU)(INT_PTR)7, NULL, NULL);
    hwndNested = win32::CreateWindowExA(0, "Edit", "secret", win32::WS_CHILD | win32::WS_VISIBLE | win32::ES_PASSWORD,
                              1, 2, 50, 20, hwndChild, (HMENU)(INT_PTR)8, NULL, NULL);
    hwndHidden = win32::CreateWindowExA(0, "Static", "Hidden", win32::WS_CHILD | win32::WS_DISABLED, 0, 0, 30, 20,
                              hwnd, (HMENU)(INT_PTR)9, NULL, NULL);
    hwndOwned = win32::CreateWindowExA(0, wc.lpszClassName, "Owned", win32::WS_POPUP | win32::WS_VISIBLE, 20, 20, 60, 50, hwnd, NULL, NULL, NULL);
    CHECK(hwnd && hwndChild && hwndNested && hwndHidden && hwndOwned);
    CHECK(!strcmp(SDL_GetWindowTitle(LpCompatWindow(hwnd)->lpWindow.get()), "Frame \"quote\" \\ \342\202\254\r\n"));
    id = StarsCompatWindowId(hwnd); idChild = StarsCompatWindowId(hwndChild); idNested = StarsCompatWindowId(hwndNested);
    CHECK(id != 0 && idChild > id && idNested > idChild);
    win32::SetFocus(hwndChild); win32::SetCapture(hwndChild);
    cBefore = cMessages;
    CHECK(StarsCompatCaptureHierarchy(&snap) && snap.cWindow == 5);
    CHECK(cMessages == cBefore);
    CHECK(!StarsCompatCaptureHierarchy(&snap));
    CHECK(StarsCompatResolveWindow(idChild) == hwndChild);
    CHECK(StarsCompatFindControl(id, 7, "button") == idChild);
    CHECK(StarsCompatFindControl(id, 8, NULL) == 0);
    CHECK(StarsCompatFindControl(idChild, 8, "Edit") == idNested);
    lp = LpItem(&snap, idChild);
    CHECK(lp && lp->idParent == id && lp->idOwner == 0 && lp->idControl == 7 && lp->fFocused && lp->fCaptured);
    CHECK(lp->fVisible && lp->fEnabled && !strcmp(lp->pszClass, "Button"));
    CHECK(lp->rcClient.right == 90 && lp->rcClient.bottom == 30);
    lp = LpItem(&snap, idNested);
    CHECK(lp && !strcmp(lp->pszText, "[password]"));
    lp = LpItem(&snap, StarsCompatWindowId(hwndHidden));
    CHECK(lp && !lp->fVisible && !lp->fEnabled);
    lp = LpItem(&snap, StarsCompatWindowId(hwndOwned));
    CHECK(lp && lp->idParent == 0 && lp->idOwner == id);
    lpthread = SDL_CreateThread(Worker, "inspection-thread-contract", &id);
    CHECK(lpthread); SDL_WaitThread(lpthread, &iStatus); CHECK(iStatus == 0);
    hwndDuplicate = win32::CreateWindowExA(0, "Static", "Duplicate", win32::WS_CHILD, 0, 0, 1, 1, hwnd, (HMENU)(INT_PTR)7, NULL, NULL);
    CHECK(hwndDuplicate && StarsCompatFindControl(id, 7, NULL) == 0);
    CHECK(StarsCompatFindControl(id, 7, "Button") == idChild);
    CHECK(win32::DestroyWindow(hwndDuplicate));
    // A JSON consumer must preserve IDs above JavaScript's exact-integer range.
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == StarsCompatWindowId(hwndOwned))
        snap.rgWindow[i].id = UINT64_C(9007199254740993);
    CHECK(StarsCompatBeginDialogSequence(hwnd));
    CHECK(DialogBoxParamA(NULL, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
    CHECK(!StarsCompatResolveWindow(rgidDialog[0]));
    CHECK(DialogBoxParamA(NULL, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
    CHECK(cDialogs == 2 && cDialogErrors == 0 && rgidDialog[1] > rgidDialog[0]);
    CHECK(rgidNative[0] == rgidNative[1] && !StarsCompatResolveWindow(rgidDialog[1]));
    StarsCompatEndDialogSequence();
    lpfile = PfNativeTemporary(); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile));
    ISeekNativeFile(lpfile, 0, NATIVESEEKORIGIN::begin); cb = CReadNative(std::span{sz}.first(sizeof(sz) - 1), lpfile); sz[cb] = 0; ICloseNativeFile(lpfile);
    CHECK(strstr(sz, "\\\"quote\\\"") && strstr(sz, "\\\\") && strstr(sz, "\\u000d\\u000a"));
    CHECK(strstr(sz, "\342\202\254") && strstr(sz, "[password]") && !strstr(sz, "secret"));
    if (argc == 2) { lpfile = PfNativeOpen(argv[1], NATIVEFILEMODE::writeTruncate); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile)); CHECK(ICloseNativeFile(lpfile) == 0); }
    win32::ReleaseCapture();
    hwndViewer = StarsCompatShowInspector(); CHECK(hwndViewer && win32::IsWindowVisible(hwndViewer));
    idViewer = StarsCompatWindowId(hwndViewer);
    CHECK(StarsCompatShowInspector() == hwndViewer);
    CHECK(win32::GetDlgItem(hwndViewer, 1) && win32::GetDlgItem(hwndViewer, 2) && win32::GetDlgItem(hwndViewer, 3));
    CHECK(win32::SendMessageA(win32::GetDlgItem(hwndViewer, 1), win32::LB_GETCOUNT, 0, 0) == 10);
    win32::SendMessageA(win32::GetDlgItem(hwndViewer, 1), win32::LB_SETCURSEL, 1, 0);
    win32::SendMessageA(hwndViewer, win32::WM_COMMAND, win32::MAKEWPARAM(1, win32::LBN_SELCHANGE), (LPARAM)win32::GetDlgItem(hwndViewer, 1));
    CHECK(win32::GetWindowTextA(win32::GetDlgItem(hwndViewer, 2), sz, sizeof(sz)) && strstr(sz, "Text:\r\nAction"));
    CHECK(strstr(sz, "WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON") && strstr(sz, "Control kind: 1 (button)"));
    CHECK(win32::SetWindowTextA(hwnd, "Caf\303\251"));
    CHECK(!strcmp(SDL_GetWindowTitle(LpCompatWindow(hwnd)->lpWindow.get()), "Caf\303\251"));
    CHECK(win32::GetWindowTextA(hwnd, sz, sizeof(sz)) == 5 && !strcmp(sz, "Caf\303\251"));
    win32::SetWindowTextA(hwndChild, "Updated");
    win32::SendMessageA(win32::GetDlgItem(hwndViewer, 3), win32::BM_CLICK, 0, 0);
    win32::SendMessageA(win32::GetDlgItem(hwndViewer, 1), win32::LB_SETCURSEL, 1, 0);
    win32::SendMessageA(hwndViewer, win32::WM_COMMAND, win32::MAKEWPARAM(1, win32::LBN_SELCHANGE), (LPARAM)win32::GetDlgItem(hwndViewer, 1));
    CHECK(win32::GetWindowTextA(win32::GetDlgItem(hwndViewer, 2), sz, sizeof(sz)) && strstr(sz, "Text:\r\nUpdated"));
    win32::UpdateWindow(hwndViewer);
    for (hwndPaint = win32::GetWindow(hwndViewer, win32::GW_CHILD); hwndPaint; hwndPaint = win32::GetWindow(hwndPaint, win32::GW_HWNDNEXT)) win32::UpdateWindow(hwndPaint);
    CHECK(FCompatPresent(hwndViewer) && LpCompatWindow(hwndViewer)->lpComposite.get());
    CHECK(SDL_ReadSurfacePixel(LpCompatWindow(hwndViewer)->lpComposite.get(), 0, 0, &r, &g, &b, &a));
    CHECK(win32::RGB(r, g, b) == win32::GetSysColor(win32::COLOR_BTNFACE));
    CHECK(win32::DestroyWindow(hwndViewer) && !StarsCompatResolveWindow(idViewer));
    // The diagnostic chord must not forward the game's F12 turn-generation shortcut.
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = LpCompatWindow(hwnd)->id;
    event.key.scancode = SDL_SCANCODE_F12; event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    CompatRouteEvent(&event);
    CHECK(StarsCompatShowInspector());
    win32::SendMessageA(StarsCompatResolveWindow(StarsCompatFindControl(id, 7, "Button")), win32::BM_CLICK, 0, 0);
    CHECK(cCommands == 1);
    CHECK(win32::DestroyWindow(hwndChild));
    CHECK(!StarsCompatResolveWindow(idChild) && !StarsCompatResolveWindow(idNested));
    CHECK(StarsCompatCaptureHierarchy(&snapAfter));
    CHECK(!LpItem(&snapAfter, idChild) && !LpItem(&snapAfter, idNested));
    StarsCompatFreeHierarchy(&snapAfter);
    CHECK(!strcmp(LpItem(&snap, idChild)->pszText, "Action"));
    StarsCompatProcessExit();
    CHECK(!StarsCompatResolveWindow(id));
    CHECK(win32::RegisterClassA(&wc));
    hwndNew = win32::CreateWindowExA(0, wc.lpszClassName, "Restart", win32::WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    CHECK(hwndNew && StarsCompatWindowId(hwndNew) > idViewer && !StarsCompatResolveWindow(id));
    CHECK(StarsCompatShowInspector());
    StarsCompatShutdown();
    StarsCompatFreeHierarchy(&snap); CHECK(!snap.rgWindow && snap.cWindow == 0);
    StarsCompatFreeHierarchy(&snap);
    SDL_Quit();
    return 0;
}
