#ifndef STARS_COMPATEXTERNALIO_H
#define STARS_COMPATEXTERNALIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATEXTERNALIO {
    decltype(&SDL_OpenURL)              pfnOpenURL = SDL_OpenURL;
    decltype(&SDL_ShowSimpleMessageBox) pfnShowSimpleMessageBox = SDL_ShowSimpleMessageBox;
};
const COMPATEXTERNALIO &CompatExternalIO() noexcept;
class COMPATEXTERNALIOSCOPE {
    COMPATEXTERNALIO        io;
    const COMPATEXTERNALIO *lpPrev;

  public:
    explicit COMPATEXTERNALIOSCOPE(const COMPATEXTERNALIO &io) noexcept;
    ~COMPATEXTERNALIOSCOPE() noexcept;
    COMPATEXTERNALIOSCOPE(const COMPATEXTERNALIOSCOPE &) = delete;
    COMPATEXTERNALIOSCOPE &operator=(const COMPATEXTERNALIOSCOPE &) = delete;
};
#endif
