#ifndef STARS_DECOMPILED_RACEUI_H
#define STARS_DECOMPILED_RACEUI_H

#include <stdint.h>
#include <windows.h>

int16_t          RaceCreationWizard(HWND hwndParent, int16_t fReadOnly, int16_t fDontWrite);
INT_PTR CALLBACK RaceWizardDlg1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RaceWizardDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawRace2(HWND hwnd, HDC hdc, int16_t iDraw);
int16_t          IrcRaceDlgHitTest(POINT16 pt);
int16_t          FTrackRaceDlg2(HWND hwnd, POINT16 pt, int16_t kbd);
INT_PTR CALLBACK RaceWizardDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawRace3(HWND hwnd, HDC hdc, int16_t iDraw);
int16_t          FTrackRaceDlg3(HWND hwnd, POINT16 pt, int16_t kbd);
INT_PTR CALLBACK RaceWizardDlg4(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RaceWizardDlg5(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RaceWizardDlg6(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InvalidateAdvPtsRect(HWND hwnd);
void             DrawRaceAdvantagePoints(HDC hdc, RECT *prc, PLAYER *pplr);
int16_t          FSaveRace(char *szFileSuggest, PLAYER *pplr);
void             SetSzRaceFile(const char *pszFile);
void             SetRCWTitle(HWND hwnd, int16_t iStep);

#endif
