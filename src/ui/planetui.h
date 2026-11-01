#ifndef STARS_DECOMPILED_PLANETUI_H
#define STARS_DECOMPILED_PLANETUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

LRESULT  PlanetPaneWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND             CreatePlanetPane(APPSESSION &app, HWND hwndParent);
void             MovePlanetPane(APPSESSION &app, int x, int y, int dx, int dy);
void             QueuePlanetPaneLayout(APPSESSION &app);
void             ClosePlanetPane(APPSESSION &app);

LRESULT  PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t          FDrawTileNC(APPSESSION &app, HDC hdc, TILE *ptile, RECT *prc, char *pszTitle);
void             DrawPlanetMinSum(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStats(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStarbase(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawMassWarpGauge(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t iBest, std::int16_t iCur);
void             DrawPlanetProduction(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanShipBitmap(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetShipList(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             PlanetClick(APPSESSION &app, std::int16_t x, std::int16_t y, std::int16_t sks, std::int16_t fRightBtn);
HCURSOR          ClickInPlanetOrders(APPSESSION &app, POINT16 pt, std::int16_t sks, std::int16_t fCursor, std::int16_t fRightBtn);
void             EnsureTileSize(APPSESSION &app, std::int16_t fSmallTiles);
void             ReflowColumn(APPSESSION &app, std::int16_t iCol, std::int16_t iTile, std::int16_t fRedraw);
void             HandleFocusState(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate);
void             DrawCBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate);
void             DrawProductionItem(APPSESSION &app, HDC hdc, RECT *prc, char *psz, std::int16_t inflate, std::int16_t fSelected, std::int16_t fListbox);
void             DrawPlanShip(APPSESSION &app, HDC hdc, TileBits grbit);
void             SetPlanetTitleBar(APPSESSION &app, HWND hwnd);
void             FillPlanetProdLB(APPSESSION &app, HWND hwnd, PLPROD *lpplprod, PLANET *lppl);

#endif
