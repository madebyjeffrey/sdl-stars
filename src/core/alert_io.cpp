#include "alert_io.h"

static thread_local const ALERTIO *lpIO;
const ALERTIO                     &AlertIO() noexcept {
    static const ALERTIO io;
    return lpIO ? *lpIO : io;
}
ALERTIOSCOPE::ALERTIOSCOPE(const ALERTIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
ALERTIOSCOPE::~ALERTIOSCOPE() noexcept { lpIO = lpPrev; }
