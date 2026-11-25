#ifndef STARS_COMPAT_FILE_READ_H
#define STARS_COMPAT_FILE_READ_H
#include "fileio.h"

// The legacy file adapter owns its lock; the provider supplies only the read.
class COMPATFILEREADSCOPE {
    decltype(&CbReadNativeFile) pfnPrev;
public:
    explicit COMPATFILEREADSCOPE(decltype(&CbReadNativeFile) pfnRead) noexcept;
    ~COMPATFILEREADSCOPE() noexcept;
    COMPATFILEREADSCOPE(const COMPATFILEREADSCOPE &) = delete;
    COMPATFILEREADSCOPE &operator=(const COMPATFILEREADSCOPE &) = delete;
};
#endif
