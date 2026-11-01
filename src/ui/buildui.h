#ifndef STARS_DECOMPILED_BUILDUI_H
#define STARS_DECOMPILED_BUILDUI_H

#include "win.h"
#include <cstdint>
#include <span>
#include <windows.h>

std::int16_t          ShipBuilder(APPSESSION &app, POINT16 ptDlgSize);
void             ShowMainControls(APPSESSION &app, HWND hwnd, std::int16_t sw);
std::int16_t          FCheckQueuedShip(APPSESSION &app, HWND hwnd, SHDEF *lpshdef, std::int16_t fEdit);
INT_PTR  SlotDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawSlotDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t iDraw);
std::int16_t          FTrackSlot(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t fkb, std::int16_t fListBox, std::int16_t fRightBtn);
void             DrawBuildSelComp(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw);
void             DrawBuildSelHull(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw, RECT *prc);
void             SetBuildSelection(APPSESSION &app, std::int16_t iSrc);
std::int16_t          IDropPart(APPSESSION &app, POINT16 pt, HS hsSrc, std::int16_t iSrc, std::int16_t fNoModify);
void             DrawDlgLBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate);
void             FillBuildDD(APPSESSION &app, HWND hwndDD, MdBuild md);
void             FillBuildPartsLB(APPSESSION &app, HWND hwndLB, std::int16_t grbit);
LRESULT  FakeListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             UpdateSlotGlobals(APPSESSION &app);
std::int16_t          IEmptyBmpFromGrhst(APPSESSION &app, std::int16_t grhst);
void             MakeNewName(APPSESSION &app, std::span<char, 32> lpsz);

#endif
