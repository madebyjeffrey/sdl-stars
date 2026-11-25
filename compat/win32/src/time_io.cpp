#include "time_io.h"

static thread_local const COMPATTIMEIO *lpIO;
const COMPATTIMEIO                     &CompatTimeIO() noexcept {
    static const COMPATTIMEIO io;
    return lpIO ? *lpIO : io;
}
COMPATTIMEIOSCOPE::COMPATTIMEIOSCOPE(const COMPATTIMEIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATTIMEIOSCOPE::~COMPATTIMEIOSCOPE() noexcept { lpIO = lpPrev; }
