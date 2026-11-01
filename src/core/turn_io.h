#ifndef STARS_TURNIO_H
#define STARS_TURNIO_H
#include "common.h"

struct TURNIO {
    decltype(&FSendPlrMsg)          pfnMessage = FSendPlrMsg;
    decltype(&FSendPlrMsg2)         pfnMessage2 = FSendPlrMsg2;
    decltype(&FRemovePlayerMessage) pfnRemoveMessage = FRemovePlayerMessage;
    decltype(&MineMinerals)         pfnMine = MineMinerals;
    decltype(&UpdatePopulations)    pfnPopulation = UpdatePopulations;
    decltype(&UpdateResearchStatus) pfnResearch = UpdateResearchStatus;
    decltype(&RandomEvents)         pfnEvents = RandomEvents;
};
const TURNIO &TurnIO() noexcept;
class TURNIOSCOPE {
    TURNIO        io;
    const TURNIO *lpPrev;

  public:
    explicit TURNIOSCOPE(const TURNIO &io) noexcept;
    ~TURNIOSCOPE() noexcept;
    TURNIOSCOPE(const TURNIOSCOPE &) = delete;
    TURNIOSCOPE &operator=(const TURNIOSCOPE &) = delete;
};
#endif
