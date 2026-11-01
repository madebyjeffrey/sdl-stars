#include "message_alloc_io.h"

static thread_local const COMPATMESSAGEALLOCIO *lpIO;
const COMPATMESSAGEALLOCIO                     &CompatMessageAllocIO() noexcept {
    static const COMPATMESSAGEALLOCIO io;
    return lpIO ? *lpIO : io;
}
COMPATMESSAGEALLOCIOSCOPE::COMPATMESSAGEALLOCIOSCOPE(const COMPATMESSAGEALLOCIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATMESSAGEALLOCIOSCOPE::~COMPATMESSAGEALLOCIOSCOPE() noexcept { lpIO = lpPrev; }
