#include "library_path_io.h"
static thread_local const LIBRARYPATHIO *lpIO;
const LIBRARYPATHIO                     &LibraryPathIO() noexcept {
    static const LIBRARYPATHIO io;
    return lpIO ? *lpIO : io;
}
LIBRARYPATHIOSCOPE::LIBRARYPATHIOSCOPE(const LIBRARYPATHIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
LIBRARYPATHIOSCOPE::~LIBRARYPATHIOSCOPE() noexcept { lpIO = lpPrev; }
