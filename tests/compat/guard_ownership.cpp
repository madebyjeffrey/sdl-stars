/* Fail inside a held production file lock, then retry through the same API. */
#include "internal.h"
#include "path_utf8.h"
#include "file_read.h"
#include "profile_io.h"
#include <stars_compat.h>
#include <cstdio>
#include <cstring>
#include <string>

static bool fFailRead, fCleanupFailure, fMenuFailure;
static int cNested, cDepthErrors, cProfileLines = -1, cReads;
static bool fFailProfileWrite;
std::optional<std::size_t> CbReadGuardFile(NATIVEFILE *pf, std::span<std::byte> rgb) {
    cReads++;
    if (fFailRead) {
        COMPATFILEREADSCOPE nested(CbReadNativeFile);
        fFailRead = false;
        StarsThrowError(starsErrorFile, 417);
    }
    return CbReadNativeFile(pf, rgb);
}
std::optional<std::string> SzReadGuardLine(NATIVEFILE *pf) {
    auto sz = SzGetNativeLine(pf);
    if (cProfileLines == 0) { cProfileLines = -1; StarsThrowError(starsErrorFile, 417); }
    if (cProfileLines > 0) cProfileLines--;
    return sz;
}
size_t CWriteGuardProfile(std::span<const std::byte> rgb, NATIVEFILE *pf) {
    if (fFailProfileWrite) { fFailProfileWrite = false; StarsThrowError(starsErrorFile, 417); }
    return CbWriteNativeStream(rgb, pf);
}
static LRESULT  GuardProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == win32::WM_APP + 1) {
        if (CCompatDispatch() != 1) cDepthErrors++;
        win32::SendMessageA(hwnd, win32::WM_APP + 2, 0, 0);
        StarsThrowError(starsErrorFile, 417);
    }
    if (msg == win32::WM_APP + 2) {
        if (CCompatDispatch() != 2) cDepthErrors++;
        cNested++;
    }
    if (msg == win32::WM_DESTROY && fCleanupFailure) StarsThrowError(starsErrorMemory, 418);
    if (msg == win32::WM_UNINITMENUPOPUP && fMenuFailure) StarsThrowError(starsErrorFile, 417);
    return win32::DefWindowProcA(hwnd, msg, w, l);
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
    win32::MSG msg = {};
    UINT rgflags[2] = {0, win32::TPM_RETURNCMD};
    int i;
    CHECK(argc == 2 && !FStarsErrorPending());
    // Build the Unicode suffix as UTF-8 here; Windows CRT argv may use an ACP.
    std::string szPath = std::string(argv[1]) + "-Ω.dat";
    hf = win32::OpenFile(szPath.c_str(), &of, win32::OF_CREATE | win32::OF_READWRITE | win32::OF_SHARE_DENY_NONE);
    CHECK(hf != win32::HFILE_ERROR && win32::_lwrite(hf, "wire", 4) == 4 && win32::_llseek(hf, 0, 0) == 0);
    {
        COMPATFILEREADSCOPE readScope(CbReadGuardFile);
        fFailRead = true;
        try { (void)win32::_lread(hf, rgb, sizeof(rgb)); }
        catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
        CHECK(fCaught && !fFailRead && !FStarsErrorPending());
        // This retry hangs with the old manual lock/unlock pair after unwinding.
        std::fputs("Injected exception caught; retrying the same file token\n", stderr);
        CHECK(win32::_lread(hf, rgb, sizeof(rgb)) == 4 && !std::memcmp(rgb, "wire", 4) && cReads == 2);
    }
    CHECK(win32::_llseek(hf, 0, 0) == 0);
    CHECK(win32::_lread(hf, rgb, sizeof(rgb)) == 4 && !std::memcmp(rgb, "wire", 4) && cReads == 2);
    CHECK(win32::_lclose(hf) == 0 && IUnlinkNativePath(szPath.c_str()) == 0);

    std::string szProfile = std::string(argv[1]) + "-Ω.ini";
    auto pathProfile = PathFromUtf8(szProfile.c_str());
    std::string szPrefix = SzFromUtf8(pathProfile.filename().u8string()) + ".";
    PROFILEIO io;
    io.pfnReadLine = SzReadGuardLine; io.pfnWrite = CWriteGuardProfile;
    PROFILEIOSCOPE profileScope(io);
    CHECK(win32::WritePrivateProfileStringA("Game", "Name", "wire", szProfile.c_str()));
    for (i = 0; i < 3; i++) {
        cProfileLines = i < 2 ? 1 : -1; fFailProfileWrite = i == 2; fCaught = false;
        try {
            if (i == 0) (void)win32::GetPrivateProfileStringA("Game", "Name", "", rgb, sizeof(rgb), szProfile.c_str());
            else (void)win32::WritePrivateProfileStringA("Game", "Name", "changed", szProfile.c_str());
        } catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
        CHECK(fCaught && cProfileLines == -1 && !fFailProfileWrite && !FStarsErrorPending());
        std::fputs("Injected profile exception caught; retrying the profile registry\n", stderr);
        CHECK(win32::GetPrivateProfileStringA("Game", "Name", "", rgb, sizeof(rgb), szProfile.c_str()) == 4);
        CHECK(!std::strcmp(rgb, "wire"));
        for (const auto &entry : std::filesystem::directory_iterator(pathProfile.parent_path()))
            CHECK(!SzFromUtf8(entry.path().filename().u8string()).starts_with(szPrefix));
    }
    CHECK(win32::WritePrivateProfileStringA("Game", "Name", "retry", szProfile.c_str()));
    CHECK(win32::GetPrivateProfileStringA("Game", "Name", "", rgb, sizeof(rgb), szProfile.c_str()) == 5 && !std::strcmp(rgb, "retry"));
    CHECK(IUnlinkNativePath(szProfile.c_str()) == 0);

    wc.lpfnWndProc = GuardProc; wc.lpszClassName = "GuardOwnership";
    CHECK(win32::RegisterClassA(&wc));
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Guard", win32::WS_OVERLAPPEDWINDOW, 0, 0, 120, 80, NULL, NULL, NULL, NULL);
    CHECK(hwnd && CCompatDispatch() == 0);
    win32::SendMessageA(hwnd, win32::WM_APP + 1, 0, 0);
    CHECK(FStarsErrorPending() && CCompatDispatch() == 0 && cNested == 1 && cDepthErrors == 0);
    fCleanupFailure = true;
    CHECK(win32::DestroyWindow(hwnd) && !win32::IsWindow(hwnd) && CCompatDispatch() == 0);
    fCaught = false;
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    CHECK(fCaught && !FStarsErrorPending());
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Unwinding", win32::WS_OVERLAPPEDWINDOW, 0, 0, 120, 80, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    fCaught = false;
    try {
        struct WINDOWCLEANUP {
            HWND hwnd;
            ~WINDOWCLEANUP() noexcept { win32::DestroyWindow(hwnd); }
        } cleanup{hwnd};
        StarsThrowError(starsErrorFile, 417);
    } catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    CHECK(fCaught && !win32::IsWindow(hwnd) && !FStarsErrorPending() && CCompatDispatch() == 0);
    fCleanupFailure = false;
    hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Popup failure", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE, 0, 0, 240, 160, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    hwndChild = win32::CreateWindowExA(0, wc.lpszClassName, "Saved capture", win32::WS_CHILD | win32::WS_VISIBLE, 0, 0, 80, 24, hwnd, (HMENU)(UINT_PTR)74, NULL, NULL);
    hmenu = win32::CreatePopupMenu(); CHECK(hwndChild && hmenu && win32::AppendMenuA(hmenu, win32::MF_STRING, 73, "Choose"));
    win32::SetCapture(hwndChild); CHECK(win32::GetCapture() == hwndChild);
    fMenuFailure = true;
    for (i = 0; i < 2; i++) {
        win32::PostMessageA(hwnd, win32::WM_KEYDOWN, win32::VK_DOWN, 1); win32::PostMessageA(hwnd, win32::WM_KEYDOWN, win32::VK_RETURN, 1);
        CHECK(!win32::TrackPopupMenu(hmenu, rgflags[i], 0, 0, 0, hwnd, NULL));
        CHECK(FStarsErrorPending() && CCompatDispatch() == 0 && win32::GetCapture() == hwndChild);
        CHECK(!win32::PeekMessageA(&msg, hwnd, win32::WM_COMMAND, win32::WM_COMMAND, win32::PM_REMOVE));
        fCaught = false;
        try { StarsRethrowPendingError(); }
        catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
        CHECK(fCaught && !FStarsErrorPending());
    }
    fMenuFailure = false;
    win32::PostMessageA(hwnd, win32::WM_KEYDOWN, win32::VK_DOWN, 1); win32::PostMessageA(hwnd, win32::WM_KEYDOWN, win32::VK_RETURN, 1);
    CHECK(win32::TrackPopupMenu(hmenu, 0, 0, 0, 0, hwnd, NULL));
    CHECK(win32::PeekMessageA(&msg, hwnd, win32::WM_COMMAND, win32::WM_COMMAND, win32::PM_REMOVE) && LOWORD(msg.wParam) == 73);
    CHECK(win32::GetCapture() == hwndChild && !FStarsErrorPending());
    win32::ReleaseCapture(); CHECK(win32::DestroyMenu(hmenu) && win32::DestroyWindow(hwnd));
    StarsCompatShutdown();
    CHECK(!FStarsErrorPending() && CCompatDispatch() == 0);
    std::puts("Held-lock retries, scoped dispatch, primary failures and popup command suppression passed");
    return 0;
}
