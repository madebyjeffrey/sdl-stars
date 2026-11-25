#include "text_io.h"

static thread_local const TEXTIO *lpIO;
const TEXTIO                     &TextIO() noexcept {
    static const TEXTIO io;
    return lpIO ? *lpIO : io;
}
TEXTIOSCOPE::TEXTIOSCOPE(const TEXTIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
TEXTIOSCOPE::~TEXTIOSCOPE() noexcept { lpIO = lpPrev; }
