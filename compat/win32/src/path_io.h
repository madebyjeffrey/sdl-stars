#ifndef STARS_COMPATPATHIO_H
#define STARS_COMPATPATHIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATPATHIO {
    decltype(&SDL_GetBasePath) pfnGetBasePath = SDL_GetBasePath;
    decltype(&SDL_GetPrefPath) pfnGetPrefPath = SDL_GetPrefPath;
};
const COMPATPATHIO &CompatPathIO() noexcept;
class COMPATPATHIOSCOPE {
    COMPATPATHIO        io;
    const COMPATPATHIO *lpPrev;

  public:
    explicit COMPATPATHIOSCOPE(const COMPATPATHIO &io) noexcept;
    ~COMPATPATHIOSCOPE() noexcept;
    COMPATPATHIOSCOPE(const COMPATPATHIOSCOPE &) = delete;
    COMPATPATHIOSCOPE &operator=(const COMPATPATHIOSCOPE &) = delete;
};
#endif
