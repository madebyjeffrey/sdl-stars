#ifndef STARS_DECOMPILED_UTILUI_H
#define STARS_DECOMPILED_UTILUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

void SelectOursAtObject(APPSESSION &app, POINT16 *ppt);
void DrawABunchOfStars(APPSESSION &app, HDC hdc, RECT *prc);
void DrawPlanetPrintDot(HDC hdc, std::int16_t x, std::int16_t y, std::int16_t iSize);

#endif
