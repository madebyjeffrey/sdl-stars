#include "profile_io.h"
#include "ownership.h"
#include <cerrno>
#include <cstdio>
#include <cstring>

// Bind only this thread's profile operation; nested callbacks restore the
// previous provider, and each save borrows a stable provider through cleanup.
static thread_local const PROFILEIO *lpProfileIO;
const PROFILEIO &ProfileIO() noexcept {
    static const PROFILEIO io;
    return lpProfileIO ? *lpProfileIO : io;
}
PROFILEIOSCOPE::PROFILEIOSCOPE(const PROFILEIO &io) noexcept : io(io), lpPrev(lpProfileIO) { lpProfileIO = &this->io; }
PROFILEIOSCOPE::~PROFILEIOSCOPE() noexcept { lpProfileIO = lpPrev; }

DWORD DwProfileError(void) {
    if (errno == ENOENT) return win32::ERROR_FILE_NOT_FOUND;
    if (errno == EACCES || errno == EPERM) return win32::ERROR_ACCESS_DENIED;
    if (errno == ENOMEM) return win32::ERROR_NOT_ENOUGH_MEMORY;
    return win32::ERROR_WRITE_FAULT;
}

BOOL FSaveProfile(COMPATPROFILE *lp, const char *pszPath, const PROFILEIO &io) {
    char *szTemp = nullptr;
    NATIVEFILE *fp = nullptr;
    BOOL fRet = 0;
    BOOL fTemporary = 0;
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept {
        if (fp) io.pfnClose(std::exchange(fp, nullptr));
        if (!fRet && fTemporary) io.pfnUnlink(szTemp);
        free(szTemp);
    });

    szTemp = static_cast<char *>(io.pfnAllocate(strlen(pszPath) + 12));
    if (!szTemp) { win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); return 0; }
    snprintf(szTemp, strlen(pszPath) + 12, "%s.XXXXXX", pszPath);
    fp = io.pfnCreateTemp(szTemp);
    if (!fp) { win32::SetLastError(DwProfileError()); goto LReturn; }
    fTemporary = 1;
    if (io.pfnCopyPermissions(fp, pszPath) != 0) {
        win32::SetLastError(DwProfileError()); io.pfnClose(std::exchange(fp, nullptr)); goto LReturn;
    }
    if (io.pfnWrite("\xef\xbb\xbf", 1, 3, fp) != 3) { io.pfnClose(std::exchange(fp, nullptr)); goto LReturn; }
    for (; lp; lp = lp->lpNext) {
        if (io.pfnPutString(lp->szRaw, fp) == EOF) break;
        if (lp->lpNext && *lp->szRaw && lp->szRaw[strlen(lp->szRaw) - 1] != '\n')
            if (io.pfnPutChar('\n', fp) == EOF) break;
    }
    if (lp || io.pfnFlush(fp) != 0 || io.pfnSync(fp) != 0) {
        win32::SetLastError(DwProfileError()); io.pfnClose(std::exchange(fp, nullptr)); goto LReturn;
    }
    if (io.pfnClose(std::exchange(fp, nullptr)) != 0 || io.pfnRename(szTemp, pszPath, 1, 1) != 0) { win32::SetLastError(DwProfileError()); goto LReturn; }
    fRet = 1;
LReturn:
    return fRet;
}
