#ifndef STARS_DECOMPILED_RESEARCHUI_H
#define STARS_DECOMPILED_RESEARCHUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

INT_PTR  ResearchDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawResearchDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t grbitDraw);
std::int16_t          FTrackResearchDlg(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t fkb);
INT_PTR  BrowserDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT  BrowserWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DisplayComponentInfo(APPSESSION &app, HDC hdc, std::int16_t dx, std::int16_t dy, PART *ppart);

#endif
