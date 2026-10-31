#include "display_io.h"

static thread_local const COMPATDISPLAYIO *lpIO;
const COMPATDISPLAYIO                     &CompatDisplayIO() noexcept {
    static const COMPATDISPLAYIO io;
    return lpIO ? *lpIO : io;
}
COMPATDISPLAYIOSCOPE::COMPATDISPLAYIOSCOPE(const COMPATDISPLAYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATDISPLAYIOSCOPE::~COMPATDISPLAYIOSCOPE() noexcept { lpIO = lpPrev; }
