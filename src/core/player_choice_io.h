#ifndef STARS_PLAYERCHOICEIO_H
#define STARS_PLAYERCHOICEIO_H
#include "common.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct PLAYERCHOICEIO {
    decltype(&PromptPassword)            pfnPromptPassword = PromptPassword;
    decltype(&FPromptLegacyRacePassword) pfnFPromptLegacyRacePassword = FPromptLegacyRacePassword;
};
const PLAYERCHOICEIO &PlayerChoiceIO() noexcept;
class PLAYERCHOICEIOSCOPE {
    PLAYERCHOICEIO        io;
    const PLAYERCHOICEIO *lpPrev;

  public:
    explicit PLAYERCHOICEIOSCOPE(const PLAYERCHOICEIO &io) noexcept;
    ~PLAYERCHOICEIOSCOPE() noexcept;
    PLAYERCHOICEIOSCOPE(const PLAYERCHOICEIOSCOPE &) = delete;
    PLAYERCHOICEIOSCOPE &operator=(const PLAYERCHOICEIOSCOPE &) = delete;
};
#endif
