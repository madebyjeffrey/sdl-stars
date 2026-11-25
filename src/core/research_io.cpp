#include "research_io.h"

static thread_local const RESEARCHIO *lpIO;
const RESEARCHIO                     &ResearchIO() noexcept {
    static const RESEARCHIO io;
    return lpIO ? *lpIO : io;
}
RESEARCHIOSCOPE::RESEARCHIOSCOPE(const RESEARCHIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
RESEARCHIOSCOPE::~RESEARCHIOSCOPE() noexcept { lpIO = lpPrev; }
