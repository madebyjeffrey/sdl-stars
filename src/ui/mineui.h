#ifndef STARS_DECOMPILED_MINEUI_H
#define STARS_DECOMPILED_MINEUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

LRESULT  MineWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InvalidateMineralBars(APPSESSION &app);
void             DrawMineSurvey(APPSESSION &app, HDC hdc, RECT *prc);
HtMineType       HtMineWindow(APPSESSION &app, HWND hwnd, int16_t x, int16_t y);
void             MineClick(APPSESSION &app, int16_t x, int16_t y, int16_t msg, int16_t sks);
void             SetMineralTitleBar(APPSESSION &app, HWND hwnd);
void             DrawSelectionArrow(APPSESSION &app, HDC hdc, RECT *prc, int16_t fEnabled);
void             DrawDiamond(APPSESSION &app, HDC hdc, RECT *prc, HBRUSH hbr);
void             PopupMineralScanChoices(APPSESSION &app, HWND hwnd, int16_t x, int16_t y);

#endif
