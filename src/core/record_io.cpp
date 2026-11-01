#include "record_io.h"

static thread_local const RECORDIO *lpIO;
const RECORDIO                     &RecordIO() noexcept {
    static const RECORDIO io;
    return lpIO ? *lpIO : io;
}
RECORDIOSCOPE::RECORDIOSCOPE(const RECORDIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
RECORDIOSCOPE::~RECORDIOSCOPE() noexcept { lpIO = lpPrev; }
