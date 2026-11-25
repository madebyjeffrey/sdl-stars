#include "presentation_io.h"

static thread_local const PRESENTATIONIO *lpIO;
const PRESENTATIONIO                     &PresentationIO() noexcept {
    static const PRESENTATIONIO io;
    return lpIO ? *lpIO : io;
}
PRESENTATIONIOSCOPE::PRESENTATIONIOSCOPE(const PRESENTATIONIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PRESENTATIONIOSCOPE::~PRESENTATIONIOSCOPE() noexcept { lpIO = lpPrev; }
