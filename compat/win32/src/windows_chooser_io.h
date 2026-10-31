#ifndef STARS_WINDOWSCHOOSERIO_H
#define STARS_WINDOWSCHOOSERIO_H
#include "filedialog_windows_bridge.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct WINDOWSCHOOSERIO {
    decltype(&IChooseWindowsFile) pfnIChooseWindowsFile = IChooseWindowsFile;
};
const WINDOWSCHOOSERIO &WindowsChooserIO() noexcept;
class WINDOWSCHOOSERIOSCOPE {
    WINDOWSCHOOSERIO        io;
    const WINDOWSCHOOSERIO *lpPrev;

  public:
    explicit WINDOWSCHOOSERIOSCOPE(const WINDOWSCHOOSERIO &io) noexcept;
    ~WINDOWSCHOOSERIOSCOPE() noexcept;
    WINDOWSCHOOSERIOSCOPE(const WINDOWSCHOOSERIOSCOPE &) = delete;
    WINDOWSCHOOSERIOSCOPE &operator=(const WINDOWSCHOOSERIOSCOPE &) = delete;
};
#endif
