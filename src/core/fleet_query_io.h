#ifndef STARS_FLEETQUERYIO_H
#define STARS_FLEETQUERYIO_H
#include "common.h"

struct FLEETQUERYIO {
    decltype(&FLookupFleet) pfnLookup = FLookupFleet;
};
const FLEETQUERYIO &FleetQueryIO() noexcept;
class FLEETQUERYIOSCOPE {
    FLEETQUERYIO        io;
    const FLEETQUERYIO *lpPrev;

  public:
    explicit FLEETQUERYIOSCOPE(const FLEETQUERYIO &io) noexcept;
    ~FLEETQUERYIOSCOPE() noexcept;
    FLEETQUERYIOSCOPE(const FLEETQUERYIOSCOPE &) = delete;
    FLEETQUERYIOSCOPE &operator=(const FLEETQUERYIOSCOPE &) = delete;
};
#endif
