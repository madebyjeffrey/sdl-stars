#ifndef STARS_RACEQUERYIO_H
#define STARS_RACEQUERYIO_H
#include "common.h"

struct RACEQUERYIO {
    decltype(&GetRaceStat)  pfnStat = GetRaceStat;
    decltype(&GetRaceGrbit) pfnGrbit = GetRaceGrbit;
};
const RACEQUERYIO &RaceQueryIO() noexcept;
class RACEQUERYIOSCOPE {
    RACEQUERYIO        io;
    const RACEQUERYIO *lpPrev;

  public:
    explicit RACEQUERYIOSCOPE(const RACEQUERYIO &io) noexcept;
    ~RACEQUERYIOSCOPE() noexcept;
    RACEQUERYIOSCOPE(const RACEQUERYIOSCOPE &) = delete;
    RACEQUERYIOSCOPE &operator=(const RACEQUERYIOSCOPE &) = delete;
};
#endif
