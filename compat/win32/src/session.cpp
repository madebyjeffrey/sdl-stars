/* Desktop logoff is an explicit platform difference, not a successful no-op. */
#include <windows.h>
#include <stdio.h>

BOOL WINAPI ExitWindowsEx(UINT uFlags, DWORD dwReason) {
    (void)uFlags;
    (void)dwReason;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL StarsCompatExitWindows(int nExit) {
    if (!ExitWindowsEx(EWX_LOGOFF, UINT32_MAX)) {
        fprintf(stderr, "stars-sdl: desktop logoff is unsupported; exiting Stars only\n");
        PostQuitMessage(nExit);
        return FALSE;
    }
    return TRUE;
}
