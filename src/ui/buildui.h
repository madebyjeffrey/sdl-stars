#ifndef STARS_DECOMPILED_BUILDUI_H
#define STARS_DECOMPILED_BUILDUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

int16_t          ShipBuilder(APPSESSION &app, POINT16 ptDlgSize);
void             ShowMainControls(APPSESSION &app, HWND hwnd, int16_t sw);
int16_t          FCheckQueuedShip(APPSESSION &app, HWND hwnd, SHDEF *lpshdef, int16_t fEdit);
INT_PTR  SlotDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawSlotDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, int16_t iDraw);
int16_t          FTrackSlot(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t fkb, int16_t fListBox, int16_t fRightBtn);
void             DrawBuildSelComp(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw);
void             DrawBuildSelHull(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw, RECT *prc);
void             SetBuildSelection(APPSESSION &app, int16_t iSrc);
int16_t          IDropPart(APPSESSION &app, POINT16 pt, HS hsSrc, int16_t iSrc, int16_t fNoModify);
void             DrawDlgLBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, int16_t inflate);
void             FillBuildDD(APPSESSION &app, HWND hwndDD, MdBuild md);
void             FillBuildPartsLB(APPSESSION &app, HWND hwndLB, int16_t grbit);
LRESULT  FakeListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             UpdateSlotGlobals(APPSESSION &app);
int16_t          IEmptyBmpFromGrhst(APPSESSION &app, int16_t grhst);
void             MakeNewName(APPSESSION &app, char *lpsz);

#endif
