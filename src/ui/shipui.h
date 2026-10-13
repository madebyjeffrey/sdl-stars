#ifndef STARS_DECOMPILED_SHIPUI_H
#define STARS_DECOMPILED_SHIPUI_H

#include <stdint.h>
#include <windows.h>

void DrawShipOrders(HDC hdc, TILE *ptile, OBJ obj);
void DrawShipWayPtOrders(HDC hdc, TILE *ptile, OBJ obj);
void DrawShipPlanet(HDC hdc, TILE *ptile, OBJ obj);
void DrawShipCargo(HDC hdc, TILE *ptile, OBJ obj);
void DrawFleetComp(HDC hdc, TILE *ptile, OBJ obj);
void ShipCommandProc(HWND hwnd, WPARAM wParam, LPARAM lParam);
void DrawFleetGauge(HDC hdc, RECT *prc, FLEET *lpfl, int16_t grbit);
void DrawFleetBitmap(FLEET *lpfl, HDC hdc, int16_t x, int16_t y, int16_t fFrame, int16_t ibmp, int16_t cDiff, int16_t fShrink, int16_t ibmpRace, int16_t csh);
int16_t          TransferStuff(int16_t id1, GrobjClass grobj1, int16_t id2, GrobjClass grobj2, MdXfer mdXfer);
INT_PTR CALLBACK TransferDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FTrackXfer(HWND hwnd, int16_t x, int16_t y, int16_t fkb);
void             UpdateXferBtns();
void             DrawXferDlg(HWND hwnd, HDC hdc, RECT *prc, MineralType iSupply);
void             GetXferLeftRightRcs(RECT *prcWhole, RECT *prcLeft, RECT *prcRight);
int16_t          FSetupXferBtns(RECT *prc);
void             DrawThingXferSide(HDC hdc, RECT *prc, THING *pth, MineralType iSupply);
void             DrawFleetCargoXferSide(HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawFleetShipsXferSide(HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply);
void             DrawPlanetXferSide(HDC hdc, RECT *prc, PLANET *ppl, MineralType iSupply);
HCURSOR          ClickInShipOrders(POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn);
void             DeleteCurWayPoint(int16_t fBackup);
LRESULT CALLBACK FakeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             FillOrdersLB();
void             SetOrdersLbSel(int16_t iSel);
void             UpdateOrdersDDs(int16_t iLevel);
void             FillBattleDD(int16_t iSel);

#endif
