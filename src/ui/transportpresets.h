#ifndef STARS_TRANSPORT_PRESETS_H
#define STARS_TRANSPORT_PRESETS_H
#include "win.h"

struct TRANSPORTQUANTITYRESULT {
    bool fActive;
    std::int16_t iUnit;
};

void    ApplyTransportPresetPopup(APPSESSION &app, POINT16 pt);
void    ChangeTransportAmount(APPSESSION &app);
void    ChangeTransportAction(APPSESSION &app);
TRANSPORTQUANTITYRESULT FTransportQuantityActive(std::int32_t l, std::int16_t iUnit);
std::int16_t FormatTransportPresetAction(APPSESSION &app, std::int16_t i);
#endif
