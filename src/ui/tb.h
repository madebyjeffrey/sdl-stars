#ifndef STARS_DECOMPILED_TB_H
#define STARS_DECOMPILED_TB_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

inline constexpr auto IDC_STARS_TOOLBAR = 16000;
inline constexpr auto IDC_STARS_TOOLBUTTON_FIRST = 16100;

extern char    vrgTBBtn[29];
extern int16_t vrgpctZoom[9];

LRESULT  TbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             DrawToolbar(APPSESSION &app, HDC hdc, RECT *prc);
void             DrawBitmapButton(APPSESSION &app, HDC hdc, POINT16 pt, ToolbarButton ibtn, int16_t fDown);
int16_t          ItbFromPpt(APPSESSION &app, POINT16 *ppt);
int16_t          DxOfBtn(APPSESSION &app, ToolbarButton itb);
int16_t          FIsButtonDown(APPSESSION &app, ToolbarButton itb);
void             ExecuteButton(APPSESSION &app, ToolbarButton itb, int16_t fDown);
void             TerminateToolbarFocus(APPSESSION &app, int16_t fCancel);
void             ShowTooltip(APPSESSION &app, StringId ids, RECT *prc);
LRESULT  TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT  FakeComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT  FakeCEProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
