/* Win32 creates and dispatches; SDL is used only to inject native events in tests. */
#include <windows.h>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #f, SDL_GetError()); return 1; } } while (0)
#define msgEcho (WM_APP + 10)
#define msgCount (WM_APP + 11)
#define msgSubclass (WM_APP + 12)
#define msgNested (WM_APP + 13)
#define msgShutdown (WM_APP + 14)

typedef struct WINDOWTEST {
    int i;
    int cNcCreate, cCreate, cDestroy, cNcDestroy, cKey, cChar, cMouse, cSize, cCustom, cFocus, cBlur;
    BOOL fReject, fSelfDestroy, fCounted, fReentrant, fCreateData;
    WPARAM wMouse;
    LPARAM lMouse, lKey;
} WINDOWTEST;

static BOOL fInteractive;
static int cLive;

static LRESULT CALLBACK TestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WINDOWTEST *lpwin = (WINDOWTEST *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    CREATESTRUCT *lpcs;
    char szTitle[100];

    if (msg == WM_NCCREATE) {
        lpcs = (CREATESTRUCT *)lParam;
        lpwin = (WINDOWTEST*)(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpwin);
        lpwin->cNcCreate++;
        lpwin->fCreateData = lpcs->cx == 320 && lpcs->cy == 240;
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    if (!lpwin)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_CREATE:
        lpwin->cCreate++;
        if (lpwin->fReject)
            return -1;
        lpwin->fCounted = TRUE;
        cLive++;
        if (lpwin->fSelfDestroy)
            DestroyWindow(hwnd);
        return 0;
    case WM_KEYDOWN:
        lpwin->cKey++;
        lpwin->lKey = lParam;
        if (fInteractive) {
            snprintf(szTitle, sizeof(szTitle), "Stars Win32 window %d: %d keys (close either window)", lpwin->i + 1, lpwin->cKey);
            SetWindowText(hwnd, szTitle);
        }
        return 0;
    case WM_CHAR:
        lpwin->cChar++;
        return 0;
    case WM_MOUSEMOVE:
        lpwin->cMouse++;
        lpwin->wMouse = wParam;
        lpwin->lMouse = lParam;
        return 0;
    case WM_SIZE:
        lpwin->cSize++;
        return 0;
    case WM_SETFOCUS:
        lpwin->cFocus++;
        return 0;
    case WM_KILLFOCUS:
        lpwin->cBlur++;
        return 0;
    case msgEcho:
        return (LRESULT)((UINT_PTR)wParam ^ (UINT_PTR)lParam);
    case msgNested:
        return SendMessage(hwnd, msgEcho, wParam, lParam);
    case msgShutdown:
        StarsCompatShutdown();
        return GetWindowLongPtr(hwnd, GWLP_USERDATA) == (LONG_PTR)lpwin;
    case msgCount:
        lpwin->cCustom++;
        return (LRESULT)wParam;
    case WM_DESTROY:
        lpwin->cDestroy++;
        lpwin->fReentrant = !DestroyWindow(hwnd);
        if (lpwin->fCounted) {
            cLive--;
            lpwin->fCounted = FALSE;
        }
        if (fInteractive && cLive == 0)
            PostQuitMessage(0);
        return 0;
    case WM_NCDESTROY:
        lpwin->cNcDestroy++;
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == msgSubclass)
        return (LRESULT)UINT64_C(0x123456789abcdef0);
    return CallWindowProc(TestWndProc, hwnd, msg, wParam, lParam);
}

