#ifndef STARS_PLANETQUERYIO_H
#define STARS_PLANETQUERYIO_H
#include "common.h"

struct PLANETQUERYIO {
    void (*pfnTruncate)(double) noexcept = nullptr;
    decltype(&FLookupPlanet)         pfnLookup = FLookupPlanet;
    decltype(&PctPlanetDesirability) pfnDesirability = PctPlanetDesirability;
    decltype(&ChgPopFromPlanet)      pfnGrowth = ChgPopFromPlanet;
};
const PLANETQUERYIO &PlanetQueryIO() noexcept;
class PLANETQUERYIOSCOPE {
    PLANETQUERYIO        io;
    const PLANETQUERYIO *lpPrev;

  public:
    explicit PLANETQUERYIOSCOPE(const PLANETQUERYIO &io) noexcept;
    ~PLANETQUERYIOSCOPE() noexcept;
    PLANETQUERYIOSCOPE(const PLANETQUERYIOSCOPE &) = delete;
    PLANETQUERYIOSCOPE &operator=(const PLANETQUERYIOSCOPE &) = delete;
};
// Existing binary64-to-int32 boundary; inputs must be representable.
int32_t LFromPlanetValue(double dValue);
#endif
