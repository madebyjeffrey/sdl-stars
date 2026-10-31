#include "window_io.h"

static thread_local const COMPATWINDOWIO *lpIO;
const COMPATWINDOWIO                     &CompatWindowIO() noexcept {
    static const COMPATWINDOWIO io;
    return lpIO ? *lpIO : io;
}
COMPATWINDOWIOSCOPE::COMPATWINDOWIOSCOPE(const COMPATWINDOWIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATWINDOWIOSCOPE::~COMPATWINDOWIOSCOPE() noexcept { lpIO = lpPrev; }
