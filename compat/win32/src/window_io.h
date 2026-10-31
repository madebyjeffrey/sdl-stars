#ifndef STARS_COMPATWINDOWIO_H
#define STARS_COMPATWINDOWIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATWINDOWIO {
    decltype(&SDL_ShowWindow)     pfnShowWindow = SDL_ShowWindow;
    decltype(&SDL_SetWindowTitle) pfnSetWindowTitle = SDL_SetWindowTitle;
    decltype(&SDL_FlashWindow)    pfnFlashWindow = SDL_FlashWindow;
};
const COMPATWINDOWIO &CompatWindowIO() noexcept;
class COMPATWINDOWIOSCOPE {
    COMPATWINDOWIO        io;
    const COMPATWINDOWIO *lpPrev;

  public:
    explicit COMPATWINDOWIOSCOPE(const COMPATWINDOWIO &io) noexcept;
    ~COMPATWINDOWIOSCOPE() noexcept;
    COMPATWINDOWIOSCOPE(const COMPATWINDOWIOSCOPE &) = delete;
    COMPATWINDOWIOSCOPE &operator=(const COMPATWINDOWIOSCOPE &) = delete;
};
#endif
