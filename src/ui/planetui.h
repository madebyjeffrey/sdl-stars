#ifndef STARS_DECOMPILED_PLANETUI_H
#define STARS_DECOMPILED_PLANETUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

LRESULT CALLBACK PlanetPaneWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND             CreatePlanetPane(HWND hwndParent);
void             MovePlanetPane(int x, int y, int dx, int dy);
void             QueuePlanetPaneLayout(void);
void             ClosePlanetPane(void);

LRESULT CALLBACK PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FDrawTileNC(HDC hdc, TILE *ptile, RECT *prc, char *pszTitle);
void             DrawPlanetMinSum(HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStats(HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetStarbase(HDC hdc, TILE *ptile, OBJ obj);
void             DrawMassWarpGauge(HDC hdc, RECT *prc, int16_t iBest, int16_t iCur);
void             DrawPlanetProduction(HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanShipBitmap(HDC hdc, TILE *ptile, OBJ obj);
void             DrawPlanetShipList(HDC hdc, TILE *ptile, OBJ obj);
void             PlanetClick(int16_t x, int16_t y, int16_t sks, int16_t fRightBtn);
HCURSOR          ClickInPlanetOrders(POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn);
void             EnsureTileSize(int16_t fSmallTiles);
void             ReflowColumn(int16_t iCol, int16_t iTile, int16_t fRedraw);
void             HandleFocusState(DRAWITEMSTRUCT *lpdis, int16_t inflate);
void             DrawCBEntireItem(DRAWITEMSTRUCT *lpdis, int16_t inflate);
void             DrawProductionItem(HDC hdc, RECT *prc, char *psz, int16_t inflate, int16_t fSelected, int16_t fListbox);
void             DrawPlanShip(HDC hdc, TileBits grbit);
void             SetPlanetTitleBar(HWND hwnd);
void             FillPlanetProdLB(HWND hwnd, PLPROD *lpplprod, PLANET *lppl);

#endif
