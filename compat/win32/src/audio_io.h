#ifndef STARS_COMPATAUDIOIO_H
#define STARS_COMPATAUDIOIO_H
#include <SDL3/SDL.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATAUDIOIO {
    decltype(&SDL_PutAudioStreamData) pfnPutAudioStreamData = SDL_PutAudioStreamData;
};
const COMPATAUDIOIO &CompatAudioIO() noexcept;
class COMPATAUDIOIOSCOPE {
    COMPATAUDIOIO        io;
    const COMPATAUDIOIO *lpPrev;

  public:
    explicit COMPATAUDIOIOSCOPE(const COMPATAUDIOIO &io) noexcept;
    ~COMPATAUDIOIOSCOPE() noexcept;
    COMPATAUDIOIOSCOPE(const COMPATAUDIOIOSCOPE &) = delete;
    COMPATAUDIOIOSCOPE &operator=(const COMPATAUDIOIOSCOPE &) = delete;
};
#endif
