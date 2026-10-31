#include "profile_text_io.h"

static thread_local const PROFILETEXTIO *lpIO;
const PROFILETEXTIO                     &ProfileTextIO() noexcept {
    static const PROFILETEXTIO io;
    return lpIO ? *lpIO : io;
}
PROFILETEXTIOSCOPE::PROFILETEXTIOSCOPE(const PROFILETEXTIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PROFILETEXTIOSCOPE::~PROFILETEXTIOSCOPE() noexcept { lpIO = lpPrev; }
