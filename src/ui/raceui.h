#ifndef STARS_DECOMPILED_RACEUI_H
#define STARS_DECOMPILED_RACEUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

int16_t          RaceCreationWizard(APPSESSION &app, HWND hwndParent, int16_t fReadOnly, int16_t fDontWrite);
INT_PTR  RaceWizardDlg1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  RaceWizardDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawRace2(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw);
int16_t          IrcRaceDlgHitTest(APPSESSION &app, POINT16 pt);
int16_t          FTrackRaceDlg2(APPSESSION &app, HWND hwnd, POINT16 pt, int16_t kbd);
INT_PTR  RaceWizardDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawRace3(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw);
int16_t          FTrackRaceDlg3(APPSESSION &app, HWND hwnd, POINT16 pt, int16_t kbd);
INT_PTR  RaceWizardDlg4(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  RaceWizardDlg5(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  RaceWizardDlg6(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InvalidateAdvPtsRect(APPSESSION &app, HWND hwnd);
void             DrawRaceAdvantagePoints(APPSESSION &app, HDC hdc, RECT *prc, PLAYER *pplr);
int16_t          FSaveRace(APPSESSION &app, char *szFileSuggest, PLAYER *pplr);
void             SetSzRaceFile(APPSESSION &app, const char *pszFile);
void             SetRCWTitle(APPSESSION &app, HWND hwnd, int16_t iStep);

#endif
