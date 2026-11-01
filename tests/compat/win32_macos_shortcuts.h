#include "uistate.h"
/* Use the application's configured tables while intercepting commands in a
   separate window, so Save, Submit and Quit never act on real game files. */
static UINT idMacCommand;
static int cMacCommand;
static BOOL fMacCommandInvalid;

static LRESULT  MacShortcutProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == win32::WM_COMMAND && HIWORD(w) == 1) {
        idMacCommand = LOWORD(w);
        cMacCommand++;
        if (l) fMacCommandInvalid = 1;
        return 0;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}

static BOOL FMacShortcutKey(HWND hwnd, HACCEL haccel, SDL_Scancode sc, SDL_Keymod mod, UINT idExpected) {
    SDL_Event event = {};
    win32::MSG msg;

    cMacCommand = 0;
    idMacCommand = 0;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = hwnd->id;
    event.key.scancode = sc;
    event.key.mod = mod;
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_KEY_UP;
    event.key.mod = SDL_KMOD_NONE;
    if (!SDL_PushEvent(&event)) return 0;
    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) {
        if (!win32::TranslateAcceleratorA(hwnd, haccel, &msg)) {
            win32::TranslateMessage(&msg);
            win32::DispatchMessageA(&msg);
        }
    }
    if (cMacCommand != (idExpected != 0) || idMacCommand != idExpected || fMacCommandInvalid) {
        fprintf(stderr, "Mac shortcut %d mod %u: command %u (%d times), expected %u\n",
                sc, (unsigned)mod, idMacCommand, cMacCommand, idExpected);
        return 0;
    }
    return !(win32::GetKeyState(win32::VK_CONTROL) & 0x8000);
}

static BOOL FCheckMacShortcuts(APPSESSION &app) {
    static const struct {
        SDL_Scancode sc;
        SDL_Keymod mod;
        UINT id;
    } rgmain[] = {
        {SDL_SCANCODE_N, 0, IDM_FILE_NEW_GAME},
        {SDL_SCANCODE_O, 0, IDM_FILE_OPEN_GAME},
        {SDL_SCANCODE_S, 0, IDM_FILE_SAVE},
        {SDL_SCANCODE_F, 0, IDM_VIEW_FIND2},
        {SDL_SCANCODE_S, SDL_KMOD_LSHIFT, IDM_TURN_SAVE_SUBMIT},
        {SDL_SCANCODE_W, 0, IDM_FILE_RETURN_TO_TITLE},
        {SDL_SCANCODE_Q, 0, IDM_FILE_EXIT},
        {SDL_SCANCODE_A, 0, 0}, {SDL_SCANCODE_C, 0, 0},
        {SDL_SCANCODE_O, SDL_KMOD_LSHIFT, 0},
        {SDL_SCANCODE_O, SDL_KMOD_LALT, 0},
        {SDL_SCANCODE_F12, 0, 0}
    }, rgtitle[] = {
        {SDL_SCANCODE_N, 0, IDM_TITLE_NEW_GAME},
        {SDL_SCANCODE_O, 0, IDM_TITLE_OPEN_GAME},
        {SDL_SCANCODE_R, 0, IDM_TITLE_CONTINUE},
        {SDL_SCANCODE_Q, 0, IDM_TITLE_EXIT},
        {SDL_SCANCODE_C, 0, 0}, {SDL_SCANCODE_X, 0, 0}
    };
    static const struct { UINT id; const char *psz; } rglabel[] = {
        {IDM_TOOL_NEW_GAME, "\tCmd+N"}, {IDM_TOOL_OPEN_GAME, "\tCmd+O"},
        {IDM_TURN_END_A, "\tCmd+S"}, {IDM_TURN_END_B, "\tCmd+Shift+S"},
        {IDM_VIEW_FIND, "\tCmd+F"}, {IDM_FILE_RETURN_TO_TITLE, "\tCmd+W"},
        {IDM_FILE_EXIT, "\tCmd+Q"}
    };
    WNDCLASS wc = {};
    HWND hwnd = NULL, hwndEdit;
    HMENU hmenu = win32::GetMenu(app.lpui->hwndFrame);
    SDL_Keymod mod;
    char sz[160];
    size_t i;
    int j;
    BOOL fOkay = 0;

    for (i = 0; i < sizeof(rglabel) / sizeof(rglabel[0]); i++) {
        if (!win32::GetMenuStringA(hmenu, rglabel[i].id, sz, sizeof(sz), win32::MF_BYCOMMAND) ||
            !strstr(sz, rglabel[i].psz) || strstr(sz, "Ctrl+")) {
            fprintf(stderr, "Mac menu hint %u: [%s], expected [%s]\n", rglabel[i].id, sz, rglabel[i].psz);
            return 0;
        }
    }
    wc.hInstance = app.lpui->hInst;
    wc.lpfnWndProc = MacShortcutProc;
    wc.lpszClassName = "MacShortcutContracts";
    if (!win32::RegisterClassA(&wc)) return 0;
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "", win32::WS_POPUP, 0, 0, 220, 60, NULL, NULL, app.lpui->hInst, NULL);
    if (!hwnd) return 0;
    hwndEdit = win32::CreateWindowExA(0, "Edit", "alpha beta", win32::WS_CHILD | win32::WS_VISIBLE | win32::ES_AUTOHSCROLL,
                            0, 0, 180, 25, hwnd, NULL, app.lpui->hInst, NULL);
    if (!hwndEdit) goto LDone;
    win32::SetFocus(hwndEdit);
    for (j = 0; j < 2; j++) {
        mod = j == 0 ? SDL_KMOD_LGUI : SDL_KMOD_RGUI;
        for (i = 0; i < sizeof(rgmain) / sizeof(rgmain[0]); i++) {
            if (!FMacShortcutKey(hwnd, app.lpui->hAccel, rgmain[i].sc, mod | rgmain[i].mod, rgmain[i].id)) goto LDone;
        }
        for (i = 0; i < sizeof(rgtitle) / sizeof(rgtitle[0]); i++) {
            if (!FMacShortcutKey(hwnd, app.lpui->hAccelTitle, rgtitle[i].sc, mod | rgtitle[i].mod, rgtitle[i].id)) goto LDone;
        }
        if (!win32::SetWindowTextA(hwndEdit, "alpha beta")) goto LDone;
        if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_A, mod, 0) ||
            win32::SendMessageA(hwndEdit, win32::EM_GETSEL, 0, 0) != MAKELONG(0, 10)) goto LDone;
        if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_C, mod, 0)) goto LDone;
        if (!FMacShortcutKey(hwnd, app.lpui->hAccelTitle, SDL_SCANCODE_X, mod, 0) || win32::SendMessageA(hwndEdit, win32::WM_GETTEXTLENGTH, 0, 0) != 0) goto LDone;
        if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_V, mod, 0) ||
            !win32::GetWindowTextA(hwndEdit, sz, sizeof(sz)) || strcmp(sz, "alpha beta")) goto LDone;
        if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_Z, mod, 0) || win32::SendMessageA(hwndEdit, win32::WM_GETTEXTLENGTH, 0, 0) != 0) goto LDone;
    }
    if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_F12, 0, IDM_DEBUG_GEN_10_TURNS)) goto LDone;
    if (!FMacShortcutKey(hwnd, app.lpui->hAccel, SDL_SCANCODE_N, 0, 0)) goto LDone;
    fOkay = 1;
LDone:
    if (!fOkay) fprintf(stderr, "Mac menu/edit shortcut check failed\n");
    win32::DestroyWindow(hwnd);
    return fOkay;
}
