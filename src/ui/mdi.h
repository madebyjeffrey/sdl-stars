#ifndef STARS_DECOMPILED_MDI_H
#define STARS_DECOMPILED_MDI_H

#include <stdint.h>
#include <windows.h>

extern char rgTOWidth[2][2];

int16_t          InitMDIApp();
LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
POINT16          InvertPaneBorder(HDC hdc, PaneSplitter grSel, POINT16 dpt, POINT16 *pdptPrev);
HCURSOR          HcrsFromFrameWindowPt(POINT16 pt, int16_t *pgrSel);
void             RestoreSelection();
void             CommandHandler(HWND hwnd, WPARAM wParam);
void             InitializeMenu(HMENU hmenu);
HMENU            GetASubMenu(HWND hwnd, MainMenu iMenu);
int16_t          FOpenGame(HWND hwnd, int16_t fRaceOnly);
void             BringUpHostDlg();
void             DrawHostDialog2(HWND hwnd, HDC hdcIn);
INT_PTR CALLBACK HostModeDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HostOptionsDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawHostOptions(HWND hwnd, HDC hdc, int16_t iDraw);
VOID CALLBACK    HostTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);
void             GetWindowRc(HWND hwnd, RECT *prc);
void             SetWindowIniString(char *sz, HWND hwnd);
void             WriteIniSettings();
void             RefitFrameChildren();
LRESULT CALLBACK TitleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
