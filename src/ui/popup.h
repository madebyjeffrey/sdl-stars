#ifndef STARS_DECOMPILED_POPUP_H
#define STARS_DECOMPILED_POPUP_H

#include "win.h"
#include <cstdint>
#include <windows.h>

extern BattleUnitFlags mpimdgrbitBU[8];

LRESULT  PopupWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t          FIsPopupHullType(APPSESSION &app, std::int16_t ishdef);
void             DrawPopup(APPSESSION &app, HWND hwnd, HDC hdc);
void             Popup(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y);
std::int16_t          PopupMenu(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t cString, std::int32_t *rgids, const char *const *rgsz, std::int16_t iChecked, std::int16_t fRightBtn);
POINT16          PtDisplayPlanetStateInfo(APPSESSION &app, HDC hdc, std::int16_t fPrint);
POINT16          PtDisplayPlanetPopInfo(APPSESSION &app, HDC hdc, std::int16_t fPrint);
POINT16          PtDisplayZipOrdInfo(APPSESSION &app, HDC hdc, std::int16_t xCtr, std::int16_t fPrint);
POINT16          PtDisplayFactoryMineInfo(APPSESSION &app, HDC hdc, std::int16_t dx, std::int16_t fPrint);
POINT16          PtDisplayResourceInfo(APPSESSION &app, HDC hdc, std::int16_t dx, std::int16_t fPrint);
POINT16          PtDisplayString(APPSESSION &app, HDC hdc, std::int16_t dx, std::int16_t fPrint);

#endif
