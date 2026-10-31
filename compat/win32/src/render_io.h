#ifndef STARS_COMPATRENDERIO_H
#define STARS_COMPATRENDERIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATRENDERIO {
    decltype(&SDL_RenderPresent)  pfnRenderPresent = SDL_RenderPresent;
    decltype(&SDL_DestroySurface) pfnDestroySurface = SDL_DestroySurface;
};
const COMPATRENDERIO &CompatRenderIO() noexcept;
class COMPATRENDERIOSCOPE {
    COMPATRENDERIO        io;
    const COMPATRENDERIO *lpPrev;

  public:
    explicit COMPATRENDERIOSCOPE(const COMPATRENDERIO &io) noexcept;
    ~COMPATRENDERIOSCOPE() noexcept;
    COMPATRENDERIOSCOPE(const COMPATRENDERIOSCOPE &) = delete;
    COMPATRENDERIOSCOPE &operator=(const COMPATRENDERIOSCOPE &) = delete;
};
#endif