static void DrainMessages(void) {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static SDL_WindowID IdWindowWithTitle(const char *pszTitle) {
    SDL_Window **rglpwin;
    SDL_WindowID id = 0;
    int cwin, i;

    rglpwin = SDL_GetWindows(&cwin);
    for (i = 0; i < cwin; i++)
        if (strcmp(SDL_GetWindowTitle(rglpwin[i]), pszTitle) == 0)
            id = SDL_GetWindowID(rglpwin[i]);
    SDL_free(rglpwin);
    return id;
}

static int PushDelayedKey(void *lpId) {
    SDL_Event event = {};

    SDL_zero(event);
    SDL_Delay(20);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = *(SDL_WindowID *)lpId;
    event.key.scancode = SDL_SCANCODE_SPACE;
    event.key.key = SDLK_SPACE;
    return SDL_PushEvent(&event) ? 0 : 1;
}

static int TestWindows(HWND *rghwnd, WINDOWTEST *rgwin, ATOM atom, HINSTANCE hInstance) {
    MSG msg, stale;
    RECT rc;
    SDL_Event event = {};
    SDL_WindowID rgid[2];
    SDL_Thread *hThread;
    WINDOWTEST winFailure = {};
    HWND hwndDefault;
    LONG_PTR lProc, lOriginal;
    WNDPROC lpfn = SubclassProc;
    char sz[80];
    int cKey, cMouse, cSize, cFocus, cBlur, cThreadResult;

    CHECK(rghwnd[0] != rghwnd[1]);
    hwndDefault = CreateWindow(MAKEINTRESOURCE(atom), "Default size", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, &winFailure);
    CHECK(hwndDefault && GetClientRect(hwndDefault, &rc) && rc.right == 640 && rc.bottom == 480);
    CHECK(StarsCompatSetInitialShowCommand(hwndDefault, SW_SHOWMAXIMIZED));
    CHECK(!IsWindowVisible(hwndDefault) && !IsZoomed(hwndDefault));
    CHECK(!ShowWindow(hwndDefault, SW_HIDE) && !IsWindowVisible(hwndDefault));
    // The dummy driver cannot maximize; an explicit normal show overrides the deferred command.
    CHECK(!ShowWindow(hwndDefault, SW_SHOWNORMAL) && IsWindowVisible(hwndDefault) && !IsZoomed(hwndDefault));
    CHECK(!StarsCompatSetInitialShowCommand(hwndDefault, SW_SHOWNORMAL));
    CHECK(ShowWindow(hwndDefault, SW_HIDE));
    CHECK(StarsCompatSetInitialShowCommand(hwndDefault, SW_SHOWNORMAL));
    CHECK(!ShowWindow(hwndDefault, SW_SHOW) && IsWindowVisible(hwndDefault) && !IsZoomed(hwndDefault));
    CHECK(ShowWindow(hwndDefault, SW_HIDE));
    CHECK(StarsCompatSetInitialShowCommand(hwndDefault, SW_SHOWMAXIMIZED));
    CHECK(!ShowWindow(hwndDefault, SW_SHOW) && IsWindowVisible(hwndDefault) && !IsZoomed(hwndDefault));
    CHECK(DestroyWindow(hwndDefault));
    memset(&winFailure, 0, sizeof(winFailure));
    CHECK(cLive == 2 && rgwin[0].cCreate == 1 && rgwin[1].cCreate == 1);
    CHECK(rgwin[0].cNcCreate == 1 && rgwin[0].fCreateData);
    CHECK(rgwin[0].cSize > 0 && rgwin[1].cSize > 0);
    CHECK(GetWindowLongPtr(rghwnd[0], GWLP_HINSTANCE) == (LONG_PTR)hInstance);
    CHECK(SetWindowLongPtr(rghwnd[0], 0, (LONG_PTR)UINT64_C(0x123456789abcdef0)) == 0);
    CHECK(GetWindowLongPtr(rghwnd[0], 0) == (LONG_PTR)UINT64_C(0x123456789abcdef0));
    CHECK(GetWindowLongPtr(rghwnd[1], 0) == 0);
    CHECK(SetWindowLongPtr(rghwnd[0], GWLP_USERDATA, (LONG_PTR)UINT64_C(0x123456789abcdef0)) == (LONG_PTR)&rgwin[0]);
    CHECK(GetWindowLongPtr(rghwnd[0], GWLP_USERDATA) == (LONG_PTR)UINT64_C(0x123456789abcdef0));
    SetWindowLongPtr(rghwnd[0], GWLP_USERDATA, (LONG_PTR)&rgwin[0]);
    CHECK(SendMessage(rghwnd[0], msgEcho, (WPARAM)UINT64_C(0x123456789abcdef0),
                      (LPARAM)UINT64_C(0x76543210fedcba98)) == (LRESULT)UINT64_C(0x6460646864606468));
    CHECK(SendMessage(rghwnd[0], msgNested, (WPARAM)UINT64_C(0x123456789abcdef0),
                      (LPARAM)UINT64_C(0x76543210fedcba98)) == (LRESULT)UINT64_C(0x6460646864606468));
    CHECK(SendMessage(rghwnd[0], msgShutdown, 0, 0));
    memcpy(&lProc, &lpfn, sizeof(lProc));
    lOriginal = SetWindowLongPtr(rghwnd[0], GWLP_WNDPROC, lProc);
    CHECK(lOriginal != 0 && GetWindowLongPtr(rghwnd[0], GWLP_WNDPROC) == lProc);
    CHECK(SendMessage(rghwnd[0], msgSubclass, 0, 0) == (LRESULT)UINT64_C(0x123456789abcdef0));
    CHECK(SendMessage(rghwnd[1], msgSubclass, 0, 0) == 0);
    CHECK(SetWindowLongPtr(rghwnd[0], GWLP_WNDPROC, lOriginal) == lProc);
    CHECK(GetClassName(rghwnd[0], sz, sizeof(sz)) > 0 && strcmp(sz, "StarsWin32Windows") == 0);
    CHECK(IsWindowVisible(rghwnd[0]));
    CHECK(ShowWindow(rghwnd[0], SW_HIDE));
    CHECK(!IsWindowVisible(rghwnd[0]) && !ShowWindow(rghwnd[0], SW_SHOW));
    CHECK(SetWindowText(rghwnd[0], "Stars Win32 first"));
    CHECK(GetWindowText(rghwnd[0], sz, sizeof(sz)) == 17 && strcmp(sz, "Stars Win32 first") == 0);
    CHECK(MoveWindow(rghwnd[0], 40, 40, 400, 300, FALSE));
    CHECK(GetClientRect(rghwnd[0], &rc) && rc.right == 400 && rc.bottom == 300);
    rgid[0] = IdWindowWithTitle("Stars Win32 first");
    rgid[1] = IdWindowWithTitle("Stars Win32 second");
    CHECK(rgid[0] != 0 && rgid[1] != 0 && rgid[0] != rgid[1]);
    winFailure.fReject = TRUE;
    CHECK(!CreateWindow(MAKEINTRESOURCE(atom), "Reject", WS_OVERLAPPED, 0, 0, 320, 240, NULL, NULL, hInstance, &winFailure));
    CHECK(winFailure.cDestroy == 0 && winFailure.cNcDestroy == 1 && cLive == 2);
    memset(&winFailure, 0, sizeof(winFailure));
    winFailure.fSelfDestroy = TRUE;
    CHECK(!CreateWindow(MAKEINTRESOURCE(atom), "Self destroy", WS_OVERLAPPED, 0, 0, 320, 240, NULL, NULL, hInstance, &winFailure));
    CHECK(winFailure.cDestroy == 1 && winFailure.cNcDestroy == 1 && cLive == 2);
    CHECK(!CreateWindow("Missing class", "", WS_OVERLAPPED, 0, 0, 320, 240, NULL, NULL, hInstance, NULL));
    CHECK(!CreateWindow(MAKEINTRESOURCE(atom), "", WS_CHILD, 0, 0, 320, 240, NULL, NULL, hInstance, NULL));
    DrainMessages();
    CHECK(PostMessage(rghwnd[1], msgEcho, (WPARAM)UINT64_C(0x123456789abcdef0),
                       (LPARAM)UINT64_C(0x76543210fedcba98)));
    CHECK(PeekMessage(&msg, rghwnd[1], msgEcho, msgEcho, PM_REMOVE));
    CHECK(msg.wParam == (WPARAM)UINT64_C(0x123456789abcdef0) &&
          DispatchMessage(&msg) == (LRESULT)UINT64_C(0x6460646864606468));
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    event.window.windowID = rgid[0]; CHECK(SDL_PushEvent(&event));
    event.window.windowID = rgid[1]; CHECK(SDL_PushEvent(&event));
    DrainMessages();
    CHECK(GetFocus() == NULL && GetActiveWindow() == NULL);
    cFocus = rgwin[0].cFocus; cBlur = rgwin[0].cBlur;
    event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    event.window.windowID = rgid[0]; CHECK(SDL_PushEvent(&event));
    DrainMessages();
    CHECK(GetFocus() == rghwnd[0] && rgwin[0].cFocus == cFocus + 1);
    CHECK(GetActiveWindow() == rghwnd[0]);
    event.window.windowID = rgid[1]; CHECK(SDL_PushEvent(&event));
    DrainMessages();
    CHECK(GetFocus() == rghwnd[1] && rgwin[0].cBlur == cBlur + 1);
    CHECK(GetActiveWindow() == rghwnd[1]);
    cBlur = rgwin[1].cBlur;
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    CHECK(SDL_PushEvent(&event));
    DrainMessages();
    CHECK(GetFocus() == NULL && rgwin[1].cBlur == cBlur + 1);
    CHECK(GetActiveWindow() == NULL);
    SetFocus(rghwnd[1]);
    CHECK(GetActiveWindow() == rghwnd[1]);
    SetFocus(NULL);
    CHECK(GetActiveWindow() == rghwnd[1] && GetFocus() == NULL);
    CHECK(SDL_PushEvent(&event));
    DrainMessages();
    CHECK(GetActiveWindow() == NULL);
    CHECK(PostMessage(rghwnd[0], msgCount, 7, 0));
    CHECK(PostMessage(rghwnd[1], msgCount, 9, 0));
    CHECK(PostMessage(NULL, msgCount, 11, 0));
    CHECK(PeekMessage(&msg, rghwnd[1], msgCount, msgCount, PM_NOREMOVE) && msg.wParam == 9);
    CHECK(PeekMessage(&msg, rghwnd[1], msgCount, msgCount, PM_REMOVE) && msg.wParam == 9);
    CHECK(DispatchMessage(&msg) == 9 && rgwin[1].cCustom == 1);
    CHECK(PeekMessage(&msg, (HWND)(INT_PTR)-1, msgCount, msgCount, PM_REMOVE) && msg.hwnd == NULL);
    CHECK(DispatchMessage(&msg) == 0 && rgwin[0].cCustom == 0);
    CHECK(PeekMessage(&stale, rghwnd[0], msgCount, msgCount, PM_REMOVE));
    CHECK(PostMessage(rghwnd[0], msgCount, 13, 0));
    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    event.window.windowID = rgid[0];
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, rghwnd[0], WM_CLOSE, WM_CLOSE, PM_REMOVE));
    DispatchMessage(&msg);
    DrainMessages();
    CHECK(rgwin[0].cDestroy == 1 && rgwin[0].cNcDestroy == 1 && rgwin[0].fReentrant && cLive == 1);
    CHECK(!DestroyWindow(rghwnd[0]) && !PostMessage(rghwnd[0], msgCount, 0, 0));
    CHECK(DispatchMessage(&stale) == 0);
    CHECK(rgwin[0].cCustom == 0);
    CHECK(GetMessage(&msg, rghwnd[0], 0, 0) == -1);
    CHECK(!PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE));
    cKey = rgwin[1].cKey; cMouse = rgwin[1].cMouse; cSize = rgwin[1].cSize;
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = rgid[0]; // A destroyed ID must not be routed to the survivor.
    event.key.scancode = SDL_SCANCODE_SPACE;
    CHECK(SDL_PushEvent(&event));
    event.key.windowID = rgid[1];
    event.key.mod = SDL_KMOD_SHIFT;
    CHECK(SDL_PushEvent(&event));
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = rgid[1];
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = -12; event.motion.y = -34;
    SDL_SetModState(SDL_KMOD_SHIFT);
    CHECK(SDL_PushEvent(&event));
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_WINDOW_RESIZED;
    event.window.windowID = rgid[1]; event.window.data1 = 500; event.window.data2 = 350;
    CHECK(SDL_PushEvent(&event));
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = rgid[1]; event.text.text = "AZ";
    CHECK(SDL_PushEvent(&event));
    DrainMessages();
    SDL_SetModState(SDL_KMOD_NONE);
    CHECK(rgwin[1].cKey == cKey + 1 && rgwin[0].cKey == 0);
    CHECK((UINT_PTR)rgwin[1].lKey == UINT32_C(0x00390001));
    CHECK(rgwin[1].cMouse == cMouse + 1 && rgwin[1].cSize == cSize + 1 && rgwin[1].cChar == 2);
    CHECK(rgwin[1].wMouse == (MK_LBUTTON | MK_SHIFT));
    CHECK((int16_t)LOWORD(rgwin[1].lMouse) == -12 && (int16_t)HIWORD(rgwin[1].lMouse) == -34);
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_KEY_UP;
    event.key.windowID = rgid[1]; event.key.scancode = SDL_SCANCODE_RIGHT;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, rghwnd[1], WM_KEYUP, WM_KEYUP, PM_REMOVE));
    CHECK(msg.wParam == VK_RIGHT && (UINT_PTR)msg.lParam == UINT32_C(0xc14d0001));
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_F10;
    CHECK(SDL_PushEvent(&event));
    CHECK(PeekMessage(&msg, rghwnd[1], WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE) && msg.wParam == VK_F10);
    CHECK(TranslateMessage(&msg));
    msg.message = msgCount;
    CHECK(!TranslateMessage(&msg));
    hThread = SDL_CreateThread(PushDelayedKey, "compat-wake", &rgid[1]);
    CHECK(hThread);
    CHECK(GetMessage(&msg, rghwnd[1], WM_KEYDOWN, WM_KEYDOWN) == TRUE && msg.hwnd == rghwnd[1]);
    SDL_WaitThread(hThread, &cThreadResult);
    CHECK(cThreadResult == 0);
    DispatchMessage(&msg);
    CHECK(rgwin[1].cKey == cKey + 2);
    CHECK(PostMessage(rghwnd[1], msgCount, 17, 0) && WaitMessage());
    CHECK(PeekMessage(&msg, rghwnd[1], msgCount, msgCount, PM_REMOVE) && DispatchMessage(&msg) == 17);
    PostQuitMessage(23);
    CHECK(GetMessage(&msg, rghwnd[1], WM_APP + 100, WM_APP + 101) == FALSE && msg.message == WM_QUIT && msg.wParam == 23);
    CHECK(DestroyWindow(rghwnd[1]) && cLive == 0 && rgwin[1].cDestroy == 1 && rgwin[1].cNcDestroy == 1);
    CHECK(GetActiveWindow() == NULL);
    CHECK(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    return 0;
}

