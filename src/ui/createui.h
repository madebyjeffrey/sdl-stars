#ifndef STARS_DECOMPILED_CREATEUI_H
#define STARS_DECOMPILED_CREATEUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

void             NewGameWizard(APPSESSION &app, HWND hwnd, std::int16_t fReadOnly);
std::int16_t          FGetNewGameName(APPSESSION &app, HWND hwnd, char *szFileSuggest);
INT_PTR  NewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  NewGameDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawNewGame2(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw);
INT_PTR  NewGameDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawNewGame3(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw);
std::int16_t          FTrackNewGameDlg3(APPSESSION &app, HWND hwnd, POINT16 pt, std::int16_t kbd);
void             SetNGWTitle(APPSESSION &app, HWND hwnd, std::int16_t iStep);


#endif
