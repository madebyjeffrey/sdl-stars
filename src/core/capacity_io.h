#ifndef STARS_CAPACITYIO_H
#define STARS_CAPACITYIO_H
#include "common.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct CAPACITYIO {
    decltype(&PctPlanetCapacity) pfnPctPlanetCapacity = PctPlanetCapacity;
};
const CAPACITYIO &CapacityIO() noexcept;
class CAPACITYIOSCOPE {
    CAPACITYIO        io;
    const CAPACITYIO *lpPrev;

  public:
    explicit CAPACITYIOSCOPE(const CAPACITYIO &io) noexcept;
    ~CAPACITYIOSCOPE() noexcept;
    CAPACITYIOSCOPE(const CAPACITYIOSCOPE &) = delete;
    CAPACITYIOSCOPE &operator=(const CAPACITYIOSCOPE &) = delete;
};
#endif
