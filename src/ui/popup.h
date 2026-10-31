#ifndef STARS_DECOMPILED_POPUP_H
#define STARS_DECOMPILED_POPUP_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

extern BattleUnitFlags mpimdgrbitBU[8];

LRESULT  PopupWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FIsPopupHullType(APPSESSION &app, int16_t ishdef);
void             DrawPopup(APPSESSION &app, HWND hwnd, HDC hdc);
void             Popup(APPSESSION &app, HWND hwnd, int16_t x, int16_t y);
int16_t          PopupMenu(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t cString, int32_t *rgids, const char *const *rgsz, int16_t iChecked, int16_t fRightBtn);
POINT16          PtDisplayPlanetStateInfo(APPSESSION &app, HDC hdc, int16_t fPrint);
POINT16          PtDisplayPlanetPopInfo(APPSESSION &app, HDC hdc, int16_t fPrint);
POINT16          PtDisplayZipOrdInfo(APPSESSION &app, HDC hdc, int16_t xCtr, int16_t fPrint);
POINT16          PtDisplayFactoryMineInfo(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint);
POINT16          PtDisplayResourceInfo(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint);
POINT16          PtDisplayString(APPSESSION &app, HDC hdc, int16_t dx, int16_t fPrint);

#endif
