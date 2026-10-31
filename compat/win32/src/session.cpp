/* Desktop logoff is an explicit platform difference, not a successful no-op. */
#include <windows.h>
#include <stdio.h>

BOOL win32::ExitWindowsEx(UINT uFlags, DWORD dwReason) {
    (void)uFlags;
    (void)dwReason;
    win32::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL StarsCompatExitWindows(int nExit) {
    if (!win32::ExitWindowsEx(EWX_LOGOFF, UINT32_MAX)) {
        fprintf(stderr, "stars-sdl: desktop logoff is unsupported; exiting Stars only\n");
        win32::PostQuitMessage(nExit);
        return FALSE;
    }
    return TRUE;
}
