#ifndef STARS_DECOMPILED_PRODUCEUI_H
#define STARS_DECOMPILED_PRODUCEUI_H

#include <stdint.h>
#include <windows.h>

int16_t          ChangeProduction(int16_t fClear);
INT_PTR CALLBACK ProductionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ProdCommandHandler(HWND hwnd, WPARAM wParam, LPARAM lParam);
void             InitializeProductionDlg(HWND hwnd);
void             DrawProductionDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t iDraw);
void             FillProdSrcLB(HWND hwndLB, int16_t mdFill);
INT_PTR CALLBACK ZipProdDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             EnableZipProdBtns(HWND hwnd, int16_t iSel);
void             FillZipProdLB(HWND hwndDlg, ZIPPRODQ *pzpq);

#endif
