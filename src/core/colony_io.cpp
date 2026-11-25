#include "colony_io.h"

static thread_local const COLONYIO *lpIO;
const COLONYIO                     &ColonyIO() noexcept {
    static const COLONYIO io;
    return lpIO ? *lpIO : io;
}
COLONYIOSCOPE::COLONYIOSCOPE(const COLONYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COLONYIOSCOPE::~COLONYIOSCOPE() noexcept { lpIO = lpPrev; }
