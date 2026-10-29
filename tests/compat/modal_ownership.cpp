/* A C++ chooser failure must unwind owner state before reaching its caller. */
#include "filedialog_backend.h"
#include <stars_compat.h>
#include <cstdio>
#include <cstring>
#include <new>

static HWND hwndRoot;
static int mdFailure = 1;
static bool fCleanupFailure;
static int cCalls, cErrors, cCleanupFailures;
int StarsTest_ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                               LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                               int *lpiFilter, char *pszPath, size_t cbPath) {
    (void)pszTitle; (void)pszFolder; (void)lpiFilter; (void)pszPath; (void)cbPath;
    cCalls++;
    if (hwndOwner != hwndRoot || fSave || IsWindowEnabled(hwndRoot) || hwndRoot->cNativeFileDialogs != 1 ||
        std::strcmp(pszName, "café-Ω.m1") || !rgfilter || cFilter != 2) cErrors++;
    if (mdFailure == 1) StarsThrowError(starsErrorFile, 417);
    if (mdFailure == 2) throw std::bad_alloc();
    return 0;
}
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_ENABLE && w && fCleanupFailure) {
        cCleanupFailures++; StarsThrowError(starsErrorMemory, 418);
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main() {
    WNDCLASSA wc = {};
    HWND hwndEdit;
    OPENFILENAMEA of = {};
    char szFile[256] = "café-Ω.m1";
    bool fCaught = false;
    wc.lpfnWndProc = OwnerProc; wc.lpszClassName = "ModalOwnership";
    CHECK(RegisterClassA(&wc));
    hwndRoot = CreateWindowExA(0, wc.lpszClassName, "Modal owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               0, 0, 240, 160, NULL, NULL, NULL, NULL);
    CHECK(hwndRoot);
    hwndEdit = CreateWindowExA(0, "Edit", "Saved focus", WS_CHILD | WS_VISIBLE, 4, 4, 100, 24,
                              hwndRoot, (HMENU)(UINT_PTR)73, NULL, NULL);
    CHECK(hwndEdit);
    SetFocus(hwndEdit); CHECK(GetFocus() == hwndEdit);
    of.lStructSize = sizeof(of); of.hwndOwner = hwndRoot;
    of.lpstrFile = szFile; of.nMaxFile = sizeof(szFile);
    of.lpstrFilter = "Stars\0*.m1;*.hst\0All files\0*.*\0";
    of.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    fCleanupFailure = true;
    try { (void)GetOpenFileNameA(&of); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    if (!IsWindowEnabled(hwndRoot) || hwndRoot->cNativeFileDialogs != 0)
        std::fprintf(stderr, "Owner state after exception: enabled=%d native-dialogs=%u\n", IsWindowEnabled(hwndRoot), hwndRoot->cNativeFileDialogs);
    CHECK(fCaught && cCalls == 1 && cErrors == 0 && cCleanupFailures == 1 && !FStarsErrorPending());
    CHECK(IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && GetFocus() == hwndEdit);
    CHECK(!std::strcmp(szFile, "café-Ω.m1"));
    mdFailure = 2; fCaught = false;
    try { (void)GetOpenFileNameA(&of); }
    catch (const std::bad_alloc &) { fCaught = true; }
    CHECK(fCaught && cCalls == 2 && cCleanupFailures == 2 && !FStarsErrorPending());
    CHECK(IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && GetFocus() == hwndEdit);
    mdFailure = 0;
    CHECK(!GetOpenFileNameA(&of) && cCalls == 3 && cCleanupFailures == 3 && FStarsErrorPending());
    CHECK(IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0);
    fCaught = false;
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorMemory && error.ids == 418; }
    CHECK(fCaught && !FStarsErrorPending());
    fCleanupFailure = false;
    CHECK(!GetOpenFileNameA(&of) && CommDlgExtendedError() == 0 && cCalls == 4 && cErrors == 0);
    CHECK(IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0 && GetFocus() == hwndEdit);
    EnableWindow(hwndRoot, FALSE);
    CHECK(!GetOpenFileNameA(&of) && cCalls == 5 && cErrors == 0);
    CHECK(!IsWindowEnabled(hwndRoot) && hwndRoot->cNativeFileDialogs == 0);
    EnableWindow(hwndRoot, TRUE);
    CHECK(DestroyWindow(hwndRoot));
    StarsCompatShutdown();
    CHECK(!FStarsErrorPending() && CCompatDispatch() == 0);
    std::puts("Chooser failure/cancellation preserve owner, native counter, focus and UTF-8 buffers");
    return 0;
}
