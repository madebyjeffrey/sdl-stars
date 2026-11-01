#ifndef STARS_DECOMPILED_SHIP2UI_H
#define STARS_DECOMPILED_SHIP2UI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

INT_PTR  ZipOrderDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             EnableZipBtns(APPSESSION &app, HWND hwnd, int16_t iSel);
INT_PTR  RenameZipDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  RenameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  MergeFleetsDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
