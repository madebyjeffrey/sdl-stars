#include "desktop_choice_io.h"

static thread_local const DESKTOPCHOICEIO *lpIO;
const DESKTOPCHOICEIO                     &DesktopChoiceIO() noexcept {
    static const DESKTOPCHOICEIO io;
    return lpIO ? *lpIO : io;
}
DESKTOPCHOICEIOSCOPE::DESKTOPCHOICEIOSCOPE(const DESKTOPCHOICEIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
DESKTOPCHOICEIOSCOPE::~DESKTOPCHOICEIOSCOPE() noexcept { lpIO = lpPrev; }
