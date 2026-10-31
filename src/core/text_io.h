#ifndef STARS_TEXTIO_H
#define STARS_TEXTIO_H
#include "common.h"

struct TEXTIO {
    decltype(&PszPlayerName)          pfnPlayerName = PszPlayerName;
    decltype(&PszGetCompressedString) pfnCompressed = PszGetCompressedString;
    decltype(&PszFormatIds)           pfnFormat = PszFormatIds;
    decltype(&PszGetPlanetName)       pfnPlanetName = PszGetPlanetName;
    decltype(&CchGetString)           pfnCchGetString = CchGetString;
};
const TEXTIO &TextIO() noexcept;
class TEXTIOSCOPE {
    TEXTIO        io;
    const TEXTIO *lpPrev;

  public:
    explicit TEXTIOSCOPE(const TEXTIO &io) noexcept;
    ~TEXTIOSCOPE() noexcept;
    TEXTIOSCOPE(const TEXTIOSCOPE &) = delete;
    TEXTIOSCOPE &operator=(const TEXTIOSCOPE &) = delete;
};
#endif
