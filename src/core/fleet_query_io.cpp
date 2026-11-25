#include "fleet_query_io.h"

static thread_local const FLEETQUERYIO *lpIO;
const FLEETQUERYIO                     &FleetQueryIO() noexcept {
    static const FLEETQUERYIO io;
    return lpIO ? *lpIO : io;
}
FLEETQUERYIOSCOPE::FLEETQUERYIOSCOPE(const FLEETQUERYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
FLEETQUERYIOSCOPE::~FLEETQUERYIOSCOPE() noexcept { lpIO = lpPrev; }
