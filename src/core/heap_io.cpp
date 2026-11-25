#include "heap_io.h"

static thread_local const HEAPIO *lpIO;
const HEAPIO                     &HeapIO() noexcept {
    static const HEAPIO io;
    return lpIO ? *lpIO : io;
}
HEAPIOSCOPE::HEAPIOSCOPE(const HEAPIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
HEAPIOSCOPE::~HEAPIOSCOPE() noexcept { lpIO = lpPrev; }
