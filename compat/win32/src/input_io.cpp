#include "input_io.h"

static thread_local const COMPATINPUTIO *lpIO;
const COMPATINPUTIO                     &CompatInputIO() noexcept {
    static const COMPATINPUTIO io;
    return lpIO ? *lpIO : io;
}
COMPATINPUTIOSCOPE::COMPATINPUTIOSCOPE(const COMPATINPUTIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATINPUTIOSCOPE::~COMPATINPUTIOSCOPE() noexcept { lpIO = lpPrev; }
