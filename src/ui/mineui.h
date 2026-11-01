#ifndef STARS_DECOMPILED_MINEUI_H
#define STARS_DECOMPILED_MINEUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

LRESULT  MineWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InvalidateMineralBars(APPSESSION &app);
void             DrawMineSurvey(APPSESSION &app, HDC hdc, RECT *prc);
HtMineType       HtMineWindow(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y);
void             MineClick(APPSESSION &app, std::int16_t x, std::int16_t y, std::int16_t msg, std::int16_t sks);
void             SetMineralTitleBar(APPSESSION &app, HWND hwnd);
void             DrawSelectionArrow(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t fEnabled);
void             DrawDiamond(APPSESSION &app, HDC hdc, RECT *prc, HBRUSH hbr);
void             PopupMineralScanChoices(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y);

#endif
