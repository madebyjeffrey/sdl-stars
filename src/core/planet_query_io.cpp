#include "planet_query_io.h"

static thread_local const PLANETQUERYIO *lpIO;
const PLANETQUERYIO                     &PlanetQueryIO() noexcept {
    static const PLANETQUERYIO io;
    return lpIO ? *lpIO : io;
}
PLANETQUERYIOSCOPE::PLANETQUERYIOSCOPE(const PLANETQUERYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PLANETQUERYIOSCOPE::~PLANETQUERYIOSCOPE() noexcept { lpIO = lpPrev; }
