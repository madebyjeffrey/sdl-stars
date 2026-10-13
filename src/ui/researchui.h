#ifndef STARS_DECOMPILED_RESEARCHUI_H
#define STARS_DECOMPILED_RESEARCHUI_H

#include <stdint.h>
#include <windows.h>

INT_PTR CALLBACK ResearchDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawResearchDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t grbitDraw);
int16_t          FTrackResearchDlg(HWND hwnd, int16_t x, int16_t y, int16_t fkb);
INT_PTR CALLBACK BrowserDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK BrowserWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DisplayComponentInfo(HDC hdc, int16_t dx, int16_t dy, PART *ppart);

#endif
