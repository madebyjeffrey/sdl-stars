#include "external_io.h"

static thread_local const COMPATEXTERNALIO *lpIO;
const COMPATEXTERNALIO                     &CompatExternalIO() noexcept {
    static const COMPATEXTERNALIO io;
    return lpIO ? *lpIO : io;
}
COMPATEXTERNALIOSCOPE::COMPATEXTERNALIOSCOPE(const COMPATEXTERNALIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATEXTERNALIOSCOPE::~COMPATEXTERNALIOSCOPE() noexcept { lpIO = lpPrev; }
