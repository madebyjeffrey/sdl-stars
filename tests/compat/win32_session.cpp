/* Logoff rejection must not strand the legacy game's exit branches. */
#include <wincompat_calls.h>
#include <stars_compat.h>
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main(void) {
    win32::MSG msg;
    CHECK(!win32::ExitWindowsEx(win32::EWX_LOGOFF, UINT32_MAX));
    CHECK(win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(!win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE));
    CHECK(!win32::ExitWindowsEx(1, 0));
    CHECK(win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(!win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE));
    CHECK(!ui::RequestQuit(77));
    CHECK(win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE));
    CHECK(msg.message == win32::WM_QUIT && msg.wParam == 77);
    CHECK(!win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE));
    StarsCompatShutdown();
    return 0;
}
