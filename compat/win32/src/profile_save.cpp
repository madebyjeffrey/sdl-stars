#include "profile_io.h"
#include "ownership.h"
#include <cerrno>
#include <cstdio>
#include <new>

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

std::optional<std::string> SzProfileTempTemplate(std::string_view szPath) {
    try { return std::string(szPath) + ".XXXXXX"; }
    catch (const std::bad_alloc &) { errno = ENOMEM; }
    catch (...) { errno = EIO; }
    return std::nullopt;
}

bool FSaveProfile(COMPATPROFILE *lp, std::string_view pszPath, const PROFILEIO &io) {
    std::optional<std::string> szTemplate;
    NATIVEFILETEMPRESULT nft;
    NATIVEFILE *fp = nullptr;
    bool fRet = false;
    bool fTemporary = false;
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept {
        if (fp) io.pfnClose(std::exchange(fp, nullptr));
        if (!fRet && fTemporary) io.pfnUnlink(nft.szPath);
    });

    szTemplate = io.pfnTempTemplate(pszPath);
    if (!szTemplate) { win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); return false; }
    nft = io.pfnCreateTemp(*szTemplate);
    fp = nft.lpfile.release();
    if (!fp) { win32::SetLastError(DwProfileError()); goto LReturn; }
    fTemporary = true;
    if (io.pfnCopyPermissions(fp, pszPath) != 0) {
        win32::SetLastError(DwProfileError()); io.pfnClose(std::exchange(fp, nullptr)); goto LReturn;
    }
    if (io.pfnWrite(std::as_bytes(std::span{"\xef\xbb\xbf"}).first(3), fp) != 3) { io.pfnClose(std::exchange(fp, nullptr)); goto LReturn; }
    for (; lp; lp = lp->lpNext) {
        if (io.pfnPutString(lp->szRaw.c_str(), fp) == EOF) break;
        if (lp->lpNext && !lp->szRaw.empty() && lp->szRaw.back() != '\n')
            if (io.pfnPutChar('\n', fp) == EOF) break;
    }
    if (lp || io.pfnFlush(fp) != 0 || io.pfnSync(fp) != 0) {
        win32::SetLastError(DwProfileError()); io.pfnClose(std::exchange(fp, nullptr)); goto LReturn;
    }
    if (io.pfnClose(std::exchange(fp, nullptr)) != 0 || io.pfnRename(nft.szPath, pszPath, 1, 1) != 0) { win32::SetLastError(DwProfileError()); goto LReturn; }
    fRet = true;
LReturn:
    return fRet;
}
