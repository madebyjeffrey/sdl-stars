#include "native_publish.h"
#include "native.h"

static thread_local const NATIVEPUBLISHIO *lpPublishIO;
const NATIVEPUBLISHIO                     &NativePublishIO() noexcept {
    static const NATIVEPUBLISHIO io;
    return lpPublishIO ? *lpPublishIO : io;
}
NATIVEPUBLISHIOSCOPE::NATIVEPUBLISHIOSCOPE(const NATIVEPUBLISHIO &io) noexcept : io(io), lpPrev(lpPublishIO) { lpPublishIO = &this->io; }
NATIVEPUBLISHIOSCOPE::~NATIVEPUBLISHIOSCOPE() noexcept { lpPublishIO = lpPrev; }

int RemoveNativeFile(const char *psz) { return NativePublishIO().pfnRemove(psz, 0); }
int ReplaceNativeFile(const char *pszOld, const char *pszNew) { return NativePublishIO().pfnRename(pszOld, pszNew, 1, 0); }
int RenameNativeFile(const char *pszOld, const char *pszNew) { return NativePublishIO().pfnRename(pszOld, pszNew, 0, 0); }

int RemoveNativePath(const char *psz) { return NativePublishIO().pfnRemove(psz, 1); }
