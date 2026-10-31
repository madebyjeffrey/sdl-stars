#ifndef STARS_COMPATINPUTIO_H
#define STARS_COMPATINPUTIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATINPUTIO {
    decltype(&SDL_GetKeyboardState)      pfnGetKeyboardState = SDL_GetKeyboardState;
    decltype(&SDL_GetModState)           pfnGetModState = SDL_GetModState;
    decltype(&SDL_GetGlobalMouseState)   pfnGetGlobalMouseState = SDL_GetGlobalMouseState;
    decltype(&SDL_GetCurrentVideoDriver) pfnGetCurrentVideoDriver = SDL_GetCurrentVideoDriver;
};
const COMPATINPUTIO &CompatInputIO() noexcept;
class COMPATINPUTIOSCOPE {
    COMPATINPUTIO        io;
    const COMPATINPUTIO *lpPrev;

  public:
    explicit COMPATINPUTIOSCOPE(const COMPATINPUTIO &io) noexcept;
    ~COMPATINPUTIOSCOPE() noexcept;
    COMPATINPUTIOSCOPE(const COMPATINPUTIOSCOPE &) = delete;
    COMPATINPUTIOSCOPE &operator=(const COMPATINPUTIOSCOPE &) = delete;
};
#endif
