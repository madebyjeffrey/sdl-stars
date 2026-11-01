#ifndef STARS_DECOMPILED_PLANETUI_H
#define STARS_DECOMPILED_PLANETUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

LRESULT  PlanetPaneWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND             CreatePlanetPane(APPSESSION &app, HWND hwndParent);
void             MovePlanetPane(APPSESSION &app, int x, int y, int dx, int dy);
void             QueuePlanetPaneLayout(APPSESSION &app);
void             ClosePlanetPane(APPSESSION &app);

LRESULT  PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FDrawTileNC(APPSESSION &app, HDC hdc, TILE *ptile, RECT *prc, char *pszTitle);
void             DrawPlanetMinSum(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStats(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStarbase(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawMassWarpGauge(APPSESSION &app, HDC hdc, RECT *prc, int16_t iBest, int16_t iCur);
void             DrawPlanetProduction(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanShipBitmap(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetShipList(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void             PlanetClick(APPSESSION &app, int16_t x, int16_t y, int16_t sks, int16_t fRightBtn);
HCURSOR          ClickInPlanetOrders(APPSESSION &app, POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn);
void             EnsureTileSize(APPSESSION &app, int16_t fSmallTiles);
void             ReflowColumn(APPSESSION &app, int16_t iCol, int16_t iTile, int16_t fRedraw);
void             HandleFocusState(APPSESSION &app, DRAWITEMSTRUCT *lpdis, int16_t inflate);
void             DrawCBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, int16_t inflate);
void             DrawProductionItem(APPSESSION &app, HDC hdc, RECT *prc, char *psz, int16_t inflate, int16_t fSelected, int16_t fListbox);
void             DrawPlanShip(APPSESSION &app, HDC hdc, TileBits grbit);
void             SetPlanetTitleBar(APPSESSION &app, HWND hwnd);
void             FillPlanetProdLB(APPSESSION &app, HWND hwnd, PLPROD *lpplprod, PLANET *lppl);

#endif
