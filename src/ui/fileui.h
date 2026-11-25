#ifndef STARS_DECOMPILED_FILEUI_H
#define STARS_DECOMPILED_FILEUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

INT_PTR  AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t FCanLeaveCurrentGame(APPSESSION &app);


#endif
