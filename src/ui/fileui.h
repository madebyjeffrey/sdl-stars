#ifndef STARS_DECOMPILED_FILEUI_H
#define STARS_DECOMPILED_FILEUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

INT_PTR CALLBACK AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t FCanLeaveCurrentGame(void);

decltype(AskSaveDialog) STARS_TEST_IMPL(AskSaveDialog);

#endif
