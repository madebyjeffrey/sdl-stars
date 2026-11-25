#include "player_choice_io.h"

static thread_local const PLAYERCHOICEIO *lpIO;
const PLAYERCHOICEIO                     &PlayerChoiceIO() noexcept {
    static const PLAYERCHOICEIO io;
    return lpIO ? *lpIO : io;
}
PLAYERCHOICEIOSCOPE::PLAYERCHOICEIOSCOPE(const PLAYERCHOICEIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
PLAYERCHOICEIOSCOPE::~PLAYERCHOICEIOSCOPE() noexcept { lpIO = lpPrev; }
