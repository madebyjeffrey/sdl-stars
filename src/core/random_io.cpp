#include "random_io.h"

static thread_local const RANDOMIO *lpIO;
const RANDOMIO                     &RandomIO() noexcept {
    static const RANDOMIO io;
    return lpIO ? *lpIO : io;
}
RANDOMIOSCOPE::RANDOMIOSCOPE(const RANDOMIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
RANDOMIOSCOPE::~RANDOMIOSCOPE() noexcept { lpIO = lpPrev; }
