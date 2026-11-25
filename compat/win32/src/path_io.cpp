#include "path_io.h"

static thread_local const COMPATPATHIO *lpIO;
const COMPATPATHIO                     &CompatPathIO() noexcept {
    static const COMPATPATHIO io;
    return lpIO ? *lpIO : io;
}
COMPATPATHIOSCOPE::COMPATPATHIOSCOPE(const COMPATPATHIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATPATHIOSCOPE::~COMPATPATHIOSCOPE() noexcept { lpIO = lpPrev; }
