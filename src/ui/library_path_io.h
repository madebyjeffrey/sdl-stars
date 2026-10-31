#ifndef STARS_LIBRARYPATHIO_H
#define STARS_LIBRARYPATHIO_H
#include "libraryfiles.h"
struct LIBRARYPATHIO {
    decltype(&FPathUnderLibrary) pfnUnder = FPathUnderLibrary;
};
const LIBRARYPATHIO &LibraryPathIO() noexcept;
class LIBRARYPATHIOSCOPE {
    LIBRARYPATHIO        io;
    const LIBRARYPATHIO *lpPrev;

  public:
    explicit LIBRARYPATHIOSCOPE(const LIBRARYPATHIO &io) noexcept;
    ~LIBRARYPATHIOSCOPE() noexcept;
    LIBRARYPATHIOSCOPE(const LIBRARYPATHIOSCOPE &) = delete;
    LIBRARYPATHIOSCOPE &operator=(const LIBRARYPATHIOSCOPE &) = delete;
};
#endif
