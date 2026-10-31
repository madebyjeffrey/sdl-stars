#include "part_query_io.h"

static thread_local const PARTQUERYIO *lpIO;
const PARTQUERYIO                     &PartQueryIO() noexcept {
    static const PARTQUERYIO io;
    return lpIO ? *lpIO : io;
}
PARTQUERYIOSCOPE::PARTQUERYIOSCOPE(const PARTQUERYIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PARTQUERYIOSCOPE::~PARTQUERYIOSCOPE() noexcept { lpIO = lpPrev; }
