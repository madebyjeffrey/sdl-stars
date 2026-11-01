/* Desktop logoff is an explicit platform difference, not a successful no-op. */
#include <wincompat_calls.h>
#include <stdio.h>

BOOL win32::ExitWindowsEx(UINT uFlags, DWORD dwReason) {
    (void)uFlags;
    (void)dwReason;
    win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
    return 0;
}

BOOL ui::RequestQuit(int nExit) {
    if (!win32::ExitWindowsEx(win32::EWX_LOGOFF, UINT32_MAX)) {
        fprintf(stderr, "stars-sdl: desktop logoff is unsupported; exiting Stars only\n");
        win32::PostQuitMessage(nExit);
        return 0;
    }
    return 1;
}
