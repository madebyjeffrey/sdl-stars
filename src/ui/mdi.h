#ifndef STARS_DECOMPILED_MDI_H
#define STARS_DECOMPILED_MDI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

extern char rgTOWidth[2][2];

struct FRAMEWINDOWPOINTRESULT {
    HCURSOR hcs;
    std::int16_t grSel;
};

std::int16_t          InitMDIApp(APPSESSION &app);
LRESULT  FrameWndProc(APPSESSION &app, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
POINT16          InvertPaneBorder(APPSESSION &app, HDC hdc, PaneSplitter grSel, POINT16 dpt, POINT16 *pdptPrev);
FRAMEWINDOWPOINTRESULT HcrsFromFrameWindowPt(APPSESSION &app, POINT16 pt);
void             RestoreSelection(APPSESSION &app);
void             CommandHandler(APPSESSION &app, HWND hwnd, WPARAM wParam);
void             InitializeMenu(APPSESSION &app, HMENU hmenu);
HMENU            GetASubMenu(APPSESSION &app, HWND hwnd, MainMenu iMenu);
std::int16_t          FOpenGame(APPSESSION &app, HWND hwnd, std::int16_t fRaceOnly);
void             BringUpHostDlg(APPSESSION &app);
void             DrawHostDialog2(APPSESSION &app, HWND hwnd, HDC hdcIn);
INT_PTR  HostModeDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  HostOptionsDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawHostOptions(HWND hwnd, HDC hdc, std::int16_t iDraw);
void     HostTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);
RECT             GetWindowRc(HWND hwnd);
void             SetWindowIniString(APPSESSION &app, HWND hwnd);
void             WriteIniSettings(APPSESSION &app);
void             RefitFrameChildren(APPSESSION &app);
LRESULT  TitleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
