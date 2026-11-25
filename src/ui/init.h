#ifndef STARS_DECOMPILED_INIT_H
#define STARS_DECOMPILED_INIT_H

#include "win.h"
#include <cstdint>
#include <span>
#include <string_view>
#include <windows.h>

extern std::uint8_t rgPalGray[20];

std::int16_t FCreateStuff(APPSESSION &app);
std::int16_t FCreateFonts(APPSESSION &app, HDC hdc);
std::int16_t InitInstance(APPSESSION &app, std::int16_t nCmdShow);
void    InitTiles(APPSESSION &app);
WN      GetIniWinRc(APPSESSION &app, std::string_view szSection, std::string_view szIniFile, StringId ids);
void    ReadIniSettings(APPSESSION &app);
void    ReadIniTileSettings(std::string_view szFormat, std::span<TILE> rgtile);


#endif
