#include "../../compat/win32/src/chooser_io.h"
/* A C++ chooser failure must unwind owner state before reaching its caller. */
#include "filedialog_backend.h"
#include <stars_compat.h>
#include <cstdio>
#include <cstring>
#include <new>

static HWND hwndRoot;
static int mdFailure = 1;
static bool fCleanupFailure, fDisableFailure;
static int cCalls, cErrors, cCleanupFailures;
win32::files::FILESELECTIONRESULT StarsTest_ICompatChooseFile(HWND hwndOwner, const win32::files::FILESELECTIONREQUEST &request) {
    cCalls++;
    if (hwndOwner != hwndRoot || request.mode != win32::files::FILESELECTIONMODE::open ||
        win32::IsWindowEnabled(hwndRoot) || hwndRoot->cNativeFileDialogs != 1 ||
        request.szInitialPath != "café-Ω.m1" || request.rgFilters.size() != 2) cErrors++;
    if (mdFailure == 1) StarsThrowError(starsErrorFile, 417);
    if (mdFailure == 2) throw std::bad_alloc();
    return win32::files::FILESELECTIONCANCELLED{};
}
static LRESULT  OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == win32::WM_ENABLE && !w && fDisableFailure) StarsThrowError(starsErrorFile, 419);
    if (msg == win32::WM_ENABLE && w && fCleanupFailure) {
        cCleanupFailures++; StarsThrowError(starsErrorMemory, 418);
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main() {
    WNDCLASSA wc = {};
    HWND hwndEdit;
    OPENFILENAMEA of = {};
    char szFile[256] = "café-Ω.m1";
    bool fCaught = false;
    COMPATCHOOSERIO io_compatchooserio;
    io_compatchooserio.pfnICompatChooseFile = StarsTest_ICompatChooseFile;
    COMPATCHOOSERIOSCOPE scope_compatchooserio(io_compatchooserio);

    wc.lpfnWndProc = OwnerProc; wc.lpszClassName = "ModalOwnership";
    CHECK(win32::RegisterClassA(&wc));
    hwndRoot = win32::CreateWindowExA(0, wc.lpszClassName, "Modal owner", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE,
                               0, 0, 240, 160, NULL, NULL, NULL, NULL);
    CHECK(hwndRoot);
    hwndEdit = win32::CreateWindowExA(0, "Edit", "Saved focus", win32::WS_CHILD | win32::WS_VISIBLE, 4, 4, 100, 24,
                              hwndRoot, (HMENU)(UINT_PTR)73, NULL, NULL);
    CHECK(hwndEdit);
    win32::SetFocus(hwndEdit); CHECK(win32::GetFocus() == hwndEdit);
    of.lStructSize = sizeof(of); of.hwndOwner = hwndRoot;
    of.lpstrFile = szFile; of.nMaxFile = sizeof(szFile);
    of.lpstrFilter = "Stars\0*.m1;*.hst\0All files\0*.*\0";
    of.Flags = win32::OFN_PATHMUSTEXIST | win32::OFN_NOCHANGEDIR | win32::OFN_EXPLORER;
    fCleanupFailure = true;
    try { (void)win32::GetOpenFileNameA(&of); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    if (!win32::IsWindowEnabled(hwndRoot) || hwndRoot->cNativeFileDialogs != 0)
        std::fprintf(stderr, "Owner state after exception: enabled=%d native-dialogs=%u\n", win32::IsWindowEnabled(hwndRoot), hwndRoot->cNativeFileDialogs);
    CHECK(fCaught && cCalls == 1 && cErrors == 0 && cCleanupFailures == 1 && !FStarsErrorPending());
    CHECK(win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && win32::GetFocus() == hwndEdit);
    CHECK(!std::strcmp(szFile, "café-Ω.m1"));
    mdFailure = 2; fCaught = false;
    try { (void)win32::GetOpenFileNameA(&of); }
    catch (const std::bad_alloc &) { fCaught = true; }
    CHECK(fCaught && cCalls == 2 && cCleanupFailures == 2 && !FStarsErrorPending());
    CHECK(win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && win32::GetFocus() == hwndEdit);
    mdFailure = 0;
    CHECK(!win32::GetOpenFileNameA(&of) && cCalls == 3 && cCleanupFailures == 3 && FStarsErrorPending());
    CHECK(win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0);
    fCaught = false;
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorMemory && error.ids == 418; }
    CHECK(fCaught && !FStarsErrorPending());
    fCleanupFailure = false;
    CHECK(!win32::GetOpenFileNameA(&of) && win32::CommDlgExtendedError() == 0 && cCalls == 4 && cErrors == 0);
    CHECK(win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && win32::GetFocus() == hwndEdit);
    win32::EnableWindow(hwndRoot, 0);
    CHECK(!win32::GetOpenFileNameA(&of) && cCalls == 5 && cErrors == 0);
    CHECK(!win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0);
    win32::EnableWindow(hwndRoot, 1);
    win32::SetFocus(hwndEdit); CHECK(win32::GetFocus() == hwndEdit);
    fDisableFailure = true; fCleanupFailure = true;
    CHECK(!win32::GetOpenFileNameA(&of) && cCalls == 5 && FStarsErrorPending());
    CHECK(win32::IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && win32::GetFocus() == hwndEdit);
    CHECK(!win32::GetOpenFileNameA(&of) && cCalls == 5 && FStarsErrorPending());
    try { StarsRethrowPendingError(); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 419); }
    fDisableFailure = fCleanupFailure = false;
    CHECK(!FStarsErrorPending() && !std::strcmp(szFile, "café-Ω.m1"));
    CHECK(win32::DestroyWindow(hwndRoot));
    StarsCompatShutdown();
    CHECK(!FStarsErrorPending() && CCompatDispatch() == 0);
    std::puts("Chooser failure/cancellation preserve owner, native counter, focus and UTF-8 buffers");
    return 0;
}
