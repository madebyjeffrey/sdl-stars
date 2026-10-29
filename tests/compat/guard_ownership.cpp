/* Fail inside a held production file lock, then retry through the same API. */
#include "internal.h"
#include <stars_compat.h>
#include <cstdio>
#include <cstring>
#include <string>

static bool fFailRead, fCleanupFailure, fMenuFailure;
static int cNested, cDepthErrors;
int64_t StarsTest_CbReadNativeFile(NATIVEFILE *pf, void *rgb, size_t cb) {
    if (fFailRead) { fFailRead = false; StarsThrowError(starsErrorFile, 417); }
    return CbReadNativeFile(pf, rgb, cb);
}
static LRESULT CALLBACK GuardProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_APP + 1) {
        if (CCompatDispatch() != 1) cDepthErrors++;
        SendMessageA(hwnd, WM_APP + 2, 0, 0);
        StarsThrowError(starsErrorFile, 417);
    }
    if (msg == WM_APP + 2) {
        if (CCompatDispatch() != 2) cDepthErrors++;
        cNested++;
    }
    if (msg == WM_DESTROY && fCleanupFailure) StarsThrowError(starsErrorMemory, 418);
    if (msg == WM_UNINITMENUPOPUP && fMenuFailure) StarsThrowError(starsErrorFile, 417);
    return DefWindowProcA(hwnd, msg, w, l);
}
#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main(int argc, char **argv) {
    OFSTRUCT of = {};
    HFILE hf;
    char rgb[8] = {};
    bool fCaught = false;
    WNDCLASSA wc = {};
    HWND hwnd, hwndChild;
    HMENU hmenu;
    MSG msg = {};
    UINT rgflags[2] = {0, TPM_RETURNCMD};
    int i;
    CHECK(argc == 2 && !FStarsErrorPending());
    // Build the Unicode suffix as UTF-8 here; Windows CRT argv may use an ACP.
    std::string szPath = std::string(argv[1]) + "-Ω.dat";
    hf = OpenFile(szPath.c_str(), &of, OF_CREATE | OF_READWRITE | OF_SHARE_DENY_NONE);
    CHECK(hf != HFILE_ERROR && _lwrite(hf, "wire", 4) == 4 && _llseek(hf, 0, 0) == 0);
    fFailRead = true;
    try { (void)_lread(hf, rgb, sizeof(rgb)); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    CHECK(fCaught && !fFailRead && !FStarsErrorPending());
    // This retry hangs with the old manual lock/unlock pair after unwinding.
    std::fputs("Injected exception caught; retrying the same file token\n", stderr);
    CHECK(_lread(hf, rgb, sizeof(rgb)) == 4 && !std::memcmp(rgb, "wire", 4));
    CHECK(_lclose(hf) == 0 && IUnlinkNativePath(szPath.c_str()) == 0);

    wc.lpfnWndProc = GuardProc; wc.lpszClassName = "GuardOwnership";
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Guard", WS_OVERLAPPEDWINDOW, 0, 0, 120, 80, NULL, NULL, NULL, NULL);
    CHECK(hwnd && CCompatDispatch() == 0);
    SendMessageA(hwnd, WM_APP + 1, 0, 0);
    CHECK(FStarsErrorPending() && CCompatDispatch() == 0 && cNested == 1 && cDepthErrors == 0);
    fCleanupFailure = true;
    CHECK(DestroyWindow(hwnd) && !IsWindow(hwnd) && CCompatDispatch() == 0);
    fCaught = false;
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    CHECK(fCaught && !FStarsErrorPending());
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Unwinding", WS_OVERLAPPEDWINDOW, 0, 0, 120, 80, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    fCaught = false;
    try {
        struct WINDOWCLEANUP {
            HWND hwnd;
            ~WINDOWCLEANUP() noexcept { DestroyWindow(hwnd); }
        } cleanup{hwnd};
        StarsThrowError(starsErrorFile, 417);
    } catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    CHECK(fCaught && !IsWindow(hwnd) && !FStarsErrorPending() && CCompatDispatch() == 0);
    fCleanupFailure = false;
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Popup failure", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 240, 160, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    hwndChild = CreateWindowExA(0, wc.lpszClassName, "Saved capture", WS_CHILD | WS_VISIBLE, 0, 0, 80, 24, hwnd, (HMENU)(UINT_PTR)74, NULL, NULL);
    hmenu = CreatePopupMenu(); CHECK(hwndChild && hmenu && AppendMenuA(hmenu, MF_STRING, 73, "Choose"));
    SetCapture(hwndChild); CHECK(GetCapture() == hwndChild);
    fMenuFailure = true;
    for (i = 0; i < 2; i++) {
        PostMessageA(hwnd, WM_KEYDOWN, VK_DOWN, 1); PostMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 1);
        CHECK(!TrackPopupMenu(hmenu, rgflags[i], 0, 0, 0, hwnd, NULL));
        CHECK(FStarsErrorPending() && CCompatDispatch() == 0 && GetCapture() == hwndChild);
        CHECK(!PeekMessageA(&msg, hwnd, WM_COMMAND, WM_COMMAND, PM_REMOVE));
        fCaught = false;
        try { StarsRethrowPendingError(); }
        catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
        CHECK(fCaught && !FStarsErrorPending());
    }
    fMenuFailure = false;
    PostMessageA(hwnd, WM_KEYDOWN, VK_DOWN, 1); PostMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 1);
    CHECK(TrackPopupMenu(hmenu, 0, 0, 0, 0, hwnd, NULL));
    CHECK(PeekMessageA(&msg, hwnd, WM_COMMAND, WM_COMMAND, PM_REMOVE) && LOWORD(msg.wParam) == 73);
    CHECK(GetCapture() == hwndChild && !FStarsErrorPending());
    ReleaseCapture(); CHECK(DestroyMenu(hmenu) && DestroyWindow(hwnd));
    StarsCompatShutdown();
    CHECK(!FStarsErrorPending() && CCompatDispatch() == 0);
    std::puts("Held-lock retries, scoped dispatch, primary failures and popup command suppression passed");
    return 0;
}
