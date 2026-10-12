#ifndef STARS_DECOMPILED_MINEUI_H
#define STARS_DECOMPILED_MINEUI_H

#include <stdint.h>
#include <windows.h>

LRESULT CALLBACK MineWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InvalidateMineralBars();
void             DrawMineSurvey(HDC hdc, RECT *prc);
HtMineType       HtMineWindow(HWND hwnd, int16_t x, int16_t y);
void             MineClick(int16_t x, int16_t y, int16_t msg, int16_t sks);
void             SetMineralTitleBar(HWND hwnd);
void             DrawSelectionArrow(HDC hdc, RECT *prc, int16_t fEnabled);
void             DrawDiamond(HDC hdc, RECT *prc, HBRUSH hbr);
void             PopupMineralScanChoices(HWND hwnd, int16_t x, int16_t y);

#endif
