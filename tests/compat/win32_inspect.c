/* Snapshot lifetime, logical IDs and a built-in viewer without desktop automation. */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "../../res/resource.h"
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static int cMessages, cDialogs, cDialogErrors, cCommands;
static uint64_t rgidDialog[2];
static SDL_WindowID rgidNative[2];
static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    STARSCOMPATSNAPSHOT snap = {0};
    size_t i;
    BOOL fFound = FALSE;
    (void)w; (void)l;
    if (msg != WM_INITDIALOG) return FALSE;
    rgidDialog[cDialogs] = StarsCompatWindowId(hwnd);
    rgidNative[cDialogs] = LpCompatWindow(hwnd)->id;
    if (!StarsCompatCaptureHierarchy(&snap)) cDialogErrors++;
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == rgidDialog[cDialogs]) {
        fFound = snap.rgWindow[i].fDialog && snap.rgWindow[i].idOwner != 0;
    }
    if (!fFound) cDialogErrors++;
    StarsCompatFreeHierarchy(&snap); cDialogs++;
    EndDialog(hwnd, 2);
    return TRUE;
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    cMessages++;
    if (msg == WM_COMMAND && LOWORD(w) == 7 && HIWORD(w) == BN_CLICKED) cCommands++;
    return DefWindowProc(hwnd, msg, w, l);
}
static const STARSCOMPATWINDOW *LpItem(const STARSCOMPATSNAPSHOT *lpsnap, uint64_t id) {
    size_t i;
    for (i = 0; i < lpsnap->cWindow; i++) if (lpsnap->rgWindow[i].id == id) return &lpsnap->rgWindow[i];
    return NULL;
}
static int SDLCALL Worker(void *lpv) {
    STARSCOMPATSNAPSHOT snap = {0};
    uint64_t id = *(uint64_t *)lpv;
    if (StarsCompatCaptureHierarchy(&snap) || StarsCompatResolveWindow(id) || StarsCompatShowInspector()) return 1;
    return 0;
}
static int TestStyles(void) {
    struct STYLECASE { UINT iControl; BOOL fDialog; DWORD dw; const char *psz, *pszAbsent; };
    static const struct STYLECASE rg[] = {
        { 0, FALSE, WS_OVERLAPPEDWINDOW, "WS_CAPTION", "WS_GROUP" },
        { 1, FALSE, WS_CHILD | WS_GROUP | BS_AUTOCHECKBOX | BS_CENTER, "BS_AUTOCHECKBOX | BS_CENTER", "BS_PUSHBUTTON" },
        { 2, FALSE, WS_CHILD | SS_CENTER | SS_ENDELLIPSIS, "SS_CENTER | SS_ENDELLIPSIS", "SS_LEFT" },
        { 3, FALSE, WS_CHILD | ES_CENTER | ES_PASSWORD | ES_NUMBER, "ES_CENTER | ES_PASSWORD | ES_NUMBER", "ES_LEFT" },
        { 4, FALSE, WS_CHILD | LBS_EXTENDEDSEL | LBS_HASSTRINGS, "LBS_HASSTRINGS | LBS_EXTENDEDSEL", "CBS_" },
        { 5, FALSE, WS_CHILD | SBS_VERT | SBS_LEFTALIGN, "SBS_VERT | SBS_LEFTALIGN", "SBS_TOPALIGN" },
        { 5, FALSE, WS_CHILD | SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN, "SBS_SIZEBOXBOTTOMRIGHTALIGN", "SBS_BOTTOMALIGN" },
        { 6, FALSE, WS_CHILD | CBS_DROPDOWNLIST | CBS_SORT, "CBS_DROPDOWNLIST | CBS_SORT", "CBS_DROPDOWN |" },
        { 0, TRUE, WS_POPUP | DS_SETFONT | DS_MODALFRAME, "DS_SETFONT | DS_MODALFRAME", "BS_" },
        { 1, FALSE, WS_CHILD | 0x000c, "0x0000000c", "BS_PUSHBUTTON" },
    };
    STARSCOMPATWINDOW item = {0};
    STARSCOMPATSNAPSHOT snap = { &item, 1 };
    size_t i, cb;
    FILE *lpfile;
    char sz[8192];
    item.pszClass = "StyleTest"; item.pszText = "";
    item.dwExStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | UINT32_C(0x40000000);
    for (i = 0; i < SDL_arraysize(rg); i++) {
        item.iControl = rg[i].iControl; item.fDialog = rg[i].fDialog; item.dwStyle = rg[i].dw;
        lpfile = tmpfile(); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile));
        rewind(lpfile); cb = fread(sz, 1, sizeof(sz) - 1, lpfile); sz[cb] = 0; fclose(lpfile);
        CHECK(strstr(sz, rg[i].psz) && !strstr(sz, rg[i].pszAbsent));
        CHECK(strstr(sz, "WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | 0x40000000"));
    }
    return 0;
}
int main(int argc, char **argv) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndChild, hwndNested, hwndHidden, hwndOwned, hwndDuplicate, hwndViewer, hwndNew, hwndPaint;
    uint64_t id, idChild, idNested, idViewer;
    STARSCOMPATSNAPSHOT snap = {0}, snapAfter = {0};
    const STARSCOMPATWINDOW *lp;
    FILE *lpfile;
    char sz[8192];
    size_t cb, i;
    int cBefore, iStatus;
    SDL_Thread *lpthread;
    SDL_Event event = {0};
    BYTE r, g, b, a;

    CHECK(TestStyles() == 0);
    wc.lpfnWndProc = WndProc; wc.lpszClassName = "InspectTest";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Frame \"quote\" \\ \200\r\n", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        10, 20, 320, 240, NULL, NULL, NULL, NULL);
    hwndChild = CreateWindow("Button", "Action", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             4, 5, 90, 30, hwnd, (HMENU)(INT_PTR)7, NULL, NULL);
    hwndNested = CreateWindow("Edit", "secret", WS_CHILD | WS_VISIBLE | ES_PASSWORD,
                              1, 2, 50, 20, hwndChild, (HMENU)(INT_PTR)8, NULL, NULL);
    hwndHidden = CreateWindow("Static", "Hidden", WS_CHILD | WS_DISABLED, 0, 0, 30, 20,
                              hwnd, (HMENU)(INT_PTR)9, NULL, NULL);
    hwndOwned = CreateWindow(wc.lpszClassName, "Owned", WS_POPUP | WS_VISIBLE, 20, 20, 60, 50, hwnd, NULL, NULL, NULL);
    CHECK(hwnd && hwndChild && hwndNested && hwndHidden && hwndOwned);
    CHECK(!strcmp(SDL_GetWindowTitle(LpCompatWindow(hwnd)->lpWindow), "Frame \"quote\" \\ \342\202\254\r\n"));
    id = StarsCompatWindowId(hwnd); idChild = StarsCompatWindowId(hwndChild); idNested = StarsCompatWindowId(hwndNested);
    CHECK(id != 0 && idChild > id && idNested > idChild);
    SetFocus(hwndChild); SetCapture(hwndChild);
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
    hwndDuplicate = CreateWindow("Static", "Duplicate", WS_CHILD, 0, 0, 1, 1, hwnd, (HMENU)(INT_PTR)7, NULL, NULL);
    CHECK(hwndDuplicate && StarsCompatFindControl(id, 7, NULL) == 0);
    CHECK(StarsCompatFindControl(id, 7, "Button") == idChild);
    CHECK(DestroyWindow(hwndDuplicate));
    // A JSON consumer must preserve IDs above JavaScript's exact-integer range.
    for (i = 0; i < snap.cWindow; i++) if (snap.rgWindow[i].id == StarsCompatWindowId(hwndOwned))
        snap.rgWindow[i].id = UINT64_C(9007199254740993);
    CHECK(StarsCompatBeginDialogSequence(hwnd));
    CHECK(DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
    CHECK(!StarsCompatResolveWindow(rgidDialog[0]));
    CHECK(DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DialogProc, 0) == 2);
    CHECK(cDialogs == 2 && cDialogErrors == 0 && rgidDialog[1] > rgidDialog[0]);
    CHECK(rgidNative[0] == rgidNative[1] && !StarsCompatResolveWindow(rgidDialog[1]));
    StarsCompatEndDialogSequence();
    lpfile = tmpfile(); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile));
    rewind(lpfile); cb = fread(sz, 1, sizeof(sz) - 1, lpfile); sz[cb] = 0; fclose(lpfile);
    CHECK(strstr(sz, "\\\"quote\\\"") && strstr(sz, "\\\\") && strstr(sz, "\\u000d\\u000a"));
    CHECK(strstr(sz, "\342\202\254") && strstr(sz, "[password]") && !strstr(sz, "secret"));
    if (argc == 2) { lpfile = fopen(argv[1], "wb"); CHECK(lpfile && StarsCompatWriteHierarchy(&snap, lpfile)); CHECK(fclose(lpfile) == 0); }
    ReleaseCapture();
    hwndViewer = StarsCompatShowInspector(); CHECK(hwndViewer && IsWindowVisible(hwndViewer));
    idViewer = StarsCompatWindowId(hwndViewer);
    CHECK(StarsCompatShowInspector() == hwndViewer);
    CHECK(GetDlgItem(hwndViewer, 1) && GetDlgItem(hwndViewer, 2) && GetDlgItem(hwndViewer, 3));
    CHECK(SendMessage(GetDlgItem(hwndViewer, 1), LB_GETCOUNT, 0, 0) == 10);
    SendMessage(GetDlgItem(hwndViewer, 1), LB_SETCURSEL, 1, 0);
    SendMessage(hwndViewer, WM_COMMAND, MAKEWPARAM(1, LBN_SELCHANGE), (LPARAM)GetDlgItem(hwndViewer, 1));
    CHECK(GetWindowText(GetDlgItem(hwndViewer, 2), sz, sizeof(sz)) && strstr(sz, "Text:\r\nAction"));
    CHECK(strstr(sz, "WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON") && strstr(sz, "Control kind: 1 (button)"));
    CHECK(SetWindowText(hwnd, "Caf\351"));
    CHECK(!strcmp(SDL_GetWindowTitle(LpCompatWindow(hwnd)->lpWindow), "Caf\303\251"));
    CHECK(GetWindowText(hwnd, sz, sizeof(sz)) == 4 && !strcmp(sz, "Caf\351"));
    SetWindowText(hwndChild, "Updated");
    SendMessage(GetDlgItem(hwndViewer, 3), BM_CLICK, 0, 0);
    SendMessage(GetDlgItem(hwndViewer, 1), LB_SETCURSEL, 1, 0);
    SendMessage(hwndViewer, WM_COMMAND, MAKEWPARAM(1, LBN_SELCHANGE), (LPARAM)GetDlgItem(hwndViewer, 1));
    CHECK(GetWindowText(GetDlgItem(hwndViewer, 2), sz, sizeof(sz)) && strstr(sz, "Text:\r\nUpdated"));
    UpdateWindow(hwndViewer);
    for (hwndPaint = GetWindow(hwndViewer, GW_CHILD); hwndPaint; hwndPaint = GetWindow(hwndPaint, GW_HWNDNEXT)) UpdateWindow(hwndPaint);
    CHECK(FCompatPresent(hwndViewer) && LpCompatWindow(hwndViewer)->lpComposite);
    CHECK(SDL_ReadSurfacePixel(LpCompatWindow(hwndViewer)->lpComposite, 0, 0, &r, &g, &b, &a));
    CHECK(RGB(r, g, b) == GetSysColor(COLOR_BTNFACE));
    CHECK(DestroyWindow(hwndViewer) && !StarsCompatResolveWindow(idViewer));
    // The diagnostic chord must not forward the game's F12 turn-generation shortcut.
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = LpCompatWindow(hwnd)->id;
    event.key.scancode = SDL_SCANCODE_F12; event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    CompatRouteEvent(&event);
    CHECK(StarsCompatShowInspector());
    SendMessage(StarsCompatResolveWindow(StarsCompatFindControl(id, 7, "Button")), BM_CLICK, 0, 0);
    CHECK(cCommands == 1);
    CHECK(DestroyWindow(hwndChild));
    CHECK(!StarsCompatResolveWindow(idChild) && !StarsCompatResolveWindow(idNested));
    CHECK(StarsCompatCaptureHierarchy(&snapAfter));
    CHECK(!LpItem(&snapAfter, idChild) && !LpItem(&snapAfter, idNested));
    StarsCompatFreeHierarchy(&snapAfter);
    CHECK(!strcmp(LpItem(&snap, idChild)->pszText, "Action"));
    StarsCompatProcessExit();
    CHECK(!StarsCompatResolveWindow(id));
    CHECK(RegisterClass(&wc));
    hwndNew = CreateWindow(wc.lpszClassName, "Restart", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    CHECK(hwndNew && StarsCompatWindowId(hwndNew) > idViewer && !StarsCompatResolveWindow(id));
    CHECK(StarsCompatShowInspector());
    StarsCompatShutdown();
    StarsCompatFreeHierarchy(&snap); CHECK(!snap.rgWindow && snap.cWindow == 0);
    StarsCompatFreeHierarchy(&snap);
    SDL_Quit();
    return 0;
}
