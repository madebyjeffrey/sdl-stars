#include "capacity_io.h"

static thread_local const CAPACITYIO *lpIO;
const CAPACITYIO                     &CapacityIO() noexcept {
    static const CAPACITYIO io;
    return lpIO ? *lpIO : io;
}
CAPACITYIOSCOPE::CAPACITYIOSCOPE(const CAPACITYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
CAPACITYIOSCOPE::~CAPACITYIOSCOPE() noexcept { lpIO = lpPrev; }
