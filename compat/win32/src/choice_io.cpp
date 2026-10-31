#include "choice_io.h"

static thread_local const COMPATCHOICEIO *lpIO;
const COMPATCHOICEIO                     &CompatChoiceIO() noexcept {
    static const COMPATCHOICEIO io;
    return lpIO ? *lpIO : io;
}
COMPATCHOICEIOSCOPE::COMPATCHOICEIOSCOPE(const COMPATCHOICEIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATCHOICEIOSCOPE::~COMPATCHOICEIOSCOPE() noexcept { lpIO = lpPrev; }
