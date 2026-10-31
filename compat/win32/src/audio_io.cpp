#include "audio_io.h"

static thread_local const COMPATAUDIOIO *lpIO;
const COMPATAUDIOIO                     &CompatAudioIO() noexcept {
    static const COMPATAUDIOIO io;
    return lpIO ? *lpIO : io;
}
COMPATAUDIOIOSCOPE::COMPATAUDIOIOSCOPE(const COMPATAUDIOIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATAUDIOIOSCOPE::~COMPATAUDIOIOSCOPE() noexcept { lpIO = lpPrev; }
