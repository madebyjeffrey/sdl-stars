#ifndef STARS_DECOMPILED_RESEARCHUI_H
#define STARS_DECOMPILED_RESEARCHUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

INT_PTR  ResearchDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawResearchDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, int16_t grbitDraw);
int16_t          FTrackResearchDlg(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t fkb);
INT_PTR  BrowserDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT  BrowserWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DisplayComponentInfo(APPSESSION &app, HDC hdc, int16_t dx, int16_t dy, PART *ppart);

#endif
