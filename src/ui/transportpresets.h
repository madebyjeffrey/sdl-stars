#ifndef STARS_TRANSPORT_PRESETS_H
#define STARS_TRANSPORT_PRESETS_H
#include "win.h"
void    ApplyTransportPresetPopup(APPSESSION &app, POINT16 pt);
void    ChangeTransportAmount(APPSESSION &app);
void    ChangeTransportAction(APPSESSION &app);
std::int16_t FTransportQuantityActive(std::int32_t l, std::int16_t *pi);
void    FormatTransportPresetAction(APPSESSION &app, std::int16_t i);
#endif
