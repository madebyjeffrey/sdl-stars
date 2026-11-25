#include "folder_io.h"

static thread_local const COMPATFOLDERIO *lpIO;
const COMPATFOLDERIO                     &CompatFolderIO() noexcept {
    static const COMPATFOLDERIO io;
    return lpIO ? *lpIO : io;
}
COMPATFOLDERIOSCOPE::COMPATFOLDERIOSCOPE(const COMPATFOLDERIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
COMPATFOLDERIOSCOPE::~COMPATFOLDERIOSCOPE() noexcept { lpIO = lpPrev; }
