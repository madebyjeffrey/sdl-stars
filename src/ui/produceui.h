#ifndef STARS_DECOMPILED_PRODUCEUI_H
#define STARS_DECOMPILED_PRODUCEUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

std::int16_t          ChangeProduction(APPSESSION &app, std::int16_t fClear);
INT_PTR  ProductionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ProdCommandHandler(APPSESSION &app, HWND hwnd, WPARAM wParam, LPARAM lParam);
void             InitializeProductionDlg(APPSESSION &app, HWND hwnd);
void             DrawProductionDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t iDraw);
void             FillProdSrcLB(APPSESSION &app, HWND hwndLB, std::int16_t mdFill);
INT_PTR  ZipProdDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             EnableZipProdBtns(APPSESSION &app, HWND hwnd, std::int16_t iSel);
void             FillZipProdLB(APPSESSION &app, HWND hwndDlg, ZIPPRODQ *pzpq);

#endif
