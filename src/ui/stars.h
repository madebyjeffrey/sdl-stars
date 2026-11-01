#ifndef STARS_DECOMPILED_STARS_H
#define STARS_DECOMPILED_STARS_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

namespace ui { int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow); }
int16_t          IPlrAlsoCheater(int16_t iplr);
int16_t          FGetSystemColors(APPSESSION &app);
void             FreeStuff(APPSESSION &app) noexcept;
INT_PTR  About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int16_t          FHandleChar(APPSESSION &app, HWND hwnd, uint16_t ch, int32_t lParam);
int16_t          FHandleKey(APPSESSION &app, HWND hwnd, int16_t iMsg, int16_t iKey, uint32_t dw);

#endif
