#ifndef STARS_TRANSPORT_PRESETS_H
#define STARS_TRANSPORT_PRESETS_H
#include "win.h"
void    ApplyTransportPresetPopup(POINT16 pt);
void    ChangeTransportAmount();
void    ChangeTransportAction();
int16_t FTransportQuantityActive(int32_t l, int16_t *pi);
void    FormatTransportPresetAction(int16_t i);
#endif
