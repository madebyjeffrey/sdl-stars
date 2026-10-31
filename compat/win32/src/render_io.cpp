#include "render_io.h"

static thread_local const COMPATRENDERIO *lpIO;
const COMPATRENDERIO                     &CompatRenderIO() noexcept {
    static const COMPATRENDERIO io;
    return lpIO ? *lpIO : io;
}
COMPATRENDERIOSCOPE::COMPATRENDERIOSCOPE(const COMPATRENDERIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATRENDERIOSCOPE::~COMPATRENDERIOSCOPE() noexcept { lpIO = lpPrev; }
