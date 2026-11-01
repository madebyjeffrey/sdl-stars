#ifndef STARS_TRANSPORT_PRESETS_H
#define STARS_TRANSPORT_PRESETS_H
#include "win.h"
void    ApplyTransportPresetPopup(APPSESSION &app, POINT16 pt);
void    ChangeTransportAmount(APPSESSION &app);
void    ChangeTransportAction(APPSESSION &app);
int16_t FTransportQuantityActive(int32_t l, int16_t *pi);
void    FormatTransportPresetAction(APPSESSION &app, int16_t i);
#endif
