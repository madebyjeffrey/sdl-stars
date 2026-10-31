#include "native_publish.h"
#include "native.h"

static thread_local const NATIVEPUBLISHIO *lpPublishIO;
const NATIVEPUBLISHIO                     &NativePublishIO() noexcept {
    static const NATIVEPUBLISHIO io;
    return lpPublishIO ? *lpPublishIO : io;
}
NATIVEPUBLISHIOSCOPE::NATIVEPUBLISHIOSCOPE(const NATIVEPUBLISHIO &io) noexcept : io(io), lpPrev(lpPublishIO) { lpPublishIO = &this->io; }
NATIVEPUBLISHIOSCOPE::~NATIVEPUBLISHIOSCOPE() noexcept { lpPublishIO = lpPrev; }

int RemoveNativeFile(const char *psz) { return NativePublishIO().pfnRemove(psz, FALSE); }
int ReplaceNativeFile(const char *pszOld, const char *pszNew) { return NativePublishIO().pfnRename(pszOld, pszNew, TRUE, FALSE); }
int RenameNativeFile(const char *pszOld, const char *pszNew) { return NativePublishIO().pfnRename(pszOld, pszNew, FALSE, FALSE); }

int RemoveNativePath(const char *psz) { return NativePublishIO().pfnRemove(psz, TRUE); }
