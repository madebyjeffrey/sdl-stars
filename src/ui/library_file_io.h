#ifndef STARS_LIBRARYFILEIO_H
#define STARS_LIBRARYFILEIO_H
#include "win.h"

struct LIBRARYFILEIO {
    decltype(&PfOpenNativeFile)   pfnOpen = PfOpenNativeFile;
    decltype(&CReadNative)        pfnRead = CReadNative;
    decltype(&CWriteNative)       pfnWrite = CWriteNative;
    decltype(&FNativeStreamError) pfnError = FNativeStreamError;
    decltype(&ICloseNativeFile)   pfnClose = ICloseNativeFile;
};
const LIBRARYFILEIO &LibraryFileIO() noexcept;
class LIBRARYFILEIOSCOPE {
    LIBRARYFILEIO        io;
    const LIBRARYFILEIO *lpPrev;

  public:
    explicit LIBRARYFILEIOSCOPE(const LIBRARYFILEIO &io) noexcept;
    ~LIBRARYFILEIOSCOPE() noexcept;
    LIBRARYFILEIOSCOPE(const LIBRARYFILEIOSCOPE &) = delete;
    LIBRARYFILEIOSCOPE &operator=(const LIBRARYFILEIOSCOPE &) = delete;
};
#endif
