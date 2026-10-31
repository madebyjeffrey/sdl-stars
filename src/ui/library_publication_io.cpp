#include "library_publication_io.h"

static thread_local const LIBRARYPUBLICATIONIO *lpIO;
const LIBRARYPUBLICATIONIO                     &LibraryPublicationIO() noexcept {
    static const LIBRARYPUBLICATIONIO io;
    return lpIO ? *lpIO : io;
}
LIBRARYPUBLICATIONIOSCOPE::LIBRARYPUBLICATIONIOSCOPE(const LIBRARYPUBLICATIONIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
LIBRARYPUBLICATIONIOSCOPE::~LIBRARYPUBLICATIONIOSCOPE() noexcept { lpIO = lpPrev; }
