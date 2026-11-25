#include "chooser_io.h"

static thread_local const COMPATCHOOSERIO *lpIO;
const COMPATCHOOSERIO                     &CompatChooserIO() noexcept {
    static const COMPATCHOOSERIO io;
    return lpIO ? *lpIO : io;
}
COMPATCHOOSERIOSCOPE::COMPATCHOOSERIOSCOPE(const COMPATCHOOSERIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATCHOOSERIOSCOPE::~COMPATCHOOSERIOSCOPE() noexcept { lpIO = lpPrev; }
