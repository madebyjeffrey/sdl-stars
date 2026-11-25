#ifndef STARS_PRESENTATIONIO_H
#define STARS_PRESENTATIONIO_H
#include "common.h"

struct PRESENTATIONIO {
    decltype(&InvalidateReport)          pfnInvalidateReport = InvalidateReport;
    decltype(&LogChangePlanet)           pfnLogPlanet = LogChangePlanet;
    decltype(&AdvanceTutor)              pfnTutor = AdvanceTutor;
    decltype(&RedrawPlanShip)            pfnRedraw = RedrawPlanShip;
    decltype(&SelectAdjPlanet)           pfnSelectPlanet = SelectAdjPlanet;
    decltype(&SelectAdjFleet)            pfnSelectFleet = SelectAdjFleet;
    decltype(&FFindSomethingAndSelectIt) pfnFindSelection = FFindSomethingAndSelectIt;
    decltype(&IdmGetMessageN)            pfnMessageId = IdmGetMessageN;
};
const PRESENTATIONIO &PresentationIO() noexcept;
class PRESENTATIONIOSCOPE {
    PRESENTATIONIO        io;
    const PRESENTATIONIO *lpPrev;

  public:
    explicit PRESENTATIONIOSCOPE(const PRESENTATIONIO &io) noexcept;
    ~PRESENTATIONIOSCOPE() noexcept;
    PRESENTATIONIOSCOPE(const PRESENTATIONIOSCOPE &) = delete;
    PRESENTATIONIOSCOPE &operator=(const PRESENTATIONIOSCOPE &) = delete;
};
#endif
