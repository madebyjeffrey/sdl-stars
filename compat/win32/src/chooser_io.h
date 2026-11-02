#ifndef STARS_COMPATCHOOSERIO_H
#define STARS_COMPATCHOOSERIO_H
#include "filedialog_backend.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATCHOOSERIO {
    FILESELECTIONBACKEND pfnICompatChooseFile = ICompatChooseFile;
};
const COMPATCHOOSERIO &CompatChooserIO() noexcept;
class COMPATCHOOSERIOSCOPE {
    COMPATCHOOSERIO        io;
    const COMPATCHOOSERIO *lpPrev;

  public:
    explicit COMPATCHOOSERIOSCOPE(const COMPATCHOOSERIO &io) noexcept;
    ~COMPATCHOOSERIOSCOPE() noexcept;
    COMPATCHOOSERIOSCOPE(const COMPATCHOOSERIOSCOPE &) = delete;
    COMPATCHOOSERIOSCOPE &operator=(const COMPATCHOOSERIOSCOPE &) = delete;
};
#endif
