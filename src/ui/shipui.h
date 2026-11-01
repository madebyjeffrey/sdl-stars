#ifndef STARS_DECOMPILED_SHIPUI_H
#define STARS_DECOMPILED_SHIPUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

void DrawShipOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipWayPtOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipPlanet(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipCargo(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawFleetComp(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void ShipCommandProc(APPSESSION &app, HWND hwnd, WPARAM wParam, LPARAM lParam);
void DrawFleetGauge(APPSESSION &app, HDC hdc, RECT *prc, FLEET *lpfl, int16_t grbit);
void DrawFleetBitmap(APPSESSION &app, FLEET *lpfl, HDC hdc, int16_t x, int16_t y, int16_t fFrame, int16_t ibmp, int16_t cDiff, int16_t fShrink, int16_t ibmpRace, int16_t csh);
int16_t          TransferStuff(APPSESSION &app, int16_t id1, GrobjClass grobj1, int16_t id2, GrobjClass grobj2, MdXfer mdXfer);
INT_PTR  TransferDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FTrackXfer(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, int16_t fkb);
void             UpdateXferBtns(APPSESSION &app);
void             DrawXferDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, MineralType iSupply);
void             GetXferLeftRightRcs(APPSESSION &app, RECT *prcWhole, RECT *prcLeft, RECT *prcRight);
int16_t          FSetupXferBtns(APPSESSION &app, RECT *prc);
void             DrawThingXferSide(APPSESSION &app, HDC hdc, RECT *prc, THING *pth, MineralType iSupply);
void             DrawFleetCargoXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawFleetShipsXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawPlanetXferSide(APPSESSION &app, HDC hdc, RECT *prc, PLANET *ppl, MineralType iSupply);
HCURSOR          ClickInShipOrders(APPSESSION &app, POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn);
void             DeleteCurWayPoint(APPSESSION &app, int16_t fBackup);
LRESULT  FakeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             FillOrdersLB(APPSESSION &app);
void             SetOrdersLbSel(APPSESSION &app, int16_t iSel);
void             UpdateOrdersDDs(APPSESSION &app, int16_t iLevel);
void             FillBattleDD(APPSESSION &app, int16_t iSel);

#endif
