#ifndef STARS_DECOMPILED_STARS_H
#define STARS_DECOMPILED_STARS_H

#include "win.h"
#include <cstdint>
#include <windows.h>

namespace ui { int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow); }
std::int16_t          IPlrAlsoCheater(std::int16_t iplr);
std::int16_t          FGetSystemColors(APPSESSION &app);
void             FreeStuff(APPSESSION &app) noexcept;
INT_PTR  About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t          FHandleChar(APPSESSION &app, HWND hwnd, std::uint16_t ch, std::int32_t lParam);
std::int16_t          FHandleKey(APPSESSION &app, HWND hwnd, std::int16_t iMsg, std::int16_t iKey, std::uint32_t dw);

#endif
