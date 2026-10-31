#include "control_io.h"

static thread_local const CONTROLIO *lpIO;
const CONTROLIO                     &ControlIO() noexcept {
    static const CONTROLIO io;
    return lpIO ? *lpIO : io;
}
CONTROLIOSCOPE::CONTROLIOSCOPE(const CONTROLIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
CONTROLIOSCOPE::~CONTROLIOSCOPE() noexcept { lpIO = lpPrev; }