int main(int argc, char **argv) {
    WNDCLASS wc = {};
    WINDOWTEST rgwin[2] = {};
    HWND rghwnd[2];
    ATOM atom;
    HINSTANCE hInstance = (HINSTANCE)(UINT_PTR)0x1234;
    MSG msg;
    char szClass[] = "StarsWin32Windows";
    BOOL fTest = argc == 2 && strcmp(argv[1], "--self-test") == 0;
    int cResult;

    if (argc > 1 && !fTest) {
        fprintf(stderr, "Usage: %s [--self-test]\n", argv[0]);
        return 1;
    }
    fInteractive = !fTest;
    wc.lpfnWndProc = TestWndProc;
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.cbClsExtra = sizeof(LONG_PTR);
    wc.hInstance = hInstance;
    wc.lpszClassName = szClass;
    atom = RegisterClass(&wc);
    CHECK(atom != 0 && RegisterClass(&wc) == 0);
    szClass[0] = 'X'; // Registration must own the name, not retain caller storage.
    rgwin[1].i = 1;
    rghwnd[0] = CreateWindow("StarsWin32Windows", "Stars Win32 first", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              40, 40, 320, 240, NULL, NULL, hInstance, &rgwin[0]);
    rghwnd[1] = CreateWindow(MAKEINTRESOURCE(atom), "Stars Win32 second", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              400, 40, 320, 240, NULL, NULL, hInstance, &rgwin[1]);
    CHECK(rghwnd[0] && rghwnd[1]);
    if (fTest) {
        cResult = TestWindows(rghwnd, rgwin, atom, hInstance);
        StarsCompatShutdown();
        if (cResult == 0)
            puts("Win32 windows: creation, state, filters, native input, blocking wait and independent close passed");
        return cResult;
    }
    puts("Two Win32 compatibility windows: type in either; close either, then the survivor.");
    while ((cResult = GetMessage(&msg, NULL, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    StarsCompatShutdown();
    return cResult == -1 ? 1 : (int)msg.wParam;
}
