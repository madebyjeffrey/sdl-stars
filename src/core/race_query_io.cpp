#include "race_query_io.h"

static thread_local const RACEQUERYIO *lpIO;
const RACEQUERYIO                     &RaceQueryIO() noexcept {
    static const RACEQUERYIO io;
    return lpIO ? *lpIO : io;
}
RACEQUERYIOSCOPE::RACEQUERYIOSCOPE(const RACEQUERYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
RACEQUERYIOSCOPE::~RACEQUERYIOSCOPE() noexcept { lpIO = lpPrev; }
