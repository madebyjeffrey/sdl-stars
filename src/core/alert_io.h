#ifndef STARS_ALERTIO_H
#define STARS_ALERTIO_H
#include "common.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct ALERTIO {
    decltype(&AlertSz) pfnAlertSz = AlertSz;
};
const ALERTIO &AlertIO() noexcept;
class ALERTIOSCOPE {
    ALERTIO        io;
    const ALERTIO *lpPrev;

  public:
    explicit ALERTIOSCOPE(const ALERTIO &io) noexcept;
    ~ALERTIOSCOPE() noexcept;
    ALERTIOSCOPE(const ALERTIOSCOPE &) = delete;
    ALERTIOSCOPE &operator=(const ALERTIOSCOPE &) = delete;
};
#endif
