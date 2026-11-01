#ifndef STARS_NATIVE_PUBLISH_H
#define STARS_NATIVE_PUBLISH_H
#include "fileio.h"

// Publication supplies filesystem effects, never a replacement transaction.
struct NATIVEPUBLISHIO {
    decltype(&IRemoveNativePath) pfnRemove = IRemoveNativePath;
    decltype(&IRenameNativePath) pfnRename = IRenameNativePath;
};
const NATIVEPUBLISHIO &NativePublishIO() noexcept;
class NATIVEPUBLISHIOSCOPE {
    NATIVEPUBLISHIO        io;
    const NATIVEPUBLISHIO *lpPrev;

  public:
    explicit NATIVEPUBLISHIOSCOPE(const NATIVEPUBLISHIO &io) noexcept;
    ~NATIVEPUBLISHIOSCOPE() noexcept;
    NATIVEPUBLISHIOSCOPE(const NATIVEPUBLISHIOSCOPE &) = delete;
    NATIVEPUBLISHIOSCOPE &operator=(const NATIVEPUBLISHIOSCOPE &) = delete;
};
#endif
