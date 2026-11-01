#ifndef STARS_DECOMPILED_SCAN_H
#define STARS_DECOMPILED_SCAN_H

#include "win.h"
#include <cstdint>
#include <windows.h>

extern std::uint32_t rgcrScanMine[3];
extern std::int16_t  vrgPopRad[19];

LRESULT  ScannerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
std::int16_t          DrawScanner(APPSESSION &app, HDC hdc, RECT *prc);
void             DrawScanFleetCount(APPSESSION &app, FLEET *lpfl, std::int16_t x, std::int16_t y, HDC hdc, HDC hdcMem);
std::int32_t          CShipsScanVis(APPSESSION &app, FLEET *lpfl);
void             DrawRadarCircle(DRAWCIR *pdc, RECT *prc);
void             SetScanPathWarp(APPSESSION &app, std::int16_t iwp, std::int16_t iWarp);
void             DrawShipScanPath(APPSESSION &app, HDC hdc, std::int16_t fShow);
void             DrawScannerSBar(APPSESSION &app, HDC hdc, RECT *prc, SBAR *psbar, std::int16_t fFullRedraw);
void             DrawLockLight(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t fFullRedraw);
void             SetScanScrollBars(APPSESSION &app, HWND hwnd);
void             ScrollScanner(APPSESSION &app, std::int16_t dx, std::int16_t dy);
void             RedrawScanSel(APPSESSION &app, HDC hdc, std::int16_t fVis);
std::int16_t          FEnsurePointOnScreen(APPSESSION &app, POINT16 pt, std::int16_t fScroll);
void             CtrPointScan(APPSESSION &app, POINT16 pt, std::int16_t fScroll);
void             ZoomScanAt(APPSESSION &app, ScanZoom iScanNew, POINT16 ptScan);
void             LogicalToScan(APPSESSION &app, POINT16 *ppt);
void             ScanToLogical(APPSESSION &app, POINT16 *ppt);
std::int16_t          FAddWayPoint(APPSESSION &app, POINT16 ptIn, SCAN *pscan);
std::int16_t          FNearAWayPoint(APPSESSION &app, POINT16 pt, std::int16_t fLogical);
std::int16_t          FHandleWayPointDrag(APPSESSION &app, POINT16 pt);
void             DrawScanXorLines(APPSESSION &app, HDC hdc, POINT16 *rgpt, std::int16_t cpt);
std::int16_t          SetScanWp(APPSESSION &app, std::int16_t iNew);
std::int16_t          FGetNextObjHere(APPSESSION &app, SCAN *pscan, std::int16_t fOnlyOurs);
INT_PTR  FindDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
std::int16_t          FSelectSz(APPSESSION &app, char *szName);
void             GetScanFleetOrientation(APPSESSION &app, FLEET *lpfl, POINT16 *ppt, POINT16 *pptD);
void             GetDxDyOrientation(APPSESSION &app, std::int16_t dx, std::int16_t dy, POINT16 *ppt, POINT16 *pptD);
std::int16_t          FHandleMeasuringTape(APPSESSION &app, SCAN *pscan, POINT16 pt);

#endif
