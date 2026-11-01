#ifndef STARS_DECOMPILED_SHIPUI_H
#define STARS_DECOMPILED_SHIPUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

void DrawShipOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipWayPtOrders(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipPlanet(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawShipCargo(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void DrawFleetComp(APPSESSION &app, HDC hdc, TILE *ptile, OBJ obj);
void ShipCommandProc(APPSESSION &app, HWND hwnd, WPARAM wParam, LPARAM lParam);
void DrawFleetGauge(APPSESSION &app, HDC hdc, RECT *prc, FLEET *lpfl, std::int16_t grbit);
void DrawFleetBitmap(APPSESSION &app, FLEET *lpfl, HDC hdc, std::int16_t x, std::int16_t y, std::int16_t fFrame, std::int16_t ibmp, std::int16_t cDiff, std::int16_t fShrink, std::int16_t ibmpRace, std::int16_t csh);
std::int16_t          TransferStuff(APPSESSION &app, std::int16_t id1, GrobjClass grobj1, std::int16_t id2, GrobjClass grobj2, MdXfer mdXfer);
INT_PTR  TransferDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t          FTrackXfer(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t fkb);
void             UpdateXferBtns(APPSESSION &app);
void             DrawXferDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, MineralType iSupply);
void             GetXferLeftRightRcs(APPSESSION &app, RECT *prcWhole, RECT *prcLeft, RECT *prcRight);
std::int16_t          FSetupXferBtns(APPSESSION &app, RECT *prc);
void             DrawThingXferSide(APPSESSION &app, HDC hdc, RECT *prc, THING *pth, MineralType iSupply);
void             DrawFleetCargoXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawFleetShipsXferSide(APPSESSION &app, HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawPlanetXferSide(APPSESSION &app, HDC hdc, RECT *prc, PLANET *ppl, MineralType iSupply);
HCURSOR          ClickInShipOrders(APPSESSION &app, POINT16 pt, std::int16_t sks, std::int16_t fCursor, std::int16_t fRightBtn);
void             DeleteCurWayPoint(APPSESSION &app, std::int16_t fBackup);
LRESULT  FakeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             FillOrdersLB(APPSESSION &app);
void             SetOrdersLbSel(APPSESSION &app, std::int16_t iSel);
void             UpdateOrdersDDs(APPSESSION &app, std::int16_t iLevel);
void             FillBattleDD(APPSESSION &app, std::int16_t iSel);

#endif
