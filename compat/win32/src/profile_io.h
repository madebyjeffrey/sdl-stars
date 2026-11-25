#ifndef STARS_COMPAT_PROFILE_IO_H
#define STARS_COMPAT_PROFILE_IO_H
#include <windows.h>
#include "fileio.h"
#include <optional>
#include <string>
#include <string_view>

struct COMPATPROFILE {
    std::string szRaw, szParsed;
    std::optional<std::string_view> pszSection, pszKey, pszValue;
    struct COMPATPROFILE *lpNext;
};

// Profile parsing and publication use the same native service as the product.
// A provider changes only I/O effects; the serializer and stage order stay linked.
// ReadLine belongs to loading; the other effects belong to saving. Close and
// Unlink run during noexcept cleanup and must not throw. Profile nodes own
// their raw/parsed lines; line reads return owned string values.
std::optional<std::string> SzProfileTempTemplate(std::string_view szPath);
struct PROFILEIO {
    decltype(&SzProfileTempTemplate) pfnTempTemplate = SzProfileTempTemplate;
    decltype(&PfNativeOpen) pfnOpen = PfNativeOpen;
    decltype(&SzGetNativeLine) pfnReadLine = SzGetNativeLine;
    decltype(&NftNativeCreateTemp) pfnCreateTemp = NftNativeCreateTemp;
    decltype(&ICopyNativePermissions) pfnCopyPermissions = ICopyNativePermissions;
    decltype(&CbWriteNativeStream) pfnWrite = CbWriteNativeStream;
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
bool FSaveProfile(COMPATPROFILE *lp, std::string_view pszPath, const PROFILEIO &io = ProfileIO());
#endif
