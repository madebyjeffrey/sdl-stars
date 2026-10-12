#ifndef STARS_DECOMPILED_CREATEUI_H
#define STARS_DECOMPILED_CREATEUI_H

#include <stdint.h>
#include <windows.h>

void             NewGameWizard(HWND hwnd, int16_t fReadOnly);
int16_t          FGetNewGameName(char *szFileSuggest);
INT_PTR CALLBACK SimpleNewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewGameDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawNewGame2(HWND hwnd, HDC hdc, int16_t iDraw);
INT_PTR CALLBACK NewGameDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawNewGame3(HWND hwnd, HDC hdc, int16_t iDraw);
int16_t          FTrackNewGameDlg3(HWND hwnd, POINT16 pt, int16_t kbd);
void             SetNGWTitle(HWND hwnd, int16_t iStep);

#endif
