#ifndef STARS_COMPAT_PROFILE_IO_H
#define STARS_COMPAT_PROFILE_IO_H
#include <windows.h>
#include "fileio.h"
#include <cstdlib>

struct COMPATPROFILE {
    char *szRaw, *szParsed, *pszSection, *pszKey, *pszValue;
    struct COMPATPROFILE *lpNext;
};

// Profile parsing and publication use the same native service as the product.
// A provider changes only I/O effects; the serializer and stage order stay linked.
// ReadLine belongs to loading; the other effects belong to saving. Close and
// Unlink run during noexcept cleanup and must not throw. Allocations use free().
struct PROFILEIO {
    void *(*pfnAllocate)(size_t) = std::malloc;
    decltype(&PfNativeOpen) pfnOpen = PfNativeOpen;
    decltype(&CbGetNativeLine) pfnReadLine = CbGetNativeLine;
    decltype(&PfNativeCreateTemp) pfnCreateTemp = PfNativeCreateTemp;
    decltype(&ICopyNativePermissions) pfnCopyPermissions = ICopyNativePermissions;
    decltype(&CWriteNative) pfnWrite = CWriteNative;
    decltype(&IPutNativeString) pfnPutString = IPutNativeString;
    decltype(&ChPutNativeFile) pfnPutChar = ChPutNativeFile;
    decltype(&IFlushNativeFile) pfnFlush = IFlushNativeFile;
    decltype(&ISyncNativeFile) pfnSync = ISyncNativeFile;
    decltype(&ICloseNativeFile) pfnClose = ICloseNativeFile;
    decltype(&IRenameNativePath) pfnRename = IRenameNativePath;
    decltype(&IUnlinkNativePath) pfnUnlink = IUnlinkNativePath;
};

const PROFILEIO &ProfileIO() noexcept;
class PROFILEIOSCOPE {
    PROFILEIO io;
    const PROFILEIO *lpPrev;
public:
    explicit PROFILEIOSCOPE(const PROFILEIO &io) noexcept;
    ~PROFILEIOSCOPE() noexcept;
    PROFILEIOSCOPE(const PROFILEIOSCOPE &) = delete;
    PROFILEIOSCOPE &operator=(const PROFILEIOSCOPE &) = delete;
};
DWORD DwProfileError();
BOOL FSaveProfile(COMPATPROFILE *lp, const char *pszPath, const PROFILEIO &io = ProfileIO());
#endif
