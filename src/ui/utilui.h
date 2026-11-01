#ifndef STARS_DECOMPILED_UTILUI_H
#define STARS_DECOMPILED_UTILUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

void SelectOursAtObject(APPSESSION &app, POINT16 *ppt);
void DrawABunchOfStars(APPSESSION &app, HDC hdc, RECT *prc);
void DrawPlanetPrintDot(HDC hdc, int16_t x, int16_t y, int16_t iSize);

#endif
