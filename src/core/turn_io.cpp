#include "turn_io.h"

static thread_local const TURNIO *lpIO;
const TURNIO                     &TurnIO() noexcept {
    static const TURNIO io;
    return lpIO ? *lpIO : io;
}
TURNIOSCOPE::TURNIOSCOPE(const TURNIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
TURNIOSCOPE::~TURNIOSCOPE() noexcept { lpIO = lpPrev; }
