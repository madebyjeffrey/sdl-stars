#include "library_file_io.h"

static thread_local const LIBRARYFILEIO *lpIO;
const LIBRARYFILEIO                     &LibraryFileIO() noexcept {
    static const LIBRARYFILEIO io;
    return lpIO ? *lpIO : io;
}
LIBRARYFILEIOSCOPE::LIBRARYFILEIOSCOPE(const LIBRARYFILEIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
LIBRARYFILEIOSCOPE::~LIBRARYFILEIOSCOPE() noexcept { lpIO = lpPrev; }
