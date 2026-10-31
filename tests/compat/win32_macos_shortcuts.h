/* Use the application's configured tables while intercepting commands in a
   separate window, so Save, Submit and Quit never act on real game files. */
static UINT idMacCommand;
static int cMacCommand;
static BOOL fMacCommandInvalid;

static LRESULT CALLBACK MacShortcutProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_COMMAND && HIWORD(w) == 1) {
        idMacCommand = LOWORD(w);
        cMacCommand++;
        if (l) fMacCommandInvalid = TRUE;
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

static BOOL FMacShortcutKey(HWND hwnd, HACCEL haccel, SDL_Scancode sc, SDL_Keymod mod, UINT idExpected) {
    SDL_Event event = {};
    MSG msg;

    cMacCommand = 0;
    idMacCommand = 0;
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = hwnd->id;
    event.key.scancode = sc;
    event.key.mod = mod;
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_KEY_UP;
    event.key.mod = SDL_KMOD_NONE;
    if (!SDL_PushEvent(&event)) return FALSE;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (!TranslateAccelerator(hwnd, haccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    if (cMacCommand != (idExpected != 0) || idMacCommand != idExpected || fMacCommandInvalid) {
        fprintf(stderr, "Mac shortcut %d mod %u: command %u (%d times), expected %u\n",
                sc, (unsigned)mod, idMacCommand, cMacCommand, idExpected);
        return FALSE;
    }
    return !(GetKeyState(VK_CONTROL) & 0x8000);
}

static BOOL FCheckMacShortcuts(void) {
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
    HMENU hmenu = GetMenu(hwndFrame);
    SDL_Keymod mod;
    char sz[160];
    size_t i;
    int j;
    BOOL fOkay = FALSE;

    for (i = 0; i < sizeof(rglabel) / sizeof(rglabel[0]); i++) {
        if (!GetMenuString(hmenu, rglabel[i].id, sz, sizeof(sz), MF_BYCOMMAND) ||
            !strstr(sz, rglabel[i].psz) || strstr(sz, "Ctrl+")) {
            fprintf(stderr, "Mac menu hint %u: [%s], expected [%s]\n", rglabel[i].id, sz, rglabel[i].psz);
            return FALSE;
        }
    }
    wc.hInstance = hInst;
    wc.lpfnWndProc = MacShortcutProc;
    wc.lpszClassName = "MacShortcutContracts";
    if (!RegisterClass(&wc)) return FALSE;
    hwnd = CreateWindow(wc.lpszClassName, "", WS_POPUP, 0, 0, 220, 60, NULL, NULL, hInst, NULL);
    if (!hwnd) return FALSE;
    hwndEdit = CreateWindow("Edit", "alpha beta", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                            0, 0, 180, 25, hwnd, NULL, hInst, NULL);
    if (!hwndEdit) goto LDone;
    SetFocus(hwndEdit);
    for (j = 0; j < 2; j++) {
        mod = j == 0 ? SDL_KMOD_LGUI : SDL_KMOD_RGUI;
        for (i = 0; i < sizeof(rgmain) / sizeof(rgmain[0]); i++) {
            if (!FMacShortcutKey(hwnd, hAccel, rgmain[i].sc, mod | rgmain[i].mod, rgmain[i].id)) goto LDone;
        }
        for (i = 0; i < sizeof(rgtitle) / sizeof(rgtitle[0]); i++) {
            if (!FMacShortcutKey(hwnd, hAccelTitle, rgtitle[i].sc, mod | rgtitle[i].mod, rgtitle[i].id)) goto LDone;
        }
        if (!SetWindowText(hwndEdit, "alpha beta")) goto LDone;
        if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_A, mod, 0) ||
            SendMessage(hwndEdit, EM_GETSEL, 0, 0) != MAKELONG(0, 10)) goto LDone;
        if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_C, mod, 0)) goto LDone;
        if (!FMacShortcutKey(hwnd, hAccelTitle, SDL_SCANCODE_X, mod, 0) || SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0) != 0) goto LDone;
        if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_V, mod, 0) ||
            !GetWindowText(hwndEdit, sz, sizeof(sz)) || strcmp(sz, "alpha beta")) goto LDone;
        if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_Z, mod, 0) || SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0) != 0) goto LDone;
    }
    if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_F12, 0, IDM_DEBUG_GEN_10_TURNS)) goto LDone;
    if (!FMacShortcutKey(hwnd, hAccel, SDL_SCANCODE_N, 0, 0)) goto LDone;
    fOkay = TRUE;
LDone:
    if (!fOkay) fprintf(stderr, "Mac menu/edit shortcut check failed\n");
    DestroyWindow(hwnd);
    return fOkay;
}
