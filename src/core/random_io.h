#ifndef STARS_RANDOMIO_H
#define STARS_RANDOMIO_H
#include "common.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct RANDOMIO {
    decltype(&Random) pfnRandom = Random;
};
const RANDOMIO &RandomIO() noexcept;
class RANDOMIOSCOPE {
    RANDOMIO        io;
    const RANDOMIO *lpPrev;

  public:
    explicit RANDOMIOSCOPE(const RANDOMIO &io) noexcept;
    ~RANDOMIOSCOPE() noexcept;
    RANDOMIOSCOPE(const RANDOMIOSCOPE &) = delete;
    RANDOMIOSCOPE &operator=(const RANDOMIOSCOPE &) = delete;
};
#endif
