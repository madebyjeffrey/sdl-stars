#ifndef STARS_COMPATFOLDERIO_H
#define STARS_COMPATFOLDERIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATFOLDERIO {
    decltype(&SDL_ShowOpenFolderDialog) pfnShowOpenFolderDialog = SDL_ShowOpenFolderDialog;
};
const COMPATFOLDERIO &CompatFolderIO() noexcept;
class COMPATFOLDERIOSCOPE {
    COMPATFOLDERIO        io;
    const COMPATFOLDERIO *lpPrev;

  public:
    explicit COMPATFOLDERIOSCOPE(const COMPATFOLDERIO &io) noexcept;
    ~COMPATFOLDERIOSCOPE() noexcept;
    COMPATFOLDERIOSCOPE(const COMPATFOLDERIOSCOPE &) = delete;
    COMPATFOLDERIOSCOPE &operator=(const COMPATFOLDERIOSCOPE &) = delete;
};
#endif
