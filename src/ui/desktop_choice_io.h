#ifndef STARS_DESKTOPCHOICEIO_H
#define STARS_DESKTOPCHOICEIO_H
#include "win.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct DESKTOPCHOICEIO {
    decltype(&InitInstance)        pfnInitInstance = InitInstance;
    decltype(&AskSaveDialog)       pfnAskSaveDialog = AskSaveDialog;
    decltype(&NewGameWizard)       pfnNewGameWizard = NewGameWizard;
};
const DESKTOPCHOICEIO &DesktopChoiceIO() noexcept;
class DESKTOPCHOICEIOSCOPE {
    DESKTOPCHOICEIO        io;
    const DESKTOPCHOICEIO *lpPrev;

  public:
    explicit DESKTOPCHOICEIOSCOPE(const DESKTOPCHOICEIO &io) noexcept;
    ~DESKTOPCHOICEIOSCOPE() noexcept;
    DESKTOPCHOICEIOSCOPE(const DESKTOPCHOICEIOSCOPE &) = delete;
    DESKTOPCHOICEIOSCOPE &operator=(const DESKTOPCHOICEIOSCOPE &) = delete;
};
#endif
