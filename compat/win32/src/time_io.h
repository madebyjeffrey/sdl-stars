#ifndef STARS_COMPATTIMEIO_H
#define STARS_COMPATTIMEIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATTIMEIO {
    decltype(&SDL_GetTicks) pfnGetTicks = SDL_GetTicks;
};
const COMPATTIMEIO &CompatTimeIO() noexcept;
class COMPATTIMEIOSCOPE {
    COMPATTIMEIO        io;
    const COMPATTIMEIO *lpPrev;

  public:
    explicit COMPATTIMEIOSCOPE(const COMPATTIMEIO &io) noexcept;
    ~COMPATTIMEIOSCOPE() noexcept;
    COMPATTIMEIOSCOPE(const COMPATTIMEIOSCOPE &) = delete;
    COMPATTIMEIOSCOPE &operator=(const COMPATTIMEIOSCOPE &) = delete;
};
#endif
