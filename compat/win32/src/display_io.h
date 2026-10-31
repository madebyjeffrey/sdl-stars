#ifndef STARS_COMPATDISPLAYIO_H
#define STARS_COMPATDISPLAYIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATDISPLAYIO {
    decltype(&SDL_GetDisplays)            pfnGetDisplays = SDL_GetDisplays;
    decltype(&SDL_GetPrimaryDisplay)      pfnGetPrimaryDisplay = SDL_GetPrimaryDisplay;
    decltype(&SDL_GetDisplayBounds)       pfnGetDisplayBounds = SDL_GetDisplayBounds;
    decltype(&SDL_GetDisplayUsableBounds) pfnGetDisplayUsableBounds = SDL_GetDisplayUsableBounds;
};
const COMPATDISPLAYIO &CompatDisplayIO() noexcept;
class COMPATDISPLAYIOSCOPE {
    COMPATDISPLAYIO        io;
    const COMPATDISPLAYIO *lpPrev;

  public:
    explicit COMPATDISPLAYIOSCOPE(const COMPATDISPLAYIO &io) noexcept;
    ~COMPATDISPLAYIOSCOPE() noexcept;
    COMPATDISPLAYIOSCOPE(const COMPATDISPLAYIOSCOPE &) = delete;
    COMPATDISPLAYIOSCOPE &operator=(const COMPATDISPLAYIOSCOPE &) = delete;
};
#endif
