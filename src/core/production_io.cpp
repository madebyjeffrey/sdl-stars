#include "production_io.h"

static thread_local const PRODUCTIONIO *lpIO;
const PRODUCTIONIO                     &ProductionIO() noexcept {
    static const PRODUCTIONIO io;
    return lpIO ? *lpIO : io;
}
PRODUCTIONIOSCOPE::PRODUCTIONIOSCOPE(const PRODUCTIONIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PRODUCTIONIOSCOPE::~PRODUCTIONIOSCOPE() noexcept { lpIO = lpPrev; }
