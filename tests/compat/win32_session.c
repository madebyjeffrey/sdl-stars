/* Logoff rejection must not strand the legacy game's exit branches. */
#include <windows.h>
#include <stars_compat.h>
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main(void) {
    MSG msg;
    CHECK(!ExitWindowsEx(EWX_LOGOFF, UINT32_MAX));
    CHECK(GetLastError() == ERROR_NOT_SUPPORTED);
    CHECK(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    CHECK(!ExitWindowsEx(1, 0));
    CHECK(GetLastError() == ERROR_NOT_SUPPORTED);
    CHECK(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    CHECK(!ExitWindows(77, 0));
    CHECK(GetLastError() == ERROR_NOT_SUPPORTED);
    CHECK(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    CHECK(msg.message == WM_QUIT && msg.wParam == 77);
    CHECK(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    StarsCompatShutdown();
    return 0;
}
