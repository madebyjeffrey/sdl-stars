#ifndef STARS_PARTQUERYIO_H
#define STARS_PARTQUERYIO_H
#include "common.h"

struct PARTQUERYIO {
    decltype(&FLookupPart)         pfnLookup = FLookupPart;
    decltype(&TechStatus)          pfnTechStatus = TechStatus;
    decltype(&FShouldPartBeHidden) pfnHidden = FShouldPartBeHidden;
    decltype(&LphuldefFromId)      pfnHull = LphuldefFromId;
    decltype(&GetTruePartCost)     pfnTrueCost = GetTruePartCost;
};
const PARTQUERYIO &PartQueryIO() noexcept;
class PARTQUERYIOSCOPE {
    PARTQUERYIO        io;
    const PARTQUERYIO *lpPrev;

  public:
    explicit PARTQUERYIOSCOPE(const PARTQUERYIO &io) noexcept;
    ~PARTQUERYIOSCOPE() noexcept;
    PARTQUERYIOSCOPE(const PARTQUERYIOSCOPE &) = delete;
    PARTQUERYIOSCOPE &operator=(const PARTQUERYIOSCOPE &) = delete;
};
#endif
