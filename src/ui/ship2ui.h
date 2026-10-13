#ifndef STARS_DECOMPILED_SHIP2UI_H
#define STARS_DECOMPILED_SHIP2UI_H

#include <stdint.h>
#include <windows.h>

INT_PTR CALLBACK ZipOrderDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             EnableZipBtns(HWND hwnd, int16_t iSel);
INT_PTR CALLBACK RenameZipDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RenameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MergeFleetsDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
